// Copyright 2026 Beken
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
#include <driver/aud_common.h>
#include <driver/aud_lp_vad.h>
#include <driver/int_types.h>
#include <os/os.h>
#include "sys_hal.h"
#include "sys_ll.h"
#include "sys_driver.h"
#include "aon_pmu_hal.h"
#include "aon_pmu_ll.h"
#include "bk_arch.h"
#include "bk_misc.h"

#define LPVAD_LOG(fmt, ...) bk_printf("[lpvad] " fmt "\r\n", ##__VA_ARGS__)

static bool s_lp_vad_is_init = false;
static lp_vad_isr_t s_lp_vad_isr = NULL;

static uint32_t s_backup_ana_reg22 = 0;
static uint32_t s_backup_ana_reg23 = 0;
static uint32_t s_backup_ana_reg28 = 0;

bk_err_t bk_lp_vad_set_sleep_para_before_sleep(void)
{
	uint32_t reg2 = 0;

	/* VAD Config */
	sys_hal_set_gadc_config(0x7CAA5241);
	sys_hal_set_vad_config(0x40FE2A33);
	sys_hal_set_ana_reg24_value(0x014064C0);

	/* ana_reg2 bit31 (adcdcsel) clear */
	reg2 = sys_hal_get_ana_reg2_value();
	reg2 &= ~(1U << 31);
	sys_hal_set_ana_reg2_value(reg2);

	/* Enable micbias (ana_reg20 bit5) */
	//sys_hal_set_micbias_enable(1);

	/* Enable mic2 mode (ana_reg27 bit28) */
	sys_hal_set_mic2_enable(1);

	sys_hal_set_vad_enable(1);

	sys_hal_set_vad_viniset(1);
	bk_delay_us(10000);

	sys_hal_set_vad_rstn(1);
	__ISB();
	__DSB();
	bk_delay_us(1000);
	__ISB();
	__DSB();

	return BK_OK;
}

bk_err_t bk_lp_vad_set_wakeup_para_after_wakeup(void)
{
	sys_hal_set_gadc_config(s_backup_ana_reg22);
	sys_hal_set_vad_config(s_backup_ana_reg23);
	sys_hal_set_ana_reg28_value(s_backup_ana_reg28);

	return BK_OK;
}

static void lp_vad_isr(void)
{
	sys_hal_set_vad_config(0);

	//LPVAD_LOG("irq enter, cb=%p", s_lp_vad_isr);
	if (s_lp_vad_isr) {
		s_lp_vad_isr();
	}

}

static void lp_vad_sleep_ana_set_tmp(void)
{
	/* spi latch ON */
	sys_hal_set_ana_reg_spi_latch1v(1);

	/* Core LDO low voltage: vcorelsel = 0x2 */
	sys_hal_set_core_ldo_low_vol(0x2);

	/* CoreH LDO voltage: vcorehsel = 0xE */
	sys_hal_set_core_ldo_high_vol(0xE);

	/* Enter low power mode: disable HP modes, select xtal32k */
	sys_hal_set_aloldo_hp(0);
	sys_hal_set_analdo_hp(0);
	sys_hal_set_digldo_hp(0);
	sys_hal_set_coreldo_hp(0);
	sys_hal_set_rtc_clk_sel(1);

	/* Re-enable coreldo high power mode */
	sys_hal_set_coreldo_hp(1);

	/* Enable digldo and coreldo low voltage mode */
	sys_hal_set_digldo_low_vol_enable(1);
	sys_hal_set_coreldo_low_vol_enable(1);

	/* Close vdd_ana: disable bucka EA fast transient */
	sys_hal_set_bucka_ea_enable(0);

	/* spi latch OFF */
	sys_hal_set_ana_reg_spi_latch1v(0);

	/* Close HF clocks: dpll, dco, xtall, cb */
	sys_hal_set_dpll_enable(0);
	sys_hal_set_dco_enable(0);
	sys_hal_set_xtall_enable(0);
	sys_hal_set_cb_enable(0);
}

// Initialize VAD related configuration
// 0X57<31:0>=Hex40FE2A33; ana_reg23
// 0X58<31:0>=Hex014064C0; ana_reg24
bk_err_t bk_lp_vad_init(void)
{
	//LPVAD_LOG("init enter, is_init=%d", s_lp_vad_is_init);
	if (s_lp_vad_is_init) {
		//LPVAD_LOG("init skip, already inited");
		return BK_OK;
	}


	sys_hal_set_vad_config(0x40FE2A33);
	sys_hal_set_ana_reg24_value(0x014064C0);
	//LPVAD_LOG("init reg23=0x%08x reg24=0x%08x", sys_hal_get_vad_config(), sys_hal_get_ana_reg24_value());
	s_lp_vad_is_init = true;
	//LPVAD_LOG("init done");
	return BK_OK;
}

bk_err_t bk_lp_vad_deinit(void)
{
	//LPVAD_LOG("deinit enter, is_init=%d", s_lp_vad_is_init);
	if (!s_lp_vad_is_init) {
		//LPVAD_LOG("deinit skip, not inited");
		return BK_OK;
	}

	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_VAD, 0);
	bk_int_isr_unregister(INT_SRC_VAD);
	s_lp_vad_isr = NULL;

	s_lp_vad_is_init = false;
	//LPVAD_LOG("deinit done");
	return BK_OK;
}

