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
1A + 4B + 24C + 48D

A: Host CPU
B: Device AICPU (control CPU, close to accelerator)
C: AICORE (Cube matrix + Vector SIMD units)
D: AICORE (Cube matrix + Vector SIMD units)

Latencies:
  A → B/C/D: ~3μs  (Host to device via PCIe)
  B → C/D:   ~0μs  (Tightly coupled control to compute)
```

**Memory Hierarchy:**
- Host RAM ← → HBM (device DRAM, 32-64GB)
- HBM ← → L2 Cache (192MB shared)
- L2 ← → L1/UB (per-core buffers, 256KB-1MB)
- L1/UB ← → L0A/B/C (compute unit local memory, 64-256KB)

## Examples Structure

Examples progress from simple to complex, each building on previous concepts:

```
examples/                      # Standalone progressive examples
  01-device-query/             # Query NPU hardware info
  02-memory/                   # HBM allocation, H2D/D2H transfers
  03-stream/                   # Asynchronous execution streams
  04-kernel/                   # Kernel calling interface (TODO)
  05-calling-interface/        # Argument packing patterns (TODO)
  06-aicpu-basic/              # AICPU kernel execution (TODO)
  ...
  11-aicore-basic/             # AICORE kernel execution (TODO)
  12-cube-matmul/              # Matrix multiplication (TODO)
  ...
  18-sync-register/            # Register synchronization (TODO)
  19-sync-atomic/              # Atomic coordination (TODO)
  20-sync-queue/               # Queue-based coordination (TODO)
```

**Status**: Examples 01-03 converted to direct ACL APIs ✅
**Todo**: Examples 04-20 still use platform layer (deferred)

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
