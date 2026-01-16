/**
 * 01-device-query - Query Ascend NPU Device Information
 *
 * This example demonstrates:
 * - Getting the number of available NPU devices
 * - Querying device properties (SoC version, core counts, memory sizes)
 *
 * Hardware concepts taught:
 * - NPU architecture (DAV_2201, DAV_3510)
 * - Core types: AICORE (AIC + AIV), AICPU
 * - Memory hierarchy: HBM, L2, L1, L0A/B/C, UB
 */

#include <cstdio>
#include "platform.h"

const char* arch_to_string(PlatformArch arch) {
    switch (arch) {
        case PLATFORM_ARCH_DAV_1001: return "DAV_1001 (Older)";
        case PLATFORM_ARCH_DAV_2201: return "DAV_2201 (A2)";
        case PLATFORM_ARCH_DAV_3510: return "DAV_3510 (A3/910C)";
        default: return "Unknown";
    }
}

void print_size(const char* name, uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        printf("  %-20s: %.2f GB\n", name, (double)bytes / (1024.0 * 1024 * 1024));
    } else if (bytes >= 1024 * 1024) {
        printf("  %-20s: %.2f MB\n", name, (double)bytes / (1024.0 * 1024));
    } else if (bytes >= 1024) {
        printf("  %-20s: %.2f KB\n", name, (double)bytes / 1024.0);
    } else {
        printf("  %-20s: %lu bytes\n", name, (unsigned long)bytes);
    }
}

int main() {
    printf("=== Ascend NPU Device Query ===\n\n");

    /* Get device count */
    uint32_t device_count = 0;
    int ret = platform_get_device_count(&device_count);
    if (ret != PLATFORM_SUCCESS) {
        printf("Failed to get device count (error: %d)\n", ret);
        printf("Note: This example requires CANN runtime and NPU hardware.\n");
        return 1;
    }

    printf("Found %u NPU device(s)\n\n", device_count);

    if (device_count == 0) {
        printf("No NPU devices found.\n");
        return 0;
    }

    /* Query each device */
    for (uint32_t i = 0; i < device_count && i < 4; i++) {
        printf("--- Device %u ---\n", i);

        PlatformDeviceInfo info;
        ret = platform_get_device_info(i, &info);
        if (ret != PLATFORM_SUCCESS) {
            printf("  Failed to get device info (error: %d)\n", ret);
            continue;
        }

        printf("  %-20s: %s\n", "SoC Version", info.soc_version);
        printf("  %-20s: %s\n", "Architecture", arch_to_string(info.arch));
        printf("\n");

        /* Core counts */
        printf("  Core Configuration:\n");
        printf("    AICORE blocks    : %u\n", info.aicore_cnt);
        printf("    AIC (Cube) cores : %u\n", info.aic_cnt);
        printf("    AIV (Vector) cores: %u\n", info.aiv_cnt);
        printf("    AICPU count      : %u\n", info.aicpu_cnt);
        printf("\n");

        /* Memory hierarchy */
        printf("  Memory Hierarchy:\n");
        print_size("HBM Total", info.hbm_size);
        print_size("HBM Free", info.hbm_free);
        print_size("L2 Cache", info.l2_size);
        print_size("L1 Buffer", info.l1_size);
        print_size("Unified Buffer (UB)", info.ub_size);
        print_size("L0A Buffer", info.l0a_size);
        print_size("L0B Buffer", info.l0b_size);
        print_size("L0C Buffer", info.l0c_size);
        printf("\n");
    }

    printf("=== End of Device Query ===\n");
    return 0;
}
