/**
 * 01-device-query - Query Ascend NPU Device Information
 *
 * Demonstrates how to:
 * - Enumerate NPU devices on the PCIe bus
 * - Query runtime properties (core counts, memory) via aclrtGetDeviceInfo()
 * - Read architectural constants (buffer sizes) from platform config files
 *
 * Hardware concepts:
 * - AICORE: Compute units with Cube (matrix) and Vector (SIMD) engines
 * - AICPU: Control processors for task orchestration
 * - Memory hierarchy: HBM → L2 Cache → L1/UB → L0A/B/C
 * - Difference between queryable properties and fixed architectural constants
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "acl/acl.h"

// Output formatting constants for vertical alignment of all colons
// Total width from line start to colon should be: indent + LABEL_WIDTH
constexpr int LABEL_WIDTH_L1 = 34;  // Level 1 (2 spaces indent): 2 + 34 = 36
constexpr int LABEL_WIDTH_L2 = 32;  // Level 2 (4 spaces indent): 4 + 32 = 36
constexpr int LABEL_WIDTH_L3 = 30;  // Level 3 (6 spaces indent): 6 + 30 = 36

/*
 * SoC hardware configuration - architectural constants per SoC variant
 *
 * These are fixed at chip design time and most cannot be queried via ACL APIs.
 * Must be read from platform config: $ASCEND_HOME_PATH/.../platform_config/<SoC>.ini
 */
struct SoCConfig {
    // From [SoCInfo] section
    uint32_t ai_core_cnt;
    uint32_t cube_core_cnt;
    uint32_t vector_core_cnt;
    uint32_t ai_cpu_cnt;
    uint64_t l2_size;

    // From [AICoreSpec] section - per-core buffers
    uint32_t ub_size;       // Unified Buffer - general-purpose scratch memory
    uint32_t l0a_size;      // L0A - input buffer for matrix A (Cube engine)
    uint32_t l0b_size;      // L0B - input buffer for matrix B (Cube engine)
    uint32_t l0c_size;      // L0C - output buffer for matrix C (Cube engine)
    uint32_t l1_size;       // L1 - intermediate buffer between UB and L0

    bool valid;             // Whether parsing succeeded
};

/*
 * Read SoC configuration from CANN platform config INI file
 *
 * Why: Many hardware parameters are architectural constants (baked into hardware design)
 *      and cannot be queried via ACL APIs. CANN stores these in INI files
 *      specific to each SoC variant (e.g., Ascend910_9392.ini).
 *
 * Path: $ASCEND_HOME_PATH/aarch64-linux/data/platform_config/<SoC>.ini
 * Sections: [SoCInfo] for device-level config, [AICoreSpec] for per-core buffers
 */
