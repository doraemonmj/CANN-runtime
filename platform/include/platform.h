/**
 * platform.h - Ascend NPU Platform Abstraction Layer
 *
 * This is the public API for platform.so. It wraps CANN runtime APIs
 * to provide a simplified interface for teaching Ascend hardware concepts.
 *
 * Target: Ascend 910C (A3, DAV_3510)
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Codes
 * ============================================================================ */

#define PLATFORM_SUCCESS        0
#define PLATFORM_ERROR         -1
#define PLATFORM_ERROR_INIT    -2
#define PLATFORM_ERROR_MEMORY  -3
#define PLATFORM_ERROR_STREAM  -4
#define PLATFORM_ERROR_KERNEL  -5

/* ============================================================================
 * Device Info
 * ============================================================================ */

/**
 * NPU Architecture versions
 */
typedef enum {
    PLATFORM_ARCH_UNKNOWN = 0,
    PLATFORM_ARCH_DAV_1001 = 1001,  /* Older */
    PLATFORM_ARCH_DAV_2201 = 2201,  /* A2 */
    PLATFORM_ARCH_DAV_3510 = 3510,  /* A3 (910C) */
} PlatformArch;

/**
 * Device information structure
 */
typedef struct {
    char soc_version[64];       /* SoC version string, e.g. "Ascend910C" */
    PlatformArch arch;          /* NPU architecture */
    uint32_t aicore_cnt;        /* Total AICORE blocks */
    uint32_t aic_cnt;           /* Cube (matrix) cores */
    uint32_t aiv_cnt;           /* Vector cores */
    uint32_t aicpu_cnt;         /* AICPU (control) count */
    uint64_t hbm_size;          /* Total HBM in bytes */
    uint64_t hbm_free;          /* Available HBM in bytes */
    uint64_t l2_size;           /* L2 cache size */
    uint64_t ub_size;           /* Unified Buffer per core */
    uint64_t l0a_size;          /* L0A buffer size */
    uint64_t l0b_size;          /* L0B buffer size */
    uint64_t l0c_size;          /* L0C buffer size */
    uint64_t l1_size;           /* L1 buffer size */
} PlatformDeviceInfo;

/**
 * Get number of available NPU devices
 */
int platform_get_device_count(uint32_t* count);

/**
 * Get device information
 * @param device_id  Device ID (0-based)
 * @param info       Output device info structure
 */
int platform_get_device_info(int device_id, PlatformDeviceInfo* info);

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * Initialize platform for a specific device
 * @param device_id  Device ID to use (0-based)
 * @return PLATFORM_SUCCESS on success
 */
int platform_init(int device_id);

/**
 * Shutdown platform and release resources
 */
void platform_shutdown(void);

/* ============================================================================
 * Memory Management
 * ============================================================================ */

/**
 * Memory allocation policy
 */
typedef enum {
    PLATFORM_MEM_DEFAULT = 0,   /* Default (2MB pages) */
    PLATFORM_MEM_HUGE_1G = 1,   /* 1GB huge pages */
} PlatformMemPolicy;

/**
 * Allocate device (HBM) memory
 * @param size    Size in bytes
 * @return Device pointer, or NULL on failure
 */
void* platform_malloc(size_t size);

/**
 * Allocate device memory with policy
 * @param size    Size in bytes
 * @param policy  Memory allocation policy
 * @return Device pointer, or NULL on failure
 */
void* platform_malloc_ex(size_t size, PlatformMemPolicy policy);

/**
 * Free device memory
 * @param ptr  Device pointer from platform_malloc
 */
void platform_free(void* ptr);

/**
 * Copy data from host to device
 * @param dst   Device destination pointer
 * @param src   Host source pointer
 * @param size  Size in bytes
 */
int platform_memcpy_h2d(void* dst, const void* src, size_t size);

/**
 * Copy data from device to host
 * @param dst   Host destination pointer
 * @param src   Device source pointer
 * @param size  Size in bytes
 */
int platform_memcpy_d2h(void* dst, const void* src, size_t size);

/**
 * Get L2 cache offset (for L2 bypass)
 */
uint64_t platform_get_l2_offset(void);

/* ============================================================================
 * Stream Management
 * ============================================================================ */

typedef void* PlatformStream;

/**
 * Create a new stream
 */
PlatformStream platform_stream_create(void);

/**
 * Destroy a stream
 */
void platform_stream_destroy(PlatformStream stream);

/**
 * Synchronize (wait for all operations on stream to complete)
 */
int platform_stream_sync(PlatformStream stream);

/* ============================================================================
 * AICORE Kernel Launch
 * ============================================================================ */

typedef void* PlatformKernel;

/**
 * Load an AICORE kernel from binary
 * @param bin   Kernel binary (.o file contents)
 * @param size  Size of binary in bytes
 * @param name  Kernel entry point name
 * @return Kernel handle, or NULL on failure
 */
PlatformKernel platform_kernel_load(const void* bin, size_t size, const char* name);

/**
 * Unload a kernel
 */
void platform_kernel_unload(PlatformKernel kernel);

/**
 * Launch an AICORE kernel
 * @param kernel   Kernel handle from platform_kernel_load
 * @param blocks   Number of blocks (cores) to use
 * @param args     Kernel arguments buffer
 * @param args_size Size of arguments
 * @param stream   Stream to launch on (NULL for default)
 */
int platform_kernel_launch(PlatformKernel kernel,
                           uint32_t blocks,
                           void* args, size_t args_size,
                           PlatformStream stream);

/* ============================================================================
 * AICPU Kernel Launch
 * ============================================================================ */

/**
 * Launch an AICPU kernel
 * @param so_data   Shared object binary (.so file contents)
 * @param so_size   Size of .so in bytes
 * @param entry     Entry function name
 * @param args      Arguments buffer
 * @param args_size Size of arguments
 * @param stream    Stream to launch on (NULL for default)
 */
int platform_aicpu_launch(const void* so_data, size_t so_size,
                          const char* entry,
                          void* args, size_t args_size,
                          PlatformStream stream);

/* ============================================================================
 * Logging
 * ============================================================================ */

/**
 * Log levels
 */
typedef enum {
    PLATFORM_LOG_DEBUG = 0,
    PLATFORM_LOG_INFO = 1,
    PLATFORM_LOG_WARN = 2,
    PLATFORM_LOG_ERROR = 3,
} PlatformLogLevel;

/**
 * Set log level
 */
void platform_set_log_level(PlatformLogLevel level);

/**
 * Log a message
 */
void platform_log(PlatformLogLevel level, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
