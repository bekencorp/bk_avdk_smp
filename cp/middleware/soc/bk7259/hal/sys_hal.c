// Copyright 2020-2025 Beken
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
#include "sys_hal.h"
#include "sys_ll.h"
#include "sys_ahbp_ll.h"
#include "sys_aonp_struct.h"
#include "aon_pmu_hal.h"
#include "aon_pmu_ll.h"
#include "gpio_hal_v2px.h"
#include "gpio_driver_base.h"
#include "sys_types.h"
#include <driver/int.h>
#include <driver/aon_rtc.h>
#include <driver/hal/hal_spi_types.h>
#include <gpio_driver.h>
#include "timer_hal.h"
#include <driver/pwr_clk.h>
#include "bk_misc.h"
#include "cpu_id.h"
#include <driver/psram_types.h>
#include <driver/xdac_types.h>
#include <modules/pm.h>

#define PM_SYS_REG_0x8                      (SOC_SYS_REG_BASE + (0x8 << 2))
#define PM_CLKSEL_FLASH_480M                (0x1)
#define PM_CLKDIV_CORE_0                    (0)
#define PM_CLKDIV_CORE_1                    (1)
#define PM_CLKSEL_CORE_POS                  (0)
#define PM_CLKSEL_CORE_MASK                 (0x3 << PM_CLKSEL_CORE_POS)
#define PM_CLKDIV_CORE_POS                  (2)
#define PM_CLKDIV_CORE_MASK                 (0xF << PM_CLKDIV_CORE_POS)
#define PM_VDDD_H_VOL_1V                    (0x6)
#define PM_VDDDIG_H_VOL_0V825               (0x9)
#define PM_VDDDIG_H_VOL_0V85                (0xA)
#define PM_VDDDIG_H_VOL_0v9                 (0xC)
#define PM_VDDDIG_H_VOL_0V95                (0xE)
#define PM_CLKDV_CPU1_1                     (0x1)
#define PM_CLKDV_CPU0_0                     (0x0)
#define SYS_SWITCH_VDDDIG_VOL_DELAY_TIME    (2600)


#define PM_CLOCK_MODE_0_POS                 (0)
#define PM_CLOCK_MODE_1_POS                 (32)
#define PM_CLOCK_MODE_2_POS                 (64)

#define SYS_HAL_SWD_CORESIGHT_VALID         (1)
#define SYS_HAL_DCO_UNLOCK_H_BIT            (25)
#define SYS_HAL_DCO_UNLOCK_L_BIT            (26)
#define SYS_HAL_DCO_BAND_MAX                (0x3F)

#define SYS_PM_HAL_CPU_BARRIER()              do {      \
	__asm__ volatile ("dsb");                           \
	__asm__ volatile ("isb");                           \
} while (0)

/* [EXPERIMENT] Force all low-power code out of .iram (0x2C non-cacheable SRAM)
 * into Flash, to verify whether the wakeup/clock-switch crash is related to
 * fetching from the 0x2C000000 SRAM alias. Set to 0 to restore original.
 * NOTE: literal __attribute__((section(".iram"))) in this file have been
 * normalized to __IRAM_SEC so this single switch controls them. */
#ifndef PM_EXP_ALL_TO_FLASH
#define PM_EXP_ALL_TO_FLASH 0
#endif
#if PM_EXP_ALL_TO_FLASH
#undef  __IRAM_SEC
#define __IRAM_SEC
#endif

static sys_hal_t s_sys_hal;
static uint32_t s_pm_wireless_clock_state = 0;

typedef struct {
	pm_cpu_freq_e freq;
	uint32_t cksel_core;
	uint32_t ckdiv_core;
	uint32_t ckdiv_bus;
	uint32_t ckdiv_cpu0;
	uint32_t ckdiv_cpu1;
	uint32_t vdddig_vol;
} sys_hal_cpu_bus_freq_cfg_t;


static const sys_hal_cpu_bus_freq_cfg_t s_cpu_bus_freq_cfg[] = {
	{PM_CPU_FRQ_XTAL, PM_CLKSEL_CORE_26M, 0x0, 0x0, 0x0, 0x0, PM_VDDDIG_H_VOL_0V85},
	{PM_CPU_FRQ_60M, PM_CLKSEL_CORE_480M, 0x7, 0x0, 0x0, 0x0, PM_VDDDIG_H_VOL_0V85},
	{PM_CPU_FRQ_80M, PM_CLKSEL_CORE_480M, 0x5, 0x0, 0x0, 0x0, PM_VDDDIG_H_VOL_0V85},
	{PM_CPU_FRQ_120M, PM_CLKSEL_CORE_480M, 0x3, 0x0, 0x0, 0x0, PM_VDDDIG_H_VOL_0V85},
	{PM_CPU_FRQ_160M, PM_CLKSEL_CORE_480M, 0x2, 0x0, 0x0, 0x0, PM_VDDDIG_H_VOL_0v9},
	{PM_CPU_FRQ_240M, PM_CLKSEL_CORE_480M, 0x1, 0x0, 0x0, 0x0, PM_VDDDIG_H_VOL_0V95},
};

// static pm_cpu_freq_e s_pre_cpu_freq = PM_CPU_FRQ_120M;
uint32 sys_hal_get_int_group2_status(uint32_t core_id);
bk_err_t sys_hal_ctrl_vddd_h_vol(uint32_t vol_value);
bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value);
uint32_t sys_hal_vdddig_h_vol_get();
static void sys_hal_delay(volatile uint32_t times);
static bk_err_t sys_hal_m55_clock_power_init();
static bk_err_t sys_hal_m55_clock_power_init();
__IRAM_SEC int32 sys_hal_module_power_state_get(power_module_name_t module);
bk_err_t sys_hal_ap_clock_power_ctrl(power_module_state_t power_state);

bk_err_t sys_hal_init()
{
	s_sys_hal.hw = (sys_hw_t *)SOC_SYS_REG_BASE;
	//bk_int_isr_register(INT_SRC_PLL_UNLOCK, sys_hal_adjust_dpll, NULL);
    // sys_hal_int_group2_enable(CPU0_CORE_ID, DPLL_UNLOCK_INTERRUPT_CTRL_BIT);
	// bk_int_isr_register(INT_SRC_DCO_UNLOCK, sys_hal_adjust_dco, NULL);
	// sys_hal_int_group2_enable(CPU0_CORE_ID, DCO_UNLOCK_INTERRUPT_CTRL_BIT);
	return BK_OK;
}

void sys_hal_enable_swd(void)
{
	sys_ll_set_reserver_reg0x3b_coresight_valid(SYS_HAL_SWD_CORESIGHT_VALID);
}

void sys_hal_usb_enable_clk(bool en)
{
	return;
}

void sys_hal_usb_analog_phy_en(bool en)
{
	return;
}

void sys_hal_set_ana_reg_spi_latch1v(uint32_t v)
{
	sys_ll_set_ana_reg10_spi_latch1v(v);
	return;
}

void sys_hal_set_ioldo_bypass(uint32_t v)
{
	return;
}

void sys_hal_set_ioldo_volt(uint32_t v)
{
	return;
}

void sys_hal_flash_set_dco(void)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(FLASH_CLK_DPLL);
}

void sys_hal_flash_set_dpll(void)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(FLASH_CLK_APLL);
}

void sys_hal_flash_set_clk(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(value);
}

void sys_hal_flash_set_clk_div(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(value);
}

uint32_t sys_hal_flash_get_clk_sel(void)
{
	return sys_ll_get_cpu_clk_div_mode1_cksel_flash();
}

uint32_t sys_hal_flash_get_clk_div(void)
{
	return sys_ll_get_cpu_clk_div_mode1_ckdiv_flash();
}
bk_err_t sys_hal_phy_ctrl(power_module_name_t module,power_module_state_t power_state)
{
	bk_err_t ret = BK_OK;
	if ((0x0 == sys_hal_module_power_state_get(module))&& (bk_pm_phy_cali_state_get() == true))
	{
		//BK_LOGD(NULL, "phy power on already\r\n");
	}
	else
	{
		if (sys_hal_module_power_state_get(module))
		{
			sys_ll_set_reserver_reg0x10_pwd_wrls(0);
		}

		if (0x0 == sys_hal_module_power_state_get(module))
		{
			if((bk_pm_vote_power_module_get() == PM_POWER_SUB_DOMAIN_PHY) || 
			(bk_pm_vote_power_module_get() == PM_POWER_SUB_DOMAIN_BTDM))
			{
				#if CONFIG_WIFI_ENABLE
				extern void phy_wakeup_reinit(uint8 is_wifi);
				phy_wakeup_reinit(bk_pm_vote_power_module_get() == PM_POWER_SUB_DOMAIN_PHY);
				#elif CONFIG_BLUETOOTH
				extern void phy_wakeup_for_bluetooth();
				phy_wakeup_for_bluetooth();
				#else
				#endif
				bk_pm_phy_reinit_flag_set(true);
				bk_pm_phy_cali_state_set(true);
			}

		}
		else
		{
			//BK_LOGD(NULL, "phy power on fail 0x%x\r\n", sys_hal_module_power_state_get(module));
			ret = BK_FAIL;
		}
	}
	return ret;
}
__IRAM_SEC void sys_hal_module_power_ctrl(power_module_name_t module,power_module_state_t power_state)
{
	if(POWER_MODULE_NAME_BASE1 < module && module <= POWER_MODULE_NAME_NONE)
	{
		switch(module)
		{
			case PM_POWER_DOMAIN_0://POWER_DOMAIN_NAME_CP_CPU1_BAKP_AUDP:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ll_set_reserver_reg0x10_pwd_cpu1(0);
				}
				else
				{
					sys_ll_set_reserver_reg0x10_pwd_cpu1(1);
				}
				break;
			case PM_POWER_DOMAIN_1://POWER_DOMAIN_NAME_VEHP_SPI_DEBUG:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ll_set_reserver_reg0x10_pwd_vehp(0);
				}
				else
				{
					sys_ll_set_reserver_reg0x10_pwd_vehp(1);
				}
				break;
			case PM_POWER_DOMAIN_2://POWER_DOMAIN_NAME_WRLP_ENCP:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_hal_phy_ctrl(module, power_state);
					if(sys_ll_get_reserver_reg0x10_pwd_wrls() == 1)
					{
						sys_ll_set_reserver_reg0x10_pwd_wrls(0);
					}
				}
				else
				{
					bk_pm_phy_cali_state_set(false);
					sys_ll_set_reserver_reg0x10_pwd_wrls(1);
				}
				break;
			case PM_POWER_DOMAIN_3://POWER_DOMAIN_NAME_AP_CPU:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_hal_ap_clock_power_ctrl(power_state);
					//sys_hal_m55_clock_power_init();
				}
				else
				{
					sys_hal_ap_clock_power_ctrl(power_state);
				}
				break;
			case PM_POWER_DOMAIN_4://POWER_DOMAIN_NAME_HSSUB_POWER:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_hal_m55_clock_power_init();
				}
				else
				{

				}
				break;
			default:
				break;
		}
	}
}

__IRAM_SEC int32 sys_hal_module_power_state_get(power_module_name_t module)
{
	if(POWER_MODULE_NAME_BASE1 < module && module <= POWER_MODULE_NAME_NONE)
	{
		switch(module)
		{
			case PM_POWER_DOMAIN_0://POWER_DOMAIN_NAME_CP_CPU1_BAKP_AUDP:
				return sys_ll_get_reserver_reg0x10_pwd_cpu1();
				break;
			case PM_POWER_DOMAIN_1://POWER_DOMAIN_NAME_VEHP_SPI_DEBUG:
				return sys_ll_get_reserver_reg0x10_pwd_vehp();
				break;
			case PM_POWER_DOMAIN_2://POWER_DOMAIN_NAME_WRLP_ENCP:
				return sys_ll_get_reserver_reg0x10_pwd_wrls();
				break;
			case PM_POWER_DOMAIN_3://POWER_DOMAIN_NAME_AP_CPU:
				return sys_ahbp_ll_get_rege_pwd_m55();
				break;
			case PM_POWER_DOMAIN_4://POWER_DOMAIN_NAME_HSSUB_POWER:
				return 0;//to do it
				break;
			default:
				break;
		}
	}
	return 0;
}

int sys_hal_rosc_calibration(uint32_t rosc_cali_mode, uint32_t cali_interval)
{
	sys_ll_set_ana_reg5_spilatchb_rc32k(1);
	// sys_ll_set_ana_reg6_manu_ena(0);
	// sys_ll_set_ana_reg6_modifi_auto(0);
	// sys_ll_set_ana_reg6_calib_auto(0);
	// sys_ll_set_ana_reg6_spi_trig(0);

	if (rosc_cali_mode == 0) { //Auto
		sys_ll_set_ana_reg6_cal_mode(1);
		sys_ll_set_ana_reg6_calib_auto(1);
		sys_ll_set_ana_reg6_xtal_wakeup_time(8);
		sys_ll_set_ana_reg6_calib_interval(cali_interval);
		sys_ll_set_ana_reg6_spi_trig(1);
	} else if (rosc_cali_mode == 1) { //Manual
		sys_ll_set_ana_reg6_manu_ena(1);
		sys_ll_set_ana_reg6_manu_cin(cali_interval);
		sys_ll_set_ana_reg6_calib_interval(cali_interval >> 16);
	} else if (rosc_cali_mode == 2) { //Modify
		sys_ll_set_ana_reg6_cal_mode(1);
		sys_ll_set_ana_reg6_modifi_auto(1);
		sys_ll_set_ana_reg6_xtal_wakeup_time(8);
		sys_ll_set_ana_reg6_modify_interval(cali_interval);
		sys_ll_set_ana_reg6_spi_trig(1);
	} else if (rosc_cali_mode == 3) { //Trigger
		sys_ll_set_ana_reg6_cal_mode(1);
		sys_ll_set_ana_reg6_spi_trig(1);
	} else { //Disable
	}

	return BK_OK;
}

int sys_hal_rosc_test_mode(bool enabled)
{
	if (enabled) {
		//TODO:sys_ll_set_ana_reg4_ck_tst_enbale(1);
		//TODO:sys_ll_set_ana_reg4_cktst_sel(0);
		//TODO:sys_ll_set_ana_reg5_rosc_tsten(1);
		//TODO:sys_ll_set_ana_reg6_cal_mode(1);
		REG_WRITE((SOC_AON_GPIO_REG_BASE + (24 << 2)), 0x40);
	} else {
		//TODO:sys_ll_set_ana_reg4_ck_tst_enbale(0);
		//TODO:sys_ll_set_ana_reg4_cktst_sel(0);
		//TODO:sys_ll_set_ana_reg5_rosc_tsten(0);
		//TODO:sys_ll_set_ana_reg6_cal_mode(0);
		//TODO:sys_ll_set_ana_reg6_calib_auto(0);
	}
	return BK_OK;
}

void sys_hal_module_RF_power_ctrl (module_name_t module,power_module_state_t power_state)
{
        uint32_t value = 0;
        //TODO:value = sys_ana_ll_get_reg6_value();
        if(power_state == POWER_MODULE_STATE_ON)//power on
        {
        }
        else //power down
        {
                value &= ~(1 << 12);//en_dpll
                value &= ~(1 << 11);//en_audpll
                value &= ~(1 << 8);//en_dco
                value &= ~(1 << 7);//en_xtall
        }
        (void)value;
        //TODO:sys_ll_set_ana_reg6_value(value);
}

void sys_hal_cpu0_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state)
{
    sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask( clock_state);
}

void sys_hal_cpu1_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state)
{
    sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask( clock_state);
}

void sys_hal_set_cpu1_boot_address_offset(uint32_t address_offset)
{
	sys_ll_set_cpu1_int_halt_clk_op_cpu1_offset(address_offset);
}

