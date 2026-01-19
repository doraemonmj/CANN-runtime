# 05-aicpu-multithread - AICPU Multi-core Parallel Execution

Demonstrates parallel kernel execution across multiple AICPU cores using the eval model.

## ⚠️ Educational Example

**IMPORTANT**: This example uses AICPU for computation to teach threading concepts.

**Production Reality**:
- **AICPU role**: Controls AICore, handles dynamic shapes, complex control flow
- **AICore role**: Performs actual computation (matrix ops, SIMD operations)
- **Hardware limit**: Maximum **4 AICPU cores** available on Ascend 910
- **For production compute**: Use AICore examples (09-18)

**What this teaches**: Eval model threading architecture, `allocThreadIdx()` mechanism, multi-core work partitioning.

## Concept

This example shows how to execute AICPU kernels in parallel across 4 ARM Cortex-A55 cores (hardware maximum). It demonstrates the **eval model** execution pattern used by AICPU, which differs fundamentally from traditional pthread-based threading.

## Hardware Behavior

**AICPU Multi-core Architecture:**
- 4 ARM Cortex-A55 cores used (hardware maximum for scheduling)
- 8 total cores on Ascend 910, organized in 2 clusters (4 cores per cluster)
- Shared HBM access - all cores see same physical address space
- Hardware-managed cache coherency
- Each core can execute full C/C++ code independently

**Memory Model:**
```
Host CPU (x86)
  ↓ PCIe (~3μs DMA transfer)
Device HBM ←─ All 4 AICPU cores directly read/write
          └─ AICore also reads/writes (shared memory space)
```

All AICPU cores share the same HBM address space - no explicit data movement needed between cores.

## Eval Model vs Pthread Model

This is the key concept distinguishing AICPU from traditional CPU threading:

### Pthread Model (Traditional)
```
Parent Thread (CPU)
  ├─ pthread_create(worker1) → runs func()
  ├─ pthread_create(worker2) → runs func()
  └─ pthread_create(worker3) → runs func()

Each thread:
- Spawned dynamically by parent
- Gets ID via Thread-Local Storage (TLS)
- Can run different code paths
```

### Eval Model (AICPU Hardware)
```
Runtime loads .so once into shared memory
Runtime directs 4 physical CPU cores → execute same entry point

All cores simultaneously:
  1. Execute same entry point function
  2. Get thread ID from allocThreadIdx() (hardware mechanism)
  3. Thread ID based on physical CPU assignment + cluster
```

**Key Differences:**

| Aspect | Pthread Model | AICPU Eval Model |
|--------|--------------|------------------|
| Thread creation | `pthread_create()` call | Hardware directs cores to entry point |
| Thread ID source | Software TLS | Hardware CPU affinity (`sched_getcpu()`) |
| Entry points | Can differ per thread | All cores execute same function |
| Coordination | Parent/child relationship | Peer cores with barrier sync |

### Hardware Thread Indexing: allocThreadIdx()

The CANN runtime assigns thread IDs using the `allocThreadIdx()` mechanism:

```cpp
// Simplified from CANN internal implementation
int allocThreadIdx(int nrAicpu) {
    int cpu = sched_getcpu();              // Get physical CPU core ID
    cpumask.fetch_or(1 << cpu, ...);       // Register this CPU atomically

    // Barrier: wait for all nrAicpu cores to register
    while (__builtin_popcount(cpumask) != nrAicpu) {
        sched_yield();
    }

    // Assign sequential thread ID based on CPU cluster membership
    // Clusters = groups of 4 cores (CPUS_PER_CLUSTER = 4)
    return calculate_thread_idx_from_cluster(cpu);
}
```

**Characteristics:**
- **Barrier synchronization**: All threads wait until all cores have registered
- **Cluster-aware**: Groups threads by 4-core clusters (hardware layout)
- **CPU affinity**: Thread ID tied to which physical core is executing
- **Atomic coordination**: Uses atomic `cpumask` for registration

### This Example's Approach

For simplicity, this example uses **pthread TLS** as a software abstraction over the eval model:

