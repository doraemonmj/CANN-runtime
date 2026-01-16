/**
 * platform.cpp - Ascend NPU Platform Implementation (ACL-only version)
 *
 * Uses only high-level ACL APIs for simpler dependencies.
 */

#include "platform.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef BUILD_WITH_CANN
#include "acl/acl.h"
#endif

/* ============================================================================
 * Internal State
 * ============================================================================ */

static struct {
    int initialized;
    int device_id;
    PlatformLogLevel log_level;
#ifdef BUILD_WITH_CANN
    aclrtStream default_stream;
    aclrtContext context;
#endif
} g_platform = {0};

/* ============================================================================
 * Logging
 * ============================================================================ */

void platform_set_log_level(PlatformLogLevel level) {
    g_platform.log_level = level;
}

void platform_log(PlatformLogLevel level, const char* fmt, ...) {
    if (level < g_platform.log_level) {
        return;
    }

    const char* level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    fprintf(stderr, "[PLATFORM][%s] ", level_str[level]);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

#define LOG_DEBUG(...) platform_log(PLATFORM_LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  platform_log(PLATFORM_LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...)  platform_log(PLATFORM_LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(...) platform_log(PLATFORM_LOG_ERROR, __VA_ARGS__)

/* ============================================================================
 * Device Info
 * ============================================================================ */

int platform_get_device_count(uint32_t* count) {
    if (!count) {
        return PLATFORM_ERROR;
    }

#ifdef BUILD_WITH_CANN
    uint32_t dev_count = 0;
    aclError ret = aclrtGetDeviceCount(&dev_count);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtGetDeviceCount failed: %d", ret);
        return PLATFORM_ERROR;
    }
    *count = dev_count;
    return PLATFORM_SUCCESS;
#else
    *count = 0;
    return PLATFORM_SUCCESS;
#endif
}

int platform_get_device_info(int device_id, PlatformDeviceInfo* info) {
    if (!info) {
        return PLATFORM_ERROR;
    }

    memset(info, 0, sizeof(PlatformDeviceInfo));

#ifdef BUILD_WITH_CANN
    /* Get SoC name */
    const char* soc_name = aclrtGetSocName();
    if (soc_name) {
        strncpy(info->soc_version, soc_name, sizeof(info->soc_version) - 1);
    } else {
        strncpy(info->soc_version, "Unknown", sizeof(info->soc_version) - 1);
    }

    /* Determine architecture from SoC version string */
    if (strstr(info->soc_version, "910B") || strstr(info->soc_version, "910C")) {
        info->arch = PLATFORM_ARCH_DAV_3510;  /* A3 */
    } else if (strstr(info->soc_version, "910A") || strstr(info->soc_version, "910_") ||
               strstr(info->soc_version, "910")) {
        info->arch = PLATFORM_ARCH_DAV_2201;  /* A2 */
    } else {
        info->arch = PLATFORM_ARCH_UNKNOWN;
    }

    /* Get memory info - requires device to be set */
    size_t free_mem = 0, total_mem = 0;
    aclError ret = aclrtGetMemInfo(ACL_HBM_MEM, &free_mem, &total_mem);
    if (ret == ACL_SUCCESS) {
        info->hbm_size = total_mem;
        info->hbm_free = free_mem;
    }

    /* Typical A3 (910C/910B) values */
    info->aicore_cnt = 24;
    info->aic_cnt = 24;
    info->aiv_cnt = 48;
    info->aicpu_cnt = 8;
    info->l2_size = 192 * 1024 * 1024;
    info->ub_size = 256 * 1024;
    info->l0a_size = 64 * 1024;
    info->l0b_size = 64 * 1024;
    info->l0c_size = 256 * 1024;
    info->l1_size = 1024 * 1024;

    return PLATFORM_SUCCESS;
#else
    strncpy(info->soc_version, "Simulation", sizeof(info->soc_version) - 1);
    info->arch = PLATFORM_ARCH_UNKNOWN;
    return PLATFORM_SUCCESS;
#endif
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

int platform_init(int device_id) {
    if (g_platform.initialized) {
        LOG_WARN("Platform already initialized");
        return PLATFORM_SUCCESS;
    }

#ifdef BUILD_WITH_CANN
    /* Initialize ACL */
    aclError ret = aclInit(nullptr);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclInit failed: %d", ret);
        return PLATFORM_ERROR_INIT;
    }

    /* Set device */
    ret = aclrtSetDevice(device_id);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtSetDevice(%d) failed: %d", device_id, ret);
        aclFinalize();
        return PLATFORM_ERROR_INIT;
    }

    /* Create context */
    ret = aclrtCreateContext(&g_platform.context, device_id);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtCreateContext failed: %d", ret);
        aclrtResetDevice(device_id);
        aclFinalize();
        return PLATFORM_ERROR_INIT;
    }

    /* Create default stream */
    ret = aclrtCreateStream(&g_platform.default_stream);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtCreateStream failed: %d", ret);
        aclrtDestroyContext(g_platform.context);
        aclrtResetDevice(device_id);
        aclFinalize();
        return PLATFORM_ERROR_INIT;
    }

    g_platform.device_id = device_id;
    g_platform.initialized = 1;
    LOG_INFO("Platform initialized on device %d", device_id);
    return PLATFORM_SUCCESS;
#else
    g_platform.device_id = device_id;
    g_platform.initialized = 1;
    LOG_INFO("Platform initialized (simulation mode)");
    return PLATFORM_SUCCESS;
#endif
}

