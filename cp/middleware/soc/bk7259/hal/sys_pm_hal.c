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
#include <modules/pm.h>
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
#include "modules/pm.h"

#include "sys_sw_regs.h"
#include <driver/pwr_clk.h>
#include "driver/flash.h"
#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif
#if CONFIG_SUPPORT_WWDT
#include <driver/wwdt.h>
#endif
#if CONFIG_SUPPORT_CACHEABLE_SRAM
#include "cache.h"
#endif
#if CONFIG_DEEP_LV
#include "FreeRTOS.h"
//#include "armstar.h"
#include "deep_lv/deep_lv.h"
#endif
#if CONFIG_MPU
#include "mpu.h"
#endif

#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )

#define portNVIC_INT_CTRL_REG                 ( *( ( volatile uint32_t * ) 0xe000ed04 ) )

#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )

#define portNVIC_PENDSVSET_BIT                ( 1UL << 28UL )
#define portNVIC_PENDSVCLR_BIT                ( 1UL << 27UL )
#define portNVIC_SYSTICKSET_BIT               ( 1UL << 26UL )
#define portNVIC_SYSTICKCLR_BIT               ( 1UL << 25UL )

#define PM_EXIT_LOWVOL_SYSTICK_TIME           (32)      //1ms
#define PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME    (0xFFFFFF)//set max
#define PM_LOW_VOL_AON_LDO_SEL                (2)       // 0.7V, no rosc32k output when 0.65V under -45��
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
uint64_t low_voltage_sleep_duration_us = 0;
uint64_t low_voltage_wakeup_time_us = 0;
extern void bk_delay_us(UINT32 us);
extern uint64_t check_IRQ_pending(void);
extern void sys_hal_analog_set(analog_reg_t reg, uint32_t value);

static inline void sys_hal_enable_spi_latch(void);
static inline void sys_hal_disable_spi_latch(void);
#if CONFIG_DEEP_LV
__attribute__((section(".iram"))) void sys_hal_regs_digital_restore(void);
#endif
void sys_hal_analog_set_default(void)
{
	sys_hal_enable_spi_latch();
	sys_hal_analog_set(ANALOG_REG0, 0x71105b57);
	// sys_hal_analog_set(ANALOG_REG1, 0x00655044);
	// sys_hal_analog_set(ANALOG_REG2, 0x7e003430);
	/*------------------*/
	// sys_hal_analog_set(ANALOG_REG3, 0xc4500a88);
	// sys_hal_analog_set(ANALOG_REG4, 0x0001a7f0);
	/*------------------*/
	// sys_hal_analog_set(ANALOG_REG5, 0x8407a310);
	// sys_hal_analog_set(ANALOG_REG6, 0x80088200);
	// sys_hal_analog_set(ANALOG_REG7, 0x622e7080);
	// sys_hal_analog_set(ANALOG_REG8, 0x0c4ec4ec);
	// sys_hal_analog_set(ANALOG_REG9, 0x57c62323);
	/*------------------*/
	// sys_hal_analog_set(ANALOG_REG10, 0xf8aec0a4);
	// sys_hal_analog_set(ANALOG_REG11, 0x6255c3a7);
	// sys_hal_analog_set(ANALOG_REG12, 0xd47a90fa);
	// sys_hal_analog_set(ANALOG_REG13, 0xd47ac36a);

	// sys_hal_analog_set(ANALOG_REG14, 0x5c6af0ee);
	// sys_hal_analog_set(ANALOG_REG15, 0);

	// sys_hal_analog_set(ANALOG_REG16, 0x9e434000);
	// sys_hal_analog_set(ANALOG_REG17, 0x00400000);
	// sys_hal_analog_set(ANALOG_REG18, 0x00100000);
	// sys_hal_analog_set(ANALOG_REG19, 0x0ef1da51);
	// sys_hal_analog_set(ANALOG_REG20, 0x00000000);

	// sys_hal_analog_set(ANALOG_REG21, 0x00000000);
	/* 80uA start */
	sys_hal_analog_set(ANALOG_REG22, 0x7e805440);
	// uint32_t val = sys_hal_analog_get(ANALOG_REG22);
	// val &= ~(0x3 << 30);
	// val &= ~(0x1 << 24);
	// sys_hal_analog_set(ANALOG_REG22, val);

	sys_hal_analog_set(ANALOG_REG23, 0x00000033);
	// val = sys_hal_analog_get(ANALOG_REG23);
	// val &= ~(0x1 << 2);
	// sys_hal_analog_set(ANALOG_REG23, val);

	// val = sys_hal_analog_get(ANALOG_REG23);
	// val &= ~(0x1 << 8);
	// sys_hal_analog_set(ANALOG_REG23, val);
	sys_hal_analog_set(ANALOG_REG24, 0x00000033);

	sys_hal_analog_set(ANALOG_REG25, 0x8971faa4);
	sys_hal_analog_set(ANALOG_REG26, 0xc2a0ae86);
	sys_hal_analog_set(ANALOG_REG27, 0x00000000);

	// val = sys_hal_analog_get(ANALOG_REG26);
	// val &= ~(0x3 << 30);
	// sys_hal_analog_set(ANALOG_REG26, val);

	/* 80uA end */

	// sys_hal_analog_set(ANALOG_REG28, 0x00000000);
	// sys_hal_analog_set(ANALOG_REG29, 0x00000000);
	// sys_hal_analog_set(ANALOG_REG30, 0x00000000);
	// sys_hal_analog_set(ANALOG_REG31, 0x00000000);
	// sys_hal_analog_set(ANALOG_REG32, 0xc0080120);

	// sys_hal_analog_set(ANALOG_REG33, 0x40000144);
	//sys_hal_analog_set(ANALOG_REG33, 0x40000344);//for test
	// sys_hal_analog_set(ANALOG_REG34, 0x04c40000);
	// sys_hal_analog_set(ANALOG_REG35, 0x04000000);

	/*--------------------- */
	// sys_hal_analog_set(ANALOG_REG36, 0x00000000);
	// sys_hal_analog_set(ANALOG_REG37, 0x00512e40);
	// sys_hal_analog_set(ANALOG_REG38, 0x04003000);
	// sys_hal_analog_set(ANALOG_REG39, 0x00006792);
	// sys_hal_analog_set(ANALOG_REG40, 0x00006792);

	// sys_hal_analog_set(ANALOG_REG41, 0x0c3bd800);
	// sys_hal_analog_set(ANALOG_REG42, 0xc0000000);

	sys_hal_disable_spi_latch();
}

void sys_hal_analog_set_deep_sleep_default(void)
{
	sys_hal_enable_spi_latch();
	sys_hal_analog_set(ANALOG_REG0, 0x71105b57);
	sys_hal_analog_set(ANALOG_REG1, 0x00655044);
	sys_hal_analog_set(ANALOG_REG2, 0x7e003430);

	sys_hal_analog_set(ANALOG_REG3, 0xc4500a88);
	sys_hal_analog_set(ANALOG_REG4, 0x0001a7f0);

	sys_hal_analog_set(ANALOG_REG5, 0x8407a310);
	sys_hal_analog_set(ANALOG_REG6, 0x80088200);
	sys_hal_analog_set(ANALOG_REG7, 0x622e7080);
	sys_hal_analog_set(ANALOG_REG8, 0x0c4ec4ec);
	sys_hal_analog_set(ANALOG_REG9, 0x57c62323);

	sys_hal_analog_set(ANALOG_REG10, 0xf8aec0a4);
	sys_hal_analog_set(ANALOG_REG11, 0x6255c3a7);
	sys_hal_analog_set(ANALOG_REG12, 0xd47a90fa);
	sys_hal_analog_set(ANALOG_REG13, 0xd47ac36a);

	sys_hal_analog_set(ANALOG_REG14, 0x5c6af0ee);
	sys_hal_analog_set(ANALOG_REG15, 0);

	sys_hal_analog_set(ANALOG_REG16, 0x9e434000);
	sys_hal_analog_set(ANALOG_REG17, 0x00400000);
	sys_hal_analog_set(ANALOG_REG18, 0x00100000);
	sys_hal_analog_set(ANALOG_REG19, 0x0ef1da51);
	sys_hal_analog_set(ANALOG_REG20, 0x00000000);

	sys_hal_analog_set(ANALOG_REG21, 0x00000000);
	/* 80uA start */
	sys_hal_analog_set(ANALOG_REG22, 0x7e805440);

	sys_hal_analog_set(ANALOG_REG23, 0x00000033);
	sys_hal_analog_set(ANALOG_REG24, 0x00000033);

	sys_hal_analog_set(ANALOG_REG25, 0x8971faa4);
	sys_hal_analog_set(ANALOG_REG26, 0xc2a0ae86);
	sys_hal_analog_set(ANALOG_REG27, 0x00000000);
	/* 80uA end */

	sys_hal_analog_set(ANALOG_REG28, 0x00000000);
	sys_hal_analog_set(ANALOG_REG29, 0x00000000);
	sys_hal_analog_set(ANALOG_REG30, 0x00000000);
	sys_hal_analog_set(ANALOG_REG31, 0x00000000);
	sys_hal_analog_set(ANALOG_REG32, 0xc0080120);

	sys_hal_analog_set(ANALOG_REG33, 0x40000144);
	sys_hal_analog_set(ANALOG_REG34, 0x04c40000);
	sys_hal_analog_set(ANALOG_REG35, 0x04000000);

	/*--------------------- */
	sys_hal_analog_set(ANALOG_REG36, 0x00000000);
	sys_hal_analog_set(ANALOG_REG37, 0x00512e40);
	sys_hal_analog_set(ANALOG_REG38, 0x04003000);
	sys_hal_analog_set(ANALOG_REG39, 0x00006792);
	sys_hal_analog_set(ANALOG_REG40, 0x00006792);

	sys_hal_analog_set(ANALOG_REG41, 0x0c3bd800);
	sys_hal_analog_set(ANALOG_REG42, 0xc0000000);

	sys_hal_disable_spi_latch();
}
void sys_hal_gpio_state_sleep_default(void)
{
	for (gpio_id_t i = 0; i < GPIO_NUM_MAX; i++)
	{
		bk_gpio_set_value(i, 0);
	}
}
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
	return !(pd_bits & PD_WRLS);
}

static inline bool is_btsp_pd_poweron(uint32_t pd_bits)
{
	return !(pd_bits & PD_WRLS);
}

static inline void sys_hal_backup_disable_int(volatile uint32_t *int_state1, volatile uint32_t *int_state2)
{
	uint8_t ws_ena = aon_pmu_ll_get_r41_wakeup_ena();
	uint32_t int0_enabled_ws_ints = 0;
	uint32_t int1_enabled_ws_ints = 0;

	if (is_wifi_ws_enabled(ws_ena)) {
		int0_enabled_ws_ints |= WS_WIFI_INT0;
		int1_enabled_ws_ints |= WS_WIFI_INT1;
	}

	if (is_gpio_ws_enabled(ws_ena)) {
		int1_enabled_ws_ints |= WS_GPIO_INT;
	}

	if (is_rtc_ws_enabled(ws_ena)) {
		int1_enabled_ws_ints |= WS_RTC_INT;
	}

	if (is_bt_ws_enabled(ws_ena)) {
		int1_enabled_ws_ints |= WS_BT_INT;
	}

	if (is_usbplug_ws_enabled(ws_ena)) {
		int1_enabled_ws_ints |= WS_USBPLUG_INT;
	}

	if (is_touch_ws_enabled(ws_ena)) {
		int1_enabled_ws_ints |= WS_TOUCH_INT;
	}

	sys_ll_set_cpu0_int_0_31_en_value(*int_state1 & int0_enabled_ws_ints);
	sys_ll_set_cpu0_int_32_63_en_value(*int_state2 & int1_enabled_ws_ints);

}