```cpp
// In kernel code - software abstraction
static pthread_key_t g_thread_id_key;
static int g_next_thread_id = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_thread_id() {
    void* id = pthread_getspecific(g_thread_id_key);
    if (id == nullptr) {
        pthread_mutex_lock(&g_mutex);
        int my_id = g_next_thread_id++;
        pthread_mutex_unlock(&g_mutex);
        pthread_setspecific(g_thread_id_key, (void*)(intptr_t)my_id);
        return my_id;
    }
    return (int)(intptr_t)id;
}
```

**Why use pthread TLS?**
- ✅ Cleaner abstraction for application code
- ✅ Portable and well-understood pattern
- ✅ CANN runtime has already coordinated cores via `allocThreadIdx()`

**What's happening underneath:**
1. Runtime loads [libparallel_add_aicpu_kernel.so](kernel/libparallel_add_aicpu_kernel.so) into shared memory
2. Runtime directs 4 physical CPU cores → execute `DynTileFwkBackendKernelServer`
3. Each core gets hardware thread ID via `allocThreadIdx()` mechanism
4. Cores use pthread TLS to manage sequential IDs (0-3) in application layer

## Code Structure

- [main.cpp](main.cpp) - Host launcher configuring `aicpuNum=4`
- [kernel/parallel_add_kernel.cpp](kernel/parallel_add_kernel.cpp) - Multi-threaded kernel
- [kernel/CMakeLists.txt](kernel/CMakeLists.txt) - Kernel build configuration

## Key CANN APIs

