// Copyright 2020-2021 Beken
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

#include <common/bk_include.h>
#include <common/bk_compiler.h>
#include <os/mem.h>
#include <os/os.h>
#include "icu_driver.h"
#include "timer_driver.h"
#include "timer_hal.h"
#include <driver/timer.h>
#include "clock_driver.h"
#include "power_driver.h"
#include <driver/int.h>
#include <modules/pm.h>
#include "cmsis_gcc.h"
#include "sys_driver.h"
#include "timer_driver.h"

#if (SOC_TIMER_GROUP_NUM > 1)
static void timer1_isr(void) __BK_SECTION(".itcm");
#endif
static void timer_isr(void) __BK_SECTION(".itcm");

typedef struct {
    timer_hal_t hal;
#if CONFIG_TIMER_PM_CB_SUPPORT
    uint32_t pm_backup[SOC_TIMER_GROUP_NUM-1][TIMER_PM_BACKUP_REG_NUM];
    uint8_t pm_bakeup_is_valid;
#endif
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
    uint32_t fast_backup[SOC_TIMER_GROUP_NUM][5];
    bool fast_backup_valid;
#endif
} timer_driver_t;

static timer_driver_t s_timer = {0};
static timer_isr_t s_timer_isr[SOC_TIMER_CHAN_NUM_PER_UNIT] = {NULL};
static bool s_timer_driver_is_init = false;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
static bool s_timer_fast_pm_registered;
#endif

#define TIMER_RETURN_ON_NOT_INIT() do {\
        if (!s_timer_driver_is_init) {\
            return BK_ERR_TIMER_NOT_INIT;\
        }\
    } while(0)

#define TIMER_RETURN_ON_INVALID_ID(id) do {\
        if ((id) >= TIMER_ID_MAX) {\
            return BK_ERR_TIMER_ID;\
        }\
    } while(0)

// Timer ID to hardware channel mapping for AP
// Only TIMER_ID12-17 are supported on AP:
// TIMER_ID12-14: map to TIMER-4 channel 0-2 (hardware channel 0-2, timer_id - 12)
// TIMER_ID15-17: map to TIMER-5 channel 0-2 (hardware channel 3-5, timer_id - 12)
static inline uint32_t timer_id_to_hw_chan(timer_id_t timer_id)
{
    // TIMER_ID12-17 map to hardware channel 0-5 (timer_id - 12)
    return timer_id - TIMER_ID12;
}

static inline timer_id_t timer_hw_chan_to_id(uint32_t hw_chan)
{
    return (timer_id_t)(TIMER_ID12 + hw_chan);
}

// On AP side, only TIMER_ID12-17 are supported
#define TIMER_RETURN_ON_UNSUPPORTED_ID(id) do {\
        if ((id) < TIMER_ID12 || (id) > TIMER_ID17) {\
            TIMER_LOGE("timer id %d is not supported on AP, only TIMER_ID12-17 are supported\r\n", (id));\
            return BK_ERR_TIMER_ID;\
        }\
    } while(0)

#define TIMER_RETURN_ON_IS_RUNNING(id, status) do {\
        if ((status) & BIT((id))) {\
            return BK_ERR_TIMER_IS_RUNNING;\
        }\
    } while(0)

#define TIMER_RETURN_TIMER_ID_IS_ERR(id) do {\
        if ((~CONFIG_TIMER_SUPPORT_ID_BITS) & BIT((id))) {\
            return BK_ERR_TIMER_ID_ON_DEFCONFIG;\
        }\
    } while(0)

#if CONFIG_TIMER_PM_CB_SUPPORT
//TODO: 1_15 modify the group_id，need to verify
#define TIMER_PM_CHECK_RESTORE(id) do {\
    uint32_t group_id;\
    uint32_t hw_chan = timer_id_to_hw_chan(id);\
    GLOBAL_INT_DECLARATION();\
    GLOBAL_INT_DISABLE();\
    group_id = hw_chan / SOC_TIMER_CHAN_NUM_PER_GROUP;\
    switch (group_id){\
    case 0:\
        break;\
    case 1:\
        if (bk_pm_module_lv_sleep_state_get(PM_DEV_ID_TIMER_1)) {\
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1, PM_POWER_MODULE_STATE_ON);\
            timer_pm_restore(0, (void *)group_id);\
            bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_TIMER_1);\
            }\
        break;\
    default:\
        break;\
    }\
    GLOBAL_INT_RESTORE();\
}while(0)
#else
#define TIMER_PM_CHECK_RESTORE(id)
#endif