void sys_hal_set_cpu2_boot_address_offset(uint32_t address_offset)
{
	sys_ahbp_ll_set_reg4_cpu0_offset(address_offset);
}

void sys_hal_set_cpu1_reset(uint32_t reset_value)
{
	sys_ll_set_cpu1_int_halt_clk_op_cpu1_sw_rst(reset_value);
}

void sys_hal_set_cpu2_reset(uint32_t reset_value)
{
	sys_ahbp_ll_set_reg4_cpu0_sw_rstn(reset_value);
}

void sys_hal_set_cpu1_pwr_dw(uint32_t is_pwr_down)
{
	return;
}

void sys_hal_set_cpu2_pwr_dw(uint32_t is_pwr_down)
{
	return;
}

void sys_hal_all_modules_clk_div_set(clk_div_reg_e reg, uint32_t value)
{
        clk_div_address_map_t clk_div_address_map_table[] = CLK_DIV_ADDRESS_MAP;
        clk_div_address_map_t *clk_div_addr = &clk_div_address_map_table[reg];

        uint32_t clk_div_reg_address = clk_div_addr->reg_address;

        REG_WRITE(clk_div_reg_address, value);
}

uint32_t sys_hal_all_modules_clk_div_get(clk_div_reg_e reg)
{
        clk_div_address_map_t clk_div_address_map_table[] = CLK_DIV_ADDRESS_MAP;
        clk_div_address_map_t *clk_div_addr = &clk_div_address_map_table[reg];

        uint32_t clk_div_reg_address = clk_div_addr->reg_address;

        return REG_READ(clk_div_reg_address);
}
void sys_hal_cpu_clk_div_set(uint32_t core_index, uint32_t value)
{
	return;
}
uint32_t sys_hal_cpu_clk_div_get(uint32_t core_index)
{
	return 0;
}
void sys_hal_set_iobyapssen(uint32_t enable)
{
	return;
}
void sys_hal_set_violdosel(uint32_t flag)
{
	return;
}
void sys_hal_lp_anabuf_set(uint32_t v) {
	return;
}
int32 sys_hal_lp_vol_set(uint32_t value)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg10_vcorelsel(value);
	sys_ll_set_ana_reg10_spi_latch1v(0);
	return BK_OK;
}
uint32_t sys_hal_lp_vol_get()
{
	return sys_ll_get_ana_reg10_vcorelsel();
}
int32 sys_hal_rf_tx_vol_set(uint32_t value)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_t_vanaldosel(value);
	sys_ll_set_ana_reg10_spi_latch1v(0);
	return BK_OK;
}

uint32_t sys_hal_rf_tx_vol_get()
{
	return sys_ll_get_ana_reg9_t_vanaldosel();
}
int32 sys_hal_rf_rx_vol_set(uint32_t value)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_r_vanaldosel(value);
	sys_ll_set_ana_reg10_spi_latch1v(0);
	return BK_OK;
}

uint32_t sys_hal_rf_rx_vol_get()
{
	return sys_ll_get_ana_reg9_r_vanaldosel();
}

int32 sys_hal_bandgap_cali_set(uint32_t value)//increase or decrease the dvdddig voltage
{
	return 0;
}

uint32_t sys_hal_bandgap_cali_get()
{
	return 0;
}
bk_err_t sys_hal_core_bus_clock_ctrl(uint32_t cksel_core, uint32_t ckdiv_core,uint32_t ckdiv_bus, uint32_t ckdiv_cpu0,uint32_t ckdiv_cpu1)
{
	uint32_t clk_param;
	uint32_t next_clk_param;
	uint32_t target_cksel_core = cksel_core << PM_CLKSEL_CORE_POS;
	uint32_t target_ckdiv_core = ckdiv_core << PM_CLKDIV_CORE_POS;
	if(cksel_core > PM_CLKSEL_CORE_MAX)
	{
		os_printf("Set dvfs cksel core > %d invalid\r\n",PM_CLKSEL_CORE_MAX);
		return BK_FAIL;
	}

	if((ckdiv_core > PM_FREQUNCY_DIV_MAX))
	{
		os_printf("Set dvfs ckdiv_core > %d invalid\r\n",PM_FREQUNCY_DIV_MAX);
		return BK_FAIL;
	}
	if(((cksel_core == PM_CLKSEL_CORE_320M)&&(ckdiv_core == PM_CLKDIV_CORE_0))||((cksel_core == PM_CLKSEL_CORE_480M)&&(ckdiv_core == PM_CLKDIV_CORE_0)))
	{
		os_printf("unsupport the cpu freq setting %d %d \r\n",cksel_core,ckdiv_core);
		return BK_FAIL;
	}

	clk_param = sys_ll_get_cpu_clk_div_mode1_value();
	if(((clk_param & PM_CLKSEL_CORE_MASK) >> PM_CLKSEL_CORE_POS) > cksel_core)//when it from the higher frequency to lower frequency
	{
		/*1.core clk select*/
		next_clk_param = (clk_param & ~PM_CLKSEL_CORE_MASK) | target_cksel_core;
		if(next_clk_param != clk_param)
		{
			sys_ll_set_cpu_clk_div_mode1_value(next_clk_param);
			clk_param = next_clk_param;
		}

		/*2.config bus and core clk div*/
		next_clk_param = (clk_param & ~PM_CLKDIV_CORE_MASK) | target_ckdiv_core;
		if(next_clk_param != clk_param)
		{
			sys_ll_set_cpu_clk_div_mode1_value(next_clk_param);
		}
	}
	else//when it from the lower frequency to higher frequency
	{
		/*1.config bus and core clk div*/
		next_clk_param = (clk_param & ~PM_CLKDIV_CORE_MASK) | target_ckdiv_core;
		if(next_clk_param != clk_param)
		{
			sys_ll_set_cpu_clk_div_mode1_value(next_clk_param);
			clk_param = next_clk_param;
		}

		/*2.core clk select*/
		next_clk_param = (clk_param & ~PM_CLKSEL_CORE_MASK) | target_cksel_core;
		if(next_clk_param != clk_param)
		{
			sys_ll_set_cpu_clk_div_mode1_value(next_clk_param);
		}
	}

	return BK_OK;
}

bk_err_t sys_hal_ctrl_vddd_h_vol(uint32_t vol_value)
{
	/*TODO: implement the function; default:1.0v*/
	return BK_OK;
}

__IRAM_SEC bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value)
{
	uint32_t cur_vol = sys_ll_get_ana_reg10_vcorehsel();
	uint32_t next_vol;

	if(cur_vol == vol_value)
	{
		return BK_OK;
	}

	if(cur_vol > vol_value)
	{
		sys_ll_set_ana_reg10_spi_latch1v(1);
		sys_ll_set_ana_reg10_vcorehsel(vol_value);
		sys_ll_set_ana_reg10_spi_latch1v(0);
		return BK_OK;
	}

	for(next_vol = cur_vol + 1; next_vol <= vol_value; next_vol++)
	{
		sys_ll_set_ana_reg10_spi_latch1v(1);
		sys_ll_set_ana_reg10_vcorehsel(next_vol);
		sys_ll_set_ana_reg10_spi_latch1v(0);
		//sys_hal_delay(SYS_SWITCH_VDDDIG_VOL_DELAY_TIME);//delay 10uS for each VDDDIG step-up
		bk_delay_us(10);
	}

	return BK_OK;
}

uint32_t sys_hal_vdddig_h_vol_get()
{
	return sys_ll_get_ana_reg10_vcorehsel();
}

static const sys_hal_cpu_bus_freq_cfg_t *sys_hal_get_cpu_bus_freq_cfg(pm_cpu_freq_e cpu_bus_freq)
{
	if((cpu_bus_freq < PM_CPU_FRQ_XTAL) || (cpu_bus_freq > PM_CPU_FRQ_240M))
		return NULL;

	return &s_cpu_bus_freq_cfg[cpu_bus_freq];
}

static bk_err_t sys_hal_set_cpu_bus_freq_clock(const sys_hal_cpu_bus_freq_cfg_t *cfg)
{
#if CONFIG_DCO_CLK_ENABLE
	if(cfg->freq == PM_CPU_FRQ_60M)
	{
		return sys_hal_core_bus_clock_ctrl(PM_CLKSEL_CORE_DCO, 0x3, cfg->ckdiv_bus,
			cfg->ckdiv_cpu0, cfg->ckdiv_cpu1);
	}
	else if(cfg->freq == PM_CPU_FRQ_80M)
	{
		return sys_hal_core_bus_clock_ctrl(PM_CLKSEL_CORE_DCO, 0x2, cfg->ckdiv_bus,
			cfg->ckdiv_cpu0, cfg->ckdiv_cpu1);
	}
	else if(cfg->freq == PM_CPU_FRQ_120M)
	{
		return sys_hal_core_bus_clock_ctrl(PM_CLKSEL_CORE_DCO, 0x1, cfg->ckdiv_bus,
			cfg->ckdiv_cpu0, cfg->ckdiv_cpu1);
	}
#endif

	return sys_hal_core_bus_clock_ctrl(cfg->cksel_core, cfg->ckdiv_core, cfg->ckdiv_bus,
		cfg->ckdiv_cpu0, cfg->ckdiv_cpu1);
}

bk_err_t sys_hal_switch_cpu_bus_freq_high_to_low(pm_cpu_freq_e cpu_bus_freq)
{
	const sys_hal_cpu_bus_freq_cfg_t *cfg = sys_hal_get_cpu_bus_freq_cfg(cpu_bus_freq);
	bk_err_t ret;

	if(cfg == NULL)
		return BK_FAIL;

	ret = sys_hal_set_cpu_bus_freq_clock(cfg);
	SYS_PM_HAL_CPU_BARRIER();

	if(ret != BK_OK)
		return ret;

	if(cpu_bus_freq < PM_CPU_FRQ_160M)
	{
		sys_hal_set_ram_low_speed();
	}
	else
	{
		sys_hal_set_ram_high_speed();
	}
	SYS_PM_HAL_CPU_BARRIER();

	sys_hal_ctrl_vddd_h_vol(PM_VDDD_H_VOL_1V);
	sys_hal_ctrl_vdddig_h_vol(cfg->vdddig_vol);
	SYS_PM_HAL_CPU_BARRIER();

	return ret;
}
bk_err_t sys_hal_switch_cpu_bus_freq_low_to_high(pm_cpu_freq_e cpu_bus_freq)
{
	const sys_hal_cpu_bus_freq_cfg_t *cfg = sys_hal_get_cpu_bus_freq_cfg(cpu_bus_freq);
	bk_err_t ret;

	if(cfg == NULL)
		return BK_FAIL;

	sys_hal_ctrl_vddd_h_vol(PM_VDDD_H_VOL_1V);
	sys_hal_ctrl_vdddig_h_vol(cfg->vdddig_vol);

	SYS_PM_HAL_CPU_BARRIER();

	if(cpu_bus_freq < PM_CPU_FRQ_160M)
	{
		sys_hal_set_ram_low_speed();
	}
	else
	{
		sys_hal_set_ram_high_speed();
	}
	SYS_PM_HAL_CPU_BARRIER();

	ret = sys_hal_set_cpu_bus_freq_clock(cfg);
	SYS_PM_HAL_CPU_BARRIER();

	return ret;
}

static pm_cpu_freq_e s_pre_cpu_freq = PM_CPU_FRQ_120M;
bk_err_t sys_hal_switch_cpu_bus_freq(pm_cpu_freq_e cpu_bus_freq)
{
	bk_err_t ret = BK_OK;

	pm_cpu_freq_e prev_freq = s_pre_cpu_freq;

	if(prev_freq == cpu_bus_freq)
		return BK_OK;

	if(prev_freq > cpu_bus_freq)// eg: 240->60
	{
		ret = sys_hal_switch_cpu_bus_freq_high_to_low(cpu_bus_freq);
	}
	else // eg: 60-240
	{
		ret = sys_hal_switch_cpu_bus_freq_low_to_high(cpu_bus_freq);
	}
	if(ret == BK_OK)
	{
		s_pre_cpu_freq = cpu_bus_freq;
	}
	return ret;
}

/*for low power function end*/
/*sleep feature end*/

uint32 sys_hal_get_chip_id(void)
{
	return sys_ll_get_version_id_versionid();
}

uint32 sys_hal_get_device_id(void)
{
	return sys_ll_get_device_id_deviceid();
}

__IRAM_SEC void sys_hal_set_ram_sph_cfg(uint32_t value)
{
	sys_ll_set_reserver_reg0x1e_value(value);
}

__IRAM_SEC void sys_hal_set_ram_tph_cfg(uint32_t value)
{
	sys_ll_set_reserver_reg0x1f_value(value);
}

__IRAM_SEC void sys_hal_set_ram_spl_cfg(uint32_t value)
{
	sys_ll_set_reserver_reg0x2e_value(value);
}

__IRAM_SEC void sys_hal_set_ram_tpl_cfg(uint32_t value)
{
	sys_ll_set_reserver_reg0x2f_value(value);
}


