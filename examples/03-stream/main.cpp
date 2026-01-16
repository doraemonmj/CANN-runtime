/**
 * 03-stream - Ascend NPU Stream Operations
 *
 * This example demonstrates:
 * - Creating and destroying streams
 * - Using default stream vs custom streams
 * - Stream synchronization
 * - Concept of asynchronous execution
 *
 * Hardware concepts taught:
 * - Streams: Command queues for asynchronous device operations
 * - Default stream: Implicit stream for simple programs
 * - Synchronization: Waiting for device operations to complete
 * - Parallelism: Multiple streams can execute concurrently
 */

#include <cstdio>
#include "acl/acl.h"

int main() {
    printf("=== Ascend NPU Stream Operations ===\n\n");

    /* Step 1: Initialize ACL and create context */
    printf("Step 1: Initialize ACL runtime...\n");
    aclError ret = aclInit(nullptr);
    if (ret != ACL_SUCCESS) {
        printf("Failed to initialize ACL (error: %d)\n", ret);
        return 1;
    }

    ret = aclrtSetDevice(0);
    if (ret != ACL_SUCCESS) {
        printf("Failed to set device (error: %d)\n", ret);
        aclFinalize();
        return 1;
    }

    aclrtContext context;
    ret = aclrtCreateContext(&context, 0);
    if (ret != ACL_SUCCESS) {
        printf("Failed to create context (error: %d)\n", ret);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }
    printf("  ACL initialized\n\n");

    /* Step 2: Create default stream */
    printf("Step 2: Create default stream...\n");
    aclrtStream default_stream = nullptr;
    ret = aclrtCreateStream(&default_stream);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to create default stream (error: %d)\n", ret);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }
    printf("  Default stream: %p\n", default_stream);
    printf("\n");

    /* Step 3: Create custom streams */
    printf("Step 3: Create custom streams...\n");
    aclrtStream stream1 = nullptr;
    aclrtStream stream2 = nullptr;

    ret = aclrtCreateStream(&stream1);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to create stream 1 (error: %d)\n", ret);
        aclrtDestroyStream(default_stream);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }

    ret = aclrtCreateStream(&stream2);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to create stream 2 (error: %d)\n", ret);
        aclrtDestroyStream(stream1);
        aclrtDestroyStream(default_stream);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }

    printf("  Stream 1: %p\n", stream1);
    printf("  Stream 2: %p\n", stream2);
    printf("\n");

    /* Step 4: Demonstrate stream usage pattern */
    printf("Step 4: Stream usage pattern...\n");
    printf("  Typical workflow:\n");
    printf("    1. Create stream with aclrtCreateStream()\n");
    printf("    2. Launch async operations (memcpy, kernel) on stream\n");
    printf("    3. Launch more operations (can overlap with step 2)\n");
    printf("    4. Synchronize stream with aclrtSynchronizeStream()\n");
    printf("\n");

    /* Step 5: Allocate memory to demonstrate stream usage */
    printf("Step 5: Allocate device memory...\n");
    void* dev_buf1 = nullptr;
    void* dev_buf2 = nullptr;

    ret = aclrtMalloc(&dev_buf1, 1024, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate buffer 1 (error: %d)\n", ret);
        aclrtDestroyStream(stream2);
        aclrtDestroyStream(stream1);
        aclrtDestroyStream(default_stream);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }

    ret = aclrtMalloc(&dev_buf2, 1024, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate buffer 2 (error: %d)\n", ret);
        aclrtFree(dev_buf1);
        aclrtDestroyStream(stream2);
        aclrtDestroyStream(stream1);
        aclrtDestroyStream(default_stream);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }

    printf("  Allocated 2 buffers on device\n");
    printf("  (Async memcpy would use aclrtMemcpyAsync with streams)\n\n");

    /* Step 6: Synchronize streams */
    printf("Step 6: Synchronize streams...\n");
    ret = aclrtSynchronizeStream(stream1);
    printf("  Stream 1 sync: %s\n", ret == ACL_SUCCESS ? "OK" : "FAILED");

    ret = aclrtSynchronizeStream(stream2);
    printf("  Stream 2 sync: %s\n", ret == ACL_SUCCESS ? "OK" : "FAILED");

    ret = aclrtSynchronizeStream(default_stream);
    printf("  Default stream sync: %s\n", ret == ACL_SUCCESS ? "OK" : "FAILED");
    printf("\n");

    /* Step 7: Cleanup */
    printf("Step 7: Cleanup...\n");
    aclrtFree(dev_buf1);
    aclrtFree(dev_buf2);
    aclrtDestroyStream(stream2);
    aclrtDestroyStream(stream1);
    aclrtDestroyStream(default_stream);
    aclrtDestroyContext(context);
    aclrtResetDevice(0);
    aclFinalize();
    printf("  All resources freed\n");

    printf("\n=== End of Stream Operations ===\n");
    return 0;
}
