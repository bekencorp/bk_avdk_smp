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

#if CONFIG_AP_EMUBOOT
#include "cmsis_gcc.h"
#include "cache.h"
#endif

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

static inline volatile ap_extra_dump_info_t *sys_sw_regs_ap_extra_dump_slot(uint32_t index)
{
    if (index >= BK_SYS_SW_REGS_AP_EXTRA_DUMP_MAX) {
        return NULL;
    }

    return &s_sys_sw_regs.ap_extra_dump[index];
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

uint32_t bk_sys_sw_regs_get_ap_extra_dump(uint32_t index, ap_extra_dump_info_t *info)
{
    volatile ap_extra_dump_info_t *slot = sys_sw_regs_ap_extra_dump_slot(index);

    if ((slot == NULL) || (info == NULL)) {
        return 0;
    }

    info->valid_seq = slot->valid_seq;
    info->start_addr = slot->start_addr;
    info->size = slot->size;

    return ((info->valid_seq & BK_SYS_SW_REGS_AP_EXTRA_DUMP_VALID_MASK) == BK_SYS_SW_REGS_AP_EXTRA_DUMP_VALID) ? 1 : 0;
}

uint32_t bk_sys_sw_regs_get_adc_key_sample(adc_key_sample_info_t *info)
{
    uint32_t flags;

    if (info == NULL) {
        return 0;
    }

    flags = sys_sw_regs_lock();
    info->valid = s_sys_sw_regs.adc_key_sample.valid;
    info->seq = s_sys_sw_regs.adc_key_sample.seq;
    info->sample_tick = s_sys_sw_regs.adc_key_sample.sample_tick;
    info->raw = s_sys_sw_regs.adc_key_sample.raw;
    info->mv = s_sys_sw_regs.adc_key_sample.mv;
    info->status = s_sys_sw_regs.adc_key_sample.status;
    info->channel = s_sys_sw_regs.adc_key_sample.channel;
    info->reserved0 = s_sys_sw_regs.adc_key_sample.reserved0;
    info->sample_period_ms = s_sys_sw_regs.adc_key_sample.sample_period_ms;
    sys_sw_regs_unlock(flags);

    return (info->valid == BK_SYS_SW_REGS_ADC_KEY_VALID) ? 1 : 0;
}

bk_err_t bk_sys_sw_regs_get_pm_shared_info(pm_shared_info_t *info)
{
   uint32_t flags;
    uint32_t i;

    if (info == NULL) {
        return BK_ERR_PARAM;
    }

   flags = sys_sw_regs_lock();
    info->pm_ap0_sleep_state = s_sys_sw_regs.pm_shared_info.pm_ap0_sleep_state;
    info->pm_ap1_sleep_state = s_sys_sw_regs.pm_shared_info.pm_ap1_sleep_state;
    info->pm_cp0_sleep_state = s_sys_sw_regs.pm_shared_info.pm_cp0_sleep_state;
    info->pm_cp1_sleep_state = s_sys_sw_regs.pm_shared_info.pm_cp1_sleep_state;
    info->pm_ap_work_state = s_sys_sw_regs.pm_shared_info.pm_ap_work_state;
    info->wakeup_source = s_sys_sw_regs.pm_shared_info.wakeup_source;
    for (i = 0; i <= ALARM_NAME_MAX_LEN; i++) {
        info->wakeup_alarm_name[i] = s_sys_sw_regs.pm_shared_info.wakeup_alarm_name[i];
    }
    info->gpio_id = s_sys_sw_regs.pm_shared_info.gpio_id;
    info->param0 = s_sys_sw_regs.pm_shared_info.param0;
    info->param1 = s_sys_sw_regs.pm_shared_info.param1;
    info->param2 = s_sys_sw_regs.pm_shared_info.param2;
    sys_sw_regs_unlock(flags);
    return BK_OK;
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

#if CONFIG_AP_EMUBOOT
uint32_t bk_sys_sw_regs_get_flash_init_done(void)
{
    flush_dcache((void *)&s_sys_sw_regs.flash_init_done, sizeof(s_sys_sw_regs.flash_init_done));
    __DSB();
    uint32_t value = s_sys_sw_regs.flash_init_done;
    __DSB();
    return value;
}

void bk_sys_sw_regs_set_flash_init_done(uint32_t value)
{
    uint32_t flags = sys_sw_regs_lock();
    s_sys_sw_regs.flash_init_done = value;
    __DSB();
    flush_dcache((void *)&s_sys_sw_regs.flash_init_done, sizeof(s_sys_sw_regs.flash_init_done));
    __DSB();
    sys_sw_regs_unlock(flags);
}
#endif

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

void bk_sys_sw_regs_update_ap_extra_dump(uint32_t index, uint32_t start_addr, uint32_t size)
{
    volatile ap_extra_dump_info_t *slot = sys_sw_regs_ap_extra_dump_slot(index);
    uint32_t flags;
    uint32_t seq;

    if ((slot == NULL) || (start_addr == 0U) || (size == 0U)) {
        return;
    }

    flags = sys_sw_regs_lock();

    seq = (slot->valid_seq + 1U) & BK_SYS_SW_REGS_AP_EXTRA_DUMP_SEQ_MASK;
    slot->valid_seq = 0U;
    slot->start_addr = start_addr;
    slot->size = size;
    slot->valid_seq = BK_SYS_SW_REGS_AP_EXTRA_DUMP_VALID | seq;

    sys_sw_regs_unlock(flags);
}

void bk_sys_sw_regs_set_adc_key_sample(uint16_t raw, uint16_t mv, uint8_t status, uint8_t channel, uint32_t sample_period_ms, uint32_t sample_tick)
{
    uint32_t flags = sys_sw_regs_lock();

    s_sys_sw_regs.adc_key_sample.valid = 0U;
    s_sys_sw_regs.adc_key_sample.raw = raw;
    s_sys_sw_regs.adc_key_sample.mv = mv;
    s_sys_sw_regs.adc_key_sample.status = status;
    s_sys_sw_regs.adc_key_sample.channel = channel;
    s_sys_sw_regs.adc_key_sample.sample_period_ms = sample_period_ms;
    s_sys_sw_regs.adc_key_sample.sample_tick = sample_tick;
    s_sys_sw_regs.adc_key_sample.seq += 1U;
    s_sys_sw_regs.adc_key_sample.valid = BK_SYS_SW_REGS_ADC_KEY_VALID;

    sys_sw_regs_unlock(flags);
}

bk_err_t bk_sys_sw_regs_update_pm_shared_info(const pm_shared_info_t *info, uint32_t field_mask, uint8_t use_lock)
{
    uint32_t flags = 0;
    uint32_t i;

    if ((info == NULL) || (field_mask == 0U)) {
        return BK_ERR_PARAM;
    }

    if (use_lock == BK_SYS_SW_REGS_LOCK_ENABLE) {
        flags = sys_sw_regs_lock();
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP0_SLEEP_STATE) != 0U) {
        s_sys_sw_regs.pm_shared_info.pm_ap0_sleep_state = info->pm_ap0_sleep_state;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP1_SLEEP_STATE) != 0U) {
        s_sys_sw_regs.pm_shared_info.pm_ap1_sleep_state = info->pm_ap1_sleep_state;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_CP0_SLEEP_STATE) != 0U) {
        s_sys_sw_regs.pm_shared_info.pm_cp0_sleep_state = info->pm_cp0_sleep_state;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_CP1_SLEEP_STATE) != 0U) {
        s_sys_sw_regs.pm_shared_info.pm_cp1_sleep_state = info->pm_cp1_sleep_state;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP_WORK_STATE) != 0U) {
        s_sys_sw_regs.pm_shared_info.pm_ap_work_state = info->pm_ap_work_state;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_WAKEUP_SOURCE) != 0U) {
        s_sys_sw_regs.pm_shared_info.wakeup_source = info->wakeup_source;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_WAKEUP_ALARM_NAME) != 0U) {
        for (i = 0; i <= ALARM_NAME_MAX_LEN; i++) {
            s_sys_sw_regs.pm_shared_info.wakeup_alarm_name[i] = info->wakeup_alarm_name[i];
        }
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_GPIO_ID) != 0U) {
        s_sys_sw_regs.pm_shared_info.gpio_id = info->gpio_id;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM0) != 0U) {
        s_sys_sw_regs.pm_shared_info.param0 = info->param0;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM1) != 0U) {
        s_sys_sw_regs.pm_shared_info.param1 = info->param1;
    }
    if ((field_mask & BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_PARAM2) != 0U) {
        s_sys_sw_regs.pm_shared_info.param2 = info->param2;
    }
    if (use_lock == BK_SYS_SW_REGS_LOCK_ENABLE) {
        sys_sw_regs_unlock(flags);
    }
    return BK_OK;
}

void *bk_sys_sw_regs_get_sspl_list(void)
{
    return (void *)s_sys_sw_regs.sspl_list;
}

volatile sys_sw_regs_t *bk_sys_sw_regs_ptr(void)
{
    return &s_sys_sw_regs;
}