__IRAM_SEC void sys_hal_set_ram_high_speed(void)
{

	/* When running at high frequency (more than half of the maximum frequency),
	 * RAM CFG parameters need to be modified. Please modify the configuration
	 * in the initialization program. This RAM parameter affects RAM speed and
	 * yield. RAM library recommends using high-speed configuration for
	 * high-speed operation and low-speed configuration for low-speed operation.
	 */
    //  uint32_t ram_sph_cfg = 0x0;//Default: 0x442 << 10 | 0x242;
    //  ram_sph_cfg = 0x441 << 10 | 0x241;
    //  sys_hal_set_ram_sph_cfg(0x5A << 24 | ram_sph_cfg); ///Default: Low-speed configuration
    //  sys_hal_set_ram_sph_cfg(0xA5 << 24 | ram_sph_cfg); ///Default: Low-speed configuration

    //  uint32_t ram_tph_cfg = 0x0;//Default: 0x902;
    //  ram_tph_cfg = 0x901;
    //  sys_hal_set_ram_tph_cfg(0x5A << 24 | ram_tph_cfg); ///Default: Low-speed configuration
    //  sys_hal_set_ram_tph_cfg(0xA5 << 24 | ram_tph_cfg); ///Default: Low-speed configuration

	 uint32_t ram_spl_cfg = 0x0;//Default: 0x444 << 10 | 0x244;
     ram_spl_cfg = 0x443 << 10 | 0x243;
     sys_hal_set_ram_spl_cfg(0x5A << 24 | ram_spl_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_spl_cfg(0xA5 << 24 | ram_spl_cfg); ///Default: Low-speed configuration

     uint32_t ram_tpl_cfg = 0x0;//Default: 0x904;
     ram_tpl_cfg = 0x903;
     sys_hal_set_ram_tpl_cfg(0x5A << 24 | ram_tpl_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_tpl_cfg(0xA5 << 24 | ram_tpl_cfg); ///Default: Low-speed configuration
}

void sys_hal_set_ram_low_speed(void)
{

	/* When running at high frequency (more than half of the maximum frequency),
	 * RAM CFG parameters need to be modified. Please modify the configuration
	 * in the initialization program. This RAM parameter affects RAM speed and
	 * yield. RAM library recommends using high-speed configuration for
	 * high-speed operation and low-speed configuration for low-speed operation.
	 */
    //  uint32_t ram_sph_cfg = 0x0;//Default: 0x442 << 10 | 0x242;
    //  ram_sph_cfg = 0x442 << 10 | 0x242;
    //  sys_hal_set_ram_sph_cfg(0x5A << 24 | ram_sph_cfg);
    //  sys_hal_set_ram_sph_cfg(0xA5 << 24 | ram_sph_cfg);

    //  uint32_t ram_tph_cfg = 0x0;//Default: 0x902;
    //  ram_tph_cfg = 0x902;
    //  sys_hal_set_ram_tph_cfg(0x5A << 24 | ram_tph_cfg);
    //  sys_hal_set_ram_tph_cfg(0xA5 << 24 | ram_tph_cfg);

	 uint32_t ram_spl_cfg = 0x0;//Default: 0x444 << 10 | 0x244;
     ram_spl_cfg = 0x443 << 10 | 0x243;
     sys_hal_set_ram_spl_cfg(0x5A << 24 | ram_spl_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_spl_cfg(0xA5 << 24 | ram_spl_cfg); ///Default: Low-speed configuration

     uint32_t ram_tpl_cfg = 0x0;//Default: 0x904;
     ram_tpl_cfg = 0x903;
     sys_hal_set_ram_tpl_cfg(0x5A << 24 | ram_tpl_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_tpl_cfg(0xA5 << 24 | ram_tpl_cfg); ///Default: Low-speed configuration
}

__IRAM_SEC int32_t sys_hal_set_int_en(uint32_t core_id, uint32_t int_num, uint32_t int_en)
{
	uint32_t reg = 0;
	uint32_t value = 0;

	switch (core_id) {
	case 0:
		reg = REG_READ(SYS_CPU0_INT_0_31_EN_ADDR + ((int_num / 32) << 2));
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		REG_WRITE(SYS_CPU0_INT_0_31_EN_ADDR + ((int_num / 32) << 2), reg);
		break;
	case 1:
		reg = REG_READ(SYS_CPU1_INT_0_31_EN_ADDR + ((int_num / 32) << 2));
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		REG_WRITE(SYS_CPU1_INT_0_31_EN_ADDR + ((int_num / 32) << 2), reg);
		break;
	case 2:
		reg = REG_READ(SYS_AHBP_REG10_ADDR + ((int_num / 32) << 2));
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		REG_WRITE(SYS_AHBP_REG10_ADDR + ((int_num / 32) << 2), reg);
		break;
	case 3:
		reg = REG_READ(SYS_AHBP_REG12_ADDR + ((int_num / 32) << 2));
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		REG_WRITE(SYS_AHBP_REG12_ADDR + ((int_num / 32) << 2), reg);
		break;
	default:
		break;
	}

	return value;
}

__IRAM_SEC int32 sys_hal_int_disable(uint32_t core_index, uint32 param) //CMD_ICU_INT_DISABLE
{
	uint32 reg = 0;
	uint32 value = 0;
	if(core_index == 0)//cpu0
	{
		reg = sys_ll_get_cpu0_int_0_31_en_value();
		value = reg;
		reg &= ~(param);
		sys_ll_set_cpu0_int_0_31_en_value(reg);
	}
	else
	{
		reg = sys_ll_get_cpu1_int_0_31_en_value();
		value = reg;
		reg &= ~(param);
		sys_ll_set_cpu1_int_0_31_en_value(reg);
	}
	return value;
}

__IRAM_SEC int32 sys_hal_int_enable(uint32_t core_index,uint32 param) //CMD_ICU_INT_ENABLE
{
	uint32 reg = 0;
	if(core_index == 0)//cpu0
	{
		reg = sys_ll_get_cpu0_int_0_31_en_value();
		reg |= (param);
		sys_ll_set_cpu0_int_0_31_en_value(reg);
	}
	else
	{
		reg = sys_ll_get_cpu1_int_0_31_en_value();
		reg |= (param);
		sys_ll_set_cpu1_int_0_31_en_value(reg);
	}
	return 0;
}

__IRAM_SEC int32 sys_hal_int_group2_disable(uint32_t core_index,uint32 param)
{
	uint32 reg = 0;
	uint32 value = 0;
	if(core_index == 0)//cpu0
	{
		reg = sys_ll_get_cpu0_int_32_63_en_value();
		value = reg;
		reg &= ~(param);
		sys_ll_set_cpu0_int_32_63_en_value(reg);
	}
	else
	{
		reg = sys_ll_get_cpu1_int_32_63_en_value();
		value = reg;
		reg &= ~(param);
		sys_ll_set_cpu1_int_32_63_en_value(reg);
	}
	return value;
}

__IRAM_SEC int32 sys_hal_int_group2_enable(uint32_t core_index,uint32 param)
{
	uint32 reg = 0;
	if(core_index == 0)//cpu0
	{
		reg = sys_ll_get_cpu0_int_32_63_en_value();
		reg |= (param);
		sys_ll_set_cpu0_int_32_63_en_value(reg);
	}
	else
	{
		reg = sys_ll_get_cpu1_int_32_63_en_value();
		reg |= (param);
		sys_ll_set_cpu1_int_32_63_en_value(reg);
	}
	return 0;
}

int32 sys_hal_core_int_group1_disable(uint32_t core_id, uint32 param)
{
	return 0;
}

int32 sys_hal_core_int_group1_enable(uint32_t core_id, uint32 param)
{
	return 0;
}

int32 sys_hal_core_int_group2_disable(uint32_t core_id, uint32 param)
{
	return 0;
}

int32 sys_hal_core_int_group2_enable(uint32_t core_id, uint32 param)
{
	return 0;
}

int32 sys_hal_fiq_disable(uint32_t core_index,uint32 param)
{
	return 0;
}

int32 sys_hal_fiq_enable(uint32_t core_index,uint32 param)
{
	return 0;
}

int32 sys_hal_global_int_disable(uint32 param)
{
	return 0;
}

int32 sys_hal_global_int_enable(uint32 param)
{
	return 0;
}

uint32 sys_hal_get_int_status(uint32_t core_index)
{
	uint32_t ret = 0;

	if(core_index == 0) {
		ret = sys_ll_get_cpu0_int_0_31_status_value();
	} else if(core_index == 1) {
		ret = sys_ll_get_cpu1_int_0_31_status_value();
	}

	return ret;
}

__IRAM_SEC uint32 sys_hal_get_int_group2_status(uint32_t core_index)
{
	uint32_t ret = 0;

	if(core_index == 0) {
		ret = sys_ll_get_cpu0_int_32_63_status_value();
	} else if(core_index == 1) {
		ret = sys_ll_get_cpu1_int_32_63_status_value();
	}

	return ret;
}

uint32_t sys_hal_get_cpu0_gpio_int_st(void)
{
	return 0;
}

int32 sys_hal_set_int_status(uint32 param)
{
	return 0;
}

uint32 sys_hal_get_fiq_reg_status(uint32_t core_index)
{
	return 0;
}

uint32 sys_hal_set_fiq_reg_status(uint32 param)
{
	return 0;
}

uint32 sys_hal_get_intr_raw_status(void)
{
	return 0;
}

uint32 sys_hal_set_intr_raw_status(uint32 param)
{
	return 0;
}

int32 sys_hal_set_jtag_mode(uint32 param)
{
	return 0;
}

uint32 sys_hal_get_jtag_mode(void)
{
	return 0;
}

bk_err_t sys_hal_wireless_clock_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up)
{
	uint32_t dev_pos = 0;
	if((dev == CLK_PWR_ID_XVR) || (dev == CLK_PWR_ID_MAC) || (dev == CLK_PWR_ID_PHY) || (dev == CLK_PWR_ID_RF) || (dev == CLK_PWR_ID_THREAD))
	{
		dev_pos = dev%PM_CLOCK_MODE_1_POS;
		if(power_up == CLK_PWR_CTRL_PWR_UP)
		{
			if(s_pm_wireless_clock_state == 0x0)
			{
				if(sys_ll_get_reserver_reg0xd_wlss_cken() == 0x0)
				{
					sys_ll_set_reserver_reg0xd_wlss_cken(0x1);
				}
			}
			s_pm_wireless_clock_state |= (0x1 << dev_pos);
		}
		else
		{
			if(s_pm_wireless_clock_state &(0x1 << dev_pos))
			{
				s_pm_wireless_clock_state &= ~(0x1 << dev_pos);
				if (0x0 == s_pm_wireless_clock_state) {
					sys_ll_set_reserver_reg0xd_wlss_cken(0x0);
				}
			}
		}
	}
	return BK_OK;
}
__IRAM_SEC void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up)
{
	uint32_t reg_val = 0;

	sys_hal_wireless_clock_ctrl(dev, power_up);

	if (dev < PM_CLOCK_MODE_1_POS) {
		reg_val = sys_ll_get_cpu_device_clk_enable_value();
		if (power_up == CLK_PWR_CTRL_PWR_UP) {
			reg_val |= BIT(dev);
		} else {
			reg_val &= ~BIT(dev);
		}
		sys_ll_set_cpu_device_clk_enable_value(reg_val);
	} else if (dev < PM_CLOCK_MODE_2_POS) {
		reg_val = sys_ll_get_reserver_reg0xd_value();
		if (power_up == CLK_PWR_CTRL_PWR_UP) {
			reg_val |= BIT(dev - PM_CLOCK_MODE_1_POS);
		} else {
			reg_val &= ~BIT(dev - PM_CLOCK_MODE_1_POS);
		}
		sys_ll_set_reserver_reg0xd_value(reg_val);
	} else {
		reg_val = sys_ahbp_ll_get_rega_value();
		if (power_up == CLK_PWR_CTRL_PWR_UP) {
			reg_val |= BIT(dev - PM_CLOCK_MODE_2_POS);
		} else {
			reg_val &= ~BIT(dev - PM_CLOCK_MODE_2_POS);
		}
		sys_ahbp_ll_set_rega_value(reg_val);
	}
}

uint32_t sys_hal_clk_pwr_status_get(dev_clk_pwr_id_t dev)
{
	if (dev < PM_CLOCK_MODE_1_POS) {
		return sys_ll_get_cpu_device_clk_enable_value();
	} else if (dev < PM_CLOCK_MODE_2_POS) {
		return sys_ll_get_reserver_reg0xd_value();
	} else {
		return sys_ahbp_ll_get_rega_value();
	}
}

uint32_t sys_hal_clk_pwr_is_enabled(dev_clk_pwr_id_t dev)
{
	uint32_t reg_val = sys_hal_clk_pwr_status_get(dev);

	if (dev < PM_CLOCK_MODE_1_POS) {
		return (reg_val >> dev) & 0x1;
	} else if (dev < PM_CLOCK_MODE_2_POS) {
		return (reg_val >> (dev - PM_CLOCK_MODE_1_POS)) & 0x1;
	} else {
		return (reg_val >> (dev - PM_CLOCK_MODE_2_POS)) & 0x1;
	}
}

void sys_hal_set_cpu_power_sleep_wakeup_pwd_ofdm(uint32_t v)
{
#if (CONFIG_SOC_BK7239XX)
	sys_ll_set_cpu_power_sleep_wakeup_pwd_ofdm(v);
#else
	(void)v;
#endif
}

uint32_t sys_hal_get_cpu_power_sleep_wakeup_pwd_ofdm(void)
{
#if (CONFIG_SOC_BK7239XX)
	return sys_ll_get_cpu_power_sleep_wakeup_pwd_ofdm();
#elif CONFIG_SOC_BK7259
#if 1 //workaround
	bool get_dsss_only_flag(void);
	return get_dsss_only_flag();
#else//platform wakeup failure,to Do...
	return (0 == sys_ll_get_reserver_reg0xd_ofdm_cken());
#endif
#else
	return 0;
#endif
}

void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode)
{
	int sel_xtal = 0;
	int sel_apll = 1;

	switch(id)
	{
		case UART_ID_0:
		{
			if(mode == UART_SCLK_APLL)
				sys_ll_set_cpu_clk_div_mode2_cksel_uart0(sel_apll);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_uart0(sel_xtal);
			break;
		}
		case UART_ID_1:
		{
			if(mode == UART_SCLK_APLL)
				sys_ll_set_cpu_clk_div_mode2_cksel_uart1(sel_apll);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_uart1(sel_xtal);
			break;
		}
		case UART_ID_2:
		{
			if(mode == UART_SCLK_APLL)
				sys_ll_set_cpu_clk_div_mode2_cksel_uart2(sel_apll);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_uart2(sel_xtal);
			break;
		}
		case UART_ID_3:
		{
			if(mode == UART_SCLK_APLL)
				sys_ll_set_cpu_clk_div_mode2_cksel_uart3(sel_apll);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_uart3(sel_xtal);
			break;
		}
		case UART_ID_4:
		{
			if(mode == UART_SCLK_APLL)
				sys_ll_set_cpu_clk_div_mode2_cksel_uart4(sel_apll);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_uart4(sel_xtal);
			break;
		}
		default:
			break;
	}
}

void sys_hal_i2c_select_clock(i2c_id_t id, i2c_src_clk_t mode)
{
	int sel_xtal = 0;
	int sel_80m = 1;

	switch(id)
	{
		case I2C_ID_0:
		{
			if(mode == I2C_SCLK_80M)
				sys_ll_set_cpu_clk_div_mode2_cksel_i2c0(sel_80m);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_i2c0(sel_xtal);
			break;
		}
#if (SOC_I2C_UNIT_NUM > 1)
		case I2C_ID_1:
		{
			// I2C_ID_1 may not have separate clock select in sys_ll
			break;
		}
#endif
		default:
			break;
	}
}

void sys_hal_pwm_set_clock(uint32_t mode, uint32_t param)
{
	// TODO: Implement based on mode and param
	return;
}

void sys_hal_pwm_select_clock(sys_sel_pwm_t num, pwm_src_clk_t mode)
{
	int sel_clk32 = 0;
	int sel_xtal = 1;

	switch(num)
	{
		case SYS_SEL_PWM0:
			if(mode == PWM_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode3_cksel_pwm0(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode3_cksel_pwm0(sel_clk32);
			break;
		default:
			break;
	}
}

void sys_hal_timer_select_clock(sys_sel_timer_t num, timer_src_clk_t mode)
{
	int sel_clk32 = 0;
	int sel_xtal = 1;

	switch(num)
	{
		case SYS_SEL_TIMER0:
			if(mode == TIMER_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_tim0(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_tim0(sel_clk32);
			break;
		case SYS_SEL_TIMER1:
			if(mode == TIMER_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_tim1(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_tim1(sel_clk32);
			break;
		case SYS_SEL_TIMER2:
			if(mode == TIMER_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_tim2(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_tim2(sel_clk32);
			break;
		case SYS_SEL_TIMER3:
			if(mode == TIMER_SCLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_tim3(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_tim3(sel_clk32);
			break;

		default:
			break;
	}
}

uint32_t sys_hal_timer_select_clock_get(sys_sel_timer_t id)
{
    uint32_t ret = 0;

    switch(id)
    {
        case SYS_SEL_TIMER0:
        {
            ret = sys_ll_get_cpu_clk_div_mode2_cksel_tim0();
            break;
        }
        case SYS_SEL_TIMER1:
        {
            ret = sys_ll_get_cpu_clk_div_mode2_cksel_tim1();
            break;
        }
        case SYS_SEL_TIMER2:
        {
            ret = sys_ll_get_cpu_clk_div_mode2_cksel_tim2();
            break;
        }
        case SYS_SEL_TIMER3:
        {
            ret = sys_ll_get_cpu_clk_div_mode2_cksel_tim3();
            break;
        }
        default:
            break;
    }

    ret = (ret)?TIMER_SCLK_XTAL:TIMER_SCLK_CLK32;

    return ret;
}

void sys_hal_spi_select_clock(spi_id_t num, spi_src_clk_t mode)
{
	int sel_xtal = 0;
	int sel_160m = 1;

	switch(num)
	{
		case SPI_ID_0:
			if(mode == SPI_CLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_spi0(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_spi0(sel_160m);
			break;
#if (SOC_SPI_UNIT_NUM > 1)
		case SPI_ID_1:
			if(mode == SPI_CLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_spi1(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_spi1(sel_160m);
			break;
#endif
#if (SOC_SPI_UNIT_NUM > 2)
		case SPI_ID_2:
			if(mode == SPI_CLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_spi2(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_spi2(sel_160m);
			break;
#endif
#if (SOC_SPI_UNIT_NUM > 3)
		case SPI_ID_3:
			if(mode == SPI_CLK_XTAL)
				sys_ll_set_cpu_clk_div_mode2_cksel_spi3(sel_xtal);
			else
				sys_ll_set_cpu_clk_div_mode2_cksel_spi3(sel_160m);
			break;
#endif
		default:
			break;
	}
}

#if 1	//tmp build
void sys_hal_set_clk_select(dev_clk_select_id_t dev, dev_clk_select_t clk_sel)
{
	return;
}
dev_clk_select_t sys_hal_get_clk_select(dev_clk_select_id_t dev)
{
	return 0;
}
//DCO divider is valid for all of the peri-devices.
void sys_hal_set_dco_div(dev_clk_dco_div_t div)
{
	return;
}
//DCO divider is valid for all of the peri-devices.
dev_clk_dco_div_t sys_hal_get_dco_div(void)
{
	return CLK_DCO_DIV_1;
}
#endif	//temp build
/*clock power control end*/
void sys_hal_set_cksel_sadc(uint32_t value)
{
    sys_ll_set_cpu_clk_div_mode2_cksel_sadc(value);
}
void sys_hal_set_cksel_pwm0(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_pwm0(value);
}

void sys_hal_set_cksel_pwm1(uint32_t value)
{
	// Not supported in BK7259
	return;
}

void sys_hal_set_cksel_pwm(uint32_t value)
{
	sys_hal_set_cksel_pwm0(value);
}
uint32_t sys_hal_uart_select_clock_get(uart_id_t id)
{
	uint32_t ret = 0;

	switch(id)
	{
		case UART_ID_0:
		{
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart0();
			break;
		}
		case UART_ID_1:
		{
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart1();
			break;
		}
		case UART_ID_2:
		{
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart2();
			break;
		}
		case UART_ID_3:
		{
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart3();
			break;
		}
		case UART_ID_4:
		{
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart4();
			break;
		}
		default:
			break;
	}

	ret = (!ret)?UART_SCLK_XTAL_26M:UART_SCLK_APLL;

	return ret;
}
uint32_t sys_hal_i2c_select_clock_get(i2c_id_t id)
{
	uint32_t ret = 0;

	switch(id)
	{
		case I2C_ID_0:
		{
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_i2c0();
			break;
		}
#if (SOC_I2C_UNIT_NUM > 1)
		case I2C_ID_1:
		{
			// I2C_ID_1 may not have separate clock select in sys_ll
			ret = I2C_SCLK_XTAL;
			break;
		}
#endif
		default:
			break;
	}

	ret = (!ret)?I2C_SCLK_XTAL:I2C_SCLK_80M;

	return ret;
}
void sys_hal_sadc_int_enable(void)
{
	sys_hal_int_enable(rtos_get_core_id(),SADC_INTERRUPT_CTRL_BIT);
}
void sys_hal_sadc_int_disable(void)
{
	sys_hal_int_disable(rtos_get_core_id(),SADC_INTERRUPT_CTRL_BIT);
}
void sys_hal_sadc_pwr_up(void)
{
	sys_ll_set_cpu_device_clk_enable_sadc_cken(1);
	sys_ll_set_ana_reg22_gadc_en_spi(1);
}
void sys_hal_sadc_pwr_down(void)
{
	sys_ll_set_ana_reg22_gadc_en_spi(0);
	sys_ll_set_cpu_device_clk_enable_sadc_cken(0);
}

void sys_hal_set_saradc_cali_config(void)
{
    sys_ll_set_ana_reg22_value(0x3CAA5241);
    sys_ll_set_ana_reg23_value(0x33);
    sys_ll_set_ana_reg24_value(0x33);
    sys_ll_set_ana_reg2_adcdcsel(0x0);
}

void sys_hal_set_saradc_config(void)
{
    /* ana_reg0x22 */
    uint32_t reg_val = sys_ll_get_ana_reg22_value();
    reg_val |= BIT(31);
    reg_val &= ~BIT(30);
    reg_val &= ~BIT(25);
    sys_ll_set_ana_reg22_value(reg_val);
}

void sys_hal_set_sdmadc_config(void)
{
	// TODO: Implement SDMADC configuration
	return;
}

/** GADC Config (ana_reg22) Start **/

void sys_hal_set_gadc_config(uint32_t value)
{
	sys_ll_set_ana_reg22_value(value);
}

uint32_t sys_hal_get_gadc_config(void)
{
	return sys_ll_get_ana_reg22_value();
}

void sys_hal_set_gadc_enable(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_en_spi(value);
}

uint32_t sys_hal_get_gadc_enable(void)
{
	return sys_ll_get_ana_reg22_gadc_en_spi();
}

void sys_hal_set_gadc_inbuf_enable(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_inbuf_en(value);
}

uint32_t sys_hal_get_gadc_inbuf_enable(void)
{
	return sys_ll_get_ana_reg22_gadc_inbuf_en();
}

void sys_hal_set_gadc_calcap_ch(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_calcap_ch(value);
}

uint32_t sys_hal_get_gadc_calcap_ch(void)
{
	return sys_ll_get_ana_reg22_gadc_calcap_ch();
}

void sys_hal_set_gadc_clk_inv(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_clk_in(value);
}

uint32_t sys_hal_get_gadc_clk_inv(void)
{
	return sys_ll_get_ana_reg22_gadc_clk_in();
}

void sys_hal_set_gadc_clk_sel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_clk_sel(value);
}

uint32_t sys_hal_get_gadc_clk_sel(void)
{
	return sys_ll_get_ana_reg22_gadc_clk_sel();
}

void sys_hal_set_gadc_cal_ramp_enable(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_calintsaw_en(value);
}

uint32_t sys_hal_get_gadc_cal_ramp_enable(void)
{
	return sys_ll_get_ana_reg22_gadc_calintsaw_en();
}

void sys_hal_set_gadc_clk_relatch(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_clk_rlten(value);
}

uint32_t sys_hal_get_gadc_clk_relatch(void)
{
	return sys_ll_get_ana_reg22_gadc_clk_rlten();
}

void sys_hal_set_gadc_vbg_sel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_vbg_sel(value);
}

uint32_t sys_hal_get_gadc_vbg_sel(void)
{
	return sys_ll_get_ana_reg22_gadc_vbg_sel();
}

void sys_hal_set_gadc_comp_isel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_comp_isel(value);
}

uint32_t sys_hal_get_gadc_comp_isel(void)
{
	return sys_ll_get_ana_reg22_gadc_comp_isel();
}

void sys_hal_set_gadc_preamp_isel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_preamp_isel(value);
}

uint32_t sys_hal_get_gadc_preamp_isel(void)
{
	return sys_ll_get_ana_reg22_gadc_preamp_isel();
}

void sys_hal_set_gadc_bufamp_isel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_bufamp_isel(value);
}

uint32_t sys_hal_get_gadc_bufamp_isel(void)
{
	return sys_ll_get_ana_reg22_gadc_bufamp_isel();
}

void sys_hal_set_gadc_vref_sel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_vref_sel(value);
}

uint32_t sys_hal_get_gadc_vref_sel(void)
{
	return sys_ll_get_ana_reg22_gadc_vref_sel();
}

void sys_hal_set_gadc_inbuf_isel(uint32_t value)
{
	sys_ll_set_ana_reg22_inbuffer_isel(value);
}

uint32_t sys_hal_get_gadc_inbuf_isel(void)
{
	return sys_ll_get_ana_reg22_inbuffer_isel();
}

/** GADC Config (ana_reg22) End **/

/** Analog Comparator Config (ana_reg43) Start **/

uint32_t sys_hal_get_ana_reg43_config(void)
{
	return sys_ll_get_ana_reg43_value();
}

void sys_hal_set_ana_reg43_config(uint32_t value)
{
	sys_ll_set_ana_reg43_value(value);
}

void sys_hal_anacomp_charge_release_pulse(void)
{
    uint32_t saved = sys_ll_get_ana_reg43_value();
    sys_ana_reg43_t pulse;

    pulse.v = saved;
    pulse.acmp_chsel_a = 2;
    pulse.en_anacomp_a = 1;
    pulse.acmp_npmd_a = 1;
    pulse.acmp_chsel_b = 2;
    pulse.en_anacomp_b = 1;
    pulse.acmp_npmd_b = 1;
    sys_ll_set_ana_reg43_value(pulse.v);
    //sys_ll_set_ana_reg43_value(saved);
}

/** Analog Comparator Config (ana_reg43) End **/

/** Audio Bias & Mic Control (ana_reg20/ana_reg27) Start **/

void sys_hal_set_micbias_enable(uint32_t value)
{
	sys_ll_set_ana_reg20_enmicbias(value);
}

void sys_hal_set_mic2_enable(uint32_t value)
{
	sys_ll_set_ana_reg27_micen_mic2(value);
}

/** Audio Bias & Mic Control (ana_reg20/ana_reg27) End **/

/** VAD Control (ana_reg23) Start **/

void sys_hal_set_vad_config(uint32_t value)
{
	sys_ll_set_ana_reg23_value(value);
}

uint32_t sys_hal_get_vad_config(void)
{
	return sys_ll_get_ana_reg23_value();
}

void sys_hal_set_vad_enable(uint32_t value)
{
	sys_ll_set_ana_reg23_vad_en(value);
}

uint32_t sys_hal_get_vad_enable(void)
{
	return sys_ll_get_ana_reg23_vad_en();
}

void sys_hal_set_vad_cftrm(uint32_t value)
{
	sys_ll_set_ana_reg23_vad_cftrm(value);
}

uint32_t sys_hal_get_vad_cftrm(void)
{
	return sys_ll_get_ana_reg23_vad_cftrm();
}

void sys_hal_set_vad_cstrm(uint32_t value)
{
	sys_ll_set_ana_reg23_vad_cstrm(value);
}

uint32_t sys_hal_get_vad_cstrm(void)
{
	return sys_ll_get_ana_reg23_vad_cstrm();
}

void sys_hal_set_vad_viniset(uint32_t value)
{
	sys_ll_set_ana_reg23_vad_viniset(value);
}

uint32_t sys_hal_get_vad_viniset(void)
{
	return sys_ll_get_ana_reg23_vad_viniset();
}

void sys_hal_set_vad_rstn(uint32_t value)
{
	sys_ll_set_ana_reg23_vad_rstn(value);
}

uint32_t sys_hal_get_vad_rstn(void)
{
	return sys_ll_get_ana_reg23_vad_rstn();
}

void sys_hal_set_vad_vcm_sel(uint32_t value)
{
	sys_ll_set_ana_reg23_vadrefsel(value);
}

uint32_t sys_hal_get_vad_vcm_sel(void)
{
	return sys_ll_get_ana_reg23_vadrefsel();
}

void sys_hal_set_vad_clk_inv(uint32_t value)
{
	sys_ll_set_ana_reg23_vadckinven(value);
}

uint32_t sys_hal_get_vad_clk_inv(void)
{
	return sys_ll_get_ana_reg23_vadckinven();
}

/** VAD Control (ana_reg23) End **/

/** Power/LDO Control (ana_reg10/0x4a) Start **/

void sys_hal_set_core_ldo_low_vol(uint32_t value)
{
	sys_ll_set_ana_reg10_vcorelsel(value);
}

uint32_t sys_hal_get_core_ldo_low_vol(void)
{
	return sys_ll_get_ana_reg10_vcorelsel();
}

void sys_hal_set_core_ldo_high_vol(uint32_t value)
{
	sys_ll_set_ana_reg10_vcorehsel(value);
}

uint32_t sys_hal_get_core_ldo_high_vol(void)
{
	return sys_ll_get_ana_reg10_vcorehsel();
}

void sys_hal_set_digldo_low_vol_enable(uint32_t value)
{
	sys_ll_set_ana_reg10_vdd12lden(value);
}

void sys_hal_set_coreldo_low_vol_enable(uint32_t value)
{
	sys_ll_set_ana_reg10_vlden(value);
}

/** Power/LDO Control (ana_reg10/0x4a) End **/

/** LDO HP Mode & RTC Clock (ana_reg9/0x49) Start **/

void sys_hal_set_aloldo_hp(uint32_t value)
{
	sys_ll_set_ana_reg9_aloldohp(value);
}

void sys_hal_set_analdo_hp(uint32_t value)
{
	sys_ll_set_ana_reg9_aldohp(value);
}

void sys_hal_set_digldo_hp(uint32_t value)
{
	sys_ll_set_ana_reg9_dldohp(value);
}

void sys_hal_set_coreldo_hp(uint32_t value)
{
	sys_ll_set_ana_reg9_coreldo_hp(value);
}

void sys_hal_set_rtc_clk_sel(uint32_t value)
{
	sys_ll_set_ana_reg9_clk_sel(value);
}

void sys_hal_set_aon_ldo_vol(uint32_t value)
{
	sys_ll_set_ana_reg9_valoldosel(value);
}

/** LDO HP Mode & RTC Clock (ana_reg9/0x49) End **/

/** BuckA Control (ana_reg12/0x4c) Start **/

void sys_hal_set_bucka_ea_enable(uint32_t value)
{
	sys_ll_set_ana_reg12_enpowa(value);
}

/** BuckA Control (ana_reg12/0x4c) End **/

/** Analog Clock Control (ana_reg5/0x45) Start **/

void sys_hal_set_dpll_enable(uint32_t value)
{
	sys_ll_set_ana_reg5_en_dpll(value);
}

void sys_hal_set_dco_enable(uint32_t value)
{
	sys_ll_set_ana_reg5_en_dco(value);
}

void sys_hal_set_xtall_enable(uint32_t value)
{
	sys_ll_set_ana_reg5_en_xtall(value);
}

void sys_hal_set_cb_enable(uint32_t value)
{
	sys_ll_set_ana_reg5_en_cb(value);
}

/** Analog Clock Control (ana_reg5/0x45) End **/

void sys_hal_set_clksel_spi0(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_spi0(value);
}

void sys_hal_set_clksel_spi1(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_spi1(value);
}

void sys_hal_set_clksel_spi(uint32_t value)
{
	if((SPI_CLK_SRC_XTAL == value) || (SPI_CLK_SRC_APLL == value))
	{
		sys_ll_set_cpu_clk_div_mode2_cksel_spi0(value);
#if (SOC_SPI_UNIT_NUM > 1)
		sys_ll_set_cpu_clk_div_mode2_cksel_spi1(value);
#endif
#if (SOC_SPI_UNIT_NUM > 2)
		sys_ll_set_cpu_clk_div_mode2_cksel_spi2(value);
#endif
#if (SOC_SPI_UNIT_NUM > 3)
		sys_ll_set_cpu_clk_div_mode2_cksel_spi3(value);
#endif
	}
}


void sys_hal_trng_disckg_set(uint32_t value)
{
	// TODO: Implement TRNG disckg setting
	return;
}

uint32_t sys_hal_interrupt_status_get(void)
{
	return sys_hal_get_int_status(rtos_get_core_id());
}

void sys_hal_interrupt_status_set(uint32_t value)
{
	// Status register is read-only, this function may not be needed
	return;
}

void sys_hal_en_tempdet(uint32_t value)
{
	sys_ll_set_ana_reg5_en_temp(value);
}
uint32_t sys_hal_mclk_mux_get(void)
{
	uint32_t ret = 0;

	ret = sys_ll_get_cpu_clk_div_mode1_cksel_core();

	return ret;
}

void sys_hal_mclk_mux_set(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_core(value);
}

uint32_t sys_hal_mclk_div_get(void)
{
	uint32_t ret = 0;

	ret = sys_ll_get_cpu_clk_div_mode1_ckdiv_core();

	return ret;
}

void sys_hal_mclk_div_set(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode1_ckdiv_core(value);
}

void sys_hal_mclk_select(uint32_t value)
{
	sys_hal_mclk_mux_set(value);
}
/**  Platform End **/
/**  BT Start **/
//BT
void sys_hal_bt_power_ctrl(bool power_up)
{
	return;
}

void sys_hal_bt_clock_ctrl(bool en)
{
	return;
}

void sys_hal_xvr_clock_ctrl(bool en)
{
	return;
}

void sys_hal_btdm_interrupt_ctrl(bool en)
{
    if (en)
    {
        sys_ll_set_cpu0_int_32_63_en_cpu0_dm_irq_int_en(1);
            //TODO enable PLIC Int Enable Registers
    }
    else
    {
        sys_ll_set_cpu0_int_32_63_en_cpu0_dm_irq_int_en(0);
            //TODO disable PLIC Int Enable Registers
    }
}

void sys_hal_ble_interrupt_ctrl(bool en)
{
    if (en)
    {
        sys_ll_set_cpu0_int_32_63_en_cpu0_ble_irq_int_en(1);
            //TODO enable PLIC Int Enable Registers
    }
    else
    {
        sys_ll_set_cpu0_int_32_63_en_cpu0_ble_irq_int_en(0);
            //TODO disable PLIC Int Enable Registers
    }
}

void sys_hal_bt_interrupt_ctrl(bool en)
{
    if (en)
    {
        sys_ll_set_cpu0_int_32_63_en_cpu0_bt_irq_int_en(1);
            //TODO enable PLIC Int Enable Registers
    }
    else
    {
        sys_ll_set_cpu0_int_32_63_en_cpu0_bt_irq_int_en(0);
            //TODO disable PLIC Int Enable Registers
    }
}

#if CONFIG_MAC802154_ENABLE
extern int32_t sys_drv_set_int_en(uint32_t core_id, uint32_t int_num, uint32_t int_en);
void sys_hal_thread_interrupt_ctrl(bool en)
{
    if (en) {
		sys_drv_set_int_en(CPU0_CORE_ID, INT_SRC_THREAD, 1);
    } else {
		sys_drv_set_int_en(CPU0_CORE_ID, INT_SRC_THREAD, 0);
    }
}
#endif

void sys_hal_bt_rf_ctrl(bool en)
{
	return;
}

void sys_hal_rf_ctrl(uint8_t type)
{
    if (type == RF_CTRL_PTA) {
        // rf ctrl by PTA
        sys_ll_set_cpu_storage_connect_op_select_rf_switch_manual_en(0);
    } else if (type == RF_CTRL_WIFI) {
        // rf ctrl by Wi-Fi
        sys_ll_set_cpu_storage_connect_op_select_rf_switch_manual_en(1);
        sys_ll_set_cpu_storage_connect_op_select_rf_source(RF_CTRL_WIFI);
    } else if (type == RF_CTRL_BT) {
        // rf ctrl by BT
        sys_ll_set_cpu_storage_connect_op_select_rf_switch_manual_en(1);
        sys_ll_set_cpu_storage_connect_op_select_rf_source(RF_CTRL_BT);
    } else if (type == RF_CTRL_THREAD) {
        // rf ctrl by thread
       sys_ll_set_cpu_storage_connect_op_select_rf_switch_manual_en(1);
       sys_ll_set_cpu_storage_connect_op_select_rf_source(RF_CTRL_THREAD);
    }
}

uint8_t sys_hal_rf_ctrl_type_get(void)
{
    uint8_t type = 0;
    if (sys_ll_get_cpu_storage_connect_op_select_rf_switch_manual_en()) {
        type = sys_ll_get_cpu_storage_connect_op_select_rf_source();
        if (type == RF_CTRL_PTA || type == RF_CTRL_WIFI) {
            type = RF_CTRL_WIFI;
        }
    } else {
        type = RF_CTRL_PTA;
    }

    return type;
}

uint32_t sys_hal_bt_rf_status_get(void)
{
	return 0;
}
void sys_hal_bt_sleep_exit_ctrl(bool en)
{
    if (en)
    {
        sys_ll_set_cpu_power_sleep_wakeup_bts_soft_wakeup_req(1);
    }
    else
    {
        sys_ll_set_cpu_power_sleep_wakeup_bts_soft_wakeup_req(0);
    }
}

void sys_hal_set_bts_wakeup_platform_en(bool value)
{
	return;
}
uint32 sys_hal_get_bts_wakeup_platform_en()
{
	return 0;
}
/**  BT End **/
/**  Audio Start **/
//Audio
/**  Audio End **/
/**  Video Start **/
/**  Video End **/
/**  WIFI Start **/
//WIFI
//Yantao Add Start
void sys_hal_modem_core_reset(void)
{
	return;
}
void sys_hal_mpif_invert(void)
{
	return;
}
void sys_hal_modem_subsys_reset(void)
{
	return;
}
void sys_hal_mac_subsys_reset(void)
{
	return;
}
void sys_hal_usb_subsys_reset(void)
{
	return;
}
void sys_hal_dsp_subsys_reset(void)
{
	return;
}
void sys_hal_mac_power_ctrl(bool power_up)
{
	return;
}
void sys_hal_modem_power_ctrl(bool power_up)
{
	return;
}
void sys_hal_pta_ctrl(bool pta_en)
{
	return;
}
__IRAM_SEC void sys_hal_modem_bus_clk_ctrl(bool clk_en)
{
	return;
}
__IRAM_SEC void sys_hal_modem_clk_ctrl(bool clk_en)
{
	sys_ll_set_reserver_reg0xd_phy_cken(clk_en);
}
void sys_hal_mac_bus_clk_ctrl(bool clk_en)
{
	return;
}
void sys_hal_mac_clk_ctrl(bool clk_en)
{
	sys_ll_set_reserver_reg0xd_mac_cken(clk_en);
}

uint32_t sys_hal_wifi_wlss_clk_is_enabled(void)
{
	return sys_ll_get_reserver_reg0xd_wlss_cken();
}

uint32_t sys_hal_wifi_mac_clk_is_enabled(void)
{
	return sys_ll_get_reserver_reg0xd_mac_cken();
}

uint32_t sys_hal_wifi_phy_clk_is_enabled(void)
{
	return sys_ll_get_reserver_reg0xd_phy_cken();
}

void sys_hal_wifi_reg_access_status_get(uint32_t *clk_status, uint32_t *power_status)
{
	if (clk_status) {
		*clk_status = sys_ll_get_reserver_reg0xd_value();
	}

	if (power_status) {
		*power_status = sys_ll_get_reserver_reg0x10_value();
	}
}

void sys_hal_set_vdd_value(uint32_t param)
{
	sys_hal_ctrl_vdddig_h_vol(param);
}
uint32_t sys_hal_get_vdd_value(void)
{
	return sys_ll_get_ana_reg10_vcorehsel();
}
//CMD_SCTRL_BLOCK_EN_MUX_SET
void sys_hal_block_en_mux_set(uint32_t param)
{
	return;
}
void sys_hal_enable_mac_gen_int(void)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_gen_n_int_en(1);
	return;
}
void sys_hal_enable_mac_prot_int(void)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_port_trigger_n_int_en(1);
	return;
}
void sys_hal_enable_mac_tx_trigger_int(void)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_tx_trigger_n_int_en(1);
	return;
}
void sys_hal_enable_mac_rx_trigger_int(void)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_rx_trigger_n_int_en(1);
	return;
}
void sys_hal_enable_mac_txrx_misc_int(void)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_tx_rx_misc_n_int_en(1);
	return;
}
void sys_hal_enable_mac_txrx_timer_int(void)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_mac_int_tx_rx_timer_n_int_en(1);
	return;
}
void sys_hal_enable_modem_int(void)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_phy_mbp_int_en(1);
}
void sys_hal_enable_modem_rc_int(void)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_phy_riu_int_en(1);
}
void sys_hal_enable_hsu_int(void)
{
	return;
}
void sys_hal_disable_hsu_int(void)
{
	return;
}
void sys_hal_set_debug_mux(int type)
{
	return;
}
void sys_hal_diag_debug_mac()
{
	return;
}
void sys_hal_diag_debug_mac802154(void)
{
	//sys_ll_set_dbug_config0_dbug_mux(DIAG_DEBUG_THREAD);
	return;
}
void sys_hal_diag_debug_phy()
{
	return;
}
void dbg_enable_debug_gpio(void)
{
	return;
}

__IRAM_SEC void sys_hal_rf_clk_ctrl(bool clk_en)
{
	sys_ll_set_reserver_reg0xd_rf_cken(clk_en);
}
//Yantao Add End
void sys_hal_cali_dpll_spi_trig_disable(void)
{
	sys_ll_set_ana_reg0_spitrig(0);
}

void sys_hal_cali_dpll_spi_trig_enable(void)
{
	sys_ll_set_ana_reg0_spitrig(1);
}

void sys_hal_cali_dpll_spi_detect_disable(void)
{
	sys_ll_set_ana_reg0_spideten(0);
}

void sys_hal_cali_dpll_spi_detect_enable(void)
{
	sys_ll_set_ana_reg0_spideten(1);
}
void sys_hal_set_xtalh_ctune(uint32_t value)
{
	sys_ll_set_ana_reg3_ctune(value);
	return;
}
__IRAM_SEC void sys_hal_analog_set(analog_reg_t reg, uint32_t value)
{
	uint32_t analog_reg_address;

	if ((reg < ANALOG_REG0) || (reg >= ANALOG_MAX)) {
		return;
	}

	analog_reg_address = SYS_ANA_REG0_ADDR + (reg - ANALOG_REG0) * 4;

	sys_ll_set_analog_reg_value(analog_reg_address, value);
}

uint32_t sys_hal_analog_get(analog_reg_t reg)
{
	uint32_t analog_reg_address;

    if ((reg < ANALOG_REG0) || (reg >= ANALOG_MAX)) {
        return 0;
    }

    analog_reg_address = SYS_ANA_REG0_ADDR + (reg - ANALOG_REG0) * 4;

	return sys_ll_get_analog_reg_value(analog_reg_address);
}

void sys_hal_set_ana_reg1_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg2_value(uint32_t value)
{
	return;
}
uint32_t sys_hal_get_ana_reg2_value(void)
{
	return sys_ll_get_ana_reg2_value();
}
void sys_hal_set_ana_reg3_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg4_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg12_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg13_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg14_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg15_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg16_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg17_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg18_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg19_value(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg20_value(uint32_t value)
{
    sys_ll_set_ana_reg20_value(value);
}
uint32_t sys_hal_get_ana_reg20_value(void)
{
    return sys_ll_get_ana_reg20_value();
}
void sys_hal_set_ana_reg21_value(uint32_t value)
{
    sys_ll_set_ana_reg21_value(value);
}
void sys_hal_set_ana_reg24_value(uint32_t value)
{
    sys_ll_set_ana_reg24_value(value);
}
uint32_t sys_hal_get_ana_reg24_value(void)
{
    return sys_ll_get_ana_reg24_value();
}
void sys_hal_set_ana_reg25_value(uint32_t value)
{
    sys_ll_set_ana_reg25_value(value);
}
void sys_hal_set_ana_reg27_value(uint32_t value)
{
    sys_ll_set_ana_reg27_value(value);
}
uint32_t sys_hal_get_ana_reg27_value(void)
{
    return sys_ll_get_ana_reg27_value();
}
void sys_hal_set_ana_reg28_value(uint32_t value)
{
    sys_ll_set_ana_reg28_value(value);
}

uint32_t sys_hal_get_ana_reg28_value(void)
{
    return sys_ll_get_ana_reg28_value();
}

void sys_hal_set_ana_reg29_value(uint32_t value)
{
    sys_ll_set_ana_reg29_value(value);
}
void sys_hal_set_ana_reg30_value(uint32_t value)
{
    sys_ll_set_ana_reg30_value(value);
}

uint32_t sys_hal_bias_reg_read(void)
{
	return 0;
}
uint32_t sys_hal_bias_reg_write(uint32_t param)
{
	return 0;
}
uint32_t sys_hal_analog_reg2_get(void)
{
	return 0;
}
uint32_t sys_hal_bias_reg_set(uint32_t param)
{
	return 0;
}
uint32_t sys_hal_bias_reg_clean(uint32_t param)
{
	return 0;
}
uint32_t sys_hal_get_xtalh_ctune(void)
{
	return sys_ll_get_ana_reg3_ctune();
}
uint32_t sys_hal_cali_bgcalm(void)
{
	uint32_t bandgap;

	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_pwd_bgcal(0);
	sys_ll_set_ana_reg9_vbgcalmode(0);
	sys_ll_set_ana_reg9_vbgcalstart(0);
	sys_ll_set_ana_reg9_vbgcalstart(1);
	bk_delay_us(100);//100us
	sys_ll_set_ana_reg9_vbgcalstart(0);

	bk_delay_us(2000);//2ms to avoid consistency issue

	bandgap = aon_pmu_ll_get_r7d_bgcal();

	sys_ll_set_ana_reg9_vbgcalmode(1);
	sys_ll_set_ana_reg9_pwd_bgcal(1);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	return bandgap;
}

uint32_t sys_hal_get_bgcalm(void)
{
	return sys_ll_get_ana_reg9_bgcal();
}

void sys_hal_set_bgcalm(uint32_t value)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_bgcal(value);
	sys_ll_set_ana_reg10_spi_latch1v(0);
}
void sys_hal_set_audioen(uint32_t value)
{
	return;
}
void sys_hal_set_dpll_div_cksel(uint32_t value)
{
	return;
}
void sys_hal_set_dpll_reset(uint32_t value)
{
	return;
}
void sys_hal_set_gadc_ten(uint32_t value)
{
	return;
}
/**  WIFI End **/
/**  Audio Start  **/
void sys_hal_aud_select_clock(uint32_t value)
{
    sys_ll_set_cpu_clk_div_mode3_cksel_audio(value);
}
void sys_hal_aud_set_ckdiv_audio(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode3_ckdiv_audio(value);
}
void sys_hal_aud_clock_en(uint32_t value)
{
    sys_ll_set_reserver_reg0xd_audio_cken(value);
}

void sys_hal_aud_dacl_en(uint32_t value)
{
    sys_ll_set_ana_reg29_daclen(value);
}
void sys_hal_aud_diffen_en(uint32_t value)
{
	return;
}
void sys_hal_aud_mic_rst_set(uint32_t value)
{
	return;
}
void sys_hal_aud_mic1_en(uint32_t value)
{
    sys_ll_set_ana_reg21_micen_mic1(value);
}
void sys_hal_aud_mic1_rst_set(uint32_t value)
{
    sys_ll_set_ana_reg21_rst_mic1(value);
}
void sys_hal_aud_mic1_gain_set(uint32_t value)
{
    sys_ll_set_ana_reg21_micgain_mic1(value);
}
void sys_hal_aud_mic1_single_en(uint32_t value)
{
    sys_ll_set_ana_reg21_micsingleen_mic1(value);
}
void sys_hal_aud_dac_diffen_en(uint32_t value)
{
	sys_ll_set_ana_reg29_diffen(value);
}
void sys_hal_aud_dcoc_en(uint32_t value)
{
    sys_ll_set_ana_reg29_lendcoc(value);
}
void sys_hal_aud_lmdcin_set(uint32_t value)
{
	return;
}
void sys_hal_aud_audbias_en(uint32_t value)
{
	sys_ll_set_ana_reg20_enaudbias(value);
}
void sys_hal_aud_adcbias_en(uint32_t value)
{
    sys_ll_set_ana_reg20_enadcbias(value);
}
void sys_hal_aud_micbias_en(uint32_t value)
{
    sys_ll_set_ana_reg20_enmicbias(value);
}
void sys_hal_aud_dac_bias_en(uint32_t value)
{
    sys_ll_set_ana_reg30_enbs(value);
}
//void sys_hal_aud_idac_en(uint32_t value)
//{
//	return;
//}
void sys_hal_aud_idacl_en(uint32_t value)
{
	sys_ll_set_ana_reg30_enidacl(value);
}

void sys_hal_aud_idacr_en(uint32_t value)
{
	sys_ll_set_ana_reg30_enidacr(value);
}
void sys_hal_aud_dac_drv_en(uint32_t value)
{
    sys_ll_set_ana_reg29_dacdrven(value);
}

void sys_hal_aud_int_en(uint32_t value)
{
	//sys_ll_set_cpu0_int_0_31_en_aud_int(value); ///cm33
    sys_ll_set_cpu0_int_0_31_en_cpu0_aud_int_en(value);
}
void sys_hal_aud_power_en(uint32_t value)
{
	return;
}
void sys_hal_aud_lvcmd_en(uint32_t value)
{
	return;
}
void sys_hal_aud_micbias1v_en(uint32_t value)
{
	return;
}
void sys_hal_aud_micbias_trim_set(uint32_t value)
{
	return;
}
void sys_hal_aud_audpll_en(uint32_t value)
{
	return;
}
void sys_hal_aud_dacr_en(uint32_t value)
{
    sys_ll_set_ana_reg29_dacren(value);
}
void sys_hal_aud_vdd1v_en(uint32_t value)
{
	return;
}
void sys_hal_aud_vdd1v5_en(uint32_t value)
{
	return;
}
void sys_hal_aud_mic2_en(uint32_t value)
{
    sys_ll_set_ana_reg27_micen_mic2(value);
}
void sys_hal_aud_mic2_rst_set(uint32_t value)
{
    sys_ll_set_ana_reg27_rst_mic2(value);
}
void sys_hal_aud_mic2_gain_set(uint32_t value)
{
    sys_ll_set_ana_reg27_micgain_mic2(value);
}
void sys_hal_aud_mic2_single_en(uint32_t value)
{
    sys_ll_set_ana_reg27_micsingleen_mic2(value);
}
void sys_hal_aud_dacg_set(uint32_t value)
{
    sys_ll_set_ana_reg29_dacg(value);
}
void sys_hal_aud_aud_en(uint32_t value)
{
	return;
}
void sys_hal_aud_rvcmd_en(uint32_t value)
{
	return;
}
void sys_hal_dmic_clk_div_set(uint32_t value)
{
	return;
}
void sys_hal_aud_dac_dacmute_en(uint32_t value)
{
	return;
}
void sys_hal_aud_mic3_en(uint32_t value)
{
    sys_ll_set_ana_reg28_micen_mic3(value);
}
void sys_hal_aud_mic3_rst_set(uint32_t value)
{
    sys_ll_set_ana_reg28_rst_mic3(value);
}
void sys_hal_aud_mic3_gain_set(uint32_t value)
{
    sys_ll_set_ana_reg28_micgain_mic3(value);
}
void sys_hal_aud_mic3_single_en(uint32_t value)
{
    sys_ll_set_ana_reg28_micsingleen_mic3(value);
}
/**  Audio End  **/
/**  FFT Start  **/
void sys_hal_fft_disckg_set(uint32_t value)
{
	return;
}
void sys_hal_cpu_fft_int_en(uint32_t value)
{
	return;
}
/**  FFT End  **/
/**  I2S Start  **/
void sys_hal_i2s_select_clock(uint32_t value)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s0(value);
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s1(value);
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s2(value);
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s3(value);
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s4(value);
}
void sys_hal_i2s_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_i2s0_cken(value);
}
void sys_hal_i2s1_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_i2s1_cken(value);
}
void sys_hal_i2s2_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_i2s2_cken(value);
}
void sys_hal_i2s3_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_i2s3_cken(value);
}
void sys_hal_i2s4_clock_en(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_i2s4_cken(value);
}

