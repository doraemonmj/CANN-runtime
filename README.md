# Ascend NPU Programming Examples

Pure educational examples illustrating Ascend NPU hardware features through direct CANN API usage.

## Purpose

This is a **learning resource** that teaches Ascend NPU programming through minimal, progressive, standalone examples. Each example demonstrates one hardware concept using real CANN (Ascend Computing Language) APIs directly.

**Modeled after**: CUDA Programming Guide's pedagogical approach

**What this is:**
- Educational tutorial showing hardware features
- Progressive examples building complexity step-by-step
- Direct ACL API usage (no abstraction layers)
- Each example is self-contained and independently runnable

**What this is NOT:**
- Production runtime library or framework
- Scheduling system or task orchestrator
- Abstraction layer over CANN
- Framework integration (PyTorch/TensorFlow)

## Hardware Architecture (Simplified)

```
Host CPU
  ↓ PCIe (~3μs)
AICPU (control CPU, coordinates AICore blocks)
  ↓ On-chip (~0μs)
AICore Blocks (24 blocks, each containing):
  - 1 Cube Core (matrix operations)
  - 2 Vector Cores (element-wise SIMD)
  - 1 Scalar Unit (control flow)
  - Shared L1 Buffer

Latencies:
  Host CPU → AICPU/AICore: ~3μs (PCIe transfer)
  AICPU → AICore:          ~0μs (tightly coupled on-chip)
```

**Memory Hierarchy:**
- Host RAM ← → HBM (device DRAM, 32-64GB)
- HBM ← → L2 Cache (192MB shared)
- L2 ← → L1/UB (per-core buffers, 256KB-1MB)
- L1/UB ← → L0A/B/C (compute unit local memory, 64-256KB)

## Examples Structure

Examples progress from simple to complex, each building on previous concepts.

### Status Legend
- ✅ **Complete**: Code + Example README + ReadTheDocs + Direct ACL APIs
- 🟢 **Functional**: Code + ReadTheDocs (missing example README or using platform layer)
- 🚧 **In Progress**: Partial implementation
- ❌ **Not Started**: Planned but not implemented

### Example List

| Example | Concept | Documentation | Status |
|---------|---------|---------------|--------|
| **Getting Started** | | | |
| [01-device-query](examples/01-device-query/) | Query NPU hardware info (devices, cores, memory) | [docs](docs/getting-started/device-query.rst) | ✅ |
| [02-memory](examples/02-memory/) | HBM allocation, H2D/D2H transfers | [docs](docs/getting-started/memory.rst) | ✅ |
| [03-stream](examples/03-stream/) | Asynchronous execution streams | [docs](docs/getting-started/streams.rst) | ✅ |
| **AICPU Programming** | | | |
| [04-aicpu-basic](examples/04-aicpu-basic/) | AICPU kernel execution with Runtime API (.so loading) | [docs](docs/aicpu/kernel-launch.rst) | ✅ |
| [05-aicpu-scale](examples/05-aicpu-scale/) | Complete AICPU launch workflow | [docs](docs/aicpu/kernel-launch.rst) | 🟢 |
| [06-aicpu-logging](examples/06-aicpu-logging/) | Debug output and error handling | [docs](docs/aicpu/logging.rst) | 🟢 |
| [07-aicpu-atomic](examples/07-aicpu-atomic/) | Multi-core synchronization with atomics | [docs](docs/aicpu/atomic.rst) | 🟢 |
| [08-aicpu-queue](examples/08-aicpu-queue/) | Lock-free queues (SPSC/MPMC) | [docs](docs/aicpu/queue.rst) | 🟢 |
| **AICORE Programming** | | | |
| [09-aicore-basic](examples/09-aicore-basic/) | AICORE architecture and execution model | [docs](docs/aicore/overview.rst) | 🟢 |
| [10-cube-matmul](examples/10-cube-matmul/) | Cube unit matrix multiplication (MMAD) | [docs](docs/aicore/cube-unit/compute.rst) | 🟢 |
| [11-cube-memory](examples/11-cube-memory/) | L1/L0A/B/C buffer management for Cube | [docs](docs/aicore/cube-unit/l0l1-allocation.rst) | 🟢 |
| [12-vector-simd](examples/12-vector-simd/) | Vector unit SIMD operations | [docs](docs/aicore/vector-unit/compute.rst) | 🟢 |
| [13-vector-ub](examples/13-vector-ub/) | UB (Unified Buffer) memory management | [docs](docs/aicore/vector-unit/ub-allocation.rst) | 🟢 |
| [14-aicore-atomic](examples/14-aicore-atomic/) | Multi-block atomic operations | [docs](docs/aicore/atomic.rst) | 🟢 |
| [15-aicore-pmu](examples/15-aicore-pmu/) | Performance Monitoring Unit (profiling) | [docs](docs/aicore/pmu.rst) | 🟢 |
| **AICPU-AICORE Coordination** | | | |
| [16-aicpu-aicore-register](examples/16-aicpu-aicore-register/) | Register-based AICPU↔AICORE coordination | [docs](docs/synchronization/register.rst) | 🟢 |
| [17-aicpu-aicore-atomic](examples/17-aicpu-aicore-atomic/) | HBM atomic-based coordination | [docs](docs/synchronization/atomic.rst) | 🟢 |
| [18-aicpu-aicore-queue](examples/18-aicpu-aicore-queue/) | Queue-based data passing | [docs](docs/synchronization/queue.rst) | 🟢 |

