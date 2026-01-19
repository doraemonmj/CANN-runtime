/**
 * AICPU Kernel: parallel_add_kernel (Multi-threaded Backend Server Pattern)
 *
 * Hardware execution: Runs on ARM Cortex-A55 cores with full C++ support.
 * Demonstrates parallel execution across multiple AICPU cores.
 *
 * EDUCATIONAL PURPOSE: This example uses AICPU for compute to teach eval model.
 * - In production: AICPU controls AICore, AICore does computation
 * - Hardware limit: Maximum 4 AICPU cores on Ascend 910
 * - For production compute: Use AICore (examples 09-18)
 *
 * EVAL MODEL (not pthread model):
 * When host launches with aicpuNum=4, the CANN runtime:
 * 1. Loads this .so once into shared memory
 * 2. Directs 4 physical CPU cores → execute entry points simultaneously
 * 3. Each core gets thread ID via allocThreadIdx() (hardware mechanism)
 * 4. All cores execute same DynTileFwkBackendKernelServer() function
 *
 * This is NOT pthread spawning:
 * - No pthread_create() calls at runtime level
 * - All cores execute simultaneously (not parent→child)
 * - Thread ID from hardware CPU affinity, not software TLS alone
 *
 * Backend server pattern: System kernel (libaicpu_extend_kernels.so) loads
 * our .so and calls entry points by name.
 */

#include <cstdint>
#include <pthread.h>

