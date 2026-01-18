/**
 * AICPU Monitor Kernel - Tracks AICORE progress using HBM atomics
 */

#include <cstdint>
#include <cstdio>
#include <atomic>
#include <unistd.h>

/* Synchronization block layout (matches AICORE) */
struct SyncBlock {
    int32_t ready_count;
    int32_t processed_count;
    int32_t barrier_count;
    int32_t release_flag;
    int32_t work_index;
    int32_t error_flag;
    int32_t padding[2];
};

/* Platform atomic operations (provided by runtime) */
extern "C" {
    void platform_atomic_add(volatile int32_t* addr, int32_t val);
    int32_t platform_atomic_read(volatile int32_t* addr);
    void platform_atomic_write(volatile int32_t* addr, int32_t val);
    int32_t platform_atomic_cas(volatile int32_t* addr, int32_t expected, int32_t desired);
}

/**
 * Progress monitor - tracks AICORE kernel execution
 */
struct MonitorArgs {
    SyncBlock* sync;
    int32_t expected_total;
    int32_t poll_interval_us;
};

extern "C" int aicpu_progress_monitor(void* args_ptr) {
    MonitorArgs* args = static_cast<MonitorArgs*>(args_ptr);
    SyncBlock* sync = args->sync;

    printf("[AICPU Monitor] Starting, expecting %d items\n", args->expected_total);

    int32_t last_progress = 0;

    while (1) {
        /* Read progress atomically */
        int32_t progress = platform_atomic_read(&sync->processed_count);

        /* Check for errors */
        int32_t err = platform_atomic_read(&sync->error_flag);
        if (err != 0) {
            printf("[AICPU Monitor] ERROR detected: 0x%x\n", err);
            return -1;
        }

        /* Report progress if changed */
        if (progress != last_progress) {
            printf("[AICPU Monitor] Progress: %d/%d (%.1f%%)\n",
                   progress, args->expected_total,
                   100.0f * progress / args->expected_total);
            last_progress = progress;
        }

        /* Check completion */
        if (progress >= args->expected_total) {
            printf("[AICPU Monitor] Complete!\n");
            break;
        }

        /* Sleep between polls */
        usleep(args->poll_interval_us);
    }

    return 0;
}

/**
 * Barrier controller - manages distributed barrier
 */
struct BarrierArgs {
    SyncBlock* sync;
    int32_t num_participants;
    int32_t timeout_ms;
};

extern "C" int aicpu_barrier_controller(void* args_ptr) {
    BarrierArgs* args = static_cast<BarrierArgs*>(args_ptr);
    SyncBlock* sync = args->sync;

    printf("[AICPU Barrier] Waiting for %d participants\n", args->num_participants);

    int elapsed_ms = 0;

    /* Wait for all participants to arrive */
    while (1) {
        int32_t arrived = platform_atomic_read(&sync->barrier_count);

        if (arrived >= args->num_participants) {
            printf("[AICPU Barrier] All %d participants arrived\n", arrived);
            break;
        }

        /* Timeout check */
        if (elapsed_ms >= args->timeout_ms) {
            printf("[AICPU Barrier] TIMEOUT: only %d/%d arrived\n",
                   arrived, args->num_participants);
            platform_atomic_add(&sync->error_flag, 1);  /* Signal error */
            return -1;
        }

        usleep(1000);  /* 1ms poll */
        elapsed_ms++;
    }

    /* Release all waiters */
    printf("[AICPU Barrier] Releasing barrier\n");
    platform_atomic_write(&sync->release_flag, 1);

    /* Reset for next barrier (after small delay for safety) */
    usleep(1000);
    platform_atomic_write(&sync->barrier_count, 0);
    platform_atomic_write(&sync->release_flag, 0);

    printf("[AICPU Barrier] Reset complete\n");
    return 0;
}

/**
 * Work coordinator - manages dynamic work distribution
 */
struct WorkCoordArgs {
    SyncBlock* sync;
    int32_t* work_queue;     /* Array of work items */
    int32_t total_items;
    int32_t* results;        /* Output results */
};

extern "C" int aicpu_work_coordinator(void* args_ptr) {
    WorkCoordArgs* args = static_cast<WorkCoordArgs*>(args_ptr);
    SyncBlock* sync = args->sync;

    printf("[AICPU Work] Initializing %d work items\n", args->total_items);

    /* Initialize work queue with items */
    for (int32_t i = 0; i < args->total_items; i++) {
        args->work_queue[i] = i * 100;  /* Example work data */
    }

    /* Initialize sync block */
    platform_atomic_write(&sync->work_index, 0);
    platform_atomic_write(&sync->processed_count, 0);

    printf("[AICPU Work] Work queue ready, waiting for completion\n");

    /* Monitor until all work complete */
    while (1) {
        int32_t processed = platform_atomic_read(&sync->processed_count);

        if (processed >= args->total_items) {
            printf("[AICPU Work] All items processed\n");
            break;
        }

        usleep(10000);  /* 10ms poll */
    }

    /* Verify results */
    int32_t errors = 0;
    for (int32_t i = 0; i < args->total_items; i++) {
        /* Expected: work_item * 2 + 1 */
        int32_t expected = args->work_queue[i] * 2 + 1;
        if (args->results[i] != expected) {
            printf("[AICPU Work] Error at %d: expected %d, got %d\n",
                   i, expected, args->results[i]);
            errors++;
        }
    }

    printf("[AICPU Work] Verification complete, %d errors\n", errors);
    return errors > 0 ? -1 : 0;
}

/**
 * Error handler - responds to error signals
 */
struct ErrorHandlerArgs {
    SyncBlock* sync;
    int32_t poll_interval_us;
};

extern "C" int aicpu_error_handler(void* args_ptr) {
    ErrorHandlerArgs* args = static_cast<ErrorHandlerArgs*>(args_ptr);
    SyncBlock* sync = args->sync;

    printf("[AICPU Error Handler] Started\n");

    while (1) {
        int32_t err = platform_atomic_read(&sync->error_flag);

        if (err != 0) {
            printf("[AICPU Error Handler] Error detected: 0x%x\n", err);

            /* Decode error flags */
            if (err & 0x1) printf("  - Memory allocation failed\n");
            if (err & 0x2) printf("  - Computation overflow\n");
            if (err & 0x4) printf("  - DMA timeout\n");
            if (err & 0x8) printf("  - Invalid data\n");

            /* Initiate cleanup */
            printf("[AICPU Error Handler] Initiating cleanup...\n");

            /* Signal all processors to abort */
            platform_atomic_write(&sync->release_flag, -1);  /* -1 = abort */

            return err;
        }

        usleep(args->poll_interval_us);
    }

    return 0;
}
