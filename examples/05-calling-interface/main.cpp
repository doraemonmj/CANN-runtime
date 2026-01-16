/**
 * 05-calling-interface - Kernel Calling Interface and Argument Packing
 *
 * This example demonstrates:
 * - Argument struct layout and alignment requirements
 * - Pointer vs value semantics for kernel arguments
 * - Memory address types (Global Memory vs workspace)
 * - How the kernel sees its arguments
 *
 * Hardware concepts taught:
 * - Argument buffer: Contiguous memory block passed to kernel
 * - Alignment: 8-byte alignment for pointers, natural alignment for scalars
 * - Address spaces: GM (Global Memory/HBM) vs local buffers (UB, L1)
 *
 * This is the "contract" between host code and kernel code.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "acl/acl.h"

/* Example 1: Simple kernel args (vector operation) */
struct SimpleArgs {
    void* input;      /* GM pointer: input data */
    void* output;     /* GM pointer: output data */
    int32_t count;    /* Scalar: number of elements */
    int32_t _pad;     /* Padding for 8-byte alignment */
};

/* Example 2: Matrix multiply args */
struct MatMulArgs {
    void* A;          /* GM pointer: matrix A [M x K] */
    void* B;          /* GM pointer: matrix B [K x N] */
    void* C;          /* GM pointer: matrix C [M x N] */
    int32_t M;        /* Scalar: rows of A and C */
    int32_t N;        /* Scalar: cols of B and C */
    int32_t K;        /* Scalar: cols of A, rows of B */
    int32_t _pad;     /* Padding */
};

/* Example 3: Complex args with workspace */
struct ConvArgs {
    void* input;      /* GM pointer: input tensor */
    void* weight;     /* GM pointer: weight tensor */
    void* output;     /* GM pointer: output tensor */
    void* workspace;  /* GM pointer: temporary buffer for intermediate results */
    int32_t batch;
    int32_t in_c;
    int32_t out_c;
    int32_t height;
    int32_t width;
    int32_t kernel_h;
    int32_t kernel_w;
    int32_t _pad;
};

void print_struct_layout(const char* name, size_t size) {
    printf("  struct %s {\n", name);
    printf("    /* Total size: %zu bytes */\n", size);
    printf("  }\n\n");
}

