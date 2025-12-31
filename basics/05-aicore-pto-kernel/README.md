# AICORE PTO Kernel Demo - Tensor Add (TADD)

A complete example demonstrating how to build and run an AI Core kernel using [PTO (Parallel Tile Operation) ISA](https://gitcode.com/cann/pto-isa) with modern CMake dependency management.

## Overview

This project implements a **tensor addition kernel** (`TADD`) that performs element-wise addition on 64x64 float matrices using Ascend AI Core hardware. It showcases:

- ✅ **Modern dependency management** with CMake FetchContent
- ✅ **Binary kernel loading** with `rtRegisterAllKernel`
- ✅ **PTO instruction usage** (`TLOAD`, `TADD`, `TSTORE`)
- ✅ **Dual-path compilation** (Cube AIC + Vector AIV)
- ✅ **Zero manual setup** - pto-isa automatically fetched

## Features

### 🚀 Automatic Dependency Management
- Uses CMake FetchContent to automatically download [pto-isa](https://gitcode.com/cann/pto-isa)
- No manual path configuration needed
- Reproducible builds across different environments

### 🎯 PTO Tile Operations
Demonstrates core PTO instructions:
```cpp
TLOAD(src0Tile, src0Global);    // Load from global memory
TLOAD(src1Tile, src1Global);
TADD(dstTile, src0Tile, src1Tile);  // Tile-level addition
TSTORE(dstGlobal, dstTile);     // Store back to global memory
```

### 🔧 Production-Ready Build
- BiSheng compiler integration
- Optimized compilation flags (`-O3`)
- Proper synchronization with event flags
- Error handling and validation

## Prerequisites

### Required
- **CANN toolkit** with `ASCEND_HOME_PATH` environment variable set
  ```bash
  export ASCEND_HOME_PATH=/usr/local/Ascend/latest
  ```
  The path should contain:
  - `compiler/ccec_compiler/bin/bisheng` - BiSheng compiler
  - `compiler/ccec_compiler/bin/ld.lld` - LLD linker
  - `include/` - CANN runtime headers
  - `lib64/` - CANN runtime libraries

- **CMake** >= 3.16.3
- **C++ compiler** with C++17 support
- **Git** (for fetching pto-isa)
- **Ascend NPU device** (910B/910C/950) or simulator

### Optional
- Internet connection (first build only, to fetch pto-isa)

## Quick Start

```bash
# Navigate to project directory
cd simpler/basics/05-aicore-pto-kernel

# Create build directory
mkdir -p build && cd build

# Configure with CMake (will auto-fetch pto-isa)
cmake ..

# Build (compiles kernel + launcher)
make -j4

# Run on device 0 (default)
./launcher

# Or specify device ID
./launcher 1
```

### Expected Output
```
=== Launching AICORE PTO kernel ===
PASS: i=0, src0=0, src1=0, got=0, expect=0
PASS: i=1, src0=1, src1=2, got=3, expect=3
...
PASS: i=9, src0=9, src1=18, got=27, expect=27
=== 校验通过 ===
```

## Project Structure

```
05-aicore-pto-kernel/
├── CMakeLists.txt           # Main build configuration
├── launcher.cpp             # Host-side launcher (loads & runs kernel)
├── kernel/
│   ├── CMakeLists.txt       # Kernel build with FetchContent
│   └── pto_kernel.cpp       # PTO TADD kernel implementation
├── build/                   # Build artifacts
│   ├── launcher             # Executable
│   ├── kernel/
│   │   └── pto_kernel.o     # Compiled kernel binary
│   └── _deps/
│       └── pto-isa-src/     # Auto-fetched pto-isa headers
└── README.md
```

## How It Works

### 1. Kernel Compilation (kernel/CMakeLists.txt)

```cmake
# Automatically fetch pto-isa from gitcode.com
FetchContent_Declare(
    pto-isa
    GIT_REPOSITORY https://gitcode.com/cann/pto-isa.git
    GIT_TAG        master
)
FetchContent_Populate(pto-isa)

# Compile for both Cube (AIC) and Vector (AIV) cores
COMMAND ${BISHENG_COMPILER} ... -D__AIC__ --cce-aicore-arch=dav-c220-cube ...
COMMAND ${BISHENG_COMPILER} ... -D__AIV__ --cce-aicore-arch=dav-c220-vec ...

# Link into single binary
COMMAND ${BISHENG_LD} -o pto_kernel.o pto_kernel_aic.o pto_kernel_aiv.o
```

### 2. Kernel Implementation (kernel/pto_kernel.cpp)

```cpp
// PTO-based tensor addition
template <typename T, int kTRows_, int kTCols_, int vRows, int vCols>
void runTAdd(__gm__ T *out, __gm__ T *src0, __gm__ T *src1) {
    // Define tile types
    using TileData = Tile<TileType::Vec, T, kTRows_, kTCols_, ...>;

    // Allocate tiles in local buffer
    TileData src0Tile(vRows, vCols);
    TileData src1Tile(vRows, vCols);
    TileData dstTile(vRows, vCols);

    // Assign buffer addresses
    TASSIGN(src0Tile, 0x0);
    TASSIGN(src1Tile, 0x10000);
    TASSIGN(dstTile, 0x20000);

    // Pipeline: Load → Compute → Store
    TLOAD(src0Tile, src0Global);
    TLOAD(src1Tile, src1Global);
    set_flag(PIPE_MTE2, PIPE_V, EVENT_ID0);  // Sync load complete
    wait_flag(PIPE_MTE2, PIPE_V, EVENT_ID0);

    TADD(dstTile, src0Tile, src1Tile);       // Element-wise add

    set_flag(PIPE_V, PIPE_MTE3, EVENT_ID0);  // Sync compute complete
    wait_flag(PIPE_V, PIPE_MTE3, EVENT_ID0);
    TSTORE(dstGlobal, dstTile);              // Write back
}
```

### 3. Runtime Loading (launcher.cpp)

```cpp
// Read compiled kernel binary
std::vector<uint8_t> bin;
ReadFile("./kernel/pto_kernel.o", bin);

// Register with CANN runtime
rtDevBinary_t binary = { .data = bin.data(), .length = bin.size() };
void *binHandle;
rtRegisterAllKernel(&binary, &binHandle);

// Prepare arguments
struct Args { void *out; void *src0; void *src1; };
Args args = {dOut, dSrc0, dSrc1};

// Launch kernel (1 block, vector core)
rtKernelLaunchWithHandleV2(binHandle, 0, /*blocks=*/1, &args, ...);
```

## Key Technical Details

### Compilation Flags
- `-DMEMORY_BASE` - Enable A2/A3 architecture support (910B/910C)
- `-D__AIV__` / `-D__AIC__` - Select Vector/Cube core path
- `--cce-aicore-arch=dav-c220-{vec,cube}` - Target architecture
- `-mllvm -cce-aicore-stack-size=0x8000` - Configure stack size

### Memory Layout
- **UB (Unified Buffer)** tile offsets:
  - `src0Tile`: 0x00000 (256 bytes)
  - `src1Tile`: 0x10000 (256 bytes)
  - `dstTile`:  0x20000 (256 bytes)

### Test Configuration
- Matrix size: 64x64 float (4096 elements)
- Input data: `src0[i] = i`, `src1[i] = i * 2`
- Expected output: `out[i] = src0[i] + src1[i] = 3 * i`

## Customization

### Change Matrix Size
Modify the template parameters in `kernel/pto_kernel.cpp`:
```cpp
// Original: 64x64
runTAdd<float, 64, 64, 64, 64>(out, src0, src1);

// Change to 128x128
runTAdd<float, 128, 128, 128, 128>(out, src0, src1);
```

Also update `launcher.cpp`:
```cpp
constexpr size_t elems = 128 * 128;  // Update size
```

### Target Different Architecture
In `kernel/CMakeLists.txt`, change:
```cmake
--cce-aicore-arch=dav-c220-vec    # A2/A3 Vector
# to
--cce-aicore-arch=dav-c310-vec    # A5 Vector
```

And use `-DREGISTER_BASE` instead of `-DMEMORY_BASE` for A5.

### Pin pto-isa Version
In `kernel/CMakeLists.txt`, specify a commit hash:
```cmake
FetchContent_Declare(
    pto-isa
    GIT_REPOSITORY https://gitcode.com/cann/pto-isa.git
    GIT_TAG        abc123def456  # Specific commit
)
```

## Troubleshooting

### CMake can't find BiSheng compiler
```
Error: ASCEND_HOME_PATH is not set
```
**Solution**: Set environment variable:
```bash
export ASCEND_HOME_PATH=/usr/local/Ascend/latest
```

### pto-isa fetch fails
```
Failed to checkout tag: 'master'
```
**Solution**: Check network connection or manually specify a working commit hash.

### Kernel compilation fails
```
fatal error: 'pto/pto-inst.hpp' file not found
```
**Solution**: Ensure CMake successfully fetched pto-isa. Check `build/_deps/pto-isa-src/`.

### Runtime kernel launch fails
```
rtKernelLaunchWithHandleV2失败: 107001
```
**Solution**:
- Verify device is available: `npu-smi info`
- Check binary compatibility with target device
- Ensure sufficient device memory

## References

- [PTO-ISA Documentation](https://gitcode.com/cann/pto-isa)
- [CANN Kernel Development Guide](https://www.hiascend.com/document)
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)

## Related Examples

- `03-aicpu-kernel/` - CPU-side kernel example
- `04-aicore-kernel/` - Basic AI Core kernel without PTO
- `pto-isa/tests/npu/a2a3/src/st/testcase/tadd/` - Original PTO TADD reference

## License

This example follows the CANN Open Software License Agreement Version 2.0.
