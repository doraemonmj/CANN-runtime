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
 * @file inner_platform_regs.cpp
 * @brief AICPU register read/write for real hardware (a5)
 *
 * halResMap maps each AICore as 3MB of contiguous MMIO. Hardware offsets
 * (e.g. DATA_MAIN_BASE=0xD0, COND=0x5108) are applied directly to the
 * virtual address with no remapping.
 */

#include <cstdint>
#include "aicpu/platform_regs.h"
#include "common/platform_config.h"

// MMIO register access.
//
// read_reg (COND polling hot path): the result is only used to drive software
// state (slot_state.task_state CAS), and `volatile` already enforces ordering
// against other volatile MMIO accesses. The previous wrapping pair of
// __sync_synchronize() (full DMB ISH) was overkill for the read direction.
// cann/pypto's aicore_hal::GetFinishedTask uses the same bare-volatile pattern.
//
// write_reg: KEEP the full barriers. Dispatch flow writes the AICore task
// payload to cacheable memory THEN writes the trigger register (MMIO). Without
// a DMB before the register write, the payload-store may be observable to the
// AICore AFTER the trigger — corrupting the just-dispatched task. (Verified:
// dropping the write_reg barriers triggers golden-output mismatches across
// 100-round runs.)
uint64_t read_reg(uint64_t reg_base_addr, RegId reg) {
    uint32_t offset = reg_offset(reg);
    volatile uint32_t *ptr = reinterpret_cast<volatile uint32_t *>(reg_base_addr + offset);
    return static_cast<uint64_t>(*ptr);
}

void write_reg(uint64_t reg_base_addr, RegId reg, uint64_t value) {
    uint32_t offset = reg_offset(reg);
    volatile uint32_t *ptr = reinterpret_cast<volatile uint32_t *>(reg_base_addr + offset);

    __sync_synchronize();
    *ptr = static_cast<uint32_t>(value);
    __sync_synchronize();
}