// static void timer_clock_select(timer_id_t id, timer_src_clk_t mode)
// {
// 	uint32_t group_index = 0;
// 	uint32_t hw_chan = timer_id_to_hw_chan(id);

// 	group_index = hw_chan / SOC_TIMER_CHAN_NUM_PER_GROUP;
// 	switch(group_index)
// 	{
// 		case 0:
// 			sys_drv_timer_select_clock(SYS_SEL_TIMER0, mode);
// 			break;
// 		case 1:
// 			sys_drv_timer_select_clock(SYS_SEL_TIMER1, mode);
// 			break;
// 		default:
// 			break;
// 	}
// }

static void timer_clock_enable(timer_id_t id)
{
	uint32_t group_index = 0;
	uint32_t hw_chan = timer_id_to_hw_chan(id);

	group_index = hw_chan / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
			bk_pm_clock_ctrl(PM_CLK_ID_TIMER4, PM_CLK_CTRL_PWR_UP);
			break;
		case 1:
			bk_pm_clock_ctrl(PM_CLK_ID_TIMER5, PM_CLK_CTRL_PWR_UP);
			break;
		default:
			break;
	}
}

static void timer_clock_disable(timer_id_t id)
{
	uint32_t group_index = 0;
	uint32_t hw_chan = timer_id_to_hw_chan(id);

	group_index = hw_chan / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
			bk_pm_clock_ctrl(PM_CLK_ID_TIMER4, PM_CLK_CTRL_PWR_DOWN);
			break;
		case 1:
			bk_pm_clock_ctrl(PM_CLK_ID_TIMER5, PM_CLK_CTRL_PWR_DOWN);
			break;
		default:
			break;
	}
}

static void timer_interrupt_enable(timer_id_t id)
{
	uint32_t group_index = 0;
	uint32_t hw_chan = timer_id_to_hw_chan(id);

	group_index = hw_chan / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
#if CONFIG_SOC_SMP
            sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_TIMER4, 1);
#else
            sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_TIMER4, 1);
#endif
			break;

		case 1:
#if CONFIG_SOC_SMP
            sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_TIMER5, 1);
#else
            sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_TIMER5, 1);
#endif
			break;

		default:
			break;
	}
}

static void timer_chan_init_common(timer_id_t timer_id)
{
	timer_clock_enable(timer_id);
}

static void timer_chan_deinit_common(timer_id_t timer_id)
{
    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    timer_hal_stop_common(&s_timer.hal, hw_chan);
    timer_hal_reset_config_to_default(&s_timer.hal, hw_chan);

	timer_clock_disable(timer_id);

}

static void timer_chan_enable_interrupt_common(timer_id_t timer_id)
{
    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);

	timer_interrupt_enable(timer_id);

    timer_hal_enable_interrupt(&s_timer.hal, hw_chan);
}

#if CONFIG_TIMER_PM_CB_SUPPORT
static bk_err_t timer_pm_backup(uint64_t sleep_time, void *args)
{
    uint32_t group_id = (uint32_t)args;
    TIMER_RETURN_ON_NOT_INIT();

    if (group_id > SOC_TIMER_GROUP_NUM - 1)
        return BK_FAIL;

    if (group_id < 1)
        return BK_OK;

    for (group_id; group_id < SOC_TIMER_GROUP_NUM; group_id++ )
    {
        if (!s_timer.pm_bakeup_is_valid)
        {
            timer_hal_backup(&s_timer.hal, group_id, s_timer.pm_backup[group_id - 1]);
            s_timer.pm_bakeup_is_valid = 1;
        }
    }

    return BK_OK;
}

static bk_err_t timer_pm_restore(uint64_t sleep_time, void *args)
{
    uint32_t group_id = (uint32_t)args;
    TIMER_RETURN_ON_NOT_INIT();

    if (group_id > SOC_TIMER_GROUP_NUM - 1)
        return BK_FAIL;

    if (group_id < 1)
        return BK_OK;

    for (group_id; group_id < SOC_TIMER_GROUP_NUM; group_id++ )
    {
        if (s_timer.pm_bakeup_is_valid)
        {
            timer_hal_restore(&s_timer.hal, group_id, s_timer.pm_backup[group_id - 1]);
            s_timer.pm_bakeup_is_valid = 0;
        }
    }

    return BK_OK;
}

