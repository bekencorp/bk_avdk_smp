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
#include "wwdt_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WWDT_LL_REG_BASE   SOC_WWDT_REG_BASE

static inline void wwdt_ll_reset_config_to_default(wwdt_hw_t *hw)
{
	hw->wdt_win_set.win_en = 0;
	hw->wdt_win_set.win_val = WWDT_V_PERIOD_DEFAULT_VALUE;
}

//reg smb_devid:
static inline void wwdt_ll_set_smb_devid_value(uint32_t v) {
	wwdt_smb_devid_t *r = (wwdt_smb_devid_t*)(SOC_WWDT_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_smb_devid_value(void) {
	wwdt_smb_devid_t *r = (wwdt_smb_devid_t*)(SOC_WWDT_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline void wwdt_ll_set_smb_devid_device_id(uint32_t v) {
	wwdt_smb_devid_t *r = (wwdt_smb_devid_t*)(SOC_WWDT_REG_BASE + (0x0 << 2));
	r->device_id = v;
}

//reg smb_verid:
static inline void wwdt_ll_set_smb_verid_value(uint32_t v) {
	wwdt_smb_verid_t *r = (wwdt_smb_verid_t*)(SOC_WWDT_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_smb_verid_value(void) {
	wwdt_smb_verid_t *r = (wwdt_smb_verid_t*)(SOC_WWDT_REG_BASE + (0x1 << 2));
	return r->v;
}

//reg smb_clkrst:
static inline void wwdt_ll_set_smb_clkrst_value(uint32_t v) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_smb_clkrst_value(void) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void wwdt_ll_set_smb_clkrst_clkg_bypass(uint32_t v) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	r->clkg_bypass = v;
}

static inline uint32_t wwdt_ll_get_smb_clkrst_clkg_bypass(void) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	return r->clkg_bypass;
}

static inline void wwdt_ll_set_smb_clkrst_soft_reset(uint32_t v) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	r->soft_reset = v;
}

static inline void wwdt_ll_set_smb_clkrst_resv(uint32_t v) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	r->resv = v;
}

static inline uint32_t wwdt_ll_get_smb_clkrst_resv(void) {
	wwdt_smb_clkrst_t *r = (wwdt_smb_clkrst_t*)(SOC_WWDT_REG_BASE + (0x2 << 2));
	return r->resv;
}

//reg smb_state:

static inline void wwdt_ll_set_smb_state_value(uint32_t v) {
	wwdt_smb_state_t *r = (wwdt_smb_state_t*)(SOC_WWDT_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_smb_state_value(void) {
	wwdt_smb_state_t *r = (wwdt_smb_state_t*)(SOC_WWDT_REG_BASE + (0x3 << 2));
	return r->v;
}

//reg wdt_config:
static inline void wwdt_ll_1st_set_wdt_config_period(uint32_t v) {
	uint32_t val;
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));

	val = (WWDT_V_KEY_1ST & WWDT_WDT_CONFIG_KEY_MASK) << WWDT_WDT_CONFIG_KEY_POS;
	val |= (v & WWDT_WDT_CONFIG_PERIOD_MASK) << WWDT_WDT_CONFIG_PERIOD_POS;
	r->v = val;
}

static inline void wwdt_ll_2nd_set_wdt_config_period(uint32_t v) {
	uint32_t val;
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));

	val = (WWDT_V_KEY_2ND & WWDT_WDT_CONFIG_KEY_MASK) << WWDT_WDT_CONFIG_KEY_POS;
	val |= (v & WWDT_WDT_CONFIG_PERIOD_MASK) << WWDT_WDT_CONFIG_PERIOD_POS;
	r->v = val;
}

static inline void wwdt_ll_set_wdt_config_value(uint32_t v) {
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_wdt_config_value(void) {
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void wwdt_ll_set_wdt_config_period(uint32_t v) {
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));
	r->period = v;
}

static inline uint32_t wwdt_ll_get_wdt_config_period(void) {
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));
	return r->period;
}

static inline void wwdt_ll_set_wdt_config_key(uint32_t v) {
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));
	r->key = v;
}

static inline uint32_t wwdt_ll_get_wdt_config_key(void) {
	wwdt_wdt_config_t *r = (wwdt_wdt_config_t*)(SOC_WWDT_REG_BASE + (0x4 << 2));
	return r->key;
}

