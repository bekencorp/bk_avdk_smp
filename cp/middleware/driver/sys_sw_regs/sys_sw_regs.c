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

#include <stddef.h>
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

static inline volatile ap_heap_dump_info_t *sys_sw_regs_ap_heap_slot(bk_sys_sw_regs_ap_heap_id_t id)
{
    if ((uint32_t)id >= BK_SYS_SW_REGS_AP_HEAP_MAX) {
        return NULL;
    }

    return &s_sys_sw_regs.ap_heap_dump[id];
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

uint32_t bk_sys_sw_regs_get_ap_heap_dump(bk_sys_sw_regs_ap_heap_id_t id, ap_heap_dump_info_t *info)
{
    volatile ap_heap_dump_info_t *slot = sys_sw_regs_ap_heap_slot(id);

    if ((slot == NULL) || (info == NULL)) {
        return 0;
    }

    info->valid = slot->valid;
    info->pool_base = slot->pool_base;
    info->max_alloc_end = slot->max_alloc_end;
    info->reserved = slot->reserved;

    return (info->valid == BK_SYS_SW_REGS_AP_HEAP_DUMP_VALID) ? 1 : 0;
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

void bk_sys_sw_regs_update_ap_heap_dump(bk_sys_sw_regs_ap_heap_id_t id, uint32_t pool_base, uint32_t max_alloc_end)
{
    volatile ap_heap_dump_info_t *slot = sys_sw_regs_ap_heap_slot(id);
    uint32_t flags;

    if ((slot == NULL) || (pool_base == 0U) || (max_alloc_end <= pool_base)) {
        return;
    }

    flags = sys_sw_regs_lock();

    if ((slot->valid != BK_SYS_SW_REGS_AP_HEAP_DUMP_VALID) || (slot->pool_base != pool_base)) {
        slot->valid = 0U;
        slot->pool_base = pool_base;
        slot->max_alloc_end = max_alloc_end;
        slot->reserved = 0U;
        slot->valid = BK_SYS_SW_REGS_AP_HEAP_DUMP_VALID;
    } else if (max_alloc_end > slot->max_alloc_end) {
        slot->max_alloc_end = max_alloc_end;
    }

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
