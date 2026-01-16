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
#include "platform.h"

int main() {
    printf("=== Ascend NPU Stream Operations ===\n\n");

    /* Step 1: Initialize platform */
    printf("Step 1: Initialize platform...\n");
    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Failed to initialize platform (error: %d)\n", ret);
        return 1;
    }
    printf("  Platform initialized\n\n");

    /* Step 2: Use default stream (NULL) */
    printf("Step 2: Default stream...\n");
    printf("  The default stream is created during platform_init()\n");
    printf("  Pass NULL to functions to use the default stream\n");
    ret = platform_stream_sync(NULL);  /* Sync default stream */
    if (ret != PLATFORM_SUCCESS) {
        printf("  Failed to sync default stream (error: %d)\n", ret);
    } else {
        printf("  Default stream synchronized\n");
    }
    printf("\n");

    /* Step 3: Create custom streams */
    printf("Step 3: Create custom streams...\n");
    PlatformStream stream1 = platform_stream_create();
    PlatformStream stream2 = platform_stream_create();

    if (!stream1 || !stream2) {
        printf("  Failed to create streams\n");
        if (stream1) platform_stream_destroy(stream1);
        platform_shutdown();
        return 1;
    }
    printf("  Stream 1: %p\n", stream1);
    printf("  Stream 2: %p\n", stream2);
    printf("\n");

    /* Step 4: Demonstrate stream usage pattern */
    printf("Step 4: Stream usage pattern...\n");
    printf("  Typical workflow:\n");
    printf("    1. Create stream\n");
    printf("    2. Launch async operations (memcpy, kernel) on stream\n");
    printf("    3. Launch more operations (can overlap with step 2)\n");
    printf("    4. Synchronize stream to wait for completion\n");
    printf("\n");

    /* Step 5: Allocate memory and demonstrate async copy */
    printf("Step 5: Async memory operations...\n");
    void* dev_buf1 = platform_malloc(1024);
    void* dev_buf2 = platform_malloc(1024);

    if (!dev_buf1 || !dev_buf2) {
        printf("  Failed to allocate device memory\n");
        platform_stream_destroy(stream1);
        platform_stream_destroy(stream2);
        platform_shutdown();
        return 1;
    }

    /* In a real scenario, we would use async versions of memcpy */
    /* For now, we demonstrate the synchronization pattern */
    printf("  Allocated 2 buffers on device\n");
    printf("  (Note: Full async memcpy requires additional APIs)\n\n");

    /* Step 6: Synchronize streams */
    printf("Step 6: Synchronize streams...\n");
    ret = platform_stream_sync(stream1);
    printf("  Stream 1 sync: %s\n", ret == PLATFORM_SUCCESS ? "OK" : "FAILED");

    ret = platform_stream_sync(stream2);
    printf("  Stream 2 sync: %s\n", ret == PLATFORM_SUCCESS ? "OK" : "FAILED");
    printf("\n");

    /* Step 7: Cleanup */
    printf("Step 7: Cleanup...\n");
    platform_free(dev_buf1);
    platform_free(dev_buf2);
    platform_stream_destroy(stream1);
    platform_stream_destroy(stream2);
    platform_shutdown();
    printf("  All resources freed\n");

    printf("\n=== End of Stream Operations ===\n");
    return 0;
}