bool read_soc_config(const char* soc_name, SoCConfig* config) {
    if (!soc_name || !config) return false;

    config->valid = false;

    // Construct config file path
    const char* ascend_home = getenv("ASCEND_HOME_PATH");
    if (!ascend_home) {
        ascend_home = "/usr/local/Ascend/ascend-toolkit/latest";
    }

    char config_path[512];
    snprintf(config_path, sizeof(config_path),
             "%s/aarch64-linux/data/platform_config/%s.ini",
             ascend_home, soc_name);

    // Try to open the config file
    FILE* fp = fopen(config_path, "r");
    if (!fp) {
        return false;
    }

    // Parse both [SoCInfo] and [AICoreSpec] sections
    char line[256];
    bool in_soc_info = false;
    bool in_aicore_spec = false;
    int fields_found = 0;
    const int REQUIRED_FIELDS = 10;  // 5 from SoCInfo + 5 from AICoreSpec

    while (fgets(line, sizeof(line), fp)) {
        // Check for section headers
        if (strstr(line, "[SoCInfo]")) {
            in_soc_info = true;
            in_aicore_spec = false;
            continue;
        }
        if (strstr(line, "[AICoreSpec]")) {
            in_aicore_spec = true;
            in_soc_info = false;
            continue;
        }

        // Exit section if we hit another [Section]
        if (line[0] == '[') {
            in_soc_info = false;
            in_aicore_spec = false;
            continue;
        }

        // Parse fields in [SoCInfo]
        if (in_soc_info) {
            if (sscanf(line, "ai_core_cnt=%u", &config->ai_core_cnt) == 1) fields_found++;
            else if (sscanf(line, "cube_core_cnt=%u", &config->cube_core_cnt) == 1) fields_found++;
            else if (sscanf(line, "vector_core_cnt=%u", &config->vector_core_cnt) == 1) fields_found++;
            else if (sscanf(line, "ai_cpu_cnt=%u", &config->ai_cpu_cnt) == 1) fields_found++;
            else if (sscanf(line, "l2_size=%lu", &config->l2_size) == 1) fields_found++;
        }

        // Parse buffer size fields in [AICoreSpec]
        if (in_aicore_spec) {
            if (sscanf(line, "ub_size=%u", &config->ub_size) == 1) fields_found++;
            else if (sscanf(line, "l0_a_size=%u", &config->l0a_size) == 1) fields_found++;
            else if (sscanf(line, "l0_b_size=%u", &config->l0b_size) == 1) fields_found++;
            else if (sscanf(line, "l0_c_size=%u", &config->l0c_size) == 1) fields_found++;
            else if (sscanf(line, "l1_size=%u", &config->l1_size) == 1) fields_found++;
        }
    }

    fclose(fp);

    // Validate we found all required fields
    config->valid = (fields_found == REQUIRED_FIELDS);
    return config->valid;
}

