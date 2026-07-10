// Copyright 2022-2023 Beken
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
#include "aon_pmu_hal.h"
#include "gpio_hal_v2px.h"
#include "gpio_driver_base.h"
#include "gpio_driver.h"
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <driver/hal/hal_spi_types.h>
#include "bk_arch.h"
#include "hal_port.h"
#include <os/os.h>
#include "sys_pm_hal.h"
#include "sys_pm_hal_ctrl.h"
#include "sys_sw_regs.h"
#include "modules/pm.h"
#include <driver/pwr_clk.h>
#include "driver/flash.h"
#include "cache.h"
#include "sys_ahbp_ll.h"
#include "multicore_driver.h"
#include <driver/dma.h>

extern uint64_t check_IRQ_pending(void);

#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )

#define portNVIC_INT_CTRL_REG                 ( *( ( volatile uint32_t * ) 0xe000ed04 ) )

#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )

#define portNVIC_PENDSVSET_BIT                ( 1UL << 28UL )
#define portNVIC_PENDSVCLR_BIT                ( 1UL << 27UL )
#define portNVIC_SYSTICKSET_BIT               ( 1UL << 26UL )
#define portNVIC_SYSTICKCLR_BIT               ( 1UL << 25UL )

#define PM_EXIT_LOWVOL_SYSTICK_TIME           (32)      //1ms
#define PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME    (0xFFFFFF)//set max
#define PM_LOW_VOL_AON_LDO_SEL                (2)       // 0.7V
#define PM_LOW_VOL_VIO_LDO_SEL                (0)       // 2.9V

#if CONFIG_OTA_POSITION_INDEPENDENT_AB || CONFIG_DIRECT_XIP
#define FLASH_BASE_ADDRESS                    SOC_FLASH_REG_BASE
#define FLASH_OFFSET_ADDR_BEGIN               (0x16)
#define FLASH_OFFSET_ADDR_END                 (0x17)
#define FLASH_ADDR_OFFSET                     (0x18)
#define FLASH_OFFSET_ENABLE                   (0x19)

typedef struct
{
	uint32_t flash_offset_enable_val;
	uint32_t offset_addr_begin_val;
	uint32_t offset_addr_end_val ;
	uint32_t flash_addr_offset_val;
}flash_ab_reg_t;

#endif

uint64_t low_voltage_exit_tick = 0;
uint64_t low_voltage_wakeup_time_us = 0;
extern void bk_delay_us(UINT32 us);
static inline bool is_lpo_src_26m32k(void)
{
	return (aon_pmu_ll_get_r41_lpo_config() == SYS_LPO_SRC_26M32K);
}

static inline bool is_lpo_src_ext32k(void)
{
	return (aon_pmu_ll_get_r41_lpo_config() == SYS_LPO_SRC_EXTERNAL_32K);
}

static inline bool is_wifi_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_WIFI);
}

static inline bool is_gpio_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_GPIO);
}

static inline bool is_rtc_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_RTC);
}

static inline bool is_bt_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_BT);
}

static inline bool is_usbplug_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_USBPLUG);
}

static inline bool is_touch_ws_enabled(uint8_t ena_bits)
{
	return !!(ena_bits & WS_TOUCH);
}

static inline bool is_wifi_pd_poweron(uint32_t pd_bits)
{
	return !(pd_bits & PD_WIFI);
}

static inline bool is_btsp_pd_poweron(uint32_t pd_bits)
{
	return !(pd_bits & PD_BTSP);
}

static inline void sys_hal_backup_disable_int(volatile uint32_t *int_state1, volatile uint32_t *int_state2)
{
    return;
}

static inline void sys_hal_restore_int(volatile uint32_t int_state1, volatile uint32_t int_state2)
{
    return;
}

static inline void sys_hal_enable_wakeup_int(void)
{
}

static inline void sys_hal_set_core_freq(volatile uint8_t cksel_core, volatile uint8_t clkdiv_core, volatile uint8_t clkdiv_bus)
{
    return;
}

static inline void sys_hal_set_core_26m(void)
{
	sys_hal_set_core_freq(0, 0, 0);
}

static inline void sys_hal_backup_set_core_26m(volatile uint8_t *cksel_core, volatile uint8_t *clkdiv_core, volatile uint8_t *clkdiv_bus)
{
    return;
}

static inline void sys_hal_restore_core_freq(volatile uint8_t cksel_core, volatile uint8_t clkdiv_core, volatile uint8_t clkdiv_bus)
{
    return;
}

static inline void sys_hal_set_flash_freq(volatile uint8_t cksel_flash, volatile uint8_t ckdiv_flash)
{
    return;
}

static inline void sys_hal_set_flash_26m(void)
{
	sys_hal_set_flash_freq(0, 0);
}