static inline void sys_hal_restore_int(volatile uint32_t int_state1, volatile uint32_t int_state2, volatile uint32_t int_state3)
{
	sys_ll_set_cpu0_int_0_31_en_value(int_state1);
	sys_ll_set_cpu0_int_32_63_en_value(int_state2);
	sys_ll_set_cpu0_int_64_95_en_value(int_state3);
}

static inline void sys_hal_enable_wakeup_int(void)
{
}

static inline void sys_hal_set_core_freq(volatile uint8_t cksel_core, volatile uint8_t clkdiv_core, volatile uint8_t clkdiv_bus)
{
        sys_ll_set_cpu_clk_div_mode1_cksel_core(cksel_core);
        sys_ll_set_cpu_clk_div_mode1_ckdiv_core(clkdiv_core);
        ;//TODO: sys_ll_set_cpu_clk_div_mode1_clkdiv_bus(clkdiv_bus);
}

static inline void sys_hal_set_core_26m(void)
{
	sys_hal_set_core_freq(0, 0, 0);
}

static inline void sys_hal_backup_set_core_26m(volatile uint8_t *cksel_core, volatile uint8_t *clkdiv_core, volatile uint8_t *clkdiv_bus)
{
	IF_LV_CTRL_CORE() {
		*cksel_core = sys_ll_get_cpu_clk_div_mode1_cksel_core();
		*clkdiv_core = sys_ll_get_cpu_clk_div_mode1_ckdiv_core();
		;//TODO: *clkdiv_bus = sys_ll_get_cpu_clk_div_mode1_clkdiv_bus();
		sys_hal_set_core_freq(0, 0, 0);
	}
}

static inline void sys_hal_restore_core_freq(volatile uint8_t cksel_core, volatile uint8_t clkdiv_core, volatile uint8_t clkdiv_bus)
{
	IF_LV_CTRL_CORE() {
		;//TODO: sys_ll_set_cpu_clk_div_mode1_clkdiv_bus(clkdiv_bus);
		sys_ll_set_cpu_clk_div_mode1_ckdiv_core(clkdiv_core);
		sys_ll_set_cpu_clk_div_mode1_cksel_core(cksel_core);
	}
}

static inline void sys_hal_set_flash_freq(volatile uint8_t cksel_flash, volatile uint8_t ckdiv_flash)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(cksel_flash);
	sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(ckdiv_flash);
}

static inline void sys_hal_set_flash_26m(void)
{
	sys_hal_set_flash_freq(0, 0);
}

static inline void sys_hal_backup_set_flash_26m(volatile uint8_t *cksel_flash, volatile  uint8_t *ckdiv_flash)
{
	IF_LV_CTRL_FLASH() {
		*cksel_flash = sys_ll_get_cpu_clk_div_mode1_cksel_flash();
		*ckdiv_flash = sys_ll_get_cpu_clk_div_mode1_ckdiv_flash();
		sys_ll_set_cpu_clk_div_mode1_cksel_flash(0);//eg:from the 80m to 26m, it need select clk source first
		sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(0);//then ckdiv
	}
}

static inline void sys_hal_set_flash_120m(void)
{
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(2);
	sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(1);
}

static inline void sys_hal_set_anaspi_freq(uint8_t anaspi_freq)
{
	sys_ll_set_cpu_anaspi_freq_value(anaspi_freq);
}

static inline void sys_hal_backup_set_anaspi_freq_26m(uint8_t *anaspi_freq)
{
	*anaspi_freq = sys_ll_get_cpu_anaspi_freq_value();
	sys_hal_set_anaspi_freq(1); //26M/4
}

static inline void sys_hal_restore_anaspi_freq(uint8_t anaspi_freq)
{
	sys_hal_set_anaspi_freq(anaspi_freq);
}

static inline void sys_hal_restore_flash_freq(volatile uint8_t cksel_flash, volatile uint8_t ckdiv_flash)
{
	sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(ckdiv_flash);//eg:from the 26m to 80m, it need config clk div first
	sys_ll_set_cpu_clk_div_mode1_cksel_flash(cksel_flash);//then clk source

}

static inline void sys_hal_mask_cpu0_int(void)
{
	sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask(1);
}

static inline void sys_hal_enable_spi_latch(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
}

static inline void sys_hal_disable_spi_latch(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(0);
}

static inline void sys_hal_disable_audio(void)
{
	sys_ll_set_ana_reg5_bcal_en(0);//bit23  audio bias calibration disable
	sys_ll_set_ana_reg5_pwdaudpll(0);//bit13  audio pll power disable

	sys_ll_set_ana_reg20_enadcbias(0);//bit4 audio bias disable
    sys_ll_set_ana_reg21_micen_mic1(0);//bit28  audio mic1 disable
	sys_ll_set_ana_reg27_micen_mic2(0);//bit28  audio mic2 disable
	sys_ll_set_ana_reg28_micen_mic3(0);//bit28  audio mic3 disable
	sys_ll_set_ana_reg30_enbs(0);//bit23  audio dac bias disable

	sys_ll_set_ana_reg29_rendcoc(0);//bit15  audio dac L DCOC disable
	sys_ll_set_ana_reg29_lendcoc(0);//bit16  audio dac R DCOS disable
	sys_ll_set_ana_reg29_dacren(0);//bit20  audio dac driver enable
	sys_ll_set_ana_reg29_daclen(0);//bit21  audio dac driver enable

	sys_ll_set_ana_reg30_enidacr(0);//bit17  audio idac R disable
	sys_ll_set_ana_reg30_enidacl(0);//bit18  audio idac L disable

	sys_ll_set_ana_reg32_rstb_dig(1);//bit8  touch rstb digital disable
	sys_ll_set_ana_reg32_ldoen(0);//bit5  touch ldo  disable

	sys_ll_set_ana_reg41_en_auxldo3v(0);//bit28  audio auxldo3v disable
	sys_ll_set_ana_reg41_en_auxldo2p8v(0);//bit29  audio auxldo2p8v disable
	sys_ll_set_ana_reg41_en_auxldo_1p8v(0);//bit30  audio auxldo_1p8v disable
	sys_ll_set_ana_reg41_en_auxldo_1p2v(0);//bit31  audio auxldo_1p2v disable

	//psram ldo
	sys_ll_set_ana_reg14_enpsram(0);//bit31  psram power down
	//hs usb ldo

	//fs usb
	sys_ll_set_ana_reg42_pwd_usb(1);//bit31  usb power down

	sys_ll_set_ana_reg43_en_anacomp_a(0);//bit10  audio anacomp disable
	sys_ll_set_ana_reg43_en_anacomp_b(0);//bit26  audio anacomp disable


    uint32_t val = sys_ll_get_ana_reg8_value();
	val &= ~(0x1 << 19);//PMU BUCKH
	val &= ~(0x1 << 13);//PMU BUCKA
	sys_ll_set_ana_reg8_value(val);


	sys_ll_set_ana_reg9_pwd_bgcal(1); //bit30  PMU_LPBG spd bgcal1v power down
	sys_ll_set_ana_reg9_spi_envbg(0); //bit31  PMU_LPBG spi_envbg1v power down
}
static inline uint32_t sys_hal_disable_hf_clock(void)
{
	uint32_t val = sys_ll_get_ana_reg5_value();
	uint32_t ret_val = val;

	val &= ~EN_ALL;

	if (is_lpo_src_ext32k()) {
	val |= EN_XTAL;
	}

	sys_ll_set_ana_reg5_value(val);



	return ret_val;
}

static inline void sys_hal_restore_hf_clock(volatile uint32_t val)
{
	sys_ll_set_ana_reg5_value(val);
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
	volatile uint8_t cksel_core = 0, clkdiv_core = 0, clkdiv_bus = 0;

	if (flag == 0) {
		os_printf("disable buckA and buckD.\r\n");
		sys_hal_enable_spi_latch();
		sys_ll_set_ana_reg12_aldosel(1);
		sys_ll_set_ana_reg13_dldosel(1);
		sys_hal_disable_spi_latch();
	} else if (flag == 1){
		os_printf("enable buckA and buckD.\r\n");
		//let the cpu frequency to 26m, in order to be successfully switch voltage provide from ldo to buck
		sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);

		sys_hal_enable_spi_latch();
		sys_ll_set_ana_reg12_aldosel(0);
		sys_ll_set_ana_reg13_dldosel(0);
		sys_hal_disable_spi_latch();

		sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	} else if (flag == 2){
		os_printf("enable buckA and disable buckD.\r\n");
		//let the cpu frequency to 26m, in order to be successfully switch voltage provide from ldo to buck
		sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);

		sys_hal_enable_spi_latch();
		sys_ll_set_ana_reg12_aldosel(0);
		sys_ll_set_ana_reg13_dldosel(1);
		sys_hal_disable_spi_latch();

		sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	} else if (flag == 3){
		os_printf("disable buckA and enable buckD.\r\n");
		//let the cpu frequency to 26m, in order to be successfully switch voltage provide from ldo to buck
		sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);
		//disable buckA enable buckD
		sys_hal_enable_spi_latch();
		sys_ll_set_ana_reg12_aldosel(1);
		sys_ll_set_ana_reg13_dldosel(0);
		sys_hal_disable_spi_latch();

		sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	}
	else {
		os_printf("set buck power supply param %d must < 4 \r\n",flag);
	}
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
	sys_hal_enable_spi_latch();
	if(sys_ll_get_ana_reg10_vcorehsel() != value)
	{
		sys_ll_set_ana_reg10_vcorehsel(value);
	}
	sys_hal_disable_spi_latch();
}
static inline void sys_hal_deep_sleep_set_buck(void)
{
	sys_ll_set_ana_reg12_aldosel(1);
	sys_ll_set_ana_reg13_dldosel(1);
}

static inline void sys_hal_deep_sleep_set_vldo(void)
{
	sys_ll_set_ana_reg9_coreldo_hp(0);
	/*dldohp disabling causes OTP read failed!, so it can't set 0*/
	sys_ll_set_ana_reg9_dldohp(1);

	sys_ll_set_ana_reg9_aldohp(0); //20230423 tenglong
}

static inline void sys_hal_clear_wakeup_source(void)
{
	aon_pmu_ll_set_r43_clr_wakeup(1);
	aon_pmu_ll_set_r43_clr_wakeup(0);
}

