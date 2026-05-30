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

#include "sdkconfig.h"
#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/wwdt.h>
#include <driver/timer.h>
#include <driver/aon_rtc.h>
#include "wwdt_driver.h"
#include "wwdt_hal.h"
#include "reset_reason.h"
#include "sys_hal.h"
#include <components/system.h>
#include "aon_pmu_driver.h"
#include "sys_driver.h"
#if CONFIG_GPIO_RETENTION_SUPPORT
#include "gpio_driver.h"
#endif

typedef struct {
	wwdt_hal_t hal;
	uint8_t init_bits;
} wwdt_driver_t;

#define WWDT_RETURN_ON_DRIVER_NOT_INIT() do {\
	if (!s_wwdt_driver_is_init) {\
		WWDT_LOGE("WWDT driver not init\r\n");\
		return BK_ERR_WWDT_DRIVER_NOT_INIT;\
	}\
} while(0)

#define WWDT_RETURN_ON_CORE_NOT_INIT(core) do {\
	if (!(s_wwdt.init_bits & BIT(core))) {\
		return BK_ERR_WWDT_NOT_INIT;\
	}\
} while(0)

#define WWDT_RETURN_ON_INVALID_PERIOD(timeout) do {\
	if ((timeout) > WWDT_F_PERIOD_V) {\
		WWDT_LOGE("WWDT invalid timeout\r\n");\
		return BK_ERR_WWDT_INVALID_PERIOD;\
	}\
} while(0)

#if CONFIG_SOC_SMP
#define WWDT_CORE_NUM CONFIG_SMP_CORE_CNT
#else
#define WWDT_CORE_NUM 1
#endif

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#define GET_WWDT_CURRENT_TICK()  (BK_MS_TO_TICKS(bk_aon_rtc_get_ms()))
#else
#define GET_WWDT_CURRENT_TICK()  (bk_get_tick())
#endif

static wwdt_driver_t s_wwdt = {0};
static bool s_wwdt_driver_is_init = false;
static uint32_t s_wwdt_period = CONFIG_INT_WWDT_PERIOD_MS;
static uint64_t s_last_wwdt_feed_tick[WWDT_CORE_NUM] = {0};
static uint32_t s_feed_wwdt_time = 0;
static uint8_t s_debug_started_log_bits = 0;
static volatile uint8_t s_wwdt_auto_start_disable_bits = 0;
#if CONFIG_WWDT_TEST
static uint8_t s_skip_feed_bits = 0;
#endif

static inline uint32_t wwdt_get_current_core_id(void)
{
#if CONFIG_SOC_SMP
	return rtos_get_core_id() & 0x1;
#else
	return CPU0_CORE_ID;
#endif
}

static uint32_t wwdt_get_default_feed_time(void)
{
	uint32_t feed_time = BK_MS_TO_TICKS(CONFIG_INT_WWDT_PERIOD_MS / 4);

	return feed_time ? feed_time : 1;
}

static uint32_t wwdt_get_feed_time(void)
{
	return s_feed_wwdt_time ? s_feed_wwdt_time : wwdt_get_default_feed_time();
}

