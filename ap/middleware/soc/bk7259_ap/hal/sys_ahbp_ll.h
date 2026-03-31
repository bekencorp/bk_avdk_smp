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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "sys_ahbp_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_AHBP_LL_REG_BASE   SOC_SYS_AHBP_REG_BASE

//reg reg0:

static inline void sys_ahbp_ll_set_reg0_value(uint32_t v) {
	sys_ahbp_reg0_t *r = (sys_ahbp_reg0_t*)(SOC_SYS_AHBP_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg0_value(void) {
	sys_ahbp_reg0_t *r = (sys_ahbp_reg0_t*)(SOC_SYS_AHBP_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg0_deviceid(void) {
	sys_ahbp_reg0_t *r = (sys_ahbp_reg0_t*)(SOC_SYS_AHBP_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg reg1:

static inline void sys_ahbp_ll_set_reg1_value(uint32_t v) {
	sys_ahbp_reg1_t *r = (sys_ahbp_reg1_t*)(SOC_SYS_AHBP_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1_value(void) {
	sys_ahbp_reg1_t *r = (sys_ahbp_reg1_t*)(SOC_SYS_AHBP_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1_versionid(void) {
	sys_ahbp_reg1_t *r = (sys_ahbp_reg1_t*)(SOC_SYS_AHBP_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg reg2:

static inline void sys_ahbp_ll_set_reg2_value(uint32_t v) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2_value(void) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg2_reserved_0_0(uint32_t v) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	r->reserved_0_0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2_reserved_0_0(void) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	return r->reserved_0_0;
}

static inline void sys_ahbp_ll_set_reg2_clkg_bypass(uint32_t v) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	r->clkg_bypass = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2_clkg_bypass(void) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	return r->clkg_bypass;
}

static inline void sys_ahbp_ll_set_reg2_reserved_2_31(uint32_t v) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	r->reserved_2_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2_reserved_2_31(void) {
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t*)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));
	return r->reserved_2_31;
}

//reg reg3:

static inline void sys_ahbp_ll_set_reg3_value(uint32_t v) {
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t*)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3_value(void) {
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t*)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg3_cpu0_sleeping(void) {
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t*)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));
	return r->cpu0_sleeping;
}

static inline uint32_t sys_ahbp_ll_get_reg3_cpu0_deepsleep(void) {
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t*)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));
	return r->cpu0_deepsleep;
}

static inline uint32_t sys_ahbp_ll_get_reg3_cpu1_sleeping(void) {
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t*)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));
	return r->cpu1_sleeping;
}

static inline uint32_t sys_ahbp_ll_get_reg3_cpu1_deepsleep(void) {
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t*)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));
	return r->cpu1_deepsleep;
}

//reg reg4:

