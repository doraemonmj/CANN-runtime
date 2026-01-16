/**
 * AICPU Kernel: producer/consumer queue operations
 *
 * Compile:
 *   aarch64-linux-gnu-g++ -shared -fPIC -O2 -o queue_kernels.so producer.cpp
 */

#include <cstdint>
#include <atomic>

extern "C" {

/* Work item structure */
struct WorkItem {
    void* data_ptr;
    int32_t size;
    int32_t flags;
};

#define FLAG_NONE 0
#define FLAG_LAST 1

/* SPSC Queue - must match host definition */
template <typename T, int CAPACITY>
struct SPSCQueue {
    T data[CAPACITY];
    std::atomic<uint32_t> head{0};
    std::atomic<uint32_t> tail{0};
    int32_t _pad[14];

    bool push(const T& item) {
        uint32_t h = head.load(std::memory_order_relaxed);
        uint32_t next = (h + 1) % CAPACITY;

        if (next == tail.load(std::memory_order_acquire)) {
            return false;
        }

        data[h] = item;
        head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T* item) {
        uint32_t t = tail.load(std::memory_order_relaxed);

        if (t == head.load(std::memory_order_acquire)) {
            return false;
        }

        *item = data[t];
        tail.store((t + 1) % CAPACITY, std::memory_order_release);
        return true;
    }
};

using WorkQueue = SPSCQueue<WorkItem, 64>;

/*
 * Producer arguments
 */
struct ProducerArgs {
    void** input_ptrs;    /* Array of input data pointers */
    int32_t* sizes;       /* Array of sizes */
    int32_t num_items;    /* Number of items to produce */
    WorkQueue* queue;     /* Shared queue */
    int32_t _pad;
};

/**
 * Producer kernel - pushes work items to queue
 */
void producer_entry(ProducerArgs* args) {
    WorkQueue* queue = args->queue;

    /* Push all work items */
    for (int32_t i = 0; i < args->num_items; i++) {
        WorkItem item;
        item.data_ptr = args->input_ptrs[i];
        item.size = args->sizes[i];
        item.flags = FLAG_NONE;

        /* Spin until space available */
        while (!queue->push(item)) {
            /* Could add backoff here */
        }
    }

    /* Push termination signal */
    WorkItem last;
    last.data_ptr = nullptr;
    last.size = 0;
    last.flags = FLAG_LAST;

    while (!queue->push(last)) {
        /* Spin */
    }
}

/*
 * Consumer arguments
 */
struct ConsumerArgs {
    void* output;           /* Output buffer */
    WorkQueue* queue;       /* Shared queue */
    std::atomic<int32_t>* processed_count;  /* Progress counter */
    int32_t _pad;
};

/**
 * Consumer kernel - pops and processes work items
 */
void consumer_entry(ConsumerArgs* args) {
    WorkQueue* queue = args->queue;
    float* out = reinterpret_cast<float*>(args->output);
    int32_t out_idx = 0;

    while (true) {
        WorkItem item;

        if (queue->pop(&item)) {
            /* Check for termination */
            if (item.flags & FLAG_LAST) {
                break;
            }

            /* Process item */
            float* data = reinterpret_cast<float*>(item.data_ptr);
            int32_t count = item.size / sizeof(float);

            /* Example: sum all elements */
            float sum = 0.0f;
            for (int32_t i = 0; i < count; i++) {
                sum += data[i];
            }
            out[out_idx++] = sum;

            /* Update progress */
            if (args->processed_count) {
                args->processed_count->fetch_add(1, std::memory_order_relaxed);
            }
        }
        /* If empty, keep polling */
    }
}

}  /* extern "C" */
