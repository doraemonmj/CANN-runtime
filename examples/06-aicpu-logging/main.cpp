/**
 * 08-aicpu-logging - Debug Output Patterns in AICPU Kernels
 *
 * This example demonstrates logging and debugging techniques for AICPU:
 * - printf() for debug output
 * - Conditional logging with verbosity levels
 * - Error code returns
 * - Progress reporting
 * - Performance timing
 *
 * AICPU kernels run as standard ARM code, so all C/C++ debugging
 * techniques work.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "platform.h"

/*
 * Verbosity levels for conditional logging
 */
#define LOG_NONE  0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4

/*
 * Kernel arguments including debug options
 */
struct DebugKernelArgs {
    void* input;
    void* output;
    int32_t count;
    int32_t log_level;    /* Verbosity: 0=none, 4=debug */
    int32_t* error_code;  /* Output: error code (in HBM) */
    int32_t _pad;
};

int main() {
    printf("=== AICPU Logging and Debug Patterns ===\n\n");

    /* Initialize */
    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== Pattern 1: printf() Debugging ====== */
    printf("Pattern 1: printf() in AICPU kernels\n");
    printf("  +---------------------------------------------------------+\n");
    printf("  |  AICPU kernels can use printf() directly!               |\n");
    printf("  |                                                         |\n");
    printf("  |  void my_kernel(Args* args) {                           |\n");
    printf("  |      printf(\"Kernel started, count=%%d\\n\", args->count);|\n");
    printf("  |      // ... kernel code ...                             |\n");
    printf("  |      printf(\"Kernel done\\n\");                          |\n");
    printf("  |  }                                                      |\n");
    printf("  |                                                         |\n");
    printf("  |  Output appears in device logs (dmesg or CANN log)      |\n");
    printf("  +---------------------------------------------------------+\n\n");

    /* ====== Pattern 2: Conditional Logging ====== */
    printf("Pattern 2: Conditional logging with levels\n");
    printf("  Pass log_level in kernel args:\n");
    printf("  \n");
    printf("  #define LOG_ERROR 1\n");
    printf("  #define LOG_INFO  3\n");
    printf("  #define LOG_DEBUG 4\n");
    printf("  \n");
    printf("  void kernel(Args* args) {\n");
    printf("      if (args->log_level >= LOG_DEBUG)\n");
    printf("          printf(\"[DEBUG] Starting...\\n\");\n");
    printf("  \n");
    printf("      if (error && args->log_level >= LOG_ERROR)\n");
    printf("          printf(\"[ERROR] Failed at %%d\\n\", i);\n");
    printf("  }\n\n");

    /* ====== Pattern 3: Error Codes ====== */
    printf("Pattern 3: Error codes via HBM\n");

    /* Allocate error code in HBM (accessible by both host and kernel) */
    int32_t* dev_error = (int32_t*)platform_malloc(sizeof(int32_t));
    int32_t host_error = 0;
    platform_memcpy_h2d(dev_error, &host_error, sizeof(int32_t));

    printf("  Allocate error_code in HBM:\n");
    printf("    int32_t* dev_error = platform_malloc(sizeof(int32_t));\n");
    printf("  \n");
    printf("  Kernel writes error code:\n");
    printf("    if (bad_condition) {\n");
    printf("        *args->error_code = ERROR_BAD_INPUT;\n");
    printf("        return;  // Early exit\n");
    printf("    }\n");
    printf("  \n");
    printf("  Host reads after sync:\n");
    printf("    platform_stream_sync(NULL);\n");
    printf("    platform_memcpy_d2h(&host_error, dev_error, ...);\n");
    printf("    if (host_error != 0) handle_error(host_error);\n\n");

    /* ====== Pattern 4: Progress Reporting ====== */
    printf("Pattern 4: Progress reporting\n");
    printf("  For long-running kernels, report progress:\n");
    printf("  \n");
    printf("  struct ProgressArgs {\n");
    printf("      // ... data pointers ...\n");
    printf("      atomic<int32_t>* progress;  // In HBM\n");
    printf("  };\n");
    printf("  \n");
    printf("  void kernel(Args* args) {\n");
    printf("      for (int i = 0; i < N; i++) {\n");
    printf("          process(i);\n");
    printf("          if (i %% 1000 == 0)\n");
    printf("              args->progress->store(i);\n");
    printf("      }\n");
    printf("  }\n");
    printf("  \n");
    printf("  // Host can poll progress while kernel runs\n\n");

    /* ====== Pattern 5: Assertions ====== */
    printf("Pattern 5: Assertions for development\n");
    printf("  \n");
    printf("  #ifdef DEBUG\n");
    printf("  #define KERNEL_ASSERT(cond, msg) \\\n");
    printf("      if (!(cond)) { \\\n");
    printf("          printf(\"ASSERT FAILED: %%s\\n\", msg); \\\n");
    printf("          *args->error_code = -1; \\\n");
    printf("          return; \\\n");
    printf("      }\n");
    printf("  #else\n");
    printf("  #define KERNEL_ASSERT(cond, msg) /* no-op */\n");
    printf("  #endif\n");
    printf("  \n");
    printf("  void kernel(Args* args) {\n");
    printf("      KERNEL_ASSERT(args->count > 0, \"count must be positive\");\n");
    printf("      KERNEL_ASSERT(args->input != nullptr, \"null input\");\n");
    printf("  }\n\n");

    /* ====== Demo: Simulated kernel with logging ====== */
    printf("Demo: Simulated kernel execution with logging\n");
    printf("  ----------------------------------------\n");

    /* Simulate what kernel printf would output */
    int log_level = LOG_DEBUG;
    int count = 100;

    if (log_level >= LOG_INFO)
        printf("  [INFO] Kernel started, count=%d\n", count);

    if (log_level >= LOG_DEBUG)
        printf("  [DEBUG] Input pointer: %p\n", (void*)0x12345678);

    for (int i = 0; i < count; i += 25) {
        if (log_level >= LOG_DEBUG)
            printf("  [DEBUG] Processing batch %d-%d\n", i, i + 24);
    }

    if (log_level >= LOG_INFO)
        printf("  [INFO] Kernel completed successfully\n");

    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_free(dev_error);
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of AICPU Logging Example ===\n");
    return 0;
}
