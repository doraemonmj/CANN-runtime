/**
 * 13-cube-memory - L1/L0 Buffer Management for Cube Operations
 *
 * This example demonstrates:
 * - L1 buffer as staging area between HBM and L0
 * - L0A/L0B/L0C buffer sizes and constraints
 * - Double buffering in L1 to hide DMA latency
 * - Data layout transformation (NZ/ZN formats)
 * - Prefetching strategies
 *
 * Memory hierarchy for Cube: HBM -> L1 -> L0A/L0B -> MMAD -> L0C -> L1 -> HBM
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

int main() {
    printf("=== L1/L0 Buffer Management for Cube Unit ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Memory Hierarchy ====== */
    printf("Cube Unit Memory Hierarchy:\n");
    printf("  +-----------------------------------------------------------+\n");
    printf("  |  HBM (High Bandwidth Memory)                              |\n");
    printf("  |  - 32/64 GB total                                         |\n");
    printf("  |  - ~2 TB/s bandwidth                                      |\n");
    printf("  +-----------------------------------------------------------+\n");
    printf("               |                              ^\n");
    printf("               v (DMA Load)                   | (DMA Store)\n");
    printf("  +-----------------------------------------------------------+\n");
    printf("  |  L1 Buffer (1 MB per AICORE block)                        |\n");
    printf("  |  - Staging area for tiles                                 |\n");
    printf("  |  - Layout transformation (row/col major)                  |\n");
    printf("  |  - Double buffering for latency hiding                    |\n");
    printf("  +-----------------------------------------------------------+\n");
    printf("          |              |              ^\n");
    printf("          v              v              |\n");
    printf("  +----------+    +----------+    +----------+\n");
    printf("  |   L0A    |    |   L0B    |    |   L0C    |\n");
    printf("  |  64 KB   |    |  64 KB   |    |  256 KB  |\n");
    printf("  | [M x K]  |    | [K x N]  |    | [M x N]  |\n");
    printf("  +----------+    +----------+    +----------+\n");
    printf("          \\           |            /\n");
    printf("           \\          v           /\n");
    printf("            +------------------+\n");
    printf("            |   MMAD Engine    |\n");
    printf("            | C += A * B       |\n");
    printf("            +------------------+\n\n");

    /* ====== L0 Buffer Constraints ====== */
    printf("L0 Buffer Sizes (FP16):\n");
    printf("  +-----------------------------------------------+\n");
    printf("  | Buffer | Size   | Elements | Typical Tile    |\n");
    printf("  +-----------------------------------------------+\n");
    printf("  | L0A    | 64 KB  | 32K FP16 | 128x256 or 64x512|\n");
    printf("  | L0B    | 64 KB  | 32K FP16 | 256x128 or 512x64|\n");
    printf("  | L0C    | 256 KB | 128K FP16| 256x512 or 512x256|\n");
    printf("  +-----------------------------------------------+\n");
    printf("  \n");
    printf("  Constraint: M * K <= 32K for L0A\n");
    printf("              K * N <= 32K for L0B\n");
    printf("              M * N <= 128K for L0C\n\n");

    /* ====== L1 Buffer Organization ====== */
    printf("L1 Buffer Organization (1 MB total):\n");
    printf("  \n");
    printf("  Static Allocation Strategy:\n");
    printf("  +------------+------------+------------+------------+\n");
    printf("  |   A ping   |   A pong   |   B ping   |   B pong   |\n");
    printf("  |   256 KB   |   256 KB   |   256 KB   |   256 KB   |\n");
    printf("  +------------+------------+------------+------------+\n");
    printf("  0            256K         512K         768K         1M\n");
    printf("  \n");
    printf("  Each 256KB = 128K FP16 values\n");
    printf("  Can hold A[256x512] or B[512x256] tile\n\n");

    /* ====== Double Buffering Pipeline ====== */
    printf("Double Buffering Pipeline:\n");
    printf("  \n");
    printf("  Timeline (each row = one iteration):\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  | Time | DMA (A ping)| DMA (A pong)| L0 Load  | MMAD      |\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  |  0   | Load tile 0 |     -       |    -     |    -      |\n");
    printf("  |  1   |     -       | Load tile 1 | A0->L0A  | (wait)    |\n");
    printf("  |  2   | Load tile 2 |     -       | A1->L0A  | compute 0 |\n");
    printf("  |  3   |     -       | Load tile 3 | A2->L0A  | compute 1 |\n");
    printf("  |  4   | Load tile 4 |     -       | A3->L0A  | compute 2 |\n");
    printf("  +----------------------------------------------------------+\n");
    printf("  \n");
    printf("  Key: While computing tile N, DMA loads tile N+2\n\n");

    /* ====== Data Layout Transformation ====== */
    printf("Data Layout Transformation:\n");
    printf("  \n");
    printf("  Cube unit requires specific layouts:\n");
    printf("    - Matrix A: NZ format (row-major, fractal Z)\n");
    printf("    - Matrix B: ZN format (col-major, fractal Z)\n");
    printf("  \n");
    printf("  L1 can perform layout transformation during load:\n");
    printf("  \n");
    printf("  # Load A from HBM (row-major) to L1 (NZ format)\n");
    printf("  DMA_LOAD_NZ L1[A_buf], GM[A_ptr], rows, cols, stride\n");
    printf("  \n");
    printf("  # Load B from HBM (row-major) to L1 (ZN format) \n");
    printf("  DMA_LOAD_ZN L1[B_buf], GM[B_ptr], rows, cols, stride\n");
    printf("  \n");
    printf("  NZ Format (for A):           ZN Format (for B):\n");
    printf("  +----+----+----+----+        +----+----+----+----+\n");
    printf("  |Z0,0|Z0,1|Z0,2|Z0,3|        |Z0,0|Z1,0|Z2,0|Z3,0|\n");
    printf("  |Z1,0|Z1,1|Z1,2|Z1,3|        |Z0,1|Z1,1|Z2,1|Z3,1|\n");
    printf("  |Z2,0|Z2,1|Z2,2|Z2,3|        |Z0,2|Z1,2|Z2,2|Z3,2|\n");
    printf("  |Z3,0|Z3,1|Z3,2|Z3,3|        |Z0,3|Z1,3|Z2,3|Z3,3|\n");
    printf("  +----+----+----+----+        +----+----+----+----+\n");
    printf("  Each Zi,j is a 16x16 block   Transposed block order\n\n");

    /* ====== Prefetching Strategy ====== */
    printf("Prefetching Strategy:\n");
    printf("  \n");
    printf("  # Setup: Load first tiles into L1\n");
    printf("  DMA_LOAD_ASYNC L1[A_ping], GM[A_ptr + 0]\n");
    printf("  DMA_LOAD_ASYNC L1[B_ping], GM[B_ptr + 0]\n");
    printf("  WAIT_DMA\n");
    printf("  \n");
    printf("  for k_tile in range(0, K, TILE_K):\n");
    printf("      # Start prefetch for NEXT tile (if not last)\n");
    printf("      if k_tile + TILE_K < K:\n");
    printf("          DMA_LOAD_ASYNC L1[A_pong], GM[A_ptr + next_offset]\n");
    printf("          DMA_LOAD_ASYNC L1[B_pong], GM[B_ptr + next_offset]\n");
    printf("      \n");
    printf("      # Move CURRENT tile from L1 to L0\n");
    printf("      L1_TO_L0A L0A, L1[A_ping], tile_m, tile_k\n");
    printf("      L1_TO_L0B L0B, L1[B_ping], tile_k, tile_n\n");
    printf("      \n");
    printf("      # Compute while DMA runs in background\n");
    printf("      MMAD L0C, L0A, L0B\n");
    printf("      WAIT_CUBE\n");
    printf("      \n");
    printf("      # Swap ping/pong buffers\n");
    printf("      WAIT_DMA  # Ensure prefetch completed\n");
    printf("      swap(A_ping, A_pong)\n");
    printf("      swap(B_ping, B_pong)\n\n");

    /* ====== Tile Size Selection ====== */
    printf("Tile Size Selection:\n");
    printf("  \n");
    printf("  Goal: Maximize compute/memory ratio\n");
    printf("  \n");
    printf("  Compute: 2 * M * K * N FLOPs\n");
    printf("  Memory:  2 * (M*K + K*N + M*N) bytes (FP16)\n");
    printf("  \n");
    printf("  Ratio = M*K*N / (M*K + K*N + M*N)\n");
    printf("  \n");
    printf("  For M=N=256, K=128:\n");
    printf("    Compute = 2 * 256 * 128 * 256 = 16.7M FLOPs\n");
    printf("    Memory  = 2 * (32K + 32K + 64K) = 256KB\n");
    printf("    Ratio   = ~65 FLOPs/byte (good!)\n");
    printf("  \n");
    printf("  For M=N=64, K=64:\n");
    printf("    Compute = 2 * 64 * 64 * 64 = 0.5M FLOPs\n");
    printf("    Memory  = 2 * (4K + 4K + 4K) = 24KB\n");
    printf("    Ratio   = ~21 FLOPs/byte (lower efficiency)\n\n");

    /* ====== Complete Example ====== */
    printf("Complete GEMM with L1 Staging:\n");
    printf("  ----------------------------------------\n");
    printf("  # Allocate L1 buffers\n");
    printf("  .const L1_A_PING = 0\n");
    printf("  .const L1_A_PONG = 262144    # 256KB\n");
    printf("  .const L1_B_PING = 524288    # 512KB\n");
    printf("  .const L1_B_PONG = 786432    # 768KB\n");
    printf("  \n");
    printf("  # Outer loops over M and N tiles\n");
    printf("  for m_tile, n_tile:\n");
    printf("      # Clear accumulator\n");
    printf("      MMAD_CLEAR L0C\n");
    printf("      \n");
    printf("      # K reduction with double buffering\n");
    printf("      active = PING\n");
    printf("      DMA_LOAD L1[A_ping], GM[A + offset_0]\n");
    printf("      DMA_LOAD L1[B_ping], GM[B + offset_0]\n");
    printf("      \n");
    printf("      for k_tile in range(K):\n");
    printf("          # Prefetch next\n");
    printf("          DMA_LOAD_ASYNC L1[A_other], GM[A + next]\n");
    printf("          DMA_LOAD_ASYNC L1[B_other], GM[B + next]\n");
    printf("          \n");
    printf("          # Current tile: L1 -> L0 -> MMAD\n");
    printf("          L1_TO_L0A L0A, L1[A_active]\n");
    printf("          L1_TO_L0B L0B, L1[B_active]\n");
    printf("          MMAD L0C, L0A, L0B\n");
    printf("          \n");
    printf("          # Swap and sync\n");
    printf("          WAIT_DMA\n");
    printf("          active = 1 - active\n");
    printf("      \n");
    printf("      # Store result: L0C -> L1 -> HBM\n");
    printf("      L0C_TO_L1 L1[C_buf], L0C\n");
    printf("      DMA_STORE GM[C + c_offset], L1[C_buf]\n");
    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of L1/L0 Buffer Management Example ===\n");
    return 0;
}
