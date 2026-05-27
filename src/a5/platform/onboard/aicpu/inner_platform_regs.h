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
 * @file inner_platform_regs.h (onboard)
 * @brief MMIO register access — inline definitions for real hardware (a5).
 *
 * Included by platform_regs.h; resolved via the onboard aicpu include path
 * (`platform/onboard/aicpu/`). The sim variant lives in
 * `platform/sim/aicpu/inner_platform_regs.h`.
 *
 * Moving the definitions to a header lets every call site (including the
 * sched completion-poll hot loop in scheduler_completion.cpp) fully inline
 * `read_reg` / `write_reg` / `get_cond_reg_ptr` and fold the constexpr
 * `reg_offset(RegId::*)` switch at compile time — no function call, no
 * runtime switch.
 *
 * halResMap maps each AICore as 3MB of contiguous MMIO; hardware register
 * offsets (e.g. DATA_MAIN_BASE=0xD0, COND=0x5108) apply directly.
 */

#ifndef PLATFORM_ONBOARD_AICPU_INNER_PLATFORM_REGS_H_
#define PLATFORM_ONBOARD_AICPU_INNER_PLATFORM_REGS_H_

#include <cstdint>
#include "common/platform_config.h"  // RegId, reg_offset

// MMIO register read.
//
// `volatile` enforces ordering with respect to other volatile MMIO
// accesses, which is all MMIO requires. No explicit memory barriers —
// cann/pypto's aicore_hal::GetFinishedTask uses the same bare-volatile
// pattern, validated by upstream.
inline uint64_t read_reg(uint64_t reg_base_addr, RegId reg) {
    volatile uint32_t *ptr = reinterpret_cast<volatile uint32_t *>(reg_base_addr + reg_offset(reg));
    return static_cast<uint64_t>(*ptr);
}

// MMIO register write. Keeps the bracketing __sync_synchronize (DMB ISH).
// Dispatch writes the AICore task payload to cacheable memory THEN writes
// the trigger register; without the DMB before the register write, the
// payload-store may become observable to the AICore AFTER the trigger,
// corrupting the dispatched task. (Verified — dropping the write_reg
// barriers triggers golden-output mismatches across 100-round runs.)
inline void write_reg(uint64_t reg_base_addr, RegId reg, uint64_t value) {
    volatile uint32_t *ptr = reinterpret_cast<volatile uint32_t *>(reg_base_addr + reg_offset(reg));
    __sync_synchronize();
    *ptr = static_cast<uint32_t>(value);
    __sync_synchronize();
}

// Precomputed COND register pointer.
inline volatile uint32_t *get_cond_reg_ptr(uint64_t reg_base_addr) {
    return reinterpret_cast<volatile uint32_t *>(reg_base_addr + reg_offset(RegId::COND));
}

#endif  // PLATFORM_ONBOARD_AICPU_INNER_PLATFORM_REGS_H_
