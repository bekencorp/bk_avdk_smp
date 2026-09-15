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

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "aon_pmu_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AON_PMU_LL_REG_BASE   SOC_AON_PMU_REG_BASE

//reg r0:

static inline void aon_pmu_ll_set_r0(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r0(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r0_memchk_bps(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->memchk_bps = v;
}

static inline uint32_t aon_pmu_ll_get_r0_memchk_bps(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->memchk_bps;
}

static inline uint32_t aon_pmu_ll_get_r7b_memchk_bps(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->memchk_bps;
}

static inline void aon_pmu_ll_set_r0_fast_boot(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->fast_boot = v;
}

static inline uint32_t aon_pmu_ll_get_r0_fast_boot(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->fast_boot;
}

static inline uint32_t aon_pmu_ll_get_r7b_fast_boot(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->fast_boot;
}

static inline void aon_pmu_ll_set_r0_aon_reg0_for_software(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->aon_reg0_for_software = v;
}

static inline uint32_t aon_pmu_ll_get_r0_aon_reg0_for_software(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->aon_reg0_for_software;
}

static inline void aon_pmu_ll_set_r0_gpio_sleep(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->gpio_sleep = v;
}

static inline uint32_t aon_pmu_ll_get_r0_gpio_sleep(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->gpio_sleep;
}

static inline uint32_t aon_pmu_ll_get_r7b_gpio_sleep(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->gpio_sleep;
}

static inline void aon_pmu_ll_set_r0_reset_reason(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->reset_reason = v;
}

static inline uint32_t aon_pmu_ll_get_r0_reset_reason(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->reset_reason;
}

static inline uint32_t aon_pmu_ll_get_r7a_reset_reason(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x7a << 2));
	return r->reset_reason;
}

static inline uint32_t aon_pmu_ll_get_r7b_reset_reason(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->reset_reason;
}

static inline void aon_pmu_ll_set_r0_gpio_retention_bitmap(uint32_t v) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	r->gpio_retention_bitmap = v;
}

static inline uint32_t aon_pmu_ll_get_r0_gpio_retention_bitmap(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x0 << 2));
	return r->gpio_retention_bitmap;
}

