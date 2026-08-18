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
#include "aon_pmu_hal.h"
#include "aon_pmu_ll.h"
#include "system_hw.h"
#include "sys_types.h"
#include "modules/pm.h"

bk_err_t aon_pmu_hal_init(void)
{
	return BK_OK;
}

int aon_pmu_hal_set_sleep_parameters(uint32_t value)
{
	aon_pmu_ll_set_r40(value);
	return 0;
}

void aon_pmu_hal_set_wakeup_source_reg(uint32_t value)
{
	//TODO fix it
	aon_pmu_ll_set_r41(value);
}

uint32_t aon_pmu_hal_get_wakeup_source_reg(void)
{
	return aon_pmu_ll_get_r41();
}
uint32_t aon_pmu_hal_get_chipid(void)
{
	return aon_pmu_ll_get_r7c_id();
}

bk7259_chip_model_e aon_pmu_hal_get_chip_model(void)
{
	switch (aon_pmu_hal_get_chipid()) {
	case BK7259_CHIP_ID_V2_MPW:
		return BK7259_CHIP_MODEL_V2_MPW;
	case BK7259_CHIP_ID_V3A:
		return BK7259_CHIP_MODEL_V3A;
	case BK7259_CHIP_ID_V3B:
		return BK7259_CHIP_MODEL_V3B;
	default:
		return BK7259_CHIP_MODEL_UNKNOWN;
	}
}

/*
 * Read the DPLL unlock latches from R7D (dpll_unlockL bit 22, dpll_unlockH
 * bit 23). Either out-pointer may be NULL. Returns the logical OR of the
 * two latch values (1 = currently out-of-lock, 0 = locked).
 */
uint32_t aon_pmu_hal_get_dpll_unlock(uint32_t *unlockL, uint32_t *unlockH)
{
	uint32_t dpll_unlock_l = aon_pmu_ll_get_r7d_dpll_unlock_l();
	uint32_t dpll_unlock_h = aon_pmu_ll_get_r7d_dpll_unlock_h();

	if (NULL != unlockL)
	{
		*unlockL = dpll_unlock_l;
	}
	if (NULL != unlockH)
	{
		*unlockH = dpll_unlock_h;
	}

	return (dpll_unlock_l | dpll_unlock_h) ? 1 : 0;
}

uint32_t aon_pmu_hal_get_dpll_band(void)
{
    return aon_pmu_ll_get_r7d_dpll_band();
}

/**
 * WAKEUP SOURCE (reg41 wakeup_ena[10:4] / reg71 wakeup_source[26:20])
 * BIT0(0x1): GPIO
 * BIT1(0x2): RTC
 * BIT2(0x4): WIFI
 * BIT3(0x8): BT
 * BIT4(0x10): USBPLUG
 * BIT5(0x20): TOUCHED
 * BIT6(0x40): VAD
 * - write r41 to enable/disable wakeup source before sleep
 * - read r71 to identify the wakeup source after wakeup
*/
__IRAM_SEC void aon_pmu_hal_clear_wakeup_source(wakeup_source_t value)
{
	uint32_t wakeup_source = 0;
	wakeup_source = aon_pmu_ll_get_r41_wakeup_ena();
	wakeup_source &= ~(0x1 << value);
	aon_pmu_ll_set_r41_wakeup_ena(wakeup_source);
}

void aon_pmu_hal_set_wakeup_source(wakeup_source_t value)
{
	uint32_t wakeup_source = 0;
	wakeup_source = aon_pmu_ll_get_r41_wakeup_ena();
	wakeup_source |= (0x1 << value);
	aon_pmu_ll_set_r41_wakeup_ena(wakeup_source);
}

uint32_t aon_pmu_hal_get_wakeup_source(void)
{
	return aon_pmu_ll_get_r71_wakeup_source();
}

void aon_pmu_hal_usbplug_int_en(uint32_t value)
{

}

void aon_pmu_hal_touch_int_en(uint32_t value)
{

}

uint32_t aon_pmu_hal_get_touch_int_status(void)
{
	return aon_pmu_ll_get_r71_touch_state();
}

uint32_t aon_pmu_hal_get_cap_cal(void)
{
	return aon_pmu_ll_get_r73_cap_cal_mode1();
}

uint32_t aon_pmu_hal_get_td_caldone_mode1(void)
{
	return aon_pmu_ll_get_r73_cal_done_mode1();
}

uint32_t aon_pmu_hal_get_touch_state(void)
{
	return aon_pmu_ll_get_r71_touch_state();
}

