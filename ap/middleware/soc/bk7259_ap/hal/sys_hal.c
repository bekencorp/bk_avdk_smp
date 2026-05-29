// Copyright 2020-2026 Beken
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
#include "gpio_hal_v2px.h"
#include "gpio_driver_base.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <driver/hal/hal_spi_types.h>
#include <driver/hal/hal_qspi_types.h>
#include <gpio_driver.h>
#include "timer_hal.h"
#include <driver/pwr_clk.h>
#include "bk_misc.h"
#include "cpu_id.h"

#define PM_CLKSEL_CORE_320M                 (2)
#define PM_CLKSEL_CORE_480M                 (3)
#define PM_CLKSEL_FLASH_480M                (0x1)
#define PM_CLKDIV_CORE_0                    (0)
#define PM_CLKDIV_CORE_1                    (1)
#define PM_VDDD_H_VOL_1V                    (0x6)
#define PM_VDDDIG_H_VOL_0v9                 (0xC)
#define PM_CLKDV_CPU1_1                     (0x1)
#define PM_CLKDV_CPU0_0                     (0x0)
#define SYS_SWITCH_VDDDIG_VOL_DELAY_TIME    (2600)

static sys_hal_t s_sys_hal;
// static pm_cpu_freq_e s_pre_cpu_freq = PM_CPU_FRQ_120M;
uint32 sys_hal_get_int_group2_status(uint32_t core_id);
bk_err_t sys_hal_ctrl_vddd_h_vol(uint32_t vol_value);
bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value);
uint32_t sys_hal_vdddig_h_vol_get();
static void sys_hal_delay(volatile uint32_t times);
extern void rtos_setup_systick(uint32_t cpu_clock_hz);

bk_err_t sys_hal_init()
{
	s_sys_hal.hw = (sys_hw_t *)SYS_LL_REG_BASE;
	return BK_OK;
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
}

void sys_hal_set_ioldo_bypass(uint32_t v)
{
	return;
}

void sys_hal_set_ioldo_volt(uint32_t v)
{
	return;
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

__IRAM_SEC void sys_hal_module_power_ctrl(power_module_name_t module,power_module_state_t power_state)
{
	if(POWER_MODULE_NAME_BASE1 < module && module <= POWER_DOMAIN_NAME_NPU)
	{
		switch(module)
		{
			case POWER_DOMAIN_NAME_AP_CPU:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ahbp_ll_set_rege_pwd_m55(0);
				}
				else
				{
					//sys_ahbp_ll_set_rege_pwd_m55(1);//ap can not directly power off AP
				}
				break;
			case POWER_DOMAIN_NAME_VIDEO_POST:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ahbp_ll_set_rege_pwd_video_post(0);
				}
				else
				{
					sys_ahbp_ll_set_rege_pwd_video_post(1);
				}
				break;
			case POWER_DOMAIN_NAME_H26E:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ahbp_ll_set_rege_pwd_h26e(0);
				}
				else
				{
					sys_ahbp_ll_set_rege_pwd_h26e(1);
				}
				break;
			case POWER_DOMAIN_NAME_ISP:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ahbp_ll_set_rege_pwd_isp(0);
				}
				else
				{
					sys_ahbp_ll_set_rege_pwd_isp(1);
				}
				break;
			case POWER_DOMAIN_NAME_NPU:
				if(power_state == POWER_MODULE_STATE_ON)
				{
					sys_ahbp_ll_set_rege_pwd_npu(0);
				}
				else
				{
					sys_ahbp_ll_set_rege_pwd_npu(1);
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
			case POWER_DOMAIN_NAME_AP_CPU:
				return sys_ahbp_ll_get_rege_pwd_m55();
				break;
			case POWER_DOMAIN_NAME_VIDEO_POST:
				return sys_ahbp_ll_get_rege_pwd_video_post();
				break;
			case POWER_DOMAIN_NAME_H26E:
				return sys_ahbp_ll_get_rege_pwd_h26e();
				break;
			case POWER_DOMAIN_NAME_ISP:
				return sys_ahbp_ll_get_rege_pwd_isp();
				break;
			case POWER_DOMAIN_NAME_NPU:
				return sys_ahbp_ll_get_rege_pwd_npu();
				break;
			default:
				break;
		}
	}
	return 0;
}

int sys_hal_rosc_calibration(uint32_t rosc_cali_mode, uint32_t cali_interval)
{
	return 0;
}

int sys_hal_rosc_test_mode(bool enabled)
{
	return 0;
}

void sys_hal_module_RF_power_ctrl (module_name_t module,power_module_state_t power_state)
{
	return;
}

void sys_hal_cpu0_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state)
{
	sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask(clock_state);
}