bk_err_t bk_lp_vad_deepsleep_enter(void)
{
	//uint32_t reg2 = 0;
	uint32_t value = 0;
	uint8_t mem_halt_pwd = 0;

	//LPVAD_LOG("set_para_before_sleep enter, is_init=%d", s_lp_vad_is_init);
	if (!s_lp_vad_is_init) {
		//LPVAD_LOG("set_para_before_sleep fail, not inited");
		return BK_ERR_AUD_DRV_NOT_INIT;
	}
	/* spi latch ON */
	sys_hal_set_ana_reg_spi_latch1v(1);

	/* Aon LDO: valoldosel=0x6 */
	sys_hal_set_aon_ldo_vol(0x6);
	/* Core LDO: vcorelsel=0x6, vcorehsel=0x8 (fix off-by-one from original shift 19 to correct shift 20) */
	sys_hal_set_core_ldo_low_vol(0x6);
	sys_hal_set_core_ldo_high_vol(0x8);
	value = sys_ll_get_reserver_reg0x10_value();
	value &= ~((0x7U << 15) | (0xFU << 11));
	value |= ((0x6U & 0x7U) << 15) | ((0x8U & 0xFU) << 11);
	sys_ll_set_reserver_reg0x10_value(value);
	/* spi latch OFF */
	sys_hal_set_ana_reg_spi_latch1v(0);

	value = sys_ll_get_reserver_reg0x10_value();
	value |= 0xFU;
	sys_ll_set_reserver_reg0x10_value(value);
	/* Power down blp and wlp peripherals */
	aon_pmu_ll_set_r42_pwd_blppwd(1);
	aon_pmu_hal_set_wlp_power_down(1);
	/* Set Core and Flash clock to XTAL */
	value = sys_ll_get_cpu_clk_div_mode1_value();
	value &= ~((0x7U << 8) | (0x3U << 6) | (0xFU << 2) | (0x3U << 0));
	sys_ll_set_cpu_clk_div_mode1_value(value);
	aon_pmu_hal_set_wakeup_source(WAKEUP_SOURCE_INT_VAD);
	aon_pmu_hal_lpo_src_set(0x2);
	if (mem_halt_pwd & (1U << 0)) {
		aon_pmu_ll_set_r41_halt_sram0(1);
	}
	if (mem_halt_pwd & (1U << 1)) {
		aon_pmu_ll_set_r41_halt_sram1(1);
	}
	if (mem_halt_pwd & (1U << 2)) {
		aon_pmu_ll_set_r41_halt_sram2(1);
	}
	if (mem_halt_pwd & (1U << 3)) {
		aon_pmu_ll_set_r41_halt_cache(1);
	}
	aon_pmu_ll_set_r41_mem_ret_en(1);

	/* sleep_ana_set_tmp(); */
	lp_vad_sleep_ana_set_tmp();

	/* Clear wakeup source status */
	aon_pmu_ll_set_r43_clr_wakeup(1);
	aon_pmu_ll_set_r43_clr_wakeup(0);
	value = sys_ll_get_cpu_power_sleep_wakeup_value();
	value |= 0x1U | (1U << 4);
	sys_ll_set_cpu_power_sleep_wakeup_value(value);
	value = aon_pmu_ll_get_r0();
	value = (value & (~0x3U)) | 0x3U;
	aon_pmu_ll_set_r0(value);

	s_backup_ana_reg22 = sys_hal_get_gadc_config();
	s_backup_ana_reg23 = sys_hal_get_vad_config();
	s_backup_ana_reg28 = sys_hal_get_ana_reg28_value();

	bk_lp_vad_set_sleep_para_before_sleep();

	__ISB();
	__DSB();
	bk_delay_us(1000000);

	/* SetSleepMode(1) equivalent: enter sleepdeep then sync barrier */
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__ISB();
	__DSB();

	/* post sequence: spi latch ON */
	sys_hal_set_ana_reg_spi_latch1v(1);
	/* Enable vdd_ana: bucka EA enable */
	sys_hal_set_bucka_ea_enable(1);
	/* Aon LDO restore: valoldosel=0x1 (commented out write) */
	//sys_hal_set_aon_ldo_vol(0x1); //charl20260323
	/* spi latch OFF */
	sys_hal_set_ana_reg_spi_latch1v(0);

	__ISB();
	__DSB();
	//__WFI();

	//LPVAD_LOG("set_para_before_sleep done (after WFI)");
	return BK_OK;
}

bk_err_t bk_lp_vad_register_isr(lp_vad_isr_t isr)
{
	//LPVAD_LOG("register_isr enter, is_init=%d, cb=%p", s_lp_vad_is_init, isr);
	if (!s_lp_vad_is_init) {
		//LPVAD_LOG("register_isr fail, not inited");
		return BK_ERR_AUD_DRV_NOT_INIT;
	}
	s_lp_vad_isr = isr;
	bk_int_isr_register(INT_SRC_VAD, lp_vad_isr, NULL);
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_VAD, 1);

	//LPVAD_LOG("register_isr done, int_src=%d", INT_SRC_VAD);
	return BK_OK;
}