static inline void sys_hal_backup_set_flash_26m(volatile uint8_t *cksel_flash, volatile  uint8_t *ckdiv_flash)
{
    return;
}

static inline void sys_hal_set_flash_120m(void)
{
    return;
}

static inline void sys_hal_set_anaspi_freq(uint8_t anaspi_freq)
{
    return;
}

static inline void sys_hal_backup_set_anaspi_freq_26m(uint8_t *anaspi_freq)
{
    return;
}

static inline void sys_hal_restore_anaspi_freq(uint8_t anaspi_freq)
{
	sys_hal_set_anaspi_freq(anaspi_freq);
}

static inline void sys_hal_restore_flash_freq(volatile uint8_t cksel_flash, volatile uint8_t ckdiv_flash)
{
    return;
}

static inline void sys_hal_mask_cpu0_int(void)
{
    return;
}

static inline void sys_hal_enable_spi_latch(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
}

static inline void sys_hal_disable_spi_latch(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(0);
}

static inline uint32_t sys_hal_disable_hf_clock(void)
{
    return 0;
}

static inline void sys_hal_restore_hf_clock(volatile uint32_t val)
{
    return;
}

/**
 * buck power supply switch
 *
 * uint32_t type input:
 *   0: close buck
 *   other: open buck
 *
 * note: please declare this function as static when buck is stable
*/
void sys_hal_buck_switch(uint32_t flag)
{
    return;
}

/**
 * high digital voltage
 *
 * uint32_t value input:
 *   voltage value
 *
 *
 * note: please declare this function
*/
void sys_hal_v_core_h_sel(uint32_t value)
{
    return;
}
static inline void sys_hal_deep_sleep_set_buck(void)
{
    return;
}

static inline void sys_hal_deep_sleep_set_vldo(void)
{
    return;
}

static inline void sys_hal_clear_wakeup_source(void)
{
	aon_pmu_ll_set_r43_clr_wakeup(1);
	aon_pmu_ll_set_r43_clr_wakeup(0);
}

