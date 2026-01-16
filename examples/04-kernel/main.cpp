/**
 * 04-kernel - Ascend NPU Kernel Launch Interface
 *
 * This example demonstrates:
 * - Loading a kernel from binary
 * - Kernel launch parameters (blocks, arguments)
 * - AICORE vs AICPU kernel types
 * - Argument packing for kernel launch
 *
 * Hardware concepts taught:
 * - AICORE: Matrix/Vector compute cores (24 blocks on A2/A3)
 *   - Each AICORE has: Cube unit (matrix), Vector unit, Scalar unit
 *   - Executes compiled TIK/Ascend C kernels (.o binary format)
 * - AICPU: Control processors for complex operations
 *   - Executes regular ARM code (.so shared library format)
 *   - Used for ops that don't fit AICORE (dynamic shapes, control flow)
 *
 * Note: This example uses stub implementations. Real kernel execution
 * requires compiled kernel binaries from TIK or Ascend C toolchains.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "platform.h"

/* Simulated kernel binary (in real use, this would be .o file contents) */
static const char DUMMY_KERNEL_BIN[] = "AICORE_KERNEL_BINARY_PLACEHOLDER";

/* Kernel argument structure - must match kernel's expected layout */
struct VecAddArgs {
    void* a;        /* Input tensor A (device pointer) */
    void* b;        /* Input tensor B (device pointer) */
    void* c;        /* Output tensor C (device pointer) */
    int32_t n;      /* Number of elements */
};

int main() {
    printf("=== Ascend NPU Kernel Launch Interface ===\n\n");

    /* Step 1: Initialize platform */
    printf("Step 1: Initialize platform...\n");
    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Failed to initialize platform (error: %d)\n", ret);
        return 1;
    }
    printf("  Platform initialized\n\n");

    /* Step 2: Explain kernel types */
    printf("Step 2: Kernel Types on Ascend NPU\n");
    printf("  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  AICORE Kernels (.o binary)                         │\n");
    printf("  │  - Run on Cube/Vector compute cores                 │\n");
    printf("  │  - High throughput for matrix/vector ops            │\n");
    printf("  │  - Written in TIK or Ascend C                       │\n");
    printf("  │  - Fixed data flow, static shapes preferred         │\n");
    printf("  ├─────────────────────────────────────────────────────┤\n");
    printf("  │  AICPU Kernels (.so shared library)                 │\n");
    printf("  │  - Run on ARM control processors                    │\n");
    printf("  │  - Flexible for complex logic                       │\n");
    printf("  │  - Written in standard C/C++                        │\n");
    printf("  │  - Dynamic shapes, control flow support             │\n");
    printf("  └─────────────────────────────────────────────────────┘\n\n");

    /* Step 3: Load kernel */
    printf("Step 3: Load AICORE kernel...\n");
    PlatformKernel kernel = platform_kernel_load(
        DUMMY_KERNEL_BIN,
        sizeof(DUMMY_KERNEL_BIN),
        "vec_add_kernel"
    );
    if (!kernel) {
        printf("  Failed to load kernel\n");
        platform_shutdown();
        return 1;
    }
    printf("  Kernel loaded: %p\n", kernel);
    printf("  Entry point: vec_add_kernel\n\n");

    /* Step 4: Prepare kernel arguments */
    printf("Step 4: Prepare kernel arguments...\n");
    printf("  Argument layout (VecAddArgs):\n");
    printf("    offset 0:  void* a      (input A)\n");
    printf("    offset 8:  void* b      (input B)\n");
    printf("    offset 16: void* c      (output C)\n");
    printf("    offset 24: int32_t n    (element count)\n");
    printf("  Total size: %zu bytes\n\n", sizeof(VecAddArgs));

    /* Allocate device memory for tensors */
    const int N = 1024;
    void* dev_a = platform_malloc(N * sizeof(float));
    void* dev_b = platform_malloc(N * sizeof(float));
    void* dev_c = platform_malloc(N * sizeof(float));

    if (!dev_a || !dev_b || !dev_c) {
        printf("  Failed to allocate device memory\n");
        platform_kernel_unload(kernel);
        platform_shutdown();
        return 1;
    }

    /* Pack arguments into struct */
    VecAddArgs args;
    args.a = dev_a;
    args.b = dev_b;
    args.c = dev_c;
    args.n = N;

    printf("  Arguments packed:\n");
    printf("    a = %p\n", args.a);
    printf("    b = %p\n", args.b);
    printf("    c = %p\n", args.c);
    printf("    n = %d\n\n", args.n);

    /* Step 5: Launch kernel */
    printf("Step 5: Launch kernel...\n");
    printf("  Launch parameters:\n");
    printf("    blocks = 1     (number of AICORE blocks to use)\n");
    printf("    args   = %p    (packed argument struct)\n", &args);
    printf("    size   = %zu   (argument struct size)\n", sizeof(args));
    printf("    stream = NULL  (default stream)\n\n");

    uint32_t blocks = 1;  /* Use 1 AICORE block */
    ret = platform_kernel_launch(kernel, blocks, &args, sizeof(args), NULL);
    if (ret != PLATFORM_SUCCESS) {
        printf("  Kernel launch failed (error: %d)\n", ret);
    } else {
        printf("  Kernel launched (stub - no actual execution)\n");
    }

    /* Step 6: Synchronize */
    printf("\nStep 6: Synchronize...\n");
    ret = platform_stream_sync(NULL);
    printf("  Stream synchronized\n\n");

    /* Step 7: Explain block count selection */
    printf("Step 7: Block Count Selection\n");
    printf("  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  How many blocks to use?                            │\n");
    printf("  │                                                     │\n");
    printf("  │  - A2 (910A): 24 AICORE blocks available            │\n");
    printf("  │  - A3 (910C): 24 AICORE blocks available            │\n");
    printf("  │                                                     │\n");
    printf("  │  Guidelines:                                        │\n");
    printf("  │  - Small data: 1 block (reduce overhead)            │\n");
    printf("  │  - Large data: N blocks (parallelize)               │\n");
    printf("  │  - Each block processes: total_size / N             │\n");
    printf("  └─────────────────────────────────────────────────────┘\n\n");

    /* Step 8: Cleanup */
    printf("Step 8: Cleanup...\n");
    platform_free(dev_a);
    platform_free(dev_b);
    platform_free(dev_c);
    platform_kernel_unload(kernel);
    platform_shutdown();
    printf("  Resources freed\n");

    printf("\n=== End of Kernel Launch Interface ===\n");
    return 0;
}
