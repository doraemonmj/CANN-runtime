/**
 * 18-sync-register - Register-Based Synchronization
 *
 * This example demonstrates:
 * - Shared registers between AICPU and AICORE
 * - Signal-based synchronization patterns
 * - Producer-consumer coordination
 * - Low-latency inter-processor communication
 *
 * Shared registers provide the fastest synchronization mechanism
 * between AICPU and AICORE (sub-microsecond latency).
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

int main() {
    printf("=== Register-Based Synchronization ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Shared Register Overview ====== */
    printf("Shared Register Overview:\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  |  AICPU and AICORE share special-purpose registers     |\n");
    printf("  |                                                       |\n");
    printf("  |  +--------+     Shared Registers     +--------+       |\n");
    printf("  |  | AICPU  | <---- REG[0..31] ----> | AICORE |       |\n");
    printf("  |  +--------+      (32-bit each)       +--------+       |\n");
    printf("  |                                                       |\n");
    printf("  |  Properties:                                          |\n");
    printf("  |  - 32 registers available (REG0 - REG31)              |\n");
    printf("  |  - Each register is 32 bits                           |\n");
    printf("  |  - Read/write latency: ~100ns                         |\n");
    printf("  |  - Atomically updated (no partial reads)              |\n");
    printf("  +-------------------------------------------------------+\n\n");

    /* ====== Register Access ====== */
    printf("Register Access:\n");
    printf("  \n");
    printf("  From AICPU (C code):\n");
    printf("  ----------------------------------------\n");
    printf("  // Write to shared register\n");
    printf("  platform_write_shared_reg(REG_DATA_READY, 1);\n");
    printf("  \n");
    printf("  // Read from shared register\n");
    printf("  uint32_t status = platform_read_shared_reg(REG_STATUS);\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  From AICORE (PTO-ISA):\n");
    printf("  ----------------------------------------\n");
    printf("  # Write to shared register\n");
    printf("  REG_WRITE REG_STATUS, 0x1\n");
    printf("  \n");
    printf("  # Read from shared register\n");
    printf("  REG_READ status, REG_DATA_READY\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Signaling Pattern ====== */
    printf("Basic Signaling Pattern:\n");
    printf("  \n");
    printf("  AICPU prepares data, signals AICORE, waits for completion.\n");
    printf("  \n");
    printf("  Register allocation:\n");
    printf("  - REG0: DATA_READY (AICPU -> AICORE)\n");
    printf("  - REG1: COMPUTE_DONE (AICORE -> AICPU)\n");
    printf("  \n");
    printf("  Timeline:\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  | AICPU                    | AICORE                       |\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  | 1. Prepare data in HBM   |                              |\n");
    printf("  | 2. Write REG0 = 1        |                              |\n");
    printf("  |    (signal data ready)   |                              |\n");
    printf("  | 3. Poll REG1 until != 0  | 4. Poll REG0 until != 0      |\n");
    printf("  |                          | 5. Read data from HBM        |\n");
    printf("  |                          | 6. Compute                   |\n");
    printf("  |                          | 7. Write result to HBM       |\n");
    printf("  |                          | 8. Write REG1 = 1            |\n");
    printf("  | 9. Read result from HBM  |                              |\n");
    printf("  +---------------------------------------------------------+\n\n");

    /* ====== Multi-Stage Pipeline ====== */
    printf("Multi-Stage Pipeline Pattern:\n");
    printf("  \n");
    printf("  AICPU prepares batches while AICORE processes.\n");
    printf("  \n");
    printf("  Register allocation:\n");
    printf("  - REG0: BATCH_ID (current batch ready, AICPU -> AICORE)\n");
    printf("  - REG1: PROCESSED_ID (last processed, AICORE -> AICPU)\n");
    printf("  \n");
    printf("  AICPU:                       AICORE:\n");
    printf("  ----------------------------------------\n");
    printf("  for batch in 0..N:           while PROCESSED_ID < N:\n");
    printf("      prepare(batch)               wait(BATCH_ID > PROCESSED_ID)\n");
    printf("      BATCH_ID = batch + 1         process(BATCH_ID - 1)\n");
    printf("      wait(PROCESSED_ID >= batch)  PROCESSED_ID = BATCH_ID\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  Benefits:\n");
    printf("  - Overlaps data preparation with compute\n");
    printf("  - Natural flow control (AICPU can't get too far ahead)\n\n");

    /* ====== Register-Based Barrier ====== */
    printf("Register-Based Barrier:\n");
    printf("  \n");
    printf("  Synchronize all AICORE blocks at a point.\n");
    printf("  \n");
    printf("  Register allocation:\n");
    printf("  - REG0: ARRIVE_COUNT (blocks that have arrived)\n");
    printf("  - REG1: RELEASE_FLAG (barrier released)\n");
    printf("  \n");
    printf("  AICORE block barrier:\n");
    printf("  ----------------------------------------\n");
    printf("  # Arrive at barrier\n");
    printf("  REG_ATOMIC_ADD REG0, 1\n");
    printf("  \n");
    printf("  # Wait for release (AICPU detects all arrived, sets flag)\n");
    printf("  .wait_release:\n");
    printf("      REG_READ flag, REG1\n");
    printf("      CMP flag, 0\n");
    printf("      JEQ .wait_release\n");
    printf("  \n");
    printf("  # Continue execution\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  AICPU barrier controller:\n");
    printf("  ----------------------------------------\n");
    printf("  // Wait for all blocks\n");
    printf("  while (platform_read_shared_reg(REG_ARRIVE) < num_blocks) {\n");
    printf("      // spin\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Release all blocks\n");
    printf("  platform_write_shared_reg(REG_RELEASE, 1);\n");
    printf("  \n");
    printf("  // Reset for next barrier\n");
    printf("  platform_write_shared_reg(REG_ARRIVE, 0);\n");
    printf("  platform_write_shared_reg(REG_RELEASE, 0);\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Status Monitoring ====== */
    printf("Progress Monitoring Pattern:\n");
    printf("  \n");
    printf("  AICORE reports progress, AICPU monitors.\n");
    printf("  \n");
    printf("  AICORE (in loop):\n");
    printf("  ----------------------------------------\n");
    printf("  for tile in 0..num_tiles:\n");
    printf("      process_tile(tile)\n");
    printf("      REG_WRITE REG_PROGRESS, tile + 1\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  AICPU (monitoring):\n");
    printf("  ----------------------------------------\n");
    printf("  while (1) {\n");
    printf("      uint32_t progress = platform_read_shared_reg(REG_PROGRESS);\n");
    printf("      printf(\"Progress: %%d/%%d tiles\\n\", progress, total);\n");
    printf("      if (progress >= total) break;\n");
    printf("      usleep(1000);  // 1ms poll interval\n");
    printf("  }\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Best Practices ====== */
    printf("Best Practices:\n");
    printf("  \n");
    printf("  1. Register allocation:\n");
    printf("     - Document which registers are used for what\n");
    printf("     - Reserve ranges: REG0-7 for system, REG8-31 for app\n");
    printf("  \n");
    printf("  2. Avoid busy-waiting when possible:\n");
    printf("     - Use timeout to detect hangs\n");
    printf("     - Consider yielding CPU if wait is long\n");
    printf("  \n");
    printf("  3. Clear registers between uses:\n");
    printf("     - Reset to 0 before starting new operation\n");
    printf("     - Prevents stale values causing issues\n");
    printf("  \n");
    printf("  4. Use atomic operations for counters:\n");
    printf("     - REG_ATOMIC_ADD for incrementing\n");
    printf("     - Prevents race conditions\n\n");

    /* ====== Performance Comparison ====== */
    printf("Synchronization Method Comparison:\n");
    printf("  +-----------------------------------------------------+\n");
    printf("  | Method          | Latency    | Bandwidth | Use Case |\n");
    printf("  +-----------------------------------------------------+\n");
    printf("  | Shared Register | ~100 ns    | Low       | Signals  |\n");
    printf("  | HBM Atomic      | ~500 ns    | Medium    | Counters |\n");
    printf("  | HBM Queue       | ~1-5 us    | High      | Data     |\n");
    printf("  | Stream Event    | ~10 us     | N/A       | Ordering |\n");
    printf("  +-----------------------------------------------------+\n");
    printf("  \n");
    printf("  Use shared registers for:\n");
    printf("  - Simple ready/done signals\n");
    printf("  - Progress counters\n");
    printf("  - Barrier synchronization\n");
    printf("  - Low-latency coordination\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of Register Synchronization Example ===\n");
    return 0;
}