static inline void sys_hal_set_halt_config(void)
{
	uint32_t v = aon_pmu_ll_get_r41();

	//halt_lpo = 1
	//halt_busrst = 0
	//halt_busiso = 1, halt_buspwd = 1
	//halt_blpiso = 1, halt_blppwd = 1
	//halt_wlpiso = 1, halt_wlppwd = 1
	v |= (0xFD << 24);
	aon_pmu_ll_set_r41(v);
}
static inline void sys_hal_power_on_and_select_rosc(pm_lpo_src_e lpo_src)
{
    return;
}
static inline void sys_hal_set_power_parameter(uint8_t sleep_mode)
{
	/*  r40[3:0]   wake1_delay = 0x1;
 	 *  r40[7:4]   wake2_delay = 0x1;
 	 *  r40[11:8]  wake3_delay = 0x1;
 	 *  r40[15:12] halt1_delay = 0x1;
 	 *  r40[19:16] halt2_delay = 0x1;
 	 *  r40[23:20] halt3_delay = 0x1;
 	 *  r40[24] halt_volt: deep = 0, lv = 1
 	 *  r40[25] halt_xtal = 1 //If LPO is 26M32K, halt_xtal = 0
 	 *  r40[26] halt_core: deep = 1, lv = 0
 	 *  r40[27] halt_flash = 1
 	 *  r40[28] halt_rosc = 0
 	 *  r40[29] halt_resten: deep = 0, lv = 1
 	 *  r40[30] halt_isolat = 1
 	 *  r40[31] halt_clkena = 0
 	 **/
	if (sleep_mode == PM_MODE_DEEP_SLEEP)
	{
#if CONFIG_SPE
		aon_pmu_ll_set_r40(0x4E1116EE);//using the external 32k , it need more wake delay time
#else
		// OTP need more delay to recovery voltage
		aon_pmu_ll_set_r40(0x4E1116EE);
#endif
	}
	else if (sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
#if CONFIG_SPE
		aon_pmu_ll_set_r40(0x5E1116EE);//using the external 32k , it need more wake delay time
#else
		// OTP need more delay to recovery voltage
		aon_pmu_ll_set_r40(0x5E1116EE);
#endif
	}
	else
	{
		uint32_t val;
		if (is_lpo_src_26m32k())//26m/32k
		{
			#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
			val = (0x61111000
				|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
			#else
			val = (0x69111000
				|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
			#endif
		}
		else//external 32k and rosc
		{
			#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
			val = (0x63111000
				|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
			#else
			val = (0x6B111000
				|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
			#endif
		}
		aon_pmu_ll_set_r40(val);
	}
}

static inline void sys_hal_set_sleep_condition(void)
{
    return;
}

static inline void sys_hal_power_down_pd(volatile uint32_t *pd_reg_v)
{
    return;
}

static inline void sys_hal_power_on_pd(volatile uint32_t v_sys_r10)
{
    return;
}

static inline void sys_hal_set_wakeup_source(void)
{
	aon_pmu_ll_set_r41_wakeup_ena(0x7f);
}

static inline void sys_hal_clear_wakeup_status(void)
{
	aon_pmu_ll_set_r43_clr_wakeup(1);
	aon_pmu_ll_set_r43_clr_wakeup(0);
}

#if !CONFIG_AON_PMU_REG0_REFACTOR_DEV
void sys_hal_gpio_state_switch(bool lock)
{
	/*pass aon_pmu_r0 to ana*/
	if (lock) {
		aon_pmu_ll_set_r0_gpio_sleep(1);
	} else {
		aon_pmu_ll_set_r0_gpio_sleep(0);
	}
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}
#endif

__attribute__((section(".itcm_sec_code"))) void sys_hal_enter_deep_sleep(void *param)
{

}

static inline void sys_hal_set_low_voltage(volatile uint32_t *ana_r8, volatile uint32_t *core_low_voltage)
{
    return;
}

static inline void sys_hal_restore_voltage(volatile uint32_t ana_r8, volatile uint32_t core_low_voltage)
{
    return;
}

static void sys_hal_delay(volatile uint32_t times)
{
        while(times--);
}

void sys_hal_exit_low_voltage(void)
{
}

uint64_t sys_hal_get_exit_low_voltage_tick(void)
{
	return low_voltage_exit_tick;
}

__attribute__((section(".itcm_sec_code"))) void sys_hal_set_exit_low_voltage_tick(uint64_t tick)
{
	low_voltage_exit_tick= tick;

}

inline uint64_t sys_hal_get_low_voltage_wakeup_time_us(void)
{
	return low_voltage_wakeup_time_us;
}

inline void sys_hal_set_low_voltage_wakeup_time_us(uint64_t wakeup_time)
{
	low_voltage_wakeup_time_us = wakeup_time;
}

static inline void sys_hal_lv_set_buck(void)
{
    return;
}

static inline void sys_hal_lv_restore_buck(void)
{
    return;
}

#if CONFIG_OTA_POSITION_INDEPENDENT_AB || CONFIG_DIRECT_XIP
//when enter lowvoltage need backup related flash information.
static inline void flash_ab_info_backup(flash_ab_reg_t *p_flash_ab_reg)
{
	if(!p_flash_ab_reg)
	{
		return;
	}

	p_flash_ab_reg->offset_addr_begin_val = REG_READ(FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_BEGIN*4);
	p_flash_ab_reg->offset_addr_end_val = REG_READ(FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_END*4);
	p_flash_ab_reg->flash_addr_offset_val = REG_READ(FLASH_BASE_ADDRESS + FLASH_ADDR_OFFSET*4);
	p_flash_ab_reg->flash_offset_enable_val = (REG_READ(FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4) & 0x1);
}

//when exit lowvoltage need restore related flash information.
static inline void flash_ab_info_restore(flash_ab_reg_t *p_flash_ab_reg)
{
	if(!p_flash_ab_reg)
	{
		return;
	}

	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_BEGIN*4), p_flash_ab_reg->offset_addr_begin_val);
	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_OFFSET_ADDR_END*4), p_flash_ab_reg->offset_addr_end_val);
	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_ADDR_OFFSET*4), p_flash_ab_reg->flash_addr_offset_val);
	REG_WRITE((FLASH_BASE_ADDRESS + FLASH_OFFSET_ENABLE*4), p_flash_ab_reg->flash_offset_enable_val);
}
#endif

__attribute__((section(".itcm_sec_code"))) void sys_hal_enter_low_voltage(void)
{

}

void sys_hal_touch_wakeup_enable(uint8_t index)
{
	return;
}

void sys_hal_usb_wakeup_enable(uint8_t index)
{
    return;
}

void sys_hal_rtc_wakeup_enable(uint32_t value)
{
    return;
}

void sys_hal_rtc_ana_wakeup_enable(uint32_t period)
{
    return;
}

void sys_hal_gpio_ana_wakeup_enable(uint32_t count, uint32_t index, uint32_t type)
{
	sys_hal_enable_spi_latch();

	sys_hal_disable_spi_latch();
}