void sys_hal_cpu1_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state)
{
	// CPU0 and CPU1 share the same interrupt mask register
	sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask(clock_state);
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

void sys_hal_set_npu_reset(uint32_t reset_value)
{
	sys_ahbp_ll_set_reg6_npu_sw_rstn(reset_value);
}

void sys_hal_set_cpu1_pwr_dw(uint32_t is_pwr_down)
{
	sys_ll_set_cpu1_int_halt_clk_op_cpu1_pwr_dw(is_pwr_down);
}

void sys_hal_set_cpu2_pwr_dw(uint32_t is_pwr_down)
{
	// CPU2 (M55 sub-core) does not support pwr_dw control in sys_ahbp_reg4
	// Power control for CPU2 is handled through aon_pmu registers
	(void)is_pwr_down;
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
	if (core_index == 0) {
		sys_ll_set_cpu_clk_div_mode1_ckdiv_core(value);
	} else if (core_index == 1) {
		// CPU1 clock divider is controlled by the same register
		sys_ll_set_cpu_clk_div_mode1_ckdiv_core(value);
	}
}

uint32_t sys_hal_cpu_clk_div_get(uint32_t core_index)
{
	return sys_ll_get_cpu_clk_div_mode1_ckdiv_core();
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
	return 0;
}
uint32_t sys_hal_lp_vol_get()
{
	return 0;
}
int32 sys_hal_rf_tx_vol_set(uint32_t value)
{
	return 0;
}
uint32_t sys_hal_rf_tx_vol_get()
{
	return 0;
}
int32 sys_hal_rf_rx_vol_set(uint32_t value)
{
	return 0;
}
uint32_t sys_hal_rf_rx_vol_get()
{
	return 0;
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
	uint32_t clk_param  = 0;
	bool is_480m    = false;
	bool is_320m    = false;
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
	// if(((cksel_core == PM_CLKSEL_CORE_320M)&&(ckdiv_core == PM_CLKDIV_CORE_0))||((cksel_core == PM_CLKSEL_CORE_480M)&&(ckdiv_core == PM_CLKDIV_CORE_0)))
	// {
	// 	os_printf("unsupport the cpu freq setting %d %d \r\n",cksel_core,ckdiv_core);
	// 	return BK_FAIL;
	// }

	clk_param = 0;
	clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
	if(((clk_param & 0x3) == 0x1) && ((clk_param & BIT(4)) != 0) && (((clk_param >> 2) & 0x3) == 0x0))
	{
		is_480m = true;
	}
	if(((clk_param & 0x3) == 0x2) && ((clk_param & BIT(4)) != 0) && (((clk_param >> 2) & 0x3) == 0x0))
	{
		is_320m = true;
	}

	if(is_480m || is_320m)//when it from the higher frequency to lower frequency
	{
		/*1.core clk select*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x3 << 0);
		clk_param |=  cksel_core << 0;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*2.config core clk div*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x3 << 2);
		clk_param |=  ckdiv_core << 2;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*3.config bus clk div*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x1 << 4);
		clk_param |=  ckdiv_bus << 4;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);
	}
	else//when it from the lower frequency to higher frequency
	{
		/*1.config bus*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x1 << 4);
		clk_param |=  ckdiv_bus << 4;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*2.config core clk div*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x3 << 2);
		clk_param |=  ckdiv_core << 2;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);

		/*3.core clk select*/
		clk_param = 0;
		clk_param = sys_hal_all_modules_clk_div_get(CLK_DIV_REG0);
		clk_param &=  ~(0x3 << 0);
		clk_param |=  cksel_core << 0;
		sys_hal_all_modules_clk_div_set(CLK_DIV_REG0,clk_param);
	}

	return BK_OK;
}

bk_err_t sys_hal_ctrl_vddd_h_vol(uint32_t vol_value)
{
	return 0;
}

bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value)
{
	if(sys_ll_get_ana_reg10_vcorehsel() != vol_value)
	{
		sys_ll_set_ana_reg10_spi_latch1v(1);
		sys_ll_set_ana_reg10_vcorehsel(vol_value);
		sys_ll_set_ana_reg10_spi_latch1v(0);
		sys_hal_delay(SYS_SWITCH_VDDDIG_VOL_DELAY_TIME);//when cpu0 max freq 240m ,it delay 10uS for voltage stability
	}

	return BK_OK;
}

uint32_t sys_hal_vdddig_h_vol_get()
{
	return sys_ll_get_ana_reg10_vcorehsel();
}
bk_err_t sys_hal_switch_cpu_bus_freq_high_to_low(pm_cpu_freq_e cpu_bus_freq)
{
	bk_err_t ret = BK_OK;
	switch(cpu_bus_freq)
	{
		case PM_CPU_FRQ_480M://cpu0:480m;bus:240m
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x0,0x1,0x0,0x0);
			rtos_setup_systick(480000000);
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_high_speed();
			break;
		case PM_CPU_FRQ_320M://cpu0:320m;bus:160m
			ret = sys_hal_core_bus_clock_ctrl(0x2,0x1,0x1,0x0,0x0);
			rtos_setup_systick(320000000);
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_high_speed();
			break;
		case PM_CPU_FRQ_240M://cpu0:240m;cpu0 bus:240m
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x1,0x0,0x0,0x0);
			rtos_setup_systick(240000000);
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0 v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_high_speed();
			break;
		case PM_CPU_FRQ_160M://cpu0:160m;bus:160m
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x2,0x0,0x0,0x0);
			rtos_setup_systick(160000000);
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_low_speed();
			break;
		case PM_CPU_FRQ_120M://cpu0:120m;bus:120m
			#if CONFIG_DCO_CLK_ENABLE
			ret = sys_hal_core_bus_clock_ctrl(0x3,0x1,0x0,0x0,0x0);
			#else
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x3,0x0,0x0,0x0);
			#endif
			rtos_setup_systick(120000000);
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0V
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_low_speed();
			break;
		case PM_CPU_FRQ_80M://cpu0:80m;bus:80m
			#if CONFIG_DCO_CLK_ENABLE
			ret = sys_hal_core_bus_clock_ctrl(0x3,0x2,0x0,0x0,0x0);
			#else
			ret = sys_hal_core_bus_clock_ctrl(0x0,0x1,0x0,0x0,0x0);
			#endif
			rtos_setup_systick(80000000);
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0V
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_low_speed();
			break;
		default:
			break;
	}

	return ret;
}

bk_err_t sys_hal_switch_cpu_bus_freq_low_to_high(pm_cpu_freq_e cpu_bus_freq)
{
	bk_err_t ret = BK_OK;
	switch(cpu_bus_freq)
	{
		case PM_CPU_FRQ_480M://cpu0:480m;bus:240m
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_high_speed();
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x0,0x1,0x0,0x0);
			rtos_setup_systick(480000000);
			break;
		case PM_CPU_FRQ_320M://cpu0:320m;bus:160m
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_high_speed();
			ret = sys_hal_core_bus_clock_ctrl(0x2,0x1,0x1,0x0,0x0);
			rtos_setup_systick(320000000);
			break;
		case PM_CPU_FRQ_240M://cpu0:240m;bus:240m
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_high_speed();
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x1,0x0,0x0,0x0);
			rtos_setup_systick(240000000);
			break;
		case PM_CPU_FRQ_160M://cpu0:160m;bus:160m
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0v
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_low_speed();
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x2,0x0,0x0,0x0);
			rtos_setup_systick(160000000);
			break;
		case PM_CPU_FRQ_120M://cpu0:120m;bus:120m
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0V
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_low_speed();
			#if CONFIG_DCO_CLK_ENABLE
			ret = sys_hal_core_bus_clock_ctrl(0x3,0x1,0x0,0x0,0x0);
			#else
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x3,0x0,0x0,0x0);
			#endif
			rtos_setup_systick(120000000);
			break;
		case PM_CPU_FRQ_80M://cpu0:80m;bus:80m
			sys_hal_ctrl_vddd_h_vol(0x6);// 1.0V
			sys_hal_ctrl_vdddig_h_vol(0x8);//0.9V
			sys_hal_set_ram_low_speed();
			#if CONFIG_DCO_CLK_ENABLE
			ret = sys_hal_core_bus_clock_ctrl(0x1,0x2,0x0,0x0,0x0);
			#else
			ret = sys_hal_core_bus_clock_ctrl(0x0,0x1,0x0,0x0,0x0);
			#endif
			rtos_setup_systick(80000000);
			break;
		default:
			break;
	}
	return ret;
}