//reg wdt_cnt:

static inline void wwdt_ll_set_wdt_cnt_value(uint32_t v) {
	wwdt_wdt_cnt_t *r = (wwdt_wdt_cnt_t*)(SOC_WWDT_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_wdt_cnt_value(void) {
	wwdt_wdt_cnt_t *r = (wwdt_wdt_cnt_t*)(SOC_WWDT_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void wwdt_ll_set_wdt_cnt_count(uint32_t v) {
	wwdt_wdt_cnt_t *r = (wwdt_wdt_cnt_t*)(SOC_WWDT_REG_BASE + (0x5 << 2));
	r->count = v;
}

//reg wdt_win_set:

static inline void wwdt_ll_set_wdt_win_set_value(uint32_t v) {
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_wdt_win_set_value(void) {
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void wwdt_ll_set_wdt_win_set_win_val(uint32_t v) {
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));
	r->win_val = v;
}

static inline uint32_t wwdt_ll_get_wdt_win_set_win_val(void) {
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));
	return r->win_val;
}

static inline void wwdt_ll_set_wdt_win_1st_set_win_val(uint32_t v) {
	uint32_t val;
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));

	val = (v & WWDT_WDT_WIN_SET_WIN_VAL_MASK) << WWDT_WDT_WIN_SET_WIN_VAL_POS;
	val |= (WWDT_V_KEY_1ST & WWDT_WDT_WIN_SET_WIN_KEY_MASK) << WWDT_WDT_WIN_SET_WIN_KEY_POS;
	r->v = val;
}

static inline void wwdt_ll_set_wdt_win_2nd_set_win_val(uint32_t v) {
	uint32_t val;
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));

	val = (v & WWDT_WDT_WIN_SET_WIN_VAL_MASK) << WWDT_WDT_WIN_SET_WIN_VAL_POS;
	val |= (WWDT_V_KEY_2ND & WWDT_WDT_WIN_SET_WIN_KEY_MASK) << WWDT_WDT_WIN_SET_WIN_KEY_POS;
	r->v = val;
}

static inline void wwdt_ll_set_wdt_win_set_win_key(uint32_t v) {
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));
	r->win_key = v;
}

static inline uint32_t wwdt_ll_get_wdt_win_set_win_key(void) {
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));
	return r->win_key;
}

static inline void wwdt_ll_set_wdt_win_1st_set_win_en(uint32_t v) {
	uint32_t val;
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));

	val = (v & WWDT_WDT_WIN_SET_WIN_EN_MASK) << WWDT_WDT_WIN_SET_WIN_EN_POS;
	val |= (WWDT_V_KEY_1ST & WWDT_WDT_WIN_SET_WIN_KEY_MASK) << WWDT_WDT_WIN_SET_WIN_KEY_POS;
	r->v = val;
}

static inline void wwdt_ll_set_wdt_win_2nd_set_win_en(uint32_t v) {
	uint32_t val;
	wwdt_wdt_win_set_t *r = (wwdt_wdt_win_set_t*)(SOC_WWDT_REG_BASE + (0x6 << 2));

	val = (v & WWDT_WDT_WIN_SET_WIN_EN_MASK) << WWDT_WDT_WIN_SET_WIN_EN_POS;
	val |= (WWDT_V_KEY_2ND & WWDT_WDT_WIN_SET_WIN_KEY_MASK) << WWDT_WDT_WIN_SET_WIN_KEY_POS;
	r->v = val;
}

//reg cpuid:
static inline void wwdt_ll_set_cpuid_value(uint32_t v) {
	wwdt_cpuid_t *r = (wwdt_cpuid_t*)(SOC_WWDT_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t wwdt_ll_get_cpuid_value(void) {
	wwdt_cpuid_t *r = (wwdt_cpuid_t*)(SOC_WWDT_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline uint32_t wwdt_ll_get_cpuid_cpu_id(void) {
	wwdt_cpuid_t *r = (wwdt_cpuid_t*)(SOC_WWDT_REG_BASE + (0x7 << 2));
	return r->cpu_id;
}

static inline uint32_t wwdt_ll_get_cpuid_magic_word(void) {
	wwdt_cpuid_t *r = (wwdt_cpuid_t*)(SOC_WWDT_REG_BASE + (0x7 << 2));
	return r->magic_word;
}

#ifdef __cplusplus
}
#endif