void sys_hal_i2s_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_i2s0_int_en(value);
}
void sys_hal_i2s1_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_0_31_en_cpu0_i2s1_int_en(value);
}
void sys_hal_i2s2_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_i2s2_int_en(value);
}
void sys_hal_i2s3_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_i2s3_int_en(value);
}
void sys_hal_i2s4_int_en(uint32_t value)
{
	sys_ll_set_cpu0_int_64_95_en_cpu0_i2s4_int_en(value);
}

void sys_hal_i2s_disckg_set(uint32_t value)
{
	return;
}
void sys_hal_apll_en(uint32_t value)  /// modify - 260105
{
	if(value == 0)
	{
		sys_ll_set_ana_reg5_pwdaudpll(1);
	}
	else
	{
		sys_ll_set_ana_reg5_pwdaudpll(0);
	}
    //sys_ll_set_ana_reg5_pwdaudpll(value);
}
void sys_hal_cb_manu_val_set(uint32_t value)
{
	return;
}
void sys_hal_ana_reg11_vsel_set(uint32_t value)
{
	return;
}
void sys_hal_apll_cal_val_set(uint32_t value)  /// modify - 260105
{
    //sys_ll_set_ana_reg26_value(value);
    sys_ll_set_ana_reg26_n(value);
	sys_ll_set_ana_reg26_calres_spien(0);
	sys_ll_set_ana_reg26_calrefen(1);
}
void sys_hal_apll_spi_trigger_set(uint32_t value)
{
    sys_ll_set_ana_reg25_spi_trigger(value);
}
void sys_hal_i2s0_ckdiv_set(uint32_t value)
{
	return;
}
void sys_hal_apll_config_set(uint32_t value)
{
	sys_ll_set_ana_reg25_value(value);///
	return;
}
/**  I2S End  **/
/**  Touch Start **/
void sys_hal_touch_power_down(uint32_t value)
{
	sys_ll_set_ana_reg32_pwd_td(value);
}
void sys_hal_touch_sensitivity_level_set(uint32_t value)
{
	sys_ll_set_ana_reg32_gain_s(value);
}
void sys_hal_touch_scan_mode_enable(uint32_t value)
{
	sys_ll_set_ana_reg33_en_scan(value);
}
void sys_hal_touch_detect_threshold_set(uint32_t value)
{
	sys_ll_set_ana_reg32_vrefs(value);
}
void sys_hal_touch_detect_range_set(uint32_t value)
{
	sys_ll_set_ana_reg32_crg(value);
}
void sys_hal_touch_calib_enable(uint32_t value)
{
	sys_ll_set_ana_reg33_en_cal_force(value);
}
void sys_hal_touch_cal_ctrl_set(uint32_t value)
{
	sys_ll_set_ana_reg32_cal_ctrl(value);
}
uint32_t sys_hal_touch_cal_ctrl_get(void)
{
	return sys_ll_get_ana_reg32_cal_ctrl();
}
void sys_hal_touch_cal_vth_set(uint32_t value)
{
	sys_ll_set_ana_reg32_cal_vth(value);
}
uint32_t sys_hal_touch_cal_vth_get(void)
{
	return sys_ll_get_ana_reg32_cal_vth();
}
void sys_hal_touch_rstb_dig_set(uint32_t value)
{
	sys_ll_set_ana_reg32_rstb_dig(value);
}
uint32_t sys_hal_touch_rstb_dig_get(void)
{
	return sys_ll_get_ana_reg32_rstb_dig();
}
void sys_hal_touch_ldoen_set(uint32_t value)
{
	sys_ll_set_ana_reg32_ldoen(value);
}
uint32_t sys_hal_touch_ldoen_get(void)
{
	return sys_ll_get_ana_reg32_ldoen();
}
void sys_hal_touch_cal_auto_set(uint32_t value)
{
	sys_ll_set_ana_reg33_en_cal_auto(value);
}
void sys_hal_touch_cal_done_clr(uint32_t value)
{
	sys_ll_set_ana_reg33_cal_done_clr(value);
}
void sys_hal_touch_manul_mode_calib_value_set(uint32_t value)
{
	sys_ll_set_ana_reg35_cap_calspi(value);
}
uint32_t sys_hal_touch_calib_value_get(void)
{
	return sys_ll_get_ana_reg35_cap_calspi();
}
void sys_hal_touch_manul_mode_enable(uint32_t value)
{
	sys_ll_set_ana_reg35_en_manmode(value);
}
void sys_hal_touch_scan_mode_chann_set(uint32_t value)
{
	sys_ll_set_ana_reg33_chs(value);
}
void sys_hal_touch_scan_mode_chann_sel(uint32_t value)
{
	sys_ll_set_ana_reg33_chs_sel_cal(value);
}
void sys_hal_touch_serial_cap_enable(void)
{
	sys_ll_set_ana_reg32_en_seri_cap(1);
}
void sys_hal_touch_serial_cap_disable(void)
{
	sys_ll_set_ana_reg32_en_seri_cap(0);
}
void sys_hal_touch_serial_cap_sel(uint32_t value)
{
	sys_ll_set_ana_reg32_sel_seri_cap(value);
}
void sys_hal_touch_spi_lock(void)
{
	sys_ll_set_ana_reg32_td_latch(0);
}
void sys_hal_touch_spi_unlock(void)
{
	sys_ll_set_ana_reg32_td_latch(1);
}
void sys_hal_touch_test_period_set(uint32_t value)
{
	sys_ll_set_ana_reg33_test_period(value);
}
void sys_hal_touch_test_number_set(uint32_t value)
{
	sys_ll_set_ana_reg33_test_number(value);
}
void sys_hal_touch_calib_period_set(uint32_t value)
{
	sys_ll_set_ana_reg34_cl_period(value);
}
uint32_t sys_hal_touch_calib_period_get(void)
{
	return sys_ll_get_ana_reg34_cl_period();
}
void sys_hal_touch_calib_number_set(uint32_t value)
{
	sys_ll_set_ana_reg34_cal_number(value);
}
uint32_t sys_hal_touch_calib_number_get(void)
{
	return sys_ll_get_ana_reg34_cal_number();
}
void sys_hal_touch_modsel_spi_set(uint32_t value)
{
	sys_ll_set_ana_reg34_modsel_spi(value);
}
uint32_t sys_hal_touch_modsel_spi_get(void)
{
	return sys_ll_get_ana_reg34_modsel_spi();
}
void sys_hal_touch_int_set(uint32_t value)
{
	sys_ll_set_ana_reg34_int_en(value);
}
void sys_hal_touch_int_clear(uint32_t value)
{
	sys_ll_set_ana_reg35_int_clr(value);
}
void sys_hal_touch_int_enable(uint32_t value)
{
	sys_ll_set_cpu0_int_32_63_en_cpu0_touched_int_en(value);
}
/**  Touch End **/
/** jpeg start **/
void sys_hal_set_jpeg_clk_sel(uint32_t value)
{
	return;
}
void sys_hal_set_clk_div_mode1_clkdiv_jpeg(uint32_t value)
{
	return;
}
void sys_hal_set_jpeg_disckg(uint32_t value)
{
	return;
}
void sys_hal_set_cpu_clk_div_mode1_clkdiv_bus(uint32_t value)
{
	return;
}
void sys_hal_video_power_en(uint32_t value)
{
	return;
}
void sys_hal_set_auxs_cis_clk_sel(uint32_t value)
{
	return;
}
void sys_hal_set_auxs_cis_clk_div(uint32_t value)
{
	return;
}
void sys_hal_set_jpeg_clk_en(uint32_t value)
{
	return;
}
void sys_hal_set_cis_auxs_clk_en(uint32_t value)
{
	return;
}
/** jpeg end **/
/** h264 Start **/
void sys_hal_set_h264_clk_en(uint32_t value)
{
	return;
}
/** h264 End **/
/**  psram Start **/
void sys_hal_psram_volstage_sel(uint32_t enable)
{
	return;
}
void sys_hal_psram_xtall_osc_enable(uint32_t enable)
{
	return;
}
void sys_hal_psram_doc_enable(uint32_t enable)
{
	return;
}
void sys_hal_psram_ldo_enable(uint32_t enable)
{
	sys_ll_set_ana_reg14_enpsram(enable);
}

