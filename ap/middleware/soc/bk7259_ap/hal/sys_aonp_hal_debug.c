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
#include "sys_aonp_hw.h"
#include "sys_aonp_hal.h"

typedef void (*sys_aonp_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	sys_aonp_dump_fn_t fn;
} sys_aonp_reg_fn_map_t;

static void sys_aonp_dump_reg0(void)
{
	SOC_LOGI("reg0: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x0 << 2)));
}

static void sys_aonp_dump_reg1(void)
{
	SOC_LOGI("reg1: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x1 << 2)));
}

static void sys_aonp_dump_reg2(void)
{
	sys_aonp_reg2_t *r = (sys_aonp_reg2_t *)(SOC_SYS_AONP_REG_BASE + (0x2 << 2));

	SOC_LOGI("reg2: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	boot_mode: %8x\r\n", r->boot_mode);
	SOC_LOGI("	clkg_bps: %8x\r\n", r->clkg_bps);
	SOC_LOGI("	reserved_bit_2_3: %8x\r\n", r->reserved_bit_2_3);
	SOC_LOGI("	rf_switch_manual_en: %8x\r\n", r->rf_switch_manual_en);
	SOC_LOGI("	rf_source: %8x\r\n", r->rf_source);
	SOC_LOGI("	jtag_core_sel: %8x\r\n", r->jtag_core_sel);
	SOC_LOGI("	reserved_bit_8_8: %8x\r\n", r->reserved_bit_8_8);
	SOC_LOGI("	flash_sel: %8x\r\n", r->flash_sel);
	SOC_LOGI("	fem_bps_txen: %8x\r\n", r->fem_bps_txen);
	SOC_LOGI("	gpio_flash_sys_enable: %8x\r\n", r->gpio_flash_sys_enable);
	SOC_LOGI("	boot_mode_norst: %8x\r\n", r->boot_mode_norst);
	SOC_LOGI("	reserved_bit_13_31: %8x\r\n", r->reserved_bit_13_31);
}

static void sys_aonp_dump_reg3(void)
{
	sys_aonp_reg3_t *r = (sys_aonp_reg3_t *)(SOC_SYS_AONP_REG_BASE + (0x3 << 2));

	SOC_LOGI("reg3: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3 << 2)));
	SOC_LOGI("	core0_halted: %8x\r\n", r->core0_halted);
	SOC_LOGI("	core1_halted: %8x\r\n", r->core1_halted);
	SOC_LOGI("	reserved_bit_2_3: %8x\r\n", r->reserved_bit_2_3);
	SOC_LOGI("	cpu0_sw_reset: %8x\r\n", r->cpu0_sw_reset);
	SOC_LOGI("	cpu1_sw_reset: %8x\r\n", r->cpu1_sw_reset);
	SOC_LOGI("	reserved_bit_6_7: %8x\r\n", r->reserved_bit_6_7);
	SOC_LOGI("	cpu0_pwr_dw_state: %8x\r\n", r->cpu0_pwr_dw_state);
	SOC_LOGI("	cpu1_pwr_dw_state: %8x\r\n", r->cpu1_pwr_dw_state);
	SOC_LOGI("	reserved_bit_10_11: %8x\r\n", r->reserved_bit_10_11);
	SOC_LOGI("	cpu0_exist: %8x\r\n", r->cpu0_exist);
	SOC_LOGI("	cpu1_exist: %8x\r\n", r->cpu1_exist);
	SOC_LOGI("	cpu2_exist: %8x\r\n", r->cpu2_exist);
	SOC_LOGI("	cpu3_exist: %8x\r\n", r->cpu3_exist);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void sys_aonp_dump_reg4(void)
{
	sys_aonp_reg4_t *r = (sys_aonp_reg4_t *)(SOC_SYS_AONP_REG_BASE + (0x4 << 2));

	SOC_LOGI("reg4: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	cpu0_sw_rst: %8x\r\n", r->cpu0_sw_rst);
	SOC_LOGI("	cpu0_pwr_dw: %8x\r\n", r->cpu0_pwr_dw);
	SOC_LOGI("	cpu_int_mask: %8x\r\n", r->cpu_int_mask);
	SOC_LOGI("	cpu0_halt: %8x\r\n", r->cpu0_halt);
	SOC_LOGI("	reserved_bit_4_4: %8x\r\n", r->reserved_bit_4_4);
	SOC_LOGI("	cpu0_rxevt_sel: %8x\r\n", r->cpu0_rxevt_sel);
	SOC_LOGI("	reserved_7_7: %8x\r\n", r->reserved_7_7);
	SOC_LOGI("	cpu0_offset: %8x\r\n", r->cpu0_offset);
}

static void sys_aonp_dump_reg5(void)
{
	sys_aonp_reg5_t *r = (sys_aonp_reg5_t *)(SOC_SYS_AONP_REG_BASE + (0x5 << 2));

	SOC_LOGI("reg5: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	cpu1_sw_rst: %8x\r\n", r->cpu1_sw_rst);
	SOC_LOGI("	cpu1_pwr_dw: %8x\r\n", r->cpu1_pwr_dw);
	SOC_LOGI("	reserved_2_2: %8x\r\n", r->reserved_2_2);
	SOC_LOGI("	cpu1_halt: %8x\r\n", r->cpu1_halt);
	SOC_LOGI("	reserved_bit_4_4: %8x\r\n", r->reserved_bit_4_4);
	SOC_LOGI("	cpu1_rxevt_sel: %8x\r\n", r->cpu1_rxevt_sel);
	SOC_LOGI("	reserved_7_7: %8x\r\n", r->reserved_7_7);
	SOC_LOGI("	cpu1_offset: %8x\r\n", r->cpu1_offset);
}

static void sys_aonp_dump_rsv_6_7(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGI("rsv_6_7: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + ((0x6 + idx) << 2)));
	}
}

static void sys_aonp_dump_reg8(void)
{
	sys_aonp_reg8_t *r = (sys_aonp_reg8_t *)(SOC_SYS_AONP_REG_BASE + (0x8 << 2));

	SOC_LOGI("reg8: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x8 << 2)));
	SOC_LOGI("	cksel_core: %8x\r\n", r->cksel_core);
	SOC_LOGI("	ckdiv_core: %8x\r\n", r->ckdiv_core);
	SOC_LOGI("	cksel_flash: %8x\r\n", r->cksel_flash);
	SOC_LOGI("	ckdiv_flash: %8x\r\n", r->ckdiv_flash);
	SOC_LOGI("	cksel_auxs: %8x\r\n", r->cksel_auxs);
	SOC_LOGI("	ckdiv_auxs: %8x\r\n", r->ckdiv_auxs);
	SOC_LOGI("	ckdiv_26mo: %8x\r\n", r->ckdiv_26mo);
	SOC_LOGI("	reserved_22_23: %8x\r\n", r->reserved_22_23);
	SOC_LOGI("	phase_cfg_960m: %8x\r\n", r->phase_cfg_960m);
}

static void sys_aonp_dump_reg9(void)
{
	sys_aonp_reg9_t *r = (sys_aonp_reg9_t *)(SOC_SYS_AONP_REG_BASE + (0x9 << 2));

	SOC_LOGI("reg9: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x9 << 2)));
	SOC_LOGI("	cksel_i2c0: %8x\r\n", r->cksel_i2c0);
	SOC_LOGI("	cksel_i2c3: %8x\r\n", r->cksel_i2c3);
	SOC_LOGI("	cksel_uart0: %8x\r\n", r->cksel_uart0);
	SOC_LOGI("	cksel_uart1: %8x\r\n", r->cksel_uart1);
	SOC_LOGI("	cksel_uart2: %8x\r\n", r->cksel_uart2);
	SOC_LOGI("	cksel_uart3: %8x\r\n", r->cksel_uart3);
	SOC_LOGI("	cksel_uart4: %8x\r\n", r->cksel_uart4);
	SOC_LOGI("	cksel_spi0: %8x\r\n", r->cksel_spi0);
	SOC_LOGI("	cksel_spi1: %8x\r\n", r->cksel_spi1);
	SOC_LOGI("	cksel_spi2: %8x\r\n", r->cksel_spi2);
	SOC_LOGI("	cksel_spi3: %8x\r\n", r->cksel_spi3);
	SOC_LOGI("	cksel_i2s0: %8x\r\n", r->cksel_i2s0);
	SOC_LOGI("	ckdiv_i2s0: %8x\r\n", r->ckdiv_i2s0);
	SOC_LOGI("	cksel_i2s1: %8x\r\n", r->cksel_i2s1);
	SOC_LOGI("	ckdiv_i2s1: %8x\r\n", r->ckdiv_i2s1);
	SOC_LOGI("	cksel_i2s2: %8x\r\n", r->cksel_i2s2);
	SOC_LOGI("	ckdiv_i2s2: %8x\r\n", r->ckdiv_i2s2);
	SOC_LOGI("	cksel_i2s3: %8x\r\n", r->cksel_i2s3);
	SOC_LOGI("	ckdiv_i2s3: %8x\r\n", r->ckdiv_i2s3);
	SOC_LOGI("	cksel_i2s4: %8x\r\n", r->cksel_i2s4);
	SOC_LOGI("	ckdiv_i2s4: %8x\r\n", r->ckdiv_i2s4);
	SOC_LOGI("	cksel_sadc: %8x\r\n", r->cksel_sadc);
	SOC_LOGI("	cksel_i3c: %8x\r\n", r->cksel_i3c);
	SOC_LOGI("	cksel_tim0: %8x\r\n", r->cksel_tim0);
	SOC_LOGI("	cksel_tim1: %8x\r\n", r->cksel_tim1);
	SOC_LOGI("	cksel_tim2: %8x\r\n", r->cksel_tim2);
	SOC_LOGI("	cksel_tim3: %8x\r\n", r->cksel_tim3);
}

static void sys_aonp_dump_rega(void)
{
	sys_aonp_rega_t *r = (sys_aonp_rega_t *)(SOC_SYS_AONP_REG_BASE + (0xa << 2));

	SOC_LOGI("rega: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0xa << 2)));
	SOC_LOGI("	cksel_pwm0: %8x\r\n", r->cksel_pwm0);
	SOC_LOGI("	cksel_can0: %8x\r\n", r->cksel_can0);
	SOC_LOGI("	cksel_can1: %8x\r\n", r->cksel_can1);
	SOC_LOGI("	cksel_scr0: %8x\r\n", r->cksel_scr0);
	SOC_LOGI("	cksel_audio: %8x\r\n", r->cksel_audio);
	SOC_LOGI("	ckdiv_audio: %8x\r\n", r->ckdiv_audio);
	SOC_LOGI("	cksel_audif0: %8x\r\n", r->cksel_audif0);
	SOC_LOGI("	ckdiv_audif0: %8x\r\n", r->ckdiv_audif0);
	SOC_LOGI("	cksel_audif1: %8x\r\n", r->cksel_audif1);
	SOC_LOGI("	ckdiv_audif1: %8x\r\n", r->ckdiv_audif1);
	SOC_LOGI("	ckdiv_i2so: %8x\r\n", r->ckdiv_i2so);
	SOC_LOGI("	cksel_auxs_enet: %8x\r\n", r->cksel_auxs_enet);
	SOC_LOGI("	ckdiv_auxs_enet: %8x\r\n", r->ckdiv_auxs_enet);
	SOC_LOGI("	cksel_trace: %8x\r\n", r->cksel_trace);
	SOC_LOGI("	ckdiv_trace: %8x\r\n", r->ckdiv_trace);
	SOC_LOGI("	reserved_28_31: %8x\r\n", r->reserved_28_31);
}

static void sys_aonp_dump_regb(void)
{
	sys_aonp_regb_t *r = (sys_aonp_regb_t *)(SOC_SYS_AONP_REG_BASE + (0xb << 2));

	SOC_LOGI("regb: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0xb << 2)));
	SOC_LOGI("	anaspi_freq: %8x\r\n", r->anaspi_freq);
	SOC_LOGI("	reserved_bit_6_31: %8x\r\n", r->reserved_bit_6_31);
}

static void sys_aonp_dump_regc(void)
{
	sys_aonp_regc_t *r = (sys_aonp_regc_t *)(SOC_SYS_AONP_REG_BASE + (0xc << 2));

	SOC_LOGI("regc: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0xc << 2)));
	SOC_LOGI("	tim0_cken: %8x\r\n", r->tim0_cken);
	SOC_LOGI("	tim1_cken: %8x\r\n", r->tim1_cken);
	SOC_LOGI("	tim2_cken: %8x\r\n", r->tim2_cken);
	SOC_LOGI("	tim3_cken: %8x\r\n", r->tim3_cken);
	SOC_LOGI("	uart0_cken: %8x\r\n", r->uart0_cken);
	SOC_LOGI("	uart1_cken: %8x\r\n", r->uart1_cken);
	SOC_LOGI("	uart2_cken: %8x\r\n", r->uart2_cken);
	SOC_LOGI("	uart3_cken: %8x\r\n", r->uart3_cken);
	SOC_LOGI("	uart4_cken: %8x\r\n", r->uart4_cken);
	SOC_LOGI("	spi0_cken: %8x\r\n", r->spi0_cken);
	SOC_LOGI("	spi1_cken: %8x\r\n", r->spi1_cken);
	SOC_LOGI("	spi2_cken: %8x\r\n", r->spi2_cken);
	SOC_LOGI("	spi3_cken: %8x\r\n", r->spi3_cken);
	SOC_LOGI("	sadc_cken: %8x\r\n", r->sadc_cken);
	SOC_LOGI("	pwm0_cken: %8x\r\n", r->pwm0_cken);
	SOC_LOGI("	otp_cken: %8x\r\n", r->otp_cken);
	SOC_LOGI("	i3c_cken: %8x\r\n", r->i3c_cken);
	SOC_LOGI("	i2s0_cken: %8x\r\n", r->i2s0_cken);
	SOC_LOGI("	i2s1_cken: %8x\r\n", r->i2s1_cken);
	SOC_LOGI("	i2s2_cken: %8x\r\n", r->i2s2_cken);
	SOC_LOGI("	i2s3_cken: %8x\r\n", r->i2s3_cken);
	SOC_LOGI("	i2s4_cken: %8x\r\n", r->i2s4_cken);
	SOC_LOGI("	i2c0_cken: %8x\r\n", r->i2c0_cken);
	SOC_LOGI("	i2c3_cken: %8x\r\n", r->i2c3_cken);
	SOC_LOGI("	irda0_cken: %8x\r\n", r->irda0_cken);
	SOC_LOGI("	irda1_cken: %8x\r\n", r->irda1_cken);
	SOC_LOGI("	irda2_cken: %8x\r\n", r->irda2_cken);
	SOC_LOGI("	irda3_cken: %8x\r\n", r->irda3_cken);
	SOC_LOGI("	can0_cken: %8x\r\n", r->can0_cken);
	SOC_LOGI("	can1_cken: %8x\r\n", r->can1_cken);
	SOC_LOGI("	lin0_cken: %8x\r\n", r->lin0_cken);
	SOC_LOGI("	scr0_cken: %8x\r\n", r->scr0_cken);
}

static void sys_aonp_dump_regd(void)
{
	sys_aonp_regd_t *r = (sys_aonp_regd_t *)(SOC_SYS_AONP_REG_BASE + (0xd << 2));

	SOC_LOGI("regd: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0xd << 2)));
	SOC_LOGI("	audio_cken: %8x\r\n", r->audio_cken);
	SOC_LOGI("	audif0_cken: %8x\r\n", r->audif0_cken);
	SOC_LOGI("	audif1_cken: %8x\r\n", r->audif1_cken);
	SOC_LOGI("	i2so_cken: %8x\r\n", r->i2so_cken);
	SOC_LOGI("	cec_cken: %8x\r\n", r->cec_cken);
	SOC_LOGI("	xdac0_cken: %8x\r\n", r->xdac0_cken);
	SOC_LOGI("	xdac1_cken: %8x\r\n", r->xdac1_cken);
	SOC_LOGI("	auxs_cken: %8x\r\n", r->auxs_cken);
	SOC_LOGI("	auxs_enet_cken: %8x\r\n", r->auxs_enet_cken);
	SOC_LOGI("	sig_26ms_cken: %8x\r\n", r->sig_26ms_cken);
	SOC_LOGI("	sig_32ks_cken: %8x\r\n", r->sig_32ks_cken);
	SOC_LOGI("	sig_26mo_cken: %8x\r\n", r->sig_26mo_cken);
	SOC_LOGI("	sig_240m_cken: %8x\r\n", r->sig_240m_cken);
	SOC_LOGI("	sig_320m_cken: %8x\r\n", r->sig_320m_cken);
	SOC_LOGI("	sig_480m_cken: %8x\r\n", r->sig_480m_cken);
	SOC_LOGI("	sig_160m_cken: %8x\r\n", r->sig_160m_cken);
	SOC_LOGI("	sig_120m_cken: %8x\r\n", r->sig_120m_cken);
	SOC_LOGI("	trace_cken: %8x\r\n", r->trace_cken);
	SOC_LOGI("	reserved_18_21: %8x\r\n", r->reserved_18_21);
	SOC_LOGI("	wlss_cken: %8x\r\n", r->wlss_cken);
	SOC_LOGI("	btdm_cken: %8x\r\n", r->btdm_cken);
	SOC_LOGI("	xver_cken: %8x\r\n", r->xver_cken);
	SOC_LOGI("	mac_cken: %8x\r\n", r->mac_cken);
	SOC_LOGI("	phy_cken: %8x\r\n", r->phy_cken);
	SOC_LOGI("	thread_cken: %8x\r\n", r->thread_cken);
	SOC_LOGI("	bk24_cken: %8x\r\n", r->bk24_cken);
	SOC_LOGI("	rf_cken: %8x\r\n", r->rf_cken);
	SOC_LOGI("	ofdm_cken: %8x\r\n", r->ofdm_cken);
	SOC_LOGI("	reserved_31_31: %8x\r\n", r->reserved_31_31);
}

static void sys_aonp_dump_rsv_e_e(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_e_e: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + ((0xe + idx) << 2)));
	}
}

static void sys_aonp_dump_regf(void)
{
	sys_aonp_regf_t *r = (sys_aonp_regf_t *)(SOC_SYS_AONP_REG_BASE + (0xf << 2));

	SOC_LOGI("regf: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0xf << 2)));
	SOC_LOGI("	reserved_0_0: %8x\r\n", r->reserved_0_0);
	SOC_LOGI("	reserved_1_1: %8x\r\n", r->reserved_1_1);
	SOC_LOGI("	reserved_2_2: %8x\r\n", r->reserved_2_2);
	SOC_LOGI("	macp_mem_ret: %8x\r\n", r->macp_mem_ret);
	SOC_LOGI("	phyp_mem_ret: %8x\r\n", r->phyp_mem_ret);
	SOC_LOGI("	thread_mem_ret: %8x\r\n", r->thread_mem_ret);
	SOC_LOGI("	encp_mem_ret: %8x\r\n", r->encp_mem_ret);
	SOC_LOGI("	can0_mem_ret: %8x\r\n", r->can0_mem_ret);
	SOC_LOGI("	can1_mem_ret: %8x\r\n", r->can1_mem_ret);
	SOC_LOGI("	irda0_mem_ret: %8x\r\n", r->irda0_mem_ret);
	SOC_LOGI("	irda1_mem_ret: %8x\r\n", r->irda1_mem_ret);
	SOC_LOGI("	dma0_mem_ret: %8x\r\n", r->dma0_mem_ret);
	SOC_LOGI("	spi1_mem_ret: %8x\r\n", r->spi1_mem_ret);
	SOC_LOGI("	spi2_mem_ret: %8x\r\n", r->spi2_mem_ret);
	SOC_LOGI("	uart1_mem_ret: %8x\r\n", r->uart1_mem_ret);
	SOC_LOGI("	uart2_mem_ret: %8x\r\n", r->uart2_mem_ret);
	SOC_LOGI("	uart3_mem_ret: %8x\r\n", r->uart3_mem_ret);
	SOC_LOGI("	uart0_mem_ret: %8x\r\n", r->uart0_mem_ret);
	SOC_LOGI("	spi0_mem_ret: %8x\r\n", r->spi0_mem_ret);
	SOC_LOGI("	flsh_mem_ret: %8x\r\n", r->flsh_mem_ret);
	SOC_LOGI("	audp_mem_ret: %8x\r\n", r->audp_mem_ret);
	SOC_LOGI("	i3c_mem_ret: %8x\r\n", r->i3c_mem_ret);
	SOC_LOGI("	xvr_mem_ret: %8x\r\n", r->xvr_mem_ret);
	SOC_LOGI("	reserved_23_23: %8x\r\n", r->reserved_23_23);
	SOC_LOGI("	bk24_mem_ret: %8x\r\n", r->bk24_mem_ret);
	SOC_LOGI("	irda2_mem_ret: %8x\r\n", r->irda2_mem_ret);
	SOC_LOGI("	irda3_mem_ret: %8x\r\n", r->irda3_mem_ret);
	SOC_LOGI("	spi3_mem_ret: %8x\r\n", r->spi3_mem_ret);
	SOC_LOGI("	uart4_mem_ret: %8x\r\n", r->uart4_mem_ret);
	SOC_LOGI("	cpu0_mem_ret: %8x\r\n", r->cpu0_mem_ret);
	SOC_LOGI("	cpu1_mem_ret: %8x\r\n", r->cpu1_mem_ret);
	SOC_LOGI("	coresight_mem_ret: %8x\r\n", r->coresight_mem_ret);
}

static void sys_aonp_dump_reg10(void)
{
	sys_aonp_reg10_t *r = (sys_aonp_reg10_t *)(SOC_SYS_AONP_REG_BASE + (0x10 << 2));

	SOC_LOGI("reg10: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x10 << 2)));
	SOC_LOGI("	pwd_cpu1: %8x\r\n", r->pwd_cpu1);
	SOC_LOGI("	pwd_vehp: %8x\r\n", r->pwd_vehp);
	SOC_LOGI("	pwd_wrls: %8x\r\n", r->pwd_wrls);
	SOC_LOGI("	rom_pgen: %8x\r\n", r->rom_pgen);
	SOC_LOGI("	cpu1_isolate_state: %8x\r\n", r->cpu1_isolate_state);
	SOC_LOGI("	vehp_isolate_state: %8x\r\n", r->vehp_isolate_state);
	SOC_LOGI("	wrls_isolate_state: %8x\r\n", r->wrls_isolate_state);
	SOC_LOGI("	reserved_7_30: %8x\r\n", r->reserved_7_30);
	SOC_LOGI("	busmatrix_busy: %8x\r\n", r->busmatrix_busy);
}

static void sys_aonp_dump_reg11(void)
{
	sys_aonp_reg11_t *r = (sys_aonp_reg11_t *)(SOC_SYS_AONP_REG_BASE + (0x11 << 2));

	SOC_LOGI("reg11: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x11 << 2)));
	SOC_LOGI("	sleep_en_global: %8x\r\n", r->sleep_en_global);
	SOC_LOGI("	sleep_en_need_flash_idle: %8x\r\n", r->sleep_en_need_flash_idle);
	SOC_LOGI("	sleep_bus_idle_bypass: %8x\r\n", r->sleep_bus_idle_bypass);
	SOC_LOGI("	sleep_en_need_cpu0_wfi: %8x\r\n", r->sleep_en_need_cpu0_wfi);
	SOC_LOGI("	sleep_en_need_cpu1_wfi: %8x\r\n", r->sleep_en_need_cpu1_wfi);
	SOC_LOGI("	reserved_bit_5_11: %8x\r\n", r->reserved_bit_5_11);
	SOC_LOGI("	reserved_12_15: %8x\r\n", r->reserved_12_15);
	SOC_LOGI("	cpu0_ticktimer_32k_enable: %8x\r\n", r->cpu0_ticktimer_32k_enable);
	SOC_LOGI("	cpu1_ticktimer_32k_enable: %8x\r\n", r->cpu1_ticktimer_32k_enable);
	SOC_LOGI("	reserved_bit_18_19: %8x\r\n", r->reserved_bit_18_19);
	SOC_LOGI("	reserved_20_24: %8x\r\n", r->reserved_20_24);
	SOC_LOGI("	bts_soft_wakeup_req: %8x\r\n", r->bts_soft_wakeup_req);
	SOC_LOGI("	rom_rd_disable: %8x\r\n", r->rom_rd_disable);
	SOC_LOGI("	otp_rd_disable: %8x\r\n", r->otp_rd_disable);
	SOC_LOGI("	share_mem_clkgating_disable: %8x\r\n", r->share_mem_clkgating_disable);
	SOC_LOGI("	reserved_29_31: %8x\r\n", r->reserved_29_31);
}

static void sys_aonp_dump_rsv_12_13(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGI("rsv_12_13: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + ((0x12 + idx) << 2)));
	}
}

static void sys_aonp_dump_reg14(void)
{
	SOC_LOGI("reg14: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x14 << 2)));
}

static void sys_aonp_dump_reg15(void)
{
	SOC_LOGI("reg15: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x15 << 2)));
}

static void sys_aonp_dump_reg16(void)
{
	SOC_LOGI("reg16: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x16 << 2)));
}

static void sys_aonp_dump_reg17(void)
{
	SOC_LOGI("reg17: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x17 << 2)));
}

static void sys_aonp_dump_reg18(void)
{
	SOC_LOGI("reg18: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x18 << 2)));
}

static void sys_aonp_dump_reg19(void)
{
	SOC_LOGI("reg19: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x19 << 2)));
}

static void sys_aonp_dump_reg1a(void)
{
	SOC_LOGI("reg1a: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x1a << 2)));
}

static void sys_aonp_dump_reg1b(void)
{
	SOC_LOGI("reg1b: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x1b << 2)));
}

static void sys_aonp_dump_reg1c(void)
{
	sys_aonp_reg1c_t *r = (sys_aonp_reg1c_t *)(SOC_SYS_AONP_REG_BASE + (0x1c << 2));

	SOC_LOGI("reg1c: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x1c << 2)));
	SOC_LOGI("	m55sub_inten: %8x\r\n", r->m55sub_inten);
	SOC_LOGI("	m55sub_wakeup: %8x\r\n", r->m55sub_wakeup);
}

static void sys_aonp_dump_rsv_1d_1d(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_1d_1d: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + ((0x1d + idx) << 2)));
	}
}

static void sys_aonp_dump_reg1e(void)
{
	sys_aonp_reg1e_t *r = (sys_aonp_reg1e_t *)(SOC_SYS_AONP_REG_BASE + (0x1e << 2));

	SOC_LOGI("reg1e: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x1e << 2)));
	SOC_LOGI("	spsh_cfg: %8x\r\n", r->spsh_cfg);
	SOC_LOGI("	spbh_cfg: %8x\r\n", r->spbh_cfg);
	SOC_LOGI("	reserved_bit_21_23: %8x\r\n", r->reserved_bit_21_23);
	SOC_LOGI("	set_key: %8x\r\n", r->set_key);
}

static void sys_aonp_dump_reg1f(void)
{
	sys_aonp_reg1f_t *r = (sys_aonp_reg1f_t *)(SOC_SYS_AONP_REG_BASE + (0x1f << 2));

	SOC_LOGI("reg1f: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x1f << 2)));
	SOC_LOGI("	stph_cfg: %8x\r\n", r->stph_cfg);
	SOC_LOGI("	reserved_bit_12_23: %8x\r\n", r->reserved_bit_12_23);
	SOC_LOGI("	set_key: %8x\r\n", r->set_key);
}

static void sys_aonp_dump_reg20(void)
{
	SOC_LOGI("reg20: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x20 << 2)));
}

static void sys_aonp_dump_reg21(void)
{
	SOC_LOGI("reg21: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x21 << 2)));
}

static void sys_aonp_dump_reg22(void)
{
	SOC_LOGI("reg22: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x22 << 2)));
}

static void sys_aonp_dump_reg23(void)
{
	SOC_LOGI("reg23: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x23 << 2)));
}

static void sys_aonp_dump_reg24(void)
{
	SOC_LOGI("reg24: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x24 << 2)));
}

static void sys_aonp_dump_reg25(void)
{
	SOC_LOGI("reg25: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x25 << 2)));
}

static void sys_aonp_dump_reg26(void)
{
	SOC_LOGI("reg26: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x26 << 2)));
}

static void sys_aonp_dump_reg27(void)
{
	SOC_LOGI("reg27: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x27 << 2)));
}

static void sys_aonp_dump_reg28(void)
{
	SOC_LOGI("reg28: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x28 << 2)));
}

static void sys_aonp_dump_rsv_29_29(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_29_29: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + ((0x29 + idx) << 2)));
	}
}

static void sys_aonp_dump_reg2a(void)
{
	SOC_LOGI("reg2a: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x2a << 2)));
}

static void sys_aonp_dump_reg2b(void)
{
	SOC_LOGI("reg2b: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x2b << 2)));
}

static void sys_aonp_dump_reg2c(void)
{
	sys_aonp_reg2c_t *r = (sys_aonp_reg2c_t *)(SOC_SYS_AONP_REG_BASE + (0x2c << 2));

	SOC_LOGI("reg2c: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x2c << 2)));
	SOC_LOGI("	cache_clean_mode: %8x\r\n", r->cache_clean_mode);
	SOC_LOGI("	cpu0_icache_clean_mode: %8x\r\n", r->cpu0_icache_clean_mode);
	SOC_LOGI("	cpu0_icache_clean_tag_sel: %8x\r\n", r->cpu0_icache_clean_tag_sel);
	SOC_LOGI("	l2_cache_clean_mode: %8x\r\n", r->l2_cache_clean_mode);
	SOC_LOGI("	l2_cache_clean_tag_sel: %8x\r\n", r->l2_cache_clean_tag_sel);
	SOC_LOGI("	reserved_6_23: %8x\r\n", r->reserved_6_23);
	SOC_LOGI("	set_key: %8x\r\n", r->set_key);
}

static void sys_aonp_dump_rsv_2d_2d(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_2d_2d: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + ((0x2d + idx) << 2)));
	}
}

static void sys_aonp_dump_reg2e(void)
{
	sys_aonp_reg2e_t *r = (sys_aonp_reg2e_t *)(SOC_SYS_AONP_REG_BASE + (0x2e << 2));

	SOC_LOGI("reg2e: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x2e << 2)));
	SOC_LOGI("	spsl_cfg: %8x\r\n", r->spsl_cfg);
	SOC_LOGI("	spbl_cfg: %8x\r\n", r->spbl_cfg);
	SOC_LOGI("	reserved_bit_21_23: %8x\r\n", r->reserved_bit_21_23);
	SOC_LOGI("	set_key: %8x\r\n", r->set_key);
}

static void sys_aonp_dump_reg2f(void)
{
	sys_aonp_reg2f_t *r = (sys_aonp_reg2f_t *)(SOC_SYS_AONP_REG_BASE + (0x2f << 2));

	SOC_LOGI("reg2f: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x2f << 2)));
	SOC_LOGI("	stpl_cfg: %8x\r\n", r->stpl_cfg);
	SOC_LOGI("	reserved_bit_12_23: %8x\r\n", r->reserved_bit_12_23);
	SOC_LOGI("	set_key: %8x\r\n", r->set_key);
}

static void sys_aonp_dump_reg30(void)
{
	SOC_LOGI("reg30: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x30 << 2)));
}

static void sys_aonp_dump_reg31(void)
{
	SOC_LOGI("reg31: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x31 << 2)));
}

static void sys_aonp_dump_reg32(void)
{
	sys_aonp_reg32_t *r = (sys_aonp_reg32_t *)(SOC_SYS_AONP_REG_BASE + (0x32 << 2));

	SOC_LOGI("reg32: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x32 << 2)));
	SOC_LOGI("	gpio_input_status2: %8x\r\n", r->gpio_input_status2);
	SOC_LOGI("	reserved_bit_8_30: %8x\r\n", r->reserved_bit_8_30);
	SOC_LOGI("	gpio_input_status_en: %8x\r\n", r->gpio_input_status_en);
}

static void sys_aonp_dump_reg33(void)
{
	sys_aonp_reg33_t *r = (sys_aonp_reg33_t *)(SOC_SYS_AONP_REG_BASE + (0x33 << 2));

	SOC_LOGI("reg33: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x33 << 2)));
	SOC_LOGI("	acomp0_pwm0_sample_en: %8x\r\n", r->acomp0_pwm0_sample_en);
	SOC_LOGI("	acomp1_pwm0_sample_en: %8x\r\n", r->acomp1_pwm0_sample_en);
	SOC_LOGI("	reserved_2_15: %8x\r\n", r->reserved_2_15);
	SOC_LOGI("	l2_dis_pwr_down_maint: %8x\r\n", r->l2_dis_pwr_down_maint);
	SOC_LOGI("	l2_apb_violation_resp: %8x\r\n", r->l2_apb_violation_resp);
	SOC_LOGI("	cpu0_dbgen_l2_rst_dis: %8x\r\n", r->cpu0_dbgen_l2_rst_dis);
	SOC_LOGI("	cpu1_dbgen_l2_rst_dis: %8x\r\n", r->cpu1_dbgen_l2_rst_dis);
	SOC_LOGI("	reserved_20_23: %8x\r\n", r->reserved_20_23);
	SOC_LOGI("	cpu0_wfe_src: %8x\r\n", r->cpu0_wfe_src);
	SOC_LOGI("	cpu0_wfe_pulse: %8x\r\n", r->cpu0_wfe_pulse);
	SOC_LOGI("	cpu1_wfe_src: %8x\r\n", r->cpu1_wfe_src);
	SOC_LOGI("	cpu1_wfe_pulse: %8x\r\n", r->cpu1_wfe_pulse);
	SOC_LOGI("	cpu0_sleeping_state: %8x\r\n", r->cpu0_sleeping_state);
	SOC_LOGI("	cpu0_deepsleep_state: %8x\r\n", r->cpu0_deepsleep_state);
	SOC_LOGI("	cpu1_sleeping_state: %8x\r\n", r->cpu1_sleeping_state);
	SOC_LOGI("	cpu1_deepsleep_state: %8x\r\n", r->cpu1_deepsleep_state);
}

static void sys_aonp_dump_reg34(void)
{
	SOC_LOGI("reg34: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x34 << 2)));
}

static void sys_aonp_dump_reg35(void)
{
	sys_aonp_reg35_t *r = (sys_aonp_reg35_t *)(SOC_SYS_AONP_REG_BASE + (0x35 << 2));

	SOC_LOGI("reg35: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x35 << 2)));
	SOC_LOGI("	cpu0_faultstat_h: %8x\r\n", r->cpu0_faultstat_h);
	SOC_LOGI("	reserved_11_31: %8x\r\n", r->reserved_11_31);
}

static void sys_aonp_dump_reg36(void)
{
	SOC_LOGI("reg36: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x36 << 2)));
}

static void sys_aonp_dump_reg37(void)
{
	sys_aonp_reg37_t *r = (sys_aonp_reg37_t *)(SOC_SYS_AONP_REG_BASE + (0x37 << 2));

	SOC_LOGI("reg37: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x37 << 2)));
	SOC_LOGI("	cpu0_intnum: %8x\r\n", r->cpu0_intnum);
	SOC_LOGI("	cpu0_currpri: %8x\r\n", r->cpu0_currpri);
	SOC_LOGI("	cpu0_currns: %8x\r\n", r->cpu0_currns);
	SOC_LOGI("	cpu0_halted: %8x\r\n", r->cpu0_halted);
	SOC_LOGI("	cpu0_nc_hready: %8x\r\n", r->cpu0_nc_hready);
	SOC_LOGI("	cpu_cache_m_hready: %8x\r\n", r->cpu_cache_m_hready);
	SOC_LOGI("	cpu0_resetn: %8x\r\n", r->cpu0_resetn);
	SOC_LOGI("	reserved_22_31: %8x\r\n", r->reserved_22_31);
}

static void sys_aonp_dump_reg38(void)
{
	SOC_LOGI("reg38: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x38 << 2)));
}

static void sys_aonp_dump_reg39(void)
{
	SOC_LOGI("reg39: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x39 << 2)));
}

static void sys_aonp_dump_reg3a(void)
{
	SOC_LOGI("reg3a: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3a << 2)));
}

static void sys_aonp_dump_reg3b(void)
{
	sys_aonp_reg3b_t *r = (sys_aonp_reg3b_t *)(SOC_SYS_AONP_REG_BASE + (0x3b << 2));

	SOC_LOGI("reg3b: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3b << 2)));
	SOC_LOGI("	coresight_chn_gate_en: %8x\r\n", r->coresight_chn_gate_en);
	SOC_LOGI("	coresight_tpmaxdatasize: %8x\r\n", r->coresight_tpmaxdatasize);
	SOC_LOGI("	coresight_valid: %8x\r\n", r->coresight_valid);
	SOC_LOGI("	reserved_30_30: %8x\r\n", r->reserved_30_30);
	SOC_LOGI("	anaregb_stat: %8x\r\n", r->anaregb_stat);
}

static void sys_aonp_dump_reg3c(void)
{
	SOC_LOGI("reg3c: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3c << 2)));
}

static void sys_aonp_dump_reg3d(void)
{
	sys_aonp_reg3d_t *r = (sys_aonp_reg3d_t *)(SOC_SYS_AONP_REG_BASE + (0x3d << 2));

	SOC_LOGI("reg3d: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3d << 2)));
	SOC_LOGI("	cpu1_faultstat_h: %8x\r\n", r->cpu1_faultstat_h);
	SOC_LOGI("	reserved_11_31: %8x\r\n", r->reserved_11_31);
}

static void sys_aonp_dump_reg3e(void)
{
	SOC_LOGI("reg3e: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3e << 2)));
}

static void sys_aonp_dump_reg3f(void)
{
	sys_aonp_reg3f_t *r = (sys_aonp_reg3f_t *)(SOC_SYS_AONP_REG_BASE + (0x3f << 2));

	SOC_LOGI("reg3f: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x3f << 2)));
	SOC_LOGI("	cpu1_intnum: %8x\r\n", r->cpu1_intnum);
	SOC_LOGI("	cpu1_currpri: %8x\r\n", r->cpu1_currpri);
	SOC_LOGI("	cpu1_currns: %8x\r\n", r->cpu1_currns);
	SOC_LOGI("	cpu1_halted: %8x\r\n", r->cpu1_halted);
	SOC_LOGI("	cpu1_nc_hready: %8x\r\n", r->cpu1_nc_hready);
	SOC_LOGI("	reserved_20_20: %8x\r\n", r->reserved_20_20);
	SOC_LOGI("	cpu1_resetn: %8x\r\n", r->cpu1_resetn);
	SOC_LOGI("	reserved_22_31: %8x\r\n", r->reserved_22_31);
}

static void sys_aonp_dump_ana_reg0(void)
{
	sys_aonp_ana_reg0_t *r = (sys_aonp_ana_reg0_t *)(SOC_SYS_AONP_REG_BASE + (0x40 << 2));

	SOC_LOGI("ana_reg0: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x40 << 2)));
	SOC_LOGI("	dpll_tsten: %8x\r\n", r->dpll_tsten);
	SOC_LOGI("	cp: %8x\r\n", r->cp);
	SOC_LOGI("	spideten: %8x\r\n", r->spideten);
	SOC_LOGI("	hvref: %8x\r\n", r->hvref);
	SOC_LOGI("	lvref: %8x\r\n", r->lvref);
	SOC_LOGI("	rzctrl26m: %8x\r\n", r->rzctrl26m);
	SOC_LOGI("	looprzctrl: %8x\r\n", r->looprzctrl);
	SOC_LOGI("	rpc: %8x\r\n", r->rpc);
	SOC_LOGI("	openloop_en: %8x\r\n", r->openloop_en);
	SOC_LOGI("	cksel: %8x\r\n", r->cksel);
	SOC_LOGI("	spitrig: %8x\r\n", r->spitrig);
	SOC_LOGI("	band: %8x\r\n", r->band);
	SOC_LOGI("	band_1: %8x\r\n", r->band_1);
	SOC_LOGI("	band_2: %8x\r\n", r->band_2);
	SOC_LOGI("	bandmanual: %8x\r\n", r->bandmanual);
	SOC_LOGI("	dsptrig: %8x\r\n", r->dsptrig);
	SOC_LOGI("	lpen_dpll: %8x\r\n", r->lpen_dpll);
	SOC_LOGI("	nc_28_29: %8x\r\n", r->nc_28_29);
	SOC_LOGI("	bp_caldone: %8x\r\n", r->bp_caldone);
	SOC_LOGI("	vctrl_dpllldo: %8x\r\n", r->vctrl_dpllldo);
}

static void sys_aonp_dump_ana_reg1(void)
{
	sys_aonp_ana_reg1_t *r = (sys_aonp_ana_reg1_t *)(SOC_SYS_AONP_REG_BASE + (0x41 << 2));

	SOC_LOGI("ana_reg1: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x41 << 2)));
	SOC_LOGI("	vcooffset: %8x\r\n", r->vcooffset);
	SOC_LOGI("	selpol: %8x\r\n", r->selpol);
	SOC_LOGI("	dlysel: %8x\r\n", r->dlysel);
	SOC_LOGI("	edgesel_nck: %8x\r\n", r->edgesel_nck);
	SOC_LOGI("	nload_dlyen: %8x\r\n", r->nload_dlyen);
	SOC_LOGI("	cp: %8x\r\n", r->cp);
	SOC_LOGI("	spideten: %8x\r\n", r->spideten);
	SOC_LOGI("	cben: %8x\r\n", r->cben);
	SOC_LOGI("	hvref: %8x\r\n", r->hvref);
	SOC_LOGI("	lvref: %8x\r\n", r->lvref);
	SOC_LOGI("	rzctrl26m: %8x\r\n", r->rzctrl26m);
	SOC_LOGI("	lpfrz: %8x\r\n", r->lpfrz);
	SOC_LOGI("	rpc: %8x\r\n", r->rpc);
	SOC_LOGI("	dpll_tsten: %8x\r\n", r->dpll_tsten);
	SOC_LOGI("	kctrl: %8x\r\n", r->kctrl);
	SOC_LOGI("	vsel_ldo: %8x\r\n", r->vsel_ldo);
	SOC_LOGI("	div_sw: %8x\r\n", r->div_sw);
	SOC_LOGI("	bp_caldone: %8x\r\n", r->bp_caldone);
	SOC_LOGI("	ck2xen: %8x\r\n", r->ck2xen);
	SOC_LOGI("	int_mod: %8x\r\n", r->int_mod);
}

static void sys_aonp_dump_ana_reg2(void)
{
	sys_aonp_ana_reg2_t *r = (sys_aonp_ana_reg2_t *)(SOC_SYS_AONP_REG_BASE + (0x42 << 2));

	SOC_LOGI("ana_reg2: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x42 << 2)));
	SOC_LOGI("	xtalh_ctune: %8x\r\n", r->xtalh_ctune);
	SOC_LOGI("	force_26mpll: %8x\r\n", r->force_26mpll);
	SOC_LOGI("	nc_9_11: %8x\r\n", r->nc_9_11);
	SOC_LOGI("	gadc_sd1v: %8x\r\n", r->gadc_sd1v);
	SOC_LOGI("	gadc_bscalsaw: %8x\r\n", r->gadc_bscalsaw);
	SOC_LOGI("	gadc_vncalsaw: %8x\r\n", r->gadc_vncalsaw);
	SOC_LOGI("	gadc_vpcalsaw: %8x\r\n", r->gadc_vpcalsaw);
	SOC_LOGI("	nc_22_22: %8x\r\n", r->nc_22_22);
	SOC_LOGI("	gadc_vbg_sel: %8x\r\n", r->gadc_vbg_sel);
	SOC_LOGI("	gadc_clk_rlten: %8x\r\n", r->gadc_clk_rlten);
	SOC_LOGI("	gadc_calintsaw_en: %8x\r\n", r->gadc_calintsaw_en);
	SOC_LOGI("	gadc_clk_sel: %8x\r\n", r->gadc_clk_sel);
	SOC_LOGI("	gadc_clk_inv: %8x\r\n", r->gadc_clk_inv);
	SOC_LOGI("	gadc_calcap_ch: %8x\r\n", r->gadc_calcap_ch);
	SOC_LOGI("	gadc_inbuf_en: %8x\r\n", r->gadc_inbuf_en);
	SOC_LOGI("	gadc_en_spi: %8x\r\n", r->gadc_en_spi);
}

static void sys_aonp_dump_ana_reg3(void)
{
	sys_aonp_ana_reg3_t *r = (sys_aonp_ana_reg3_t *)(SOC_SYS_AONP_REG_BASE + (0x43 << 2));

	SOC_LOGI("ana_reg3: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x43 << 2)));
	SOC_LOGI("	nc_0_6: %8x\r\n", r->nc_0_6);
	SOC_LOGI("	anabuf_sel_rx: %8x\r\n", r->anabuf_sel_rx);
	SOC_LOGI("	hpssren: %8x\r\n", r->hpssren);
	SOC_LOGI("	ck_sel: %8x\r\n", r->ck_sel);
	SOC_LOGI("	anabuf_sel_tx: %8x\r\n", r->anabuf_sel_tx);
	SOC_LOGI("	pwd_xtalldo: %8x\r\n", r->pwd_xtalldo);
	SOC_LOGI("	iamp: %8x\r\n", r->iamp);
	SOC_LOGI("	vddren: %8x\r\n", r->vddren);
	SOC_LOGI("	xamp: %8x\r\n", r->xamp);
	SOC_LOGI("	vosel: %8x\r\n", r->vosel);
	SOC_LOGI("	en_xtalh_sleep: %8x\r\n", r->en_xtalh_sleep);
	SOC_LOGI("	xtal40_en: %8x\r\n", r->xtal40_en);
	SOC_LOGI("	bufictrl: %8x\r\n", r->bufictrl);
	SOC_LOGI("	ibias_ctrl: %8x\r\n", r->ibias_ctrl);
	SOC_LOGI("	icore_ctrl: %8x\r\n", r->icore_ctrl);
}

static void sys_aonp_dump_ana_reg4(void)
{
	sys_aonp_ana_reg4_t *r = (sys_aonp_ana_reg4_t *)(SOC_SYS_AONP_REG_BASE + (0x44 << 2));

	SOC_LOGI("ana_reg4: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x44 << 2)));
	SOC_LOGI("	cktst_sel: %8x\r\n", r->cktst_sel);
	SOC_LOGI("	ck_tst_en: %8x\r\n", r->ck_tst_en);
	SOC_LOGI("	vusbsel: %8x\r\n", r->vusbsel);
	SOC_LOGI("	nc_5_16: %8x\r\n", r->nc_5_16);
	SOC_LOGI("	gadc_inbuff_isel: %8x\r\n", r->gadc_inbuff_isel);
	SOC_LOGI("	gadc_biasamp_isel: %8x\r\n", r->gadc_biasamp_isel);
	SOC_LOGI("	gadc_comp_isel: %8x\r\n", r->gadc_comp_isel);
	SOC_LOGI("	gadc_preamp_isel: %8x\r\n", r->gadc_preamp_isel);
	SOC_LOGI("	gadc_bufamp_isel: %8x\r\n", r->gadc_bufamp_isel);
}

static void sys_aonp_dump_ana_reg5(void)
{
	sys_aonp_ana_reg5_t *r = (sys_aonp_ana_reg5_t *)(SOC_SYS_AONP_REG_BASE + (0x45 << 2));

	SOC_LOGI("ana_reg5: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x45 << 2)));
	SOC_LOGI("	en_vout: %8x\r\n", r->en_vout);
	SOC_LOGI("	en_xtall: %8x\r\n", r->en_xtall);
	SOC_LOGI("	en_dco: %8x\r\n", r->en_dco);
	SOC_LOGI("	nc_3_3: %8x\r\n", r->nc_3_3);
	SOC_LOGI("	en_temp: %8x\r\n", r->en_temp);
	SOC_LOGI("	en_dpll: %8x\r\n", r->en_dpll);
	SOC_LOGI("	en_cb: %8x\r\n", r->en_cb);
	SOC_LOGI("	gpio_latch: %8x\r\n", r->gpio_latch);
	SOC_LOGI("	nc_8_11: %8x\r\n", r->nc_8_11);
	SOC_LOGI("	rosc_disable: %8x\r\n", r->rosc_disable);
	SOC_LOGI("	pwdaudpll: %8x\r\n", r->pwdaudpll);
	SOC_LOGI("	pwd_rosc_spi: %8x\r\n", r->pwd_rosc_spi);
	SOC_LOGI("	nc_15_15: %8x\r\n", r->nc_15_15);
	SOC_LOGI("	itune_xtall: %8x\r\n", r->itune_xtall);
	SOC_LOGI("	xtall_ten: %8x\r\n", r->xtall_ten);
	SOC_LOGI("	rosc_tsten: %8x\r\n", r->rosc_tsten);
	SOC_LOGI("	bcal_start: %8x\r\n", r->bcal_start);
	SOC_LOGI("	bcal_en: %8x\r\n", r->bcal_en);
	SOC_LOGI("	bcal_sel: %8x\r\n", r->bcal_sel);
	SOC_LOGI("	vbias: %8x\r\n", r->vbias);
}

static void sys_aonp_dump_ana_reg6(void)
{
	sys_aonp_ana_reg6_t *r = (sys_aonp_ana_reg6_t *)(SOC_SYS_AONP_REG_BASE + (0x46 << 2));

	SOC_LOGI("ana_reg6: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x46 << 2)));
	SOC_LOGI("	calib_interval: %8x\r\n", r->calib_interval);
	SOC_LOGI("	modify_interval: %8x\r\n", r->modify_interval);
	SOC_LOGI("	xtal_wakeup_time: %8x\r\n", r->xtal_wakeup_time);
	SOC_LOGI("	spi_trig: %8x\r\n", r->spi_trig);
	SOC_LOGI("	modifi_auto: %8x\r\n", r->modifi_auto);
	SOC_LOGI("	calib_auto: %8x\r\n", r->calib_auto);
	SOC_LOGI("	cal_mode: %8x\r\n", r->cal_mode);
	SOC_LOGI("	manu_ena: %8x\r\n", r->manu_ena);
	SOC_LOGI("	manu_cin: %8x\r\n", r->manu_cin);
}

static void sys_aonp_dump_ana_reg7(void)
{
	sys_aonp_ana_reg7_t *r = (sys_aonp_ana_reg7_t *)(SOC_SYS_AONP_REG_BASE + (0x47 << 2));

	SOC_LOGI("ana_reg7: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x47 << 2)));
	SOC_LOGI("	nsyn: %8x\r\n", r->nsyn);
	SOC_LOGI("	bandmanual: %8x\r\n", r->bandmanual);
	SOC_LOGI("	ckref_loop_sel: %8x\r\n", r->ckref_loop_sel);
	SOC_LOGI("	ioffs: %8x\r\n", r->ioffs);
	SOC_LOGI("	reset_nload: %8x\r\n", r->reset_nload);
	SOC_LOGI("	closeloop_en: %8x\r\n", r->closeloop_en);
	SOC_LOGI("	modecal: %8x\r\n", r->modecal);
	SOC_LOGI("	spi_rstn: %8x\r\n", r->spi_rstn);
	SOC_LOGI("	osccal_trig: %8x\r\n", r->osccal_trig);
	SOC_LOGI("	manual: %8x\r\n", r->manual);
	SOC_LOGI("	diff: %8x\r\n", r->diff);
	SOC_LOGI("	ictrlmanual: %8x\r\n", r->ictrlmanual);
	SOC_LOGI("	cnti: %8x\r\n", r->cnti);
}

static void sys_aonp_dump_ana_reg8(void)
{
	sys_aonp_ana_reg8_t *r = (sys_aonp_ana_reg8_t *)(SOC_SYS_AONP_REG_BASE + (0x48 << 2));

	SOC_LOGI("ana_reg8: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x48 << 2)));
	SOC_LOGI("	reserved_bit_0_30: %8x\r\n", r->reserved_bit_0_30);
	SOC_LOGI("	n: %8x\r\n", r->n);
}

static void sys_aonp_dump_ana_reg9(void)
{
	sys_aonp_ana_reg9_t *r = (sys_aonp_ana_reg9_t *)(SOC_SYS_AONP_REG_BASE + (0x49 << 2));

	SOC_LOGI("ana_reg9: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x49 << 2)));
	SOC_LOGI("	clk_sel: %8x\r\n", r->clk_sel);
	SOC_LOGI("	coreldo_hp: %8x\r\n", r->coreldo_hp);
	SOC_LOGI("	dldohp: %8x\r\n", r->dldohp);
	SOC_LOGI("	t_vanaldosel: %8x\r\n", r->t_vanaldosel);
	SOC_LOGI("	r_vanaldosel: %8x\r\n", r->r_vanaldosel);
	SOC_LOGI("	en_trsw: %8x\r\n", r->en_trsw);
	SOC_LOGI("	aldohp: %8x\r\n", r->aldohp);
	SOC_LOGI("	anacurlim: %8x\r\n", r->anacurlim);
	SOC_LOGI("	hsldo_hp: %8x\r\n", r->hsldo_hp);
	SOC_LOGI("	pwd_hsldo: %8x\r\n", r->pwd_hsldo);
	SOC_LOGI("	enfast_hsldo: %8x\r\n", r->enfast_hsldo);
	SOC_LOGI("	nc_15_15: %8x\r\n", r->nc_15_15);
	SOC_LOGI("	valoldosel: %8x\r\n", r->valoldosel);
	SOC_LOGI("	alopowsel: %8x\r\n", r->alopowsel);
	SOC_LOGI("	en_fast_aloldo: %8x\r\n", r->en_fast_aloldo);
	SOC_LOGI("	aloldohp: %8x\r\n", r->aloldohp);
	SOC_LOGI("	bgcal: %8x\r\n", r->bgcal);
	SOC_LOGI("	vbgcalmode: %8x\r\n", r->vbgcalmode);
	SOC_LOGI("	vbgcalstart: %8x\r\n", r->vbgcalstart);
	SOC_LOGI("	pwd_bgcal: %8x\r\n", r->pwd_bgcal);
	SOC_LOGI("	spi_envbg: %8x\r\n", r->spi_envbg);
}

static void sys_aonp_dump_ana_reg10(void)
{
	sys_aonp_ana_reg10_t *r = (sys_aonp_ana_reg10_t *)(SOC_SYS_AONP_REG_BASE + (0x4a << 2));

	SOC_LOGI("ana_reg10: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4a << 2)));
	SOC_LOGI("	azcd_manual: %8x\r\n", r->azcd_manual);
	SOC_LOGI("	azcdrefs: %8x\r\n", r->azcdrefs);
	SOC_LOGI("	reserved_bit_8_8: %8x\r\n", r->reserved_bit_8_8);
	SOC_LOGI("	spi_latchb: %8x\r\n", r->spi_latchb);
	SOC_LOGI("	digcurlim: %8x\r\n", r->digcurlim);
	SOC_LOGI("	rtc_wkrstn: %8x\r\n", r->rtc_wkrstn);
	SOC_LOGI("	rst_wks: %8x\r\n", r->rst_wks);
	SOC_LOGI("	d_veasel1v: %8x\r\n", r->d_veasel1v);
	SOC_LOGI("	ensfsdd: %8x\r\n", r->ensfsdd);
	SOC_LOGI("	vcorehsel: %8x\r\n", r->vcorehsel);
	SOC_LOGI("	vcorelsel: %8x\r\n", r->vcorelsel);
	SOC_LOGI("	vlden: %8x\r\n", r->vlden);
	SOC_LOGI("	en_fast_coreldo: %8x\r\n", r->en_fast_coreldo);
	SOC_LOGI("	pwdcoreldo: %8x\r\n", r->pwdcoreldo);
	SOC_LOGI("	vdighsel: %8x\r\n", r->vdighsel);
	SOC_LOGI("	vdigsel: %8x\r\n", r->vdigsel);
	SOC_LOGI("	vdd12lden: %8x\r\n", r->vdd12lden);
}

static void sys_aonp_dump_ana_reg11(void)
{
	sys_aonp_ana_reg11_t *r = (sys_aonp_ana_reg11_t *)(SOC_SYS_AONP_REG_BASE + (0x4b << 2));

	SOC_LOGI("ana_reg11: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4b << 2)));
	SOC_LOGI("	aldo_czsel: %8x\r\n", r->aldo_czsel);
	SOC_LOGI("	zldo_rzsel: %8x\r\n", r->zldo_rzsel);
	SOC_LOGI("	azcdswvs: %8x\r\n", r->azcdswvs);
	SOC_LOGI("	aenzcddy: %8x\r\n", r->aenzcddy);
	SOC_LOGI("	aenzcdmsel: %8x\r\n", r->aenzcdmsel);
	SOC_LOGI("	aenzcdcalib: %8x\r\n", r->aenzcdcalib);
	SOC_LOGI("	en_corepsw: %8x\r\n", r->en_corepsw);
	SOC_LOGI("	en_alopsw: %8x\r\n", r->en_alopsw);
	SOC_LOGI("	nc_13_14: %8x\r\n", r->nc_13_14);
	SOC_LOGI("	spi_timerwken: %8x\r\n", r->spi_timerwken);
	SOC_LOGI("	spi_byp32pwd: %8x\r\n", r->spi_byp32pwd);
	SOC_LOGI("	sd: %8x\r\n", r->sd);
	SOC_LOGI("	nc_18_18: %8x\r\n", r->nc_18_18);
	SOC_LOGI("	gpio_wkrst1v: %8x\r\n", r->gpio_wkrst1v);
	SOC_LOGI("	ckfs: %8x\r\n", r->ckfs);
	SOC_LOGI("	ckintsel: %8x\r\n", r->ckintsel);
	SOC_LOGI("	osccaltrig: %8x\r\n", r->osccaltrig);
	SOC_LOGI("	mroscsel: %8x\r\n", r->mroscsel);
	SOC_LOGI("	mrosci_cal: %8x\r\n", r->mrosci_cal);
	SOC_LOGI("	mrosccap_cal: %8x\r\n", r->mrosccap_cal);
}

static void sys_aonp_dump_ana_reg12(void)
{
	sys_aonp_ana_reg12_t *r = (sys_aonp_ana_reg12_t *)(SOC_SYS_AONP_REG_BASE + (0x4c << 2));

	SOC_LOGI("ana_reg12: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4c << 2)));
	SOC_LOGI("	sfsr: %8x\r\n", r->sfsr);
	SOC_LOGI("	ensfsaa: %8x\r\n", r->ensfsaa);
	SOC_LOGI("	apfms: %8x\r\n", r->apfms);
	SOC_LOGI("	atmpo_sel: %8x\r\n", r->atmpo_sel);
	SOC_LOGI("	ampoen: %8x\r\n", r->ampoen);
	SOC_LOGI("	enpowa: %8x\r\n", r->enpowa);
	SOC_LOGI("	avea_sel: %8x\r\n", r->avea_sel);
	SOC_LOGI("	aforcepfm: %8x\r\n", r->aforcepfm);
	SOC_LOGI("	acls: %8x\r\n", r->acls);
	SOC_LOGI("	aswrsten: %8x\r\n", r->aswrsten);
	SOC_LOGI("	aripc: %8x\r\n", r->aripc);
	SOC_LOGI("	arampc: %8x\r\n", r->arampc);
	SOC_LOGI("	arampcen: %8x\r\n", r->arampcen);
	SOC_LOGI("	aenburst: %8x\r\n", r->aenburst);
	SOC_LOGI("	apfmen: %8x\r\n", r->apfmen);
	SOC_LOGI("	aldosel: %8x\r\n", r->aldosel);
}

static void sys_aonp_dump_ana_reg13(void)
{
	sys_aonp_ana_reg13_t *r = (sys_aonp_ana_reg13_t *)(SOC_SYS_AONP_REG_BASE + (0x4d << 2));

	SOC_LOGI("ana_reg13: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4d << 2)));
	SOC_LOGI("	buckd_softst: %8x\r\n", r->buckd_softst);
	SOC_LOGI("	denzcdcalib: %8x\r\n", r->denzcdcalib);
	SOC_LOGI("	nc_5_6: %8x\r\n", r->nc_5_6);
	SOC_LOGI("	vddgpio_sel: %8x\r\n", r->vddgpio_sel);
	SOC_LOGI("	dpfms: %8x\r\n", r->dpfms);
	SOC_LOGI("	dtmpo_sel: %8x\r\n", r->dtmpo_sel);
	SOC_LOGI("	dmpoen: %8x\r\n", r->dmpoen);
	SOC_LOGI("	dforcepfm: %8x\r\n", r->dforcepfm);
	SOC_LOGI("	dcls: %8x\r\n", r->dcls);
	SOC_LOGI("	dswrsten: %8x\r\n", r->dswrsten);
	SOC_LOGI("	dripc: %8x\r\n", r->dripc);
	SOC_LOGI("	drampc: %8x\r\n", r->drampc);
	SOC_LOGI("	drampcen: %8x\r\n", r->drampcen);
	SOC_LOGI("	denburst: %8x\r\n", r->denburst);
	SOC_LOGI("	dpfmen: %8x\r\n", r->dpfmen);
	SOC_LOGI("	dldosel: %8x\r\n", r->dldosel);
}

static void sys_aonp_dump_ana_reg14(void)
{
	sys_aonp_ana_reg14_t *r = (sys_aonp_ana_reg14_t *)(SOC_SYS_AONP_REG_BASE + (0x4e << 2));

	SOC_LOGI("ana_reg14: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4e << 2)));
	SOC_LOGI("	pwdovp1v: %8x\r\n", r->pwdovp1v);
	SOC_LOGI("	asoft_stc: %8x\r\n", r->asoft_stc);
	SOC_LOGI("	dldo_czsel: %8x\r\n", r->dldo_czsel);
	SOC_LOGI("	dldo_rzsel: %8x\r\n", r->dldo_rzsel);
	SOC_LOGI("	en_usbvcc18: %8x\r\n", r->en_usbvcc18);
	SOC_LOGI("	en_usbvcc3v: %8x\r\n", r->en_usbvcc3v);
	SOC_LOGI("	vtrxspisel: %8x\r\n", r->vtrxspisel);
	SOC_LOGI("	denzcddy: %8x\r\n", r->denzcddy);
	SOC_LOGI("	dzcd_swvs: %8x\r\n", r->dzcd_swvs);
	SOC_LOGI("	dzcd_refs: %8x\r\n", r->dzcd_refs);
	SOC_LOGI("	dzcd_manu: %8x\r\n", r->dzcd_manu);
	SOC_LOGI("	azcdmsel: %8x\r\n", r->azcdmsel);
	SOC_LOGI("	psldo_swb: %8x\r\n", r->psldo_swb);
	SOC_LOGI("	vpsramsel: %8x\r\n", r->vpsramsel);
	SOC_LOGI("	enpsram: %8x\r\n", r->enpsram);
}

static void sys_aonp_dump_ana_reg15(void)
{
	SOC_LOGI("ana_reg15: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x4f << 2)));
}

static void sys_aonp_dump_ana_reg16(void)
{
	sys_aonp_ana_reg16_t *r = (sys_aonp_ana_reg16_t *)(SOC_SYS_AONP_REG_BASE + (0x50 << 2));

	SOC_LOGI("ana_reg16: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x50 << 2)));
	SOC_LOGI("	timer_set: %8x\r\n", r->timer_set);
	SOC_LOGI("	rtc_set: %8x\r\n", r->rtc_set);
	SOC_LOGI("	nc_8_10: %8x\r\n", r->nc_8_10);
	SOC_LOGI("	vcorehssel: %8x\r\n", r->vcorehssel);
	SOC_LOGI("	vbuckhssel: %8x\r\n", r->vbuckhssel);
	SOC_LOGI("	hsenfast: %8x\r\n", r->hsenfast);
	SOC_LOGI("	enhspw: %8x\r\n", r->enhspw);
	SOC_LOGI("	buckhs_soft_stc: %8x\r\n", r->buckhs_soft_stc);
	SOC_LOGI("	hs_veasel: %8x\r\n", r->hs_veasel);
	SOC_LOGI("	hszcd_manual: %8x\r\n", r->hszcd_manual);
}

static void sys_aonp_dump_ana_reg17(void)
{
	SOC_LOGI("ana_reg17: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x51 << 2)));
}

static void sys_aonp_dump_ana_reg18(void)
{
	SOC_LOGI("ana_reg18: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x52 << 2)));
}

static void sys_aonp_dump_ana_reg19(void)
{
	sys_aonp_ana_reg19_t *r = (sys_aonp_ana_reg19_t *)(SOC_SYS_AONP_REG_BASE + (0x53 << 2));

	SOC_LOGI("ana_reg19: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x53 << 2)));
	SOC_LOGI("	hsenzcddy: %8x\r\n", r->hsenzcddy);
	SOC_LOGI("	hsenzcdcalib: %8x\r\n", r->hsenzcdcalib);
	SOC_LOGI("	hszcdswvs: %8x\r\n", r->hszcdswvs);
	SOC_LOGI("	hszcdrefs: %8x\r\n", r->hszcdrefs);
	SOC_LOGI("	hszcdmsel: %8x\r\n", r->hszcdmsel);
	SOC_LOGI("	hspfms: %8x\r\n", r->hspfms);
	SOC_LOGI("	hstmpo_sel: %8x\r\n", r->hstmpo_sel);
	SOC_LOGI("	hsmpoen: %8x\r\n", r->hsmpoen);
	SOC_LOGI("	hsforcepfm: %8x\r\n", r->hsforcepfm);
	SOC_LOGI("	hscls: %8x\r\n", r->hscls);
	SOC_LOGI("	hsswrsten: %8x\r\n", r->hsswrsten);
	SOC_LOGI("	hsripc: %8x\r\n", r->hsripc);
	SOC_LOGI("	hsrampc: %8x\r\n", r->hsrampc);
	SOC_LOGI("	hsrampcen: %8x\r\n", r->hsrampcen);
	SOC_LOGI("	hsenburst: %8x\r\n", r->hsenburst);
	SOC_LOGI("	hspfmen: %8x\r\n", r->hspfmen);
}

static void sys_aonp_dump_ana_reg20(void)
{
	sys_aonp_ana_reg20_t *r = (sys_aonp_ana_reg20_t *)(SOC_SYS_AONP_REG_BASE + (0x54 << 2));

	SOC_LOGI("ana_reg20: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x54 << 2)));
	SOC_LOGI("	iselaud: %8x\r\n", r->iselaud);
	SOC_LOGI("	audck_rlcen: %8x\r\n", r->audck_rlcen);
	SOC_LOGI("	lchckinven: %8x\r\n", r->lchckinven);
	SOC_LOGI("	enaudbias: %8x\r\n", r->enaudbias);
	SOC_LOGI("	enadcbias: %8x\r\n", r->enadcbias);
	SOC_LOGI("	enmicbias: %8x\r\n", r->enmicbias);
	SOC_LOGI("	adcckinven: %8x\r\n", r->adcckinven);
	SOC_LOGI("	spi: %8x\r\n", r->spi);
	SOC_LOGI("	adctsten: %8x\r\n", r->adctsten);
	SOC_LOGI("	micbias_trm: %8x\r\n", r->micbias_trm);
	SOC_LOGI("	micbias_voc: %8x\r\n", r->micbias_voc);
	SOC_LOGI("	vrefsel: %8x\r\n", r->vrefsel);
	SOC_LOGI("	capsw: %8x\r\n", r->capsw);
	SOC_LOGI("	adcref_sel: %8x\r\n", r->adcref_sel);
	SOC_LOGI("	adcvcmsel: %8x\r\n", r->adcvcmsel);
	SOC_LOGI("	spi_1: %8x\r\n", r->spi_1);
	SOC_LOGI("	audadjref: %8x\r\n", r->audadjref);
}

static void sys_aonp_dump_ana_reg21(void)
{
	sys_aonp_ana_reg21_t *r = (sys_aonp_ana_reg21_t *)(SOC_SYS_AONP_REG_BASE + (0x55 << 2));

	SOC_LOGI("ana_reg21: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x55 << 2)));
	SOC_LOGI("	isel_mic1: %8x\r\n", r->isel_mic1);
	SOC_LOGI("	micirsel1_mic1: %8x\r\n", r->micirsel1_mic1);
	SOC_LOGI("	vcmsel_mic1: %8x\r\n", r->vcmsel_mic1);
	SOC_LOGI("	enfsr_mic1: %8x\r\n", r->enfsr_mic1);
	SOC_LOGI("	enopoclip_mic1: %8x\r\n", r->enopoclip_mic1);
	SOC_LOGI("	da2aden_mic1: %8x\r\n", r->da2aden_mic1);
	SOC_LOGI("	imatch_mic1: %8x\r\n", r->imatch_mic1);
	SOC_LOGI("	imatch_en_mic1: %8x\r\n", r->imatch_en_mic1);
	SOC_LOGI("	dccompen_mic1: %8x\r\n", r->dccompen_mic1);
	SOC_LOGI("	micsingleen_mic1: %8x\r\n", r->micsingleen_mic1);
	SOC_LOGI("	nc_14_14: %8x\r\n", r->nc_14_14);
	SOC_LOGI("	micgain_mic1: %8x\r\n", r->micgain_mic1);
	SOC_LOGI("	nc_19_23: %8x\r\n", r->nc_19_23);
	SOC_LOGI("	dwamode_mic1: %8x\r\n", r->dwamode_mic1);
	SOC_LOGI("	nc_25_27: %8x\r\n", r->nc_25_27);
	SOC_LOGI("	micen_mic1: %8x\r\n", r->micen_mic1);
	SOC_LOGI("	rst_mic1: %8x\r\n", r->rst_mic1);
	SOC_LOGI("	bpdwa1v_mic1: %8x\r\n", r->bpdwa1v_mic1);
	SOC_LOGI("	hcen1stg_mic1: %8x\r\n", r->hcen1stg_mic1);
}

static void sys_aonp_dump_ana_reg22(void)
{
	sys_aonp_ana_reg22_t *r = (sys_aonp_ana_reg22_t *)(SOC_SYS_AONP_REG_BASE + (0x56 << 2));

	SOC_LOGI("ana_reg22: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x56 << 2)));
	SOC_LOGI("	ictrl_dsppll: %8x\r\n", r->ictrl_dsppll);
	SOC_LOGI("	lvref: %8x\r\n", r->lvref);
	SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGI("	nc_4_18: %8x\r\n", r->nc_4_18);
	SOC_LOGI("	mode: %8x\r\n", r->mode);
	SOC_LOGI("	iamsel: %8x\r\n", r->iamsel);
	SOC_LOGI("	hvref: %8x\r\n", r->hvref);
	SOC_LOGI("	lvref_1: %8x\r\n", r->lvref_1);
	SOC_LOGI("	nc_25_31: %8x\r\n", r->nc_25_31);
}

static void sys_aonp_dump_ana_reg23(void)
{
	sys_aonp_ana_reg23_t *r = (sys_aonp_ana_reg23_t *)(SOC_SYS_AONP_REG_BASE + (0x57 << 2));

	SOC_LOGI("ana_reg23: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x57 << 2)));
	SOC_LOGI("	camsel: %8x\r\n", r->camsel);
	SOC_LOGI("	msw: %8x\r\n", r->msw);
	SOC_LOGI("	tstcken_dpll: %8x\r\n", r->tstcken_dpll);
	SOC_LOGI("	osccal_trig: %8x\r\n", r->osccal_trig);
	SOC_LOGI("	cnti: %8x\r\n", r->cnti);
	SOC_LOGI("	nc_21_21: %8x\r\n", r->nc_21_21);
	SOC_LOGI("	spi_rst: %8x\r\n", r->spi_rst);
	SOC_LOGI("	closeloop_en: %8x\r\n", r->closeloop_en);
	SOC_LOGI("	caltime: %8x\r\n", r->caltime);
	SOC_LOGI("	lpfrz: %8x\r\n", r->lpfrz);
	SOC_LOGI("	icp: %8x\r\n", r->icp);
	SOC_LOGI("	cp2ctrl: %8x\r\n", r->cp2ctrl);
}

static void sys_aonp_dump_ana_reg24(void)
{
	SOC_LOGI("ana_reg24: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x58 << 2)));
}

static void sys_aonp_dump_ana_reg25(void)
{
	sys_aonp_ana_reg25_t *r = (sys_aonp_ana_reg25_t *)(SOC_SYS_AONP_REG_BASE + (0x59 << 2));

	SOC_LOGI("ana_reg25: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x59 << 2)));
	SOC_LOGI("	int_mod: %8x\r\n", r->int_mod);
	SOC_LOGI("	nsyn: %8x\r\n", r->nsyn);
	SOC_LOGI("	open_enb: %8x\r\n", r->open_enb);
	SOC_LOGI("	reset: %8x\r\n", r->reset);
	SOC_LOGI("	ioffsetl: %8x\r\n", r->ioffsetl);
	SOC_LOGI("	lpfrz: %8x\r\n", r->lpfrz);
	SOC_LOGI("	vsel: %8x\r\n", r->vsel);
	SOC_LOGI("	vsel_cal: %8x\r\n", r->vsel_cal);
	SOC_LOGI("	pwd_lockdet: %8x\r\n", r->pwd_lockdet);
	SOC_LOGI("	lockdet_bypass: %8x\r\n", r->lockdet_bypass);
	SOC_LOGI("	ckref_loop_sel: %8x\r\n", r->ckref_loop_sel);
	SOC_LOGI("	spi_trigger: %8x\r\n", r->spi_trigger);
	SOC_LOGI("	manual: %8x\r\n", r->manual);
	SOC_LOGI("	test_ckaudio_en: %8x\r\n", r->test_ckaudio_en);
	SOC_LOGI("	ck2xen: %8x\r\n", r->ck2xen);
	SOC_LOGI("	icp: %8x\r\n", r->icp);
	SOC_LOGI("	cktst_sel: %8x\r\n", r->cktst_sel);
	SOC_LOGI("	edgesel_nck: %8x\r\n", r->edgesel_nck);
	SOC_LOGI("	nloaddlyen: %8x\r\n", r->nloaddlyen);
	SOC_LOGI("	bypass_caldone_auto: %8x\r\n", r->bypass_caldone_auto);
	SOC_LOGI("	cal_res_spi: %8x\r\n", r->cal_res_spi);
	SOC_LOGI("	audioen: %8x\r\n", r->audioen);
}

static void sys_aonp_dump_ana_reg26(void)
{
	sys_aonp_ana_reg26_t *r = (sys_aonp_ana_reg26_t *)(SOC_SYS_AONP_REG_BASE + (0x5a << 2));

	SOC_LOGI("ana_reg26: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5a << 2)));
	SOC_LOGI("	n: %8x\r\n", r->n);
	SOC_LOGI("	calres_spien: %8x\r\n", r->calres_spien);
	SOC_LOGI("	calrefen: %8x\r\n", r->calrefen);
}

static void sys_aonp_dump_ana_reg27(void)
{
	sys_aonp_ana_reg27_t *r = (sys_aonp_ana_reg27_t *)(SOC_SYS_AONP_REG_BASE + (0x5b << 2));

	SOC_LOGI("ana_reg27: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5b << 2)));
	SOC_LOGI("	isel_mic2: %8x\r\n", r->isel_mic2);
	SOC_LOGI("	micirsel1_mic2: %8x\r\n", r->micirsel1_mic2);
	SOC_LOGI("	vcmsel_mic2: %8x\r\n", r->vcmsel_mic2);
	SOC_LOGI("	enfsr_mic2: %8x\r\n", r->enfsr_mic2);
	SOC_LOGI("	enopoclip_mic2: %8x\r\n", r->enopoclip_mic2);
	SOC_LOGI("	da2aden_mic2: %8x\r\n", r->da2aden_mic2);
	SOC_LOGI("	imatch_mic2: %8x\r\n", r->imatch_mic2);
	SOC_LOGI("	imatch_en_mic2: %8x\r\n", r->imatch_en_mic2);
	SOC_LOGI("	dccompen_mic2: %8x\r\n", r->dccompen_mic2);
	SOC_LOGI("	micsingleen_mic2: %8x\r\n", r->micsingleen_mic2);
	SOC_LOGI("	nc_14_14: %8x\r\n", r->nc_14_14);
	SOC_LOGI("	micgain_mic2: %8x\r\n", r->micgain_mic2);
	SOC_LOGI("	nc_19_23: %8x\r\n", r->nc_19_23);
	SOC_LOGI("	dwamode_mic2: %8x\r\n", r->dwamode_mic2);
	SOC_LOGI("	nc_25_27: %8x\r\n", r->nc_25_27);
	SOC_LOGI("	micen_mic2: %8x\r\n", r->micen_mic2);
	SOC_LOGI("	rst_mic2: %8x\r\n", r->rst_mic2);
	SOC_LOGI("	bpdwa1v_mic2: %8x\r\n", r->bpdwa1v_mic2);
	SOC_LOGI("	hcen1stg_mic2: %8x\r\n", r->hcen1stg_mic2);
}

static void sys_aonp_dump_ana_reg28(void)
{
	sys_aonp_ana_reg28_t *r = (sys_aonp_ana_reg28_t *)(SOC_SYS_AONP_REG_BASE + (0x5c << 2));

	SOC_LOGI("ana_reg28: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5c << 2)));
	SOC_LOGI("	isel_mic3: %8x\r\n", r->isel_mic3);
	SOC_LOGI("	micirsel1_mic3: %8x\r\n", r->micirsel1_mic3);
	SOC_LOGI("	vcmsel_mic3: %8x\r\n", r->vcmsel_mic3);
	SOC_LOGI("	enfsr_mic3: %8x\r\n", r->enfsr_mic3);
	SOC_LOGI("	enopoclip_mic3: %8x\r\n", r->enopoclip_mic3);
	SOC_LOGI("	da2aden_mic3: %8x\r\n", r->da2aden_mic3);
	SOC_LOGI("	imatch_mic3: %8x\r\n", r->imatch_mic3);
	SOC_LOGI("	imatch_en_mic3: %8x\r\n", r->imatch_en_mic3);
	SOC_LOGI("	dccompen_mic3: %8x\r\n", r->dccompen_mic3);
	SOC_LOGI("	micsingleen_mic3: %8x\r\n", r->micsingleen_mic3);
	SOC_LOGI("	nc_14_14: %8x\r\n", r->nc_14_14);
	SOC_LOGI("	micgain_mic3: %8x\r\n", r->micgain_mic3);
	SOC_LOGI("	nc_19_23: %8x\r\n", r->nc_19_23);
	SOC_LOGI("	dwamode_mic3: %8x\r\n", r->dwamode_mic3);
	SOC_LOGI("	nc_25_27: %8x\r\n", r->nc_25_27);
	SOC_LOGI("	micen_mic3: %8x\r\n", r->micen_mic3);
	SOC_LOGI("	rst_mic3: %8x\r\n", r->rst_mic3);
	SOC_LOGI("	bpdwa1v_mic3: %8x\r\n", r->bpdwa1v_mic3);
	SOC_LOGI("	hcen1stg_mic3: %8x\r\n", r->hcen1stg_mic3);
}

static void sys_aonp_dump_ana_reg29(void)
{
	sys_aonp_ana_reg29_t *r = (sys_aonp_ana_reg29_t *)(SOC_SYS_AONP_REG_BASE + (0x5d << 2));

	SOC_LOGI("ana_reg29: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5d << 2)));
	SOC_LOGI("	hpdac: %8x\r\n", r->hpdac);
	SOC_LOGI("	iselstg: %8x\r\n", r->iselstg);
	SOC_LOGI("	oscdac: %8x\r\n", r->oscdac);
	SOC_LOGI("	ocendac: %8x\r\n", r->ocendac);
	SOC_LOGI("	vseldco: %8x\r\n", r->vseldco);
	SOC_LOGI("	srsel: %8x\r\n", r->srsel);
	SOC_LOGI("	hpoen: %8x\r\n", r->hpoen);
	SOC_LOGI("	lbwen: %8x\r\n", r->lbwen);
	SOC_LOGI("	calsel: %8x\r\n", r->calsel);
	SOC_LOGI("	bp2vldo: %8x\r\n", r->bp2vldo);
	SOC_LOGI("	dcochg: %8x\r\n", r->dcochg);
	SOC_LOGI("	diffen: %8x\r\n", r->diffen);
	SOC_LOGI("	endaccal: %8x\r\n", r->endaccal);
	SOC_LOGI("	rendcoc: %8x\r\n", r->rendcoc);
	SOC_LOGI("	lendcoc: %8x\r\n", r->lendcoc);
	SOC_LOGI("	renvcmd: %8x\r\n", r->renvcmd);
	SOC_LOGI("	lenvcmd: %8x\r\n", r->lenvcmd);
	SOC_LOGI("	dacdrven: %8x\r\n", r->dacdrven);
	SOC_LOGI("	dacren: %8x\r\n", r->dacren);
	SOC_LOGI("	daclen: %8x\r\n", r->daclen);
	SOC_LOGI("	dacg: %8x\r\n", r->dacg);
	SOC_LOGI("	dacmute: %8x\r\n", r->dacmute);
	SOC_LOGI("	dacdwamode_sel: %8x\r\n", r->dacdwamode_sel);
	SOC_LOGI("	ckpsel: %8x\r\n", r->ckpsel);
	SOC_LOGI("	nc_29_31: %8x\r\n", r->nc_29_31);
}

static void sys_aonp_dump_ana_reg30(void)
{
	sys_aonp_ana_reg30_t *r = (sys_aonp_ana_reg30_t *)(SOC_SYS_AONP_REG_BASE + (0x5e << 2));

	SOC_LOGI("ana_reg30: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5e << 2)));
	SOC_LOGI("	lmdcin: %8x\r\n", r->lmdcin);
	SOC_LOGI("	rmdcin: %8x\r\n", r->rmdcin);
	SOC_LOGI("	spirst_ovc: %8x\r\n", r->spirst_ovc);
	SOC_LOGI("	enidacr: %8x\r\n", r->enidacr);
	SOC_LOGI("	enidacl: %8x\r\n", r->enidacl);
	SOC_LOGI("	dac3rdhc0v9: %8x\r\n", r->dac3rdhc0v9);
	SOC_LOGI("	hc2s: %8x\r\n", r->hc2s);
	SOC_LOGI("	sng_fb_en: %8x\r\n", r->sng_fb_en);
	SOC_LOGI("	rfb_ctrl: %8x\r\n", r->rfb_ctrl);
	SOC_LOGI("	enbs: %8x\r\n", r->enbs);
	SOC_LOGI("	calck_sel0v9: %8x\r\n", r->calck_sel0v9);
	SOC_LOGI("	bpdwa0v9: %8x\r\n", r->bpdwa0v9);
	SOC_LOGI("	looprst0v9: %8x\r\n", r->looprst0v9);
	SOC_LOGI("	oct0v9: %8x\r\n", r->oct0v9);
	SOC_LOGI("	sout0v9: %8x\r\n", r->sout0v9);
	SOC_LOGI("	hc0v9: %8x\r\n", r->hc0v9);
}

static void sys_aonp_dump_ana_reg31(void)
{
	SOC_LOGI("ana_reg31: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x5f << 2)));
}

static void sys_aonp_dump_ana_reg32(void)
{
	SOC_LOGI("ana_reg32: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x60 << 2)));
}

static void sys_aonp_dump_ana_reg33(void)
{
	SOC_LOGI("ana_reg33: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x61 << 2)));
}

static void sys_aonp_dump_ana_reg34(void)
{
	SOC_LOGI("ana_reg34: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x62 << 2)));
}

static void sys_aonp_dump_ana_reg35(void)
{
	SOC_LOGI("ana_reg35: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x63 << 2)));
}

static void sys_aonp_dump_ana_reg36(void)
{
	SOC_LOGI("ana_reg36: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x64 << 2)));
}

static void sys_aonp_dump_ana_reg37(void)
{
	SOC_LOGI("ana_reg37: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x65 << 2)));
}

static void sys_aonp_dump_ana_reg38(void)
{
	SOC_LOGI("ana_reg38: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x66 << 2)));
}

static void sys_aonp_dump_ana_reg39(void)
{
	SOC_LOGI("ana_reg39: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x67 << 2)));
}

static void sys_aonp_dump_ana_reg40(void)
{
	SOC_LOGI("ana_reg40: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x68 << 2)));
}

static void sys_aonp_dump_ana_reg41(void)
{
	SOC_LOGI("ana_reg41: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x69 << 2)));
}

static void sys_aonp_dump_ana_reg42(void)
{
	SOC_LOGI("ana_reg42: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x6a << 2)));
}

static void sys_aonp_dump_ana_reg43(void)
{
	SOC_LOGI("ana_reg43: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x6b << 2)));
}

static void sys_aonp_dump_ana_reg44(void)
{
	SOC_LOGI("ana_reg44: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x6c << 2)));
}

static void sys_aonp_dump_ana_reg45(void)
{
	SOC_LOGI("ana_reg45: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x6d << 2)));
}

static void sys_aonp_dump_ana_reg46(void)
{
	SOC_LOGI("ana_reg46: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x6e << 2)));
}

static void sys_aonp_dump_ana_reg47(void)
{
	SOC_LOGI("ana_reg47: %8x\r\n", REG_READ(SOC_SYS_AONP_REG_BASE + (0x6f << 2)));
}

static sys_aonp_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, sys_aonp_dump_reg0},
	{0x1, 0x1, sys_aonp_dump_reg1},
	{0x2, 0x2, sys_aonp_dump_reg2},
	{0x3, 0x3, sys_aonp_dump_reg3},
	{0x4, 0x4, sys_aonp_dump_reg4},
	{0x5, 0x5, sys_aonp_dump_reg5},
	{0x6, 0x8, sys_aonp_dump_rsv_6_7},
	{0x8, 0x8, sys_aonp_dump_reg8},
	{0x9, 0x9, sys_aonp_dump_reg9},
	{0xa, 0xa, sys_aonp_dump_rega},
	{0xb, 0xb, sys_aonp_dump_regb},
	{0xc, 0xc, sys_aonp_dump_regc},
	{0xd, 0xd, sys_aonp_dump_regd},
	{0xe, 0xf, sys_aonp_dump_rsv_e_e},
	{0xf, 0xf, sys_aonp_dump_regf},
	{0x10, 0x10, sys_aonp_dump_reg10},
	{0x11, 0x11, sys_aonp_dump_reg11},
	{0x12, 0x14, sys_aonp_dump_rsv_12_13},
	{0x14, 0x14, sys_aonp_dump_reg14},
	{0x15, 0x15, sys_aonp_dump_reg15},
	{0x16, 0x16, sys_aonp_dump_reg16},
	{0x17, 0x17, sys_aonp_dump_reg17},
	{0x18, 0x18, sys_aonp_dump_reg18},
	{0x19, 0x19, sys_aonp_dump_reg19},
	{0x1a, 0x1a, sys_aonp_dump_reg1a},
	{0x1b, 0x1b, sys_aonp_dump_reg1b},
	{0x1c, 0x1c, sys_aonp_dump_reg1c},
	{0x1d, 0x1e, sys_aonp_dump_rsv_1d_1d},
	{0x1e, 0x1e, sys_aonp_dump_reg1e},
	{0x1f, 0x1f, sys_aonp_dump_reg1f},
	{0x20, 0x20, sys_aonp_dump_reg20},
	{0x21, 0x21, sys_aonp_dump_reg21},
	{0x22, 0x22, sys_aonp_dump_reg22},
	{0x23, 0x23, sys_aonp_dump_reg23},
	{0x24, 0x24, sys_aonp_dump_reg24},
	{0x25, 0x25, sys_aonp_dump_reg25},
	{0x26, 0x26, sys_aonp_dump_reg26},
	{0x27, 0x27, sys_aonp_dump_reg27},
	{0x28, 0x28, sys_aonp_dump_reg28},
	{0x29, 0x2a, sys_aonp_dump_rsv_29_29},
	{0x2a, 0x2a, sys_aonp_dump_reg2a},
	{0x2b, 0x2b, sys_aonp_dump_reg2b},
	{0x2c, 0x2c, sys_aonp_dump_reg2c},
	{0x2d, 0x2e, sys_aonp_dump_rsv_2d_2d},
	{0x2e, 0x2e, sys_aonp_dump_reg2e},
	{0x2f, 0x2f, sys_aonp_dump_reg2f},
	{0x30, 0x30, sys_aonp_dump_reg30},
	{0x31, 0x31, sys_aonp_dump_reg31},
	{0x32, 0x32, sys_aonp_dump_reg32},
	{0x33, 0x33, sys_aonp_dump_reg33},
	{0x34, 0x34, sys_aonp_dump_reg34},
	{0x35, 0x35, sys_aonp_dump_reg35},
	{0x36, 0x36, sys_aonp_dump_reg36},
	{0x37, 0x37, sys_aonp_dump_reg37},
	{0x38, 0x38, sys_aonp_dump_reg38},
	{0x39, 0x39, sys_aonp_dump_reg39},
	{0x3a, 0x3a, sys_aonp_dump_reg3a},
	{0x3b, 0x3b, sys_aonp_dump_reg3b},
	{0x3c, 0x3c, sys_aonp_dump_reg3c},
	{0x3d, 0x3d, sys_aonp_dump_reg3d},
	{0x3e, 0x3e, sys_aonp_dump_reg3e},
	{0x3f, 0x3f, sys_aonp_dump_reg3f},
	{0x40, 0x40, sys_aonp_dump_ana_reg0},
	{0x41, 0x41, sys_aonp_dump_ana_reg1},
	{0x42, 0x42, sys_aonp_dump_ana_reg2},
	{0x43, 0x43, sys_aonp_dump_ana_reg3},
	{0x44, 0x44, sys_aonp_dump_ana_reg4},
	{0x45, 0x45, sys_aonp_dump_ana_reg5},
	{0x46, 0x46, sys_aonp_dump_ana_reg6},
	{0x47, 0x47, sys_aonp_dump_ana_reg7},
	{0x48, 0x48, sys_aonp_dump_ana_reg8},
	{0x49, 0x49, sys_aonp_dump_ana_reg9},
	{0x4a, 0x4a, sys_aonp_dump_ana_reg10},
	{0x4b, 0x4b, sys_aonp_dump_ana_reg11},
	{0x4c, 0x4c, sys_aonp_dump_ana_reg12},
	{0x4d, 0x4d, sys_aonp_dump_ana_reg13},
	{0x4e, 0x4e, sys_aonp_dump_ana_reg14},
	{0x4f, 0x4f, sys_aonp_dump_ana_reg15},
	{0x50, 0x50, sys_aonp_dump_ana_reg16},
	{0x51, 0x51, sys_aonp_dump_ana_reg17},
	{0x52, 0x52, sys_aonp_dump_ana_reg18},
	{0x53, 0x53, sys_aonp_dump_ana_reg19},
	{0x54, 0x54, sys_aonp_dump_ana_reg20},
	{0x55, 0x55, sys_aonp_dump_ana_reg21},
	{0x56, 0x56, sys_aonp_dump_ana_reg22},
	{0x57, 0x57, sys_aonp_dump_ana_reg23},
	{0x58, 0x58, sys_aonp_dump_ana_reg24},
	{0x59, 0x59, sys_aonp_dump_ana_reg25},
	{0x5a, 0x5a, sys_aonp_dump_ana_reg26},
	{0x5b, 0x5b, sys_aonp_dump_ana_reg27},
	{0x5c, 0x5c, sys_aonp_dump_ana_reg28},
	{0x5d, 0x5d, sys_aonp_dump_ana_reg29},
	{0x5e, 0x5e, sys_aonp_dump_ana_reg30},
	{0x5f, 0x5f, sys_aonp_dump_ana_reg31},
	{0x60, 0x60, sys_aonp_dump_ana_reg32},
	{0x61, 0x61, sys_aonp_dump_ana_reg33},
	{0x62, 0x62, sys_aonp_dump_ana_reg34},
	{0x63, 0x63, sys_aonp_dump_ana_reg35},
	{0x64, 0x64, sys_aonp_dump_ana_reg36},
	{0x65, 0x65, sys_aonp_dump_ana_reg37},
	{0x66, 0x66, sys_aonp_dump_ana_reg38},
	{0x67, 0x67, sys_aonp_dump_ana_reg39},
	{0x68, 0x68, sys_aonp_dump_ana_reg40},
	{0x69, 0x69, sys_aonp_dump_ana_reg41},
	{0x6a, 0x6a, sys_aonp_dump_ana_reg42},
	{0x6b, 0x6b, sys_aonp_dump_ana_reg43},
	{0x6c, 0x6c, sys_aonp_dump_ana_reg44},
	{0x6d, 0x6d, sys_aonp_dump_ana_reg45},
	{0x6e, 0x6e, sys_aonp_dump_ana_reg46},
	{0x6f, 0x6f, sys_aonp_dump_ana_reg47},
	{-1, -1, 0}
};

void sys_aonp_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
