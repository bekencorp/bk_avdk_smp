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
#include <common/bk_assert.h>
#include <components/log.h>
#include <driver/timer.h>
#include <os/os.h>
#include "bk_wdt.h"

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#include <driver/aon_rtc.h>
#endif

#if CONFIG_TASK_WDT

#define TASK_WDT_TAG "task_wdt"
#define TASK_WDT_LOGW(...) BK_LOGW(TASK_WDT_TAG, ##__VA_ARGS__)

#define TASK_WDT_BARK_TIME_MS    1000
#define TASK_WDT_PERIOD_TICK     (BK_MS_TO_TICKS(CONFIG_TASK_WDT_PERIOD_MS))

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#define GET_TASK_CURRENT_TICK()  (BK_MS_TO_TICKS(bk_aon_rtc_get_us() / 1000))
#else
#define GET_TASK_CURRENT_TICK()  (bk_get_tick())
#endif

static bool s_task_wdt_driver_is_init = false;
static uint64_t s_last_task_wdt_feed_tick = 0;
static uint64_t s_last_task_wdt_log_tick = 0;
static bool s_task_wdt_enabled = true;

void bk_task_wdt_feed_handle(void)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_task_wdt_timeout_check();
	GLOBAL_INT_RESTORE();
}

bk_err_t bk_task_wdt_driver_init(void)
{
	if (s_task_wdt_driver_is_init) {
		return BK_OK;
	}

	bk_timer_start(TIMER_ID2, TASK_WDT_BARK_TIME_MS, (timer_isr_t)bk_task_wdt_feed_handle);
	s_task_wdt_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_task_wdt_driver_deinit(void)
{
	if (!s_task_wdt_driver_is_init) {
		return BK_OK;
	}

	bk_timer_stop(TIMER_ID2);
	s_task_wdt_driver_is_init = false;

	return BK_OK;
}

__IRAM_SEC void bk_task_wdt_start(void)
{
	s_task_wdt_enabled = true;
}

__attribute__((section(".itcm_sec_code"))) void bk_task_wdt_stop(void)
{
	s_task_wdt_enabled = false;
}

void bk_task_wdt_feed(void)
{
	s_last_task_wdt_feed_tick = GET_TASK_CURRENT_TICK();
}

void bk_task_wdt_timeout_check(void)
{
	if (s_last_task_wdt_feed_tick && s_task_wdt_enabled) {
		const uint64_t c_last_feed_tick = s_last_task_wdt_feed_tick;
		const uint64_t current_tick = GET_TASK_CURRENT_TICK();

		if (current_tick > c_last_feed_tick) {
			if ((current_tick - c_last_feed_tick) > TASK_WDT_PERIOD_TICK) {
				if ((current_tick - s_last_task_wdt_log_tick) > TASK_WDT_PERIOD_TICK) {
					TASK_WDT_LOGW("task watchdog triggered\r\n");
					s_last_task_wdt_log_tick = current_tick;
					BK_ASSERT(0);
				}
			}
		}
	}
}

#endif // CONFIG_TASK_WDT