uint32_t sys_hal_psram_ldo_status(void)
{
	return sys_ll_get_ana_reg14_enpsram();
}

void sys_hal_psram_dpll_enable(uint32_t enable)
{
	return;
}
void sys_hal_psram_clk_sel_with_id(uint32_t id, uint32_t value)
{
	if (id == PSRAM_ID_0)
	{
		sys_ahbp_ll_set_reg8_cksel_pram0(value);
	}
	else
	{
		sys_ahbp_ll_set_reg9_cksel_pram1(value);
	}
}
void sys_hal_psram_set_clkdiv_with_id(uint32_t id, uint32_t value)
{
	if (id == PSRAM_ID_0)
	{
		sys_ahbp_ll_set_reg8_ckdiv_pram0(value);
	}
	else
	{
		sys_ahbp_ll_set_reg9_ckdiv_pram1(value);
	}
}

void sys_hal_psram_psldo_vsel(uint32_t value)
{
	return;
}
void sys_hal_psram_psldo_vset(uint32_t output_voltage, uint32_t is_add_200mv)
{
	(void)is_add_200mv;
	sys_ll_set_ana_reg14_vpsramsel(output_voltage); //1.2v-1.95v

	return;
}
void sys_hal_psram_psram0_disckg(uint32_t value)
{
	sys_ahbp_ll_set_rega_pram0_cken(value);
	return;
}
void sys_hal_psram_psram1_disckg(uint32_t value)
{
	sys_ahbp_ll_set_rega_pram1_cken(value);
	return;
}
void sys_hal_psram_disckg_with_id(uint32_t id, uint32_t value)
{
	if (id == PSRAM_ID_0)
	{
		sys_ahbp_ll_set_rega_pram0_cken(value);
	}
	else
	{
		sys_ahbp_ll_set_rega_pram1_cken(value);
	}
}
/**  psram End **/
/* REG_0x03:cpu_storage_connect_op_select->flash_sel:0: normal flash operation 1:flash download by spi,R/W,0x3[9]*/
uint32_t sys_hal_get_cpu_storage_connect_op_select_flash_sel(void)
{
	return sys_ll_get_cpu_storage_connect_op_select_flash_sel();
}