static void timer_register_lvsleep_cb(uint32_t group_id)
{
    pm_cb_conf_t timer_enter_config = {
        .cb = (pm_cb)timer_pm_backup,
        .args = (void *)group_id
        };

    switch(group_id)
    {
        case 0:
            break;  // timer0 of BK7236 is in AON domain
        case 1:
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1, PM_POWER_MODULE_STATE_ON);
            bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_TIMER_1, &timer_enter_config, NULL);
            bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_TIMER_1);
            break;
        default:
            break;
    }
}

static void timer_unregister_lvsleep_cb(uint32_t group_id)
{
    switch(group_id)
    {
        case 0:
            break;
        case 1:
            bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_TIMER_1, true, true);
            bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1, PM_POWER_MODULE_STATE_OFF);
            break;
        default:
            break;
    }
}
#endif

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#define TIMER_FAST_CTRL_MASK (0x7FU)

static bk_err_t timer_fast_backup(void *arg)
{
    (void)arg;

    for (uint32_t group = 0; group < SOC_TIMER_GROUP_NUM; group++) {
        s_timer.fast_backup[group][0] =
            s_timer.hal.hw->group[group].global_ctrl.v;
        s_timer.fast_backup[group][1] =
            s_timer.hal.hw->group[group].timer_cnt[0];
        s_timer.fast_backup[group][2] =
            s_timer.hal.hw->group[group].timer_cnt[1];
        s_timer.fast_backup[group][3] =
            s_timer.hal.hw->group[group].timer_cnt[2];
        s_timer.fast_backup[group][4] =
            s_timer.hal.hw->group[group].ctrl.v & TIMER_FAST_CTRL_MASK;

        for (uint32_t chan = 0; chan < SOC_TIMER_CHAN_NUM_PER_GROUP; chan++) {
            timer_ll_set_enable(s_timer.hal.hw, group, chan, 0);
        }
    }
    s_timer.fast_backup_valid = true;
    __DMB();
    return BK_OK;
}

static bk_err_t timer_fast_restore(void *arg)
{
    (void)arg;

    if (!s_timer.fast_backup_valid) {
        return BK_OK;
    }

    for (uint32_t group = 0; group < SOC_TIMER_GROUP_NUM; group++) {
        s_timer.hal.hw->group[group].global_ctrl.v =
            s_timer.fast_backup[group][0];
        s_timer.hal.hw->group[group].timer_cnt[0] =
            s_timer.fast_backup[group][1];
        s_timer.hal.hw->group[group].timer_cnt[1] =
            s_timer.fast_backup[group][2];
        s_timer.hal.hw->group[group].timer_cnt[2] =
            s_timer.fast_backup[group][3];
        s_timer.hal.hw->group[group].ctrl.v =
            s_timer.fast_backup[group][4];
    }
    s_timer.fast_backup_valid = false;
    __DMB();
    return BK_OK;
}

static const pm_ap_fast_pm_ops_t s_timer_fast_pm_ops = {
    .name = "timer45",
    .backup = timer_fast_backup,
    .restore = timer_fast_restore,
    .priority = PM_AP_FAST_PRIORITY_PERIPHERAL,
};
#endif

bk_err_t bk_timer_driver_init(void)
{
    if (s_timer_driver_is_init) {
        return BK_OK;
    }

    os_memset(&s_timer, 0, sizeof(s_timer));
    os_memset(&s_timer_isr, 0, sizeof(s_timer_isr));

#if CONFIG_TIMER_PM_CB_SUPPORT
    for(uint32_t group_id = 1; group_id < SOC_TIMER_GROUP_NUM; group_id++)
    {
        timer_register_lvsleep_cb(group_id);
    }
#endif

    bk_int_isr_register(INT_SRC_TIMER4, timer_isr, NULL);
#if (SOC_TIMER_GROUP_NUM > 1)
    bk_int_isr_register(INT_SRC_TIMER5, timer1_isr, NULL);
#endif
    timer_hal_init(&s_timer.hal);

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
    bk_err_t pm_ret = bk_pm_ap_fast_ops_register(&s_timer_fast_pm_ops);
    if (pm_ret != BK_OK) {
        return pm_ret;
    }
    s_timer_fast_pm_registered = true;
#endif

    s_timer_driver_is_init = true;

    return BK_OK;
}

