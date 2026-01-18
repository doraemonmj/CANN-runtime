/**
 * 15-vector-ub - UB (Unified Buffer) Memory Management
 *
 * This example demonstrates:
 * - UB size and organization (256 KB per block)
 * - Static vs dynamic allocation strategies
 * - Alignment requirements
 * - Double buffering for overlapping DMA and compute
 * - Tiling strategies to fit data in UB
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

int main() {
    printf("=== UB (Unified Buffer) Memory Management ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== UB Overview ====== */
    printf("Unified Buffer (UB) Overview:\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |  UB is the fast local memory for Vector Unit     |\n");
    printf("  |                                                  |\n");
    printf("  |  Size: 256 KB per AICORE block                   |\n");
    printf("  |  Type: SRAM (fast, ~1 cycle access)              |\n");
    printf("  |  Scope: Private to each AICORE block             |\n");
    printf("  |  Usage: Vector operands, intermediate results    |\n");
    printf("  +--------------------------------------------------+\n\n");

    /* ====== UB Capacity ====== */
    printf("UB Capacity:\n");
    printf("  Total size: 256 KB = 262,144 bytes\n");
    printf("  \n");
    printf("  Can hold:\n");
    printf("    - 65,536 float32 values (64K floats)\n");
    printf("    - 131,072 float16 values (128K halfs)\n");
    printf("    - 262,144 int8 values (256K bytes)\n");
    printf("  \n");
    printf("  Typical allocation (3 buffers):\n");
    printf("    Buffer A:  64 KB = 16K floats (input)\n");
    printf("    Buffer B:  64 KB = 16K floats (output)\n");
    printf("    Buffer C:  64 KB = 16K floats (temp)\n");
    printf("    Reserved:  64 KB (system/scalar)\n\n");

    /* ====== Alignment Requirements ====== */
    printf("Alignment Requirements:\n");
    printf("  +-------------------------------------------+\n");
    printf("  | Data Type   | Alignment  | Block Size    |\n");
    printf("  +-------------------------------------------+\n");
    printf("  | float32     | 32 bytes   | 256 elements  |\n");
    printf("  | float16     | 32 bytes   | 512 elements  |\n");
    printf("  | int8        | 32 bytes   | 1024 elements |\n");
    printf("  +-------------------------------------------+\n");
    printf("  \n");
    printf("  Why 32 bytes? Vector unit processes 256 bits at a time.\n");
    printf("  Misaligned access works but is slower.\n\n");

    /* ====== Static Allocation ====== */
    printf("Static Allocation Strategy:\n");
    printf("  # Define buffers at compile time\n");
    printf("  .const UB_INPUT   = 0           # Offset 0\n");
    printf("  .const UB_OUTPUT  = 16384       # Offset 16K floats\n");
    printf("  .const UB_TEMP    = 32768       # Offset 32K floats\n");
    printf("  \n");
    printf("  Pros: No allocation overhead, predictable layout\n");
    printf("  Cons: Fixed sizes, may waste space\n\n");

    /* ====== Dynamic Allocation ====== */
    printf("Dynamic Allocation Strategy:\n");
    printf("  # Allocate based on runtime sizes\n");
    printf("  SCALAR ub_ptr = 0\n");
    printf("  \n");
    printf("  # Allocate input buffer\n");
    printf("  SCALAR input_buf = ub_ptr\n");
    printf("  SCALAR input_size = ALIGN_UP(n * 4, 32)\n");
    printf("  ub_ptr = ub_ptr + input_size\n");
    printf("  \n");
    printf("  # Allocate output buffer\n");
    printf("  SCALAR output_buf = ub_ptr\n");
    printf("  ...\n");
    printf("  \n");
    printf("  Pros: Flexible, efficient use of UB\n");
    printf("  Cons: Runtime overhead, harder to debug\n\n");

    /* ====== Double Buffering ====== */
    printf("Double Buffering (overlap DMA and compute):\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |  Without double buffering:                       |\n");
    printf("  |    [Load] -> [Compute] -> [Store] -> [Load] ...  |\n");
    printf("  |                                                  |\n");
    printf("  |  With double buffering:                          |\n");
    printf("  |    [Load A]                                      |\n");
    printf("  |           [Load B] [Compute A] [Store prev]      |\n");
    printf("  |                    [Load A] [Compute B] [Store A]|\n");
    printf("  |    Compute overlaps with Load/Store!             |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  \n");
    printf("  UB layout for double buffering:\n");
    printf("    Buffer A0: input ping\n");
    printf("    Buffer A1: input pong\n");
    printf("    Buffer B0: output ping\n");
    printf("    Buffer B1: output pong\n\n");

    /* ====== Tiling Strategy ====== */
    printf("Tiling Strategy (when data > UB size):\n");
    printf("  \n");
    printf("  Problem: 1M floats = 4 MB, but UB = 256 KB\n");
    printf("  Solution: Process in tiles that fit UB\n");
    printf("  \n");
    printf("  // Calculate tile size\n");
    printf("  total_elements = 1000000\n");
    printf("  tile_size = 16000           // Fits in UB with temp space\n");
    printf("  num_tiles = (total + tile - 1) / tile\n");
    printf("  \n");
    printf("  for tile_idx in 0..num_tiles:\n");
    printf("      start = tile_idx * tile_size\n");
    printf("      end = min(start + tile_size, total)\n");
    printf("      \n");
    printf("      DMA_LOAD UB[0], GM[input + start]\n");
    printf("      WAIT_DMA\n");
    printf("      \n");
    printf("      # Process tile in UB\n");
    printf("      VADD UB[output_buf], UB[0], UB[temp]\n");
    printf("      WAIT_VEC\n");
    printf("      \n");
    printf("      DMA_STORE GM[output + start], UB[output_buf]\n");
    printf("      WAIT_DMA\n\n");

    /* ====== Memory Layout Example ====== */
    printf("Example: Fused y = relu(a*x + b) + c\n");
    printf("  \n");
    printf("  UB Layout (16K floats per buffer):\n");
    printf("  +------+------+------+------+\n");
    printf("  |  x   |  a   |  b   |  c   |  64K floats total\n");
    printf("  | 0-16K|16-32K|32-48K|48-64K|\n");
    printf("  +------+------+------+------+\n");
    printf("  |temp| out |      unused     |\n");
    printf("  +------+------+----------------+\n");
    printf("  \n");
    printf("  # Load all inputs\n");
    printf("  DMA_LOAD UB[0], GM[x_ptr]      # x\n");
    printf("  DMA_LOAD UB[16K], GM[a_ptr]    # a\n");
    printf("  DMA_LOAD UB[32K], GM[b_ptr]    # b\n");
    printf("  DMA_LOAD UB[48K], GM[c_ptr]    # c\n");
    printf("  WAIT_DMA\n");
    printf("  \n");
    printf("  # Compute\n");
    printf("  VMUL  UB[64K], UB[0], UB[16K]     # temp = a*x\n");
    printf("  VADD  UB[64K], UB[64K], UB[32K]   # temp = temp + b\n");
    printf("  VRELU UB[64K], UB[64K]            # temp = relu(temp)\n");
    printf("  VADD  UB[80K], UB[64K], UB[48K]   # out = temp + c\n");
    printf("  WAIT_VEC\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of UB Memory Management Example ===\n");
    return 0;
}
