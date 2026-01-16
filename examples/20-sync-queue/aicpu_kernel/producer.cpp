/**
 * AICPU Producer Kernel - Pushes work items to queue
 */

#include <cstdint>
#include <cstdio>
#include <cstring>

/* Queue structure (must match AICORE) */
struct SPSCQueue {
    int32_t head;
    int32_t _pad1[15];      /* Padding to separate cache lines */
    int32_t tail;
    int32_t _pad2[15];
    int32_t capacity;
    int32_t item_size;
    int32_t _pad3[14];
    uint8_t data[];
};

/* Task descriptor */
struct TaskDesc {
    int32_t task_id;
    int32_t task_type;
    int32_t priority;
    int32_t data_offset;
    int32_t data_size;
    int32_t _pad[11];       /* Pad to 64 bytes */
};

#define TASK_TYPE_SCALE   1
#define TASK_TYPE_ADD     2
#define TASK_TYPE_REDUCE  3
#define TASK_TYPE_END     -1

/* Platform memory barrier */
extern "C" void platform_memory_barrier(void);

/**
 * Check if queue has space
 */
static inline bool queue_has_space(SPSCQueue* q) {
    int next = (q->head + 1) % q->capacity;
    return next != q->tail;
}

/**
 * Push item to queue
 */
static bool queue_push(SPSCQueue* q, void* item) {
    int next = (q->head + 1) % q->capacity;
    if (next == q->tail) {
        return false;  /* Queue full */
    }

    /* Copy item to queue */
    uint8_t* slot = &q->data[q->head * q->item_size];
    memcpy(slot, item, q->item_size);

    /* Memory barrier to ensure data visible before head update */
    platform_memory_barrier();

    /* Update head */
    q->head = next;
    return true;
}

/**
 * Wait and push (blocking)
 */
static void queue_push_wait(SPSCQueue* q, void* item) {
    while (!queue_push(q, item)) {
        /* Spin - could yield or sleep here */
    }
}

/**
 * Producer kernel arguments
 */
struct ProducerArgs {
    SPSCQueue* queue;
    float* data_buffer;
    int32_t num_tasks;
    int32_t elements_per_task;
};

/**
 * Producer entry point - generates tasks for AICORE
 */
extern "C" int aicpu_queue_producer(void* args_ptr) {
    ProducerArgs* args = static_cast<ProducerArgs*>(args_ptr);
    SPSCQueue* queue = args->queue;

    printf("[AICPU Producer] Starting, %d tasks of %d elements each\n",
           args->num_tasks, args->elements_per_task);

    /* Generate tasks */
    for (int32_t i = 0; i < args->num_tasks; i++) {
        /* Prepare task data in HBM buffer */
        float* task_data = args->data_buffer + i * args->elements_per_task;
        for (int32_t j = 0; j < args->elements_per_task; j++) {
            task_data[j] = static_cast<float>(i * 1000 + j);
        }

        platform_memory_barrier();

        /* Create task descriptor */
        TaskDesc task = {0};
        task.task_id = i;
        task.task_type = (i % 3) + 1;  /* Rotate through task types */
        task.priority = (args->num_tasks - i);  /* Higher ID = lower priority */
        task.data_offset = i * args->elements_per_task * sizeof(float);
        task.data_size = args->elements_per_task * sizeof(float);

        printf("[AICPU Producer] Pushing task %d (type=%d, size=%d)\n",
               task.task_id, task.task_type, task.data_size);

        /* Push to queue (blocks if full) */
        queue_push_wait(queue, &task);
    }

    /* Send end-of-work marker */
    TaskDesc end_task = {0};
    end_task.task_id = -1;
    end_task.task_type = TASK_TYPE_END;
    queue_push_wait(queue, &end_task);

    printf("[AICPU Producer] All tasks pushed, end marker sent\n");
    return 0;
}

/**
 * Pipeline stage producer - loads data from file/network
 */
struct PipelineProducerArgs {
    SPSCQueue* output_queue;
    float* staging_buffer;
    int32_t num_items;
    int32_t item_size;
};

