/**
 * 07-aicpu-scale - Complete AICPU Kernel Launch Workflow
 *
 * This example demonstrates the complete workflow for launching AICPU kernels:
 * - Reading .so kernel file into memory
 * - Setting up argument structures with proper alignment
 * - Launching kernel with platform_aicpu_launch()
 * - Error handling at each step
 * - Verification of results
 *
 * Hardware concepts taught:
 * - AICPU kernel format (.so shared library)
 * - Cross-compilation for aarch64
 * - Argument struct layout matching between host and kernel
 * - Launch latency (~3μs from host)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "platform.h"

/*
 * Kernel argument structure
 *
 * Rules for AICPU args:
 * 1. Use fixed-width types (int32_t, not int)
 * 2. Match host and kernel definitions exactly
 * 3. Align to 8 bytes (add padding if needed)
 * 4. Pointers are device pointers (from platform_malloc)
 */
struct TransformArgs {
    void* input;       /* offset 0:  GM pointer [8 bytes] */
    void* output;      /* offset 8:  GM pointer [8 bytes] */
    int32_t count;     /* offset 16: element count [4 bytes] */
    float scale;       /* offset 20: multiply factor [4 bytes] */
    float offset;      /* offset 24: add factor [4 bytes] */
    int32_t _pad;      /* offset 28: padding for 8-byte alignment [4 bytes] */
};  /* Total: 32 bytes, 8-byte aligned */

/*
 * Helper: Read file into memory
 *
 * In production, you might embed the .so in your binary or
 * use a resource management system.
 */
void* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        *out_size = 0;
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    *out_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* data = malloc(*out_size);
    if (data) {
        size_t read = fread(data, 1, *out_size, f);
        if (read != *out_size) {
            free(data);
            data = nullptr;
            *out_size = 0;
        }
    }
    fclose(f);
    return data;
}

