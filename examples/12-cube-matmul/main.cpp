/**
 * 12-cube-matmul - AICORE Cube Unit Matrix Multiplication
 *
 * This example demonstrates:
 * - Cube unit architecture (MMAD instruction)
 * - Matrix layout requirements (row-major A, col-major B)
 * - Tiling for large matrices
 * - L0A/L0B/L0C buffer usage
 * - INT8/FP16 quantization
 *
 * Cube unit is the matrix multiply engine - the heart of AICORE.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

struct MatMulArgs {
    void* A;           /* Matrix A [M x K] row-major */
    void* B;           /* Matrix B [K x N] col-major */
    void* C;           /* Matrix C [M x N] row-major */
    int32_t M;
    int32_t K;
    int32_t N;
    int32_t _pad;
};

int main() {
    printf("=== AICORE Cube Unit Matrix Multiplication ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Cube Unit Architecture ====== */
    printf("Cube Unit (AIC) Architecture:\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  |                    Cube Unit                          |\n");
    printf("  |                                                       |\n");
    printf("  |  Operation: C = A × B                                 |\n");
    printf("  |                                                       |\n");
    printf("  |  +--------+    +--------+    +--------+               |\n");
    printf("  |  |  L0A   | ×  |  L0B   | -> |  L0C   |               |\n");
    printf("  |  | 64 KB  |    | 64 KB  |    | 256 KB |               |\n");
    printf("  |  |[M×K]   |    |[K×N]   |    |[M×N]   |               |\n");
    printf("  |  +--------+    +--------+    +--------+               |\n");
    printf("  |                                                       |\n");
    printf("  |  Tile sizes (FP16):                                   |\n");
    printf("  |    M=16, K=16, N=16 per MMAD instruction              |\n");
    printf("  |                                                       |\n");
    printf("  |  Throughput: ~320 TFLOPS (FP16, A3/910C)              |\n");
    printf("  +-------------------------------------------------------+\n\n");

    /* ====== MMAD Instruction ====== */
    printf("MMAD (Matrix Multiply-Add) Instruction:\n");
    printf("  \n");
    printf("  MMAD L0C, L0A, L0B\n");
    printf("  \n");
    printf("  Computes: L0C += L0A × L0B\n");
    printf("  \n");
    printf("  Tile sizes vary by data type:\n");
    printf("  +------------------------------------------+\n");
    printf("  | Type  | M    | K    | N    | TFLOPS     |\n");
    printf("  +------------------------------------------+\n");
    printf("  | FP16  | 16   | 16   | 16   | ~320       |\n");
    printf("  | BF16  | 16   | 16   | 16   | ~320       |\n");
    printf("  | INT8  | 16   | 32   | 16   | ~640       |\n");
    printf("  | FP32  | 16   | 8    | 16   | ~160       |\n");
    printf("  +------------------------------------------+\n\n");

    /* ====== Matrix Layout Requirements ====== */
    printf("Matrix Layout Requirements:\n");
    printf("  \n");
    printf("  IMPORTANT: Matrices must be in specific layouts!\n");
    printf("  \n");
    printf("  Matrix A [M × K]: Row-major (NZ format)\n");
    printf("    Memory: A[0,0], A[0,1], ..., A[0,K-1], A[1,0], ...\n");
    printf("  \n");
    printf("  Matrix B [K × N]: Column-major (ZN format)\n");
    printf("    Memory: B[0,0], B[1,0], ..., B[K-1,0], B[0,1], ...\n");
    printf("  \n");
    printf("  Matrix C [M × N]: Row-major (NZ format)\n");
    printf("  \n");
    printf("  Transpose B before matmul if needed:\n");
    printf("    B_colmajor[i,j] = B_rowmajor[j,i]\n\n");

    /* ====== Tiling for Large Matrices ====== */
    printf("Tiling for Large Matrices:\n");
    printf("  \n");
    printf("  L0A = 64 KB: fits 32K FP16 values\n");
    printf("  L0B = 64 KB: fits 32K FP16 values\n");
    printf("  L0C = 256 KB: fits 128K FP16 values\n");
    printf("  \n");
    printf("  For C[1024 × 1024] = A[1024 × 512] × B[512 × 1024]:\n");
    printf("  \n");
    printf("  # Tile loops\n");
    printf("  for m_tile in range(0, M, TILE_M):    # TILE_M = 256\n");
    printf("      for n_tile in range(0, N, TILE_N):  # TILE_N = 256\n");
    printf("          # Clear accumulator in L0C\n");
    printf("          L0C = 0\n");
    printf("          \n");
    printf("          for k_tile in range(0, K, TILE_K):  # TILE_K = 128\n");
    printf("              # Load A tile to L0A\n");
    printf("              L0A = A[m_tile:m_tile+TILE_M, k_tile:k_tile+TILE_K]\n");
    printf("              # Load B tile to L0B\n");
    printf("              L0B = B[k_tile:k_tile+TILE_K, n_tile:n_tile+TILE_N]\n");
    printf("              # Accumulate\n");
    printf("              MMAD L0C, L0A, L0B\n");
    printf("          \n");
    printf("          # Store result\n");
    printf("          C[m_tile:m_tile+TILE_M, n_tile:n_tile+TILE_N] = L0C\n\n");

    /* ====== L1 Staging ====== */
    printf("L1 Buffer for Staging:\n");
    printf("  \n");
    printf("  L1 (1 MB) sits between HBM and L0 buffers.\n");
    printf("  Used for:\n");
    printf("    - Prefetching next tiles while processing current\n");
    printf("    - Layout transformation (row to col major)\n");
    printf("    - Reducing HBM bandwidth pressure\n");
    printf("  \n");
    printf("  Flow: HBM -> L1 -> L0A/L0B -> MMAD -> L0C -> L1 -> HBM\n\n");

    /* ====== Quantization (INT8) ====== */
    printf("INT8 Quantization for 2x Throughput:\n");
    printf("  \n");
    printf("  INT8 matmul: ~640 TOPS (vs 320 TFLOPS for FP16)\n");
    printf("  \n");
    printf("  # Quantize inputs\n");
    printf("  A_int8 = quantize(A_fp16, scale_a)\n");
    printf("  B_int8 = quantize(B_fp16, scale_b)\n");
    printf("  \n");
    printf("  # INT8 matmul accumulates to INT32\n");
    printf("  C_int32 = A_int8 × B_int8\n");
    printf("  \n");
    printf("  # Dequantize output\n");
    printf("  C_fp16 = dequantize(C_int32, scale_a * scale_b)\n\n");

    /* ====== Demo ====== */
    printf("Demo: MatMul kernel launch\n");
    printf("  ----------------------------------------\n");

    int M = 1024, K = 512, N = 1024;
    size_t A_size = M * K * sizeof(uint16_t);  /* FP16 */
    size_t B_size = K * N * sizeof(uint16_t);
    size_t C_size = M * N * sizeof(uint16_t);

    printf("  Problem: C[%d×%d] = A[%d×%d] × B[%d×%d]\n", M, N, M, K, K, N);
    printf("  Data type: FP16\n");
    printf("  Memory: A=%.1f MB, B=%.1f MB, C=%.1f MB\n",
           A_size/1e6, B_size/1e6, C_size/1e6);

    void* dev_A = platform_malloc(A_size);
    void* dev_B = platform_malloc(B_size);
    void* dev_C = platform_malloc(C_size);

    MatMulArgs args = {dev_A, dev_B, dev_C, M, K, N, 0};

    printf("  \n");
    printf("  platform_kernel_launch(matmul_kernel, 24, &args, ...);\n");
    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(dev_A);
    platform_free(dev_B);
    platform_free(dev_C);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of Cube MatMul Example ===\n");
    return 0;
}
