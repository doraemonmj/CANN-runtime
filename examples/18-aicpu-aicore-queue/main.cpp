/**
 * 20-sync-queue - Queue-Based Data Passing
 *
 * This example demonstrates:
 * - Lock-free queues in HBM for producer-consumer patterns
 * - SPSC (Single Producer, Single Consumer) queues
 * - MPMC (Multi Producer, Multi Consumer) queues
 * - Task queues for dynamic workload distribution
 * - Pipeline stages connected by queues
 *
 * Queues provide the highest bandwidth synchronization mechanism
 * when data needs to pass between processors.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

/**
 * Single Producer Single Consumer (SPSC) Queue
 * - Lock-free
 * - One writer (AICPU), one reader (AICORE)
 * - Fixed size ring buffer
 */
struct SPSCQueue {
    int32_t head;           /* Write position (producer) */
    int32_t tail;           /* Read position (consumer) */
    int32_t capacity;       /* Queue size */
    int32_t item_size;      /* Size of each item in bytes */
    uint8_t data[];         /* Ring buffer data */
};

/**
 * Multi Producer Multi Consumer (MPMC) Queue
 * - Lock-free using CAS
 * - Multiple writers and readers
 * - More overhead than SPSC
 */
struct MPMCQueue {
    int32_t head;           /* Claimed write position */
    int32_t head_committed; /* Committed write position */
    int32_t tail;           /* Claimed read position */
    int32_t tail_committed; /* Committed read position */
    int32_t capacity;
    int32_t item_size;
    uint8_t data[];
};

/**
 * Task descriptor for task queue
 */
struct TaskDesc {
    int32_t task_id;
    int32_t task_type;
    int32_t priority;
    int32_t data_offset;    /* Offset into data buffer */
    int32_t data_size;
};

