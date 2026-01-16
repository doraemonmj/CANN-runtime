# Design Document: platform.so

## Decision Record

**Date**: 2026-01-15

**Decision**: Option A - Platform + Examples in this repo, A3 only

**Rationale**:
- Tests = Examples = Documentation (single source of truth)
- A3 focus keeps it simple - no multi-backend abstraction
- Users can run real code on A3 hardware

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Worker (kernels)                                       │
├─────────────────────────────────────────────────────────┤
│  Machine (task dispatch)                                │
├─────────────────────────────────────────────────────────┤
│  Platform  ← THIS PROJECT (A3 only)                     │
│  - Memory / KernelLaunch / Sync                         │
└─────────────────────────────────────────────────────────┘
```

## Directory Structure

```
simpler/
├── platform/
│   ├── platform.h           # A3-specific API
│   └── platform.cpp         # A3 implementation
│
├── examples/
│   ├── 01-device-query/     # Query A3 hardware info
│   ├── 02-memory/           # A3 HBM alloc/transfer
│   ├── 03-aicore-launch/    # Launch kernel to A3 AICORE
│   ├── 04-aicpu-launch/     # Launch kernel to A3 AICPU
│   ├── 05-events/           # A3 event sync
│   └── 06-multi-kernel/     # Multiple kernels with sync
│
└── CMakeLists.txt
```

## Platform API (A3-specific)

```cpp
// Init
int platform_init();
void platform_shutdown();

// Memory (A3 HBM)
void* platform_malloc(size_t size);
void  platform_free(void* ptr);
int   platform_memcpy_h2d(void* dst, const void* src, size_t size);
int   platform_memcpy_d2h(void* dst, const void* src, size_t size);

// Kernel Launch (A3)
int platform_launch_aicore(void* kernel, void* args, size_t args_size);
int platform_launch_aicpu(void* kernel, void* args, size_t args_size);

// Sync
PlatformEvent platform_event_create();
void platform_event_record(PlatformEvent event);
void platform_event_wait(PlatformEvent event);
```

## Implementation Phases

| Phase | Goal | Example |
|-------|------|---------|
| 1 | Init + Device Query | 01-device-query |
| 2 | Memory | 02-memory |
| 3 | AICORE Launch | 03-aicore-launch |
| 4 | AICPU Launch | 04-aicpu-launch |
| 5 | Events | 05-events |
| 6 | Multi-kernel | 06-multi-kernel |

## Answers from learn branch

### 1. A3 SDK: CANN Runtime API (`<runtime/rt.h>`)

| Category | APIs |
|----------|------|
| Device | `rtSetDevice`, `rtResetDevice` |
| Stream | `rtStreamCreate`, `rtStreamDestroy`, `rtStreamSynchronize` |
| Memory | `rtMalloc(RT_MEMORY_HBM)`, `rtFree`, `rtMemcpy` |
| AICORE | `rtRegisterAllKernel`, `rtKernelLaunchWithHandleV2` |
| AICPU | `rtAicpuKernelLaunchExWithArgs` |

### 2. Kernel Format

| Target | Format | How to Load |
|--------|--------|-------------|
| AICORE | `.o` binary (ELF) | `rtRegisterAllKernel` → `rtKernelLaunchWithHandleV2` |
| AICPU | `.so` shared object | Upload to HBM → `rtAicpuKernelLaunchExWithArgs` |

AICORE kernel example (`kernel.cpp`):
```cpp
extern "C" __global__ __aicore__ void kernel_entry(__gm__ uint8_t *Out, int64_t Stride) {
    // kernel code
}
```

### 3. Testing: Real A3 Hardware

- Device IDs: 0-15
- Requires CANN toolkit installed
- Examples run directly on hardware

---

## Refined Platform API

Based on learn branch findings, platform.so wraps CANN runtime:

```cpp
// === Device ===
int platform_init(int device_id);       // wraps rtSetDevice
void platform_shutdown();               // wraps rtResetDevice

// === Stream ===
typedef void* PlatformStream;
PlatformStream platform_stream_create();
void platform_stream_destroy(PlatformStream stream);
void platform_stream_sync(PlatformStream stream);

// === Memory ===
void* platform_malloc(size_t size);                              // rtMalloc(RT_MEMORY_HBM)
void  platform_free(void* ptr);                                  // rtFree
int   platform_memcpy_h2d(void* dst, const void* src, size_t n); // RT_MEMCPY_HOST_TO_DEVICE
int   platform_memcpy_d2h(void* dst, const void* src, size_t n); // RT_MEMCPY_DEVICE_TO_HOST

