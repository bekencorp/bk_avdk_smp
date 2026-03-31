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
#include "aud_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_REG_LL_REG_BASE   SOC_AUDIO_REG_REG_BASE

//reg device_id:

static inline void audio_reg_ll_set_device_id_value(uint32_t v) {
	audio_reg_device_id_t *r = (audio_reg_device_id_t*)(SOC_AUDIO_REG_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_device_id_value(void) {
	audio_reg_device_id_t *r = (audio_reg_device_id_t*)(SOC_AUDIO_REG_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_device_id_device_id(void) {
	audio_reg_device_id_t *r = (audio_reg_device_id_t*)(SOC_AUDIO_REG_REG_BASE + (0x0 << 2));
	return r->device_id;
}

//reg version_id:

static inline void audio_reg_ll_set_version_id_value(uint32_t v) {
	audio_reg_version_id_t *r = (audio_reg_version_id_t*)(SOC_AUDIO_REG_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_version_id_value(void) {
	audio_reg_version_id_t *r = (audio_reg_version_id_t*)(SOC_AUDIO_REG_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_version_id_version_id(void) {
	audio_reg_version_id_t *r = (audio_reg_version_id_t*)(SOC_AUDIO_REG_REG_BASE + (0x1 << 2));
	return r->version_id;
}

//reg reserved0:

static inline void audio_reg_ll_set_reserved0_value(uint32_t v) {
	audio_reg_reserved0_t *r = (audio_reg_reserved0_t*)(SOC_AUDIO_REG_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_reserved0_value(void) {
	audio_reg_reserved0_t *r = (audio_reg_reserved0_t*)(SOC_AUDIO_REG_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_reserved0_reserved0(void) {
	audio_reg_reserved0_t *r = (audio_reg_reserved0_t*)(SOC_AUDIO_REG_REG_BASE + (0x2 << 2));
	return r->reserved0;
}

//reg reserved1:

static inline void audio_reg_ll_set_reserved1_value(uint32_t v) {
	audio_reg_reserved1_t *r = (audio_reg_reserved1_t*)(SOC_AUDIO_REG_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_reserved1_value(void) {
	audio_reg_reserved1_t *r = (audio_reg_reserved1_t*)(SOC_AUDIO_REG_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_reserved1_reserved1(void) {
	audio_reg_reserved1_t *r = (audio_reg_reserved1_t*)(SOC_AUDIO_REG_REG_BASE + (0x3 << 2));
	return r->reserved1;
}

//reg sys_cfg:

static inline void audio_reg_ll_set_sys_cfg_value(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_value(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_sys_cfg_digmic_en(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->digmic_en = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_digmic_en(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->digmic_en;
}

static inline void audio_reg_ll_set_sys_cfg_dmic_sel(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->dmic_sel = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_dmic_sel(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->dmic_sel;
}

static inline void audio_reg_ll_set_sys_cfg_spl_sel_adc(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->spl_sel_adc = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_spl_sel_adc(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->spl_sel_adc;
}

static inline void audio_reg_ll_set_sys_cfg_mem_dac_iir1x_sw_init(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_dac_iir1x_sw_init = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_dac_iir1x_sw_init(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_dac_iir1x_sw_init;
}

static inline void audio_reg_ll_set_sys_cfg_mem_auto_init_trig(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_auto_init_trig = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_auto_init_trig(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_auto_init_trig;
}

static inline void audio_reg_ll_set_sys_cfg_mem_dac_sw_init(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_dac_sw_init = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_dac_sw_init(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_dac_sw_init;
}

static inline void audio_reg_ll_set_sys_cfg_mem_mic_sw_init(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_mic_sw_init = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_mic_sw_init(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_mic_sw_init;
}

static inline void audio_reg_ll_set_sys_cfg_mem_anc_sw_init(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_anc_sw_init = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_anc_sw_init(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_anc_sw_init;
}

static inline void audio_reg_ll_set_sys_cfg_clk_mic_sel(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->clk_mic_sel = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_clk_mic_sel(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->clk_mic_sel;
}

static inline void audio_reg_ll_set_sys_cfg_rx_sp_sel(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->rx_sp_sel = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_rx_sp_sel(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->rx_sp_sel;
}

static inline void audio_reg_ll_set_sys_cfg_mem_dac_eq_sw_init(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_dac_eq_sw_init = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_dac_eq_sw_init(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_dac_eq_sw_init;
}

static inline void audio_reg_ll_set_sys_cfg_mem_dac_comp_sw_init(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->mem_dac_comp_sw_init = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_mem_dac_comp_sw_init(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->mem_dac_comp_sw_init;
}

static inline void audio_reg_ll_set_sys_cfg_apb_clk_en_dis(uint32_t v) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	r->apb_clk_en_dis = v;
}

static inline uint32_t audio_reg_ll_get_sys_cfg_apb_clk_en_dis(void) {
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));
	return r->apb_clk_en_dis;
}

//reg a2dp_comp:

static inline void audio_reg_ll_set_a2dp_comp_value(uint32_t v) {
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t*)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_a2dp_comp_value(void) {
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t*)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_a2dp_comp_anc_comp_st_val(uint32_t v) {
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t*)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));
	r->anc_comp_st_val = v;
}

static inline uint32_t audio_reg_ll_get_a2dp_comp_anc_comp_st_val(void) {
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t*)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));
	return r->anc_comp_st_val;
}

static inline void audio_reg_ll_set_a2dp_comp_anc_comp_en_frc(uint32_t v) {
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t*)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));
	r->anc_comp_en_frc = v;
}

static inline uint32_t audio_reg_ll_get_a2dp_comp_anc_comp_en_frc(void) {
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t*)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));
	return r->anc_comp_en_frc;
}

//reg adc_cfg:

static inline void audio_reg_ll_set_adc_cfg_value(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_value(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cfg_aec_en(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->aec_en = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_aec_en(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->aec_en;
}

static inline void audio_reg_ll_set_adc_cfg_adc_16b_sel(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->adc_16b_sel = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_adc_16b_sel(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->adc_16b_sel;
}

static inline void audio_reg_ll_set_adc_cfg_aec_16b_sel(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->aec_16b_sel = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_aec_16b_sel(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->aec_16b_sel;
}

static inline void audio_reg_ll_set_adc_cfg_adc_en(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->adc_en = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_adc_en(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->adc_en;
}

static inline void audio_reg_ll_set_adc_cfg_adc_lpf_bps1(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->adc_lpf_bps1 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_adc_lpf_bps1(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->adc_lpf_bps1;
}

static inline void audio_reg_ll_set_adc_cfg_adc_lpf_bps2(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->adc_lpf_bps2 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_adc_lpf_bps2(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->adc_lpf_bps2;
}

static inline void audio_reg_ll_set_adc_cfg_adc_lpf_bps3(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->adc_lpf_bps3 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_adc_lpf_bps3(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->adc_lpf_bps3;
}

static inline void audio_reg_ll_set_adc_cfg_adc_hpf_bps(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->adc_hpf_bps = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_adc_hpf_bps(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->adc_hpf_bps;
}

static inline void audio_reg_ll_set_adc_cfg_clk_adc_inv(uint32_t v) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	r->clk_adc_inv = v;
}

static inline uint32_t audio_reg_ll_get_adc_cfg_clk_adc_inv(void) {
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));
	return r->clk_adc_inv;
}

//reg anc_cfg:

static inline void audio_reg_ll_set_anc_cfg_value(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_value(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc_cfg_anc_frc_on(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc_frc_on = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc_frc_on(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc_frc_on;
}

static inline void audio_reg_ll_set_anc_cfg_anc0_ramp_bps(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc0_ramp_bps = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc0_ramp_bps(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc0_ramp_bps;
}

static inline void audio_reg_ll_set_anc_cfg_anc1_ramp_bps(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc1_ramp_bps = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc1_ramp_bps(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc1_ramp_bps;
}

static inline void audio_reg_ll_set_anc_cfg_anc0_ramp_down_trig(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc0_ramp_down_trig = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc0_ramp_down_trig(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc0_ramp_down_trig;
}

static inline void audio_reg_ll_set_anc_cfg_anc1_ramp_down_trig(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc1_ramp_down_trig = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc1_ramp_down_trig(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc1_ramp_down_trig;
}

static inline void audio_reg_ll_set_anc_cfg_anc_ramp_cfg(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc_ramp_cfg = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc_ramp_cfg(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc_ramp_cfg;
}

static inline void audio_reg_ll_set_anc_cfg_anc_cic_setp_3(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc_cic_setp_3 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc_cic_setp_3(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc_cic_setp_3;
}

static inline void audio_reg_ll_set_anc_cfg_dac_cic_step_2(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->dac_cic_step_2 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_dac_cic_step_2(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->dac_cic_step_2;
}

static inline void audio_reg_ll_set_anc_cfg_anc0_ramp_up_trig(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc0_ramp_up_trig = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc0_ramp_up_trig(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc0_ramp_up_trig;
}

static inline void audio_reg_ll_set_anc_cfg_anc1_ramp_up_trig(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc1_ramp_up_trig = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc1_ramp_up_trig(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc1_ramp_up_trig;
}

static inline void audio_reg_ll_set_anc_cfg_anc_en0(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc_en0 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc_en0(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc_en0;
}

static inline void audio_reg_ll_set_anc_cfg_anc_en1(uint32_t v) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	r->anc_en1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg_anc_en1(void) {
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));
	return r->anc_en1;
}

//reg dac_cfg:

static inline void audio_reg_ll_set_dac_cfg_value(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_value(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_cfg_reserved_0_0(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->reserved_0_0 = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_reserved_0_0(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->reserved_0_0;
}

static inline void audio_reg_ll_set_dac_cfg_dac_enable_l(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_enable_l = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_enable_l(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_enable_l;
}

static inline void audio_reg_ll_set_dac_cfg_dac_enable_r(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_enable_r = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_enable_r(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_enable_r;
}

static inline void audio_reg_ll_set_dac_cfg_dac_iir_bps(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_iir_bps = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_iir_bps(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_iir_bps;
}

static inline void audio_reg_ll_set_dac_cfg_dac_lpf_bps1(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_lpf_bps1 = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_lpf_bps1(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_lpf_bps1;
}

static inline void audio_reg_ll_set_dac_cfg_dac_lpf_bps2(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_lpf_bps2 = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_lpf_bps2(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_lpf_bps2;
}

static inline void audio_reg_ll_set_dac_cfg_dac_lpf_bps3(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_lpf_bps3 = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_lpf_bps3(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_lpf_bps3;
}

static inline void audio_reg_ll_set_dac_cfg_dac_tx_anc_d2(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_tx_anc_d2 = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_tx_anc_d2(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_tx_anc_d2;
}

static inline void audio_reg_ll_set_dac_cfg_dac_16b_sel(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_16b_sel = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_16b_sel(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_16b_sel;
}

static inline void audio_reg_ll_set_dac_cfg_dac_spl_sel(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_spl_sel = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_spl_sel(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_spl_sel;
}

static inline void audio_reg_ll_set_dac_cfg_dac_hpf_bps(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dac_hpf_bps = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dac_hpf_bps(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dac_hpf_bps;
}

static inline void audio_reg_ll_set_dac_cfg_stereo_en(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->stereo_en = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_stereo_en(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->stereo_en;
}

static inline void audio_reg_ll_set_dac_cfg_drc_bypass(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->drc_bypass = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_drc_bypass(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->drc_bypass;
}

static inline void audio_reg_ll_set_dac_cfg_spk2mic_tst(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->spk2mic_tst = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_spk2mic_tst(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->spk2mic_tst;
}

static inline void audio_reg_ll_set_dac_cfg_mono_sel(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->mono_sel = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_mono_sel(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->mono_sel;
}

static inline void audio_reg_ll_set_dac_cfg_hint_spl_sel(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->hint_spl_sel = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_hint_spl_sel(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->hint_spl_sel;
}

static inline void audio_reg_ll_set_dac_cfg_call_spl_sel(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->call_spl_sel = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_call_spl_sel(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->call_spl_sel;
}

static inline void audio_reg_ll_set_dac_cfg_dith_en(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->dith_en = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_dith_en(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->dith_en;
}

static inline void audio_reg_ll_set_dac_cfg_clk_dac_inv(uint32_t v) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	r->clk_dac_inv = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg_clk_dac_inv(void) {
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));
	return r->clk_dac_inv;
}

//reg adc_cic_coef0:

static inline void audio_reg_ll_set_adc_cic_coef0_value(uint32_t v) {
	audio_reg_adc_cic_coef0_t *r = (audio_reg_adc_cic_coef0_t*)(SOC_AUDIO_REG_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef0_value(void) {
	audio_reg_adc_cic_coef0_t *r = (audio_reg_adc_cic_coef0_t*)(SOC_AUDIO_REG_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef0_reg_cic_coef0(uint32_t v) {
	audio_reg_adc_cic_coef0_t *r = (audio_reg_adc_cic_coef0_t*)(SOC_AUDIO_REG_REG_BASE + (0x9 << 2));
	r->reg_cic_coef0 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef0_reg_cic_coef0(void) {
	audio_reg_adc_cic_coef0_t *r = (audio_reg_adc_cic_coef0_t*)(SOC_AUDIO_REG_REG_BASE + (0x9 << 2));
	return r->reg_cic_coef0;
}

//reg adc_cic_coef1:

static inline void audio_reg_ll_set_adc_cic_coef1_value(uint32_t v) {
	audio_reg_adc_cic_coef1_t *r = (audio_reg_adc_cic_coef1_t*)(SOC_AUDIO_REG_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef1_value(void) {
	audio_reg_adc_cic_coef1_t *r = (audio_reg_adc_cic_coef1_t*)(SOC_AUDIO_REG_REG_BASE + (0xa << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef1_reg_cic_coef1(uint32_t v) {
	audio_reg_adc_cic_coef1_t *r = (audio_reg_adc_cic_coef1_t*)(SOC_AUDIO_REG_REG_BASE + (0xa << 2));
	r->reg_cic_coef1 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef1_reg_cic_coef1(void) {
	audio_reg_adc_cic_coef1_t *r = (audio_reg_adc_cic_coef1_t*)(SOC_AUDIO_REG_REG_BASE + (0xa << 2));
	return r->reg_cic_coef1;
}

//reg adc_cic_coef2:

static inline void audio_reg_ll_set_adc_cic_coef2_value(uint32_t v) {
	audio_reg_adc_cic_coef2_t *r = (audio_reg_adc_cic_coef2_t*)(SOC_AUDIO_REG_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef2_value(void) {
	audio_reg_adc_cic_coef2_t *r = (audio_reg_adc_cic_coef2_t*)(SOC_AUDIO_REG_REG_BASE + (0xb << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef2_reg_cic_coef2(uint32_t v) {
	audio_reg_adc_cic_coef2_t *r = (audio_reg_adc_cic_coef2_t*)(SOC_AUDIO_REG_REG_BASE + (0xb << 2));
	r->reg_cic_coef2 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef2_reg_cic_coef2(void) {
	audio_reg_adc_cic_coef2_t *r = (audio_reg_adc_cic_coef2_t*)(SOC_AUDIO_REG_REG_BASE + (0xb << 2));
	return r->reg_cic_coef2;
}

//reg adc_cic_coef3:

static inline void audio_reg_ll_set_adc_cic_coef3_value(uint32_t v) {
	audio_reg_adc_cic_coef3_t *r = (audio_reg_adc_cic_coef3_t*)(SOC_AUDIO_REG_REG_BASE + (0xc << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef3_value(void) {
	audio_reg_adc_cic_coef3_t *r = (audio_reg_adc_cic_coef3_t*)(SOC_AUDIO_REG_REG_BASE + (0xc << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef3_reg_cic_coef3(uint32_t v) {
	audio_reg_adc_cic_coef3_t *r = (audio_reg_adc_cic_coef3_t*)(SOC_AUDIO_REG_REG_BASE + (0xc << 2));
	r->reg_cic_coef3 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef3_reg_cic_coef3(void) {
	audio_reg_adc_cic_coef3_t *r = (audio_reg_adc_cic_coef3_t*)(SOC_AUDIO_REG_REG_BASE + (0xc << 2));
	return r->reg_cic_coef3;
}

//reg adc_cic_coef4:

static inline void audio_reg_ll_set_adc_cic_coef4_value(uint32_t v) {
	audio_reg_adc_cic_coef4_t *r = (audio_reg_adc_cic_coef4_t*)(SOC_AUDIO_REG_REG_BASE + (0xd << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef4_value(void) {
	audio_reg_adc_cic_coef4_t *r = (audio_reg_adc_cic_coef4_t*)(SOC_AUDIO_REG_REG_BASE + (0xd << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef4_reg_cic_coef4(uint32_t v) {
	audio_reg_adc_cic_coef4_t *r = (audio_reg_adc_cic_coef4_t*)(SOC_AUDIO_REG_REG_BASE + (0xd << 2));
	r->reg_cic_coef4 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef4_reg_cic_coef4(void) {
	audio_reg_adc_cic_coef4_t *r = (audio_reg_adc_cic_coef4_t*)(SOC_AUDIO_REG_REG_BASE + (0xd << 2));
	return r->reg_cic_coef4;
}

//reg adc_cic_coef5:

static inline void audio_reg_ll_set_adc_cic_coef5_value(uint32_t v) {
	audio_reg_adc_cic_coef5_t *r = (audio_reg_adc_cic_coef5_t*)(SOC_AUDIO_REG_REG_BASE + (0xe << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef5_value(void) {
	audio_reg_adc_cic_coef5_t *r = (audio_reg_adc_cic_coef5_t*)(SOC_AUDIO_REG_REG_BASE + (0xe << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef5_reg_cic_coef5(uint32_t v) {
	audio_reg_adc_cic_coef5_t *r = (audio_reg_adc_cic_coef5_t*)(SOC_AUDIO_REG_REG_BASE + (0xe << 2));
	r->reg_cic_coef5 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef5_reg_cic_coef5(void) {
	audio_reg_adc_cic_coef5_t *r = (audio_reg_adc_cic_coef5_t*)(SOC_AUDIO_REG_REG_BASE + (0xe << 2));
	return r->reg_cic_coef5;
}

//reg adc_cic_coef6:

static inline void audio_reg_ll_set_adc_cic_coef6_value(uint32_t v) {
	audio_reg_adc_cic_coef6_t *r = (audio_reg_adc_cic_coef6_t*)(SOC_AUDIO_REG_REG_BASE + (0xf << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef6_value(void) {
	audio_reg_adc_cic_coef6_t *r = (audio_reg_adc_cic_coef6_t*)(SOC_AUDIO_REG_REG_BASE + (0xf << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef6_reg_cic_coef6(uint32_t v) {
	audio_reg_adc_cic_coef6_t *r = (audio_reg_adc_cic_coef6_t*)(SOC_AUDIO_REG_REG_BASE + (0xf << 2));
	r->reg_cic_coef6 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef6_reg_cic_coef6(void) {
	audio_reg_adc_cic_coef6_t *r = (audio_reg_adc_cic_coef6_t*)(SOC_AUDIO_REG_REG_BASE + (0xf << 2));
	return r->reg_cic_coef6;
}

//reg adc_cic_coef7:

static inline void audio_reg_ll_set_adc_cic_coef7_value(uint32_t v) {
	audio_reg_adc_cic_coef7_t *r = (audio_reg_adc_cic_coef7_t*)(SOC_AUDIO_REG_REG_BASE + (0x10 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef7_value(void) {
	audio_reg_adc_cic_coef7_t *r = (audio_reg_adc_cic_coef7_t*)(SOC_AUDIO_REG_REG_BASE + (0x10 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cic_coef7_reg_cic_coef7(uint32_t v) {
	audio_reg_adc_cic_coef7_t *r = (audio_reg_adc_cic_coef7_t*)(SOC_AUDIO_REG_REG_BASE + (0x10 << 2));
	r->reg_cic_coef7 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cic_coef7_reg_cic_coef7(void) {
	audio_reg_adc_cic_coef7_t *r = (audio_reg_adc_cic_coef7_t*)(SOC_AUDIO_REG_REG_BASE + (0x10 << 2));
	return r->reg_cic_coef7;
}

//reg mic1_dbg_ctrl:

static inline void audio_reg_ll_set_mic1_dbg_ctrl_value(uint32_t v) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_mic1_dbg_ctrl_value(void) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_mic1_dbg_ctrl_spk2mic_dbg_en(uint32_t v) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	r->spk2mic_dbg_en = v;
}

static inline uint32_t audio_reg_ll_get_mic1_dbg_ctrl_spk2mic_dbg_en(void) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	return r->spk2mic_dbg_en;
}

static inline void audio_reg_ll_set_mic1_dbg_ctrl_dac_cfg_anc_add(uint32_t v) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	r->dac_cfg_anc_add = v;
}

static inline uint32_t audio_reg_ll_get_mic1_dbg_ctrl_dac_cfg_anc_add(void) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	return r->dac_cfg_anc_add;
}

static inline void audio_reg_ll_set_mic1_dbg_ctrl_dac_cfg_dac_add(uint32_t v) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	r->dac_cfg_dac_add = v;
}

static inline uint32_t audio_reg_ll_get_mic1_dbg_ctrl_dac_cfg_dac_add(void) {
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));
	return r->dac_cfg_dac_add;
}

//reg buf_ctrl:

static inline void audio_reg_ll_set_buf_ctrl_value(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_value(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_buf_ctrl_en_spk0(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->en_spk0 = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_en_spk0(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->en_spk0;
}

static inline void audio_reg_ll_set_buf_ctrl_en_spk1(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->en_spk1 = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_en_spk1(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->en_spk1;
}

static inline void audio_reg_ll_set_buf_ctrl_en_mic(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->en_mic = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_en_mic(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->en_mic;
}

static inline void audio_reg_ll_set_buf_ctrl_dma_mask_spk0(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->dma_mask_spk0 = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_dma_mask_spk0(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->dma_mask_spk0;
}

static inline void audio_reg_ll_set_buf_ctrl_dma_mask_spk1(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->dma_mask_spk1 = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_dma_mask_spk1(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->dma_mask_spk1;
}

static inline void audio_reg_ll_set_buf_ctrl_dma_mask_mic(uint32_t v) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	r->dma_mask_mic = v;
}

static inline uint32_t audio_reg_ll_get_buf_ctrl_dma_mask_mic(void) {
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));
	return r->dma_mask_mic;
}

//reg mic_fifo_cfg:

static inline void audio_reg_ll_set_mic_fifo_cfg_value(uint32_t v) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_mic_fifo_cfg_value(void) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_mic_fifo_cfg_mic0_wr_thrd(uint32_t v) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	r->mic0_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_mic_fifo_cfg_mic0_wr_thrd(void) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	return r->mic0_wr_thrd;
}

static inline void audio_reg_ll_set_mic_fifo_cfg_mic0_rd_thrd(uint32_t v) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	r->mic0_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_mic_fifo_cfg_mic0_rd_thrd(void) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	return r->mic0_rd_thrd;
}

static inline void audio_reg_ll_set_mic_fifo_cfg_mic1_wr_thrd(uint32_t v) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	r->mic1_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_mic_fifo_cfg_mic1_wr_thrd(void) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	return r->mic1_wr_thrd;
}

static inline void audio_reg_ll_set_mic_fifo_cfg_mic1_rd_thrd(uint32_t v) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	r->mic1_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_mic_fifo_cfg_mic1_rd_thrd(void) {
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));
	return r->mic1_rd_thrd;
}

//reg spk0_fifo_cfg:

static inline void audio_reg_ll_set_spk0_fifo_cfg_value(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_value(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_spk0_fifo_cfg_spk0_hint_wr_thrd(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->spk0_hint_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_spk0_hint_wr_thrd(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->spk0_hint_wr_thrd;
}

static inline void audio_reg_ll_set_spk0_fifo_cfg_spk0_hint_rd_thrd(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->spk0_hint_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_spk0_hint_rd_thrd(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->spk0_hint_rd_thrd;
}

static inline void audio_reg_ll_set_spk0_fifo_cfg_spk0_call_wr_thrd(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->spk0_call_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_spk0_call_wr_thrd(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->spk0_call_wr_thrd;
}

static inline void audio_reg_ll_set_spk0_fifo_cfg_spk0_call_rd_thrd(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->spk0_call_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_spk0_call_rd_thrd(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->spk0_call_rd_thrd;
}

static inline void audio_reg_ll_set_spk0_fifo_cfg_spk0_a2dp_wr_thrd(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->spk0_a2dp_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_spk0_a2dp_wr_thrd(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->spk0_a2dp_wr_thrd;
}

static inline void audio_reg_ll_set_spk0_fifo_cfg_spk0_a2dp_rd_thrd(uint32_t v) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	r->spk0_a2dp_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk0_fifo_cfg_spk0_a2dp_rd_thrd(void) {
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));
	return r->spk0_a2dp_rd_thrd;
}

//reg spk1_fifo_cfg:

static inline void audio_reg_ll_set_spk1_fifo_cfg_value(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_value(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_spk1_fifo_cfg_spk1_hint_wr_thrd(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->spk1_hint_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_spk1_hint_wr_thrd(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->spk1_hint_wr_thrd;
}

static inline void audio_reg_ll_set_spk1_fifo_cfg_spk1_hint_rd_thrd(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->spk1_hint_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_spk1_hint_rd_thrd(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->spk1_hint_rd_thrd;
}

static inline void audio_reg_ll_set_spk1_fifo_cfg_spk1_call_wr_thrd(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->spk1_call_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_spk1_call_wr_thrd(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->spk1_call_wr_thrd;
}

static inline void audio_reg_ll_set_spk1_fifo_cfg_spk1_call_rd_thrd(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->spk1_call_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_spk1_call_rd_thrd(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->spk1_call_rd_thrd;
}

static inline void audio_reg_ll_set_spk1_fifo_cfg_spk1_a2dp_wr_thrd(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->spk1_a2dp_wr_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_spk1_a2dp_wr_thrd(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->spk1_a2dp_wr_thrd;
}

static inline void audio_reg_ll_set_spk1_fifo_cfg_spk1_a2dp_rd_thrd(uint32_t v) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	r->spk1_a2dp_rd_thrd = v;
}

static inline uint32_t audio_reg_ll_get_spk1_fifo_cfg_spk1_a2dp_rd_thrd(void) {
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));
	return r->spk1_a2dp_rd_thrd;
}

//reg adc_cut_cfg:

static inline void audio_reg_ll_set_adc_cut_cfg_value(uint32_t v) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_cut_cfg_value(void) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_cut_cfg_adc_cut0(uint32_t v) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	r->adc_cut0 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cut_cfg_adc_cut0(void) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	return r->adc_cut0;
}

static inline void audio_reg_ll_set_adc_cut_cfg_adc_cut1(uint32_t v) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	r->adc_cut1 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cut_cfg_adc_cut1(void) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	return r->adc_cut1;
}

static inline void audio_reg_ll_set_adc_cut_cfg_adc_cut2(uint32_t v) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	r->adc_cut2 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cut_cfg_adc_cut2(void) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	return r->adc_cut2;
}

static inline void audio_reg_ll_set_adc_cut_cfg_adc_cut3(uint32_t v) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	r->adc_cut3 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cut_cfg_adc_cut3(void) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	return r->adc_cut3;
}

static inline void audio_reg_ll_set_adc_cut_cfg_adc_cut4(uint32_t v) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	r->adc_cut4 = v;
}

static inline uint32_t audio_reg_ll_get_adc_cut_cfg_adc_cut4(void) {
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));
	return r->adc_cut4;
}

//reg iir_sft_cfg:

static inline void audio_reg_ll_set_iir_sft_cfg_value(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_value(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_r_sel_0(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->dac_eq_sft_r_sel_0 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_r_sel_0(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->dac_eq_sft_r_sel_0;
}

static inline void audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_r_sel_1(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->dac_eq_sft_r_sel_1 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_r_sel_1(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->dac_eq_sft_r_sel_1;
}

static inline void audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_l_sel_0(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->dac_eq_sft_l_sel_0 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_l_sel_0(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->dac_eq_sft_l_sel_0;
}

static inline void audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_l_sel_1(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->dac_eq_sft_l_sel_1 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_l_sel_1(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->dac_eq_sft_l_sel_1;
}

static inline void audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_r_sel_0(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->comp_eq_sft_r_sel_0 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_r_sel_0(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->comp_eq_sft_r_sel_0;
}

static inline void audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_r_sel_1(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->comp_eq_sft_r_sel_1 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_r_sel_1(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->comp_eq_sft_r_sel_1;
}

static inline void audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_l_sel_0(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->comp_eq_sft_l_sel_0 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_l_sel_0(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->comp_eq_sft_l_sel_0;
}

static inline void audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_l_sel_1(uint32_t v) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	r->comp_eq_sft_l_sel_1 = v;
}

static inline uint32_t audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_l_sel_1(void) {
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));
	return r->comp_eq_sft_l_sel_1;
}

//reg dac_cfg1:

static inline void audio_reg_ll_set_dac_cfg1_value(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_value(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_cfg1_dac_pn_conf(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->dac_pn_conf = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_dac_pn_conf(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->dac_pn_conf;
}

static inline void audio_reg_ll_set_dac_cfg1_notchen(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->notchen = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_notchen(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->notchen;
}

static inline void audio_reg_ll_set_dac_cfg1_sw_board(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->sw_board = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_sw_board(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->sw_board;
}

static inline void audio_reg_ll_set_dac_cfg1_cfg_dac_wait_cnt(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->cfg_dac_wait_cnt = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_cfg_dac_wait_cnt(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->cfg_dac_wait_cnt;
}

static inline void audio_reg_ll_set_dac_cfg1_dac_eq_bps(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->dac_eq_bps = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_dac_eq_bps(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->dac_eq_bps;
}

static inline void audio_reg_ll_set_dac_cfg1_rsp_bps(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->rsp_bps = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_rsp_bps(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->rsp_bps;
}

static inline void audio_reg_ll_set_dac_cfg1_dac_frc_o(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->dac_frc_o = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_dac_frc_o(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->dac_frc_o;
}

static inline void audio_reg_ll_set_dac_cfg1_dac_frc_hw(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->dac_frc_hw = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_dac_frc_hw(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->dac_frc_hw;
}

static inline void audio_reg_ll_set_dac_cfg1_dac_frc_hw_mask(uint32_t v) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	r->dac_frc_hw_mask = v;
}

static inline uint32_t audio_reg_ll_get_dac_cfg1_dac_frc_hw_mask(void) {
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));
	return r->dac_frc_hw_mask;
}

//reg anc_cfg2:

static inline void audio_reg_ll_set_anc_cfg2_value(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_value(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc_cfg2_anc1_sft_sel_1(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc1_sft_sel_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc1_sft_sel_1(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc1_sft_sel_1;
}

static inline void audio_reg_ll_set_anc_cfg2_anc1_sft_sel_2(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc1_sft_sel_2 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc1_sft_sel_2(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc1_sft_sel_2;
}

static inline void audio_reg_ll_set_anc_cfg2_anc1_rshift0(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc1_rshift0 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc1_rshift0(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc1_rshift0;
}

static inline void audio_reg_ll_set_anc_cfg2_anc1_rshift1(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc1_rshift1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc1_rshift1(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc1_rshift1;
}

static inline void audio_reg_ll_set_anc_cfg2_up_spl_sel(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->up_spl_sel = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_up_spl_sel(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->up_spl_sel;
}

static inline void audio_reg_ll_set_anc_cfg2_anc2_sft_sel_1(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc2_sft_sel_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc2_sft_sel_1(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc2_sft_sel_1;
}

static inline void audio_reg_ll_set_anc_cfg2_anc2_sft_sel_2(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc2_sft_sel_2 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc2_sft_sel_2(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc2_sft_sel_2;
}

static inline void audio_reg_ll_set_anc_cfg2_anc2_rshift0(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc2_rshift0 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc2_rshift0(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc2_rshift0;
}

static inline void audio_reg_ll_set_anc_cfg2_anc2_rshift1(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc2_rshift1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc2_rshift1(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc2_rshift1;
}

static inline void audio_reg_ll_set_anc_cfg2_anc_spl_sel(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->anc_spl_sel = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_anc_spl_sel(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->anc_spl_sel;
}

static inline void audio_reg_ll_set_anc_cfg2_dac_sdm_dis(uint32_t v) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	r->dac_sdm_dis = v;
}

static inline uint32_t audio_reg_ll_get_anc_cfg2_dac_sdm_dis(void) {
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));
	return r->dac_sdm_dis;
}

//reg ramp_up_cfg:

static inline void audio_reg_ll_set_ramp_up_cfg_value(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_value(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->v;
}

static inline void audio_reg_ll_set_ramp_up_cfg_anc_comp_en(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->anc_comp_en = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_anc_comp_en(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->anc_comp_en;
}

static inline void audio_reg_ll_set_ramp_up_cfg_comp_ramp_down_trig(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->comp_ramp_down_trig = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_comp_ramp_down_trig(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->comp_ramp_down_trig;
}

static inline void audio_reg_ll_set_ramp_up_cfg_comp_ramp_cfg(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->comp_ramp_cfg = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_comp_ramp_cfg(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->comp_ramp_cfg;
}

static inline void audio_reg_ll_set_ramp_up_cfg_comp_ramp_bps(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->comp_ramp_bps = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_comp_ramp_bps(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->comp_ramp_bps;
}

static inline void audio_reg_ll_set_ramp_up_cfg_eq_ramp_down_trig(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->eq_ramp_down_trig = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_eq_ramp_down_trig(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->eq_ramp_down_trig;
}

static inline void audio_reg_ll_set_ramp_up_cfg_eq_ramp_cfg(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->eq_ramp_cfg = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_eq_ramp_cfg(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->eq_ramp_cfg;
}

static inline void audio_reg_ll_set_ramp_up_cfg_eq_ramp_bps(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->eq_ramp_bps = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_eq_ramp_bps(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->eq_ramp_bps;
}

static inline void audio_reg_ll_set_ramp_up_cfg_eq_ramp_up_trig(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->eq_ramp_up_trig = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_eq_ramp_up_trig(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->eq_ramp_up_trig;
}

static inline void audio_reg_ll_set_ramp_up_cfg_comp_ramp_up_trig(uint32_t v) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	r->comp_ramp_up_trig = v;
}

static inline uint32_t audio_reg_ll_get_ramp_up_cfg_comp_ramp_up_trig(void) {
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));
	return r->comp_ramp_up_trig;
}

//reg anc1_gain_cfg1:

static inline void audio_reg_ll_set_anc1_gain_cfg1_value(uint32_t v) {
	audio_reg_anc1_gain_cfg1_t *r = (audio_reg_anc1_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x1b << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg1_value(void) {
	audio_reg_anc1_gain_cfg1_t *r = (audio_reg_anc1_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x1b << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc1_gain_cfg1_anc1_gain1_1(uint32_t v) {
	audio_reg_anc1_gain_cfg1_t *r = (audio_reg_anc1_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x1b << 2));
	r->anc1_gain1_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg1_anc1_gain1_1(void) {
	audio_reg_anc1_gain_cfg1_t *r = (audio_reg_anc1_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x1b << 2));
	return r->anc1_gain1_1;
}

//reg anc1_gain_cfg2:

static inline void audio_reg_ll_set_anc1_gain_cfg2_value(uint32_t v) {
	audio_reg_anc1_gain_cfg2_t *r = (audio_reg_anc1_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x1c << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg2_value(void) {
	audio_reg_anc1_gain_cfg2_t *r = (audio_reg_anc1_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x1c << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc1_gain_cfg2_anc1_gain_comp(uint32_t v) {
	audio_reg_anc1_gain_cfg2_t *r = (audio_reg_anc1_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x1c << 2));
	r->anc1_gain_comp = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg2_anc1_gain_comp(void) {
	audio_reg_anc1_gain_cfg2_t *r = (audio_reg_anc1_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x1c << 2));
	return r->anc1_gain_comp;
}

//reg anc1_gain_cfg3:

static inline void audio_reg_ll_set_anc1_gain_cfg3_value(uint32_t v) {
	audio_reg_anc1_gain_cfg3_t *r = (audio_reg_anc1_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x1d << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg3_value(void) {
	audio_reg_anc1_gain_cfg3_t *r = (audio_reg_anc1_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x1d << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc1_gain_cfg3_anc1_gain0_1(uint32_t v) {
	audio_reg_anc1_gain_cfg3_t *r = (audio_reg_anc1_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x1d << 2));
	r->anc1_gain0_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg3_anc1_gain0_1(void) {
	audio_reg_anc1_gain_cfg3_t *r = (audio_reg_anc1_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x1d << 2));
	return r->anc1_gain0_1;
}

//reg anc1_gain_cfg4:

static inline void audio_reg_ll_set_anc1_gain_cfg4_value(uint32_t v) {
	audio_reg_anc1_gain_cfg4_t *r = (audio_reg_anc1_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x1f << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg4_value(void) {
	audio_reg_anc1_gain_cfg4_t *r = (audio_reg_anc1_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x1f << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc1_gain_cfg4_anc1_gain0_0(uint32_t v) {
	audio_reg_anc1_gain_cfg4_t *r = (audio_reg_anc1_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x1f << 2));
	r->anc1_gain0_0 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_gain_cfg4_anc1_gain0_0(void) {
	audio_reg_anc1_gain_cfg4_t *r = (audio_reg_anc1_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x1f << 2));
	return r->anc1_gain0_0;
}

//reg sync_ctrl1:

static inline void audio_reg_ll_set_sync_ctrl1_value(uint32_t v) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl1_value(void) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_sync_ctrl1_sync_a2dp_cnt(uint32_t v) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	r->sync_a2dp_cnt = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl1_sync_a2dp_cnt(void) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	return r->sync_a2dp_cnt;
}

static inline void audio_reg_ll_set_sync_ctrl1_start_a2dp_cnt(uint32_t v) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	r->start_a2dp_cnt = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl1_start_a2dp_cnt(void) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	return r->start_a2dp_cnt;
}

static inline void audio_reg_ll_set_sync_ctrl1_sync_call_cnt(uint32_t v) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	r->sync_call_cnt = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl1_sync_call_cnt(void) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	return r->sync_call_cnt;
}

static inline void audio_reg_ll_set_sync_ctrl1_start_call_cnt(uint32_t v) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	r->start_call_cnt = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl1_start_call_cnt(void) {
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t*)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));
	return r->start_call_cnt;
}

//reg sync_ctrl2:

static inline void audio_reg_ll_set_sync_ctrl2_value(uint32_t v) {
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t*)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl2_value(void) {
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t*)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_sync_ctrl2_sync_hint_cnt(uint32_t v) {
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t*)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));
	r->sync_hint_cnt = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl2_sync_hint_cnt(void) {
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t*)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));
	return r->sync_hint_cnt;
}

static inline void audio_reg_ll_set_sync_ctrl2_start_hint_cnt(uint32_t v) {
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t*)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));
	r->start_hint_cnt = v;
}

static inline uint32_t audio_reg_ll_get_sync_ctrl2_start_hint_cnt(void) {
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t*)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));
	return r->start_hint_cnt;
}

//reg dac_ro_sts:

static inline void audio_reg_ll_set_dac_ro_sts_value(uint32_t v) {
	audio_reg_dac_ro_sts_t *r = (audio_reg_dac_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x24 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_ro_sts_value(void) {
	audio_reg_dac_ro_sts_t *r = (audio_reg_dac_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x24 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_dac_ro_sts_dac_fifo_status(void) {
	audio_reg_dac_ro_sts_t *r = (audio_reg_dac_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x24 << 2));
	return r->dac_fifo_status;
}

static inline uint32_t audio_reg_ll_get_dac_ro_sts_mem_auto_init_done(void) {
	audio_reg_dac_ro_sts_t *r = (audio_reg_dac_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x24 << 2));
	return r->mem_auto_init_done;
}

//reg adc_ro_sts:

static inline void audio_reg_ll_set_adc_ro_sts_value(uint32_t v) {
	audio_reg_adc_ro_sts_t *r = (audio_reg_adc_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x25 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_ro_sts_value(void) {
	audio_reg_adc_ro_sts_t *r = (audio_reg_adc_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x25 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_adc_ro_sts_adc_fifo_status(void) {
	audio_reg_adc_ro_sts_t *r = (audio_reg_adc_ro_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x25 << 2));
	return r->adc_fifo_status;
}

//reg anc_sts:

static inline void audio_reg_ll_set_anc_sts_value(uint32_t v) {
	audio_reg_anc_sts_t *r = (audio_reg_anc_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x26 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_sts_value(void) {
	audio_reg_anc_sts_t *r = (audio_reg_anc_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x26 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_anc_sts_anc_status(void) {
	audio_reg_anc_sts_t *r = (audio_reg_anc_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x26 << 2));
	return r->anc_status;
}

//reg ramp_intr_ctrl:

static inline void audio_reg_ll_set_ramp_intr_ctrl_value(uint32_t v) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_ramp_intr_ctrl_value(void) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_ramp_intr_ctrl_ramp_interrupt_mask(uint32_t v) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	r->ramp_interrupt_mask = v;
}

static inline uint32_t audio_reg_ll_get_ramp_intr_ctrl_ramp_interrupt_mask(void) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	return r->ramp_interrupt_mask;
}

static inline void audio_reg_ll_set_ramp_intr_ctrl_iir_ovl_int_mask(uint32_t v) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	r->iir_ovl_int_mask = v;
}

static inline uint32_t audio_reg_ll_get_ramp_intr_ctrl_iir_ovl_int_mask(void) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	return r->iir_ovl_int_mask;
}

static inline void audio_reg_ll_set_ramp_intr_ctrl_ramp_interrupt_clr(uint32_t v) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	r->ramp_interrupt_clr = v;
}

static inline uint32_t audio_reg_ll_get_ramp_intr_ctrl_ramp_interrupt_clr(void) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	return r->ramp_interrupt_clr;
}

static inline void audio_reg_ll_set_ramp_intr_ctrl_iir_ovl_int_clr(uint32_t v) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	r->iir_ovl_int_clr = v;
}

static inline uint32_t audio_reg_ll_get_ramp_intr_ctrl_iir_ovl_int_clr(void) {
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));
	return r->iir_ovl_int_clr;
}

//reg aud_int_ctrl:

static inline void audio_reg_ll_set_aud_int_ctrl_value(uint32_t v) {
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_aud_int_ctrl_value(void) {
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_aud_int_ctrl_aud_interrupt_mask(uint32_t v) {
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));
	r->aud_interrupt_mask = v;
}

static inline uint32_t audio_reg_ll_get_aud_int_ctrl_aud_interrupt_mask(void) {
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));
	return r->aud_interrupt_mask;
}

static inline void audio_reg_ll_set_aud_int_ctrl_aud_interrupt_clr(uint32_t v) {
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));
	r->aud_interrupt_clr = v;
}

static inline uint32_t audio_reg_ll_get_aud_int_ctrl_aud_interrupt_clr(void) {
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t*)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));
	return r->aud_interrupt_clr;
}

//reg aud_int_sts:

static inline void audio_reg_ll_set_aud_int_sts_value(uint32_t v) {
	audio_reg_aud_int_sts_t *r = (audio_reg_aud_int_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x29 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_aud_int_sts_value(void) {
	audio_reg_aud_int_sts_t *r = (audio_reg_aud_int_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x29 << 2));
	return r->v;
}

static inline uint32_t audio_reg_ll_get_aud_int_sts_aud_interrupt_status(void) {
	audio_reg_aud_int_sts_t *r = (audio_reg_aud_int_sts_t*)(SOC_AUDIO_REG_REG_BASE + (0x29 << 2));
	return r->aud_interrupt_status;
}

//reg anc2_limit_cfg1:

static inline void audio_reg_ll_set_anc2_limit_cfg1_value(uint32_t v) {
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc2_limit_cfg1_value(void) {
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc2_limit_cfg1_anc2_limit_val1(uint32_t v) {
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));
	r->anc2_limit_val1 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_limit_cfg1_anc2_limit_val1(void) {
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));
	return r->anc2_limit_val1;
}

static inline void audio_reg_ll_set_anc2_limit_cfg1_anc2_limit_bps1(uint32_t v) {
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));
	r->anc2_limit_bps1 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_limit_cfg1_anc2_limit_bps1(void) {
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));
	return r->anc2_limit_bps1;
}

//reg anc2_limit_cfg2:

static inline void audio_reg_ll_set_anc2_limit_cfg2_value(uint32_t v) {
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc2_limit_cfg2_value(void) {
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc2_limit_cfg2_anc2_limit_val2(uint32_t v) {
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));
	r->anc2_limit_val2 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_limit_cfg2_anc2_limit_val2(void) {
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));
	return r->anc2_limit_val2;
}

static inline void audio_reg_ll_set_anc2_limit_cfg2_anc2_limit_bps2(uint32_t v) {
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));
	r->anc2_limit_bps2 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_limit_cfg2_anc2_limit_bps2(void) {
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));
	return r->anc2_limit_bps2;
}

//reg anc1_limit_cfg1:

static inline void audio_reg_ll_set_anc1_limit_cfg1_value(uint32_t v) {
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc1_limit_cfg1_value(void) {
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc1_limit_cfg1_anc1_limit_val1(uint32_t v) {
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));
	r->anc1_limit_val1 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_limit_cfg1_anc1_limit_val1(void) {
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));
	return r->anc1_limit_val1;
}

static inline void audio_reg_ll_set_anc1_limit_cfg1_anc1_limit_bps1(uint32_t v) {
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));
	r->anc1_limit_bps1 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_limit_cfg1_anc1_limit_bps1(void) {
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));
	return r->anc1_limit_bps1;
}

//reg anc1_limit_cfg2:

static inline void audio_reg_ll_set_anc1_limit_cfg2_value(uint32_t v) {
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc1_limit_cfg2_value(void) {
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc1_limit_cfg2_anc1_limit_val2(uint32_t v) {
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));
	r->anc1_limit_val2 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_limit_cfg2_anc1_limit_val2(void) {
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));
	return r->anc1_limit_val2;
}

static inline void audio_reg_ll_set_anc1_limit_cfg2_anc1_limit_bps2(uint32_t v) {
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));
	r->anc1_limit_bps2 = v;
}

static inline uint32_t audio_reg_ll_get_anc1_limit_cfg2_anc1_limit_bps2(void) {
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));
	return r->anc1_limit_bps2;
}

//reg anc2_gain_cfg1:

static inline void audio_reg_ll_set_anc2_gain_cfg1_value(uint32_t v) {
	audio_reg_anc2_gain_cfg1_t *r = (audio_reg_anc2_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2e << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg1_value(void) {
	audio_reg_anc2_gain_cfg1_t *r = (audio_reg_anc2_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2e << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc2_gain_cfg1_anc2_gain1_1(uint32_t v) {
	audio_reg_anc2_gain_cfg1_t *r = (audio_reg_anc2_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2e << 2));
	r->anc2_gain1_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg1_anc2_gain1_1(void) {
	audio_reg_anc2_gain_cfg1_t *r = (audio_reg_anc2_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x2e << 2));
	return r->anc2_gain1_1;
}

//reg anc2_gain_cfg2:

static inline void audio_reg_ll_set_anc2_gain_cfg2_value(uint32_t v) {
	audio_reg_anc2_gain_cfg2_t *r = (audio_reg_anc2_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2f << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg2_value(void) {
	audio_reg_anc2_gain_cfg2_t *r = (audio_reg_anc2_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2f << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc2_gain_cfg2_anc2_gain_comp(uint32_t v) {
	audio_reg_anc2_gain_cfg2_t *r = (audio_reg_anc2_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2f << 2));
	r->anc2_gain_comp = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg2_anc2_gain_comp(void) {
	audio_reg_anc2_gain_cfg2_t *r = (audio_reg_anc2_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x2f << 2));
	return r->anc2_gain_comp;
}

//reg anc2_gain_cfg3:

static inline void audio_reg_ll_set_anc2_gain_cfg3_value(uint32_t v) {
	audio_reg_anc2_gain_cfg3_t *r = (audio_reg_anc2_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x30 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg3_value(void) {
	audio_reg_anc2_gain_cfg3_t *r = (audio_reg_anc2_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x30 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc2_gain_cfg3_anc2_gain0_1(uint32_t v) {
	audio_reg_anc2_gain_cfg3_t *r = (audio_reg_anc2_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x30 << 2));
	r->anc2_gain0_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg3_anc2_gain0_1(void) {
	audio_reg_anc2_gain_cfg3_t *r = (audio_reg_anc2_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x30 << 2));
	return r->anc2_gain0_1;
}

//reg anc2_gain_cfg4:

static inline void audio_reg_ll_set_anc2_gain_cfg4_value(uint32_t v) {
	audio_reg_anc2_gain_cfg4_t *r = (audio_reg_anc2_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x31 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg4_value(void) {
	audio_reg_anc2_gain_cfg4_t *r = (audio_reg_anc2_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x31 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc2_gain_cfg4_anc2_gain0_0(uint32_t v) {
	audio_reg_anc2_gain_cfg4_t *r = (audio_reg_anc2_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x31 << 2));
	r->anc2_gain0_0 = v;
}

static inline uint32_t audio_reg_ll_get_anc2_gain_cfg4_anc2_gain0_0(void) {
	audio_reg_anc2_gain_cfg4_t *r = (audio_reg_anc2_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x31 << 2));
	return r->anc2_gain0_0;
}

//reg anc_comp_cfg:

static inline void audio_reg_ll_set_anc_comp_cfg_value(uint32_t v) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_comp_cfg_value(void) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc_comp_cfg_comp_iir_bps(uint32_t v) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	r->comp_iir_bps = v;
}

static inline uint32_t audio_reg_ll_get_anc_comp_cfg_comp_iir_bps(void) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	return r->comp_iir_bps;
}

static inline void audio_reg_ll_set_anc_comp_cfg_dac_comp_spl(uint32_t v) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	r->dac_comp_spl = v;
}

static inline uint32_t audio_reg_ll_get_anc_comp_cfg_dac_comp_spl(void) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	return r->dac_comp_spl;
}

static inline void audio_reg_ll_set_anc_comp_cfg_anc_cfg_start_val(uint32_t v) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	r->anc_cfg_start_val = v;
}

static inline uint32_t audio_reg_ll_get_anc_comp_cfg_anc_cfg_start_val(void) {
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t*)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));
	return r->anc_cfg_start_val;
}

//reg dac_gain_cfg0:

static inline void audio_reg_ll_set_dac_gain_cfg0_value(uint32_t v) {
	audio_reg_dac_gain_cfg0_t *r = (audio_reg_dac_gain_cfg0_t*)(SOC_AUDIO_REG_REG_BASE + (0x33 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg0_value(void) {
	audio_reg_dac_gain_cfg0_t *r = (audio_reg_dac_gain_cfg0_t*)(SOC_AUDIO_REG_REG_BASE + (0x33 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_gain_cfg0_spk0_a2dp_gain(uint32_t v) {
	audio_reg_dac_gain_cfg0_t *r = (audio_reg_dac_gain_cfg0_t*)(SOC_AUDIO_REG_REG_BASE + (0x33 << 2));
	r->spk0_a2dp_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg0_spk0_a2dp_gain(void) {
	audio_reg_dac_gain_cfg0_t *r = (audio_reg_dac_gain_cfg0_t*)(SOC_AUDIO_REG_REG_BASE + (0x33 << 2));
	return r->spk0_a2dp_gain;
}

//reg dac_gain_cfg1:

static inline void audio_reg_ll_set_dac_gain_cfg1_value(uint32_t v) {
	audio_reg_dac_gain_cfg1_t *r = (audio_reg_dac_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x34 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg1_value(void) {
	audio_reg_dac_gain_cfg1_t *r = (audio_reg_dac_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x34 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_gain_cfg1_spk0_call_gain(uint32_t v) {
	audio_reg_dac_gain_cfg1_t *r = (audio_reg_dac_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x34 << 2));
	r->spk0_call_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg1_spk0_call_gain(void) {
	audio_reg_dac_gain_cfg1_t *r = (audio_reg_dac_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x34 << 2));
	return r->spk0_call_gain;
}

//reg dac_gain_cfg2:

static inline void audio_reg_ll_set_dac_gain_cfg2_value(uint32_t v) {
	audio_reg_dac_gain_cfg2_t *r = (audio_reg_dac_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x35 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg2_value(void) {
	audio_reg_dac_gain_cfg2_t *r = (audio_reg_dac_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x35 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_gain_cfg2_spk0_hint_gain(uint32_t v) {
	audio_reg_dac_gain_cfg2_t *r = (audio_reg_dac_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x35 << 2));
	r->spk0_hint_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg2_spk0_hint_gain(void) {
	audio_reg_dac_gain_cfg2_t *r = (audio_reg_dac_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x35 << 2));
	return r->spk0_hint_gain;
}

//reg dac_gain_cfg3:

static inline void audio_reg_ll_set_dac_gain_cfg3_value(uint32_t v) {
	audio_reg_dac_gain_cfg3_t *r = (audio_reg_dac_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x36 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg3_value(void) {
	audio_reg_dac_gain_cfg3_t *r = (audio_reg_dac_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x36 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_gain_cfg3_spk1_a2dp_gain(uint32_t v) {
	audio_reg_dac_gain_cfg3_t *r = (audio_reg_dac_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x36 << 2));
	r->spk1_a2dp_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg3_spk1_a2dp_gain(void) {
	audio_reg_dac_gain_cfg3_t *r = (audio_reg_dac_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x36 << 2));
	return r->spk1_a2dp_gain;
}

//reg dac_gain_cfg4:

static inline void audio_reg_ll_set_dac_gain_cfg4_value(uint32_t v) {
	audio_reg_dac_gain_cfg4_t *r = (audio_reg_dac_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x37 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg4_value(void) {
	audio_reg_dac_gain_cfg4_t *r = (audio_reg_dac_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x37 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_gain_cfg4_spk1_call_gain(uint32_t v) {
	audio_reg_dac_gain_cfg4_t *r = (audio_reg_dac_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x37 << 2));
	r->spk1_call_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg4_spk1_call_gain(void) {
	audio_reg_dac_gain_cfg4_t *r = (audio_reg_dac_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x37 << 2));
	return r->spk1_call_gain;
}

//reg dac_gain_cfg5:

static inline void audio_reg_ll_set_dac_gain_cfg5_value(uint32_t v) {
	audio_reg_dac_gain_cfg5_t *r = (audio_reg_dac_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x38 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg5_value(void) {
	audio_reg_dac_gain_cfg5_t *r = (audio_reg_dac_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x38 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_gain_cfg5_spk1_hint_gain(uint32_t v) {
	audio_reg_dac_gain_cfg5_t *r = (audio_reg_dac_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x38 << 2));
	r->spk1_hint_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_gain_cfg5_spk1_hint_gain(void) {
	audio_reg_dac_gain_cfg5_t *r = (audio_reg_dac_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x38 << 2));
	return r->spk1_hint_gain;
}

//reg adc_gain_cfg1:

static inline void audio_reg_ll_set_adc_gain_cfg1_value(uint32_t v) {
	audio_reg_adc_gain_cfg1_t *r = (audio_reg_adc_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x39 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg1_value(void) {
	audio_reg_adc_gain_cfg1_t *r = (audio_reg_adc_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x39 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_gain_cfg1_adc_chn1_gain(uint32_t v) {
	audio_reg_adc_gain_cfg1_t *r = (audio_reg_adc_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x39 << 2));
	r->adc_chn1_gain = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg1_adc_chn1_gain(void) {
	audio_reg_adc_gain_cfg1_t *r = (audio_reg_adc_gain_cfg1_t*)(SOC_AUDIO_REG_REG_BASE + (0x39 << 2));
	return r->adc_chn1_gain;
}

//reg dac_l_gain_mix:

static inline void audio_reg_ll_set_dac_l_gain_mix_value(uint32_t v) {
	audio_reg_dac_l_gain_mix_t *r = (audio_reg_dac_l_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3a << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_l_gain_mix_value(void) {
	audio_reg_dac_l_gain_mix_t *r = (audio_reg_dac_l_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3a << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_l_gain_mix_dac_l_gain(uint32_t v) {
	audio_reg_dac_l_gain_mix_t *r = (audio_reg_dac_l_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3a << 2));
	r->dac_l_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_l_gain_mix_dac_l_gain(void) {
	audio_reg_dac_l_gain_mix_t *r = (audio_reg_dac_l_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3a << 2));
	return r->dac_l_gain;
}

//reg dac_r_gain_mix:

static inline void audio_reg_ll_set_dac_r_gain_mix_value(uint32_t v) {
	audio_reg_dac_r_gain_mix_t *r = (audio_reg_dac_r_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3b << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_dac_r_gain_mix_value(void) {
	audio_reg_dac_r_gain_mix_t *r = (audio_reg_dac_r_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3b << 2));
	return r->v;
}

static inline void audio_reg_ll_set_dac_r_gain_mix_dac_r_gain(uint32_t v) {
	audio_reg_dac_r_gain_mix_t *r = (audio_reg_dac_r_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3b << 2));
	r->dac_r_gain = v;
}

static inline uint32_t audio_reg_ll_get_dac_r_gain_mix_dac_r_gain(void) {
	audio_reg_dac_r_gain_mix_t *r = (audio_reg_dac_r_gain_mix_t*)(SOC_AUDIO_REG_REG_BASE + (0x3b << 2));
	return r->dac_r_gain;
}

//reg adc_gain_cfg2:

static inline void audio_reg_ll_set_adc_gain_cfg2_value(uint32_t v) {
	audio_reg_adc_gain_cfg2_t *r = (audio_reg_adc_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x40 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg2_value(void) {
	audio_reg_adc_gain_cfg2_t *r = (audio_reg_adc_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x40 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_gain_cfg2_adc_chn0_gain(uint32_t v) {
	audio_reg_adc_gain_cfg2_t *r = (audio_reg_adc_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x40 << 2));
	r->adc_chn0_gain = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg2_adc_chn0_gain(void) {
	audio_reg_adc_gain_cfg2_t *r = (audio_reg_adc_gain_cfg2_t*)(SOC_AUDIO_REG_REG_BASE + (0x40 << 2));
	return r->adc_chn0_gain;
}

//reg adc_gain_cfg3:

static inline void audio_reg_ll_set_adc_gain_cfg3_value(uint32_t v) {
	audio_reg_adc_gain_cfg3_t *r = (audio_reg_adc_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x41 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg3_value(void) {
	audio_reg_adc_gain_cfg3_t *r = (audio_reg_adc_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x41 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_gain_cfg3_adc_chn4_gain(uint32_t v) {
	audio_reg_adc_gain_cfg3_t *r = (audio_reg_adc_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x41 << 2));
	r->adc_chn4_gain = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg3_adc_chn4_gain(void) {
	audio_reg_adc_gain_cfg3_t *r = (audio_reg_adc_gain_cfg3_t*)(SOC_AUDIO_REG_REG_BASE + (0x41 << 2));
	return r->adc_chn4_gain;
}

//reg adc_gain_cfg4:

static inline void audio_reg_ll_set_adc_gain_cfg4_value(uint32_t v) {
	audio_reg_adc_gain_cfg4_t *r = (audio_reg_adc_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x42 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg4_value(void) {
	audio_reg_adc_gain_cfg4_t *r = (audio_reg_adc_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x42 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_gain_cfg4_adc_chn3_gain(uint32_t v) {
	audio_reg_adc_gain_cfg4_t *r = (audio_reg_adc_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x42 << 2));
	r->adc_chn3_gain = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg4_adc_chn3_gain(void) {
	audio_reg_adc_gain_cfg4_t *r = (audio_reg_adc_gain_cfg4_t*)(SOC_AUDIO_REG_REG_BASE + (0x42 << 2));
	return r->adc_chn3_gain;
}

//reg adc_gain_cfg5:

static inline void audio_reg_ll_set_adc_gain_cfg5_value(uint32_t v) {
	audio_reg_adc_gain_cfg5_t *r = (audio_reg_adc_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x43 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg5_value(void) {
	audio_reg_adc_gain_cfg5_t *r = (audio_reg_adc_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x43 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_adc_gain_cfg5_adc_chn2_gain(uint32_t v) {
	audio_reg_adc_gain_cfg5_t *r = (audio_reg_adc_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x43 << 2));
	r->adc_chn2_gain = v;
}

static inline uint32_t audio_reg_ll_get_adc_gain_cfg5_adc_chn2_gain(void) {
	audio_reg_adc_gain_cfg5_t *r = (audio_reg_adc_gain_cfg5_t*)(SOC_AUDIO_REG_REG_BASE + (0x43 << 2));
	return r->adc_chn2_gain;
}

//reg anc_sft_l_para:

static inline void audio_reg_ll_set_anc_sft_l_para_value(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_value(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_0(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->anc_if_sft_l_0 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_0(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->anc_if_sft_l_0;
}

static inline void audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_1(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->anc_if_sft_l_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_1(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->anc_if_sft_l_1;
}

static inline void audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_2(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->anc_if_sft_l_2 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_2(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->anc_if_sft_l_2;
}

static inline void audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_3(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->anc_if_sft_l_3 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_3(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->anc_if_sft_l_3;
}

static inline void audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_4(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->anc_if_sft_l_4 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_4(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->anc_if_sft_l_4;
}

static inline void audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_5(uint32_t v) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	r->anc_if_sft_l_5 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_5(void) {
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));
	return r->anc_if_sft_l_5;
}

//reg anc_sft_r_para:

static inline void audio_reg_ll_set_anc_sft_r_para_value(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_value(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_0(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->anc_if_sft_r_0 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_0(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->anc_if_sft_r_0;
}

static inline void audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_1(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->anc_if_sft_r_1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_1(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->anc_if_sft_r_1;
}

static inline void audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_2(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->anc_if_sft_r_2 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_2(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->anc_if_sft_r_2;
}

static inline void audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_3(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->anc_if_sft_r_3 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_3(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->anc_if_sft_r_3;
}

static inline void audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_4(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->anc_if_sft_r_4 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_4(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->anc_if_sft_r_4;
}

static inline void audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_5(uint32_t v) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	r->anc_if_sft_r_5 = v;
}

static inline uint32_t audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_5(void) {
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t*)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));
	return r->anc_if_sft_r_5;
}

//reg k_val_0:

static inline void audio_reg_ll_set_k_val_0_value(uint32_t v) {
	audio_reg_k_val_0_t *r = (audio_reg_k_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x46 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_0_value(void) {
	audio_reg_k_val_0_t *r = (audio_reg_k_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x46 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_0_k_val_0(uint32_t v) {
	audio_reg_k_val_0_t *r = (audio_reg_k_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x46 << 2));
	r->k_val_0 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_0_k_val_0(void) {
	audio_reg_k_val_0_t *r = (audio_reg_k_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x46 << 2));
	return r->k_val_0;
}

//reg k_val_1:

static inline void audio_reg_ll_set_k_val_1_value(uint32_t v) {
	audio_reg_k_val_1_t *r = (audio_reg_k_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x47 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_1_value(void) {
	audio_reg_k_val_1_t *r = (audio_reg_k_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x47 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_1_k_val_1(uint32_t v) {
	audio_reg_k_val_1_t *r = (audio_reg_k_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x47 << 2));
	r->k_val_1 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_1_k_val_1(void) {
	audio_reg_k_val_1_t *r = (audio_reg_k_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x47 << 2));
	return r->k_val_1;
}

//reg k_val_2:

static inline void audio_reg_ll_set_k_val_2_value(uint32_t v) {
	audio_reg_k_val_2_t *r = (audio_reg_k_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x48 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_2_value(void) {
	audio_reg_k_val_2_t *r = (audio_reg_k_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x48 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_2_k_val_2(uint32_t v) {
	audio_reg_k_val_2_t *r = (audio_reg_k_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x48 << 2));
	r->k_val_2 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_2_k_val_2(void) {
	audio_reg_k_val_2_t *r = (audio_reg_k_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x48 << 2));
	return r->k_val_2;
}

//reg k_val_3:

static inline void audio_reg_ll_set_k_val_3_value(uint32_t v) {
	audio_reg_k_val_3_t *r = (audio_reg_k_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x49 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_3_value(void) {
	audio_reg_k_val_3_t *r = (audio_reg_k_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x49 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_3_k_val_3(uint32_t v) {
	audio_reg_k_val_3_t *r = (audio_reg_k_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x49 << 2));
	r->k_val_3 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_3_k_val_3(void) {
	audio_reg_k_val_3_t *r = (audio_reg_k_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x49 << 2));
	return r->k_val_3;
}

//reg k_val_4:

static inline void audio_reg_ll_set_k_val_4_value(uint32_t v) {
	audio_reg_k_val_4_t *r = (audio_reg_k_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x4a << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_4_value(void) {
	audio_reg_k_val_4_t *r = (audio_reg_k_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x4a << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_4_k_val_4(uint32_t v) {
	audio_reg_k_val_4_t *r = (audio_reg_k_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x4a << 2));
	r->k_val_4 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_4_k_val_4(void) {
	audio_reg_k_val_4_t *r = (audio_reg_k_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x4a << 2));
	return r->k_val_4;
}

//reg k_val_5:

static inline void audio_reg_ll_set_k_val_5_value(uint32_t v) {
	audio_reg_k_val_5_t *r = (audio_reg_k_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x4b << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_5_value(void) {
	audio_reg_k_val_5_t *r = (audio_reg_k_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x4b << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_5_k_val_5(uint32_t v) {
	audio_reg_k_val_5_t *r = (audio_reg_k_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x4b << 2));
	r->k_val_5 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_5_k_val_5(void) {
	audio_reg_k_val_5_t *r = (audio_reg_k_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x4b << 2));
	return r->k_val_5;
}

//reg k_val_6:

static inline void audio_reg_ll_set_k_val_6_value(uint32_t v) {
	audio_reg_k_val_6_t *r = (audio_reg_k_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x4c << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_6_value(void) {
	audio_reg_k_val_6_t *r = (audio_reg_k_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x4c << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_6_k_val_6(uint32_t v) {
	audio_reg_k_val_6_t *r = (audio_reg_k_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x4c << 2));
	r->k_val_6 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_6_k_val_6(void) {
	audio_reg_k_val_6_t *r = (audio_reg_k_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x4c << 2));
	return r->k_val_6;
}

//reg k_val_7:

static inline void audio_reg_ll_set_k_val_7_value(uint32_t v) {
	audio_reg_k_val_7_t *r = (audio_reg_k_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x4d << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_k_val_7_value(void) {
	audio_reg_k_val_7_t *r = (audio_reg_k_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x4d << 2));
	return r->v;
}

static inline void audio_reg_ll_set_k_val_7_k_val_7(uint32_t v) {
	audio_reg_k_val_7_t *r = (audio_reg_k_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x4d << 2));
	r->k_val_7 = v;
}

static inline uint32_t audio_reg_ll_get_k_val_7_k_val_7(void) {
	audio_reg_k_val_7_t *r = (audio_reg_k_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x4d << 2));
	return r->k_val_7;
}

//reg st_val_0:

static inline void audio_reg_ll_set_st_val_0_value(uint32_t v) {
	audio_reg_st_val_0_t *r = (audio_reg_st_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x4e << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_0_value(void) {
	audio_reg_st_val_0_t *r = (audio_reg_st_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x4e << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_0_st_val_0(uint32_t v) {
	audio_reg_st_val_0_t *r = (audio_reg_st_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x4e << 2));
	r->st_val_0 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_0_st_val_0(void) {
	audio_reg_st_val_0_t *r = (audio_reg_st_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x4e << 2));
	return r->st_val_0;
}

//reg st_val_1:

static inline void audio_reg_ll_set_st_val_1_value(uint32_t v) {
	audio_reg_st_val_1_t *r = (audio_reg_st_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x4f << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_1_value(void) {
	audio_reg_st_val_1_t *r = (audio_reg_st_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x4f << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_1_st_val_1(uint32_t v) {
	audio_reg_st_val_1_t *r = (audio_reg_st_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x4f << 2));
	r->st_val_1 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_1_st_val_1(void) {
	audio_reg_st_val_1_t *r = (audio_reg_st_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x4f << 2));
	return r->st_val_1;
}

//reg st_val_2:

static inline void audio_reg_ll_set_st_val_2_value(uint32_t v) {
	audio_reg_st_val_2_t *r = (audio_reg_st_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x50 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_2_value(void) {
	audio_reg_st_val_2_t *r = (audio_reg_st_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x50 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_2_st_val_2(uint32_t v) {
	audio_reg_st_val_2_t *r = (audio_reg_st_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x50 << 2));
	r->st_val_2 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_2_st_val_2(void) {
	audio_reg_st_val_2_t *r = (audio_reg_st_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x50 << 2));
	return r->st_val_2;
}

//reg st_val_3:

static inline void audio_reg_ll_set_st_val_3_value(uint32_t v) {
	audio_reg_st_val_3_t *r = (audio_reg_st_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x51 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_3_value(void) {
	audio_reg_st_val_3_t *r = (audio_reg_st_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x51 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_3_st_val_3(uint32_t v) {
	audio_reg_st_val_3_t *r = (audio_reg_st_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x51 << 2));
	r->st_val_3 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_3_st_val_3(void) {
	audio_reg_st_val_3_t *r = (audio_reg_st_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x51 << 2));
	return r->st_val_3;
}

//reg st_val_4:

static inline void audio_reg_ll_set_st_val_4_value(uint32_t v) {
	audio_reg_st_val_4_t *r = (audio_reg_st_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x52 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_4_value(void) {
	audio_reg_st_val_4_t *r = (audio_reg_st_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x52 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_4_st_val_4(uint32_t v) {
	audio_reg_st_val_4_t *r = (audio_reg_st_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x52 << 2));
	r->st_val_4 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_4_st_val_4(void) {
	audio_reg_st_val_4_t *r = (audio_reg_st_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x52 << 2));
	return r->st_val_4;
}

//reg st_val_5:

static inline void audio_reg_ll_set_st_val_5_value(uint32_t v) {
	audio_reg_st_val_5_t *r = (audio_reg_st_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x53 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_5_value(void) {
	audio_reg_st_val_5_t *r = (audio_reg_st_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x53 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_5_st_val_5(uint32_t v) {
	audio_reg_st_val_5_t *r = (audio_reg_st_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x53 << 2));
	r->st_val_5 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_5_st_val_5(void) {
	audio_reg_st_val_5_t *r = (audio_reg_st_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x53 << 2));
	return r->st_val_5;
}

//reg st_val_6:

static inline void audio_reg_ll_set_st_val_6_value(uint32_t v) {
	audio_reg_st_val_6_t *r = (audio_reg_st_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x54 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_6_value(void) {
	audio_reg_st_val_6_t *r = (audio_reg_st_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x54 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_6_st_val_6(uint32_t v) {
	audio_reg_st_val_6_t *r = (audio_reg_st_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x54 << 2));
	r->st_val_6 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_6_st_val_6(void) {
	audio_reg_st_val_6_t *r = (audio_reg_st_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x54 << 2));
	return r->st_val_6;
}

//reg st_val_7:

static inline void audio_reg_ll_set_st_val_7_value(uint32_t v) {
	audio_reg_st_val_7_t *r = (audio_reg_st_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x55 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_st_val_7_value(void) {
	audio_reg_st_val_7_t *r = (audio_reg_st_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x55 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_st_val_7_st_val_7(uint32_t v) {
	audio_reg_st_val_7_t *r = (audio_reg_st_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x55 << 2));
	r->st_val_7 = v;
}

static inline uint32_t audio_reg_ll_get_st_val_7_st_val_7(void) {
	audio_reg_st_val_7_t *r = (audio_reg_st_val_7_t*)(SOC_AUDIO_REG_REG_BASE + (0x55 << 2));
	return r->st_val_7;
}

//reg p_val_0:

static inline void audio_reg_ll_set_p_val_0_value(uint32_t v) {
	audio_reg_p_val_0_t *r = (audio_reg_p_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x56 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_0_value(void) {
	audio_reg_p_val_0_t *r = (audio_reg_p_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x56 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_0_p_val_0(uint32_t v) {
	audio_reg_p_val_0_t *r = (audio_reg_p_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x56 << 2));
	r->p_val_0 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_0_p_val_0(void) {
	audio_reg_p_val_0_t *r = (audio_reg_p_val_0_t*)(SOC_AUDIO_REG_REG_BASE + (0x56 << 2));
	return r->p_val_0;
}

//reg p_val_1:

static inline void audio_reg_ll_set_p_val_1_value(uint32_t v) {
	audio_reg_p_val_1_t *r = (audio_reg_p_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x57 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_1_value(void) {
	audio_reg_p_val_1_t *r = (audio_reg_p_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x57 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_1_p_val_1(uint32_t v) {
	audio_reg_p_val_1_t *r = (audio_reg_p_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x57 << 2));
	r->p_val_1 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_1_p_val_1(void) {
	audio_reg_p_val_1_t *r = (audio_reg_p_val_1_t*)(SOC_AUDIO_REG_REG_BASE + (0x57 << 2));
	return r->p_val_1;
}

//reg p_val_2:

static inline void audio_reg_ll_set_p_val_2_value(uint32_t v) {
	audio_reg_p_val_2_t *r = (audio_reg_p_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x58 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_2_value(void) {
	audio_reg_p_val_2_t *r = (audio_reg_p_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x58 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_2_p_val_2(uint32_t v) {
	audio_reg_p_val_2_t *r = (audio_reg_p_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x58 << 2));
	r->p_val_2 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_2_p_val_2(void) {
	audio_reg_p_val_2_t *r = (audio_reg_p_val_2_t*)(SOC_AUDIO_REG_REG_BASE + (0x58 << 2));
	return r->p_val_2;
}

//reg p_val_3:

static inline void audio_reg_ll_set_p_val_3_value(uint32_t v) {
	audio_reg_p_val_3_t *r = (audio_reg_p_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x59 << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_3_value(void) {
	audio_reg_p_val_3_t *r = (audio_reg_p_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x59 << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_3_p_val_3(uint32_t v) {
	audio_reg_p_val_3_t *r = (audio_reg_p_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x59 << 2));
	r->p_val_3 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_3_p_val_3(void) {
	audio_reg_p_val_3_t *r = (audio_reg_p_val_3_t*)(SOC_AUDIO_REG_REG_BASE + (0x59 << 2));
	return r->p_val_3;
}

//reg p_val_4:

static inline void audio_reg_ll_set_p_val_4_value(uint32_t v) {
	audio_reg_p_val_4_t *r = (audio_reg_p_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x5a << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_4_value(void) {
	audio_reg_p_val_4_t *r = (audio_reg_p_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x5a << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_4_p_val_4(uint32_t v) {
	audio_reg_p_val_4_t *r = (audio_reg_p_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x5a << 2));
	r->p_val_4 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_4_p_val_4(void) {
	audio_reg_p_val_4_t *r = (audio_reg_p_val_4_t*)(SOC_AUDIO_REG_REG_BASE + (0x5a << 2));
	return r->p_val_4;
}

//reg p_val_5:

static inline void audio_reg_ll_set_p_val_5_value(uint32_t v) {
	audio_reg_p_val_5_t *r = (audio_reg_p_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x5b << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_5_value(void) {
	audio_reg_p_val_5_t *r = (audio_reg_p_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x5b << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_5_p_val_5(uint32_t v) {
	audio_reg_p_val_5_t *r = (audio_reg_p_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x5b << 2));
	r->p_val_5 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_5_p_val_5(void) {
	audio_reg_p_val_5_t *r = (audio_reg_p_val_5_t*)(SOC_AUDIO_REG_REG_BASE + (0x5b << 2));
	return r->p_val_5;
}

//reg p_val_6:

static inline void audio_reg_ll_set_p_val_6_value(uint32_t v) {
	audio_reg_p_val_6_t *r = (audio_reg_p_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x5c << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_p_val_6_value(void) {
	audio_reg_p_val_6_t *r = (audio_reg_p_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x5c << 2));
	return r->v;
}

static inline void audio_reg_ll_set_p_val_6_p_val_6(uint32_t v) {
	audio_reg_p_val_6_t *r = (audio_reg_p_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x5c << 2));
	r->p_val_6 = v;
}

static inline uint32_t audio_reg_ll_get_p_val_6_p_val_6(void) {
	audio_reg_p_val_6_t *r = (audio_reg_p_val_6_t*)(SOC_AUDIO_REG_REG_BASE + (0x5c << 2));
	return r->p_val_6;
}

//reg anc_iir_bps:

static inline void audio_reg_ll_set_anc_iir_bps_value(uint32_t v) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_anc_iir_bps_value(void) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	return r->v;
}

static inline void audio_reg_ll_set_anc_iir_bps_anc_iir_bps0(uint32_t v) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	r->anc_iir_bps0 = v;
}

static inline uint32_t audio_reg_ll_get_anc_iir_bps_anc_iir_bps0(void) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	return r->anc_iir_bps0;
}

static inline void audio_reg_ll_set_anc_iir_bps_anc_iir_bps1(uint32_t v) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	r->anc_iir_bps1 = v;
}

static inline uint32_t audio_reg_ll_get_anc_iir_bps_anc_iir_bps1(void) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	return r->anc_iir_bps1;
}

static inline void audio_reg_ll_set_anc_iir_bps_anc_iir_bps2(uint32_t v) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	r->anc_iir_bps2 = v;
}

static inline uint32_t audio_reg_ll_get_anc_iir_bps_anc_iir_bps2(void) {
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t*)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));
	return r->anc_iir_bps2;
}

//reg interface_matrix:

static inline void audio_reg_ll_set_interface_matrix_value(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->v = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_value(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->v;
}

static inline void audio_reg_ll_set_interface_matrix_adc_chn0_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->adc_chn0_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_adc_chn0_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->adc_chn0_sel;
}

static inline void audio_reg_ll_set_interface_matrix_adc_chn1_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->adc_chn1_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_adc_chn1_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->adc_chn1_sel;
}

static inline void audio_reg_ll_set_interface_matrix_adc_chn2_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->adc_chn2_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_adc_chn2_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->adc_chn2_sel;
}

static inline void audio_reg_ll_set_interface_matrix_adc_chn3_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->adc_chn3_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_adc_chn3_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->adc_chn3_sel;
}

static inline void audio_reg_ll_set_interface_matrix_adc_chn4_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->adc_chn4_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_adc_chn4_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->adc_chn4_sel;
}

static inline void audio_reg_ll_set_interface_matrix_dac_r_chn_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->dac_r_chn_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_dac_r_chn_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->dac_r_chn_sel;
}

static inline void audio_reg_ll_set_interface_matrix_dac_l_chn_sel(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->dac_l_chn_sel = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_dac_l_chn_sel(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->dac_l_chn_sel;
}

static inline void audio_reg_ll_set_interface_matrix_clk_frc_on(uint32_t v) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	r->clk_frc_on = v;
}

static inline uint32_t audio_reg_ll_get_interface_matrix_clk_frc_on(void) {
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t*)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));
	return r->clk_frc_on;
}
#ifdef __cplusplus
}
#endif
