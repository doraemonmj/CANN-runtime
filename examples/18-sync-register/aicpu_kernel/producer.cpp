/**
 * AICPU Producer Kernel - Prepares data and signals AICORE
 *
 * Demonstrates register-based signaling from AICPU to AICORE.
 */

#include <cstdint>
#include <cstdio>

/* Shared register indices */
#define REG_DATA_READY    0
#define REG_DATA_SIZE     1
#define REG_COMPUTE_DONE  2

/* Platform register access (provided by runtime) */
extern "C" {
    void platform_write_shared_reg(int reg_id, uint32_t value);
    uint32_t platform_read_shared_reg(int reg_id);
    void platform_memory_barrier(void);
}

/**
 * Producer kernel arguments
 */
struct ProducerArgs {
    float* output_buffer;   /* HBM buffer to fill with data */
    int32_t count;          /* Number of elements to produce */
    int32_t batch_id;       /* Batch identifier */
};

/**
 * Producer kernel entry point
 *
 * 1. Prepares data in HBM buffer
 * 2. Signals AICORE via shared register
 * 3. Waits for AICORE to complete processing
 */
extern "C" int aicpu_producer_kernel(void* args_ptr) {
    ProducerArgs* args = static_cast<ProducerArgs*>(args_ptr);

    printf("[AICPU] Producer starting batch %d, count=%d\n",
           args->batch_id, args->count);

    /* Step 1: Prepare data in HBM buffer */
    for (int32_t i = 0; i < args->count; i++) {
        /* Generate test data: batch_id * 1000 + index */
        args->output_buffer[i] = static_cast<float>(args->batch_id * 1000 + i);
    }

    /* Ensure all writes are visible before signaling */
    platform_memory_barrier();

    printf("[AICPU] Data prepared, signaling AICORE...\n");

    /* Step 2: Signal data ready via shared register */
    platform_write_shared_reg(REG_DATA_SIZE, args->count);
    platform_write_shared_reg(REG_DATA_READY, args->batch_id + 1);  /* Non-zero = ready */

    /* Step 3: Wait for AICORE to complete */
    printf("[AICPU] Waiting for AICORE to complete...\n");

    int timeout = 1000000;  /* 1 second timeout at ~1us per iteration */
    while (timeout > 0) {
        uint32_t done = platform_read_shared_reg(REG_COMPUTE_DONE);
        if (done >= args->batch_id + 1) {
            printf("[AICPU] AICORE completed (done=%u)\n", done);
            break;
        }
        timeout--;
    }

    if (timeout == 0) {
        printf("[AICPU] ERROR: Timeout waiting for AICORE\n");
        return -1;
    }

    printf("[AICPU] Producer batch %d finished\n", args->batch_id);
    return 0;
}

/**
 * Multi-batch producer - demonstrates pipeline pattern
 */
struct MultiBatchArgs {
    float* buffer;          /* Double-buffered HBM area */
    int32_t count_per_batch;
    int32_t num_batches;
};

extern "C" int aicpu_multi_batch_producer(void* args_ptr) {
    MultiBatchArgs* args = static_cast<MultiBatchArgs*>(args_ptr);

    printf("[AICPU] Multi-batch producer: %d batches of %d elements\n",
           args->num_batches, args->count_per_batch);

    size_t batch_size = args->count_per_batch * sizeof(float);

    for (int32_t batch = 0; batch < args->num_batches; batch++) {
        /* Use ping-pong buffers */
        float* buffer = args->buffer + (batch % 2) * args->count_per_batch;

        printf("[AICPU] Preparing batch %d in buffer %d\n", batch, batch % 2);

        /* Prepare this batch's data */
        for (int32_t i = 0; i < args->count_per_batch; i++) {
            buffer[i] = static_cast<float>(batch * 1000 + i);
        }

        platform_memory_barrier();

        /* Signal this batch is ready */
        platform_write_shared_reg(REG_DATA_SIZE, args->count_per_batch);
        platform_write_shared_reg(REG_DATA_READY, batch + 1);

        /* Wait for previous batch to complete (flow control) */
        /* This prevents AICPU from getting too far ahead */
        if (batch > 0) {
            while (platform_read_shared_reg(REG_COMPUTE_DONE) < batch) {
                /* spin */
            }
        }
    }

    /* Wait for all batches to complete */
    while (platform_read_shared_reg(REG_COMPUTE_DONE) < args->num_batches) {
        /* spin */
    }

    printf("[AICPU] All batches completed\n");
    return 0;
}
