/**
 * 16-aicore-atomic - Multi-Block Atomic Operations
 *
 * This example demonstrates:
 * - Atomic operations across AICORE blocks
 * - Parallel reduction using atomics
 * - Global barrier synchronization
 * - Atomic counters for work distribution
 *
 * Unlike AICPU atomics (C++11), AICORE uses special instructions
 * that operate on HBM memory locations visible to all blocks.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

int main() {
    printf("=== AICORE Multi-Block Atomic Operations ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Why Atomics in AICORE? ====== */
    printf("Why Atomics in AICORE?\n");
    printf("  \n");
    printf("  Multiple AICORE blocks run in parallel:\n");
    printf("  +--------+  +--------+  +--------+  +--------+\n");
    printf("  |Block 0 |  |Block 1 |  |Block 2 |  |Block 3 |  ... (24+ blocks)\n");
    printf("  +--------+  +--------+  +--------+  +--------+\n");
    printf("       \\          |          |          /\n");
    printf("        \\         |          |         /\n");
    printf("         +--------+----------+--------+\n");
    printf("         |    Shared HBM Memory       |\n");
    printf("         +----------------------------+\n");
    printf("  \n");
    printf("  Use cases:\n");
    printf("  - Parallel reduction (sum/max/min across blocks)\n");
    printf("  - Global counters (work stealing, progress tracking)\n");
    printf("  - Barrier synchronization (all blocks reach point)\n");
    printf("  - Lock-free data structures\n\n");

    /* ====== Atomic Instructions ====== */
    printf("AICORE Atomic Instructions:\n");
    printf("  +------------------------------------------------------+\n");
    printf("  | Instruction     | Operation        | Return Value    |\n");
    printf("  +------------------------------------------------------+\n");
    printf("  | ATOMIC_ADD      | *addr += val     | old value       |\n");
    printf("  | ATOMIC_SUB      | *addr -= val     | old value       |\n");
    printf("  | ATOMIC_MAX      | *addr = max(...) | old value       |\n");
    printf("  | ATOMIC_MIN      | *addr = min(...) | old value       |\n");
    printf("  | ATOMIC_AND      | *addr &= val     | old value       |\n");
    printf("  | ATOMIC_OR       | *addr |= val     | old value       |\n");
    printf("  | ATOMIC_XOR      | *addr ^= val     | old value       |\n");
    printf("  | ATOMIC_CAS      | if (*addr==exp)  | old value       |\n");
    printf("  |                 |   *addr = val    |                 |\n");
    printf("  +------------------------------------------------------+\n");
    printf("  \n");
    printf("  All atomics operate on HBM addresses (global memory).\n");
    printf("  Return the OLD value before the operation.\n\n");

    /* ====== Parallel Reduction ====== */
    printf("Parallel Reduction Pattern:\n");
    printf("  \n");
    printf("  Problem: Sum 1M elements across 24 blocks\n");
    printf("  \n");
    printf("  Step 1: Each block computes local sum\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | Block 0: sum[0..41K]    -> local_sum_0 = 12345   |\n");
    printf("  | Block 1: sum[41K..83K]  -> local_sum_1 = 23456   |\n");
    printf("  | Block 2: sum[83K..125K] -> local_sum_2 = 34567   |\n");
    printf("  | ...                                               |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  \n");
    printf("  Step 2: Atomic add to global accumulator\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | global_sum = 0  (initialized in HBM)             |\n");
    printf("  |                                                   |\n");
    printf("  | Block 0: ATOMIC_ADD(global_sum, local_sum_0)     |\n");
    printf("  | Block 1: ATOMIC_ADD(global_sum, local_sum_1)     |\n");
    printf("  | Block 2: ATOMIC_ADD(global_sum, local_sum_2)     |\n");
    printf("  | ...                                               |\n");
    printf("  |                                                   |\n");
    printf("  | Final: global_sum = sum of all local sums        |\n");
    printf("  +--------------------------------------------------+\n\n");

    /* ====== Global Barrier ====== */
    printf("Global Barrier Synchronization:\n");
    printf("  \n");
    printf("  Ensure all blocks reach a synchronization point:\n");
    printf("  \n");
    printf("  # Barrier implementation using atomics\n");
    printf("  ATOMIC_ADD barrier_count, 1      # Increment counter\n");
    printf("  \n");
    printf("  .wait_loop:\n");
    printf("      LOAD tmp, [barrier_count]    # Read counter\n");
    printf("      CMP tmp, num_blocks          # All blocks arrived?\n");
    printf("      JLT .wait_loop               # No, keep waiting\n");
    printf("  \n");
    printf("  # All blocks have reached this point\n");
    printf("  # Continue execution...\n");
    printf("  \n");
    printf("  Note: Busy-waiting is expensive. Prefer restructuring\n");
    printf("  algorithms to avoid inter-block synchronization.\n\n");

    /* ====== Work Stealing ====== */
    printf("Work Stealing with Atomic Counter:\n");
    printf("  \n");
    printf("  Problem: Process N items with unknown per-item cost\n");
    printf("  \n");
    printf("  # Shared work counter in HBM\n");
    printf("  work_idx = 0\n");
    printf("  \n");
    printf("  # Each block:\n");
    printf("  .work_loop:\n");
    printf("      # Atomically claim next work item\n");
    printf("      SCALAR my_idx = ATOMIC_ADD(work_idx, 1)\n");
    printf("      \n");
    printf("      # Check if work remains\n");
    printf("      CMP my_idx, total_items\n");
    printf("      JGE .work_done\n");
    printf("      \n");
    printf("      # Process item my_idx\n");
    printf("      process(items[my_idx])\n");
    printf("      JMP .work_loop\n");
    printf("  \n");
    printf("  .work_done:\n");
    printf("      # Block is finished\n");
    printf("  \n");
    printf("  Benefit: Automatic load balancing if item costs vary!\n\n");

    /* ====== Atomic CAS Pattern ====== */
    printf("Compare-And-Swap (CAS) Pattern:\n");
    printf("  \n");
    printf("  Implement custom atomic operations:\n");
    printf("  \n");
    printf("  # Atomic multiply (not a built-in)\n");
    printf("  .atomic_mul:\n");
    printf("      LOAD expected, [addr]           # Read current\n");
    printf("      MUL desired, expected, factor   # Compute new value\n");
    printf("      ATOMIC_CAS result, [addr], expected, desired\n");
    printf("      CMP result, expected            # Did CAS succeed?\n");
    printf("      JNE .atomic_mul                 # No, retry\n");
    printf("  \n");
    printf("  # Lock-free stack push\n");
    printf("  .push:\n");
    printf("      LOAD old_head, [stack_head]     # Current head\n");
    printf("      STORE [new_node.next], old_head # Link new node\n");
    printf("      ATOMIC_CAS result, [stack_head], old_head, new_node\n");
    printf("      CMP result, old_head\n");
    printf("      JNE .push                       # Retry on conflict\n\n");

    /* ====== Memory Ordering ====== */
    printf("Memory Ordering Considerations:\n");
    printf("  \n");
    printf("  AICORE atomics provide sequential consistency:\n");
    printf("  - All atomic operations appear in a global order\n");
    printf("  - Non-atomic accesses may be reordered around atomics\n");
    printf("  \n");
    printf("  Use FENCE instruction for explicit ordering:\n");
    printf("  \n");
    printf("  STORE [data], value        # Non-atomic write\n");
    printf("  FENCE                       # Ensure data visible\n");
    printf("  ATOMIC_ADD [flag], 1       # Signal completion\n");
    printf("  \n");
    printf("  # Other block:\n");
    printf("  .wait:\n");
    printf("      LOAD tmp, [flag]       # Check flag\n");
    printf("      CMP tmp, expected\n");
    printf("      JLT .wait\n");
    printf("  FENCE                       # Ensure we see data\n");
    printf("  LOAD result, [data]        # Safe to read now\n\n");

    /* ====== Performance Tips ====== */
    printf("Performance Tips:\n");
    printf("  \n");
    printf("  1. Minimize atomic contention:\n");
    printf("     - Compute locally first, atomic once at end\n");
    printf("     - Use hierarchical reduction if needed\n");
    printf("  \n");
    printf("  2. Avoid barriers when possible:\n");
    printf("     - Restructure algorithm for independent blocks\n");
    printf("     - Use producer-consumer instead of barrier\n");
    printf("  \n");
    printf("  3. Align atomic addresses:\n");
    printf("     - 4-byte alignment for i32/f32\n");
    printf("     - 8-byte alignment for i64\n");
    printf("  \n");
    printf("  4. Consider split-K for GEMM:\n");
    printf("     - Split K dimension across blocks\n");
    printf("     - Each block computes partial C\n");
    printf("     - Final atomic reduction to merge results\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of AICORE Atomic Operations Example ===\n");
    return 0;
}