int main() {
    printf("=== Kernel Calling Interface and Argument Packing ===\n\n");

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
    printf("  ACL initialized\n\n");

    /* Step 2: Alignment rules */
    printf("Step 2: Alignment Rules\n");
    printf("  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  Type          Size    Alignment                    │\n");
    printf("  │  ─────────────────────────────────────              │\n");
    printf("  │  void*         8       8                            │\n");
    printf("  │  int64_t       8       8                            │\n");
    printf("  │  int32_t       4       4                            │\n");
    printf("  │  int16_t       2       2                            │\n");
    printf("  │  float         4       4                            │\n");
    printf("  │  half (fp16)   2       2                            │\n");
    printf("  │                                                     │\n");
    printf("  │  Rule: Struct must be 8-byte aligned as a whole     │\n");
    printf("  │  Rule: Add padding to maintain alignment            │\n");
    printf("  └─────────────────────────────────────────────────────┘\n\n");

    /* Step 3: Struct layouts */
    printf("Step 3: Example Struct Layouts\n\n");

    printf("  SimpleArgs (vector operation):\n");
    printf("    offset 0:   void*   input   [8 bytes]\n");
    printf("    offset 8:   void*   output  [8 bytes]\n");
    printf("    offset 16:  int32_t count   [4 bytes]\n");
    printf("    offset 20:  int32_t _pad    [4 bytes] <- padding!\n");
    printf("    Total: %zu bytes\n\n", sizeof(SimpleArgs));

    printf("  MatMulArgs (matrix multiply):\n");
    printf("    offset 0:   void*   A       [8 bytes]\n");
    printf("    offset 8:   void*   B       [8 bytes]\n");
    printf("    offset 16:  void*   C       [8 bytes]\n");
    printf("    offset 24:  int32_t M       [4 bytes]\n");
    printf("    offset 28:  int32_t N       [4 bytes]\n");
    printf("    offset 32:  int32_t K       [4 bytes]\n");
    printf("    offset 36:  int32_t _pad    [4 bytes]\n");
    printf("    Total: %zu bytes\n\n", sizeof(MatMulArgs));

    /* Step 4: Memory address types */
    printf("Step 4: Memory Address Types\n");
    printf("  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  From kernel's perspective:                         │\n");
    printf("  │                                                     │\n");
    printf("  │  GM (Global Memory = HBM):                          │\n");
    printf("  │    - Large, slow (relatively)                       │\n");
    printf("  │    - Visible to all cores                           │\n");
    printf("  │    - Used for input/output tensors                  │\n");
    printf("  │    - Pointers passed as arguments                   │\n");
    printf("  │                                                     │\n");
    printf("  │  Local Memory (UB, L1, L0):                         │\n");
    printf("  │    - Small, fast                                    │\n");
    printf("  │    - Private to each core                           │\n");
    printf("  │    - Kernel allocates internally                    │\n");
    printf("  │    - NOT passed as arguments                        │\n");
    printf("  └─────────────────────────────────────────────────────┘\n\n");

    /* Step 5: Practical example */
    printf("Step 5: Practical Argument Passing\n\n");

    /* Allocate GM buffers */
    const int SIZE = 1024 * sizeof(float);
    void* input = nullptr;
    void* output = nullptr;

    ret = aclrtMalloc(&input, SIZE, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate input (error: %d)\n", ret);
        aclrtDestroyContext(context);
        aclrtResetDevice(device_id);
        aclFinalize();
        return 1;
    }

    ret = aclrtMalloc(&output, SIZE, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        printf("  Failed to allocate output (error: %d)\n", ret);
        aclrtFree(input);
        aclrtDestroyContext(context);
        aclrtResetDevice(device_id);
        aclFinalize();
        return 1;
    }

    SimpleArgs args;
    args.input = input;
    args.output = output;
    args.count = 1024;
    args._pad = 0;

    printf("  Host code packs arguments:\n");
    printf("    SimpleArgs args;\n");
    printf("    args.input  = %p;  // GM pointer\n", args.input);
    printf("    args.output = %p;  // GM pointer\n", args.output);
    printf("    args.count  = %d;\n", args.count);
    printf("\n");

    printf("  Kernel receives same layout:\n");
    printf("    __global__ void my_kernel(SimpleArgs* args) {\n");
    printf("      void* in  = args->input;   // GM address\n");
    printf("      void* out = args->output;  // GM address\n");
    printf("      int n     = args->count;   // Copy to register\n");
    printf("      // ... kernel logic ...\n");
    printf("    }\n\n");

    /* Step 6: Common mistakes */
    printf("Step 6: Common Mistakes to Avoid\n");
    printf("  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  X  Passing host pointers (crash or wrong data)     │\n");
    printf("  │  X  Forgetting padding (misaligned access)          │\n");
    printf("  │  X  Wrong struct size in launch call                │\n");
    printf("  │  X  Mixing up argument order                        │\n");
    printf("  │                                                     │\n");
    printf("  │  ✓  Always use aclrtMalloc for GM pointers          │\n");
    printf("  │  ✓  Keep structs 8-byte aligned                     │\n");
    printf("  │  ✓  Use sizeof(args) for size parameter             │\n");
    printf("  │  ✓  Match host struct with kernel definition        │\n");
    printf("  └─────────────────────────────────────────────────────┘\n\n");

    /* Step 7: Demonstrate launch */
    printf("Step 7: Complete Launch Sequence\n");
    printf("  // 1. Allocate device memory\n");
    printf("  void* input = nullptr;\n");
    printf("  aclrtMalloc(&input, size, ACL_MEM_MALLOC_HUGE_FIRST);\n\n");
    printf("  // 2. Copy input data to device\n");
    printf("  aclrtMemcpy(input, size, host_data, size, ACL_MEMCPY_HOST_TO_DEVICE);\n\n");
    printf("  // 3. Pack arguments\n");
    printf("  SimpleArgs args = {input, output, count, 0};\n\n");
    printf("  // 4. Launch kernel (using aclrtLaunchKernel or aclmdlExecute)\n");
    printf("  aclrtLaunchKernel(...);\n\n");
    printf("  // 5. Synchronize\n");
    printf("  aclrtSynchronizeStream(stream);\n\n");
    printf("  // 6. Copy output back\n");
    printf("  aclrtMemcpy(host_result, size, output, size, ACL_MEMCPY_DEVICE_TO_HOST);\n\n");

    /* Cleanup */
    printf("Step 8: Cleanup...\n");
    aclrtFree(input);
    aclrtFree(output);
    aclrtDestroyContext(context);
    aclrtResetDevice(device_id);
    aclFinalize();
    printf("  Done\n");

    printf("\n=== End of Calling Interface ===\n");
    return 0;
}
