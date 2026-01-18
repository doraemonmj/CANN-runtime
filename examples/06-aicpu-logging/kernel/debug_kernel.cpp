/**
 * AICPU Kernel: debug_kernel
 *
 * Demonstrates logging patterns in AICPU kernels.
 *
 * Compile:
 *   aarch64-linux-gnu-g++ -shared -fPIC -O2 -DDEBUG -o debug_kernel.so debug_kernel.cpp
 */

#include <cstdio>
#include <cstdint>

extern "C" {

/* Log levels */
#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4

/* Error codes */
#define ERR_OK           0
#define ERR_NULL_INPUT  -1
#define ERR_NULL_OUTPUT -2
#define ERR_INVALID_COUNT -3

/* Logging macros */
#define LOG(level, args_ptr, fmt, ...) \
    do { \
        if ((args_ptr)->log_level >= (level)) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR_MSG(args, fmt, ...) LOG(LOG_ERROR, args, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN_MSG(args, fmt, ...)  LOG(LOG_WARN, args, "[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO_MSG(args, fmt, ...)  LOG(LOG_INFO, args, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG_MSG(args, fmt, ...) LOG(LOG_DEBUG, args, "[DEBUG] " fmt "\n", ##__VA_ARGS__)

/* Assertion macro (only in DEBUG builds) */
#ifdef DEBUG
#define KERNEL_ASSERT(args, cond, err_code, msg) \
    do { \
        if (!(cond)) { \
            LOG_ERROR_MSG(args, "ASSERT FAILED: %s", msg); \
            if ((args)->error_code) *(args)->error_code = (err_code); \
            return; \
        } \
    } while(0)
#else
#define KERNEL_ASSERT(args, cond, err_code, msg) /* no-op in release */
#endif

struct DebugKernelArgs {
    void* input;
    void* output;
    int32_t count;
    int32_t log_level;
    int32_t* error_code;
    int32_t _pad;
};

/**
 * Kernel with comprehensive logging
 */
void debug_kernel_entry(DebugKernelArgs* args) {
    /* Initialize error code */
    if (args->error_code) {
        *args->error_code = ERR_OK;
    }

    LOG_INFO_MSG(args, "Kernel started");
    LOG_DEBUG_MSG(args, "  input=%p output=%p count=%d",
                  args->input, args->output, args->count);

    /* Validate inputs */
    KERNEL_ASSERT(args, args->input != nullptr, ERR_NULL_INPUT, "input is null");
    KERNEL_ASSERT(args, args->output != nullptr, ERR_NULL_OUTPUT, "output is null");
    KERNEL_ASSERT(args, args->count > 0, ERR_INVALID_COUNT, "count <= 0");

    float* in = reinterpret_cast<float*>(args->input);
    float* out = reinterpret_cast<float*>(args->output);

    /* Process with progress logging */
    int32_t progress_interval = args->count / 4;
    if (progress_interval < 1) progress_interval = 1;

    for (int32_t i = 0; i < args->count; i++) {
        out[i] = in[i] * 2.0f;

        /* Log progress */
        if (i > 0 && i % progress_interval == 0) {
            LOG_DEBUG_MSG(args, "Progress: %d/%d (%.1f%%)",
                         i, args->count, 100.0f * i / args->count);
        }

        /* Example: warn on unusual values */
        if (in[i] < 0.0f) {
            LOG_WARN_MSG(args, "Negative value at index %d: %.2f", i, in[i]);
        }
    }

    LOG_INFO_MSG(args, "Kernel completed, processed %d elements", args->count);
}

}  /* extern "C" */
