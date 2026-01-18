/**
 * 10-aicpu-queue - Lock-free Queue for Producer-Consumer Pattern
 *
 * This example demonstrates:
 * - SPSC (Single Producer, Single Consumer) queue
 * - MPMC (Multiple Producer, Multiple Consumer) queue
 * - Lock-free data structures in HBM
 * - Producer-consumer coordination patterns
 *
 * Queues enable efficient data passing between:
 * - Multiple AICPU kernels
 * - AICPU and AICORE (covered in sync examples)
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <atomic>
#include "platform.h"

/*
 * Work item that flows through the queue
 */
struct WorkItem {
    void* data_ptr;     /* Pointer to data in HBM */
    int32_t size;       /* Size of data */
    int32_t flags;      /* Control flags (e.g., FLAG_LAST) */
};

#define FLAG_NONE 0
#define FLAG_LAST 1

/*
 * SPSC Queue - Single Producer, Single Consumer
 * Lock-free, wait-free for both push and pop
 */
template <typename T, int CAPACITY>
struct SPSCQueue {
    T data[CAPACITY];
    std::atomic<uint32_t> head{0};  /* Write position (producer) */
    std::atomic<uint32_t> tail{0};  /* Read position (consumer) */
    int32_t _pad[14];               /* Pad to avoid false sharing */

    bool push(const T& item) {
        uint32_t h = head.load(std::memory_order_relaxed);
        uint32_t next = (h + 1) % CAPACITY;

        if (next == tail.load(std::memory_order_acquire)) {
            return false;  /* Full */
        }

        data[h] = item;
        head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T* item) {
        uint32_t t = tail.load(std::memory_order_relaxed);

        if (t == head.load(std::memory_order_acquire)) {
            return false;  /* Empty */
        }

        *item = data[t];
        tail.store((t + 1) % CAPACITY, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return tail.load(std::memory_order_acquire) ==
               head.load(std::memory_order_acquire);
    }
};

using WorkQueue = SPSCQueue<WorkItem, 64>;

int main() {
    printf("=== AICPU Lock-free Queue Example ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Queue Types ====== */
    printf("Queue types for AICPU:\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  | Type    | Producers | Consumers | Complexity            |\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  | SPSC    | 1         | 1         | Simple, fastest       |\n");
    printf("  | MPSC    | N         | 1         | Medium                |\n");
    printf("  | SPMC    | 1         | N         | Medium                |\n");
    printf("  | MPMC    | N         | N         | Complex, CAS-based    |\n");
    printf("  +---------------------------------------------------------+\n\n");

    /* ====== SPSC Queue Structure ====== */
    printf("SPSC Queue structure:\n");
    printf("  template <typename T, int CAPACITY>\n");
    printf("  struct SPSCQueue {\n");
    printf("      T data[CAPACITY];\n");
    printf("      std::atomic<uint32_t> head{0};  // Producer writes here\n");
    printf("      std::atomic<uint32_t> tail{0};  // Consumer reads here\n");
    printf("  };\n\n");

    /* ====== Push Operation ====== */
    printf("Push operation (producer):\n");
    printf("  bool push(const T& item) {\n");
    printf("      uint32_t h = head.load(relaxed);\n");
    printf("      uint32_t next = (h + 1) %% CAPACITY;\n");
    printf("      \n");
    printf("      if (next == tail.load(acquire))  // Full?\n");
    printf("          return false;\n");
    printf("      \n");
    printf("      data[h] = item;\n");
    printf("      head.store(next, release);  // Make visible\n");
    printf("      return true;\n");
    printf("  }\n\n");

    /* ====== Pop Operation ====== */
    printf("Pop operation (consumer):\n");
    printf("  bool pop(T* item) {\n");
    printf("      uint32_t t = tail.load(relaxed);\n");
    printf("      \n");
    printf("      if (t == head.load(acquire))  // Empty?\n");
    printf("          return false;\n");
    printf("      \n");
    printf("      *item = data[t];\n");
    printf("      tail.store((t + 1) %% CAPACITY, release);\n");
    printf("      return true;\n");
    printf("  }\n\n");

    /* ====== Demo: Allocate queue in HBM ====== */
    printf("Demo: Queue in HBM\n");
    printf("  ----------------------------------------\n");

    /* Allocate queue in device memory */
    WorkQueue* dev_queue = (WorkQueue*)platform_malloc(sizeof(WorkQueue));
    printf("  Queue allocated at %p (size: %zu bytes)\n",
           (void*)dev_queue, sizeof(WorkQueue));

    /* Initialize queue (clear head/tail) */
    WorkQueue init_queue;
    init_queue.head.store(0);
    init_queue.tail.store(0);
    platform_memcpy_h2d(dev_queue, &init_queue, sizeof(WorkQueue));
    printf("  Queue initialized (head=0, tail=0)\n\n");

    /* Simulate producer pushing items */
    printf("  Simulating producer (5 items):\n");
    for (int i = 0; i < 5; i++) {
        WorkItem item;
        item.data_ptr = (void*)(uintptr_t)(0x1000 + i * 0x100);
        item.size = 1024;
        item.flags = (i == 4) ? FLAG_LAST : FLAG_NONE;

        printf("    Push: data=%p size=%d flags=%d\n",
               item.data_ptr, item.size, item.flags);
    }
    printf("\n");

    /* Simulate consumer popping items */
    printf("  Simulating consumer:\n");
    for (int i = 0; i < 5; i++) {
        printf("    Pop: item %d processed\n", i);
    }
    printf("    Got FLAG_LAST, consumer exits\n");
    printf("  ----------------------------------------\n\n");

    /* ====== Producer-Consumer Pattern ====== */
    printf("Producer-Consumer pattern:\n");
    printf("  // Producer kernel\n");
    printf("  void producer(ProducerArgs* args) {\n");
    printf("      for (int i = 0; i < num_items; i++) {\n");
    printf("          WorkItem item = prepare_item(i);\n");
    printf("          while (!queue->push(item)) { /*spin*/ }\n");
    printf("      }\n");
    printf("      // Push termination signal\n");
    printf("      WorkItem last = {0, 0, FLAG_LAST};\n");
    printf("      while (!queue->push(last)) { /*spin*/ }\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Consumer kernel\n");
    printf("  void consumer(ConsumerArgs* args) {\n");
    printf("      while (true) {\n");
    printf("          WorkItem item;\n");
    printf("          if (queue->pop(&item)) {\n");
    printf("              if (item.flags & FLAG_LAST) break;\n");
    printf("              process(item);\n");
    printf("          }\n");
    printf("      }\n");
    printf("  }\n\n");

    /* ====== Performance Tips ====== */
    printf("Performance tips:\n");
    printf("  1. Choose capacity = power of 2 for fast modulo\n");
    printf("  2. Align queue struct to cache line (64 bytes)\n");
    printf("  3. Keep items small, store data pointers\n");
    printf("  4. Batch operations when possible\n");
    printf("  5. Use backoff when spinning on full/empty queue\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(dev_queue);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of AICPU Queue Example ===\n");
    return 0;
}