bk_err_t bk_timer_driver_deinit(void)
{
    if (!s_timer_driver_is_init) {
        return BK_OK;
    }

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
    if (s_timer_fast_pm_registered) {
        bk_err_t pm_ret =
            bk_pm_ap_fast_ops_unregister(&s_timer_fast_pm_ops);
        if (pm_ret != BK_OK) {
            return pm_ret;
        }
        s_timer_fast_pm_registered = false;
    }
#endif

#if CONFIG_TIMER_PM_CB_SUPPORT
    for (uint32_t group_id = 1; group_id < SOC_TIMER_GROUP_NUM; group_id++)
    {
        timer_unregister_lvsleep_cb(group_id);
    }
#endif

    for (timer_id_t id = TIMER_ID12; id <= TIMER_ID17; id++) {
        timer_chan_deinit_common(id);
    }

    s_timer_driver_is_init = false;

    return BK_OK;
}

extern void delay(int num);//TODO fix me

bk_err_t bk_timer_start_without_callback(timer_id_t timer_id, uint32_t time_ms)
{
    uint32_t en_status = 0;

#if CONFIG_TIMER_US
	if (timer_id == TIMER_ID0) {
		TIMER_LOGE("timer0 is reserved for us timer!\r\n");
	}
#endif

    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    timer_chan_init_common(timer_id);

#if !CONFIG_RTC_TIMER_PRECISION_TEST
    timer_chan_enable_interrupt_common(timer_id);
#endif

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    en_status = timer_hal_get_enable_status(&s_timer.hal);
    if (en_status & BIT(hw_chan)) {
        TIMER_LOGV("timer(%d) is running, stop it\r\n", timer_id);
        timer_hal_disable(&s_timer.hal, hw_chan);
        /* Delay to fix the bug that timer counter becomes bigger than
         * timer period. Once timer counter becomes bigger than timer period,
         * the timer will never timeout, or takes very very long time to
         * timeout.
         *
         * This issue is firstly observed in HOS tick timer. HOS restarts
         * the tick timer with different time_ms(timer period) again and again,
         * without the delay, the tick timer counter becomes bigger than timer
         * period very soon, then the tick interrupt will never be triggered.
         * */
        delay(4);
    }

    timer_hal_init_timer(&s_timer.hal, hw_chan, time_ms, TIMER_UNIT_MS);
    timer_hal_start_common(&s_timer.hal, hw_chan);

    return BK_OK;
}

bk_err_t bk_timer_start(timer_id_t timer_id, uint32_t time_ms, timer_isr_t callback)
{
#if CONFIG_TIMER_US
	if (timer_id == TIMER_ID0) {
		TIMER_LOGE("timer0 is reserved for us timer!\r\n");
	}
#endif

#if CONFIG_TIMER_SUPPORT_ID_BITS
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
#endif
    BK_LOG_ON_ERR(bk_timer_start_without_callback(timer_id, time_ms));

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    if (hw_chan < SOC_TIMER_CHAN_NUM_PER_UNIT){
        s_timer_isr[hw_chan] = callback;
    }

    return BK_OK;
}

bk_err_t bk_timer_stop(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
#if CONFIG_TIMER_SUPPORT_ID_BITS
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
#endif
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    timer_hal_stop_common(&s_timer.hal, hw_chan);

    return BK_OK;
}

bk_err_t bk_timer_cancel(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
#if CONFIG_TIMER_SUPPORT_ID_BITS
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
#endif
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    timer_ll_reset_config_to_default(s_timer.hal.hw, hw_chan);

    s_timer_isr[hw_chan] = NULL;

    return BK_OK;
}

uint32_t bk_timer_get_cnt(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    return timer_hal_get_count(&s_timer.hal, hw_chan);
}

bk_err_t bk_timer_enable(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    timer_hal_enable(&s_timer.hal, hw_chan);
    return BK_OK;
}

bk_err_t bk_timer_disable(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    timer_hal_disable(&s_timer.hal, hw_chan);
    return BK_OK;
}

uint32_t bk_timer_get_period(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    return timer_hal_get_end_count(&s_timer.hal, hw_chan);
}

uint32_t bk_timer_get_enable_status(void)
{
    TIMER_RETURN_ON_NOT_INIT();
    return timer_hal_get_enable_status(&s_timer.hal);
}

bool bk_timer_is_interrupt_triggered(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    uint32_t int_status = timer_hal_get_interrupt_status(&s_timer.hal);
    return timer_hal_is_interrupt_triggered(&s_timer.hal, hw_chan, int_status);
}

uint32_t timer_clear_isr_status(void)
{
	uint32_t int_status;
    timer_hal_t *hal = &s_timer.hal;

    int_status = timer_hal_get_interrupt_status(hal);
    timer_hal_clear_interrupt_status(hal, int_status);

	return int_status;
}

/* GCC 14+ requires general-regs-only for interrupt handlers when FPU is enabled. */
#pragma GCC push_options
#pragma GCC target("general-regs-only")

