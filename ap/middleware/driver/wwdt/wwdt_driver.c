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
#include <os/os.h>
#include <os/mem.h>
#include <driver/wwdt.h>
#include <driver/timer.h>
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

#define WWDT_RETURN_ON_NOT_INIT() do {\
	if (!(s_wwdt.init_bits & BIT(0))) {\
		return BK_ERR_WWDT_NOT_INIT;\
	}\
} while(0)

#define WWDT_RETURN_ON_INVALID_PERIOD(timeout) do {\
	if ((timeout) > WWDT_F_PERIOD_V) {\
		WWDT_LOGE("WWDT invalid timeout\r\n");\
		return BK_ERR_WWDT_INVALID_PERIOD;\
	}\
} while(0)

static wwdt_driver_t s_wwdt = {0};
static bool s_wwdt_driver_is_init = false;
static uint32_t s_wwdt_period = CONFIG_INT_WWDT_PERIOD_MS;

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
	wwdt_hal_init(&s_wwdt.hal);

	/* M55 requires the external 32 kHz crystal clock to be enabled here; this call site should be optimized later. */
	{
		const uint32_t reg = SOC_SYS_REG_BASE + (0x45u << 2);
		uint32_t v = REG_READ(reg);
		v = (v & ~(0xFu << 16)) | (4u << 16);
		v |= (1u << 1);
		REG_WRITE(reg, v);
	}

	// bk_timer_start(TIMER_ID2, WWDT_BARK_TIME_MS, (timer_isr_t)bk_wwdt_feed_handle);
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
	// bk_timer_stop(TIMER_ID2);

	s_wwdt_driver_is_init = false;

	return BK_OK;
}

__IRAM_SEC bk_err_t bk_wwdt_start(uint32_t timeout_ms, bool is_enable_window, uint32_t window_val)
{
	WWDT_RETURN_ON_DRIVER_NOT_INIT();
	WWDT_RETURN_ON_INVALID_PERIOD(timeout_ms);

	if (!timeout_ms) {
		timeout_ms = CONFIG_INT_WWDT_PERIOD_MS;
	}

	s_wwdt_period = timeout_ms;
	if (is_enable_window) {
		wwdt_hal_set_wdt_win_1st_set_win_val(window_val);
		wwdt_hal_set_wdt_win_2nd_set_win_val(window_val);
	}

	wwdt_hal_init_wwdt(&s_wwdt.hal, timeout_ms);

	if (is_enable_window) {
		wwdt_hal_set_wdt_win_1st_set_win_en(1);
		wwdt_hal_set_wdt_win_2nd_set_win_en(1);
	}

	s_wwdt.init_bits |= BIT(0);
	WWDT_LOGV("bk_wwdt_start, s_wwdt.init_bits:%x\r\n", s_wwdt.init_bits);

	return BK_OK;
}

uint32_t bk_wwdt_get_window_val(void)
{
	return wwdt_hal_get_wdt_win_get_win_val();
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_wwdt_stop(void)
{
	WWDT_RETURN_ON_DRIVER_NOT_INIT();
	wwdt_deinit_common();
	s_wwdt.init_bits &= ~(BIT(0));

	return BK_OK;
}

bk_err_t bk_wwdt_feed(void)
{
	WWDT_RETURN_ON_DRIVER_NOT_INIT();
	WWDT_RETURN_ON_NOT_INIT();

	wwdt_hal_init_wwdt(&s_wwdt.hal, s_wwdt_period);

	return BK_OK;
}

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
#if (CONFIG_TASK_WDT)
	bk_task_wdt_timeout_check();
#endif
	bk_wwdt_feed();
	GLOBAL_INT_RESTORE();
}

__attribute__((section(".itcm_sec_code"))) void bk_wwdt_close(void)
{
	wwdt_hal_close();
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
// #if CONFIG_GPIO_RETENTION_SUPPORT
// 	// lock gpio if retention map is set up
// 	// Attention: hot-flash-write will not work if gpio locked
// 	if (0 != gpio_retention_map_get())
// 	{
// #if CONFIG_AON_PMU_REG0_REFACTOR_DEV
// 		aon_pmu_drv_gpio_state_lock(false);
// #else
// 		sys_hal_gpio_state_switch(true);
// #endif
// 	}
// #endif
// #if CONFIG_AON_PMU_REG0_REFACTOR_DEV
// 	aon_pmu_drv_r0_latch_to_r7b();
// #endif
	while(1);
	GLOBAL_INT_RESTORE();
}

uint32_t bk_wwdt_get_cpu_id(void)
{
	return wwdt_hal_get_cpu_id();
}
// eof

