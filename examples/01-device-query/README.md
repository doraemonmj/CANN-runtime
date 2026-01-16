# 01-device-query - Query NPU Hardware Information

## Concept

Query Ascend NPU hardware properties including device count, SoC version, core counts, and memory hierarchy.

## Hardware Behavior

When you call `aclrtGetDeviceCount()`, the ACL runtime queries the PCIe bus to enumerate Ascend NPU devices. Each device has:

- **AICORE**: Compute units with Cube (matrix multiplication) and Vector (SIMD) engines
- **AICPU**: Control processors for task scheduling and scalar operations
- **Memory Hierarchy**: HBM (device DRAM) → L2 Cache (shared) → L1/UB (per-core) → L0A/B/C (compute local)

The example demonstrates two types of queries:
1. **Runtime queries via ACL APIs**: Device count, core counts, HBM/L2 sizes (queried at runtime)
2. **Architectural constants from config files**: AICORE buffer sizes (UB, L0A/B/C, L1) read from platform INI files

AICORE buffer sizes cannot be queried via ACL APIs because they're fixed architectural constants defined at chip design time. The code reads them from CANN's platform configuration files located at `$ASCEND_HOME_PATH/aarch64-linux/data/platform_config/<SoC>.ini`.

## Code Structure

- [`main.cpp`](main.cpp) - Device enumeration and property queries
  - [`read_soc_config()`](main.cpp#L61) - Parses platform config INI files for SoC architecture
  - [`main()`](main.cpp#L136) - Device query workflow

## Key CANN APIs

| API | Purpose | Line |
|-----|---------|------|
| `aclInit()` | Initialize ACL runtime | [main.cpp:140](main.cpp#L140) |
| `aclrtGetDeviceCount()` | Get number of NPU devices | [main.cpp:149](main.cpp#L149) |
| `aclrtSetDevice()` | Set active device context | [main.cpp:169](main.cpp#L169) |
| `aclrtGetSocName()` | Get SoC version string | [main.cpp:177](main.cpp#L177) |
| `aclrtGetDeviceInfo()` | Query device properties (cores) | [main.cpp:190-199](main.cpp#L190) |
| `aclrtGetMemInfo()` | Query HBM capacity and usage | [main.cpp:210](main.cpp#L210) |
| `aclrtResetDevice()` | Reset device context | [main.cpp:252](main.cpp#L252) |
| `aclFinalize()` | Clean up ACL runtime | [main.cpp:259](main.cpp#L259) |

## Build and Run

```bash
cd examples/01-device-query
mkdir build && cd build
cmake ..
make
./01-device-query
```

**Prerequisites:**
- CANN toolkit installed
- `ASCEND_HOME_PATH` environment variable set (or defaults to `/usr/local/Ascend/ascend-toolkit/latest`)
- At least one Ascend NPU device

## Expected Output

```
=== Ascend NPU Device Query ===

Found 16 NPU device(s)

--- Device 0 ---
  SoC Version                       : Ascend910_9392 (via aclrtGetSocName)

  Core Configuration (via aclrtGetDeviceInfo):
    AICORE cores                    : 24
    Vector cores                    : 48
    AICPU cores                     : 6

  Memory Hierarchy (via aclrtGetMemInfo):
    HBM Total                       : 61.27 GB
    HBM Free                        : 60.87 GB

  Hardware Configuration (from /path/to/CANN/platform_config/Ascend910_9392.ini):
    AICORE cores                    : 24
    Cube cores                      : 24
    Vector cores                    : 48
    AICPU cores                     : 6

    L2 Cache                        : 192.00 MB

    AICORE Buffers (per core):
      Unified Buffer (UB)           : 192.00 KB
      L1 Buffer                     : 512.00 KB
      L0A Buffer                    : 64.00 KB
      L0B Buffer                    : 64.00 KB
      L0C Buffer                    : 128.00 KB

--- Device 1 ---
...
```

**Note:** Output varies by hardware. This example shows Ascend910_9392 with 16 devices. Your system may show different SoC versions (910A/910B/910C) and device counts. All colons align at column 36 for readability.

## What You Learned

- How to enumerate NPU devices and query their properties via ACL APIs
- The difference between runtime-queryable properties and architectural constants
- Ascend NPU hardware architecture: AICORE, AICPU, and memory hierarchy
- How to read SoC-specific configurations from CANN platform files

## Next Steps

See [02-memory](../02-memory/) to learn about HBM allocation and host-device memory transfers.
