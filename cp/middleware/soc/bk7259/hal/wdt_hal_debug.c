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

#include "hal_config.h"
#include "wdt_hal.h"
#include "aon_wdt_ll.h"

#if CFG_HAL_DEBUG_WDT

void wdt_struct_dump(void)
{
	uint32_t ctrl = REG_READ(AON_WDT_R_CTRL);
	uint32_t period = ((ctrl >> AON_WDT_CONFIG_WD_PERIOD_LOW_POS) & AON_WDT_CONFIG_WD_PERIOD_LOW_MASK) |
		(((ctrl >> AON_WDT_CONFIG_WD_PERIOD_HIGH_POS) & AON_WDT_CONFIG_WD_PERIOD_HIGH_MASK) << 16);
	uint32_t key = (ctrl >> AON_WDT_CONFIG_WD_KEY_POS) & AON_WDT_CONFIG_WD_KEY_MASK;

	SOC_LOGD("system_0xa:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0xa * 4));
	SOC_LOGD("system_0xc:%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0xc * 4));
	SOC_LOGD("system_0x20=0x%x\r\n", REG_READ(SOC_SYSTEM_REG_BASE + 0x20 * 4));
	SOC_LOGD("pmu_0x2=0x%x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + 0x2 * 4));
	SOC_LOGD("aon_wdt_base=%x\r\n", SOC_AON_WDT_REG_BASE);
	SOC_LOGD("  ctrl=0x%x value=0x%x\n", AON_WDT_R_CTRL, ctrl);
	SOC_LOGD("    period: 0x%x\n", period);
	SOC_LOGD("    key: 0x%x\n", key);
}

#endif


