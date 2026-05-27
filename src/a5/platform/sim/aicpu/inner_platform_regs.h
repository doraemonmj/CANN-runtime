/*
 * Copyright (c) PyPTO Contributors.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 * -----------------------------------------------------------------------------------------------------------
 */
/**
 * @file inner_platform_regs.h (sim)
 * @brief MMIO register access — inline definitions for simulation (a5sim).
 *
 * Included by platform_regs.h; resolved via the sim aicpu include path
 * (`platform/sim/aicpu/`). The onboard variant lives in
 * `platform/onboard/aicpu/inner_platform_regs.h`.
 *
 * Simulated registers are three compact pages per core (16KB total):
 *   0x0000-0x0FFF -> page 0 (AICore SPR low: CTRL, DATA_MAIN_BASE)
 *   0x2400-0x43FF -> page 2 (PMU MMIO)
 *   0x5000-0x5FFF -> page 1 (AICore SPR high: COND)
 * sparse_reg_ptr() performs the offset remapping.
 */

#ifndef PLATFORM_SIM_AICPU_INNER_PLATFORM_REGS_H_
#define PLATFORM_SIM_AICPU_INNER_PLATFORM_REGS_H_

#include <cstdint>
#include "common/platform_config.h"  // RegId, reg_offset, sparse_reg_ptr

inline uint64_t read_reg(uint64_t reg_base_addr, RegId reg) {
    volatile uint8_t *reg_base = reinterpret_cast<volatile uint8_t *>(reg_base_addr);
    volatile uint32_t *ptr = reinterpret_cast<volatile uint32_t *>(sparse_reg_ptr(reg_base, reg_offset(reg)));
    return static_cast<uint64_t>(*ptr);
}

// See onboard variant for the rationale on keeping the __sync_synchronize
// pair around the write.
inline void write_reg(uint64_t reg_base_addr, RegId reg, uint64_t value) {
    volatile uint8_t *reg_base = reinterpret_cast<volatile uint8_t *>(reg_base_addr);
    volatile uint32_t *ptr = reinterpret_cast<volatile uint32_t *>(sparse_reg_ptr(reg_base, reg_offset(reg)));
    __sync_synchronize();
    *ptr = static_cast<uint32_t>(value);
    __sync_synchronize();
}

inline volatile uint32_t *get_cond_reg_ptr(uint64_t reg_base_addr) {
    volatile uint8_t *reg_base = reinterpret_cast<volatile uint8_t *>(reg_base_addr);
    return reinterpret_cast<volatile uint32_t *>(sparse_reg_ptr(reg_base, reg_offset(RegId::COND)));
}

#endif  // PLATFORM_SIM_AICPU_INNER_PLATFORM_REGS_H_
