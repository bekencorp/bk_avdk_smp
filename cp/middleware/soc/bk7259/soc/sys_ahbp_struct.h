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
		uint32_t deviceid                         : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg0_t;


typedef volatile union {
	struct {
		uint32_t versionid                        : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_0                     :  1; /**<bit[0 : 0] */
		uint32_t clkg_bypass                      :  1; /**<bit[1 : 1] */
		uint32_t reserved_2_31                    : 30; /**<bit[2 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2_t;


typedef volatile union {
	struct {
		uint32_t cpu0_sleeping                    :  1; /**<bit[0 : 0] */
		uint32_t cpu0_deepsleep                   :  1; /**<bit[1 : 1] */
		uint32_t cpu1_sleeping                    :  1; /**<bit[2 : 2] */
		uint32_t cpu1_deepsleep                   :  1; /**<bit[3 : 3] */
		uint32_t reserved_bit_4_31                : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3_t;


typedef volatile union {
	struct {
		uint32_t cpu0_sw_rstn                     :  1; /**<bit[0 : 0] */
		uint32_t cpu0_init_dtcm_en                :  1; /**<bit[1 : 1] */
		uint32_t cpu0_sys_nmi                     :  1; /**<bit[2 : 2] */
		uint32_t cpu0_wfe_src                     :  1; /**<bit[3 : 3] */
		uint32_t cpu0_wfe_pulse                   :  1; /**<bit[4 : 4] */
		uint32_t cpu0_wait                        :  1; /**<bit[5 : 5] */
		uint32_t cpu0_dbg_rstn_disable            :  1; /**<bit[6 : 6] */
		uint32_t reserved_7_7                     :  1; /**<bit[7 : 7] */
		uint32_t cpu0_offset                      : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4_t;


typedef volatile union {
	struct {
		uint32_t cpu1_sw_rstn                     :  1; /**<bit[0 : 0] */
		uint32_t cpu1_init_dtcm_en                :  1; /**<bit[1 : 1] */
		uint32_t cpu1_sys_nmi                     :  1; /**<bit[2 : 2] */
		uint32_t cpu1_wfe_src                     :  1; /**<bit[3 : 3] */
		uint32_t cpu1_wfe_pulse                   :  1; /**<bit[4 : 4] */
		uint32_t cpu1_wait                        :  1; /**<bit[5 : 5] */
		uint32_t cpu1_dbg_rstn_disable            :  1; /**<bit[6 : 6] */
		uint32_t reserved_7_7                     :  1; /**<bit[7 : 7] */
		uint32_t cpu1_offset                      : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg5_t;


typedef volatile union {
	struct {
		uint32_t npu_sw_rstn                      :  1; /**<bit[0 : 0] */
		uint32_t npu_clkbps                       :  1; /**<bit[1 : 1] */
		uint32_t axi0_awcache                     :  4; /**<bit[2 : 5] */
		uint32_t axi0_arcache                     :  4; /**<bit[6 : 9] */
		uint32_t axi1_awcache                     :  4; /**<bit[10 : 13] */
		uint32_t axi1_arcache                     :  4; /**<bit[14 : 17] */
		uint32_t cache_src                        :  1; /**<bit[18 : 18] */
		uint32_t reserved_19_31                   : 13; /**<bit[19 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg6_t;


typedef volatile union {
	struct {
		uint32_t psram_inv_config                 :  2; /**<bit[0 : 1] */
		uint32_t videopost_m_arcache              :  4; /**<bit[2 : 5] */
		uint32_t videopost_m_awcache              :  4; /**<bit[6 : 9] */
		uint32_t reserved_10_15                   :  6; /**<bit[10 : 15] */
		uint32_t icache_clean_mode                :  1; /**<bit[16 : 16] */
		uint32_t cpu0_icache_clean_mode           :  1; /**<bit[17 : 17] */
		uint32_t cpu0_icache_clean_tag_sel        :  1; /**<bit[18 : 18] */
		uint32_t cpu1_icache_clean_mode           :  1; /**<bit[19 : 19] */
		uint32_t cpu1_icache_clean_tag_sel        :  1; /**<bit[20 : 20] */
		uint32_t reserved_21_23                   :  3; /**<bit[21 : 23] */
		uint32_t icache_clean_key                 :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg7_t;


typedef volatile union {
	struct {
		uint32_t cksel_core                       :  2; /**<bit[0 : 1] */
		uint32_t ckdiv_core                       :  2; /**<bit[2 : 3] */
		uint32_t ckdiv_bus_ls                     :  1; /**<bit[4 : 4] */
		uint32_t reserved_5_5                     :  1; /**<bit[5 : 5] */
		uint32_t ckdiv_uart5                      :  4; /**<bit[6 : 9] */
		uint32_t cksel_qspi0                      :  1; /**<bit[10 : 10] */
		uint32_t ckdiv_qspi0                      :  4; /**<bit[11 : 14] */
		uint32_t cksel_qspi1                      :  1; /**<bit[15 : 15] */
		uint32_t ckdiv_qspi1                      :  4; /**<bit[16 : 19] */
		uint32_t cksel_pram0                      :  2; /**<bit[20 : 21] */
		uint32_t ckdiv_pram0                      :  1; /**<bit[22 : 22] */
		uint32_t cksel_mbist                      :  1; /**<bit[23 : 23] */
		uint32_t ckdiv_sdio0                      :  4; /**<bit[24 : 27] */
		uint32_t ckdiv_sdio1                      :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg8_t;


typedef volatile union {
	struct {
		uint32_t cksel_cis_mclk                   :  2; /**<bit[0 : 1] */
		uint32_t ckdiv_cis_mclk                   :  4; /**<bit[2 : 5] */
		uint32_t ckdiv_cis_auxs                   :  4; /**<bit[6 : 9] */
		uint32_t cksel_cisp                       :  1; /**<bit[10 : 10] */
		uint32_t ckdiv_cisp                       :  2; /**<bit[11 : 12] */
		uint32_t cksel_gpu                        :  1; /**<bit[13 : 13] */
		uint32_t ckdiv_gpu                        :  2; /**<bit[14 : 15] */
		uint32_t cksel_h265                       :  1; /**<bit[16 : 16] */
		uint32_t ckdiv_h265                       :  2; /**<bit[17 : 18] */
		uint32_t cksel_dpu                        :  1; /**<bit[19 : 19] */
		uint32_t ckdiv_dpu                        :  5; /**<bit[20 : 24] */
		uint32_t cksel_pram1                      :  2; /**<bit[25 : 26] */
		uint32_t ckdiv_pram1                      :  1; /**<bit[27 : 27] */
		uint32_t ckdiv_trace                      :  2; /**<bit[28 : 29] */
		uint32_t cksel_cis_auxs                   :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg9_t;


typedef volatile union {
	struct {
		uint32_t cpua_cken                        :  1; /**<bit[0 : 0] */
		uint32_t uart5_cken                       :  1; /**<bit[1 : 1] */
		uint32_t usb_hs_cken                      :  1; /**<bit[2 : 2] */
		uint32_t pram0_cken                       :  1; /**<bit[3 : 3] */
		uint32_t pram1_cken                       :  1; /**<bit[4 : 4] */
		uint32_t qspi0_cken                       :  1; /**<bit[5 : 5] */
		uint32_t qspi1_cken                       :  1; /**<bit[6 : 6] */
		uint32_t sdio0_cken                       :  1; /**<bit[7 : 7] */
		uint32_t sdio1_cken                       :  1; /**<bit[8 : 8] */
		uint32_t cisp_cken                        :  1; /**<bit[9 : 9] */
		uint32_t gpu_cken                         :  1; /**<bit[10 : 10] */
		uint32_t h26e_cken                        :  1; /**<bit[11 : 11] */
		uint32_t csi_cken                         :  1; /**<bit[12 : 12] */
		uint32_t dsi_cken                         :  1; /**<bit[13 : 13] */
		uint32_t dpu_cken                         :  1; /**<bit[14 : 14] */
		uint32_t usb_fs_cken                      :  1; /**<bit[15 : 15] */
		uint32_t conf0_cken                       :  1; /**<bit[16 : 16] */
		uint32_t conf1_cken                       :  1; /**<bit[17 : 17] */
		uint32_t conf2_cken                       :  1; /**<bit[18 : 18] */
		uint32_t conf3_cken                       :  1; /**<bit[19 : 19] */
		uint32_t cpu0_cken                        :  1; /**<bit[20 : 20] */
		uint32_t cpu1_cken                        :  1; /**<bit[21 : 21] */
		uint32_t npu_cken                         :  1; /**<bit[22 : 22] */
		uint32_t timer4_cken                      :  1; /**<bit[23 : 23] */
		uint32_t timer5_cken                      :  1; /**<bit[24 : 24] */
		uint32_t trace_cken                       :  1; /**<bit[25 : 25] */
		uint32_t reserved_26_31                   :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} sys_ahbp_rega_t;


typedef volatile union {
	struct {
		uint32_t phase_ck640                      :  8; /**<bit[0 : 7] */
		uint32_t reserved_8_31                    : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} sys_ahbp_regb_t;


typedef volatile union {
	struct {
		uint32_t cpu0_mem_sd                      :  1; /**<bit[0 : 0] */
		uint32_t cpu1_mem_sd                      :  1; /**<bit[1 : 1] */
		uint32_t dtcm_mem_sd                      :  1; /**<bit[2 : 2] */
		uint32_t l2ch_mem_sd                      :  1; /**<bit[3 : 3] */
		uint32_t mem3_mem_sd                      :  1; /**<bit[4 : 4] */
		uint32_t mem4_mem_sd                      :  1; /**<bit[5 : 5] */
		uint32_t mem5_mem_sd                      :  1; /**<bit[6 : 6] */
		uint32_t mem6_mem_sd                      :  1; /**<bit[7 : 7] */
		uint32_t ahbp_mem_sd                      :  1; /**<bit[8 : 8] */
		uint32_t h265_mem_sd                      :  1; /**<bit[9 : 9] */
		uint32_t gpub_mem_sd                      :  1; /**<bit[10 : 10] */
		uint32_t dpub_mem_sd                      :  1; /**<bit[11 : 11] */
		uint32_t disb_mem_sd                      :  1; /**<bit[12 : 12] */
		uint32_t h264_mem_sd                      :  1; /**<bit[13 : 13] */
		uint32_t ispb_mem_sd                      :  1; /**<bit[14 : 14] */
		uint32_t csib_mem_sd                      :  1; /**<bit[15 : 15] */
		uint32_t npub_mem_sd                      :  1; /**<bit[16 : 16] */
		uint32_t reserved_17_31                   : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} sys_ahbp_regc_t;


typedef volatile union {
	struct {
		uint32_t cpu0_mem_ds                      :  1; /**<bit[0 : 0] */
		uint32_t cpu1_mem_ds                      :  1; /**<bit[1 : 1] */
		uint32_t dtcm_mem_ds                      :  1; /**<bit[2 : 2] */
		uint32_t l2ch_mem_ds                      :  1; /**<bit[3 : 3] */
		uint32_t mem3_mem_ds                      :  1; /**<bit[4 : 4] */
		uint32_t mem4_mem_ds                      :  1; /**<bit[5 : 5] */
		uint32_t mem5_mem_ds                      :  1; /**<bit[6 : 6] */
		uint32_t mem6_mem_ds                      :  1; /**<bit[7 : 7] */
		uint32_t ahbp_mem_ds                      :  1; /**<bit[8 : 8] */
		uint32_t h265_mem_ds                      :  1; /**<bit[9 : 9] */
		uint32_t gpub_mem_ds                      :  1; /**<bit[10 : 10] */
		uint32_t dpub_mem_ds                      :  1; /**<bit[11 : 11] */
		uint32_t disb_mem_ds                      :  1; /**<bit[12 : 12] */
		uint32_t h264_mem_ds                      :  1; /**<bit[13 : 13] */
		uint32_t ispb_mem_ds                      :  1; /**<bit[14 : 14] */
		uint32_t csib_mem_ds                      :  1; /**<bit[15 : 15] */
		uint32_t npub_mem_ds                      :  1; /**<bit[16 : 16] */
		uint32_t reserved_17_31                   : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} sys_ahbp_regd_t;


typedef volatile union {
	struct {
		uint32_t system_halt_en                   :  1; /**<bit[0 : 0] */
		uint32_t system_halt_high_cpu0wfi         :  1; /**<bit[1 : 1] */
		uint32_t system_halt_high_cpu1wfi         :  1; /**<bit[2 : 2] */
		uint32_t reserved_3_7                     :  5; /**<bit[3 : 7] */
		uint32_t cpu_halt_en                      :  1; /**<bit[8 : 8] */
		uint32_t cpu_halt_high_cpu0wfi            :  1; /**<bit[9 : 9] */
		uint32_t cpu_halt_high_cpu1wfi            :  1; /**<bit[10 : 10] */
		uint32_t reserved_11_15                   :  5; /**<bit[11 : 15] */
		uint32_t pwd_m55                          :  1; /**<bit[16 : 16] */
		uint32_t pwd_video_post                   :  1; /**<bit[17 : 17] */
		uint32_t pwd_h26e                         :  1; /**<bit[18 : 18] */
		uint32_t pwd_isp                          :  1; /**<bit[19 : 19] */
		uint32_t pwd_npu                          :  1; /**<bit[20 : 20] */
		uint32_t reserved_21_23                   :  3; /**<bit[21 : 23] */
		uint32_t soft_rstn_isp                    :  1; /**<bit[24 : 24] */
		uint32_t soft_rstn_h264e                  :  1; /**<bit[25 : 25] */
		uint32_t soft_rstn_h264d                  :  1; /**<bit[26 : 26] */
		uint32_t soft_rstn_gpu                    :  1; /**<bit[27 : 27] */
		uint32_t soft_rstn_dpu                    :  1; /**<bit[28 : 28] */
		uint32_t reserved_29_31                   :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} sys_ahbp_rege_t;


typedef volatile union {
	struct {
		uint32_t wwdt_region_wwdt                 :  1; /**<bit[0 : 0] */
		uint32_t wwdt_region_sys_cfg              :  1; /**<bit[1 : 1] */
		uint32_t wwdt_region_busx                 :  1; /**<bit[2 : 2] */
		uint32_t wwdt_region_cpu0                 :  1; /**<bit[3 : 3] */
		uint32_t wwdt_region_cpu1                 :  1; /**<bit[4 : 4] */
		uint32_t wwdt_region_ahbp                 :  1; /**<bit[5 : 5] */
		uint32_t wwdt_region_smem3                :  1; /**<bit[6 : 6] */
		uint32_t wwdt_region_smem4                :  1; /**<bit[7 : 7] */
		uint32_t wwdt_region_smem5                :  1; /**<bit[8 : 8] */
		uint32_t wwdt_region_smem6                :  1; /**<bit[9 : 9] */
		uint32_t wwdt_region_npu                  :  1; /**<bit[10 : 10] */
		uint32_t wwdt_region_h26e                 :  1; /**<bit[11 : 11] */
		uint32_t wwdt_region_isp                  :  1; /**<bit[12 : 12] */
		uint32_t wwdt_region_video_post           :  1; /**<bit[13 : 13] */
		uint32_t reserved_14_31                   : 18; /**<bit[14 : 31] */
	};
	uint32_t v;
} sys_ahbp_regf_t;


typedef volatile union {
	struct {
		uint32_t ints_config_m55a_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg10_t;


typedef volatile union {
	struct {
		uint32_t ints_config_m55a_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg11_t;


typedef volatile union {
	struct {
		uint32_t ints_config_m55b_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg12_t;


typedef volatile union {
	struct {
		uint32_t ints_config_m55b_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg13_t;


typedef volatile union {
	struct {
		uint32_t ints_config_m52s_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg14_t;


typedef volatile union {
	struct {
		uint32_t ints_config_m52s_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg15_t;


typedef volatile union {
	struct {
		uint32_t ints_config_scr1_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg16_t;


typedef volatile union {
	struct {
		uint32_t ints_config_scr1_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg17_t;


typedef volatile union {
	struct {
		uint32_t ints_status_m55a_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg18_t;


typedef volatile union {
	struct {
		uint32_t ints_status_m55a_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg19_t;


typedef volatile union {
	struct {
		uint32_t ints_status_m55b_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1a_t;


typedef volatile union {
	struct {
		uint32_t ints_status_m55b_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1b_t;


typedef volatile union {
	struct {
		uint32_t ints_status_m52s_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1c_t;


typedef volatile union {
	struct {
		uint32_t ints_status_m52s_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1d_t;


typedef volatile union {
	struct {
		uint32_t ints_status_scr1_0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1e_t;


typedef volatile union {
	struct {
		uint32_t ints_status_scr1_1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg1f_t;


typedef volatile union {
	struct {
		uint32_t cpu0_qos                         :  2; /**<bit[0 : 1] */
		uint32_t dma1_qos                         :  2; /**<bit[2 : 3] */
		uint32_t sdio0_qos                        :  2; /**<bit[4 : 5] */
		uint32_t sdio1_qos                        :  2; /**<bit[6 : 7] */
		uint32_t enet0_qos                        :  2; /**<bit[8 : 9] */
		uint32_t enet1_qos                        :  2; /**<bit[10 : 11] */
		uint32_t usb_qos                          :  2; /**<bit[12 : 13] */
		uint32_t h26e_qos                         :  2; /**<bit[14 : 15] */
		uint32_t isp_qos                          :  2; /**<bit[16 : 17] */
		uint32_t videopost_qos                    :  2; /**<bit[18 : 19] */
		uint32_t dpu_qos                          :  2; /**<bit[20 : 21] */
		uint32_t cbus_qos                         :  2; /**<bit[22 : 23] */
		uint32_t npu0_qos                         :  2; /**<bit[24 : 25] */
		uint32_t npu1_qos                         :  2; /**<bit[26 : 27] */
		uint32_t cpu1_qos                         :  2; /**<bit[28 : 29] */
		uint32_t reserved_30_31                   :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg20_t;


typedef volatile union {
	struct {
		uint32_t dpu_gate_cpu_en                  :  1; /**<bit[0 : 0] */
		uint32_t dpu_gate_videopost_en            :  1; /**<bit[1 : 1] */
		uint32_t dpu_gate_h26e_en                 :  1; /**<bit[2 : 2] */
		uint32_t reserved_3_7                     :  5; /**<bit[3 : 7] */
		uint32_t reserved_8_8                     :  1; /**<bit[8 : 8] */
		uint32_t dpu_gate_videopost_override      :  1; /**<bit[9 : 9] */
		uint32_t dpu_gate_h26e_override           :  1; /**<bit[10 : 10] */
		uint32_t reserved_11_15                   :  5; /**<bit[11 : 15] */
		uint32_t cpu_bus_dis                      :  1; /**<bit[16 : 16] */
		uint32_t videopost_bus_dis                :  1; /**<bit[17 : 17] */
		uint32_t h26e_bus_dis                     :  1; /**<bit[18 : 18] */
		uint32_t reserved_19_23                   :  5; /**<bit[19 : 23] */
		uint32_t dpu_gate_key                     :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg21_t;


typedef volatile union {
	struct {
		uint32_t dbug_config0                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg22_t;


typedef volatile union {
	struct {
		uint32_t dbug_mux                         :  4; /**<bit[0 : 3] */
		uint32_t dbug_config1                     : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg23_t;


typedef volatile union {
	struct {
		uint32_t trace_config                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg24_t;


typedef volatile union {
	struct {
		uint32_t gpio_dbug_readout                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg25_t;


typedef volatile union {
	struct {
		uint32_t general0                         : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg26_t;


typedef volatile union {
	struct {
		uint32_t general1                         : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg27_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffa_enable                 :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg28_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffa_begin                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg29_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffa_size                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2a_t;


typedef volatile union {
	struct {
		uint32_t gpu_pica_begin                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2b_t;


typedef volatile union {
	struct {
		uint32_t gpu_pica_halfbuff_end            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2c_t;


typedef volatile union {
	struct {
		uint32_t gpu_pica_end                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2d_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_31                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2e_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffa_remap_addr             : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg2f_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffb_enable                 :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg30_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffb_begin                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg31_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffb_size                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg32_t;


typedef volatile union {
	struct {
		uint32_t gpu_picb_begin                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg33_t;


typedef volatile union {
	struct {
		uint32_t gpu_picb_halfbuff_end            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg34_t;


typedef volatile union {
	struct {
		uint32_t gpu_picb_end                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg35_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_31                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg36_t;


typedef volatile union {
	struct {
		uint32_t gpu_buffb_remap_addr             : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg37_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffa_enable                :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg38_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffa_begin                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg39_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffa_size                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3a_t;


typedef volatile union {
	struct {
		uint32_t h26d_pica_begin                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3b_t;


typedef volatile union {
	struct {
		uint32_t h26d_pica_halfbuff_end           : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3c_t;


typedef volatile union {
	struct {
		uint32_t h26d_pica_end                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3d_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_31                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3e_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffa_remap_addr            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg3f_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffb_enable                :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg40_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffb_begin                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg41_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffb_size                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg42_t;


typedef volatile union {
	struct {
		uint32_t h26d_picb_begin                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg43_t;


typedef volatile union {
	struct {
		uint32_t h26d_picb_halfbuff_end           : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg44_t;


typedef volatile union {
	struct {
		uint32_t h26d_picb_end                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg45_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_31                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg46_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffb_remap_addr            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg47_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffc_enable                :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg48_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffc_begin                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg49_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffc_size                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4a_t;


typedef volatile union {
	struct {
		uint32_t h26d_picc_begin                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4b_t;


typedef volatile union {
	struct {
		uint32_t h26d_picc_halfbuff_end           : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4c_t;


typedef volatile union {
	struct {
		uint32_t h26d_picc_end                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4d_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_31                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4e_t;


typedef volatile union {
	struct {
		uint32_t h26d_buffc_remap_addr            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg4f_t;


typedef volatile union {
	struct {
		uint32_t ram_spsh_cfg                     : 10; /**<bit[0 : 9] */
		uint32_t ram_spbh_cfg                     : 11; /**<bit[10 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t ram_spsh_set_key                 :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg50_t;


typedef volatile union {
	struct {
		uint32_t ram_stph_cfg                     : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_23               : 12; /**<bit[12 : 23] */
		uint32_t ram_stph_set_key                 :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg51_t;


typedef volatile union {
	struct {
		uint32_t ram_spsl_cfg                     : 10; /**<bit[0 : 9] */
		uint32_t ram_spbl_cfg                     : 11; /**<bit[10 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t ram_spsl_set_key                 :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg52_t;


typedef volatile union {
	struct {
		uint32_t ram_stpl_cfg                     : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_23               : 12; /**<bit[12 : 23] */
		uint32_t ram_stpl_set_key                 :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_ahbp_reg53_t;

typedef volatile struct {
	volatile sys_ahbp_reg0_t reg0;
	volatile sys_ahbp_reg1_t reg1;
	volatile sys_ahbp_reg2_t reg2;
	volatile sys_ahbp_reg3_t reg3;
	volatile sys_ahbp_reg4_t reg4;
	volatile sys_ahbp_reg5_t reg5;
	volatile sys_ahbp_reg6_t reg6;
	volatile sys_ahbp_reg7_t reg7;
	volatile sys_ahbp_reg8_t reg8;
	volatile sys_ahbp_reg9_t reg9;
	volatile sys_ahbp_rega_t rega;
	volatile sys_ahbp_regb_t regb;
	volatile sys_ahbp_regc_t regc;
	volatile sys_ahbp_regd_t regd;
	volatile sys_ahbp_rege_t rege;
	volatile sys_ahbp_regf_t regf;
	volatile sys_ahbp_reg10_t reg10;
	volatile sys_ahbp_reg11_t reg11;
	volatile sys_ahbp_reg12_t reg12;
	volatile sys_ahbp_reg13_t reg13;
	volatile sys_ahbp_reg14_t reg14;
	volatile sys_ahbp_reg15_t reg15;
	volatile sys_ahbp_reg16_t reg16;
	volatile sys_ahbp_reg17_t reg17;
	volatile sys_ahbp_reg18_t reg18;
	volatile sys_ahbp_reg19_t reg19;
	volatile sys_ahbp_reg1a_t reg1a;
	volatile sys_ahbp_reg1b_t reg1b;
	volatile sys_ahbp_reg1c_t reg1c;
	volatile sys_ahbp_reg1d_t reg1d;
	volatile sys_ahbp_reg1e_t reg1e;
	volatile sys_ahbp_reg1f_t reg1f;
	volatile sys_ahbp_reg20_t reg20;
	volatile sys_ahbp_reg21_t reg21;
	volatile sys_ahbp_reg22_t reg22;
	volatile sys_ahbp_reg23_t reg23;
	volatile sys_ahbp_reg24_t reg24;
	volatile sys_ahbp_reg25_t reg25;
	volatile sys_ahbp_reg26_t reg26;
	volatile sys_ahbp_reg27_t reg27;
	volatile sys_ahbp_reg28_t reg28;
	volatile sys_ahbp_reg29_t reg29;
	volatile sys_ahbp_reg2a_t reg2a;
	volatile sys_ahbp_reg2b_t reg2b;
	volatile sys_ahbp_reg2c_t reg2c;
	volatile sys_ahbp_reg2d_t reg2d;
	volatile sys_ahbp_reg2e_t reg2e;
	volatile sys_ahbp_reg2f_t reg2f;
	volatile sys_ahbp_reg30_t reg30;
	volatile sys_ahbp_reg31_t reg31;
	volatile sys_ahbp_reg32_t reg32;
	volatile sys_ahbp_reg33_t reg33;
	volatile sys_ahbp_reg34_t reg34;
	volatile sys_ahbp_reg35_t reg35;
	volatile sys_ahbp_reg36_t reg36;
	volatile sys_ahbp_reg37_t reg37;
	volatile sys_ahbp_reg38_t reg38;
	volatile sys_ahbp_reg39_t reg39;
	volatile sys_ahbp_reg3a_t reg3a;
	volatile sys_ahbp_reg3b_t reg3b;
	volatile sys_ahbp_reg3c_t reg3c;
	volatile sys_ahbp_reg3d_t reg3d;
	volatile sys_ahbp_reg3e_t reg3e;
	volatile sys_ahbp_reg3f_t reg3f;
	volatile sys_ahbp_reg40_t reg40;
	volatile sys_ahbp_reg41_t reg41;
	volatile sys_ahbp_reg42_t reg42;
	volatile sys_ahbp_reg43_t reg43;
	volatile sys_ahbp_reg44_t reg44;
	volatile sys_ahbp_reg45_t reg45;
	volatile sys_ahbp_reg46_t reg46;
	volatile sys_ahbp_reg47_t reg47;
	volatile sys_ahbp_reg48_t reg48;
	volatile sys_ahbp_reg49_t reg49;
	volatile sys_ahbp_reg4a_t reg4a;
	volatile sys_ahbp_reg4b_t reg4b;
	volatile sys_ahbp_reg4c_t reg4c;
	volatile sys_ahbp_reg4d_t reg4d;
	volatile sys_ahbp_reg4e_t reg4e;
	volatile sys_ahbp_reg4f_t reg4f;
	volatile sys_ahbp_reg50_t reg50;
	volatile sys_ahbp_reg51_t reg51;
	volatile sys_ahbp_reg52_t reg52;
	volatile sys_ahbp_reg53_t reg53;
} sys_ahbp_hw_t;

#ifdef __cplusplus
}
#endif
