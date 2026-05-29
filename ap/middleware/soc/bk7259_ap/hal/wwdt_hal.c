// Copyright 2020-2024 Beken
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

#include "wwdt_hal.h"
#include "sys_hal.h"

#define WWDT_XTALL_ITUNE_VALUE 4

static void wwdt_hal_enable_32k_clock(void)
{
	uint32_t reg_val;

	/* M55 CPU WWDT is driven by the 32 kHz low-speed clock, so enable both the clock gate and XTALL source. */
	sys_hal_clk_pwr_ctrl(CLK_PWR_ID_32KS, CLK_PWR_CTRL_PWR_UP);

	reg_val = REG_READ(SYS_ANA_REG5_ADDR);
	reg_val &= ~(SYS_ANA_REG5_ITUNE_XTALL_MASK << SYS_ANA_REG5_ITUNE_XTALL_POS);
	reg_val |= ((WWDT_XTALL_ITUNE_VALUE & SYS_ANA_REG5_ITUNE_XTALL_MASK) << SYS_ANA_REG5_ITUNE_XTALL_POS);
	reg_val |= (SYS_ANA_REG5_EN_XTALL_MASK << SYS_ANA_REG5_EN_XTALL_POS);
	REG_WRITE(SYS_ANA_REG5_ADDR, reg_val);
}

bk_err_t wwdt_hal_init(wwdt_hal_t *hal)
{
	hal->id = CPU_WWDT_ID;
	hal->hw = (wwdt_hw_t *)WWDT_LL_REG_BASE;

	wwdt_hal_enable_32k_clock();
	wwdt_hal_set_smb_clkrst_soft_reset(1);

	return BK_OK;
}

bk_err_t wwdt_hal_init_wwdt(wwdt_hal_t *hal, uint32_t timeout_ms)
{
	uint32_t timeout = MULTI_VAL_MS * timeout_ms;

	wwdt_hal_1st_set_wdt_config_period(timeout);
	wwdt_hal_2nd_set_wdt_config_period(timeout);

	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) void wwdt_hal_close(void)
{
	wwdt_hal_1st_set_wdt_config_period(0);
	wwdt_hal_2nd_set_wdt_config_period(0);
}

void wwdt_hal_force_feed(void)
{
	wwdt_hal_1st_set_wdt_config_period(WWDT_F_PERIOD_MAX_V);
	wwdt_hal_2nd_set_wdt_config_period(WWDT_F_PERIOD_MAX_V);
}

void wwdt_hal_force_reboot(void)
{
	wwdt_hal_1st_set_wdt_config_period(WWDT_F_PERIOD_MIN_V);
	wwdt_hal_2nd_set_wdt_config_period(WWDT_F_PERIOD_MIN_V);
}

uint32_t wwdt_hal_get_cpu_id(void)
{
	uint32_t id = CPU_WWDT_INVALID_ID;

	if (wwdt_hal_get_cpuid_magic_word() == WWDT_CPUID_VALID_MAGIC_WORD) {
		id = wwdt_hal_get_cpuid_cpu_id();
	}

	return id;
}
//eof

