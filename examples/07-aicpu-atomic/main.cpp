/**
 * 09-aicpu-atomic - Multi-core AICPU Synchronization with Atomics
 *
 * This example demonstrates:
 * - C++11 atomic operations on AICPU
 * - Memory ordering (acquire, release, seq_cst)
 * - Atomic counters for work distribution
 * - Compare-and-swap patterns
 * - Barriers across AICPU cores
 *
 * AICPU cores share HBM, so atomics work like multi-threaded code.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <atomic>
#include "platform.h"

/*
 * Shared state in HBM - accessible by all AICPU cores
 */
struct SharedState {
    std::atomic<int32_t> counter;      /* Atomic counter */
    std::atomic<int32_t> work_index;   /* Next work item to claim */
    std::atomic<int32_t> done_count;   /* Number of workers done */
    int32_t total_work;                /* Total work items */
    int32_t _pad[4];                   /* Padding to cache line */
};

struct AtomicKernelArgs {
    void* input;
    void* output;
    int32_t count;
    int32_t worker_id;       /* Which AICPU core (0-7 typical) */
    SharedState* shared;     /* Pointer to shared state in HBM */
    int32_t _pad;
};

int main() {
    printf("=== AICPU Atomic Operations Example ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Atomic Types ====== */
    printf("Available atomic types on AICPU (ARM Cortex-A55):\n");
    printf("  std::atomic<int32_t>   - 32-bit atomic integer\n");
    printf("  std::atomic<int64_t>   - 64-bit atomic integer\n");
    printf("  std::atomic<uint32_t>  - 32-bit unsigned\n");
    printf("  std::atomic<void*>     - pointer atomics\n");
    printf("  std::atomic_flag       - lock-free boolean flag\n\n");

    /* ====== Memory Ordering ====== */
    printf("Memory ordering options:\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  | Ordering       | Use When                               |\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  | relaxed        | Counter only, no data dependencies     |\n");
    printf("  | acquire        | Reading flag that guards other data    |\n");
    printf("  | release        | Writing flag after data is ready       |\n");
    printf("  | acq_rel        | Both (e.g., in CAS loops)              |\n");
    printf("  | seq_cst        | Need total ordering (usually overkill) |\n");
    printf("  +---------------------------------------------------------+\n\n");

    /* ====== Pattern 1: Atomic Counter ====== */
    printf("Pattern 1: Atomic Counter\n");
    printf("  // Shared state in HBM\n");
    printf("  std::atomic<int32_t> counter{0};\n");
    printf("  \n");
    printf("  // Worker kernel\n");
    printf("  void worker(Args* args) {\n");
    printf("      for (int i = 0; i < my_work; i++) {\n");
    printf("          process(i);\n");
    printf("          args->shared->counter.fetch_add(1, relaxed);\n");
    printf("      }\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Host can poll counter for progress\n\n");

    /* ====== Pattern 2: Work Stealing ====== */
    printf("Pattern 2: Work Stealing (dynamic distribution)\n");
    printf("  struct Shared {\n");
    printf("      std::atomic<int32_t> next_item{0};\n");
    printf("      int32_t total_items;\n");
    printf("  };\n");
    printf("  \n");
    printf("  void worker(Args* args) {\n");
    printf("      while (true) {\n");
    printf("          int item = args->shared->next_item.fetch_add(1, acq_rel);\n");
    printf("          if (item >= args->shared->total_items) break;\n");
    printf("          process(item);\n");
    printf("      }\n");
    printf("  }\n\n");

    /* ====== Pattern 3: Compare-and-Swap ====== */
    printf("Pattern 3: Compare-and-Swap (CAS)\n");
    printf("  // Only update if value hasn't changed\n");
    printf("  int expected = counter.load();\n");
    printf("  while (!counter.compare_exchange_weak(\n");
    printf("             expected, expected + delta,\n");
    printf("             memory_order_acq_rel)) {\n");
    printf("      // expected is updated with current value\n");
    printf("      // retry with new expected\n");
    printf("  }\n\n");

    /* ====== Pattern 4: Barrier ====== */
    printf("Pattern 4: Barrier (wait for all workers)\n");
    printf("  struct Shared {\n");
    printf("      std::atomic<int32_t> arrived{0};\n");
    printf("      std::atomic<int32_t> generation{0};\n");
    printf("      int32_t num_workers;\n");
    printf("  };\n");
    printf("  \n");
    printf("  void barrier(Shared* s) {\n");
    printf("      int gen = s->generation.load(acquire);\n");
    printf("      if (s->arrived.fetch_add(1, acq_rel) == s->num_workers - 1) {\n");
    printf("          // Last to arrive: reset and advance generation\n");
    printf("          s->arrived.store(0, relaxed);\n");
    printf("          s->generation.fetch_add(1, release);\n");
    printf("      } else {\n");
    printf("          // Wait for generation to change\n");
    printf("          while (s->generation.load(acquire) == gen) { /*spin*/ }\n");
    printf("      }\n");
    printf("  }\n\n");

    /* ====== Demo: Simulate atomic operations ====== */
    printf("Demo: Simulating multi-worker atomic counter\n");
    printf("  ----------------------------------------\n");

    /* Allocate shared state in device memory */
    SharedState* dev_shared = (SharedState*)platform_malloc(sizeof(SharedState));

    /* Initialize shared state */
    SharedState init_state;
    init_state.counter.store(0);
    init_state.work_index.store(0);
    init_state.done_count.store(0);
    init_state.total_work = 100;
    platform_memcpy_h2d(dev_shared, &init_state, sizeof(SharedState));

    printf("  Shared state allocated at %p\n", (void*)dev_shared);
    printf("  Total work items: %d\n", init_state.total_work);
    printf("  Simulating 4 workers...\n");

    /* Simulate 4 workers each processing items */
    int simulated_counter = 0;
    for (int worker = 0; worker < 4; worker++) {
        int items_processed = 25;  /* Each worker gets ~25 items */
        simulated_counter += items_processed;
        printf("    Worker %d: processed %d items (total: %d)\n",
               worker, items_processed, simulated_counter);
    }

    printf("  Final counter: %d\n", simulated_counter);
    printf("  ----------------------------------------\n\n");

    /* ====== Performance Tips ====== */
    printf("Performance tips:\n");
    printf("  1. Use relaxed ordering when possible (fastest)\n");
    printf("  2. Avoid contention - spread atomics across cache lines\n");
    printf("  3. Use fetch_add instead of CAS loops for counters\n");
    printf("  4. Batch local work, update atomics less frequently\n");
    printf("  5. Consider lock-free data structures for complex patterns\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(dev_shared);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of AICPU Atomic Example ===\n");
    return 0;
}
