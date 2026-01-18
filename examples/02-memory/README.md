# Concept: HBM Memory Operations

## What This Demonstrates
Allocating High Bandwidth Memory (HBM) on the device and performing host-to-device (H2D) and device-to-host (D2H) data transfers.

## Hardware Behavior
The Ascend NPU has its own High Bandwidth Memory (HBM) physically located on the accelerator card. When you call `aclrtMalloc()`, you're allocating space in this device memory, which is separate from host RAM. Transfers between host and device memory happen via DMA (Direct Memory Access) over PCIe.

The memory hierarchy in Ascend NPUs:
- **Host RAM**: System memory managed by the CPU
- **HBM**: Device DRAM (32-64GB), accessed via PCIe from host
- **L2 Cache**: Shared 192MB cache across all AIC units
- **L1/UB**: Per-core buffers for compute operations

Data must be in HBM before kernels can operate on it. The `aclrtMemcpy()` function performs synchronous DMA transfers, blocking until the data copy completes.

## Code Structure
- [`main.cpp`](main.cpp) - Host-side code demonstrating HBM allocation and H2D/D2H transfers

## Key CANN APIs
- `aclInit()` - Initializes the ACL runtime (see [main.cpp:29](main.cpp#L29))
- `aclrtSetDevice()` - Sets the active NPU device (see [main.cpp:36](main.cpp#L36))
- `aclrtCreateContext()` - Creates a device context for operations (see [main.cpp:45](main.cpp#L45))
- `aclrtMalloc()` - Allocates HBM memory on the device (see [main.cpp:85](main.cpp#L85))
  - `ACL_MEM_MALLOC_HUGE_FIRST` - Allocates using 2MB huge pages for better TLB efficiency
  - Returns a device pointer (not directly accessible from host code)
- `aclrtMemcpy()` - **Synchronously** copies data between host and device, blocking until transfer completes (see [main.cpp:106](main.cpp#L106) and [main.cpp:128](main.cpp#L128))
  - `ACL_MEMCPY_HOST_TO_DEVICE` - Host → Device transfer via DMA over PCIe
  - `ACL_MEMCPY_DEVICE_TO_HOST` - Device → Host transfer via DMA over PCIe
  - For asynchronous transfers, see `aclrtMemcpyAsync()` in example 03-stream
- `aclrtFree()` - Frees device memory (see [main.cpp:162](main.cpp#L162))
- `aclrtDestroyContext()` - Destroys the device context (see [main.cpp:165](main.cpp#L165))
- `aclrtResetDevice()` - Resets the device (see [main.cpp:166](main.cpp#L166))
- `aclFinalize()` - Cleans up ACL runtime (see [main.cpp:167](main.cpp#L167))

## Running
```bash
export ASCEND_HOME_PATH=/usr/local/Ascend/latest  # or your CANN installation path
mkdir build && cd build
cmake ..
make
./02-memory
```

## Expected Output
```
=== Ascend NPU Memory Operations ===

Step 1: Initialize ACL runtime...
  ACL initialized on device 0

Step 2: Prepare host data...
  Allocated 4096 bytes on host
  Source data: [0.0, 0.5, 1.0, ... 511.5]

Step 3: Allocate device memory...
  Allocated 4096 bytes on device (HBM)
  Device pointer: 0x108000000

Step 4: Copy host -> device (H2D)...
  Copied 4096 bytes to device

Step 5: Copy device -> host (D2H)...
  Copied 4096 bytes from device

Step 6: Verify data...
  Result data: [0.0, 0.5, 1.0, ... 511.5]
  PASS: All 1024 elements match!

Step 7: Cleanup...
  Resources freed

=== End of Memory Operations ===
```

## Next Steps
See [`../03-stream/`](../03-stream/) to learn about asynchronous execution using streams.