static inline void sys_ahbp_ll_set_reg4_value(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_value(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_sw_rstn(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_sw_rstn = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_sw_rstn(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_sw_rstn;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_init_dtcm_en(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_init_dtcm_en = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_init_dtcm_en(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_init_dtcm_en;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_sys_nmi(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_sys_nmi = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_sys_nmi(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_sys_nmi;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_wfe_src(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_wfe_src = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_wfe_src(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_wfe_src;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_wfe_pulse(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_wfe_pulse = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_wfe_pulse(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_wfe_pulse;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_wait(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_wait = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_wait(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_wait;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_dbg_rstn_disable(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_dbg_rstn_disable = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_dbg_rstn_disable(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_dbg_rstn_disable;
}

static inline void sys_ahbp_ll_set_reg4_reserved_7_7(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_reserved_7_7(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->reserved_7_7;
}

static inline void sys_ahbp_ll_set_reg4_cpu0_offset(uint32_t v) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	r->cpu0_offset = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4_cpu0_offset(void) {
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t*)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));
	return r->cpu0_offset;
}

//reg reg5:

static inline void sys_ahbp_ll_set_reg5_value(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_value(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_sw_rstn(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_sw_rstn = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_sw_rstn(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_sw_rstn;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_init_dtcm_en(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_init_dtcm_en = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_init_dtcm_en(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_init_dtcm_en;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_sys_nmi(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_sys_nmi = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_sys_nmi(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_sys_nmi;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_wfe_src(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_wfe_src = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_wfe_src(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_wfe_src;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_wfe_pulse(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_wfe_pulse = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_wfe_pulse(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_wfe_pulse;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_wait(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_wait = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_wait(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_wait;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_dbg_rstn_disable(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_dbg_rstn_disable = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_dbg_rstn_disable(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_dbg_rstn_disable;
}

static inline void sys_ahbp_ll_set_reg5_reserved_7_7(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_reserved_7_7(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->reserved_7_7;
}

static inline void sys_ahbp_ll_set_reg5_cpu1_offset(uint32_t v) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	r->cpu1_offset = v;
}

static inline uint32_t sys_ahbp_ll_get_reg5_cpu1_offset(void) {
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t*)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));
	return r->cpu1_offset;
}

//reg reg6:

static inline void sys_ahbp_ll_set_reg6_value(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_value(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg6_npu_sw_rstn(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->npu_sw_rstn = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_npu_sw_rstn(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->npu_sw_rstn;
}

static inline void sys_ahbp_ll_set_reg6_npu_clkbps(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->npu_clkbps = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_npu_clkbps(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->npu_clkbps;
}

static inline void sys_ahbp_ll_set_reg6_axi0_awcache(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->axi0_awcache = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_axi0_awcache(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->axi0_awcache;
}

static inline void sys_ahbp_ll_set_reg6_axi0_arcache(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->axi0_arcache = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_axi0_arcache(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->axi0_arcache;
}

static inline void sys_ahbp_ll_set_reg6_axi1_awcache(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->axi1_awcache = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_axi1_awcache(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->axi1_awcache;
}

static inline void sys_ahbp_ll_set_reg6_axi1_arcache(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->axi1_arcache = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_axi1_arcache(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->axi1_arcache;
}

static inline void sys_ahbp_ll_set_reg6_cache_src(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->cache_src = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_cache_src(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->cache_src;
}

static inline void sys_ahbp_ll_set_reg6_reserved_19_31(uint32_t v) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	r->reserved_19_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg6_reserved_19_31(void) {
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t*)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));
	return r->reserved_19_31;
}

//reg reg7:

static inline void sys_ahbp_ll_set_reg7_value(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_value(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg7_psram_inv_config(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->psram_inv_config = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_psram_inv_config(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->psram_inv_config;
}

static inline void sys_ahbp_ll_set_reg7_videopost_m_arcache(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->videopost_m_arcache = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_videopost_m_arcache(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->videopost_m_arcache;
}

static inline void sys_ahbp_ll_set_reg7_videopost_m_awcache(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->videopost_m_awcache = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_videopost_m_awcache(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->videopost_m_awcache;
}

static inline void sys_ahbp_ll_set_reg7_reserved_10_15(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->reserved_10_15 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_reserved_10_15(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->reserved_10_15;
}

static inline void sys_ahbp_ll_set_reg7_icache_clean_mode(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->icache_clean_mode = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_icache_clean_mode(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->icache_clean_mode;
}

static inline void sys_ahbp_ll_set_reg7_cpu0_icache_clean_mode(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->cpu0_icache_clean_mode = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_cpu0_icache_clean_mode(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->cpu0_icache_clean_mode;
}

static inline void sys_ahbp_ll_set_reg7_cpu0_icache_clean_tag_sel(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->cpu0_icache_clean_tag_sel = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_cpu0_icache_clean_tag_sel(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->cpu0_icache_clean_tag_sel;
}

static inline void sys_ahbp_ll_set_reg7_cpu1_icache_clean_mode(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->cpu1_icache_clean_mode = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_cpu1_icache_clean_mode(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->cpu1_icache_clean_mode;
}

static inline void sys_ahbp_ll_set_reg7_cpu1_icache_clean_tag_sel(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->cpu1_icache_clean_tag_sel = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_cpu1_icache_clean_tag_sel(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->cpu1_icache_clean_tag_sel;
}

static inline void sys_ahbp_ll_set_reg7_reserved_21_23(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->reserved_21_23 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_reserved_21_23(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->reserved_21_23;
}

static inline void sys_ahbp_ll_set_reg7_icache_clean_key(uint32_t v) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	r->icache_clean_key = v;
}

static inline uint32_t sys_ahbp_ll_get_reg7_icache_clean_key(void) {
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t*)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));
	return r->icache_clean_key;
}

//reg reg8:

static inline void sys_ahbp_ll_set_reg8_value(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_value(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg8_cksel_core(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->cksel_core = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_cksel_core(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->cksel_core;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_core(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_core = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_core(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_core;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_bus_ls(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_bus_ls = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_bus_ls(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_bus_ls;
}

static inline void sys_ahbp_ll_set_reg8_reserved_5_5(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->reserved_5_5 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_reserved_5_5(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->reserved_5_5;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_uart5(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_uart5 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_uart5(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_uart5;
}

static inline void sys_ahbp_ll_set_reg8_cksel_qspi0(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->cksel_qspi0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_cksel_qspi0(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->cksel_qspi0;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_qspi0(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_qspi0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_qspi0(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_qspi0;
}

static inline void sys_ahbp_ll_set_reg8_cksel_qspi1(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->cksel_qspi1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_cksel_qspi1(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->cksel_qspi1;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_qspi1(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_qspi1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_qspi1(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_qspi1;
}

static inline void sys_ahbp_ll_set_reg8_cksel_pram0(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->cksel_pram0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_cksel_pram0(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->cksel_pram0;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_pram0(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_pram0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_pram0(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_pram0;
}

static inline void sys_ahbp_ll_set_reg8_cksel_mbist(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->cksel_mbist = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_cksel_mbist(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->cksel_mbist;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_sdio0(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_sdio0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_sdio0(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_sdio0;
}

static inline void sys_ahbp_ll_set_reg8_ckdiv_sdio1(uint32_t v) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	r->ckdiv_sdio1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg8_ckdiv_sdio1(void) {
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t*)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));
	return r->ckdiv_sdio1;
}

//reg reg9:

static inline void sys_ahbp_ll_set_reg9_value(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_value(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg9_cksel_cis_mclk(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_cis_mclk = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_cis_mclk(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_cis_mclk;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_cis_mclk(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_cis_mclk = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_cis_mclk(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_cis_mclk;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_cis_auxs(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_cis_auxs = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_cis_auxs(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_cis_auxs;
}

static inline void sys_ahbp_ll_set_reg9_cksel_cisp(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_cisp = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_cisp(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_cisp;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_cisp(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_cisp = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_cisp(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_cisp;
}

static inline void sys_ahbp_ll_set_reg9_cksel_gpu(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_gpu = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_gpu(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_gpu;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_gpu(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_gpu = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_gpu(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_gpu;
}

static inline void sys_ahbp_ll_set_reg9_cksel_h265(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_h265 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_h265(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_h265;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_h265(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_h265 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_h265(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_h265;
}

static inline void sys_ahbp_ll_set_reg9_cksel_dpu(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_dpu = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_dpu(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_dpu;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_dpu(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_dpu = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_dpu(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_dpu;
}

static inline void sys_ahbp_ll_set_reg9_cksel_pram1(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_pram1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_pram1(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_pram1;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_pram1(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_pram1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_pram1(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_pram1;
}

static inline void sys_ahbp_ll_set_reg9_ckdiv_trace(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->ckdiv_trace = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_ckdiv_trace(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->ckdiv_trace;
}

static inline void sys_ahbp_ll_set_reg9_cksel_cis_auxs(uint32_t v) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	r->cksel_cis_auxs = v;
}

static inline uint32_t sys_ahbp_ll_get_reg9_cksel_cis_auxs(void) {
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t*)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));
	return r->cksel_cis_auxs;
}

//reg rega:

static inline void sys_ahbp_ll_set_rega_value(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_value(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_rega_cpua_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->cpua_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_cpua_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->cpua_cken;
}

static inline void sys_ahbp_ll_set_rega_uart5_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->uart5_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_uart5_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->uart5_cken;
}

static inline void sys_ahbp_ll_set_rega_usb_hs_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->usb_hs_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_usb_hs_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->usb_hs_cken;
}

static inline void sys_ahbp_ll_set_rega_pram0_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->pram0_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_pram0_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->pram0_cken;
}

static inline void sys_ahbp_ll_set_rega_pram1_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->pram1_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_pram1_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->pram1_cken;
}

static inline void sys_ahbp_ll_set_rega_qspi0_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->qspi0_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_qspi0_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->qspi0_cken;
}

static inline void sys_ahbp_ll_set_rega_qspi1_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->qspi1_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_qspi1_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->qspi1_cken;
}

static inline void sys_ahbp_ll_set_rega_sdio0_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->sdio0_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_sdio0_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->sdio0_cken;
}

static inline void sys_ahbp_ll_set_rega_sdio1_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->sdio1_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_sdio1_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->sdio1_cken;
}

static inline void sys_ahbp_ll_set_rega_cisp_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->cisp_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_cisp_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->cisp_cken;
}

static inline void sys_ahbp_ll_set_rega_gpu_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->gpu_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_gpu_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->gpu_cken;
}

static inline void sys_ahbp_ll_set_rega_h26e_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->h26e_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_h26e_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->h26e_cken;
}

static inline void sys_ahbp_ll_set_rega_csi_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->csi_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_csi_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->csi_cken;
}

static inline void sys_ahbp_ll_set_rega_dsi_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->dsi_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_dsi_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->dsi_cken;
}

static inline void sys_ahbp_ll_set_rega_dpu_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->dpu_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_dpu_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->dpu_cken;
}

static inline void sys_ahbp_ll_set_rega_usb_fs_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->usb_fs_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_usb_fs_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->usb_fs_cken;
}

static inline void sys_ahbp_ll_set_rega_conf0_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->conf0_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_conf0_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->conf0_cken;
}

static inline void sys_ahbp_ll_set_rega_conf1_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->conf1_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_conf1_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->conf1_cken;
}

static inline void sys_ahbp_ll_set_rega_conf2_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->conf2_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_conf2_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->conf2_cken;
}

static inline void sys_ahbp_ll_set_rega_conf3_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->conf3_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_conf3_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->conf3_cken;
}

static inline void sys_ahbp_ll_set_rega_cpu0_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->cpu0_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_cpu0_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->cpu0_cken;
}

static inline void sys_ahbp_ll_set_rega_cpu1_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->cpu1_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_cpu1_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->cpu1_cken;
}

static inline void sys_ahbp_ll_set_rega_npu_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->npu_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_npu_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->npu_cken;
}

static inline void sys_ahbp_ll_set_rega_timer4_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->timer4_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_timer4_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->timer4_cken;
}

static inline void sys_ahbp_ll_set_rega_timer5_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->timer5_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_timer5_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->timer5_cken;
}

static inline void sys_ahbp_ll_set_rega_trace_cken(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->trace_cken = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_trace_cken(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->trace_cken;
}

static inline void sys_ahbp_ll_set_rega_reserved_26_31(uint32_t v) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	r->reserved_26_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_rega_reserved_26_31(void) {
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t*)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));
	return r->reserved_26_31;
}

//reg regb:

static inline void sys_ahbp_ll_set_regb_value(uint32_t v) {
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t*)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_regb_value(void) {
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t*)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_regb_phase_ck640(uint32_t v) {
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t*)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));
	r->phase_ck640 = v;
}

static inline uint32_t sys_ahbp_ll_get_regb_phase_ck640(void) {
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t*)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));
	return r->phase_ck640;
}

static inline void sys_ahbp_ll_set_regb_reserved_8_31(uint32_t v) {
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t*)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));
	r->reserved_8_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_regb_reserved_8_31(void) {
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t*)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));
	return r->reserved_8_31;
}

//reg regc:

static inline void sys_ahbp_ll_set_regc_value(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_value(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_regc_cpu0_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->cpu0_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_cpu0_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->cpu0_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_cpu1_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->cpu1_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_cpu1_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->cpu1_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_dtcm_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->dtcm_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_dtcm_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->dtcm_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_l2ch_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->l2ch_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_l2ch_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->l2ch_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_mem3_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->mem3_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_mem3_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->mem3_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_mem4_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->mem4_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_mem4_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->mem4_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_mem5_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->mem5_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_mem5_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->mem5_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_mem6_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->mem6_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_mem6_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->mem6_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_ahbp_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->ahbp_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_ahbp_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->ahbp_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_h265_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->h265_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_h265_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->h265_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_gpub_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->gpub_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_gpub_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->gpub_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_dpub_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->dpub_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_dpub_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->dpub_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_disb_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->disb_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_disb_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->disb_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_h264_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->h264_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_h264_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->h264_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_ispb_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->ispb_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_ispb_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->ispb_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_csib_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->csib_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_csib_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->csib_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_npub_mem_sd(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->npub_mem_sd = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_npub_mem_sd(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->npub_mem_sd;
}

static inline void sys_ahbp_ll_set_regc_reserved_17_31(uint32_t v) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	r->reserved_17_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_regc_reserved_17_31(void) {
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t*)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));
	return r->reserved_17_31;
}

//reg regd:

static inline void sys_ahbp_ll_set_regd_value(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_value(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_regd_cpu0_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->cpu0_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_cpu0_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->cpu0_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_cpu1_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->cpu1_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_cpu1_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->cpu1_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_dtcm_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->dtcm_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_dtcm_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->dtcm_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_l2ch_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->l2ch_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_l2ch_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->l2ch_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_mem3_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->mem3_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_mem3_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->mem3_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_mem4_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->mem4_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_mem4_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->mem4_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_mem5_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->mem5_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_mem5_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->mem5_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_mem6_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->mem6_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_mem6_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->mem6_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_ahbp_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->ahbp_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_ahbp_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->ahbp_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_h265_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->h265_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_h265_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->h265_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_gpub_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->gpub_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_gpub_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->gpub_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_dpub_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->dpub_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_dpub_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->dpub_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_disb_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->disb_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_disb_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->disb_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_h264_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->h264_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_h264_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->h264_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_ispb_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->ispb_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_ispb_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->ispb_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_csib_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->csib_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_csib_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->csib_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_npub_mem_ds(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->npub_mem_ds = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_npub_mem_ds(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->npub_mem_ds;
}

static inline void sys_ahbp_ll_set_regd_reserved_17_31(uint32_t v) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	r->reserved_17_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_regd_reserved_17_31(void) {
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t*)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));
	return r->reserved_17_31;
}

//reg rege:

static inline void sys_ahbp_ll_set_rege_value(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_value(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_rege_system_halt_en(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->system_halt_en = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_system_halt_en(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->system_halt_en;
}

static inline void sys_ahbp_ll_set_rege_system_halt_high_cpu0wfi(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->system_halt_high_cpu0wfi = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_system_halt_high_cpu0wfi(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->system_halt_high_cpu0wfi;
}

static inline void sys_ahbp_ll_set_rege_system_halt_high_cpu1wfi(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->system_halt_high_cpu1wfi = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_system_halt_high_cpu1wfi(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->system_halt_high_cpu1wfi;
}

static inline void sys_ahbp_ll_set_rege_reserved_3_7(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->reserved_3_7 = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_reserved_3_7(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->reserved_3_7;
}

static inline void sys_ahbp_ll_set_rege_cpu_halt_en(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->cpu_halt_en = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_cpu_halt_en(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->cpu_halt_en;
}

static inline void sys_ahbp_ll_set_rege_cpu_halt_high_cpu0wfi(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->cpu_halt_high_cpu0wfi = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_cpu_halt_high_cpu0wfi(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->cpu_halt_high_cpu0wfi;
}

static inline void sys_ahbp_ll_set_rege_cpu_halt_high_cpu1wfi(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->cpu_halt_high_cpu1wfi = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_cpu_halt_high_cpu1wfi(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->cpu_halt_high_cpu1wfi;
}

static inline void sys_ahbp_ll_set_rege_reserved_11_15(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->reserved_11_15 = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_reserved_11_15(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->reserved_11_15;
}

static inline void sys_ahbp_ll_set_rege_pwd_m55(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->pwd_m55 = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_pwd_m55(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->pwd_m55;
}

static inline void sys_ahbp_ll_set_rege_pwd_video_post(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->pwd_video_post = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_pwd_video_post(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->pwd_video_post;
}

static inline void sys_ahbp_ll_set_rege_pwd_h26e(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->pwd_h26e = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_pwd_h26e(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->pwd_h26e;
}

static inline void sys_ahbp_ll_set_rege_pwd_isp(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->pwd_isp = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_pwd_isp(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->pwd_isp;
}

static inline void sys_ahbp_ll_set_rege_pwd_npu(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->pwd_npu = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_pwd_npu(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->pwd_npu;
}

static inline void sys_ahbp_ll_set_rege_reserved_21_23(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->reserved_21_23 = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_reserved_21_23(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->reserved_21_23;
}

static inline void sys_ahbp_ll_set_rege_soft_rstn_isp  (uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->soft_rstn_isp   = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_soft_rstn_isp  (void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->soft_rstn_isp  ;
}

static inline void sys_ahbp_ll_set_rege_soft_rstn_h264e(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->soft_rstn_h264e = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_soft_rstn_h264e(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->soft_rstn_h264e;
}

static inline void sys_ahbp_ll_set_rege_soft_rstn_h264d(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->soft_rstn_h264d = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_soft_rstn_h264d(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->soft_rstn_h264d;
}

static inline void sys_ahbp_ll_set_rege_soft_rstn_gpu  (uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->soft_rstn_gpu   = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_soft_rstn_gpu  (void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->soft_rstn_gpu  ;
}

static inline void sys_ahbp_ll_set_rege_soft_rstn_dpu  (uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->soft_rstn_dpu   = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_soft_rstn_dpu  (void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->soft_rstn_dpu  ;
}

static inline void sys_ahbp_ll_set_rege_reserved_29_31(uint32_t v) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	r->reserved_29_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_rege_reserved_29_31(void) {
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t*)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));
	return r->reserved_29_31;
}

//reg regf:

static inline void sys_ahbp_ll_set_regf_value(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_value(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_wwdt(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_wwdt = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_wwdt(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_wwdt;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_sys_cfg(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_sys_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_sys_cfg(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_sys_cfg;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_busx(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_busx = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_busx(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_busx;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_cpu0(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_cpu0 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_cpu0(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_cpu0;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_cpu1(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_cpu1 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_cpu1(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_cpu1;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_ahbp(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_ahbp = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_ahbp(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_ahbp;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_smem3(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_smem3 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_smem3(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_smem3;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_smem4(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_smem4 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_smem4(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_smem4;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_smem5(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_smem5 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_smem5(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_smem5;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_smem6(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_smem6 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_smem6(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_smem6;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_npu(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_npu = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_npu(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_npu;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_h26e(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_h26e = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_h26e(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_h26e;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_isp(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_isp = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_isp(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_isp;
}

static inline void sys_ahbp_ll_set_regf_wwdt_region_video_post(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->wwdt_region_video_post = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_wwdt_region_video_post(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->wwdt_region_video_post;
}

static inline void sys_ahbp_ll_set_regf_reserved_14_31(uint32_t v) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	r->reserved_14_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_regf_reserved_14_31(void) {
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t*)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));
	return r->reserved_14_31;
}

//reg reg10:

static inline void sys_ahbp_ll_set_reg10_value(uint32_t v) {
	sys_ahbp_reg10_t *r = (sys_ahbp_reg10_t*)(SOC_SYS_AHBP_REG_BASE + (0x10 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg10_value(void) {
	sys_ahbp_reg10_t *r = (sys_ahbp_reg10_t*)(SOC_SYS_AHBP_REG_BASE + (0x10 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg10_ints_config_m55a_0(uint32_t v) {
	sys_ahbp_reg10_t *r = (sys_ahbp_reg10_t*)(SOC_SYS_AHBP_REG_BASE + (0x10 << 2));
	r->ints_config_m55a_0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg10_ints_config_m55a_0(void) {
	sys_ahbp_reg10_t *r = (sys_ahbp_reg10_t*)(SOC_SYS_AHBP_REG_BASE + (0x10 << 2));
	return r->ints_config_m55a_0;
}

//reg reg11:

static inline void sys_ahbp_ll_set_reg11_value(uint32_t v) {
	sys_ahbp_reg11_t *r = (sys_ahbp_reg11_t*)(SOC_SYS_AHBP_REG_BASE + (0x11 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg11_value(void) {
	sys_ahbp_reg11_t *r = (sys_ahbp_reg11_t*)(SOC_SYS_AHBP_REG_BASE + (0x11 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg11_ints_config_m55a_1(uint32_t v) {
	sys_ahbp_reg11_t *r = (sys_ahbp_reg11_t*)(SOC_SYS_AHBP_REG_BASE + (0x11 << 2));
	r->ints_config_m55a_1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg11_ints_config_m55a_1(void) {
	sys_ahbp_reg11_t *r = (sys_ahbp_reg11_t*)(SOC_SYS_AHBP_REG_BASE + (0x11 << 2));
	return r->ints_config_m55a_1;
}

//reg reg12:

static inline void sys_ahbp_ll_set_reg12_value(uint32_t v) {
	sys_ahbp_reg12_t *r = (sys_ahbp_reg12_t*)(SOC_SYS_AHBP_REG_BASE + (0x12 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg12_value(void) {
	sys_ahbp_reg12_t *r = (sys_ahbp_reg12_t*)(SOC_SYS_AHBP_REG_BASE + (0x12 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg12_ints_config_m55b_0(uint32_t v) {
	sys_ahbp_reg12_t *r = (sys_ahbp_reg12_t*)(SOC_SYS_AHBP_REG_BASE + (0x12 << 2));
	r->ints_config_m55b_0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg12_ints_config_m55b_0(void) {
	sys_ahbp_reg12_t *r = (sys_ahbp_reg12_t*)(SOC_SYS_AHBP_REG_BASE + (0x12 << 2));
	return r->ints_config_m55b_0;
}

//reg reg13:

static inline void sys_ahbp_ll_set_reg13_value(uint32_t v) {
	sys_ahbp_reg13_t *r = (sys_ahbp_reg13_t*)(SOC_SYS_AHBP_REG_BASE + (0x13 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg13_value(void) {
	sys_ahbp_reg13_t *r = (sys_ahbp_reg13_t*)(SOC_SYS_AHBP_REG_BASE + (0x13 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg13_ints_config_m55b_1(uint32_t v) {
	sys_ahbp_reg13_t *r = (sys_ahbp_reg13_t*)(SOC_SYS_AHBP_REG_BASE + (0x13 << 2));
	r->ints_config_m55b_1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg13_ints_config_m55b_1(void) {
	sys_ahbp_reg13_t *r = (sys_ahbp_reg13_t*)(SOC_SYS_AHBP_REG_BASE + (0x13 << 2));
	return r->ints_config_m55b_1;
}

//reg reg14:

static inline void sys_ahbp_ll_set_reg14_value(uint32_t v) {
	sys_ahbp_reg14_t *r = (sys_ahbp_reg14_t*)(SOC_SYS_AHBP_REG_BASE + (0x14 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg14_value(void) {
	sys_ahbp_reg14_t *r = (sys_ahbp_reg14_t*)(SOC_SYS_AHBP_REG_BASE + (0x14 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg14_ints_config_m52s_0(uint32_t v) {
	sys_ahbp_reg14_t *r = (sys_ahbp_reg14_t*)(SOC_SYS_AHBP_REG_BASE + (0x14 << 2));
	r->ints_config_m52s_0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg14_ints_config_m52s_0(void) {
	sys_ahbp_reg14_t *r = (sys_ahbp_reg14_t*)(SOC_SYS_AHBP_REG_BASE + (0x14 << 2));
	return r->ints_config_m52s_0;
}

//reg reg15:

static inline void sys_ahbp_ll_set_reg15_value(uint32_t v) {
	sys_ahbp_reg15_t *r = (sys_ahbp_reg15_t*)(SOC_SYS_AHBP_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg15_value(void) {
	sys_ahbp_reg15_t *r = (sys_ahbp_reg15_t*)(SOC_SYS_AHBP_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg15_ints_config_m52s_1(uint32_t v) {
	sys_ahbp_reg15_t *r = (sys_ahbp_reg15_t*)(SOC_SYS_AHBP_REG_BASE + (0x15 << 2));
	r->ints_config_m52s_1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg15_ints_config_m52s_1(void) {
	sys_ahbp_reg15_t *r = (sys_ahbp_reg15_t*)(SOC_SYS_AHBP_REG_BASE + (0x15 << 2));
	return r->ints_config_m52s_1;
}

//reg reg16:

static inline void sys_ahbp_ll_set_reg16_value(uint32_t v) {
	sys_ahbp_reg16_t *r = (sys_ahbp_reg16_t*)(SOC_SYS_AHBP_REG_BASE + (0x16 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg16_value(void) {
	sys_ahbp_reg16_t *r = (sys_ahbp_reg16_t*)(SOC_SYS_AHBP_REG_BASE + (0x16 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg16_ints_config_scr1_0(uint32_t v) {
	sys_ahbp_reg16_t *r = (sys_ahbp_reg16_t*)(SOC_SYS_AHBP_REG_BASE + (0x16 << 2));
	r->ints_config_scr1_0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg16_ints_config_scr1_0(void) {
	sys_ahbp_reg16_t *r = (sys_ahbp_reg16_t*)(SOC_SYS_AHBP_REG_BASE + (0x16 << 2));
	return r->ints_config_scr1_0;
}

//reg reg17:

static inline void sys_ahbp_ll_set_reg17_value(uint32_t v) {
	sys_ahbp_reg17_t *r = (sys_ahbp_reg17_t*)(SOC_SYS_AHBP_REG_BASE + (0x17 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg17_value(void) {
	sys_ahbp_reg17_t *r = (sys_ahbp_reg17_t*)(SOC_SYS_AHBP_REG_BASE + (0x17 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg17_ints_config_scr1_1(uint32_t v) {
	sys_ahbp_reg17_t *r = (sys_ahbp_reg17_t*)(SOC_SYS_AHBP_REG_BASE + (0x17 << 2));
	r->ints_config_scr1_1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg17_ints_config_scr1_1(void) {
	sys_ahbp_reg17_t *r = (sys_ahbp_reg17_t*)(SOC_SYS_AHBP_REG_BASE + (0x17 << 2));
	return r->ints_config_scr1_1;
}

//reg reg18:

static inline void sys_ahbp_ll_set_reg18_value(uint32_t v) {
	sys_ahbp_reg18_t *r = (sys_ahbp_reg18_t*)(SOC_SYS_AHBP_REG_BASE + (0x18 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg18_value(void) {
	sys_ahbp_reg18_t *r = (sys_ahbp_reg18_t*)(SOC_SYS_AHBP_REG_BASE + (0x18 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg18_ints_status_m55a_0(void) {
	sys_ahbp_reg18_t *r = (sys_ahbp_reg18_t*)(SOC_SYS_AHBP_REG_BASE + (0x18 << 2));
	return r->ints_status_m55a_0;
}

//reg reg19:

static inline void sys_ahbp_ll_set_reg19_value(uint32_t v) {
	sys_ahbp_reg19_t *r = (sys_ahbp_reg19_t*)(SOC_SYS_AHBP_REG_BASE + (0x19 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg19_value(void) {
	sys_ahbp_reg19_t *r = (sys_ahbp_reg19_t*)(SOC_SYS_AHBP_REG_BASE + (0x19 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg19_ints_status_m55a_1(void) {
	sys_ahbp_reg19_t *r = (sys_ahbp_reg19_t*)(SOC_SYS_AHBP_REG_BASE + (0x19 << 2));
	return r->ints_status_m55a_1;
}

//reg reg1a:

static inline void sys_ahbp_ll_set_reg1a_value(uint32_t v) {
	sys_ahbp_reg1a_t *r = (sys_ahbp_reg1a_t*)(SOC_SYS_AHBP_REG_BASE + (0x1a << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1a_value(void) {
	sys_ahbp_reg1a_t *r = (sys_ahbp_reg1a_t*)(SOC_SYS_AHBP_REG_BASE + (0x1a << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1a_ints_status_m55b_0(void) {
	sys_ahbp_reg1a_t *r = (sys_ahbp_reg1a_t*)(SOC_SYS_AHBP_REG_BASE + (0x1a << 2));
	return r->ints_status_m55b_0;
}

//reg reg1b:

static inline void sys_ahbp_ll_set_reg1b_value(uint32_t v) {
	sys_ahbp_reg1b_t *r = (sys_ahbp_reg1b_t*)(SOC_SYS_AHBP_REG_BASE + (0x1b << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1b_value(void) {
	sys_ahbp_reg1b_t *r = (sys_ahbp_reg1b_t*)(SOC_SYS_AHBP_REG_BASE + (0x1b << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1b_ints_status_m55b_1(void) {
	sys_ahbp_reg1b_t *r = (sys_ahbp_reg1b_t*)(SOC_SYS_AHBP_REG_BASE + (0x1b << 2));
	return r->ints_status_m55b_1;
}

//reg reg1c:

static inline void sys_ahbp_ll_set_reg1c_value(uint32_t v) {
	sys_ahbp_reg1c_t *r = (sys_ahbp_reg1c_t*)(SOC_SYS_AHBP_REG_BASE + (0x1c << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1c_value(void) {
	sys_ahbp_reg1c_t *r = (sys_ahbp_reg1c_t*)(SOC_SYS_AHBP_REG_BASE + (0x1c << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1c_ints_status_m52s_0(void) {
	sys_ahbp_reg1c_t *r = (sys_ahbp_reg1c_t*)(SOC_SYS_AHBP_REG_BASE + (0x1c << 2));
	return r->ints_status_m52s_0;
}

//reg reg1d:

static inline void sys_ahbp_ll_set_reg1d_value(uint32_t v) {
	sys_ahbp_reg1d_t *r = (sys_ahbp_reg1d_t*)(SOC_SYS_AHBP_REG_BASE + (0x1d << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1d_value(void) {
	sys_ahbp_reg1d_t *r = (sys_ahbp_reg1d_t*)(SOC_SYS_AHBP_REG_BASE + (0x1d << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1d_ints_status_m52s_1(void) {
	sys_ahbp_reg1d_t *r = (sys_ahbp_reg1d_t*)(SOC_SYS_AHBP_REG_BASE + (0x1d << 2));
	return r->ints_status_m52s_1;
}

//reg reg1e:

static inline void sys_ahbp_ll_set_reg1e_value(uint32_t v) {
	sys_ahbp_reg1e_t *r = (sys_ahbp_reg1e_t*)(SOC_SYS_AHBP_REG_BASE + (0x1e << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1e_value(void) {
	sys_ahbp_reg1e_t *r = (sys_ahbp_reg1e_t*)(SOC_SYS_AHBP_REG_BASE + (0x1e << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1e_ints_status_scr1_0(void) {
	sys_ahbp_reg1e_t *r = (sys_ahbp_reg1e_t*)(SOC_SYS_AHBP_REG_BASE + (0x1e << 2));
	return r->ints_status_scr1_0;
}

//reg reg1f:

static inline void sys_ahbp_ll_set_reg1f_value(uint32_t v) {
	sys_ahbp_reg1f_t *r = (sys_ahbp_reg1f_t*)(SOC_SYS_AHBP_REG_BASE + (0x1f << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg1f_value(void) {
	sys_ahbp_reg1f_t *r = (sys_ahbp_reg1f_t*)(SOC_SYS_AHBP_REG_BASE + (0x1f << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg1f_ints_status_scr1_1(void) {
	sys_ahbp_reg1f_t *r = (sys_ahbp_reg1f_t*)(SOC_SYS_AHBP_REG_BASE + (0x1f << 2));
	return r->ints_status_scr1_1;
}

//reg reg20:

static inline void sys_ahbp_ll_set_reg20_value(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_value(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg20_cpu0_qos     (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->cpu0_qos      = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_cpu0_qos     (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->cpu0_qos     ;
}

static inline void sys_ahbp_ll_set_reg20_dma1_qos     (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->dma1_qos      = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_dma1_qos     (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->dma1_qos     ;
}

static inline void sys_ahbp_ll_set_reg20_sdio0_qos    (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->sdio0_qos     = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_sdio0_qos    (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->sdio0_qos    ;
}

static inline void sys_ahbp_ll_set_reg20_sdio1_qos    (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->sdio1_qos     = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_sdio1_qos    (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->sdio1_qos    ;
}

static inline void sys_ahbp_ll_set_reg20_enet0_qos    (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->enet0_qos     = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_enet0_qos    (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->enet0_qos    ;
}

static inline void sys_ahbp_ll_set_reg20_enet1_qos    (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->enet1_qos     = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_enet1_qos    (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->enet1_qos    ;
}

static inline void sys_ahbp_ll_set_reg20_usb_qos      (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->usb_qos       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_usb_qos      (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->usb_qos      ;
}

static inline void sys_ahbp_ll_set_reg20_h26e_qos     (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->h26e_qos      = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_h26e_qos     (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->h26e_qos     ;
}

static inline void sys_ahbp_ll_set_reg20_isp_qos      (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->isp_qos       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_isp_qos      (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->isp_qos      ;
}

static inline void sys_ahbp_ll_set_reg20_videopost_qos(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->videopost_qos = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_videopost_qos(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->videopost_qos;
}

static inline void sys_ahbp_ll_set_reg20_dpu_qos      (uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->dpu_qos       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_dpu_qos      (void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->dpu_qos      ;
}

static inline void sys_ahbp_ll_set_reg20_cbus_qos(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->cbus_qos = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_cbus_qos(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->cbus_qos;
}

static inline void sys_ahbp_ll_set_reg20_npu0_qos(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->npu0_qos = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_npu0_qos(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->npu0_qos;
}

static inline void sys_ahbp_ll_set_reg20_npu1_qos(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->npu1_qos = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_npu1_qos(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->npu1_qos;
}

static inline void sys_ahbp_ll_set_reg20_cpu1_qos(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->cpu1_qos = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_cpu1_qos(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->cpu1_qos;
}

static inline void sys_ahbp_ll_set_reg20_reserved_30_31(uint32_t v) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	r->reserved_30_31 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg20_reserved_30_31(void) {
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t*)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));
	return r->reserved_30_31;
}

//reg reg21:

static inline void sys_ahbp_ll_set_reg21_value(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_value(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg21_dpu_gate_cpu_en(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->dpu_gate_cpu_en = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_dpu_gate_cpu_en(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->dpu_gate_cpu_en;
}

static inline void sys_ahbp_ll_set_reg21_dpu_gate_videopost_en(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->dpu_gate_videopost_en = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_dpu_gate_videopost_en(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->dpu_gate_videopost_en;
}

static inline void sys_ahbp_ll_set_reg21_dpu_gate_h26e_en(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->dpu_gate_h26e_en = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_dpu_gate_h26e_en(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->dpu_gate_h26e_en;
}

static inline void sys_ahbp_ll_set_reg21_reserved_3_7(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->reserved_3_7 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_reserved_3_7(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->reserved_3_7;
}

static inline void sys_ahbp_ll_set_reg21_reserved_8_8(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->reserved_8_8 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_reserved_8_8(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->reserved_8_8;
}

static inline void sys_ahbp_ll_set_reg21_dpu_gate_videopost_override(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->dpu_gate_videopost_override = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_dpu_gate_videopost_override(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->dpu_gate_videopost_override;
}

static inline void sys_ahbp_ll_set_reg21_dpu_gate_h26e_override(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->dpu_gate_h26e_override = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_dpu_gate_h26e_override(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->dpu_gate_h26e_override;
}

static inline void sys_ahbp_ll_set_reg21_reserved_11_15(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->reserved_11_15 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_reserved_11_15(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->reserved_11_15;
}

static inline uint32_t sys_ahbp_ll_get_reg21_cpu_bus_dis(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->cpu_bus_dis;
}

static inline uint32_t sys_ahbp_ll_get_reg21_videopost_bus_dis(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->videopost_bus_dis;
}

static inline uint32_t sys_ahbp_ll_get_reg21_h26e_bus_dis(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->h26e_bus_dis;
}

static inline uint32_t sys_ahbp_ll_get_reg21_reserved_19_23(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->reserved_19_23;
}

static inline void sys_ahbp_ll_set_reg21_dpu_gate_key(uint32_t v) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	r->dpu_gate_key = v;
}

static inline uint32_t sys_ahbp_ll_get_reg21_dpu_gate_key(void) {
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t*)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));
	return r->dpu_gate_key;
}

//reg reg22:

static inline void sys_ahbp_ll_set_reg22_value(uint32_t v) {
	sys_ahbp_reg22_t *r = (sys_ahbp_reg22_t*)(SOC_SYS_AHBP_REG_BASE + (0x22 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg22_value(void) {
	sys_ahbp_reg22_t *r = (sys_ahbp_reg22_t*)(SOC_SYS_AHBP_REG_BASE + (0x22 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg22_dbug_config0(uint32_t v) {
	sys_ahbp_reg22_t *r = (sys_ahbp_reg22_t*)(SOC_SYS_AHBP_REG_BASE + (0x22 << 2));
	r->dbug_config0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg22_dbug_config0(void) {
	sys_ahbp_reg22_t *r = (sys_ahbp_reg22_t*)(SOC_SYS_AHBP_REG_BASE + (0x22 << 2));
	return r->dbug_config0;
}

//reg reg23:

static inline void sys_ahbp_ll_set_reg23_value(uint32_t v) {
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t*)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg23_value(void) {
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t*)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg23_dbug_mux(uint32_t v) {
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t*)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));
	r->dbug_mux = v;
}

static inline uint32_t sys_ahbp_ll_get_reg23_dbug_mux(void) {
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t*)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));
	return r->dbug_mux;
}

static inline void sys_ahbp_ll_set_reg23_dbug_config1(uint32_t v) {
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t*)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));
	r->dbug_config1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg23_dbug_config1(void) {
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t*)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));
	return r->dbug_config1;
}

//reg reg24:

static inline void sys_ahbp_ll_set_reg24_value(uint32_t v) {
	sys_ahbp_reg24_t *r = (sys_ahbp_reg24_t*)(SOC_SYS_AHBP_REG_BASE + (0x24 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg24_value(void) {
	sys_ahbp_reg24_t *r = (sys_ahbp_reg24_t*)(SOC_SYS_AHBP_REG_BASE + (0x24 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg24_trace_config(uint32_t v) {
	sys_ahbp_reg24_t *r = (sys_ahbp_reg24_t*)(SOC_SYS_AHBP_REG_BASE + (0x24 << 2));
	r->trace_config = v;
}

static inline uint32_t sys_ahbp_ll_get_reg24_trace_config(void) {
	sys_ahbp_reg24_t *r = (sys_ahbp_reg24_t*)(SOC_SYS_AHBP_REG_BASE + (0x24 << 2));
	return r->trace_config;
}

//reg reg25:

static inline void sys_ahbp_ll_set_reg25_value(uint32_t v) {
	sys_ahbp_reg25_t *r = (sys_ahbp_reg25_t*)(SOC_SYS_AHBP_REG_BASE + (0x25 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg25_value(void) {
	sys_ahbp_reg25_t *r = (sys_ahbp_reg25_t*)(SOC_SYS_AHBP_REG_BASE + (0x25 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg25_gpio_dbug_readout(void) {
	sys_ahbp_reg25_t *r = (sys_ahbp_reg25_t*)(SOC_SYS_AHBP_REG_BASE + (0x25 << 2));
	return r->gpio_dbug_readout;
}

//reg reg26:

static inline void sys_ahbp_ll_set_reg26_value(uint32_t v) {
	sys_ahbp_reg26_t *r = (sys_ahbp_reg26_t*)(SOC_SYS_AHBP_REG_BASE + (0x26 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg26_value(void) {
	sys_ahbp_reg26_t *r = (sys_ahbp_reg26_t*)(SOC_SYS_AHBP_REG_BASE + (0x26 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg26_general0(uint32_t v) {
	sys_ahbp_reg26_t *r = (sys_ahbp_reg26_t*)(SOC_SYS_AHBP_REG_BASE + (0x26 << 2));
	r->general0 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg26_general0(void) {
	sys_ahbp_reg26_t *r = (sys_ahbp_reg26_t*)(SOC_SYS_AHBP_REG_BASE + (0x26 << 2));
	return r->general0;
}

//reg reg27:

static inline void sys_ahbp_ll_set_reg27_value(uint32_t v) {
	sys_ahbp_reg27_t *r = (sys_ahbp_reg27_t*)(SOC_SYS_AHBP_REG_BASE + (0x27 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg27_value(void) {
	sys_ahbp_reg27_t *r = (sys_ahbp_reg27_t*)(SOC_SYS_AHBP_REG_BASE + (0x27 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg27_general1(uint32_t v) {
	sys_ahbp_reg27_t *r = (sys_ahbp_reg27_t*)(SOC_SYS_AHBP_REG_BASE + (0x27 << 2));
	r->general1 = v;
}

static inline uint32_t sys_ahbp_ll_get_reg27_general1(void) {
	sys_ahbp_reg27_t *r = (sys_ahbp_reg27_t*)(SOC_SYS_AHBP_REG_BASE + (0x27 << 2));
	return r->general1;
}

//reg reg28:

static inline void sys_ahbp_ll_set_reg28_value(uint32_t v) {
	sys_ahbp_reg28_t *r = (sys_ahbp_reg28_t*)(SOC_SYS_AHBP_REG_BASE + (0x28 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg28_value(void) {
	sys_ahbp_reg28_t *r = (sys_ahbp_reg28_t*)(SOC_SYS_AHBP_REG_BASE + (0x28 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg28_gpu_buffa_enable      (uint32_t v) {
	sys_ahbp_reg28_t *r = (sys_ahbp_reg28_t*)(SOC_SYS_AHBP_REG_BASE + (0x28 << 2));
	r->gpu_buffa_enable       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg28_gpu_buffa_enable      (void) {
	sys_ahbp_reg28_t *r = (sys_ahbp_reg28_t*)(SOC_SYS_AHBP_REG_BASE + (0x28 << 2));
	return r->gpu_buffa_enable      ;
}

//reg reg29:

static inline void sys_ahbp_ll_set_reg29_value(uint32_t v) {
	sys_ahbp_reg29_t *r = (sys_ahbp_reg29_t*)(SOC_SYS_AHBP_REG_BASE + (0x29 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg29_value(void) {
	sys_ahbp_reg29_t *r = (sys_ahbp_reg29_t*)(SOC_SYS_AHBP_REG_BASE + (0x29 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg29_gpu_buffa_begin       (uint32_t v) {
	sys_ahbp_reg29_t *r = (sys_ahbp_reg29_t*)(SOC_SYS_AHBP_REG_BASE + (0x29 << 2));
	r->gpu_buffa_begin        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg29_gpu_buffa_begin       (void) {
	sys_ahbp_reg29_t *r = (sys_ahbp_reg29_t*)(SOC_SYS_AHBP_REG_BASE + (0x29 << 2));
	return r->gpu_buffa_begin       ;
}

//reg reg2a:

static inline void sys_ahbp_ll_set_reg2a_value(uint32_t v) {
	sys_ahbp_reg2a_t *r = (sys_ahbp_reg2a_t*)(SOC_SYS_AHBP_REG_BASE + (0x2a << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2a_value(void) {
	sys_ahbp_reg2a_t *r = (sys_ahbp_reg2a_t*)(SOC_SYS_AHBP_REG_BASE + (0x2a << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg2a_gpu_buffa_size        (uint32_t v) {
	sys_ahbp_reg2a_t *r = (sys_ahbp_reg2a_t*)(SOC_SYS_AHBP_REG_BASE + (0x2a << 2));
	r->gpu_buffa_size         = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2a_gpu_buffa_size        (void) {
	sys_ahbp_reg2a_t *r = (sys_ahbp_reg2a_t*)(SOC_SYS_AHBP_REG_BASE + (0x2a << 2));
	return r->gpu_buffa_size        ;
}

//reg reg2b:

static inline void sys_ahbp_ll_set_reg2b_value(uint32_t v) {
	sys_ahbp_reg2b_t *r = (sys_ahbp_reg2b_t*)(SOC_SYS_AHBP_REG_BASE + (0x2b << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2b_value(void) {
	sys_ahbp_reg2b_t *r = (sys_ahbp_reg2b_t*)(SOC_SYS_AHBP_REG_BASE + (0x2b << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg2b_gpu_pica_begin        (uint32_t v) {
	sys_ahbp_reg2b_t *r = (sys_ahbp_reg2b_t*)(SOC_SYS_AHBP_REG_BASE + (0x2b << 2));
	r->gpu_pica_begin         = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2b_gpu_pica_begin        (void) {
	sys_ahbp_reg2b_t *r = (sys_ahbp_reg2b_t*)(SOC_SYS_AHBP_REG_BASE + (0x2b << 2));
	return r->gpu_pica_begin        ;
}

//reg reg2c:

static inline void sys_ahbp_ll_set_reg2c_value(uint32_t v) {
	sys_ahbp_reg2c_t *r = (sys_ahbp_reg2c_t*)(SOC_SYS_AHBP_REG_BASE + (0x2c << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2c_value(void) {
	sys_ahbp_reg2c_t *r = (sys_ahbp_reg2c_t*)(SOC_SYS_AHBP_REG_BASE + (0x2c << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg2c_gpu_pica_halfbuff_end (uint32_t v) {
	sys_ahbp_reg2c_t *r = (sys_ahbp_reg2c_t*)(SOC_SYS_AHBP_REG_BASE + (0x2c << 2));
	r->gpu_pica_halfbuff_end  = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2c_gpu_pica_halfbuff_end (void) {
	sys_ahbp_reg2c_t *r = (sys_ahbp_reg2c_t*)(SOC_SYS_AHBP_REG_BASE + (0x2c << 2));
	return r->gpu_pica_halfbuff_end ;
}

//reg reg2d:

static inline void sys_ahbp_ll_set_reg2d_value(uint32_t v) {
	sys_ahbp_reg2d_t *r = (sys_ahbp_reg2d_t*)(SOC_SYS_AHBP_REG_BASE + (0x2d << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2d_value(void) {
	sys_ahbp_reg2d_t *r = (sys_ahbp_reg2d_t*)(SOC_SYS_AHBP_REG_BASE + (0x2d << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg2d_gpu_pica_end          (uint32_t v) {
	sys_ahbp_reg2d_t *r = (sys_ahbp_reg2d_t*)(SOC_SYS_AHBP_REG_BASE + (0x2d << 2));
	r->gpu_pica_end           = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2d_gpu_pica_end          (void) {
	sys_ahbp_reg2d_t *r = (sys_ahbp_reg2d_t*)(SOC_SYS_AHBP_REG_BASE + (0x2d << 2));
	return r->gpu_pica_end          ;
}

//reg reg2e:

static inline void sys_ahbp_ll_set_reg2e_value(uint32_t v) {
	sys_ahbp_reg2e_t *r = (sys_ahbp_reg2e_t*)(SOC_SYS_AHBP_REG_BASE + (0x2e << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2e_value(void) {
	sys_ahbp_reg2e_t *r = (sys_ahbp_reg2e_t*)(SOC_SYS_AHBP_REG_BASE + (0x2e << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg2e_reserved_0_31(void) {
	sys_ahbp_reg2e_t *r = (sys_ahbp_reg2e_t*)(SOC_SYS_AHBP_REG_BASE + (0x2e << 2));
	return r->reserved_0_31;
}

//reg reg2f:

static inline void sys_ahbp_ll_set_reg2f_value(uint32_t v) {
	sys_ahbp_reg2f_t *r = (sys_ahbp_reg2f_t*)(SOC_SYS_AHBP_REG_BASE + (0x2f << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg2f_value(void) {
	sys_ahbp_reg2f_t *r = (sys_ahbp_reg2f_t*)(SOC_SYS_AHBP_REG_BASE + (0x2f << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg2f_gpu_buffa_remap_addr(void) {
	sys_ahbp_reg2f_t *r = (sys_ahbp_reg2f_t*)(SOC_SYS_AHBP_REG_BASE + (0x2f << 2));
	return r->gpu_buffa_remap_addr;
}

//reg reg30:

static inline void sys_ahbp_ll_set_reg30_value(uint32_t v) {
	sys_ahbp_reg30_t *r = (sys_ahbp_reg30_t*)(SOC_SYS_AHBP_REG_BASE + (0x30 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg30_value(void) {
	sys_ahbp_reg30_t *r = (sys_ahbp_reg30_t*)(SOC_SYS_AHBP_REG_BASE + (0x30 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg30_gpu_buffb_enable      (uint32_t v) {
	sys_ahbp_reg30_t *r = (sys_ahbp_reg30_t*)(SOC_SYS_AHBP_REG_BASE + (0x30 << 2));
	r->gpu_buffb_enable       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg30_gpu_buffb_enable      (void) {
	sys_ahbp_reg30_t *r = (sys_ahbp_reg30_t*)(SOC_SYS_AHBP_REG_BASE + (0x30 << 2));
	return r->gpu_buffb_enable      ;
}

//reg reg31:

static inline void sys_ahbp_ll_set_reg31_value(uint32_t v) {
	sys_ahbp_reg31_t *r = (sys_ahbp_reg31_t*)(SOC_SYS_AHBP_REG_BASE + (0x31 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg31_value(void) {
	sys_ahbp_reg31_t *r = (sys_ahbp_reg31_t*)(SOC_SYS_AHBP_REG_BASE + (0x31 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg31_gpu_buffb_begin       (uint32_t v) {
	sys_ahbp_reg31_t *r = (sys_ahbp_reg31_t*)(SOC_SYS_AHBP_REG_BASE + (0x31 << 2));
	r->gpu_buffb_begin        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg31_gpu_buffb_begin       (void) {
	sys_ahbp_reg31_t *r = (sys_ahbp_reg31_t*)(SOC_SYS_AHBP_REG_BASE + (0x31 << 2));
	return r->gpu_buffb_begin       ;
}

//reg reg32:

static inline void sys_ahbp_ll_set_reg32_value(uint32_t v) {
	sys_ahbp_reg32_t *r = (sys_ahbp_reg32_t*)(SOC_SYS_AHBP_REG_BASE + (0x32 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg32_value(void) {
	sys_ahbp_reg32_t *r = (sys_ahbp_reg32_t*)(SOC_SYS_AHBP_REG_BASE + (0x32 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg32_gpu_buffb_size        (uint32_t v) {
	sys_ahbp_reg32_t *r = (sys_ahbp_reg32_t*)(SOC_SYS_AHBP_REG_BASE + (0x32 << 2));
	r->gpu_buffb_size         = v;
}

static inline uint32_t sys_ahbp_ll_get_reg32_gpu_buffb_size        (void) {
	sys_ahbp_reg32_t *r = (sys_ahbp_reg32_t*)(SOC_SYS_AHBP_REG_BASE + (0x32 << 2));
	return r->gpu_buffb_size        ;
}

//reg reg33:

static inline void sys_ahbp_ll_set_reg33_value(uint32_t v) {
	sys_ahbp_reg33_t *r = (sys_ahbp_reg33_t*)(SOC_SYS_AHBP_REG_BASE + (0x33 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg33_value(void) {
	sys_ahbp_reg33_t *r = (sys_ahbp_reg33_t*)(SOC_SYS_AHBP_REG_BASE + (0x33 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg33_gpu_picb_begin        (uint32_t v) {
	sys_ahbp_reg33_t *r = (sys_ahbp_reg33_t*)(SOC_SYS_AHBP_REG_BASE + (0x33 << 2));
	r->gpu_picb_begin         = v;
}

static inline uint32_t sys_ahbp_ll_get_reg33_gpu_picb_begin        (void) {
	sys_ahbp_reg33_t *r = (sys_ahbp_reg33_t*)(SOC_SYS_AHBP_REG_BASE + (0x33 << 2));
	return r->gpu_picb_begin        ;
}

//reg reg34:

static inline void sys_ahbp_ll_set_reg34_value(uint32_t v) {
	sys_ahbp_reg34_t *r = (sys_ahbp_reg34_t*)(SOC_SYS_AHBP_REG_BASE + (0x34 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg34_value(void) {
	sys_ahbp_reg34_t *r = (sys_ahbp_reg34_t*)(SOC_SYS_AHBP_REG_BASE + (0x34 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg34_gpu_picb_halfbuff_end (uint32_t v) {
	sys_ahbp_reg34_t *r = (sys_ahbp_reg34_t*)(SOC_SYS_AHBP_REG_BASE + (0x34 << 2));
	r->gpu_picb_halfbuff_end  = v;
}

static inline uint32_t sys_ahbp_ll_get_reg34_gpu_picb_halfbuff_end (void) {
	sys_ahbp_reg34_t *r = (sys_ahbp_reg34_t*)(SOC_SYS_AHBP_REG_BASE + (0x34 << 2));
	return r->gpu_picb_halfbuff_end ;
}

//reg reg35:

static inline void sys_ahbp_ll_set_reg35_value(uint32_t v) {
	sys_ahbp_reg35_t *r = (sys_ahbp_reg35_t*)(SOC_SYS_AHBP_REG_BASE + (0x35 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg35_value(void) {
	sys_ahbp_reg35_t *r = (sys_ahbp_reg35_t*)(SOC_SYS_AHBP_REG_BASE + (0x35 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg35_gpu_picb_end          (uint32_t v) {
	sys_ahbp_reg35_t *r = (sys_ahbp_reg35_t*)(SOC_SYS_AHBP_REG_BASE + (0x35 << 2));
	r->gpu_picb_end           = v;
}

static inline uint32_t sys_ahbp_ll_get_reg35_gpu_picb_end          (void) {
	sys_ahbp_reg35_t *r = (sys_ahbp_reg35_t*)(SOC_SYS_AHBP_REG_BASE + (0x35 << 2));
	return r->gpu_picb_end          ;
}

//reg reg36:

static inline void sys_ahbp_ll_set_reg36_value(uint32_t v) {
	sys_ahbp_reg36_t *r = (sys_ahbp_reg36_t*)(SOC_SYS_AHBP_REG_BASE + (0x36 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg36_value(void) {
	sys_ahbp_reg36_t *r = (sys_ahbp_reg36_t*)(SOC_SYS_AHBP_REG_BASE + (0x36 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg36_reserved_0_31(void) {
	sys_ahbp_reg36_t *r = (sys_ahbp_reg36_t*)(SOC_SYS_AHBP_REG_BASE + (0x36 << 2));
	return r->reserved_0_31;
}

//reg reg37:

static inline void sys_ahbp_ll_set_reg37_value(uint32_t v) {
	sys_ahbp_reg37_t *r = (sys_ahbp_reg37_t*)(SOC_SYS_AHBP_REG_BASE + (0x37 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg37_value(void) {
	sys_ahbp_reg37_t *r = (sys_ahbp_reg37_t*)(SOC_SYS_AHBP_REG_BASE + (0x37 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg37_gpu_buffb_remap_addr(void) {
	sys_ahbp_reg37_t *r = (sys_ahbp_reg37_t*)(SOC_SYS_AHBP_REG_BASE + (0x37 << 2));
	return r->gpu_buffb_remap_addr;
}

//reg reg38:

static inline void sys_ahbp_ll_set_reg38_value(uint32_t v) {
	sys_ahbp_reg38_t *r = (sys_ahbp_reg38_t*)(SOC_SYS_AHBP_REG_BASE + (0x38 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg38_value(void) {
	sys_ahbp_reg38_t *r = (sys_ahbp_reg38_t*)(SOC_SYS_AHBP_REG_BASE + (0x38 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg38_h26d_buffa_enable     (uint32_t v) {
	sys_ahbp_reg38_t *r = (sys_ahbp_reg38_t*)(SOC_SYS_AHBP_REG_BASE + (0x38 << 2));
	r->h26d_buffa_enable      = v;
}

static inline uint32_t sys_ahbp_ll_get_reg38_h26d_buffa_enable     (void) {
	sys_ahbp_reg38_t *r = (sys_ahbp_reg38_t*)(SOC_SYS_AHBP_REG_BASE + (0x38 << 2));
	return r->h26d_buffa_enable     ;
}

//reg reg39:

static inline void sys_ahbp_ll_set_reg39_value(uint32_t v) {
	sys_ahbp_reg39_t *r = (sys_ahbp_reg39_t*)(SOC_SYS_AHBP_REG_BASE + (0x39 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg39_value(void) {
	sys_ahbp_reg39_t *r = (sys_ahbp_reg39_t*)(SOC_SYS_AHBP_REG_BASE + (0x39 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg39_h26d_buffa_begin      (uint32_t v) {
	sys_ahbp_reg39_t *r = (sys_ahbp_reg39_t*)(SOC_SYS_AHBP_REG_BASE + (0x39 << 2));
	r->h26d_buffa_begin       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg39_h26d_buffa_begin      (void) {
	sys_ahbp_reg39_t *r = (sys_ahbp_reg39_t*)(SOC_SYS_AHBP_REG_BASE + (0x39 << 2));
	return r->h26d_buffa_begin      ;
}

//reg reg3a:

static inline void sys_ahbp_ll_set_reg3a_value(uint32_t v) {
	sys_ahbp_reg3a_t *r = (sys_ahbp_reg3a_t*)(SOC_SYS_AHBP_REG_BASE + (0x3a << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3a_value(void) {
	sys_ahbp_reg3a_t *r = (sys_ahbp_reg3a_t*)(SOC_SYS_AHBP_REG_BASE + (0x3a << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg3a_h26d_buffa_size       (uint32_t v) {
	sys_ahbp_reg3a_t *r = (sys_ahbp_reg3a_t*)(SOC_SYS_AHBP_REG_BASE + (0x3a << 2));
	r->h26d_buffa_size        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3a_h26d_buffa_size       (void) {
	sys_ahbp_reg3a_t *r = (sys_ahbp_reg3a_t*)(SOC_SYS_AHBP_REG_BASE + (0x3a << 2));
	return r->h26d_buffa_size       ;
}

//reg reg3b:

static inline void sys_ahbp_ll_set_reg3b_value(uint32_t v) {
	sys_ahbp_reg3b_t *r = (sys_ahbp_reg3b_t*)(SOC_SYS_AHBP_REG_BASE + (0x3b << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3b_value(void) {
	sys_ahbp_reg3b_t *r = (sys_ahbp_reg3b_t*)(SOC_SYS_AHBP_REG_BASE + (0x3b << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg3b_h26d_pica_begin       (uint32_t v) {
	sys_ahbp_reg3b_t *r = (sys_ahbp_reg3b_t*)(SOC_SYS_AHBP_REG_BASE + (0x3b << 2));
	r->h26d_pica_begin        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3b_h26d_pica_begin       (void) {
	sys_ahbp_reg3b_t *r = (sys_ahbp_reg3b_t*)(SOC_SYS_AHBP_REG_BASE + (0x3b << 2));
	return r->h26d_pica_begin       ;
}

//reg reg3c:

static inline void sys_ahbp_ll_set_reg3c_value(uint32_t v) {
	sys_ahbp_reg3c_t *r = (sys_ahbp_reg3c_t*)(SOC_SYS_AHBP_REG_BASE + (0x3c << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3c_value(void) {
	sys_ahbp_reg3c_t *r = (sys_ahbp_reg3c_t*)(SOC_SYS_AHBP_REG_BASE + (0x3c << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg3c_h26d_pica_halfbuff_end(uint32_t v) {
	sys_ahbp_reg3c_t *r = (sys_ahbp_reg3c_t*)(SOC_SYS_AHBP_REG_BASE + (0x3c << 2));
	r->h26d_pica_halfbuff_end = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3c_h26d_pica_halfbuff_end(void) {
	sys_ahbp_reg3c_t *r = (sys_ahbp_reg3c_t*)(SOC_SYS_AHBP_REG_BASE + (0x3c << 2));
	return r->h26d_pica_halfbuff_end;
}

//reg reg3d:

static inline void sys_ahbp_ll_set_reg3d_value(uint32_t v) {
	sys_ahbp_reg3d_t *r = (sys_ahbp_reg3d_t*)(SOC_SYS_AHBP_REG_BASE + (0x3d << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3d_value(void) {
	sys_ahbp_reg3d_t *r = (sys_ahbp_reg3d_t*)(SOC_SYS_AHBP_REG_BASE + (0x3d << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg3d_h26d_pica_end         (uint32_t v) {
	sys_ahbp_reg3d_t *r = (sys_ahbp_reg3d_t*)(SOC_SYS_AHBP_REG_BASE + (0x3d << 2));
	r->h26d_pica_end          = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3d_h26d_pica_end         (void) {
	sys_ahbp_reg3d_t *r = (sys_ahbp_reg3d_t*)(SOC_SYS_AHBP_REG_BASE + (0x3d << 2));
	return r->h26d_pica_end         ;
}

//reg reg3e:

static inline void sys_ahbp_ll_set_reg3e_value(uint32_t v) {
	sys_ahbp_reg3e_t *r = (sys_ahbp_reg3e_t*)(SOC_SYS_AHBP_REG_BASE + (0x3e << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3e_value(void) {
	sys_ahbp_reg3e_t *r = (sys_ahbp_reg3e_t*)(SOC_SYS_AHBP_REG_BASE + (0x3e << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg3e_reserved_0_31(void) {
	sys_ahbp_reg3e_t *r = (sys_ahbp_reg3e_t*)(SOC_SYS_AHBP_REG_BASE + (0x3e << 2));
	return r->reserved_0_31;
}

//reg reg3f:

static inline void sys_ahbp_ll_set_reg3f_value(uint32_t v) {
	sys_ahbp_reg3f_t *r = (sys_ahbp_reg3f_t*)(SOC_SYS_AHBP_REG_BASE + (0x3f << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg3f_value(void) {
	sys_ahbp_reg3f_t *r = (sys_ahbp_reg3f_t*)(SOC_SYS_AHBP_REG_BASE + (0x3f << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg3f_h26d_buffa_remap_addr(void) {
	sys_ahbp_reg3f_t *r = (sys_ahbp_reg3f_t*)(SOC_SYS_AHBP_REG_BASE + (0x3f << 2));
	return r->h26d_buffa_remap_addr;
}

//reg reg40:

static inline void sys_ahbp_ll_set_reg40_value(uint32_t v) {
	sys_ahbp_reg40_t *r = (sys_ahbp_reg40_t*)(SOC_SYS_AHBP_REG_BASE + (0x40 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg40_value(void) {
	sys_ahbp_reg40_t *r = (sys_ahbp_reg40_t*)(SOC_SYS_AHBP_REG_BASE + (0x40 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg40_h26d_buffb_enable     (uint32_t v) {
	sys_ahbp_reg40_t *r = (sys_ahbp_reg40_t*)(SOC_SYS_AHBP_REG_BASE + (0x40 << 2));
	r->h26d_buffb_enable      = v;
}

static inline uint32_t sys_ahbp_ll_get_reg40_h26d_buffb_enable     (void) {
	sys_ahbp_reg40_t *r = (sys_ahbp_reg40_t*)(SOC_SYS_AHBP_REG_BASE + (0x40 << 2));
	return r->h26d_buffb_enable     ;
}

//reg reg41:

static inline void sys_ahbp_ll_set_reg41_value(uint32_t v) {
	sys_ahbp_reg41_t *r = (sys_ahbp_reg41_t*)(SOC_SYS_AHBP_REG_BASE + (0x41 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg41_value(void) {
	sys_ahbp_reg41_t *r = (sys_ahbp_reg41_t*)(SOC_SYS_AHBP_REG_BASE + (0x41 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg41_h26d_buffb_begin      (uint32_t v) {
	sys_ahbp_reg41_t *r = (sys_ahbp_reg41_t*)(SOC_SYS_AHBP_REG_BASE + (0x41 << 2));
	r->h26d_buffb_begin       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg41_h26d_buffb_begin      (void) {
	sys_ahbp_reg41_t *r = (sys_ahbp_reg41_t*)(SOC_SYS_AHBP_REG_BASE + (0x41 << 2));
	return r->h26d_buffb_begin      ;
}

//reg reg42:

static inline void sys_ahbp_ll_set_reg42_value(uint32_t v) {
	sys_ahbp_reg42_t *r = (sys_ahbp_reg42_t*)(SOC_SYS_AHBP_REG_BASE + (0x42 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg42_value(void) {
	sys_ahbp_reg42_t *r = (sys_ahbp_reg42_t*)(SOC_SYS_AHBP_REG_BASE + (0x42 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg42_h26d_buffb_size       (uint32_t v) {
	sys_ahbp_reg42_t *r = (sys_ahbp_reg42_t*)(SOC_SYS_AHBP_REG_BASE + (0x42 << 2));
	r->h26d_buffb_size        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg42_h26d_buffb_size       (void) {
	sys_ahbp_reg42_t *r = (sys_ahbp_reg42_t*)(SOC_SYS_AHBP_REG_BASE + (0x42 << 2));
	return r->h26d_buffb_size       ;
}

//reg reg43:

static inline void sys_ahbp_ll_set_reg43_value(uint32_t v) {
	sys_ahbp_reg43_t *r = (sys_ahbp_reg43_t*)(SOC_SYS_AHBP_REG_BASE + (0x43 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg43_value(void) {
	sys_ahbp_reg43_t *r = (sys_ahbp_reg43_t*)(SOC_SYS_AHBP_REG_BASE + (0x43 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg43_h26d_picb_begin       (uint32_t v) {
	sys_ahbp_reg43_t *r = (sys_ahbp_reg43_t*)(SOC_SYS_AHBP_REG_BASE + (0x43 << 2));
	r->h26d_picb_begin        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg43_h26d_picb_begin       (void) {
	sys_ahbp_reg43_t *r = (sys_ahbp_reg43_t*)(SOC_SYS_AHBP_REG_BASE + (0x43 << 2));
	return r->h26d_picb_begin       ;
}

//reg reg44:

static inline void sys_ahbp_ll_set_reg44_value(uint32_t v) {
	sys_ahbp_reg44_t *r = (sys_ahbp_reg44_t*)(SOC_SYS_AHBP_REG_BASE + (0x44 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg44_value(void) {
	sys_ahbp_reg44_t *r = (sys_ahbp_reg44_t*)(SOC_SYS_AHBP_REG_BASE + (0x44 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg44_h26d_picb_halfbuff_end(uint32_t v) {
	sys_ahbp_reg44_t *r = (sys_ahbp_reg44_t*)(SOC_SYS_AHBP_REG_BASE + (0x44 << 2));
	r->h26d_picb_halfbuff_end = v;
}

static inline uint32_t sys_ahbp_ll_get_reg44_h26d_picb_halfbuff_end(void) {
	sys_ahbp_reg44_t *r = (sys_ahbp_reg44_t*)(SOC_SYS_AHBP_REG_BASE + (0x44 << 2));
	return r->h26d_picb_halfbuff_end;
}

//reg reg45:

static inline void sys_ahbp_ll_set_reg45_value(uint32_t v) {
	sys_ahbp_reg45_t *r = (sys_ahbp_reg45_t*)(SOC_SYS_AHBP_REG_BASE + (0x45 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg45_value(void) {
	sys_ahbp_reg45_t *r = (sys_ahbp_reg45_t*)(SOC_SYS_AHBP_REG_BASE + (0x45 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg45_h26d_picb_end         (uint32_t v) {
	sys_ahbp_reg45_t *r = (sys_ahbp_reg45_t*)(SOC_SYS_AHBP_REG_BASE + (0x45 << 2));
	r->h26d_picb_end          = v;
}

static inline uint32_t sys_ahbp_ll_get_reg45_h26d_picb_end         (void) {
	sys_ahbp_reg45_t *r = (sys_ahbp_reg45_t*)(SOC_SYS_AHBP_REG_BASE + (0x45 << 2));
	return r->h26d_picb_end         ;
}

//reg reg46:

static inline void sys_ahbp_ll_set_reg46_value(uint32_t v) {
	sys_ahbp_reg46_t *r = (sys_ahbp_reg46_t*)(SOC_SYS_AHBP_REG_BASE + (0x46 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg46_value(void) {
	sys_ahbp_reg46_t *r = (sys_ahbp_reg46_t*)(SOC_SYS_AHBP_REG_BASE + (0x46 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg46_reserved_0_31(void) {
	sys_ahbp_reg46_t *r = (sys_ahbp_reg46_t*)(SOC_SYS_AHBP_REG_BASE + (0x46 << 2));
	return r->reserved_0_31;
}

//reg reg47:

static inline void sys_ahbp_ll_set_reg47_value(uint32_t v) {
	sys_ahbp_reg47_t *r = (sys_ahbp_reg47_t*)(SOC_SYS_AHBP_REG_BASE + (0x47 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg47_value(void) {
	sys_ahbp_reg47_t *r = (sys_ahbp_reg47_t*)(SOC_SYS_AHBP_REG_BASE + (0x47 << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg47_h26d_buffb_remap_addr(void) {
	sys_ahbp_reg47_t *r = (sys_ahbp_reg47_t*)(SOC_SYS_AHBP_REG_BASE + (0x47 << 2));
	return r->h26d_buffb_remap_addr;
}

//reg reg48:

static inline void sys_ahbp_ll_set_reg48_value(uint32_t v) {
	sys_ahbp_reg48_t *r = (sys_ahbp_reg48_t*)(SOC_SYS_AHBP_REG_BASE + (0x48 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg48_value(void) {
	sys_ahbp_reg48_t *r = (sys_ahbp_reg48_t*)(SOC_SYS_AHBP_REG_BASE + (0x48 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg48_h26d_buffc_enable     (uint32_t v) {
	sys_ahbp_reg48_t *r = (sys_ahbp_reg48_t*)(SOC_SYS_AHBP_REG_BASE + (0x48 << 2));
	r->h26d_buffc_enable      = v;
}

static inline uint32_t sys_ahbp_ll_get_reg48_h26d_buffc_enable     (void) {
	sys_ahbp_reg48_t *r = (sys_ahbp_reg48_t*)(SOC_SYS_AHBP_REG_BASE + (0x48 << 2));
	return r->h26d_buffc_enable     ;
}

//reg reg49:

static inline void sys_ahbp_ll_set_reg49_value(uint32_t v) {
	sys_ahbp_reg49_t *r = (sys_ahbp_reg49_t*)(SOC_SYS_AHBP_REG_BASE + (0x49 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg49_value(void) {
	sys_ahbp_reg49_t *r = (sys_ahbp_reg49_t*)(SOC_SYS_AHBP_REG_BASE + (0x49 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg49_h26d_buffc_begin      (uint32_t v) {
	sys_ahbp_reg49_t *r = (sys_ahbp_reg49_t*)(SOC_SYS_AHBP_REG_BASE + (0x49 << 2));
	r->h26d_buffc_begin       = v;
}

static inline uint32_t sys_ahbp_ll_get_reg49_h26d_buffc_begin      (void) {
	sys_ahbp_reg49_t *r = (sys_ahbp_reg49_t*)(SOC_SYS_AHBP_REG_BASE + (0x49 << 2));
	return r->h26d_buffc_begin      ;
}

//reg reg4a:

static inline void sys_ahbp_ll_set_reg4a_value(uint32_t v) {
	sys_ahbp_reg4a_t *r = (sys_ahbp_reg4a_t*)(SOC_SYS_AHBP_REG_BASE + (0x4a << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4a_value(void) {
	sys_ahbp_reg4a_t *r = (sys_ahbp_reg4a_t*)(SOC_SYS_AHBP_REG_BASE + (0x4a << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg4a_h26d_buffc_size       (uint32_t v) {
	sys_ahbp_reg4a_t *r = (sys_ahbp_reg4a_t*)(SOC_SYS_AHBP_REG_BASE + (0x4a << 2));
	r->h26d_buffc_size        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4a_h26d_buffc_size       (void) {
	sys_ahbp_reg4a_t *r = (sys_ahbp_reg4a_t*)(SOC_SYS_AHBP_REG_BASE + (0x4a << 2));
	return r->h26d_buffc_size       ;
}

//reg reg4b:

static inline void sys_ahbp_ll_set_reg4b_value(uint32_t v) {
	sys_ahbp_reg4b_t *r = (sys_ahbp_reg4b_t*)(SOC_SYS_AHBP_REG_BASE + (0x4b << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4b_value(void) {
	sys_ahbp_reg4b_t *r = (sys_ahbp_reg4b_t*)(SOC_SYS_AHBP_REG_BASE + (0x4b << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg4b_h26d_picc_begin       (uint32_t v) {
	sys_ahbp_reg4b_t *r = (sys_ahbp_reg4b_t*)(SOC_SYS_AHBP_REG_BASE + (0x4b << 2));
	r->h26d_picc_begin        = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4b_h26d_picc_begin       (void) {
	sys_ahbp_reg4b_t *r = (sys_ahbp_reg4b_t*)(SOC_SYS_AHBP_REG_BASE + (0x4b << 2));
	return r->h26d_picc_begin       ;
}

//reg reg4c:

static inline void sys_ahbp_ll_set_reg4c_value(uint32_t v) {
	sys_ahbp_reg4c_t *r = (sys_ahbp_reg4c_t*)(SOC_SYS_AHBP_REG_BASE + (0x4c << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4c_value(void) {
	sys_ahbp_reg4c_t *r = (sys_ahbp_reg4c_t*)(SOC_SYS_AHBP_REG_BASE + (0x4c << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg4c_h26d_picc_halfbuff_end(uint32_t v) {
	sys_ahbp_reg4c_t *r = (sys_ahbp_reg4c_t*)(SOC_SYS_AHBP_REG_BASE + (0x4c << 2));
	r->h26d_picc_halfbuff_end = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4c_h26d_picc_halfbuff_end(void) {
	sys_ahbp_reg4c_t *r = (sys_ahbp_reg4c_t*)(SOC_SYS_AHBP_REG_BASE + (0x4c << 2));
	return r->h26d_picc_halfbuff_end;
}

//reg reg4d:

static inline void sys_ahbp_ll_set_reg4d_value(uint32_t v) {
	sys_ahbp_reg4d_t *r = (sys_ahbp_reg4d_t*)(SOC_SYS_AHBP_REG_BASE + (0x4d << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4d_value(void) {
	sys_ahbp_reg4d_t *r = (sys_ahbp_reg4d_t*)(SOC_SYS_AHBP_REG_BASE + (0x4d << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg4d_h26d_picc_end         (uint32_t v) {
	sys_ahbp_reg4d_t *r = (sys_ahbp_reg4d_t*)(SOC_SYS_AHBP_REG_BASE + (0x4d << 2));
	r->h26d_picc_end          = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4d_h26d_picc_end         (void) {
	sys_ahbp_reg4d_t *r = (sys_ahbp_reg4d_t*)(SOC_SYS_AHBP_REG_BASE + (0x4d << 2));
	return r->h26d_picc_end         ;
}

//reg reg4e:

static inline void sys_ahbp_ll_set_reg4e_value(uint32_t v) {
	sys_ahbp_reg4e_t *r = (sys_ahbp_reg4e_t*)(SOC_SYS_AHBP_REG_BASE + (0x4e << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4e_value(void) {
	sys_ahbp_reg4e_t *r = (sys_ahbp_reg4e_t*)(SOC_SYS_AHBP_REG_BASE + (0x4e << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg4e_reserved_0_31(void) {
	sys_ahbp_reg4e_t *r = (sys_ahbp_reg4e_t*)(SOC_SYS_AHBP_REG_BASE + (0x4e << 2));
	return r->reserved_0_31;
}

//reg reg4f:

static inline void sys_ahbp_ll_set_reg4f_value(uint32_t v) {
	sys_ahbp_reg4f_t *r = (sys_ahbp_reg4f_t*)(SOC_SYS_AHBP_REG_BASE + (0x4f << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg4f_value(void) {
	sys_ahbp_reg4f_t *r = (sys_ahbp_reg4f_t*)(SOC_SYS_AHBP_REG_BASE + (0x4f << 2));
	return r->v;
}

static inline uint32_t sys_ahbp_ll_get_reg4f_h26d_buffc_remap_addr(void) {
	sys_ahbp_reg4f_t *r = (sys_ahbp_reg4f_t*)(SOC_SYS_AHBP_REG_BASE + (0x4f << 2));
	return r->h26d_buffc_remap_addr;
}

//reg reg50:

static inline void sys_ahbp_ll_set_reg50_value(uint32_t v) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg50_value(void) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg50_ram_spsh_cfg(uint32_t v) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	r->ram_spsh_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_reg50_ram_spsh_cfg(void) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	return r->ram_spsh_cfg;
}

static inline void sys_ahbp_ll_set_reg50_ram_spbh_cfg(uint32_t v) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	r->ram_spbh_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_reg50_ram_spbh_cfg(void) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	return r->ram_spbh_cfg;
}

static inline void sys_ahbp_ll_set_reg50_ram_spsh_set_key(uint32_t v) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	r->ram_spsh_set_key = v;
}

static inline uint32_t sys_ahbp_ll_get_reg50_ram_spsh_set_key(void) {
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t*)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));
	return r->ram_spsh_set_key;
}

//reg reg51:

static inline void sys_ahbp_ll_set_reg51_value(uint32_t v) {
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t*)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg51_value(void) {
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t*)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg51_ram_stph_cfg(uint32_t v) {
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t*)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));
	r->ram_stph_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_reg51_ram_stph_cfg(void) {
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t*)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));
	return r->ram_stph_cfg;
}

static inline void sys_ahbp_ll_set_reg51_ram_stph_set_key(uint32_t v) {
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t*)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));
	r->ram_stph_set_key = v;
}

static inline uint32_t sys_ahbp_ll_get_reg51_ram_stph_set_key(void) {
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t*)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));
	return r->ram_stph_set_key;
}

//reg reg52:

static inline void sys_ahbp_ll_set_reg52_value(uint32_t v) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg52_value(void) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg52_ram_spsl_cfg(uint32_t v) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	r->ram_spsl_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_reg52_ram_spsl_cfg(void) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	return r->ram_spsl_cfg;
}

static inline void sys_ahbp_ll_set_reg52_ram_spbl_cfg(uint32_t v) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	r->ram_spbl_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_reg52_ram_spbl_cfg(void) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	return r->ram_spbl_cfg;
}

static inline void sys_ahbp_ll_set_reg52_ram_spsl_set_key(uint32_t v) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	r->ram_spsl_set_key = v;
}

static inline uint32_t sys_ahbp_ll_get_reg52_ram_spsl_set_key(void) {
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t*)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));
	return r->ram_spsl_set_key;
}

//reg reg53:

static inline void sys_ahbp_ll_set_reg53_value(uint32_t v) {
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t*)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));
	r->v = v;
}

static inline uint32_t sys_ahbp_ll_get_reg53_value(void) {
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t*)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));
	return r->v;
}

static inline void sys_ahbp_ll_set_reg53_ram_stpl_cfg(uint32_t v) {
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t*)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));
	r->ram_stpl_cfg = v;
}

static inline uint32_t sys_ahbp_ll_get_reg53_ram_stpl_cfg(void) {
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t*)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));
	return r->ram_stpl_cfg;
}

static inline void sys_ahbp_ll_set_reg53_ram_stpl_set_key(uint32_t v) {
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t*)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));
	r->ram_stpl_set_key = v;
}

static inline uint32_t sys_ahbp_ll_get_reg53_ram_stpl_set_key(void) {
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t*)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));
	return r->ram_stpl_set_key;
}
#ifdef __cplusplus
}
#endif