void sys_hal_set_cpu_storage_connect_op_select_flash_sel(uint32_t value)
{
	sys_ll_set_cpu_storage_connect_op_select_flash_sel(value);
}
__IRAM_SEC void sys_hal_set_sys2flsh_2wire(uint32_t value)
{
	return;
}
/** Ethernet start **/
#ifdef CONFIG_ETH
void sys_hal_enable_eth_int(uint32_t value)
{
	return;
}

void sys_hal_set_eth_clk_en(uint32_t value)
{
	sys_aonp_regd_t *r = (sys_aonp_regd_t *)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->auxs_enet_cken = value;
}
#endif
/** Ethernet End**/
void sys_hal_set_ana_trxt_tst_enable(uint32_t value)
{
	return;
}
void sys_hal_set_ana_scal_en(uint32_t value)
{
	return;
}
void sys_hal_set_ana_gadc_buf_ictrl(uint32_t value)
{
	return;
}
void sys_hal_set_ana_gadc_cmp_ictrl(uint32_t value)
{
	return;
}
void sys_hal_set_ana_pwd_gadc_buf(uint32_t value)
{
	return;
}
void sys_hal_set_ana_vref_sel(uint32_t value)
{
	return;
}
void sys_hal_set_ana_cb_cal_manu(uint32_t value)
{
    sys_ll_set_ana_reg5_bcal_en(value);
}
void sys_hal_set_ana_reg5_pwd_rosc_spi(uint32_t value)
{
    sys_ll_set_ana_reg5_pwd_rosc_spi(value);
}
void sys_hal_set_ana_cb_cal_trig(uint32_t value)
{
    sys_ll_set_ana_reg5_bcal_start(value);
}
void sys_hal_set_ana_vlsel_ldodig(uint32_t value)
{
	return;
}
void sys_hal_set_ana_vhsel_ldodig(uint32_t value)
{
	return;
}
void sys_hal_set_sdio_clk_en(uint32_t value)
{
	return;
}
void sys_hal_set_cpu0_sdio_int_en(uint32_t value)
{
	return;
}
void sys_hal_set_cpu1_sdio_int_en(uint32_t value)
{
	return;
}
void sys_hal_set_cpu2_sdio_int_en(uint32_t value)
{
	return;
}
void sys_hal_set_sdio_clk_div(uint32_t value)
{
	return;
}
uint32_t sys_hal_get_sdio_clk_div()
{
	return 0;
}
void sys_hal_set_sdio_clk_sel(uint32_t value)
{
	return;
}
uint32_t sys_hal_get_sdio_clk_sel()
{
	return 0;
}
void sys_hal_set_ana_vctrl_sysldo(uint32_t value)
{
	return;
}
void sys_hal_set_yuv_buf_clock_en(uint32_t value)
{
	return;
}
void sys_hal_set_h264_clock_en(uint32_t value)
{
	return;
}
void sys_hal_set_ana_cb_cal_manu_val(uint32_t value)
{
    sys_ll_set_ana_reg5_vbias(value);
}
void sys_hal_set_ana_reg11_apfms(uint32_t value)
{
	sys_ll_set_ana_reg12_apfms(value);
}
void sys_hal_set_ana_reg12_dpfms(uint32_t value)
{
	sys_ll_set_ana_reg13_dpfms(value);
}
static void sys_hal_delay(volatile uint32_t times)
{
	while(times--);
}