uint32_t aon_pmu_hal_get_adc_cal()
{
	return aon_pmu_ll_get_r7d_adc_cal();
}

__IRAM_SEC void aon_pmu_hal_reg_set(pmu_reg_e reg, uint32_t value)
{
    pmu_address_map_t pmu_addr_map[] = PMU_ADDRESS_MAP;
    pmu_address_map_t *pmu_addr = &pmu_addr_map[reg];

    uint32_t pmu_reg_addr = pmu_addr->reg_address;

	REG_WRITE(pmu_reg_addr, value);
}
__IRAM_SEC uint32_t aon_pmu_hal_reg_get(pmu_reg_e reg)
{
    pmu_address_map_t pmu_addr_map[] = PMU_ADDRESS_MAP;
    pmu_address_map_t *pmu_addr = &pmu_addr_map[reg];

    uint32_t pmu_reg_addr = pmu_addr->reg_address;

	return REG_READ(pmu_reg_addr);
}


void aon_pmu_hal_wdt_rst_dev_enable()
{
	uint32_t aon_pmu_r2 = 0;
	aon_pmu_r2 = aon_pmu_ll_get_r2();
	aon_pmu_r2 &= ~0x3f;
	aon_pmu_r2 |= 0x26;

	aon_pmu_ll_set_r2(aon_pmu_r2);
}

void aon_pmu_hal_lpo_src_extern32k_enable(void)
{
	//Empty function
}

void aon_pmu_hal_lpo_src_set(uint32_t lpo_src)
{
	volatile uint32_t count = PM_POWER_ON_ROSC_STABILITY_TIME;
	if(lpo_src == PM_LPO_SRC_ROSC)
	{
		if(sys_ll_get_ana_reg5_pwd_rosc_spi() != 0x0)
		{
			sys_ll_set_ana_reg5_pwd_rosc_spi(0x0);//power on rosc
			while(count--)//delay time for stability when power on rosc
			{
			}
		}
	}

	if(aon_pmu_ll_get_r41_lpo_config() != lpo_src)
	{
		aon_pmu_ll_set_r41_lpo_config(lpo_src);
	}

	if(lpo_src == PM_LPO_SRC_ROSC)
	{
		if(aon_pmu_hal_get_chip_model() != BK7259_CHIP_MODEL_V2_MPW)
		{
			if(sys_ll_get_ana_reg5_en_xtall() == 0x1)
			{
				sys_ll_set_ana_reg5_en_xtall(0x0);
			}

			if(sys_ll_get_ana_reg5_itune_xtall() != 0xF)
			{
				sys_ll_set_ana_reg5_itune_xtall(0xF);
			}
		}

		if(sys_ll_get_ana_reg5_rosc_disable() == 0x1)
		{
			sys_ll_set_ana_reg5_rosc_disable(0);
		}
	}
}

__IRAM_SEC uint32_t aon_pmu_hal_lpo_src_get()
{
	return aon_pmu_ll_get_r41_lpo_config();
}

uint32_t aon_pmu_hal_bias_cal_get()
{
	return aon_pmu_ll_get_r7e_cbcal();
}

uint32_t aon_pmu_hal_band_cal_get()
{
	return aon_pmu_ll_get_r7e_bandcal();
}

void aon_pmu_hal_r0_latch_to_r7b(void)
{
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}

#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
uint32_t aon_pmu_hal_get_reset_reason(void)
{
	return aon_pmu_ll_get_r7b();
}

void aon_pmu_hal_set_reset_reason(uint32_t value, bool write_immediately)
{
	if (write_immediately) {
		uint32_t r0 = aon_pmu_ll_get_r0_value();
		uint32_t r7b = aon_pmu_ll_get_r7b_value();
		aon_pmu_ll_set_r0_value(r7b);
		aon_pmu_ll_set_r0_reset_reason(value);
		aon_pmu_hal_r0_latch_to_r7b();
        delay_us(5);    //add delay to make sure ana value is set successfully
		aon_pmu_ll_set_r0_value(r0);
		aon_pmu_ll_set_r0_reset_reason(value);
	} else {
		aon_pmu_ll_set_r0_reset_reason(value);
	}
}

uint32_t aon_pmu_hal_get_gpio_sleep(void)
{
	return aon_pmu_ll_get_r7b_gpio_sleep();
}