int main() {
    printf("=== Queue-Based Data Passing ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Queue Overview ====== */
    printf("Queue-Based Communication Overview:\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  |  Queues provide structured data passing between       |\n");
    printf("  |  AICPU and AICORE with flow control                   |\n");
    printf("  |                                                       |\n");
    printf("  |  +--------+      Queue (HBM)       +--------+         |\n");
    printf("  |  |Producer| ---> [|||||||] ---->  |Consumer|         |\n");
    printf("  |  | AICPU  |      head    tail     | AICORE |         |\n");
    printf("  |  +--------+                        +--------+         |\n");
    printf("  |                                                       |\n");
    printf("  |  Benefits:                                            |\n");
    printf("  |  - Decouples producer from consumer speed             |\n");
    printf("  |  - Natural flow control (full/empty)                  |\n");
    printf("  |  - Batching for efficiency                            |\n");
    printf("  +-------------------------------------------------------+\n\n");

    /* ====== SPSC Queue ====== */
    printf("SPSC (Single Producer Single Consumer) Queue:\n");
    printf("  \n");
    printf("  struct SPSCQueue {      // In HBM\n");
    printf("      int32_t head;       // Producer writes here\n");
    printf("      int32_t tail;       // Consumer reads here\n");
    printf("      int32_t capacity;\n");
    printf("      int32_t item_size;\n");
    printf("      uint8_t data[];     // Ring buffer\n");
    printf("  };\n");
    printf("  \n");
    printf("  Producer (AICPU):                   Consumer (AICORE):\n");
    printf("  ----------------------------------------\n");
    printf("  bool push(item):                    bool pop(&item):\n");
    printf("      next = (head + 1) %% cap            if (tail == head)\n");
    printf("      if (next == tail)                       return false  // empty\n");
    printf("          return false  // full           copy(item, data[tail])\n");
    printf("      copy(data[head], item)              tail = (tail + 1) %% cap\n");
    printf("      head = next                         return true\n");
    printf("      return true\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  Properties:\n");
    printf("  - Lock-free (no atomics needed for SPSC)\n");
    printf("  - One cache line for head, one for tail\n");
    printf("  - Throughput: millions of items/sec\n\n");

    /* ====== MPMC Queue ====== */
    printf("MPMC (Multi Producer Multi Consumer) Queue:\n");
    printf("  \n");
    printf("  struct MPMCQueue {\n");
    printf("      int32_t head;            // Claimed write pos\n");
    printf("      int32_t head_committed;  // Visible write pos\n");
    printf("      int32_t tail;            // Claimed read pos\n");
    printf("      int32_t tail_committed;  // Visible read pos\n");
    printf("      ...\n");
    printf("  };\n");
    printf("  \n");
    printf("  Push (uses CAS):                   Pop (uses CAS):\n");
    printf("  ----------------------------------------\n");
    printf("  1. CAS claim head slot             1. CAS claim tail slot\n");
    printf("  2. Copy data to slot               2. Wait for data ready\n");
    printf("  3. Wait turn to commit             3. Copy data from slot\n");
    printf("  4. Increment head_committed        4. Increment tail_committed\n");
    printf("  ----------------------------------------\n");
    printf("  \n");
    printf("  Use cases:\n");
    printf("  - Multiple AICPU threads producing\n");
    printf("  - Multiple AICORE blocks consuming\n");
    printf("  - Load balancing across consumers\n\n");

    /* ====== Queue Operations Pseudocode ====== */
    printf("Queue Operations (SPSC - AICPU producer):\n");
    printf("  \n");
    printf("  // Initialize queue\n");
    printf("  void queue_init(SPSCQueue* q, int capacity, int item_size) {\n");
    printf("      q->head = 0;\n");
    printf("      q->tail = 0;\n");
    printf("      q->capacity = capacity;\n");
    printf("      q->item_size = item_size;\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Push item (producer side)\n");
    printf("  bool queue_push(SPSCQueue* q, void* item) {\n");
    printf("      int next = (q->head + 1) %% q->capacity;\n");
    printf("      if (next == q->tail) return false;  // Full\n");
    printf("      \n");
    printf("      memcpy(&q->data[q->head * q->item_size], item, q->item_size);\n");
    printf("      __sync_synchronize();  // Memory barrier\n");
    printf("      q->head = next;\n");
    printf("      return true;\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Pop item (consumer side - in AICORE kernel)\n");
    printf("  // See kernel.pto for PTO-ISA implementation\n\n");

    /* ====== Task Queue Pattern ====== */
    printf("Task Queue Pattern:\n");
    printf("  \n");
    printf("  AICPU creates tasks, AICORE processes them.\n");
    printf("  \n");
    printf("  struct TaskDesc {\n");
    printf("      int32_t task_id;\n");
    printf("      int32_t task_type;    // COMPUTE, COPY, REDUCE, etc.\n");
    printf("      int32_t priority;\n");
    printf("      int32_t data_offset;  // Where task data is\n");
    printf("      int32_t data_size;\n");
    printf("  };\n");
    printf("  \n");
    printf("  Flow:\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  | AICPU                     | AICORE                    |\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  | 1. Prepare task data      |                           |\n");
    printf("  | 2. Create TaskDesc        |                           |\n");
    printf("  | 3. Push to queue          |                           |\n");
    printf("  |                           | 4. Pop TaskDesc           |\n");
    printf("  |                           | 5. Load task data         |\n");
    printf("  |                           | 6. Execute based on type  |\n");
    printf("  |                           | 7. Store result           |\n");
    printf("  |                           | 8. Signal completion      |\n");
    printf("  | 9. Collect results        |                           |\n");
    printf("  +-------------------------------------------------------+\n\n");

    /* ====== Pipeline Pattern ====== */
    printf("Pipeline with Multiple Queues:\n");
    printf("  \n");
    printf("  Stage1 --Q1--> Stage2 --Q2--> Stage3\n");
    printf("  \n");
    printf("  Example: Image processing pipeline\n");
    printf("  \n");
    printf("  [AICPU Load] --Q1--> [AICORE Process] --Q2--> [AICPU Save]\n");
    printf("  \n");
    printf("  // Stage 1: Load images (AICPU)\n");
    printf("  for (img in images) {\n");
    printf("      load_image_to_hbm(img, buffer);\n");
    printf("      queue_push(q1, {buffer, size});\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Stage 2: Process (AICORE) - see kernel\n");
    printf("  \n");
    printf("  // Stage 3: Save results (AICPU)\n");
    printf("  while (results_remaining) {\n");
    printf("      if (queue_pop(q2, &result)) {\n");
    printf("          save_to_disk(result);\n");
    printf("      }\n");
    printf("  }\n\n");

    /* ====== Memory Layout ====== */
    printf("Queue Memory Layout:\n");
    printf("  \n");
    printf("  | Offset | Field          | Size     |\n");
    printf("  |--------|----------------|----------|\n");
    printf("  | 0      | head           | 4 bytes  | <- Cache line 0\n");
    printf("  | 4      | padding        | 60 bytes |\n");
    printf("  | 64     | tail           | 4 bytes  | <- Cache line 1\n");
    printf("  | 68     | padding        | 60 bytes |\n");
    printf("  | 128    | capacity       | 4 bytes  | <- Cache line 2\n");
    printf("  | 132    | item_size      | 4 bytes  |\n");
    printf("  | 136    | padding        | 56 bytes |\n");
    printf("  | 192    | data[0]        | varies   | <- Data starts\n");
    printf("  | ...    | data[N-1]      |          |\n");
    printf("  \n");
    printf("  Key: Separate cache lines for head/tail to avoid false sharing!\n\n");

    /* ====== Performance Tips ====== */
    printf("Performance Tips:\n");
    printf("  \n");
    printf("  1. Batch operations:\n");
    printf("     - Push/pop multiple items at once\n");
    printf("     - Reduces synchronization overhead\n");
    printf("  \n");
    printf("  2. Size appropriately:\n");
    printf("     - Too small: frequent full/empty\n");
    printf("     - Too large: wasted memory, cache pollution\n");
    printf("     - Rule of thumb: 2-4x expected in-flight items\n");
    printf("  \n");
    printf("  3. Align for DMA:\n");
    printf("     - Item size multiple of 32 bytes\n");
    printf("     - Queue start aligned to 64 bytes\n");
    printf("  \n");
    printf("  4. Use SPSC when possible:\n");
    printf("     - Much simpler and faster than MPMC\n");
    printf("     - One queue per AICORE block if needed\n\n");

    /* ====== Demo ====== */
    printf("Demo: SPSC Queue allocation\n");
    printf("  ----------------------------------------\n");

    int capacity = 256;
    int item_size = 64;  /* 64-byte items */
    size_t queue_size = sizeof(SPSCQueue) + capacity * item_size;

    SPSCQueue* queue = (SPSCQueue*)platform_malloc(queue_size);
    printf("  Allocated SPSC queue at %p\n", (void*)queue);
    printf("  Capacity: %d items\n", capacity);
    printf("  Item size: %d bytes\n", item_size);
    printf("  Total size: %zu bytes\n", queue_size);

    /* Initialize */
    queue->head = 0;
    queue->tail = 0;
    queue->capacity = capacity;
    queue->item_size = item_size;

    printf("  Queue initialized (head=0, tail=0)\n");
    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(queue);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of Queue Synchronization Example ===\n");
    return 0;
}
