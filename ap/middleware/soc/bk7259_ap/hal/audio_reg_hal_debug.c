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
#include "aud_hw.h"
//#include "audio_reg_hw.h"
#include "audio_reg_hal.h"

typedef void (*audio_reg_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	audio_reg_dump_fn_t fn;
} audio_reg_reg_fn_map_t;

static void audio_reg_dump_device_id(void)
{
	SOC_LOGI("device_id: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x0 << 2)));
}

static void audio_reg_dump_version_id(void)
{
	SOC_LOGI("version_id: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x1 << 2)));
}

static void audio_reg_dump_reserved0(void)
{
	SOC_LOGI("reserved0: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2 << 2)));
}

static void audio_reg_dump_reserved1(void)
{
	SOC_LOGI("reserved1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x3 << 2)));
}

static void audio_reg_dump_sys_cfg(void)
{
	audio_reg_sys_cfg_t *r = (audio_reg_sys_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x4 << 2));

	SOC_LOGI("sys_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	digmic_en: %8x\r\n", r->digmic_en);
	SOC_LOGI("	dmic_sel: %8x\r\n", r->dmic_sel);
	SOC_LOGI("	spl_sel_adc: %8x\r\n", r->spl_sel_adc);
	SOC_LOGI("	reserved_bit_10_17: %8x\r\n", r->reserved_bit_10_17);
	SOC_LOGI("	mem_dac_iir1x_sw_init: %8x\r\n", r->mem_dac_iir1x_sw_init);
	SOC_LOGI("	mem_auto_init_trig: %8x\r\n", r->mem_auto_init_trig);
	SOC_LOGI("	mem_dac_sw_init: %8x\r\n", r->mem_dac_sw_init);
	SOC_LOGI("	mem_mic_sw_init: %8x\r\n", r->mem_mic_sw_init);
	SOC_LOGI("	mem_anc_sw_init: %8x\r\n", r->mem_anc_sw_init);
	SOC_LOGI("	clk_mic_sel: %8x\r\n", r->clk_mic_sel);
	SOC_LOGI("	rx_sp_sel: %8x\r\n", r->rx_sp_sel);
	SOC_LOGI("	mem_dac_eq_sw_init: %8x\r\n", r->mem_dac_eq_sw_init);
	SOC_LOGI("	mem_dac_comp_sw_init: %8x\r\n", r->mem_dac_comp_sw_init);
	SOC_LOGI("	apb_clk_en_dis: %8x\r\n", r->apb_clk_en_dis);
}

static void audio_reg_dump_a2dp_comp(void)
{
	audio_reg_a2dp_comp_t *r = (audio_reg_a2dp_comp_t *)(SOC_AUDIO_REG_REG_BASE + (0x5 << 2));

	SOC_LOGI("a2dp_comp: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	anc_comp_st_val: %8x\r\n", r->anc_comp_st_val);
	SOC_LOGI("	reserved_bit_10_30: %8x\r\n", r->reserved_bit_10_30);
	SOC_LOGI("	anc_comp_en_frc: %8x\r\n", r->anc_comp_en_frc);
}

static void audio_reg_dump_adc_cfg(void)
{
	audio_reg_adc_cfg_t *r = (audio_reg_adc_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x6 << 2));

	SOC_LOGI("adc_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x6 << 2)));
	SOC_LOGI("	aec_en: %8x\r\n", r->aec_en);
	SOC_LOGI("	reserved_bit_2_2: %8x\r\n", r->reserved_bit_2_2);
	SOC_LOGI("	adc_16b_sel: %8x\r\n", r->adc_16b_sel);
	SOC_LOGI("	aec_16b_sel: %8x\r\n", r->aec_16b_sel);
	SOC_LOGI("	reserved_bit_10_11: %8x\r\n", r->reserved_bit_10_11);
	SOC_LOGI("	adc_en: %8x\r\n", r->adc_en);
	SOC_LOGI("	reserved_bit_17_24: %8x\r\n", r->reserved_bit_17_24);
	SOC_LOGI("	adc_lpf_bps1: %8x\r\n", r->adc_lpf_bps1);
	SOC_LOGI("	adc_lpf_bps2: %8x\r\n", r->adc_lpf_bps2);
	SOC_LOGI("	adc_lpf_bps3: %8x\r\n", r->adc_lpf_bps3);
	SOC_LOGI("	adc_hpf_bps: %8x\r\n", r->adc_hpf_bps);
	SOC_LOGI("	reserved_bit_29_29: %8x\r\n", r->reserved_bit_29_29);
	SOC_LOGI("	clk_adc_inv: %8x\r\n", r->clk_adc_inv);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc_cfg(void)
{
	audio_reg_anc_cfg_t *r = (audio_reg_anc_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x7 << 2));

	SOC_LOGI("anc_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x7 << 2)));
	SOC_LOGI("	anc_frc_on: %8x\r\n", r->anc_frc_on);
	SOC_LOGI("	anc0_ramp_bps: %8x\r\n", r->anc0_ramp_bps);
	SOC_LOGI("	anc1_ramp_bps: %8x\r\n", r->anc1_ramp_bps);
	SOC_LOGI("	anc0_ramp_down_trig: %8x\r\n", r->anc0_ramp_down_trig);
	SOC_LOGI("	anc1_ramp_down_trig: %8x\r\n", r->anc1_ramp_down_trig);
	SOC_LOGI("	anc_ramp_cfg: %8x\r\n", r->anc_ramp_cfg);
	SOC_LOGI("	anc_cic_setp_3: %8x\r\n", r->anc_cic_setp_3);
	SOC_LOGI("	dac_cic_step_2: %8x\r\n", r->dac_cic_step_2);
	SOC_LOGI("	anc0_ramp_up_trig: %8x\r\n", r->anc0_ramp_up_trig);
	SOC_LOGI("	anc1_ramp_up_trig: %8x\r\n", r->anc1_ramp_up_trig);
	SOC_LOGI("	reserved_bit_24_25: %8x\r\n", r->reserved_bit_24_25);
	SOC_LOGI("	anc_en0: %8x\r\n", r->anc_en0);
	SOC_LOGI("	anc_en1: %8x\r\n", r->anc_en1);
}

static void audio_reg_dump_dac_cfg(void)
{
	audio_reg_dac_cfg_t *r = (audio_reg_dac_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x8 << 2));

	SOC_LOGI("dac_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x8 << 2)));
	SOC_LOGI("	reserved_0_0: %8x\r\n", r->reserved_0_0);
	SOC_LOGI("	dac_enable_l: %8x\r\n", r->dac_enable_l);
	SOC_LOGI("	dac_enable_r: %8x\r\n", r->dac_enable_r);
	SOC_LOGI("	dac_iir_bps: %8x\r\n", r->dac_iir_bps);
	SOC_LOGI("	dac_lpf_bps1: %8x\r\n", r->dac_lpf_bps1);
	SOC_LOGI("	dac_lpf_bps2: %8x\r\n", r->dac_lpf_bps2);
	SOC_LOGI("	dac_lpf_bps3: %8x\r\n", r->dac_lpf_bps3);
	SOC_LOGI("	dac_tx_anc_d2: %8x\r\n", r->dac_tx_anc_d2);
	SOC_LOGI("	dac_16b_sel: %8x\r\n", r->dac_16b_sel);
	SOC_LOGI("	reserved_bit_12_12: %8x\r\n", r->reserved_bit_12_12);
	SOC_LOGI("	dac_spl_sel: %8x\r\n", r->dac_spl_sel);
	SOC_LOGI("	reserved_bit_14_14: %8x\r\n", r->reserved_bit_14_14);
	SOC_LOGI("	dac_hpf_bps: %8x\r\n", r->dac_hpf_bps);
	SOC_LOGI("	stereo_en: %8x\r\n", r->stereo_en);
	SOC_LOGI("	reserved_bit_19_21: %8x\r\n", r->reserved_bit_19_21);
	SOC_LOGI("	spk2mic_tst: %8x\r\n", r->spk2mic_tst);
	SOC_LOGI("	mono_sel: %8x\r\n", r->mono_sel);
	SOC_LOGI("	hint_spl_sel: %8x\r\n", r->hint_spl_sel);
	SOC_LOGI("	call_spl_sel: %8x\r\n", r->call_spl_sel);
	SOC_LOGI("	dith_en: %8x\r\n", r->dith_en);
	SOC_LOGI("	clk_dac_inv: %8x\r\n", r->clk_dac_inv);
}

static void audio_reg_dump_adc_cic_coef0(void)
{
	audio_reg_adc_cic_coef0_t *r = (audio_reg_adc_cic_coef0_t *)(SOC_AUDIO_REG_REG_BASE + (0x9 << 2));

	SOC_LOGI("adc_cic_coef0: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x9 << 2)));
	SOC_LOGI("	reg_cic_coef0: %8x\r\n", r->reg_cic_coef0);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef1(void)
{
	audio_reg_adc_cic_coef1_t *r = (audio_reg_adc_cic_coef1_t *)(SOC_AUDIO_REG_REG_BASE + (0xa << 2));

	SOC_LOGI("adc_cic_coef1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0xa << 2)));
	SOC_LOGI("	reg_cic_coef1: %8x\r\n", r->reg_cic_coef1);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef2(void)
{
	audio_reg_adc_cic_coef2_t *r = (audio_reg_adc_cic_coef2_t *)(SOC_AUDIO_REG_REG_BASE + (0xb << 2));

	SOC_LOGI("adc_cic_coef2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0xb << 2)));
	SOC_LOGI("	reg_cic_coef2: %8x\r\n", r->reg_cic_coef2);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef3(void)
{
	audio_reg_adc_cic_coef3_t *r = (audio_reg_adc_cic_coef3_t *)(SOC_AUDIO_REG_REG_BASE + (0xc << 2));

	SOC_LOGI("adc_cic_coef3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0xc << 2)));
	SOC_LOGI("	reg_cic_coef3: %8x\r\n", r->reg_cic_coef3);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef4(void)
{
	audio_reg_adc_cic_coef4_t *r = (audio_reg_adc_cic_coef4_t *)(SOC_AUDIO_REG_REG_BASE + (0xd << 2));

	SOC_LOGI("adc_cic_coef4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0xd << 2)));
	SOC_LOGI("	reg_cic_coef4: %8x\r\n", r->reg_cic_coef4);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef5(void)
{
	audio_reg_adc_cic_coef5_t *r = (audio_reg_adc_cic_coef5_t *)(SOC_AUDIO_REG_REG_BASE + (0xe << 2));

	SOC_LOGI("adc_cic_coef5: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0xe << 2)));
	SOC_LOGI("	reg_cic_coef5: %8x\r\n", r->reg_cic_coef5);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef6(void)
{
	audio_reg_adc_cic_coef6_t *r = (audio_reg_adc_cic_coef6_t *)(SOC_AUDIO_REG_REG_BASE + (0xf << 2));

	SOC_LOGI("adc_cic_coef6: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0xf << 2)));
	SOC_LOGI("	reg_cic_coef6: %8x\r\n", r->reg_cic_coef6);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_adc_cic_coef7(void)
{
	audio_reg_adc_cic_coef7_t *r = (audio_reg_adc_cic_coef7_t *)(SOC_AUDIO_REG_REG_BASE + (0x10 << 2));

	SOC_LOGI("adc_cic_coef7: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x10 << 2)));
	SOC_LOGI("	reg_cic_coef7: %8x\r\n", r->reg_cic_coef7);
	SOC_LOGI("	reserved_bit_18_31: %8x\r\n", r->reserved_bit_18_31);
}

static void audio_reg_dump_mic1_dbg_ctrl(void)
{
	audio_reg_mic1_dbg_ctrl_t *r = (audio_reg_mic1_dbg_ctrl_t *)(SOC_AUDIO_REG_REG_BASE + (0x11 << 2));

	SOC_LOGI("mic1_dbg_ctrl: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x11 << 2)));
	SOC_LOGI("	reserved_bit_0_3: %8x\r\n", r->reserved_bit_0_3);
	SOC_LOGI("	spk2mic_dbg_en: %8x\r\n", r->spk2mic_dbg_en);
	SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
	SOC_LOGI("	dac_cfg_anc_add: %8x\r\n", r->dac_cfg_anc_add);
	SOC_LOGI("	dac_cfg_dac_add: %8x\r\n", r->dac_cfg_dac_add);
	SOC_LOGI("	reserved_bit_22_31: %8x\r\n", r->reserved_bit_22_31);
}

static void audio_reg_dump_buf_ctrl(void)
{
	audio_reg_buf_ctrl_t *r = (audio_reg_buf_ctrl_t *)(SOC_AUDIO_REG_REG_BASE + (0x12 << 2));

	SOC_LOGI("buf_ctrl: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x12 << 2)));
	SOC_LOGI("	en_spk0: %8x\r\n", r->en_spk0);
	SOC_LOGI("	en_spk1: %8x\r\n", r->en_spk1);
	SOC_LOGI("	en_mic: %8x\r\n", r->en_mic);
	SOC_LOGI("	reserved_bit_8_10: %8x\r\n", r->reserved_bit_8_10);
	SOC_LOGI("	dma_mask_spk0: %8x\r\n", r->dma_mask_spk0);
	SOC_LOGI("	dma_mask_spk1: %8x\r\n", r->dma_mask_spk1);
	SOC_LOGI("	dma_mask_mic: %8x\r\n", r->dma_mask_mic);
	SOC_LOGI("	reserved_bit_19_31: %8x\r\n", r->reserved_bit_19_31);
}

static void audio_reg_dump_mic_fifo_cfg(void)
{
	audio_reg_mic_fifo_cfg_t *r = (audio_reg_mic_fifo_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x13 << 2));

	SOC_LOGI("mic_fifo_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x13 << 2)));
	SOC_LOGI("	mic0_wr_thrd: %8x\r\n", r->mic0_wr_thrd);
	SOC_LOGI("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGI("	mic0_rd_thrd: %8x\r\n", r->mic0_rd_thrd);
	SOC_LOGI("	mic1_wr_thrd: %8x\r\n", r->mic1_wr_thrd);
	SOC_LOGI("	reserved_bit_19_19: %8x\r\n", r->reserved_bit_19_19);
	SOC_LOGI("	mic1_rd_thrd: %8x\r\n", r->mic1_rd_thrd);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_spk0_fifo_cfg(void)
{
	audio_reg_spk0_fifo_cfg_t *r = (audio_reg_spk0_fifo_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x14 << 2));

	SOC_LOGI("spk0_fifo_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x14 << 2)));
	SOC_LOGI("	spk0_hint_wr_thrd: %8x\r\n", r->spk0_hint_wr_thrd);
	SOC_LOGI("	spk0_hint_rd_thrd: %8x\r\n", r->spk0_hint_rd_thrd);
	SOC_LOGI("	spk0_call_wr_thrd: %8x\r\n", r->spk0_call_wr_thrd);
	SOC_LOGI("	spk0_call_rd_thrd: %8x\r\n", r->spk0_call_rd_thrd);
	SOC_LOGI("	spk0_a2dp_wr_thrd: %8x\r\n", r->spk0_a2dp_wr_thrd);
	SOC_LOGI("	spk0_a2dp_rd_thrd: %8x\r\n", r->spk0_a2dp_rd_thrd);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_spk1_fifo_cfg(void)
{
	audio_reg_spk1_fifo_cfg_t *r = (audio_reg_spk1_fifo_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x15 << 2));

	SOC_LOGI("spk1_fifo_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x15 << 2)));
	SOC_LOGI("	spk1_hint_wr_thrd: %8x\r\n", r->spk1_hint_wr_thrd);
	SOC_LOGI("	spk1_hint_rd_thrd: %8x\r\n", r->spk1_hint_rd_thrd);
	SOC_LOGI("	spk1_call_wr_thrd: %8x\r\n", r->spk1_call_wr_thrd);
	SOC_LOGI("	spk1_call_rd_thrd: %8x\r\n", r->spk1_call_rd_thrd);
	SOC_LOGI("	spk1_a2dp_wr_thrd: %8x\r\n", r->spk1_a2dp_wr_thrd);
	SOC_LOGI("	spk1_a2dp_rd_thrd: %8x\r\n", r->spk1_a2dp_rd_thrd);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_adc_cut_cfg(void)
{
	audio_reg_adc_cut_cfg_t *r = (audio_reg_adc_cut_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x16 << 2));

	SOC_LOGI("adc_cut_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x16 << 2)));
	SOC_LOGI("	adc_cut0: %8x\r\n", r->adc_cut0);
	SOC_LOGI("	adc_cut1: %8x\r\n", r->adc_cut1);
	SOC_LOGI("	adc_cut2: %8x\r\n", r->adc_cut2);
	SOC_LOGI("	adc_cut3: %8x\r\n", r->adc_cut3);
	SOC_LOGI("	adc_cut4: %8x\r\n", r->adc_cut4);
	SOC_LOGI("	reserved_bit_20_31: %8x\r\n", r->reserved_bit_20_31);
}

static void audio_reg_dump_iir_sft_cfg(void)
{
	audio_reg_iir_sft_cfg_t *r = (audio_reg_iir_sft_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x17 << 2));

	SOC_LOGI("iir_sft_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x17 << 2)));
	SOC_LOGI("	dac_eq_sft_r_sel_0: %8x\r\n", r->dac_eq_sft_r_sel_0);
	SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGI("	dac_eq_sft_r_sel_1: %8x\r\n", r->dac_eq_sft_r_sel_1);
	SOC_LOGI("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGI("	dac_eq_sft_l_sel_0: %8x\r\n", r->dac_eq_sft_l_sel_0);
	SOC_LOGI("	reserved_bit_11_11: %8x\r\n", r->reserved_bit_11_11);
	SOC_LOGI("	dac_eq_sft_l_sel_1: %8x\r\n", r->dac_eq_sft_l_sel_1);
	SOC_LOGI("	reserved_bit_15_15: %8x\r\n", r->reserved_bit_15_15);
	SOC_LOGI("	comp_eq_sft_r_sel_0: %8x\r\n", r->comp_eq_sft_r_sel_0);
	SOC_LOGI("	reserved_bit_19_19: %8x\r\n", r->reserved_bit_19_19);
	SOC_LOGI("	comp_eq_sft_r_sel_1: %8x\r\n", r->comp_eq_sft_r_sel_1);
	SOC_LOGI("	reserved_bit_23_23: %8x\r\n", r->reserved_bit_23_23);
	SOC_LOGI("	comp_eq_sft_l_sel_0: %8x\r\n", r->comp_eq_sft_l_sel_0);
	SOC_LOGI("	reserved_bit_27_27: %8x\r\n", r->reserved_bit_27_27);
	SOC_LOGI("	comp_eq_sft_l_sel_1: %8x\r\n", r->comp_eq_sft_l_sel_1);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_cfg1(void)
{
	audio_reg_dac_cfg1_t *r = (audio_reg_dac_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x18 << 2));

	SOC_LOGI("dac_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x18 << 2)));
	SOC_LOGI("	dac_pn_conf: %8x\r\n", r->dac_pn_conf);
	SOC_LOGI("	notchen: %8x\r\n", r->notchen);
	SOC_LOGI("	sw_board: %8x\r\n", r->sw_board);
	SOC_LOGI("	reserved_bit_9_9: %8x\r\n", r->reserved_bit_9_9);
	SOC_LOGI("	cfg_dac_wait_cnt: %8x\r\n", r->cfg_dac_wait_cnt);
	SOC_LOGI("	reserved_bit_15_15: %8x\r\n", r->reserved_bit_15_15);
	SOC_LOGI("	dac_eq_bps: %8x\r\n", r->dac_eq_bps);
	SOC_LOGI("	rsp_bps: %8x\r\n", r->rsp_bps);
	SOC_LOGI("	dac_frc_o: %8x\r\n", r->dac_frc_o);
	SOC_LOGI("	dac_frc_hw: %8x\r\n", r->dac_frc_hw);
	SOC_LOGI("	dac_frc_hw_mask: %8x\r\n", r->dac_frc_hw_mask);
	SOC_LOGI("	reserved_bit_30_31: %8x\r\n", r->reserved_bit_30_31);
}

static void audio_reg_dump_anc_cfg2(void)
{
	audio_reg_anc_cfg2_t *r = (audio_reg_anc_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x19 << 2));

	SOC_LOGI("anc_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x19 << 2)));
	SOC_LOGI("	anc1_sft_sel_1: %8x\r\n", r->anc1_sft_sel_1);
	SOC_LOGI("	anc1_sft_sel_2: %8x\r\n", r->anc1_sft_sel_2);
	SOC_LOGI("	anc1_rshift0: %8x\r\n", r->anc1_rshift0);
	SOC_LOGI("	anc1_rshift1: %8x\r\n", r->anc1_rshift1);
	SOC_LOGI("	reserved_bit_12_12: %8x\r\n", r->reserved_bit_12_12);
	SOC_LOGI("	up_spl_sel: %8x\r\n", r->up_spl_sel);
	SOC_LOGI("	reserved_bit_15_16: %8x\r\n", r->reserved_bit_15_16);
	SOC_LOGI("	anc2_sft_sel_1: %8x\r\n", r->anc2_sft_sel_1);
	SOC_LOGI("	anc2_sft_sel_2: %8x\r\n", r->anc2_sft_sel_2);
	SOC_LOGI("	anc2_rshift0: %8x\r\n", r->anc2_rshift0);
	SOC_LOGI("	anc2_rshift1: %8x\r\n", r->anc2_rshift1);
	SOC_LOGI("	anc_spl_sel: %8x\r\n", r->anc_spl_sel);
	SOC_LOGI("	dac_sdm_dis: %8x\r\n", r->dac_sdm_dis);
}

static void audio_reg_dump_ramp_up_cfg(void)
{
	audio_reg_ramp_up_cfg_t *r = (audio_reg_ramp_up_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x1a << 2));

	SOC_LOGI("ramp_up_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x1a << 2)));
	SOC_LOGI("	anc_comp_en: %8x\r\n", r->anc_comp_en);
	SOC_LOGI("	comp_ramp_down_trig: %8x\r\n", r->comp_ramp_down_trig);
	SOC_LOGI("	comp_ramp_cfg: %8x\r\n", r->comp_ramp_cfg);
	SOC_LOGI("	reserved_bit_7_10: %8x\r\n", r->reserved_bit_7_10);
	SOC_LOGI("	comp_ramp_bps: %8x\r\n", r->comp_ramp_bps);
	SOC_LOGI("	eq_ramp_down_trig: %8x\r\n", r->eq_ramp_down_trig);
	SOC_LOGI("	eq_ramp_cfg: %8x\r\n", r->eq_ramp_cfg);
	SOC_LOGI("	reserved_bit_18_21: %8x\r\n", r->reserved_bit_18_21);
	SOC_LOGI("	eq_ramp_bps: %8x\r\n", r->eq_ramp_bps);
	SOC_LOGI("	eq_ramp_up_trig: %8x\r\n", r->eq_ramp_up_trig);
	SOC_LOGI("	comp_ramp_up_trig: %8x\r\n", r->comp_ramp_up_trig);
	SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void audio_reg_dump_anc1_gain_cfg1(void)
{
	audio_reg_anc1_gain_cfg1_t *r = (audio_reg_anc1_gain_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x1b << 2));

	SOC_LOGI("anc1_gain_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x1b << 2)));
	SOC_LOGI("	anc1_gain1_1: %8x\r\n", r->anc1_gain1_1);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc1_gain_cfg2(void)
{
	audio_reg_anc1_gain_cfg2_t *r = (audio_reg_anc1_gain_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x1c << 2));

	SOC_LOGI("anc1_gain_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x1c << 2)));
	SOC_LOGI("	anc1_gain_comp: %8x\r\n", r->anc1_gain_comp);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc1_gain_cfg3(void)
{
	audio_reg_anc1_gain_cfg3_t *r = (audio_reg_anc1_gain_cfg3_t *)(SOC_AUDIO_REG_REG_BASE + (0x1d << 2));

	SOC_LOGI("anc1_gain_cfg3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x1d << 2)));
	SOC_LOGI("	anc1_gain0_1: %8x\r\n", r->anc1_gain0_1);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_rsv_1e_1e(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_1e_1e: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + ((0x1e + idx) << 2)));
	}
}

static void audio_reg_dump_anc1_gain_cfg4(void)
{
	audio_reg_anc1_gain_cfg4_t *r = (audio_reg_anc1_gain_cfg4_t *)(SOC_AUDIO_REG_REG_BASE + (0x1f << 2));

	SOC_LOGI("anc1_gain_cfg4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x1f << 2)));
	SOC_LOGI("	anc1_gain0_0: %8x\r\n", r->anc1_gain0_0);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_sync_ctrl1(void)
{
	audio_reg_sync_ctrl1_t *r = (audio_reg_sync_ctrl1_t *)(SOC_AUDIO_REG_REG_BASE + (0x20 << 2));

	SOC_LOGI("sync_ctrl1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x20 << 2)));
	SOC_LOGI("	sync_a2dp_cnt: %8x\r\n", r->sync_a2dp_cnt);
	SOC_LOGI("	start_a2dp_cnt: %8x\r\n", r->start_a2dp_cnt);
	SOC_LOGI("	sync_call_cnt: %8x\r\n", r->sync_call_cnt);
	SOC_LOGI("	start_call_cnt: %8x\r\n", r->start_call_cnt);
}

static void audio_reg_dump_sync_ctrl2(void)
{
	audio_reg_sync_ctrl2_t *r = (audio_reg_sync_ctrl2_t *)(SOC_AUDIO_REG_REG_BASE + (0x21 << 2));

	SOC_LOGI("sync_ctrl2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x21 << 2)));
	SOC_LOGI("	sync_hint_cnt: %8x\r\n", r->sync_hint_cnt);
	SOC_LOGI("	start_hint_cnt: %8x\r\n", r->start_hint_cnt);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_rsv_22_23(void)
{
	for (uint32_t idx = 0; idx < 2; idx++) {
		SOC_LOGI("rsv_22_23: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + ((0x22 + idx) << 2)));
	}
}

static void audio_reg_dump_dac_ro_sts(void)
{
	audio_reg_dac_ro_sts_t *r = (audio_reg_dac_ro_sts_t *)(SOC_AUDIO_REG_REG_BASE + (0x24 << 2));

	SOC_LOGI("dac_ro_sts: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x24 << 2)));
	SOC_LOGI("	dac_fifo_status: %8x\r\n", r->dac_fifo_status);
	SOC_LOGI("	reserved_bit_12_30: %8x\r\n", r->reserved_bit_12_30);
	SOC_LOGI("	mem_auto_init_done: %8x\r\n", r->mem_auto_init_done);
}

static void audio_reg_dump_adc_ro_sts(void)
{
	audio_reg_adc_ro_sts_t *r = (audio_reg_adc_ro_sts_t *)(SOC_AUDIO_REG_REG_BASE + (0x25 << 2));

	SOC_LOGI("adc_ro_sts: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x25 << 2)));
	SOC_LOGI("	adc_fifo_status: %8x\r\n", r->adc_fifo_status);
	SOC_LOGI("	reserved_bit_4_31: %8x\r\n", r->reserved_bit_4_31);
}

static void audio_reg_dump_anc_sts(void)
{
	SOC_LOGI("anc_sts: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x26 << 2)));
}

static void audio_reg_dump_ramp_intr_ctrl(void)
{
	audio_reg_ramp_intr_ctrl_t *r = (audio_reg_ramp_intr_ctrl_t *)(SOC_AUDIO_REG_REG_BASE + (0x27 << 2));

	SOC_LOGI("ramp_intr_ctrl: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x27 << 2)));
	SOC_LOGI("	ramp_interrupt_mask: %8x\r\n", r->ramp_interrupt_mask);
	SOC_LOGI("	iir_ovl_int_mask: %8x\r\n", r->iir_ovl_int_mask);
	SOC_LOGI("	ramp_interrupt_clr: %8x\r\n", r->ramp_interrupt_clr);
	SOC_LOGI("	iir_ovl_int_clr: %8x\r\n", r->iir_ovl_int_clr);
}

static void audio_reg_dump_aud_int_ctrl(void)
{
	audio_reg_aud_int_ctrl_t *r = (audio_reg_aud_int_ctrl_t *)(SOC_AUDIO_REG_REG_BASE + (0x28 << 2));

	SOC_LOGI("aud_int_ctrl: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x28 << 2)));
	SOC_LOGI("	aud_interrupt_mask: %8x\r\n", r->aud_interrupt_mask);
	SOC_LOGI("	aud_interrupt_clr: %8x\r\n", r->aud_interrupt_clr);
}

static void audio_reg_dump_aud_int_sts(void)
{
	audio_reg_aud_int_sts_t *r = (audio_reg_aud_int_sts_t *)(SOC_AUDIO_REG_REG_BASE + (0x29 << 2));

	SOC_LOGI("aud_int_sts: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x29 << 2)));
	SOC_LOGI("	aud_interrupt_status: %8x\r\n", r->aud_interrupt_status);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc2_limit_cfg1(void)
{
	audio_reg_anc2_limit_cfg1_t *r = (audio_reg_anc2_limit_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x2a << 2));

	SOC_LOGI("anc2_limit_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2a << 2)));
	SOC_LOGI("	anc2_limit_val1: %8x\r\n", r->anc2_limit_val1);
	SOC_LOGI("	reserved_bit_24_30: %8x\r\n", r->reserved_bit_24_30);
	SOC_LOGI("	anc2_limit_bps1: %8x\r\n", r->anc2_limit_bps1);
}

static void audio_reg_dump_anc2_limit_cfg2(void)
{
	audio_reg_anc2_limit_cfg2_t *r = (audio_reg_anc2_limit_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x2b << 2));

	SOC_LOGI("anc2_limit_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2b << 2)));
	SOC_LOGI("	anc2_limit_val2: %8x\r\n", r->anc2_limit_val2);
	SOC_LOGI("	reserved_bit_24_30: %8x\r\n", r->reserved_bit_24_30);
	SOC_LOGI("	anc2_limit_bps2: %8x\r\n", r->anc2_limit_bps2);
}

static void audio_reg_dump_anc1_limit_cfg1(void)
{
	audio_reg_anc1_limit_cfg1_t *r = (audio_reg_anc1_limit_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x2c << 2));

	SOC_LOGI("anc1_limit_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2c << 2)));
	SOC_LOGI("	anc1_limit_val1: %8x\r\n", r->anc1_limit_val1);
	SOC_LOGI("	reserved_bit_24_30: %8x\r\n", r->reserved_bit_24_30);
	SOC_LOGI("	anc1_limit_bps1: %8x\r\n", r->anc1_limit_bps1);
}

static void audio_reg_dump_anc1_limit_cfg2(void)
{
	audio_reg_anc1_limit_cfg2_t *r = (audio_reg_anc1_limit_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x2d << 2));

	SOC_LOGI("anc1_limit_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2d << 2)));
	SOC_LOGI("	anc1_limit_val2: %8x\r\n", r->anc1_limit_val2);
	SOC_LOGI("	reserved_bit_24_30: %8x\r\n", r->reserved_bit_24_30);
	SOC_LOGI("	anc1_limit_bps2: %8x\r\n", r->anc1_limit_bps2);
}

static void audio_reg_dump_anc2_gain_cfg1(void)
{
	audio_reg_anc2_gain_cfg1_t *r = (audio_reg_anc2_gain_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x2e << 2));

	SOC_LOGI("anc2_gain_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2e << 2)));
	SOC_LOGI("	anc2_gain1_1: %8x\r\n", r->anc2_gain1_1);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc2_gain_cfg2(void)
{
	audio_reg_anc2_gain_cfg2_t *r = (audio_reg_anc2_gain_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x2f << 2));

	SOC_LOGI("anc2_gain_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x2f << 2)));
	SOC_LOGI("	anc2_gain_comp: %8x\r\n", r->anc2_gain_comp);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc2_gain_cfg3(void)
{
	audio_reg_anc2_gain_cfg3_t *r = (audio_reg_anc2_gain_cfg3_t *)(SOC_AUDIO_REG_REG_BASE + (0x30 << 2));

	SOC_LOGI("anc2_gain_cfg3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x30 << 2)));
	SOC_LOGI("	anc2_gain0_1: %8x\r\n", r->anc2_gain0_1);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc2_gain_cfg4(void)
{
	audio_reg_anc2_gain_cfg4_t *r = (audio_reg_anc2_gain_cfg4_t *)(SOC_AUDIO_REG_REG_BASE + (0x31 << 2));

	SOC_LOGI("anc2_gain_cfg4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x31 << 2)));
	SOC_LOGI("	anc2_gain0_0: %8x\r\n", r->anc2_gain0_0);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_anc_comp_cfg(void)
{
	audio_reg_anc_comp_cfg_t *r = (audio_reg_anc_comp_cfg_t *)(SOC_AUDIO_REG_REG_BASE + (0x32 << 2));

	SOC_LOGI("anc_comp_cfg: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x32 << 2)));
	SOC_LOGI("	comp_iir_bps: %8x\r\n", r->comp_iir_bps);
	SOC_LOGI("	dac_comp_spl: %8x\r\n", r->dac_comp_spl);
	SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
	SOC_LOGI("	anc_cfg_start_val: %8x\r\n", r->anc_cfg_start_val);
	SOC_LOGI("	reserved_bit_25_31: %8x\r\n", r->reserved_bit_25_31);
}

static void audio_reg_dump_dac_gain_cfg0(void)
{
	audio_reg_dac_gain_cfg0_t *r = (audio_reg_dac_gain_cfg0_t *)(SOC_AUDIO_REG_REG_BASE + (0x33 << 2));

	SOC_LOGI("dac_gain_cfg0: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x33 << 2)));
	SOC_LOGI("	spk0_a2dp_gain: %8x\r\n", r->spk0_a2dp_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_gain_cfg1(void)
{
	audio_reg_dac_gain_cfg1_t *r = (audio_reg_dac_gain_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x34 << 2));

	SOC_LOGI("dac_gain_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x34 << 2)));
	SOC_LOGI("	spk0_call_gain: %8x\r\n", r->spk0_call_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_gain_cfg2(void)
{
	audio_reg_dac_gain_cfg2_t *r = (audio_reg_dac_gain_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x35 << 2));

	SOC_LOGI("dac_gain_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x35 << 2)));
	SOC_LOGI("	spk0_hint_gain: %8x\r\n", r->spk0_hint_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_gain_cfg3(void)
{
	audio_reg_dac_gain_cfg3_t *r = (audio_reg_dac_gain_cfg3_t *)(SOC_AUDIO_REG_REG_BASE + (0x36 << 2));

	SOC_LOGI("dac_gain_cfg3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x36 << 2)));
	SOC_LOGI("	spk1_a2dp_gain: %8x\r\n", r->spk1_a2dp_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_gain_cfg4(void)
{
	audio_reg_dac_gain_cfg4_t *r = (audio_reg_dac_gain_cfg4_t *)(SOC_AUDIO_REG_REG_BASE + (0x37 << 2));

	SOC_LOGI("dac_gain_cfg4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x37 << 2)));
	SOC_LOGI("	spk1_call_gain: %8x\r\n", r->spk1_call_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_gain_cfg5(void)
{
	audio_reg_dac_gain_cfg5_t *r = (audio_reg_dac_gain_cfg5_t *)(SOC_AUDIO_REG_REG_BASE + (0x38 << 2));

	SOC_LOGI("dac_gain_cfg5: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x38 << 2)));
	SOC_LOGI("	spk1_hint_gain: %8x\r\n", r->spk1_hint_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_adc_gain_cfg1(void)
{
	audio_reg_adc_gain_cfg1_t *r = (audio_reg_adc_gain_cfg1_t *)(SOC_AUDIO_REG_REG_BASE + (0x39 << 2));

	SOC_LOGI("adc_gain_cfg1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x39 << 2)));
	SOC_LOGI("	adc_chn1_gain: %8x\r\n", r->adc_chn1_gain);
	SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void audio_reg_dump_dac_l_gain_mix(void)
{
	audio_reg_dac_l_gain_mix_t *r = (audio_reg_dac_l_gain_mix_t *)(SOC_AUDIO_REG_REG_BASE + (0x3a << 2));

	SOC_LOGI("dac_l_gain_mix: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x3a << 2)));
	SOC_LOGI("	dac_l_gain: %8x\r\n", r->dac_l_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_dac_r_gain_mix(void)
{
	audio_reg_dac_r_gain_mix_t *r = (audio_reg_dac_r_gain_mix_t *)(SOC_AUDIO_REG_REG_BASE + (0x3b << 2));

	SOC_LOGI("dac_r_gain_mix: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x3b << 2)));
	SOC_LOGI("	dac_r_gain: %8x\r\n", r->dac_r_gain);
	SOC_LOGI("	reserved_bit_31_31: %8x\r\n", r->reserved_bit_31_31);
}

static void audio_reg_dump_rsv_3c_3f(void)
{
	for (uint32_t idx = 0; idx < 4; idx++) {
		SOC_LOGI("rsv_3c_3f: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + ((0x3c + idx) << 2)));
	}
}

static void audio_reg_dump_adc_gain_cfg2(void)
{
	audio_reg_adc_gain_cfg2_t *r = (audio_reg_adc_gain_cfg2_t *)(SOC_AUDIO_REG_REG_BASE + (0x40 << 2));

	SOC_LOGI("adc_gain_cfg2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x40 << 2)));
	SOC_LOGI("	adc_chn0_gain: %8x\r\n", r->adc_chn0_gain);
	SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void audio_reg_dump_adc_gain_cfg3(void)
{
	audio_reg_adc_gain_cfg3_t *r = (audio_reg_adc_gain_cfg3_t *)(SOC_AUDIO_REG_REG_BASE + (0x41 << 2));

	SOC_LOGI("adc_gain_cfg3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x41 << 2)));
	SOC_LOGI("	adc_chn4_gain: %8x\r\n", r->adc_chn4_gain);
	SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void audio_reg_dump_adc_gain_cfg4(void)
{
	audio_reg_adc_gain_cfg4_t *r = (audio_reg_adc_gain_cfg4_t *)(SOC_AUDIO_REG_REG_BASE + (0x42 << 2));

	SOC_LOGI("adc_gain_cfg4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x42 << 2)));
	SOC_LOGI("	adc_chn3_gain: %8x\r\n", r->adc_chn3_gain);
	SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void audio_reg_dump_adc_gain_cfg5(void)
{
	audio_reg_adc_gain_cfg5_t *r = (audio_reg_adc_gain_cfg5_t *)(SOC_AUDIO_REG_REG_BASE + (0x43 << 2));

	SOC_LOGI("adc_gain_cfg5: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x43 << 2)));
	SOC_LOGI("	adc_chn2_gain: %8x\r\n", r->adc_chn2_gain);
	SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void audio_reg_dump_anc_sft_l_para(void)
{
	audio_reg_anc_sft_l_para_t *r = (audio_reg_anc_sft_l_para_t *)(SOC_AUDIO_REG_REG_BASE + (0x44 << 2));

	SOC_LOGI("anc_sft_l_para: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x44 << 2)));
	SOC_LOGI("	anc_if_sft_l_0: %8x\r\n", r->anc_if_sft_l_0);
	SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGI("	anc_if_sft_l_1: %8x\r\n", r->anc_if_sft_l_1);
	SOC_LOGI("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGI("	anc_if_sft_l_2: %8x\r\n", r->anc_if_sft_l_2);
	SOC_LOGI("	reserved_bit_11_11: %8x\r\n", r->reserved_bit_11_11);
	SOC_LOGI("	anc_if_sft_l_3: %8x\r\n", r->anc_if_sft_l_3);
	SOC_LOGI("	reserved_bit_15_15: %8x\r\n", r->reserved_bit_15_15);
	SOC_LOGI("	anc_if_sft_l_4: %8x\r\n", r->anc_if_sft_l_4);
	SOC_LOGI("	reserved_bit_19_19: %8x\r\n", r->reserved_bit_19_19);
	SOC_LOGI("	anc_if_sft_l_5: %8x\r\n", r->anc_if_sft_l_5);
	SOC_LOGI("	reserved_bit_23_31: %8x\r\n", r->reserved_bit_23_31);
}

static void audio_reg_dump_anc_sft_r_para(void)
{
	audio_reg_anc_sft_r_para_t *r = (audio_reg_anc_sft_r_para_t *)(SOC_AUDIO_REG_REG_BASE + (0x45 << 2));

	SOC_LOGI("anc_sft_r_para: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x45 << 2)));
	SOC_LOGI("	anc_if_sft_r_0: %8x\r\n", r->anc_if_sft_r_0);
	SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGI("	anc_if_sft_r_1: %8x\r\n", r->anc_if_sft_r_1);
	SOC_LOGI("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGI("	anc_if_sft_r_2: %8x\r\n", r->anc_if_sft_r_2);
	SOC_LOGI("	reserved_bit_11_11: %8x\r\n", r->reserved_bit_11_11);
	SOC_LOGI("	anc_if_sft_r_3: %8x\r\n", r->anc_if_sft_r_3);
	SOC_LOGI("	reserved_bit_15_15: %8x\r\n", r->reserved_bit_15_15);
	SOC_LOGI("	anc_if_sft_r_4: %8x\r\n", r->anc_if_sft_r_4);
	SOC_LOGI("	reserved_bit_19_19: %8x\r\n", r->reserved_bit_19_19);
	SOC_LOGI("	anc_if_sft_r_5: %8x\r\n", r->anc_if_sft_r_5);
	SOC_LOGI("	reserved_bit_23_31: %8x\r\n", r->reserved_bit_23_31);
}

static void audio_reg_dump_k_val_0(void)
{
	audio_reg_k_val_0_t *r = (audio_reg_k_val_0_t *)(SOC_AUDIO_REG_REG_BASE + (0x46 << 2));

	SOC_LOGI("k_val_0: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x46 << 2)));
	SOC_LOGI("	k_val_0: %8x\r\n", r->k_val_0);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_1(void)
{
	audio_reg_k_val_1_t *r = (audio_reg_k_val_1_t *)(SOC_AUDIO_REG_REG_BASE + (0x47 << 2));

	SOC_LOGI("k_val_1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x47 << 2)));
	SOC_LOGI("	k_val_1: %8x\r\n", r->k_val_1);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_2(void)
{
	audio_reg_k_val_2_t *r = (audio_reg_k_val_2_t *)(SOC_AUDIO_REG_REG_BASE + (0x48 << 2));

	SOC_LOGI("k_val_2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x48 << 2)));
	SOC_LOGI("	k_val_2: %8x\r\n", r->k_val_2);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_3(void)
{
	audio_reg_k_val_3_t *r = (audio_reg_k_val_3_t *)(SOC_AUDIO_REG_REG_BASE + (0x49 << 2));

	SOC_LOGI("k_val_3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x49 << 2)));
	SOC_LOGI("	k_val_3: %8x\r\n", r->k_val_3);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_4(void)
{
	audio_reg_k_val_4_t *r = (audio_reg_k_val_4_t *)(SOC_AUDIO_REG_REG_BASE + (0x4a << 2));

	SOC_LOGI("k_val_4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4a << 2)));
	SOC_LOGI("	k_val_4: %8x\r\n", r->k_val_4);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_5(void)
{
	audio_reg_k_val_5_t *r = (audio_reg_k_val_5_t *)(SOC_AUDIO_REG_REG_BASE + (0x4b << 2));

	SOC_LOGI("k_val_5: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4b << 2)));
	SOC_LOGI("	k_val_5: %8x\r\n", r->k_val_5);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_6(void)
{
	audio_reg_k_val_6_t *r = (audio_reg_k_val_6_t *)(SOC_AUDIO_REG_REG_BASE + (0x4c << 2));

	SOC_LOGI("k_val_6: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4c << 2)));
	SOC_LOGI("	k_val_6: %8x\r\n", r->k_val_6);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_k_val_7(void)
{
	audio_reg_k_val_7_t *r = (audio_reg_k_val_7_t *)(SOC_AUDIO_REG_REG_BASE + (0x4d << 2));

	SOC_LOGI("k_val_7: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4d << 2)));
	SOC_LOGI("	k_val_7: %8x\r\n", r->k_val_7);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void audio_reg_dump_st_val_0(void)
{
	audio_reg_st_val_0_t *r = (audio_reg_st_val_0_t *)(SOC_AUDIO_REG_REG_BASE + (0x4e << 2));

	SOC_LOGI("st_val_0: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4e << 2)));
	SOC_LOGI("	st_val_0: %8x\r\n", r->st_val_0);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_1(void)
{
	audio_reg_st_val_1_t *r = (audio_reg_st_val_1_t *)(SOC_AUDIO_REG_REG_BASE + (0x4f << 2));

	SOC_LOGI("st_val_1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x4f << 2)));
	SOC_LOGI("	st_val_1: %8x\r\n", r->st_val_1);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_2(void)
{
	audio_reg_st_val_2_t *r = (audio_reg_st_val_2_t *)(SOC_AUDIO_REG_REG_BASE + (0x50 << 2));

	SOC_LOGI("st_val_2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x50 << 2)));
	SOC_LOGI("	st_val_2: %8x\r\n", r->st_val_2);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_3(void)
{
	audio_reg_st_val_3_t *r = (audio_reg_st_val_3_t *)(SOC_AUDIO_REG_REG_BASE + (0x51 << 2));

	SOC_LOGI("st_val_3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x51 << 2)));
	SOC_LOGI("	st_val_3: %8x\r\n", r->st_val_3);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_4(void)
{
	audio_reg_st_val_4_t *r = (audio_reg_st_val_4_t *)(SOC_AUDIO_REG_REG_BASE + (0x52 << 2));

	SOC_LOGI("st_val_4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x52 << 2)));
	SOC_LOGI("	st_val_4: %8x\r\n", r->st_val_4);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_5(void)
{
	audio_reg_st_val_5_t *r = (audio_reg_st_val_5_t *)(SOC_AUDIO_REG_REG_BASE + (0x53 << 2));

	SOC_LOGI("st_val_5: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x53 << 2)));
	SOC_LOGI("	st_val_5: %8x\r\n", r->st_val_5);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_6(void)
{
	audio_reg_st_val_6_t *r = (audio_reg_st_val_6_t *)(SOC_AUDIO_REG_REG_BASE + (0x54 << 2));

	SOC_LOGI("st_val_6: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x54 << 2)));
	SOC_LOGI("	st_val_6: %8x\r\n", r->st_val_6);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_st_val_7(void)
{
	audio_reg_st_val_7_t *r = (audio_reg_st_val_7_t *)(SOC_AUDIO_REG_REG_BASE + (0x55 << 2));

	SOC_LOGI("st_val_7: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x55 << 2)));
	SOC_LOGI("	st_val_7: %8x\r\n", r->st_val_7);
	SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void audio_reg_dump_p_val_0(void)
{
	audio_reg_p_val_0_t *r = (audio_reg_p_val_0_t *)(SOC_AUDIO_REG_REG_BASE + (0x56 << 2));

	SOC_LOGI("p_val_0: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x56 << 2)));
	SOC_LOGI("	p_val_0: %8x\r\n", r->p_val_0);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_p_val_1(void)
{
	audio_reg_p_val_1_t *r = (audio_reg_p_val_1_t *)(SOC_AUDIO_REG_REG_BASE + (0x57 << 2));

	SOC_LOGI("p_val_1: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x57 << 2)));
	SOC_LOGI("	p_val_1: %8x\r\n", r->p_val_1);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_p_val_2(void)
{
	audio_reg_p_val_2_t *r = (audio_reg_p_val_2_t *)(SOC_AUDIO_REG_REG_BASE + (0x58 << 2));

	SOC_LOGI("p_val_2: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x58 << 2)));
	SOC_LOGI("	p_val_2: %8x\r\n", r->p_val_2);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_p_val_3(void)
{
	audio_reg_p_val_3_t *r = (audio_reg_p_val_3_t *)(SOC_AUDIO_REG_REG_BASE + (0x59 << 2));

	SOC_LOGI("p_val_3: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x59 << 2)));
	SOC_LOGI("	p_val_3: %8x\r\n", r->p_val_3);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_p_val_4(void)
{
	audio_reg_p_val_4_t *r = (audio_reg_p_val_4_t *)(SOC_AUDIO_REG_REG_BASE + (0x5a << 2));

	SOC_LOGI("p_val_4: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x5a << 2)));
	SOC_LOGI("	p_val_4: %8x\r\n", r->p_val_4);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_p_val_5(void)
{
	audio_reg_p_val_5_t *r = (audio_reg_p_val_5_t *)(SOC_AUDIO_REG_REG_BASE + (0x5b << 2));

	SOC_LOGI("p_val_5: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x5b << 2)));
	SOC_LOGI("	p_val_5: %8x\r\n", r->p_val_5);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_p_val_6(void)
{
	audio_reg_p_val_6_t *r = (audio_reg_p_val_6_t *)(SOC_AUDIO_REG_REG_BASE + (0x5c << 2));

	SOC_LOGI("p_val_6: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x5c << 2)));
	SOC_LOGI("	p_val_6: %8x\r\n", r->p_val_6);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void audio_reg_dump_anc_iir_bps(void)
{
	audio_reg_anc_iir_bps_t *r = (audio_reg_anc_iir_bps_t *)(SOC_AUDIO_REG_REG_BASE + (0x5d << 2));

	SOC_LOGI("anc_iir_bps: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x5d << 2)));
	SOC_LOGI("	anc_iir_bps0: %8x\r\n", r->anc_iir_bps0);
	SOC_LOGI("	anc_iir_bps1: %8x\r\n", r->anc_iir_bps1);
	SOC_LOGI("	anc_iir_bps2: %8x\r\n", r->anc_iir_bps2);
	SOC_LOGI("	reserved_bit_30_31: %8x\r\n", r->reserved_bit_30_31);
}

static void audio_reg_dump_interface_matrix(void)
{
	audio_reg_interface_matrix_t *r = (audio_reg_interface_matrix_t *)(SOC_AUDIO_REG_REG_BASE + (0x5e << 2));

	SOC_LOGI("interface_matrix: %8x\r\n", REG_READ(SOC_AUDIO_REG_REG_BASE + (0x5e << 2)));
	SOC_LOGI("	adc_chn0_sel: %8x\r\n", r->adc_chn0_sel);
	SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
	SOC_LOGI("	adc_chn1_sel: %8x\r\n", r->adc_chn1_sel);
	SOC_LOGI("	reserved_bit_7_7: %8x\r\n", r->reserved_bit_7_7);
	SOC_LOGI("	adc_chn2_sel: %8x\r\n", r->adc_chn2_sel);
	SOC_LOGI("	reserved_bit_11_11: %8x\r\n", r->reserved_bit_11_11);
	SOC_LOGI("	adc_chn3_sel: %8x\r\n", r->adc_chn3_sel);
	SOC_LOGI("	reserved_bit_15_15: %8x\r\n", r->reserved_bit_15_15);
	SOC_LOGI("	adc_chn4_sel: %8x\r\n", r->adc_chn4_sel);
	SOC_LOGI("	reserved_bit_19_19: %8x\r\n", r->reserved_bit_19_19);
	SOC_LOGI("	dac_r_chn_sel: %8x\r\n", r->dac_r_chn_sel);
	SOC_LOGI("	dac_l_chn_sel: %8x\r\n", r->dac_l_chn_sel);
	SOC_LOGI("	clk_frc_on: %8x\r\n", r->clk_frc_on);
	SOC_LOGI("	reserved_bit_27_31: %8x\r\n", r->reserved_bit_27_31);
}

static audio_reg_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, audio_reg_dump_device_id},
	{0x1, 0x1, audio_reg_dump_version_id},
	{0x2, 0x2, audio_reg_dump_reserved0},
	{0x3, 0x3, audio_reg_dump_reserved1},
	{0x4, 0x4, audio_reg_dump_sys_cfg},
	{0x5, 0x5, audio_reg_dump_a2dp_comp},
	{0x6, 0x6, audio_reg_dump_adc_cfg},
	{0x7, 0x7, audio_reg_dump_anc_cfg},
	{0x8, 0x8, audio_reg_dump_dac_cfg},
	{0x9, 0x9, audio_reg_dump_adc_cic_coef0},
	{0xa, 0xa, audio_reg_dump_adc_cic_coef1},
	{0xb, 0xb, audio_reg_dump_adc_cic_coef2},
	{0xc, 0xc, audio_reg_dump_adc_cic_coef3},
	{0xd, 0xd, audio_reg_dump_adc_cic_coef4},
	{0xe, 0xe, audio_reg_dump_adc_cic_coef5},
	{0xf, 0xf, audio_reg_dump_adc_cic_coef6},
	{0x10, 0x10, audio_reg_dump_adc_cic_coef7},
	{0x11, 0x11, audio_reg_dump_mic1_dbg_ctrl},
	{0x12, 0x12, audio_reg_dump_buf_ctrl},
	{0x13, 0x13, audio_reg_dump_mic_fifo_cfg},
	{0x14, 0x14, audio_reg_dump_spk0_fifo_cfg},
	{0x15, 0x15, audio_reg_dump_spk1_fifo_cfg},
	{0x16, 0x16, audio_reg_dump_adc_cut_cfg},
	{0x17, 0x17, audio_reg_dump_iir_sft_cfg},
	{0x18, 0x18, audio_reg_dump_dac_cfg1},
	{0x19, 0x19, audio_reg_dump_anc_cfg2},
	{0x1a, 0x1a, audio_reg_dump_ramp_up_cfg},
	{0x1b, 0x1b, audio_reg_dump_anc1_gain_cfg1},
	{0x1c, 0x1c, audio_reg_dump_anc1_gain_cfg2},
	{0x1d, 0x1d, audio_reg_dump_anc1_gain_cfg3},
	{0x1e, 0x1f, audio_reg_dump_rsv_1e_1e},
	{0x1f, 0x1f, audio_reg_dump_anc1_gain_cfg4},
	{0x20, 0x20, audio_reg_dump_sync_ctrl1},
	{0x21, 0x21, audio_reg_dump_sync_ctrl2},
	{0x22, 0x24, audio_reg_dump_rsv_22_23},
	{0x24, 0x24, audio_reg_dump_dac_ro_sts},
	{0x25, 0x25, audio_reg_dump_adc_ro_sts},
	{0x26, 0x26, audio_reg_dump_anc_sts},
	{0x27, 0x27, audio_reg_dump_ramp_intr_ctrl},
	{0x28, 0x28, audio_reg_dump_aud_int_ctrl},
	{0x29, 0x29, audio_reg_dump_aud_int_sts},
	{0x2a, 0x2a, audio_reg_dump_anc2_limit_cfg1},
	{0x2b, 0x2b, audio_reg_dump_anc2_limit_cfg2},
	{0x2c, 0x2c, audio_reg_dump_anc1_limit_cfg1},
	{0x2d, 0x2d, audio_reg_dump_anc1_limit_cfg2},
	{0x2e, 0x2e, audio_reg_dump_anc2_gain_cfg1},
	{0x2f, 0x2f, audio_reg_dump_anc2_gain_cfg2},
	{0x30, 0x30, audio_reg_dump_anc2_gain_cfg3},
	{0x31, 0x31, audio_reg_dump_anc2_gain_cfg4},
	{0x32, 0x32, audio_reg_dump_anc_comp_cfg},
	{0x33, 0x33, audio_reg_dump_dac_gain_cfg0},
	{0x34, 0x34, audio_reg_dump_dac_gain_cfg1},
	{0x35, 0x35, audio_reg_dump_dac_gain_cfg2},
	{0x36, 0x36, audio_reg_dump_dac_gain_cfg3},
	{0x37, 0x37, audio_reg_dump_dac_gain_cfg4},
	{0x38, 0x38, audio_reg_dump_dac_gain_cfg5},
	{0x39, 0x39, audio_reg_dump_adc_gain_cfg1},
	{0x3a, 0x3a, audio_reg_dump_dac_l_gain_mix},
	{0x3b, 0x3b, audio_reg_dump_dac_r_gain_mix},
	{0x3c, 0x40, audio_reg_dump_rsv_3c_3f},
	{0x40, 0x40, audio_reg_dump_adc_gain_cfg2},
	{0x41, 0x41, audio_reg_dump_adc_gain_cfg3},
	{0x42, 0x42, audio_reg_dump_adc_gain_cfg4},
	{0x43, 0x43, audio_reg_dump_adc_gain_cfg5},
	{0x44, 0x44, audio_reg_dump_anc_sft_l_para},
	{0x45, 0x45, audio_reg_dump_anc_sft_r_para},
	{0x46, 0x46, audio_reg_dump_k_val_0},
	{0x47, 0x47, audio_reg_dump_k_val_1},
	{0x48, 0x48, audio_reg_dump_k_val_2},
	{0x49, 0x49, audio_reg_dump_k_val_3},
	{0x4a, 0x4a, audio_reg_dump_k_val_4},
	{0x4b, 0x4b, audio_reg_dump_k_val_5},
	{0x4c, 0x4c, audio_reg_dump_k_val_6},
	{0x4d, 0x4d, audio_reg_dump_k_val_7},
	{0x4e, 0x4e, audio_reg_dump_st_val_0},
	{0x4f, 0x4f, audio_reg_dump_st_val_1},
	{0x50, 0x50, audio_reg_dump_st_val_2},
	{0x51, 0x51, audio_reg_dump_st_val_3},
	{0x52, 0x52, audio_reg_dump_st_val_4},
	{0x53, 0x53, audio_reg_dump_st_val_5},
	{0x54, 0x54, audio_reg_dump_st_val_6},
	{0x55, 0x55, audio_reg_dump_st_val_7},
	{0x56, 0x56, audio_reg_dump_p_val_0},
	{0x57, 0x57, audio_reg_dump_p_val_1},
	{0x58, 0x58, audio_reg_dump_p_val_2},
	{0x59, 0x59, audio_reg_dump_p_val_3},
	{0x5a, 0x5a, audio_reg_dump_p_val_4},
	{0x5b, 0x5b, audio_reg_dump_p_val_5},
	{0x5c, 0x5c, audio_reg_dump_p_val_6},
	{0x5d, 0x5d, audio_reg_dump_anc_iir_bps},
	{0x5e, 0x5e, audio_reg_dump_interface_matrix},
	{-1, -1, 0}
};

void audio_reg_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