static inline void sys_hal_set_halt_config(pm_sleep_mode_e sleep_mode)
{

	if(sleep_mode == PM_MODE_LOW_VOLTAGE)
	{ /* sram0/1/2 default on */
#if CONFIG_DEEP_LV
		aon_pmu_ll_set_r41_halt_lpo(0);
		aon_pmu_ll_set_r41_mem_ret_en(1);
#else
		aon_pmu_ll_set_r41_halt_lpo(1);//must set to 1 if support wifi/ble wakeup.cost 0.3uA
		aon_pmu_ll_set_r41_mem_ret_en(0);
#endif
	}
	else
	{
		aon_pmu_ll_set_r41_halt_lpo(0);
		aon_pmu_ll_set_r41_halt_sram0(1);
		aon_pmu_ll_set_r41_halt_sram1(1);
		aon_pmu_ll_set_r41_halt_sram2(1);
		aon_pmu_ll_set_r41_mem_ret_en(1);
	}
}
static inline void sys_hal_power_on_and_select_rosc(pm_lpo_src_e lpo_src)
{
	volatile uint32_t count = PM_POWER_ON_ROSC_STABILITY_TIME;
	if((lpo_src == PM_LPO_SRC_ROSC)||(lpo_src == PM_LPO_SRC_DIVD))
	{
		if(sys_ll_get_ana_reg5_pwd_rosc_spi() != 0x0)
		{
			sys_ll_set_ana_reg5_pwd_rosc_spi(0x0);//power on rosc
			while(count--)//delay time for stability when power on rosc
			{
			}
		}

		if(aon_pmu_ll_get_r41_lpo_config() != PM_LPO_SRC_ROSC)
		{
			aon_pmu_ll_set_r41_lpo_config(PM_LPO_SRC_ROSC);
		}
	}
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
	if ((sleep_mode == PM_MODE_DEEP_SLEEP)
					#if CONFIG_DEEP_LV
						|| (sleep_mode == PM_MODE_LOW_VOLTAGE)
					#endif
	)
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
			val = (0x21111000
				|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
			#else
			val = (0x29111000
				|(PM_CURRENT_LOW_VOLTAGE_WAKEUP1_DELAY&0xF)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP2_DELAY&0xF)<<4)
				|((PM_CURRENT_LOW_VOLTAGE_WAKEUP3_DELAY&0xF)<<8));
			#endif
		}
		else//external 32k and rosc
		{
			#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
			val = (0x23111000
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
	sys_ll_set_cpu_power_sleep_wakeup_sleep_en_need_cpu1_wfi(1);
	sys_ll_set_cpu_power_sleep_wakeup_sleep_en_global(1);
}

static inline void sys_hal_power_down_pd(volatile uint32_t *pd_reg_v)
{
	IF_LV_CTRL_PD() {
		uint32_t v = sys_ll_get_reserver_reg0x10_value();
		*pd_reg_v = v;

		/*config power domain*/
		v |= PD_DOWN_DOMAIN;

		/*config r41 bus wl,bl pw and iso*/
		sys_hal_set_halt_config(PM_MODE_LOW_VOLTAGE);

#if CONFIG_DEEP_LV
		aon_pmu_ll_set_r42_pwd_wlppwd(1);
		aon_pmu_ll_set_r42_pwd_blppwd(1);
#else
		aon_pmu_ll_set_r42_pwd_wlppwd(0);

#if 0  //bt self-contorl
		uint32_t wakeup_source = 0;
		wakeup_source = aon_pmu_ll_get_r41_wakeup_ena();
		if(wakeup_source & (1<<WAKEUP_SOURCE_INT_BT))
			aon_pmu_ll_set_r42_pwd_blppwd(0);
		else
			aon_pmu_ll_set_r42_pwd_blppwd(1);
#endif
#endif

	sys_ll_set_reserver_reg0x10_value(v);

#if SYS_PM_TODO
        	uint32_t value = 0;
		/*config cpu0 subpwdm*/
		sys_ll_set_cpu_power_sleep_wakeup_cpu0_subpwdm_en(1);
		value = sys_ll_get_cpu0_lv_sleep_cfg();
		/*bit2(1: fpu powerdown  when lv sleep & cpu0_subpwdm_en==1'b1)*/
		/*bit0(1: cache retension when lv sleep & cpu0_subpwdm_en==1'b1)*/
		value |= (0x1 << 0) | (0x1 << 2);
		sys_ll_set_cpu0_lv_sleep_cfg(value);
#endif

#if CONFIG_DEEP_LV
		/* cpu is off during lv-sleep */
		aon_pmu_ll_set_r3_shutdown_flag(0);
		/* PMU_REG0x40: BIT[26]=1, BIT[29]=0, BIT[30]=1 */
		// aon_pmu_ll_set_r40_halt_volt(0);
		// aon_pmu_ll_set_r40_halt_core(1);
		// aon_pmu_ll_set_r40_halt_rosc(0);
		// aon_pmu_ll_set_r40_halt_resten(0);
		// aon_pmu_ll_set_r40_halt_isolat(1);
		// aon_pmu_ll_set_r40_halt_xtal(1);
#endif
		aon_pmu_ll_set_r2_otp_vdd_en(0);
		// sys_ll_set_reserver_reg0x10_pwd_cpu1(0x1);
		// sys_ll_set_reserver_reg0x10_pwd_wrls(0x1);
	}
}

static inline void sys_hal_power_on_pd(volatile uint32_t v_sys_r10)
{
	IF_LV_CTRL_PD() {
		sys_ll_set_reserver_reg0x10_value(v_sys_r10);
	}
}

static inline void sys_hal_set_wakeup_source(void)
{
	aon_pmu_ll_set_r41_wakeup_ena(0x3f);
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

static inline void sys_hal_set_low_voltage(pm_sleep_mode_e sleep_mode, volatile uint32_t *ana_r9, volatile uint32_t *core_low_voltage)
{
	sys_hal_enable_spi_latch();

	*ana_r9 = sys_ll_get_ana_reg9_value();
	*core_low_voltage = sys_ll_get_ana_reg10_vcorelsel();

	if (sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		sys_ll_set_ana_reg9_clk_sel(1);//bit0

		sys_ll_set_ana_reg9_t_vanaldosel(0);//tenglong20230417(need modify setting value)
		sys_ll_set_ana_reg9_r_vanaldosel(0);//tenglong20230417(need modify setting value)

		sys_ll_set_ana_reg10_vlden(1);//bit23//0x1: coreldo low voltage enable
		#if CONFIG_DIGLDO_LOW_VOLTAGE_ENABLE
		if(sys_ll_get_ana_reg10_vdd12lden() != 0x1)
		{
			sys_ll_set_ana_reg10_vdd12lden(1);//bit31//0x1: digldo low voltage enable
		}
		#else
		if(sys_ll_get_ana_reg10_vdd12lden() != 0x0)
		{
			sys_ll_set_ana_reg10_vdd12lden(0);//0x0: digldo low voltage disable
		}
		#endif
		sys_ll_set_ana_reg10_vlden(1);

		sys_ll_set_ana_reg9_valoldosel(PM_LOW_VOL_AON_LDO_SEL); //bit16
		//sys_ll_set_ana_reg9_aloldohp(0);//bit21

	#if CONFIG_LDO_SELF_LOW_POWER_MODE_ENA
		sys_ll_set_ana_reg9_clk_sel(0); //bit0 0:rosc 1:xtal 32k
		sys_ll_set_ana_reg9_coreldo_hp(0); //bit1 0:coreldo low power mode
		sys_ll_set_ana_reg9_dldohp(0); //bit2 0:dldo low power mode
		sys_ll_set_ana_reg9_aldohp(0); //bit10 0:aldohp low power mode
		sys_ll_set_ana_reg9_aloldohp(0); //bit21 0:aloldohp low power mode
	#endif

		//sys_ll_set_ana_reg12_enpowa(0); //bit13 0:bucka EA fast transient disable
	}
	else
	{
		sys_ll_set_ana_reg9_aldohp(0);//bit10
		sys_ll_set_ana_reg9_aloldohp(0);//bit21
		sys_ll_set_ana_reg9_dldohp(0);//bit2
		sys_ll_set_ana_reg9_hsldo_hp(0);//bit12
		//ronghui suggest ana0x49[1][2][10][12] 4bit at least 1bit = 1 when deepsleep,otherwise otp will not power on when wakeup
		sys_ll_set_ana_reg9_coreldo_hp(0);//bit1
	}
	sys_hal_disable_spi_latch();
}


__attribute__((section(".iram")))  void sys_hal_enter_deep_sleep(void *param)
{
	volatile uint32_t int_state1, int_state2, int_state3;
	uint32_t systick_ctrl_value = 0;
	pm_lpo_src_e lpo_src        = PM_LPO_SRC_ROSC;
	volatile uint32_t v_ana_r9, core_low_voltage;

	//workaround to fix that the BT wakesource cause deepsleep wakeup soon
	uint8_t wakesource_ena = aon_pmu_ll_get_r41_wakeup_ena();
	wakesource_ena &= ~BIT(WAKEUP_SOURCE_INT_BT);
	aon_pmu_ll_set_r41_wakeup_ena(wakesource_ena);

	portNVIC_INT_CTRL_REG |= portNVIC_SYSTICKCLR_BIT;
	systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
	portNVIC_SYSTICK_CTRL_REG = 0;//disable the systick, avoid it affect the enter deepsleep

	int_state1 = sys_ll_get_cpu0_int_0_31_en_value();
	int_state2 = sys_ll_get_cpu0_int_32_63_en_value();
	int_state3 = sys_ll_get_cpu0_int_64_95_en_value();
	sys_ll_set_cpu0_int_0_31_en_value(0x0);
	sys_ll_set_cpu0_int_32_63_en_value(0x0);
	sys_ll_set_cpu0_int_64_95_en_value(0x0);
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );
	__asm volatile( "nop" );

	/*confirm here hasn't external interrupt*/
	if(check_IRQ_pending()||(portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT))
	{
		sys_ll_set_cpu0_int_0_31_en_value(int_state1);
		sys_ll_set_cpu0_int_32_63_en_value(int_state2);
		sys_ll_set_cpu0_int_64_95_en_value(int_state3);
		portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
		return;
	}
	sys_hal_mask_cpu0_int();

	//sys_hal_gpio_state_sleep_default();

	sys_hal_set_core_26m();
	sys_hal_set_flash_26m();

#if CONFIG_INT_WDT
#if CONFIG_SUPPORT_WWDT
    bk_wwdt_stop();
#else
	bk_wdt_stop();
#endif
	#if CONFIG_TASK_WDT
	bk_task_wdt_stop();
	#endif
#endif

	/*enable several interrupt for wakeup*/
	// sys_ll_set_cpu0_int_32_63_en_cpu0_touched_int_en(0x1);
	sys_ll_set_cpu0_int_32_63_en_cpu0_gpio_s_int_en(0x1);
	sys_ll_set_cpu0_int_32_63_en_cpu0_rtc_int_en(0x1);

	lpo_src = aon_pmu_ll_get_r41_lpo_config();

	/*disable corresponding int before pwd*/
	// sys_ll_set_cpu0_int_32_63_en_cpu0_btdm_wake_up_int_en(0);
	// sys_ll_set_cpu1_int_32_63_en_cpu1_btdm_wake_up_int_en(0);
	aon_pmu_ll_set_r42_pwd_blppwd(1);
	// sys_ll_set_cpu0_int_32_63_en_cpu0_mac_wakeup_int_en(0);
	// sys_ll_set_cpu1_int_32_63_en_cpu1_mac_wakeup_int_en(0);
	aon_pmu_ll_set_r42_pwd_wlppwd(1);

	sys_hal_clear_wakeup_status();
	sys_hal_set_sleep_condition();

	if (param && *(uint8_t *)param) {
		sys_hal_set_halt_config(PM_MODE_SUPER_DEEP_SLEEP);
		sys_hal_set_power_parameter(PM_MODE_SUPER_DEEP_SLEEP);
	} else {
		 sys_hal_set_halt_config(PM_MODE_DEEP_SLEEP);
		 sys_hal_set_power_parameter(PM_MODE_DEEP_SLEEP);
		 aon_pmu_ll_set_r2_otp_vdd_en(0);// close OTPLDO
	     #if CONFIG_GPIO_WAKEUP_SUPPORT
		extern bk_err_t gpio_enable_interrupt_mult_for_wake(void);
		gpio_enable_interrupt_mult_for_wake();
        #endif
		aon_pmu_ll_set_r41_gpio_func_ctrl_en(0);//disable second function for all gpios
	}


#if 0
	sys_hal_enable_spi_latch();
#if !CONFIG_SPE
	/* ensfsdd enabling causes OTP read failed! */
	sys_ll_set_ana_reg10_ensfsdd(0);
#endif
	sys_hal_deep_sleep_set_buck();
	sys_hal_deep_sleep_set_vldo();
	sys_hal_disable_spi_latch();
#endif
	sys_hal_set_low_voltage(PM_MODE_DEEP_SLEEP, &v_ana_r9, &core_low_voltage);

	sys_hal_enable_spi_latch();
	sys_hal_disable_hf_clock();

	sys_hal_enable_spi_latch();
	/*disable psram*/
	if(sys_ll_get_ana_reg14_enpsram() != 0x0)
	{
		sys_ll_set_ana_reg14_enpsram(0x0);
	}

	/*power optimization*/
	if(sys_ll_get_ana_reg12_enpowa() != 0x0)
	{
		sys_ll_set_ana_reg12_enpowa(0x0);
	}

	/*buffer low power*/
	//TODO: if(sys_ll_get_ana_reg10_vbspbuflp1v() != 0x1)
	{
		//TODO: sys_ll_set_ana_reg10_vbspbuflp1v(0x1);
	}

	if(sys_ll_get_ana_reg13_denburst() != 0x0)
	{
		sys_ll_set_ana_reg13_denburst(0x0);//buckD burst disable
	}

	if(sys_ll_get_ana_reg12_aenburst() != 0x0)
	{
		sys_ll_set_ana_reg12_aenburst(0x0);//buckA burst disable
	}

	sys_ll_set_ana_reg5_en_cb(0);

	if((lpo_src == PM_LPO_SRC_ROSC)||(lpo_src == PM_LPO_SRC_DIVD))
	{
		sys_hal_power_on_and_select_rosc(lpo_src);
	}
	sys_ll_set_ana_reg9_valoldosel(PM_LOW_VOL_AON_LDO_SEL); //0x2:0.7V aon voltage
	sys_hal_disable_spi_latch();

	if (param && *(uint8_t *)param) {
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
		aon_pmu_hal_set_gpio_sleep(1, false);
		aon_pmu_hal_r0_latch_to_r7b();
#else
		sys_hal_gpio_state_switch(true);
#endif
		aon_pmu_ll_set_r3_shutdown_flag(1);
		sys_ll_set_ana_reg11_sd(1);//shutdown directly
	} else {
#if CONFIG_AON_PMU_REG0_REFACTOR_DEV
		aon_pmu_hal_r0_latch_to_r7b();
#endif
	/*-----enter deep sleep-------*/
		arch_deep_sleep();
	}

}
static inline void sys_hal_restore_voltage(volatile uint32_t ana_r8, volatile uint32_t core_low_voltage)
{
	sys_hal_enable_spi_latch();
	sys_ll_set_ana_reg8_value(ana_r8);
	sys_hal_disable_spi_latch();
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

/*__attribute__((section(".iram")))*/ void sys_hal_set_exit_low_voltage_tick(uint64_t tick)
{
	low_voltage_exit_tick= tick;
}

inline uint64_t sys_hal_get_low_voltage_sleep_duration_us(void)
{
	return low_voltage_sleep_duration_us;
}

inline void sys_hal_set_low_voltage_sleep_duration_us(uint64_t sleep_duration)
{
	low_voltage_sleep_duration_us= sleep_duration;
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
	if (sys_ll_get_ana_reg12_aldosel() == 0) {
		sys_ll_set_ana_reg12_aforcepfm(1);//tenglong20230417 modify buck mode(increase buck effect)
		sys_ll_set_ana_reg13_dforcepfm(1);//tenglong20230417
	}
}

static inline void sys_hal_lv_restore_buck(void)
{
	if (sys_ll_get_ana_reg12_aldosel() == 0) {
		sys_ll_set_ana_reg12_aforcepfm(0);//tenglong20230417
		sys_ll_set_ana_reg13_dforcepfm(0);//tenglong20230417
	}
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

#if CONFIG_DEEP_LV
static uint32_t s_sys_saved_regs[18] = {0};
static uint32_t s_sys_ana_regs[32] = {0};
static uint32_t s_mailbox_saved_regs[0x59] = {0};
__attribute__((section(".iram")))  void sys_hal_mailbox_regs_backup(void);
__attribute__((section(".iram")))  void sys_hal_mailbox_regs_restore(void);
//static uint32_t s_saved_sram[4] = {0};

__attribute__((section(".iram"))) static void _deep_lv_enter_(void)
{
	__asm volatile
	(
		" .syntax unified           \n"
		" cpsie i                   \n" /* Globally enable interrupts. */
		" cpsie f                   \n"
		" dsb                       \n"
		" isb                       \n"
		" svc %0                    \n"
		" nop                       \n"
		"                           \n"
		::"i"(portSVC_DEEP_LV_ENTER):"memory"
	);
}

__attribute__((section(".iram"))) void sys_hal_deep_lv_enter(void)
{
	_deep_lv_enter_();
	__asm volatile
	(
		" .syntax unified           \n"
		" nop                       \n"
	);
}

__attribute__((section(".iram"))) void sys_hal_regs_save(void)
{
	s_sys_saved_regs[0] = sys_ll_get_cpu_clk_div_mode1_value(); // reg_0x8
	s_sys_saved_regs[1] = sys_ll_get_cpu_clk_div_mode2_value(); // reg_0x9
	s_sys_saved_regs[2] = sys_ll_get_cpu_clk_div_mode3_value(); // reg_0xa
	s_sys_saved_regs[3] = sys_ll_get_cpu_anaspi_freq_value(); // reg_0xb
	s_sys_saved_regs[4] = sys_ll_get_cpu_device_clk_enable_value(); // reg_0xc
	s_sys_saved_regs[5] = sys_ll_get_reserver_reg0xd_value(); //reg 0xd
	s_sys_saved_regs[6] = sys_ll_get_reserver_reg0xf_value(); // reg_0xf
	s_sys_saved_regs[7] = sys_ll_get_reserver_reg0x10_value(); // reg_0x10
	s_sys_saved_regs[8] = sys_ll_get_cpu_power_sleep_wakeup_value(); // reg_0x11
	s_sys_saved_regs[9] = sys_ll_get_cpu0_int_0_31_en_value(); // reg_0x14
	s_sys_saved_regs[10] = sys_ll_get_cpu0_int_32_63_en_value(); // reg_0x15
	s_sys_saved_regs[11] = sys_ll_get_cpu0_int_64_95_en_value(); // reg_0x16
	s_sys_saved_regs[12] = sys_ll_get_cpu1_int_0_31_en_value(); // reg_0x17
	s_sys_saved_regs[13] = sys_ll_get_cpu1_int_32_63_en_value(); // reg_0x18
	s_sys_saved_regs[14] = sys_ll_get_cpu1_int_64_95_en_value(); // reg_0x19
	s_sys_saved_regs[15] = sys_ll_get_m55sub_int_0_31_en_value(); // reg_0x1a
	s_sys_saved_regs[16] = sys_ll_get_m55sub_int_32_63_en_value(); // reg_0x1b
	s_sys_saved_regs[17] = sys_ll_get_m55sub_int_64_95_en_value(); // reg_0x1c

	for (uint32_t i = 0; i < 32; i++) {
		s_sys_ana_regs[i] = sys_hal_analog_get(ANALOG_REG0 + i);
	}

	//sys_hal_mailbox_regs_backup();

	// for (uint32_t i = 0; i < 4; i++) {
	// 	s_saved_sram[i] = REG_READ(0x2801FFF0 + (i << 2));
	// }
}

__attribute__((section(".iram")))  void sys_hal_mailbox_regs_backup(void)
{
	static const uint8_t s_mailbox_backup_start[] = {0x10, 0x20, 0x30, 0x40, 0x50};

	s_mailbox_saved_regs[0x2] = REG_READ(SOC_MBOX0_REG_BASE + (0x2 << 2));

	for (uint32_t i = 0; i < sizeof(s_mailbox_backup_start) / sizeof(s_mailbox_backup_start[0]); i++) {
		uint32_t start = s_mailbox_backup_start[i];
		uint32_t end = start + 0x8;

		for (uint32_t reg_idx = start; reg_idx <= end; reg_idx++) {
			s_mailbox_saved_regs[reg_idx] = REG_READ(SOC_MBOX0_REG_BASE + (reg_idx << 2));
		}
	}
}

void sys_hal_mailbox_saved_regs_dump(void)
{
	return;
	static const uint8_t s_mailbox_dump_start[] = {0x10, 0x20, 0x30, 0x40, 0x50};

	PM_HAL_LOGD("mailbox backup regs dump start\r\n");
	PM_HAL_LOGD("mbox reg[0x02]=0x%08x\r\n", s_mailbox_saved_regs[0x2]);
	for (uint32_t i = 0; i < sizeof(s_mailbox_dump_start) / sizeof(s_mailbox_dump_start[0]); i++) {
		uint32_t start = s_mailbox_dump_start[i];
		uint32_t end = start + 0x8;

		for (uint32_t reg_idx = start; reg_idx <= end; reg_idx++) {
			PM_HAL_LOGD("mbox reg[0x%02x]=0x%08x\r\n", reg_idx, s_mailbox_saved_regs[reg_idx]);
		}
	}
	PM_HAL_LOGD("mailbox backup regs dump end\r\n");
}

__attribute__((section(".iram"))) void sys_hal_mailbox_regs_restore(void)
{
	static const uint8_t s_mailbox_restore_start[] = {0x10, 0x20, 0x30, 0x40, 0x50};
	uint32_t mailbox_reg_0x2 = REG_READ(SOC_MBOX0_REG_BASE + (0x2 << 2));
	uint32_t mailbox_saved_reg_0x2 = s_mailbox_saved_regs[0x2];

	/* clear bit0 of reg_0x2 before restoring mailbox registers */
	mailbox_reg_0x2 &= ~BIT(0);
	REG_WRITE(SOC_MBOX0_REG_BASE + (0x2 << 2), mailbox_reg_0x2);

	mailbox_reg_0x2 |= BIT(0);
	REG_WRITE(SOC_MBOX0_REG_BASE + (0x2 << 2), mailbox_reg_0x2);

	for (uint32_t i = 0; i < sizeof(s_mailbox_restore_start) / sizeof(s_mailbox_restore_start[0]); i++) {
		uint32_t start = s_mailbox_restore_start[i];
		uint32_t end = start + 0x4; /* only restore writable regs, skip RO status/data regs */

		for (uint32_t reg_idx = start; reg_idx <= end; reg_idx++) {
			uint32_t reg_val = s_mailbox_saved_regs[reg_idx];

			/* wrerr/rderr/wrfull bits are W1C in reg_x0 */
			if ((reg_idx == 0x10) || (reg_idx == 0x20) || (reg_idx == 0x30) || (reg_idx == 0x40) || (reg_idx == 0x50)) {
				reg_val &= ~(0x7 << 16);
			}

			REG_WRITE(SOC_MBOX0_REG_BASE + (reg_idx << 2), reg_val);
		}
	}

	/* restore reg_0x2 from backup value */
	REG_WRITE(SOC_MBOX0_REG_BASE + (0x2 << 2), mailbox_saved_reg_0x2);
}

__attribute__((section(".iram"))) void sys_hal_regs_digital_restore(void)
{
	sys_ll_set_reserver_reg0xd_value(s_sys_saved_regs[5]); //reg 0xd

	s_sys_saved_regs[0] |= (0x3 << 0);
	s_sys_saved_regs[0] |= (0x3 << 2);
	//keep flash 120M
	s_sys_saved_regs[0] |= (0x3 << 6);
	s_sys_saved_regs[0] |= (0x3 << 8);
	sys_ll_set_cpu_clk_div_mode1_value(s_sys_saved_regs[0]); // reg_0x8

	sys_ll_set_cpu_clk_div_mode2_value(s_sys_saved_regs[1]); // reg_0x9
	sys_ll_set_cpu_clk_div_mode3_value(s_sys_saved_regs[2]); // reg_0xa
	sys_ll_set_cpu_anaspi_freq_value(s_sys_saved_regs[3]); // reg_0xb
	sys_ll_set_cpu_device_clk_enable_value(s_sys_saved_regs[4]); // reg_0xc

	sys_ll_set_reserver_reg0xf_value(s_sys_saved_regs[6]); // reg_0xf
	//sys_ll_set_reserver_reg0x10_value(s_sys_saved_regs[7]); // reg_0x10
	sys_ll_set_cpu_power_sleep_wakeup_value(s_sys_saved_regs[8]); // reg_0x11
	sys_ll_set_cpu0_int_0_31_en_value(s_sys_saved_regs[9]); // reg_0x14
	sys_ll_set_cpu0_int_32_63_en_value(s_sys_saved_regs[10]); // reg_0x15
	sys_ll_set_cpu0_int_64_95_en_value(s_sys_saved_regs[11]); // reg_0x16
	// sys_ll_set_cpu1_int_0_31_en_value(s_sys_saved_regs[12]); // reg_0x17
	// sys_ll_set_cpu1_int_32_63_en_value(s_sys_saved_regs[13]); // reg_0x18
	// sys_ll_set_cpu1_int_64_95_en_value(s_sys_saved_regs[14]); // reg_0x19
	// sys_ll_set_m55sub_int_0_31_en_value(s_sys_saved_regs[15]); // reg_0x1a
	// sys_ll_set_m55sub_int_32_63_en_value(s_sys_saved_regs[16]); // reg_0x1b
	// sys_ll_set_m55sub_int_64_95_en_value(s_sys_saved_regs[17]); // reg_0x1c

	sys_hal_mailbox_regs_restore();
}
__attribute__((section(".iram"))) void sys_hal_regs_analog_restore(void)
{
	sys_hal_enable_spi_latch();
	/* restore analog regs */
	for (uint32_t i = 0; i < 32; i++) {
		if (( i == 0)||( i == 3)||( i == 5)||( i == 7)||( i == 8)||( i == 10)||( i == 11)||( i == 12)||( i == 13)||( i == 14))
			continue;
		sys_hal_analog_set(ANALOG_REG0 + i, s_sys_ana_regs[i]);
	}
	sys_hal_disable_spi_latch();

}
#endif

__attribute__((section(".iram"))) void sys_hal_enter_low_voltage(void)
{
	volatile uint32_t int_state1, int_state2, int_state3;
	volatile uint8_t cksel_core = 0, clkdiv_core = 0, clkdiv_bus = 0;
	volatile uint8_t cksel_flash = 0, clkdiv_flash = 0;
	volatile uint32_t v_ana_r9, core_low_voltage;
	volatile uint32_t v_sys_r10    = 0;
	uint32_t systick_ctrl_value    = 0;
	//uint32_t valoldosel            = 0;
	// uint32_t violdosel          = 0;
	//uint8_t  ustep                 = 0;
///	uint32_t chip_id               = 0;
	pm_lpo_src_e lpo_src           = PM_LPO_SRC_ROSC;

#if CONFIG_OTA_POSITION_INDEPENDENT_AB || CONFIG_DIRECT_XIP
	flash_ab_reg_t  ab_flash_reg   = {0};
#endif
	portNVIC_INT_CTRL_REG |= portNVIC_SYSTICKCLR_BIT;
	systick_ctrl_value = portNVIC_SYSTICK_CTRL_REG;
	portNVIC_SYSTICK_LOAD_REG = PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME;
	portNVIC_SYSTICK_CTRL_REG = 0;//disable the systick, avoid it affect the enter low voltage sleep

	int_state1 = sys_ll_get_cpu0_int_0_31_en_value();
	int_state2 = sys_ll_get_cpu0_int_32_63_en_value();
	int_state3 = sys_ll_get_cpu0_int_64_95_en_value();
	sys_ll_set_cpu0_int_0_31_en_value(0x0);
	sys_ll_set_cpu0_int_32_63_en_value(0x0);
	sys_ll_set_cpu0_int_64_95_en_value(0x0);

	if(check_IRQ_pending()||(sys_ll_get_cpu0_int_0_31_status_value()||(sys_ll_get_cpu0_int_32_63_status_value()) || (sys_ll_get_cpu0_int_64_95_status_value()))||(portNVIC_INT_CTRL_REG&portNVIC_SYSTICKSET_BIT))
	{
		sys_ll_set_cpu0_int_0_31_en_value(int_state1);
		sys_ll_set_cpu0_int_32_63_en_value(int_state2);
		sys_ll_set_cpu0_int_64_95_en_value(int_state3);
		portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
		return;
	}

	portNVIC_SYSTICK_LOAD_REG = PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME;

	sys_hal_mask_cpu0_int();
#if CONFIG_GPIO_WAKEUP_SUPPORT
	extern bk_err_t gpio_enable_interrupt_mult_for_wake(void);
	gpio_enable_interrupt_mult_for_wake();
#endif

	bk_pm_module_lv_sleep_state_set();

	//sys_hal_backup_disable_int(&int_state1, &int_state2);
	sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);
	sys_hal_backup_set_flash_26m(&cksel_flash, &clkdiv_flash);

#if CONFIG_INT_WDT
#if CONFIG_SUPPORT_WWDT
    bk_wwdt_stop();
#endif

#if INT_AON_WDT
	bk_wdt_suspend();
#endif

#if CONFIG_TASK_WDT
	bk_task_wdt_stop();
#endif
#endif

#if CONFIG_OTA_POSITION_INDEPENDENT_AB || CONFIG_DIRECT_XIP
	flash_ab_info_backup(&ab_flash_reg);
#endif

#if !CONFIG_DEEP_LV
#if CONFIG_SPE
	sys_ll_set_cpu0_int_32_63_en_cpu0_gpio_s_int_en(0x1);
#else
	//enable gpio ns interrupt
	sys_hal_enable_hsu_int();
#endif
	sys_ll_set_cpu0_int_32_63_en_cpu0_rtc_int_en(0x1);
	sys_ll_set_cpu0_int_32_63_en_cpu0_touched_int_en(0x1);
#endif

	lpo_src = aon_pmu_ll_get_r41_lpo_config();
	sys_hal_set_power_parameter(PM_MODE_LOW_VOLTAGE);
	sys_hal_set_sleep_condition();
	uint32_t pwd_cpu1 = sys_ll_get_reserver_reg0x10_pwd_cpu1();
	uint32_t pwd_vehp = sys_ll_get_reserver_reg0x10_pwd_vehp();
	uint32_t pwd_wrls = sys_ll_get_reserver_reg0x10_pwd_wrls();
	uint32_t rom_pgen = sys_ll_get_reserver_reg0x10_rom_pgen();

	sys_hal_power_down_pd(&v_sys_r10);

	#if CONFIG_DEEP_LV
	uint32_t v_ana_r0 = sys_ll_get_ana_reg0_value();
	uint32_t v_ana_r3 = sys_ll_get_ana_reg3_value();
	uint32_t v_ana_r7 = sys_ll_get_ana_reg7_value();
	uint32_t v_ana_r8 = sys_ll_get_ana_reg8_value();
	#endif
	uint32_t v_ana_r10 = sys_ll_get_ana_reg10_value();
	uint32_t v_ana_r11 = sys_ll_get_ana_reg11_value();
	uint32_t v_ana_r12 = sys_ll_get_ana_reg12_value();
	uint32_t v_ana_r13 = sys_ll_get_ana_reg13_value();
	uint32_t v_ana_r14 = sys_ll_get_ana_reg14_value();
	#if CONFIG_DEEP_LV
	sys_hal_set_low_voltage(PM_MODE_DEEP_SLEEP, &v_ana_r9, &core_low_voltage);
	#else
	sys_hal_set_low_voltage(PM_MODE_LOW_VOLTAGE, &v_ana_r9, &core_low_voltage);
	#endif

	uint32_t hf_reg_v = sys_hal_disable_hf_clock();

	sys_hal_enable_spi_latch();

	#if CONFIG_PSRAM_POWER_DOMAIN_LV_DISABLE
	uint32_t psram_state = sys_ll_get_ana_reg14_enpsram();
	if(psram_state != 0x0)
	{
		sys_ll_set_ana_reg14_enpsram(0x0);//the psram power domain need the app enable again
	}
	#endif

	#if CONFIG_DEEP_LV
	/*power optimization*/
	if(sys_ll_get_ana_reg12_enpowa() != 0x0)
	{
		sys_ll_set_ana_reg12_enpowa(0x0);
	}
	if(sys_ll_get_ana_reg12_ampoen() != 0x0)
	{
		sys_ll_set_ana_reg12_ampoen(0x0);
	}

	if(sys_ll_get_ana_reg13_denburst() != 0x0)
	{
		sys_ll_set_ana_reg13_denburst(0x0);//buckD burst disable
	}

	if(sys_ll_get_ana_reg12_aenburst() != 0x0)
	{
		sys_ll_set_ana_reg12_aenburst(0x0);//buckA burst disable
	}
	#else
	/*low voltage power optimization*/
	if(sys_ll_get_ana_reg12_enpowa() != 0x1)
	{
		sys_ll_set_ana_reg12_enpowa(0x1);
	}

	if(sys_ll_get_ana_reg13_denburst() != 0x1)
	{
		sys_ll_set_ana_reg13_denburst(0x1);//buckD burst enable
	}

	if(sys_ll_get_ana_reg12_aenburst() != 0x1)
	{
		sys_ll_set_ana_reg12_aenburst(0x1);//buckA burst enable
	}
	sys_ll_set_ana_reg12_enpowa(0x0);
	#endif

	sys_ll_set_ana_reg5_en_cb(0);

	if(lpo_src == PM_LPO_SRC_ROSC)
	{
		sys_hal_power_on_and_select_rosc(lpo_src);
	}

	#if CONFIG_SPE
	bool otp_vdd = aon_pmu_ll_get_r2_otp_vdd_en();
	aon_pmu_ll_set_r2_otp_vdd_en(0);// close OTPLDO, 1.5uA decrease
	#endif

	/*vio voltage*/
	// violdosel = sys_ana_ll_get_reg8_violdosel();
	// sys_ana_ll_set_reg8_violdosel(PM_LOW_VOL_VIO_LDO_SEL); //0x0:2.9V vio voltage
	/*aon voltage*/
	// valoldosel = sys_ll_get_ana_reg9_valoldosel();
	// sys_ll_set_ana_reg9_valoldosel(PM_LOW_VOL_AON_LDO_SEL); //0x4:0.8V aon voltage
	sys_hal_disable_spi_latch();

	uint64_t before = bk_aon_rtc_get_us();
/*----enter low voltage sleep-------*/
#if CONFIG_DEEP_LV
	sys_hal_regs_save();
	aon_pmu_hal_backup();
	sys_hal_deep_lv_enter();
	__NOP();
	if (aon_pmu_hal_get_dlv_startup_iram())
	{
		aon_pmu_hal_r0_latch_to_r7b();
		arch_deep_sleep();
	}
#else
	#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
	bk_flash_enter_deep_sleep();
	#endif
	arch_deep_sleep();
#endif
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//4
	GPIO_DOWN(27);
	#endif

#if CONFIG_OTA_POSITION_INDEPENDENT_AB || CONFIG_DIRECT_XIP
	flash_ab_info_restore(&ab_flash_reg);
#endif

/*--------------------wake up---------------------*/
	#if CONFIG_DEEP_LV
	extern void mpu_enable(void);
	mpu_enable();
	extern void timer_hal_us_init(uint32_t us);
	timer_hal_us_init(0);
	#endif

	uint64_t current = bk_aon_rtc_get_us();
	sys_hal_set_low_voltage_wakeup_time_us(current);
	sys_hal_set_low_voltage_sleep_duration_us(current - before);
	#if CONFIG_LV_FLASH_ENTER_LP_ENABLE
	bk_flash_exit_deep_sleep();
	#endif

	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//5
	GPIO_DOWN(27);
	#endif
/*-----------restore voltage  start----------------*/
	sys_hal_enable_spi_latch();
	/*aon voltage*/
	// for(ustep = PM_LOW_VOL_AON_LDO_SEL+1; ustep <= valoldosel; ustep++)
	// {
	// 	sys_ll_set_ana_reg9_valoldosel(ustep); //restore to 0.9V aon voltage
	// }

	#if CONFIG_SPE
	aon_pmu_ll_set_r2(otp_vdd);// restore OTPLDO
	#endif

	#if CONFIG_DEEP_LV
	sys_ll_set_ana_reg0_value(v_ana_r0);
	sys_ll_set_ana_reg3_value(v_ana_r3);
	sys_ll_set_ana_reg7_value(v_ana_r7);
	sys_ll_set_ana_reg8_value(v_ana_r8);
	#endif
	sys_ll_set_ana_reg10_value(v_ana_r10);
	sys_ll_set_ana_reg11_value(v_ana_r11);
	sys_ll_set_ana_reg12_value(v_ana_r12);
	sys_ll_set_ana_reg13_value(v_ana_r13);
	sys_ll_set_ana_reg14_value(v_ana_r14);
	sys_ll_set_ana_reg9_value(v_ana_r9);
	sys_hal_disable_spi_latch();
/*-------------restore voltage  end-----------------*/
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//6
	GPIO_DOWN(27);
	#endif
/*----------restore analog clock  start--------------*/
	sys_ll_set_ana_reg5_en_cb(1);
	bk_delay_us(10);
	#if CONFIG_DEEP_LV
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//7
	GPIO_DOWN(27);
	#endif
	//sys_hal_regs_digital_restore();
	aon_pmu_hal_restore();
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//8
	GPIO_DOWN(27);
	#endif

#endif
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//9
	GPIO_DOWN(27);
	#endif
	sys_hal_restore_hf_clock(hf_reg_v);

	volatile uint64_t previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	volatile uint64_t current_tick  = 0;
	current_tick = previous_tick;
	while(((current_tick - previous_tick)) < (LOW_POWER_DPLL_STABILITY_DELAY_TIME*AON_RTC_MS_TICK_CNT))
	{
		current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	}
	/*---------------wifi debug end -----------------*/
	#if CONFIG_DEEP_LV_DEBUG
		GPIO_UP(27);//10
		GPIO_DOWN(27);
	#endif
/*-----------restore analog clock  end --------------*/

/*-----------wifi debug  start time --------------*/
#if CONFIG_WIFI_ENABLE
	extern uint32_t pm_wake_int_flag2;
	pm_wake_int_flag2 = sys_hal_get_int_group2_status(0);

	if(pm_wake_int_flag2&(WIFI_MAC_GEN_INT_BIT))
	{
		extern void rwnxl_set_wifi_low_vol_flag();
		rwnxl_set_wifi_low_vol_flag();
	}
#endif
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//11
	GPIO_DOWN(27);
	#endif

	/*Use a function instead of delay*/
	void pm_low_voltage_bsp_restore(void);
	pm_low_voltage_bsp_restore();

	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//12
	GPIO_DOWN(27);
	#endif

	sys_hal_set_exit_low_voltage_tick(previous_tick);
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//13
	GPIO_DOWN(27);
	#endif
/*---------------at least delay 190us-----------------*/
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//14
	GPIO_DOWN(27);
	#endif
	/*restore power domain*/
	if(pwd_cpu1 != sys_ll_get_reserver_reg0x10_pwd_cpu1())
	{
		sys_ll_set_reserver_reg0x10_pwd_cpu1(pwd_cpu1);
	}
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//15
	GPIO_DOWN(27);
	#endif

	if(pwd_vehp != sys_ll_get_reserver_reg0x10_pwd_vehp())
	{
		sys_ll_set_reserver_reg0x10_pwd_vehp(pwd_vehp);
	}
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//16
	GPIO_DOWN(27);
	#endif
	if(pwd_wrls != sys_ll_get_reserver_reg0x10_pwd_wrls())
	{
		sys_ll_set_reserver_reg0x10_pwd_wrls(pwd_wrls);
	}
	if(rom_pgen != sys_ll_get_reserver_reg0x10_rom_pgen())
	{
		sys_ll_set_reserver_reg0x10_rom_pgen(rom_pgen);
	}

/*---------------at least delay 190us end -----------------*/
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//17
	GPIO_DOWN(27);
	#endif
	#if CONFIG_DEEP_LV
	sys_hal_regs_digital_restore();
	#endif
	sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	sys_hal_restore_flash_freq(cksel_flash, clkdiv_flash);

	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//18
	GPIO_DOWN(27);
	#endif

	sys_hal_restore_int(int_state1, int_state2, int_state3);
	portNVIC_SYSTICK_LOAD_REG = PM_EXIT_LOWVOL_SYSTICK_RELOAD_TIME;
	portNVIC_SYSTICK_CTRL_REG = systick_ctrl_value;
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(27);//19
	GPIO_DOWN(27);
	#endif
}

void sys_hal_touch_wakeup_enable(uint8_t index)
{
	/* Keep touch analog circuit powered during deep sleep so it can
	 * autonomously detect a touch and trigger the AON PMU wakeup interrupt.
	 * Channel selection is configured by bk_touch_enable() before sleep.
	 * BK7259 does not have per-channel AON touch select like BK7258;
	 * all enabled channels are monitored via AON PMU r70.int_touched. */
	sys_hal_touch_power_down(0);
	aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_TOUCHED);
}

void sys_hal_usb_wakeup_enable(uint8_t index)
{
	// aon_pmu_ll_set_r1_usbplug_int_en(1);
	// sys_ll_set_cpu0_int_32_63_en_usbplug_int(1);
	// aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_USBPLUG);
}

void sys_hal_rtc_wakeup_enable(uint32_t value)
{
	// aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_RTC);
	// sys_ll_set_cpu0_int_32_63_en_rtc_int(1);
}

void sys_hal_rtc_ana_wakeup_enable(uint32_t period)
{
	sys_hal_enable_spi_latch();
	sys_ll_set_ana_reg11_spi_timerwken(1);

	// period =0: 32ms, ... =5: 1s, =6: 2s ... =15: 1024s
	//TODO: sys_ll_set_ana_reg10_timer_sel(period);
#if CONFIG_EXTERN_32K
	// set xtll as rtc clk
	//TODO: sys_ll_set_ana_reg12_clk_sel(1);
#else
	// set rosc as rtc clk
	//TODO: sys_ll_set_ana_reg12_clk_sel(0);
#endif
	sys_hal_disable_spi_latch();
}

void sys_hal_gpio_ana_wakeup_enable(uint32_t count, uint32_t index, uint32_t type)
{
}
void sys_hal_enter_cpu_wfi()
{
	// TODO - find out the reason:
	// When sys_ll_set_cpu0_int_halt_clk_op_cpu0_int_mask() is called, the normal sleep can't wakedup,
	// need to find out!!!
	// sys_ll_set_cpu0_int_halt_clk_op_cpu0_halt(1);
	arch_sleep();
}
void sys_hal_enter_normal_sleep(uint32_t peri_clk)
{
	// TODO - find out the reason:
	// When sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask() is called, the normal sleep can't wakedup,
	// need to find out!!!
	// sys_ll_set_cpu0_int_halt_clk_op_cpu0_halt(1);
	arch_sleep();
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
	PM_HAL_LOGD("set lpo src: %u\r\n", src);
	//TODO
	return BK_OK;
}

void sys_hal_enter_low_analog(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_t_vanaldosel(0);
	sys_ll_set_ana_reg9_r_vanaldosel(0);
	sys_ll_set_ana_reg9_alopowsel(1);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	//sys_ll_set_ana_reg3_hpssren(0);
	//sys_ll_set_ana_reg3_anabuf_sel_rx(1);
	//sys_ll_set_ana_reg3_anabuf_sel_tx(1);
}

void sys_hal_exit_low_analog(void)
{
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_t_vanaldosel(4);
	sys_ll_set_ana_reg9_r_vanaldosel(4);
	sys_ll_set_ana_reg9_alopowsel(0);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	//sys_ll_set_ana_reg3_hpssren(1);
	//sys_ll_set_ana_reg3_anabuf_sel_rx(0);
	//sys_ll_set_ana_reg3_anabuf_sel_tx(0);
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
	//TODO:sys_ll_set_ana_reg8_ioldo_lp(!!val);
}

static int sys_hal_config_32k_source_default()
{
	pm_lpo_src_e lpo_src = PM_LPO_SRC_ROSC;
	lpo_src = bk_clk_32k_customer_config_get();
	if(lpo_src == PM_LPO_SRC_X32K)
	{
		sys_ll_set_ana_reg5_en_xtall(0x1);
		if(sys_ll_get_ana_reg5_itune_xtall() != 0xF)
		{
			sys_ll_set_ana_reg5_itune_xtall(0xF);
		}

		sys_ll_set_ana_reg5_itune_xtall(0xA);//0x0 provide highest current for external 32k,because the signal path long
		sys_ll_set_ana_reg5_itune_xtall(0x4);

		aon_pmu_hal_lpo_src_set(PM_LPO_SRC_X32K);

		sys_ll_set_ana_reg11_ckintsel(1);//select buck clock source(0x1: extern 32k)
	}
	else if(lpo_src == PM_LPO_SRC_DIVD)
	{
		aon_pmu_hal_lpo_src_set(PM_LPO_SRC_DIVD);
	}
	else
	{
		aon_pmu_hal_lpo_src_set(PM_LPO_SRC_ROSC);
	}
	return 0;
}

static int sys_hal_enable_buck()
{
	volatile uint8_t cksel_core = 0, clkdiv_core = 0, clkdiv_bus = 0;
	sys_hal_backup_set_core_26m(&cksel_core, &clkdiv_core, &clkdiv_bus);//let the cpu frequency to 26m, in order to be successfully switch voltage provide from ldo to buck

	sys_hal_enable_spi_latch();
	sys_ll_set_ana_reg12_aldosel(0);
	sys_ll_set_ana_reg13_dldosel(0);
	bk_delay_us(1);

	/*let the ioldo low power mode*/
	//TODO: sys_ll_set_ana_reg8_ioldo_lp(1);
	sys_hal_disable_spi_latch();

	sys_hal_restore_core_freq(cksel_core, clkdiv_core, clkdiv_bus);
	return 0;
}

static int sys_hal_power_config_default()
{
	/*config the analog power*/
	/*1.disable audio test mode save 2ma*/
	sys_ll_set_ana_reg25_test_ckaudio_en(0x0);
	sys_ll_set_ana_reg25_audioen(0x0);

	sys_ll_set_ana_reg6_manu_cin(0x0);

	#if CONFIG_PM_ONLY_CP_ENABLE
	//sys_ll_set_reserver_reg0x10_pwd_cpu1(0x1);
	sys_ll_set_reserver_reg0x10_pwd_vehp(0x1);
	//sys_ll_set_reserver_reg0x10_pwd_wrls(0x1);
	sys_ll_set_reserver_reg0x10_rom_pgen(0x1);
	#endif

	/*2.psram sel mode to save power consumption*/
	// sys_ll_set_ana_reg14_vpsramsel(0x1);
	// sys_ll_set_ana_reg14_pwdovp1v(0x1);

#if CONFIG_VBSPBUFLP1V_ENABLE
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) != (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK)){
		//TODO: sys_ana_ll_set_reg10_vbspbuflp1v(0x1);
	}
