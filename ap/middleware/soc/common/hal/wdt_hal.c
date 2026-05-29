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
#include "wdt_ll.h"
#include "reset_reason.h"

bk_err_t wdt_hal_init(wdt_hal_t *hal)
{
	hal->id = NMI_WDT_ID;
	hal->hw = (wdt_hw_t *)WDT_LL_REG_BASE(hal->id);
	wdt_ll_init(hal->hw);
	return BK_OK;
}

__IRAM_SEC bk_err_t wdt_hal_init_wdt(wdt_hal_t *hal, uint32_t timeout)
{
	wdt_ll_set_period(hal->hw, timeout);
	return BK_OK;
}

__IRAM_SEC void wdt_hal_close(void)
{
        REG_SET(SOC_WDT_REG_BASE + 4 * 2, 1, 1, 1);
        REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0x5A0000);
        REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0xA50000);
}


__IRAM_SEC void wdt_hal_force_feed(void)
{
        REG_SET(SOC_WDT_REG_BASE + 4 * 2, 1, 1, 1);
        REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0x5AFFF0);
        REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0xA5FFF0);
}

static inline void wdt_hal_nmi_reboot()
{
	reboot_tag_set();

	REG_SET(SOC_WDT_REG_BASE + 4 * 2, 1, 1, 1);
	REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0x5A000A);
	REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0xA5000A);
}

void wdt_hal_force_reboot(void)
{
	wdt_hal_nmi_reboot();
}