**Device Initialization:**
- `rtSetDevice()` - Select NPU device (see [main.cpp:213](main.cpp#L213))
- `rtStreamCreate()` - Create command queue (see [main.cpp:222](main.cpp#L222))

**Memory Management:**
- `rtMalloc()` - Allocate HBM for inputs/outputs (see [main.cpp:249](main.cpp#L249), [main.cpp:260](main.cpp#L260), [main.cpp:272](main.cpp#L272))
- `rtMemcpy()` - Transfer data host↔device (see [main.cpp:288](main.cpp#L288), [main.cpp:303](main.cpp#L303))

**Kernel Loading:**
- `AicpuSoInfo::Init()` - Load .so to HBM (see [main.cpp:322](main.cpp#L322))
- Loads compiled ARM64 binary into device memory for execution

**Multi-core Launch:**
- `LaunchAiCpuKernel()` - Launch with `aicpuNum=4` (see [main.cpp:440](main.cpp#L440))
```cpp
LaunchAiCpuKernel(stream, &kernelArgs, "DynTileFwkKernelServer", 4);
//                                                               ^^^
//                                      aicpuNum=4: Use 4 cores (max)
```

**Synchronization:**
- `rtStreamSynchronize()` - Wait for kernels to complete (see [main.cpp:458](main.cpp#L458))

## Thread ID and Work Partitioning

**Getting Thread ID:**

See [get_thread_id()](kernel/parallel_add_kernel.cpp#L84):
```cpp
int thread_id = get_thread_id();  // Returns 0-3 for 4 cores
```

**Work Partitioning Strategy:**

See [work distribution](kernel/parallel_add_kernel.cpp#L188-L197):
```cpp
int32_t chunk_size = n / num_threads;
int32_t start = thread_id * chunk_size;
int32_t end = (thread_id == num_threads - 1) ? n : start + chunk_size;

// Process this core's chunk
for (int32_t i = start; i < end; i++) {
    out[i] = in_a[i] + in_b[i];
}
```

**Characteristics:**
- **Contiguous chunks**: Each core processes sequential elements (better cache locality)
- **Disjoint memory**: No overlapping writes - no synchronization needed
- **Load balancing**: Last thread handles remainder to avoid rounding issues

**Alternative Strategy (Strided Access):**
```cpp
// Less cache-friendly but simpler load balancing
for (int32_t i = thread_id; i < n; i += num_threads) {
    out[i] = in_a[i] + in_b[i];
}
```

Contiguous chunks are usually faster due to cache line effects.

## Backend Server Pattern

AICPU kernels use the **backend server pattern** (same as example 04):

**System Kernel**: `libaicpu_extend_kernels.so` (CANN-provided)
- Acts as intermediary between runtime and user kernel
- Expects specific function names in our .so

**Our Backend Server**: `libparallel_add_aicpu_kernel.so`
- Must export these entry points:
  - `DynTileFwkBackendKernelServerInit` - Initialization (see [kernel/parallel_add_kernel.cpp:119](kernel/parallel_add_kernel.cpp#L119))
  - `DynTileFwkBackendKernelServer` - Main execution (see [kernel/parallel_add_kernel.cpp:146](kernel/parallel_add_kernel.cpp#L146))

**Two-Phase Launch:**

1. **Init Phase** (aicpuNum=1): See [main.cpp:414](main.cpp#L414))
   - Runs on single core
   - Extracts custom args pointer from DeviceArgs
   - Stores in global variable for main phase

2. **Execution Phase** (aicpuNum=4): See [main.cpp:440](main.cpp#L440))
   - Runs on 4 cores in parallel (hardware maximum)
   - Each core executes same function
   - Uses stored args pointer from init phase

## Build and Run

```bash
# Set up environment
source /usr/local/Ascend/ascend-toolkit/latest/bin/setenv.bash

# Build kernel (ARM64 .so)
cd kernel
mkdir build && cd build
cmake ..
make  # Produces libparallel_add_aicpu_kernel.so

# Build host launcher
cd ../..
mkdir build && cd build
cmake ..
make
./05-aicpu-multithread
```

## Expected Output

```
=== AICPU Multi-threaded Kernel Example ===
Using 4 AICPU cores in parallel (hardware maximum)
EDUCATIONAL: Production uses AICore for compute

PASS: All 1024 elements correct (4 cores)
  Input_A[0]  = 0, Input_B[0]  = 0  →  Output[0]  = 0
  Input_A[N-1] = 1023, Input_B[N-1] = 511.5  →  Output[N-1] = 1534.5

=== End of AICPU Multi-threaded Example ===
```

## Performance Considerations

**Parallelism:**
- Theoretical: 4x speedup with 4 cores
- Practical: Limited by memory bandwidth, cache effects, work distribution overhead

**Memory Bandwidth:**
- All cores share HBM bandwidth
- Memory-bound operations won't scale linearly
- Compute-bound operations scale better

**Cache Effects:**
- Each core has L1/L2 caches
- Contiguous chunk assignment improves cache hit rates
- Strided access can cause cache thrashing

**When Multi-core Helps:**
- ✅ Large datasets (> 1MB per core)
- ✅ Compute-intensive operations (math, transformations)
- ✅ Embarrassingly parallel workloads (no inter-thread communication)

**When Single-core is Better:**
- ❌ Small datasets (< 100KB total)
- ❌ Memory-bandwidth limited operations
- ❌ Complex inter-thread synchronization required

## What You Learned

✅ **Eval Model Execution:**
- AICPU uses eval model, not pthread spawning
- All cores execute same entry point simultaneously
- Thread ID from hardware mechanism (`allocThreadIdx`), not just software TLS
- Cluster-aware CPU assignment (4 cores per cluster)
- **Maximum 4 AICPU cores** for scheduling

✅ **Hardware Threading:**
- Barrier synchronization via atomic CPU mask
- Physical CPU core affinity determines thread assignment
- No parent/child thread relationship - peer cores

✅ **Work Partitioning:**
- Divide data into contiguous chunks for cache efficiency
- Disjoint memory regions eliminate synchronization overhead
- Last thread handles remainder for correct load balancing

✅ **Launch Configuration:**
- `aicpuNum` parameter controls core count (max 4 for AICPU)
- Init phase (aicpuNum=1) for setup
- Execution phase (aicpuNum=4) for parallel work

✅ **Production vs Educational:**
- AICPU: Control/coordination role in production
- AICore: Actual computation in production (examples 09-18)
- This example: Teaches eval model concepts using simple compute

## Next Steps

- [06-aicpu-logging](../06-aicpu-logging/) - Debug output and error handling in AICPU kernels
- [07-aicpu-atomic](../07-aicpu-atomic/) - Multi-core synchronization using atomic operations
- [08-aicpu-queue](../08-aicpu-queue/) - Lock-free queues for inter-core communication
- **For production compute**: See [09-aicore-basic](../09-aicore-basic/) - AICore architecture and execution model
