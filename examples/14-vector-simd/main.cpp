/**
 * 14-vector-simd - AICORE Vector Unit SIMD Operations
 *
 * This example demonstrates:
 * - Vector unit instruction types (arithmetic, activation, reduction)
 * - SIMD parallelism (256 elements per instruction)
 * - UB (Unified Buffer) as vector memory
 * - Type conversion operations
 * - Pipeline with vector operations
 *
 * Vector unit handles element-wise operations efficiently.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

int main() {
    printf("=== AICORE Vector Unit SIMD Operations ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Vector Unit Architecture ====== */
    printf("Vector Unit (AIV) Architecture:\n");
    printf("  +---------------------------------------------+\n");
    printf("  |              Vector Unit (AIV)              |\n");
    printf("  |                                             |\n");
    printf("  |  +---------------------------------------+  |\n");
    printf("  |  |        Unified Buffer (UB)           |  |\n");
    printf("  |  |            256 KB                     |  |\n");
    printf("  |  |   Fast SRAM for vector operands      |  |\n");
    printf("  |  +---------------------------------------+  |\n");
    printf("  |                    |                        |\n");
    printf("  |  +--------+  +--------+  +--------+        |\n");
    printf("  |  | VADD   |  | VMUL   |  | VRELU  |  ...   |\n");
    printf("  |  +--------+  +--------+  +--------+        |\n");
    printf("  |                                             |\n");
    printf("  |  SIMD Width: 256 elements per instruction   |\n");
    printf("  +---------------------------------------------+\n\n");

    /* ====== Vector Instructions ====== */
    printf("Vector Instruction Categories:\n\n");

    printf("  ARITHMETIC:\n");
    printf("    VADD   dst, src1, src2    # dst[i] = src1[i] + src2[i]\n");
    printf("    VSUB   dst, src1, src2    # dst[i] = src1[i] - src2[i]\n");
    printf("    VMUL   dst, src1, src2    # dst[i] = src1[i] * src2[i]\n");
    printf("    VDIV   dst, src1, src2    # dst[i] = src1[i] / src2[i]\n");
    printf("    VMULS  dst, src, scalar   # dst[i] = src[i] * scalar\n");
    printf("    VADDS  dst, src, scalar   # dst[i] = src[i] + scalar\n\n");

    printf("  MATH FUNCTIONS:\n");
    printf("    VEXP   dst, src           # dst[i] = exp(src[i])\n");
    printf("    VLOG   dst, src           # dst[i] = log(src[i])\n");
    printf("    VSQRT  dst, src           # dst[i] = sqrt(src[i])\n");
    printf("    VRSQRT dst, src           # dst[i] = 1/sqrt(src[i])\n");
    printf("    VREC   dst, src           # dst[i] = 1/src[i]\n\n");

    printf("  ACTIVATION:\n");
    printf("    VRELU  dst, src           # dst[i] = max(0, src[i])\n");
    printf("    VSIGM  dst, src           # dst[i] = 1/(1+exp(-src[i]))\n");
    printf("    VTANH  dst, src           # dst[i] = tanh(src[i])\n");
    printf("    VGELU  dst, src           # dst[i] = gelu(src[i])\n\n");

    printf("  COMPARISON:\n");
    printf("    VMAX   dst, src1, src2    # dst[i] = max(src1[i], src2[i])\n");
    printf("    VMIN   dst, src1, src2    # dst[i] = min(src1[i], src2[i])\n");
    printf("    VGT    dst, src1, src2    # dst[i] = src1[i] > src2[i] ? 1 : 0\n\n");

    printf("  REDUCTION:\n");
    printf("    VREDUCE_SUM  scalar, src  # scalar = sum(src[:])\n");
    printf("    VREDUCE_MAX  scalar, src  # scalar = max(src[:])\n");
    printf("    VREDUCE_MIN  scalar, src  # scalar = min(src[:])\n\n");

    printf("  TYPE CONVERSION:\n");
    printf("    VCONV  dst, src, FP16_TO_FP32  # Convert fp16 -> fp32\n");
    printf("    VCONV  dst, src, FP32_TO_FP16  # Convert fp32 -> fp16\n");
    printf("    VCONV  dst, src, INT8_TO_FP16  # Convert int8 -> fp16\n\n");

    /* ====== UB Memory Layout ====== */
    printf("UB Memory Layout:\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |                 UB (256 KB)                      |\n");
    printf("  |                                                  |\n");
    printf("  | +---------+---------+---------+---------+        |\n");
    printf("  | | Buffer A| Buffer B| Buffer C| Temp    |        |\n");
    printf("  | | 64 KB   | 64 KB   | 64 KB   | 64 KB   |        |\n");
    printf("  | +---------+---------+---------+---------+        |\n");
    printf("  |                                                  |\n");
    printf("  | Typical allocation for y = relu(a * x + b):      |\n");
    printf("  |   Buffer A: input x (16K floats)                 |\n");
    printf("  |   Buffer B: output y (16K floats)                |\n");
    printf("  |   Temp: intermediate (16K floats)                |\n");
    printf("  +--------------------------------------------------+\n\n");

    /* ====== Example: Softmax ====== */
    printf("Example: Softmax implementation\n");
    printf("  See kernel.pto for PTO-ISA pseudo-assembly\n");
    printf("  \n");
    printf("  # softmax(x) = exp(x - max(x)) / sum(exp(x - max(x)))\n");
    printf("  \n");
    printf("  # Step 1: Find max\n");
    printf("  DMA_LOAD UB[input], GM[x_ptr]\n");
    printf("  VREDUCE_MAX max_val, UB[input]\n");
    printf("  \n");
    printf("  # Step 2: Subtract max and exp\n");
    printf("  VSUBS UB[temp], UB[input], max_val\n");
    printf("  VEXP  UB[temp], UB[temp]\n");
    printf("  \n");
    printf("  # Step 3: Sum\n");
    printf("  VREDUCE_SUM sum_val, UB[temp]\n");
    printf("  \n");
    printf("  # Step 4: Divide\n");
    printf("  SCALAR inv_sum = 1.0 / sum_val\n");
    printf("  VMULS UB[output], UB[temp], inv_sum\n");
    printf("  \n");
    printf("  DMA_STORE GM[y_ptr], UB[output]\n\n");

    /* ====== Performance ====== */
    printf("Vector Unit Performance:\n");
    printf("  +-------------------------------------------+\n");
    printf("  | Operation    | Throughput    | Latency   |\n");
    printf("  +-------------------------------------------+\n");
    printf("  | VADD/VMUL    | 256 elem/cyc  | ~4 cycles |\n");
    printf("  | VEXP/VLOG    | 64 elem/cyc   | ~16 cycles|\n");
    printf("  | VREDUCE      | 256 elem/cyc  | ~8 cycles |\n");
    printf("  | VCONV        | 256 elem/cyc  | ~4 cycles |\n");
    printf("  +-------------------------------------------+\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of Vector SIMD Example ===\n");
    return 0;
}
