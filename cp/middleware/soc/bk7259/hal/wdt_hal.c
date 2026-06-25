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

#include "wdt_hal.h"
#include "aon_wdt_ll.h"
#include "reset_reason.h"

__attribute__((section(".itcm_sec_code"))) static void wdt_hal_aon_set_period(uint32_t timeout);

bk_err_t wdt_hal_init(wdt_hal_t *hal)
{
	hal->id = AON_WDT_ID;
	return BK_OK;
}

__IRAM_SEC bk_err_t wdt_hal_init_wdt(wdt_hal_t *hal, uint32_t timeout)
{
	(void)hal;
	wdt_hal_aon_set_period(timeout);
	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) void wdt_hal_close(void)
{
	wdt_hal_aon_set_period(0);
}

void wdt_hal_force_feed(void)
{
	/* CONFIG_INT_WDT_PERIOD_MS is only emitted when the INT watchdog is enabled;
	 * fall back to the AON watchdog period otherwise so this builds with INT WDT
	 * disabled. */
#ifdef CONFIG_INT_WDT_PERIOD_MS
	wdt_hal_aon_set_period(CONFIG_INT_WDT_PERIOD_MS);
#else
	wdt_hal_aon_set_period(CONFIG_INT_AON_WDT_PERIOD_MS);
#endif
}

__attribute__((section(".itcm_sec_code"))) void wdt_hal_reset_config_to_default(wdt_hal_t *hal)
{
	(void)hal;
	wdt_hal_close();
}

__attribute__((section(".itcm_sec_code"))) static void wdt_hal_aon_set_period(uint32_t timeout)
{
	uint32_t ctrl_val = aon_wdt_ll_make_ctrl_value(timeout, AON_WDT_V_KEY_1ST);
	REG_WRITE(AON_WDT_R_CTRL, ctrl_val);

	ctrl_val = aon_wdt_ll_make_ctrl_value(timeout, AON_WDT_V_KEY_2ND);
	REG_WRITE(AON_WDT_R_CTRL, ctrl_val);
}

void wdt_hal_force_reboot(void)
{
	wdt_hal_aon_set_period(10);
}
