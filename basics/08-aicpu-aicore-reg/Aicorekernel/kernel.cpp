/**
 * @file kernel.cpp
 * @brief AICore Kernel for Register-based Communication with AICPU
 *
 * This kernel implements the AICore side of register-based handshake communication.
 * It waits for AICPU signals, reads data from registers, and sends handshake responses.
 */

#include <cstdint>
#include <array>

// ============================================================================
// Compiler Directives and Macros
// ============================================================================

#ifndef __gm__
#define __gm__
#endif

#ifndef __global__
#define __global__
#endif

#ifndef __aicore__
#define __aicore__ [aicore]
#endif

// Kernel entry point naming convention
#ifdef __AIV__
#define KERNEL_ENTRY(x) x##_0_mix_aiv
#else
#define KERNEL_ENTRY(x) x##_0_mix_aic
#endif

// Block index naming convention
#ifdef __AIV__
#define blockIdx blockIdx_aiv
#else
#define blockIdx blockIdx_aic
#endif

[[block_local]] int blockIdx;

// ============================================================================
// Protocol Constants
// ============================================================================

constexpr uint64_t AICORE_SAY_HELLO = 0x80000000;  // Handshake magic value
constexpr uint64_t MAX_WAIT_CYCLES = 10000000000;  // Maximum wait cycles

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Device arguments shared between AICPU and AICore
 * @note Must match the structure layout in AICPU kernel
 */
struct DeviceArgs {
    uint64_t unused[12] = {0};      // Reserved space for compatibility
    uint64_t aicpuSoBin{0};         // AICPU backend SO binary address
    uint64_t aicpuSoLen{0};         // AICPU backend SO binary size
    volatile uint64_t regs{0};      // Register array address
    uint64_t devId{0};              // Device ID (used to store register data)
    uint64_t coreNum{0};            // Number of cores
    volatile bool fastPath{false};  // Synchronization flag
};

// ============================================================================
// AICore Kernel Implementation
// ============================================================================

/**
 * @brief Main AICore kernel entry point
 * @details Execution flow:
 *          1. Wait for AICPU to set fastPath flag
 *          2. Read data from DATA_MAIN_BASE register
 *          3. Prepare handshake message with core ID and block index
 *          4. Send handshake message via COND register
 *          5. Clear fastPath flag to signal completion
 *
 * @param args Pointer to DeviceArgs structure in global memory
 */
extern "C" __global__ __aicore__ void KERNEL_ENTRY(aicore_kernel)(__gm__ uint8_t *args) {
    auto devArgs = (__gm__ DeviceArgs *)args;

    // ========================================================================
    // Step 1: Wait for AICPU to signal readiness via fastPath flag
    // ========================================================================
    uint64_t waitCount = 0;

    while (devArgs->fastPath == false) {
        // Invalidate cache to ensure we see the latest value
        dcci(devArgs, ENTIRE_DATA_CACHE, CACHELINE_OUT);

        waitCount++;
        if (waitCount > MAX_WAIT_CYCLES) {
            // Timeout - exit without completing handshake
            return;
        }
    }

    // ========================================================================
    // Step 2: Read data from DATA_MAIN_BASE register
    // ========================================================================
    if (devArgs->fastPath == true) {
        // Read the value written by AICPU from DATA_MAIN_BASE register
        uint64_t regValue = get_data_main_base();

        // Store the register value back to device memory
        devArgs->devId = regValue;
    }

    // ========================================================================
    // Step 3: Prepare handshake message
    // ========================================================================

    // Get block index based on architecture
#ifdef __AIV__
    blockIdx = get_block_idx() * get_subblockdim() + get_subblockid() + get_block_num();
    int coreId = get_coreid();
#else
    blockIdx = get_block_idx();
    int coreId = get_coreid();
#endif

    // Construct handshake message:
    // - Bits [31:0]:  Magic value (AICORE_SAY_HELLO)
    // - Bits [47:32]: Physical core ID
    // - Bits [63:48]: Logical block index
    uint64_t handshakeMsg =
        ((uint64_t)blockIdx << 48) |      // Block index in upper 16 bits
        ((uint64_t)coreId << 32) |        // Core ID in middle 16 bits
        AICORE_SAY_HELLO;                 // Magic value in lower 32 bits

    // ========================================================================
    // Step 4: Send handshake message to AICPU via COND register
    // ========================================================================
    set_cond(handshakeMsg);

    // ========================================================================
    // Step 5: Signal completion to AICPU
    // ========================================================================
    devArgs->fastPath = false;
}
