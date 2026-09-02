// Copyright 2021-2025 Beken
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
#include <modules/pm.h>
#include "sys_driver.h"
#include <driver/rosc_32k.h>
#include <driver/pwr_clk.h>
#include <driver/aon_rtc.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "driver/flash.h"
#include <os/os.h>

#include "pm_sleep.h"
#include "pm_power.h"
#include "pm_psram.h"
#include "pm_debug.h"
#include "pm_wakeup_source.h"
#include "pm_interface.h"
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
#include "FreeRTOS.h"
#include "task.h"
static volatile bool s_pm_fast_resume_freeze_ticks;
#endif

/*=========================SLEEP/WAKEUP FUNCTION START========================*/
static void pm_enter_cpu_wfi()
{
	sys_drv_enter_cpu_wfi();
}

uint64_t pm_cpu_wfi_process()
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	uint64_t sleep_tick         = 0ULL;
	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	uint64_t entry_tick         = 0ULL;
	entry_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	#endif

	pm_enter_cpu_wfi();

	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	uint64_t exit_tick          = 0ULL;
	exit_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	if(exit_tick - entry_tick < 0)
	{
		sleep_tick = 0ULL;
	}
	else
	{
		sleep_tick = exit_tick - entry_tick;
	}
	#endif
	GLOBAL_INT_RESTORE();
	return sleep_tick;
}
uint64_t pm_management(uint32_t sleep_ticks)
{
	uint64_t missed_ticks = 0ULL;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
	/*
	 * Latch this before WFI. The resume path changes the fast-PM state to
	 * RUNNING before the FreeRTOS port performs slept-tick compensation.
	 */
	if (bk_pm_ap_fast_suspend_is_prepared()) {
		s_pm_fast_resume_freeze_ticks = true;
		__DMB();
	}
#endif
	missed_ticks = pm_cpu_wfi_process();
	return missed_ticks;
}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_CPU_HOTPLUG
TickType_t bk_pm_fast_resume_adjust_slept_ticks(TickType_t slept_ticks)
{
	/*
	 * AP tasks are suspended, not running, while the AP power domain is off.
	 * Keep their relative FreeRTOS delays paused; wall-clock users continue
	 * to use AON RTC.  This also prevents an expired-task burst from delaying
	 * CPU3 restore before AP_FULL_READY.
	 */
	__DMB();
	if (s_pm_fast_resume_freeze_ticks) {
		s_pm_fast_resume_freeze_ticks = false;
		__DMB();
		return 1;
	}
	return slept_ticks;
}

void bk_pm_fast_resume_post_irq_enable(void)
{
	/* Called by the FreeRTOS tickless port after PRIMASK is cleared. */
	if (!bk_pm_ap_full_ready_get()) {
		taskYIELD();
	}
}
#endif
/*=========================ENTER SLEEP FUNCTION END========================*/