static void timer_group_isr(uint32_t group)
{
    uint32_t int_status;
    timer_hal_t *hal = &s_timer.hal;
    uint32_t chan_base = group * SOC_TIMER_CHAN_NUM_PER_GROUP;

    int_status = timer_hal_get_group_interrupt_status(hal, group);
    timer_hal_clear_group_interrupt_status(hal, group, int_status);

    for (int i = 0; i < SOC_TIMER_CHAN_NUM_PER_GROUP; i++) {
        if (int_status & BIT(i)) {
            uint32_t hw_chan = chan_base + i;
            if (s_timer_isr[hw_chan]) {
                s_timer_isr[hw_chan](timer_hw_chan_to_id(hw_chan));
            }
        }
    }
}

static void timer_isr(void)
{
    timer_group_isr(0);
}

#if (SOC_TIMER_GROUP_NUM > 1)
static void timer1_isr(void)
{
    timer_group_isr(1);
}
#endif

#pragma GCC pop_options

uint64_t bk_timer_get_time(timer_id_t timer_id, uint32_t div, uint32_t last_count, timer_value_unit_t unit_type)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint64_t current_time = 0;
    uint64_t unit_factor = 1;

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    uint64_t current_count = timer_hal_get_count(&s_timer.hal, hw_chan) + last_count;

    if (div == 0) {
        div = 1;
    }

    unit_factor = (unit_type == TIMER_UNIT_MS) ? 1 : 1000;

    current_time = unit_factor * current_count * (uint64_t)div / timer_hal_get_counter_freq_khz();



    return current_time;
}

bk_err_t bk_timer_delay_with_callback(timer_id_t timer_id, uint64_t time_us, timer_isr_t callback)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

#if CONFIG_TIMER_SUPPORT_ID_BITS
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
#endif
    uint64_t current_count = 0;
    uint64_t delta_count = 0;
    uint64_t end_count = 0;

    TIMER_PM_CHECK_RESTORE(timer_id);

    timer_chan_init_common(timer_id);
    timer_chan_enable_interrupt_common(timer_id);

    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);
    current_count = timer_hal_get_count(&s_timer.hal, hw_chan);
    delta_count = timer_hal_cal_end_count(hw_chan, time_us, 1, TIMER_UNIT_US);
    end_count = current_count + delta_count;

    if(end_count > 0xFFFFFFFFFFFFFFFF){
        end_count = 0xFFFFFFFFFFFFFFFF;
    }

    timer_ll_set_end_count(s_timer.hal.hw, hw_chan, (uint32_t)end_count);
    timer_ll_set_clk_div(s_timer.hal.hw, hw_chan, 0);
    timer_ll_clear_chan_interrupt_status(s_timer.hal.hw, hw_chan);

    timer_hal_start_common(&s_timer.hal, hw_chan);

    s_timer_isr[hw_chan] = callback;

    return BK_OK;
}

#if CONFIG_TIMER_US
__IRAM_SEC void bk_timer_delay_us(uint32_t us)
{
	timer_hal_delay_us(us);
}
#endif

bk_err_t bk_timer_start_us(timer_id_t timer_id, uint64_t time_us, timer_isr_t callback)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    TIMER_RETURN_ON_UNSUPPORTED_ID(timer_id);

#if CONFIG_TIMER_SUPPORT_ID_BITS
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
#endif

    TIMER_PM_CHECK_RESTORE(timer_id);

    uint32_t en_status = 0;
    uint32_t hw_chan = timer_id_to_hw_chan(timer_id);

    timer_chan_init_common(timer_id);
#if !CONFIG_RTC_TIMER_PRECISION_TEST
    timer_chan_enable_interrupt_common(timer_id);
#endif

    en_status = timer_hal_get_enable_status(&s_timer.hal);
    if (en_status & BIT(hw_chan)) {
        TIMER_LOGD("timer(%d) is running, stop it\r\n", timer_id);
        timer_hal_disable(&s_timer.hal, hw_chan);
        delay(4);
    }

    timer_hal_init_timer(&s_timer.hal, hw_chan, time_us, TIMER_UNIT_US);

    uint32_t int_level = rtos_enter_critical();
    timer_hal_start_common(&s_timer.hal, hw_chan);
    if (hw_chan < SOC_TIMER_CHAN_NUM_PER_UNIT) {
        s_timer_isr[hw_chan] = callback;
    }
    rtos_exit_critical(int_level);

    return BK_OK;
}
