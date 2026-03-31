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

#ifdef __cplusplus
extern "C" {
#endif


typedef volatile union {
	struct {
		uint32_t device_id                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} audio_reg_device_id_t;


typedef volatile union {
	struct {
		uint32_t version_id               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} audio_reg_version_id_t;


typedef volatile union {
	struct {
		uint32_t reserved0                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} audio_reg_reserved0_t;


typedef volatile union {
	struct {
		uint32_t reserved1                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} audio_reg_reserved1_t;


typedef volatile union {
	struct {
		uint32_t digmic_en                :  5; /**<bit[0 : 4] */
		uint32_t dmic_sel                 :  3; /**<bit[5 : 7] */
		uint32_t spl_sel_adc              :  2; /**<bit[8 : 9] */
		uint32_t reserved_bit_10_17       :  8; /**<bit[10 : 17] */
		uint32_t mem_dac_iir1x_sw_init    :  1; /**<bit[18 : 18] */
		uint32_t mem_auto_init_trig       :  1; /**<bit[19 : 19] */
		uint32_t mem_dac_sw_init          :  1; /**<bit[20 : 20] */
		uint32_t mem_mic_sw_init          :  1; /**<bit[21 : 21] */
		uint32_t mem_anc_sw_init          :  1; /**<bit[22 : 22] */
		uint32_t clk_mic_sel              :  2; /**<bit[23 : 24] */
		uint32_t rx_sp_sel                :  4; /**<bit[25 : 28] */
		uint32_t mem_dac_eq_sw_init       :  1; /**<bit[29 : 29] */
		uint32_t mem_dac_comp_sw_init     :  1; /**<bit[30 : 30] */
		uint32_t apb_clk_en_dis           :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_sys_cfg_t;


typedef volatile union {
	struct {
		uint32_t anc_comp_st_val          : 10; /**<bit[0 : 9] */
		uint32_t reserved_bit_10_30       : 21; /**<bit[10 : 30] */
		uint32_t anc_comp_en_frc          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_a2dp_comp_t;


typedef volatile union {
	struct {
		uint32_t aec_en                   :  2; /**<bit[0 : 1] */
		uint32_t reserved_bit_2_2         :  1; /**<bit[2 : 2] */
		uint32_t adc_16b_sel              :  5; /**<bit[3 : 7] */
		uint32_t aec_16b_sel              :  2; /**<bit[8 : 9] */
		uint32_t reserved_bit_10_11       :  2; /**<bit[10 : 11] */
		uint32_t adc_en                   :  5; /**<bit[12 : 16] */
		uint32_t reserved_bit_17_24       :  8; /**<bit[17 : 24] */
		uint32_t adc_lpf_bps1             :  1; /**<bit[25 : 25] */
		uint32_t adc_lpf_bps2             :  1; /**<bit[26 : 26] */
		uint32_t adc_lpf_bps3             :  1; /**<bit[27 : 27] */
		uint32_t adc_hpf_bps              :  1; /**<bit[28 : 28] */
		uint32_t reserved_bit_29_29       :  1; /**<bit[29 : 29] */
		uint32_t clk_adc_inv              :  1; /**<bit[30 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cfg_t;


typedef volatile union {
	struct {
		uint32_t anc_frc_on               :  1; /**<bit[0 : 0] */
		uint32_t anc0_ramp_bps            :  3; /**<bit[1 : 3] */
		uint32_t anc1_ramp_bps            :  3; /**<bit[4 : 6] */
		uint32_t anc0_ramp_down_trig      :  3; /**<bit[7 : 9] */
		uint32_t anc1_ramp_down_trig      :  3; /**<bit[10 : 12] */
		uint32_t anc_ramp_cfg             :  3; /**<bit[13 : 15] */
		uint32_t anc_cic_setp_3           :  1; /**<bit[16 : 16] */
		uint32_t dac_cic_step_2           :  1; /**<bit[17 : 17] */
		uint32_t anc0_ramp_up_trig        :  3; /**<bit[18 : 20] */
		uint32_t anc1_ramp_up_trig        :  3; /**<bit[21 : 23] */
		uint32_t reserved_bit_24_25       :  2; /**<bit[24 : 25] */
		uint32_t anc_en0                  :  3; /**<bit[26 : 28] */
		uint32_t anc_en1                  :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} audio_reg_anc_cfg_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_0             :  1; /**<bit[0 : 0] */
		uint32_t dac_enable_l             :  1; /**<bit[1 : 1] */
		uint32_t dac_enable_r             :  1; /**<bit[2 : 2] */
		uint32_t dac_iir_bps              :  1; /**<bit[3 : 3] */
		uint32_t dac_lpf_bps1             :  1; /**<bit[4 : 4] */
		uint32_t dac_lpf_bps2             :  1; /**<bit[5 : 5] */
		uint32_t dac_lpf_bps3             :  1; /**<bit[6 : 6] */
		uint32_t dac_tx_anc_d2            :  2; /**<bit[7 : 8] */
		uint32_t dac_16b_sel              :  3; /**<bit[9 : 11] */
		uint32_t reserved_bit_12_12       :  1; /**<bit[12 : 12] */
		uint32_t dac_spl_sel              :  1; /**<bit[13 : 13] */
		uint32_t reserved_bit_14_14       :  1; /**<bit[14 : 14] */
		uint32_t dac_hpf_bps              :  1; /**<bit[15 : 15] */
		uint32_t stereo_en                :  3; /**<bit[16 : 18] */
		uint32_t drc_bypass               :  1; /**<bit[19 : 19] */
		uint32_t reserved_bit_20_21       :  2; /**<bit[20 : 21] */
		uint32_t spk2mic_tst              :  1; /**<bit[22 : 22] */
		uint32_t mono_sel                 :  3; /**<bit[23 : 25] */
		uint32_t hint_spl_sel             :  2; /**<bit[26 : 27] */
		uint32_t call_spl_sel             :  2; /**<bit[28 : 29] */
		uint32_t dith_en                  :  1; /**<bit[30 : 30] */
		uint32_t clk_dac_inv              :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_cfg_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef0            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef0_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef1            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef1_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef2            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef2_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef3            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef3_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef4            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef4_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef5            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef5_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef6            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef6_t;


typedef volatile union {
	struct {
		uint32_t reg_cic_coef7            : 18; /**<bit[0 : 17] */
		uint32_t reserved_bit_18_31       : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cic_coef7_t;


typedef volatile union {
	struct {
		uint32_t reserved_bit_0_3         :  4; /**<bit[0 : 3] */
		uint32_t spk2mic_dbg_en           :  8; /**<bit[4 : 11] */
		uint32_t reserved_bit_12_15       :  4; /**<bit[12 : 15] */
		uint32_t dac_cfg_anc_add          :  4; /**<bit[16 : 19] */
		uint32_t dac_cfg_dac_add          :  2; /**<bit[20 : 21] */
		uint32_t reserved_bit_22_31       : 10; /**<bit[22 : 31] */
	};
	uint32_t v;
} audio_reg_mic1_dbg_ctrl_t;


typedef volatile union {
	struct {
		uint32_t en_spk0                  :  3; /**<bit[0 : 2] */
		uint32_t en_spk1                  :  3; /**<bit[3 : 5] */
		uint32_t en_mic                   :  2; /**<bit[6 : 7] */
		uint32_t reserved_bit_8_10        :  3; /**<bit[8 : 10] */
		uint32_t dma_mask_spk0            :  3; /**<bit[11 : 13] */
		uint32_t dma_mask_spk1            :  3; /**<bit[14 : 16] */
		uint32_t dma_mask_mic             :  2; /**<bit[17 : 18] */
		uint32_t reserved_bit_19_31       : 13; /**<bit[19 : 31] */
	};
	uint32_t v;
} audio_reg_buf_ctrl_t;


typedef volatile union {
	struct {
		uint32_t mic0_wr_thrd             :  7; /**<bit[0 : 6] */
		uint32_t reserved_bit_7_7         :  1; /**<bit[7 : 7] */
		uint32_t mic0_rd_thrd             :  7; /**<bit[8 : 14] */
		uint32_t mic1_wr_thrd             :  4; /**<bit[15 : 18] */
		uint32_t reserved_bit_19_19       :  1; /**<bit[19 : 19] */
		uint32_t mic1_rd_thrd             :  4; /**<bit[20 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_mic_fifo_cfg_t;


typedef volatile union {
	struct {
		uint32_t spk0_hint_wr_thrd        :  4; /**<bit[0 : 3] */
		uint32_t spk0_hint_rd_thrd        :  4; /**<bit[4 : 7] */
		uint32_t spk0_call_wr_thrd        :  4; /**<bit[8 : 11] */
		uint32_t spk0_call_rd_thrd        :  4; /**<bit[12 : 15] */
		uint32_t spk0_a2dp_wr_thrd        :  4; /**<bit[16 : 19] */
		uint32_t spk0_a2dp_rd_thrd        :  4; /**<bit[20 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_spk0_fifo_cfg_t;


typedef volatile union {
	struct {
		uint32_t spk1_hint_wr_thrd        :  4; /**<bit[0 : 3] */
		uint32_t spk1_hint_rd_thrd        :  4; /**<bit[4 : 7] */
		uint32_t spk1_call_wr_thrd        :  4; /**<bit[8 : 11] */
		uint32_t spk1_call_rd_thrd        :  4; /**<bit[12 : 15] */
		uint32_t spk1_a2dp_wr_thrd        :  4; /**<bit[16 : 19] */
		uint32_t spk1_a2dp_rd_thrd        :  4; /**<bit[20 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_spk1_fifo_cfg_t;


typedef volatile union {
	struct {
		uint32_t adc_cut0                 :  4; /**<bit[0 : 3] */
		uint32_t adc_cut1                 :  4; /**<bit[4 : 7] */
		uint32_t adc_cut2                 :  4; /**<bit[8 : 11] */
		uint32_t adc_cut3                 :  4; /**<bit[12 : 15] */
		uint32_t adc_cut4                 :  4; /**<bit[16 : 19] */
		uint32_t reserved_bit_20_31       : 12; /**<bit[20 : 31] */
	};
	uint32_t v;
} audio_reg_adc_cut_cfg_t;


typedef volatile union {
	struct {
		uint32_t dac_eq_sft_r_sel_0       :  3; /**<bit[0 : 2] */
		uint32_t reserved_bit_3_3         :  1; /**<bit[3 : 3] */
		uint32_t dac_eq_sft_r_sel_1       :  3; /**<bit[4 : 6] */
		uint32_t reserved_bit_7_7         :  1; /**<bit[7 : 7] */
		uint32_t dac_eq_sft_l_sel_0       :  3; /**<bit[8 : 10] */
		uint32_t reserved_bit_11_11       :  1; /**<bit[11 : 11] */
		uint32_t dac_eq_sft_l_sel_1       :  3; /**<bit[12 : 14] */
		uint32_t reserved_bit_15_15       :  1; /**<bit[15 : 15] */
		uint32_t comp_eq_sft_r_sel_0      :  3; /**<bit[16 : 18] */
		uint32_t reserved_bit_19_19       :  1; /**<bit[19 : 19] */
		uint32_t comp_eq_sft_r_sel_1      :  3; /**<bit[20 : 22] */
		uint32_t reserved_bit_23_23       :  1; /**<bit[23 : 23] */
		uint32_t comp_eq_sft_l_sel_0      :  3; /**<bit[24 : 26] */
		uint32_t reserved_bit_27_27       :  1; /**<bit[27 : 27] */
		uint32_t comp_eq_sft_l_sel_1      :  3; /**<bit[28 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_iir_sft_cfg_t;


typedef volatile union {
	struct {
		uint32_t dac_pn_conf              :  4; /**<bit[0 : 3] */
		uint32_t notchen                  :  1; /**<bit[4 : 4] */
		uint32_t sw_board                 :  4; /**<bit[5 : 8] */
		uint32_t reserved_bit_9_9         :  1; /**<bit[9 : 9] */
		uint32_t cfg_dac_wait_cnt         :  5; /**<bit[10 : 14] */
		uint32_t reserved_bit_15_15       :  1; /**<bit[15 : 15] */
		uint32_t dac_eq_bps               : 10; /**<bit[16 : 25] */
		uint32_t rsp_bps                  :  1; /**<bit[26 : 26] */
		uint32_t dac_frc_o                :  1; /**<bit[27 : 27] */
		uint32_t dac_frc_hw               :  1; /**<bit[28 : 28] */
		uint32_t dac_frc_hw_mask          :  1; /**<bit[29 : 29] */
		uint32_t reserved_bit_30_31       :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} audio_reg_dac_cfg1_t;


typedef volatile union {
	struct {
		uint32_t anc1_sft_sel_1           :  3; /**<bit[0 : 2] */
		uint32_t anc1_sft_sel_2           :  3; /**<bit[3 : 5] */
		uint32_t anc1_rshift0             :  3; /**<bit[6 : 8] */
		uint32_t anc1_rshift1             :  3; /**<bit[9 : 11] */
		uint32_t reserved_bit_12_12       :  1; /**<bit[12 : 12] */
		uint32_t up_spl_sel               :  2; /**<bit[13 : 14] */
		uint32_t reserved_bit_15_16       :  2; /**<bit[15 : 16] */
		uint32_t anc2_sft_sel_1           :  3; /**<bit[17 : 19] */
		uint32_t anc2_sft_sel_2           :  3; /**<bit[20 : 22] */
		uint32_t anc2_rshift0             :  3; /**<bit[23 : 25] */
		uint32_t anc2_rshift1             :  3; /**<bit[26 : 28] */
		uint32_t anc_spl_sel              :  2; /**<bit[29 : 30] */
		uint32_t dac_sdm_dis              :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc_cfg2_t;


typedef volatile union {
	struct {
		uint32_t anc_comp_en              :  2; /**<bit[0 : 1] */
		uint32_t comp_ramp_down_trig      :  2; /**<bit[2 : 3] */
		uint32_t comp_ramp_cfg            :  3; /**<bit[4 : 6] */
		uint32_t reserved_bit_7_10        :  4; /**<bit[7 : 10] */
		uint32_t comp_ramp_bps            :  2; /**<bit[11 : 12] */
		uint32_t eq_ramp_down_trig        :  2; /**<bit[13 : 14] */
		uint32_t eq_ramp_cfg              :  3; /**<bit[15 : 17] */
		uint32_t reserved_bit_18_21       :  4; /**<bit[18 : 21] */
		uint32_t eq_ramp_bps              :  2; /**<bit[22 : 23] */
		uint32_t eq_ramp_up_trig          :  2; /**<bit[24 : 25] */
		uint32_t comp_ramp_up_trig        :  2; /**<bit[26 : 27] */
		uint32_t reserved_bit_28_31       :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} audio_reg_ramp_up_cfg_t;


typedef volatile union {
	struct {
		uint32_t anc1_gain1_1             : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc1_gain_cfg1_t;


typedef volatile union {
	struct {
		uint32_t anc1_gain_comp           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc1_gain_cfg2_t;


typedef volatile union {
	struct {
		uint32_t anc1_gain0_1             : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc1_gain_cfg3_t;


typedef volatile union {
	struct {
		uint32_t anc1_gain0_0             : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc1_gain_cfg4_t;


typedef volatile union {
	struct {
		uint32_t sync_a2dp_cnt            : 13; /**<bit[0 : 12] */
		uint32_t start_a2dp_cnt           :  3; /**<bit[13 : 15] */
		uint32_t sync_call_cnt            : 13; /**<bit[16 : 28] */
		uint32_t start_call_cnt           :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} audio_reg_sync_ctrl1_t;


typedef volatile union {
	struct {
		uint32_t sync_hint_cnt            : 13; /**<bit[0 : 12] */
		uint32_t start_hint_cnt           :  3; /**<bit[13 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_sync_ctrl2_t;


typedef volatile union {
	struct {
		uint32_t dac_fifo_status          : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_30       : 19; /**<bit[12 : 30] */
		uint32_t mem_auto_init_done       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_ro_sts_t;


typedef volatile union {
	struct {
		uint32_t adc_fifo_status          :  4; /**<bit[0 : 3] */
		uint32_t reserved_bit_4_31        : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} audio_reg_adc_ro_sts_t;


typedef volatile union {
	struct {
		uint32_t anc_status               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} audio_reg_anc_sts_t;


typedef volatile union {
	struct {
		uint32_t ramp_interrupt_mask      : 10; /**<bit[0 : 9] */
		uint32_t iir_ovl_int_mask         :  6; /**<bit[10 : 15] */
		uint32_t ramp_interrupt_clr       : 10; /**<bit[16 : 25] */
		uint32_t iir_ovl_int_clr          :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} audio_reg_ramp_intr_ctrl_t;


typedef volatile union {
	struct {
		uint32_t aud_interrupt_mask       : 16; /**<bit[0 : 15] */
		uint32_t aud_interrupt_clr        : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_aud_int_ctrl_t;


typedef volatile union {
	struct {
		uint32_t aud_interrupt_status     : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_aud_int_sts_t;


typedef volatile union {
	struct {
		uint32_t anc2_limit_val1          : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_30       :  7; /**<bit[24 : 30] */
		uint32_t anc2_limit_bps1          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc2_limit_cfg1_t;


typedef volatile union {
	struct {
		uint32_t anc2_limit_val2          : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_30       :  7; /**<bit[24 : 30] */
		uint32_t anc2_limit_bps2          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc2_limit_cfg2_t;


typedef volatile union {
	struct {
		uint32_t anc1_limit_val1          : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_30       :  7; /**<bit[24 : 30] */
		uint32_t anc1_limit_bps1          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc1_limit_cfg1_t;


typedef volatile union {
	struct {
		uint32_t anc1_limit_val2          : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_30       :  7; /**<bit[24 : 30] */
		uint32_t anc1_limit_bps2          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc1_limit_cfg2_t;


typedef volatile union {
	struct {
		uint32_t anc2_gain1_1             : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc2_gain_cfg1_t;


typedef volatile union {
	struct {
		uint32_t anc2_gain_comp           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc2_gain_cfg2_t;


typedef volatile union {
	struct {
		uint32_t anc2_gain0_1             : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc2_gain_cfg3_t;


typedef volatile union {
	struct {
		uint32_t anc2_gain0_0             : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_anc2_gain_cfg4_t;


typedef volatile union {
	struct {
		uint32_t comp_iir_bps             : 10; /**<bit[0 : 9] */
		uint32_t dac_comp_spl             :  2; /**<bit[10 : 11] */
		uint32_t reserved_bit_12_15       :  4; /**<bit[12 : 15] */
		uint32_t anc_cfg_start_val        :  9; /**<bit[16 : 24] */
		uint32_t reserved_bit_25_31       :  7; /**<bit[25 : 31] */
	};
	uint32_t v;
} audio_reg_anc_comp_cfg_t;


typedef volatile union {
	struct {
		uint32_t spk0_a2dp_gain           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_gain_cfg0_t;


typedef volatile union {
	struct {
		uint32_t spk0_call_gain           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_gain_cfg1_t;


typedef volatile union {
	struct {
		uint32_t spk0_hint_gain           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_gain_cfg2_t;


typedef volatile union {
	struct {
		uint32_t spk1_a2dp_gain           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_gain_cfg3_t;


typedef volatile union {
	struct {
		uint32_t spk1_call_gain           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_gain_cfg4_t;


typedef volatile union {
	struct {
		uint32_t spk1_hint_gain           : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_gain_cfg5_t;


typedef volatile union {
	struct {
		uint32_t adc_chn1_gain            : 17; /**<bit[0 : 16] */
		uint32_t reserved_bit_17_31       : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} audio_reg_adc_gain_cfg1_t;


typedef volatile union {
	struct {
		uint32_t dac_l_gain               : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_l_gain_mix_t;


typedef volatile union {
	struct {
		uint32_t dac_r_gain               : 31; /**<bit[0 : 30] */
		uint32_t reserved_bit_31_31       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} audio_reg_dac_r_gain_mix_t;


typedef volatile union {
	struct {
		uint32_t adc_chn0_gain            : 17; /**<bit[0 : 16] */
		uint32_t reserved_bit_17_31       : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} audio_reg_adc_gain_cfg2_t;


typedef volatile union {
	struct {
		uint32_t adc_chn4_gain            : 17; /**<bit[0 : 16] */
		uint32_t reserved_bit_17_31       : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} audio_reg_adc_gain_cfg3_t;


typedef volatile union {
	struct {
		uint32_t adc_chn3_gain            : 17; /**<bit[0 : 16] */
		uint32_t reserved_bit_17_31       : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} audio_reg_adc_gain_cfg4_t;


typedef volatile union {
	struct {
		uint32_t adc_chn2_gain            : 17; /**<bit[0 : 16] */
		uint32_t reserved_bit_17_31       : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} audio_reg_adc_gain_cfg5_t;


typedef volatile union {
	struct {
		uint32_t anc_if_sft_l_0           :  3; /**<bit[0 : 2] */
		uint32_t reserved_bit_3_3         :  1; /**<bit[3 : 3] */
		uint32_t anc_if_sft_l_1           :  3; /**<bit[4 : 6] */
		uint32_t reserved_bit_7_7         :  1; /**<bit[7 : 7] */
		uint32_t anc_if_sft_l_2           :  3; /**<bit[8 : 10] */
		uint32_t reserved_bit_11_11       :  1; /**<bit[11 : 11] */
		uint32_t anc_if_sft_l_3           :  3; /**<bit[12 : 14] */
		uint32_t reserved_bit_15_15       :  1; /**<bit[15 : 15] */
		uint32_t anc_if_sft_l_4           :  3; /**<bit[16 : 18] */
		uint32_t reserved_bit_19_19       :  1; /**<bit[19 : 19] */
		uint32_t anc_if_sft_l_5           :  3; /**<bit[20 : 22] */
		uint32_t reserved_bit_23_31       :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} audio_reg_anc_sft_l_para_t;


typedef volatile union {
	struct {
		uint32_t anc_if_sft_r_0           :  3; /**<bit[0 : 2] */
		uint32_t reserved_bit_3_3         :  1; /**<bit[3 : 3] */
		uint32_t anc_if_sft_r_1           :  3; /**<bit[4 : 6] */
		uint32_t reserved_bit_7_7         :  1; /**<bit[7 : 7] */
		uint32_t anc_if_sft_r_2           :  3; /**<bit[8 : 10] */
		uint32_t reserved_bit_11_11       :  1; /**<bit[11 : 11] */
		uint32_t anc_if_sft_r_3           :  3; /**<bit[12 : 14] */
		uint32_t reserved_bit_15_15       :  1; /**<bit[15 : 15] */
		uint32_t anc_if_sft_r_4           :  3; /**<bit[16 : 18] */
		uint32_t reserved_bit_19_19       :  1; /**<bit[19 : 19] */
		uint32_t anc_if_sft_r_5           :  3; /**<bit[20 : 22] */
		uint32_t reserved_bit_23_31       :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} audio_reg_anc_sft_r_para_t;


typedef volatile union {
	struct {
		uint32_t k_val_0                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_0_t;


typedef volatile union {
	struct {
		uint32_t k_val_1                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_1_t;


typedef volatile union {
	struct {
		uint32_t k_val_2                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_2_t;


typedef volatile union {
	struct {
		uint32_t k_val_3                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_3_t;


typedef volatile union {
	struct {
		uint32_t k_val_4                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_4_t;


typedef volatile union {
	struct {
		uint32_t k_val_5                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_5_t;


typedef volatile union {
	struct {
		uint32_t k_val_6                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_6_t;


typedef volatile union {
	struct {
		uint32_t k_val_7                  : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} audio_reg_k_val_7_t;


typedef volatile union {
	struct {
		uint32_t st_val_0                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_0_t;


typedef volatile union {
	struct {
		uint32_t st_val_1                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_1_t;


typedef volatile union {
	struct {
		uint32_t st_val_2                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_2_t;


typedef volatile union {
	struct {
		uint32_t st_val_3                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_3_t;


typedef volatile union {
	struct {
		uint32_t st_val_4                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_4_t;


typedef volatile union {
	struct {
		uint32_t st_val_5                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_5_t;


typedef volatile union {
	struct {
		uint32_t st_val_6                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_6_t;


typedef volatile union {
	struct {
		uint32_t st_val_7                 : 24; /**<bit[0 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} audio_reg_st_val_7_t;


typedef volatile union {
	struct {
		uint32_t p_val_0                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_0_t;


typedef volatile union {
	struct {
		uint32_t p_val_1                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_1_t;


typedef volatile union {
	struct {
		uint32_t p_val_2                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_2_t;


typedef volatile union {
	struct {
		uint32_t p_val_3                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_3_t;


typedef volatile union {
	struct {
		uint32_t p_val_4                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_4_t;


typedef volatile union {
	struct {
		uint32_t p_val_5                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_5_t;


typedef volatile union {
	struct {
		uint32_t p_val_6                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} audio_reg_p_val_6_t;


typedef volatile union {
	struct {
		uint32_t anc_iir_bps0             : 10; /**<bit[0 : 9] */
		uint32_t anc_iir_bps1             : 10; /**<bit[10 : 19] */
		uint32_t anc_iir_bps2             : 10; /**<bit[20 : 29] */
		uint32_t reserved_bit_30_31       :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} audio_reg_anc_iir_bps_t;


typedef volatile union {
	struct {
		uint32_t adc_chn0_sel             :  3; /**<bit[0 : 2] */
		uint32_t reserved_bit_3_3         :  1; /**<bit[3 : 3] */
		uint32_t adc_chn1_sel             :  3; /**<bit[4 : 6] */
		uint32_t reserved_bit_7_7         :  1; /**<bit[7 : 7] */
		uint32_t adc_chn2_sel             :  3; /**<bit[8 : 10] */
		uint32_t reserved_bit_11_11       :  1; /**<bit[11 : 11] */
		uint32_t adc_chn3_sel             :  3; /**<bit[12 : 14] */
		uint32_t reserved_bit_15_15       :  1; /**<bit[15 : 15] */
		uint32_t adc_chn4_sel             :  3; /**<bit[16 : 18] */
		uint32_t reserved_bit_19_19       :  1; /**<bit[19 : 19] */
		uint32_t dac_r_chn_sel            :  1; /**<bit[20 : 20] */
		uint32_t dac_l_chn_sel            :  1; /**<bit[21 : 21] */
		uint32_t clk_frc_on               :  5; /**<bit[22 : 26] */
		uint32_t reserved_bit_27_31       :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} audio_reg_interface_matrix_t;

typedef volatile struct {
	volatile audio_reg_device_id_t device_id;
	volatile audio_reg_version_id_t version_id;
	volatile audio_reg_reserved0_t reserved0;
	volatile audio_reg_reserved1_t reserved1;
	volatile audio_reg_sys_cfg_t sys_cfg;
	volatile audio_reg_a2dp_comp_t a2dp_comp;
	volatile audio_reg_adc_cfg_t adc_cfg;
	volatile audio_reg_anc_cfg_t anc_cfg;
	volatile audio_reg_dac_cfg_t dac_cfg;
	volatile audio_reg_adc_cic_coef0_t adc_cic_coef0;
	volatile audio_reg_adc_cic_coef1_t adc_cic_coef1;
	volatile audio_reg_adc_cic_coef2_t adc_cic_coef2;
	volatile audio_reg_adc_cic_coef3_t adc_cic_coef3;
	volatile audio_reg_adc_cic_coef4_t adc_cic_coef4;
	volatile audio_reg_adc_cic_coef5_t adc_cic_coef5;
	volatile audio_reg_adc_cic_coef6_t adc_cic_coef6;
	volatile audio_reg_adc_cic_coef7_t adc_cic_coef7;
	volatile audio_reg_mic1_dbg_ctrl_t mic1_dbg_ctrl;
	volatile audio_reg_buf_ctrl_t buf_ctrl;
	volatile audio_reg_mic_fifo_cfg_t mic_fifo_cfg;
	volatile audio_reg_spk0_fifo_cfg_t spk0_fifo_cfg;
	volatile audio_reg_spk1_fifo_cfg_t spk1_fifo_cfg;
	volatile audio_reg_adc_cut_cfg_t adc_cut_cfg;
	volatile audio_reg_iir_sft_cfg_t iir_sft_cfg;
	volatile audio_reg_dac_cfg1_t dac_cfg1;
	volatile audio_reg_anc_cfg2_t anc_cfg2;
	volatile audio_reg_ramp_up_cfg_t ramp_up_cfg;
	volatile audio_reg_anc1_gain_cfg1_t anc1_gain_cfg1;
	volatile audio_reg_anc1_gain_cfg2_t anc1_gain_cfg2;
	volatile audio_reg_anc1_gain_cfg3_t anc1_gain_cfg3;
	volatile uint32_t rsv_1e_1e[1];
	volatile audio_reg_anc1_gain_cfg4_t anc1_gain_cfg4;
	volatile audio_reg_sync_ctrl1_t sync_ctrl1;
	volatile audio_reg_sync_ctrl2_t sync_ctrl2;
	volatile uint32_t rsv_22_23[2];
	volatile audio_reg_dac_ro_sts_t dac_ro_sts;
	volatile audio_reg_adc_ro_sts_t adc_ro_sts;
	volatile audio_reg_anc_sts_t anc_sts;
	volatile audio_reg_ramp_intr_ctrl_t ramp_intr_ctrl;
	volatile audio_reg_aud_int_ctrl_t aud_int_ctrl;
	volatile audio_reg_aud_int_sts_t aud_int_sts;
	volatile audio_reg_anc2_limit_cfg1_t anc2_limit_cfg1;
	volatile audio_reg_anc2_limit_cfg2_t anc2_limit_cfg2;
	volatile audio_reg_anc1_limit_cfg1_t anc1_limit_cfg1;
	volatile audio_reg_anc1_limit_cfg2_t anc1_limit_cfg2;
	volatile audio_reg_anc2_gain_cfg1_t anc2_gain_cfg1;
	volatile audio_reg_anc2_gain_cfg2_t anc2_gain_cfg2;
	volatile audio_reg_anc2_gain_cfg3_t anc2_gain_cfg3;
	volatile audio_reg_anc2_gain_cfg4_t anc2_gain_cfg4;
	volatile audio_reg_anc_comp_cfg_t anc_comp_cfg;
	volatile audio_reg_dac_gain_cfg0_t dac_gain_cfg0;
	volatile audio_reg_dac_gain_cfg1_t dac_gain_cfg1;
	volatile audio_reg_dac_gain_cfg2_t dac_gain_cfg2;
	volatile audio_reg_dac_gain_cfg3_t dac_gain_cfg3;
	volatile audio_reg_dac_gain_cfg4_t dac_gain_cfg4;
	volatile audio_reg_dac_gain_cfg5_t dac_gain_cfg5;
	volatile audio_reg_adc_gain_cfg1_t adc_gain_cfg1;
	volatile audio_reg_dac_l_gain_mix_t dac_l_gain_mix;
	volatile audio_reg_dac_r_gain_mix_t dac_r_gain_mix;
	volatile uint32_t rsv_3c_3f[4];
	volatile audio_reg_adc_gain_cfg2_t adc_gain_cfg2;
	volatile audio_reg_adc_gain_cfg3_t adc_gain_cfg3;
	volatile audio_reg_adc_gain_cfg4_t adc_gain_cfg4;
	volatile audio_reg_adc_gain_cfg5_t adc_gain_cfg5;
	volatile audio_reg_anc_sft_l_para_t anc_sft_l_para;
	volatile audio_reg_anc_sft_r_para_t anc_sft_r_para;
	volatile audio_reg_k_val_0_t k_val_0;
	volatile audio_reg_k_val_1_t k_val_1;
	volatile audio_reg_k_val_2_t k_val_2;
	volatile audio_reg_k_val_3_t k_val_3;
	volatile audio_reg_k_val_4_t k_val_4;
	volatile audio_reg_k_val_5_t k_val_5;
	volatile audio_reg_k_val_6_t k_val_6;
	volatile audio_reg_k_val_7_t k_val_7;
	volatile audio_reg_st_val_0_t st_val_0;
	volatile audio_reg_st_val_1_t st_val_1;
	volatile audio_reg_st_val_2_t st_val_2;
	volatile audio_reg_st_val_3_t st_val_3;
	volatile audio_reg_st_val_4_t st_val_4;
	volatile audio_reg_st_val_5_t st_val_5;
	volatile audio_reg_st_val_6_t st_val_6;
	volatile audio_reg_st_val_7_t st_val_7;
	volatile audio_reg_p_val_0_t p_val_0;
	volatile audio_reg_p_val_1_t p_val_1;
	volatile audio_reg_p_val_2_t p_val_2;
	volatile audio_reg_p_val_3_t p_val_3;
	volatile audio_reg_p_val_4_t p_val_4;
	volatile audio_reg_p_val_5_t p_val_5;
	volatile audio_reg_p_val_6_t p_val_6;
	volatile audio_reg_anc_iir_bps_t anc_iir_bps;
	volatile audio_reg_interface_matrix_t interface_matrix;
} audio_reg_hw_t;

#ifdef __cplusplus
}
#endif