static inline uint32_t aon_pmu_ll_get_r7b_gpio_retention_bitmap(void) {
	aon_pmu_r0_t *r = (aon_pmu_r0_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->gpio_retention_bitmap;
}

//reg r2:

static inline void aon_pmu_ll_set_r2(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r2(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_ana(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_ana = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_ana(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_ana;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_top(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_top = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_top(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_top;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_aon(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_aon = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_aon(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_aon;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_awt(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_awt = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_awt(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_awt;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_gpio(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_gpio = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_gpio(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_gpio;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_rtc(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_rtc = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_rtc(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_rtc;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_wdt(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_wdt = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_wdt(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_wdt;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_pmu(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_pmu = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_pmu(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_pmu;
}

static inline void aon_pmu_ll_set_r2_wdt_rst_blp_wlp(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->wdt_rst_blp_wlp = v;
}

static inline uint32_t aon_pmu_ll_get_r2_wdt_rst_blp_wlp(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->wdt_rst_blp_wlp;
}

static inline void aon_pmu_ll_set_r2_m55_iso_en(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_iso_en = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_iso_en(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_iso_en;
}

static inline void aon_pmu_ll_set_r2_m55_rstn(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_rstn = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_rstn(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_rstn;
}

static inline void aon_pmu_ll_set_r2_m55_mem_ret(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_mem_ret = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_mem_ret(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_mem_ret;
}

static inline void aon_pmu_ll_set_r2_m55_mem3_pwd(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_mem3_pwd = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_mem3_pwd(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_mem3_pwd;
}

static inline void aon_pmu_ll_set_r2_m55_clk_en(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_clk_en = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_clk_en(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_clk_en;
}

static inline void aon_pmu_ll_set_r2_m55_mem4_pwd(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_mem4_pwd = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_mem4_pwd(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_mem4_pwd;
}

static inline void aon_pmu_ll_set_r2_m55_mem5_pwd(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_mem5_pwd = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_mem5_pwd(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_mem5_pwd;
}

static inline void aon_pmu_ll_set_r2_m55_mem6_pwd(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_mem6_pwd = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_mem6_pwd(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_mem6_pwd;
}

static inline void aon_pmu_ll_set_r2_m55_cpu2_cache_pwd(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_cpu2_cache_pwd = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_cpu2_cache_pwd(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_cpu2_cache_pwd;
}

static inline void aon_pmu_ll_set_r2_m55_cpu3_cache_pwd(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_cpu3_cache_pwd = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_cpu3_cache_pwd(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_cpu3_cache_pwd;
}

static inline void aon_pmu_ll_set_r2_m55_cpu2_cache_init_dis_maint(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_cpu2_cache_init_dis_maint = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_cpu2_cache_init_dis_maint(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_cpu2_cache_init_dis_maint;
}

static inline void aon_pmu_ll_set_r2_m55_cpu3_cache_init_dis_maint(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_cpu3_cache_init_dis_maint = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_cpu3_cache_init_dis_maint(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_cpu3_cache_init_dis_maint;
}

static inline void aon_pmu_ll_set_r2_m55_mem_auto_set(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_mem_auto_set = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_mem_auto_set(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_mem_auto_set;
}

static inline void aon_pmu_ll_set_r2_m55_auto_sel(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->m55_auto_sel = v;
}

static inline uint32_t aon_pmu_ll_get_r2_m55_auto_sel(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->m55_auto_sel;
}

static inline void aon_pmu_ll_set_r2_otp_vdd_en(uint32_t v) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	r->otp_vdd_en = v;
}

static inline uint32_t aon_pmu_ll_get_r2_otp_vdd_en(void) {
	aon_pmu_r2_t *r = (aon_pmu_r2_t*)(SOC_AON_PMU_REG_BASE + (0x2 << 2));
	return r->otp_vdd_en;
}

//reg r3:

static inline void aon_pmu_ll_set_r3_value(uint32_t v) {
	aon_pmu_r3_t *r = (aon_pmu_r3_t*)(SOC_AON_PMU_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r3_value(void) {
	aon_pmu_r3_t *r = (aon_pmu_r3_t*)(SOC_AON_PMU_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r3_for_sw(uint32_t v) {
	aon_pmu_r3_t *r = (aon_pmu_r3_t*)(SOC_AON_PMU_REG_BASE + (0x3 << 2));
	r->for_sw = v;
}

static inline uint32_t aon_pmu_ll_get_r3_for_sw(void) {
	aon_pmu_r3_t *r = (aon_pmu_r3_t*)(SOC_AON_PMU_REG_BASE + (0x3 << 2));
	return r->for_sw;
}

static inline void aon_pmu_ll_set_r3_shutdown_flag(uint32_t v) {
	aon_pmu_r3_t *r = (aon_pmu_r3_t*)(SOC_AON_PMU_REG_BASE + (0x3 << 2));
	r->shutdown_flag = v;
}

static inline uint32_t aon_pmu_ll_get_r3_shutdown_flag(void) {
	aon_pmu_r3_t *r = (aon_pmu_r3_t*)(SOC_AON_PMU_REG_BASE + (0x3 << 2));
	return r->shutdown_flag;
}

//reg r4:

static inline void aon_pmu_ll_set_r4_value(uint32_t v) {
	aon_pmu_r4_t *r = (aon_pmu_r4_t*)(SOC_AON_PMU_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r4_value(void) {
	aon_pmu_r4_t *r = (aon_pmu_r4_t*)(SOC_AON_PMU_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r4_program_state(uint32_t v) {
	aon_pmu_r4_t *r = (aon_pmu_r4_t*)(SOC_AON_PMU_REG_BASE + (0x4 << 2));
	r->program_state = v;
}

static inline uint32_t aon_pmu_ll_get_r4_program_state(void) {
	aon_pmu_r4_t *r = (aon_pmu_r4_t*)(SOC_AON_PMU_REG_BASE + (0x4 << 2));
	return r->program_state;
}

static inline void aon_pmu_ll_set_r4_aon_encp_jtag_diable(uint32_t v) {
	aon_pmu_r4_t *r = (aon_pmu_r4_t*)(SOC_AON_PMU_REG_BASE + (0x4 << 2));
	r->aon_encp_jtag_diable = v;
}

static inline uint32_t aon_pmu_ll_get_r4_aon_encp_jtag_diable(void) {
	aon_pmu_r4_t *r = (aon_pmu_r4_t*)(SOC_AON_PMU_REG_BASE + (0x4 << 2));
	return r->aon_encp_jtag_diable;
}

//reg r5:

static inline void aon_pmu_ll_set_r5_value(uint32_t v) {
	aon_pmu_r5_t *r = (aon_pmu_r5_t*)(SOC_AON_PMU_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r5_value(void) {
	aon_pmu_r5_t *r = (aon_pmu_r5_t*)(SOC_AON_PMU_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r5_encp_spi_to_ahb_enable(uint32_t v) {
	aon_pmu_r5_t *r = (aon_pmu_r5_t*)(SOC_AON_PMU_REG_BASE + (0x5 << 2));
	r->encp_spi_to_ahb_enable = v;
}

static inline uint32_t aon_pmu_ll_get_r5_encp_spi_to_ahb_enable(void) {
	aon_pmu_r5_t *r = (aon_pmu_r5_t*)(SOC_AON_PMU_REG_BASE + (0x5 << 2));
	return r->encp_spi_to_ahb_enable;
}

//reg r25:

static inline void aon_pmu_ll_set_r25(uint32_t v) {
	aon_pmu_r25_t *r = (aon_pmu_r25_t*)(SOC_AON_PMU_REG_BASE + (0x25 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r25(void) {
	aon_pmu_r25_t *r = (aon_pmu_r25_t*)(SOC_AON_PMU_REG_BASE + (0x25 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r25_reg3v_clk(uint32_t v) {
	aon_pmu_r25_t *r = (aon_pmu_r25_t*)(SOC_AON_PMU_REG_BASE + (0x25 << 2));
	r->reg3v_clk = v;
}

//reg r40:

static inline void aon_pmu_ll_set_r40(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r40(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r40_wake1_delay(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->wake1_delay = v;
}

static inline uint32_t aon_pmu_ll_get_r40_wake1_delay(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->wake1_delay;
}

static inline void aon_pmu_ll_set_r40_wake2_delay(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->wake2_delay = v;
}

static inline uint32_t aon_pmu_ll_get_r40_wake2_delay(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->wake2_delay;
}

static inline void aon_pmu_ll_set_r40_wake3_delay(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->wake3_delay = v;
}

static inline uint32_t aon_pmu_ll_get_r40_wake3_delay(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->wake3_delay;
}

static inline void aon_pmu_ll_set_r40_halt1_delay(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt1_delay = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt1_delay(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt1_delay;
}

static inline void aon_pmu_ll_set_r40_halt2_delay(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt2_delay = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt2_delay(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt2_delay;
}

static inline void aon_pmu_ll_set_r40_halt3_delay(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt3_delay = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt3_delay(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt3_delay;
}

static inline void aon_pmu_ll_set_r40_halt_volt(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_volt = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_volt(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_volt;
}

static inline void aon_pmu_ll_set_r40_halt_xtal(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_xtal = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_xtal(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_xtal;
}

static inline void aon_pmu_ll_set_r40_halt_core(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_core = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_core(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_core;
}

static inline void aon_pmu_ll_set_r40_halt_flash(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_flash = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_flash(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_flash;
}

static inline void aon_pmu_ll_set_r40_halt_rosc(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_rosc = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_rosc(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_rosc;
}

static inline void aon_pmu_ll_set_r40_halt_resten(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_resten = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_resten(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_resten;
}

static inline void aon_pmu_ll_set_r40_halt_isolat(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_isolat = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_isolat(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_isolat;
}

static inline void aon_pmu_ll_set_r40_halt_clkena(uint32_t v) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	r->halt_clkena = v;
}

static inline uint32_t aon_pmu_ll_get_r40_halt_clkena(void) {
	aon_pmu_r40_t *r = (aon_pmu_r40_t*)(SOC_AON_PMU_REG_BASE + (0x40 << 2));
	return r->halt_clkena;
}

//reg r41:

static inline void aon_pmu_ll_set_r41(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r41(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r41_lpo_config(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->lpo_config = v;
}

static inline uint32_t aon_pmu_ll_get_r41_lpo_config(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->lpo_config;
}

static inline void aon_pmu_ll_set_r41_flshsck_iocap(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->flshsck_iocap = v;
}

static inline uint32_t aon_pmu_ll_get_r41_flshsck_iocap(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->flshsck_iocap;
}

static inline void aon_pmu_ll_set_r41_wakeup_ena(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->wakeup_ena = v;
}

static inline uint32_t aon_pmu_ll_get_r41_wakeup_ena(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->wakeup_ena;
}

static inline void aon_pmu_ll_set_r41_gpio_int_clksel(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->gpio_int_clksel = v;
}

static inline uint32_t aon_pmu_ll_get_r41_gpio_int_clksel(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->gpio_int_clksel;
}

static inline void aon_pmu_ll_set_r41_xtal_sel(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->xtal_sel = v;
}

static inline uint32_t aon_pmu_ll_get_r41_xtal_sel(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->xtal_sel;
}

static inline void aon_pmu_ll_set_r41_gpio_func_ctrl_en(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->gpio_func_ctrl_en = v;
}

static inline uint32_t aon_pmu_ll_get_r41_gpio_func_ctrl_en(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->gpio_func_ctrl_en;
}

static inline void aon_pmu_ll_set_r41_sleep_wdt_disable(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->sleep_wdt_disable = v;
}

static inline uint32_t aon_pmu_ll_get_r41_sleep_wdt_disable(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->sleep_wdt_disable;
}

static inline void aon_pmu_ll_set_r41_wlp_switch_en(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->wlp_switch_en = v;
}

static inline uint32_t aon_pmu_ll_get_r41_wlp_switch_en(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->wlp_switch_en;
}

static inline void aon_pmu_ll_set_r41_lcd_clk_inv(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->lcd_clk_inv = v;
}

static inline uint32_t aon_pmu_ll_get_r41_lcd_clk_inv(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->lcd_clk_inv;
}

static inline void aon_pmu_ll_set_r41_halt_lpo(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->halt_lpo = v;
}

static inline uint32_t aon_pmu_ll_get_r41_halt_lpo(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->halt_lpo;
}

static inline void aon_pmu_ll_set_r41_halt_sram0(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->halt_sram0 = v;
}

static inline uint32_t aon_pmu_ll_get_r41_halt_sram0(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->halt_sram0;
}

static inline void aon_pmu_ll_set_r41_halt_sram1(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->halt_sram1 = v;
}

static inline uint32_t aon_pmu_ll_get_r41_halt_sram1(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->halt_sram1;
}

static inline void aon_pmu_ll_set_r41_halt_sram2(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->halt_sram2 = v;
}

static inline uint32_t aon_pmu_ll_get_r41_halt_sram2(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->halt_sram2;
}

static inline void aon_pmu_ll_set_r41_mem_ret_en(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->mem_ret_en = v;
}

static inline uint32_t aon_pmu_ll_get_r41_mem_ret_en(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->mem_ret_en;
}

static inline void aon_pmu_ll_set_r41_halt_cache(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->halt_cache = v;
}

static inline uint32_t aon_pmu_ll_get_r41_halt_cache(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->halt_cache;
}

static inline void aon_pmu_ll_set_r41_m52_l2_dis_cache_init_dis_maint(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->m52_l2_dis_cache_init_dis_maint = v;
}

static inline uint32_t aon_pmu_ll_get_r41_m52_l2_dis_cache_init_dis_maint(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->m52_l2_dis_cache_init_dis_maint;
}

static inline void aon_pmu_ll_set_r41_m52_cpu_l2_dis_cache_en_maint(uint32_t v) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	r->m52_cpu_l2_dis_cache_en_maint = v;
}

static inline uint32_t aon_pmu_ll_get_r41_m52_cpu_l2_dis_cache_en_maint(void) {
	aon_pmu_r41_t *r = (aon_pmu_r41_t*)(SOC_AON_PMU_REG_BASE + (0x41 << 2));
	return r->m52_cpu_l2_dis_cache_en_maint;
}

//reg r42:

static inline void aon_pmu_ll_set_r42_value(uint32_t v) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r42_value(void) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r42_pwd_blppwd(uint32_t v) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	r->pwd_blppwd = v;
}

static inline uint32_t aon_pmu_ll_get_r42_pwd_blppwd(void) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	return r->pwd_blppwd;
}

static inline void aon_pmu_ll_set_r42_pwd_wlppwd(uint32_t v) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	r->pwd_wlppwd = v;
}

static inline uint32_t aon_pmu_ll_get_r42_pwd_wlppwd(void) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	return r->pwd_wlppwd;
}

static inline uint32_t aon_pmu_ll_get_r42_blp_isolate_state(void) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	return r->blp_isolate_state;
}

static inline uint32_t aon_pmu_ll_get_r42_wlp_isolate_state(void) {
	aon_pmu_r42_t *r = (aon_pmu_r42_t*)(SOC_AON_PMU_REG_BASE + (0x42 << 2));
	return r->wlp_isolate_state;
}

//reg r43:

static inline void aon_pmu_ll_set_r43_value(uint32_t v) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r43_value(void) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	return r->v;
}

static inline void aon_pmu_ll_set_r43_clr_int_touched(uint32_t v) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	r->clr_int_touched = v;
}

static inline uint32_t aon_pmu_ll_get_r43_clr_int_touched(void) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	return r->clr_int_touched;
}

static inline void aon_pmu_ll_set_r43_clr_int_usbplug(uint32_t v) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	r->clr_int_usbplug = v;
}

static inline uint32_t aon_pmu_ll_get_r43_clr_int_usbplug(void) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	return r->clr_int_usbplug;
}

static inline void aon_pmu_ll_set_r43_clr_wakeup(uint32_t v) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	r->clr_wakeup = v;
}

static inline uint32_t aon_pmu_ll_get_r43_clr_wakeup(void) {
	aon_pmu_r43_t *r = (aon_pmu_r43_t*)(SOC_AON_PMU_REG_BASE + (0x43 << 2));
	return r->clr_wakeup;
}

//reg r70:

static inline void aon_pmu_ll_set_r70_value(uint32_t v) {
	aon_pmu_r70_t *r = (aon_pmu_r70_t*)(SOC_AON_PMU_REG_BASE + (0x70 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r70_value(void) {
	aon_pmu_r70_t *r = (aon_pmu_r70_t*)(SOC_AON_PMU_REG_BASE + (0x70 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r70_int_touched(void) {
	aon_pmu_r70_t *r = (aon_pmu_r70_t*)(SOC_AON_PMU_REG_BASE + (0x70 << 2));
	return r->int_touched;
}

static inline uint32_t aon_pmu_ll_get_r70_int_usbplug(void) {
	aon_pmu_r70_t *r = (aon_pmu_r70_t*)(SOC_AON_PMU_REG_BASE + (0x70 << 2));
	return r->int_usbplug;
}

//reg r71:

static inline void aon_pmu_ll_set_r71_value(uint32_t v) {
	aon_pmu_r71_t *r = (aon_pmu_r71_t*)(SOC_AON_PMU_REG_BASE + (0x71 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r71_value(void) {
	aon_pmu_r71_t *r = (aon_pmu_r71_t*)(SOC_AON_PMU_REG_BASE + (0x71 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r71_touch_state(void) {
	aon_pmu_r71_t *r = (aon_pmu_r71_t*)(SOC_AON_PMU_REG_BASE + (0x71 << 2));
	return r->touch_state;
}

static inline uint32_t aon_pmu_ll_get_r71_usbplug_state(void) {
	aon_pmu_r71_t *r = (aon_pmu_r71_t*)(SOC_AON_PMU_REG_BASE + (0x71 << 2));
	return r->usbplug_state;
}

static inline uint32_t aon_pmu_ll_get_r71_wakeup_source(void) {
	aon_pmu_r71_t *r = (aon_pmu_r71_t*)(SOC_AON_PMU_REG_BASE + (0x71 << 2));
	return r->wakeup_source;
}

//reg r72:

static inline uint32_t aon_pmu_ll_get_r72_value(void) {
	aon_pmu_r72_t *r = (aon_pmu_r72_t*)(SOC_AON_PMU_REG_BASE + (0x72 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r72_td_int_status(void) {
	aon_pmu_r72_t *r = (aon_pmu_r72_t*)(SOC_AON_PMU_REG_BASE + (0x72 << 2));
	return r->td_int_status;
}

//reg r73:

static inline uint32_t aon_pmu_ll_get_r73_value(void) {
	aon_pmu_r73_t *r = (aon_pmu_r73_t*)(SOC_AON_PMU_REG_BASE + (0x73 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r73_cap_cal_mode1(void) {
	aon_pmu_r73_t *r = (aon_pmu_r73_t*)(SOC_AON_PMU_REG_BASE + (0x73 << 2));
	return r->cap_cal_mode1;
}

static inline uint32_t aon_pmu_ll_get_r73_td_caldone_mode1(void) {
	aon_pmu_r73_t *r = (aon_pmu_r73_t*)(SOC_AON_PMU_REG_BASE + (0x73 << 2));
	return r->td_caldone_mode1;
}

//reg r74:

static inline void aon_pmu_ll_set_r74_value(uint32_t v) {
	aon_pmu_r74_t *r = (aon_pmu_r74_t*)(SOC_AON_PMU_REG_BASE + (0x74 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r74_value(void) {
	aon_pmu_r74_t *r = (aon_pmu_r74_t*)(SOC_AON_PMU_REG_BASE + (0x74 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r74_por_corehs_n(void) {
	aon_pmu_r74_t *r = (aon_pmu_r74_t*)(SOC_AON_PMU_REG_BASE + (0x74 << 2));
	return r->por_corehs_n;
}

//reg r75:

static inline void aon_pmu_ll_set_r75_value(uint32_t v) {
	aon_pmu_r75_t *r = (aon_pmu_r75_t*)(SOC_AON_PMU_REG_BASE + (0x75 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r75_value(void) {
	aon_pmu_r75_t *r = (aon_pmu_r75_t*)(SOC_AON_PMU_REG_BASE + (0x75 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r75_ana_sta4(void) {
	aon_pmu_r75_t *r = (aon_pmu_r75_t*)(SOC_AON_PMU_REG_BASE + (0x75 << 2));
	return r->ana_sta4;
}

//reg r78:

static inline void aon_pmu_ll_set_r78_value(uint32_t v) {
	aon_pmu_r78_t *r = (aon_pmu_r78_t*)(SOC_AON_PMU_REG_BASE + (0x78 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r78_value(void) {
	aon_pmu_r78_t *r = (aon_pmu_r78_t*)(SOC_AON_PMU_REG_BASE + (0x78 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r78_ana_rtc_state(void) {
	aon_pmu_r78_t *r = (aon_pmu_r78_t*)(SOC_AON_PMU_REG_BASE + (0x78 << 2));
	return r->ana_rtc_state;
}

//reg r79:

static inline void aon_pmu_ll_set_r79_value(uint32_t v) {
	aon_pmu_r79_t *r = (aon_pmu_r79_t*)(SOC_AON_PMU_REG_BASE + (0x79 << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r79_value(void) {
	aon_pmu_r79_t *r = (aon_pmu_r79_t*)(SOC_AON_PMU_REG_BASE + (0x79 << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r79_ana_rtc_value(void) {
	aon_pmu_r79_t *r = (aon_pmu_r79_t*)(SOC_AON_PMU_REG_BASE + (0x79 << 2));
	return r->ana_rtc_value;
}

//reg r7a:

static inline void aon_pmu_ll_set_r7a_value(uint32_t v) {
	aon_pmu_r7a_t *r = (aon_pmu_r7a_t*)(SOC_AON_PMU_REG_BASE + (0x7a << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r7a_value(void) {
	aon_pmu_r7a_t *r = (aon_pmu_r7a_t*)(SOC_AON_PMU_REG_BASE + (0x7a << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r7a_aon_mix0(void) {
	aon_pmu_r7a_t *r = (aon_pmu_r7a_t*)(SOC_AON_PMU_REG_BASE + (0x7a << 2));
	return r->aon_mix0;
}

//reg r7b:

static inline void aon_pmu_ll_set_r7b(uint32_t v) {
	aon_pmu_r7b_t *r = (aon_pmu_r7b_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r7b(void) {
	aon_pmu_r7b_t *r = (aon_pmu_r7b_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r7b_aon3v_regdi(void) {
	aon_pmu_r7b_t *r = (aon_pmu_r7b_t*)(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	return r->aon3v_regdi;
}

//reg r7c:

static inline void aon_pmu_ll_set_r7c_value(uint32_t v) {
	aon_pmu_r7c_t *r = (aon_pmu_r7c_t*)(SOC_AON_PMU_REG_BASE + (0x7c << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r7c_value(void) {
	aon_pmu_r7c_t *r = (aon_pmu_r7c_t*)(SOC_AON_PMU_REG_BASE + (0x7c << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r7c_id(void) {
	aon_pmu_r7c_t *r = (aon_pmu_r7c_t*)(SOC_AON_PMU_REG_BASE + (0x7c << 2));
	return r->id;
}

//reg r7d:

static inline void aon_pmu_ll_set_r7d_value(uint32_t v) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r7d_value(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r7d_lcal_dac(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->lcal_dac;
}

static inline uint32_t aon_pmu_ll_get_r7d_l(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->l;
}

static inline uint32_t aon_pmu_ll_get_r7d_adc_cal(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->adc_cal;
}

static inline uint32_t aon_pmu_ll_get_r7d_bgcal(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->bgcal;
}

static inline uint32_t aon_pmu_ll_get_r7d_sig_26mpll_unlock(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->sig_26mpll_unlock;
}

static inline uint32_t aon_pmu_ll_get_r7d_dpll_unlock_l(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->dpll_unlock_l;
}

static inline uint32_t aon_pmu_ll_get_r7d_dpll_unlock_h(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->dpll_unlock_h;
}

static inline uint32_t aon_pmu_ll_get_r7d_apll_unlock(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->apll_unlock;
}

static inline uint32_t aon_pmu_ll_get_r7d_btpll_unlock(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->btpll_unlock;
}

static inline uint32_t aon_pmu_ll_get_r7d_calfail_btpll(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->calfail_btpll;
}

static inline uint32_t aon_pmu_ll_get_r7d_dpll_band(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->dpll_band;
}

static inline uint32_t aon_pmu_ll_get_r7d_h(void) {
	aon_pmu_r7d_t *r = (aon_pmu_r7d_t*)(SOC_AON_PMU_REG_BASE + (0x7d << 2));
	return r->h;
}

//reg r7e:

static inline void aon_pmu_ll_set_r7e_value(uint32_t v) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r7e_value(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r7e_cbcal(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->cbcal;
}

static inline uint32_t aon_pmu_ll_get_r7e_ad_state(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->ad_state;
}

static inline uint32_t aon_pmu_ll_get_r7e_bandcal(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->bandcal;
}

static inline uint32_t aon_pmu_ll_get_r7e_cap_mod2_ls(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->cap_mod2_ls;
}

static inline uint32_t aon_pmu_ll_get_r7e_cap_mod2_hs(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->cap_mod2_hs;
}

static inline uint32_t aon_pmu_ll_get_r7e_h(void) {
	aon_pmu_r7e_t *r = (aon_pmu_r7e_t*)(SOC_AON_PMU_REG_BASE + (0x7e << 2));
	return r->h;
}

//reg r7f:

static inline void aon_pmu_ll_set_r7f_value(uint32_t v) {
	aon_pmu_r7f_t *r = (aon_pmu_r7f_t*)(SOC_AON_PMU_REG_BASE + (0x7f << 2));
	r->v = v;
}

static inline uint32_t aon_pmu_ll_get_r7f_value(void) {
	aon_pmu_r7f_t *r = (aon_pmu_r7f_t*)(SOC_AON_PMU_REG_BASE + (0x7f << 2));
	return r->v;
}

static inline uint32_t aon_pmu_ll_get_r7f_td_states2(void) {
	aon_pmu_r7f_t *r = (aon_pmu_r7f_t*)(SOC_AON_PMU_REG_BASE + (0x7f << 2));
	return r->td_states2;
}
#ifdef __cplusplus
}
#endif
