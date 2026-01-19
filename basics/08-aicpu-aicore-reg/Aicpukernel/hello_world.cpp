/**
 * @file hello_world.cpp
 * @brief AICPU Backend Kernel for AICPU-AICore Register-based Communication
 *
 * This kernel implements register-based handshake communication between AICPU and AICore.
 * The AICPU writes to AICore registers and reads responses to synchronize execution.
 */

#include "device_log.h"
#include <cstdint>
#include <cstdio>

// ============================================================================
// Register Definitions
// ============================================================================

constexpr uint32_t REG_SPR_FAST_PATH_ENABLE = 0x18;
constexpr uint64_t REG_SPR_FAST_PATH_OPEN = 0xE;
constexpr uint64_t REG_SPR_FAST_PATH_CLOSE = 0xF;

constexpr uint32_t REG_SPR_DATA_MAIN_BASE = 0xA0;
constexpr uint32_t REG_SPR_COND = 0x4C8;

constexpr uint64_t AICORE_SAY_HELLO = 0x80000000;

// Timeout configuration
constexpr int MAX_POLL_COUNT = 100000;

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Device arguments shared between host and AICPU kernel
 * @note Structure layout is critical - offsets are hardcoded in libaicpu_extend_kernels.so
 */
struct DeviceArgs {
    uint64_t unused[12] = {0};      // Reserved space for compatibility
    uint64_t aicpuSoBin{0};         // Device memory address of backend SO binary
    uint64_t aicpuSoLen{0};         // Size of backend SO binary
    volatile uint64_t regs{0};      // Device memory address of register array
    uint64_t devId{0};              // Device ID
    uint64_t coreNum{0};            // Number of AICore cores
    volatile bool fastPath{false};  // Fast path synchronization flag
};

/**
 * @brief Kernel arguments wrapper
 * @note Offset to deviceArgs pointer is critical for libaicpu_extend_kernels.so
 */
