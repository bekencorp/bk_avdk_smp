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

#define SYS_LL_REG_BASE   SOC_SYS_REG_BASE

//This way of setting ana_reg_bit value is only for sys_ctrl, other driver please implement by yourself!!

#define SYS_ANALOG_REG_SPI_STATE_REG (SYS_ANAREG_STAT_ADDR)
#define SYS_ANALOG_REG_SPI_STATE_REG1 (SYS_RESERVER_REG0X3B_ADDR)
#define SYS_ANALOG_REG_SPI_STATE_POS(idx) (idx)
#define SYS_ANALOG_REG_SPI_STATE1_POS (SYS_RESERVER_REG0X3B_ANAREGB_STAT_POS)
#define GET_SYS_ANALOG_REG_IDX(addr) ((addr - SYS_ANA_REG0_ADDR) >> 2)

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

static inline void sys_set_ana_reg_bit(uint32_t reg_addr, uint32_t pos, uint32_t mask, uint32_t value)
{
	uint32_t reg_value;
	reg_value = *(volatile uint32_t *)(reg_addr);
	reg_value &= ~(mask << pos);
	reg_value |= ((value & mask) <<pos);
	sys_ll_set_analog_reg_value(reg_addr, reg_value);
}

//reg device_id:

static inline void sys_ll_set_device_id_value(uint32_t v) {
	sys_device_id_t *r = (sys_device_id_t*)(SOC_SYS_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_device_id_value(void) {
	sys_device_id_t *r = (sys_device_id_t*)(SOC_SYS_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_device_id_deviceid(void) {
	sys_device_id_t *r = (sys_device_id_t*)(SOC_SYS_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg version_id:

static inline void sys_ll_set_version_id_value(uint32_t v) {
	sys_version_id_t *r = (sys_version_id_t*)(SOC_SYS_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_version_id_value(void) {
	sys_version_id_t *r = (sys_version_id_t*)(SOC_SYS_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_version_id_versionid(void) {
	sys_version_id_t *r = (sys_version_id_t*)(SOC_SYS_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg cpu_storage_connect_op_select:

static inline void sys_ll_set_cpu_storage_connect_op_select_value(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_value(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_boot_mode(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->boot_mode = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_boot_mode(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->boot_mode;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_clkg_bps(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->clkg_bps = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_clkg_bps(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->clkg_bps;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_rf_switch_manual_en(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->rf_switch_manual_en = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_rf_switch_manual_en(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->rf_switch_manual_en;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_rf_source(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->rf_source = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_rf_source(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->rf_source;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_reserved_7_7(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_reserved_7_7(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->reserved_7_7;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_flash_sel(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->flash_sel = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_flash_sel(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->flash_sel;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_fem_bps_txen(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->fem_bps_txen = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_fem_bps_txen(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->fem_bps_txen;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_gpio_flash_sys_enable(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->gpio_flash_sys_enable = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_gpio_flash_sys_enable(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->gpio_flash_sys_enable;
}

static inline void sys_ll_set_cpu_storage_connect_op_select_boot_mode_norst(uint32_t v) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	r->boot_mode_norst = v;
}

static inline uint32_t sys_ll_get_cpu_storage_connect_op_select_boot_mode_norst(void) {
	sys_cpu_storage_connect_op_select_t *r = (sys_cpu_storage_connect_op_select_t*)(SOC_SYS_REG_BASE + (0x2 << 2));
	return r->boot_mode_norst;
}

//reg cpu_current_run_status:

static inline void sys_ll_set_cpu_current_run_status_value(uint32_t v) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_value(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_core0_halted(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->core0_halted;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_core1_halted(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->core1_halted;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu0_sw_reset(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu0_sw_reset;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu1_sw_reset(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu1_sw_reset;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu0_pwr_dw_state(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu0_pwr_dw_state;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu1_pwr_dw_state(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu1_pwr_dw_state;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu0_exist(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu0_exist;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu1_exist(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu1_exist;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu2_exist(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu2_exist;
}

static inline uint32_t sys_ll_get_cpu_current_run_status_cpu3_exist(void) {
	sys_cpu_current_run_status_t *r = (sys_cpu_current_run_status_t*)(SOC_SYS_REG_BASE + (0x3 << 2));
	return r->cpu3_exist;
}

//reg cpu0_int_halt_clk_op:

static inline void sys_ll_set_cpu0_int_halt_clk_op_value(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_value(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu0_int_halt_clk_op_cpu0_sw_rst(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->cpu0_sw_rst = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_cpu0_sw_rst(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->cpu0_sw_rst;
}

static inline void sys_ll_set_cpu0_int_halt_clk_op_cpu0_pwr_dw(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->cpu0_pwr_dw = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_cpu0_pwr_dw(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->cpu0_pwr_dw;
}

static inline void sys_ll_set_cpu0_int_halt_clk_op_cpu_int_mask(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->cpu_int_mask = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_cpu_int_mask(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->cpu_int_mask;
}

static inline void sys_ll_set_cpu0_int_halt_clk_op_cpu0_halt(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->cpu0_halt = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_cpu0_halt(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->cpu0_halt;
}

static inline void sys_ll_set_cpu0_int_halt_clk_op_cpu0_rxevt_sel(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->cpu0_rxevt_sel = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_cpu0_rxevt_sel(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->cpu0_rxevt_sel;
}

static inline void sys_ll_set_cpu0_int_halt_clk_op_reserved_7_7(uint32_t v) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_reserved_7_7(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->reserved_7_7;
}

static inline uint32_t sys_ll_get_cpu0_int_halt_clk_op_cpu0_offset(void) {
	sys_cpu0_int_halt_clk_op_t *r = (sys_cpu0_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x4 << 2));
	return r->cpu0_offset;
}

//reg cpu1_int_halt_clk_op:

static inline void sys_ll_set_cpu1_int_halt_clk_op_value(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_value(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_cpu1_sw_rst(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->cpu1_sw_rst = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_cpu1_sw_rst(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->cpu1_sw_rst;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_cpu1_pwr_dw(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->cpu1_pwr_dw = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_cpu1_pwr_dw(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->cpu1_pwr_dw;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_reserved_2_2(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->reserved_2_2 = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_reserved_2_2(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->reserved_2_2;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_cpu1_halt(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->cpu1_halt = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_cpu1_halt(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->cpu1_halt;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_cpu1_rxevt_sel(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->cpu1_rxevt_sel = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_cpu1_rxevt_sel(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->cpu1_rxevt_sel;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_reserved_7_7(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->reserved_7_7 = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_reserved_7_7(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->reserved_7_7;
}

static inline void sys_ll_set_cpu1_int_halt_clk_op_cpu1_offset(uint32_t v) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	r->cpu1_offset = v;
}

static inline uint32_t sys_ll_get_cpu1_int_halt_clk_op_cpu1_offset(void) {
	sys_cpu1_int_halt_clk_op_t *r = (sys_cpu1_int_halt_clk_op_t*)(SOC_SYS_REG_BASE + (0x5 << 2));
	return r->cpu1_offset;
}

//reg cpu_clk_div_mode1:

static inline void sys_ll_set_cpu_clk_div_mode1_value(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_value(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu_clk_div_mode1_cksel_core(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->cksel_core = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_cksel_core(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->cksel_core;
}

static inline void sys_ll_set_cpu_clk_div_mode1_ckdiv_core(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->ckdiv_core = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_ckdiv_core(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->ckdiv_core;
}

static inline void sys_ll_set_cpu_clk_div_mode1_cksel_flash(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->cksel_flash = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_cksel_flash(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->cksel_flash;
}

static inline void sys_ll_set_cpu_clk_div_mode1_ckdiv_flash(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->ckdiv_flash = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_ckdiv_flash(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->ckdiv_flash;
}

static inline void sys_ll_set_cpu_clk_div_mode1_cksel_auxs(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->cksel_auxs = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_cksel_auxs(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->cksel_auxs;
}

static inline void sys_ll_set_cpu_clk_div_mode1_ckdiv_auxs(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->ckdiv_auxs = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_ckdiv_auxs(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->ckdiv_auxs;
}

static inline void sys_ll_set_cpu_clk_div_mode1_ckdiv_26mo(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->ckdiv_26mo = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_ckdiv_26mo(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->ckdiv_26mo;
}

static inline void sys_ll_set_cpu_clk_div_mode1_reserved_22_23(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->reserved_22_23 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_reserved_22_23(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->reserved_22_23;
}

static inline void sys_ll_set_cpu_clk_div_mode1_phase_cfg_960m(uint32_t v) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	r->phase_cfg_960m = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode1_phase_cfg_960m(void) {
	sys_cpu_clk_div_mode1_t *r = (sys_cpu_clk_div_mode1_t*)(SOC_SYS_REG_BASE + (0x8 << 2));
	return r->phase_cfg_960m;
}

//reg cpu_clk_div_mode2:

static inline void sys_ll_set_cpu_clk_div_mode2_value(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_value(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2c0(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2c0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2c0(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2c0;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2c3(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2c3 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2c3(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2c3;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_uart0(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_uart0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_uart0(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_uart0;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_uart1(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_uart1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_uart1(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_uart1;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_uart2(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_uart2 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_uart2(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_uart2;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_uart3(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_uart3 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_uart3(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_uart3;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_uart4(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_uart4 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_uart4(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_uart4;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_spi0(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_spi0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_spi0(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_spi0;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_spi1(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_spi1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_spi1(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_spi1;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_spi2(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_spi2 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_spi2(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_spi2;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_spi3(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_spi3 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_spi3(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_spi3;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2s0(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2s0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2s0(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2s0;
}

static inline void sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s0(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_ckdiv_i2s0(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s0;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2s1(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2s1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2s1(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2s1;
}

static inline void sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s1(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_ckdiv_i2s1(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s1;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2s2(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2s2 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2s2(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2s2;
}

static inline void sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s2(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s2 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_ckdiv_i2s2(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s2;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2s3(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2s3 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2s3(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2s3;
}

static inline void sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s3(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s3 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_ckdiv_i2s3(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s3;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i2s4(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i2s4 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i2s4(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i2s4;
}

static inline void sys_ll_set_cpu_clk_div_mode2_ckdiv_i2s4(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->ckdiv_i2s4 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_ckdiv_i2s4(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->ckdiv_i2s4;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_sadc(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_sadc = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_sadc(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_sadc;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_i3c(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_i3c = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_i3c(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_i3c;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_tim0(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_tim0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_tim0(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_tim0;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_tim1(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_tim1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_tim1(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_tim1;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_tim2(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_tim2 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_tim2(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_tim2;
}

static inline void sys_ll_set_cpu_clk_div_mode2_cksel_tim3(uint32_t v) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	r->cksel_tim3 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode2_cksel_tim3(void) {
	sys_cpu_clk_div_mode2_t *r = (sys_cpu_clk_div_mode2_t*)(SOC_SYS_REG_BASE + (0x9 << 2));
	return r->cksel_tim3;
}

//reg cpu_clk_div_mode3:

static inline void sys_ll_set_cpu_clk_div_mode3_value(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_value(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->v;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_pwm0(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_pwm0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_pwm0(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_pwm0;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_can0(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_can0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_can0(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_can0;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_can1(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_can1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_can1(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_can1;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_scr0(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_scr0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_scr0(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_scr0;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_audio(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_audio = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_audio(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_audio;
}

static inline void sys_ll_set_cpu_clk_div_mode3_ckdiv_audio(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->ckdiv_audio = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_ckdiv_audio(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->ckdiv_audio;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_audif0(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_audif0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_audif0(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_audif0;
}

static inline void sys_ll_set_cpu_clk_div_mode3_ckdiv_audif0(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->ckdiv_audif0 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_ckdiv_audif0(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->ckdiv_audif0;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_audif1(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_audif1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_audif1(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_audif1;
}

static inline void sys_ll_set_cpu_clk_div_mode3_ckdiv_audif1(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->ckdiv_audif1 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_ckdiv_audif1(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->ckdiv_audif1;
}

static inline void sys_ll_set_cpu_clk_div_mode3_ckdiv_i2so(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->ckdiv_i2so = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_ckdiv_i2so(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->ckdiv_i2so;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_auxs_enet(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_auxs_enet = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_auxs_enet(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_auxs_enet;
}

static inline void sys_ll_set_cpu_clk_div_mode3_ckdiv_auxs_enet(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->ckdiv_auxs_enet = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_ckdiv_auxs_enet(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->ckdiv_auxs_enet;
}

static inline void sys_ll_set_cpu_clk_div_mode3_cksel_trace(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->cksel_trace = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_cksel_trace(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->cksel_trace;
}

static inline void sys_ll_set_cpu_clk_div_mode3_ckdiv_trace(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->ckdiv_trace = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_ckdiv_trace(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->ckdiv_trace;
}

static inline void sys_ll_set_cpu_clk_div_mode3_reserved_28_31(uint32_t v) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	r->reserved_28_31 = v;
}

static inline uint32_t sys_ll_get_cpu_clk_div_mode3_reserved_28_31(void) {
	sys_cpu_clk_div_mode3_t *r = (sys_cpu_clk_div_mode3_t*)(SOC_SYS_REG_BASE + (0xa << 2));
	return r->reserved_28_31;
}

//reg cpu_anaspi_freq:

static inline void sys_ll_set_cpu_anaspi_freq_value(uint32_t v) {
	sys_cpu_anaspi_freq_t *r = (sys_cpu_anaspi_freq_t*)(SOC_SYS_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_anaspi_freq_value(void) {
	sys_cpu_anaspi_freq_t *r = (sys_cpu_anaspi_freq_t*)(SOC_SYS_REG_BASE + (0xb << 2));
	return r->v;
}

static inline void sys_ll_set_cpu_anaspi_freq_anaspi_freq(uint32_t v) {
	sys_cpu_anaspi_freq_t *r = (sys_cpu_anaspi_freq_t*)(SOC_SYS_REG_BASE + (0xb << 2));
	r->anaspi_freq = v;
}

static inline uint32_t sys_ll_get_cpu_anaspi_freq_anaspi_freq(void) {
	sys_cpu_anaspi_freq_t *r = (sys_cpu_anaspi_freq_t*)(SOC_SYS_REG_BASE + (0xb << 2));
	return r->anaspi_freq;
}

//reg cpu_device_clk_enable:

static inline void sys_ll_set_cpu_device_clk_enable_value(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_value(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->v;
}

static inline void sys_ll_set_cpu_device_clk_enable_tim0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->tim0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_tim0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->tim0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_tim1_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->tim1_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_tim1_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->tim1_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_tim2_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->tim2_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_tim2_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->tim2_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_tim3_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->tim3_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_tim3_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->tim3_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_uart0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->uart0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_uart0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->uart0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_uart1_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->uart1_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_uart1_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->uart1_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_uart2_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->uart2_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_uart2_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->uart2_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_uart3_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->uart3_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_uart3_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->uart3_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_uart4_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->uart4_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_uart4_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->uart4_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_spi0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->spi0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_spi0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->spi0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_spi1_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->spi1_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_spi1_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->spi1_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_spi2_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->spi2_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_spi2_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->spi2_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_spi3_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->spi3_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_spi3_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->spi3_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_sadc_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->sadc_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_sadc_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->sadc_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_pwm0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->pwm0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_pwm0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->pwm0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_otp_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->otp_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_otp_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->otp_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i3c_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i3c_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i3c_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i3c_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2s0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2s0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2s0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2s0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2s1_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2s1_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2s1_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2s1_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2s2_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2s2_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2s2_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2s2_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2s3_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2s3_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2s3_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2s3_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2s4_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2s4_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2s4_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2s4_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2c0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2c0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2c0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2c0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_i2c3_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->i2c3_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_i2c3_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->i2c3_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_irda0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->irda0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_irda0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->irda0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_irda1_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->irda1_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_irda1_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->irda1_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_irda2_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->irda2_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_irda2_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->irda2_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_irda3_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->irda3_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_irda3_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->irda3_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_can0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->can0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_can0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->can0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_can1_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->can1_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_can1_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->can1_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_lin0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->lin0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_lin0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->lin0_cken;
}

static inline void sys_ll_set_cpu_device_clk_enable_scr0_cken(uint32_t v) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	r->scr0_cken = v;
}

static inline uint32_t sys_ll_get_cpu_device_clk_enable_scr0_cken(void) {
	sys_cpu_device_clk_enable_t *r = (sys_cpu_device_clk_enable_t*)(SOC_SYS_REG_BASE + (0xc << 2));
	return r->scr0_cken;
}

//reg reserver_reg0xd:

static inline void sys_ll_set_reserver_reg0xd_value(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_value(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0xd_audio_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->audio_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_audio_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->audio_cken;
}

static inline void sys_ll_set_reserver_reg0xd_audif0_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->audif0_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_audif0_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->audif0_cken;
}

static inline void sys_ll_set_reserver_reg0xd_audif1_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->audif1_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_audif1_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->audif1_cken;
}

static inline void sys_ll_set_reserver_reg0xd_i2so_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->i2so_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_i2so_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->i2so_cken;
}

static inline void sys_ll_set_reserver_reg0xd_cec_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->cec_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_cec_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->cec_cken;
}

static inline void sys_ll_set_reserver_reg0xd_xdac0_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->xdac0_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_xdac0_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->xdac0_cken;
}

static inline void sys_ll_set_reserver_reg0xd_xdac1_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->xdac1_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_xdac1_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->xdac1_cken;
}

static inline void sys_ll_set_reserver_reg0xd_auxs_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->auxs_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_auxs_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->auxs_cken;
}

static inline void sys_ll_set_reserver_reg0xd_auxs_enet_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->auxs_enet_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_auxs_enet_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->auxs_enet_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_26ms_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_26ms_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_26ms_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_26ms_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_32ks_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_32ks_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_32ks_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_32ks_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_26mo_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_26mo_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_26mo_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_26mo_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_240m_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_240m_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_240m_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_240m_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_320m_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_320m_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_320m_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_320m_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_480m_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_480m_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_480m_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_480m_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_160m_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_160m_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_160m_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_160m_cken;
}

static inline void sys_ll_set_reserver_reg0xd_sig_120m_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->sig_120m_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_sig_120m_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->sig_120m_cken;
}

static inline void sys_ll_set_reserver_reg0xd_trace_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->trace_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_trace_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->trace_cken;
}

static inline void sys_ll_set_reserver_reg0xd_reserved_18_21(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->reserved_18_21 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_reserved_18_21(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->reserved_18_21;
}

static inline void sys_ll_set_reserver_reg0xd_wlss_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->wlss_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_wlss_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->wlss_cken;
}

static inline void sys_ll_set_reserver_reg0xd_btdm_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->btdm_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_btdm_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->btdm_cken;
}

static inline void sys_ll_set_reserver_reg0xd_xver_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->xver_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_xver_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->xver_cken;
}

static inline void sys_ll_set_reserver_reg0xd_mac_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->mac_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_mac_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->mac_cken;
}

static inline void sys_ll_set_reserver_reg0xd_phy_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->phy_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_phy_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->phy_cken;
}

static inline void sys_ll_set_reserver_reg0xd_thread_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->thread_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_thread_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->thread_cken;
}

static inline void sys_ll_set_reserver_reg0xd_bk24_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->bk24_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_bk24_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->bk24_cken;
}

static inline void sys_ll_set_reserver_reg0xd_rf_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->rf_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_rf_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->rf_cken;
}

static inline void sys_ll_set_reserver_reg0xd_ofdm_cken(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->ofdm_cken = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_ofdm_cken(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->ofdm_cken;
}

static inline void sys_ll_set_reserver_reg0xd_reserved_31_31(uint32_t v) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	r->reserved_31_31 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xd_reserved_31_31(void) {
	sys_reserver_reg0xd_t *r = (sys_reserver_reg0xd_t*)(SOC_SYS_REG_BASE + (0xd << 2));
	return r->reserved_31_31;
}

//reg reserver_reg0xf:

static inline void sys_ll_set_reserver_reg0xf_value(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_value(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0xf_reserved_0_0(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->reserved_0_0 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_reserved_0_0(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->reserved_0_0;
}

static inline void sys_ll_set_reserver_reg0xf_reserved_1_1(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->reserved_1_1 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_reserved_1_1(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->reserved_1_1;
}

static inline void sys_ll_set_reserver_reg0xf_reserved_2_2(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->reserved_2_2 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_reserved_2_2(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->reserved_2_2;
}

static inline void sys_ll_set_reserver_reg0xf_macp_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->macp_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_macp_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->macp_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_phyp_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->phyp_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_phyp_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->phyp_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_thread_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->thread_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_thread_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->thread_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_encp_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->encp_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_encp_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->encp_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_can0_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->can0_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_can0_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->can0_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_can1_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->can1_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_can1_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->can1_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_irda0_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->irda0_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_irda0_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->irda0_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_irda1_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->irda1_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_irda1_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->irda1_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_dma0_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->dma0_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_dma0_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->dma0_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_spi1_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->spi1_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_spi1_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->spi1_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_spi2_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->spi2_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_spi2_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->spi2_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_uart1_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->uart1_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_uart1_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->uart1_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_uart2_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->uart2_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_uart2_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->uart2_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_uart3_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->uart3_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_uart3_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->uart3_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_uart0_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->uart0_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_uart0_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->uart0_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_spi0_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->spi0_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_spi0_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->spi0_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_flsh_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->flsh_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_flsh_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->flsh_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_audp_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->audp_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_audp_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->audp_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_i3c_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->i3c_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_i3c_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->i3c_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_xvr_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->xvr_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_xvr_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->xvr_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_reserved_23_23(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->reserved_23_23 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_reserved_23_23(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->reserved_23_23;
}

static inline void sys_ll_set_reserver_reg0xf_bk24_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->bk24_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_bk24_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->bk24_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_irda2_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->irda2_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_irda2_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->irda2_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_irda3_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->irda3_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_irda3_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->irda3_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_spi3_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->spi3_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_spi3_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->spi3_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_uart4_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->uart4_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_uart4_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->uart4_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_cpu0_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->cpu0_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_cpu0_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->cpu0_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_cpu1_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->cpu1_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_cpu1_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->cpu1_mem_ret;
}

static inline void sys_ll_set_reserver_reg0xf_coresight_mem_ret(uint32_t v) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	r->coresight_mem_ret = v;
}

static inline uint32_t sys_ll_get_reserver_reg0xf_coresight_mem_ret(void) {
	sys_reserver_reg0xf_t *r = (sys_reserver_reg0xf_t*)(SOC_SYS_REG_BASE + (0xf << 2));
	return r->coresight_mem_ret;
}

//reg reserver_reg0x10:

static inline void sys_ll_set_reserver_reg0x10_value(uint32_t v) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_value(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x10_pwd_cpu1(uint32_t v) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	r->pwd_cpu1 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_pwd_cpu1(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->pwd_cpu1;
}

static inline void sys_ll_set_reserver_reg0x10_pwd_vehp(uint32_t v) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	r->pwd_vehp = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_pwd_vehp(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->pwd_vehp;
}

static inline void sys_ll_set_reserver_reg0x10_pwd_wrls(uint32_t v) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	r->pwd_wrls = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_pwd_wrls(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->pwd_wrls;
}

static inline void sys_ll_set_reserver_reg0x10_rom_pgen(uint32_t v) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	r->rom_pgen = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_rom_pgen(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->rom_pgen;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_cpu1_isolate_state(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->cpu1_isolate_state;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_vehp_isolate_state(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->vehp_isolate_state;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_wrls_isolate_state(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->wrls_isolate_state;
}

static inline void sys_ll_set_reserver_reg0x10_reserved_7_30(uint32_t v) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	r->reserved_7_30 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_reserved_7_30(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->reserved_7_30;
}

static inline uint32_t sys_ll_get_reserver_reg0x10_busmatrix_busy(void) {
	sys_reserver_reg0x10_t *r = (sys_reserver_reg0x10_t*)(SOC_SYS_REG_BASE + (0x10 << 2));
	return r->busmatrix_busy;
}

//reg cpu_power_sleep_wakeup:

static inline void sys_ll_set_cpu_power_sleep_wakeup_value(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_value(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->v;
}

/* BK7259 does not gate the ENCP (Dubhe TrustEngine) power domain through the
 * cpu_power_sleep_wakeup register the way bk7236/bk7239 do (the field does not
 * exist on this SoC; the TrustEngine is powered at boot). The Beken Dubhe
 * driver (dubhe_driver.c, TEE_M path) probes/clears this bit unconditionally,
 * so expose no-op accessors: "already powered" (get==0) and clear is a nop. */
static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_pwd_encp(void) {
	return 0;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(uint32_t v) {
	(void)v;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_sleep_en_global(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->sleep_en_global = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_sleep_en_global(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->sleep_en_global;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_sleep_en_need_flash_idle(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->sleep_en_need_flash_idle = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_sleep_en_need_flash_idle(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->sleep_en_need_flash_idle;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_sleep_bus_idle_bypass(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->sleep_bus_idle_bypass = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_sleep_bus_idle_bypass(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->sleep_bus_idle_bypass;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_sleep_en_need_cpu0_wfi(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->sleep_en_need_cpu0_wfi = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_sleep_en_need_cpu0_wfi(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->sleep_en_need_cpu0_wfi;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_sleep_en_need_cpu1_wfi(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->sleep_en_need_cpu1_wfi = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_sleep_en_need_cpu1_wfi(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->sleep_en_need_cpu1_wfi;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_reserved_12_15(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->reserved_12_15 = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_reserved_12_15(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->reserved_12_15;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_cpu0_ticktimer_32k_enable(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->cpu0_ticktimer_32k_enable = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_cpu0_ticktimer_32k_enable(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->cpu0_ticktimer_32k_enable;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_cpu1_ticktimer_32k_enable(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->cpu1_ticktimer_32k_enable = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_cpu1_ticktimer_32k_enable(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->cpu1_ticktimer_32k_enable;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_reserved_20_24(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->reserved_20_24 = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_reserved_20_24(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->reserved_20_24;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_bts_soft_wakeup_req(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->bts_soft_wakeup_req = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_bts_soft_wakeup_req(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->bts_soft_wakeup_req;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_rom_rd_disable(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->rom_rd_disable = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_rom_rd_disable(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->rom_rd_disable;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_otp_rd_disable(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->otp_rd_disable = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_otp_rd_disable(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->otp_rd_disable;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_share_mem_clkgating_disable(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->share_mem_clkgating_disable = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_share_mem_clkgating_disable(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->share_mem_clkgating_disable;
}

static inline void sys_ll_set_cpu_power_sleep_wakeup_reserved_29_31(uint32_t v) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	r->reserved_29_31 = v;
}

static inline uint32_t sys_ll_get_cpu_power_sleep_wakeup_reserved_29_31(void) {
	sys_cpu_power_sleep_wakeup_t *r = (sys_cpu_power_sleep_wakeup_t*)(SOC_SYS_REG_BASE + (0x11 << 2));
	return r->reserved_29_31;
}

//reg cpu0_int_0_31_en:

static inline void sys_ll_set_cpu0_int_0_31_en_value(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_value(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_dma0_nsec_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_dma0_nsec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_dma0_nsec_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_dma0_nsec_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_encp_sec_intr_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_encp_sec_intr_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_encp_sec_intr_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_encp_sec_intr_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_encp_nsec_intr_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_encp_nsec_intr_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_encp_nsec_intr_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_encp_nsec_intr_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_timer_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_timer_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_timer_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_timer_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_uart_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_uart_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_uart_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_uart_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_pwm0_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_pwm0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_pwm0_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_pwm0_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_i2c0_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_i2c0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_i2c0_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_i2c0_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_spi0_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_spi0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_spi0_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_spi0_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_sadc_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_sadc_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_sadc_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_sadc_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_irda_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_irda_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_irda_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_irda_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_l2_sec_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_l2_sec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_l2_sec_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_l2_sec_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_dma0_sec_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_dma0_sec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_dma0_sec_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_dma0_sec_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_la_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_la_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_la_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_la_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_acomp0_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_acomp0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_acomp0_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_acomp0_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_acomp1_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_acomp1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_acomp1_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_acomp1_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_uart1_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_uart1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_uart1_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_uart1_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_cpu0_fpu_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_cpu0_fpu_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_cpu0_fpu_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_cpu0_fpu_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_cpu1_fpu_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_cpu1_fpu_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_cpu1_fpu_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_cpu1_fpu_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_can_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_can_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_can_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_can_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_l2_nsec_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_l2_nsec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_l2_nsec_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_l2_nsec_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_vid_disp0_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_vid_disp0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_vid_disp0_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_vid_disp0_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_ckmn_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_ckmn_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_ckmn_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_ckmn_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_vid_disp1_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_vid_disp1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_vid_disp1_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_vid_disp1_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_aud_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_aud_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_aud_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_aud_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_i2s0_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_i2s0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_i2s0_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_i2s0_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_i2s1_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_i2s1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_i2s1_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_i2s1_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_vid_disp2_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_vid_disp2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_vid_disp2_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_vid_disp2_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_ipchecksum_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_ipchecksum_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_ipchecksum_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_ipchecksum_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_thread_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_thread_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_thread_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_thread_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_phy_mbp_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_phy_mbp_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_phy_mbp_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_phy_mbp_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_phy_riu_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_phy_riu_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_phy_riu_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_phy_riu_int_en;
}

static inline void sys_ll_set_cpu0_int_0_31_en_cpu0_mac_int_tx_rx_timer_n_int_en(uint32_t v) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	r->cpu0_mac_int_tx_rx_timer_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_en_cpu0_mac_int_tx_rx_timer_n_int_en(void) {
	sys_cpu0_int_0_31_en_t *r = (sys_cpu0_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x14 << 2));
	return r->cpu0_mac_int_tx_rx_timer_n_int_en;
}

//reg cpu0_int_32_63_en:

static inline void sys_ll_set_cpu0_int_32_63_en_value(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_value(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_tx_rx_misc_n_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_mac_int_tx_rx_misc_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_mac_int_tx_rx_misc_n_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_mac_int_tx_rx_misc_n_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_rx_trigger_n_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_mac_int_rx_trigger_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_mac_int_rx_trigger_n_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_mac_int_rx_trigger_n_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_tx_trigger_n_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_mac_int_tx_trigger_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_mac_int_tx_trigger_n_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_mac_int_tx_trigger_n_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_port_trigger_n_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_mac_int_port_trigger_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_mac_int_port_trigger_n_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_mac_int_port_trigger_n_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_mac_int_gen_n_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_mac_int_gen_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_mac_int_gen_n_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_mac_int_gen_n_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_gpio_ns_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_gpio_ns_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_gpio_ns_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_gpio_ns_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_int_mac_wakeup_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_int_mac_wakeup_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_int_mac_wakeup_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_int_mac_wakeup_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_dm_irq_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_dm_irq_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_dm_irq_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_dm_irq_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_ble_irq_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_ble_irq_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_ble_irq_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_ble_irq_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_bt_irq_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_bt_irq_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_bt_irq_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_bt_irq_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_btdm_wake_up_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_btdm_wake_up_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_btdm_wake_up_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_btdm_wake_up_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_touched_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_touched_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_touched_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_touched_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_i2s2_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_i2s2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_i2s2_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_i2s2_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_i2s3_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_i2s3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_i2s3_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_i2s3_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_spdif0_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_spdif0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_spdif0_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_spdif0_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_cec_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_cec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_cec_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_cec_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_xdac0_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_xdac0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_xdac0_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_xdac0_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_xdac1_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_xdac1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_xdac1_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_xdac1_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_otp_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_otp_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_otp_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_otp_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_dpll_unlock_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_dpll_unlock_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_dpll_unlock_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_dpll_unlock_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_dco_unlock_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_dco_unlock_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_dco_unlock_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_dco_unlock_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_usbplug_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_usbplug_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_usbplug_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_usbplug_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_rtc_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_rtc_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_rtc_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_rtc_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_gpio_s_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_gpio_s_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_gpio_s_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_gpio_s_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_uart2_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_uart2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_uart2_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_uart2_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_spi1_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_spi1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_spi1_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_spi1_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_timer1_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_timer1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_timer1_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_timer1_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_spi3_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_spi3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_spi3_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_spi3_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_scr_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_scr_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_scr_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_scr_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_lin_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_lin_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_lin_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_lin_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_can1_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_can1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_can1_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_can1_int_en;
}

static inline void sys_ll_set_cpu0_int_32_63_en_cpu0_timer2_int_en(uint32_t v) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	r->cpu0_timer2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_en_cpu0_timer2_int_en(void) {
	sys_cpu0_int_32_63_en_t *r = (sys_cpu0_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x15 << 2));
	return r->cpu0_timer2_int_en;
}

//reg cpu0_int_64_95_en:

static inline void sys_ll_set_cpu0_int_64_95_en_value(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_value(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_timer3_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_timer3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_timer3_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_timer3_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_uart3_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_uart3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_uart3_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_uart3_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_spi2_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_spi2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_spi2_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_spi2_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_uart4_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_uart4_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_uart4_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_uart4_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_i2c3_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_i2c3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_i2c3_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_i2c3_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_hspl_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_hspl_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_hspl_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_hspl_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_bk24_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_bk24_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_bk24_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_bk24_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_irda1_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_irda1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_irda1_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_irda1_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_irda2_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_irda2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_irda2_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_irda2_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_irda3_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_irda3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_irda3_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_irda3_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_i3c_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_i3c_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_i3c_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_i3c_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_i2s4_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_i2s4_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_i2s4_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_i2s4_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_spdif1_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_spdif1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_spdif1_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_spdif1_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_int_m55sub_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_int_m55sub_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_int_m55sub_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_int_m55sub_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_mailbox_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_mailbox_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_mailbox_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_mailbox_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_ipi_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_ipi_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_ipi_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_ipi_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_vid_disp3_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_vid_disp3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_vid_disp3_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_vid_disp3_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_vad_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_vad_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_vad_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_vad_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv82_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv82_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv82_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv82_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv83_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv83_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv83_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv83_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv84_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv84_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv84_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv84_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv85_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv85_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv85_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv85_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv86_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv86_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv86_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv86_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv87_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv87_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv87_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv87_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv88_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv88_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv88_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv88_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv89_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv89_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv89_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv89_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv90_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv90_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv90_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv90_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv91_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv91_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv91_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv91_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv92_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv92_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv92_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv92_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv93_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv93_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv93_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv93_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv94_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv94_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv94_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv94_int_en;
}

static inline void sys_ll_set_cpu0_int_64_95_en_cpu0_resv95_int_en(uint32_t v) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	r->cpu0_resv95_int_en = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_en_cpu0_resv95_int_en(void) {
	sys_cpu0_int_64_95_en_t *r = (sys_cpu0_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x16 << 2));
	return r->cpu0_resv95_int_en;
}

//reg cpu1_int_0_31_en:

static inline void sys_ll_set_cpu1_int_0_31_en_value(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_value(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_dma0_nsec_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_dma0_nsec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_dma0_nsec_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_dma0_nsec_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_encp_sec_intr_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_encp_sec_intr_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_encp_sec_intr_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_encp_sec_intr_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_encp_nsec_intr_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_encp_nsec_intr_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_encp_nsec_intr_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_encp_nsec_intr_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_timer_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_timer_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_timer_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_timer_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_uart_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_uart_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_uart_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_uart_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_pwm0_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_pwm0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_pwm0_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_pwm0_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_i2c0_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_i2c0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_i2c0_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_i2c0_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_spi0_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_spi0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_spi0_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_spi0_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_sadc_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_sadc_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_sadc_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_sadc_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_irda_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_irda_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_irda_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_irda_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_l2_sec_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_l2_sec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_l2_sec_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_l2_sec_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_dma0_sec_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_dma0_sec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_dma0_sec_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_dma0_sec_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_la_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_la_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_la_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_la_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_acomp0_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_acomp0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_acomp0_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_acomp0_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_acomp1_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_acomp1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_acomp1_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_acomp1_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_uart1_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_uart1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_uart1_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_uart1_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_cpu0_fpu_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_cpu0_fpu_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_cpu0_fpu_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_cpu0_fpu_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_cpu1_fpu_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_cpu1_fpu_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_cpu1_fpu_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_cpu1_fpu_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_can_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_can_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_can_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_can_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_l2_nsec_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_l2_nsec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_l2_nsec_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_l2_nsec_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_vid_disp0_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_vid_disp0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_vid_disp0_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_vid_disp0_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_ckmn_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_ckmn_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_ckmn_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_ckmn_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_vid_disp1_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_vid_disp1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_vid_disp1_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_vid_disp1_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_aud_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_aud_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_aud_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_aud_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_i2s0_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_i2s0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_i2s0_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_i2s0_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_i2s1_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_i2s1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_i2s1_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_i2s1_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_vid_disp2_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_vid_disp2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_vid_disp2_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_vid_disp2_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_ipchecksum_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_ipchecksum_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_ipchecksum_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_ipchecksum_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_thread_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_thread_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_thread_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_thread_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_phy_mbp_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_phy_mbp_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_phy_mbp_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_phy_mbp_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_phy_riu_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_phy_riu_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_phy_riu_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_phy_riu_int_en;
}

static inline void sys_ll_set_cpu1_int_0_31_en_cpu1_mac_int_tx_rx_timer_n_int_en(uint32_t v) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	r->cpu1_mac_int_tx_rx_timer_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_en_cpu1_mac_int_tx_rx_timer_n_int_en(void) {
	sys_cpu1_int_0_31_en_t *r = (sys_cpu1_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x17 << 2));
	return r->cpu1_mac_int_tx_rx_timer_n_int_en;
}

//reg cpu1_int_32_63_en:

static inline void sys_ll_set_cpu1_int_32_63_en_value(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_value(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_mac_int_tx_rx_misc_n_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_mac_int_tx_rx_misc_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_mac_int_tx_rx_misc_n_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_mac_int_tx_rx_misc_n_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_mac_int_rx_trigger_n_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_mac_int_rx_trigger_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_mac_int_rx_trigger_n_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_mac_int_rx_trigger_n_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_mac_int_tx_trigger_n_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_mac_int_tx_trigger_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_mac_int_tx_trigger_n_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_mac_int_tx_trigger_n_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_mac_int_port_trigger_n_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_mac_int_port_trigger_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_mac_int_port_trigger_n_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_mac_int_port_trigger_n_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_mac_int_gen_n_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_mac_int_gen_n_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_mac_int_gen_n_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_mac_int_gen_n_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_gpio_ns_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_gpio_ns_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_gpio_ns_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_gpio_ns_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_int_mac_wakeup_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_int_mac_wakeup_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_int_mac_wakeup_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_int_mac_wakeup_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_dm_irq_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_dm_irq_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_dm_irq_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_dm_irq_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_ble_irq_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_ble_irq_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_ble_irq_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_ble_irq_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_bt_irq_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_bt_irq_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_bt_irq_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_bt_irq_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_btdm_wake_up_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_btdm_wake_up_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_btdm_wake_up_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_btdm_wake_up_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_touched_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_touched_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_touched_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_touched_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_i2s2_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_i2s2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_i2s2_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_i2s2_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_i2s3_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_i2s3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_i2s3_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_i2s3_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_spdif0_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_spdif0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_spdif0_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_spdif0_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_cec_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_cec_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_cec_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_cec_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_xdac0_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_xdac0_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_xdac0_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_xdac0_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_xdac1_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_xdac1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_xdac1_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_xdac1_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_otp_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_otp_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_otp_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_otp_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_dpll_unlock_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_dpll_unlock_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_dpll_unlock_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_dpll_unlock_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_dco_unlock_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_dco_unlock_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_dco_unlock_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_dco_unlock_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_usbplug_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_usbplug_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_usbplug_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_usbplug_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_rtc_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_rtc_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_rtc_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_rtc_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_gpio_s_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_gpio_s_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_gpio_s_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_gpio_s_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_uart2_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_uart2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_uart2_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_uart2_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_spi1_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_spi1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_spi1_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_spi1_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_timer1_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_timer1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_timer1_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_timer1_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_spi3_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_spi3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_spi3_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_spi3_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_scr_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_scr_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_scr_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_scr_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_lin_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_lin_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_lin_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_lin_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_can1_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_can1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_can1_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_can1_int_en;
}

static inline void sys_ll_set_cpu1_int_32_63_en_cpu1_timer2_int_en(uint32_t v) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	r->cpu1_timer2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_en_cpu1_timer2_int_en(void) {
	sys_cpu1_int_32_63_en_t *r = (sys_cpu1_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x18 << 2));
	return r->cpu1_timer2_int_en;
}

//reg cpu1_int_64_95_en:

static inline void sys_ll_set_cpu1_int_64_95_en_value(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_value(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->v;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_timer3_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_timer3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_timer3_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_timer3_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_uart3_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_uart3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_uart3_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_uart3_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_spi2_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_spi2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_spi2_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_spi2_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_uart4_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_uart4_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_uart4_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_uart4_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_i2c3_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_i2c3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_i2c3_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_i2c3_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_hspl_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_hspl_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_hspl_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_hspl_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_bk24_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_bk24_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_bk24_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_bk24_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_irda1_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_irda1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_irda1_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_irda1_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_irda2_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_irda2_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_irda2_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_irda2_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_irda3_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_irda3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_irda3_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_irda3_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_i3c_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_i3c_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_i3c_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_i3c_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_i2s4_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_i2s4_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_i2s4_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_i2s4_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_spdif1_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_spdif1_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_spdif1_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_spdif1_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_int_m55sub_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_int_m55sub_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_int_m55sub_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_int_m55sub_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_mailbox_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_mailbox_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_mailbox_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_mailbox_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_ipi_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_ipi_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_ipi_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_ipi_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_vid_disp3_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_vid_disp3_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_vid_disp3_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_vid_disp3_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_vad_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_vad_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_vad_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_vad_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv82_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv82_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv82_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv82_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv83_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv83_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv83_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv83_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv84_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv84_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv84_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv84_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv85_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv85_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv85_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv85_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv86_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv86_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv86_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv86_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv87_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv87_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv87_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv87_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv88_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv88_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv88_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv88_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv89_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv89_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv89_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv89_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv90_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv90_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv90_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv90_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv91_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv91_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv91_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv91_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv92_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv92_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv92_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv92_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv93_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv93_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv93_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv93_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv94_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv94_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv94_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv94_int_en;
}

static inline void sys_ll_set_cpu1_int_64_95_en_cpu1_resv95_int_en(uint32_t v) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	r->cpu1_resv95_int_en = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_en_cpu1_resv95_int_en(void) {
	sys_cpu1_int_64_95_en_t *r = (sys_cpu1_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x19 << 2));
	return r->cpu1_resv95_int_en;
}

//reg m55sub_int_0_31_en:

static inline void sys_ll_set_m55sub_int_0_31_en_value(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_value(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->v;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_m52s_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_m52s_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_m52s_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_m52s_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_gdma1_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_gdma1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_gdma1_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_gdma1_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_mbox_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_mbox_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_mbox_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_mbox_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_ipi_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_ipi_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_ipi_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_ipi_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_gdma0_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_gdma0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_gdma0_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_gdma0_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_cpu_fpu_int_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_cpu_fpu_int_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_cpu_fpu_int_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_cpu_fpu_int_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_npu_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_npu_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_npu_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_npu_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_usb_fs_int_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_usb_fs_int_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_usb_fs_int_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_usb_fs_int_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_usb_hs_int_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_usb_hs_int_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_usb_hs_int_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_usb_hs_int_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_usb_plug_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_usb_plug_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_usb_plug_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_usb_plug_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_uart5_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_uart5_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_uart5_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_uart5_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_wwdt_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_wwdt_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_wwdt_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_wwdt_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_sdio0_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_sdio0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_sdio0_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_sdio0_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_sdio1_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_sdio1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_sdio1_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_sdio1_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_enet0_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_enet0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_enet0_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_enet0_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_enet1_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_enet1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_enet1_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_enet1_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_qspi0_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_qspi0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_qspi0_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_qspi0_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_qspi1_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_qspi1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_qspi1_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_qspi1_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_hspl_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_hspl_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_hspl_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_hspl_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_isp_mi_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_isp_mi_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_isp_mi_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_isp_mi_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_isp_fe_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_isp_fe_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_isp_fe_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_isp_fe_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_isp_isp_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_isp_isp_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_isp_isp_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_isp_isp_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_csi_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_csi_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_csi_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_csi_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_h26e_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_h26e_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_h26e_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_h26e_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_vid_disp0_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_vid_disp0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_vid_disp0_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_vid_disp0_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_vid_disp1_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_vid_disp1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_vid_disp1_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_vid_disp1_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_vid_disp2_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_vid_disp2_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_vid_disp2_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_vid_disp2_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_vid_disp3_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_vid_disp3_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_vid_disp3_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_vid_disp3_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_vid_disp4_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_vid_disp4_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_vid_disp4_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_vid_disp4_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_psram0_err_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_psram0_err_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_psram0_err_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_psram0_err_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_psram1_err_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_psram1_err_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_psram1_err_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_psram1_err_int_en;
}

static inline void sys_ll_set_m55sub_int_0_31_en_m55sub_mpc_int_en(uint32_t v) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	r->m55sub_mpc_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_en_m55sub_mpc_int_en(void) {
	sys_m55sub_int_0_31_en_t *r = (sys_m55sub_int_0_31_en_t*)(SOC_SYS_REG_BASE + (0x1a << 2));
	return r->m55sub_mpc_int_en;
}

//reg m55sub_int_32_63_en:

static inline void sys_ll_set_m55sub_int_32_63_en_value(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_value(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->v;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_timer4_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_timer4_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_timer4_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_timer4_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_timer5_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_timer5_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_timer5_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_timer5_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_gpio_ns_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_gpio_ns_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_gpio_ns_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_gpio_ns_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_gpio_s_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_gpio_s_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_gpio_s_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_gpio_s_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_audio_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_audio_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_audio_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_audio_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2s0_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2s0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2s0_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2s0_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2s1_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2s1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2s1_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2s1_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2s2_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2s2_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2s2_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2s2_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2s3_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2s3_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2s3_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2s3_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2s4_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2s4_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2s4_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2s4_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_spdif0_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_spdif0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_spdif0_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_spdif0_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_spdif1_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_spdif1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_spdif1_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_spdif1_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_cec_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_cec_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_cec_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_cec_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2c_0_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2c_0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2c_0_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2c_0_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i2c_3_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i2c_3_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i2c_3_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i2c_3_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_i3c_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_i3c_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_i3c_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_i3c_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_uart0_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_uart0_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_uart0_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_uart0_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_uart1_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_uart1_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_uart1_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_uart1_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_uart2_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_uart2_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_uart2_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_uart2_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_uart3_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_uart3_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_uart3_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_uart3_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_uart4_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_uart4_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_uart4_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_uart4_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_int_l2cache_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_int_l2cache_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_int_l2cache_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_int_l2cache_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv54_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv54_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv54_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv54_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv55_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv55_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv55_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv55_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv56_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv56_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv56_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv56_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv57_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv57_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv57_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv57_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv58_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv58_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv58_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv58_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv59_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv59_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv59_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv59_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv60_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv60_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv60_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv60_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv61_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv61_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv61_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv61_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv62_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv62_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv62_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv62_int_en;
}

static inline void sys_ll_set_m55sub_int_32_63_en_m55sub_resv63_int_en(uint32_t v) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	r->m55sub_resv63_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_en_m55sub_resv63_int_en(void) {
	sys_m55sub_int_32_63_en_t *r = (sys_m55sub_int_32_63_en_t*)(SOC_SYS_REG_BASE + (0x1b << 2));
	return r->m55sub_resv63_int_en;
}

//reg m55sub_int_64_95_en:

static inline void sys_ll_set_m55sub_int_64_95_en_value(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_value(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->v;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv64_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv64_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv64_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv64_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv65_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv65_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv65_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv65_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv66_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv66_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv66_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv66_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv67_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv67_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv67_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv67_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv68_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv68_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv68_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv68_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv69_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv69_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv69_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv69_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv70_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv70_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv70_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv70_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv71_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv71_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv71_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv71_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv72_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv72_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv72_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv72_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv73_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv73_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv73_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv73_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv74_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv74_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv74_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv74_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv75_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv75_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv75_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv75_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv76_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv76_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv76_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv76_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv77_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv77_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv77_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv77_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv78_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv78_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv78_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv78_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv79_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv79_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv79_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv79_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv80_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv80_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv80_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv80_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv81_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv81_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv81_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv81_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv82_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv82_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv82_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv82_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv83_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv83_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv83_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv83_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv84_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv84_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv84_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv84_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv85_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv85_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv85_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv85_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv86_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv86_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv86_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv86_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv87_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv87_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv87_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv87_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv88_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv88_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv88_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv88_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv89_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv89_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv89_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv89_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv90_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv90_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv90_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv90_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv91_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv91_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv91_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv91_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv92_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv92_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv92_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv92_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv93_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv93_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv93_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv93_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_resv94_int_en(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_resv94_int_en = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_resv94_int_en(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_resv94_int_en;
}

static inline void sys_ll_set_m55sub_int_64_95_en_m55sub_wakeup(uint32_t v) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	r->m55sub_wakeup = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_en_m55sub_wakeup(void) {
	sys_m55sub_int_64_95_en_t *r = (sys_m55sub_int_64_95_en_t*)(SOC_SYS_REG_BASE + (0x1c << 2));
	return r->m55sub_wakeup;
}

//reg reserver_reg0x1e:

static inline void sys_ll_set_reserver_reg0x1e_value(uint32_t v) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1e_value(void) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x1e_spsh_cfg(uint32_t v) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	r->spsh_cfg = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1e_spsh_cfg(void) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	return r->spsh_cfg;
}

static inline void sys_ll_set_reserver_reg0x1e_spbh_cfg(uint32_t v) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	r->spbh_cfg = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1e_spbh_cfg(void) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	return r->spbh_cfg;
}

static inline void sys_ll_set_reserver_reg0x1e_set_key(uint32_t v) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	r->set_key = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1e_set_key(void) {
	sys_reserver_reg0x1e_t *r = (sys_reserver_reg0x1e_t*)(SOC_SYS_REG_BASE + (0x1e << 2));
	return r->set_key;
}

//reg reserver_reg0x1f:

static inline void sys_ll_set_reserver_reg0x1f_value(uint32_t v) {
	sys_reserver_reg0x1f_t *r = (sys_reserver_reg0x1f_t*)(SOC_SYS_REG_BASE + (0x1f << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1f_value(void) {
	sys_reserver_reg0x1f_t *r = (sys_reserver_reg0x1f_t*)(SOC_SYS_REG_BASE + (0x1f << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x1f_stph_cfg(uint32_t v) {
	sys_reserver_reg0x1f_t *r = (sys_reserver_reg0x1f_t*)(SOC_SYS_REG_BASE + (0x1f << 2));
	r->stph_cfg = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1f_stph_cfg(void) {
	sys_reserver_reg0x1f_t *r = (sys_reserver_reg0x1f_t*)(SOC_SYS_REG_BASE + (0x1f << 2));
	return r->stph_cfg;
}

static inline void sys_ll_set_reserver_reg0x1f_set_key(uint32_t v) {
	sys_reserver_reg0x1f_t *r = (sys_reserver_reg0x1f_t*)(SOC_SYS_REG_BASE + (0x1f << 2));
	r->set_key = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x1f_set_key(void) {
	sys_reserver_reg0x1f_t *r = (sys_reserver_reg0x1f_t*)(SOC_SYS_REG_BASE + (0x1f << 2));
	return r->set_key;
}

//reg cpu0_int_0_31_status:

static inline void sys_ll_set_cpu0_int_0_31_status_value(uint32_t v) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_value(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_dma0_nsec_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_dma0_nsec_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_encp_sec_intr_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_encp_sec_intr_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_encp_nsec_intr_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_encp_nsec_intr_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_timer_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_timer_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_uart_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_uart_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_pwm0_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_pwm0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_i2c0_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_i2c0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_spi0_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_spi0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_sadc_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_sadc_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_irda_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_irda_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_l2_sec_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_l2_sec_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_dma0_sec_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_dma0_sec_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_la_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_la_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_acomp0_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_acomp0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_acomp1_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_acomp1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_uart1_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_uart1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_cpu0_fpu_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_cpu0_fpu_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_cpu1_fpu_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_cpu1_fpu_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_can_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_can_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_l2_nsec_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_l2_nsec_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_vid_disp0_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_vid_disp0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_ckmn_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_ckmn_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_vid_disp1_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_vid_disp1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_aud_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_aud_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_i2s0_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_i2s0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_i2s1_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_i2s1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_vid_disp2_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_vid_disp2_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_ipchecksum_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_ipchecksum_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_thread_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_thread_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_phy_mbp_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_phy_mbp_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_phy_riu_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_phy_riu_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_0_31_status_cpu0_mac_int_tx_rx_timer_n_int_st(void) {
	sys_cpu0_int_0_31_status_t *r = (sys_cpu0_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x20 << 2));
	return r->cpu0_mac_int_tx_rx_timer_n_int_st;
}

//reg cpu0_int_32_63_status:

static inline void sys_ll_set_cpu0_int_32_63_status_value(uint32_t v) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_value(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_mac_int_tx_rx_misc_n_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_mac_int_tx_rx_misc_n_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_mac_int_rx_trigger_n_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_mac_int_rx_trigger_n_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_mac_int_tx_trigger_n_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_mac_int_tx_trigger_n_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_mac_int_port_trigger_n_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_mac_int_port_trigger_n_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_mac_int_gen_n_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_mac_int_gen_n_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_gpio_ns_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_gpio_ns_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_int_mac_wakeup_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_int_mac_wakeup_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_dm_irq_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_dm_irq_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_ble_irq_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_ble_irq_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_bt_irq_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_bt_irq_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_btdm_wake_up_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_btdm_wake_up_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_touched_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_touched_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_i2s2_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_i2s2_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_i2s3_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_i2s3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_spdif0_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_spdif0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_cec_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_cec_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_xdac0_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_xdac0_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_xdac1_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_xdac1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_otp_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_otp_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_dpll_unlock_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_dpll_unlock_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_dco_unlock_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_dco_unlock_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_usbplug_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_usbplug_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_rtc_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_rtc_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_gpio_s_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_gpio_s_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_uart2_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_uart2_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_spi1_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_spi1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_timer1_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_timer1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_spi3_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_spi3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_scr_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_scr_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_lin_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_lin_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_can1_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_can1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_32_63_status_cpu0_timer2_int_st(void) {
	sys_cpu0_int_32_63_status_t *r = (sys_cpu0_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x21 << 2));
	return r->cpu0_timer2_int_st;
}

//reg cpu0_int_64_95_status:

static inline void sys_ll_set_cpu0_int_64_95_status_value(uint32_t v) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_value(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_timer3_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_timer3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_uart3_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_uart3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_spi2_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_spi2_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_uart4_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_uart4_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_i2c3_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_i2c3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_hspl_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_hspl_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_bk24_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_bk24_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_irda1_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_irda1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_irda2_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_irda2_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_irda3_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_irda3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_i3c_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_i3c_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_i2s4_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_i2s4_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_spdif1_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_spdif1_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_int_m55sub_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_int_m55sub_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_mailbox_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_mailbox_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_ipi_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_ipi_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_vid_disp3_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_vid_disp3_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_vad_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_vad_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv82_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv82_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv83_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv83_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv84_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv84_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv85_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv85_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv86_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv86_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv87_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv87_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv88_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv88_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv89_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv89_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv90_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv90_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv91_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv91_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv92_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv92_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv93_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv93_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv94_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv94_int_st;
}

static inline uint32_t sys_ll_get_cpu0_int_64_95_status_cpu0_resv95_int_st(void) {
	sys_cpu0_int_64_95_status_t *r = (sys_cpu0_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x22 << 2));
	return r->cpu0_resv95_int_st;
}

//reg cpu1_int_0_31_status:

static inline void sys_ll_set_cpu1_int_0_31_status_value(uint32_t v) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_value(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_dma0_nsec_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_dma0_nsec_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_encp_sec_intr_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_encp_sec_intr_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_encp_nsec_intr_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_encp_nsec_intr_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_timer_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_timer_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_uart_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_uart_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_pwm0_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_pwm0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_i2c0_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_i2c0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_spi0_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_spi0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_sadc_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_sadc_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_irda_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_irda_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_l2_sec_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_l2_sec_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_dma0_sec_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_dma0_sec_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_la_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_la_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_acomp0_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_acomp0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_acomp1_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_acomp1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_uart1_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_uart1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_cpu0_fpu_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_cpu0_fpu_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_cpu1_fpu_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_cpu1_fpu_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_can_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_can_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_l2_nsec_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_l2_nsec_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_vid_disp0_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_vid_disp0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_ckmn_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_ckmn_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_vid_disp1_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_vid_disp1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_aud_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_aud_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_i2s0_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_i2s0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_i2s1_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_i2s1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_vid_disp2_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_vid_disp2_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_ipchecksum_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_ipchecksum_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_thread_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_thread_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_phy_mbp_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_phy_mbp_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_phy_riu_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_phy_riu_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_0_31_status_cpu1_mac_int_tx_rx_timer_n_int_st(void) {
	sys_cpu1_int_0_31_status_t *r = (sys_cpu1_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x23 << 2));
	return r->cpu1_mac_int_tx_rx_timer_n_int_st;
}

//reg cpu1_int_32_63_status:

static inline void sys_ll_set_cpu1_int_32_63_status_value(uint32_t v) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_value(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_mac_int_tx_rx_misc_n_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_mac_int_tx_rx_misc_n_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_mac_int_rx_trigger_n_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_mac_int_rx_trigger_n_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_mac_int_tx_trigger_n_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_mac_int_tx_trigger_n_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_mac_int_port_trigger_n_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_mac_int_port_trigger_n_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_mac_int_gen_n_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_mac_int_gen_n_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_gpio_ns_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_gpio_ns_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_int_mac_wakeup_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_int_mac_wakeup_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_dm_irq_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_dm_irq_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_ble_irq_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_ble_irq_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_bt_irq_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_bt_irq_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_btdm_wake_up_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_btdm_wake_up_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_touched_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_touched_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_i2s2_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_i2s2_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_i2s3_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_i2s3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_spdif0_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_spdif0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_cec_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_cec_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_xdac0_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_xdac0_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_xdac1_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_xdac1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_otp_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_otp_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_dpll_unlock_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_dpll_unlock_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_dco_unlock_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_dco_unlock_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_usbplug_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_usbplug_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_rtc_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_rtc_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_gpio_s_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_gpio_s_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_uart2_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_uart2_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_spi1_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_spi1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_timer1_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_timer1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_spi3_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_spi3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_scr_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_scr_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_lin_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_lin_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_can1_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_can1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_32_63_status_cpu1_timer2_int_st(void) {
	sys_cpu1_int_32_63_status_t *r = (sys_cpu1_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x24 << 2));
	return r->cpu1_timer2_int_st;
}

//reg cpu1_int_64_95_status:

static inline void sys_ll_set_cpu1_int_64_95_status_value(uint32_t v) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_value(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_timer3_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_timer3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_uart3_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_uart3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_spi2_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_spi2_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_uart4_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_uart4_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_i2c3_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_i2c3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_hspl_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_hspl_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_bk24_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_bk24_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_irda1_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_irda1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_irda2_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_irda2_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_irda3_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_irda3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_i3c_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_i3c_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_i2s4_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_i2s4_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_spdif1_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_spdif1_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_int_m55sub_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_int_m55sub_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_mailbox_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_mailbox_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_ipi_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_ipi_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_vid_disp3_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_vid_disp3_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_vad_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_vad_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv82_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv82_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv83_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv83_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv84_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv84_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv85_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv85_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv86_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv86_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv87_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv87_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv88_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv88_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv89_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv89_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv90_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv90_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv91_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv91_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv92_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv92_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv93_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv93_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv94_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv94_int_st;
}

static inline uint32_t sys_ll_get_cpu1_int_64_95_status_cpu1_resv95_int_st(void) {
	sys_cpu1_int_64_95_status_t *r = (sys_cpu1_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x25 << 2));
	return r->cpu1_resv95_int_st;
}

//reg m55sub_int_0_31_status:

static inline void sys_ll_set_m55sub_int_0_31_status_value(uint32_t v) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_value(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_m52s_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_m52s_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_gdma1_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_gdma1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_mbox_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_mbox_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_ipi_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_ipi_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_gdma0_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_gdma0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_cpu_fpu_int_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_cpu_fpu_int_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_npu_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_npu_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_usb_fs_int_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_usb_fs_int_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_usb_hs_int_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_usb_hs_int_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_usb_plug_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_usb_plug_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_uart5_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_uart5_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_wwdt_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_wwdt_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_sdio0_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_sdio0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_sdio1_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_sdio1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_enet0_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_enet0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_enet1_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_enet1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_qspi0_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_qspi0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_qspi1_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_qspi1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_hspl_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_hspl_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_isp_mi_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_isp_mi_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_isp_fe_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_isp_fe_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_isp_isp_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_isp_isp_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_csi_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_csi_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_h26e_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_h26e_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_vid_disp0_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_vid_disp0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_vid_disp1_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_vid_disp1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_vid_disp2_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_vid_disp2_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_vid_disp3_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_vid_disp3_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_vid_disp4_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_vid_disp4_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_psram0_err_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_psram0_err_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_psram1_err_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_psram1_err_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_0_31_status_m55sub_mpc_int_st(void) {
	sys_m55sub_int_0_31_status_t *r = (sys_m55sub_int_0_31_status_t*)(SOC_SYS_REG_BASE + (0x26 << 2));
	return r->m55sub_mpc_int_st;
}

//reg m55sub_int_32_63_status:

static inline void sys_ll_set_m55sub_int_32_63_status_value(uint32_t v) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_value(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_timer4_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_timer4_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_timer5_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_timer5_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_gpio_ns_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_gpio_ns_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_gpio_s_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_gpio_s_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_audio_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_audio_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2s0_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2s0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2s1_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2s1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2s2_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2s2_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2s3_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2s3_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2s4_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2s4_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_spdif0_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_spdif0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_spdif1_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_spdif1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_cec_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_cec_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2c_0_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2c_0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i2c_3_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i2c_3_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_i3c_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_i3c_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_uart0_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_uart0_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_uart1_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_uart1_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_uart2_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_uart2_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_uart3_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_uart3_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_uart4_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_uart4_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_int_l2cache_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_int_l2cache_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv54_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv54_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv55_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv55_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv56_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv56_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv57_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv57_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv58_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv58_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv59_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv59_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv60_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv60_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv61_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv61_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv62_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv62_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_32_63_status_m55sub_resv63_int_st(void) {
	sys_m55sub_int_32_63_status_t *r = (sys_m55sub_int_32_63_status_t*)(SOC_SYS_REG_BASE + (0x27 << 2));
	return r->m55sub_resv63_int_st;
}

//reg m55sub_int_64_95_status:

static inline void sys_ll_set_m55sub_int_64_95_status_value(uint32_t v) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_value(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv64_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv64_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv65_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv65_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv66_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv66_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv67_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv67_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv68_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv68_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv69_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv69_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv70_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv70_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv71_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv71_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv72_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv72_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv73_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv73_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv74_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv74_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv75_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv75_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv76_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv76_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv77_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv77_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv78_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv78_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv79_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv79_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv80_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv80_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv81_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv81_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv82_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv82_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv83_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv83_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv84_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv84_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv85_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv85_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv86_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv86_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv87_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv87_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv88_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv88_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv89_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv89_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv90_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv90_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv91_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv91_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv92_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv92_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv93_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv93_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv94_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv94_int_st;
}

static inline uint32_t sys_ll_get_m55sub_int_64_95_status_m55sub_resv95_int_st(void) {
	sys_m55sub_int_64_95_status_t *r = (sys_m55sub_int_64_95_status_t*)(SOC_SYS_REG_BASE + (0x28 << 2));
	return r->m55sub_resv95_int_st;
}

//reg reserver_reg0x2a:

static inline void sys_ll_set_reserver_reg0x2a_value(uint32_t v) {
	sys_reserver_reg0x2a_t *r = (sys_reserver_reg0x2a_t*)(SOC_SYS_REG_BASE + (0x2a << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2a_value(void) {
	sys_reserver_reg0x2a_t *r = (sys_reserver_reg0x2a_t*)(SOC_SYS_REG_BASE + (0x2a << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x2a_debug_gpio_ie(uint32_t v) {
	sys_reserver_reg0x2a_t *r = (sys_reserver_reg0x2a_t*)(SOC_SYS_REG_BASE + (0x2a << 2));
	r->debug_gpio_ie = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2a_debug_gpio_ie(void) {
	sys_reserver_reg0x2a_t *r = (sys_reserver_reg0x2a_t*)(SOC_SYS_REG_BASE + (0x2a << 2));
	return r->debug_gpio_ie;
}

//reg reserver_reg0x2b:

static inline void sys_ll_set_reserver_reg0x2b_value(uint32_t v) {
	sys_reserver_reg0x2b_t *r = (sys_reserver_reg0x2b_t*)(SOC_SYS_REG_BASE + (0x2b << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2b_value(void) {
	sys_reserver_reg0x2b_t *r = (sys_reserver_reg0x2b_t*)(SOC_SYS_REG_BASE + (0x2b << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2b_debug_gpio_i(void) {
	sys_reserver_reg0x2b_t *r = (sys_reserver_reg0x2b_t*)(SOC_SYS_REG_BASE + (0x2b << 2));
	return r->debug_gpio_i;
}

//reg reserver_reg0x2c:

static inline void sys_ll_set_reserver_reg0x2c_value(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_value(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x2c_cache_clean_mode(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->cache_clean_mode = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_cache_clean_mode(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->cache_clean_mode;
}

static inline void sys_ll_set_reserver_reg0x2c_cpu0_icache_clean_mode(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->cpu0_icache_clean_mode = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_cpu0_icache_clean_mode(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->cpu0_icache_clean_mode;
}

static inline void sys_ll_set_reserver_reg0x2c_cpu0_icache_clean_tag_sel(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->cpu0_icache_clean_tag_sel = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_cpu0_icache_clean_tag_sel(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->cpu0_icache_clean_tag_sel;
}

static inline void sys_ll_set_reserver_reg0x2c_l2_cache_clean_mode(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->l2_cache_clean_mode = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_l2_cache_clean_mode(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->l2_cache_clean_mode;
}

static inline void sys_ll_set_reserver_reg0x2c_l2_cache_clean_tag_sel(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->l2_cache_clean_tag_sel = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_l2_cache_clean_tag_sel(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->l2_cache_clean_tag_sel;
}

static inline void sys_ll_set_reserver_reg0x2c_reserved_6_23(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->reserved_6_23 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_reserved_6_23(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->reserved_6_23;
}

static inline void sys_ll_set_reserver_reg0x2c_set_key(uint32_t v) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	r->set_key = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2c_set_key(void) {
	sys_reserver_reg0x2c_t *r = (sys_reserver_reg0x2c_t*)(SOC_SYS_REG_BASE + (0x2c << 2));
	return r->set_key;
}

//reg reserver_reg0x2e:

static inline void sys_ll_set_reserver_reg0x2e_value(uint32_t v) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2e_value(void) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x2e_spsl_cfg(uint32_t v) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	r->spsl_cfg = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2e_spsl_cfg(void) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	return r->spsl_cfg;
}

static inline void sys_ll_set_reserver_reg0x2e_spbl_cfg(uint32_t v) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	r->spbl_cfg = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2e_spbl_cfg(void) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	return r->spbl_cfg;
}

static inline void sys_ll_set_reserver_reg0x2e_set_key(uint32_t v) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	r->set_key = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2e_set_key(void) {
	sys_reserver_reg0x2e_t *r = (sys_reserver_reg0x2e_t*)(SOC_SYS_REG_BASE + (0x2e << 2));
	return r->set_key;
}

//reg reserver_reg0x2f:

static inline void sys_ll_set_reserver_reg0x2f_value(uint32_t v) {
	sys_reserver_reg0x2f_t *r = (sys_reserver_reg0x2f_t*)(SOC_SYS_REG_BASE + (0x2f << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2f_value(void) {
	sys_reserver_reg0x2f_t *r = (sys_reserver_reg0x2f_t*)(SOC_SYS_REG_BASE + (0x2f << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x2f_stpl_cfg(uint32_t v) {
	sys_reserver_reg0x2f_t *r = (sys_reserver_reg0x2f_t*)(SOC_SYS_REG_BASE + (0x2f << 2));
	r->stpl_cfg = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2f_stpl_cfg(void) {
	sys_reserver_reg0x2f_t *r = (sys_reserver_reg0x2f_t*)(SOC_SYS_REG_BASE + (0x2f << 2));
	return r->stpl_cfg;
}

static inline void sys_ll_set_reserver_reg0x2f_set_key(uint32_t v) {
	sys_reserver_reg0x2f_t *r = (sys_reserver_reg0x2f_t*)(SOC_SYS_REG_BASE + (0x2f << 2));
	r->set_key = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x2f_set_key(void) {
	sys_reserver_reg0x2f_t *r = (sys_reserver_reg0x2f_t*)(SOC_SYS_REG_BASE + (0x2f << 2));
	return r->set_key;
}

//reg gpio_input_status0:

static inline void sys_ll_set_gpio_input_status0_value(uint32_t v) {
	sys_gpio_input_status0_t *r = (sys_gpio_input_status0_t*)(SOC_SYS_REG_BASE + (0x30 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_gpio_input_status0_value(void) {
	sys_gpio_input_status0_t *r = (sys_gpio_input_status0_t*)(SOC_SYS_REG_BASE + (0x30 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_gpio_input_status0_gpio_input_status0(void) {
	sys_gpio_input_status0_t *r = (sys_gpio_input_status0_t*)(SOC_SYS_REG_BASE + (0x30 << 2));
	return r->gpio_input_status0;
}

//reg gpio_input_status1:

static inline void sys_ll_set_gpio_input_status1_value(uint32_t v) {
	sys_gpio_input_status1_t *r = (sys_gpio_input_status1_t*)(SOC_SYS_REG_BASE + (0x31 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_gpio_input_status1_value(void) {
	sys_gpio_input_status1_t *r = (sys_gpio_input_status1_t*)(SOC_SYS_REG_BASE + (0x31 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_gpio_input_status1_gpio_input_status1(void) {
	sys_gpio_input_status1_t *r = (sys_gpio_input_status1_t*)(SOC_SYS_REG_BASE + (0x31 << 2));
	return r->gpio_input_status1;
}

//reg gpio_input_status2:

static inline void sys_ll_set_gpio_input_status2_value(uint32_t v) {
	sys_gpio_input_status2_t *r = (sys_gpio_input_status2_t*)(SOC_SYS_REG_BASE + (0x32 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_gpio_input_status2_value(void) {
	sys_gpio_input_status2_t *r = (sys_gpio_input_status2_t*)(SOC_SYS_REG_BASE + (0x32 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_gpio_input_status2_gpio_input_status2(void) {
	sys_gpio_input_status2_t *r = (sys_gpio_input_status2_t*)(SOC_SYS_REG_BASE + (0x32 << 2));
	return r->gpio_input_status2;
}

static inline void sys_ll_set_gpio_input_status2_gpio_input_status_en(uint32_t v) {
	sys_gpio_input_status2_t *r = (sys_gpio_input_status2_t*)(SOC_SYS_REG_BASE + (0x32 << 2));
	r->gpio_input_status_en = v;
}

static inline uint32_t sys_ll_get_gpio_input_status2_gpio_input_status_en(void) {
	sys_gpio_input_status2_t *r = (sys_gpio_input_status2_t*)(SOC_SYS_REG_BASE + (0x32 << 2));
	return r->gpio_input_status_en;
}

//reg reserver_reg0x33:

static inline void sys_ll_set_reserver_reg0x33_value(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_value(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x33_acomp0_pwm0_sample_en(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->acomp0_pwm0_sample_en = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_acomp0_pwm0_sample_en(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->acomp0_pwm0_sample_en;
}

static inline void sys_ll_set_reserver_reg0x33_acomp1_pwm0_sample_en(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->acomp1_pwm0_sample_en = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_acomp1_pwm0_sample_en(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->acomp1_pwm0_sample_en;
}

static inline void sys_ll_set_reserver_reg0x33_reserved_2_15(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->reserved_2_15 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_reserved_2_15(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->reserved_2_15;
}

static inline void sys_ll_set_reserver_reg0x33_l2_dis_pwr_down_maint(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->l2_dis_pwr_down_maint = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_l2_dis_pwr_down_maint(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->l2_dis_pwr_down_maint;
}

static inline void sys_ll_set_reserver_reg0x33_l2_apb_violation_resp(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->l2_apb_violation_resp = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_l2_apb_violation_resp(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->l2_apb_violation_resp;
}

static inline void sys_ll_set_reserver_reg0x33_cpu0_dbgen_l2_rst_dis(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu0_dbgen_l2_rst_dis = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu0_dbgen_l2_rst_dis(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu0_dbgen_l2_rst_dis;
}

static inline void sys_ll_set_reserver_reg0x33_cpu1_dbgen_l2_rst_dis(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu1_dbgen_l2_rst_dis = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu1_dbgen_l2_rst_dis(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu1_dbgen_l2_rst_dis;
}

static inline void sys_ll_set_reserver_reg0x33_reserved_20_23(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->reserved_20_23 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_reserved_20_23(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->reserved_20_23;
}

static inline void sys_ll_set_reserver_reg0x33_cpu0_wfe_src(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu0_wfe_src = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu0_wfe_src(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu0_wfe_src;
}

static inline void sys_ll_set_reserver_reg0x33_cpu0_wfe_pulse(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu0_wfe_pulse = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu0_wfe_pulse(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu0_wfe_pulse;
}

static inline void sys_ll_set_reserver_reg0x33_cpu1_wfe_src(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu1_wfe_src = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu1_wfe_src(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu1_wfe_src;
}

static inline void sys_ll_set_reserver_reg0x33_cpu1_wfe_pulse(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu1_wfe_pulse = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu1_wfe_pulse(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu1_wfe_pulse;
}

static inline void sys_ll_set_reserver_reg0x33_cpu0_sleeping_state(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu0_sleeping_state = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu0_sleeping_state(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu0_sleeping_state;
}

static inline void sys_ll_set_reserver_reg0x33_cpu0_deepsleep_state(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu0_deepsleep_state = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu0_deepsleep_state(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu0_deepsleep_state;
}

static inline void sys_ll_set_reserver_reg0x33_cpu1_sleeping_state(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu1_sleeping_state = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu1_sleeping_state(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu1_sleeping_state;
}

static inline void sys_ll_set_reserver_reg0x33_cpu1_deepsleep_state(uint32_t v) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	r->cpu1_deepsleep_state = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x33_cpu1_deepsleep_state(void) {
	sys_reserver_reg0x33_t *r = (sys_reserver_reg0x33_t*)(SOC_SYS_REG_BASE + (0x33 << 2));
	return r->cpu1_deepsleep_state;
}

//reg cpu0_curpc:

static inline void sys_ll_set_cpu0_curpc_value(uint32_t v) {
	sys_cpu0_curpc_t *r = (sys_cpu0_curpc_t*)(SOC_SYS_REG_BASE + (0x34 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_curpc_value(void) {
	sys_cpu0_curpc_t *r = (sys_cpu0_curpc_t*)(SOC_SYS_REG_BASE + (0x34 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_curpc_cpu0_curpc(void) {
	sys_cpu0_curpc_t *r = (sys_cpu0_curpc_t*)(SOC_SYS_REG_BASE + (0x34 << 2));
	return r->cpu0_curpc;
}

//reg cpu0_faultstat_H:

static inline void sys_ll_set_cpu0_faultstat_H_value(uint32_t v) {
	sys_cpu0_faultstat_H_t *r = (sys_cpu0_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x35 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_faultstat_H_value(void) {
	sys_cpu0_faultstat_H_t *r = (sys_cpu0_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x35 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_faultstat_H_cpu0_faultstat_h(void) {
	sys_cpu0_faultstat_H_t *r = (sys_cpu0_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x35 << 2));
	return r->cpu0_faultstat_h;
}

static inline uint32_t sys_ll_get_cpu0_faultstat_H_reserved_11_31(void) {
	sys_cpu0_faultstat_H_t *r = (sys_cpu0_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x35 << 2));
	return r->reserved_11_31;
}

//reg cpu0_faultstat_L:

static inline void sys_ll_set_cpu0_faultstat_L_value(uint32_t v) {
	sys_cpu0_faultstat_L_t *r = (sys_cpu0_faultstat_L_t*)(SOC_SYS_REG_BASE + (0x36 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_faultstat_L_value(void) {
	sys_cpu0_faultstat_L_t *r = (sys_cpu0_faultstat_L_t*)(SOC_SYS_REG_BASE + (0x36 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_faultstat_L_cpu0_faultstat_l(void) {
	sys_cpu0_faultstat_L_t *r = (sys_cpu0_faultstat_L_t*)(SOC_SYS_REG_BASE + (0x36 << 2));
	return r->cpu0_faultstat_l;
}

//reg cpu0_info:

static inline void sys_ll_set_cpu0_info_value(uint32_t v) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu0_info_value(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu0_intnum(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu0_intnum;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu0_currpri(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu0_currpri;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu0_currns(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu0_currns;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu0_halted(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu0_halted;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu0_nc_hready(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu0_nc_hready;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu_cache_m_hready(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu_cache_m_hready;
}

static inline uint32_t sys_ll_get_cpu0_info_cpu0_resetn(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->cpu0_resetn;
}

static inline uint32_t sys_ll_get_cpu0_info_reserved_22_31(void) {
	sys_cpu0_info_t *r = (sys_cpu0_info_t*)(SOC_SYS_REG_BASE + (0x37 << 2));
	return r->reserved_22_31;
}

//reg sys_debug_config0:

static inline void sys_ll_set_sys_debug_config0_value(uint32_t v) {
	sys_sys_debug_config0_t *r = (sys_sys_debug_config0_t*)(SOC_SYS_REG_BASE + (0x38 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_sys_debug_config0_value(void) {
	sys_sys_debug_config0_t *r = (sys_sys_debug_config0_t*)(SOC_SYS_REG_BASE + (0x38 << 2));
	return r->v;
}

static inline void sys_ll_set_sys_debug_config0_dbug_config0(uint32_t v) {
	sys_sys_debug_config0_t *r = (sys_sys_debug_config0_t*)(SOC_SYS_REG_BASE + (0x38 << 2));
	r->dbug_config0 = v;
}

static inline uint32_t sys_ll_get_sys_debug_config0_dbug_config0(void) {
	sys_sys_debug_config0_t *r = (sys_sys_debug_config0_t*)(SOC_SYS_REG_BASE + (0x38 << 2));
	return r->dbug_config0;
}

//reg sys_debug_config1:

static inline void sys_ll_set_sys_debug_config1_value(uint32_t v) {
	sys_sys_debug_config1_t *r = (sys_sys_debug_config1_t*)(SOC_SYS_REG_BASE + (0x39 << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_sys_debug_config1_value(void) {
	sys_sys_debug_config1_t *r = (sys_sys_debug_config1_t*)(SOC_SYS_REG_BASE + (0x39 << 2));
	return r->v;
}

static inline void sys_ll_set_sys_debug_config1_dbug_cfg1(uint32_t v) {
	sys_sys_debug_config1_t *r = (sys_sys_debug_config1_t*)(SOC_SYS_REG_BASE + (0x39 << 2));
	r->dbug_cfg1 = v;
}

static inline uint32_t sys_ll_get_sys_debug_config1_dbug_cfg1(void) {
	sys_sys_debug_config1_t *r = (sys_sys_debug_config1_t*)(SOC_SYS_REG_BASE + (0x39 << 2));
	return r->dbug_cfg1;
}

//reg anareg_stat:

static inline void sys_ll_set_anareg_stat_value(uint32_t v) {
	sys_anareg_stat_t *r = (sys_anareg_stat_t*)(SOC_SYS_REG_BASE + (0x3a << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_anareg_stat_value(void) {
	sys_anareg_stat_t *r = (sys_anareg_stat_t*)(SOC_SYS_REG_BASE + (0x3a << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_anareg_stat_anareg_stat(void) {
	sys_anareg_stat_t *r = (sys_anareg_stat_t*)(SOC_SYS_REG_BASE + (0x3a << 2));
	return r->anareg_stat;
}

//reg reserver_reg0x3b:

static inline void sys_ll_set_reserver_reg0x3b_value(uint32_t v) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x3b_value(void) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	return r->v;
}

static inline void sys_ll_set_reserver_reg0x3b_coresight_chn_gate_en(uint32_t v) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	r->coresight_chn_gate_en = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x3b_coresight_chn_gate_en(void) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	return r->coresight_chn_gate_en;
}

static inline void sys_ll_set_reserver_reg0x3b_coresight_tpmaxdatasize(uint32_t v) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	r->coresight_tpmaxdatasize = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x3b_coresight_tpmaxdatasize(void) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	return r->coresight_tpmaxdatasize;
}

static inline void sys_ll_set_reserver_reg0x3b_coresight_valid(uint32_t v) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	r->coresight_valid = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x3b_coresight_valid(void) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	return r->coresight_valid;
}

static inline void sys_ll_set_reserver_reg0x3b_reserved_30_30(uint32_t v) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	r->reserved_30_30 = v;
}

static inline uint32_t sys_ll_get_reserver_reg0x3b_reserved_30_30(void) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	return r->reserved_30_30;
}

static inline uint32_t sys_ll_get_reserver_reg0x3b_anaregb_stat(void) {
	sys_reserver_reg0x3b_t *r = (sys_reserver_reg0x3b_t*)(SOC_SYS_REG_BASE + (0x3b << 2));
	return r->anaregb_stat;
}

//reg cpu1_curpc:

static inline void sys_ll_set_cpu1_curpc_value(uint32_t v) {
	sys_cpu1_curpc_t *r = (sys_cpu1_curpc_t*)(SOC_SYS_REG_BASE + (0x3c << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_curpc_value(void) {
	sys_cpu1_curpc_t *r = (sys_cpu1_curpc_t*)(SOC_SYS_REG_BASE + (0x3c << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_curpc_cpu1_curpc(void) {
	sys_cpu1_curpc_t *r = (sys_cpu1_curpc_t*)(SOC_SYS_REG_BASE + (0x3c << 2));
	return r->cpu1_curpc;
}

//reg cpu1_faultstat_H:

static inline void sys_ll_set_cpu1_faultstat_H_value(uint32_t v) {
	sys_cpu1_faultstat_H_t *r = (sys_cpu1_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x3d << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_faultstat_H_value(void) {
	sys_cpu1_faultstat_H_t *r = (sys_cpu1_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x3d << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_faultstat_H_cpu1_faultstat_h(void) {
	sys_cpu1_faultstat_H_t *r = (sys_cpu1_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x3d << 2));
	return r->cpu1_faultstat_h;
}

static inline uint32_t sys_ll_get_cpu1_faultstat_H_reserved_11_31(void) {
	sys_cpu1_faultstat_H_t *r = (sys_cpu1_faultstat_H_t*)(SOC_SYS_REG_BASE + (0x3d << 2));
	return r->reserved_11_31;
}

//reg cpu1_faultstat_L:

static inline void sys_ll_set_cpu1_faultstat_L_value(uint32_t v) {
	sys_cpu1_faultstat_L_t *r = (sys_cpu1_faultstat_L_t*)(SOC_SYS_REG_BASE + (0x3e << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_faultstat_L_value(void) {
	sys_cpu1_faultstat_L_t *r = (sys_cpu1_faultstat_L_t*)(SOC_SYS_REG_BASE + (0x3e << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_faultstat_L_cpu1_faultstat_l(void) {
	sys_cpu1_faultstat_L_t *r = (sys_cpu1_faultstat_L_t*)(SOC_SYS_REG_BASE + (0x3e << 2));
	return r->cpu1_faultstat_l;
}

//reg cpu1_info:

static inline void sys_ll_set_cpu1_info_value(uint32_t v) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	r->v = v;
}

static inline uint32_t sys_ll_get_cpu1_info_value(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->v;
}

static inline uint32_t sys_ll_get_cpu1_info_cpu1_intnum(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->cpu1_intnum;
}

static inline uint32_t sys_ll_get_cpu1_info_cpu1_currpri(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->cpu1_currpri;
}

static inline uint32_t sys_ll_get_cpu1_info_cpu1_currns(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->cpu1_currns;
}

static inline uint32_t sys_ll_get_cpu1_info_cpu1_halted(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->cpu1_halted;
}

static inline uint32_t sys_ll_get_cpu1_info_cpu1_nc_hready(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->cpu1_nc_hready;
}

static inline uint32_t sys_ll_get_cpu1_info_reserved_20_20(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->reserved_20_20;
}

static inline uint32_t sys_ll_get_cpu1_info_cpu1_resetn(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->cpu1_resetn;
}

static inline uint32_t sys_ll_get_cpu1_info_reserved_22_31(void) {
	sys_cpu1_info_t *r = (sys_cpu1_info_t*)(SOC_SYS_REG_BASE + (0x3f << 2));
	return r->reserved_22_31;
}

//reg ana_reg0:

static inline void sys_ll_set_ana_reg0_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x40 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg0_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x40 << 2));
}

static inline void sys_ll_set_ana_reg0_dpll_tsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_dpll_tsten(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->dpll_tsten;
}

static inline void sys_ll_set_ana_reg0_cp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 1, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg0_cp(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->cp;
}

static inline void sys_ll_set_ana_reg0_spideten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_spideten(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->spideten;
}

static inline void sys_ll_set_ana_reg0_hvref(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 5, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg0_hvref(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->hvref;
}

static inline void sys_ll_set_ana_reg0_lvref(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 7, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg0_lvref(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->lvref;
}

static inline void sys_ll_set_ana_reg0_rzctrl26m(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_rzctrl26m(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->rzctrl26m;
}

static inline void sys_ll_set_ana_reg0_looprzctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 10, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg0_looprzctrl(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->looprzctrl;
}

static inline void sys_ll_set_ana_reg0_rpc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 14, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg0_rpc(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->rpc;
}

static inline void sys_ll_set_ana_reg0_openloop_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_openloop_en(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->openloop_en;
}

static inline void sys_ll_set_ana_reg0_unlock_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_unlock_sel(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->unlock_sel;
}

static inline void sys_ll_set_ana_reg0_rst_unlock(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_rst_unlock(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->rst_unlock;
}

static inline void sys_ll_set_ana_reg0_spitrig(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_spitrig(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->spitrig;
}

static inline void sys_ll_set_ana_reg0_band(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_band(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->band;
}

static inline void sys_ll_set_ana_reg0_band_1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_band_1(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->band_1;
}

static inline void sys_ll_set_ana_reg0_band_2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 22, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg0_band_2(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->band_2;
}

static inline void sys_ll_set_ana_reg0_bandmanual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_bandmanual(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->bandmanual;
}

static inline void sys_ll_set_ana_reg0_dsptrig(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_dsptrig(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->dsptrig;
}

static inline void sys_ll_set_ana_reg0_lpen_dpll(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_lpen_dpll(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->lpen_dpll;
}

static inline void sys_ll_set_ana_reg0_cksel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 28, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg0_cksel(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->cksel;
}

static inline void sys_ll_set_ana_reg0_bp_caldone(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_bp_caldone(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->bp_caldone;
}

static inline void sys_ll_set_ana_reg0_vselldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x40 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg0_vselldo(void) {
	sys_ana_reg0_t *r = (sys_ana_reg0_t*)(SOC_SYS_REG_BASE + (0x40 << 2));
	return r->vselldo;
}

//reg ana_reg1:

static inline void sys_ll_set_ana_reg1_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x41 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg1_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x41 << 2));
}

static inline void sys_ll_set_ana_reg1_vcooffset(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_vcooffset(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->vcooffset;
}

static inline void sys_ll_set_ana_reg1_selpol(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_selpol(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->selpol;
}

static inline void sys_ll_set_ana_reg1_dlysel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 2, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg1_dlysel(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->dlysel;
}

static inline void sys_ll_set_ana_reg1_edgesel_nck(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_edgesel_nck(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->edgesel_nck;
}

static inline void sys_ll_set_ana_reg1_nload_dlyen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_nload_dlyen(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->nload_dlyen;
}

static inline void sys_ll_set_ana_reg1_cp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 6, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg1_cp(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->cp;
}

static inline void sys_ll_set_ana_reg1_spideten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_spideten(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->spideten;
}

static inline void sys_ll_set_ana_reg1_cben(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_cben(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->cben;
}

static inline void sys_ll_set_ana_reg1_hvref(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 11, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg1_hvref(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->hvref;
}

static inline void sys_ll_set_ana_reg1_lvref(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 13, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg1_lvref(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->lvref;
}

static inline void sys_ll_set_ana_reg1_rzctrl26m(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_rzctrl26m(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->rzctrl26m;
}

static inline void sys_ll_set_ana_reg1_lpfrz(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 16, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg1_lpfrz(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->lpfrz;
}

static inline void sys_ll_set_ana_reg1_rpc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 20, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg1_rpc(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->rpc;
}

static inline void sys_ll_set_ana_reg1_dpll_tsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_dpll_tsten(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->dpll_tsten;
}

static inline void sys_ll_set_ana_reg1_kctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 24, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg1_kctrl(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->kctrl;
}

static inline void sys_ll_set_ana_reg1_vsel_ldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 26, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg1_vsel_ldo(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->vsel_ldo;
}

static inline void sys_ll_set_ana_reg1_div_sw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_div_sw(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->div_sw;
}

static inline void sys_ll_set_ana_reg1_bp_caldone(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_bp_caldone(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->bp_caldone;
}

static inline void sys_ll_set_ana_reg1_ck2xen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_ck2xen(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->ck2xen;
}

static inline void sys_ll_set_ana_reg1_int_mod(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x41 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg1_int_mod(void) {
	sys_ana_reg1_t *r = (sys_ana_reg1_t*)(SOC_SYS_REG_BASE + (0x41 << 2));
	return r->int_mod;
}

//reg ana_reg2:

static inline void sys_ll_set_ana_reg2_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x42 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg2_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x42 << 2));
}

static inline void sys_ll_set_ana_reg2_nc_0_1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 0, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg2_nc_0_1(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->nc_0_1;
}

static inline void sys_ll_set_ana_reg2_vctrl_vsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 2, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg2_vctrl_vsel(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->vctrl_vsel;
}

static inline void sys_ll_set_ana_reg2_nc_5_7(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 5, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg2_nc_5_7(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->nc_5_7;
}

static inline void sys_ll_set_ana_reg2_ck_tst_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_ck_tst_en(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->ck_tst_en;
}

static inline void sys_ll_set_ana_reg2_cktst_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 9, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg2_cktst_sel(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->cktst_sel;
}

static inline void sys_ll_set_ana_reg2_dco_modecal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_dco_modecal(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->dco_modecal;
}

static inline void sys_ll_set_ana_reg2_dco_modecal_1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_dco_modecal_1(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->dco_modecal_1;
}

static inline void sys_ll_set_ana_reg2_anabufsel_rx(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_anabufsel_rx(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->anabufsel_rx;
}

static inline void sys_ll_set_ana_reg2_nc_14_15(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 14, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg2_nc_14_15(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->nc_14_15;
}

static inline void sys_ll_set_ana_reg2_cktdinven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_cktdinven(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->cktdinven;
}

static inline void sys_ll_set_ana_reg2_cktden(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_cktden(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->cktden;
}

static inline void sys_ll_set_ana_reg2_nc_18_20(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 18, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg2_nc_18_20(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->nc_18_20;
}

static inline void sys_ll_set_ana_reg2_xtal32k_dgliten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_xtal32k_dgliten(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->xtal32k_dgliten;
}

static inline void sys_ll_set_ana_reg2_xtal32k_diven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_xtal32k_diven(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->xtal32k_diven;
}

static inline void sys_ll_set_ana_reg2_rc32k_dgliten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_rc32k_dgliten(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->rc32k_dgliten;
}

static inline void sys_ll_set_ana_reg2_rc32k_diven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_rc32k_diven(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->rc32k_diven;
}

static inline void sys_ll_set_ana_reg2_nc_25_27(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 25, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg2_nc_25_27(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->nc_25_27;
}

static inline void sys_ll_set_ana_reg2_unlock_sel_dco(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_unlock_sel_dco(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->unlock_sel_dco;
}

static inline void sys_ll_set_ana_reg2_rst_unlock_dco(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_rst_unlock_dco(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->rst_unlock_dco;
}

static inline void sys_ll_set_ana_reg2_refsamen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_refsamen(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->refsamen;
}

static inline void sys_ll_set_ana_reg2_adcdcsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x42 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg2_adcdcsel(void) {
	sys_ana_reg2_t *r = (sys_ana_reg2_t*)(SOC_SYS_REG_BASE + (0x42 << 2));
	return r->adcdcsel;
}

//reg ana_reg3:

static inline void sys_ll_set_ana_reg3_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x43 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg3_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x43 << 2));
}

static inline void sys_ll_set_ana_reg3_ctune(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 0, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg3_ctune(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->ctune;
}

static inline void sys_ll_set_ana_reg3_core_hpen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_core_hpen(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->core_hpen;
}

static inline void sys_ll_set_ana_reg3_ck_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_ck_sel(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->ck_sel;
}

static inline void sys_ll_set_ana_reg3_anabuf_sel_tx(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_anabuf_sel_tx(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->anabuf_sel_tx;
}

static inline void sys_ll_set_ana_reg3_pwd_xtalldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_pwd_xtalldo(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->pwd_xtalldo;
}

static inline void sys_ll_set_ana_reg3_iamp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_iamp(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->iamp;
}

static inline void sys_ll_set_ana_reg3_vddren(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_vddren(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->vddren;
}

static inline void sys_ll_set_ana_reg3_xamp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 14, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg3_xamp(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->xamp;
}

static inline void sys_ll_set_ana_reg3_vosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 20, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg3_vosel(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->vosel;
}

static inline void sys_ll_set_ana_reg3_en_xtalh_sleep(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_en_xtalh_sleep(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->en_xtalh_sleep;
}

static inline void sys_ll_set_ana_reg3_xtal40_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_xtal40_en(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->xtal40_en;
}

static inline void sys_ll_set_ana_reg3_bufictrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg3_bufictrl(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->bufictrl;
}

static inline void sys_ll_set_ana_reg3_ibias_ctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 28, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg3_ibias_ctrl(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->ibias_ctrl;
}

static inline void sys_ll_set_ana_reg3_icore_ctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x43 << 2)), 30, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg3_icore_ctrl(void) {
	sys_ana_reg3_t *r = (sys_ana_reg3_t*)(SOC_SYS_REG_BASE + (0x43 << 2));
	return r->icore_ctrl;
}

//reg ana_reg4:

static inline void sys_ll_set_ana_reg4_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x44 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg4_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x44 << 2));
}

static inline void sys_ll_set_ana_reg4_nc_0_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x44 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg4_nc_0_31(void) {
	sys_ana_reg4_t *r = (sys_ana_reg4_t*)(SOC_SYS_REG_BASE + (0x44 << 2));
	return r->nc_0_31;
}

//reg ana_reg5:

static inline void sys_ll_set_ana_reg5_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x45 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg5_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x45 << 2));
}

static inline void sys_ll_set_ana_reg5_vselldo1_dpll(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_vselldo1_dpll(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->vselldo1_dpll;
}

static inline void sys_ll_set_ana_reg5_en_xtall(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_en_xtall(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->en_xtall;
}

static inline void sys_ll_set_ana_reg5_en_dco(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_en_dco(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->en_dco;
}

static inline void sys_ll_set_ana_reg5_temp_gsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_temp_gsel(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->temp_gsel;
}

static inline void sys_ll_set_ana_reg5_en_temp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_en_temp(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->en_temp;
}

static inline void sys_ll_set_ana_reg5_en_dpll(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_en_dpll(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->en_dpll;
}

static inline void sys_ll_set_ana_reg5_en_cb(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_en_cb(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->en_cb;
}

static inline void sys_ll_set_ana_reg5_gpio_latch(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_gpio_latch(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->gpio_latch;
}

static inline void sys_ll_set_ana_reg5_bypassen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_bypassen(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->bypassen;
}

static inline void sys_ll_set_ana_reg5_nc_9_9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_nc_9_9(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->nc_9_9;
}

static inline void sys_ll_set_ana_reg5_rc32k_refclk_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_rc32k_refclk_en(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->rc32k_refclk_en;
}

static inline void sys_ll_set_ana_reg5_spilatchb_rc32k(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_spilatchb_rc32k(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->spilatchb_rc32k;
}

static inline void sys_ll_set_ana_reg5_rosc_disable(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_rosc_disable(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->rosc_disable;
}

static inline void sys_ll_set_ana_reg5_pwdaudpll(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_pwdaudpll(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->pwdaudpll;
}

static inline void sys_ll_set_ana_reg5_pwd_rosc_spi(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_pwd_rosc_spi(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->pwd_rosc_spi;
}

static inline void sys_ll_set_ana_reg5_nc_15_15(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_nc_15_15(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->nc_15_15;
}

static inline void sys_ll_set_ana_reg5_itune_xtall(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 16, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg5_itune_xtall(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->itune_xtall;
}

static inline void sys_ll_set_ana_reg5_xtall_tsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_xtall_tsten(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->xtall_tsten;
}

static inline void sys_ll_set_ana_reg5_rosc_ten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_rosc_ten(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->rosc_ten;
}

static inline void sys_ll_set_ana_reg5_bcal_start(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_bcal_start(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->bcal_start;
}

static inline void sys_ll_set_ana_reg5_bcal_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg5_bcal_en(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->bcal_en;
}

static inline void sys_ll_set_ana_reg5_bcal_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 24, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg5_bcal_sel(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->bcal_sel;
}

static inline void sys_ll_set_ana_reg5_vbias(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x45 << 2)), 27, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg5_vbias(void) {
	sys_ana_reg5_t *r = (sys_ana_reg5_t*)(SOC_SYS_REG_BASE + (0x45 << 2));
	return r->vbias;
}

//reg ana_reg6:

static inline void sys_ll_set_ana_reg6_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x46 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg6_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x46 << 2));
}

static inline void sys_ll_set_ana_reg6_calib_interval(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 0, 0x3ff, v);
}

static inline uint32_t sys_ll_get_ana_reg6_calib_interval(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->calib_interval;
}

static inline void sys_ll_set_ana_reg6_modify_interval(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 10, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg6_modify_interval(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->modify_interval;
}

static inline void sys_ll_set_ana_reg6_xtal_wakeup_time(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 16, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg6_xtal_wakeup_time(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->xtal_wakeup_time;
}

static inline void sys_ll_set_ana_reg6_spi_trig(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg6_spi_trig(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->spi_trig;
}

static inline void sys_ll_set_ana_reg6_modifi_auto(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg6_modifi_auto(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->modifi_auto;
}

static inline void sys_ll_set_ana_reg6_calib_auto(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg6_calib_auto(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->calib_auto;
}

static inline void sys_ll_set_ana_reg6_cal_mode(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg6_cal_mode(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->cal_mode;
}

static inline void sys_ll_set_ana_reg6_manu_ena(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg6_manu_ena(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->manu_ena;
}

static inline void sys_ll_set_ana_reg6_manu_cin(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x46 << 2)), 25, 0x7f, v);
}

static inline uint32_t sys_ll_get_ana_reg6_manu_cin(void) {
	sys_ana_reg6_t *r = (sys_ana_reg6_t*)(SOC_SYS_REG_BASE + (0x46 << 2));
	return r->manu_cin;
}

//reg ana_reg7:

static inline void sys_ll_set_ana_reg7_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x47 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg7_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x47 << 2));
}

static inline void sys_ll_set_ana_reg7_nsyn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_nsyn(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->nsyn;
}

static inline void sys_ll_set_ana_reg7_bandmanual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 1, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg7_bandmanual(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->bandmanual;
}

static inline void sys_ll_set_ana_reg7_ckref_loop_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_ckref_loop_sel(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->ckref_loop_sel;
}

static inline void sys_ll_set_ana_reg7_ioffs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 8, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg7_ioffs(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->ioffs;
}

static inline void sys_ll_set_ana_reg7_reset_nload(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_reset_nload(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->reset_nload;
}

static inline void sys_ll_set_ana_reg7_closeloop_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_closeloop_en(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->closeloop_en;
}

static inline void sys_ll_set_ana_reg7_ictrlm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_ictrlm(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->ictrlm;
}

static inline void sys_ll_set_ana_reg7_spi_rstn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_spi_rstn(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->spi_rstn;
}

static inline void sys_ll_set_ana_reg7_osccal_trig(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_osccal_trig(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->osccal_trig;
}

static inline void sys_ll_set_ana_reg7_manual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg7_manual(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->manual;
}

static inline void sys_ll_set_ana_reg7_diff(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 17, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg7_diff(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->diff;
}

static inline void sys_ll_set_ana_reg7_ictrlmanual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 20, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg7_ictrlmanual(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->ictrlmanual;
}

static inline void sys_ll_set_ana_reg7_cnti(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x47 << 2)), 23, 0x1ff, v);
}

static inline uint32_t sys_ll_get_ana_reg7_cnti(void) {
	sys_ana_reg7_t *r = (sys_ana_reg7_t*)(SOC_SYS_REG_BASE + (0x47 << 2));
	return r->cnti;
}

//reg ana_reg8:

static inline void sys_ll_set_ana_reg8_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x48 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg8_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x48 << 2));
}

static inline void sys_ll_set_ana_reg8_n(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x48 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg8_n(void) {
	sys_ana_reg8_t *r = (sys_ana_reg8_t*)(SOC_SYS_REG_BASE + (0x48 << 2));
	return r->n;
}

//reg ana_reg9:

static inline void sys_ll_set_ana_reg9_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x49 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg9_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x49 << 2));
}

static inline void sys_ll_set_ana_reg9_clk_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_clk_sel(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->clk_sel;
}

static inline void sys_ll_set_ana_reg9_coreldo_hp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_coreldo_hp(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->coreldo_hp;
}

static inline void sys_ll_set_ana_reg9_dldohp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_dldohp(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->dldohp;
}

static inline void sys_ll_set_ana_reg9_t_vanaldosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 3, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg9_t_vanaldosel(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->t_vanaldosel;
}

static inline void sys_ll_set_ana_reg9_r_vanaldosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 6, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg9_r_vanaldosel(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->r_vanaldosel;
}

static inline void sys_ll_set_ana_reg9_en_trsw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_en_trsw(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->en_trsw;
}

static inline void sys_ll_set_ana_reg9_aldohp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_aldohp(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->aldohp;
}

static inline void sys_ll_set_ana_reg9_anacurlim(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_anacurlim(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->anacurlim;
}

static inline void sys_ll_set_ana_reg9_hsldo_hp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_hsldo_hp(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->hsldo_hp;
}

static inline void sys_ll_set_ana_reg9_pwd_hsldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_pwd_hsldo(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->pwd_hsldo;
}

static inline void sys_ll_set_ana_reg9_enfast_hsldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_enfast_hsldo(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->enfast_hsldo;
}

static inline void sys_ll_set_ana_reg9_vporsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_vporsel(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->vporsel;
}

static inline void sys_ll_set_ana_reg9_valoldosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 16, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg9_valoldosel(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->valoldosel;
}

static inline void sys_ll_set_ana_reg9_alopowsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_alopowsel(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->alopowsel;
}

static inline void sys_ll_set_ana_reg9_en_fast_aloldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_en_fast_aloldo(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->en_fast_aloldo;
}

static inline void sys_ll_set_ana_reg9_aloldohp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_aloldohp(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->aloldohp;
}

static inline void sys_ll_set_ana_reg9_bgcal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 22, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg9_bgcal(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->bgcal;
}

static inline void sys_ll_set_ana_reg9_vbgcalmode(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_vbgcalmode(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->vbgcalmode;
}

static inline void sys_ll_set_ana_reg9_vbgcalstart(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_vbgcalstart(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->vbgcalstart;
}

static inline void sys_ll_set_ana_reg9_pwd_bgcal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_pwd_bgcal(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->pwd_bgcal;
}

static inline void sys_ll_set_ana_reg9_spi_envbg(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x49 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg9_spi_envbg(void) {
	sys_ana_reg9_t *r = (sys_ana_reg9_t*)(SOC_SYS_REG_BASE + (0x49 << 2));
	return r->spi_envbg;
}

//reg ana_reg10:

static inline void sys_ll_set_ana_reg10_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x4a << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg10_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x4a << 2));
}

static inline void sys_ll_set_ana_reg10_azcd_manual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 0, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg10_azcd_manual(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->azcd_manual;
}

static inline void sys_ll_set_ana_reg10_azcdrefs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 6, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg10_azcdrefs(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->azcdrefs;
}

static inline void sys_ll_set_ana_reg10_spi_latch1v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_spi_latch1v(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->spi_latch1v;
}

static inline void sys_ll_set_ana_reg10_digcurlim(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_digcurlim(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->digcurlim;
}

static inline void sys_ll_set_ana_reg10_rtc_wkrstn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_rtc_wkrstn(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->rtc_wkrstn;
}

static inline void sys_ll_set_ana_reg10_rst_wks(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_rst_wks(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->rst_wks;
}

static inline void sys_ll_set_ana_reg10_d_veasel1v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 13, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg10_d_veasel1v(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->d_veasel1v;
}

static inline void sys_ll_set_ana_reg10_ensfsdd(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_ensfsdd(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->ensfsdd;
}

static inline void sys_ll_set_ana_reg10_vcorehsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 16, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg10_vcorehsel(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->vcorehsel;
}

static inline void sys_ll_set_ana_reg10_vcorelsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 20, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg10_vcorelsel(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->vcorelsel;
}

static inline void sys_ll_set_ana_reg10_vlden(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_vlden(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->vlden;
}

static inline void sys_ll_set_ana_reg10_en_fast_coreldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_en_fast_coreldo(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->en_fast_coreldo;
}

static inline void sys_ll_set_ana_reg10_pwdcoreldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_pwdcoreldo(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->pwdcoreldo;
}

static inline void sys_ll_set_ana_reg10_vdighsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 26, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg10_vdighsel(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->vdighsel;
}

static inline void sys_ll_set_ana_reg10_vdiglsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 29, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg10_vdiglsel(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->vdiglsel;
}

static inline void sys_ll_set_ana_reg10_vdd12lden(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4a << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg10_vdd12lden(void) {
	sys_ana_reg10_t *r = (sys_ana_reg10_t*)(SOC_SYS_REG_BASE + (0x4a << 2));
	return r->vdd12lden;
}

//reg ana_reg11:

static inline void sys_ll_set_ana_reg11_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x4b << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg11_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x4b << 2));
}

static inline void sys_ll_set_ana_reg11_aldo_czsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 0, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg11_aldo_czsel(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->aldo_czsel;
}

static inline void sys_ll_set_ana_reg11_zldo_rzsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 3, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg11_zldo_rzsel(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->zldo_rzsel;
}

static inline void sys_ll_set_ana_reg11_azcdswvs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 5, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg11_azcdswvs(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->azcdswvs;
}

static inline void sys_ll_set_ana_reg11_aenzcddy(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_aenzcddy(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->aenzcddy;
}

static inline void sys_ll_set_ana_reg11_aenzcdmsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_aenzcdmsel(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->aenzcdmsel;
}

static inline void sys_ll_set_ana_reg11_aenzcdcalib(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_aenzcdcalib(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->aenzcdcalib;
}

static inline void sys_ll_set_ana_reg11_en_corepsw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_en_corepsw(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->en_corepsw;
}

static inline void sys_ll_set_ana_reg11_en_alopsw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_en_alopsw(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->en_alopsw;
}

static inline void sys_ll_set_ana_reg11_vbatdetsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 13, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg11_vbatdetsel(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->vbatdetsel;
}

static inline void sys_ll_set_ana_reg11_spi_timerwken(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_spi_timerwken(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->spi_timerwken;
}

static inline void sys_ll_set_ana_reg11_spi_byp32pwd(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_spi_byp32pwd(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->spi_byp32pwd;
}

static inline void sys_ll_set_ana_reg11_sd(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_sd(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->sd;
}

static inline void sys_ll_set_ana_reg11_timer_wkrstn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_timer_wkrstn(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->timer_wkrstn;
}

static inline void sys_ll_set_ana_reg11_gpio_wkrst1v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_gpio_wkrst1v(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->gpio_wkrst1v;
}

static inline void sys_ll_set_ana_reg11_ckfs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 20, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg11_ckfs(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->ckfs;
}

static inline void sys_ll_set_ana_reg11_ckintsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_ckintsel(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->ckintsel;
}

static inline void sys_ll_set_ana_reg11_osccaltrig(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_osccaltrig(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->osccaltrig;
}

static inline void sys_ll_set_ana_reg11_mroscsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg11_mroscsel(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->mroscsel;
}

static inline void sys_ll_set_ana_reg11_mrosci_cal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 25, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg11_mrosci_cal(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->mrosci_cal;
}

static inline void sys_ll_set_ana_reg11_mrosccap_cal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4b << 2)), 28, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg11_mrosccap_cal(void) {
	sys_ana_reg11_t *r = (sys_ana_reg11_t*)(SOC_SYS_REG_BASE + (0x4b << 2));
	return r->mrosccap_cal;
}

//reg ana_reg12:

static inline void sys_ll_set_ana_reg12_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x4c << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg12_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x4c << 2));
}

static inline void sys_ll_set_ana_reg12_sfsr(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 0, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg12_sfsr(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->sfsr;
}

static inline void sys_ll_set_ana_reg12_ensfsaa(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_ensfsaa(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->ensfsaa;
}

static inline void sys_ll_set_ana_reg12_apfms(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 5, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg12_apfms(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->apfms;
}

static inline void sys_ll_set_ana_reg12_atmpo_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 10, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg12_atmpo_sel(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->atmpo_sel;
}

static inline void sys_ll_set_ana_reg12_ampoen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_ampoen(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->ampoen;
}

static inline void sys_ll_set_ana_reg12_enpowa(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_enpowa(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->enpowa;
}

static inline void sys_ll_set_ana_reg12_avea_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 14, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg12_avea_sel(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->avea_sel;
}

static inline void sys_ll_set_ana_reg12_aforcepfm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_aforcepfm(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->aforcepfm;
}

static inline void sys_ll_set_ana_reg12_acls(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 17, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg12_acls(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->acls;
}

static inline void sys_ll_set_ana_reg12_aswrsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_aswrsten(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->aswrsten;
}

static inline void sys_ll_set_ana_reg12_aripc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 21, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg12_aripc(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->aripc;
}

static inline void sys_ll_set_ana_reg12_arampc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 24, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg12_arampc(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->arampc;
}

static inline void sys_ll_set_ana_reg12_arampcen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_arampcen(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->arampcen;
}

static inline void sys_ll_set_ana_reg12_aenburst(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_aenburst(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->aenburst;
}

static inline void sys_ll_set_ana_reg12_apfmen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_apfmen(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->apfmen;
}

static inline void sys_ll_set_ana_reg12_aldosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4c << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg12_aldosel(void) {
	sys_ana_reg12_t *r = (sys_ana_reg12_t*)(SOC_SYS_REG_BASE + (0x4c << 2));
	return r->aldosel;
}

//reg ana_reg13:

static inline void sys_ll_set_ana_reg13_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x4d << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg13_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x4d << 2));
}

static inline void sys_ll_set_ana_reg13_buckd_softst(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 0, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg13_buckd_softst(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->buckd_softst;
}

static inline void sys_ll_set_ana_reg13_denzcdcalib(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_denzcdcalib(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->denzcdcalib;
}

static inline void sys_ll_set_ana_reg13_dzcdmsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dzcdmsel(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dzcdmsel;
}

static inline void sys_ll_set_ana_reg13_vbd_rstrtc_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_vbd_rstrtc_en(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->vbd_rstrtc_en;
}

static inline void sys_ll_set_ana_reg13_vddgpio_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 7, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_vddgpio_sel(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->vddgpio_sel;
}

static inline void sys_ll_set_ana_reg13_dpfms(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 8, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dpfms(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dpfms;
}

static inline void sys_ll_set_ana_reg13_dtmpo_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 13, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dtmpo_sel(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dtmpo_sel;
}

static inline void sys_ll_set_ana_reg13_dmpoen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dmpoen(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dmpoen;
}

static inline void sys_ll_set_ana_reg13_dforcepfm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dforcepfm(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dforcepfm;
}

static inline void sys_ll_set_ana_reg13_dcls(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 17, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dcls(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dcls;
}

static inline void sys_ll_set_ana_reg13_dswrsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dswrsten(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dswrsten;
}

static inline void sys_ll_set_ana_reg13_dripc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 21, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dripc(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dripc;
}

static inline void sys_ll_set_ana_reg13_drampc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 24, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg13_drampc(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->drampc;
}

static inline void sys_ll_set_ana_reg13_drampcen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_drampcen(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->drampcen;
}

static inline void sys_ll_set_ana_reg13_denburst(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_denburst(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->denburst;
}

static inline void sys_ll_set_ana_reg13_dpfmen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dpfmen(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dpfmen;
}

static inline void sys_ll_set_ana_reg13_dldosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4d << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg13_dldosel(void) {
	sys_ana_reg13_t *r = (sys_ana_reg13_t*)(SOC_SYS_REG_BASE + (0x4d << 2));
	return r->dldosel;
}

//reg ana_reg14:

static inline void sys_ll_set_ana_reg14_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x4e << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg14_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x4e << 2));
}

static inline void sys_ll_set_ana_reg14_en_alo2corepsw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg14_en_alo2corepsw(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->en_alo2corepsw;
}

static inline void sys_ll_set_ana_reg14_asoft_stc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 1, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg14_asoft_stc(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->asoft_stc;
}

static inline void sys_ll_set_ana_reg14_dldo_czsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 5, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg14_dldo_czsel(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->dldo_czsel;
}

static inline void sys_ll_set_ana_reg14_dldo_rzsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 8, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg14_dldo_rzsel(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->dldo_rzsel;
}

static inline void sys_ll_set_ana_reg14_en_usbvcc18(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg14_en_usbvcc18(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->en_usbvcc18;
}

static inline void sys_ll_set_ana_reg14_en_usbvcc3v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg14_en_usbvcc3v(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->en_usbvcc3v;
}

static inline void sys_ll_set_ana_reg14_vtrxspisel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 12, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg14_vtrxspisel(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->vtrxspisel;
}

static inline void sys_ll_set_ana_reg14_denzcddy(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg14_denzcddy(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->denzcddy;
}

static inline void sys_ll_set_ana_reg14_dzcd_swvs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 15, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg14_dzcd_swvs(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->dzcd_swvs;
}

static inline void sys_ll_set_ana_reg14_dzcd_refs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 18, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg14_dzcd_refs(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->dzcd_refs;
}

static inline void sys_ll_set_ana_reg14_dzcd_manu(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 21, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg14_dzcd_manu(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->dzcd_manu;
}

static inline void sys_ll_set_ana_reg14_vpsramsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 27, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg14_vpsramsel(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->vpsramsel;
}

static inline void sys_ll_set_ana_reg14_enpsram(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4e << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg14_enpsram(void) {
	sys_ana_reg14_t *r = (sys_ana_reg14_t*)(SOC_SYS_REG_BASE + (0x4e << 2));
	return r->enpsram;
}

//reg ana_reg15:

static inline void sys_ll_set_ana_reg15_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x4f << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg15_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x4f << 2));
}

static inline void sys_ll_set_ana_reg15_gpiowken(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x4f << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg15_gpiowken(void) {
	sys_ana_reg15_t *r = (sys_ana_reg15_t*)(SOC_SYS_REG_BASE + (0x4f << 2));
	return r->gpiowken;
}

//reg ana_reg16:

static inline void sys_ll_set_ana_reg16_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x50 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg16_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x50 << 2));
}

static inline void sys_ll_set_ana_reg16_rtc_set(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 0, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg16_rtc_set(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->rtc_set;
}

static inline void sys_ll_set_ana_reg16_nc_8_10(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 8, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg16_nc_8_10(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->nc_8_10;
}

static inline void sys_ll_set_ana_reg16_vcorehssel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 11, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg16_vcorehssel(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->vcorehssel;
}

static inline void sys_ll_set_ana_reg16_vbuckhssel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 15, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg16_vbuckhssel(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->vbuckhssel;
}

static inline void sys_ll_set_ana_reg16_hsenfast(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg16_hsenfast(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->hsenfast;
}

static inline void sys_ll_set_ana_reg16_enhspw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg16_enhspw(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->enhspw;
}

static inline void sys_ll_set_ana_reg16_buckhs_soft_stc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 20, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg16_buckhs_soft_stc(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->buckhs_soft_stc;
}

static inline void sys_ll_set_ana_reg16_hs_veasel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 24, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg16_hs_veasel(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->hs_veasel;
}

static inline void sys_ll_set_ana_reg16_hszcd_manual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x50 << 2)), 26, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg16_hszcd_manual(void) {
	sys_ana_reg16_t *r = (sys_ana_reg16_t*)(SOC_SYS_REG_BASE + (0x50 << 2));
	return r->hszcd_manual;
}

//reg ana_reg17:

static inline void sys_ll_set_ana_reg17_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x51 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg17_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x51 << 2));
}

static inline void sys_ll_set_ana_reg17_rtc_set(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x51 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg17_rtc_set(void) {
	sys_ana_reg17_t *r = (sys_ana_reg17_t*)(SOC_SYS_REG_BASE + (0x51 << 2));
	return r->rtc_set;
}

//reg ana_reg18:

static inline void sys_ll_set_ana_reg18_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x52 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg18_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x52 << 2));
}

static inline void sys_ll_set_ana_reg18_timer_set(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x52 << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg18_timer_set(void) {
	sys_ana_reg18_t *r = (sys_ana_reg18_t*)(SOC_SYS_REG_BASE + (0x52 << 2));
	return r->timer_set;
}

//reg ana_reg19:

static inline void sys_ll_set_ana_reg19_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x53 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg19_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x53 << 2));
}

static inline void sys_ll_set_ana_reg19_hsenzcddy(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsenzcddy(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsenzcddy;
}

static inline void sys_ll_set_ana_reg19_hsenzcdcalib(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsenzcdcalib(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsenzcdcalib;
}

static inline void sys_ll_set_ana_reg19_hszcdswvs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 2, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hszcdswvs(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hszcdswvs;
}

static inline void sys_ll_set_ana_reg19_hszcdrefs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 5, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hszcdrefs(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hszcdrefs;
}

static inline void sys_ll_set_ana_reg19_hszcdmsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hszcdmsel(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hszcdmsel;
}

static inline void sys_ll_set_ana_reg19_hspfms(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 9, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hspfms(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hspfms;
}

static inline void sys_ll_set_ana_reg19_hstmpo_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 14, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hstmpo_sel(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hstmpo_sel;
}

static inline void sys_ll_set_ana_reg19_hsmpoen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsmpoen(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsmpoen;
}

static inline void sys_ll_set_ana_reg19_hsforcepfm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsforcepfm(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsforcepfm;
}

static inline void sys_ll_set_ana_reg19_hscls(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 18, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hscls(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hscls;
}

static inline void sys_ll_set_ana_reg19_hsswrsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsswrsten(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsswrsten;
}

static inline void sys_ll_set_ana_reg19_hsripc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 22, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsripc(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsripc;
}

static inline void sys_ll_set_ana_reg19_hsrampc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 25, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsrampc(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsrampc;
}

static inline void sys_ll_set_ana_reg19_hsrampcen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsrampcen(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsrampcen;
}

static inline void sys_ll_set_ana_reg19_hsenburst(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hsenburst(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hsenburst;
}

static inline void sys_ll_set_ana_reg19_hspfmen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x53 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg19_hspfmen(void) {
	sys_ana_reg19_t *r = (sys_ana_reg19_t*)(SOC_SYS_REG_BASE + (0x53 << 2));
	return r->hspfmen;
}

//reg ana_reg20:

static inline void sys_ll_set_ana_reg20_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x54 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg20_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x54 << 2));
}

static inline void sys_ll_set_ana_reg20_iselaud(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_iselaud(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->iselaud;
}

static inline void sys_ll_set_ana_reg20_audck_rlcen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_audck_rlcen(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->audck_rlcen;
}

static inline void sys_ll_set_ana_reg20_lchckinven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_lchckinven(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->lchckinven;
}

static inline void sys_ll_set_ana_reg20_enaudbias(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_enaudbias(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->enaudbias;
}

static inline void sys_ll_set_ana_reg20_enadcbias(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_enadcbias(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->enadcbias;
}

static inline void sys_ll_set_ana_reg20_enmicbias(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_enmicbias(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->enmicbias;
}

static inline void sys_ll_set_ana_reg20_adcckinven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_adcckinven(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->adcckinven;
}

static inline void sys_ll_set_ana_reg20_spi(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_spi(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->spi;
}

static inline void sys_ll_set_ana_reg20_adctsten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_adctsten(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->adctsten;
}

static inline void sys_ll_set_ana_reg20_micbias_trm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 9, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg20_micbias_trm(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->micbias_trm;
}

static inline void sys_ll_set_ana_reg20_micbias_voc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 11, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg20_micbias_voc(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->micbias_voc;
}

static inline void sys_ll_set_ana_reg20_vrefsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_vrefsel(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->vrefsel;
}

static inline void sys_ll_set_ana_reg20_capsw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 17, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg20_capsw(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->capsw;
}

static inline void sys_ll_set_ana_reg20_adcref_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 22, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg20_adcref_sel(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->adcref_sel;
}

static inline void sys_ll_set_ana_reg20_adcvcmsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 24, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg20_adcvcmsel(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->adcvcmsel;
}

static inline void sys_ll_set_ana_reg20_spi_1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg20_spi_1(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->spi_1;
}

static inline void sys_ll_set_ana_reg20_audadjref(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x54 << 2)), 27, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg20_audadjref(void) {
	sys_ana_reg20_t *r = (sys_ana_reg20_t*)(SOC_SYS_REG_BASE + (0x54 << 2));
	return r->audadjref;
}

//reg ana_reg21:

static inline void sys_ll_set_ana_reg21_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x55 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg21_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x55 << 2));
}

static inline void sys_ll_set_ana_reg21_isel_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 0, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg21_isel_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->isel_mic1;
}

static inline void sys_ll_set_ana_reg21_micirsel1_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_micirsel1_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->micirsel1_mic1;
}

static inline void sys_ll_set_ana_reg21_vcmsel_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_vcmsel_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->vcmsel_mic1;
}

static inline void sys_ll_set_ana_reg21_enfsr_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_enfsr_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->enfsr_mic1;
}

static inline void sys_ll_set_ana_reg21_enopoclip_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_enopoclip_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->enopoclip_mic1;
}

static inline void sys_ll_set_ana_reg21_da2aden_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_da2aden_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->da2aden_mic1;
}

static inline void sys_ll_set_ana_reg21_imatch_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 7, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg21_imatch_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->imatch_mic1;
}

static inline void sys_ll_set_ana_reg21_imatch_en_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_imatch_en_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->imatch_en_mic1;
}

static inline void sys_ll_set_ana_reg21_dccompen_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_dccompen_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->dccompen_mic1;
}

static inline void sys_ll_set_ana_reg21_micsingleen_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_micsingleen_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->micsingleen_mic1;
}

static inline void sys_ll_set_ana_reg21_nc_14_14(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_nc_14_14(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->nc_14_14;
}

static inline void sys_ll_set_ana_reg21_micgain_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 15, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg21_micgain_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->micgain_mic1;
}

static inline void sys_ll_set_ana_reg21_nc_19_23(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 19, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg21_nc_19_23(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->nc_19_23;
}

static inline void sys_ll_set_ana_reg21_dwamode_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_dwamode_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->dwamode_mic1;
}

static inline void sys_ll_set_ana_reg21_nc_25_26(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 25, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg21_nc_25_26(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->nc_25_26;
}

static inline void sys_ll_set_ana_reg21_rstsel_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_rstsel_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->rstsel_mic1;
}

static inline void sys_ll_set_ana_reg21_micen_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_micen_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->micen_mic1;
}

static inline void sys_ll_set_ana_reg21_rst_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_rst_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->rst_mic1;
}

static inline void sys_ll_set_ana_reg21_bpdwa1v_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_bpdwa1v_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->bpdwa1v_mic1;
}

static inline void sys_ll_set_ana_reg21_hcen1stg_mic1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x55 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg21_hcen1stg_mic1(void) {
	sys_ana_reg21_t *r = (sys_ana_reg21_t*)(SOC_SYS_REG_BASE + (0x55 << 2));
	return r->hcen1stg_mic1;
}

//reg ana_reg22:

static inline void sys_ll_set_ana_reg22_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x56 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg22_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x56 << 2));
}

static inline void sys_ll_set_ana_reg22_inbuffer_isel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 0, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg22_inbuffer_isel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->inbuffer_isel;
}

static inline void sys_ll_set_ana_reg22_gadc_offset_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_offset_en(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_offset_en;
}

static inline void sys_ll_set_ana_reg22_gadc_vref_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_vref_sel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_vref_sel;
}

static inline void sys_ll_set_ana_reg22_gadc_bufamp_isel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 4, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_bufamp_isel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_bufamp_isel;
}

static inline void sys_ll_set_ana_reg22_gadc_preamp_isel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 7, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_preamp_isel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_preamp_isel;
}

static inline void sys_ll_set_ana_reg22_gadc_comp_isel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 10, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_comp_isel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_comp_isel;
}

static inline void sys_ll_set_ana_reg22_gadc_bscalsaw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 13, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_bscalsaw(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_bscalsaw;
}

static inline void sys_ll_set_ana_reg22_gadc_vncalsaw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 16, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_vncalsaw(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_vncalsaw;
}

static inline void sys_ll_set_ana_reg22_gadc_vpcalsaw(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 19, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_vpcalsaw(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_vpcalsaw;
}

static inline void sys_ll_set_ana_reg22_irefen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_irefen(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->irefen;
}

static inline void sys_ll_set_ana_reg22_gadc_vbg_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_vbg_sel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_vbg_sel;
}

static inline void sys_ll_set_ana_reg22_gadc_clk_rlten(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_clk_rlten(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_clk_rlten;
}

static inline void sys_ll_set_ana_reg22_gadc_calintsaw_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_calintsaw_en(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_calintsaw_en;
}

static inline void sys_ll_set_ana_reg22_gadc_clk_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_clk_sel(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_clk_sel;
}

static inline void sys_ll_set_ana_reg22_gadc_clk_in(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_clk_in(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_clk_in;
}

static inline void sys_ll_set_ana_reg22_gadc_calcap_ch(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 28, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_calcap_ch(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_calcap_ch;
}

static inline void sys_ll_set_ana_reg22_gadc_inbuf_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_inbuf_en(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_inbuf_en;
}

static inline void sys_ll_set_ana_reg22_gadc_en_spi(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x56 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg22_gadc_en_spi(void) {
	sys_ana_reg22_t *r = (sys_ana_reg22_t*)(SOC_SYS_REG_BASE + (0x56 << 2));
	return r->gadc_en_spi;
}

//reg ana_reg23:

static inline void sys_ll_set_ana_reg23_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x57 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg23_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x57 << 2));
}

static inline void sys_ll_set_ana_reg23_vadckinven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vadckinven(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vadckinven;
}

static inline void sys_ll_set_ana_reg23_vadrefsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vadrefsel(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vadrefsel;
}

static inline void sys_ll_set_ana_reg23_vad_rstn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vad_rstn(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vad_rstn;
}

static inline void sys_ll_set_ana_reg23_vad_viniset(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vad_viniset(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vad_viniset;
}

static inline void sys_ll_set_ana_reg23_vad_cstrm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 4, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vad_cstrm(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vad_cstrm;
}

static inline void sys_ll_set_ana_reg23_vad_cftrm(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 6, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vad_cftrm(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vad_cftrm;
}

static inline void sys_ll_set_ana_reg23_vad_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vad_en(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vad_en;
}

static inline void sys_ll_set_ana_reg23_vad_ctrl0(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x57 << 2)), 9, 0x7fffff, v);
}

static inline uint32_t sys_ll_get_ana_reg23_vad_ctrl0(void) {
	sys_ana_reg23_t *r = (sys_ana_reg23_t*)(SOC_SYS_REG_BASE + (0x57 << 2));
	return r->vad_ctrl0;
}

//reg ana_reg24:

static inline void sys_ll_set_ana_reg24_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x58 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg24_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x58 << 2));
}

static inline void sys_ll_set_ana_reg24_vad_ctrl1(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x58 << 2)), 0, 0xffffffff, v);
}

//reg ana_reg25:

static inline void sys_ll_set_ana_reg25_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x59 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg25_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x59 << 2));
}

static inline void sys_ll_set_ana_reg25_int_mod(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_int_mod(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->int_mod;
}

static inline void sys_ll_set_ana_reg25_nsyn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_nsyn(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->nsyn;
}

static inline void sys_ll_set_ana_reg25_open_enb(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_open_enb(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->open_enb;
}

static inline void sys_ll_set_ana_reg25_reset(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_reset(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->reset;
}

static inline void sys_ll_set_ana_reg25_ioffsetl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 4, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg25_ioffsetl(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->ioffsetl;
}

static inline void sys_ll_set_ana_reg25_lpfrz(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 7, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg25_lpfrz(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->lpfrz;
}

static inline void sys_ll_set_ana_reg25_vsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 11, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg25_vsel(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->vsel;
}

static inline void sys_ll_set_ana_reg25_vsel_cal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_vsel_cal(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->vsel_cal;
}

static inline void sys_ll_set_ana_reg25_pwd_lockdet(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_pwd_lockdet(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->pwd_lockdet;
}

static inline void sys_ll_set_ana_reg25_lockdet_bypass(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_lockdet_bypass(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->lockdet_bypass;
}

static inline void sys_ll_set_ana_reg25_ckref_loop_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_ckref_loop_sel(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->ckref_loop_sel;
}

static inline void sys_ll_set_ana_reg25_spi_trigger(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_spi_trigger(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->spi_trigger;
}

static inline void sys_ll_set_ana_reg25_manual(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_manual(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->manual;
}

static inline void sys_ll_set_ana_reg25_test_ckaudio_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_test_ckaudio_en(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->test_ckaudio_en;
}

static inline void sys_ll_set_ana_reg25_ck2xen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_ck2xen(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->ck2xen;
}

static inline void sys_ll_set_ana_reg25_icp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 22, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg25_icp(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->icp;
}

static inline void sys_ll_set_ana_reg25_cktst_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_cktst_sel(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->cktst_sel;
}

static inline void sys_ll_set_ana_reg25_edgesel_nck(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_edgesel_nck(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->edgesel_nck;
}

static inline void sys_ll_set_ana_reg25_nloaddlyen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_nloaddlyen(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->nloaddlyen;
}

static inline void sys_ll_set_ana_reg25_bypass_caldone_auto(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_bypass_caldone_auto(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->bypass_caldone_auto;
}

static inline void sys_ll_set_ana_reg25_cal_res_spi(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 28, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg25_cal_res_spi(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->cal_res_spi;
}

static inline void sys_ll_set_ana_reg25_audioen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x59 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg25_audioen(void) {
	sys_ana_reg25_t *r = (sys_ana_reg25_t*)(SOC_SYS_REG_BASE + (0x59 << 2));
	return r->audioen;
}

//reg ana_reg26:

static inline void sys_ll_set_ana_reg26_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x5a << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg26_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x5a << 2));
}

static inline void sys_ll_set_ana_reg26_n(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5a << 2)), 0, 0x3fffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg26_n(void) {
	sys_ana_reg26_t *r = (sys_ana_reg26_t*)(SOC_SYS_REG_BASE + (0x5a << 2));
	return r->n;
}

static inline void sys_ll_set_ana_reg26_calres_spien(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5a << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg26_calres_spien(void) {
	sys_ana_reg26_t *r = (sys_ana_reg26_t*)(SOC_SYS_REG_BASE + (0x5a << 2));
	return r->calres_spien;
}

static inline void sys_ll_set_ana_reg26_calrefen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5a << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg26_calrefen(void) {
	sys_ana_reg26_t *r = (sys_ana_reg26_t*)(SOC_SYS_REG_BASE + (0x5a << 2));
	return r->calrefen;
}

//reg ana_reg27:

static inline void sys_ll_set_ana_reg27_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x5b << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg27_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x5b << 2));
}

static inline void sys_ll_set_ana_reg27_isel_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 0, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg27_isel_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->isel_mic2;
}

static inline void sys_ll_set_ana_reg27_micirsel1_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_micirsel1_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->micirsel1_mic2;
}

static inline void sys_ll_set_ana_reg27_vcmsel_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_vcmsel_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->vcmsel_mic2;
}

static inline void sys_ll_set_ana_reg27_enfsr_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_enfsr_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->enfsr_mic2;
}

static inline void sys_ll_set_ana_reg27_enopoclip_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_enopoclip_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->enopoclip_mic2;
}

static inline void sys_ll_set_ana_reg27_da2aden_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_da2aden_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->da2aden_mic2;
}

static inline void sys_ll_set_ana_reg27_imatch_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 7, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg27_imatch_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->imatch_mic2;
}

static inline void sys_ll_set_ana_reg27_imatch_en_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_imatch_en_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->imatch_en_mic2;
}

static inline void sys_ll_set_ana_reg27_dccompen_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_dccompen_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->dccompen_mic2;
}

static inline void sys_ll_set_ana_reg27_micsingleen_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_micsingleen_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->micsingleen_mic2;
}

static inline void sys_ll_set_ana_reg27_nc_14_14(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_nc_14_14(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->nc_14_14;
}

static inline void sys_ll_set_ana_reg27_micgain_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 15, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg27_micgain_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->micgain_mic2;
}

static inline void sys_ll_set_ana_reg27_nc_19_23(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 19, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg27_nc_19_23(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->nc_19_23;
}

static inline void sys_ll_set_ana_reg27_dwamode_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_dwamode_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->dwamode_mic2;
}

static inline void sys_ll_set_ana_reg27_nc_25_26(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 25, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg27_nc_25_26(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->nc_25_26;
}

static inline void sys_ll_set_ana_reg27_rstsel_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_rstsel_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->rstsel_mic2;
}

static inline void sys_ll_set_ana_reg27_micen_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_micen_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->micen_mic2;
}

static inline void sys_ll_set_ana_reg27_rst_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_rst_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->rst_mic2;
}

static inline void sys_ll_set_ana_reg27_bpdwa1v_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_bpdwa1v_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->bpdwa1v_mic2;
}

static inline void sys_ll_set_ana_reg27_hcen1stg_mic2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5b << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg27_hcen1stg_mic2(void) {
	sys_ana_reg27_t *r = (sys_ana_reg27_t*)(SOC_SYS_REG_BASE + (0x5b << 2));
	return r->hcen1stg_mic2;
}

//reg ana_reg28:

static inline void sys_ll_set_ana_reg28_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x5c << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg28_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x5c << 2));
}

static inline void sys_ll_set_ana_reg28_isel_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 0, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg28_isel_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->isel_mic3;
}

static inline void sys_ll_set_ana_reg28_micirsel1_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_micirsel1_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->micirsel1_mic3;
}

static inline void sys_ll_set_ana_reg28_vcmsel_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_vcmsel_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->vcmsel_mic3;
}

static inline void sys_ll_set_ana_reg28_enfsr_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_enfsr_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->enfsr_mic3;
}

static inline void sys_ll_set_ana_reg28_enopoclip_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_enopoclip_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->enopoclip_mic3;
}

static inline void sys_ll_set_ana_reg28_da2aden_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_da2aden_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->da2aden_mic3;
}

static inline void sys_ll_set_ana_reg28_imatch_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 7, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg28_imatch_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->imatch_mic3;
}

static inline void sys_ll_set_ana_reg28_imatch_en_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_imatch_en_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->imatch_en_mic3;
}

static inline void sys_ll_set_ana_reg28_dccompen_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_dccompen_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->dccompen_mic3;
}

static inline void sys_ll_set_ana_reg28_micsingleen_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_micsingleen_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->micsingleen_mic3;
}

static inline void sys_ll_set_ana_reg28_nc_14_14(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_nc_14_14(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->nc_14_14;
}

static inline void sys_ll_set_ana_reg28_micgain_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 15, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg28_micgain_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->micgain_mic3;
}

static inline void sys_ll_set_ana_reg28_nc_19_23(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 19, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg28_nc_19_23(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->nc_19_23;
}

static inline void sys_ll_set_ana_reg28_dwamode_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_dwamode_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->dwamode_mic3;
}

static inline void sys_ll_set_ana_reg28_nc_25_26(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 25, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg28_nc_25_26(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->nc_25_26;
}

static inline void sys_ll_set_ana_reg28_rstsel_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_rstsel_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->rstsel_mic3;
}

static inline void sys_ll_set_ana_reg28_micen_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_micen_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->micen_mic3;
}

static inline void sys_ll_set_ana_reg28_rst_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_rst_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->rst_mic3;
}

static inline void sys_ll_set_ana_reg28_bpdwa1v_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_bpdwa1v_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->bpdwa1v_mic3;
}

static inline void sys_ll_set_ana_reg28_hcen1stg_mic3(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5c << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg28_hcen1stg_mic3(void) {
	sys_ana_reg28_t *r = (sys_ana_reg28_t*)(SOC_SYS_REG_BASE + (0x5c << 2));
	return r->hcen1stg_mic3;
}

//reg ana_reg29:

static inline void sys_ll_set_ana_reg29_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x5d << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg29_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x5d << 2));
}

static inline void sys_ll_set_ana_reg29_hpdac(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_hpdac(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->hpdac;
}

static inline void sys_ll_set_ana_reg29_iselstg(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_iselstg(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->iselstg;
}

static inline void sys_ll_set_ana_reg29_oscdac(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 2, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg29_oscdac(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->oscdac;
}

static inline void sys_ll_set_ana_reg29_ocendac(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_ocendac(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->ocendac;
}

static inline void sys_ll_set_ana_reg29_vseldco(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_vseldco(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->vseldco;
}

static inline void sys_ll_set_ana_reg29_srsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_srsel(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->srsel;
}

static inline void sys_ll_set_ana_reg29_hpoen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 7, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_hpoen(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->hpoen;
}

static inline void sys_ll_set_ana_reg29_lbwen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_lbwen(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->lbwen;
}

static inline void sys_ll_set_ana_reg29_calsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_calsel(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->calsel;
}

static inline void sys_ll_set_ana_reg29_bp2vldo(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_bp2vldo(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->bp2vldo;
}

static inline void sys_ll_set_ana_reg29_dcochg(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 11, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg29_dcochg(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->dcochg;
}

static inline void sys_ll_set_ana_reg29_diffen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_diffen(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->diffen;
}

static inline void sys_ll_set_ana_reg29_endaccal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_endaccal(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->endaccal;
}

static inline void sys_ll_set_ana_reg29_rendcoc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_rendcoc(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->rendcoc;
}

static inline void sys_ll_set_ana_reg29_lendcoc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_lendcoc(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->lendcoc;
}

static inline void sys_ll_set_ana_reg29_renvcmd(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_renvcmd(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->renvcmd;
}

static inline void sys_ll_set_ana_reg29_lenvcmd(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_lenvcmd(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->lenvcmd;
}

static inline void sys_ll_set_ana_reg29_dacdrven(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_dacdrven(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->dacdrven;
}

static inline void sys_ll_set_ana_reg29_dacren(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_dacren(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->dacren;
}

static inline void sys_ll_set_ana_reg29_daclen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_daclen(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->daclen;
}

static inline void sys_ll_set_ana_reg29_dacg(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 22, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg29_dacg(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->dacg;
}

static inline void sys_ll_set_ana_reg29_dacmute(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_dacmute(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->dacmute;
}

static inline void sys_ll_set_ana_reg29_dacdwamode_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_dacdwamode_sel(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->dacdwamode_sel;
}

static inline void sys_ll_set_ana_reg29_ckpsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg29_ckpsel(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->ckpsel;
}

static inline void sys_ll_set_ana_reg29_nc_29_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5d << 2)), 29, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg29_nc_29_31(void) {
	sys_ana_reg29_t *r = (sys_ana_reg29_t*)(SOC_SYS_REG_BASE + (0x5d << 2));
	return r->nc_29_31;
}

//reg ana_reg30:

static inline void sys_ll_set_ana_reg30_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x5e << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg30_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x5e << 2));
}

static inline void sys_ll_set_ana_reg30_lmdcin(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 0, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg30_lmdcin(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->lmdcin;
}

static inline void sys_ll_set_ana_reg30_rmdcin(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 8, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg30_rmdcin(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->rmdcin;
}

static inline void sys_ll_set_ana_reg30_spirst_ovc(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_spirst_ovc(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->spirst_ovc;
}

static inline void sys_ll_set_ana_reg30_enidacr(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_enidacr(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->enidacr;
}

static inline void sys_ll_set_ana_reg30_enidacl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_enidacl(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->enidacl;
}

static inline void sys_ll_set_ana_reg30_dac3rdhc0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_dac3rdhc0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->dac3rdhc0v9;
}

static inline void sys_ll_set_ana_reg30_hc2s(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_hc2s(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->hc2s;
}

static inline void sys_ll_set_ana_reg30_sng_fb_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_sng_fb_en(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->sng_fb_en;
}

static inline void sys_ll_set_ana_reg30_rfb_ctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_rfb_ctrl(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->rfb_ctrl;
}

static inline void sys_ll_set_ana_reg30_enbs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 23, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_enbs(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->enbs;
}

static inline void sys_ll_set_ana_reg30_calck_sel0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_calck_sel0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->calck_sel0v9;
}

static inline void sys_ll_set_ana_reg30_bpdwa0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_bpdwa0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->bpdwa0v9;
}

static inline void sys_ll_set_ana_reg30_looprst0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_looprst0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->looprst0v9;
}

static inline void sys_ll_set_ana_reg30_oct0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 27, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg30_oct0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->oct0v9;
}

static inline void sys_ll_set_ana_reg30_sout0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg30_sout0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->sout0v9;
}

static inline void sys_ll_set_ana_reg30_hc0v9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5e << 2)), 30, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg30_hc0v9(void) {
	sys_ana_reg30_t *r = (sys_ana_reg30_t*)(SOC_SYS_REG_BASE + (0x5e << 2));
	return r->hc0v9;
}

//reg ana_reg31:

static inline void sys_ll_set_ana_reg31_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x5f << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg31_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x5f << 2));
}

static inline void sys_ll_set_ana_reg31_nc_0_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x5f << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg31_nc_0_31(void) {
	sys_ana_reg31_t *r = (sys_ana_reg31_t*)(SOC_SYS_REG_BASE + (0x5f << 2));
	return r->nc_0_31;
}

//reg ana_reg32:

static inline void sys_ll_set_ana_reg32_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x60 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg32_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x60 << 2));
}

static inline void sys_ll_set_ana_reg32_nc_0_2(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 0, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg32_nc_0_2(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->nc_0_2;
}

static inline void sys_ll_set_ana_reg32_vad_ck_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 3, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_vad_ck_sel(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->vad_ck_sel;
}

static inline void sys_ll_set_ana_reg32_int_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 4, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_int_sel(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->int_sel;
}

static inline void sys_ll_set_ana_reg32_ldoen(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 5, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_ldoen(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->ldoen;
}

static inline void sys_ll_set_ana_reg32_ldoctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 6, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_ldoctrl(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->ldoctrl;
}

static inline void sys_ll_set_ana_reg32_ibctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 7, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_ibctrl(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->ibctrl;
}

static inline void sys_ll_set_ana_reg32_rstb_dig(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_rstb_dig(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->rstb_dig;
}

static inline void sys_ll_set_ana_reg32_en_vtest_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_en_vtest_sel(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->en_vtest_sel;
}

static inline void sys_ll_set_ana_reg32_en_adcmod(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_en_adcmod(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->en_adcmod;
}

static inline void sys_ll_set_ana_reg32_en_out_test(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_en_out_test(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->en_out_test;
}

static inline void sys_ll_set_ana_reg32_nc_12_12(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_nc_12_12(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->nc_12_12;
}

static inline void sys_ll_set_ana_reg32_sel_seri_cap(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 13, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_sel_seri_cap(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->sel_seri_cap;
}

static inline void sys_ll_set_ana_reg32_en_seri_cap(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 14, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_en_seri_cap(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->en_seri_cap;
}

static inline void sys_ll_set_ana_reg32_cal_ctrl(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 15, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg32_cal_ctrl(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->cal_ctrl;
}

static inline void sys_ll_set_ana_reg32_cal_vth(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 17, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg32_cal_vth(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->cal_vth;
}

static inline void sys_ll_set_ana_reg32_crg(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 20, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg32_crg(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->crg;
}

static inline void sys_ll_set_ana_reg32_vrefs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 22, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg32_vrefs(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->vrefs;
}

static inline void sys_ll_set_ana_reg32_gain_s(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 26, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg32_gain_s(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->gain_s;
}

static inline void sys_ll_set_ana_reg32_td_latch(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_td_latch(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->td_latch;
}

static inline void sys_ll_set_ana_reg32_pwd_td(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x60 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg32_pwd_td(void) {
	sys_ana_reg32_t *r = (sys_ana_reg32_t*)(SOC_SYS_REG_BASE + (0x60 << 2));
	return r->pwd_td;
}

//reg ana_reg33:

static inline void sys_ll_set_ana_reg33_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x61 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg33_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x61 << 2));
}

static inline void sys_ll_set_ana_reg33_test_number(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 0, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg33_test_number(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->test_number;
}

static inline void sys_ll_set_ana_reg33_test_period(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 4, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg33_test_period(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->test_period;
}

static inline void sys_ll_set_ana_reg33_chs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 8, 0xffff, v);
}

static inline uint32_t sys_ll_get_ana_reg33_chs(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->chs;
}

static inline void sys_ll_set_ana_reg33_chs_sel_cal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 24, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg33_chs_sel_cal(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->chs_sel_cal;
}

static inline void sys_ll_set_ana_reg33_cal_done_clr(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg33_cal_done_clr(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->cal_done_clr;
}

static inline void sys_ll_set_ana_reg33_en_cal_force(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg33_en_cal_force(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->en_cal_force;
}

static inline void sys_ll_set_ana_reg33_en_cal_auto(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg33_en_cal_auto(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->en_cal_auto;
}

static inline void sys_ll_set_ana_reg33_en_scan(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x61 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg33_en_scan(void) {
	sys_ana_reg33_t *r = (sys_ana_reg33_t*)(SOC_SYS_REG_BASE + (0x61 << 2));
	return r->en_scan;
}

//reg ana_reg34:

static inline void sys_ll_set_ana_reg34_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x62 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg34_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x62 << 2));
}

static inline void sys_ll_set_ana_reg34_int_en(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x62 << 2)), 0, 0x1ffff, v);
}

static inline uint32_t sys_ll_get_ana_reg34_int_en(void) {
	sys_ana_reg34_t *r = (sys_ana_reg34_t*)(SOC_SYS_REG_BASE + (0x62 << 2));
	return r->int_en;
}

static inline void sys_ll_set_ana_reg34_nc_17_17(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x62 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg34_nc_17_17(void) {
	sys_ana_reg34_t *r = (sys_ana_reg34_t*)(SOC_SYS_REG_BASE + (0x62 << 2));
	return r->nc_17_17;
}

static inline void sys_ll_set_ana_reg34_modsel_spi(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x62 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg34_modsel_spi(void) {
	sys_ana_reg34_t *r = (sys_ana_reg34_t*)(SOC_SYS_REG_BASE + (0x62 << 2));
	return r->modsel_spi;
}

static inline void sys_ll_set_ana_reg34_cal_number(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x62 << 2)), 19, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg34_cal_number(void) {
	sys_ana_reg34_t *r = (sys_ana_reg34_t*)(SOC_SYS_REG_BASE + (0x62 << 2));
	return r->cal_number;
}

static inline void sys_ll_set_ana_reg34_cl_period(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x62 << 2)), 23, 0x1ff, v);
}

static inline uint32_t sys_ll_get_ana_reg34_cl_period(void) {
	sys_ana_reg34_t *r = (sys_ana_reg34_t*)(SOC_SYS_REG_BASE + (0x62 << 2));
	return r->cl_period;
}

//reg ana_reg35:

static inline void sys_ll_set_ana_reg35_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x63 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg35_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x63 << 2));
}

static inline void sys_ll_set_ana_reg35_int_clr(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 0, 0x1ffff, v);
}

static inline uint32_t sys_ll_get_ana_reg35_int_clr(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->int_clr;
}

static inline void sys_ll_set_ana_reg35_nc_17_17(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg35_nc_17_17(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->nc_17_17;
}

static inline void sys_ll_set_ana_reg35_int_clr_sel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 18, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg35_int_clr_sel(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->int_clr_sel;
}

static inline void sys_ll_set_ana_reg35_en_lpmod(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg35_en_lpmod(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->en_lpmod;
}

static inline void sys_ll_set_ana_reg35_en_testcmp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg35_en_testcmp(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->en_testcmp;
}

static inline void sys_ll_set_ana_reg35_en_man_wr(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg35_en_man_wr(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->en_man_wr;
}

static inline void sys_ll_set_ana_reg35_en_manmode(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg35_en_manmode(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->en_manmode;
}

static inline void sys_ll_set_ana_reg35_cap_calspi(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x63 << 2)), 23, 0x1ff, v);
}

static inline uint32_t sys_ll_get_ana_reg35_cap_calspi(void) {
	sys_ana_reg35_t *r = (sys_ana_reg35_t*)(SOC_SYS_REG_BASE + (0x63 << 2));
	return r->cap_calspi;
}

//reg ana_reg36:

static inline void sys_ll_set_ana_reg36_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x64 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg36_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x64 << 2));
}

static inline void sys_ll_set_ana_reg36_int_clr_cal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x64 << 2)), 0, 0xffff, v);
}

static inline uint32_t sys_ll_get_ana_reg36_int_clr_cal(void) {
	sys_ana_reg36_t *r = (sys_ana_reg36_t*)(SOC_SYS_REG_BASE + (0x64 << 2));
	return r->int_clr_cal;
}

static inline void sys_ll_set_ana_reg36_int_en_cal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x64 << 2)), 16, 0xffff, v);
}

static inline uint32_t sys_ll_get_ana_reg36_int_en_cal(void) {
	sys_ana_reg36_t *r = (sys_ana_reg36_t*)(SOC_SYS_REG_BASE + (0x64 << 2));
	return r->int_en_cal;
}

//reg ana_reg37:

static inline void sys_ll_set_ana_reg37_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x65 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg37_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x65 << 2));
}

static inline void sys_ll_set_ana_reg37_reset(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_reset(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->reset;
}

static inline void sys_ll_set_ana_reg37_clear_int(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_clear_int(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->clear_int;
}

static inline void sys_ll_set_ana_reg37_xmin_init(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 2, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_xmin_init(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->xmin_init;
}

static inline void sys_ll_set_ana_reg37_gain(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 3, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg37_gain(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->gain;
}

static inline void sys_ll_set_ana_reg37_alphal(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 7, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg37_alphal(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->alphal;
}

static inline void sys_ll_set_ana_reg37_alphas(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 10, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg37_alphas(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->alphas;
}

static inline void sys_ll_set_ana_reg37_lpfn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 13, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg37_lpfn(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->lpfn;
}

static inline void sys_ll_set_ana_reg37_hpfn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 16, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg37_hpfn(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->hpfn;
}

static inline void sys_ll_set_ana_reg37_lpf_bypass(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 19, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_lpf_bypass(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->lpf_bypass;
}

static inline void sys_ll_set_ana_reg37_hpf_bypass(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 20, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_hpf_bypass(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->hpf_bypass;
}

static inline void sys_ll_set_ana_reg37_is_abs(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 21, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_is_abs(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->is_abs;
}

static inline void sys_ll_set_ana_reg37_dir(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 22, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg37_dir(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->dir;
}

static inline void sys_ll_set_ana_reg37_nc_23_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x65 << 2)), 23, 0x1ff, v);
}

static inline uint32_t sys_ll_get_ana_reg37_nc_23_31(void) {
	sys_ana_reg37_t *r = (sys_ana_reg37_t*)(SOC_SYS_REG_BASE + (0x65 << 2));
	return r->nc_23_31;
}

//reg ana_reg38:

static inline void sys_ll_set_ana_reg38_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x66 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg38_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x66 << 2));
}

static inline void sys_ll_set_ana_reg38_alpha(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x66 << 2)), 0, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg38_alpha(void) {
	sys_ana_reg38_t *r = (sys_ana_reg38_t*)(SOC_SYS_REG_BASE + (0x66 << 2));
	return r->alpha;
}

static inline void sys_ll_set_ana_reg38_comp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x66 << 2)), 8, 0xfff, v);
}

static inline uint32_t sys_ll_get_ana_reg38_comp(void) {
	sys_ana_reg38_t *r = (sys_ana_reg38_t*)(SOC_SYS_REG_BASE + (0x66 << 2));
	return r->comp;
}

static inline void sys_ll_set_ana_reg38_cntn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x66 << 2)), 20, 0xfff, v);
}

static inline uint32_t sys_ll_get_ana_reg38_cntn(void) {
	sys_ana_reg38_t *r = (sys_ana_reg38_t*)(SOC_SYS_REG_BASE + (0x66 << 2));
	return r->cntn;
}

//reg ana_reg39:

static inline void sys_ll_set_ana_reg39_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x67 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg39_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x67 << 2));
}

static inline void sys_ll_set_ana_reg39_enspi_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg39_enspi_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->enspi_i;
}

static inline void sys_ll_set_ana_reg39_ck_edge_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg39_ck_edge_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->ck_edge_i;
}

static inline void sys_ll_set_ana_reg39_outbuff_isel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 2, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg39_outbuff_isel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->outbuff_isel_i;
}

static inline void sys_ll_set_ana_reg39_refbuff_isel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 5, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg39_refbuff_isel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->refbuff_isel_i;
}

static inline void sys_ll_set_ana_reg39_cap_fb_sel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 8, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg39_cap_fb_sel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->cap_fb_sel_i;
}

static inline void sys_ll_set_ana_reg39_gain_sel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 12, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg39_gain_sel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->gain_sel_i;
}

static inline void sys_ll_set_ana_reg39_vref_cal_sel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 15, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg39_vref_cal_sel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->vref_cal_sel_i;
}

static inline void sys_ll_set_ana_reg39_cap_cal_sel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 20, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg39_cap_cal_sel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->cap_cal_sel_i;
}

static inline void sys_ll_set_ana_reg39_cal_direction_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg39_cal_direction_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->cal_direction_i;
}

static inline void sys_ll_set_ana_reg39_endigspi_sel_i(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg39_endigspi_sel_i(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->endigspi_sel_i;
}

static inline void sys_ll_set_ana_reg39_nc_26_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x67 << 2)), 26, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg39_nc_26_31(void) {
	sys_ana_reg39_t *r = (sys_ana_reg39_t*)(SOC_SYS_REG_BASE + (0x67 << 2));
	return r->nc_26_31;
}

//reg ana_reg40:

static inline void sys_ll_set_ana_reg40_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x68 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg40_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x68 << 2));
}

static inline void sys_ll_set_ana_reg40_enspi_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg40_enspi_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->enspi_q;
}

static inline void sys_ll_set_ana_reg40_ck_edge_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg40_ck_edge_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->ck_edge_q;
}

static inline void sys_ll_set_ana_reg40_outbuff_qsel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 2, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg40_outbuff_qsel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->outbuff_qsel_q;
}

static inline void sys_ll_set_ana_reg40_refbuff_qsel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 5, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg40_refbuff_qsel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->refbuff_qsel_q;
}

static inline void sys_ll_set_ana_reg40_cap_fb_sel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 8, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg40_cap_fb_sel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->cap_fb_sel_q;
}

static inline void sys_ll_set_ana_reg40_gain_sel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 12, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg40_gain_sel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->gain_sel_q;
}

static inline void sys_ll_set_ana_reg40_vref_cal_sel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 15, 0x1f, v);
}

static inline uint32_t sys_ll_get_ana_reg40_vref_cal_sel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->vref_cal_sel_q;
}

static inline void sys_ll_set_ana_reg40_cap_cal_sel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 20, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg40_cap_cal_sel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->cap_cal_sel_q;
}

static inline void sys_ll_set_ana_reg40_cal_direction_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg40_cal_direction_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->cal_direction_q;
}

static inline void sys_ll_set_ana_reg40_endigspi_sel_q(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg40_endigspi_sel_q(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->endigspi_sel_q;
}

static inline void sys_ll_set_ana_reg40_nc_26_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x68 << 2)), 26, 0x3f, v);
}

static inline uint32_t sys_ll_get_ana_reg40_nc_26_31(void) {
	sys_ana_reg40_t *r = (sys_ana_reg40_t*)(SOC_SYS_REG_BASE + (0x68 << 2));
	return r->nc_26_31;
}

//reg ana_reg41:

static inline void sys_ll_set_ana_reg41_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x69 << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg41_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x69 << 2));
}

static inline void sys_ll_set_ana_reg41_nc_0_10(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 0, 0x7ff, v);
}

static inline uint32_t sys_ll_get_ana_reg41_nc_0_10(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->nc_0_10;
}

static inline void sys_ll_set_ana_reg41_vsel_auxldo3v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 11, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg41_vsel_auxldo3v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->vsel_auxldo3v;
}

static inline void sys_ll_set_ana_reg41_vsel_auxldo2p8v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 15, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg41_vsel_auxldo2p8v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->vsel_auxldo2p8v;
}

static inline void sys_ll_set_ana_reg41_vsel_auxldo1p8v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 19, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg41_vsel_auxldo1p8v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->vsel_auxldo1p8v;
}

static inline void sys_ll_set_ana_reg41_vsel_auxldo1p2v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 23, 0x7, v);
}

static inline uint32_t sys_ll_get_ana_reg41_vsel_auxldo1p2v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->vsel_auxldo1p2v;
}

static inline void sys_ll_set_ana_reg41_swb_auxldo3v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg41_swb_auxldo3v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->swb_auxldo3v;
}

static inline void sys_ll_set_ana_reg41_swb_auxldo2p8v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg41_swb_auxldo2p8v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->swb_auxldo2p8v;
}

static inline void sys_ll_set_ana_reg41_en_auxldo3v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg41_en_auxldo3v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->en_auxldo3v;
}

static inline void sys_ll_set_ana_reg41_en_auxldo2p8v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 29, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg41_en_auxldo2p8v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->en_auxldo2p8v;
}

static inline void sys_ll_set_ana_reg41_en_auxldo_1p8v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg41_en_auxldo_1p8v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->en_auxldo_1p8v;
}

static inline void sys_ll_set_ana_reg41_en_auxldo_1p2v(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x69 << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg41_en_auxldo_1p2v(void) {
	sys_ana_reg41_t *r = (sys_ana_reg41_t*)(SOC_SYS_REG_BASE + (0x69 << 2));
	return r->en_auxldo_1p2v;
}

//reg ana_reg42:

static inline void sys_ll_set_ana_reg42_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x6a << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg42_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x6a << 2));
}

static inline void sys_ll_set_ana_reg42_nc_0_9(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 0, 0x3ff, v);
}

static inline uint32_t sys_ll_get_ana_reg42_nc_0_9(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->nc_0_9;
}

static inline void sys_ll_set_ana_reg42_dslep_disable(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg42_dslep_disable(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->dslep_disable;
}

static inline void sys_ll_set_ana_reg42_en_vout(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg42_en_vout(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->en_vout;
}

static inline void sys_ll_set_ana_reg42_vusbsel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 12, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg42_vusbsel(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->vusbsel;
}

static inline void sys_ll_set_ana_reg42_usbnen_dn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 14, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg42_usbnen_dn(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->usbnen_dn;
}

static inline void sys_ll_set_ana_reg42_usbpen_dn(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 18, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg42_usbpen_dn(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->usbpen_dn;
}

static inline void sys_ll_set_ana_reg42_usbnen_dp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 22, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg42_usbnen_dp(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->usbnen_dp;
}

static inline void sys_ll_set_ana_reg42_usbpen_dp(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 26, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg42_usbpen_dp(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->usbpen_dp;
}

static inline void sys_ll_set_ana_reg42_usb_speed(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 30, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg42_usb_speed(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->usb_speed;
}

static inline void sys_ll_set_ana_reg42_pwd_usb(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6a << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg42_pwd_usb(void) {
	sys_ana_reg42_t *r = (sys_ana_reg42_t*)(SOC_SYS_REG_BASE + (0x6a << 2));
	return r->pwd_usb;
}

//reg ana_reg43:

static inline void sys_ll_set_ana_reg43_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x6b << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg43_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x6b << 2));
}

static inline void sys_ll_set_ana_reg43_en_lpdac_a_votest(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 0, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_en_lpdac_a_votest(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->en_lpdac_a_votest;
}

static inline void sys_ll_set_ana_reg43_acmp_clr_out_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 1, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_clr_out_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_clr_out_a;
}

static inline void sys_ll_set_ana_reg43_acmp_chsel_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 2, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_chsel_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_chsel_a;
}

static inline void sys_ll_set_ana_reg43_acmp_hystsel_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 6, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_hystsel_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_hystsel_a;
}

static inline void sys_ll_set_ana_reg43_acmp_npmd_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 8, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_npmd_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_npmd_a;
}

static inline void sys_ll_set_ana_reg43_acmp_hpmd_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 9, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_hpmd_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_hpmd_a;
}

static inline void sys_ll_set_ana_reg43_en_anacomp_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 10, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_en_anacomp_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->en_anacomp_a;
}

static inline void sys_ll_set_ana_reg43_vbg_sel_lpdac_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 11, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_vbg_sel_lpdac_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->vbg_sel_lpdac_a;
}

static inline void sys_ll_set_ana_reg43_en_lpdac_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 12, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_en_lpdac_a(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->en_lpdac_a;
}

static inline void sys_ll_set_ana_reg43_comp_a_vosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 13, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg43_comp_a_vosel(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->comp_a_vosel;
}

static inline void sys_ll_set_ana_reg43_nc_15_15(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 15, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_nc_15_15(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->nc_15_15;
}

static inline void sys_ll_set_ana_reg43_en_lpdac_b_votest(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 16, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_en_lpdac_b_votest(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->en_lpdac_b_votest;
}

static inline void sys_ll_set_ana_reg43_acmp_clr_out_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 17, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_clr_out_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_clr_out_b;
}

static inline void sys_ll_set_ana_reg43_acmp_chsel_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 18, 0xf, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_chsel_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_chsel_b;
}

static inline void sys_ll_set_ana_reg43_acmp_hystsel_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 22, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_hystsel_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_hystsel_b;
}

static inline void sys_ll_set_ana_reg43_acmp_npmd_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 24, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_npmd_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_npmd_b;
}

static inline void sys_ll_set_ana_reg43_acmp_hpmd_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 25, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_acmp_hpmd_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->acmp_hpmd_b;
}

static inline void sys_ll_set_ana_reg43_en_anacomp_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 26, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_en_anacomp_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->en_anacomp_b;
}

static inline void sys_ll_set_ana_reg43_vbg_sel_lpdac_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 27, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_vbg_sel_lpdac_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->vbg_sel_lpdac_b;
}

static inline void sys_ll_set_ana_reg43_en_lpdac_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 28, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_en_lpdac_b(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->en_lpdac_b;
}

static inline void sys_ll_set_ana_reg43_comp_b_vosel(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 29, 0x3, v);
}

static inline uint32_t sys_ll_get_ana_reg43_comp_b_vosel(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->comp_b_vosel;
}

static inline void sys_ll_set_ana_reg43_nc_31_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6b << 2)), 31, 0x1, v);
}

static inline uint32_t sys_ll_get_ana_reg43_nc_31_31(void) {
	sys_ana_reg43_t *r = (sys_ana_reg43_t*)(SOC_SYS_REG_BASE + (0x6b << 2));
	return r->nc_31_31;
}

//reg ana_reg44:

static inline void sys_ll_set_ana_reg44_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x6c << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg44_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x6c << 2));
}

static inline void sys_ll_set_ana_reg44_din_lpdac_a(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6c << 2)), 0, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg44_din_lpdac_a(void) {
	sys_ana_reg44_t *r = (sys_ana_reg44_t*)(SOC_SYS_REG_BASE + (0x6c << 2));
	return r->din_lpdac_a;
}

static inline void sys_ll_set_ana_reg44_din_lpdac_b(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6c << 2)), 8, 0xff, v);
}

static inline uint32_t sys_ll_get_ana_reg44_din_lpdac_b(void) {
	sys_ana_reg44_t *r = (sys_ana_reg44_t*)(SOC_SYS_REG_BASE + (0x6c << 2));
	return r->din_lpdac_b;
}

static inline void sys_ll_set_ana_reg44_nc_16_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6c << 2)), 16, 0xffff, v);
}

static inline uint32_t sys_ll_get_ana_reg44_nc_16_31(void) {
	sys_ana_reg44_t *r = (sys_ana_reg44_t*)(SOC_SYS_REG_BASE + (0x6c << 2));
	return r->nc_16_31;
}

//reg ana_reg45:

static inline void sys_ll_set_ana_reg45_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x6d << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg45_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x6d << 2));
}

static inline void sys_ll_set_ana_reg45_nc_0_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6d << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg45_nc_0_31(void) {
	sys_ana_reg45_t *r = (sys_ana_reg45_t*)(SOC_SYS_REG_BASE + (0x6d << 2));
	return r->nc_0_31;
}

//reg ana_reg46:

static inline void sys_ll_set_ana_reg46_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x6e << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg46_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x6e << 2));
}

static inline void sys_ll_set_ana_reg46_nc_0_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6e << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg46_nc_0_31(void) {
	sys_ana_reg46_t *r = (sys_ana_reg46_t*)(SOC_SYS_REG_BASE + (0x6e << 2));
	return r->nc_0_31;
}

//reg ana_reg47:

static inline void sys_ll_set_ana_reg47_value(uint32_t v) {
	sys_ll_set_analog_reg_value((SOC_SYS_REG_BASE + (0x6f << 2)), v);
}

static inline uint32_t sys_ll_get_ana_reg47_value(void) {
	return sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (0x6f << 2));
}

static inline void sys_ll_set_ana_reg47_nc_0_31(uint32_t v) {
	sys_set_ana_reg_bit((SOC_SYS_REG_BASE + (0x6f << 2)), 0, 0xffffffff, v);
}

static inline uint32_t sys_ll_get_ana_reg47_nc_0_31(void) {
	sys_ana_reg47_t *r = (sys_ana_reg47_t*)(SOC_SYS_REG_BASE + (0x6f << 2));
	return r->nc_0_31;
}
#ifdef __cplusplus
}
#endif