### Current Progress
- **Code**: 18 examples implemented ✅ (01-18, continuous numbering)
- **ReadTheDocs**: 20/20 pages complete ✅ (docs remain complete)
- **Example READMEs**: 4 complete (01-04) 🚧
- **Direct ACL APIs**: 4 converted (01-04) 🚧

## Philosophy

### One Concept Per Example
Each example teaches exactly one hardware concept:
- Device query → Understanding NPU topology
- Memory → HBM allocation and transfers
- Streams → Async execution model

### Direct API Usage
No abstraction layers - code shows actual CANN/ACL APIs:
```cpp
// Not: platform_malloc(size)
// Instead: aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST)
```

### Hardware-First Explanations
Focus on what the hardware does, not software abstractions:
- "HBM is physically on the NPU card"
- "DMA transfers happen over PCIe"
- "Streams are command queues to the device"

### Progressive Complexity
Start with fundamentals, add one concept at a time:
01 → Query → 02 → Memory → 03 → Streams → 04 → Kernels...

## Getting Started

### Prerequisites
- Ascend NPU hardware (910A/910B/910C)
- CANN toolkit installed
- `ASCEND_HOME_PATH` environment variable set

### Build an Example
```bash
cd examples/01-device-query
mkdir build && cd build
cmake ..
make
./01-device-query
```

### Expected Output
Each example includes a README.md showing expected output and explaining the hardware behavior.

## Documentation

- **[CLAUDE.md](CLAUDE.md)** - AI assistant instructions for maintaining this codebase
- **[examples/XX-name/README.md](examples/)** - Per-example documentation with:
  - Concept being demonstrated
  - Hardware behavior explanation
  - Code structure with markdown links
  - Key CANN APIs with line references
  - Build/run instructions
  - Expected output

## Not Included

This guide deliberately **does not** provide:
- ❌ Scheduling algorithms or policies
- ❌ Production optimizations
- ❌ Framework integration (PyTorch, TensorFlow, JAX)
- ❌ Distributed computing / multi-node
- ❌ Application-specific logic
- ❌ Abstraction layers over CANN

For production use, see official [CANN documentation](https://www.hiascend.com/).

## Contributing

When adding examples:
1. Focus on one hardware concept
2. Use direct ACL APIs (include `<acl/acl.h>`)
3. Make it standalone (own CMakeLists.txt)
4. Add README.md with markdown links to code
5. Keep it minimal - no over-engineering

See [CLAUDE.md](CLAUDE.md) for detailed guidelines.