static bk_err_t wwdt_start_current_core(uint32_t timeout_ms, bool is_enable_window,
	uint32_t window_val, bool log_enable)
{
	uint32_t core_id = wwdt_get_current_core_id();

	if (!s_wwdt_driver_is_init) {
		if (log_enable) {
			WWDT_LOGE("WWDT driver not init\r\n");
		}
		return BK_ERR_WWDT_DRIVER_NOT_INIT;
	}

	if (timeout_ms > WWDT_F_PERIOD_V) {
		if (log_enable) {
			WWDT_LOGE("WWDT invalid timeout\r\n");
		}
		return BK_ERR_WWDT_INVALID_PERIOD;
	}

	if (!timeout_ms) {
		timeout_ms = CONFIG_INT_WWDT_PERIOD_MS;
	}

	s_wwdt_auto_start_disable_bits &= ~BIT(core_id);
	s_wwdt_period = timeout_ms;
	if (is_enable_window) {
		wwdt_hal_set_wdt_win_1st_set_win_val(window_val);
		wwdt_hal_set_wdt_win_2nd_set_win_val(window_val);
	}

	wwdt_hal_init_wwdt(&s_wwdt.hal, timeout_ms);

	if (is_enable_window) {
		wwdt_hal_set_wdt_win_set_win_en(1);
	}

	s_wwdt.init_bits |= BIT(core_id);
	s_last_wwdt_feed_tick[core_id] = GET_WWDT_CURRENT_TICK();
	if (log_enable) {
		WWDT_LOGV("bk_wwdt_start, core:%u, wwdt_cpu:%u, init_bits:%x\r\n",
			core_id, bk_wwdt_get_cpu_id(), s_wwdt.init_bits);
	}

	return BK_OK;
}

static bk_err_t wwdt_feed_current_core(void)
{
	uint32_t core_id = wwdt_get_current_core_id();
	uint64_t current_tick;

	if (!s_wwdt_driver_is_init) {
		return BK_ERR_WWDT_DRIVER_NOT_INIT;
	}

	if (!(s_wwdt.init_bits & BIT(core_id))) {
		return BK_ERR_WWDT_NOT_INIT;
	}

	current_tick = GET_WWDT_CURRENT_TICK();
	wwdt_hal_init_wwdt(&s_wwdt.hal, s_wwdt_period);
	s_last_wwdt_feed_tick[core_id] = current_tick;

	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) static void wwdt_deinit_common(void)
{
	s_wwdt_period = CONFIG_INT_WWDT_PERIOD_MS;
	wwdt_hal_reset_config_to_default(&s_wwdt.hal);
	bk_wwdt_close();
}

bk_err_t bk_wwdt_soft_reset(void)
{
	wwdt_hal_set_smb_clkrst_soft_reset(1);
	return BK_OK;
}

bk_err_t bk_wwdt_driver_init(void)
{
	if (s_wwdt_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_wwdt, 0, sizeof(s_wwdt));
	s_wwdt_auto_start_disable_bits = 0;
	wwdt_hal_init(&s_wwdt.hal);

	s_wwdt_driver_is_init = true;

#if CONFIG_CLI && CONFIG_WWDT_TEST
	int bk_wwdt_register_cli_test_feature(void);
	bk_wwdt_register_cli_test_feature();
#endif
	WWDT_LOGV("bk_wwdt_driver_init\r\n");
	return BK_OK;
}

bk_err_t bk_wwdt_driver_deinit(void)
{
	if (!s_wwdt_driver_is_init) {
		return BK_OK;
	}

	wwdt_deinit_common();
	s_wwdt.init_bits = 0;

	s_wwdt_driver_is_init = false;

	return BK_OK;
}

__IRAM_SEC bk_err_t bk_wwdt_start(uint32_t timeout_ms, bool is_enable_window, uint32_t window_val)
{
	return wwdt_start_current_core(timeout_ms, is_enable_window, window_val, true);
}