#endif

	/*decrease the buck ripple wave,which good for wifi evm from hardware and analog reply */
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg13_dswrsten(0x0);
	sys_ll_set_ana_reg12_aswrsten(0x0);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	return 0;
}
void sys_hal_low_power_hardware_init()
{
	pm_shared_info_t shared_info = {0};

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
	aon_pmu_ll_set_r0_fast_boot(0);

	/*set wakeup source*/
	aon_pmu_ll_set_r41_wakeup_ena(0x23);//enable wakeup source: int_touched,int_rtc,int_gpio,wifi wake(bt or wifi wakeup source enable when bt or wifi sleep)

	/*enable the buck*/
	#if CONFIG_BUCK_ENABLE
	sys_hal_enable_buck();
	#endif
	/*select lowpower lpo clk source*/
	sys_hal_config_32k_source_default();

	/*default to config the power */
	sys_hal_power_config_default();

	/*set the lp voltage*/
	sys_hal_lp_vol_set(CONFIG_LP_VOL);

	/*set rosc calib trig once*/
	sys_hal_rosc_calibration(3, 0);

	/* Early boot path: initialize shared PM info without lock dependency. */
	bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_ALL, BK_SYS_SW_REGS_LOCK_DISABLE);
}

#if CONFIG_PM_V3
static bool s_pm_is_phy_reinit_flag = false;
static uint32_t s_pm_phy_calibration_state    = 0;
static int sys_hal_pd_cb(const device_t *device, pm_device_action_t action)
{
	uint32_t v;

	switch (action) {
	case PM_DEVICE_ACTION_POWER_ON:
		//os_printf("%s power on\r\n", device_get_name(device));
		v = sys_ll_get_cpu_power_sleep_wakeup_value();
		v &= ~((uint32_t)(device->pm->data));
		sys_ll_set_cpu_power_sleep_wakeup_value(v);
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		//os_printf("%s power off\r\n", device_get_name(device));
		v = sys_ll_get_cpu_power_sleep_wakeup_value();
		v |= (uint32_t)(device->pm->data);
		sys_ll_set_cpu_power_sleep_wakeup_value(v);
		if((uint32_t)(device->pm->data) == PD_WRLS)
			s_pm_phy_calibration_state = 0x0;
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_peri1_cb(const device_t *device, pm_device_action_t action)
{
	uint32_t v;

	switch (action) {
	case PM_DEVICE_ACTION_POWER_ON:
		// os_printf("%s power on\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device_clk_enable_value();
		v |= (uint32_t)(device->pm->data);
		sys_ll_set_cpu_device_clk_enable_value(v);
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		// os_printf("%s power off\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device_clk_enable_value();
		v &= ~((uint32_t)(device->pm->data));
		sys_ll_set_cpu_device_clk_enable_value(v);
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_peri2_cb(const device_t *device, pm_device_action_t action)
{
	uint32_t v;

	switch (action) {
	case PM_DEVICE_ACTION_POWER_ON:
		// os_printf("%s power on\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v |= (uint32_t)(device->pm->data);
		sys_ll_set_cpu_device2_clk_enable_value(v);
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		// os_printf("%s power off\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v &= ~((uint32_t)(device->pm->data));
		sys_ll_set_cpu_device2_clk_enable_value(v);
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_wlss_cb(const device_t *device, pm_device_action_t action)
{
	uint32_t v;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		// os_printf("%s suspend\r\n", device_get_name(device));
		// aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_WIFI);
		break;
	case PM_DEVICE_ACTION_RESUME:
		// os_printf("%s resume\r\n", device_get_name(device));
		// aon_pmu_hal_clear_wakeup_source(WAKEUP_SOURCE_INT_WIFI);
		break;
	case PM_DEVICE_ACTION_POWER_ON:
		// os_printf("%s power on\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v |= PERI_WLSS;
		sys_ll_set_cpu_device2_clk_enable_value(v);
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		#if CONFIG_DEEP_LV  //TEMPORARILY WORKAROUND for tsf register write-back failure,must be removed after next hardware version
		// os_printf("%s power off\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v &= ~PERI_WLSS;
		sys_ll_set_cpu_device2_clk_enable_value(v);
		#endif
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}
static int sys_hal_mac_cb(const device_t *device, pm_device_action_t action)
{
	uint32_t v;

	switch (action) {
	case PM_DEVICE_ACTION_WAKEUP_ENABLE:
		// os_printf("%s wakesource enabled\r\n", device_get_name(device));
		aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_WIFI);
		break;
	case PM_DEVICE_ACTION_WAKEUP_DISABLE:
		// os_printf("%s wakesource disabled\r\n", device_get_name(device));
		aon_pmu_hal_clear_wakeup_source(WAKEUP_SOURCE_INT_WIFI);
		break;
	case PM_DEVICE_ACTION_POWER_ON:
		// os_printf("%s power on\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v |= PERI_MAC;
		sys_ll_set_cpu_device2_clk_enable_value(v);
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		#if CONFIG_DEEP_LV  //TEMPORARILY WORKAROUND for tsf register write-back failure,must be removed after next hardware version
		// os_printf("%s power off\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v &= ~PERI_MAC;
		sys_ll_set_cpu_device2_clk_enable_value(v);
		#endif
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_btdm_cb(const device_t *device, pm_device_action_t action)
{
	uint32_t v;

	switch (action) {
	case PM_DEVICE_ACTION_WAKEUP_ENABLE:
		// os_printf("%s wakesource enabled\r\n", device_get_name(device));
		aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_BT);
		break;
	case PM_DEVICE_ACTION_WAKEUP_DISABLE:
		// os_printf("%s wakesource disabled\r\n", device_get_name(device));
		aon_pmu_hal_clear_wakeup_source(WAKEUP_SOURCE_INT_BT);
		break;
	case PM_DEVICE_ACTION_POWER_ON:
		// os_printf("%s power on\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v |= PERI_BTDM;
		sys_ll_set_cpu_device2_clk_enable_value(v);
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		// os_printf("%s power off\r\n", device_get_name(device));
		v = sys_ll_get_cpu_device2_clk_enable_value();
		v &= ~PERI_BTDM;
		sys_ll_set_cpu_device2_clk_enable_value(v);
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_phy_bt_cb(const device_t *device, pm_device_action_t action)
{
	//uint32_t v;
#if CONFIG_WIFI_ENABLE
	extern void phy_wakeup_reinit(uint8 is_wifi);
#else
	extern void phy_wakeup_for_bluetooth();
#endif
	switch (action) {
	case PM_DEVICE_ACTION_POWER_ON:
		if(!s_pm_phy_calibration_state)
		{
#if CONFIG_WIFI_ENABLE
			phy_wakeup_reinit(0);
#else
			phy_wakeup_for_bluetooth();
#endif
			s_pm_is_phy_reinit_flag = true;
			s_pm_phy_calibration_state = 0x1;
		}
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_phy_wifi_cb(const device_t *device, pm_device_action_t action)
{
	//uint32_t v;
#if CONFIG_WIFI_ENABLE
	extern void phy_wakeup_reinit(uint8 is_wifi);
#else
	extern void phy_wakeup_for_bluetooth();
#endif
	switch (action) {
	case PM_DEVICE_ACTION_POWER_ON:
		if(!s_pm_phy_calibration_state)
		{
#if CONFIG_WIFI_ENABLE
			phy_wakeup_reinit(1);
#else
			phy_wakeup_for_bluetooth();
#endif
			s_pm_is_phy_reinit_flag = true;
			s_pm_phy_calibration_state = 0x1;
		}
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		break;
	default:
		// os_printf("unknown action %d\r\n", action);
		break;
	}

	return BK_OK;
}

static int sys_hal_test_cb(const device_t *device, pm_device_action_t action)
{
#if 0
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		os_printf("%s suspend\r\n", device_get_name(device));
		break;
	case PM_DEVICE_ACTION_RESUME:
		os_printf("%s resume\r\n", device_get_name(device));
		break;
	case PM_DEVICE_ACTION_POWER_ON:
		os_printf("%s power on\r\n", device_get_name(device));
		break;
	case PM_DEVICE_ACTION_POWER_OFF:
		os_printf("%s power off\r\n", device_get_name(device));
		break;
	case PM_DEVICE_ACTION_UPDATE_FREQ:
		os_printf("%s power off\r\n", device_get_name(device));
		break;
	default:
		os_printf("unknown action %d\r\n", action);
		break;
	}
#endif

	return BK_OK;
}

// power domain definition for bk7259
PM_DEVICE_DEFINE(aonp, null, PM_DEVICE_FLAG_ALWAYS_ON, 0, NULL);
PM_DEVICE_DEFINE(cpu1, null, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PD_CPU1, sys_hal_pd_cb);
PM_DEVICE_DEFINE(vehp, null, 0, PD_VEHP, sys_hal_pd_cb);
PM_DEVICE_DEFINE(wrls, null, 0, PD_WRLS, sys_hal_pd_cb);
PM_DEVICE_DEFINE(rom,  null, PM_DEVICE_FLAG_DEFAULT_ON | PM_DEVICE_FLAG_PASSIVE_ALL, PD_ROM, sys_hal_pd_cb);
// aonp
PM_DEVICE_DEFINE(sys,    aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(flash,  aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(prro,   aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(aon,    aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(ckmn,   aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(efuse,  aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(iomx,   aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(mem_check, aonp, PM_DEVICE_FLAG_ALWAYS_ON, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(timer,  aonp, PM_DEVICE_FLAG_ALWAYS_ON, PERI_TIM0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(uart,   aonp, PM_DEVICE_FLAG_ALWAYS_ON, PERI_UART0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i2c3,   aonp, PM_DEVICE_FLAG_ALWAYS_ON, PERI_I2C3, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(spi0,   aonp, PM_DEVICE_FLAG_ALWAYS_ON, PERI_SPI0, sys_hal_peri1_cb);
// cpu1: bakp audp
PM_DEVICE_DEFINE(bakp,   cpu1, PM_DEVICE_FLAG_PASSIVE_ALL, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(mbox,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(hspl,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(dma0,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(timer1, bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_TIM1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(uart1,  bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_UART1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(uart2,  bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_UART2, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(uart3,  bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_UART3, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i2c,    bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_I2C0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i2c1,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_I2C1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(spi1,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_SPI1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(spi2,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_SPI2, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(sadc,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_SADC, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(pwm,    bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_PWM0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i3c,    bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_I3C, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(timer2, bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_TIM2, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(timer3, bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_TIM3, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i2c2,   bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_I2C2, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(ipi,    bakp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(audp,   cpu1, 0, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(aud,    audp, 0, PERI_AUDIO, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(spdif,  audp, 0, PERI_AUDIF0, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(spdif1, audp, 0, PERI_AUDIF1, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(i2s,    null, PM_DEVICE_FLAG_PASSIVE_ALL, PERI_I2S0, sys_hal_peri1_cb); // TODO fix it
PM_DEVICE_DEFINE(i2s1,   audp, 0, PERI_I2S1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i2s2,   audp, 0, PERI_I2S2, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(i2s3,   audp, 0, PERI_I2S3, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(cec,    audp, 0, PERI_CEC, sys_hal_peri2_cb);
// vehp
PM_DEVICE_DEFINE(la,     vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(can0,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_CAN0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(can1,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_CAN1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(scr0,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_SCR0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(scr1,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_SCR1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(lin0,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_LIN0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(lin1,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_LIN1, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(irda,   vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_IRDA0, sys_hal_peri1_cb);
PM_DEVICE_DEFINE(irda1,  vehp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_IRDA1, sys_hal_peri1_cb);
// wrls: wrlp encp
PM_DEVICE_DEFINE(encp,   wrls, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(otp,    encp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_OTP, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(shanhai, encp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(wlss,   wrls, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_WLSS, sys_hal_wlss_cb);
PM_DEVICE_DEFINE(wifi,   wlss, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(mac,    wifi, PM_DEVICE_FLAG_WAKEUP_CAPABLE, ENTER_LOWVOL_WAKEUP_PROTECT_TIME, sys_hal_mac_cb);
PM_DEVICE_DEFINE(phy,    wifi, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_PHY, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(btsp,   wlss, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(btdm,   btsp, PM_DEVICE_FLAG_WAKEUP_CAPABLE, ENTER_LOWVOL_WAKEUP_PROTECT_TIME, sys_hal_btdm_cb);
PM_DEVICE_DEFINE(xver,   btsp, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_XVER, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(thread, wlss, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, PERI_THREAD, sys_hal_peri2_cb);
PM_DEVICE_DEFINE(phy_bt,   phy,  PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_phy_bt_cb);
PM_DEVICE_DEFINE(phy_wifi, phy,  PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_phy_wifi_cb);
PM_DEVICE_DEFINE(phy_rf,   phy,  PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);

PM_DEVICE_DEFINE(app, null, PM_DEVICE_FLAG_DEFAULT_ON | PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(log, null, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(at, null, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(pm, bakp, PM_DEVICE_FLAG_PASSIVE_ALL, 0, sys_hal_test_cb);

PM_DEVICE_DEFINE(rosc, null, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);
PM_DEVICE_DEFINE(rosc_prog, null, PM_DEVICE_FLAG_PASSIVE_DEEP_SLEEP, 0, sys_hal_test_cb);

// dummy TODO should remove it
PM_DEVICE_DEFINE(none, null, 0, 0, sys_hal_test_cb);

/**
 * Convert pm_power_module_name_e to device_t pointer for power domain control.
 * Attention: Power domain tree was different from SOCs.
*/
device_t *pm_convert_power_module_enum_to_device_ptr(pm_power_module_name_e module)
{
	switch(module)
	{
		case PM_POWER_MODULE_NAME_ENCP:
			return (device_t *)DEVICE_ID2PTR(encp);
		case PM_POWER_MODULE_NAME_BAKP:
			return (device_t *)DEVICE_ID2PTR(bakp);
		case PM_POWER_MODULE_NAME_AUDP:
			return (device_t *)DEVICE_ID2PTR(audp);
		case PM_POWER_MODULE_NAME_ROM_PGEN:
			return (device_t *)DEVICE_ID2PTR(rom);
		case PM_POWER_MODULE_NAME_CPU1:
			return (device_t *)DEVICE_ID2PTR(cpu1);
		case PM_POWER_MODULE_NAME_APP:
			return (device_t *)DEVICE_ID2PTR(app);
		case PM_POWER_SUB_MODULE_NAME_AUDP_I2S:
			return (device_t *)DEVICE_ID2PTR(i2s);
		case PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1:
			return (device_t *)DEVICE_ID2PTR(timer1);
		case PM_POWER_SUB_MODULE_NAME_BAKP_UART1:
			return (device_t *)DEVICE_ID2PTR(uart1);
		case PM_POWER_SUB_MODULE_NAME_BAKP_UART2:
			return (device_t *)DEVICE_ID2PTR(uart2);
		case PM_POWER_SUB_MODULE_NAME_BAKP_SPI1:
			return (device_t *)DEVICE_ID2PTR(spi1);
		case PM_POWER_SUB_MODULE_NAME_BAKP_I2C1:
			return (device_t *)DEVICE_ID2PTR(i2c1);
		case PM_POWER_SUB_MODULE_NAME_BAKP_SADC:
			return (device_t *)DEVICE_ID2PTR(sadc);
		case PM_POWER_SUB_MODULE_NAME_BAKP_IRDA:
			return (device_t *)DEVICE_ID2PTR(irda);
		case PM_POWER_SUB_MODULE_NAME_BAKP_DMA0:
			return (device_t *)DEVICE_ID2PTR(dma0);
		case PM_POWER_SUB_MODULE_NAME_BAKP_LA:
			return (device_t *)DEVICE_ID2PTR(la);
		case PM_POWER_SUB_MODULE_NAME_BAKP_UART3:
			return (device_t *)DEVICE_ID2PTR(uart3);
		case PM_POWER_SUB_MODULE_NAME_BAKP_I2S:
			return (device_t *)DEVICE_ID2PTR(i2s);
		case PM_POWER_MODULE_NAME_BTSP: // 8
			return (device_t *)DEVICE_ID2PTR(btdm);
		case PM_POWER_MODULE_NAME_WIFIP_MAC: // 9
			return (device_t *)DEVICE_ID2PTR(mac);
		case PM_POWER_SUB_MODULE_NAME_BAKP_PM:
			return (device_t *)DEVICE_ID2PTR(pm);
		case PM_POWER_MODULE_NAME_PHY: // 10
			return (device_t *)DEVICE_ID2PTR(phy);
		case PM_POWER_MODULE_NAME_THREAD: // 14
			return (device_t *)DEVICE_ID2PTR(thread);
		case PM_POWER_SUB_MODULE_NAME_PHY_BT:   // 300
			return (device_t *)DEVICE_ID2PTR(phy_bt);
		case PM_POWER_SUB_MODULE_NAME_PHY_WIFI: // 301
			return (device_t *)DEVICE_ID2PTR(phy_wifi);
		case PM_POWER_SUB_MODULE_NAME_PHY_RF:   // 302
			return (device_t *)DEVICE_ID2PTR(phy_rf);
		default:
			os_printf("power module %d not registed\r\n", module);
			return (device_t *)DEVICE_ID2PTR(none);
	}
	return NULL;
}

device_t *pm_convert_sleep_module_enum_to_device_ptr(pm_sleep_module_name_e module)
{
	switch(module)
	{
		case PM_SLEEP_MODULE_NAME_I2C1:
			return (device_t *)DEVICE_ID2PTR(i2c1);
		case PM_SLEEP_MODULE_NAME_SPI_1:
			return (device_t *)DEVICE_ID2PTR(spi1);
		case PM_SLEEP_MODULE_NAME_UART1:
			return (device_t *)DEVICE_ID2PTR(uart1);
		case PM_SLEEP_MODULE_NAME_TIMER_1:
			return (device_t *)DEVICE_ID2PTR(timer1);
		case PM_SLEEP_MODULE_NAME_SARADC:
			return (device_t *)DEVICE_ID2PTR(sadc);
		case PM_SLEEP_MODULE_NAME_AUDP:
			return (device_t *)DEVICE_ID2PTR(audp);
		case PM_SLEEP_MODULE_NAME_BTSP: // 8
			return (device_t *)DEVICE_ID2PTR(btdm);
		case PM_SLEEP_MODULE_NAME_WIFIP_MAC: // 9
			return (device_t *)DEVICE_ID2PTR(mac);
		case PM_SLEEP_MODULE_NAME_TIMER_2:
			return (device_t *)DEVICE_ID2PTR(timer2);
		case PM_SLEEP_MODULE_NAME_APP:
			return (device_t *)DEVICE_ID2PTR(app);
		case PM_SLEEP_MODULE_NAME_I2S_1:
			return (device_t *)DEVICE_ID2PTR(i2s1);
		case PM_SLEEP_MODULE_NAME_LOG: // 22
			return (device_t *)DEVICE_ID2PTR(log);
		case PM_SLEEP_MODULE_NAME_AT: // 23
			return (device_t *)DEVICE_ID2PTR(at);
		case PM_SLEEP_MODULE_NAME_I2C2: // 24
			return (device_t *)DEVICE_ID2PTR(i2c2);
		case PM_SLEEP_MODULE_NAME_UART2: // 25
			return (device_t *)DEVICE_ID2PTR(uart2);
		case PM_SLEEP_MODULE_NAME_UART3: // 26
			return (device_t *)DEVICE_ID2PTR(uart3);
		case PM_SLEEP_MODULE_NAME_TIMER_3: // 28
			return (device_t *)DEVICE_ID2PTR(timer3);
		case PM_SLEEP_MODULE_NAME_CPU1: // 30
			return (device_t *)DEVICE_ID2PTR(cpu1);
		case PM_SLEEP_MODULE_NAME_ROSC_PROG: // 31
			return (device_t *)DEVICE_ID2PTR(rosc_prog);
		case PM_SLEEP_MODULE_NAME_ROSC: // 32
			return (device_t *)DEVICE_ID2PTR(rosc);
		default:
			os_printf("sleep module %d not registed\r\n", module);
			return (device_t *)DEVICE_ID2PTR(none);
	}
	return NULL;
}
#endif

void sys_hal_set_ota_finish(uint32_t value)
{
	aon_pmu_ll_set_r0_ota_finish(value);
}

uint32_t sys_hal_get_ota_finish(void)
{
	return aon_pmu_ll_get_r0_ota_finish();
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