void sys_hal_enter_cpu_wfi()
{
	if(portGET_CORE_ID() == CPU0_CORE_ID)
	{
		//bk_printf("CPU0_CORE_ID\r\n");
		pm_shared_info_t shared_info = {0};
		bk_sys_sw_regs_get_pm_shared_info(&shared_info);
		if(shared_info.pm_cp0_sleep_state == 0x1)
		{
			volatile uint32_t int_state0_31;
			volatile uint32_t int_state32_63;
			uint32_t systick_ctrl_value = 0;

			systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
			//portNVIC_SYSTICK_CTRL_REG = 0;

			int_state0_31 = sys_ahbp_ll_get_reg10_value();
			int_state32_63 = sys_ahbp_ll_get_reg11_value();

			/*Disable Int exclude mailbox,mailbox int for wakeup*/
			sys_ahbp_ll_set_reg10_value(0x0);
			sys_ahbp_ll_set_reg11_value(0x0);

			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );

			if(check_IRQ_pending()||bk_dma_check_chn_status()||(sys_ahbp_ll_get_reg18_value()||(sys_ahbp_ll_get_reg19_value()))||(portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT))
			{
				sys_ahbp_ll_set_reg10_value(int_state0_31);
				sys_ahbp_ll_set_reg11_value(int_state32_63);
				portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
				return;
			}

#if CONFIG_CPU_HOTPLUG
			bk_cpu_hp_offline(CPU3_CORE_ID);
#endif

			shared_info.pm_ap0_sleep_state = 1;
			__DMB();
			bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP0_SLEEP_STATE, BK_SYS_SW_REGS_LOCK_DISABLE);
			__DMB();
			flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
			__DMB();

			arch_deep_sleep();

			shared_info.pm_ap0_sleep_state = 0;
			bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP0_SLEEP_STATE, BK_SYS_SW_REGS_LOCK_DISABLE);

			portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;

#if CONFIG_CPU_HOTPLUG
			bk_cpu_hp_online(CPU3_CORE_ID);
#endif

			sys_ahbp_ll_set_reg10_value(int_state0_31);
			sys_ahbp_ll_set_reg11_value(int_state32_63);
		}
		else
		{
			//arh_sleep();
		}
	}
	else
	{
		//arch_sleep();
	}
}

void sys_hal_enter_normal_sleep(uint32_t peri_clk)
{
	#if 0//CONFIG_PM_LV_SUBCORES_ON
	if(portGET_CORE_ID() == CPU0_CORE_ID)
	{
		if(aon_pmu_ll_get_r3_cp0_sleep_vote_state())
		{
			volatile uint32_t int_state1, int_state2;
			uint32_t systick_ctrl_value = 0;

			systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
			portNVIC_SYSTICK_CTRL_REG = 0;

			int_state1 = sys_ll_get_cpu1_int_0_31_en_value();
			int_state2 = sys_ll_get_cpu1_int_32_63_en_value();
			/*Disable Int exclude mailbox,mailbox int for wakeup*/
			sys_ll_set_cpu1_int_0_31_en_value(0x0);
			sys_ll_set_cpu1_int_32_63_en_value(0x0);
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			if(check_IRQ_pending()||bk_dma_check_chn_status()||(sys_ll_get_cpu1_int_0_31_status_value()||(sys_ll_get_cpu1_int_32_63_status_value()))||(portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT))
			{
				sys_ll_set_cpu1_int_0_31_en_value(int_state1);
				sys_ll_set_cpu1_int_32_63_en_value(int_state2);
				portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
				//BK_LOGD(NULL, "Core0 pending irq:0x%llx,0x%x\r\n",check_IRQ_pending(),bk_dma_check_chn_status());
				return;
			}
			sys_ll_set_cpu1_int_32_63_en_cpu1_mailbox_int_en(1);

			bk_pm_handle_lv_sleep_callback(PM_LV_ENTER_SLEEP);
			/*Set cpu1 wfi state*/
			aon_pmu_ll_set_r3_cp1_enter_wfi_state(1);

			FIXED_ADDR_WAKEUP_AP1_COUNT += 1;
			/*Enter deep sleep*/
			arch_deep_sleep();

			/*Clear cpu1 wfi state*/
			aon_pmu_ll_set_r3_cp1_enter_wfi_state(0);

			portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;

			bk_pm_handle_lv_sleep_callback(PM_LV_EXIT_SLEEP);
			bk_err_t ret = vPortYieldCore(1);
			s_trigger_ap1_count = 0;
			while(ret != BK_OK)
			{
				bk_delay_us(PM_AP_TRRIGER_DELAY_TIME_US);
				ret = vPortYieldCore(1);
				s_trigger_ap1_count++;
				if(s_trigger_ap1_count > PM_TRRIGER_AP_MAX_COUNT)
				{
					LOGE("Wakeup AP1 failed[%d]\r\n",ret);
					break;
				}
			}
			sys_ll_set_cpu1_int_0_31_en_value(int_state1);
			sys_ll_set_cpu1_int_32_63_en_value(int_state2);
		}
		else
		{
			arch_sleep();
		}
	}
	else if(portGET_CORE_ID() == CPU1_CORE_ID)
	{
		if(aon_pmu_ll_get_r3_cp0_sleep_vote_state())
		{
			volatile uint32_t int1_state1, int1_state2;
			uint32_t systick_ctrl_value = 0;

			systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
			portNVIC_SYSTICK_CTRL_REG = 0;

			int1_state1 = sys_ll_get_cpu2_int_0_31_en_value();
			int1_state2 = sys_ll_get_cpu2_int_32_63_en_value();
			/*Disable Int exclude mailbox,mailbox int for wakeup*/
			sys_ll_set_cpu2_int_0_31_en_value(0x0);
			sys_ll_set_cpu2_int_32_63_en_value(0x0);
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			__asm volatile( "nop" );
			if(check_IRQ_pending()||bk_dma_check_chn_status()||(sys_ll_get_cpu2_int_0_31_status_value()||(sys_ll_get_cpu2_int_32_63_status_value()))||(portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT))
			{
				sys_ll_set_cpu2_int_0_31_en_value(int1_state1);
				sys_ll_set_cpu2_int_32_63_en_value(int1_state2);
				portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
				//BK_LOGD(NULL, "Core1 pending irq:0x%llx,0x%x\r\n",check_IRQ_pending(),bk_dma_check_chn_status());
				return;
			}
			sys_ll_set_cpu2_int_32_63_en_cpu2_mailbox_int_en(1);
			/*Set cpu2 wfi state*/
			aon_pmu_ll_set_r3_cp2_enter_wfi_state(1);

			FIXED_ADDR_WAKEUP_AP1_DEBUG +=1;
			/*Enter deep sleep*/
			arch_deep_sleep();

			/*Clear cpu2 wfi state*/
			aon_pmu_ll_set_r3_cp2_enter_wfi_state(0);

			portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
			bk_err_t ret = vPortYieldCore(0);
			s_trigger_ap0_count =  0;
			while(ret != BK_OK)
			{
				bk_delay_us(PM_AP_TRRIGER_DELAY_TIME_US);
				ret = vPortYieldCore(0);
				s_trigger_ap0_count++;
				if(s_trigger_ap0_count > PM_TRRIGER_AP_MAX_COUNT)
				{
					LOGE("Wakeup AP0 failed[%d]\r\n",ret);
					break;
				}
			}
			sys_ll_set_cpu2_int_0_31_en_value(int1_state1);
			sys_ll_set_cpu2_int_32_63_en_value(int1_state2);
		}
		else
		{
			arch_sleep();
		}
	}
#else
	arch_sleep();
#endif
}