extern "C" {

/**
 * Kernel argument structure
 * Hardware memory: Resides in HBM, accessible directly by all AICPU cores.
 * Must match host-side definition exactly (alignment, sizes, padding).
 */
struct ParallelAddArgs {
    void* input_a;    // HBM pointer: first input array
    void* input_b;    // HBM pointer: second input array
    void* output;     // HBM pointer: output array
    int32_t count;    // Total number of elements
    int32_t num_threads;  // Number of AICPU cores/threads (max 4)
};

/**
 * DeviceArgs structure (matches host side)
 * Hardware indirection: System kernel passes this to extract our custom args.
 */
struct DeviceArgs {
    uint64_t unused[12];
    uint64_t aicpuSoBin;
    uint64_t aicpuSoLen;
    uint64_t customArgsPtr;  // Pointer to our ParallelAddArgs in HBM
};

/* Global pointer: Stores custom args across init/exec phases */
static ParallelAddArgs* g_args = nullptr;

/* Thread-local storage key for thread ID
 *
 * SOFTWARE ABSTRACTION over eval model:
 * We use pthread TLS to assign sequential IDs (0-3) as a clean abstraction.
 * This works because:
 * 1. CANN runtime has already assigned cores via allocThreadIdx()
 * 2. Each core is executing this same code independently
 * 3. pthread TLS provides per-core storage for sequential ID
 *
 * ACTUAL HARDWARE MECHANISM (underneath):
 * CANN runtime's allocThreadIdx() function (in device_machine.cpp):
 * - Gets physical CPU core ID via sched_getcpu()
 * - Atomically registers in CPU mask
 * - Barrier waits for all nrAicpu cores to register (max 4)
 * - Assigns thread ID based on CPU cluster membership (4 cores per cluster)
 *
 * We could use sched_getcpu() % num_threads directly, but pthread TLS
 * provides a cleaner, more portable abstraction for application code.
 */
static pthread_key_t g_thread_id_key;
static int g_next_thread_id = 0;
static pthread_mutex_t g_thread_id_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Get thread ID (0-3 for 4 cores)
 *
 * EVAL MODEL ABSTRACTION:
 * This function provides sequential thread IDs (0, 1, 2, 3) to cores.
 * Each core executing this code independently will call this function and
 * get a unique ID through pthread TLS mechanism.
 *
 * HARDWARE REALITY:
 * - CANN runtime has already assigned each of 4 physical CPU cores to execute
 *   this same code path (eval model)
 * - allocThreadIdx() in runtime gave each core a hardware thread ID based on
 *   CPU affinity (sched_getcpu()) and cluster membership
 * - We abstract this with pthread TLS for cleaner application code
 *
 * ALTERNATIVE (more hardware-direct, less portable):
 *   int cpu = sched_getcpu();
 *   return (cpu >= 0) ? (cpu % num_threads) : 0;
 *
 * But pthread TLS is cleaner and recommended for application kernels.
 */
static int get_thread_id() {
    void* id_ptr = pthread_getspecific(g_thread_id_key);
    if (id_ptr == nullptr) {
        // First time this thread calls - assign ID
        pthread_mutex_lock(&g_thread_id_mutex);
        int my_id = g_next_thread_id++;
        pthread_mutex_unlock(&g_thread_id_mutex);

        pthread_setspecific(g_thread_id_key, reinterpret_cast<void*>(static_cast<intptr_t>(my_id)));
        return my_id;
    }
    return static_cast<int>(reinterpret_cast<intptr_t>(id_ptr));
}

/**
 * Backend server initialization entry point
 * Hardware execution: Called once during DynTileFwkKernelServerInit launch.
 * Extracts custom args pointer from DeviceArgs for use by main kernel.
 */
__attribute__((visibility("default")))
int DynTileFwkBackendKernelServerInit(void *arg) {
    if (arg == nullptr) {
        return -1;
    }

    /* Hardware indirection: System kernel wraps DeviceArgs in internal struct.
     * Extract DeviceArgs pointer at offset +40 from arg base. */
    DeviceArgs* devArgs = reinterpret_cast<DeviceArgs*>(
        *reinterpret_cast<uint64_t**>(reinterpret_cast<char*>(arg) + 40));

    if (devArgs != nullptr && devArgs->customArgsPtr != 0) {
        /* Hardware memory access: Read HBM pointer to our ParallelAddArgs */
        g_args = reinterpret_cast<ParallelAddArgs*>(devArgs->customArgsPtr);
    }

    /* Initialize thread-local storage for thread IDs */
    pthread_key_create(&g_thread_id_key, nullptr);
    g_next_thread_id = 0;

    return 0;
}

/**
 * Backend server main entry point
 *
 * EVAL MODEL EXECUTION:
 * When host calls rtAicpuKernelLaunchExWithArgs() with aicpuNum=4:
 * 1. CANN runtime loads this .so into shared memory (once)
 * 2. Runtime directs 4 physical CPU cores → all execute THIS function simultaneously
 * 3. Each core calls allocThreadIdx() to get hardware thread ID
 * 4. All cores execute same code path, partition work by thread ID
 *
 * This is NOT pthread_create():
 * - No parent thread spawning children
 * - All 4 cores are peers, executing simultaneously
 * - Entry point is same for all cores
 * - Thread identity from hardware, not software spawning
 *
 * EDUCATIONAL NOTE:
 * - This example uses AICPU for simple compute to demonstrate eval model
 * - Production: AICPU controls AICore, AICore performs computation
 * - For real compute workloads: See AICore examples (09-18)
 *
 * Multi-core execution:
 * - Each core executes this same function
 * - Threads use thread ID to partition work
 * - No synchronization needed (disjoint memory accesses)
 */
__attribute__((visibility("default")))
int DynTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr || g_args == nullptr) {
        return -1;
    }

    /* Use args pointer set during init phase */
    ParallelAddArgs* args = g_args;

    /* Hardware memory: Cast HBM pointers to typed pointers.
     * All AICPU cores share the same HBM address space. */
    float* in_a = reinterpret_cast<float*>(args->input_a);
    float* in_b = reinterpret_cast<float*>(args->input_b);
    float* out = reinterpret_cast<float*>(args->output);
    int32_t n = args->count;
    int32_t num_threads = args->num_threads;

    /* Get this thread's ID (0 to num_threads-1) */
    int thread_id = get_thread_id();

    /* Hardware work partitioning: Divide data among cores.
     * Each core processes a contiguous chunk to minimize cache conflicts.
     * Last thread handles remainder to avoid rounding issues. */
    int32_t chunk_size = n / num_threads;
    int32_t start = thread_id * chunk_size;
    int32_t end;

    if (thread_id == num_threads - 1) {
        /* Last thread handles remainder */
        end = n;
    } else {
        end = start + chunk_size;
    }

    /* Hardware execution: Element-wise addition on this core's chunk.
     * Each core works on disjoint memory regions - no synchronization needed.
     * Compiler may auto-vectorize with NEON instructions if optimization enabled. */
    for (int32_t i = start; i < end; i++) {
        out[i] = in_a[i] + in_b[i];
    }

    return 0;
}

/**
 * Static backend server entry point (optional)
 * Placeholder for static kernel pattern (not used in this example)
 */
__attribute__((visibility("default")))
int StaticTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        return -1;
    }
    return 0;
}

}  /* extern "C" */
