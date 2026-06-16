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
#include <os/os.h>
#include "bk_wdt.h"

#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
#include "FreeRTOS.h"
#include "task.h"
#endif

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#include <driver/aon_rtc.h>
#endif

#if CONFIG_TASK_WDT

#define TASK_WDT_TAG "task_wdt"
#define TASK_WDT_LOGW(...) BK_LOGW(TASK_WDT_TAG, ##__VA_ARGS__)
#define TASK_WDT_LOGE(...) BK_LOGE(TASK_WDT_TAG, ##__VA_ARGS__)

#define TASK_WDT_CHECK_PERIOD_TICK (BK_MS_TO_TICKS(1000))
#define TASK_WDT_PERIOD_TICK     (BK_MS_TO_TICKS(CONFIG_TASK_WDT_PERIOD_MS))

#if CONFIG_SOC_SMP
#define TASK_WDT_CORE_NUM CONFIG_SMP_CORE_CNT
#else
#define TASK_WDT_CORE_NUM 1
#endif

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#define GET_TASK_CURRENT_TICK()  (BK_MS_TO_TICKS(bk_aon_rtc_get_ms()))
#else
#define GET_TASK_CURRENT_TICK()  (bk_get_tick())
#endif

static bool s_task_wdt_driver_is_init = false;
/* Per-core task WDT state used by feed/check paths. */
static uint64_t s_last_task_wdt_feed_tick[TASK_WDT_CORE_NUM] = {0};
/* Per-core timestamp used to throttle repeated timeout logs. */
static uint64_t s_last_task_wdt_log_tick[TASK_WDT_CORE_NUM] = {0};
static uint64_t s_last_task_wdt_check_tick = 0;
static uint32_t s_task_wdt_feed_bits = 0;
static bool s_task_wdt_enabled = false;
#if CONFIG_TASK_WDT_TEST
static uint32_t s_task_wdt_skip_feed_bits = 0;
#endif

static inline uint32_t task_wdt_get_current_core_id(void)
{
#if CONFIG_SOC_SMP
	return rtos_get_core_id() & 0x1;
#else
	return 0;
#endif
}

static void task_wdt_reset_state(void)
{
	uint32_t core_id;

	for (core_id = 0; core_id < TASK_WDT_CORE_NUM; core_id++) {
		s_last_task_wdt_feed_tick[core_id] = 0;
		s_last_task_wdt_log_tick[core_id] = 0;
	}

	s_last_task_wdt_check_tick = 0;
	s_task_wdt_feed_bits = 0;
	s_task_wdt_enabled = false;
#if CONFIG_TASK_WDT_TEST
	s_task_wdt_skip_feed_bits = 0;
#endif
}

void bk_task_wdt_systick_check(void)
{
	uint64_t current_tick = GET_TASK_CURRENT_TICK();

	if ((current_tick - s_last_task_wdt_check_tick) >= TASK_WDT_CHECK_PERIOD_TICK) {
		s_last_task_wdt_check_tick = current_tick;
		bk_task_wdt_timeout_check();
	}
}

bk_err_t bk_task_wdt_driver_init(void)
{
	if (s_task_wdt_driver_is_init) {
		return BK_OK;
	}

	task_wdt_reset_state();
	s_task_wdt_driver_is_init = true;

#if CONFIG_CLI && CONFIG_TASK_WDT_TEST
	int bk_task_wdt_register_cli_test_feature(void);
	bk_task_wdt_register_cli_test_feature();
#endif

	return BK_OK;
}

bk_err_t bk_task_wdt_driver_deinit(void)
{
	if (!s_task_wdt_driver_is_init) {
		return BK_OK;
	}

	task_wdt_reset_state();
	s_task_wdt_driver_is_init = false;

	return BK_OK;
}

