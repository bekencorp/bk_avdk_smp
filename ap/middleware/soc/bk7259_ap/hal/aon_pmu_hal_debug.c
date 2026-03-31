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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "aon_pmu_hw.h"
#include "aon_pmu_hal.h"

typedef void (*aon_pmu_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	aon_pmu_dump_fn_t fn;
} aon_pmu_reg_fn_map_t;

static void aon_pmu_dump_r0(void)
{
	aon_pmu_r0_t *r = (aon_pmu_r0_t *)(SOC_AON_PMU_REG_BASE + (0x0 << 2));

	SOC_LOGI("r0: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x0 << 2)));
	SOC_LOGI("	memchk_bps: %8x\r\n", r->memchk_bps);
	SOC_LOGI("	fast_boot: %8x\r\n", r->fast_boot);
	SOC_LOGI("	aon_reg0_for_software: %8x\r\n", r->aon_reg0_for_software);
	SOC_LOGI("	gpio_sleep: %8x\r\n", r->gpio_sleep);
}

static void aon_pmu_dump_r2(void)
{
	aon_pmu_r2_t *r = (aon_pmu_r2_t *)(SOC_AON_PMU_REG_BASE + (0x2 << 2));

	SOC_LOGI("r2: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	wdt_rst_ana: %8x\r\n", r->wdt_rst_ana);
	SOC_LOGI("	wdt_rst_top: %8x\r\n", r->wdt_rst_top);
	SOC_LOGI("	wdt_rst_aon: %8x\r\n", r->wdt_rst_aon);
	SOC_LOGI("	wdt_rst_awt: %8x\r\n", r->wdt_rst_awt);
	SOC_LOGI("	wdt_rst_gpio: %8x\r\n", r->wdt_rst_gpio);
	SOC_LOGI("	wdt_rst_rtc: %8x\r\n", r->wdt_rst_rtc);
	SOC_LOGI("	wdt_rst_wdt: %8x\r\n", r->wdt_rst_wdt);
	SOC_LOGI("	wdt_rst_pmu: %8x\r\n", r->wdt_rst_pmu);
	SOC_LOGI("	wdt_rst_blp_wlp: %8x\r\n", r->wdt_rst_blp_wlp);
	SOC_LOGI("	reserved_bit_9_15: %8x\r\n", r->reserved_bit_9_15);
	SOC_LOGI("	m55_iso_en: %8x\r\n", r->m55_iso_en);
	SOC_LOGI("	m55_rstn: %8x\r\n", r->m55_rstn);
	SOC_LOGI("	m55_mem_ret: %8x\r\n", r->m55_mem_ret);
	SOC_LOGI("	m55_mem3_pwd: %8x\r\n", r->m55_mem3_pwd);
	SOC_LOGI("	m55_clk_en: %8x\r\n", r->m55_clk_en);
	SOC_LOGI("	m55_mem4_pwd: %8x\r\n", r->m55_mem4_pwd);
	SOC_LOGI("	m55_mem5_pwd: %8x\r\n", r->m55_mem5_pwd);
	SOC_LOGI("	m55_mem6_pwd: %8x\r\n", r->m55_mem6_pwd);
	SOC_LOGI("	m55_cpu2_cache_pwd: %8x\r\n", r->m55_cpu2_cache_pwd);
	SOC_LOGI("	m55_cpu3_cache_pwd: %8x\r\n", r->m55_cpu3_cache_pwd);
	SOC_LOGI("	m55_cpu2_cache_init_dis_maint: %8x\r\n", r->m55_cpu2_cache_init_dis_maint);
	SOC_LOGI("	m55_cpu3_cache_init_dis_maint: %8x\r\n", r->m55_cpu3_cache_init_dis_maint);
	SOC_LOGI("	m55_mem_auto_set: %8x\r\n", r->m55_mem_auto_set);
	SOC_LOGI("	m55_auto_sel: %8x\r\n", r->m55_auto_sel);
	SOC_LOGI("	reserved_bit_30_30: %8x\r\n", r->reserved_bit_30_30);
	SOC_LOGI("	otp_vdd_en: %8x\r\n", r->otp_vdd_en);
}

static void aon_pmu_dump_r3(void)
{
	aon_pmu_r3_t *r = (aon_pmu_r3_t *)(SOC_AON_PMU_REG_BASE + (0x3 << 2));

	SOC_LOGI("r3: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x3 << 2)));
	SOC_LOGI("	for_sw: %8x\r\n", r->for_sw);
	SOC_LOGI("	shutdown_flag: %8x\r\n", r->shutdown_flag);
}

static void aon_pmu_dump_r4(void)
{
	aon_pmu_r4_t *r = (aon_pmu_r4_t *)(SOC_AON_PMU_REG_BASE + (0x4 << 2));

	SOC_LOGI("r4: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	program_state: %8x\r\n", r->program_state);
	SOC_LOGI("	reserved_bit_4_30: %8x\r\n", r->reserved_bit_4_30);
	SOC_LOGI("	aon_encp_jtag_diable: %8x\r\n", r->aon_encp_jtag_diable);
}

static void aon_pmu_dump_r5(void)
{
	aon_pmu_r5_t *r = (aon_pmu_r5_t *)(SOC_AON_PMU_REG_BASE + (0x5 << 2));

	SOC_LOGI("r5: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	encp_spi_to_ahb_enable: %8x\r\n", r->encp_spi_to_ahb_enable);
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void aon_pmu_dump_rsv_6_24(void)
{
	for (uint32_t idx = 0; idx < 31; idx++) {
		SOC_LOGI("rsv_6_24: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x6 + idx) << 2)));
	}
}

static void aon_pmu_dump_r25(void)
{
	SOC_LOGI("r25: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x25 << 2)));
}

static void aon_pmu_dump_rsv_26_3f(void)
{
	for (uint32_t idx = 0; idx < 26; idx++) {
		SOC_LOGI("rsv_26_3f: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x26 + idx) << 2)));
	}
}

static void aon_pmu_dump_r40(void)
{
	aon_pmu_r40_t *r = (aon_pmu_r40_t *)(SOC_AON_PMU_REG_BASE + (0x40 << 2));

	SOC_LOGI("r40: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x40 << 2)));
	SOC_LOGI("	wake1_delay: %8x\r\n", r->wake1_delay);
	SOC_LOGI("	wake2_delay: %8x\r\n", r->wake2_delay);
	SOC_LOGI("	wake3_delay: %8x\r\n", r->wake3_delay);
	SOC_LOGI("	halt1_delay: %8x\r\n", r->halt1_delay);
	SOC_LOGI("	halt2_delay: %8x\r\n", r->halt2_delay);
	SOC_LOGI("	halt3_delay: %8x\r\n", r->halt3_delay);
	SOC_LOGI("	halt_volt: %8x\r\n", r->halt_volt);
	SOC_LOGI("	halt_xtal: %8x\r\n", r->halt_xtal);
	SOC_LOGI("	halt_core: %8x\r\n", r->halt_core);
	SOC_LOGI("	halt_flash: %8x\r\n", r->halt_flash);
	SOC_LOGI("	halt_rosc: %8x\r\n", r->halt_rosc);
	SOC_LOGI("	halt_resten: %8x\r\n", r->halt_resten);
	SOC_LOGI("	halt_isolat: %8x\r\n", r->halt_isolat);
	SOC_LOGI("	halt_clkena: %8x\r\n", r->halt_clkena);
}

static void aon_pmu_dump_r41(void)
{
	aon_pmu_r41_t *r = (aon_pmu_r41_t *)(SOC_AON_PMU_REG_BASE + (0x41 << 2));

	SOC_LOGI("r41: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x41 << 2)));
	SOC_LOGI("	lpo_config: %8x\r\n", r->lpo_config);
	SOC_LOGI("	flshsck_iocap: %8x\r\n", r->flshsck_iocap);
	SOC_LOGI("	wakeup_ena: %8x\r\n", r->wakeup_ena);
	SOC_LOGI("	gpio_int_clksel: %8x\r\n", r->gpio_int_clksel);
	SOC_LOGI("	reserved_bit_12_13: %8x\r\n", r->reserved_bit_12_13);
	SOC_LOGI("	xtal_sel: %8x\r\n", r->xtal_sel);
	SOC_LOGI("	gpio_func_ctrl_en: %8x\r\n", r->gpio_func_ctrl_en);
	SOC_LOGI("	sleep_wdt_disable: %8x\r\n", r->sleep_wdt_disable);
	SOC_LOGI("	reserved_bit_17_18: %8x\r\n", r->reserved_bit_17_18);
	SOC_LOGI("	wlp_switch_en: %8x\r\n", r->wlp_switch_en);
	SOC_LOGI("	lcd_clk_inv: %8x\r\n", r->lcd_clk_inv);
	SOC_LOGI("	reserved_bit_21_23: %8x\r\n", r->reserved_bit_21_23);
	SOC_LOGI("	halt_lpo: %8x\r\n", r->halt_lpo);
	SOC_LOGI("	halt_sram0: %8x\r\n", r->halt_sram0);
	SOC_LOGI("	halt_sram1: %8x\r\n", r->halt_sram1);
	SOC_LOGI("	halt_sram2: %8x\r\n", r->halt_sram2);
	SOC_LOGI("	mem_ret_en: %8x\r\n", r->mem_ret_en);
	SOC_LOGI("	halt_cache: %8x\r\n", r->halt_cache);
	SOC_LOGI("	m52_l2_dis_cache_init_dis_maint: %8x\r\n", r->m52_l2_dis_cache_init_dis_maint);
	SOC_LOGI("	m52_cpu_l2_dis_cache_en_maint: %8x\r\n", r->m52_cpu_l2_dis_cache_en_maint);
}

static void aon_pmu_dump_r42(void)
{
	aon_pmu_r42_t *r = (aon_pmu_r42_t *)(SOC_AON_PMU_REG_BASE + (0x42 << 2));

	SOC_LOGI("r42: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x42 << 2)));
	SOC_LOGI("	pwd_blppwd: %8x\r\n", r->pwd_blppwd);
	SOC_LOGI("	pwd_wlppwd: %8x\r\n", r->pwd_wlppwd);
	SOC_LOGI("	blp_isolate_state: %8x\r\n", r->blp_isolate_state);
	SOC_LOGI("	wlp_isolate_state: %8x\r\n", r->wlp_isolate_state);
	SOC_LOGI("	reserved_bit_4_31: %8x\r\n", r->reserved_bit_4_31);
}

static void aon_pmu_dump_r43(void)
{
	aon_pmu_r43_t *r = (aon_pmu_r43_t *)(SOC_AON_PMU_REG_BASE + (0x43 << 2));

	SOC_LOGI("r43: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x43 << 2)));
	SOC_LOGI("	clr_int_touched: %8x\r\n", r->clr_int_touched);
	SOC_LOGI("	clr_int_usbplug: %8x\r\n", r->clr_int_usbplug);
	SOC_LOGI("	clr_wakeup: %8x\r\n", r->clr_wakeup);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void aon_pmu_dump_rsv_44_6f(void)
{
	for (uint32_t idx = 0; idx < 44; idx++) {
		SOC_LOGI("rsv_44_6f: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x44 + idx) << 2)));
	}
}

static void aon_pmu_dump_r70(void)
{
	aon_pmu_r70_t *r = (aon_pmu_r70_t *)(SOC_AON_PMU_REG_BASE + (0x70 << 2));

	SOC_LOGI("r70: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x70 << 2)));
	SOC_LOGI("	int_touched: %8x\r\n", r->int_touched);
	SOC_LOGI("	int_usbplug: %8x\r\n", r->int_usbplug);
	SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void aon_pmu_dump_r71(void)
{
	aon_pmu_r71_t *r = (aon_pmu_r71_t *)(SOC_AON_PMU_REG_BASE + (0x71 << 2));

	SOC_LOGI("r71: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x71 << 2)));
	SOC_LOGI("	touch_state: %8x\r\n", r->touch_state);
	SOC_LOGI("	usbplug_state: %8x\r\n", r->usbplug_state);
	SOC_LOGI("	reserved_bit_17_19: %8x\r\n", r->reserved_bit_17_19);
	SOC_LOGI("	wakeup_source: %8x\r\n", r->wakeup_source);
	SOC_LOGI("	reserved_bit_27_31: %8x\r\n", r->reserved_bit_27_31);
}

static void aon_pmu_dump_rsv_72_73(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGI("rsv_72_73: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x72 + idx) << 2)));
	}
}

static void aon_pmu_dump_r74(void)
{
	aon_pmu_r74_t *r = (aon_pmu_r74_t *)(SOC_AON_PMU_REG_BASE + (0x74 << 2));

	SOC_LOGI("r74: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x74 << 2)));
	SOC_LOGI("	por_corehs_n: %8x\r\n", r->por_corehs_n);
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void aon_pmu_dump_r75(void)
{
	SOC_LOGI("r75: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x75 << 2)));
}

static void aon_pmu_dump_rsv_76_77(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGI("rsv_76_77: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + ((0x76 + idx) << 2)));
	}
}

static void aon_pmu_dump_r78(void)
{
	aon_pmu_r78_t *r = (aon_pmu_r78_t *)(SOC_AON_PMU_REG_BASE + (0x78 << 2));

	SOC_LOGI("r78: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x78 << 2)));
	SOC_LOGI("	ana_rtc_state: %8x\r\n", r->ana_rtc_state);
	SOC_LOGI("	reserved_bit_4_31: %8x\r\n", r->reserved_bit_4_31);
}

static void aon_pmu_dump_r79(void)
{
	SOC_LOGI("r79: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x79 << 2)));
}

static void aon_pmu_dump_r7a(void)
{
	SOC_LOGI("r7a: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7a << 2)));
}

static void aon_pmu_dump_r7b(void)
{
	SOC_LOGI("r7b: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7b << 2)));
}

static void aon_pmu_dump_r7c(void)
{
	SOC_LOGI("r7c: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7c << 2)));
}

static void aon_pmu_dump_r7d(void)
{
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t *)(SOC_AON_PMU_REG_BASE + (0x7d << 2));

	SOC_LOGI("r7d: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7d << 2)));
	SOC_LOGI("	lcal: %8x\r\n", r->lcal);
	SOC_LOGI("	l: %8x\r\n", r->l);
	SOC_LOGI("	adc_cal: %8x\r\n", r->adc_cal);
	SOC_LOGI("	bgcal: %8x\r\n", r->bgcal);
	SOC_LOGI("	sig_26mpll_unlock: %8x\r\n", r->sig_26mpll_unlock);
	SOC_LOGI("	dpll_unlock: %8x\r\n", r->dpll_unlock);
	SOC_LOGI("	h: %8x\r\n", r->h);
}

static void aon_pmu_dump_r7e(void)
{
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t *)(SOC_AON_PMU_REG_BASE + (0x7e << 2));

	SOC_LOGI("r7e: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7e << 2)));
	SOC_LOGI("	cbcal: %8x\r\n", r->cbcal);
	SOC_LOGI("	ad_state: %8x\r\n", r->ad_state);
	SOC_LOGI("	td_states1: %8x\r\n", r->td_states1);
	SOC_LOGI("	h: %8x\r\n", r->h);
}

static void aon_pmu_dump_r7f(void)
{
	SOC_LOGI("r7f: %8x\r\n", REG_READ(SOC_AON_PMU_REG_BASE + (0x7f << 2)));
}

static aon_pmu_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, aon_pmu_dump_r0},
	{0x2, 0x2, aon_pmu_dump_r2},
	{0x3, 0x3, aon_pmu_dump_r3},
	{0x4, 0x4, aon_pmu_dump_r4},
	{0x5, 0x5, aon_pmu_dump_r5},
	{0x6, 0x25, aon_pmu_dump_rsv_6_24},
	{0x25, 0x25, aon_pmu_dump_r25},
	{0x26, 0x40, aon_pmu_dump_rsv_26_3f},
	{0x40, 0x40, aon_pmu_dump_r40},
	{0x41, 0x41, aon_pmu_dump_r41},
	{0x42, 0x42, aon_pmu_dump_r42},
	{0x43, 0x43, aon_pmu_dump_r43},
	{0x44, 0x70, aon_pmu_dump_rsv_44_6f},
	{0x70, 0x70, aon_pmu_dump_r70},
	{0x71, 0x71, aon_pmu_dump_r71},
	{0x72, 0x74, aon_pmu_dump_rsv_72_73},
	{0x74, 0x74, aon_pmu_dump_r74},
	{0x75, 0x75, aon_pmu_dump_r75},
	{0x76, 0x78, aon_pmu_dump_rsv_76_77},
	{0x78, 0x78, aon_pmu_dump_r78},
	{0x79, 0x79, aon_pmu_dump_r79},
	{0x7a, 0x7a, aon_pmu_dump_r7a},
	{0x7b, 0x7b, aon_pmu_dump_r7b},
	{0x7c, 0x7c, aon_pmu_dump_r7c},
	{0x7d, 0x7d, aon_pmu_dump_r7d},
	{0x7e, 0x7e, aon_pmu_dump_r7e},
	{0x7f, 0x7f, aon_pmu_dump_r7f},
	{-1, -1, 0}
};

void aon_pmu_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