struct KernelArgs {
    uint64_t unused[5] = {0};
    int64_t *deviceArgs{nullptr};
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Read and validate handshake messages from AICore via COND register
 * @param regAddrs Array of register base addresses for each core
 * @param numCores Number of cores to poll
 */
void ReadAicoreHandshake(int64_t* regAddrs, int numCores) {
    for (int i = 0; i < numCores; i++) {
        void *base = reinterpret_cast<void *>(regAddrs[i]);

        if (base == nullptr) {
            DEV_ERROR("regAddrs[%d] is null", i);
            break;
        }

        // Read COND register
        volatile uint64_t* condReg = reinterpret_cast<volatile uint64_t *>(
            static_cast<uint8_t *>(base) + REG_SPR_COND
        );
        uint64_t val = *condReg;

        // Parse handshake message
        uint32_t low32 = val & 0xFFFFFFFF;           // Low 32 bits: magic value
        uint32_t coreId = (val >> 32) & 0xFFFF;      // Bits [47:32]: physical core ID
        uint32_t blockIdx = (val >> 48) & 0xFFFF;    // Bits [63:48]: logical block index

        DEV_INFO("Core[%d] COND register: magic=0x%lx, coreId=%d, blockIdx=%d",
                 i, low32, coreId, blockIdx);

        // Validate handshake
        if (low32 == AICORE_SAY_HELLO) {
            DEV_INFO("Core[%d]: Valid handshake received!!!!", i);
        }
    }
}

/**
 * @brief Write data to AICore registers with fast path enabled
 * @param regAddrs Array of register base addresses for each core
 * @param numCores Number of cores to write to
 * @param offset Register offset to write to
 * @param val Value to write
 */
void WriteToAicore(int64_t* regAddrs, int numCores, int offset, uint32_t val) {
    for (int i = 0; i < numCores; i++) {
        // Enable fast path for register access
        volatile uint32_t* fastPathReg = reinterpret_cast<volatile uint32_t*>(
            regAddrs[i] + REG_SPR_FAST_PATH_ENABLE
        );
        *fastPathReg = REG_SPR_FAST_PATH_OPEN;
        __sync_synchronize();

        void *base = reinterpret_cast<void *>(regAddrs[i]);
        if (base != nullptr) {
            volatile uint32_t* targetReg = reinterpret_cast<volatile uint32_t *>(
                regAddrs[i] + offset
            );
            *targetReg = val;
            DEV_INFO("[AICPU->AICORE] Wrote 0x%lx to offset 0x%x", val, offset);
        }
        // 写入寄存器的值，哪怕程序结束也会在寄存器里，影响下一次读入，注意！！！
        __sync_synchronize();
    }
}

/**
 * @brief Wait for AICore to complete processing and signal via fastPath flag
 * @param devArg Device arguments containing the fastPath flag
 * @return 0 on success, -1 on timeout
 */
int WaitForAicoreCompletion(DeviceArgs* devArg) {
    int count = 0;

    while (devArg->fastPath != false) {
        count++;
        if (count > MAX_POLL_COUNT) {
            DEV_ERROR("Timeout waiting for AICore completion: exceeded %d polls", MAX_POLL_COUNT);
            return -1;
        }
    }

    DEV_INFO("AICore completed processing after %d polls", count);
    return 0;
}

/**
 * @brief Close fast path access to AICore registers
 * @param regAddrs Array of register base addresses for each core
 * @param numCores Number of cores
 */
void CloseFastPath(int64_t* regAddrs, int numCores) {
    for (int i = 0; i < numCores; i++) {
        volatile uint32_t* fastPathReg = reinterpret_cast<volatile uint32_t*>(
            regAddrs[i] + REG_SPR_FAST_PATH_ENABLE
        );
        *fastPathReg = REG_SPR_FAST_PATH_CLOSE;
    }
    __sync_synchronize();
}

// ============================================================================
// Core Handshake Logic
// ============================================================================

/**
 * @brief Perform register-based handshake with AICore
 * @details This function orchestrates the complete handshake sequence:
 *          1. Write data to AICore registers
 *          2. Signal AICore via fastPath flag
 *          3. Read handshake response from AICore
 *          4. Wait for AICore completion
 *          5. Close fast path access
 *
 * @param devArg Device arguments containing register addresses and configuration
 * @return 0 on success, -1 on failure
 */
int RegisterHandshake(DeviceArgs* devArg) {
    //  export ASCEND_GLOBAL_LOG_LEVEL=0 设置日志
    auto regs = reinterpret_cast<int64_t*>(devArg->regs);
    int numCores = devArg->coreNum;

    // Step 1: Write initial data to AICore
    WriteToAicore(regs, numCores, REG_SPR_DATA_MAIN_BASE, 1234);

    // Step 2: Signal AICore that data is ready
    devArg->fastPath = true;
    DEV_INFO("Handshake initiated: fastPath = %s", "true");

    // Step 3: Read handshake response from AICore
    ReadAicoreHandshake(regs, numCores);

    // Step 4: Wait for AICore to complete and signal back
    DEV_INFO("Waiting for AICore completion: %s", "polling fastPath flag");
    if (WaitForAicoreCompletion(devArg) != 0) {
        DEV_ERROR("AICore handshake timed out: %s", "exceeded max poll count");
        return -1;
    }
    DEV_INFO("AICore completion confirmed: fastPath = %s", "false");

    // Step 5: Close fast path access
    CloseFastPath(regs, numCores);

    return 0;
}

/**
 * @brief Main handler for AICore communication
 * @param arg Kernel arguments containing DeviceArgs pointer
 * @return 0 on success, -1 on failure
 */
int HandshakeWithAicore(void *arg) {
    auto kargs = static_cast<KernelArgs *>(arg);
    auto devArg = reinterpret_cast<DeviceArgs*>(kargs->deviceArgs);

    int ret = RegisterHandshake(devArg);
    if (ret != 0) {
        DEV_ERROR("Register handshake failed with code: %d", ret);
        return -1;
    }

    return 0;
}

// ============================================================================
// Kernel Entry Points
// ============================================================================

/**
 * @brief Static kernel server entry point
 * @note Called by libaicpu_extend_kernels.so for static kernels
 */
extern "C" __attribute__((visibility("default")))
int StaticTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        DEV_ERROR("Invalid kernel arguments: %s", "null pointer");
        return -1;
    }
    return 0;
}

/**
 * @brief Dynamic kernel initialization entry point
 * @note Called by libaicpu_extend_kernels.so when launching DynTileFwkKernelServerInit
 *       Responsible for initializing logging and validating arguments
 */
extern "C" __attribute__((visibility("default")))
int DynTileFwkBackendKernelServerInit(void *arg) {
    InitLogSwitch();

    if (arg == nullptr) {
        DEV_ERROR("Invalid kernel arguments: %s", "null pointer");
        return -1;
    }

    DEV_INFO("AICPU Kernel Init: %s", "Initializing register-based communication");
    return 0;
}

/**
 * @brief Dynamic kernel execution entry point
 * @note Called by libaicpu_extend_kernels.so when launching DynTileFwkKernelServer
 *       Executes the main kernel logic including AICore handshake
 */
extern "C" __attribute__((visibility("default")))
int DynTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        DEV_ERROR("Invalid kernel arguments: %s", "null pointer");
        return -1;
    }

    DEV_INFO("AICPU Kernel: %s", "Starting register-based handshake with AICore");

    int rc = HandshakeWithAicore(arg);
    if (rc != 0) {
        DEV_ERROR("Handshake with AICore failed with code: %d", rc);
        return -1;
    }

    DEV_INFO("Kernel execution: %s", "completed successfully");
    return 0;
}
