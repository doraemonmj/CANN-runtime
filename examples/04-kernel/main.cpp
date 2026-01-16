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
#include "acl/acl.h"

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

    /* Step 1: Initialize ACL runtime */
    printf("Step 1: Initialize ACL runtime...\n");
    aclError ret = aclInit(nullptr);
    if (ret != ACL_SUCCESS) {
        printf("Failed to initialize ACL (error: %d)\n", ret);
        return 1;
    }

    int32_t device_id = 0;
    ret = aclrtSetDevice(device_id);
    if (ret != ACL_SUCCESS) {
        printf("Failed to set device (error: %d)\n", ret);
        aclFinalize();
        return 1;
    }

    aclrtContext context;
    ret = aclrtCreateContext(&context, device_id);
    if (ret != ACL_SUCCESS) {
        printf("Failed to create context (error: %d)\n", ret);
        aclrtResetDevice(device_id);
        aclFinalize();
        return 1;
    }
    printf("  ACL initialized, device %d, context created\n\n", device_id);

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

    /* Step 3: Load kernel (educational - no actual loading needed for stub) */
    printf("Step 3: Load AICORE kernel...\n");
    printf("  In real usage:\n");
    printf("    - Use aclrtLoadKernel() for dynamic kernel loading, OR\n");
    printf("    - Use aclmdlLoadFromFile() for offline model format\n");
    printf("  Kernel binary: %p\n", DUMMY_KERNEL_BIN);
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
    void* dev_a = nullptr;
    void* dev_b = nullptr;
    void* dev_c = nullptr;

    ret = aclrtMalloc(&dev_a, N * sizeof(float), ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate dev_a (error: %d)\n", ret);
        aclrtDestroyContext(context);
        aclrtResetDevice(device_id);
        aclFinalize();
        return 1;
    }

    ret = aclrtMalloc(&dev_b, N * sizeof(float), ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate dev_b (error: %d)\n", ret);
        aclrtFree(dev_a);
        aclrtDestroyContext(context);
        aclrtResetDevice(device_id);
        aclFinalize();
        return 1;
    }

    ret = aclrtMalloc(&dev_c, N * sizeof(float), ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate dev_c (error: %d)\n", ret);
        aclrtFree(dev_a);
        aclrtFree(dev_b);
        aclrtDestroyContext(context);
        aclrtResetDevice(device_id);
        aclFinalize();
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

    /* Step 5: Launch kernel (educational - stub only) */
    printf("Step 5: Launch kernel...\n");
    printf("  Launch parameters:\n");
    printf("    blocks = 1     (number of AICORE blocks to use)\n");
    printf("    args   = %p    (packed argument struct)\n", &args);
    printf("    size   = %zu   (argument struct size)\n", sizeof(args));
    printf("    stream = NULL  (default stream)\n\n");

    uint32_t blocks = 1;  /* Use 1 AICORE block */
    printf("  In real usage:\n");
    printf("    - aclrtLaunchKernel() for custom kernels\n");
    printf("    - aclmdlExecute() for offline models\n");
    printf("  Kernel launch (stub - no actual execution)\n");

    /* Step 6: Synchronize */
    printf("\nStep 6: Synchronize...\n");
    ret = aclrtSynchronizeStream(nullptr);
    if (ret != ACL_SUCCESS) {
        printf("  Stream sync failed (error: %d)\n", ret);
    } else {
        printf("  Default stream synchronized\n\n");
    }

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
    aclrtFree(dev_a);
    aclrtFree(dev_b);
    aclrtFree(dev_c);
    aclrtDestroyContext(context);
    aclrtResetDevice(device_id);
    aclFinalize();
    printf("  Resources freed\n");

    printf("\n=== End of Kernel Launch Interface ===\n");
    return 0;
}