static uint32_t sys_hal_get_dco_unlock(uint32_t *unlockL, uint32_t *unlockH)
{
	uint32_t r7f = aon_pmu_ll_get_r7f_value();
	uint32_t dco_unlock_l = (r7f >> SYS_HAL_DCO_UNLOCK_L_BIT) & 0x1;
	uint32_t dco_unlock_h = (r7f >> SYS_HAL_DCO_UNLOCK_H_BIT) & 0x1;

	if (NULL != unlockL) {
		*unlockL = dco_unlock_l;
	}

	if (NULL != unlockH) {
		*unlockH = dco_unlock_h;
	}

	return (dco_unlock_l | dco_unlock_h) ? 1 : 0;
}

static void sys_hal_reset_dco_unlock_latch(void)
{
	sys_ll_set_ana_reg2_rst_unlock_dco(1);
	sys_ll_set_ana_reg2_rst_unlock_dco(0);
}

__IRAM_SEC void sys_hal_adjust_dco(void)
{
	uint32_t unlock, unlockL = 0, unlockH = 0;
	int32_t dco_band, count;

	unlock = sys_hal_get_dco_unlock(&unlockL, &unlockH);
	if (!unlock || (unlockL && unlockH)) {
		return;
	}

	dco_band = (int32_t)sys_ll_get_ana_reg7_bandmanual();
	for (count = 0; count <= SYS_HAL_DCO_BAND_MAX; count++) {
		if (unlockL) {
			if (dco_band >= SYS_HAL_DCO_BAND_MAX) {
				break;
			}
			dco_band++;
		} else if (unlockH) {
			if (dco_band <= 0) {
				break;
			}
			dco_band--;
		} else {
			break;
		}

		sys_ll_set_ana_reg7_bandmanual((uint32_t)dco_band);
		sys_hal_reset_dco_unlock_latch();
		timer_hal_early_delay_us(10);

		unlock = sys_hal_get_dco_unlock(&unlockL, &unlockH);
		if (!unlock || (unlockL && unlockH)) {
			break;
		}
	}
}

__IRAM_SEC void sys_hal_adjust_dpll(void)
{
    uint32_t unlock, unlockL = 0, unlockH = 0;
    int32_t dpll_band, count;

    unlock = aon_pmu_hal_get_dpll_unlock(&unlockL, &unlockH);
    //bk_printf("unlock=%d+%d=>%d,band=%d\n", unlockL, unlockH, unlock, sys_ana_ll_get_ana_reg1_bandmanual());
    if (!unlock) {
        return;
    }
    dpll_band = (int32_t)sys_ll_get_ana_reg7_bandmanual();
    for (count = 7; count > 0; count--) {
        if (unlockL) {
            dpll_band++;
        } else {
            dpll_band--;
        }
        if (dpll_band < 0) {
            break;
        } else if (dpll_band > 0x7F) {
            break;
        }
        sys_ll_set_ana_reg7_bandmanual(dpll_band);
    }
}
uint32_t sys_hal_cali_dpll(uint32_t first_time)
{
	uint32_t int_mask = sys_hal_int_group2_disable(CPU0_CORE_ID, DPLL_UNLOCK_INTERRUPT_CTRL_BIT);

	// Disable unlock detector briefly while calibrating DPLL.
	sys_ll_set_ana_reg1_cben(1);
	sys_ll_set_ana_reg7_manual(0);
	timer_hal_early_delay_us(20);

	sys_hal_cali_dpll_spi_detect_disable();

	if (first_time)
	{
		timer_hal_early_delay_us(3400);
	}
	else
	{
		timer_hal_early_delay_us(340);
	}

	sys_hal_cali_dpll_spi_detect_enable();

	if (first_time)
	{
		timer_hal_early_delay_us(3400);
	}
	else
	{
		timer_hal_early_delay_us(340);
	}

	// Toggle twice to avoid reading a stale calibration value.
	sys_hal_cali_dpll_spi_detect_disable();

	if (first_time)
	{
		timer_hal_early_delay_us(3400);
	}
	else
	{
		timer_hal_early_delay_us(340);
	}

	sys_hal_cali_dpll_spi_detect_enable();

	if (first_time)
	{
		timer_hal_early_delay_us(3400);
	}
	else
	{
		timer_hal_early_delay_us(340);
	}

	sys_ll_set_ana_reg7_bandmanual(aon_pmu_hal_get_dpll_band());
	sys_ll_set_ana_reg7_manual(1);
	sys_ll_set_ana_reg1_cben(0);

	if (int_mask & DPLL_UNLOCK_INTERRUPT_CTRL_BIT)
	{
		sys_hal_int_group2_enable(CPU0_CORE_ID, DPLL_UNLOCK_INTERRUPT_CTRL_BIT);
	}

	return BK_OK;
}
bk_err_t sys_hal_ap_clock_power_ctrl(power_module_state_t power_state)
{
	uint32_t regData = 0;
	if(power_state == POWER_MODULE_STATE_ON)
	{
		sys_ll_set_ana_reg10_spi_latch1v(1);
		sys_ll_set_ana_reg9_pwd_hsldo(1);
		bk_delay_us(20);
		sys_ll_set_ana_reg9_pwd_hsldo(0);
		bk_delay_us(200);
		sys_ll_set_ana_reg16_enhspw(1);
		bk_delay_us(200);
		sys_ll_set_ana_reg16_vcorehssel(0xA);//0.7+0.025*0xA=0.95v
		bk_delay_us(200);
		sys_ll_set_ana_reg10_spi_latch1v(0);

	#if 1
		regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
		regData &= ~((0x1F<<21)|(0x1<<19));
		regData |=  ((0x1F<<21)|(  0<<19));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		bk_delay_us(20);

		regData &= ~((0x1F<<21)|(0x1<<19));
		regData |=  ((0x1E<<21)|(  0<<19));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		bk_delay_us(20);

		regData &= ~((0x1F<<21)|(0x1<<19));
		regData |=  ((0x1C<<21)|(  0<<19));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		bk_delay_us(20);

		regData &= ~((0x1F<<21)|(0x1<<19));
		regData |=  ((0x18<<21)|(  0<<19));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		bk_delay_us(20);

		regData &= ~((0x1F<<21)|(0x1<<19));
		regData |=  ((0x10<<21)|(  0<<19));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		bk_delay_us(20);

		regData &= ~((0x1F<<21)|(0x1<<19));
		regData |=  ((0x00<<21)|(  0<<19));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		//bk_delay_us(20);
		regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
		regData &= ~((0x1<<18));
		regData |=  ((  1<<18));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
		bk_delay_us(20);

		/* PMU M55S Clk On*/
		regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
		regData &= ~((0x1<<20));
		regData |=  ((  1<<20));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);

		/* PMU M55S RstN On*/
		regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
		regData &= ~((0x1<<17));
		regData |=  ((  1<<17));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);

		/*Wait HS LDO RstN On*/
		while(!REG_READ(SOC_AON_PMU_REG_BASE + 0x74*4));

		/*PMU M55S ISO Off*/
		regData  = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
		regData &= ~((0x1<<16));
		regData |=  ((0<<16));
		REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);

#if CONFIG_SPE
		/*"M55S Access Secure*/
		regData  = REG_READ(SOC_PPRO_REG_BASE + 0xF*4);
		regData &= ~((0x1<<3)|(0x1<<2));
		regData |=  ((  0<<3)|(  0<<2));
		REG_WRITE(SOC_PPRO_REG_BASE + 0xF*4, regData);
#endif
		/*PSRAM Enable*/
		sys_ll_set_ana_reg14_enpsram(1);
		//bk_delay_us(10);
		/*M55S Memory EMA switch to 1*/
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x50*4,  (0x5A<<24) | (0x441<<10) | (0x241));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x50*4,  (0xA5<<24) | (0x441<<10) | (0x241));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x51*4,  (0x5A<<24) |               (0x901));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x51*4,  (0xA5<<24) |               (0x901));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x52*4,  (0x5A<<24) | (0x441<<10) | (0x241));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x52*4,  (0xA5<<24) | (0x441<<10) | (0x241));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x53*4,  (0x5A<<24) |               (0x901));
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x53*4,  (0xA5<<24) |               (0x901));
		bk_delay_us(10);
	#endif
		/*M55:Default enable all the clock source for bringup */
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0xA*4, 0xFFFFFFFF);

		/*M55 cpu freq and bus 480M, subbus 240M */
		regData = REG_READ(SOC_SYS_AHBP_REG_BASE + 0x8*4);
		regData |= 0x1 << 4;
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x8*4, regData);

		regData = REG_READ(SOC_SYS_AHBP_REG_BASE + 0x8*4);
		regData |= 0x0 << 2;
		regData |= 0x1 << 0;
		REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x8*4, regData);
		bk_delay_us(20);
	}
	else
	{
		//aon_pmu_ll_set_r2_m55_rstn(1); // rstn release
		//aon_pmu_ll_set_r2_m55_clk_en(0); // clk enable

		//sys_ll_set_ana_reg14_enpsram(0);

		sys_ahbp_ll_set_rege_pwd_m55(1);

		sys_ll_set_ana_reg10_spi_latch1v(1);
		sys_ll_set_ana_reg16_enhspw(0);
		sys_ll_set_ana_reg9_pwd_hsldo(1);
		sys_ll_set_ana_reg10_spi_latch1v(0);
	}

	return BK_OK;
}
static bk_err_t sys_hal_m55_clock_power_init()
{
	uint32_t regData = 0;
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_pwd_hsldo(1);
	//bk_delay_us(20);
	sys_ll_set_ana_reg9_pwd_hsldo(0);
	//bk_delay_us(200);
	sys_ll_set_ana_reg16_enhspw(1);
	//bk_delay_us(200);
	sys_ll_set_ana_reg16_vcorehssel(0xA);//0.7+0.025*0xA=0.95v
	//bk_delay_us(200);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	/* AON_PMU AP power sequence + PPRO AP secure access are secure-only
	 * registers. In the secure-boot flow TFM (ap_power_domain_on) owns the AP
	 * power-up, so the Non-Secure world (CONFIG_SPE=0) must not touch them; app /
	 * secure builds (CONFIG_SPE=1) still run the full sequence here. */
#if CONFIG_SPE
	regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
	regData &= ~((0x1F<<21)|(0x1<<19));
	regData |=  ((0x1F<<21)|(  0<<19));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);

	regData &= ~((0x1F<<21)|(0x1<<19));
	regData |=  ((0x1E<<21)|(  0<<19));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);

	regData &= ~((0x1F<<21)|(0x1<<19));
	regData |=  ((0x1C<<21)|(  0<<19));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);

	regData &= ~((0x1F<<21)|(0x1<<19));
	regData |=  ((0x18<<21)|(  0<<19));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);

	regData &= ~((0x1F<<21)|(0x1<<19));
	regData |=  ((0x10<<21)|(  0<<19));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);

	regData &= ~((0x1F<<21)|(0x1<<19));
	regData |=  ((0x00<<21)|(  0<<19));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);
	regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
	regData &= ~((0x1<<18));
	regData |=  ((  1<<18));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);
	//bk_delay_us(20);

	/* PMU M55S Clk On*/
	regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
	regData &= ~((0x1<<20));
	regData |=  ((  1<<20));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);

	/* PMU M55S RstN On*/
	regData = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
	regData &= ~((0x1<<17));
	regData |=  ((  1<<17));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);

	/*Wait HS LDO RstN On*/
	while(!REG_READ(SOC_AON_PMU_REG_BASE + 0x74*4));

	/*PMU M55S ISO Off*/
	regData  = REG_READ(SOC_AON_PMU_REG_BASE + 0x2*4);
	regData &= ~((0x1<<16));
	regData |=  ((0<<16));
	REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2*4, regData);

	/*"M55S Access Secure*/
	regData  = REG_READ(SOC_PPRO_REG_BASE + 0xF*4);
	regData &= ~((0x1<<3)|(0x1<<2));
	regData |=  ((  0<<3)|(  0<<2));
	REG_WRITE(SOC_PPRO_REG_BASE + 0xF*4, regData);
