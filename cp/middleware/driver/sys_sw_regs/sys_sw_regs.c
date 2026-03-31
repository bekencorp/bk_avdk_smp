// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file sys_sw_regs.c
 * @brief System software registers implementation
 * @author Beken
 * @date 2026-03-05
 * @version 1.0
 */

#include "sys_sw_regs.h"
#include "aspl_lock.h"

/* Shared configuration data placed in the dedicated linker section.
 * Using static to prevent direct external access; all reads/writes
 * must go through the provided API functions. */
static SYS_SW_REGS_SECTION sys_sw_regs_t s_sys_sw_regs = {0};

/* --------------------------------------------------------------------------
 * Lock helpers - delegate to HSPL SYS_SW_REGS resource lock
 * -------------------------------------------------------------------------- */

static inline uint32_t sys_sw_regs_lock(void)
{
    return bk_aspl_sys_sw_regs_enter_critical();
}

static inline void sys_sw_regs_unlock(uint32_t flags)
{
    bk_aspl_sys_sw_regs_exit_critical(flags);
}

/* --------------------------------------------------------------------------
 * Read API
 * -------------------------------------------------------------------------- */

uint32_t bk_sys_sw_regs_get_psram_power_down(void)
{
    return s_sys_sw_regs.psram_power_down;
}

uint32_t bk_sys_sw_regs_get_cp_reset_reason(void)
{
    return s_sys_sw_regs.cp_reset_reason;
}

uint32_t bk_sys_sw_regs_get_ap_reset_reason(void)
{
    return s_sys_sw_regs.ap_reset_reason;
}

/* --------------------------------------------------------------------------
 * Write API
 * -------------------------------------------------------------------------- */

void bk_sys_sw_regs_set_psram_power_down(uint32_t value)
{
    uint32_t flags = sys_sw_regs_lock();
    s_sys_sw_regs.psram_power_down = value;
    sys_sw_regs_unlock(flags);
}

void bk_sys_sw_regs_set_cp_reset_reason(uint32_t value)
{
    uint32_t flags = sys_sw_regs_lock();
    s_sys_sw_regs.cp_reset_reason = value;
    sys_sw_regs_unlock(flags);
}

void bk_sys_sw_regs_set_ap_reset_reason(uint32_t value)
{
    uint32_t flags = sys_sw_regs_lock();
    s_sys_sw_regs.ap_reset_reason = value;
    sys_sw_regs_unlock(flags);
}

void *bk_sys_sw_regs_get_sspl_list(void)
{
    return (void *)s_sys_sw_regs.sspl_list;
}

volatile sys_sw_regs_t *bk_sys_sw_regs_ptr(void)
{
    return &s_sys_sw_regs;
}