void aon_pmu_hal_set_gpio_sleep(uint32_t value, bool write_immediately)
{
	if (write_immediately) {
		uint32_t r0 = aon_pmu_ll_get_r0_value();
		uint32_t r7b = aon_pmu_ll_get_r7b_value();
		aon_pmu_ll_set_r0_value(r7b);
		aon_pmu_ll_set_r0_gpio_sleep(value);
		aon_pmu_hal_r0_latch_to_r7b();
		aon_pmu_ll_set_r0_value(r0);
		aon_pmu_ll_set_r0_gpio_sleep(value);
	} else {
		aon_pmu_ll_set_r0_gpio_sleep(value);
	}
}

uint32_t aon_pmu_hal_get_gpio_retention_bitmap(bool read_history)
{
	if (read_history)
		return aon_pmu_ll_get_r7b_gpio_retention_bitmap();
	else
		return aon_pmu_ll_get_r0_gpio_retention_bitmap();
}

void aon_pmu_hal_set_gpio_retention_bitmap(uint32_t value)
{
	aon_pmu_ll_set_r0_gpio_retention_bitmap(value);
}

uint32_t aon_pmu_hal_get_dlv_startup(void)
{
	return aon_pmu_ll_get_r7b_dlv_startup();
}

__IRAM_SEC uint32_t aon_pmu_hal_get_dlv_startup_iram(void)
{
	return aon_pmu_ll_get_r7b_dlv_startup();
}

__IRAM_SEC void aon_pmu_hal_set_dlv_startup(uint32_t value, bool write_immediately)
{
	if (write_immediately) {
		uint32_t r0 = aon_pmu_ll_get_r0_value();
		uint32_t r7b = aon_pmu_ll_get_r7b_value();
		aon_pmu_ll_set_r0_value(r7b);
		aon_pmu_ll_set_r0_dlv_startup(value);
		aon_pmu_hal_r0_latch_to_r7b();
		aon_pmu_ll_set_r0_value(r0);
		aon_pmu_ll_set_r0_dlv_startup(value);
	} else {
	    aon_pmu_ll_set_r0_dlv_startup(value);
	}
}
#else
uint32_t aon_pmu_hal_get_reset_reason(void)
{
	return aon_pmu_ll_get_r7b();
}

void aon_pmu_hal_set_reset_reason(uint32_t value)
{
	aon_pmu_ll_set_r0_reset_reason(value);
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}

uint32_t aon_pmu_hal_gpio_retention_bitmap_get()
{
	return aon_pmu_ll_get_r0_gpio_retention_bitmap();
}

void aon_pmu_hal_gpio_retention_bitmap_set(uint32_t bitmap)
{
	aon_pmu_ll_set_r0_gpio_retention_bitmap(bitmap);
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}

uint32_t aon_pmu_hal_get_dlv_startup(void)
{
	return aon_pmu_ll_get_r0_dlv_startup();
}

__IRAM_SEC uint32_t aon_pmu_hal_get_dlv_startup_iram(void)
{
	return aon_pmu_ll_get_r0_dlv_startup();
}

__IRAM_SEC void aon_pmu_hal_set_dlv_startup(uint32_t value)
{
	aon_pmu_ll_set_r0_dlv_startup(value);
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}
#endif

void aon_pmu_hal_gpio_clksel_set(uint32_t value)
{
	aon_pmu_ll_set_r41_gpio_int_clksel(value);
}

uint32_t aon_pmu_hal_gpio_clksel_get(void)
{
	return aon_pmu_ll_get_r41_gpio_int_clksel();
}

void aon_pmu_hal_set_wlp_power_down(uint32_t v)
{
	aon_pmu_ll_set_r42_pwd_wlppwd(v);
}

void aon_pmu_hal_set_r0(uint32_t value)
{
	aon_pmu_ll_set_r0(value);
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}

uint32_t aon_pmu_hal_get_r0(void)
{
	return aon_pmu_ll_get_r0();
}
static uint32_t s_pmu_saved_regs[3] = {0};

__IRAM_SEC void aon_pmu_hal_backup(void)
{
	s_pmu_saved_regs[0] = aon_pmu_ll_get_r40();
	s_pmu_saved_regs[1] = aon_pmu_ll_get_r41();
	s_pmu_saved_regs[2] = aon_pmu_ll_get_r2();
}

__IRAM_SEC void aon_pmu_hal_restore(void)
{
	uint32_t reg = aon_pmu_ll_get_r7b();
	aon_pmu_ll_set_r0(reg);
	// aon_pmu_ll_set_r40(s_pmu_saved_regs[0]);
	aon_pmu_ll_set_r41(s_pmu_saved_regs[1]);
	aon_pmu_ll_set_r2(s_pmu_saved_regs[2]);
}