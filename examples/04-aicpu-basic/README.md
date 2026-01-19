# 04-aicpu-basic - AICPU Kernel Execution

Demonstrates AICPU kernel execution using the Runtime API backend server pattern.

## Concept

This example shows how to load and execute AICPU kernels on the NPU's control processors (ARM Cortex-A55 cores). AICPU kernels are standard C/C++ code compiled for ARM architecture with full language support.

## Hardware Behavior

**AICPU Architecture:**
- ARM Cortex-A55 cores (typically 8 cores on Ascend 910)
- Full C/C++ execution environment with libc/libstdc++ support
- Direct access to HBM (same address space as AICore)
- No specialized compute units - standard ARM instruction set

**Memory Model:**
```
Host CPU (x86)
  ↓ PCIe (~3μs DMA transfer)
Device HBM ←─ AICPU (ARM Cortex-A55) directly reads/writes
          └─ AICore also reads/writes (shared memory space)
```

AICPU and AICore both access HBM directly - no explicit data movement needed between them.

**When to use AICPU vs AICore:**
- **AICPU**: Dynamic shapes, complex control flow, sparse operations, standard algorithms
- **AICore**: Dense matrix operations, SIMD vectorization, fixed-size tensor operations

## Code Structure

- [main.cpp](main.cpp) - Host launcher using Runtime API
- [kernel/scale_kernel.cpp](kernel/scale_kernel.cpp) - AICPU kernel implementation
- [kernel/CMakeLists.txt](kernel/CMakeLists.txt) - Kernel build configuration

## Backend Server Pattern

AICPU kernel execution requires the **backend server pattern** - a multi-layer structure:

1. **System Kernel**: `libaicpu_extend_kernels.so` (CANN-provided)
2. **Backend Server**: Our `libtilefwk_backend_server.so`
3. **Entry Points**: Specific function names expected by system kernel:
   - `DynTileFwkBackendKernelServerInit` - Initialization phase
   - `DynTileFwkBackendKernelServer` - Execution phase
   - `StaticTileFwkBackendKernelServer` - Optional static variant

The system kernel acts as an intermediary, loading our backend server .so and calling these entry points by name.

## Key Runtime APIs

**Device Initialization:**
- `rtSetDevice()` - Select NPU device (see [main.cpp:199](main.cpp#L199))
- `rtStreamCreate()` - Create command queue (see [main.cpp:209](main.cpp#L209))

**Memory Management:**
- `rtMalloc()` - Allocate HBM (see [main.cpp:232](main.cpp#L232), [main.cpp:242](main.cpp#L242), [main.cpp:290](main.cpp#L290))
- `rtMemcpy()` - Transfer data host↔device (see [main.cpp:256](main.cpp#L256), [main.cpp:311](main.cpp#L311))
- `rtFree()` - Free HBM allocation

**Kernel Loading and Launch:**
- `rtMalloc()` + `rtMemcpy()` - Load .so to HBM (see [AicpuSoInfo::Init](main.cpp#L107))
- `rtAicpuKernelLaunchExWithArgs()` - Launch AICPU kernel (see [main.cpp:187](main.cpp#L187))
- Two-phase launch pattern:
  - Init kernel: `DynTileFwkKernelServerInit` (see [main.cpp:353](main.cpp#L353))
  - Main kernel: `DynTileFwkKernelServer` (see [main.cpp:372](main.cpp#L372))

**Synchronization:**
- `rtStreamSynchronize()` - Wait for kernels to complete (see [main.cpp:390](main.cpp#L390))

## Argument Passing Workaround

The backend server pattern has limitations on custom argument passing. We use DeviceArgs indirection:

**Host Side** ([main.cpp:32](main.cpp#L32)):
- `DeviceArgs` contains `.so` location + `customArgsPtr` to our `ScaleArgs`
- Host allocates `ScaleArgs` in HBM and passes pointer through `DeviceArgs.customArgsPtr`
- System kernel reads `DeviceArgs` to locate backend server and custom args

**Kernel Side** ([kernel/scale_kernel.cpp:54](kernel/scale_kernel.cpp#L54)):
- Init kernel extracts `DeviceArgs` from system kernel's internal structure (offset +40)
- Stores `customArgsPtr` in global variable `g_scaleArgs`
- Main kernel uses `g_scaleArgs` to access our `ScaleArgs` in HBM

See [DeviceArgs extraction](kernel/scale_kernel.cpp#L61) for the implementation details.

## Build and Run

```bash
# Set up environment
source /usr/local/Ascend/ascend-toolkit/latest/bin/setenv.bash  # or your CANN path

# Build kernel (ARM64 .so)
cd kernel
mkdir build && cd build
cmake ..
make  # Produces libtilefwk_backend_server.so

# Build host launcher
cd ../..
mkdir build && cd build
cmake ..
make
./04-aicpu-basic
```

## Expected Output

```
=== AICPU Kernel Example ===

PASS: All 1024 elements correct (scale factor: 2.5)
  Input[0]  = 0  →  Output[0]  = 0
  Input[N-1] = 1023  →  Output[N-1] = 2557.5

=== End of AICPU Example ===
```

## What You Learned

✅ **AICPU vs AICore differences:**
- AICPU: ARM CPU cores with full C++ support
- AICore: Specialized compute units (Cube/Vector) requiring Ascend C

✅ **Backend server pattern:**
- System kernel (`libaicpu_extend_kernels.so`) acts as intermediary
- Specific function names required (`DynTileFwkBackendKernelServerInit`, etc.)
- .so must be named `libtilefwk_backend_server.so`

✅ **Argument passing workaround:**
- Custom args passed via DeviceArgs structure
- Init kernel extracts args pointer for main kernel to use
- DeviceArgs has hardcoded offsets expected by system kernel

✅ **Runtime API workflow:**
- Load .so binary to device HBM
- Use `rtAicpuKernelLaunchExWithArgs()` with complex argument structure
- Two-phase launch pattern (init + main)

✅ **When to use AICPU:**
- Dynamic shapes (size unknown at compile time)
- Complex control flow (data-dependent branches)
- Sparse or irregular memory access
- Operations better suited for CPU than specialized units

## Next Steps

See [05-aicpu-aicore-sync](../05-aicpu-aicore-sync/) to learn about coordination between AICPU and AICore kernels.
