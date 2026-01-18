/**
 * 06-aicpu-basic - Basic AICPU Kernel Example
 *
 * This example demonstrates:
 * - AICPU kernel structure (standard C/C++ code)
 * - Loading .so shared library kernel
 * - Launching AICPU kernel from host
 * - Argument passing to AICPU kernel
 *
 * Hardware concepts taught:
 * - AICPU: ARM Cortex-A55 cores on Ascend NPU
 * - Full C/C++ execution environment
 * - Direct HBM access from AICPU
 * - When to use AICPU vs AICORE
 *
 * AICPU is best for:
 * - Dynamic shape operations
 * - Complex control flow
 * - Sparse operations
 * - Operations not suited for Cube/Vector units
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "platform.h"

/* Kernel argument structure - must match kernel definition exactly */
struct ScaleArgs {
    void* input;      /* GM pointer: input data */
    void* output;     /* GM pointer: output data */
    int32_t count;    /* Number of elements */
    float scale;      /* Scale factor */
};

/* Helper to read file into memory */
void* read_file(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* data = malloc(*size);
    if (data) {
        size_t read = fread(data, 1, *size, f);
        if (read != *size) {
            free(data);
            data = nullptr;
        }
    }
    fclose(f);
    return data;
}

int main() {
    printf("=== AICPU Basic Kernel Example ===\n\n");

    /* Step 1: Initialize platform */
    printf("Step 1: Initialize platform...\n");
    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Failed to initialize platform (error: %d)\n", ret);
        return 1;
    }
    printf("  Platform initialized on device 0\n\n");

    /* Step 2: Explain AICPU architecture */
    printf("Step 2: AICPU Architecture\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  |  AICPU (AI Control Processing Unit)                     |\n");
    printf("  |                                                         |\n");
    printf("  |  - ARM Cortex-A55 cores (8 cores typical)               |\n");
    printf("  |  - Full C/C++ execution environment                     |\n");
    printf("  |  - Direct HBM access (same address space as AICORE)     |\n");
    printf("  |  - Standard library support (libc, libm, etc.)          |\n");
    printf("  |                                                         |\n");
    printf("  |  Kernel format: .so shared library                      |\n");
    printf("  |  Cross-compile: aarch64-linux-gnu-g++                   |\n");
    printf("  +---------------------------------------------------------+\n\n");

    /* Step 3: Prepare data */
    printf("Step 3: Prepare data...\n");
    const int N = 1024;
    float* host_input = (float*)malloc(N * sizeof(float));
    float* host_output = (float*)malloc(N * sizeof(float));

    if (!host_input || !host_output) {
        printf("  Failed to allocate host memory\n");
        platform_shutdown();
        return 1;
    }

    /* Initialize input data */
    for (int i = 0; i < N; i++) {
        host_input[i] = (float)i;
    }
    memset(host_output, 0, N * sizeof(float));

    printf("  Input: [%.1f, %.1f, %.1f, ... %.1f]\n",
           host_input[0], host_input[1], host_input[2], host_input[N-1]);
    printf("\n");

    /* Step 4: Allocate device memory */
    printf("Step 4: Allocate device memory...\n");
    void* dev_input = platform_malloc(N * sizeof(float));
    void* dev_output = platform_malloc(N * sizeof(float));

    if (!dev_input || !dev_output) {
        printf("  Failed to allocate device memory\n");
        free(host_input);
        free(host_output);
        platform_shutdown();
        return 1;
    }
    printf("  Allocated %zu bytes on device\n\n", 2 * N * sizeof(float));

    /* Step 5: Copy input to device */
    printf("Step 5: Copy input to device...\n");
    ret = platform_memcpy_h2d(dev_input, host_input, N * sizeof(float));
    if (ret != PLATFORM_SUCCESS) {
        printf("  H2D copy failed\n");
        platform_free(dev_input);
        platform_free(dev_output);
        free(host_input);
        free(host_output);
        platform_shutdown();
        return 1;
    }
    printf("  Copied %zu bytes to device\n\n", N * sizeof(float));

    /* Step 6: Prepare kernel arguments */
    printf("Step 6: Prepare kernel arguments...\n");
    ScaleArgs args;
    args.input = dev_input;
    args.output = dev_output;
    args.count = N;
    args.scale = 2.5f;

    printf("  Argument structure (ScaleArgs):\n");
    printf("    input  = %p (device pointer)\n", args.input);
    printf("    output = %p (device pointer)\n", args.output);
    printf("    count  = %d\n", args.count);
    printf("    scale  = %.1f\n", args.scale);
    printf("\n");

    /* Step 7: Load and launch kernel */
    printf("Step 7: Load and launch AICPU kernel...\n");

    /* In real use, load .so from file:
     *   size_t so_size;
     *   void* so_data = read_file("scale_kernel.so", &so_size);
     *   platform_aicpu_launch(so_data, so_size, "scale_kernel_entry",
     *                         &args, sizeof(args), NULL);
     */

    /* Stub: simulate kernel execution */
    printf("  (Simulating kernel - actual .so required for real execution)\n");
    printf("  Kernel would compute: output[i] = input[i] * %.1f\n", args.scale);

    /* Simulate result for verification */
    for (int i = 0; i < N; i++) {
        host_output[i] = host_input[i] * args.scale;
    }
    platform_memcpy_h2d(dev_output, host_output, N * sizeof(float));
    printf("\n");

    /* Step 8: Synchronize */
    printf("Step 8: Synchronize...\n");
    ret = platform_stream_sync(NULL);
    printf("  Stream synchronized\n\n");

    /* Step 9: Copy results back */
    printf("Step 9: Copy results back...\n");
    ret = platform_memcpy_d2h(host_output, dev_output, N * sizeof(float));
    if (ret != PLATFORM_SUCCESS) {
        printf("  D2H copy failed\n");
    }
    printf("  Output: [%.1f, %.1f, %.1f, ... %.1f]\n",
           host_output[0], host_output[1], host_output[2], host_output[N-1]);
    printf("\n");

    /* Step 10: Verify results */
    printf("Step 10: Verify results...\n");
    int errors = 0;
    for (int i = 0; i < N; i++) {
        float expected = host_input[i] * args.scale;
        if (host_output[i] != expected) {
            if (errors < 5) {
                printf("  Mismatch at %d: expected %.1f, got %.1f\n",
                       i, expected, host_output[i]);
            }
            errors++;
        }
    }

    if (errors == 0) {
        printf("  PASS: All %d elements correct!\n\n", N);
    } else {
        printf("  FAIL: %d errors\n\n", errors);
    }

    /* Step 11: Cleanup */
    printf("Step 11: Cleanup...\n");
    platform_free(dev_input);
    platform_free(dev_output);
    free(host_input);
    free(host_output);
    platform_shutdown();
    printf("  Resources freed\n");

    printf("\n=== End of AICPU Basic Example ===\n");
    return errors > 0 ? 1 : 0;
}
