# Concept: Stream Management

## What This Demonstrates
Creating, using, and synchronizing ACL streams for asynchronous device operations.

## Hardware Behavior
Streams are command queues that enable asynchronous execution on the NPU. When you launch operations (memory copies, kernel launches) on a stream, they are queued and executed in order on the device, while the host CPU continues immediately. Multiple streams can execute concurrently, enabling parallelism.

Each stream maintains FIFO ordering: operations submitted to a stream execute in the order they were enqueued. However, operations in different streams may execute in parallel if hardware resources are available. Synchronizing a stream blocks the host CPU until all operations in that stream complete.

This pattern is essential for:
- **Overlapping computation and data transfer**: One stream copies data while another runs kernels
- **Concurrent kernel execution**: Multiple AICORE blocks can run different kernels simultaneously
- **Pipeline execution**: Keep the hardware busy by queuing work ahead of time

## Code Structure
- [`main.cpp`](main.cpp) - Host-side code demonstrating stream creation, usage, and synchronization

## Key CANN APIs
- `aclrtCreateStream()` - Creates a new stream for async operations (see [main.cpp:61](main.cpp#L61), [main.cpp:83](main.cpp#L83), [main.cpp:93](main.cpp#L93))
- `aclrtSynchronizeStream()` - Blocks until all operations in the stream complete (see [main.cpp:164](main.cpp#L164), [main.cpp:167](main.cpp#L167), [main.cpp:170](main.cpp#L170))
- `aclrtDestroyStream()` - Destroys a stream and releases its resources (see [main.cpp:178](main.cpp#L178))

Streams are used with async variants of other APIs:
- `aclrtMemcpyAsync()` - Asynchronous H2D/D2H copy on a stream
- Kernel launch APIs (later examples) - Execute kernels on a stream

## Running
```bash
export ASCEND_HOME_PATH=/usr/local/Ascend/latest  # or your CANN installation path
mkdir build && cd build
cmake ..
make
./03-stream
```

## Expected Output
```
=== Ascend NPU Stream Operations ===

Step 1: Initialize ACL runtime...
  ACL initialized

Step 2: Create default stream...
  Default stream: 0x7f8c8000

Step 3: Create custom streams...
  Stream 1: 0x7f8c9000
  Stream 2: 0x7f8ca000

Step 4: Stream usage pattern...
  Typical workflow:
    1. Create stream with aclrtCreateStream()
    2. Launch async operations (memcpy, kernel) on stream
    3. Launch more operations (can overlap with step 2)
    4. Synchronize stream with aclrtSynchronizeStream()

Step 5: Allocate device memory...
  Allocated 2 buffers on device
  (Async memcpy would use aclrtMemcpyAsync with streams)

Step 6: Synchronize streams...
  Stream 1 sync: OK
  Stream 2 sync: OK
  Default stream sync: OK

Step 7: Cleanup...
  All resources freed

=== End of Stream Operations ===
```

## Next Steps
See [`../04-aicpu-basic/`](../04-aicpu-basic/) to learn about kernel loading and execution with real AICPU kernel code.
