/**
 * 11-aicore-basic - Basic AICORE Execution Model
 *
 * This example demonstrates:
 * - AICORE architecture (Cube + Vector + Scalar units)
 * - Block-based execution model
 * - Pipeline execution
 * - Memory hierarchy (HBM -> L2 -> L1 -> L0/UB)
 *
 * AICORE is the primary compute engine for tensor operations.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

struct VecScaleArgs {
    void* input;
    void* output;
    int32_t count;
    float scale;
};

int main() {
    printf("=== AICORE Basic Execution Model ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== AICORE Architecture ====== */
    printf("AICORE Block Architecture:\n");
    printf("  +-------------------------------------------------------------+\n");
    printf("  |                      AICORE Block                           |\n");
    printf("  |  +------------------+  +------------------+                 |\n");
    printf("  |  |   Cube Unit      |  |   Vector Unit    |                 |\n");
    printf("  |  |                  |  |                  |                 |\n");
    printf("  |  | Matrix Multiply  |  | Element-wise ops |                 |\n");
    printf("  |  | C = A × B        |  | y = f(x)         |                 |\n");
    printf("  |  |                  |  |                  |                 |\n");
    printf("  |  | L0A | L0B | L0C  |  |       UB         |                 |\n");
    printf("  |  +------------------+  +------------------+                 |\n");
    printf("  |                                                             |\n");
    printf("  |  +--------------------+  +--------------------+             |\n");
    printf("  |  |    Scalar Unit     |  |     L1 Buffer     |             |\n");
    printf("  |  | Control, Address   |  |     (1 MB)        |             |\n");
    printf("  |  +--------------------+  +--------------------+             |\n");
    printf("  +-------------------------------------------------------------+\n\n");

    /* ====== Block Execution Model ====== */
    printf("Block Execution Model:\n");
    printf("  // Host launches kernel on N blocks\n");
    printf("  platform_kernel_launch(kernel, N_blocks, &args, ...);\n");
    printf("  \n");
    printf("  // Inside kernel (each block):\n");
    printf("  block_idx = get_block_idx();  // 0 to N-1\n");
    printf("  block_dim = get_block_dim();  // N\n");
    printf("  \n");
    printf("  // Each block processes portion of data\n");
    printf("  chunk_size = total_elements / block_dim;\n");
    printf("  my_start = block_idx * chunk_size;\n");
    printf("  my_end = my_start + chunk_size;\n\n");

    /* ====== Memory Hierarchy ====== */
    printf("Memory Hierarchy:\n");
    printf("  +-----------------+\n");
    printf("  |      HBM        |  32-64 GB, ~1.5 TB/s\n");
    printf("  |  (Global Mem)   |  Shared by all blocks\n");
    printf("  +--------+--------+\n");
    printf("           |\n");
    printf("  +--------v--------+\n");
    printf("  |       L2        |  192 MB, ~6 TB/s\n");
    printf("  |    (Cache)      |  Auto-managed\n");
    printf("  +--------+--------+\n");
    printf("           |\n");
    printf("  +--------v--------+\n");
    printf("  |       L1        |  1 MB per block\n");
    printf("  |   (Staging)     |  Explicit DMA\n");
    printf("  +---+--------+----+\n");
    printf("      |        |\n");
    printf("  +---v---+ +--v---+\n");
    printf("  | L0A/B | |  UB  |  L0: 64KB each, UB: 256KB\n");
    printf("  | L0C   | |      |  Lowest latency\n");
    printf("  +-------+ +------+\n\n");

    /* ====== Pipeline Execution ====== */
    printf("Pipeline Execution:\n");
    printf("  AICORE uses pipeline parallelism across units:\n");
    printf("  \n");
    printf("  Time →\n");
    printf("  ─────────────────────────────────────────────────\n");
    printf("  Scalar: [addr] [addr] [addr] [addr] [addr]\n");
    printf("  Load:      [ld]    [ld]    [ld]    [ld]\n");
    printf("  Cube:          [mm]    [mm]    [mm]\n");
    printf("  Vector:            [vec]   [vec]   [vec]\n");
    printf("  Store:                 [st]    [st]    [st]\n");
    printf("  \n");
    printf("  All stages can run concurrently!\n\n");

    /* ====== PTO-ISA Example ====== */
    printf("PTO-ISA Kernel Example (vector scale):\n");
    printf("  See kernel.pto for pseudo-assembly\n");
    printf("  \n");
    printf("  # Vector scale: output[i] = input[i] * scale\n");
    printf("  \n");
    printf("  # Get block info\n");
    printf("  SCALAR block_idx = GET_BLOCK_IDX()\n");
    printf("  SCALAR block_dim = GET_BLOCK_DIM()\n");
    printf("  \n");
    printf("  # Calculate my portion\n");
    printf("  SCALAR chunk = count / block_dim\n");
    printf("  SCALAR start = block_idx * chunk\n");
    printf("  \n");
    printf("  # Load from HBM to UB\n");
    printf("  DMA_LOAD UB[0:chunk], GM[input + start]\n");
    printf("  WAIT_DMA\n");
    printf("  \n");
    printf("  # Vector multiply\n");
    printf("  VMULS UB[0:chunk], UB[0:chunk], scale\n");
    printf("  WAIT_VEC\n");
    printf("  \n");
    printf("  # Store from UB to HBM\n");
    printf("  DMA_STORE GM[output + start], UB[0:chunk]\n");
    printf("  WAIT_DMA\n\n");

    /* ====== Demo: Launch kernel ====== */
    printf("Demo: Launching AICORE kernel\n");
    printf("  ----------------------------------------\n");

    const int N = 1024 * 1024;  /* 1M elements */
    const uint32_t NUM_BLOCKS = 24;  /* Use all AICOREs */

    printf("  Data size: %d elements (%.2f MB)\n", N, N * sizeof(float) / 1e6);
    printf("  Blocks: %d (all AICOREs)\n", NUM_BLOCKS);
    printf("  Elements per block: %d\n", N / NUM_BLOCKS);

    /* Allocate */
    void* dev_in = platform_malloc(N * sizeof(float));
    void* dev_out = platform_malloc(N * sizeof(float));

    /* Pack args */
    VecScaleArgs args = {dev_in, dev_out, N, 2.0f};

    printf("  \n");
    printf("  platform_kernel_launch(\n");
    printf("      kernel,          // Loaded .o binary\n");
    printf("      %d,               // blocks\n", NUM_BLOCKS);
    printf("      &args,           // {input, output, count, scale}\n");
    printf("      sizeof(args),    // %zu bytes\n", sizeof(args));
    printf("      NULL             // default stream\n");
    printf("  );\n");
    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(dev_in);
    platform_free(dev_out);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of AICORE Basic Example ===\n");
    return 0;
}