static pm_cpu_freq_e s_pre_cpu_freq = PM_CPU_FRQ_XTAL;
bk_err_t sys_hal_switch_cpu_bus_freq(pm_cpu_freq_e cpu_bus_freq)
{
	bk_err_t ret = BK_OK;
	pm_cpu_freq_e prev_freq = s_pre_cpu_freq;

	if(prev_freq == cpu_bus_freq)
		return BK_OK;

	if(prev_freq > cpu_bus_freq)// eg: 480->60
	{
		sys_hal_switch_cpu_bus_freq_high_to_low(cpu_bus_freq);
	}
	else // eg: 60-480
	{
		sys_hal_switch_cpu_bus_freq_low_to_high(cpu_bus_freq);
	}
	s_pre_cpu_freq = cpu_bus_freq;
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

void sys_hal_set_ram_sph_cfg(uint32_t value)
{
	sys_ahbp_ll_set_reg50_value(value);
}

void sys_hal_set_ram_tph_cfg(uint32_t value)
{
	sys_ahbp_ll_set_reg51_value(value);
}

void sys_hal_set_ram_spl_cfg(uint32_t value)
{
	sys_ahbp_ll_set_reg52_value(value);
}

void sys_hal_set_ram_tpl_cfg(uint32_t value)
{
	sys_ahbp_ll_set_reg53_value(value);
}

void sys_hal_set_ram_high_speed(void)
{

	/* When running at high frequency (more than half of the maximum frequency),
	 * RAM CFG parameters need to be modified. Please modify the configuration
	 * in the initialization program. This RAM parameter affects RAM speed and
	 * yield. RAM library recommends using high-speed configuration for
	 * high-speed operation and low-speed configuration for low-speed operation.
	 */
     uint32_t ram_sph_cfg = 0x0;//Default: 0x442 << 10 | 0x242;
     ram_sph_cfg = 0x441 << 10 | 0x241;
     sys_hal_set_ram_sph_cfg(0x5A << 24 | ram_sph_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_sph_cfg(0xA5 << 24 | ram_sph_cfg); ///Default: Low-speed configuration

     uint32_t ram_tph_cfg = 0x0;//Default: 0x902;
     ram_tph_cfg = 0x901;
     sys_hal_set_ram_tph_cfg(0x5A << 24 | ram_tph_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_tph_cfg(0xA5 << 24 | ram_tph_cfg); ///Default: Low-speed configuration

	 uint32_t ram_spl_cfg = 0x0;//Default: 0x444 << 10 | 0x244;
     ram_spl_cfg = 0x441 << 10 | 0x241;
     sys_hal_set_ram_spl_cfg(0x5A << 24 | ram_spl_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_spl_cfg(0xA5 << 24 | ram_spl_cfg); ///Default: Low-speed configuration

     uint32_t ram_tpl_cfg = 0x0;//Default: 0x904;
     ram_tpl_cfg = 0x901;
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
     uint32_t ram_sph_cfg = 0x0;//Default: 0x442 << 10 | 0x242;
     ram_sph_cfg = 0x442 << 10 | 0x242;
     sys_hal_set_ram_sph_cfg(0x5A << 24 | ram_sph_cfg);
     sys_hal_set_ram_sph_cfg(0xA5 << 24 | ram_sph_cfg);

     uint32_t ram_tph_cfg = 0x0;//Default: 0x902;
     ram_tph_cfg = 0x902;
     sys_hal_set_ram_tph_cfg(0x5A << 24 | ram_tph_cfg);
     sys_hal_set_ram_tph_cfg(0xA5 << 24 | ram_tph_cfg);

	 uint32_t ram_spl_cfg = 0x0;//Default: 0x444 << 10 | 0x244;
     ram_spl_cfg = 0x444 << 10 | 0x244;
     sys_hal_set_ram_spl_cfg(0x5A << 24 | ram_spl_cfg); ///Default: Low-speed configuration
     sys_hal_set_ram_spl_cfg(0xA5 << 24 | ram_spl_cfg); ///Default: Low-speed configuration

     uint32_t ram_tpl_cfg = 0x0;//Default: 0x904;
     ram_tpl_cfg = 0x904;
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


__IRAM_SEC int32_t sys_hal_set_m55sub_int_en(uint32_t int_num, uint32_t int_en)
{
	uint32_t reg = 0;
	uint32_t value = 0;

	if (int_num < 32) {
		reg = sys_ll_get_m55sub_int_0_31_en_m55sub_inten();
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		sys_ll_set_m55sub_int_0_31_en_m55sub_inten(reg);
	} else if (int_num < 64) {
		reg = sys_ll_get_m55sub_int_32_63_en_m55sub_inten();
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		sys_ll_set_m55sub_int_32_63_en_m55sub_inten(reg);
	} else if (int_num < INT_SRC_CP_MAX_NUM) {
		reg = sys_ll_get_m55sub_int_64_95_en_m55sub_inten();
		value = reg;
		if (int_en) {
			reg |= BIT(int_num % 32);
		} else {
			reg &= ~BIT(int_num % 32);
		}
		sys_ll_set_m55sub_int_64_95_en_m55sub_inten(reg);
	}
	return value;
}

__IRAM_SEC int32_t sys_hal_get_m55sub_int_en()
{
	uint32_t reg = 0;

	reg = sys_ll_get_m55sub_int_0_31_en_m55sub_inten();
	if (reg & 0xFFFFFFFF) {
		return 1;
	}

	reg = sys_ll_get_m55sub_int_32_63_en_m55sub_inten();
	if (reg & 0xFFFFFFFF) {
		return 1;
	}

	reg = sys_ll_get_m55sub_int_64_95_en_m55sub_inten();
	if (reg & 0x3FFFF) {
		return 1;
	}

	return 0;
}


__IRAM_SEC int32_t sys_hal_get_m55sub_int_status0(void)
{
	uint32_t value = 0;
	value = sys_ll_get_m55sub_int_0_31_status_m55sub_status0();
	return value;
}

__IRAM_SEC int32_t sys_hal_get_m55sub_int_status1(void)
{
	uint32_t value = 0;
	value = sys_ll_get_m55sub_int_32_63_status_m55sub_status1();
	return value;
}

__IRAM_SEC int32_t sys_hal_get_m55sub_int_status2(void)
{
	uint32_t value = 0;
	value = sys_ll_get_m55sub_int_64_95_status_m55sub_status2();
	return value;
}

///TODO: To be removed
__IRAM_SEC int32 sys_hal_int_disable(uint32_t core_index, uint32 param) //CMD_ICU_INT_DISABLE
{
	uint32 reg = 0;
	uint32 value = 0;
	if(core_index == 0)//cpu0
	{
		reg = sys_ll_get_cpu0_int_0_31_en_cpu0_inten();
		value = reg;
		reg &= ~(param);
		sys_ll_set_cpu0_int_0_31_en_cpu0_inten(reg);
	}
	else
	{
		reg = sys_ll_get_cpu1_int_0_31_en_cpu1_inten();
		value = reg;
		reg &= ~(param);
		sys_ll_set_cpu1_int_0_31_en_cpu1_inten(reg);
	}
	return value;
}

///TODO: To be removed
__IRAM_SEC int32 sys_hal_int_enable(uint32_t core_index,uint32 param) //CMD_ICU_INT_ENABLE
{
	uint32 reg = 0;
	if(core_index == 0)//cpu0
	{
		reg = sys_ll_get_cpu0_int_0_31_en_cpu0_inten();
		reg |= (param);
		sys_ll_set_cpu0_int_0_31_en_cpu0_inten(reg);
	}
	else
	{
		reg = sys_ll_get_cpu1_int_0_31_en_cpu1_inten();
		reg |= (param);
		sys_ll_set_cpu1_int_0_31_en_cpu1_inten(reg);
	}
	return 0;
}

///TODO: To be removed
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

///TODO: To be removed
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

///TODO: To be removed
int32 sys_hal_core_int_group1_disable(uint32_t core_id, uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_core_int_group1_enable(uint32_t core_id, uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_core_int_group2_disable(uint32_t core_id, uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_core_int_group2_enable(uint32_t core_id, uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_fiq_disable(uint32_t core_index,uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_fiq_enable(uint32_t core_index,uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_global_int_disable(uint32 param)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_global_int_enable(uint32 param)
{
	return 0;
}

///TODO: To be removed
uint32 sys_hal_get_int_status(uint32_t core_index)
{
	return 0;
}

///TODO: To be removed
__attribute__((section(".itcm_sec_code"))) uint32 sys_hal_get_int_group2_status(uint32_t core_index)
{
	return 0;
}

///TODO: To be removed
uint32_t sys_hal_get_cpu0_gpio_int_st(void)
{
	return 0;
}

///TODO: To be removed
int32 sys_hal_set_int_status(uint32 param)
{
	return 0;
}

///TODO: To be removed
uint32 sys_hal_get_fiq_reg_status(uint32_t core_index)
{
	return 0;
}

///TODO: To be removed
uint32 sys_hal_set_fiq_reg_status(uint32 param)
{
	return 0;
}

///TODO: To be removed
uint32 sys_hal_get_intr_raw_status(void)
{
	return 0;
}

///TODO: To be removed
uint32 sys_hal_set_intr_raw_status(uint32 param)
{
	return 0;
}

int32 sys_hal_set_jtag_mode(uint32 param)
{
	//BK7259 need not set
	return BK_OK;
}

uint32 sys_hal_get_jtag_mode(void)
{
	//BK7259 need not get
	return BK_OK;
}

__IRAM_SEC void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up)
{
	uint32_t reg_val = 0;

	if (dev < 32) {
		reg_val = sys_ll_get_cpu_device_clk_enable_value();
		if (power_up == CLK_PWR_CTRL_PWR_UP) {
			reg_val |= BIT(dev);
		} else {
			reg_val &= ~BIT(dev);
		}
		sys_ll_set_cpu_device_clk_enable_value(reg_val);
	} else if (dev < 64) {
		reg_val = sys_ll_get_reserver_reg0xd_value();
		if (power_up == CLK_PWR_CTRL_PWR_UP) {
			reg_val |= BIT(dev - 32);
		} else {
			reg_val &= ~BIT(dev - 32);
		}
		sys_ll_set_reserver_reg0xd_value(reg_val);
	} else {
		reg_val = sys_ahbp_ll_get_rega_value();
		if (power_up == CLK_PWR_CTRL_PWR_UP) {
			reg_val |= BIT(dev - 64);
		} else {
			reg_val &= ~BIT(dev - 64);
		}
		sys_ahbp_ll_set_rega_value(reg_val);
	}
}

void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode)
{
	int sel_xtal = 0;
	int sel_appl = 1;

	switch(id)
	{
		case UART_ID_0:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode2_cksel_uart0(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode2_cksel_uart0(sel_xtal);
				break;
			}
		case UART_ID_1:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode2_cksel_uart1(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode2_cksel_uart1(sel_xtal);
				break;
			}
		case UART_ID_2:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode2_cksel_uart2(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode2_cksel_uart2(sel_xtal);
				break;
			}
		case UART_ID_3:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode2_cksel_uart3(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode2_cksel_uart3(sel_xtal);
				break;
			}
		case UART_ID_4:
			{
				if(mode == UART_SCLK_APLL)
					sys_ll_set_cpu_clk_div_mode2_cksel_uart4(sel_appl);
				else
					sys_ll_set_cpu_clk_div_mode2_cksel_uart4(sel_xtal);
				break;
			}
		case UART_ID_5:
			{
				sys_ahbp_ll_set_reg8_ckdiv_uart5(0); //120M/(N+1)
				break;
			}
		default:
			break;
	}
}

void sys_hal_i2c_select_clock(i2c_id_t id, i2c_src_clk_t mode)
{
	uint32_t sel_value = 0;

	switch (mode) {
		case I2C_SCLK_XTAL:
			sel_value = 0;
			break;
		case I2C_SCLK_120M:
			sel_value = 1;
			break;
		default:
			break;
	}

	switch (id) {
		case I2C_ID_0:
			sys_ll_set_cpu_clk_div_mode2_cksel_i2c0(sel_value);
			break;
#if (SOC_I2C_UNIT_NUM > 1)
		case I2C_ID_1:
			sys_ll_set_cpu_clk_div_mode2_cksel_i2c3(sel_value);
			break;
#endif
		default:
			break;
	}
}

void sys_hal_pwm_select_clock(sys_sel_pwm_t num, pwm_src_clk_t mode)
{
	uint32_t sel_value = 0;

	switch (mode) {
		case PWM_SCLK_XTAL:
			sel_value = 1;
			break;
		case PWM_SCLK_CLK32:
		default:
			sel_value = 0;
			break;
	}

	switch (num) {
		case SYS_SEL_PWM0:
			sys_ll_set_cpu_clk_div_mode3_cksel_pwm0(sel_value);
			break;
		default:
			break;
	}
}

void sys_hal_timer_select_clock(sys_sel_timer_t num, timer_src_clk_t mode)
{
	uint32_t sel_value = 0;

	switch (mode) {
		case TIMER_SCLK_XTAL:
			sel_value = 1;
			break;
		case TIMER_SCLK_CLK32:
		default:
			sel_value = 0;
			break;
	}

	switch (num) {
		case SYS_SEL_TIMER0:
			sys_ll_set_cpu_clk_div_mode2_cksel_tim0(sel_value);
			break;
		case SYS_SEL_TIMER1:
			sys_ll_set_cpu_clk_div_mode2_cksel_tim1(sel_value);
			break;
		default:
			break;
	}
}

uint32_t sys_hal_timer_select_clock_get(sys_sel_timer_t id)
{
	uint32_t ret = 0;

	switch (id) {
		case SYS_SEL_TIMER0:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_tim0();
			break;
		case SYS_SEL_TIMER1:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_tim1();
			break;
		default:
			break;
	}

	ret = (ret) ? TIMER_SCLK_XTAL : TIMER_SCLK_CLK32;

	return ret;
}

void sys_hal_spi_select_clock(spi_id_t num, spi_src_clk_t mode)
{
	uint32_t sel_value = 0;

	switch (mode) {
		case SPI_CLK_160MHZ:
			sel_value = 1;
			break;
		case SPI_CLK_XTAL:
		default:
			sel_value = 0;
			break;
	}

	switch (num) {
		case SPI_ID_0:
			sys_ll_set_cpu_clk_div_mode2_cksel_spi0(sel_value);
			break;
#if (SOC_SPI_UNIT_NUM > 1)
		case SPI_ID_1:
			sys_ll_set_cpu_clk_div_mode2_cksel_spi1(sel_value);
			break;
#endif
#if (SOC_SPI_UNIT_NUM > 2)
		case SPI_ID_2:
			sys_ll_set_cpu_clk_div_mode2_cksel_spi2(sel_value);
			break;
#endif
#if (SOC_SPI_UNIT_NUM > 3)
		case SPI_ID_3:
			sys_ll_set_cpu_clk_div_mode2_cksel_spi3(sel_value);
			break;
#endif
		default:
			break;
	}
}

void sys_hal_qspi_clk_sel(uint32_t id, uint32_t param)
{
	switch (id) {
		case QSPI_ID_0:
			sys_ahbp_ll_set_reg8_cksel_qspi0(param);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_ahbp_ll_set_reg8_cksel_qspi1(param);
			break;
#endif
		default:
			break;
	}
}

void sys_hal_qspi_set_src_clk_div(uint32_t id, uint32_t value)
{
	switch (id) {
		case QSPI_ID_0:
			sys_ahbp_ll_set_reg8_ckdiv_qspi0(value);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_ahbp_ll_set_reg8_ckdiv_qspi1(value);
			break;
#endif
		default:
			break;
	}
}

uint32_t sys_hal_sdio0_get_src_clk_div(void)
{
	return sys_ahbp_ll_get_reg8_ckdiv_sdio0();
}

uint32_t sys_hal_sdio1_get_src_clk_div(void)
{
	return sys_ahbp_ll_get_reg8_ckdiv_sdio1();
}

void sys_hal_sdio0_set_src_clk_div(uint32_t value)
{
	sys_ahbp_ll_set_reg8_ckdiv_sdio0(value);
}

void sys_hal_sdio1_set_src_clk_div(uint32_t value)
{
	sys_ahbp_ll_set_reg8_ckdiv_sdio1(value);
}

uint32_t sys_hal_sdio0_get_cken(void)
{
	return sys_ahbp_ll_get_rega_sdio0_cken();
}

uint32_t sys_hal_sdio1_get_cken(void)
{
	return sys_ahbp_ll_get_rega_sdio1_cken();
}

void sys_hal_sdio0_set_cken(uint32_t value)
{
	sys_ahbp_ll_set_rega_sdio0_cken(value);
}

void sys_hal_sdio1_set_cken(uint32_t value)
{
	sys_ahbp_ll_set_rega_sdio1_cken(value);
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
	// BK7259 only supports PWM0 clock selection
	(void)value;
}

void sys_hal_set_cksel_pwm(uint32_t value)
{
	sys_hal_set_cksel_pwm0(value);
}
uint32_t sys_hal_uart_select_clock_get(uart_id_t id)
{
	uint32_t ret = 0;

	switch (id) {
		case UART_ID_0:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart0();
			break;
		case UART_ID_1:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart1();
			break;
		case UART_ID_2:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart2();
			break;
		case UART_ID_3:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart3();
			break;
		case UART_ID_4:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_uart4();
			break;
		case UART_ID_5:
			ret = sys_ahbp_ll_get_reg8_ckdiv_uart5();
			break;
		default:
			ret = 0;
			break;
	}
	return ret;
}
uint32_t sys_hal_i2c_select_clock_get(i2c_id_t id)
{
	uint32_t ret = 0;

	switch (id) {
		case I2C_ID_0:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_i2c0();
			break;
#if (SOC_I2C_UNIT_NUM > 1)
		case I2C_ID_1:
			ret = sys_ll_get_cpu_clk_div_mode2_cksel_i2c3();
			break;
#endif
		default:
			break;
	}

	ret = (ret) ? I2C_SCLK_XTAL : I2C_SCLK_120M;

	return ret;
}
void sys_hal_sadc_int_enable(void)
{
	// sadc handled by CPU0
}

void sys_hal_sadc_int_disable(void)
{
	// sadc handled by CPU0
}

void sys_hal_sadc_pwr_up(void)
{
	return;
}
void sys_hal_sadc_pwr_down(void)
{
	return;
}
void sys_hal_set_saradc_config(void)
{
	return;
}
void sys_hal_set_sdmadc_config(void)
{
	return;
}
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
	if ((SPI_CLK_XTAL == value) || (SPI_CLK_APLL == value)) {
		sys_ll_set_cpu_clk_div_mode2_cksel_spi0(value);
		sys_ll_set_cpu_clk_div_mode2_cksel_spi1(value);
	}
}

void sys_hal_en_tempdet(uint32_t value)
{
	return;
}

uint32_t sys_hal_mclk_mux_get(void)
{
	return 0;
}
void sys_hal_mclk_mux_set(uint32_t value)
{
	return;
}

uint32_t sys_hal_mclk_div_get(void)
{
	return 0;
}

void sys_hal_mclk_div_set(uint32_t value)
{
	return;
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
	if (en) {
		sys_ll_set_reserver_reg0xf_xvr_mem_ret(0);
	} else {
		sys_ll_set_reserver_reg0xf_xvr_mem_ret(1);
	}
}

void sys_hal_btdm_interrupt_ctrl(bool en)
{
	return;
}

void sys_hal_ble_interrupt_ctrl(bool en)
{
	return;
}

void sys_hal_bt_interrupt_ctrl(bool en)
{
	return;
}

void sys_hal_bt_rf_ctrl(bool en)
{
	return;
}

void sys_hal_rf_ctrl(uint8_t type)
{
	return;
}

uint8_t sys_hal_rf_ctrl_type_get(void)
{
	return 0;
}
uint32_t sys_hal_bt_rf_status_get(void)
{
	return 0;
}
void sys_hal_bt_sleep_exit_ctrl(bool en)
{
	return;
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
/**
  * @brief	lcd_disp  system config
  * param1: clk source sel 0:clk_320M      1:clk_480M,
  * param2: clk_div  F/(1+clkdiv_disp_l+clkdiv_disp_h*2)
  * param1: int_en eanble lcd cpu int
  * param2: clk_always_on, BUS_CLK ENABLE,0: bus clock open when module is select,1:bus clock always open,  0 by defult
  * return none
  */
void sys_hal_lcd_disp_clk_en(uint8_t clk_src_sel, uint8_t clk_div_l, uint8_t clk_div_h, uint8_t clk_always_on)
{
	return;
}
/**
  * @brief	lcd clk close and int disable, reg value recover default.
  * return none
  */
void sys_hal_lcd_disp_close(void)
{
	return;
}
/**
  * @brief	dma2d system config
  * param1: clk source sel 0:clk_320M	   1:clk_480M,
  * param2: clk_always_on  ENABLE,0: bus clock auto open when module is select,1:bus clock always open
  * param1: int_en eanble lcd cpu int
  * return none
  */
void sys_hal_dma2d_clk_en(uint8_t clk_always_on)
{
	return;
}
void sys_hal_set_jpeg_dec_disckg(uint32_t value)
{
	return;
}
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
	return;
}
void sys_hal_mac_bus_clk_ctrl(bool clk_en)
{
	return;
}
void sys_hal_mac_clk_ctrl(bool clk_en)
{
	return;
}
void sys_hal_set_vdd_value(uint32_t param)
{
	return;
}
uint32_t sys_hal_get_vdd_value(void)
{
	return 0;
}
//CMD_SCTRL_BLOCK_EN_MUX_SET
void sys_hal_block_en_mux_set(uint32_t param)
{
	return;
}
void sys_hal_enable_mac_gen_int(void)
{
	return;
}
void sys_hal_enable_mac_prot_int(void)
{
	return;
}
void sys_hal_enable_mac_tx_trigger_int(void)
{
	return;
}
void sys_hal_enable_mac_rx_trigger_int(void)
{
	return;
}
void sys_hal_enable_mac_txrx_misc_int(void)
{
	return;
}
void sys_hal_enable_mac_txrx_timer_int(void)
{
	return;
}
void sys_hal_enable_modem_int(void)
{
	return;
}
void sys_hal_enable_modem_rc_int(void)
{
	return;
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
void sys_hal_diag_debug_phy()
{
	return;
}
void dbg_enable_debug_gpio(void)
{
	return;
}
//Yantao Add End
void sys_hal_cali_dpll_spi_trig_disable(void)
{
	return;
}
void sys_hal_cali_dpll_spi_trig_enable(void)
{
	return;
}
void sys_hal_cali_dpll_spi_detect_disable(void)
{
	return;
}
void sys_hal_cali_dpll_spi_detect_enable(void)
{
	return;
}
void sys_hal_set_xtalh_ctune(uint32_t value)
{
	sys_ll_set_ana_reg3_ctune(value);
}
void sys_hal_analog_set(analog_reg_t reg, uint32_t value)
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
	sys_ll_set_ana_reg1_value(value);
}
void sys_hal_set_ana_reg2_value(uint32_t value)
{
	sys_ll_set_ana_reg2_value(value);
}
void sys_hal_set_ana_reg3_value(uint32_t value)
{
	sys_ll_set_ana_reg3_value(value);
}
void sys_hal_set_ana_reg4_value(uint32_t value)
{
	sys_ll_set_ana_reg4_value(value);
}
void sys_hal_set_ana_reg12_value(uint32_t value)
{
	sys_ll_set_ana_reg12_value(value);
}
void sys_hal_set_ana_reg13_value(uint32_t value)
{
	sys_ll_set_ana_reg13_value(value);
}
void sys_hal_set_ana_reg14_value(uint32_t value)
{
	sys_ll_set_ana_reg14_value(value);
}
void sys_hal_set_ana_reg15_value(uint32_t value)
{
	sys_ll_set_ana_reg15_value(value);
}
void sys_hal_set_ana_reg16_value(uint32_t value)
{
	sys_ll_set_ana_reg16_value(value);
}
void sys_hal_set_ana_reg17_value(uint32_t value)
{
	sys_ll_set_ana_reg17_value(value);
}
void sys_hal_set_ana_reg18_value(uint32_t value)
{
	sys_ll_set_ana_reg18_value(value);
}
void sys_hal_set_ana_reg19_value(uint32_t value)
{
	sys_ll_set_ana_reg19_value(value);
}
void sys_hal_set_ana_reg20_value(uint32_t value)
{
	sys_ll_set_ana_reg20_value(value);
}
void sys_hal_set_ana_reg21_value(uint32_t value)
{
	sys_ll_set_ana_reg21_value(value);
}
void sys_hal_set_ana_reg25_value(uint32_t value)
{
    sys_ll_set_ana_reg25_value(value);
}
void sys_hal_set_ana_reg27_value(uint32_t value)
{
    sys_ll_set_ana_reg27_value(value);
}
void sys_hal_set_ana_reg28_value(uint32_t value)
{
    sys_ll_set_ana_reg28_value(value);
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
	return sys_ll_get_ana_reg2_value();
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
	return 0;
}
uint32_t sys_hal_get_bgcalm(void)
{
	return sys_ll_get_ana_reg9_bgcal();
}
void sys_hal_set_bgcalm(uint32_t value)
{
	sys_ll_set_ana_reg9_bgcal(value);
}
void sys_hal_set_audioen(uint32_t value)
{
	sys_ll_set_ana_reg25_audioen(value);
}
void sys_hal_set_dpll_div_cksel(uint32_t value)
{
	sys_ll_set_ana_reg0_cksel(value);
}
void sys_hal_set_dpll_reset(uint32_t value)
{
	sys_ll_set_ana_reg7_spi_rstn(value);
}
void sys_hal_set_gadc_ten(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_en_spi(value);
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

void sys_hal_aud_dac_bias_en(uint32_t value)
{
    sys_ll_set_ana_reg20_enaudbias(value);
}
void sys_hal_aud_dac_ldcoc_en(uint32_t value)
{
    sys_ll_set_ana_reg29_lendcoc(value);
}
void sys_hal_aud_dac_rdcoc_en(uint32_t value)
{
    sys_ll_set_ana_reg29_rendcoc(value);
}
void sys_hal_aud_dac_enbs_en(uint32_t value)
{
    sys_ll_set_ana_reg30_enbs(value);
}

void sys_hal_aud_looprst0v9_en(uint32_t value)
{
    sys_ll_set_ana_reg30_looprst0v9(value);
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
	sys_ll_set_ana_reg29_dacmute(value);
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
	return;
}
void sys_hal_touch_sensitivity_level_set(uint32_t value)
{
	return;
}
void sys_hal_touch_scan_mode_enable(uint32_t value)
{
	return;
}
void sys_hal_touch_detect_threshold_set(uint32_t value)
{
	return;
}
void sys_hal_touch_detect_range_set(uint32_t value)
{
	return;
}
void sys_hal_touch_calib_enable(uint32_t value)
{
	return;
}
void sys_hal_touch_cal_ctrl_set(uint32_t value)
{
	return;
}
uint32_t sys_hal_touch_cal_ctrl_get(void)
{
	return 0;
}
void sys_hal_touch_cal_vth_set(uint32_t value)
{
	return;
}
uint32_t sys_hal_touch_cal_vth_get(void)
{
	return 0;
}
void sys_hal_touch_rstb_dig_set(uint32_t value) { return; }
uint32_t sys_hal_touch_rstb_dig_get(void) { return 0; }
void sys_hal_touch_ldoen_set(uint32_t value) { return; }
uint32_t sys_hal_touch_ldoen_get(void) { return 0; }
void sys_hal_touch_cal_auto_set(uint32_t value) { return; }
void sys_hal_touch_cal_done_clr(uint32_t value) { return; }
void sys_hal_touch_manul_mode_calib_value_set(uint32_t value)
{
	return;
}
uint32_t sys_hal_touch_calib_value_get(void)
{
	return 0;
}
void sys_hal_touch_manul_mode_enable(uint32_t value)
{
	return;
}
void sys_hal_touch_scan_mode_chann_set(uint32_t value)
{
	return;
}
void sys_hal_touch_scan_mode_chann_sel(uint32_t value)
{
	return;
}
void sys_hal_touch_serial_cap_enable(void)
{
	return;
}
void sys_hal_touch_serial_cap_disable(void)
{
	return;
}
void sys_hal_touch_serial_cap_sel(uint32_t value)
{
	return;
}
void sys_hal_touch_spi_lock(void)
{
	return;
}
void sys_hal_touch_spi_unlock(void)
{
	return;
}
void sys_hal_touch_test_period_set(uint32_t value)
{
	return;
}
void sys_hal_touch_test_number_set(uint32_t value)
{
	return;
}
void sys_hal_touch_calib_period_set(uint32_t value)
{
	return;
}
void sys_hal_touch_calib_number_set(uint32_t value)
{
	return;
}
void sys_hal_touch_int_set(uint32_t value)
{
	return;
}
void sys_hal_touch_int_clear(uint32_t value)
{
	return;
}
void sys_hal_touch_int_enable(uint32_t value)
{
	return;
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
	sys_ll_set_cpu_clk_div_mode1_ckdiv_auxs(value);
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
	return;
}
void sys_hal_psram_dpll_enable(uint32_t enable)
{
	return;
}
void sys_hal_psram_clk_sel(uint32_t value)
{
	return;
}
void sys_hal_psram_set_clkdiv(uint32_t value)
{
	return;
}
void sys_hal_psram_psldo_vsel(uint32_t value)
{
	return;
}
void sys_hal_psram_psldo_vset(uint32_t output_voltage, uint32_t is_add_200mv)
{
	return;
}
void sys_hal_psram_psram_disckg(uint32_t value)
{
	return;
}

uint32_t sys_hal_get_psram_interleave_config(void)
{
#if (CONFIG_PSRAM_INTERLEAVE)
	return sys_ahbp_ll_get_reg7_psram_inv_config();
#else
	return 0;
#endif
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
	sys_ll_set_ana_reg22_gadc_bufamp_isel(value);
}

void sys_hal_set_ana_gadc_cmp_ictrl(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_comp_isel(value);
}

void sys_hal_set_ana_pwd_gadc_buf(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_inbuf_en(!value);
}

void sys_hal_set_ana_vref_sel(uint32_t value)
{
	sys_ll_set_ana_reg22_gadc_vref_sel(value);
}
void sys_hal_set_ana_cb_cal_manu(uint32_t value)
{
	return;
}
void sys_hal_set_ana_reg5_pwd_rosc_spi(uint32_t value)
{
	sys_ll_set_ana_reg5_pwd_rosc_spi(value);
}
void sys_hal_set_ana_cb_cal_trig(uint32_t value)
{
	return;
}
void sys_hal_set_ana_vlsel_ldodig(uint32_t value)
{
	sys_ll_set_ana_reg10_vdiglsel(value);
}

void sys_hal_set_ana_vhsel_ldodig(uint32_t value)
{
	sys_ll_set_ana_reg10_vdighsel(value);
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
	sys_ll_set_ana_reg2_vctrl_vsel(value);
}
void sys_hal_set_yuv_buf_clock_en(uint32_t value)
{
	return;
}
void sys_hal_set_h264_clock_en(uint32_t value)
{
	return;
}
void sys_hal_nmi_wdt_set_clk_div(uint32_t value)
{
	return;
}
__IRAM_SEC uint32_t sys_hal_nmi_wdt_get_clk_div(void)
{
	return 0;
}
void sys_hal_set_ana_cb_cal_manu_val(uint32_t value)
{
	return;
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
	while (times--);
}
uint32_t sys_hal_cali_dpll(uint32_t param)
{
	return 0;
}

static void sys_hal_dpll_cpu_flash_time_early_init(uint32_t chip_id)
{
	sys_ll_set_reserver_reg0xd_sig_240m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_320m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_480m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_160m_cken(1);
	sys_ll_set_reserver_reg0xd_sig_120m_cken(1);

	sys_ll_set_cpu_clk_div_mode1_ckdiv_core(0x3);
	sys_ll_set_cpu_clk_div_mode1_cksel_core(0x3);
	return;
}

static void sys_hal_pwd_rosc()
{
	return;
}
void sys_hal_early_init(void)
{
	return;
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
	sys_ll_set_cpu0_int_halt_clk_op_cpu0_rxevt_sel(param);
}
void sys_hal_set_cpu1_rxevt_sel(uint32_t param)
{
	sys_ll_set_cpu1_int_halt_clk_op_cpu1_rxevt_sel(param);
}
void sys_hal_set_cpu2_rxevt_sel(uint32_t param)
{
	// CPU2 (M55 sub-core) does not support rxevt_sel control in sys_ahbp_reg4
	// This register field is not available in sys_ahbp_reg4_t structure
	(void)param;
}
void sys_hal_set_cpu_device_clk_enable_otp_cken(uint32_t value)
{
	sys_ll_set_cpu_device_clk_enable_otp_cken(value);
}
void sys_hal_set_cpu_power_sleep_wakeup_ticktimer_32k_enable(uint32_t value)
{
	sys_ll_set_cpu_power_sleep_wakeup_cpu0_ticktimer_32k_enable(value);
	sys_ll_set_cpu_power_sleep_wakeup_cpu1_ticktimer_32k_enable(value);
}

bk_err_t sys_hal_cpu_freq_dump()
{
	return BK_OK;
}


/** gpu Start **/
void sys_hal_set_gpu_buffa_enable_value(uint32_t value)
{
    sys_ahbp_ll_set_reg28_gpu_buffa_enable(value);
}

uint32_t sys_hal_get_gpu_buffa_enable_value(void)
{
    return sys_ahbp_ll_get_reg28_gpu_buffa_enable();
}

void sys_hal_set_gpu_buffa_begin_value(uint32_t value)
{
    sys_ahbp_ll_set_reg29_value(value);
}

uint32_t sys_hal_get_gpu_buffa_begin_value(void)
{
    return sys_ahbp_ll_get_reg29_value();
}

void sys_hal_set_gpu_buffa_size_value(uint32_t value)
{
    sys_ahbp_ll_set_reg2a_value(value);
}

uint32_t sys_hal_get_gpu_buffa_size_value(void)
{
    return sys_ahbp_ll_get_reg2a_value();
}

void sys_hal_set_gpu_pica_begin_value(uint32_t value)
{
    sys_ahbp_ll_set_reg2b_value(value);
}

uint32_t sys_hal_get_gpu_pica_begin_value(void)
{
    return sys_ahbp_ll_get_reg2b_value();
}

void sys_hal_set_gpu_pica_halfbuff_end_value(uint32_t value)
{
    sys_ahbp_ll_set_reg2c_value(value);
}

uint32_t sys_hal_get_gpu_pica_halfbuff_end_value(void)
{
    return sys_ahbp_ll_get_reg2c_value();
}

void sys_hal_set_gpu_pica_end_value(uint32_t value)
{
    sys_ahbp_ll_set_reg2d_value(value);
}

uint32_t sys_hal_get_gpu_pica_end_value(void)
{
    return sys_ahbp_ll_get_reg2d_value();
}

void sys_hal_set_gpu_buffa_remap_addr_value(uint32_t value)
{
    sys_ahbp_ll_set_reg2f_value(value);
}

uint32_t sys_hal_get_gpu_buffa_remap_addr_value(void)
{
    return sys_ahbp_ll_get_reg2f_value();
}

void sys_hal_set_gpu_buffb_enable_value(uint32_t value)
{
    sys_ahbp_ll_set_reg30_value(value);
}

uint32_t sys_hal_get_gpu_buffb_enable_value(void)
{
    return sys_ahbp_ll_get_reg30_value();
}

void sys_hal_set_gpu_buffb_begin_value(uint32_t value)
{
    sys_ahbp_ll_set_reg31_value(value);
}

uint32_t sys_hal_get_gpu_buffb_begin_value(void)
{
    return sys_ahbp_ll_get_reg31_value();
}

void sys_hal_set_gpu_buffb_size_value(uint32_t value)
{
    sys_ahbp_ll_set_reg32_value(value);
}

uint32_t sys_hal_get_gpu_buffb_size_value(void)
{
    return sys_ahbp_ll_get_reg32_value();
}

void sys_hal_set_gpu_picb_begin_value(uint32_t value)
{
    sys_ahbp_ll_set_reg33_value(value);
}

uint32_t sys_hal_get_gpu_picb_begin_value(void)
{
    return sys_ahbp_ll_get_reg33_value();
}

void sys_hal_set_gpu_picb_halfbuff_end_value(uint32_t value)
{
    sys_ahbp_ll_set_reg34_value(value);
}

uint32_t sys_hal_get_gpu_picb_halfbuff_end_value(void)
{
    return sys_ahbp_ll_get_reg34_value();
}

void sys_hal_set_gpu_picb_end_value(uint32_t value)
{
    sys_ahbp_ll_set_reg35_value(value);
}

uint32_t sys_hal_get_gpu_picb_end_value(void)
{
    return sys_ahbp_ll_get_reg35_value();
}

void sys_hal_set_gpu_buffb_remap_addr_value(uint32_t value)
{
    sys_ahbp_ll_set_reg37_value(value);
}

uint32_t sys_hal_get_gpu_buffb_remap_addr_value(void)
{
    return sys_ahbp_ll_get_reg37_value();
}
/** gpu End **/

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

/** RISC-V Interrupt Configuration **/
void sys_hal_set_ints_config_riscv_0_31(uint32_t v)
{
	sys_ahbp_ll_set_reg16_ints_config_scr1_0(v);
}

uint32_t sys_hal_get_ints_config_riscv_0_31(void)
{
	return sys_ahbp_ll_get_reg16_ints_config_scr1_0();
}

void sys_hal_set_ints_config_riscv_32_63(uint32_t v)
{
	sys_ahbp_ll_set_reg17_ints_config_scr1_1(v);
}

uint32_t sys_hal_get_ints_config_riscv_32_63(void)
{
	return sys_ahbp_ll_get_reg17_ints_config_scr1_1();
}