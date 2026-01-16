# Concept: Argument Packing and Alignment

## What This Demonstrates
Understanding how to pack kernel arguments into structs with correct alignment, and the contract between host code and kernel code.

## Hardware Behavior
Kernel arguments are passed as a contiguous memory block (argument buffer). The Ascend NPU requires:
- 8-byte alignment for the struct as a whole
- Pointers (void*) are always 8 bytes and point to Global Memory (HBM)
- Local memory (UB, L1, L0) is allocated by kernels internally, never passed as arguments

## Code Structure
- [`main.cpp`](main.cpp) - Educational example showing argument struct patterns and alignment rules

## Key CANN APIs
- `aclInit()` - Initialize ACL runtime (see [main.cpp:70](main.cpp#L70))
- `aclrtMalloc()` - Allocate GM (HBM) memory for tensor arguments (see [main.cpp:156](main.cpp#L156))
- `aclrtMemcpy()` - Transfer data with direction flag (see [main.cpp:215-224](main.cpp#L215-L224))
- `aclrtFree()` - Free device memory (see [main.cpp:228](main.cpp#L228))

## Running
```bash
mkdir build && cd build
cmake .. && make
./05-calling-interface
```

## Expected Output
Educational text explaining:
- Alignment rules (all types aligned to their natural size, struct to 8 bytes)
- Example struct layouts (SimpleArgs, MatMulArgs, ConvArgs)
- Memory address types (GM vs local memory)
- Common mistakes and best practices
- Complete launch sequence with ACL APIs

## Next Steps
See [`../06-aicpu-basic/`](../06-aicpu-basic/) to learn about AICPU kernel execution with actual kernel code.