__IRAM_SEC void bk_task_wdt_start(void)
{
	uint32_t core_id;
	uint64_t current_tick = GET_TASK_CURRENT_TICK();

	s_task_wdt_feed_bits = 0;
	s_last_task_wdt_check_tick = current_tick;

	for (core_id = 0; core_id < TASK_WDT_CORE_NUM; core_id++) {
		s_last_task_wdt_feed_tick[core_id] = current_tick;
		s_last_task_wdt_log_tick[core_id] = current_tick;
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
		if (xTaskIsCoreActive(core_id) == pdFALSE) {
			continue;
		}
#endif
		s_task_wdt_feed_bits |= BIT(core_id);
	}

	s_task_wdt_enabled = true;
}

__attribute__((section(".itcm_sec_code"))) void bk_task_wdt_stop(void)
{
	s_task_wdt_enabled = false;
}

void bk_task_wdt_feed(void)
{
	uint32_t core_id = task_wdt_get_current_core_id();

	if (core_id >= TASK_WDT_CORE_NUM) {
		return;
	}

#if CONFIG_TASK_WDT_TEST
	if (s_task_wdt_skip_feed_bits & BIT(core_id)) {
		return;
	}
#endif

	s_last_task_wdt_feed_tick[core_id] = GET_TASK_CURRENT_TICK();
	s_task_wdt_feed_bits |= BIT(core_id);
}

void bk_task_wdt_timeout_check(void)
{
	uint32_t core_id;
	const uint64_t current_tick = GET_TASK_CURRENT_TICK();

	if (!s_task_wdt_enabled) {
		return;
	}

	for (core_id = 0; core_id < TASK_WDT_CORE_NUM; core_id++) {
		if (!(s_task_wdt_feed_bits & BIT(core_id))) {
			continue;
		}

		const uint64_t c_last_feed_tick = s_last_task_wdt_feed_tick[core_id];

		if (current_tick > c_last_feed_tick) {
			if ((current_tick - c_last_feed_tick) > TASK_WDT_PERIOD_TICK) {
				if ((current_tick - s_last_task_wdt_log_tick[core_id]) > TASK_WDT_PERIOD_TICK) {
					BK_DUMP_OUT("task watchdog triggered, core:%u\r\n", core_id);
					s_last_task_wdt_log_tick[core_id] = current_tick;
					BK_ASSERT(0);
				}
			}
		}
	}
}

bk_err_t bk_task_wdt_set_feed_bits(uint32_t core_id, bool set_flag)
{
	if (core_id >= TASK_WDT_CORE_NUM) {
		return BK_FAIL;
	}

	if (set_flag) {
		uint64_t current_tick = GET_TASK_CURRENT_TICK();
		s_task_wdt_feed_bits |= BIT(core_id);
		s_last_task_wdt_feed_tick[core_id] = current_tick;
		s_last_task_wdt_log_tick[core_id] = current_tick;
	} else {
		s_task_wdt_feed_bits &= ~BIT(core_id);
		s_last_task_wdt_feed_tick[core_id] = 0;
		s_last_task_wdt_log_tick[core_id] = 0;
	}

	return BK_OK;
}

#if CONFIG_TASK_WDT_TEST
bk_err_t bk_task_wdt_set_skip_feed_core(uint32_t core_id, bool skip)
{
	if (core_id >= TASK_WDT_CORE_NUM) {
		return BK_FAIL;
	}

	if (skip) {
		s_task_wdt_skip_feed_bits |= BIT(core_id);
	} else {
		s_task_wdt_skip_feed_bits &= ~BIT(core_id);
	}

	return BK_OK;
}

uint32_t bk_task_wdt_get_feed_bits(void)
{
	return s_task_wdt_feed_bits;
}

uint32_t bk_task_wdt_get_skip_feed_bits(void)
{
	return s_task_wdt_skip_feed_bits;
}

uint64_t bk_task_wdt_get_last_feed_tick(uint32_t core_id)
{
	if (core_id >= TASK_WDT_CORE_NUM) {
		return 0;
	}

	return s_last_task_wdt_feed_tick[core_id];
}
#endif

#endif // CONFIG_TASK_WDT