// === AICORE Kernel ===
typedef void* PlatformAICOREKernel;
PlatformAICOREKernel platform_aicore_load(const void* bin, size_t size);  // rtRegisterAllKernel
int platform_aicore_launch(PlatformAICOREKernel kernel,
                           uint32_t blocks,
                           void* args, size_t args_size,
                           PlatformStream stream);                         // rtKernelLaunchWithHandleV2

// === AICPU Kernel ===
int platform_aicpu_launch(const void* so_data, size_t so_size,
                          void* args, size_t args_size,
                          int num_cores,
                          PlatformStream stream);                          // rtAicpuKernelLaunchExWithArgs
```

## Revised Implementation Phases

| Phase | Goal | Platform APIs | Example |
|-------|------|---------------|---------|
| 1 | Device + Stream | `init`, `stream_*` | 01-hello |
| 2 | Memory | `malloc`, `free`, `memcpy_*` | 02-memory |
| 3 | AICORE Launch | `aicore_load`, `aicore_launch` | 03-aicore |
| 4 | AICPU Launch | `aicpu_launch` | 04-aicpu |
| 5 | Multi-kernel | All above + sequencing | 05-multi-kernel |

---

## Findings from PyPTO Codebase

### Hardware Hierarchy (from `platform.h`)

```
Platform
  └── Cluster
        └── SoC (NPUArch: DAV_1001, DAV_2201, DAV_3510)
              └── Die
                    ├── CoreWrap (AICORE block)
                    │     ├── AIC Core (Cube) - matrix ops
                    │     └── AIV Core (Vector) - vector ops
                    └── AICPU (Control CPU)
```

### NPU Architecture Versions

| Arch | Description | Register Base |
|------|-------------|---------------|
| DAV_1001 | Older | - |
| DAV_2201 | A2 | `REG_SPR_DATA_MAIN_BASE=0xA0`, `REG_SPR_COND=0x4C8` |
| DAV_3510 | A3 (910C) | `REG_SPR_DATA_MAIN_BASE=0xD0`, `REG_SPR_COND=0x5108` |

### Hardware Counts (from `aicore_hal.h`, `platform.h`)

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_AICORE_NUM` | 108 | Max AICORE blocks |
| `NAX_AIV_TOTAL_NUM` | 72 | Max AIV cores |
| `CORE_NUM_PER_AI_CORE` | 3 | 1 AIC + 2 AIV per block |
| `AIV_NUM_PER_AI_CORE` | 2 | Vector cores per AICORE |

### Memory Hierarchy (from `platform.h`)

| Memory Type | Description |
|-------------|-------------|
| `MEM_UB` | Unified Buffer (per-core scratch) |
| `MEM_L1` | L1 cache |
| `MEM_L0A` | L0 buffer A (input) |
| `MEM_L0B` | L0 buffer B (weight) |
| `MEM_L0C` | L0 buffer C (output/accumulator) |
| `MEM_DEVICE_DDR` | HBM (global memory) |
| `MEM_HOST1` | Host memory |

### PlatformManager Queries (from `platform_manager.h`)

```cpp
GetAiCoreCnt()          // Number of AICORE blocks
GetVecCoreCnt()         // Number of vector cores
GetAiCpuCnt()           // Number of AICPU
GetMemorySize()         // Total HBM size
GetL2Type()             // L2 cache type
GetL2Size()             // L2 cache size
GetL2PageNum()          // L2 page count
GetAiCoreCubeFreq()     // Cube unit frequency
GetAiCoreL0ASize()      // L0A buffer size
GetAiCoreL0BSize()      // L0B buffer size
GetAiCoreL0CSize()      // L0C buffer size
GetAiCoreL1Size()       // L1 size
GetAiCoreUbSize()       // Unified Buffer size
GetAiCoreUbBlockSize()  // UB block size
GetAiCoreUbBankSize()   // UB bank size
GetAiCoreUbBankNum()    // UB bank count
GetAiCoreDdrRate()      // DDR bandwidth
GetAiCoreL2Rate()       // L2 bandwidth
GetSocVersion()         // SoC version string
GetAicVersion()         // AIC version string
```

### CANN Runtime APIs Used (from `runtime.h`)

