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
#include "sys_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_AONP_LL_REG_BASE   SOC_SYS_AONP_REG_BASE

//This way of setting ana_reg_bit value is only for sys_ctrl, other driver please implement by yourself!!

#define SYS_ANALOG_REG_SPI_STATE_REG (SOC_SYS_AONP_REG_BASE + (0x3a << 2))
#define SYS_ANALOG_REG_SPI_STATE_REG1 (SOC_SYS_AONP_REG_BASE + (0x3b << 2))
#define SYS_ANALOG_REG_SPI_STATE_POS(idx) (idx)
#define SYS_ANALOG_REG_SPI_STATE1_POS (31)
#define GET_SYS_ANALOG_REG_IDX(addr) ((addr - SYS_AONP_ANA_REG0_ADDR) >> 2)

static inline uint32_t sys_ll_get_analog_reg_value(uint32_t addr)
{
	return REG_READ(addr);
}

static inline void sys_ll_set_analog_reg_value(uint32_t addr, uint32_t value)
{
	uint32_t idx;
	idx = GET_SYS_ANALOG_REG_IDX(addr);

	REG_WRITE(addr, value);

	if (idx < 32) {
		while (REG_READ(SYS_ANALOG_REG_SPI_STATE_REG) & (1U << SYS_ANALOG_REG_SPI_STATE_POS(idx)));
	} else {
		while (REG_READ(SYS_ANALOG_REG_SPI_STATE_REG1) & (1U << SYS_ANALOG_REG_SPI_STATE1_POS));
	}
}

static inline void sys_aonp_set_ana_reg_bit(uint32_t reg_addr, uint32_t pos, uint32_t mask, uint32_t value)
{
	uint32_t reg_value;
	reg_value = *(volatile uint32_t *)(reg_addr);
	reg_value &= ~(mask << pos);
	reg_value |= ((value & mask) <<pos);
	sys_ll_set_analog_reg_value(reg_addr, reg_value);
}

//reg reg0:

static inline void sys_aonp_ll_set_reg0_value(uint32_t v) {
	sys_aonp_reg0_t *r = (sys_aonp_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg0_value(void) {
	sys_aonp_reg0_t *r = (sys_aonp_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg0_deviceid(void) {
	sys_aonp_reg0_t *r = (sys_aonp_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg reg1:

static inline void sys_aonp_ll_set_reg1_value(uint32_t v) {
	sys_aonp_reg1_t *r = (sys_aonp_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg1_value(void) {
	sys_aonp_reg1_t *r = (sys_aonp_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg1_versionid(void) {
	sys_aonp_reg1_t *r = (sys_aonp_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg reg2:

static inline void sys_aonp_ll_set_reg2_value(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_value(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg2_boot_mode(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->boot_mode = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_boot_mode(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->boot_mode;
}

static inline void sys_aonp_ll_set_reg2_clkg_bps(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->clkg_bps = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_clkg_bps(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->clkg_bps;
}

static inline void sys_aonp_ll_set_reg2_rf_switch_manual_en(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->rf_switch_manual_en = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_rf_switch_manual_en(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->rf_switch_manual_en;
}

static inline void sys_aonp_ll_set_reg2_rf_source(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->rf_source = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_rf_source(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->rf_source;
}

static inline void sys_aonp_ll_set_reg2_jtag_core_sel(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->jtag_core_sel = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_jtag_core_sel(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->jtag_core_sel;
}

static inline void sys_aonp_ll_set_reg2_flash_sel(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->flash_sel = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_flash_sel(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->flash_sel;
}

static inline void sys_aonp_ll_set_reg2_fem_bps_txen(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->fem_bps_txen = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_fem_bps_txen(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->fem_bps_txen;
}

static inline void sys_aonp_ll_set_reg2_gpio_flash_sys_enable(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->gpio_flash_sys_enable = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_gpio_flash_sys_enable(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->gpio_flash_sys_enable;
}

static inline void sys_aonp_ll_set_reg2_boot_mode_norst(uint32_t v) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	r->boot_mode_norst = v;
}

static inline uint32_t sys_aonp_ll_get_reg2_boot_mode_norst(void) {
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));
	return r->boot_mode_norst;
}

//reg reg3:

static inline void sys_aonp_ll_set_reg3_value(uint32_t v) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3_value(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg3_core0_halted(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->core0_halted;
}

static inline uint32_t sys_aonp_ll_get_reg3_core1_halted(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->core1_halted;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu0_sw_reset(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu0_sw_reset;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu1_sw_reset(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu1_sw_reset;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu0_pwr_dw_state(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu0_pwr_dw_state;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu1_pwr_dw_state(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu1_pwr_dw_state;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu0_exist(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu0_exist;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu1_exist(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu1_exist;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu2_exist(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu2_exist;
}

static inline uint32_t sys_aonp_ll_get_reg3_cpu3_exist(void) {
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));
	return r->cpu3_exist;
}

//reg reg4:

static inline void sys_aonp_ll_set_reg4_value(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_value(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg4_cpu0_sw_rst(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->cpu0_sw_rst = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_cpu0_sw_rst(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->cpu0_sw_rst;
}

static inline void sys_aonp_ll_set_reg4_cpu0_pwr_dw(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->cpu0_pwr_dw = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_cpu0_pwr_dw(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->cpu0_pwr_dw;
}

static inline void sys_aonp_ll_set_reg4_cpu_int_mask(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->cpu_int_mask = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_cpu_int_mask(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->cpu_int_mask;
}

static inline void sys_aonp_ll_set_reg4_cpu0_halt(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->cpu0_halt = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_cpu0_halt(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->cpu0_halt;
}

static inline void sys_aonp_ll_set_reg4_cpu0_rxevt_sel(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->cpu0_rxevt_sel = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_cpu0_rxevt_sel(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->cpu0_rxevt_sel;
}

static inline void sys_aonp_ll_set_reg4_reserved_7_7(uint32_t v) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_aonp_ll_get_reg4_reserved_7_7(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->reserved_7_7;
}

static inline uint32_t sys_aonp_ll_get_reg4_cpu0_offset(void) {
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));
	return r->cpu0_offset;
}

//reg reg5:

static inline void sys_aonp_ll_set_reg5_value(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_value(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg5_cpu1_sw_rst(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->cpu1_sw_rst = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_cpu1_sw_rst(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->cpu1_sw_rst;
}

static inline void sys_aonp_ll_set_reg5_cpu1_pwr_dw(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->cpu1_pwr_dw = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_cpu1_pwr_dw(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->cpu1_pwr_dw;
}

static inline void sys_aonp_ll_set_reg5_reserved_2_2(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->reserved_2_2 = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_reserved_2_2(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->reserved_2_2;
}

static inline void sys_aonp_ll_set_reg5_cpu1_halt(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->cpu1_halt = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_cpu1_halt(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->cpu1_halt;
}

static inline void sys_aonp_ll_set_reg5_cpu1_rxevt_sel(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->cpu1_rxevt_sel = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_cpu1_rxevt_sel(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->cpu1_rxevt_sel;
}

static inline void sys_aonp_ll_set_reg5_reserved_7_7(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_reserved_7_7(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->reserved_7_7;
}

static inline void sys_aonp_ll_set_reg5_cpu1_offset(uint32_t v) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	r->cpu1_offset = v;
}

static inline uint32_t sys_aonp_ll_get_reg5_cpu1_offset(void) {
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));
	return r->cpu1_offset;
}

//reg reg8:

static inline void sys_aonp_ll_set_reg8_value(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_value(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg8_cksel_core(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->cksel_core = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_cksel_core(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->cksel_core;
}

static inline void sys_aonp_ll_set_reg8_ckdiv_core(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->ckdiv_core = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_ckdiv_core(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->ckdiv_core;
}

static inline void sys_aonp_ll_set_reg8_cksel_flash(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->cksel_flash = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_cksel_flash(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->cksel_flash;
}

static inline void sys_aonp_ll_set_reg8_ckdiv_flash(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->ckdiv_flash = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_ckdiv_flash(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->ckdiv_flash;
}

static inline void sys_aonp_ll_set_reg8_cksel_auxs(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->cksel_auxs = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_cksel_auxs(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->cksel_auxs;
}

static inline void sys_aonp_ll_set_reg8_ckdiv_auxs(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->ckdiv_auxs = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_ckdiv_auxs(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->ckdiv_auxs;
}

static inline void sys_aonp_ll_set_reg8_ckdiv_26mo(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->ckdiv_26mo = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_ckdiv_26mo(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->ckdiv_26mo;
}

static inline void sys_aonp_ll_set_reg8_reserved_22_23(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->reserved_22_23 = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_reserved_22_23(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->reserved_22_23;
}

static inline void sys_aonp_ll_set_reg8_phase_cfg_960m(uint32_t v) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	r->phase_cfg_960m = v;
}

static inline uint32_t sys_aonp_ll_get_reg8_phase_cfg_960m(void) {
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));
	return r->phase_cfg_960m;
}

//reg reg9:

static inline void sys_aonp_ll_set_reg9_value(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_value(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2c0(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2c0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2c0(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2c0;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2c3(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2c3 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2c3(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2c3;
}

static inline void sys_aonp_ll_set_reg9_cksel_uart0(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_uart0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_uart0(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_uart0;
}

static inline void sys_aonp_ll_set_reg9_cksel_uart1(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_uart1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_uart1(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_uart1;
}

static inline void sys_aonp_ll_set_reg9_cksel_uart2(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_uart2 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_uart2(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_uart2;
}

static inline void sys_aonp_ll_set_reg9_cksel_uart3(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_uart3 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_uart3(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_uart3;
}

static inline void sys_aonp_ll_set_reg9_cksel_uart4(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_uart4 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_uart4(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_uart4;
}

static inline void sys_aonp_ll_set_reg9_cksel_spi0(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_spi0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_spi0(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_spi0;
}

static inline void sys_aonp_ll_set_reg9_cksel_spi1(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_spi1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_spi1(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_spi1;
}

static inline void sys_aonp_ll_set_reg9_cksel_spi2(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_spi2 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_spi2(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_spi2;
}

static inline void sys_aonp_ll_set_reg9_cksel_spi3(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_spi3 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_spi3(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_spi3;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2s0(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2s0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2s0(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2s0;
}

static inline void sys_aonp_ll_set_reg9_ckdiv_i2s0(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_ckdiv_i2s0(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s0;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2s1(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2s1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2s1(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2s1;
}

static inline void sys_aonp_ll_set_reg9_ckdiv_i2s1(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_ckdiv_i2s1(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s1;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2s2(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2s2 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2s2(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2s2;
}

static inline void sys_aonp_ll_set_reg9_ckdiv_i2s2(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s2 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_ckdiv_i2s2(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s2;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2s3(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2s3 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2s3(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2s3;
}

static inline void sys_aonp_ll_set_reg9_ckdiv_i2s3(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s3 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_ckdiv_i2s3(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s3;
}

static inline void sys_aonp_ll_set_reg9_cksel_i2s4(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i2s4 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i2s4(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i2s4;
}

static inline void sys_aonp_ll_set_reg9_ckdiv_i2s4(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s4 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_ckdiv_i2s4(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s4;
}

static inline void sys_aonp_ll_set_reg9_cksel_sadc(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_sadc = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_sadc(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_sadc;
}

static inline void sys_aonp_ll_set_reg9_cksel_i3c(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_i3c = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_i3c(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_i3c;
}

static inline void sys_aonp_ll_set_reg9_cksel_tim0(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_tim0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_tim0(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_tim0;
}

static inline void sys_aonp_ll_set_reg9_cksel_tim1(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_tim1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_tim1(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_tim1;
}

static inline void sys_aonp_ll_set_reg9_cksel_tim2(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_tim2 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_tim2(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_tim2;
}

static inline void sys_aonp_ll_set_reg9_cksel_tim3(uint32_t v) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	r->cksel_tim3 = v;
}

static inline uint32_t sys_aonp_ll_get_reg9_cksel_tim3(void) {
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));
	return r->cksel_tim3;
}

//reg rega:

static inline void sys_aonp_ll_set_rega_value(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_rega_value(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_rega_cksel_pwm0(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_pwm0 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_pwm0(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_pwm0;
}

static inline void sys_aonp_ll_set_rega_cksel_can0(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_can0 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_can0(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_can0;
}

static inline void sys_aonp_ll_set_rega_cksel_can1(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_can1 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_can1(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_can1;
}

static inline void sys_aonp_ll_set_rega_cksel_scr0(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_scr0 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_scr0(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_scr0;
}

static inline void sys_aonp_ll_set_rega_cksel_audio(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_audio = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_audio(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_audio;
}

static inline void sys_aonp_ll_set_rega_ckdiv_audio(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->ckdiv_audio = v;
}

static inline uint32_t sys_aonp_ll_get_rega_ckdiv_audio(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->ckdiv_audio;
}

static inline void sys_aonp_ll_set_rega_cksel_audif0(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_audif0 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_audif0(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_audif0;
}

static inline void sys_aonp_ll_set_rega_ckdiv_audif0(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->ckdiv_audif0 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_ckdiv_audif0(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->ckdiv_audif0;
}

static inline void sys_aonp_ll_set_rega_cksel_audif1(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_audif1 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_audif1(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_audif1;
}

static inline void sys_aonp_ll_set_rega_ckdiv_audif1(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->ckdiv_audif1 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_ckdiv_audif1(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->ckdiv_audif1;
}

static inline void sys_aonp_ll_set_rega_ckdiv_i2so(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->ckdiv_i2so = v;
}

static inline uint32_t sys_aonp_ll_get_rega_ckdiv_i2so(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->ckdiv_i2so;
}

static inline void sys_aonp_ll_set_rega_cksel_auxs_enet(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_auxs_enet = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_auxs_enet(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_auxs_enet;
}

static inline void sys_aonp_ll_set_rega_ckdiv_auxs_enet(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->ckdiv_auxs_enet = v;
}

static inline uint32_t sys_aonp_ll_get_rega_ckdiv_auxs_enet(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->ckdiv_auxs_enet;
}

static inline void sys_aonp_ll_set_rega_cksel_trace(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->cksel_trace = v;
}

static inline uint32_t sys_aonp_ll_get_rega_cksel_trace(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->cksel_trace;
}

static inline void sys_aonp_ll_set_rega_ckdiv_trace(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->ckdiv_trace = v;
}

static inline uint32_t sys_aonp_ll_get_rega_ckdiv_trace(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->ckdiv_trace;
}

static inline void sys_aonp_ll_set_rega_reserved_28_31(uint32_t v) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	r->reserved_28_31 = v;
}

static inline uint32_t sys_aonp_ll_get_rega_reserved_28_31(void) {
	sys_aonp_rega_t *r = (sys_aonp_rega_t*)(SOC_SYS_AONP_REG_BASE + (0xa << 2));
	return r->reserved_28_31;
}

//reg regb:

static inline void sys_aonp_ll_set_regb_value(uint32_t v) {
	sys_aonp_regb_t *r = (sys_aonp_regb_t*)(SOC_SYS_AONP_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_regb_value(void) {
	sys_aonp_regb_t *r = (sys_aonp_regb_t*)(SOC_SYS_AONP_REG_BASE + (0xb << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_regb_anaspi_freq(uint32_t v) {
	sys_aonp_regb_t *r = (sys_aonp_regb_t*)(SOC_SYS_AONP_REG_BASE + (0xb << 2));
	r->anaspi_freq = v;
}

static inline uint32_t sys_aonp_ll_get_regb_anaspi_freq(void) {
	sys_aonp_regb_t *r = (sys_aonp_regb_t*)(SOC_SYS_AONP_REG_BASE + (0xb << 2));
	return r->anaspi_freq;
}

//reg regc:

static inline void sys_aonp_ll_set_regc_value(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_regc_value(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_regc_tim0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->tim0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_tim0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->tim0_cken;
}

static inline void sys_aonp_ll_set_regc_tim1_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->tim1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_tim1_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->tim1_cken;
}

static inline void sys_aonp_ll_set_regc_tim2_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->tim2_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_tim2_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->tim2_cken;
}

static inline void sys_aonp_ll_set_regc_tim3_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->tim3_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_tim3_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->tim3_cken;
}

static inline void sys_aonp_ll_set_regc_uart0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->uart0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_uart0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->uart0_cken;
}

static inline void sys_aonp_ll_set_regc_uart1_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->uart1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_uart1_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->uart1_cken;
}

static inline void sys_aonp_ll_set_regc_uart2_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->uart2_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_uart2_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->uart2_cken;
}

static inline void sys_aonp_ll_set_regc_uart3_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->uart3_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_uart3_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->uart3_cken;
}

static inline void sys_aonp_ll_set_regc_uart4_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->uart4_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_uart4_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->uart4_cken;
}

static inline void sys_aonp_ll_set_regc_spi0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->spi0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_spi0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->spi0_cken;
}

static inline void sys_aonp_ll_set_regc_spi1_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->spi1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_spi1_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->spi1_cken;
}

static inline void sys_aonp_ll_set_regc_spi2_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->spi2_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_spi2_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->spi2_cken;
}

static inline void sys_aonp_ll_set_regc_spi3_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->spi3_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_spi3_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->spi3_cken;
}

static inline void sys_aonp_ll_set_regc_sadc_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->sadc_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_sadc_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->sadc_cken;
}

static inline void sys_aonp_ll_set_regc_pwm0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->pwm0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_pwm0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->pwm0_cken;
}

static inline void sys_aonp_ll_set_regc_otp_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->otp_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_otp_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->otp_cken;
}

static inline void sys_aonp_ll_set_regc_i3c_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i3c_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i3c_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i3c_cken;
}

static inline void sys_aonp_ll_set_regc_i2s0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2s0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2s0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2s0_cken;
}

static inline void sys_aonp_ll_set_regc_i2s1_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2s1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2s1_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2s1_cken;
}

static inline void sys_aonp_ll_set_regc_i2s2_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2s2_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2s2_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2s2_cken;
}

static inline void sys_aonp_ll_set_regc_i2s3_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2s3_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2s3_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2s3_cken;
}

static inline void sys_aonp_ll_set_regc_i2s4_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2s4_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2s4_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2s4_cken;
}

static inline void sys_aonp_ll_set_regc_i2c0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2c0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2c0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2c0_cken;
}

static inline void sys_aonp_ll_set_regc_i2c3_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->i2c3_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_i2c3_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->i2c3_cken;
}

static inline void sys_aonp_ll_set_regc_irda0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->irda0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_irda0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->irda0_cken;
}

static inline void sys_aonp_ll_set_regc_irda1_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->irda1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_irda1_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->irda1_cken;
}

static inline void sys_aonp_ll_set_regc_irda2_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->irda2_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_irda2_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->irda2_cken;
}

static inline void sys_aonp_ll_set_regc_irda3_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->irda3_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_irda3_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->irda3_cken;
}

static inline void sys_aonp_ll_set_regc_can0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->can0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_can0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->can0_cken;
}

static inline void sys_aonp_ll_set_regc_can1_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->can1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_can1_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->can1_cken;
}

static inline void sys_aonp_ll_set_regc_lin0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->lin0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_lin0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->lin0_cken;
}

static inline void sys_aonp_ll_set_regc_scr0_cken(uint32_t v) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	r->scr0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regc_scr0_cken(void) {
	sys_aonp_regc_t *r = (sys_aonp_regc_t*)(SOC_SYS_AONP_REG_BASE + (0xc << 2));
	return r->scr0_cken;
}

//reg regd:

static inline void sys_aonp_ll_set_regd_value(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_regd_value(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_regd_audio_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->audio_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_audio_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->audio_cken;
}

static inline void sys_aonp_ll_set_regd_audif0_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->audif0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_audif0_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->audif0_cken;
}

static inline void sys_aonp_ll_set_regd_audif1_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->audif1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_audif1_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->audif1_cken;
}

static inline void sys_aonp_ll_set_regd_i2so_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->i2so_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_i2so_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->i2so_cken;
}

static inline void sys_aonp_ll_set_regd_cec_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->cec_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_cec_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->cec_cken;
}

static inline void sys_aonp_ll_set_regd_xdac0_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->xdac0_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_xdac0_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->xdac0_cken;
}

static inline void sys_aonp_ll_set_regd_xdac1_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->xdac1_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_xdac1_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->xdac1_cken;
}

static inline void sys_aonp_ll_set_regd_auxs_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->auxs_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_auxs_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->auxs_cken;
}

static inline void sys_aonp_ll_set_regd_auxs_enet_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->auxs_enet_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_auxs_enet_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->auxs_enet_cken;
}

static inline void sys_aonp_ll_set_regd_sig_26ms_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_26ms_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_26ms_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_26ms_cken;
}

static inline void sys_aonp_ll_set_regd_sig_32ks_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_32ks_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_32ks_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_32ks_cken;
}

static inline void sys_aonp_ll_set_regd_sig_26mo_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_26mo_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_26mo_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_26mo_cken;
}

static inline void sys_aonp_ll_set_regd_sig_240m_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_240m_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_240m_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_240m_cken;
}

static inline void sys_aonp_ll_set_regd_sig_320m_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_320m_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_320m_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_320m_cken;
}

static inline void sys_aonp_ll_set_regd_sig_480m_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_480m_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_480m_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_480m_cken;
}

static inline void sys_aonp_ll_set_regd_sig_160m_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_160m_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_160m_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_160m_cken;
}

static inline void sys_aonp_ll_set_regd_sig_120m_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->sig_120m_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_sig_120m_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->sig_120m_cken;
}

static inline void sys_aonp_ll_set_regd_trace_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->trace_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_trace_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->trace_cken;
}

static inline void sys_aonp_ll_set_regd_reserved_18_21(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->reserved_18_21 = v;
}

static inline uint32_t sys_aonp_ll_get_regd_reserved_18_21(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->reserved_18_21;
}

static inline void sys_aonp_ll_set_regd_wlss_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->wlss_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_wlss_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->wlss_cken;
}

static inline void sys_aonp_ll_set_regd_btdm_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->btdm_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_btdm_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->btdm_cken;
}

static inline void sys_aonp_ll_set_regd_xver_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->xver_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_xver_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->xver_cken;
}

static inline void sys_aonp_ll_set_regd_mac_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->mac_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_mac_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->mac_cken;
}

static inline void sys_aonp_ll_set_regd_phy_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->phy_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_phy_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->phy_cken;
}

static inline void sys_aonp_ll_set_regd_thread_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->thread_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_thread_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->thread_cken;
}

static inline void sys_aonp_ll_set_regd_bk24_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->bk24_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_bk24_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->bk24_cken;
}

static inline void sys_aonp_ll_set_regd_rf_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->rf_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_rf_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->rf_cken;
}

static inline void sys_aonp_ll_set_regd_ofdm_cken(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->ofdm_cken = v;
}

static inline uint32_t sys_aonp_ll_get_regd_ofdm_cken(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->ofdm_cken;
}

static inline void sys_aonp_ll_set_regd_reserved_31_31(uint32_t v) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	r->reserved_31_31 = v;
}

static inline uint32_t sys_aonp_ll_get_regd_reserved_31_31(void) {
	sys_aonp_regd_t *r = (sys_aonp_regd_t*)(SOC_SYS_AONP_REG_BASE + (0xd << 2));
	return r->reserved_31_31;
}

//reg regf:

static inline void sys_aonp_ll_set_regf_value(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_regf_value(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_regf_reserved_0_0(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->reserved_0_0 = v;
}

static inline uint32_t sys_aonp_ll_get_regf_reserved_0_0(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->reserved_0_0;
}

static inline void sys_aonp_ll_set_regf_reserved_1_1(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->reserved_1_1 = v;
}

static inline uint32_t sys_aonp_ll_get_regf_reserved_1_1(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->reserved_1_1;
}

static inline void sys_aonp_ll_set_regf_reserved_2_2(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->reserved_2_2 = v;
}

static inline uint32_t sys_aonp_ll_get_regf_reserved_2_2(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->reserved_2_2;
}

static inline void sys_aonp_ll_set_regf_macp_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->macp_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_macp_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->macp_mem_ret;
}

static inline void sys_aonp_ll_set_regf_phyp_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->phyp_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_phyp_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->phyp_mem_ret;
}

static inline void sys_aonp_ll_set_regf_thread_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->thread_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_thread_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->thread_mem_ret;
}

static inline void sys_aonp_ll_set_regf_encp_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->encp_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_encp_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->encp_mem_ret;
}

static inline void sys_aonp_ll_set_regf_can0_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->can0_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_can0_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->can0_mem_ret;
}

static inline void sys_aonp_ll_set_regf_can1_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->can1_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_can1_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->can1_mem_ret;
}

static inline void sys_aonp_ll_set_regf_irda0_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->irda0_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_irda0_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->irda0_mem_ret;
}

static inline void sys_aonp_ll_set_regf_irda1_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->irda1_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_irda1_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->irda1_mem_ret;
}

static inline void sys_aonp_ll_set_regf_dma0_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->dma0_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_dma0_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->dma0_mem_ret;
}

static inline void sys_aonp_ll_set_regf_spi1_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->spi1_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_spi1_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->spi1_mem_ret;
}

static inline void sys_aonp_ll_set_regf_spi2_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->spi2_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_spi2_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->spi2_mem_ret;
}

static inline void sys_aonp_ll_set_regf_uart1_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->uart1_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_uart1_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->uart1_mem_ret;
}

static inline void sys_aonp_ll_set_regf_uart2_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->uart2_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_uart2_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->uart2_mem_ret;
}

static inline void sys_aonp_ll_set_regf_uart3_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->uart3_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_uart3_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->uart3_mem_ret;
}

static inline void sys_aonp_ll_set_regf_uart0_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->uart0_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_uart0_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->uart0_mem_ret;
}

static inline void sys_aonp_ll_set_regf_spi0_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->spi0_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_spi0_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->spi0_mem_ret;
}

static inline void sys_aonp_ll_set_regf_flsh_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->flsh_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_flsh_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->flsh_mem_ret;
}

static inline void sys_aonp_ll_set_regf_audp_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->audp_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_audp_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->audp_mem_ret;
}

static inline void sys_aonp_ll_set_regf_i3c_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->i3c_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_i3c_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->i3c_mem_ret;
}

static inline void sys_aonp_ll_set_regf_xvr_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->xvr_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_xvr_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->xvr_mem_ret;
}

static inline void sys_aonp_ll_set_regf_reserved_23_23(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->reserved_23_23 = v;
}

static inline uint32_t sys_aonp_ll_get_regf_reserved_23_23(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->reserved_23_23;
}

static inline void sys_aonp_ll_set_regf_bk24_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->bk24_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_bk24_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->bk24_mem_ret;
}

static inline void sys_aonp_ll_set_regf_irda2_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->irda2_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_irda2_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->irda2_mem_ret;
}

static inline void sys_aonp_ll_set_regf_irda3_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->irda3_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_irda3_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->irda3_mem_ret;
}

static inline void sys_aonp_ll_set_regf_spi3_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->spi3_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_spi3_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->spi3_mem_ret;
}

static inline void sys_aonp_ll_set_regf_uart4_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->uart4_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_uart4_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->uart4_mem_ret;
}

static inline void sys_aonp_ll_set_regf_cpu0_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->cpu0_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_cpu0_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->cpu0_mem_ret;
}

static inline void sys_aonp_ll_set_regf_cpu1_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->cpu1_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_cpu1_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->cpu1_mem_ret;
}

static inline void sys_aonp_ll_set_regf_coresight_mem_ret(uint32_t v) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	r->coresight_mem_ret = v;
}

static inline uint32_t sys_aonp_ll_get_regf_coresight_mem_ret(void) {
	sys_aonp_regf_t *r = (sys_aonp_regf_t*)(SOC_SYS_AONP_REG_BASE + (0xf << 2));
	return r->coresight_mem_ret;
}

//reg reg10:

static inline void sys_aonp_ll_set_reg10_value(uint32_t v) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg10_value(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg10_pwd_cpu1(uint32_t v) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	r->pwd_cpu1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg10_pwd_cpu1(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->pwd_cpu1;
}

static inline void sys_aonp_ll_set_reg10_pwd_vehp(uint32_t v) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	r->pwd_vehp = v;
}

static inline uint32_t sys_aonp_ll_get_reg10_pwd_vehp(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->pwd_vehp;
}

static inline void sys_aonp_ll_set_reg10_pwd_wrls(uint32_t v) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	r->pwd_wrls = v;
}

static inline uint32_t sys_aonp_ll_get_reg10_pwd_wrls(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->pwd_wrls;
}

static inline void sys_aonp_ll_set_reg10_rom_pgen(uint32_t v) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	r->rom_pgen = v;
}

static inline uint32_t sys_aonp_ll_get_reg10_rom_pgen(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->rom_pgen;
}

static inline uint32_t sys_aonp_ll_get_reg10_cpu1_isolate_state(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->cpu1_isolate_state;
}

static inline uint32_t sys_aonp_ll_get_reg10_vehp_isolate_state(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->vehp_isolate_state;
}

static inline uint32_t sys_aonp_ll_get_reg10_wrls_isolate_state(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->wrls_isolate_state;
}

static inline void sys_aonp_ll_set_reg10_reserved_7_30(uint32_t v) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	r->reserved_7_30 = v;
}

static inline uint32_t sys_aonp_ll_get_reg10_reserved_7_30(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->reserved_7_30;
}

static inline uint32_t sys_aonp_ll_get_reg10_busmatrix_busy(void) {
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));
	return r->busmatrix_busy;
}

//reg reg11:

static inline void sys_aonp_ll_set_reg11_value(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_value(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg11_sleep_en_global(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->sleep_en_global = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_sleep_en_global(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->sleep_en_global;
}

static inline void sys_aonp_ll_set_reg11_sleep_en_need_flash_idle(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->sleep_en_need_flash_idle = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_sleep_en_need_flash_idle(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->sleep_en_need_flash_idle;
}

static inline void sys_aonp_ll_set_reg11_sleep_bus_idle_bypass(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->sleep_bus_idle_bypass = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_sleep_bus_idle_bypass(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->sleep_bus_idle_bypass;
}

static inline void sys_aonp_ll_set_reg11_sleep_en_need_cpu0_wfi(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->sleep_en_need_cpu0_wfi = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_sleep_en_need_cpu0_wfi(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->sleep_en_need_cpu0_wfi;
}

static inline void sys_aonp_ll_set_reg11_sleep_en_need_cpu1_wfi(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->sleep_en_need_cpu1_wfi = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_sleep_en_need_cpu1_wfi(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->sleep_en_need_cpu1_wfi;
}

static inline void sys_aonp_ll_set_reg11_reserved_12_15(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->reserved_12_15 = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_reserved_12_15(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->reserved_12_15;
}

static inline void sys_aonp_ll_set_reg11_cpu0_ticktimer_32k_enable(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->cpu0_ticktimer_32k_enable = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_cpu0_ticktimer_32k_enable(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->cpu0_ticktimer_32k_enable;
}

static inline void sys_aonp_ll_set_reg11_cpu1_ticktimer_32k_enable(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->cpu1_ticktimer_32k_enable = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_cpu1_ticktimer_32k_enable(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->cpu1_ticktimer_32k_enable;
}

static inline void sys_aonp_ll_set_reg11_reserved_20_24(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->reserved_20_24 = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_reserved_20_24(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->reserved_20_24;
}

static inline void sys_aonp_ll_set_reg11_bts_soft_wakeup_req(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->bts_soft_wakeup_req = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_bts_soft_wakeup_req(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->bts_soft_wakeup_req;
}

static inline void sys_aonp_ll_set_reg11_rom_rd_disable(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->rom_rd_disable = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_rom_rd_disable(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->rom_rd_disable;
}

static inline void sys_aonp_ll_set_reg11_otp_rd_disable(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->otp_rd_disable = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_otp_rd_disable(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->otp_rd_disable;
}

static inline void sys_aonp_ll_set_reg11_share_mem_clkgating_disable(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->share_mem_clkgating_disable = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_share_mem_clkgating_disable(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->share_mem_clkgating_disable;
}

static inline void sys_aonp_ll_set_reg11_reserved_29_31(uint32_t v) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	r->reserved_29_31 = v;
}

static inline uint32_t sys_aonp_ll_get_reg11_reserved_29_31(void) {
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));
	return r->reserved_29_31;
}

//reg reg14:

static inline void sys_aonp_ll_set_reg14_value(uint32_t v) {
	sys_aonp_reg14_t *r = (sys_aonp_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x14 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg14_value(void) {
	sys_aonp_reg14_t *r = (sys_aonp_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x14 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg14_cpu0_inten(uint32_t v) {
	sys_aonp_reg14_t *r = (sys_aonp_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x14 << 2));
	r->cpu0_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg14_cpu0_inten(void) {
	sys_aonp_reg14_t *r = (sys_aonp_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x14 << 2));
	return r->cpu0_inten;
}

//reg reg15:

static inline void sys_aonp_ll_set_reg15_value(uint32_t v) {
	sys_aonp_reg15_t *r = (sys_aonp_reg15_t*)(SOC_SYS_AONP_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg15_value(void) {
	sys_aonp_reg15_t *r = (sys_aonp_reg15_t*)(SOC_SYS_AONP_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg15_cpu0_inten(uint32_t v) {
	sys_aonp_reg15_t *r = (sys_aonp_reg15_t*)(SOC_SYS_AONP_REG_BASE + (0x15 << 2));
	r->cpu0_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg15_cpu0_inten(void) {
	sys_aonp_reg15_t *r = (sys_aonp_reg15_t*)(SOC_SYS_AONP_REG_BASE + (0x15 << 2));
	return r->cpu0_inten;
}

//reg reg16:

static inline void sys_aonp_ll_set_reg16_value(uint32_t v) {
	sys_aonp_reg16_t *r = (sys_aonp_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x16 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg16_value(void) {
	sys_aonp_reg16_t *r = (sys_aonp_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x16 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg16_cpu0_inten(uint32_t v) {
	sys_aonp_reg16_t *r = (sys_aonp_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x16 << 2));
	r->cpu0_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg16_cpu0_inten(void) {
	sys_aonp_reg16_t *r = (sys_aonp_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x16 << 2));
	return r->cpu0_inten;
}

//reg reg17:

static inline void sys_aonp_ll_set_reg17_value(uint32_t v) {
	sys_aonp_reg17_t *r = (sys_aonp_reg17_t*)(SOC_SYS_AONP_REG_BASE + (0x17 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg17_value(void) {
	sys_aonp_reg17_t *r = (sys_aonp_reg17_t*)(SOC_SYS_AONP_REG_BASE + (0x17 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg17_cpu1_inten(uint32_t v) {
	sys_aonp_reg17_t *r = (sys_aonp_reg17_t*)(SOC_SYS_AONP_REG_BASE + (0x17 << 2));
	r->cpu1_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg17_cpu1_inten(void) {
	sys_aonp_reg17_t *r = (sys_aonp_reg17_t*)(SOC_SYS_AONP_REG_BASE + (0x17 << 2));
	return r->cpu1_inten;
}

//reg reg18:

static inline void sys_aonp_ll_set_reg18_value(uint32_t v) {
	sys_aonp_reg18_t *r = (sys_aonp_reg18_t*)(SOC_SYS_AONP_REG_BASE + (0x18 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg18_value(void) {
	sys_aonp_reg18_t *r = (sys_aonp_reg18_t*)(SOC_SYS_AONP_REG_BASE + (0x18 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg18_cpu1_inten(uint32_t v) {
	sys_aonp_reg18_t *r = (sys_aonp_reg18_t*)(SOC_SYS_AONP_REG_BASE + (0x18 << 2));
	r->cpu1_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg18_cpu1_inten(void) {
	sys_aonp_reg18_t *r = (sys_aonp_reg18_t*)(SOC_SYS_AONP_REG_BASE + (0x18 << 2));
	return r->cpu1_inten;
}

//reg reg19:

static inline void sys_aonp_ll_set_reg19_value(uint32_t v) {
	sys_aonp_reg19_t *r = (sys_aonp_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x19 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg19_value(void) {
	sys_aonp_reg19_t *r = (sys_aonp_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x19 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg19_cpu1_inten(uint32_t v) {
	sys_aonp_reg19_t *r = (sys_aonp_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x19 << 2));
	r->cpu1_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg19_cpu1_inten(void) {
	sys_aonp_reg19_t *r = (sys_aonp_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x19 << 2));
	return r->cpu1_inten;
}

//reg reg1a:

static inline void sys_aonp_ll_set_reg1a_value(uint32_t v) {
	sys_aonp_reg1a_t *r = (sys_aonp_reg1a_t*)(SOC_SYS_AONP_REG_BASE + (0x1a << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg1a_value(void) {
	sys_aonp_reg1a_t *r = (sys_aonp_reg1a_t*)(SOC_SYS_AONP_REG_BASE + (0x1a << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg1a_m55sub_inten(uint32_t v) {
	sys_aonp_reg1a_t *r = (sys_aonp_reg1a_t*)(SOC_SYS_AONP_REG_BASE + (0x1a << 2));
	r->m55sub_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg1a_m55sub_inten(void) {
	sys_aonp_reg1a_t *r = (sys_aonp_reg1a_t*)(SOC_SYS_AONP_REG_BASE + (0x1a << 2));
	return r->m55sub_inten;
}

//reg reg1b:

static inline void sys_aonp_ll_set_reg1b_value(uint32_t v) {
	sys_aonp_reg1b_t *r = (sys_aonp_reg1b_t*)(SOC_SYS_AONP_REG_BASE + (0x1b << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg1b_value(void) {
	sys_aonp_reg1b_t *r = (sys_aonp_reg1b_t*)(SOC_SYS_AONP_REG_BASE + (0x1b << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg1b_m55sub_inten(uint32_t v) {
	sys_aonp_reg1b_t *r = (sys_aonp_reg1b_t*)(SOC_SYS_AONP_REG_BASE + (0x1b << 2));
	r->m55sub_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg1b_m55sub_inten(void) {
	sys_aonp_reg1b_t *r = (sys_aonp_reg1b_t*)(SOC_SYS_AONP_REG_BASE + (0x1b << 2));
	return r->m55sub_inten;
}

//reg reg1c:

static inline void sys_aonp_ll_set_reg1c_value(uint32_t v) {
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t*)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg1c_value(void) {
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t*)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg1c_m55sub_inten(uint32_t v) {
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t*)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));
	r->m55sub_inten = v;
}

static inline uint32_t sys_aonp_ll_get_reg1c_m55sub_inten(void) {
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t*)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));
	return r->m55sub_inten;
}

static inline void sys_aonp_ll_set_reg1c_m55sub_wakeup(uint32_t v) {
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t*)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));
	r->m55sub_wakeup = v;
}

static inline uint32_t sys_aonp_ll_get_reg1c_m55sub_wakeup(void) {
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t*)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));
	return r->m55sub_wakeup;
}

//reg reg1e:

static inline void sys_aonp_ll_set_reg1e_value(uint32_t v) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg1e_value(void) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg1e_spsh_cfg(uint32_t v) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	r->spsh_cfg = v;
}

static inline uint32_t sys_aonp_ll_get_reg1e_spsh_cfg(void) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	return r->spsh_cfg;
}

static inline void sys_aonp_ll_set_reg1e_spbh_cfg(uint32_t v) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	r->spbh_cfg = v;
}

static inline uint32_t sys_aonp_ll_get_reg1e_spbh_cfg(void) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	return r->spbh_cfg;
}

static inline void sys_aonp_ll_set_reg1e_set_key(uint32_t v) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	r->set_key = v;
}

static inline uint32_t sys_aonp_ll_get_reg1e_set_key(void) {
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t*)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));
	return r->set_key;
}

//reg reg1f:

static inline void sys_aonp_ll_set_reg1f_value(uint32_t v) {
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t*)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg1f_value(void) {
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t*)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg1f_stph_cfg(uint32_t v) {
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t*)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));
	r->stph_cfg = v;
}

static inline uint32_t sys_aonp_ll_get_reg1f_stph_cfg(void) {
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t*)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));
	return r->stph_cfg;
}

static inline void sys_aonp_ll_set_reg1f_set_key(uint32_t v) {
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t*)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));
	r->set_key = v;
}

static inline uint32_t sys_aonp_ll_get_reg1f_set_key(void) {
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t*)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));
	return r->set_key;
}

//reg reg20:

static inline void sys_aonp_ll_set_reg20_value(uint32_t v) {
	sys_aonp_reg20_t *r = (sys_aonp_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x20 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg20_value(void) {
	sys_aonp_reg20_t *r = (sys_aonp_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x20 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg20_ints_status0(void) {
	sys_aonp_reg20_t *r = (sys_aonp_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x20 << 2));
	return r->ints_status0;
}

//reg reg21:

static inline void sys_aonp_ll_set_reg21_value(uint32_t v) {
	sys_aonp_reg21_t *r = (sys_aonp_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x21 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg21_value(void) {
	sys_aonp_reg21_t *r = (sys_aonp_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x21 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg21_ints_status1(void) {
	sys_aonp_reg21_t *r = (sys_aonp_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x21 << 2));
	return r->ints_status1;
}

//reg reg22:

static inline void sys_aonp_ll_set_reg22_value(uint32_t v) {
	sys_aonp_reg22_t *r = (sys_aonp_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x22 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg22_value(void) {
	sys_aonp_reg22_t *r = (sys_aonp_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x22 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg22_ints_status2(void) {
	sys_aonp_reg22_t *r = (sys_aonp_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x22 << 2));
	return r->ints_status2;
}

//reg reg23:

static inline void sys_aonp_ll_set_reg23_value(uint32_t v) {
	sys_aonp_reg23_t *r = (sys_aonp_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x23 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg23_value(void) {
	sys_aonp_reg23_t *r = (sys_aonp_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x23 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg23_ints_status3(void) {
	sys_aonp_reg23_t *r = (sys_aonp_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x23 << 2));
	return r->ints_status3;
}

//reg reg24:

static inline void sys_aonp_ll_set_reg24_value(uint32_t v) {
	sys_aonp_reg24_t *r = (sys_aonp_reg24_t*)(SOC_SYS_AONP_REG_BASE + (0x24 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg24_value(void) {
	sys_aonp_reg24_t *r = (sys_aonp_reg24_t*)(SOC_SYS_AONP_REG_BASE + (0x24 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg24_ints_status4(void) {
	sys_aonp_reg24_t *r = (sys_aonp_reg24_t*)(SOC_SYS_AONP_REG_BASE + (0x24 << 2));
	return r->ints_status4;
}

//reg reg25:

static inline void sys_aonp_ll_set_reg25_value(uint32_t v) {
	sys_aonp_reg25_t *r = (sys_aonp_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x25 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg25_value(void) {
	sys_aonp_reg25_t *r = (sys_aonp_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x25 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg25_ints_status5(void) {
	sys_aonp_reg25_t *r = (sys_aonp_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x25 << 2));
	return r->ints_status5;
}

//reg reg26:

static inline void sys_aonp_ll_set_reg26_value(uint32_t v) {
	sys_aonp_reg26_t *r = (sys_aonp_reg26_t*)(SOC_SYS_AONP_REG_BASE + (0x26 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg26_value(void) {
	sys_aonp_reg26_t *r = (sys_aonp_reg26_t*)(SOC_SYS_AONP_REG_BASE + (0x26 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg26_m55sub_status0(void) {
	sys_aonp_reg26_t *r = (sys_aonp_reg26_t*)(SOC_SYS_AONP_REG_BASE + (0x26 << 2));
	return r->m55sub_status0;
}

//reg reg27:

static inline void sys_aonp_ll_set_reg27_value(uint32_t v) {
	sys_aonp_reg27_t *r = (sys_aonp_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x27 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg27_value(void) {
	sys_aonp_reg27_t *r = (sys_aonp_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x27 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg27_m55sub_status1(void) {
	sys_aonp_reg27_t *r = (sys_aonp_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x27 << 2));
	return r->m55sub_status1;
}

//reg reg28:

static inline void sys_aonp_ll_set_reg28_value(uint32_t v) {
	sys_aonp_reg28_t *r = (sys_aonp_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x28 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg28_value(void) {
	sys_aonp_reg28_t *r = (sys_aonp_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x28 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg28_m55sub_status2(void) {
	sys_aonp_reg28_t *r = (sys_aonp_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x28 << 2));
	return r->m55sub_status2;
}

//reg reg2a:

static inline void sys_aonp_ll_set_reg2a_value(uint32_t v) {
	sys_aonp_reg2a_t *r = (sys_aonp_reg2a_t*)(SOC_SYS_AONP_REG_BASE + (0x2a << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg2a_value(void) {
	sys_aonp_reg2a_t *r = (sys_aonp_reg2a_t*)(SOC_SYS_AONP_REG_BASE + (0x2a << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg2a_debug_gpio_ie(uint32_t v) {
	sys_aonp_reg2a_t *r = (sys_aonp_reg2a_t*)(SOC_SYS_AONP_REG_BASE + (0x2a << 2));
	r->debug_gpio_ie = v;
}

static inline uint32_t sys_aonp_ll_get_reg2a_debug_gpio_ie(void) {
	sys_aonp_reg2a_t *r = (sys_aonp_reg2a_t*)(SOC_SYS_AONP_REG_BASE + (0x2a << 2));
	return r->debug_gpio_ie;
}

//reg reg2b:

static inline void sys_aonp_ll_set_reg2b_value(uint32_t v) {
	sys_aonp_reg2b_t *r = (sys_aonp_reg2b_t*)(SOC_SYS_AONP_REG_BASE + (0x2b << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg2b_value(void) {
	sys_aonp_reg2b_t *r = (sys_aonp_reg2b_t*)(SOC_SYS_AONP_REG_BASE + (0x2b << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg2b_debug_gpio_i(void) {
	sys_aonp_reg2b_t *r = (sys_aonp_reg2b_t*)(SOC_SYS_AONP_REG_BASE + (0x2b << 2));
	return r->debug_gpio_i;
}

//reg reg2c:

static inline void sys_aonp_ll_set_reg2c_value(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_value(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg2c_cache_clean_mode(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->cache_clean_mode = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_cache_clean_mode(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->cache_clean_mode;
}

static inline void sys_aonp_ll_set_reg2c_cpu0_icache_clean_mode(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->cpu0_icache_clean_mode = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_cpu0_icache_clean_mode(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->cpu0_icache_clean_mode;
}

static inline void sys_aonp_ll_set_reg2c_cpu0_icache_clean_tag_sel(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->cpu0_icache_clean_tag_sel = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_cpu0_icache_clean_tag_sel(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->cpu0_icache_clean_tag_sel;
}

static inline void sys_aonp_ll_set_reg2c_l2_cache_clean_mode(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->l2_cache_clean_mode = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_l2_cache_clean_mode(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->l2_cache_clean_mode;
}

static inline void sys_aonp_ll_set_reg2c_l2_cache_clean_tag_sel(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->l2_cache_clean_tag_sel = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_l2_cache_clean_tag_sel(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->l2_cache_clean_tag_sel;
}

static inline void sys_aonp_ll_set_reg2c_reserved_6_23(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->reserved_6_23 = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_reserved_6_23(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->reserved_6_23;
}

static inline void sys_aonp_ll_set_reg2c_set_key(uint32_t v) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	r->set_key = v;
}

static inline uint32_t sys_aonp_ll_get_reg2c_set_key(void) {
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t*)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));
	return r->set_key;
}

//reg reg2e:

static inline void sys_aonp_ll_set_reg2e_value(uint32_t v) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg2e_value(void) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg2e_spsl_cfg(uint32_t v) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	r->spsl_cfg = v;
}

static inline uint32_t sys_aonp_ll_get_reg2e_spsl_cfg(void) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	return r->spsl_cfg;
}

static inline void sys_aonp_ll_set_reg2e_spbl_cfg(uint32_t v) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	r->spbl_cfg = v;
}

static inline uint32_t sys_aonp_ll_get_reg2e_spbl_cfg(void) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	return r->spbl_cfg;
}

static inline void sys_aonp_ll_set_reg2e_set_key(uint32_t v) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	r->set_key = v;
}

static inline uint32_t sys_aonp_ll_get_reg2e_set_key(void) {
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t*)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));
	return r->set_key;
}

//reg reg2f:

static inline void sys_aonp_ll_set_reg2f_value(uint32_t v) {
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t*)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg2f_value(void) {
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t*)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg2f_stpl_cfg(uint32_t v) {
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t*)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));
	r->stpl_cfg = v;
}

static inline uint32_t sys_aonp_ll_get_reg2f_stpl_cfg(void) {
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t*)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));
	return r->stpl_cfg;
}

static inline void sys_aonp_ll_set_reg2f_set_key(uint32_t v) {
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t*)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));
	r->set_key = v;
}

static inline uint32_t sys_aonp_ll_get_reg2f_set_key(void) {
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t*)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));
	return r->set_key;
}

//reg reg30:

static inline void sys_aonp_ll_set_reg30_value(uint32_t v) {
	sys_aonp_reg30_t *r = (sys_aonp_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x30 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg30_value(void) {
	sys_aonp_reg30_t *r = (sys_aonp_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x30 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg30_gpio_input_status0(void) {
	sys_aonp_reg30_t *r = (sys_aonp_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x30 << 2));
	return r->gpio_input_status0;
}

//reg reg31:

static inline void sys_aonp_ll_set_reg31_value(uint32_t v) {
	sys_aonp_reg31_t *r = (sys_aonp_reg31_t*)(SOC_SYS_AONP_REG_BASE + (0x31 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg31_value(void) {
	sys_aonp_reg31_t *r = (sys_aonp_reg31_t*)(SOC_SYS_AONP_REG_BASE + (0x31 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg31_gpio_input_status1(void) {
	sys_aonp_reg31_t *r = (sys_aonp_reg31_t*)(SOC_SYS_AONP_REG_BASE + (0x31 << 2));
	return r->gpio_input_status1;
}

//reg reg32:

static inline void sys_aonp_ll_set_reg32_value(uint32_t v) {
	sys_aonp_reg32_t *r = (sys_aonp_reg32_t*)(SOC_SYS_AONP_REG_BASE + (0x32 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg32_value(void) {
	sys_aonp_reg32_t *r = (sys_aonp_reg32_t*)(SOC_SYS_AONP_REG_BASE + (0x32 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg32_gpio_input_status2(void) {
	sys_aonp_reg32_t *r = (sys_aonp_reg32_t*)(SOC_SYS_AONP_REG_BASE + (0x32 << 2));
	return r->gpio_input_status2;
}

static inline void sys_aonp_ll_set_reg32_gpio_input_status_en(uint32_t v) {
	sys_aonp_reg32_t *r = (sys_aonp_reg32_t*)(SOC_SYS_AONP_REG_BASE + (0x32 << 2));
	r->gpio_input_status_en = v;
}

static inline uint32_t sys_aonp_ll_get_reg32_gpio_input_status_en(void) {
	sys_aonp_reg32_t *r = (sys_aonp_reg32_t*)(SOC_SYS_AONP_REG_BASE + (0x32 << 2));
	return r->gpio_input_status_en;
}

//reg reg33:

static inline void sys_aonp_ll_set_reg33_value(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_value(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg33_acomp0_pwm0_sample_en(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->acomp0_pwm0_sample_en = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_acomp0_pwm0_sample_en(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->acomp0_pwm0_sample_en;
}

static inline void sys_aonp_ll_set_reg33_acomp1_pwm0_sample_en(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->acomp1_pwm0_sample_en = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_acomp1_pwm0_sample_en(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->acomp1_pwm0_sample_en;
}

static inline void sys_aonp_ll_set_reg33_reserved_2_15(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->reserved_2_15 = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_reserved_2_15(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->reserved_2_15;
}

static inline void sys_aonp_ll_set_reg33_l2_dis_pwr_down_maint(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->l2_dis_pwr_down_maint = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_l2_dis_pwr_down_maint(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->l2_dis_pwr_down_maint;
}

static inline void sys_aonp_ll_set_reg33_l2_apb_violation_resp(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->l2_apb_violation_resp = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_l2_apb_violation_resp(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->l2_apb_violation_resp;
}

static inline void sys_aonp_ll_set_reg33_cpu0_dbgen_l2_rst_dis(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu0_dbgen_l2_rst_dis = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu0_dbgen_l2_rst_dis(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu0_dbgen_l2_rst_dis;
}

static inline void sys_aonp_ll_set_reg33_cpu1_dbgen_l2_rst_dis(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu1_dbgen_l2_rst_dis = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu1_dbgen_l2_rst_dis(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu1_dbgen_l2_rst_dis;
}

static inline void sys_aonp_ll_set_reg33_reserved_20_23(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->reserved_20_23 = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_reserved_20_23(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->reserved_20_23;
}

static inline void sys_aonp_ll_set_reg33_cpu0_wfe_src(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu0_wfe_src = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu0_wfe_src(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu0_wfe_src;
}

static inline void sys_aonp_ll_set_reg33_cpu0_wfe_pulse(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu0_wfe_pulse = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu0_wfe_pulse(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu0_wfe_pulse;
}

static inline void sys_aonp_ll_set_reg33_cpu1_wfe_src(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu1_wfe_src = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu1_wfe_src(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu1_wfe_src;
}

static inline void sys_aonp_ll_set_reg33_cpu1_wfe_pulse(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu1_wfe_pulse = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu1_wfe_pulse(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu1_wfe_pulse;
}

static inline void sys_aonp_ll_set_reg33_cpu0_sleeping_state(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu0_sleeping_state = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu0_sleeping_state(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu0_sleeping_state;
}

static inline void sys_aonp_ll_set_reg33_cpu0_deepsleep_state(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu0_deepsleep_state = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu0_deepsleep_state(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu0_deepsleep_state;
}

static inline void sys_aonp_ll_set_reg33_cpu1_sleeping_state(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu1_sleeping_state = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu1_sleeping_state(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu1_sleeping_state;
}

static inline void sys_aonp_ll_set_reg33_cpu1_deepsleep_state(uint32_t v) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	r->cpu1_deepsleep_state = v;
}

static inline uint32_t sys_aonp_ll_get_reg33_cpu1_deepsleep_state(void) {
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));
	return r->cpu1_deepsleep_state;
}

//reg reg34:

static inline void sys_aonp_ll_set_reg34_value(uint32_t v) {
	sys_aonp_reg34_t *r = (sys_aonp_reg34_t*)(SOC_SYS_AONP_REG_BASE + (0x34 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg34_value(void) {
	sys_aonp_reg34_t *r = (sys_aonp_reg34_t*)(SOC_SYS_AONP_REG_BASE + (0x34 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg34_cpu0_curpc(void) {
	sys_aonp_reg34_t *r = (sys_aonp_reg34_t*)(SOC_SYS_AONP_REG_BASE + (0x34 << 2));
	return r->cpu0_curpc;
}

//reg reg35:

static inline void sys_aonp_ll_set_reg35_value(uint32_t v) {
	sys_aonp_reg35_t *r = (sys_aonp_reg35_t*)(SOC_SYS_AONP_REG_BASE + (0x35 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg35_value(void) {
	sys_aonp_reg35_t *r = (sys_aonp_reg35_t*)(SOC_SYS_AONP_REG_BASE + (0x35 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg35_cpu0_faultstat_h(void) {
	sys_aonp_reg35_t *r = (sys_aonp_reg35_t*)(SOC_SYS_AONP_REG_BASE + (0x35 << 2));
	return r->cpu0_faultstat_h;
}

static inline uint32_t sys_aonp_ll_get_reg35_reserved_11_31(void) {
	sys_aonp_reg35_t *r = (sys_aonp_reg35_t*)(SOC_SYS_AONP_REG_BASE + (0x35 << 2));
	return r->reserved_11_31;
}

//reg reg36:

static inline void sys_aonp_ll_set_reg36_value(uint32_t v) {
	sys_aonp_reg36_t *r = (sys_aonp_reg36_t*)(SOC_SYS_AONP_REG_BASE + (0x36 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg36_value(void) {
	sys_aonp_reg36_t *r = (sys_aonp_reg36_t*)(SOC_SYS_AONP_REG_BASE + (0x36 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg36_cpu0_faultstat_l(void) {
	sys_aonp_reg36_t *r = (sys_aonp_reg36_t*)(SOC_SYS_AONP_REG_BASE + (0x36 << 2));
	return r->cpu0_faultstat_l;
}

//reg reg37:

static inline void sys_aonp_ll_set_reg37_value(uint32_t v) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg37_value(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu0_intnum(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu0_intnum;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu0_currpri(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu0_currpri;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu0_currns(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu0_currns;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu0_halted(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu0_halted;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu0_nc_hready(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu0_nc_hready;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu_cache_m_hready(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu_cache_m_hready;
}

static inline uint32_t sys_aonp_ll_get_reg37_cpu0_resetn(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->cpu0_resetn;
}

static inline uint32_t sys_aonp_ll_get_reg37_reserved_22_31(void) {
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));
	return r->reserved_22_31;
}

//reg reg38:

static inline void sys_aonp_ll_set_reg38_value(uint32_t v) {
	sys_aonp_reg38_t *r = (sys_aonp_reg38_t*)(SOC_SYS_AONP_REG_BASE + (0x38 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg38_value(void) {
	sys_aonp_reg38_t *r = (sys_aonp_reg38_t*)(SOC_SYS_AONP_REG_BASE + (0x38 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg38_dbug_config0(uint32_t v) {
	sys_aonp_reg38_t *r = (sys_aonp_reg38_t*)(SOC_SYS_AONP_REG_BASE + (0x38 << 2));
	r->dbug_config0 = v;
}

static inline uint32_t sys_aonp_ll_get_reg38_dbug_config0(void) {
	sys_aonp_reg38_t *r = (sys_aonp_reg38_t*)(SOC_SYS_AONP_REG_BASE + (0x38 << 2));
	return r->dbug_config0;
}

//reg reg39:

static inline void sys_aonp_ll_set_reg39_value(uint32_t v) {
	sys_aonp_reg39_t *r = (sys_aonp_reg39_t*)(SOC_SYS_AONP_REG_BASE + (0x39 << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg39_value(void) {
	sys_aonp_reg39_t *r = (sys_aonp_reg39_t*)(SOC_SYS_AONP_REG_BASE + (0x39 << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg39_dbug_config1(uint32_t v) {
	sys_aonp_reg39_t *r = (sys_aonp_reg39_t*)(SOC_SYS_AONP_REG_BASE + (0x39 << 2));
	r->dbug_config1 = v;
}

static inline uint32_t sys_aonp_ll_get_reg39_dbug_config1(void) {
	sys_aonp_reg39_t *r = (sys_aonp_reg39_t*)(SOC_SYS_AONP_REG_BASE + (0x39 << 2));
	return r->dbug_config1;
}

//reg reg3a:

static inline void sys_aonp_ll_set_reg3a_value(uint32_t v) {
	sys_aonp_reg3a_t *r = (sys_aonp_reg3a_t*)(SOC_SYS_AONP_REG_BASE + (0x3a << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3a_value(void) {
	sys_aonp_reg3a_t *r = (sys_aonp_reg3a_t*)(SOC_SYS_AONP_REG_BASE + (0x3a << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg3a_anareg_stat(void) {
	sys_aonp_reg3a_t *r = (sys_aonp_reg3a_t*)(SOC_SYS_AONP_REG_BASE + (0x3a << 2));
	return r->anareg_stat;
}

//reg reg3b:

static inline void sys_aonp_ll_set_reg3b_value(uint32_t v) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3b_value(void) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	return r->v;
}

static inline void sys_aonp_ll_set_reg3b_coresight_chn_gate_en(uint32_t v) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	r->coresight_chn_gate_en = v;
}

static inline uint32_t sys_aonp_ll_get_reg3b_coresight_chn_gate_en(void) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	return r->coresight_chn_gate_en;
}

static inline void sys_aonp_ll_set_reg3b_coresight_tpmaxdatasize(uint32_t v) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	r->coresight_tpmaxdatasize = v;
}

static inline uint32_t sys_aonp_ll_get_reg3b_coresight_tpmaxdatasize(void) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	return r->coresight_tpmaxdatasize;
}

static inline void sys_aonp_ll_set_reg3b_coresight_valid(uint32_t v) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	r->coresight_valid = v;
}

static inline uint32_t sys_aonp_ll_get_reg3b_coresight_valid(void) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	return r->coresight_valid;
}

static inline void sys_aonp_ll_set_reg3b_reserved_30_30(uint32_t v) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	r->reserved_30_30 = v;
}

static inline uint32_t sys_aonp_ll_get_reg3b_reserved_30_30(void) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	return r->reserved_30_30;
}

static inline uint32_t sys_aonp_ll_get_reg3b_anaregb_stat(void) {
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t*)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));
	return r->anaregb_stat;
}

//reg reg3c:

static inline void sys_aonp_ll_set_reg3c_value(uint32_t v) {
	sys_aonp_reg3c_t *r = (sys_aonp_reg3c_t*)(SOC_SYS_AONP_REG_BASE + (0x3c << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3c_value(void) {
	sys_aonp_reg3c_t *r = (sys_aonp_reg3c_t*)(SOC_SYS_AONP_REG_BASE + (0x3c << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg3c_cpu1_curpc(void) {
	sys_aonp_reg3c_t *r = (sys_aonp_reg3c_t*)(SOC_SYS_AONP_REG_BASE + (0x3c << 2));
	return r->cpu1_curpc;
}

//reg reg3d:

static inline void sys_aonp_ll_set_reg3d_value(uint32_t v) {
	sys_aonp_reg3d_t *r = (sys_aonp_reg3d_t*)(SOC_SYS_AONP_REG_BASE + (0x3d << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3d_value(void) {
	sys_aonp_reg3d_t *r = (sys_aonp_reg3d_t*)(SOC_SYS_AONP_REG_BASE + (0x3d << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg3d_cpu1_faultstat_h(void) {
	sys_aonp_reg3d_t *r = (sys_aonp_reg3d_t*)(SOC_SYS_AONP_REG_BASE + (0x3d << 2));
	return r->cpu1_faultstat_h;
}

static inline uint32_t sys_aonp_ll_get_reg3d_reserved_11_31(void) {
	sys_aonp_reg3d_t *r = (sys_aonp_reg3d_t*)(SOC_SYS_AONP_REG_BASE + (0x3d << 2));
	return r->reserved_11_31;
}

//reg reg3e:

static inline void sys_aonp_ll_set_reg3e_value(uint32_t v) {
	sys_aonp_reg3e_t *r = (sys_aonp_reg3e_t*)(SOC_SYS_AONP_REG_BASE + (0x3e << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3e_value(void) {
	sys_aonp_reg3e_t *r = (sys_aonp_reg3e_t*)(SOC_SYS_AONP_REG_BASE + (0x3e << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg3e_cpu1_faultstat_l(void) {
	sys_aonp_reg3e_t *r = (sys_aonp_reg3e_t*)(SOC_SYS_AONP_REG_BASE + (0x3e << 2));
	return r->cpu1_faultstat_l;
}

//reg reg3f:

static inline void sys_aonp_ll_set_reg3f_value(uint32_t v) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	r->v = v;
}

static inline uint32_t sys_aonp_ll_get_reg3f_value(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->v;
}

static inline uint32_t sys_aonp_ll_get_reg3f_cpu1_intnum(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->cpu1_intnum;
}

static inline uint32_t sys_aonp_ll_get_reg3f_cpu1_currpri(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->cpu1_currpri;
}

static inline uint32_t sys_aonp_ll_get_reg3f_cpu1_currns(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->cpu1_currns;
}

static inline uint32_t sys_aonp_ll_get_reg3f_cpu1_halted(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->cpu1_halted;
}

static inline uint32_t sys_aonp_ll_get_reg3f_cpu1_nc_hready(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->cpu1_nc_hready;
}

static inline uint32_t sys_aonp_ll_get_reg3f_reserved_20_20(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->reserved_20_20;
}

static inline uint32_t sys_aonp_ll_get_reg3f_cpu1_resetn(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->cpu1_resetn;
}

static inline uint32_t sys_aonp_ll_get_reg3f_reserved_22_31(void) {
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t*)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));
	return r->reserved_22_31;
}

//reg ana_reg0:

static inline void sys_aonp_ll_set_ana_reg0_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
}

static inline void sys_aonp_ll_set_ana_reg0_dpll_tsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_dpll_tsten(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->dpll_tsten;
}

static inline void sys_aonp_ll_set_ana_reg0_cp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 1, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_cp(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->cp;
}

static inline void sys_aonp_ll_set_ana_reg0_spideten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_spideten(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->spideten;
}

static inline void sys_aonp_ll_set_ana_reg0_hvref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 5, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_hvref(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->hvref;
}

static inline void sys_aonp_ll_set_ana_reg0_lvref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 7, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_lvref(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->lvref;
}

static inline void sys_aonp_ll_set_ana_reg0_rzctrl26m(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_rzctrl26m(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->rzctrl26m;
}

static inline void sys_aonp_ll_set_ana_reg0_looprzctrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 10, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_looprzctrl(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->looprzctrl;
}

static inline void sys_aonp_ll_set_ana_reg0_rpc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 14, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_rpc(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->rpc;
}

static inline void sys_aonp_ll_set_ana_reg0_openloop_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_openloop_en(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->openloop_en;
}

static inline void sys_aonp_ll_set_ana_reg0_cksel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 17, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_cksel(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->cksel;
}

static inline void sys_aonp_ll_set_ana_reg0_spitrig(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_spitrig(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->spitrig;
}

static inline void sys_aonp_ll_set_ana_reg0_band(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_band(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->band;
}

static inline void sys_aonp_ll_set_ana_reg0_band_1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_band_1(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->band_1;
}

static inline void sys_aonp_ll_set_ana_reg0_band_2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 22, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_band_2(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->band_2;
}

static inline void sys_aonp_ll_set_ana_reg0_bandmanual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_bandmanual(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->bandmanual;
}

static inline void sys_aonp_ll_set_ana_reg0_dsptrig(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_dsptrig(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->dsptrig;
}

static inline void sys_aonp_ll_set_ana_reg0_lpen_dpll(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_lpen_dpll(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->lpen_dpll;
}

static inline void sys_aonp_ll_set_ana_reg0_nc_28_29(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 28, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_nc_28_29(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->nc_28_29;
}

static inline void sys_aonp_ll_set_ana_reg0_bp_caldone(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_bp_caldone(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->bp_caldone;
}

static inline void sys_aonp_ll_set_ana_reg0_vctrl_dpllldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x40 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg0_vctrl_dpllldo(void) {
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t*)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));
	return r->vctrl_dpllldo;
}

//reg ana_reg1:

static inline void sys_aonp_ll_set_ana_reg1_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
}

static inline void sys_aonp_ll_set_ana_reg1_vcooffset(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_vcooffset(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->vcooffset;
}

static inline void sys_aonp_ll_set_ana_reg1_selpol(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_selpol(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->selpol;
}

static inline void sys_aonp_ll_set_ana_reg1_dlysel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 2, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_dlysel(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->dlysel;
}

static inline void sys_aonp_ll_set_ana_reg1_edgesel_nck(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_edgesel_nck(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->edgesel_nck;
}

static inline void sys_aonp_ll_set_ana_reg1_nload_dlyen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_nload_dlyen(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->nload_dlyen;
}

static inline void sys_aonp_ll_set_ana_reg1_cp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 6, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_cp(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->cp;
}

static inline void sys_aonp_ll_set_ana_reg1_spideten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_spideten(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->spideten;
}

static inline void sys_aonp_ll_set_ana_reg1_cben(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_cben(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->cben;
}

static inline void sys_aonp_ll_set_ana_reg1_hvref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 11, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_hvref(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->hvref;
}

static inline void sys_aonp_ll_set_ana_reg1_lvref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 13, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_lvref(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->lvref;
}

static inline void sys_aonp_ll_set_ana_reg1_rzctrl26m(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_rzctrl26m(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->rzctrl26m;
}

static inline void sys_aonp_ll_set_ana_reg1_lpfrz(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 16, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_lpfrz(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->lpfrz;
}

static inline void sys_aonp_ll_set_ana_reg1_rpc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 20, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_rpc(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->rpc;
}

static inline void sys_aonp_ll_set_ana_reg1_dpll_tsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_dpll_tsten(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->dpll_tsten;
}

static inline void sys_aonp_ll_set_ana_reg1_kctrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 24, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_kctrl(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->kctrl;
}

static inline void sys_aonp_ll_set_ana_reg1_vsel_ldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 26, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_vsel_ldo(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->vsel_ldo;
}

static inline void sys_aonp_ll_set_ana_reg1_div_sw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_div_sw(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->div_sw;
}

static inline void sys_aonp_ll_set_ana_reg1_bp_caldone(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_bp_caldone(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->bp_caldone;
}

static inline void sys_aonp_ll_set_ana_reg1_ck2xen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_ck2xen(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->ck2xen;
}

static inline void sys_aonp_ll_set_ana_reg1_int_mod(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x41 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg1_int_mod(void) {
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t*)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));
	return r->int_mod;
}

//reg ana_reg2:

static inline void sys_aonp_ll_set_ana_reg2_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
}

static inline void sys_aonp_ll_set_ana_reg2_xtalh_ctune(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 0, 0xff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_xtalh_ctune(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->xtalh_ctune;
}

static inline void sys_aonp_ll_set_ana_reg2_force_26mpll(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_force_26mpll(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->force_26mpll;
}

static inline void sys_aonp_ll_set_ana_reg2_nc_9_11(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 9, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_nc_9_11(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->nc_9_11;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_sd1v(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_sd1v(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_sd1v;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_bscalsaw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 13, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_bscalsaw(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_bscalsaw;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_vncalsaw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 16, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_vncalsaw(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_vncalsaw;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_vpcalsaw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 19, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_vpcalsaw(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_vpcalsaw;
}

static inline void sys_aonp_ll_set_ana_reg2_nc_22_22(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_nc_22_22(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->nc_22_22;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_vbg_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_vbg_sel(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_vbg_sel;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_clk_rlten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_clk_rlten(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_clk_rlten;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_calintsaw_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_calintsaw_en(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_calintsaw_en;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_clk_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_clk_sel(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_clk_sel;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_clk_inv(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_clk_inv(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_clk_inv;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_calcap_ch(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 28, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_calcap_ch(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_calcap_ch;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_inbuf_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_inbuf_en(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_inbuf_en;
}

static inline void sys_aonp_ll_set_ana_reg2_gadc_en_spi(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x42 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg2_gadc_en_spi(void) {
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t*)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));
	return r->gadc_en_spi;
}

//reg ana_reg3:

static inline void sys_aonp_ll_set_ana_reg3_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
}

static inline void sys_aonp_ll_set_ana_reg3_nc_0_6(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 0, 0x7f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_nc_0_6(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->nc_0_6;
}

static inline void sys_aonp_ll_set_ana_reg3_anabuf_sel_rx(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_anabuf_sel_rx(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->anabuf_sel_rx;
}

static inline void sys_aonp_ll_set_ana_reg3_hpssren(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_hpssren(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->hpssren;
}

static inline void sys_aonp_ll_set_ana_reg3_ck_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_ck_sel(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->ck_sel;
}

static inline void sys_aonp_ll_set_ana_reg3_anabuf_sel_tx(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_anabuf_sel_tx(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->anabuf_sel_tx;
}

static inline void sys_aonp_ll_set_ana_reg3_pwd_xtalldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_pwd_xtalldo(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->pwd_xtalldo;
}

static inline void sys_aonp_ll_set_ana_reg3_iamp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_iamp(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->iamp;
}

static inline void sys_aonp_ll_set_ana_reg3_vddren(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_vddren(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->vddren;
}

static inline void sys_aonp_ll_set_ana_reg3_xamp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 14, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_xamp(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->xamp;
}

static inline void sys_aonp_ll_set_ana_reg3_vosel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 20, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_vosel(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->vosel;
}

static inline void sys_aonp_ll_set_ana_reg3_en_xtalh_sleep(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_en_xtalh_sleep(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->en_xtalh_sleep;
}

static inline void sys_aonp_ll_set_ana_reg3_xtal40_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_xtal40_en(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->xtal40_en;
}

static inline void sys_aonp_ll_set_ana_reg3_bufictrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_bufictrl(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->bufictrl;
}

static inline void sys_aonp_ll_set_ana_reg3_ibias_ctrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 28, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_ibias_ctrl(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->ibias_ctrl;
}

static inline void sys_aonp_ll_set_ana_reg3_icore_ctrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x43 << 2)), 30, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg3_icore_ctrl(void) {
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t*)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));
	return r->icore_ctrl;
}

//reg ana_reg4:

static inline void sys_aonp_ll_set_ana_reg4_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
}

static inline void sys_aonp_ll_set_ana_reg4_cktst_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 0, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_cktst_sel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->cktst_sel;
}

static inline void sys_aonp_ll_set_ana_reg4_ck_tst_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_ck_tst_en(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->ck_tst_en;
}

static inline void sys_aonp_ll_set_ana_reg4_vusbsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 3, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_vusbsel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->vusbsel;
}

static inline void sys_aonp_ll_set_ana_reg4_nc_5_16(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 5, 0xfff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_nc_5_16(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->nc_5_16;
}

static inline void sys_aonp_ll_set_ana_reg4_gadc_inbuff_isel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 17, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_gadc_inbuff_isel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->gadc_inbuff_isel;
}

static inline void sys_aonp_ll_set_ana_reg4_gadc_biasamp_isel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 20, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_gadc_biasamp_isel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->gadc_biasamp_isel;
}

static inline void sys_aonp_ll_set_ana_reg4_gadc_comp_isel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 23, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_gadc_comp_isel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->gadc_comp_isel;
}

static inline void sys_aonp_ll_set_ana_reg4_gadc_preamp_isel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 26, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_gadc_preamp_isel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->gadc_preamp_isel;
}

static inline void sys_aonp_ll_set_ana_reg4_gadc_bufamp_isel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x44 << 2)), 29, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg4_gadc_bufamp_isel(void) {
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t*)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));
	return r->gadc_bufamp_isel;
}

//reg ana_reg5:

static inline void sys_aonp_ll_set_ana_reg5_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
}

static inline void sys_aonp_ll_set_ana_reg5_en_vout(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_en_vout(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->en_vout;
}

static inline void sys_aonp_ll_set_ana_reg5_en_xtall(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_en_xtall(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->en_xtall;
}

static inline void sys_aonp_ll_set_ana_reg5_en_dco(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_en_dco(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->en_dco;
}

static inline void sys_aonp_ll_set_ana_reg5_nc_3_3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_nc_3_3(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->nc_3_3;
}

static inline void sys_aonp_ll_set_ana_reg5_en_temp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_en_temp(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->en_temp;
}

static inline void sys_aonp_ll_set_ana_reg5_en_dpll(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_en_dpll(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->en_dpll;
}

static inline void sys_aonp_ll_set_ana_reg5_en_cb(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_en_cb(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->en_cb;
}

static inline void sys_aonp_ll_set_ana_reg5_gpio_latch(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_gpio_latch(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->gpio_latch;
}

static inline void sys_aonp_ll_set_ana_reg5_nc_8_11(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 8, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_nc_8_11(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->nc_8_11;
}

static inline void sys_aonp_ll_set_ana_reg5_rosc_disable(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_rosc_disable(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->rosc_disable;
}

static inline void sys_aonp_ll_set_ana_reg5_pwdaudpll(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_pwdaudpll(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->pwdaudpll;
}

static inline void sys_aonp_ll_set_ana_reg5_pwd_rosc_spi(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_pwd_rosc_spi(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->pwd_rosc_spi;
}

static inline void sys_aonp_ll_set_ana_reg5_nc_15_15(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_nc_15_15(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->nc_15_15;
}

static inline void sys_aonp_ll_set_ana_reg5_itune_xtall(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 16, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_itune_xtall(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->itune_xtall;
}

static inline void sys_aonp_ll_set_ana_reg5_xtall_ten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_xtall_ten(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->xtall_ten;
}

static inline void sys_aonp_ll_set_ana_reg5_rosc_tsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_rosc_tsten(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->rosc_tsten;
}

static inline void sys_aonp_ll_set_ana_reg5_bcal_start(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_bcal_start(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->bcal_start;
}

static inline void sys_aonp_ll_set_ana_reg5_bcal_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_bcal_en(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->bcal_en;
}

static inline void sys_aonp_ll_set_ana_reg5_bcal_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 24, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_bcal_sel(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->bcal_sel;
}

static inline void sys_aonp_ll_set_ana_reg5_vbias(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x45 << 2)), 27, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg5_vbias(void) {
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t*)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));
	return r->vbias;
}

//reg ana_reg6:

static inline void sys_aonp_ll_set_ana_reg6_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
}

static inline void sys_aonp_ll_set_ana_reg6_calib_interval(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 0, 0x3ff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_calib_interval(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->calib_interval;
}

static inline void sys_aonp_ll_set_ana_reg6_modify_interval(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 10, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_modify_interval(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->modify_interval;
}

static inline void sys_aonp_ll_set_ana_reg6_xtal_wakeup_time(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 16, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_xtal_wakeup_time(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->xtal_wakeup_time;
}

static inline void sys_aonp_ll_set_ana_reg6_spi_trig(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_spi_trig(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->spi_trig;
}

static inline void sys_aonp_ll_set_ana_reg6_modifi_auto(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_modifi_auto(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->modifi_auto;
}

static inline void sys_aonp_ll_set_ana_reg6_calib_auto(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_calib_auto(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->calib_auto;
}

static inline void sys_aonp_ll_set_ana_reg6_cal_mode(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_cal_mode(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->cal_mode;
}

static inline void sys_aonp_ll_set_ana_reg6_manu_ena(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_manu_ena(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->manu_ena;
}

static inline void sys_aonp_ll_set_ana_reg6_manu_cin(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x46 << 2)), 25, 0x7f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg6_manu_cin(void) {
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t*)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));
	return r->manu_cin;
}

//reg ana_reg7:

static inline void sys_aonp_ll_set_ana_reg7_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
}

static inline void sys_aonp_ll_set_ana_reg7_nsyn(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_nsyn(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->nsyn;
}

static inline void sys_aonp_ll_set_ana_reg7_bandmanual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 1, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_bandmanual(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->bandmanual;
}

static inline void sys_aonp_ll_set_ana_reg7_ckref_loop_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_ckref_loop_sel(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->ckref_loop_sel;
}

static inline void sys_aonp_ll_set_ana_reg7_ioffs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 8, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_ioffs(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->ioffs;
}

static inline void sys_aonp_ll_set_ana_reg7_reset_nload(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_reset_nload(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->reset_nload;
}

static inline void sys_aonp_ll_set_ana_reg7_closeloop_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_closeloop_en(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->closeloop_en;
}

static inline void sys_aonp_ll_set_ana_reg7_modecal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_modecal(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->modecal;
}

static inline void sys_aonp_ll_set_ana_reg7_spi_rstn(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_spi_rstn(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->spi_rstn;
}

static inline void sys_aonp_ll_set_ana_reg7_osccal_trig(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_osccal_trig(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->osccal_trig;
}

static inline void sys_aonp_ll_set_ana_reg7_manual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_manual(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->manual;
}

static inline void sys_aonp_ll_set_ana_reg7_diff(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 17, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_diff(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->diff;
}

static inline void sys_aonp_ll_set_ana_reg7_ictrlmanual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 20, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_ictrlmanual(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->ictrlmanual;
}

static inline void sys_aonp_ll_set_ana_reg7_cnti(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x47 << 2)), 23, 0x1ff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg7_cnti(void) {
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t*)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));
	return r->cnti;
}

//reg ana_reg8:

static inline void sys_aonp_ll_set_ana_reg8_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x48 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg8_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x48 << 2));
}

static inline void sys_aonp_ll_set_ana_reg8_n(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x48 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg8_n(void) {
	sys_aonp_ana_reg8_t *r = (sys_aonp_ana_reg8_t*)(SOC_SYS_AONP_REG_BASE + (0x48 << 2));
	return r->n;
}

//reg ana_reg9:

static inline void sys_aonp_ll_set_ana_reg9_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
}

static inline void sys_aonp_ll_set_ana_reg9_clk_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_clk_sel(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->clk_sel;
}

static inline void sys_aonp_ll_set_ana_reg9_coreldo_hp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_coreldo_hp(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->coreldo_hp;
}

static inline void sys_aonp_ll_set_ana_reg9_dldohp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_dldohp(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->dldohp;
}

static inline void sys_aonp_ll_set_ana_reg9_t_vanaldosel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 3, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_t_vanaldosel(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->t_vanaldosel;
}

static inline void sys_aonp_ll_set_ana_reg9_r_vanaldosel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 6, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_r_vanaldosel(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->r_vanaldosel;
}

static inline void sys_aonp_ll_set_ana_reg9_en_trsw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_en_trsw(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->en_trsw;
}

static inline void sys_aonp_ll_set_ana_reg9_aldohp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_aldohp(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->aldohp;
}

static inline void sys_aonp_ll_set_ana_reg9_anacurlim(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_anacurlim(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->anacurlim;
}

static inline void sys_aonp_ll_set_ana_reg9_hsldo_hp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_hsldo_hp(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->hsldo_hp;
}

static inline void sys_aonp_ll_set_ana_reg9_pwd_hsldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_pwd_hsldo(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->pwd_hsldo;
}

static inline void sys_aonp_ll_set_ana_reg9_enfast_hsldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_enfast_hsldo(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->enfast_hsldo;
}

static inline void sys_aonp_ll_set_ana_reg9_nc_15_15(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_nc_15_15(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->nc_15_15;
}

static inline void sys_aonp_ll_set_ana_reg9_valoldosel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 16, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_valoldosel(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->valoldosel;
}

static inline void sys_aonp_ll_set_ana_reg9_alopowsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_alopowsel(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->alopowsel;
}

static inline void sys_aonp_ll_set_ana_reg9_en_fast_aloldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_en_fast_aloldo(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->en_fast_aloldo;
}

static inline void sys_aonp_ll_set_ana_reg9_aloldohp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_aloldohp(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->aloldohp;
}

static inline void sys_aonp_ll_set_ana_reg9_bgcal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 22, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_bgcal(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->bgcal;
}

static inline void sys_aonp_ll_set_ana_reg9_vbgcalmode(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_vbgcalmode(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->vbgcalmode;
}

static inline void sys_aonp_ll_set_ana_reg9_vbgcalstart(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_vbgcalstart(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->vbgcalstart;
}

static inline void sys_aonp_ll_set_ana_reg9_pwd_bgcal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_pwd_bgcal(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->pwd_bgcal;
}

static inline void sys_aonp_ll_set_ana_reg9_spi_envbg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x49 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg9_spi_envbg(void) {
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t*)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));
	return r->spi_envbg;
}

//reg ana_reg10:

static inline void sys_aonp_ll_set_ana_reg10_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
}

static inline void sys_aonp_ll_set_ana_reg10_azcd_manual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 0, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_azcd_manual(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->azcd_manual;
}

static inline void sys_aonp_ll_set_ana_reg10_azcdrefs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 6, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_azcdrefs(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->azcdrefs;
}

static inline void sys_aonp_ll_set_ana_reg10_spi_latchb(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_spi_latchb(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->spi_latchb;
}

static inline void sys_aonp_ll_set_ana_reg10_digcurlim(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_digcurlim(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->digcurlim;
}

static inline void sys_aonp_ll_set_ana_reg10_rtc_wkrstn(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_rtc_wkrstn(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->rtc_wkrstn;
}

static inline void sys_aonp_ll_set_ana_reg10_rst_wks(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_rst_wks(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->rst_wks;
}

static inline void sys_aonp_ll_set_ana_reg10_d_veasel1v(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 13, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_d_veasel1v(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->d_veasel1v;
}

static inline void sys_aonp_ll_set_ana_reg10_ensfsdd(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_ensfsdd(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->ensfsdd;
}

static inline void sys_aonp_ll_set_ana_reg10_vcorehsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 16, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_vcorehsel(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->vcorehsel;
}

static inline void sys_aonp_ll_set_ana_reg10_vcorelsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 20, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_vcorelsel(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->vcorelsel;
}

static inline void sys_aonp_ll_set_ana_reg10_vlden(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_vlden(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->vlden;
}

static inline void sys_aonp_ll_set_ana_reg10_en_fast_coreldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_en_fast_coreldo(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->en_fast_coreldo;
}

static inline void sys_aonp_ll_set_ana_reg10_pwdcoreldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 25, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_pwdcoreldo(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->pwdcoreldo;
}

static inline void sys_aonp_ll_set_ana_reg10_vdighsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 26, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_vdighsel(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->vdighsel;
}

static inline void sys_aonp_ll_set_ana_reg10_vdigsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 29, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_vdigsel(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->vdigsel;
}

static inline void sys_aonp_ll_set_ana_reg10_vdd12lden(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4a << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg10_vdd12lden(void) {
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t*)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));
	return r->vdd12lden;
}

//reg ana_reg11:

static inline void sys_aonp_ll_set_ana_reg11_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
}

static inline void sys_aonp_ll_set_ana_reg11_aldo_czsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 0, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_aldo_czsel(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->aldo_czsel;
}

static inline void sys_aonp_ll_set_ana_reg11_zldo_rzsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 3, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_zldo_rzsel(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->zldo_rzsel;
}

static inline void sys_aonp_ll_set_ana_reg11_azcdswvs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 5, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_azcdswvs(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->azcdswvs;
}

static inline void sys_aonp_ll_set_ana_reg11_aenzcddy(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 8, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_aenzcddy(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->aenzcddy;
}

static inline void sys_aonp_ll_set_ana_reg11_aenzcdmsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_aenzcdmsel(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->aenzcdmsel;
}

static inline void sys_aonp_ll_set_ana_reg11_aenzcdcalib(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_aenzcdcalib(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->aenzcdcalib;
}

static inline void sys_aonp_ll_set_ana_reg11_en_corepsw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_en_corepsw(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->en_corepsw;
}

static inline void sys_aonp_ll_set_ana_reg11_en_alopsw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_en_alopsw(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->en_alopsw;
}

static inline void sys_aonp_ll_set_ana_reg11_nc_13_14(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 13, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_nc_13_14(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->nc_13_14;
}

static inline void sys_aonp_ll_set_ana_reg11_spi_timerwken(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_spi_timerwken(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->spi_timerwken;
}

static inline void sys_aonp_ll_set_ana_reg11_spi_byp32pwd(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_spi_byp32pwd(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->spi_byp32pwd;
}

static inline void sys_aonp_ll_set_ana_reg11_sd(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 17, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_sd(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->sd;
}

static inline void sys_aonp_ll_set_ana_reg11_nc_18_18(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 18, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_nc_18_18(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->nc_18_18;
}

static inline void sys_aonp_ll_set_ana_reg11_gpio_wkrst1v(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_gpio_wkrst1v(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->gpio_wkrst1v;
}

static inline void sys_aonp_ll_set_ana_reg11_ckfs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 20, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_ckfs(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->ckfs;
}

static inline void sys_aonp_ll_set_ana_reg11_ckintsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 22, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_ckintsel(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->ckintsel;
}

static inline void sys_aonp_ll_set_ana_reg11_osccaltrig(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_osccaltrig(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->osccaltrig;
}

static inline void sys_aonp_ll_set_ana_reg11_mroscsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_mroscsel(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->mroscsel;
}

static inline void sys_aonp_ll_set_ana_reg11_mrosci_cal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 25, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_mrosci_cal(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->mrosci_cal;
}

static inline void sys_aonp_ll_set_ana_reg11_mrosccap_cal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4b << 2)), 28, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg11_mrosccap_cal(void) {
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t*)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));
	return r->mrosccap_cal;
}

//reg ana_reg12:

static inline void sys_aonp_ll_set_ana_reg12_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
}

static inline void sys_aonp_ll_set_ana_reg12_sfsr(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 0, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_sfsr(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->sfsr;
}

static inline void sys_aonp_ll_set_ana_reg12_ensfsaa(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_ensfsaa(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->ensfsaa;
}

static inline void sys_aonp_ll_set_ana_reg12_apfms(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 5, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_apfms(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->apfms;
}

static inline void sys_aonp_ll_set_ana_reg12_atmpo_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 10, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_atmpo_sel(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->atmpo_sel;
}

static inline void sys_aonp_ll_set_ana_reg12_ampoen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_ampoen(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->ampoen;
}

static inline void sys_aonp_ll_set_ana_reg12_enpowa(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_enpowa(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->enpowa;
}

static inline void sys_aonp_ll_set_ana_reg12_avea_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 14, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_avea_sel(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->avea_sel;
}

static inline void sys_aonp_ll_set_ana_reg12_aforcepfm(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_aforcepfm(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->aforcepfm;
}

static inline void sys_aonp_ll_set_ana_reg12_acls(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 17, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_acls(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->acls;
}

static inline void sys_aonp_ll_set_ana_reg12_aswrsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_aswrsten(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->aswrsten;
}

static inline void sys_aonp_ll_set_ana_reg12_aripc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 21, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_aripc(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->aripc;
}

static inline void sys_aonp_ll_set_ana_reg12_arampc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 24, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_arampc(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->arampc;
}

static inline void sys_aonp_ll_set_ana_reg12_arampcen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_arampcen(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->arampcen;
}

static inline void sys_aonp_ll_set_ana_reg12_aenburst(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_aenburst(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->aenburst;
}

static inline void sys_aonp_ll_set_ana_reg12_apfmen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_apfmen(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->apfmen;
}

static inline void sys_aonp_ll_set_ana_reg12_aldosel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4c << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg12_aldosel(void) {
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t*)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));
	return r->aldosel;
}

//reg ana_reg13:

static inline void sys_aonp_ll_set_ana_reg13_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
}

static inline void sys_aonp_ll_set_ana_reg13_buckd_softst(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 0, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_buckd_softst(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->buckd_softst;
}

static inline void sys_aonp_ll_set_ana_reg13_denzcdcalib(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_denzcdcalib(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->denzcdcalib;
}

static inline void sys_aonp_ll_set_ana_reg13_nc_5_6(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 5, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_nc_5_6(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->nc_5_6;
}

static inline void sys_aonp_ll_set_ana_reg13_vddgpio_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 7, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_vddgpio_sel(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->vddgpio_sel;
}

static inline void sys_aonp_ll_set_ana_reg13_dpfms(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 8, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dpfms(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dpfms;
}

static inline void sys_aonp_ll_set_ana_reg13_dtmpo_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 13, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dtmpo_sel(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dtmpo_sel;
}

static inline void sys_aonp_ll_set_ana_reg13_dmpoen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dmpoen(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dmpoen;
}

static inline void sys_aonp_ll_set_ana_reg13_dforcepfm(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dforcepfm(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dforcepfm;
}

static inline void sys_aonp_ll_set_ana_reg13_dcls(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 17, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dcls(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dcls;
}

static inline void sys_aonp_ll_set_ana_reg13_dswrsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dswrsten(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dswrsten;
}

static inline void sys_aonp_ll_set_ana_reg13_dripc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 21, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dripc(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dripc;
}

static inline void sys_aonp_ll_set_ana_reg13_drampc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 24, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_drampc(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->drampc;
}

static inline void sys_aonp_ll_set_ana_reg13_drampcen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_drampcen(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->drampcen;
}

static inline void sys_aonp_ll_set_ana_reg13_denburst(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_denburst(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->denburst;
}

static inline void sys_aonp_ll_set_ana_reg13_dpfmen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dpfmen(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dpfmen;
}

static inline void sys_aonp_ll_set_ana_reg13_dldosel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4d << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg13_dldosel(void) {
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t*)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));
	return r->dldosel;
}

//reg ana_reg14:

static inline void sys_aonp_ll_set_ana_reg14_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
}

static inline void sys_aonp_ll_set_ana_reg14_pwdovp1v(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_pwdovp1v(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->pwdovp1v;
}

static inline void sys_aonp_ll_set_ana_reg14_asoft_stc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 1, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_asoft_stc(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->asoft_stc;
}

static inline void sys_aonp_ll_set_ana_reg14_dldo_czsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 5, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_dldo_czsel(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->dldo_czsel;
}

static inline void sys_aonp_ll_set_ana_reg14_dldo_rzsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 8, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_dldo_rzsel(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->dldo_rzsel;
}

static inline void sys_aonp_ll_set_ana_reg14_en_usbvcc18(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_en_usbvcc18(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->en_usbvcc18;
}

static inline void sys_aonp_ll_set_ana_reg14_en_usbvcc3v(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_en_usbvcc3v(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->en_usbvcc3v;
}

static inline void sys_aonp_ll_set_ana_reg14_vtrxspisel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 12, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_vtrxspisel(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->vtrxspisel;
}

static inline void sys_aonp_ll_set_ana_reg14_denzcddy(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_denzcddy(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->denzcddy;
}

static inline void sys_aonp_ll_set_ana_reg14_dzcd_swvs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 15, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_dzcd_swvs(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->dzcd_swvs;
}

static inline void sys_aonp_ll_set_ana_reg14_dzcd_refs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 18, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_dzcd_refs(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->dzcd_refs;
}

static inline void sys_aonp_ll_set_ana_reg14_dzcd_manu(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 21, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_dzcd_manu(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->dzcd_manu;
}

static inline void sys_aonp_ll_set_ana_reg14_azcdmsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 27, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_azcdmsel(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->azcdmsel;
}

static inline void sys_aonp_ll_set_ana_reg14_psldo_swb(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_psldo_swb(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->psldo_swb;
}

static inline void sys_aonp_ll_set_ana_reg14_vpsramsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 29, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_vpsramsel(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->vpsramsel;
}

static inline void sys_aonp_ll_set_ana_reg14_enpsram(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4e << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg14_enpsram(void) {
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t*)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));
	return r->enpsram;
}

//reg ana_reg15:

static inline void sys_aonp_ll_set_ana_reg15_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x4f << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg15_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x4f << 2));
}

static inline void sys_aonp_ll_set_ana_reg15_gpiowken(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x4f << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg15_gpiowken(void) {
	sys_aonp_ana_reg15_t *r = (sys_aonp_ana_reg15_t*)(SOC_SYS_AONP_REG_BASE + (0x4f << 2));
	return r->gpiowken;
}

//reg ana_reg16:

static inline void sys_aonp_ll_set_ana_reg16_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
}

static inline void sys_aonp_ll_set_ana_reg16_timer_set(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 0, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_timer_set(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->timer_set;
}

static inline void sys_aonp_ll_set_ana_reg16_rtc_set(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 4, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_rtc_set(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->rtc_set;
}

static inline void sys_aonp_ll_set_ana_reg16_nc_8_10(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 8, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_nc_8_10(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->nc_8_10;
}

static inline void sys_aonp_ll_set_ana_reg16_vcorehssel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 11, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_vcorehssel(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->vcorehssel;
}

static inline void sys_aonp_ll_set_ana_reg16_vbuckhssel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 15, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_vbuckhssel(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->vbuckhssel;
}

static inline void sys_aonp_ll_set_ana_reg16_hsenfast(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_hsenfast(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->hsenfast;
}

static inline void sys_aonp_ll_set_ana_reg16_enhspw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_enhspw(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->enhspw;
}

static inline void sys_aonp_ll_set_ana_reg16_buckhs_soft_stc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 20, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_buckhs_soft_stc(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->buckhs_soft_stc;
}

static inline void sys_aonp_ll_set_ana_reg16_hs_veasel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 24, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_hs_veasel(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->hs_veasel;
}

static inline void sys_aonp_ll_set_ana_reg16_hszcd_manual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x50 << 2)), 26, 0x3f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg16_hszcd_manual(void) {
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t*)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));
	return r->hszcd_manual;
}

//reg ana_reg17:

static inline void sys_aonp_ll_set_ana_reg17_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x51 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg17_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x51 << 2));
}

static inline void sys_aonp_ll_set_ana_reg17_rtc_set(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x51 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg17_rtc_set(void) {
	sys_aonp_ana_reg17_t *r = (sys_aonp_ana_reg17_t*)(SOC_SYS_AONP_REG_BASE + (0x51 << 2));
	return r->rtc_set;
}

//reg ana_reg18:

static inline void sys_aonp_ll_set_ana_reg18_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x52 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg18_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x52 << 2));
}

static inline void sys_aonp_ll_set_ana_reg18_timer_set(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x52 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg18_timer_set(void) {
	sys_aonp_ana_reg18_t *r = (sys_aonp_ana_reg18_t*)(SOC_SYS_AONP_REG_BASE + (0x52 << 2));
	return r->timer_set;
}

//reg ana_reg19:

static inline void sys_aonp_ll_set_ana_reg19_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
}

static inline void sys_aonp_ll_set_ana_reg19_hsenzcddy(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsenzcddy(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsenzcddy;
}

static inline void sys_aonp_ll_set_ana_reg19_hsenzcdcalib(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsenzcdcalib(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsenzcdcalib;
}

static inline void sys_aonp_ll_set_ana_reg19_hszcdswvs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 2, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hszcdswvs(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hszcdswvs;
}

static inline void sys_aonp_ll_set_ana_reg19_hszcdrefs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 5, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hszcdrefs(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hszcdrefs;
}

static inline void sys_aonp_ll_set_ana_reg19_hszcdmsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hszcdmsel(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hszcdmsel;
}

static inline void sys_aonp_ll_set_ana_reg19_hspfms(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 9, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hspfms(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hspfms;
}

static inline void sys_aonp_ll_set_ana_reg19_hstmpo_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 14, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hstmpo_sel(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hstmpo_sel;
}

static inline void sys_aonp_ll_set_ana_reg19_hsmpoen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsmpoen(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsmpoen;
}

static inline void sys_aonp_ll_set_ana_reg19_hsforcepfm(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsforcepfm(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsforcepfm;
}

static inline void sys_aonp_ll_set_ana_reg19_hscls(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 18, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hscls(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hscls;
}

static inline void sys_aonp_ll_set_ana_reg19_hsswrsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsswrsten(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsswrsten;
}

static inline void sys_aonp_ll_set_ana_reg19_hsripc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 22, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsripc(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsripc;
}

static inline void sys_aonp_ll_set_ana_reg19_hsrampc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 25, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsrampc(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsrampc;
}

static inline void sys_aonp_ll_set_ana_reg19_hsrampcen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsrampcen(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsrampcen;
}

static inline void sys_aonp_ll_set_ana_reg19_hsenburst(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hsenburst(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hsenburst;
}

static inline void sys_aonp_ll_set_ana_reg19_hspfmen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x53 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg19_hspfmen(void) {
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t*)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));
	return r->hspfmen;
}

//reg ana_reg20:

static inline void sys_aonp_ll_set_ana_reg20_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
}

static inline void sys_aonp_ll_set_ana_reg20_iselaud(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_iselaud(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->iselaud;
}

static inline void sys_aonp_ll_set_ana_reg20_audck_rlcen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_audck_rlcen(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->audck_rlcen;
}

static inline void sys_aonp_ll_set_ana_reg20_lchckinven(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_lchckinven(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->lchckinven;
}

static inline void sys_aonp_ll_set_ana_reg20_enaudbias(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_enaudbias(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->enaudbias;
}

static inline void sys_aonp_ll_set_ana_reg20_enadcbias(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_enadcbias(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->enadcbias;
}

static inline void sys_aonp_ll_set_ana_reg20_enmicbias(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_enmicbias(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->enmicbias;
}

static inline void sys_aonp_ll_set_ana_reg20_adcckinven(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_adcckinven(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->adcckinven;
}

static inline void sys_aonp_ll_set_ana_reg20_spi(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_spi(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->spi;
}

static inline void sys_aonp_ll_set_ana_reg20_adctsten(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_adctsten(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->adctsten;
}

static inline void sys_aonp_ll_set_ana_reg20_micbias_trm(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 9, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_micbias_trm(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->micbias_trm;
}

static inline void sys_aonp_ll_set_ana_reg20_micbias_voc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 11, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_micbias_voc(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->micbias_voc;
}

static inline void sys_aonp_ll_set_ana_reg20_vrefsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_vrefsel(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->vrefsel;
}

static inline void sys_aonp_ll_set_ana_reg20_capsw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 17, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_capsw(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->capsw;
}

static inline void sys_aonp_ll_set_ana_reg20_adcref_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 22, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_adcref_sel(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->adcref_sel;
}

static inline void sys_aonp_ll_set_ana_reg20_adcvcmsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 24, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_adcvcmsel(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->adcvcmsel;
}

static inline void sys_aonp_ll_set_ana_reg20_spi_1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_spi_1(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->spi_1;
}

static inline void sys_aonp_ll_set_ana_reg20_audadjref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x54 << 2)), 27, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg20_audadjref(void) {
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t*)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));
	return r->audadjref;
}

//reg ana_reg21:

static inline void sys_aonp_ll_set_ana_reg21_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
}

static inline void sys_aonp_ll_set_ana_reg21_isel_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 0, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_isel_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->isel_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_micirsel1_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_micirsel1_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->micirsel1_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_vcmsel_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_vcmsel_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->vcmsel_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_enfsr_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_enfsr_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->enfsr_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_enopoclip_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_enopoclip_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->enopoclip_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_da2aden_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_da2aden_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->da2aden_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_imatch_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 7, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_imatch_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->imatch_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_imatch_en_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_imatch_en_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->imatch_en_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_dccompen_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_dccompen_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->dccompen_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_micsingleen_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_micsingleen_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->micsingleen_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_nc_14_14(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_nc_14_14(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->nc_14_14;
}

static inline void sys_aonp_ll_set_ana_reg21_micgain_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 15, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_micgain_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->micgain_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_nc_19_23(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 19, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_nc_19_23(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->nc_19_23;
}

static inline void sys_aonp_ll_set_ana_reg21_dwamode_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_dwamode_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->dwamode_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_nc_25_27(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 25, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_nc_25_27(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->nc_25_27;
}

static inline void sys_aonp_ll_set_ana_reg21_micen_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_micen_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->micen_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_rst_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_rst_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->rst_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_bpdwa1v_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_bpdwa1v_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->bpdwa1v_mic1;
}

static inline void sys_aonp_ll_set_ana_reg21_hcen1stg_mic1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x55 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg21_hcen1stg_mic1(void) {
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t*)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));
	return r->hcen1stg_mic1;
}

//reg ana_reg22:

static inline void sys_aonp_ll_set_ana_reg22_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
}

static inline void sys_aonp_ll_set_ana_reg22_ictrl_dsppll(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_ictrl_dsppll(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->ictrl_dsppll;
}

static inline void sys_aonp_ll_set_ana_reg22_lvref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 1, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_lvref(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->lvref;
}

static inline void sys_aonp_ll_set_ana_reg22_nc_4_18(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 4, 0x7fff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_nc_4_18(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->nc_4_18;
}

static inline void sys_aonp_ll_set_ana_reg22_mode(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_mode(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->mode;
}

static inline void sys_aonp_ll_set_ana_reg22_iamsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_iamsel(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->iamsel;
}

static inline void sys_aonp_ll_set_ana_reg22_hvref(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 21, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_hvref(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->hvref;
}

static inline void sys_aonp_ll_set_ana_reg22_lvref_1(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 23, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_lvref_1(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->lvref_1;
}

static inline void sys_aonp_ll_set_ana_reg22_nc_25_31(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x56 << 2)), 25, 0x7f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg22_nc_25_31(void) {
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t*)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));
	return r->nc_25_31;
}

//reg ana_reg23:

static inline void sys_aonp_ll_set_ana_reg23_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
}

static inline void sys_aonp_ll_set_ana_reg23_camsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_camsel(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->camsel;
}

static inline void sys_aonp_ll_set_ana_reg23_msw(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 1, 0x1ff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_msw(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->msw;
}

static inline void sys_aonp_ll_set_ana_reg23_tstcken_dpll(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_tstcken_dpll(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->tstcken_dpll;
}

static inline void sys_aonp_ll_set_ana_reg23_osccal_trig(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_osccal_trig(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->osccal_trig;
}

static inline void sys_aonp_ll_set_ana_reg23_cnti(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 12, 0x1ff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_cnti(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->cnti;
}

static inline void sys_aonp_ll_set_ana_reg23_nc_21_21(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_nc_21_21(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->nc_21_21;
}

static inline void sys_aonp_ll_set_ana_reg23_spi_rst(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_spi_rst(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->spi_rst;
}

static inline void sys_aonp_ll_set_ana_reg23_closeloop_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_closeloop_en(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->closeloop_en;
}

static inline void sys_aonp_ll_set_ana_reg23_caltime(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_caltime(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->caltime;
}

static inline void sys_aonp_ll_set_ana_reg23_lpfrz(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 25, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_lpfrz(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->lpfrz;
}

static inline void sys_aonp_ll_set_ana_reg23_icp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 27, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_icp(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->icp;
}

static inline void sys_aonp_ll_set_ana_reg23_cp2ctrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x57 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg23_cp2ctrl(void) {
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t*)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));
	return r->cp2ctrl;
}

//reg ana_reg24:

static inline void sys_aonp_ll_set_ana_reg24_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x58 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg24_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x58 << 2));
}

static inline void sys_aonp_ll_set_ana_reg24_nc_0_31(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x58 << 2)), 0, 0xffffffff, v);
}

//reg ana_reg25:

static inline void sys_aonp_ll_set_ana_reg25_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
}

static inline void sys_aonp_ll_set_ana_reg25_int_mod(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_int_mod(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->int_mod;
}

static inline void sys_aonp_ll_set_ana_reg25_nsyn(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_nsyn(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->nsyn;
}

static inline void sys_aonp_ll_set_ana_reg25_open_enb(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_open_enb(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->open_enb;
}

static inline void sys_aonp_ll_set_ana_reg25_reset(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_reset(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->reset;
}

static inline void sys_aonp_ll_set_ana_reg25_ioffsetl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 4, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_ioffsetl(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->ioffsetl;
}

static inline void sys_aonp_ll_set_ana_reg25_lpfrz(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 7, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_lpfrz(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->lpfrz;
}

static inline void sys_aonp_ll_set_ana_reg25_vsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 11, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_vsel(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->vsel;
}

static inline void sys_aonp_ll_set_ana_reg25_vsel_cal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_vsel_cal(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->vsel_cal;
}

static inline void sys_aonp_ll_set_ana_reg25_pwd_lockdet(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_pwd_lockdet(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->pwd_lockdet;
}

static inline void sys_aonp_ll_set_ana_reg25_lockdet_bypass(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_lockdet_bypass(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->lockdet_bypass;
}

static inline void sys_aonp_ll_set_ana_reg25_ckref_loop_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_ckref_loop_sel(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->ckref_loop_sel;
}

static inline void sys_aonp_ll_set_ana_reg25_spi_trigger(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_spi_trigger(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->spi_trigger;
}

static inline void sys_aonp_ll_set_ana_reg25_manual(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_manual(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->manual;
}

static inline void sys_aonp_ll_set_ana_reg25_test_ckaudio_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_test_ckaudio_en(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->test_ckaudio_en;
}

static inline void sys_aonp_ll_set_ana_reg25_ck2xen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_ck2xen(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->ck2xen;
}

static inline void sys_aonp_ll_set_ana_reg25_icp(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 22, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_icp(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->icp;
}

static inline void sys_aonp_ll_set_ana_reg25_cktst_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_cktst_sel(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->cktst_sel;
}

static inline void sys_aonp_ll_set_ana_reg25_edgesel_nck(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_edgesel_nck(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->edgesel_nck;
}

static inline void sys_aonp_ll_set_ana_reg25_nloaddlyen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_nloaddlyen(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->nloaddlyen;
}

static inline void sys_aonp_ll_set_ana_reg25_bypass_caldone_auto(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_bypass_caldone_auto(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->bypass_caldone_auto;
}

static inline void sys_aonp_ll_set_ana_reg25_cal_res_spi(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 28, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_cal_res_spi(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->cal_res_spi;
}

static inline void sys_aonp_ll_set_ana_reg25_audioen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x59 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg25_audioen(void) {
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t*)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));
	return r->audioen;
}

//reg ana_reg26:

static inline void sys_aonp_ll_set_ana_reg26_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x5a << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg26_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x5a << 2));
}

static inline void sys_aonp_ll_set_ana_reg26_n(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5a << 2)), 0, 0x3fffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg26_n(void) {
	sys_aonp_ana_reg26_t *r = (sys_aonp_ana_reg26_t*)(SOC_SYS_AONP_REG_BASE + (0x5a << 2));
	return r->n;
}

static inline void sys_aonp_ll_set_ana_reg26_calres_spien(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5a << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg26_calres_spien(void) {
	sys_aonp_ana_reg26_t *r = (sys_aonp_ana_reg26_t*)(SOC_SYS_AONP_REG_BASE + (0x5a << 2));
	return r->calres_spien;
}

static inline void sys_aonp_ll_set_ana_reg26_calrefen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5a << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg26_calrefen(void) {
	sys_aonp_ana_reg26_t *r = (sys_aonp_ana_reg26_t*)(SOC_SYS_AONP_REG_BASE + (0x5a << 2));
	return r->calrefen;
}

//reg ana_reg27:

static inline void sys_aonp_ll_set_ana_reg27_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
}

static inline void sys_aonp_ll_set_ana_reg27_isel_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 0, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_isel_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->isel_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_micirsel1_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_micirsel1_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->micirsel1_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_vcmsel_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 3, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_vcmsel_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->vcmsel_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_enfsr_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_enfsr_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->enfsr_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_enopoclip_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_enopoclip_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->enopoclip_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_da2aden_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 6, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_da2aden_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->da2aden_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_imatch_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 7, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_imatch_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->imatch_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_imatch_en_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_imatch_en_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->imatch_en_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_dccompen_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_dccompen_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->dccompen_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_micsingleen_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_micsingleen_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->micsingleen_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_nc_14_14(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_nc_14_14(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->nc_14_14;
}

static inline void sys_aonp_ll_set_ana_reg27_micgain_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 15, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_micgain_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->micgain_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_nc_19_23(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 19, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_nc_19_23(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->nc_19_23;
}

static inline void sys_aonp_ll_set_ana_reg27_dwamode_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_dwamode_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->dwamode_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_nc_25_27(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 25, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_nc_25_27(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->nc_25_27;
}

static inline void sys_aonp_ll_set_ana_reg27_micen_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_micen_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->micen_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_rst_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_rst_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->rst_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_bpdwa1v_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_bpdwa1v_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->bpdwa1v_mic2;
}

static inline void sys_aonp_ll_set_ana_reg27_hcen1stg_mic2(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5b << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg27_hcen1stg_mic2(void) {
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t*)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));
	return r->hcen1stg_mic2;
}

//reg ana_reg28:

static inline void sys_aonp_ll_set_ana_reg28_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
}

static inline void sys_aonp_ll_set_ana_reg28_isel_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 0, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_isel_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->isel_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_micirsel1_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 2, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_micirsel1_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->micirsel1_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_vcmsel_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 3, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_vcmsel_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->vcmsel_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_enfsr_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_enfsr_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->enfsr_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_enopoclip_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_enopoclip_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->enopoclip_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_da2aden_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 6, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_da2aden_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->da2aden_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_imatch_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 7, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_imatch_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->imatch_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_imatch_en_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 11, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_imatch_en_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->imatch_en_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_dccompen_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 12, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_dccompen_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->dccompen_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_micsingleen_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_micsingleen_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->micsingleen_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_nc_14_14(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_nc_14_14(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->nc_14_14;
}

static inline void sys_aonp_ll_set_ana_reg28_micgain_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 15, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_micgain_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->micgain_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_nc_19_23(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 19, 0x1f, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_nc_19_23(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->nc_19_23;
}

static inline void sys_aonp_ll_set_ana_reg28_dwamode_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_dwamode_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->dwamode_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_nc_25_27(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 25, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_nc_25_27(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->nc_25_27;
}

static inline void sys_aonp_ll_set_ana_reg28_micen_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_micen_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->micen_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_rst_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_rst_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->rst_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_bpdwa1v_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 30, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_bpdwa1v_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->bpdwa1v_mic3;
}

static inline void sys_aonp_ll_set_ana_reg28_hcen1stg_mic3(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5c << 2)), 31, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg28_hcen1stg_mic3(void) {
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t*)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));
	return r->hcen1stg_mic3;
}

//reg ana_reg29:

static inline void sys_aonp_ll_set_ana_reg29_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
}

static inline void sys_aonp_ll_set_ana_reg29_hpdac(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 0, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_hpdac(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->hpdac;
}

static inline void sys_aonp_ll_set_ana_reg29_iselstg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 1, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_iselstg(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->iselstg;
}

static inline void sys_aonp_ll_set_ana_reg29_oscdac(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 2, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_oscdac(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->oscdac;
}

static inline void sys_aonp_ll_set_ana_reg29_ocendac(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 4, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_ocendac(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->ocendac;
}

static inline void sys_aonp_ll_set_ana_reg29_vseldco(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 5, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_vseldco(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->vseldco;
}

static inline void sys_aonp_ll_set_ana_reg29_srsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 6, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_srsel(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->srsel;
}

static inline void sys_aonp_ll_set_ana_reg29_hpoen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 7, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_hpoen(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->hpoen;
}

static inline void sys_aonp_ll_set_ana_reg29_lbwen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 8, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_lbwen(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->lbwen;
}

static inline void sys_aonp_ll_set_ana_reg29_calsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 9, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_calsel(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->calsel;
}

static inline void sys_aonp_ll_set_ana_reg29_bp2vldo(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 10, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_bp2vldo(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->bp2vldo;
}

static inline void sys_aonp_ll_set_ana_reg29_dcochg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 11, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_dcochg(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->dcochg;
}

static inline void sys_aonp_ll_set_ana_reg29_diffen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 13, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_diffen(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->diffen;
}

static inline void sys_aonp_ll_set_ana_reg29_endaccal(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 14, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_endaccal(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->endaccal;
}

static inline void sys_aonp_ll_set_ana_reg29_rendcoc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 15, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_rendcoc(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->rendcoc;
}

static inline void sys_aonp_ll_set_ana_reg29_lendcoc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_lendcoc(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->lendcoc;
}

static inline void sys_aonp_ll_set_ana_reg29_renvcmd(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 17, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_renvcmd(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->renvcmd;
}

static inline void sys_aonp_ll_set_ana_reg29_lenvcmd(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 18, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_lenvcmd(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->lenvcmd;
}

static inline void sys_aonp_ll_set_ana_reg29_dacdrven(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_dacdrven(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->dacdrven;
}

static inline void sys_aonp_ll_set_ana_reg29_dacren(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_dacren(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->dacren;
}

static inline void sys_aonp_ll_set_ana_reg29_daclen(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_daclen(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->daclen;
}

static inline void sys_aonp_ll_set_ana_reg29_dacg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 22, 0xf, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_dacg(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->dacg;
}

static inline void sys_aonp_ll_set_ana_reg29_dacmute(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_dacmute(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->dacmute;
}

static inline void sys_aonp_ll_set_ana_reg29_dacdwamode_sel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 27, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_dacdwamode_sel(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->dacdwamode_sel;
}

static inline void sys_aonp_ll_set_ana_reg29_ckpsel(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 28, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_ckpsel(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->ckpsel;
}

static inline void sys_aonp_ll_set_ana_reg29_nc_29_31(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5d << 2)), 29, 0x7, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg29_nc_29_31(void) {
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t*)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));
	return r->nc_29_31;
}

//reg ana_reg30:

static inline void sys_aonp_ll_set_ana_reg30_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
}

static inline void sys_aonp_ll_set_ana_reg30_lmdcin(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 0, 0xff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_lmdcin(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->lmdcin;
}

static inline void sys_aonp_ll_set_ana_reg30_rmdcin(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 8, 0xff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_rmdcin(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->rmdcin;
}

static inline void sys_aonp_ll_set_ana_reg30_spirst_ovc(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 16, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_spirst_ovc(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->spirst_ovc;
}

static inline void sys_aonp_ll_set_ana_reg30_enidacr(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 17, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_enidacr(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->enidacr;
}

static inline void sys_aonp_ll_set_ana_reg30_enidacl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 18, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_enidacl(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->enidacl;
}

static inline void sys_aonp_ll_set_ana_reg30_dac3rdhc0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 19, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_dac3rdhc0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->dac3rdhc0v9;
}

static inline void sys_aonp_ll_set_ana_reg30_hc2s(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 20, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_hc2s(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->hc2s;
}

static inline void sys_aonp_ll_set_ana_reg30_sng_fb_en(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 21, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_sng_fb_en(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->sng_fb_en;
}

static inline void sys_aonp_ll_set_ana_reg30_rfb_ctrl(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 22, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_rfb_ctrl(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->rfb_ctrl;
}

static inline void sys_aonp_ll_set_ana_reg30_enbs(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 23, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_enbs(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->enbs;
}

static inline void sys_aonp_ll_set_ana_reg30_calck_sel0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 24, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_calck_sel0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->calck_sel0v9;
}

static inline void sys_aonp_ll_set_ana_reg30_bpdwa0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 25, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_bpdwa0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->bpdwa0v9;
}

static inline void sys_aonp_ll_set_ana_reg30_looprst0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 26, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_looprst0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->looprst0v9;
}

static inline void sys_aonp_ll_set_ana_reg30_oct0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 27, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_oct0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->oct0v9;
}

static inline void sys_aonp_ll_set_ana_reg30_sout0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 29, 0x1, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_sout0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->sout0v9;
}

static inline void sys_aonp_ll_set_ana_reg30_hc0v9(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5e << 2)), 30, 0x3, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg30_hc0v9(void) {
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t*)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));
	return r->hc0v9;
}

//reg ana_reg31:

static inline void sys_aonp_ll_set_ana_reg31_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x5f << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg31_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x5f << 2));
}

static inline void sys_aonp_ll_set_ana_reg31_nc_0_31(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x5f << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg31_nc_0_31(void) {
	sys_aonp_ana_reg31_t *r = (sys_aonp_ana_reg31_t*)(SOC_SYS_AONP_REG_BASE + (0x5f << 2));
	return r->nc_0_31;
}

//reg ana_reg32:

static inline void sys_aonp_ll_set_ana_reg32_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x60 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg32_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x60 << 2));
}

static inline void sys_aonp_ll_set_ana_reg32_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x60 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg32_new_ana_reg(void) {
	sys_aonp_ana_reg32_t *r = (sys_aonp_ana_reg32_t*)(SOC_SYS_AONP_REG_BASE + (0x60 << 2));
	return r->new_ana_reg;
}

//reg ana_reg33:

static inline void sys_aonp_ll_set_ana_reg33_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x61 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg33_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x61 << 2));
}

static inline void sys_aonp_ll_set_ana_reg33_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x61 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg33_new_ana_reg(void) {
	sys_aonp_ana_reg33_t *r = (sys_aonp_ana_reg33_t*)(SOC_SYS_AONP_REG_BASE + (0x61 << 2));
	return r->new_ana_reg;
}

//reg ana_reg34:

static inline void sys_aonp_ll_set_ana_reg34_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x62 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg34_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x62 << 2));
}

static inline void sys_aonp_ll_set_ana_reg34_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x62 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg34_new_ana_reg(void) {
	sys_aonp_ana_reg34_t *r = (sys_aonp_ana_reg34_t*)(SOC_SYS_AONP_REG_BASE + (0x62 << 2));
	return r->new_ana_reg;
}

//reg ana_reg35:

static inline void sys_aonp_ll_set_ana_reg35_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x63 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg35_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x63 << 2));
}

static inline void sys_aonp_ll_set_ana_reg35_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x63 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg35_new_ana_reg(void) {
	sys_aonp_ana_reg35_t *r = (sys_aonp_ana_reg35_t*)(SOC_SYS_AONP_REG_BASE + (0x63 << 2));
	return r->new_ana_reg;
}

//reg ana_reg36:

static inline void sys_aonp_ll_set_ana_reg36_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x64 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg36_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x64 << 2));
}

static inline void sys_aonp_ll_set_ana_reg36_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x64 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg36_new_ana_reg(void) {
	sys_aonp_ana_reg36_t *r = (sys_aonp_ana_reg36_t*)(SOC_SYS_AONP_REG_BASE + (0x64 << 2));
	return r->new_ana_reg;
}

//reg ana_reg37:

static inline void sys_aonp_ll_set_ana_reg37_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x65 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg37_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x65 << 2));
}

static inline void sys_aonp_ll_set_ana_reg37_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x65 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg37_new_ana_reg(void) {
	sys_aonp_ana_reg37_t *r = (sys_aonp_ana_reg37_t*)(SOC_SYS_AONP_REG_BASE + (0x65 << 2));
	return r->new_ana_reg;
}

//reg ana_reg38:

static inline void sys_aonp_ll_set_ana_reg38_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x66 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg38_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x66 << 2));
}

static inline void sys_aonp_ll_set_ana_reg38_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x66 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg38_new_ana_reg(void) {
	sys_aonp_ana_reg38_t *r = (sys_aonp_ana_reg38_t*)(SOC_SYS_AONP_REG_BASE + (0x66 << 2));
	return r->new_ana_reg;
}

//reg ana_reg39:

static inline void sys_aonp_ll_set_ana_reg39_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x67 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg39_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x67 << 2));
}

static inline void sys_aonp_ll_set_ana_reg39_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x67 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg39_new_ana_reg(void) {
	sys_aonp_ana_reg39_t *r = (sys_aonp_ana_reg39_t*)(SOC_SYS_AONP_REG_BASE + (0x67 << 2));
	return r->new_ana_reg;
}

//reg ana_reg40:

static inline void sys_aonp_ll_set_ana_reg40_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x68 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg40_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x68 << 2));
}

static inline void sys_aonp_ll_set_ana_reg40_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x68 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg40_new_ana_reg(void) {
	sys_aonp_ana_reg40_t *r = (sys_aonp_ana_reg40_t*)(SOC_SYS_AONP_REG_BASE + (0x68 << 2));
	return r->new_ana_reg;
}

//reg ana_reg41:

static inline void sys_aonp_ll_set_ana_reg41_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x69 << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg41_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x69 << 2));
}

static inline void sys_aonp_ll_set_ana_reg41_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x69 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg41_new_ana_reg(void) {
	sys_aonp_ana_reg41_t *r = (sys_aonp_ana_reg41_t*)(SOC_SYS_AONP_REG_BASE + (0x69 << 2));
	return r->new_ana_reg;
}

//reg ana_reg42:

static inline void sys_aonp_ll_set_ana_reg42_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x6a << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg42_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x6a << 2));
}

static inline void sys_aonp_ll_set_ana_reg42_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x6a << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg42_new_ana_reg(void) {
	sys_aonp_ana_reg42_t *r = (sys_aonp_ana_reg42_t*)(SOC_SYS_AONP_REG_BASE + (0x6a << 2));
	return r->new_ana_reg;
}

//reg ana_reg43:

static inline void sys_aonp_ll_set_ana_reg43_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x6b << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg43_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x6b << 2));
}

static inline void sys_aonp_ll_set_ana_reg43_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x6b << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg43_new_ana_reg(void) {
	sys_aonp_ana_reg43_t *r = (sys_aonp_ana_reg43_t*)(SOC_SYS_AONP_REG_BASE + (0x6b << 2));
	return r->new_ana_reg;
}

//reg ana_reg44:

static inline void sys_aonp_ll_set_ana_reg44_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x6c << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg44_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x6c << 2));
}

static inline void sys_aonp_ll_set_ana_reg44_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x6c << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg44_new_ana_reg(void) {
	sys_aonp_ana_reg44_t *r = (sys_aonp_ana_reg44_t*)(SOC_SYS_AONP_REG_BASE + (0x6c << 2));
	return r->new_ana_reg;
}

//reg ana_reg45:

static inline void sys_aonp_ll_set_ana_reg45_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x6d << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg45_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x6d << 2));
}

static inline void sys_aonp_ll_set_ana_reg45_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x6d << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg45_new_ana_reg(void) {
	sys_aonp_ana_reg45_t *r = (sys_aonp_ana_reg45_t*)(SOC_SYS_AONP_REG_BASE + (0x6d << 2));
	return r->new_ana_reg;
}

//reg ana_reg46:

static inline void sys_aonp_ll_set_ana_reg46_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x6e << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg46_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x6e << 2));
}

static inline void sys_aonp_ll_set_ana_reg46_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x6e << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg46_new_ana_reg(void) {
	sys_aonp_ana_reg46_t *r = (sys_aonp_ana_reg46_t*)(SOC_SYS_AONP_REG_BASE + (0x6e << 2));
	return r->new_ana_reg;
}

//reg ana_reg47:

static inline void sys_aonp_ll_set_ana_reg47_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_AONP_REG_BASE + (0x6f << 2)), v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg47_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_AONP_REG_BASE + (0x6f << 2));
}

static inline void sys_aonp_ll_set_ana_reg47_new_ana_reg(uint32_t v) {
	sys_aonp_set_ana_reg_bit((SOC_SYS_AONP_REG_BASE + (0x6f << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_aonp_ll_get_ana_reg47_new_ana_reg(void) {
	sys_aonp_ana_reg47_t *r = (sys_aonp_ana_reg47_t*)(SOC_SYS_AONP_REG_BASE + (0x6f << 2));
	return r->new_ana_reg;
}
#ifdef __cplusplus
}
#endif