uint32_t bk_wwdt_get_window_val(void)
{
	return wwdt_hal_get_wdt_win_get_win_val();
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_wwdt_stop(void)
{
	WWDT_RETURN_ON_DRIVER_NOT_INIT();
	wwdt_deinit_common();

	return BK_OK;
}

bk_err_t bk_wwdt_feed(void)
{
	WWDT_RETURN_ON_DRIVER_NOT_INIT();
	WWDT_RETURN_ON_CORE_NOT_INIT(wwdt_get_current_core_id());

	return wwdt_feed_current_core();
}

void bk_wwdt_feed_current_core(void)
{
	uint32_t core_id = wwdt_get_current_core_id();
	uint64_t current_tick = GET_WWDT_CURRENT_TICK();

	if (!s_wwdt_driver_is_init) {
		return;
	}

	if (!(s_wwdt.init_bits & BIT(core_id))) {
		if (s_wwdt_auto_start_disable_bits & BIT(core_id)) {
			return;
		}

		BK_LOG_ON_ERR(bk_wwdt_start(CONFIG_INT_WWDT_PERIOD_MS, false, 0));
		return;
	}

	if ((current_tick - s_last_wwdt_feed_tick[core_id]) >= wwdt_get_feed_time()) {
		BK_LOG_ON_ERR(bk_wwdt_feed());
	}
}

void bk_wwdt_feed_current_core_from_isr(void)
{
	uint32_t core_id = wwdt_get_current_core_id();
	uint64_t current_tick;

	if (!s_wwdt_driver_is_init) {
		return;
	}

	if (!(s_wwdt.init_bits & BIT(core_id))) {
		if (s_wwdt_auto_start_disable_bits & BIT(core_id)) {
			return;
		}

		(void)wwdt_start_current_core(CONFIG_INT_WWDT_PERIOD_MS, false, 0, false);
		if (!(s_debug_started_log_bits & BIT(core_id))) {
			s_debug_started_log_bits |= BIT(core_id);
			WWDT_LOGI("wwdt start from systick core=%u, wwdt_cpu=%u\r\n",
				core_id, bk_wwdt_get_cpu_id());
		}
		return;
	}

#if CONFIG_WWDT_TEST
	if (s_skip_feed_bits & BIT(core_id)) {
		return;
	}
#endif

	current_tick = GET_WWDT_CURRENT_TICK();
	if ((current_tick - s_last_wwdt_feed_tick[core_id]) >= wwdt_get_feed_time()) {
		(void)wwdt_feed_current_core();  
	}
}

uint32_t bk_wwdt_get_feed_time(void)
{
	return wwdt_get_feed_time();
}

void bk_wwdt_set_feed_time(uint32_t dw_set_time)
{
	s_feed_wwdt_time = dw_set_time;
}

#if CONFIG_WWDT_TEST
bk_err_t bk_wwdt_set_skip_feed_core(uint32_t core_id, bool skip)
{
	if (core_id >= WWDT_CORE_NUM) {
		return BK_FAIL;
	}

	if (skip) {
		s_skip_feed_bits |= BIT(core_id);
	} else {
		s_skip_feed_bits &= ~BIT(core_id);
	}

	return BK_OK;
}

uint32_t bk_wwdt_get_skip_feed_bits(void)
{
	return s_skip_feed_bits;
}
#endif

bool bk_wwdt_is_driver_inited()
{
	return s_wwdt_driver_is_init;
}

void bk_wwdt_feed_handle(void)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

#if (CONFIG_INT_WDT)
	bk_int_wdt_feed();
#endif
	bk_wwdt_feed();
	GLOBAL_INT_RESTORE();
}

__attribute__((section(".itcm_sec_code"))) void bk_wwdt_close(void)
{
	uint32_t core_id = wwdt_get_current_core_id();
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	s_wwdt_auto_start_disable_bits |= BIT(core_id);
	s_wwdt.init_bits &= ~BIT(core_id);
	s_last_wwdt_feed_tick[core_id] = 0;
	wwdt_hal_close();
	GLOBAL_INT_RESTORE();
}

void bk_wwdt_force_feed(void)
{
	wwdt_hal_force_feed();
}

void bk_wwdt_force_reboot(void)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	wwdt_hal_force_reboot();
#if CONFIG_GPIO_RETENTION_SUPPORT
	// lock gpio if retention map is set up
	// Attention: hot-flash-write will not work if gpio locked
	if (0 != gpio_retention_map_get())
	{
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
		aon_pmu_drv_gpio_state_lock(false);
#else
		sys_hal_gpio_state_switch(true);
#endif
	}
#endif
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_drv_r0_latch_to_r7b();
#endif
	while(1);
	GLOBAL_INT_RESTORE();
}

uint32_t bk_wwdt_get_cpu_id(void)
{
	return wwdt_hal_get_cpu_id();
}
// eof

