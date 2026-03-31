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
#include "xdac0_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XDAC0_LL_REG_BASE   SOC_XDAC0_REG_BASE

//reg reg0:

static inline void xdac0_ll_set_reg0_value(uint32_t v) {
	xdac0_reg0_t *r = (xdac0_reg0_t*)(SOC_XDAC0_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg0_value(void) {
	xdac0_reg0_t *r = (xdac0_reg0_t*)(SOC_XDAC0_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t xdac0_ll_get_reg0_deviceid(void) {
	xdac0_reg0_t *r = (xdac0_reg0_t*)(SOC_XDAC0_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg reg1:

static inline void xdac0_ll_set_reg1_value(uint32_t v) {
	xdac0_reg1_t *r = (xdac0_reg1_t*)(SOC_XDAC0_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg1_value(void) {
	xdac0_reg1_t *r = (xdac0_reg1_t*)(SOC_XDAC0_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t xdac0_ll_get_reg1_versionid(void) {
	xdac0_reg1_t *r = (xdac0_reg1_t*)(SOC_XDAC0_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg reg2:

static inline void xdac0_ll_set_reg2_value(uint32_t v) {
	xdac0_reg2_t *r = (xdac0_reg2_t*)(SOC_XDAC0_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg2_value(void) {
	xdac0_reg2_t *r = (xdac0_reg2_t*)(SOC_XDAC0_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void xdac0_ll_set_reg2_soft_reset(uint32_t v) {
	xdac0_reg2_t *r = (xdac0_reg2_t*)(SOC_XDAC0_REG_BASE + (0x2 << 2));
	r->soft_reset = v;
}

static inline uint32_t xdac0_ll_get_reg2_soft_reset(void) {
	xdac0_reg2_t *r = (xdac0_reg2_t*)(SOC_XDAC0_REG_BASE + (0x2 << 2));
	return r->soft_reset;
}

static inline void xdac0_ll_set_reg2_clkg_bypass(uint32_t v) {
	xdac0_reg2_t *r = (xdac0_reg2_t*)(SOC_XDAC0_REG_BASE + (0x2 << 2));
	r->clkg_bypass = v;
}

static inline uint32_t xdac0_ll_get_reg2_clkg_bypass(void) {
	xdac0_reg2_t *r = (xdac0_reg2_t*)(SOC_XDAC0_REG_BASE + (0x2 << 2));
	return r->clkg_bypass;
}

//reg reg3:

static inline void xdac0_ll_set_reg3_value(uint32_t v) {
	xdac0_reg3_t *r = (xdac0_reg3_t*)(SOC_XDAC0_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg3_value(void) {
	xdac0_reg3_t *r = (xdac0_reg3_t*)(SOC_XDAC0_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t xdac0_ll_get_reg3_devstatus(void) {
	xdac0_reg3_t *r = (xdac0_reg3_t*)(SOC_XDAC0_REG_BASE + (0x3 << 2));
	return r->devstatus;
}

//reg reg4:

static inline void xdac0_ll_set_reg4_value(uint32_t v) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg4_value(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void xdac0_ll_set_reg4_dac_enable(uint32_t v) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	r->dac_enable = v;
}

static inline uint32_t xdac0_ll_get_reg4_dac_enable(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->dac_enable;
}

static inline void xdac0_ll_set_reg4_dac_clk_en(uint32_t v) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	r->dac_clk_en = v;
}

static inline uint32_t xdac0_ll_get_reg4_dac_clk_en(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->dac_clk_en;
}

static inline void xdac0_ll_set_reg4_dac_mode(uint32_t v) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	r->dac_mode = v;
}

static inline uint32_t xdac0_ll_get_reg4_dac_mode(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->dac_mode;
}

static inline void xdac0_ll_set_reg4_fifo_enable(uint32_t v) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	r->fifo_enable = v;
}

static inline uint32_t xdac0_ll_get_reg4_fifo_enable(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->fifo_enable;
}

static inline uint32_t xdac0_ll_get_reg4_reserved_4_15(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->reserved_4_15;
}

static inline void xdac0_ll_set_reg4_dac_clk_div(uint32_t v) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	r->dac_clk_div = v;
}

static inline uint32_t xdac0_ll_get_reg4_dac_clk_div(void) {
	xdac0_reg4_t *r = (xdac0_reg4_t*)(SOC_XDAC0_REG_BASE + (0x4 << 2));
	return r->dac_clk_div;
}

//reg reg5:

static inline void xdac0_ll_set_reg5_value(uint32_t v) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg5_value(void) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline uint32_t xdac0_ll_get_reg5_fifo_empty_int(void) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	return r->fifo_empty_int;
}

static inline uint32_t xdac0_ll_get_reg5_fifo_full_int(void) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	return r->fifo_full_int;
}

static inline uint32_t xdac0_ll_get_reg5_fifo_near_full_int(void) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	return r->fifo_near_full_int;
}

static inline uint32_t xdac0_ll_get_reg5_fifo_near_empty_int(void) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	return r->fifo_near_empty_int;
}

static inline uint32_t xdac0_ll_get_reg5_reserved_4_31(void) {
	xdac0_reg5_t *r = (xdac0_reg5_t*)(SOC_XDAC0_REG_BASE + (0x5 << 2));
	return r->reserved_4_31;
}

//reg reg6:

static inline void xdac0_ll_set_reg6_value(uint32_t v) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg6_value(void) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void xdac0_ll_set_reg6_fifo_empty_int_en(uint32_t v) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	r->fifo_empty_int_en = v;
}

static inline uint32_t xdac0_ll_get_reg6_fifo_empty_int_en(void) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	return r->fifo_empty_int_en;
}

static inline void xdac0_ll_set_reg6_fifo_full_int_en(uint32_t v) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	r->fifo_full_int_en = v;
}

static inline uint32_t xdac0_ll_get_reg6_fifo_full_int_en(void) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	return r->fifo_full_int_en;
}

static inline void xdac0_ll_set_reg6_fifo_near_full_int_en(uint32_t v) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	r->fifo_near_full_int_en = v;
}

static inline uint32_t xdac0_ll_get_reg6_fifo_near_full_int_en(void) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	return r->fifo_near_full_int_en;
}

static inline void xdac0_ll_set_reg6_fifo_near_empty_int_en(uint32_t v) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	r->fifo_near_empty_int_en = v;
}

static inline uint32_t xdac0_ll_get_reg6_fifo_near_empty_int_en(void) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	return r->fifo_near_empty_int_en;
}

static inline void xdac0_ll_set_reg6_reserved_4_31(uint32_t v) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	r->reserved_4_31 = v;
}

static inline uint32_t xdac0_ll_get_reg6_reserved_4_31(void) {
	xdac0_reg6_t *r = (xdac0_reg6_t*)(SOC_XDAC0_REG_BASE + (0x6 << 2));
	return r->reserved_4_31;
}

//reg reg7:

static inline void xdac0_ll_set_reg7_value(uint32_t v) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg7_value(void) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline void xdac0_ll_set_reg7_dac_rthrd(uint32_t v) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	r->dac_rthrd = v;
}

static inline uint32_t xdac0_ll_get_reg7_dac_rthrd(void) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	return r->dac_rthrd;
}

static inline void xdac0_ll_set_reg7_dac_wthrd(uint32_t v) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	r->dac_wthrd = v;
}