void sys_hal_enter_normal_wakeup()
{
}

void sys_hal_enable_mac_wakeup_source()
{
	uint8_t wakeup_ena = aon_pmu_ll_get_r41_wakeup_ena();
	wakeup_ena |= BIT(WAKEUP_SOURCE_INT_WIFI);
	aon_pmu_ll_set_r41_wakeup_ena(wakeup_ena);
}

void sys_hal_enable_bt_wakeup_source()
{
	uint8_t wakeup_ena = aon_pmu_ll_get_r41_wakeup_ena();
	wakeup_ena |= BIT(WAKEUP_SOURCE_INT_BT);
	aon_pmu_ll_set_r41_wakeup_ena(wakeup_ena);
}

void sys_hal_wakeup_interrupt_clear(wakeup_source_t interrupt_source)
{
	if (interrupt_source == WAKEUP_SOURCE_INT_USBPLUG) {
		aon_pmu_ll_set_r43_clr_int_usbplug(1);
		aon_pmu_ll_set_r43_clr_int_usbplug(0);
	} else if (interrupt_source == WAKEUP_SOURCE_INT_TOUCHED) {
		aon_pmu_ll_set_r43_clr_int_touched(1);
		aon_pmu_ll_set_r43_clr_int_touched(0);
	}
}

int sys_hal_set_lpo_src(sys_lpo_src_t src)
{
	PM_HAL_LOGV("set lpo src: %u\r\n", src);
	//TODO
	return BK_OK;
}

void sys_hal_enter_low_analog(void)
{
	return;
}

void sys_hal_exit_low_analog(void)
{
	return;
}

/**
 * set io ldo power mode
 *
 * uint32_t type input:
 *   0: high power mode
 *   1: low power mode
 *   other: undefine
*/
void sys_hal_set_ioldo_lp(uint32_t val)
{
    return;
}

void sys_hal_dco_switch_freq(dco_cali_speed_e speed)
{
	return;
}

static int sys_hal_dco_cali(dco_cali_speed_e speed)
{
	return 0;
}