int main() {
    printf("=== Ascend NPU Device Query ===\n\n");

    /* Initialize ACL runtime - loads drivers and establishes connection to NPUs */
    aclError ret = aclInit(nullptr);
    if (ret != ACL_SUCCESS) {
        printf("Failed to initialize ACL (error: %d)\n", ret);
        printf("Note: This example requires CANN runtime and NPU hardware.\n");
        return 1;
    }

    /* Query PCIe bus for Ascend NPU devices */
    uint32_t device_count = 0;
    ret = aclrtGetDeviceCount(&device_count);
    if (ret != ACL_SUCCESS) {
        printf("Failed to get device count (error: %d)\n", ret);
        aclFinalize();
        return 1;
    }

    printf("Found %u NPU device(s)\n\n", device_count);

    if (device_count == 0) {
        printf("No NPU devices found.\n");
        aclFinalize();
        return 0;
    }

    /* Query each device */
    for (uint32_t i = 0; i < device_count; i++) {
        printf("--- Device %u ---\n", i);

        /* Set device context - required before querying device-specific properties */
        ret = aclrtSetDevice(i);
        if (ret != ACL_SUCCESS) {
            printf("  Failed to set device %u (error: %d)\n", i, ret);
            continue;
        }

        /* Get SoC version string (e.g., "Ascend910_9392")
         * Source: Runtime query via aclrtGetSocName() */
        const char* soc_name = aclrtGetSocName();
        const char* soc_version = soc_name ? soc_name : "Unknown";

        printf("  %-*s: %s (via aclrtGetSocName)\n", LABEL_WIDTH_L1, "SoC Version", soc_version);
        printf("\n");

        /* Query core counts - each AICORE has Cube + Vector engines
         * Source: Runtime query via aclrtGetDeviceInfo() */
        int64_t aicore_count = 0, aicpu_count = 0;
        int64_t vector_count = 0;

        printf("  Core Configuration (via aclrtGetDeviceInfo):\n");

        if (aclrtGetDeviceInfo(i, ACL_DEV_ATTR_AICORE_CORE_NUM, &aicore_count) == ACL_SUCCESS) {
            printf("    %-*s: %ld\n", LABEL_WIDTH_L2, "AICORE cores", aicore_count);
        }

        if (aclrtGetDeviceInfo(i, ACL_DEV_ATTR_VECTOR_CORE_NUM, &vector_count) == ACL_SUCCESS) {
            printf("    %-*s: %ld\n", LABEL_WIDTH_L2, "Vector cores", vector_count);
        }

        if (aclrtGetDeviceInfo(i, ACL_DEV_ATTR_AICPU_CORE_NUM, &aicpu_count) == ACL_SUCCESS) {
            printf("    %-*s: %ld\n", LABEL_WIDTH_L2, "AICPU cores", aicpu_count);
        }
        printf("\n");

        /* Query memory hierarchy - HBM (device DRAM)
         * Source: Runtime query via aclrtGetMemInfo() */
        size_t free_mem = 0, total_mem = 0;

        printf("  Memory Hierarchy (via aclrtGetMemInfo):\n");

        /* HBM is the device's main memory (like GDDR on GPUs) */
        ret = aclrtGetMemInfo(ACL_HBM_MEM, &free_mem, &total_mem);
        if (ret == ACL_SUCCESS) {
            printf("    %-*s: %.2f GB\n", LABEL_WIDTH_L2, "HBM Total", (double)total_mem / (1024.0 * 1024 * 1024));
            printf("    %-*s: %.2f GB\n", LABEL_WIDTH_L2, "HBM Free", (double)free_mem / (1024.0 * 1024 * 1024));
        }
        printf("\n");

        /* Read SoC configuration from platform config file
         * Source: $ASCEND_HOME_PATH/aarch64-linux/data/platform_config/<SoC>.ini
         * Sections: [SoCInfo] for core counts and L2, [AICoreSpec] for per-core buffers */
        SoCConfig config = {};
        if (read_soc_config(soc_version, &config)) {
            const char* ascend_home = getenv("ASCEND_HOME_PATH");
            if (!ascend_home) ascend_home = "/usr/local/Ascend/ascend-toolkit/latest";

            printf("  Hardware Configuration (from %s/aarch64-linux/data/platform_config/%s.ini):\n",
                   ascend_home, soc_version);

            // Core counts from config
            printf("    %-*s: %u\n", LABEL_WIDTH_L2, "AICORE cores", config.ai_core_cnt);
            printf("    %-*s: %u\n", LABEL_WIDTH_L2, "Cube cores", config.cube_core_cnt);
            printf("    %-*s: %u\n", LABEL_WIDTH_L2, "Vector cores", config.vector_core_cnt);
            printf("    %-*s: %u\n", LABEL_WIDTH_L2, "AICPU cores", config.ai_cpu_cnt);
            printf("\n");

            // L2 cache size
            printf("    %-*s: %.2f MB\n", LABEL_WIDTH_L2, "L2 Cache", (double)config.l2_size / (1024.0 * 1024));
            printf("\n");

            // Per-core AICORE buffers
            printf("    AICORE Buffers (per core):\n");
            printf("      %-*s: %.2f KB\n", LABEL_WIDTH_L3, "Unified Buffer (UB)", (double)config.ub_size / 1024.0);
            printf("      %-*s: %.2f KB\n", LABEL_WIDTH_L3, "L1 Buffer", (double)config.l1_size / 1024.0);
            printf("      %-*s: %.2f KB\n", LABEL_WIDTH_L3, "L0A Buffer", (double)config.l0a_size / 1024.0);
            printf("      %-*s: %.2f KB\n", LABEL_WIDTH_L3, "L0B Buffer", (double)config.l0b_size / 1024.0);
            printf("      %-*s: %.2f KB\n", LABEL_WIDTH_L3, "L0C Buffer", (double)config.l0c_size / 1024.0);
        } else {
            printf("  Hardware Configuration: Could not read platform config for %s\n", soc_version);
        }
        printf("\n");

        /* Release device context for this device */
        ret = aclrtResetDevice(i);
        if (ret != ACL_SUCCESS) {
            printf("  Warning: Failed to reset device %u (error: %d)\n", i, ret);
        }
    }

    /* Shutdown ACL runtime and release resources */
    aclFinalize();

    printf("=== End of Device Query ===\n");
    return 0;
}