static inline uint32_t xdac0_ll_get_reg7_dac_wthrd(void) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	return r->dac_wthrd;
}

static inline uint32_t xdac0_ll_get_reg7_reserved_10_31(void) {
	xdac0_reg7_t *r = (xdac0_reg7_t*)(SOC_XDAC0_REG_BASE + (0x7 << 2));
	return r->reserved_10_31;
}

//reg reg8:

static inline void xdac0_ll_set_reg8_value(uint32_t v) {
	xdac0_reg8_t *r = (xdac0_reg8_t*)(SOC_XDAC0_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t xdac0_ll_get_reg8_value(void) {
	xdac0_reg8_t *r = (xdac0_reg8_t*)(SOC_XDAC0_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void xdac0_ll_set_reg8_tx_fifo_wr_data(uint32_t v) {
	xdac0_reg8_t *r = (xdac0_reg8_t*)(SOC_XDAC0_REG_BASE + (0x8 << 2));
	r->tx_fifo_wr_data = v;
}

static inline uint32_t xdac0_ll_get_reg8_tx_fifo_wr_data(void) {
	xdac0_reg8_t *r = (xdac0_reg8_t*)(SOC_XDAC0_REG_BASE + (0x8 << 2));
	return r->tx_fifo_wr_data;
}

static inline uint32_t xdac0_ll_get_reg8_reserved_12_31(void) {
	xdac0_reg8_t *r = (xdac0_reg8_t*)(SOC_XDAC0_REG_BASE + (0x8 << 2));
	return r->reserved_12_31;
}
#ifdef __cplusplus
}
#endif
