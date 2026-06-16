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

#include "timer_hal.h"
#include "timer_ll.h"
#include "sys_hal.h"
#include <modules/pm.h>
#include <soc/bk7259/timer_cap.h>
#include "sdkconfig.h"

/*
 * timer_s = counter_value * (1 / (freq /div))
 *
 * AP TIMER4/5 are on AHBP and count at bus clock (not CP TIMER0~3 XTAL mux).
 */
static uint32_t timer_hal_bus_clock_hz(pm_cpu_freq_e cpu_freq)
{
	switch (cpu_freq) {
	case PM_CPU_FRQ_XTAL:
		return CONFIG_XTAL_FREQ;
	case PM_CPU_FRQ_60M:
		return 60000000U;
	case PM_CPU_FRQ_80M:
		return 80000000U;
	case PM_CPU_FRQ_120M:
		return 120000000U;
	case PM_CPU_FRQ_160M:
		return 160000000U;
	case PM_CPU_FRQ_240M:
		return 120000000U;
	case PM_CPU_FRQ_320M:
		return 160000000U;
	case PM_CPU_FRQ_480M:
		return 240000000U;
	case PM_CPU_FRQ_HIGHEST:
	case PM_CPU_FRQ_DEFAULT:
	default:
		return 160000000U;
	}
}

uint32_t timer_hal_get_counter_freq_khz(void)
{
	pm_cpu_freq_e cpu_freq = bk_pm_current_max_cpu_freq_get();

	if (cpu_freq == PM_CPU_FRQ_XTAL || cpu_freq == PM_CPU_FRQ_DEFAULT) {
		pm_cpu_freq_e hw_freq = sys_hal_get_cpu_bus_freq();
		if (hw_freq != PM_CPU_FRQ_XTAL) {
			cpu_freq = hw_freq;
		} else {
			cpu_freq = (pm_cpu_freq_e)CONFIG_PM_CPU_FRQ_HIGHEST;
		}
	}

	return timer_hal_bus_clock_hz(cpu_freq) / 1000U;
}

uint32_t timer_hal_cal_end_count(timer_id_t chan, uint64_t time, uint32_t div, timer_value_unit_t unit_type)
{
	uint32_t counter_freq_khz;
	uint64_t value = 0;
	uint16_t unit_factor = 1;

	(void)chan;

	if (div == 0) {
		div = 1;
	}

	unit_factor = (unit_type == TIMER_UNIT_MS) ? 1 : 1000;
	counter_freq_khz = timer_hal_get_counter_freq_khz();
	value = time * counter_freq_khz / unit_factor / div;

	if (value > 0xffffffff) {
		value = 0xffffffff;
	}

	return (uint32_t)value;
}

bk_err_t timer_hal_init(timer_hal_t *hal)
{
    hal->hw = (timer_hw_t *)TIMER_LL_REG_BASE(hal->id);

    for (int chan = 0; chan < SOC_TIMER_CHAN_NUM_PER_UNIT; chan++) {
        timer_ll_init(hal->hw, chan);
    }

    return BK_OK;
}

bk_err_t timer_hal_init_timer(timer_hal_t *hal, timer_id_t chan, uint64_t time, timer_value_unit_t unit_type)
{
    uint32_t end_count = timer_hal_cal_end_count(chan, time, 1, unit_type);
    timer_ll_set_end_count(hal->hw, chan, end_count);
    timer_ll_set_clk_div(hal->hw, chan, 0);
    timer_ll_clear_chan_interrupt_status(hal->hw, chan);
    return BK_OK;
}

bk_err_t timer_hal_set_period(timer_hal_t *hal, timer_id_t chan, uint32_t time_ms)
{
    uint32_t end_count = timer_hal_cal_end_count(chan, time_ms, 1, TIMER_UNIT_MS);
    timer_ll_set_end_count(hal->hw, chan, end_count);
    return BK_OK;
}


bk_err_t timer_hal_start_common(timer_hal_t *hal, timer_id_t chan)
{
    timer_ll_enable(hal->hw, chan);
    return BK_OK;
}

bk_err_t timer_hal_stop_common(timer_hal_t *hal, timer_id_t chan)
{
    timer_ll_disable_interrupt(hal->hw, chan);
    timer_ll_clear_chan_interrupt_status(hal->hw, chan);
    timer_ll_disable(hal->hw, chan);
    return BK_OK;
}

uint32_t timer_hal_get_count(timer_hal_t *hal, timer_id_t chan)
{
    uint32_t en_status = timer_ll_get_enable_status(hal->hw);
    if (!(en_status & BIT(chan))) {
        return 0;
    }
    timer_ll_set_read_index(hal->hw, chan);
    timer_ll_set_cnt_read(hal->hw, chan);

    //Wait hardware to prepare the data
    BK_WHILE (!timer_ll_is_cnt_read_valid(hal->hw, chan));

    return timer_ll_get_timer_count(hal->hw, chan);
}

