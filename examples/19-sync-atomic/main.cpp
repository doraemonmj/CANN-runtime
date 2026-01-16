/**
 * 19-sync-atomic - Atomic-Based AICPU/AICORE Coordination
 *
 * This example demonstrates:
 * - HBM-based atomic operations for synchronization
 * - Progress tracking between AICPU and AICORE
 * - Distributed counters and barriers
 * - Lock-free coordination patterns
 *
 * HBM atomics provide higher bandwidth than shared registers
 * but with higher latency. Best for counters and less frequent sync.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

/* Synchronization control block in HBM */
struct SyncBlock {
    int32_t ready_count;      /* Blocks that have data ready */
    int32_t processed_count;  /* Blocks that finished processing */
    int32_t barrier_count;    /* Blocks arrived at barrier */
    int32_t release_flag;     /* Barrier release signal */
    int32_t work_index;       /* Next work item to claim */
    int32_t error_flag;       /* Error indicator */
    int32_t padding[2];       /* Align to 32 bytes */
};

int main() {
    printf("=== Atomic-Based AICPU/AICORE Coordination ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== HBM Atomics Overview ====== */
    printf("HBM Atomic Operations Overview:\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  |  HBM (global memory) supports atomic operations       |\n");
    printf("  |                                                       |\n");
    printf("  |  +--------+         HBM          +--------+           |\n");
    printf("  |  | AICPU  | <---- atomics ---->  | AICORE |           |\n");
    printf("  |  +--------+     [sync_block]     +--------+           |\n");
    printf("  |                                                       |\n");
    printf("  |  Operations:                                          |\n");
    printf("  |  - atomic_add, atomic_sub                             |\n");
    printf("  |  - atomic_max, atomic_min                             |\n");
    printf("  |  - atomic_and, atomic_or, atomic_xor                  |\n");
    printf("  |  - atomic_cas (compare-and-swap)                      |\n");
    printf("  |                                                       |\n");
    printf("  |  Latency: ~500ns (higher than registers)              |\n");
    printf("  |  Bandwidth: Higher (good for many small ops)          |\n");
    printf("  +-------------------------------------------------------+\n\n");

    /* ====== Sync Block Layout ====== */
    printf("Synchronization Block Layout:\n");
    printf("  \n");
    printf("  struct SyncBlock {        // Allocated in HBM\n");
    printf("      int32_t ready_count;      // Offset 0\n");
    printf("      int32_t processed_count;  // Offset 4\n");
    printf("      int32_t barrier_count;    // Offset 8\n");
    printf("      int32_t release_flag;     // Offset 12\n");
    printf("      int32_t work_index;       // Offset 16\n");
    printf("      int32_t error_flag;       // Offset 20\n");
    printf("  };\n");
    printf("  \n");
    printf("  // Initialize before kernel launch\n");
    printf("  memset(sync_block, 0, sizeof(SyncBlock));\n\n");

    /* ====== Progress Tracking Pattern ====== */
    printf("Progress Tracking Pattern:\n");
    printf("  \n");
    printf("  AICORE reports progress, AICPU monitors.\n");
    printf("  \n");
    printf("  AICORE (each block):\n");
    printf("  ----------------------------------------\n");
    printf("  for tile in 0..my_tiles:\n");
    printf("      process_tile(tile)\n");
    printf("      ATOMIC_ADD GM[sync.processed_count], 1\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  AICPU (monitor thread):\n");
    printf("  ----------------------------------------\n");
    printf("  while (1) {\n");
    printf("      int32_t done;\n");
    printf("      platform_atomic_read(&sync.processed_count, &done);\n");
    printf("      printf(\"Progress: %%d/%%d\\n\", done, total);\n");
    printf("      if (done >= total) break;\n");
    printf("      usleep(10000);  // 10ms poll\n");
    printf("  }\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Distributed Barrier ====== */
    printf("Distributed Barrier with HBM Atomics:\n");
    printf("  \n");
    printf("  Unlike register barriers, HBM barriers scale better.\n");
    printf("  \n");
    printf("  AICORE block:\n");
    printf("  ----------------------------------------\n");
    printf("  # Arrive at barrier\n");
    printf("  ATOMIC_ADD old, GM[sync.barrier_count], 1\n");
    printf("  \n");
    printf("  # Last block to arrive?\n");
    printf("  CMP old, num_blocks - 1\n");
    printf("  JNE .wait_release\n");
    printf("  \n");
    printf("  # I'm the last one - release all\n");
    printf("  STORE GM[sync.release_flag], 1\n");
    printf("  JMP .barrier_done\n");
    printf("  \n");
    printf("  .wait_release:\n");
    printf("      LOAD flag, GM[sync.release_flag]\n");
    printf("      CMP flag, 0\n");
    printf("      JEQ .wait_release\n");
    printf("  \n");
    printf("  .barrier_done:\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  Advantage: No AICPU involvement needed!\n\n");

    /* ====== Work Stealing ====== */
    printf("Work Stealing with Atomic Index:\n");
    printf("  \n");
    printf("  Dynamic load balancing between AICPU and AICORE.\n");
    printf("  \n");
    printf("  AICPU can add work:\n");
    printf("  ----------------------------------------\n");
    printf("  void add_work_batch(int count) {\n");
    printf("      // Just increment available work count\n");
    printf("      // (actual data already in work queue)\n");
    printf("      platform_atomic_add(&sync.available_work, count);\n");
    printf("  }\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  AICORE claims work:\n");
    printf("  ----------------------------------------\n");
    printf("  .get_work:\n");
    printf("      ATOMIC_ADD my_idx, GM[sync.work_index], 1\n");
    printf("      LOAD avail, GM[sync.available_work]\n");
    printf("      CMP my_idx, avail\n");
    printf("      JGE .no_work    # Nothing available\n");
    printf("      \n");
    printf("      process(work_items[my_idx])\n");
    printf("      JMP .get_work\n");
    printf("  \n");
    printf("  .no_work:\n");
    printf("      # Could wait for more or exit\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Error Handling ====== */
    printf("Error Signaling Pattern:\n");
    printf("  \n");
    printf("  Any processor can signal error, all see it.\n");
    printf("  \n");
    printf("  AICORE (on error):\n");
    printf("  ----------------------------------------\n");
    printf("  # Detect error condition\n");
    printf("  CMP status, ERROR_CODE\n");
    printf("  JNE .no_error\n");
    printf("  \n");
    printf("  # Signal error (atomic to avoid races)\n");
    printf("  ATOMIC_OR old, GM[sync.error_flag], ERROR_CODE\n");
    printf("  JMP .abort\n");
    printf("  \n");
    printf("  .no_error:\n");
    printf("  # Check if another block signaled error\n");
    printf("  LOAD err, GM[sync.error_flag]\n");
    printf("  CMP err, 0\n");
    printf("  JNE .abort\n");
    printf("  \n");
    printf("  # Continue normal processing...\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  AICPU (monitor):\n");
    printf("  ----------------------------------------\n");
    printf("  int32_t err;\n");
    printf("  platform_atomic_read(&sync.error_flag, &err);\n");
    printf("  if (err != 0) {\n");
    printf("      printf(\"Error detected: 0x%%x\\n\", err);\n");
    printf("      // Initiate cleanup...\n");
    printf("  }\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Two-Phase Commit ====== */
    printf("Two-Phase Commit Pattern:\n");
    printf("  \n");
    printf("  Coordinate multi-step operations.\n");
    printf("  \n");
    printf("  Phase 1 - Prepare:\n");
    printf("  - Each participant prepares its work\n");
    printf("  - Atomic increment prepare_count\n");
    printf("  - Wait for all to prepare\n");
    printf("  \n");
    printf("  Phase 2 - Commit:\n");
    printf("  - Coordinator checks all prepared\n");
    printf("  - Sets commit_flag\n");
    printf("  - All participants finalize\n");
    printf("  \n");
    printf("  struct TwoPhaseBlock {\n");
    printf("      int32_t prepare_count;\n");
    printf("      int32_t commit_flag;\n");
    printf("      int32_t abort_flag;\n");
    printf("  };\n\n");

    /* ====== Performance Considerations ====== */
    printf("Performance Considerations:\n");
    printf("  \n");
    printf("  +--------------------------------------------------+\n");
    printf("  | Operation      | Latency  | Contention Impact   |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | atomic_add     | ~500ns   | Serializes at addr  |\n");
    printf("  | atomic_cas     | ~600ns   | May retry on fail   |\n");
    printf("  | Regular load   | ~200ns   | No contention       |\n");
    printf("  | Regular store  | ~200ns   | No contention       |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  \n");
    printf("  Tips:\n");
    printf("  1. Batch updates: Do local work, atomic once at end\n");
    printf("  2. Reduce contention: Use per-block counters, sum later\n");
    printf("  3. Avoid CAS loops: Prefer atomic_add when possible\n");
    printf("  4. Align atomic addresses: 4-byte for int32\n\n");

    /* ====== Demo ====== */
    printf("Demo: Sync block allocation\n");
    printf("  ----------------------------------------\n");

    SyncBlock* sync = (SyncBlock*)platform_malloc(sizeof(SyncBlock));
    printf("  Allocated SyncBlock at %p\n", (void*)sync);
    printf("  Size: %zu bytes\n", sizeof(SyncBlock));

    /* Initialize */
    sync->ready_count = 0;
    sync->processed_count = 0;
    sync->barrier_count = 0;
    sync->release_flag = 0;
    sync->work_index = 0;
    sync->error_flag = 0;

    printf("  Initialized all counters to 0\n");
    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(sync);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of Atomic Synchronization Example ===\n");
    return 0;
}