void platform_shutdown(void) {
    if (!g_platform.initialized) {
        return;
    }

#ifdef BUILD_WITH_CANN
    if (g_platform.default_stream) {
        aclrtDestroyStream(g_platform.default_stream);
        g_platform.default_stream = nullptr;
    }
    if (g_platform.context) {
        aclrtDestroyContext(g_platform.context);
        g_platform.context = nullptr;
    }
    aclrtResetDevice(g_platform.device_id);
    aclFinalize();
#endif

    g_platform.initialized = 0;
    LOG_INFO("Platform shutdown");
}

/* ============================================================================
 * Memory Management
 * ============================================================================ */

void* platform_malloc(size_t size) {
    return platform_malloc_ex(size, PLATFORM_MEM_DEFAULT);
}

void* platform_malloc_ex(size_t size, PlatformMemPolicy policy) {
    if (!g_platform.initialized) {
        LOG_ERROR("Platform not initialized");
        return nullptr;
    }

    if (size == 0) {
        return nullptr;
    }

#ifdef BUILD_WITH_CANN
    void* ptr = nullptr;
    aclError ret = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtMalloc(%zu) failed: %d", size, ret);
        return nullptr;
    }

    LOG_DEBUG("Allocated %zu bytes at %p", size, ptr);
    return ptr;
#else
    void* ptr = malloc(size);
    LOG_DEBUG("Allocated %zu bytes at %p (host simulation)", size, ptr);
    return ptr;
#endif
}

void platform_free(void* ptr) {
    if (!ptr) {
        return;
    }

#ifdef BUILD_WITH_CANN
    aclError ret = aclrtFree(ptr);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtFree(%p) failed: %d", ptr, ret);
    }
    LOG_DEBUG("Freed %p", ptr);
#else
    free(ptr);
    LOG_DEBUG("Freed %p (host simulation)", ptr);
#endif
}

int platform_memcpy_h2d(void* dst, const void* src, size_t size) {
    if (!g_platform.initialized) {
        return PLATFORM_ERROR_INIT;
    }

#ifdef BUILD_WITH_CANN
    aclError ret = aclrtMemcpy(dst, size, src, size, ACL_MEMCPY_HOST_TO_DEVICE);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtMemcpy H2D failed: %d", ret);
        return PLATFORM_ERROR_MEMORY;
    }
    return PLATFORM_SUCCESS;
#else
    memcpy(dst, src, size);
    return PLATFORM_SUCCESS;
#endif
}

int platform_memcpy_d2h(void* dst, const void* src, size_t size) {
    if (!g_platform.initialized) {
        return PLATFORM_ERROR_INIT;
    }

#ifdef BUILD_WITH_CANN
    aclError ret = aclrtMemcpy(dst, size, src, size, ACL_MEMCPY_DEVICE_TO_HOST);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtMemcpy D2H failed: %d", ret);
        return PLATFORM_ERROR_MEMORY;
    }
    return PLATFORM_SUCCESS;
#else
    memcpy(dst, src, size);
    return PLATFORM_SUCCESS;
#endif
}

uint64_t platform_get_l2_offset(void) {
    return 0;
}

/* ============================================================================
 * Stream Management
 * ============================================================================ */

PlatformStream platform_stream_create(void) {
    if (!g_platform.initialized) {
        LOG_ERROR("Platform not initialized");
        return nullptr;
    }

#ifdef BUILD_WITH_CANN
    aclrtStream stream = nullptr;
    aclError ret = aclrtCreateStream(&stream);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtCreateStream failed: %d", ret);
        return nullptr;
    }
    return (PlatformStream)stream;
#else
    return (PlatformStream)(uintptr_t)1;
#endif
}

void platform_stream_destroy(PlatformStream stream) {
    if (!stream) {
        return;
    }

#ifdef BUILD_WITH_CANN
    aclrtDestroyStream((aclrtStream)stream);
#endif
}

int platform_stream_sync(PlatformStream stream) {
    if (!g_platform.initialized) {
        return PLATFORM_ERROR_INIT;
    }

#ifdef BUILD_WITH_CANN
    aclrtStream acl_stream = stream ? (aclrtStream)stream : g_platform.default_stream;
    aclError ret = aclrtSynchronizeStream(acl_stream);
    if (ret != ACL_SUCCESS) {
        LOG_ERROR("aclrtSynchronizeStream failed: %d", ret);
        return PLATFORM_ERROR_STREAM;
    }
    return PLATFORM_SUCCESS;
#else
    return PLATFORM_SUCCESS;
#endif
}

/* ============================================================================
 * Kernel Launch (Stubs - need full implementation)
 * ============================================================================ */

PlatformKernel platform_kernel_load(const void* bin, size_t size, const char* name) {
    (void)bin; (void)size;
    if (!g_platform.initialized || !name) {
        return nullptr;
    }
    char* kernel_name = strdup(name);
    LOG_DEBUG("Kernel load stub: %s", name);
    return (PlatformKernel)kernel_name;
}

void platform_kernel_unload(PlatformKernel kernel) {
    if (kernel) {
        free(kernel);
    }
}

int platform_kernel_launch(PlatformKernel kernel, uint32_t blocks,
                           void* args, size_t args_size, PlatformStream stream) {
    (void)blocks; (void)args; (void)args_size; (void)stream;
    if (!g_platform.initialized || !kernel) {
        return PLATFORM_ERROR_INIT;
    }
    LOG_DEBUG("Kernel launch stub: %s", (const char*)kernel);
    return PLATFORM_SUCCESS;
}

int platform_aicpu_launch(const void* so_data, size_t so_size, const char* entry,
                          void* args, size_t args_size, PlatformStream stream) {
    (void)so_data; (void)so_size; (void)args; (void)args_size; (void)stream;
    if (!g_platform.initialized || !entry) {
        return PLATFORM_ERROR_INIT;
    }
    LOG_DEBUG("AICPU launch stub: %s", entry);
    return PLATFORM_SUCCESS;
}