static int sys_hal_config_32k_source_default()
{
	return 0;
}

static int sys_hal_enable_buck()
{
	return 0;
}

bk_err_t sys_hal_qspi0_cksel_clkdiv_set(cksel_qspi0_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg8_cksel_qspi0(cksel);
	sys_ahbp_ll_set_reg8_ckdiv_qspi0(ckdiv);

	return BK_OK;
}

bk_err_t sys_hal_qspi1_cksel_clkdiv_set(cksel_qspi1_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg8_cksel_qspi1(cksel);
	sys_ahbp_ll_set_reg8_ckdiv_qspi1(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_pram0_cksel_clkdiv_set(cksel_pram0_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg8_cksel_pram0(cksel);
	sys_ahbp_ll_set_reg8_ckdiv_pram0(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_mbist_cksel_set(cksel_mbist_t cksel)
{
	sys_ahbp_ll_set_reg8_cksel_mbist(cksel);
	return BK_OK;
}

bk_err_t sys_hal_sdio0_clkdiv_set(uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg8_ckdiv_sdio0(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_sdio1_clkdiv_set(uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg8_ckdiv_sdio1(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_cis_mclk_cksel_clkdiv_set(cksel_cis_mclk_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_cksel_cis_mclk(cksel);
	sys_ahbp_ll_set_reg9_ckdiv_cis_mclk(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_cis_auxs_clkdiv_set(uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_ckdiv_cis_auxs(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_cisp_cksel_clkdiv_set(cksel_cisp_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_cksel_cisp(cksel);
	sys_ahbp_ll_set_reg9_ckdiv_cisp(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_gpu_cksel_clkdiv_set(cksel_gpu_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_cksel_gpu(cksel);
	sys_ahbp_ll_set_reg9_ckdiv_gpu(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_h265_cksel_clkdiv_set(cksel_h265_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_cksel_h265(cksel);
	sys_ahbp_ll_set_reg9_ckdiv_h265(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_dpu_cksel_clkdiv_set(cksel_dpu_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_cksel_dpu(cksel);
	sys_ahbp_ll_set_reg9_ckdiv_dpu(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_pram1_cksel_clkdiv_set(cksel_pram1_t cksel, uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_cksel_pram1(cksel);
	sys_ahbp_ll_set_reg9_ckdiv_pram1(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_trace_clkdiv_set(uint32_t ckdiv)
{
	sys_ahbp_ll_set_reg9_ckdiv_trace(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_cis_auxs_cksel_set(cksel_cis_auxs_t cksel)
{
	sys_ahbp_ll_set_reg9_cksel_cis_auxs(cksel);
	return BK_OK;
}

bk_err_t sys_hal_flash_cksel_clkdiv_set(cksel_sys_flash_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(cksel);
	sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_auxs_cksel_clkdiv_set(cksel_sys_auxs_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_auxs(cksel);
	sys_ll_set_cpu_clk_div_mode1_ckdiv_auxs(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_26mo_clkdiv_set(uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode1_ckdiv_26mo(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_tim0_cksel_set(cksel_sys_tim_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_tim0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_tim1_cksel_set(cksel_sys_tim_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_tim1(cksel);
	return BK_OK;
}

bk_err_t sys_hal_tim2_cksel_set(cksel_sys_tim_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_tim2(cksel);
	return BK_OK;
}

bk_err_t sys_hal_tim3_cksel_set(cksel_sys_tim_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_tim3(cksel);
	return BK_OK;
}

bk_err_t sys_hal_i3c_cksel_set(cksel_sys_xtal_apll_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i3c(cksel);
	return BK_OK;
}

bk_err_t sys_hal_sadc_cksel_set(cksel_sys_xtal_apll_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_sadc(cksel);
	return BK_OK;
}

bk_err_t sys_hal_i2s0_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s0(cksel);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_i2s1_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s1(cksel);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s1(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_i2s2_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s2(cksel);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s2(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_i2s3_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s3(cksel);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s3(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_i2s4_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2s4(cksel);
	sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s4(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_spi0_cksel_set(cksel_sys_xtal_160m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_spi0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_spi1_cksel_set(cksel_sys_xtal_160m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_spi1(cksel);
	return BK_OK;
}

bk_err_t sys_hal_spi2_cksel_set(cksel_sys_xtal_160m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_spi2(cksel);
	return BK_OK;
}

bk_err_t sys_hal_spi3_cksel_set(cksel_sys_xtal_160m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_spi3(cksel);
	return BK_OK;
}

bk_err_t sys_hal_uart0_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_uart0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_uart1_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_uart1(cksel);
	return BK_OK;
}

bk_err_t sys_hal_uart2_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_uart2(cksel);
	return BK_OK;
}

bk_err_t sys_hal_uart3_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_uart3(cksel);
	return BK_OK;
}

bk_err_t sys_hal_uart4_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_uart4(cksel);
	return BK_OK;
}

bk_err_t sys_hal_i2c0_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2c0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_i2c3_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode2_cksel_i2c3(cksel);
	return BK_OK;
}

bk_err_t sys_hal_pwm0_cksel_set(cksel_sys_pwm0_t cksel)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_pwm0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_can0_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_can0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_can1_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_can1(cksel);
	return BK_OK;
}

bk_err_t sys_hal_scr0_cksel_set(cksel_sys_xtal_120m_t cksel)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_scr0(cksel);
	return BK_OK;
}

bk_err_t sys_hal_audio_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_audio(cksel);
	sys_ll_set_cpu_clk_div_mode3_ckdiv_audio(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_audif0_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_audif0(cksel);
	sys_ll_set_cpu_clk_div_mode3_ckdiv_audif0(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_audif1_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_audif1(cksel);
	sys_ll_set_cpu_clk_div_mode3_ckdiv_audif1(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_i2so_clkdiv_set(uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode3_ckdiv_i2so(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_auxs_enet_cksel_clkdiv_set(cksel_sys_dco_apll_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_auxs_enet(cksel);
	sys_ll_set_cpu_clk_div_mode3_ckdiv_auxs_enet(ckdiv);
	return BK_OK;
}

bk_err_t sys_hal_trace_cksel_clkdiv_set(cksel_sys_trace_t cksel, uint32_t ckdiv)
{
	sys_ll_set_cpu_clk_div_mode3_cksel_trace(cksel);
	sys_ll_set_cpu_clk_div_mode3_ckdiv_trace(ckdiv);
	return BK_OK;
}
bk_err_t sys_hal_auxldo_enable(auxldo_sel_t auxldo_sel,uint32_t value)
{
	if(value > 1)
	{
		return BK_ERR_PARAM;
	}
	switch (auxldo_sel)
	{
		case AUXLDOS_SEL_3V:
			if(sys_ll_get_ana_reg41_en_auxldo3v() != value)
			{
				sys_ll_set_ana_reg41_en_auxldo3v(value);
			}
			break;
		case AUXLDOS_SEL_2P8V:
			if(sys_ll_get_ana_reg41_en_auxldo2p8v() != value)
			{
				sys_ll_set_ana_reg41_en_auxldo2p8v(value);
			}
			break;
		case AUXLDOS_SEL_1P8V:
			if(sys_ll_get_ana_reg41_en_auxldo_1p8v() != value)
			{
				sys_ll_set_ana_reg41_en_auxldo_1p8v(value);
			}
			break;
		case AUXLDOS_SEL_1P2V:
			if(sys_ll_get_ana_reg41_en_auxldo_1p2v() != value)
			{
				sys_ll_set_ana_reg41_en_auxldo_1p2v(value);
			}
			break;
		case AUXLDOS_SEL_NONE:
			break;
		default:
			return BK_ERR_PARAM;
	}
	if(value != 0)
	{
		bk_delay_us(10);
	}
	return BK_OK;
}
uint32_t sys_hal_auxldo_enable_state_get(auxldo_sel_t auxldo_sel)
{
	switch (auxldo_sel)
	{
		case AUXLDOS_SEL_3V:
			return sys_ll_get_ana_reg41_en_auxldo3v();
		case AUXLDOS_SEL_2P8V:
			return sys_ll_get_ana_reg41_en_auxldo2p8v();
		case AUXLDOS_SEL_1P8V:
			return sys_ll_get_ana_reg41_en_auxldo_1p8v();
		case AUXLDOS_SEL_1P2V:
			return sys_ll_get_ana_reg41_en_auxldo_1p2v();
		case AUXLDOS_SEL_NONE:
			return BK_ERR_NOT_SUPPORT;
		default:
			return BK_ERR_NOT_SUPPORT;
	}
}
bk_err_t sys_hal_auxldo_swb_set(auxldo_swb_t swb, bool bypass)
{
	/* ana_reg41: 0 = bypass：This interface is used so that, when configured to 0,
	 the 2.8 V and 3 V LDOs are directly connected to VBAT, and their output voltage is the same as VBAT */
	uint32_t v = bypass ? 0U : 1U;

	switch (swb)
	{
		case AUXLDO_SWB_2P8V:
			sys_ll_set_ana_reg41_swb_auxldo2p8v(v);
			break;
		case AUXLDO_SWB_3V:
			sys_ll_set_ana_reg41_swb_auxldo3v(v);
			break;
		default:
			return BK_ERR_PARAM;
	}
	return BK_OK;
}

uint32_t sys_hal_auxldo_swb_get(auxldo_swb_t swb)
{
	switch (swb)
	{
		case AUXLDO_SWB_2P8V:
			return sys_ll_get_ana_reg41_swb_auxldo2p8v();
		case AUXLDO_SWB_3V:
			return sys_ll_get_ana_reg41_swb_auxldo3v();
		default:
			return 0U;
	}
}

bk_err_t sys_hal_auxldo_out_set(auxldo_sel_t auxldo_sel, uint32_t value)
{
	switch (auxldo_sel)
	{
		case AUXLDOS_SEL_1P2V://for 1.2v camera peripheral
			if (value > 7U)
			{
				return BK_ERR_PARAM;
			}
			if(sys_ll_get_ana_reg41_vsel_auxldo1p2v() != value)
			{
				sys_ll_set_ana_reg41_vsel_auxldo1p2v(value);
			}
			break;
		case AUXLDOS_SEL_1P8V://for 1.8v camera peripheral
			if (value > 15U)
			{
				return BK_ERR_PARAM;
			}
			if(sys_ll_get_ana_reg41_vsel_auxldo1p8v() != value)
			{
				sys_ll_set_ana_reg41_vsel_auxldo1p8v(value);
			}
			break;
		case AUXLDOS_SEL_2P8V://for PHY: DSI VDDH:display, CSI_VDDH:camera  and the current load is 50 mA
			if (value > 15U)
			{
				return BK_ERR_PARAM;
			}
			if(sys_ll_get_ana_reg41_vsel_auxldo2p8v() != value)
			{
				sys_ll_set_ana_reg41_vsel_auxldo2p8v(value);
			}
			break;
		case AUXLDOS_SEL_3V://for 3.0v camera peripheral and the current load is 100 mA
			if (value > 15U)
			{
				return BK_ERR_PARAM;
			}
			if(sys_ll_get_ana_reg41_vsel_auxldo3v() != value)
			{
				sys_ll_set_ana_reg41_vsel_auxldo3v(value);
			}
			break;
		default:
			return BK_ERR_PARAM;
	}
	return BK_OK;
}

uint32_t sys_hal_auxldo_out_get(auxldo_sel_t auxldo_sel)
{
	switch (auxldo_sel)
	{
		case AUXLDOS_SEL_1P2V:
			return sys_ll_get_ana_reg41_vsel_auxldo1p2v();
		case AUXLDOS_SEL_1P8V:
			return sys_ll_get_ana_reg41_vsel_auxldo1p8v();
		case AUXLDOS_SEL_2P8V:
			return sys_ll_get_ana_reg41_vsel_auxldo2p8v();
		case AUXLDOS_SEL_3V:
			return sys_ll_get_ana_reg41_vsel_auxldo3v();
		default:
			return 0U;
	}
}

static int sys_hal_power_config_default()
{
    return 0;
}
void sys_hal_low_power_hardware_init()
{
	#if !CONFIG_AON_PMU_REG0_REFACTOR_DEV
	/*recover aon pmu reg0*/
	uint32_t reg = aon_pmu_ll_get_r7b();
	aon_pmu_ll_set_r0(reg);
	#endif

#if CONFIG_GPIO_RETENTION_SUPPORT
	// must before gpio state unlock
	gpio_retention_sync(true);
#endif

	/*gpio state unlock for shutdown wakeup*/
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
	aon_pmu_hal_set_gpio_sleep(0, true);
#else
	sys_hal_gpio_state_switch(false);
#endif

	/*set memery bypass*/
	aon_pmu_ll_set_r0_memchk_bps(1);
	aon_pmu_ll_set_r0_fast_boot(1);

	/*set wakeup source*/
	aon_pmu_ll_set_r41_wakeup_ena(0x23);//enable wakeup source: int_touched,int_rtc,int_gpio,wifi wake(bt or wifi wakeup source enable when bt or wifi sleep)

	/*enable the buck*/
	#if CONFIG_BUCK_ENABLE
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) != (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK)){
		sys_hal_enable_buck();
	}
	#endif
	/*select lowpower lpo clk source*/
	sys_hal_config_32k_source_default();

	/*default to config the power */
	sys_hal_power_config_default();

	/*set the lp voltage*/
	sys_hal_lp_vol_set(CONFIG_LP_VOL);

	/*set rosc calib trig once*/
	sys_hal_rosc_calibration(3, 0);

	/*dco cali*/
	sys_hal_dco_cali(DCO_CALIB_SPEED_240M);
}

void sys_hal_set_ota_finish(uint32_t value)
{
	aon_pmu_ll_set_r0_ota_finish(value);
}

uint32_t sys_hal_get_ota_finish(void)
{
	return aon_pmu_ll_get_r0_ota_finish();
}