#endif
	/*PSRAM Enable*/
	sys_ll_set_ana_reg14_enpsram(1);
	//bk_delay_us(10);
	/*M55S Memory EMA switch to 1*/
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x50*4,  (0x5A<<24) | (0x441<<10) | (0x241));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x50*4,  (0xA5<<24) | (0x441<<10) | (0x241));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x51*4,  (0x5A<<24) |               (0x901));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x51*4,  (0xA5<<24) |               (0x901));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x52*4,  (0x5A<<24) | (0x441<<10) | (0x241));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x52*4,  (0xA5<<24) | (0x441<<10) | (0x241));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x53*4,  (0x5A<<24) |               (0x901));
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x53*4,  (0xA5<<24) |               (0x901));
	//bk_delay_us(10);
	/*M55:Default enable all the clock source for bringup */
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0xA*4, 0xFFFFFFFF);

	/*M55 cpu freq and bus 480M, subbus 240M */
	regData = REG_READ(SOC_SYS_AHBP_REG_BASE + 0x8*4);
	regData |= 0x1 << 4;
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x8*4, regData);

	regData = REG_READ(SOC_SYS_AHBP_REG_BASE + 0x8*4);
	regData |= 0x0 << 2;
	regData |= 0x1 << 0;
	REG_WRITE(SOC_SYS_AHBP_REG_BASE + 0x8*4, regData);
	//bk_delay_us(20);
	return BK_OK;
}
static void sys_hal_dpll_cpu_flash_time_early_init(uint32_t chip_id)
{
	uint32_t cksel_flash = 0;
	uint32_t ckdiv_flash = 0;
	/*Calibrate the dpll*/
	sys_hal_cali_dpll(1);

	/*Enable all the clock sources*/
	sys_ll_set_reserver_reg0xd_sig_240m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_320m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_480m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_160m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_120m_cken(1);

	/*Keep bootloader flash clock config when it is already 240M/(2 + 1).*/
	cksel_flash = sys_ll_get_cpu_clk_div_mode1_cksel_flash();
	ckdiv_flash = sys_ll_get_cpu_clk_div_mode1_ckdiv_flash();
	if ((cksel_flash != CKSEL_SYS_FLASH_240M) || (ckdiv_flash != 0x2)) {
		sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(0x2);
		sys_ll_set_cpu_clk_div_mode1_cksel_flash(CKSEL_SYS_FLASH_240M);
	}
	/*Default enable all the clock source for bringup */
	REG_WRITE(SYS_CPU_DEVICE_CLK_ENABLE_ADDR, 0xFFFFFFFF);
	REG_WRITE(SYS_RESERVER_REG0XD_ADDR, 0xFFFFFFFF);

	/*Set the cpu clock div:default:240M*/
	sys_ll_set_cpu_clk_div_mode1_ckdiv_core(0x1);
	sys_ll_set_cpu_clk_div_mode1_cksel_core(0x3);

}
static void sys_hal_dpll_cpu_flash_time_early_init_sleep(uint32_t chip_id)
{
	/*Calibrate the dpll*/
	sys_hal_cali_dpll(0);

	/*Enable all the clock sources*/
	sys_ll_set_reserver_reg0xd_sig_240m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_320m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_480m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_160m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_120m_cken(1);
}
static void sys_hal_pwd_rosc()
{
	return;
}
void sys_hal_early_init(void)
{
	uint32_t chip_id;
	uint32_t val;
	chip_id = aon_pmu_hal_get_chipid();

	sys_ll_set_ana_reg10_spi_latch1v(1);

	sys_hal_analog_set(ANALOG_REG0, 0xC1385B56);
	sys_hal_analog_set(ANALOG_REG5, 0x640F836C);

	val = sys_hal_analog_get(ANALOG_REG0);
	val |= 0x1 << 26;
	sys_hal_analog_set(ANALOG_REG0,val);

	val = sys_hal_analog_get(ANALOG_REG0);
	val &= ~(0x1 << 26);
	sys_hal_analog_set(ANALOG_REG0,val);

	sys_hal_analog_set(ANALOG_REG2, 0x04248050); //wangjian20221110 xtal=0x50
	sys_hal_analog_set(ANALOG_REG3, 0xC5F00B88); //ronghui20241226 <10>=1 for xtal
	sys_hal_analog_set(ANALOG_REG4, 0x9FC9A7F0);
#if CONFIG_SPE
	/* ana_reg9 carries the AP HS-LDO power-down bit (pwd_hsldo). On NS
	 * (CONFIG_SPE=0) TFM already brought the AP HS domain up; re-writing this
	 * table value (pwd_hsldo=1) powers it down and breaks PSRAM/AHBP, so only
	 * the app/secure monolithic build writes it. */
	sys_hal_analog_set(ANALOG_REG9, 0x57E627E6); //shuguang20241226 <8:6>=7 for EVM
#endif

	//if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_BK7259 & PM_CHIP_ID_MASK))
	{
		sys_hal_analog_set(ANALOG_REG10, 0x786BC867 | (0x1<<9));
		sys_hal_analog_set(ANALOG_REG11, 0xC3DD4587);//siqing20260202 bit[27:25] = 1 for Reduce BUCK frequency to 1 MHz
		sys_hal_analog_set(ANALOG_REG12, 0x346E9878);
		sys_hal_analog_set(ANALOG_REG13, 0x346E9858);//siqing20260330 bit[5]=0 per V2 sys_ana.ini 0x4d: dzcdcal (bit4)=1, dzcdmsel (bit5)=0; do not set both to 1. Low-power BUCK_L discontinuous-current reverse conduction caused abnormal measured efficiency.
		sys_hal_analog_set(ANALOG_REG14, 0x74E670EE);
		sys_hal_analog_set(ANALOG_REG15, 0);

#if CONFIG_SPE
		/* ana_reg16 carries the AP HS power-switch enable (enhspw) + vcorehssel.
		 * Same reason as ana_reg9: NS must not clobber TFM's HS-domain bring-up. */
		sys_hal_analog_set(ANALOG_REG16, 0x9E436000);
#endif
		sys_hal_analog_set(ANALOG_REG19, 0xEE1D8033);//tenglong20251231 bit[24:22] = 0 for evm;siqing20260202 bit[13:9] = 0 for Reduce buck ripple
	}

    sys_ll_set_ana_reg10_spi_latch1v(0);

	/*early init cpu flash time*/
	sys_hal_dpll_cpu_flash_time_early_init(chip_id);

	/*M55: clock power init*/
	#if !CONFIG_PM_ONLY_CP_ENABLE && !CONFIG_PM_AP_POWERDOWN_WHEN_LV
	sys_hal_m55_clock_power_init();
	#endif
}
void sys_hal_early_init_sleep(void)
{
	//uint32_t chip_id;
	uint32_t val;
	//chip_id = aon_pmu_hal_get_chipid();

	sys_ll_set_ana_reg10_spi_latch1v(1);

	sys_hal_analog_set(ANALOG_REG0, 0xC1385B56);
	sys_hal_analog_set(ANALOG_REG5, 0x640F836C);

	val = sys_hal_analog_get(ANALOG_REG0);
	val |= 0x1 << 26;
	sys_hal_analog_set(ANALOG_REG0,val);

	val = sys_hal_analog_get(ANALOG_REG0);
	val &= ~(0x1 << 26);
	sys_hal_analog_set(ANALOG_REG0,val);

	sys_hal_analog_set(ANALOG_REG2, 0x04248050); //wangjian20221110 xtal=0x50
	sys_hal_analog_set(ANALOG_REG3, 0xC5F00B88); //ronghui20241226 <10>=1 for xtal
	sys_hal_analog_set(ANALOG_REG4, 0x9FC9A7F0);
	sys_hal_analog_set(ANALOG_REG9, 0x57E627E6); //shuguang20241226 <8:6>=7 for EVM

	//if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_BK7259 & PM_CHIP_ID_MASK))
	{
		sys_hal_analog_set(ANALOG_REG10, 0x786BC867 | (0x1<<9));
		sys_hal_analog_set(ANALOG_REG11, 0xC3DD4587);//siqing20260202 bit[27:25] = 1 for Reduce BUCK frequency to 1 MHz
		sys_hal_analog_set(ANALOG_REG12, 0x346E9878);
		sys_hal_analog_set(ANALOG_REG13, 0x346E9858);//siqing20260330 bit[5]=0 per V2 sys_ana.ini 0x4d: dzcdcal (bit4)=1, dzcdmsel (bit5)=0; do not set both to 1. Low-power BUCK_L discontinuous-current reverse conduction caused abnormal measured efficiency.
		sys_hal_analog_set(ANALOG_REG14, 0xF4E670EE);
		sys_hal_analog_set(ANALOG_REG15, 0);

		sys_hal_analog_set(ANALOG_REG16, 0x9E436000);
		sys_hal_analog_set(ANALOG_REG19, 0xEE1D8033);//tenglong20251231 bit[24:22] = 0 for evm;siqing20260202 bit[13:9] = 0 for Reduce buck ripple
	}

    sys_ll_set_ana_reg10_spi_latch1v(0);

	/*early init cpu flash time*/
	//sys_hal_dpll_cpu_flash_time_early_init_sleep(chip_id);

}
void sys_hal_set_7816_int_en(uint32_t core_index, uint32_t value)
{
	return;
}
void sys_hal_set_scr_clk(uint32_t value)
{
	return;
}
void sys_hal_set_cpu0_rxevt_sel(uint32_t param)
{
	return;
}
void sys_hal_set_cpu1_rxevt_sel(uint32_t param)
{
	return;
}
void sys_hal_set_cpu2_rxevt_sel(uint32_t param)
{
	return;
}
void sys_hal_set_cpu_device_clk_enable_otp_cken(uint32_t value)
{
	return;
}
void sys_hal_set_cpu_power_sleep_wakeup_ticktimer_32k_enable(uint32_t value)
{
	return sys_ll_set_cpu_power_sleep_wakeup_cpu0_ticktimer_32k_enable(value);
}

bk_err_t sys_hal_cpu_freq_dump()
{
	uint32_t value_8 = REG_READ(PM_SYS_REG_0x8);
	uint32_t cksel_core = value_8 & 0x3;
	uint32_t ckdiv_core = (value_8 >> 2) & 0xF;
	uint32_t cp0_div = ckdiv_core + 1;

	switch (cksel_core) {
	case PM_CLKSEL_CORE_26M:
		os_printf("Cur freq: CP:(26/%d)M\r\n", cp0_div);
		break;
	case PM_CLKSEL_CORE_DCO:
		os_printf("Cur freq: CP:(240/%d)M\r\n", cp0_div);
		break;
	case PM_CLKSEL_CORE_320M:
		os_printf("Cur freq: CP:(320/%d)M\r\n", cp0_div);
		break;
	case PM_CLKSEL_CORE_480M:
		os_printf("Cur freq: CP:(480/%d)M\r\n", cp0_div);
		break;
	default:
		break;
	}
	os_printf("Freq_reg:0x%x\r\n", value_8);

	return BK_OK;
}

bk_err_t sys_hal_xdac_clk_en(uint8_t ch, uint8_t clk_en)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        sys_ll_set_reserver_reg0xd_xdac0_cken(clk_en);
    }
    else if(XDAC_1 == ch)
    {
        sys_ll_set_reserver_reg0xd_xdac1_cken(clk_en);
    }
    else
    {
        ret = BK_ERR_XDAC_CH_INVALID;
    }

    return ret;

}

bk_err_t sys_hal_xdac_int_en(uint8_t cpu, uint8_t ch, uint8_t int_en)
{
    bk_err_t ret = BK_OK;

    if(0 == cpu)
    {
        if(XDAC_0 == ch)
        {
            sys_ll_set_cpu0_int_32_63_en_cpu0_xdac0_int_en(int_en);
        }
        else if(XDAC_1 == ch)
        {
            sys_ll_set_cpu0_int_32_63_en_cpu0_xdac1_int_en(int_en);
        }
        else
        {
            ret = BK_FAIL;
        }
    }
    else if(1 == cpu)
    {
        if(XDAC_0 == ch)
        {
            sys_ll_set_cpu1_int_32_63_en_cpu1_xdac0_int_en(int_en);
        }
        else if(XDAC_1 == ch)
        {
            sys_ll_set_cpu1_int_32_63_en_cpu1_xdac1_int_en(int_en);
        }
        else
        {
            ret = BK_FAIL;
        }
    }
    else
    {
        ret = BK_FAIL;
    }

    return ret;

}

bk_err_t sys_hal_xdac_set_enspi(uint8_t ch, uint8_t enspi)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        sys_ll_set_ana_reg39_enspi_i(enspi);
    }
    else if(XDAC_1 == ch)
    {
        sys_ll_set_ana_reg40_enspi_q(enspi);
    }
    else
    {
        ret = BK_FAIL;
    }

    return ret;

}

bk_err_t sys_hal_xdac_set_endigspi_sel(uint8_t ch, uint8_t endigspi_sel)
{
    bk_err_t ret = BK_OK;

    if(XDAC_0 == ch)
    {
        sys_ll_set_ana_reg39_endigspi_sel_i(endigspi_sel);
    }
    else if(XDAC_1 == ch)
    {
        sys_ll_set_ana_reg40_endigspi_sel_q(endigspi_sel);
    }
    else
    {
        ret = BK_FAIL;
    }

    return ret;

}

/** reg20 - AHB Peripheral Bus Master QoS Control **/

void sys_hal_set_psram_qos_value(uint32_t v)
{
	sys_ahbp_ll_set_reg20_value(v);
}

uint32_t sys_hal_get_psram_qos_value(void)
{
	return sys_ahbp_ll_get_reg20_value();
}

void sys_hal_set_psram_cpu0_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_cpu0_qos(v);
}

uint32_t sys_hal_get_psram_cpu0_qos(void)
{
	return sys_ahbp_ll_get_reg20_cpu0_qos();
}

void sys_hal_set_psram_dma1_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_dma1_qos(v);
}

uint32_t sys_hal_get_psram_dma1_qos(void)
{
	return sys_ahbp_ll_get_reg20_dma1_qos();
}

void sys_hal_set_psram_sdio0_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_sdio0_qos(v);
}

uint32_t sys_hal_get_psram_sdio0_qos(void)
{
	return sys_ahbp_ll_get_reg20_sdio0_qos();
}

void sys_hal_set_psram_sdio1_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_sdio1_qos(v);
}

uint32_t sys_hal_get_psram_sdio1_qos(void)
{
	return sys_ahbp_ll_get_reg20_sdio1_qos();
}

/* NOTE: enet0 QoS is marked as INVALID in hardware spec. Do NOT use. */
void sys_hal_set_psram_enet0_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_enet0_qos(v);
}

uint32_t sys_hal_get_psram_enet0_qos(void)
{
	return sys_ahbp_ll_get_reg20_enet0_qos();
}

/* NOTE: enet1 QoS is marked as INVALID in hardware spec. Do NOT use. */
void sys_hal_set_psram_enet1_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_enet1_qos(v);
}

uint32_t sys_hal_get_psram_enet1_qos(void)
{
	return sys_ahbp_ll_get_reg20_enet1_qos();
}

void sys_hal_set_psram_usb_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_usb_qos(v);
}

uint32_t sys_hal_get_psram_usb_qos(void)
{
	return sys_ahbp_ll_get_reg20_usb_qos();
}

void sys_hal_set_psram_h26e_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_h26e_qos(v);
}

uint32_t sys_hal_get_psram_h26e_qos(void)
{
	return sys_ahbp_ll_get_reg20_h26e_qos();
}

void sys_hal_set_psram_isp_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_isp_qos(v);
}

uint32_t sys_hal_get_psram_isp_qos(void)
{
	return sys_ahbp_ll_get_reg20_isp_qos();
}

void sys_hal_set_psram_videopost_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_videopost_qos(v);
}

uint32_t sys_hal_get_psram_videopost_qos(void)
{
	return sys_ahbp_ll_get_reg20_videopost_qos();
}

void sys_hal_set_psram_dpu_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_dpu_qos(v);
}

uint32_t sys_hal_get_psram_dpu_qos(void)
{
	return sys_ahbp_ll_get_reg20_dpu_qos();
}

void sys_hal_set_psram_cbus_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_cbus_qos(v);
}

uint32_t sys_hal_get_psram_cbus_qos(void)
{
	return sys_ahbp_ll_get_reg20_cbus_qos();
}

void sys_hal_set_psram_npu0_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_npu0_qos(v);
}

uint32_t sys_hal_get_psram_npu0_qos(void)
{
	return sys_ahbp_ll_get_reg20_npu0_qos();
}

void sys_hal_set_psram_npu1_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_npu1_qos(v);
}

uint32_t sys_hal_get_psram_npu1_qos(void)
{
	return sys_ahbp_ll_get_reg20_npu1_qos();
}

void sys_hal_set_psram_cpu1_qos(uint32_t v)
{
	sys_ahbp_ll_set_reg20_cpu1_qos(v);
}

uint32_t sys_hal_get_psram_cpu1_qos(void)
{
	return sys_ahbp_ll_get_reg20_cpu1_qos();
}
