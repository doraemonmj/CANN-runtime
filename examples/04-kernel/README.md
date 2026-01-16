# Concept: Kernel Launch Interface

## What This Demonstrates
Understanding the interface for loading and launching kernels on Ascend NPU, including AICORE vs AICPU kernel types.

## Hardware Behavior
The Ascend NPU has two types of compute units:
- **AICORE** (24 blocks): Matrix/Vector specialized cores that execute compiled TIK/Ascend C kernels stored as `.o` binaries
- **AICPU** (4 processors): ARM control CPUs that execute standard C/C++ code compiled as `.so` shared libraries

Kernel launch involves packing arguments into a struct and specifying how many AICORE blocks should process the data in parallel.

## Code Structure
- [`main.cpp`](main.cpp) - Host-side educational example showing kernel loading concepts

## Key CANN APIs
- `aclInit()` - Initialize ACL runtime (see [main.cpp:43](main.cpp#L43))
- `aclrtSetDevice()` - Select device for execution (see [main.cpp:50](main.cpp#L50))
- `aclrtCreateContext()` - Create execution context (see [main.cpp:58](main.cpp#L58))
- `aclrtMalloc()` - Allocate device memory for kernel arguments (see [main.cpp:106](main.cpp#L106))
- `aclrtSynchronizeStream()` - Wait for kernels to complete (see [main.cpp:165](main.cpp#L165))
- `aclrtFree()` - Free device memory (see [main.cpp:188](main.cpp#L188))

Note: Real kernel loading would use `aclrtLoadKernel()` or `aclmdlLoadFromFile()` APIs. This example is educational and uses stub implementations.

## Running
```bash
mkdir build && cd build
cmake .. && make
./04-kernel
```

## Expected Output
Educational text explaining:
- Kernel types (AICORE vs AICPU)
- Argument struct layout (alignment at 8-byte boundaries)
- Launch parameters (block count, argument pointer, stream)
- Block count selection guidelines

## Next Steps
See [`../05-calling-interface/`](../05-calling-interface/) to learn about argument packing patterns and alignment rules.