extern "C" int aicpu_pipeline_producer(void* args_ptr) {
    PipelineProducerArgs* args = static_cast<PipelineProducerArgs*>(args_ptr);
    SPSCQueue* queue = args->output_queue;

    printf("[AICPU Pipeline Producer] Starting, %d items\n", args->num_items);

    for (int32_t i = 0; i < args->num_items; i++) {
        /* Simulate loading data (from file, network, etc.) */
        float* item_data = args->staging_buffer + i * args->item_size / sizeof(float);
        for (int32_t j = 0; j < args->item_size / sizeof(float); j++) {
            item_data[j] = static_cast<float>(i + j * 0.01f);
        }

        /* Create task pointing to this data */
        TaskDesc task = {0};
        task.task_id = i;
        task.task_type = TASK_TYPE_SCALE;
        task.data_offset = i * args->item_size;
        task.data_size = args->item_size;

        /* Push to next stage */
        queue_push_wait(queue, &task);
    }

    /* End marker */
    TaskDesc end_task = {0};
    end_task.task_type = TASK_TYPE_END;
    queue_push_wait(queue, &end_task);

    printf("[AICPU Pipeline Producer] Done\n");
    return 0;
}

/**
 * Pipeline consumer - saves processed results
 */
struct PipelineConsumerArgs {
    SPSCQueue* input_queue;
    float* result_buffer;
    int32_t* processed_count;
};

extern "C" int aicpu_pipeline_consumer(void* args_ptr) {
    PipelineConsumerArgs* args = static_cast<PipelineConsumerArgs*>(args_ptr);
    SPSCQueue* queue = args->input_queue;

    printf("[AICPU Pipeline Consumer] Starting\n");

    int count = 0;
    TaskDesc task;

    while (1) {
        /* Try to pop from queue */
        if (queue->tail == queue->head) {
            /* Queue empty, spin */
            continue;
        }

        /* Read task */
        uint8_t* slot = &queue->data[queue->tail * queue->item_size];
        memcpy(&task, slot, sizeof(TaskDesc));

        platform_memory_barrier();

        /* Update tail */
        queue->tail = (queue->tail + 1) % queue->capacity;

        /* Check for end marker */
        if (task.task_type == TASK_TYPE_END) {
            printf("[AICPU Pipeline Consumer] End marker received\n");
            break;
        }

        /* Process result (simulate saving to disk) */
        printf("[AICPU Pipeline Consumer] Saving result %d\n", task.task_id);
        count++;
    }

    *args->processed_count = count;
    printf("[AICPU Pipeline Consumer] Done, processed %d items\n", count);
    return 0;
}

/**
 * Batch producer - pushes multiple items at once for efficiency
 */
struct BatchProducerArgs {
    SPSCQueue* queue;
    float* data_buffer;
    int32_t num_batches;
    int32_t items_per_batch;
    int32_t elements_per_item;
};

extern "C" int aicpu_batch_producer(void* args_ptr) {
    BatchProducerArgs* args = static_cast<BatchProducerArgs*>(args_ptr);
    SPSCQueue* queue = args->queue;

    printf("[AICPU Batch Producer] %d batches of %d items\n",
           args->num_batches, args->items_per_batch);

    int total_items = 0;

    for (int32_t batch = 0; batch < args->num_batches; batch++) {
        /* Prepare batch of tasks */
        TaskDesc tasks[16];  /* Max batch size */
        int batch_size = args->items_per_batch;
        if (batch_size > 16) batch_size = 16;

        for (int32_t i = 0; i < batch_size; i++) {
            int item_id = batch * args->items_per_batch + i;

            /* Prepare data */
            float* item_data = args->data_buffer + item_id * args->elements_per_item;
            for (int32_t j = 0; j < args->elements_per_item; j++) {
                item_data[j] = static_cast<float>(item_id * 100 + j);
            }

            /* Create task */
            tasks[i].task_id = item_id;
            tasks[i].task_type = TASK_TYPE_SCALE;
            tasks[i].priority = 0;
            tasks[i].data_offset = item_id * args->elements_per_item * sizeof(float);
            tasks[i].data_size = args->elements_per_item * sizeof(float);
        }

        platform_memory_barrier();

        /* Push batch to queue */
        for (int32_t i = 0; i < batch_size; i++) {
            queue_push_wait(queue, &tasks[i]);
            total_items++;
        }

        printf("[AICPU Batch Producer] Batch %d pushed (%d items)\n", batch, batch_size);
    }

    /* End marker */
    TaskDesc end_task = {0};
    end_task.task_type = TASK_TYPE_END;
    queue_push_wait(queue, &end_task);

    printf("[AICPU Batch Producer] Done, %d total items\n", total_items);
    return 0;
}