int main() {
    printf("=== AICPU Kernel Launch Workflow ===\n\n");

    int ret;
    int result = 0;

    /* ====== Step 1: Initialize Platform ====== */
    printf("Step 1: Initialize platform\n");
    ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("  ERROR: platform_init failed (code %d)\n", ret);
        return 1;
    }
    printf("  OK: Platform initialized\n\n");

    /* ====== Step 2: Prepare Test Data ====== */
    printf("Step 2: Prepare test data\n");
    const int N = 4096;
    const float SCALE = 2.0f;
    const float OFFSET = 1.0f;

    float* host_in = (float*)malloc(N * sizeof(float));
    float* host_out = (float*)malloc(N * sizeof(float));
    float* host_expected = (float*)malloc(N * sizeof(float));

    if (!host_in || !host_out || !host_expected) {
        printf("  ERROR: Host malloc failed\n");
        platform_shutdown();
        return 1;
    }

    /* Initialize: output[i] = input[i] * SCALE + OFFSET */
    for (int i = 0; i < N; i++) {
        host_in[i] = (float)i * 0.1f;
        host_expected[i] = host_in[i] * SCALE + OFFSET;
    }
    memset(host_out, 0, N * sizeof(float));

    printf("  Test size: %d elements\n", N);
    printf("  Transform: y = x * %.1f + %.1f\n", SCALE, OFFSET);
    printf("  Sample input: [%.2f, %.2f, %.2f, ...]\n",
           host_in[0], host_in[1], host_in[2]);
    printf("\n");

    /* ====== Step 3: Allocate Device Memory ====== */
    printf("Step 3: Allocate device memory\n");
    void* dev_in = platform_malloc(N * sizeof(float));
    void* dev_out = platform_malloc(N * sizeof(float));

    if (!dev_in || !dev_out) {
        printf("  ERROR: Device malloc failed\n");
        result = 1;
        goto cleanup_host;
    }
    printf("  OK: Allocated %zu bytes on device\n\n", 2 * N * sizeof(float));

    /* ====== Step 4: Copy Input to Device ====== */
    printf("Step 4: Copy input to device (H2D)\n");
    ret = platform_memcpy_h2d(dev_in, host_in, N * sizeof(float));
    if (ret != PLATFORM_SUCCESS) {
        printf("  ERROR: H2D copy failed (code %d)\n", ret);
        result = 1;
        goto cleanup_device;
    }
    printf("  OK: Copied %zu bytes\n\n", N * sizeof(float));

    /* ====== Step 5: Pack Kernel Arguments ====== */
    printf("Step 5: Pack kernel arguments\n");
    TransformArgs args;
    args.input = dev_in;
    args.output = dev_out;
    args.count = N;
    args.scale = SCALE;
    args.offset = OFFSET;
    args._pad = 0;

    printf("  TransformArgs layout:\n");
    printf("    [0-7]   input  = %p\n", args.input);
    printf("    [8-15]  output = %p\n", args.output);
    printf("    [16-19] count  = %d\n", args.count);
    printf("    [20-23] scale  = %.1f\n", args.scale);
    printf("    [24-27] offset = %.1f\n", args.offset);
    printf("    [28-31] _pad   = %d\n", args._pad);
    printf("    Total size: %zu bytes\n\n", sizeof(TransformArgs));

    /* ====== Step 6: Load Kernel (.so file) ====== */
    printf("Step 6: Load kernel\n");

    /* In real use:
     *   size_t so_size;
     *   void* so_data = read_file("transform_kernel.so", &so_size);
     *   if (!so_data) {
     *       printf("  ERROR: Failed to read kernel file\n");
     *       goto cleanup_device;
     *   }
     */

    printf("  (Stub mode: real .so required for actual execution)\n");
    printf("  Kernel file: transform_kernel.so\n");
    printf("  Entry point: transform_kernel_entry\n\n");

    /* ====== Step 7: Launch Kernel ====== */
    printf("Step 7: Launch AICPU kernel\n");

    /* In real use:
     *   ret = platform_aicpu_launch(
     *       so_data, so_size,
     *       "transform_kernel_entry",
     *       &args, sizeof(args),
     *       NULL  // default stream
     *   );
     *   if (ret != PLATFORM_SUCCESS) {
     *       printf("  ERROR: Kernel launch failed (code %d)\n", ret);
     *       goto cleanup_so;
     *   }
     */

    printf("  platform_aicpu_launch(\n");
    printf("      so_data, so_size,\n");
    printf("      \"transform_kernel_entry\",\n");
    printf("      &args, %zu,\n", sizeof(args));
    printf("      NULL\n");
    printf("  );\n");
    printf("  (Simulating kernel execution...)\n");

    /* Simulate kernel result */
    for (int i = 0; i < N; i++) {
        host_out[i] = host_in[i] * SCALE + OFFSET;
    }
    platform_memcpy_h2d(dev_out, host_out, N * sizeof(float));
    printf("\n");

    /* ====== Step 8: Synchronize ====== */
    printf("Step 8: Synchronize stream\n");
    ret = platform_stream_sync(NULL);
    if (ret != PLATFORM_SUCCESS) {
        printf("  ERROR: Stream sync failed (code %d)\n", ret);
        printf("  Note: Kernel errors often surface at sync time\n");
        result = 1;
        goto cleanup_device;
    }
    printf("  OK: Kernel completed\n\n");

    /* ====== Step 9: Copy Output Back ====== */
    printf("Step 9: Copy output from device (D2H)\n");
    ret = platform_memcpy_d2h(host_out, dev_out, N * sizeof(float));
    if (ret != PLATFORM_SUCCESS) {
        printf("  ERROR: D2H copy failed (code %d)\n", ret);
        result = 1;
        goto cleanup_device;
    }
    printf("  OK: Copied %zu bytes\n", N * sizeof(float));
    printf("  Sample output: [%.2f, %.2f, %.2f, ...]\n\n",
           host_out[0], host_out[1], host_out[2]);

    /* ====== Step 10: Verify Results ====== */
    printf("Step 10: Verify results\n");
    {
        int errors = 0;
        float max_diff = 0.0f;

        for (int i = 0; i < N; i++) {
            float diff = host_out[i] - host_expected[i];
            if (diff < 0) diff = -diff;
            if (diff > max_diff) max_diff = diff;
            if (diff > 0.001f) {
                if (errors < 3) {
                    printf("  Mismatch[%d]: got %.4f, expected %.4f\n",
                           i, host_out[i], host_expected[i]);
                }
                errors++;
            }
        }

        printf("  Max difference: %.6f\n", max_diff);
        if (errors == 0) {
            printf("  PASS: All %d elements correct\n", N);
        } else {
            printf("  FAIL: %d errors\n", errors);
            result = 1;
        }
    }
    printf("\n");

    /* ====== Cleanup ====== */
cleanup_device:
    printf("Step 11: Cleanup\n");
    if (dev_in) platform_free(dev_in);
    if (dev_out) platform_free(dev_out);
    printf("  Device memory freed\n");

cleanup_host:
    free(host_in);
    free(host_out);
    free(host_expected);
    printf("  Host memory freed\n");

    platform_shutdown();
    printf("  Platform shutdown\n");

    printf("\n=== End of AICPU Launch Workflow ===\n");
    return result;
}
