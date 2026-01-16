/**
 * 02-memory - Ascend NPU Memory Operations
 *
 * This example demonstrates:
 * - Platform initialization (required for memory operations)
 * - Allocating device (HBM) memory
 * - Copying data from host to device (H2D)
 * - Copying data from device to host (D2H)
 * - Freeing device memory
 *
 * Hardware concepts taught:
 * - HBM (High Bandwidth Memory): Main device memory, ~32-64GB per device
 * - Memory hierarchy: Host RAM -> HBM -> L2 -> L1 -> L0/UB
 * - DMA transfers: Asynchronous copy between host and device
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "platform.h"

#define ARRAY_SIZE 1024

int main() {
    printf("=== Ascend NPU Memory Operations ===\n\n");

    /* Step 1: Initialize platform on device 0 */
    printf("Step 1: Initialize platform...\n");
    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Failed to initialize platform (error: %d)\n", ret);
        printf("Note: This example requires CANN runtime and NPU hardware.\n");
        return 1;
    }
    printf("  Platform initialized on device 0\n\n");

    /* Step 2: Allocate host memory and initialize data */
    printf("Step 2: Prepare host data...\n");
    float* host_src = (float*)malloc(ARRAY_SIZE * sizeof(float));
    float* host_dst = (float*)malloc(ARRAY_SIZE * sizeof(float));

    if (!host_src || !host_dst) {
        printf("Failed to allocate host memory\n");
        platform_shutdown();
        return 1;
    }

    /* Initialize source with pattern */
    for (int i = 0; i < ARRAY_SIZE; i++) {
        host_src[i] = (float)i * 0.5f;
    }
    memset(host_dst, 0, ARRAY_SIZE * sizeof(float));
    printf("  Allocated %zu bytes on host\n", ARRAY_SIZE * sizeof(float));
    printf("  Source data: [%.1f, %.1f, %.1f, ... %.1f]\n",
           host_src[0], host_src[1], host_src[2], host_src[ARRAY_SIZE-1]);
    printf("\n");

    /* Step 3: Allocate device (HBM) memory */
    printf("Step 3: Allocate device memory...\n");
    void* dev_buf = platform_malloc(ARRAY_SIZE * sizeof(float));
    if (!dev_buf) {
        printf("Failed to allocate device memory\n");
        free(host_src);
        free(host_dst);
        platform_shutdown();
        return 1;
    }
    printf("  Allocated %zu bytes on device (HBM)\n", ARRAY_SIZE * sizeof(float));
    printf("  Device pointer: %p\n\n", dev_buf);

    /* Step 4: Copy data from host to device */
    printf("Step 4: Copy host -> device (H2D)...\n");
    ret = platform_memcpy_h2d(dev_buf, host_src, ARRAY_SIZE * sizeof(float));
    if (ret != PLATFORM_SUCCESS) {
        printf("H2D copy failed (error: %d)\n", ret);
        platform_free(dev_buf);
        free(host_src);
        free(host_dst);
        platform_shutdown();
        return 1;
    }
    printf("  Copied %zu bytes to device\n\n", ARRAY_SIZE * sizeof(float));

    /* Step 5: Copy data from device to host */
    printf("Step 5: Copy device -> host (D2H)...\n");
    ret = platform_memcpy_d2h(host_dst, dev_buf, ARRAY_SIZE * sizeof(float));
    if (ret != PLATFORM_SUCCESS) {
        printf("D2H copy failed (error: %d)\n", ret);
        platform_free(dev_buf);
        free(host_src);
        free(host_dst);
        platform_shutdown();
        return 1;
    }
    printf("  Copied %zu bytes from device\n\n", ARRAY_SIZE * sizeof(float));

    /* Step 6: Verify data */
    printf("Step 6: Verify data...\n");
    printf("  Result data: [%.1f, %.1f, %.1f, ... %.1f]\n",
           host_dst[0], host_dst[1], host_dst[2], host_dst[ARRAY_SIZE-1]);

    int errors = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (host_src[i] != host_dst[i]) {
            if (errors < 5) {
                printf("  Mismatch at index %d: expected %.1f, got %.1f\n",
                       i, host_src[i], host_dst[i]);
            }
            errors++;
        }
    }

    if (errors == 0) {
        printf("  PASS: All %d elements match!\n\n", ARRAY_SIZE);
    } else {
        printf("  FAIL: %d mismatches found\n\n", errors);
    }

    /* Step 7: Cleanup */
    printf("Step 7: Cleanup...\n");
    platform_free(dev_buf);
    free(host_src);
    free(host_dst);
    platform_shutdown();
    printf("  Resources freed\n");

    printf("\n=== End of Memory Operations ===\n");
    return errors > 0 ? 1 : 0;
}
