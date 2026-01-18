/**
 * AICPU Kernel: atomic_counter
 *
 * Demonstrates atomic operations in AICPU kernels.
 *
 * Compile:
 *   aarch64-linux-gnu-g++ -shared -fPIC -O2 -o atomic_counter.so atomic_counter.cpp
 */

#include <cstdint>
#include <atomic>

extern "C" {

/*
 * Shared state structure - allocated in HBM
 * Accessible by all AICPU cores and host
 */
struct SharedState {
    std::atomic<int32_t> counter;
    std::atomic<int32_t> work_index;
    std::atomic<int32_t> done_count;
    int32_t total_work;
    int32_t _pad[4];  /* Padding */
};

struct AtomicKernelArgs {
    void* input;
    void* output;
    int32_t count;
    int32_t worker_id;
    SharedState* shared;
    int32_t _pad;
};

/**
 * Work-stealing pattern: each worker claims items dynamically
 */
void atomic_worker_entry(AtomicKernelArgs* args) {
    float* in = reinterpret_cast<float*>(args->input);
    float* out = reinterpret_cast<float*>(args->output);
    SharedState* shared = args->shared;

    /* Work stealing loop */
    while (true) {
        /* Atomically claim next work item */
        int32_t item = shared->work_index.fetch_add(1, std::memory_order_acq_rel);

        if (item >= shared->total_work) {
            break;  /* No more work */
        }

        /* Process this item */
        out[item] = in[item] * 2.0f;

        /* Update progress counter */
        shared->counter.fetch_add(1, std::memory_order_relaxed);
    }

    /* Signal this worker is done */
    shared->done_count.fetch_add(1, std::memory_order_release);
}

/**
 * Barrier-based pattern: all workers sync at barrier points
 */
struct BarrierState {
    std::atomic<int32_t> arrived;
    std::atomic<int32_t> generation;
    int32_t num_workers;
    int32_t _pad;
};

struct BarrierKernelArgs {
    void* data;
    int32_t count;
    int32_t worker_id;
    int32_t num_workers;
    BarrierState* barrier;
};

/* Barrier synchronization primitive */
static void barrier_wait(BarrierState* barrier, int32_t num_workers) {
    int gen = barrier->generation.load(std::memory_order_acquire);

    if (barrier->arrived.fetch_add(1, std::memory_order_acq_rel) == num_workers - 1) {
        /* Last to arrive */
        barrier->arrived.store(0, std::memory_order_relaxed);
        barrier->generation.fetch_add(1, std::memory_order_release);
    } else {
        /* Wait for generation change */
        while (barrier->generation.load(std::memory_order_acquire) == gen) {
            /* Spin - could add yield or backoff */
        }
    }
}

void barrier_kernel_entry(BarrierKernelArgs* args) {
    float* data = reinterpret_cast<float*>(args->data);
    int32_t n = args->count;
    int32_t id = args->worker_id;
    int32_t num = args->num_workers;

    /* Calculate my portion */
    int32_t chunk = n / num;
    int32_t start = id * chunk;
    int32_t end = (id == num - 1) ? n : start + chunk;

    /* Phase 1: Independent processing */
    for (int32_t i = start; i < end; i++) {
        data[i] = data[i] * 2.0f;
    }

    /* Barrier: wait for all workers */
    barrier_wait(args->barrier, num);

    /* Phase 2: All workers past barrier */
    /* Could read neighbor's results here */
    for (int32_t i = start; i < end; i++) {
        data[i] = data[i] + 1.0f;
    }
}

}  /* extern "C" */