```cpp
// Initialization
aclInit(nullptr)
aclFinalize()

// Device
rtSetDevice(deviceId)
rtGetDevice(&deviceId)
rtResetDevice(deviceId)
rtGetLogicDevIdByUserDevId(userDevId, &logicDevId)

// Memory
rtMalloc(&ptr, size, RT_MEMORY_HBM | RT_MEMORY_POLICY_HUGE1G_PAGE_ONLY, 0)
rtMalloc(&ptr, size, RT_MEMORY_HBM | RT_MEMORY_POLICY_HUGE_PAGE_FIRST, 0)
rtFree(ptr)
rtMemcpy(dst, dstSize, src, srcSize, RT_MEMCPY_HOST_TO_DEVICE)
rtMemcpy(dst, dstSize, src, srcSize, RT_MEMCPY_DEVICE_TO_HOST)
rtMemcpyAsync(dst, dstSize, src, srcSize, kind, stream)

// Stream
rtStreamCreate(&stream, RT_STREAM_PRIORITY_DEFAULT)
rtStreamDestroy(stream)
rtStreamSynchronize(stream)

// L2 Cache
rtGetL2CacheOffset(deviceId, &offset)
```

### Task Dispatch (from `aicore_manager.h`, `device_machine.h`)

```
Host (A)
  │
  └── AICPU (B/D) - runs DeviceMachine
        ├── AiCoreManager[0] ─── manages AIC[0..n], AIV[0..m]
        ├── AiCoreManager[1] ─── manages AIC[n..p], AIV[m..q]
        └── ...
              │
              └── Task Queues (SPSC, lock-free)
                    ├── readyRegQueues_[core] - send task to AICORE
                    └── finishRegQueues_[core] - receive completion
```

### Hardware Register Access (from `aicore_hal.h`)

```cpp
// Memory-mapped register access
ReadReg32(coreIdx, offset)   // Read 32-bit register
WriteReg32(coreIdx, offset, val)  // Write 32-bit register

// Task dispatch via registers
SetReadyQueue(coreIdx, taskId)   // Send task to AICORE
GetFinishedTask(coreIdx)         // Read completion status
```

### Key Data Structures

```cpp
// Device arguments passed to AICPU
struct DeviceArgs {
    uint64_t sharedBuffer;
    int64_t taskData;
    int64_t *coreRegAddr;    // Memory-mapped register addresses
    uint32_t nrAic;          // Number of AIC cores
    uint32_t nrAiv;          // Number of AIV cores
    ArchInfo archInfo;       // DAV_2201 or DAV_3510
    // ...
};

// Task sent to AICORE
struct DeviceTask {
    uint64_t coreFunctionCnt;
    uint64_t coreFunctionReadyStateAddr;
    uint64_t readyAicCoreFunctionQue;
    uint64_t readyAivCoreFunctionQue;
    CoreFuncData coreFuncData;
};
```

---

## Revised Platform API (Enhanced)

Based on PyPTO findings, expand platform.so to expose hardware details:

```cpp
// === Device Info ===
typedef struct {
    const char* soc_version;    // "Ascend910C"
    uint32_t npu_arch;          // DAV_3510
    uint32_t aicore_cnt;        // Total AICORE blocks
    uint32_t aic_cnt;           // Cube cores
    uint32_t aiv_cnt;           // Vector cores
    uint32_t aicpu_cnt;         // AICPU count
    size_t hbm_size;            // Total HBM
    size_t l2_size;             // L2 cache size
    size_t ub_size;             // Unified Buffer per core
    size_t l0a_size;            // L0A buffer size
    size_t l0b_size;            // L0B buffer size
    size_t l0c_size;            // L0C buffer size
} PlatformDeviceInfo;

int platform_get_device_info(PlatformDeviceInfo* info);

// === Memory (HugePages support) ===
typedef enum {
    PLATFORM_MEM_DEFAULT,       // 2MB pages
    PLATFORM_MEM_HUGE_1G,       // 1GB pages
} PlatformMemPolicy;

void* platform_malloc_ex(size_t size, PlatformMemPolicy policy);
uint64_t platform_get_l2_offset();  // For L2 cache bypass
```

---

## Revised Implementation Phases

| Phase | Goal | What to Teach |
|-------|------|---------------|
| 1 | Device Query | SoC info, core counts, memory sizes |
| 2 | Memory | HBM alloc, HugePages, H2D/D2H transfer |
| 3 | Streams | Create, sync, async operations |
| 4 | AICORE Launch | Load .o, launch to AIC/AIV cores |
| 5 | AICPU Launch | Load .so, launch control tasks |
| 6 | Multi-kernel | Task queues, dependencies, sync |
| 7 | Registers | Direct register access (advanced) |
