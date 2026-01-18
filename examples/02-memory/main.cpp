/**
 * 02-memory - Ascend NPU Memory Operations
 *
 * This example demonstrates:
 * - Initializing ACL runtime and creating device context
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
#include "acl/acl.h"

#define ARRAY_SIZE 1024

int main() {
    printf("=== Ascend NPU Memory Operations ===\n\n");

    /* Step 1: Initialize ACL runtime and set device */
    printf("Step 1: Initialize ACL runtime...\n");
    aclError ret = aclInit(nullptr);
    if (ret != ACL_SUCCESS) {
        printf("Failed to initialize ACL (error: %d)\n", ret);
        printf("Note: This example requires CANN runtime and NPU hardware.\n");
        return 1;
    }

    ret = aclrtSetDevice(0);
    if (ret != ACL_SUCCESS) {
        printf("Failed to set device 0 (error: %d)\n", ret);
        aclFinalize();
        return 1;
    }

    /* Create context for device operations */
    aclrtContext context;
    ret = aclrtCreateContext(&context, 0);
    if (ret != ACL_SUCCESS) {
        printf("Failed to create context (error: %d)\n", ret);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }
    printf("  ACL initialized on device 0\n\n");

    /* Step 2: Allocate host memory and initialize data */
    printf("Step 2: Prepare host data...\n");
    float* host_src = (float*)malloc(ARRAY_SIZE * sizeof(float));
    float* host_dst = (float*)malloc(ARRAY_SIZE * sizeof(float));

    if (!host_src || !host_dst) {
        printf("Failed to allocate host memory\n");
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
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

    /* Step 3: Allocate device (HBM) memory
     *
     * Hardware: HBM is physically separate from host RAM, located on the NPU card.
     * The ACL_MEM_MALLOC_HUGE_FIRST flag requests 2MB huge pages for better TLB
     * efficiency. Memory is allocated from the device's HBM pool (32-64GB capacity).
     */
    printf("Step 3: Allocate device memory...\n");
    void* dev_buf = nullptr;
    ret = aclrtMalloc(&dev_buf, ARRAY_SIZE * sizeof(float), ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("Failed to allocate device memory (error: %d)\n", ret);
        free(host_src);
        free(host_dst);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }
    printf("  Allocated %zu bytes on device (HBM)\n", ARRAY_SIZE * sizeof(float));
    printf("  Device pointer: %p\n\n", dev_buf);

    /* Step 4: Copy data from host to device
     *
     * Hardware: Data transfer happens via DMA (Direct Memory Access) over PCIe.
     * The PCIe link provides ~32 GB/s bandwidth (PCIe 4.0 x16). aclrtMemcpy() is
     * SYNCHRONOUS - it blocks until the DMA transfer completes. For async transfers,
     * use aclrtMemcpyAsync() with streams (see example 03-stream).
     */
    printf("Step 4: Copy host -> device (H2D)...\n");
    ret = aclrtMemcpy(dev_buf, ARRAY_SIZE * sizeof(float),
                      host_src, ARRAY_SIZE * sizeof(float),
                      ACL_MEMCPY_HOST_TO_DEVICE);
    if (ret != ACL_SUCCESS) {
        printf("H2D copy failed (error: %d)\n", ret);
        aclrtFree(dev_buf);
        free(host_src);
        free(host_dst);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }
    printf("  Copied %zu bytes to device\n\n", ARRAY_SIZE * sizeof(float));

    /* Step 5: Copy data from device to host
     *
     * Hardware: Reverse DMA transfer from HBM back to host RAM via PCIe.
     * Same synchronous behavior as H2D - blocks until transfer completes.
     * In production, minimize D2H transfers as they're bandwidth-limited.
     */
    printf("Step 5: Copy device -> host (D2H)...\n");
    ret = aclrtMemcpy(host_dst, ARRAY_SIZE * sizeof(float),
                      dev_buf, ARRAY_SIZE * sizeof(float),
                      ACL_MEMCPY_DEVICE_TO_HOST);
    if (ret != ACL_SUCCESS) {
        printf("D2H copy failed (error: %d)\n", ret);
        aclrtFree(dev_buf);
        free(host_src);
        free(host_dst);
        aclrtDestroyContext(context);
        aclrtResetDevice(0);
        aclFinalize();
        return 1;
    }
    printf("  Copied %zu bytes from device\n\n", ARRAY_SIZE * sizeof(float));

    /* Step 6: Verify data
     *
     * Round-trip verification: host → device → host. If data matches, we've
     * confirmed that HBM allocation and DMA transfers work correctly. This is
     * a common pattern for validating memory operations.
     */
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
    aclrtFree(dev_buf);
    free(host_src);
    free(host_dst);
    aclrtDestroyContext(context);
    aclrtResetDevice(0);
    aclFinalize();
    printf("  Resources freed\n");

    printf("\n=== End of Memory Operations ===\n");
    return errors > 0 ? 1 : 0;
}
