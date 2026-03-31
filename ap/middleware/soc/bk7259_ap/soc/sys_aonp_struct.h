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
} sys_aonp_reg0_t;


typedef volatile union {
	struct {
		uint32_t versionid                        : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg1_t;


typedef volatile union {
	struct {
		uint32_t boot_mode                        :  1; /**<bit[0 : 0] */
		uint32_t clkg_bps                         :  1; /**<bit[1 : 1] */
		uint32_t reserved_bit_2_3                 :  2; /**<bit[2 : 3] */
		uint32_t rf_switch_manual_en              :  1; /**<bit[4 : 4] */
		uint32_t rf_source                        :  2; /**<bit[5 : 6] */
		uint32_t jtag_core_sel                    :  1; /**<bit[7 : 7] */
		uint32_t reserved_bit_8_8                 :  1; /**<bit[8 : 8] */
		uint32_t flash_sel                        :  1; /**<bit[9 : 9] */
		uint32_t fem_bps_txen                     :  1; /**<bit[10 : 10] */
		uint32_t gpio_flash_sys_enable            :  1; /**<bit[11 : 11] */
		uint32_t boot_mode_norst                  :  1; /**<bit[12 : 12] */
		uint32_t reserved_bit_13_31               : 19; /**<bit[13 : 31] */
	};
	uint32_t v;
} sys_aonp_reg2_t;


typedef volatile union {
	struct {
		uint32_t core0_halted                     :  1; /**<bit[0 : 0] */
		uint32_t core1_halted                     :  1; /**<bit[1 : 1] */
		uint32_t reserved_bit_2_3                 :  2; /**<bit[2 : 3] */
		uint32_t cpu0_sw_reset                    :  1; /**<bit[4 : 4] */
		uint32_t cpu1_sw_reset                    :  1; /**<bit[5 : 5] */
		uint32_t reserved_bit_6_7                 :  2; /**<bit[6 : 7] */
		uint32_t cpu0_pwr_dw_state                :  1; /**<bit[8 : 8] */
		uint32_t cpu1_pwr_dw_state                :  1; /**<bit[9 : 9] */
		uint32_t reserved_bit_10_11               :  2; /**<bit[10 : 11] */
		uint32_t cpu0_exist                       :  1; /**<bit[12 : 12] */
		uint32_t cpu1_exist                       :  1; /**<bit[13 : 13] */
		uint32_t cpu2_exist                       :  1; /**<bit[14 : 14] */
		uint32_t cpu3_exist                       :  1; /**<bit[15 : 15] */
		uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3_t;


typedef volatile union {
	struct {
		uint32_t cpu0_sw_rst                      :  1; /**<bit[0 : 0] */
		uint32_t cpu0_pwr_dw                      :  1; /**<bit[1 : 1] */
		uint32_t cpu_int_mask                     :  1; /**<bit[2 : 2] */
		uint32_t cpu0_halt                        :  1; /**<bit[3 : 3] */
		uint32_t reserved_bit_4_4                 :  1; /**<bit[4 : 4] */
		uint32_t cpu0_rxevt_sel                   :  2; /**<bit[5 : 6] */
		uint32_t reserved_7_7                     :  1; /**<bit[7 : 7] */
		uint32_t cpu0_offset                      : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} sys_aonp_reg4_t;


typedef volatile union {
	struct {
		uint32_t cpu1_sw_rst                      :  1; /**<bit[0 : 0] */
		uint32_t cpu1_pwr_dw                      :  1; /**<bit[1 : 1] */
		uint32_t reserved_2_2                     :  1; /**<bit[2 : 2] */
		uint32_t cpu1_halt                        :  1; /**<bit[3 : 3] */
		uint32_t reserved_bit_4_4                 :  1; /**<bit[4 : 4] */
		uint32_t cpu1_rxevt_sel                   :  2; /**<bit[5 : 6] */
		uint32_t reserved_7_7                     :  1; /**<bit[7 : 7] */
		uint32_t cpu1_offset                      : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} sys_aonp_reg5_t;


typedef volatile union {
	struct {
		uint32_t cksel_core                       :  2; /**<bit[0 : 1] */
		uint32_t ckdiv_core                       :  4; /**<bit[2 : 5] */
		uint32_t cksel_flash                      :  2; /**<bit[6 : 7] */
		uint32_t ckdiv_flash                      :  3; /**<bit[8 : 10] */
		uint32_t cksel_auxs                       :  2; /**<bit[11 : 12] */
		uint32_t ckdiv_auxs                       :  6; /**<bit[13 : 18] */
		uint32_t ckdiv_26mo                       :  3; /**<bit[19 : 21] */
		uint32_t reserved_22_23                   :  2; /**<bit[22 : 23] */
		uint32_t phase_cfg_960m                   :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_aonp_reg8_t;


typedef volatile union {
	struct {
		uint32_t cksel_i2c0                       :  1; /**<bit[0 : 0] */
		uint32_t cksel_i2c3                       :  1; /**<bit[1 : 1] */
		uint32_t cksel_uart0                      :  1; /**<bit[2 : 2] */
		uint32_t cksel_uart1                      :  1; /**<bit[3 : 3] */
		uint32_t cksel_uart2                      :  1; /**<bit[4 : 4] */
		uint32_t cksel_uart3                      :  1; /**<bit[5 : 5] */
		uint32_t cksel_uart4                      :  1; /**<bit[6 : 6] */
		uint32_t cksel_spi0                       :  1; /**<bit[7 : 7] */
		uint32_t cksel_spi1                       :  1; /**<bit[8 : 8] */
		uint32_t cksel_spi2                       :  1; /**<bit[9 : 9] */
		uint32_t cksel_spi3                       :  1; /**<bit[10 : 10] */
		uint32_t cksel_i2s0                       :  1; /**<bit[11 : 11] */
		uint32_t ckdiv_i2s0                       :  2; /**<bit[12 : 13] */
		uint32_t cksel_i2s1                       :  1; /**<bit[14 : 14] */
		uint32_t ckdiv_i2s1                       :  2; /**<bit[15 : 16] */
		uint32_t cksel_i2s2                       :  1; /**<bit[17 : 17] */
		uint32_t ckdiv_i2s2                       :  2; /**<bit[18 : 19] */
		uint32_t cksel_i2s3                       :  1; /**<bit[20 : 20] */
		uint32_t ckdiv_i2s3                       :  2; /**<bit[21 : 22] */
		uint32_t cksel_i2s4                       :  1; /**<bit[23 : 23] */
		uint32_t ckdiv_i2s4                       :  2; /**<bit[24 : 25] */
		uint32_t cksel_sadc                       :  1; /**<bit[26 : 26] */
		uint32_t cksel_i3c                        :  1; /**<bit[27 : 27] */
		uint32_t cksel_tim0                       :  1; /**<bit[28 : 28] */
		uint32_t cksel_tim1                       :  1; /**<bit[29 : 29] */
		uint32_t cksel_tim2                       :  1; /**<bit[30 : 30] */
		uint32_t cksel_tim3                       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_reg9_t;


typedef volatile union {
	struct {
		uint32_t cksel_pwm0                       :  1; /**<bit[0 : 0] */
		uint32_t cksel_can0                       :  1; /**<bit[1 : 1] */
		uint32_t cksel_can1                       :  1; /**<bit[2 : 2] */
		uint32_t cksel_scr0                       :  1; /**<bit[3 : 3] */
		uint32_t cksel_audio                      :  1; /**<bit[4 : 4] */
		uint32_t ckdiv_audio                      :  3; /**<bit[5 : 7] */
		uint32_t cksel_audif0                     :  1; /**<bit[8 : 8] */
		uint32_t ckdiv_audif0                     :  3; /**<bit[9 : 11] */
		uint32_t cksel_audif1                     :  1; /**<bit[12 : 12] */
		uint32_t ckdiv_audif1                     :  3; /**<bit[13 : 15] */
		uint32_t ckdiv_i2so                       :  3; /**<bit[16 : 18] */
		uint32_t cksel_auxs_enet                  :  1; /**<bit[19 : 19] */
		uint32_t ckdiv_auxs_enet                  :  4; /**<bit[20 : 23] */
		uint32_t cksel_trace                      :  1; /**<bit[24 : 24] */
		uint32_t ckdiv_trace                      :  3; /**<bit[25 : 27] */
		uint32_t reserved_28_31                   :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} sys_aonp_rega_t;


typedef volatile union {
	struct {
		uint32_t anaspi_freq                      :  6; /**<bit[0 : 5] */
		uint32_t reserved_bit_6_31                : 26; /**<bit[6 : 31] */
	};
	uint32_t v;
} sys_aonp_regb_t;


typedef volatile union {
	struct {
		uint32_t tim0_cken                        :  1; /**<bit[0 : 0] */
		uint32_t tim1_cken                        :  1; /**<bit[1 : 1] */
		uint32_t tim2_cken                        :  1; /**<bit[2 : 2] */
		uint32_t tim3_cken                        :  1; /**<bit[3 : 3] */
		uint32_t uart0_cken                       :  1; /**<bit[4 : 4] */
		uint32_t uart1_cken                       :  1; /**<bit[5 : 5] */
		uint32_t uart2_cken                       :  1; /**<bit[6 : 6] */
		uint32_t uart3_cken                       :  1; /**<bit[7 : 7] */
		uint32_t uart4_cken                       :  1; /**<bit[8 : 8] */
		uint32_t spi0_cken                        :  1; /**<bit[9 : 9] */
		uint32_t spi1_cken                        :  1; /**<bit[10 : 10] */
		uint32_t spi2_cken                        :  1; /**<bit[11 : 11] */
		uint32_t spi3_cken                        :  1; /**<bit[12 : 12] */
		uint32_t sadc_cken                        :  1; /**<bit[13 : 13] */
		uint32_t pwm0_cken                        :  1; /**<bit[14 : 14] */
		uint32_t otp_cken                         :  1; /**<bit[15 : 15] */
		uint32_t i3c_cken                         :  1; /**<bit[16 : 16] */
		uint32_t i2s0_cken                        :  1; /**<bit[17 : 17] */
		uint32_t i2s1_cken                        :  1; /**<bit[18 : 18] */
		uint32_t i2s2_cken                        :  1; /**<bit[19 : 19] */
		uint32_t i2s3_cken                        :  1; /**<bit[20 : 20] */
		uint32_t i2s4_cken                        :  1; /**<bit[21 : 21] */
		uint32_t i2c0_cken                        :  1; /**<bit[22 : 22] */
		uint32_t i2c3_cken                        :  1; /**<bit[23 : 23] */
		uint32_t irda0_cken                       :  1; /**<bit[24 : 24] */
		uint32_t irda1_cken                       :  1; /**<bit[25 : 25] */
		uint32_t irda2_cken                       :  1; /**<bit[26 : 26] */
		uint32_t irda3_cken                       :  1; /**<bit[27 : 27] */
		uint32_t can0_cken                        :  1; /**<bit[28 : 28] */
		uint32_t can1_cken                        :  1; /**<bit[29 : 29] */
		uint32_t lin0_cken                        :  1; /**<bit[30 : 30] */
		uint32_t scr0_cken                        :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_regc_t;


typedef volatile union {
	struct {
		uint32_t audio_cken                       :  1; /**<bit[0 : 0] */
		uint32_t audif0_cken                      :  1; /**<bit[1 : 1] */
		uint32_t audif1_cken                      :  1; /**<bit[2 : 2] */
		uint32_t i2so_cken                        :  1; /**<bit[3 : 3] */
		uint32_t cec_cken                         :  1; /**<bit[4 : 4] */
		uint32_t xdac0_cken                       :  1; /**<bit[5 : 5] */
		uint32_t xdac1_cken                       :  1; /**<bit[6 : 6] */
		uint32_t auxs_cken                        :  1; /**<bit[7 : 7] */
		uint32_t auxs_enet_cken                   :  1; /**<bit[8 : 8] */
		uint32_t sig_26ms_cken                    :  1; /**<bit[9 : 9] */
		uint32_t sig_32ks_cken                    :  1; /**<bit[10 : 10] */
		uint32_t sig_26mo_cken                    :  1; /**<bit[11 : 11] */
		uint32_t sig_240m_cken                    :  1; /**<bit[12 : 12] */
		uint32_t sig_320m_cken                    :  1; /**<bit[13 : 13] */
		uint32_t sig_480m_cken                    :  1; /**<bit[14 : 14] */
		uint32_t sig_160m_cken                    :  1; /**<bit[15 : 15] */
		uint32_t sig_120m_cken                    :  1; /**<bit[16 : 16] */
		uint32_t trace_cken                       :  1; /**<bit[17 : 17] */
		uint32_t reserved_18_21                   :  4; /**<bit[18 : 21] */
		uint32_t wlss_cken                        :  1; /**<bit[22 : 22] */
		uint32_t btdm_cken                        :  1; /**<bit[23 : 23] */
		uint32_t xver_cken                        :  1; /**<bit[24 : 24] */
		uint32_t mac_cken                         :  1; /**<bit[25 : 25] */
		uint32_t phy_cken                         :  1; /**<bit[26 : 26] */
		uint32_t thread_cken                      :  1; /**<bit[27 : 27] */
		uint32_t bk24_cken                        :  1; /**<bit[28 : 28] */
		uint32_t rf_cken                          :  1; /**<bit[29 : 29] */
		uint32_t ofdm_cken                        :  1; /**<bit[30 : 30] */
		uint32_t reserved_31_31                   :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_regd_t;


typedef volatile union {
	struct {
		uint32_t reserved_0_0                     :  1; /**<bit[0 : 0] */
		uint32_t reserved_1_1                     :  1; /**<bit[1 : 1] */
		uint32_t reserved_2_2                     :  1; /**<bit[2 : 2] */
		uint32_t macp_mem_ret                     :  1; /**<bit[3 : 3] */
		uint32_t phyp_mem_ret                     :  1; /**<bit[4 : 4] */
		uint32_t thread_mem_ret                   :  1; /**<bit[5 : 5] */
		uint32_t encp_mem_ret                     :  1; /**<bit[6 : 6] */
		uint32_t can0_mem_ret                     :  1; /**<bit[7 : 7] */
		uint32_t can1_mem_ret                     :  1; /**<bit[8 : 8] */
		uint32_t irda0_mem_ret                    :  1; /**<bit[9 : 9] */
		uint32_t irda1_mem_ret                    :  1; /**<bit[10 : 10] */
		uint32_t dma0_mem_ret                     :  1; /**<bit[11 : 11] */
		uint32_t spi1_mem_ret                     :  1; /**<bit[12 : 12] */
		uint32_t spi2_mem_ret                     :  1; /**<bit[13 : 13] */
		uint32_t uart1_mem_ret                    :  1; /**<bit[14 : 14] */
		uint32_t uart2_mem_ret                    :  1; /**<bit[15 : 15] */
		uint32_t uart3_mem_ret                    :  1; /**<bit[16 : 16] */
		uint32_t uart0_mem_ret                    :  1; /**<bit[17 : 17] */
		uint32_t spi0_mem_ret                     :  1; /**<bit[18 : 18] */
		uint32_t flsh_mem_ret                     :  1; /**<bit[19 : 19] */
		uint32_t audp_mem_ret                     :  1; /**<bit[20 : 20] */
		uint32_t i3c_mem_ret                      :  1; /**<bit[21 : 21] */
		uint32_t xvr_mem_ret                      :  1; /**<bit[22 : 22] */
		uint32_t reserved_23_23                   :  1; /**<bit[23 : 23] */
		uint32_t bk24_mem_ret                     :  1; /**<bit[24 : 24] */
		uint32_t irda2_mem_ret                    :  1; /**<bit[25 : 25] */
		uint32_t irda3_mem_ret                    :  1; /**<bit[26 : 26] */
		uint32_t spi3_mem_ret                     :  1; /**<bit[27 : 27] */
		uint32_t uart4_mem_ret                    :  1; /**<bit[28 : 28] */
		uint32_t cpu0_mem_ret                     :  1; /**<bit[29 : 29] */
		uint32_t cpu1_mem_ret                     :  1; /**<bit[30 : 30] */
		uint32_t coresight_mem_ret                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_regf_t;


typedef volatile union {
	struct {
		uint32_t pwd_cpu1                         :  1; /**<bit[0 : 0] */
		uint32_t pwd_vehp                         :  1; /**<bit[1 : 1] */
		uint32_t pwd_wrls                         :  1; /**<bit[2 : 2] */
		uint32_t rom_pgen                         :  1; /**<bit[3 : 3] */
		uint32_t cpu1_isolate_state               :  1; /**<bit[4 : 4] */
		uint32_t vehp_isolate_state               :  1; /**<bit[5 : 5] */
		uint32_t wrls_isolate_state               :  1; /**<bit[6 : 6] */
		uint32_t reserved_7_30                    : 24; /**<bit[7 : 30] */
		uint32_t busmatrix_busy                   :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_reg10_t;


typedef volatile union {
	struct {
		uint32_t sleep_en_global                  :  1; /**<bit[0 : 0] */
		uint32_t sleep_en_need_flash_idle         :  1; /**<bit[1 : 1] */
		uint32_t sleep_bus_idle_bypass            :  1; /**<bit[2 : 2] */
		uint32_t sleep_en_need_cpu0_wfi           :  1; /**<bit[3 : 3] */
		uint32_t sleep_en_need_cpu1_wfi           :  1; /**<bit[4 : 4] */
		uint32_t reserved_bit_5_11                :  7; /**<bit[5 : 11] */
		uint32_t reserved_12_15                   :  4; /**<bit[12 : 15] */
		uint32_t cpu0_ticktimer_32k_enable        :  1; /**<bit[16 : 16] */
		uint32_t cpu1_ticktimer_32k_enable        :  1; /**<bit[17 : 17] */
		uint32_t reserved_bit_18_19               :  2; /**<bit[18 : 19] */
		uint32_t reserved_20_24                   :  5; /**<bit[20 : 24] */
		uint32_t bts_soft_wakeup_req              :  1; /**<bit[25 : 25] */
		uint32_t rom_rd_disable                   :  1; /**<bit[26 : 26] */
		uint32_t otp_rd_disable                   :  1; /**<bit[27 : 27] */
		uint32_t share_mem_clkgating_disable      :  1; /**<bit[28 : 28] */
		uint32_t reserved_29_31                   :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} sys_aonp_reg11_t;


typedef volatile union {
	struct {
		uint32_t cpu0_inten                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg14_t;


typedef volatile union {
	struct {
		uint32_t cpu0_inten                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg15_t;


typedef volatile union {
	struct {
		uint32_t cpu0_inten                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg16_t;


typedef volatile union {
	struct {
		uint32_t cpu1_inten                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg17_t;


typedef volatile union {
	struct {
		uint32_t cpu1_inten                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg18_t;


typedef volatile union {
	struct {
		uint32_t cpu1_inten                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg19_t;


typedef volatile union {
	struct {
		uint32_t m55sub_inten                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg1a_t;


typedef volatile union {
	struct {
		uint32_t m55sub_inten                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg1b_t;


typedef volatile union {
	struct {
		uint32_t m55sub_inten                     : 31; /**<bit[0 : 30] */
		uint32_t m55sub_wakeup                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_reg1c_t;


typedef volatile union {
	struct {
		uint32_t spsh_cfg                         : 10; /**<bit[0 : 9] */
		uint32_t spbh_cfg                         : 11; /**<bit[10 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_aonp_reg1e_t;


typedef volatile union {
	struct {
		uint32_t stph_cfg                         : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_23               : 12; /**<bit[12 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_aonp_reg1f_t;


typedef volatile union {
	struct {
		uint32_t ints_status0                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg20_t;


typedef volatile union {
	struct {
		uint32_t ints_status1                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg21_t;


typedef volatile union {
	struct {
		uint32_t ints_status2                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg22_t;


typedef volatile union {
	struct {
		uint32_t ints_status3                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg23_t;


typedef volatile union {
	struct {
		uint32_t ints_status4                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg24_t;


typedef volatile union {
	struct {
		uint32_t ints_status5                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg25_t;


typedef volatile union {
	struct {
		uint32_t m55sub_status0                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg26_t;


typedef volatile union {
	struct {
		uint32_t m55sub_status1                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg27_t;


typedef volatile union {
	struct {
		uint32_t m55sub_status2                   : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg28_t;


typedef volatile union {
	struct {
		uint32_t debug_gpio_ie                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg2a_t;


typedef volatile union {
	struct {
		uint32_t debug_gpio_i                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg2b_t;


typedef volatile union {
	struct {
		uint32_t cache_clean_mode                 :  1; /**<bit[0 : 0] */
		uint32_t cpu0_icache_clean_mode           :  1; /**<bit[1 : 1] */
		uint32_t cpu0_icache_clean_tag_sel        :  1; /**<bit[2 : 2] */
		uint32_t l2_cache_clean_mode              :  1; /**<bit[3 : 3] */
		uint32_t l2_cache_clean_tag_sel           :  2; /**<bit[4 : 5] */
		uint32_t reserved_6_23                    : 18; /**<bit[6 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_aonp_reg2c_t;


typedef volatile union {
	struct {
		uint32_t spsl_cfg                         : 10; /**<bit[0 : 9] */
		uint32_t spbl_cfg                         : 11; /**<bit[10 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_aonp_reg2e_t;


typedef volatile union {
	struct {
		uint32_t stpl_cfg                         : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_23               : 12; /**<bit[12 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_aonp_reg2f_t;


typedef volatile union {
	struct {
		uint32_t gpio_input_status0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg30_t;


typedef volatile union {
	struct {
		uint32_t gpio_input_status1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg31_t;


typedef volatile union {
	struct {
		uint32_t gpio_input_status2               :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_30                : 23; /**<bit[8 : 30] */
		uint32_t gpio_input_status_en             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_reg32_t;


typedef volatile union {
	struct {
		uint32_t acomp0_pwm0_sample_en            :  1; /**<bit[0 : 0] */
		uint32_t acomp1_pwm0_sample_en            :  1; /**<bit[1 : 1] */
		uint32_t reserved_2_15                    : 14; /**<bit[2 : 15] */
		uint32_t l2_dis_pwr_down_maint            :  1; /**<bit[16 : 16] */
		uint32_t l2_apb_violation_resp            :  1; /**<bit[17 : 17] */
		uint32_t cpu0_dbgen_l2_rst_dis            :  1; /**<bit[18 : 18] */
		uint32_t cpu1_dbgen_l2_rst_dis            :  1; /**<bit[19 : 19] */
		uint32_t reserved_20_23                   :  4; /**<bit[20 : 23] */
		uint32_t cpu0_wfe_src                     :  1; /**<bit[24 : 24] */
		uint32_t cpu0_wfe_pulse                   :  1; /**<bit[25 : 25] */
		uint32_t cpu1_wfe_src                     :  1; /**<bit[26 : 26] */
		uint32_t cpu1_wfe_pulse                   :  1; /**<bit[27 : 27] */
		uint32_t cpu0_sleeping_state              :  1; /**<bit[28 : 28] */
		uint32_t cpu0_deepsleep_state             :  1; /**<bit[29 : 29] */
		uint32_t cpu1_sleeping_state              :  1; /**<bit[30 : 30] */
		uint32_t cpu1_deepsleep_state             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_reg33_t;


typedef volatile union {
	struct {
		uint32_t cpu0_curpc                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg34_t;


typedef volatile union {
	struct {
		uint32_t cpu0_faultstat_h                 : 11; /**<bit[0 : 10] */
		uint32_t reserved_11_31                   : 21; /**<bit[11 : 31] */
	};
	uint32_t v;
} sys_aonp_reg35_t;


typedef volatile union {
	struct {
		uint32_t cpu0_faultstat_l                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg36_t;


typedef volatile union {
	struct {
		uint32_t cpu0_intnum                      :  9; /**<bit[0 : 8] */
		uint32_t cpu0_currpri                     :  8; /**<bit[9 : 16] */
		uint32_t cpu0_currns                      :  1; /**<bit[17 : 17] */
		uint32_t cpu0_halted                      :  1; /**<bit[18 : 18] */
		uint32_t cpu0_nc_hready                   :  1; /**<bit[19 : 19] */
		uint32_t cpu_cache_m_hready               :  1; /**<bit[20 : 20] */
		uint32_t cpu0_resetn                      :  1; /**<bit[21 : 21] */
		uint32_t reserved_22_31                   : 10; /**<bit[22 : 31] */
	};
	uint32_t v;
} sys_aonp_reg37_t;


typedef volatile union {
	struct {
		uint32_t dbug_config0                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg38_t;


typedef volatile union {
	struct {
		uint32_t dbug_config1                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg39_t;


typedef volatile union {
	struct {
		uint32_t anareg_stat                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3a_t;


typedef volatile union {
	struct {
		uint32_t coresight_chn_gate_en            : 24; /**<bit[0 : 23] */
		uint32_t coresight_tpmaxdatasize          :  5; /**<bit[24 : 28] */
		uint32_t coresight_valid                  :  1; /**<bit[29 : 29] */
		uint32_t reserved_30_30                   :  1; /**<bit[30 : 30] */
		uint32_t anaregb_stat                     :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3b_t;


typedef volatile union {
	struct {
		uint32_t cpu1_curpc                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3c_t;


typedef volatile union {
	struct {
		uint32_t cpu1_faultstat_h                 : 11; /**<bit[0 : 10] */
		uint32_t reserved_11_31                   : 21; /**<bit[11 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3d_t;


typedef volatile union {
	struct {
		uint32_t cpu1_faultstat_l                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3e_t;


typedef volatile union {
	struct {
		uint32_t cpu1_intnum                      :  9; /**<bit[0 : 8] */
		uint32_t cpu1_currpri                     :  8; /**<bit[9 : 16] */
		uint32_t cpu1_currns                      :  1; /**<bit[17 : 17] */
		uint32_t cpu1_halted                      :  1; /**<bit[18 : 18] */
		uint32_t cpu1_nc_hready                   :  1; /**<bit[19 : 19] */
		uint32_t reserved_20_20                   :  1; /**<bit[20 : 20] */
		uint32_t cpu1_resetn                      :  1; /**<bit[21 : 21] */
		uint32_t reserved_22_31                   : 10; /**<bit[22 : 31] */
	};
	uint32_t v;
} sys_aonp_reg3f_t;


typedef volatile union {
	struct {
		uint32_t dpll_tsten                       :  1; /**<bit[0 : 0] */
		uint32_t cp                               :  3; /**<bit[1 : 3] */
		uint32_t spideten                         :  1; /**<bit[4 : 4] */
		uint32_t hvref                            :  2; /**<bit[5 : 6] */
		uint32_t lvref                            :  2; /**<bit[7 : 8] */
		uint32_t rzctrl26m                        :  1; /**<bit[9 : 9] */
		uint32_t looprzctrl                       :  4; /**<bit[10 : 13] */
		uint32_t rpc                              :  2; /**<bit[14 : 15] */
		uint32_t openloop_en                      :  1; /**<bit[16 : 16] */
		uint32_t cksel                            :  2; /**<bit[17 : 18] */
		uint32_t spitrig                          :  1; /**<bit[19 : 19] */
		uint32_t band                             :  1; /**<bit[20 : 20] */
		uint32_t band_1                           :  1; /**<bit[21 : 21] */
		uint32_t band_2                           :  3; /**<bit[22 : 24] */
		uint32_t bandmanual                       :  1; /**<bit[25 : 25] */
		uint32_t dsptrig                          :  1; /**<bit[26 : 26] */
		uint32_t lpen_dpll                        :  1; /**<bit[27 : 27] */
		uint32_t nc_28_29                         :  2; /**<bit[28 : 29] */
		uint32_t bp_caldone                       :  1; /**<bit[30 : 30] */
		uint32_t vctrl_dpllldo                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg0_t;


typedef volatile union {
	struct {
		uint32_t vcooffset                        :  1; /**<bit[0 : 0] */
		uint32_t selpol                           :  1; /**<bit[1 : 1] */
		uint32_t dlysel                           :  2; /**<bit[2 : 3] */
		uint32_t edgesel_nck                      :  1; /**<bit[4 : 4] */
		uint32_t nload_dlyen                      :  1; /**<bit[5 : 5] */
		uint32_t cp                               :  3; /**<bit[6 : 8] */
		uint32_t spideten                         :  1; /**<bit[9 : 9] */
		uint32_t cben                             :  1; /**<bit[10 : 10] */
		uint32_t hvref                            :  2; /**<bit[11 : 12] */
		uint32_t lvref                            :  2; /**<bit[13 : 14] */
		uint32_t rzctrl26m                        :  1; /**<bit[15 : 15] */
		uint32_t lpfrz                            :  4; /**<bit[16 : 19] */
		uint32_t rpc                              :  3; /**<bit[20 : 22] */
		uint32_t dpll_tsten                       :  1; /**<bit[23 : 23] */
		uint32_t kctrl                            :  2; /**<bit[24 : 25] */
		uint32_t vsel_ldo                         :  2; /**<bit[26 : 27] */
		uint32_t div_sw                           :  1; /**<bit[28 : 28] */
		uint32_t bp_caldone                       :  1; /**<bit[29 : 29] */
		uint32_t ck2xen                           :  1; /**<bit[30 : 30] */
		uint32_t int_mod                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg1_t;


typedef volatile union {
	struct {
		uint32_t xtalh_ctune                      :  8; /**<bit[0 : 7] */
		uint32_t force_26mpll                     :  1; /**<bit[8 : 8] */
		uint32_t nc_9_11                          :  3; /**<bit[9 : 11] */
		uint32_t gadc_sd1v                        :  1; /**<bit[12 : 12] */
		uint32_t gadc_bscalsaw                    :  3; /**<bit[13 : 15] */
		uint32_t gadc_vncalsaw                    :  3; /**<bit[16 : 18] */
		uint32_t gadc_vpcalsaw                    :  3; /**<bit[19 : 21] */
		uint32_t nc_22_22                         :  1; /**<bit[22 : 22] */
		uint32_t gadc_vbg_sel                     :  1; /**<bit[23 : 23] */
		uint32_t gadc_clk_rlten                   :  1; /**<bit[24 : 24] */
		uint32_t gadc_calintsaw_en                :  1; /**<bit[25 : 25] */
		uint32_t gadc_clk_sel                     :  1; /**<bit[26 : 26] */
		uint32_t gadc_clk_inv                     :  1; /**<bit[27 : 27] */
		uint32_t gadc_calcap_ch                   :  2; /**<bit[28 : 29] */
		uint32_t gadc_inbuf_en                    :  1; /**<bit[30 : 30] */
		uint32_t gadc_en_spi                      :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg2_t;


typedef volatile union {
	struct {
		uint32_t nc_0_6                           :  7; /**<bit[0 : 6] */
		uint32_t anabuf_sel_rx                    :  1; /**<bit[7 : 7] */
		uint32_t hpssren                          :  1; /**<bit[8 : 8] */
		uint32_t ck_sel                           :  1; /**<bit[9 : 9] */
		uint32_t anabuf_sel_tx                    :  1; /**<bit[10 : 10] */
		uint32_t pwd_xtalldo                      :  1; /**<bit[11 : 11] */
		uint32_t iamp                             :  1; /**<bit[12 : 12] */
		uint32_t vddren                           :  1; /**<bit[13 : 13] */
		uint32_t xamp                             :  6; /**<bit[14 : 19] */
		uint32_t vosel                            :  5; /**<bit[20 : 24] */
		uint32_t en_xtalh_sleep                   :  1; /**<bit[25 : 25] */
		uint32_t xtal40_en                        :  1; /**<bit[26 : 26] */
		uint32_t bufictrl                         :  1; /**<bit[27 : 27] */
		uint32_t ibias_ctrl                       :  2; /**<bit[28 : 29] */
		uint32_t icore_ctrl                       :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg3_t;


typedef volatile union {
	struct {
		uint32_t cktst_sel                        :  2; /**<bit[0 : 1] */
		uint32_t ck_tst_en                        :  1; /**<bit[2 : 2] */
		uint32_t vusbsel                          :  2; /**<bit[3 : 4] */
		uint32_t nc_5_16                          : 12; /**<bit[5 : 16] */
		uint32_t gadc_inbuff_isel                 :  3; /**<bit[17 : 19] */
		uint32_t gadc_biasamp_isel                :  3; /**<bit[20 : 22] */
		uint32_t gadc_comp_isel                   :  3; /**<bit[23 : 25] */
		uint32_t gadc_preamp_isel                 :  3; /**<bit[26 : 28] */
		uint32_t gadc_bufamp_isel                 :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg4_t;


typedef volatile union {
	struct {
		uint32_t en_vout                          :  1; /**<bit[0 : 0] */
		uint32_t en_xtall                         :  1; /**<bit[1 : 1] */
		uint32_t en_dco                           :  1; /**<bit[2 : 2] */
		uint32_t nc_3_3                           :  1; /**<bit[3 : 3] */
		uint32_t en_temp                          :  1; /**<bit[4 : 4] */
		uint32_t en_dpll                          :  1; /**<bit[5 : 5] */
		uint32_t en_cb                            :  1; /**<bit[6 : 6] */
		uint32_t gpio_latch                       :  1; /**<bit[7 : 7] */
		uint32_t nc_8_11                          :  4; /**<bit[8 : 11] */
		uint32_t rosc_disable                     :  1; /**<bit[12 : 12] */
		uint32_t pwdaudpll                        :  1; /**<bit[13 : 13] */
		uint32_t pwd_rosc_spi                     :  1; /**<bit[14 : 14] */
		uint32_t nc_15_15                         :  1; /**<bit[15 : 15] */
		uint32_t itune_xtall                      :  4; /**<bit[16 : 19] */
		uint32_t xtall_ten                        :  1; /**<bit[20 : 20] */
		uint32_t rosc_tsten                       :  1; /**<bit[21 : 21] */
		uint32_t bcal_start                       :  1; /**<bit[22 : 22] */
		uint32_t bcal_en                          :  1; /**<bit[23 : 23] */
		uint32_t bcal_sel                         :  3; /**<bit[24 : 26] */
		uint32_t vbias                            :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg5_t;


typedef volatile union {
	struct {
		uint32_t calib_interval                   : 10; /**<bit[0 : 9] */
		uint32_t modify_interval                  :  6; /**<bit[10 : 15] */
		uint32_t xtal_wakeup_time                 :  4; /**<bit[16 : 19] */
		uint32_t spi_trig                         :  1; /**<bit[20 : 20] */
		uint32_t modifi_auto                      :  1; /**<bit[21 : 21] */
		uint32_t calib_auto                       :  1; /**<bit[22 : 22] */
		uint32_t cal_mode                         :  1; /**<bit[23 : 23] */
		uint32_t manu_ena                         :  1; /**<bit[24 : 24] */
		uint32_t manu_cin                         :  7; /**<bit[25 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg6_t;


typedef volatile union {
	struct {
		uint32_t nsyn                             :  1; /**<bit[0 : 0] */
		uint32_t bandmanual                       :  6; /**<bit[1 : 6] */
		uint32_t ckref_loop_sel                   :  1; /**<bit[7 : 7] */
		uint32_t ioffs                            :  3; /**<bit[8 : 10] */
		uint32_t reset_nload                      :  1; /**<bit[11 : 11] */
		uint32_t closeloop_en                     :  1; /**<bit[12 : 12] */
		uint32_t modecal                          :  1; /**<bit[13 : 13] */
		uint32_t spi_rstn                         :  1; /**<bit[14 : 14] */
		uint32_t osccal_trig                      :  1; /**<bit[15 : 15] */
		uint32_t manual                           :  1; /**<bit[16 : 16] */
		uint32_t diff                             :  3; /**<bit[17 : 19] */
		uint32_t ictrlmanual                      :  3; /**<bit[20 : 22] */
		uint32_t cnti                             :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg7_t;


typedef volatile union {
	struct {
		uint32_t reserved_bit_0_30                : 31; /**<bit[0 : 30] */
		uint32_t n                                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg8_t;


typedef volatile union {
	struct {
		uint32_t clk_sel                          :  1; /**<bit[0 : 0] */
		uint32_t coreldo_hp                       :  1; /**<bit[1 : 1] */
		uint32_t dldohp                           :  1; /**<bit[2 : 2] */
		uint32_t t_vanaldosel                     :  3; /**<bit[3 : 5] */
		uint32_t r_vanaldosel                     :  3; /**<bit[6 : 8] */
		uint32_t en_trsw                          :  1; /**<bit[9 : 9] */
		uint32_t aldohp                           :  1; /**<bit[10 : 10] */
		uint32_t anacurlim                        :  1; /**<bit[11 : 11] */
		uint32_t hsldo_hp                         :  1; /**<bit[12 : 12] */
		uint32_t pwd_hsldo                        :  1; /**<bit[13 : 13] */
		uint32_t enfast_hsldo                     :  1; /**<bit[14 : 14] */
		uint32_t nc_15_15                         :  1; /**<bit[15 : 15] */
		uint32_t valoldosel                       :  3; /**<bit[16 : 18] */
		uint32_t alopowsel                        :  1; /**<bit[19 : 19] */
		uint32_t en_fast_aloldo                   :  1; /**<bit[20 : 20] */
		uint32_t aloldohp                         :  1; /**<bit[21 : 21] */
		uint32_t bgcal                            :  6; /**<bit[22 : 27] */
		uint32_t vbgcalmode                       :  1; /**<bit[28 : 28] */
		uint32_t vbgcalstart                      :  1; /**<bit[29 : 29] */
		uint32_t pwd_bgcal                        :  1; /**<bit[30 : 30] */
		uint32_t spi_envbg                        :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg9_t;


typedef volatile union {
	struct {
		uint32_t azcd_manual                      :  6; /**<bit[0 : 5] */
		uint32_t azcdrefs                         :  2; /**<bit[6 : 7] */
		uint32_t reserved_bit_8_8                 :  1; /**<bit[8 : 8] */
		uint32_t spi_latchb                       :  1; /**<bit[9 : 9] */
		uint32_t digcurlim                        :  1; /**<bit[10 : 10] */
		uint32_t rtc_wkrstn                       :  1; /**<bit[11 : 11] */
		uint32_t rst_wks                          :  1; /**<bit[12 : 12] */
		uint32_t d_veasel1v                       :  2; /**<bit[13 : 14] */
		uint32_t ensfsdd                          :  1; /**<bit[15 : 15] */
		uint32_t vcorehsel                        :  4; /**<bit[16 : 19] */
		uint32_t vcorelsel                        :  3; /**<bit[20 : 22] */
		uint32_t vlden                            :  1; /**<bit[23 : 23] */
		uint32_t en_fast_coreldo                  :  1; /**<bit[24 : 24] */
		uint32_t pwdcoreldo                       :  1; /**<bit[25 : 25] */
		uint32_t vdighsel                         :  3; /**<bit[26 : 28] */
		uint32_t vdigsel                          :  2; /**<bit[29 : 30] */
		uint32_t vdd12lden                        :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg10_t;


typedef volatile union {
	struct {
		uint32_t aldo_czsel                       :  3; /**<bit[0 : 2] */
		uint32_t zldo_rzsel                       :  2; /**<bit[3 : 4] */
		uint32_t azcdswvs                         :  3; /**<bit[5 : 7] */
		uint32_t aenzcddy                         :  1; /**<bit[8 : 8] */
		uint32_t aenzcdmsel                       :  1; /**<bit[9 : 9] */
		uint32_t aenzcdcalib                      :  1; /**<bit[10 : 10] */
		uint32_t en_corepsw                       :  1; /**<bit[11 : 11] */
		uint32_t en_alopsw                        :  1; /**<bit[12 : 12] */
		uint32_t nc_13_14                         :  2; /**<bit[13 : 14] */
		uint32_t spi_timerwken                    :  1; /**<bit[15 : 15] */
		uint32_t spi_byp32pwd                     :  1; /**<bit[16 : 16] */
		uint32_t sd                               :  1; /**<bit[17 : 17] */
		uint32_t nc_18_18                         :  1; /**<bit[18 : 18] */
		uint32_t gpio_wkrst1v                     :  1; /**<bit[19 : 19] */
		uint32_t ckfs                             :  2; /**<bit[20 : 21] */
		uint32_t ckintsel                         :  1; /**<bit[22 : 22] */
		uint32_t osccaltrig                       :  1; /**<bit[23 : 23] */
		uint32_t mroscsel                         :  1; /**<bit[24 : 24] */
		uint32_t mrosci_cal                       :  3; /**<bit[25 : 27] */
		uint32_t mrosccap_cal                     :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg11_t;


typedef volatile union {
	struct {
		uint32_t sfsr                             :  4; /**<bit[0 : 3] */
		uint32_t ensfsaa                          :  1; /**<bit[4 : 4] */
		uint32_t apfms                            :  5; /**<bit[5 : 9] */
		uint32_t atmpo_sel                        :  2; /**<bit[10 : 11] */
		uint32_t ampoen                           :  1; /**<bit[12 : 12] */
		uint32_t enpowa                           :  1; /**<bit[13 : 13] */
		uint32_t avea_sel                         :  2; /**<bit[14 : 15] */
		uint32_t aforcepfm                        :  1; /**<bit[16 : 16] */
		uint32_t acls                             :  3; /**<bit[17 : 19] */
		uint32_t aswrsten                         :  1; /**<bit[20 : 20] */
		uint32_t aripc                            :  3; /**<bit[21 : 23] */
		uint32_t arampc                           :  4; /**<bit[24 : 27] */
		uint32_t arampcen                         :  1; /**<bit[28 : 28] */
		uint32_t aenburst                         :  1; /**<bit[29 : 29] */
		uint32_t apfmen                           :  1; /**<bit[30 : 30] */
		uint32_t aldosel                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg12_t;


typedef volatile union {
	struct {
		uint32_t buckd_softst                     :  4; /**<bit[0 : 3] */
		uint32_t denzcdcalib                      :  1; /**<bit[4 : 4] */
		uint32_t nc_5_6                           :  2; /**<bit[5 : 6] */
		uint32_t vddgpio_sel                      :  1; /**<bit[7 : 7] */
		uint32_t dpfms                            :  5; /**<bit[8 : 12] */
		uint32_t dtmpo_sel                        :  2; /**<bit[13 : 14] */
		uint32_t dmpoen                           :  1; /**<bit[15 : 15] */
		uint32_t dforcepfm                        :  1; /**<bit[16 : 16] */
		uint32_t dcls                             :  3; /**<bit[17 : 19] */
		uint32_t dswrsten                         :  1; /**<bit[20 : 20] */
		uint32_t dripc                            :  3; /**<bit[21 : 23] */
		uint32_t drampc                           :  4; /**<bit[24 : 27] */
		uint32_t drampcen                         :  1; /**<bit[28 : 28] */
		uint32_t denburst                         :  1; /**<bit[29 : 29] */
		uint32_t dpfmen                           :  1; /**<bit[30 : 30] */
		uint32_t dldosel                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg13_t;


typedef volatile union {
	struct {
		uint32_t pwdovp1v                         :  1; /**<bit[0 : 0] */
		uint32_t asoft_stc                        :  4; /**<bit[1 : 4] */
		uint32_t dldo_czsel                       :  3; /**<bit[5 : 7] */
		uint32_t dldo_rzsel                       :  2; /**<bit[8 : 9] */
		uint32_t en_usbvcc18                      :  1; /**<bit[10 : 10] */
		uint32_t en_usbvcc3v                      :  1; /**<bit[11 : 11] */
		uint32_t vtrxspisel                       :  2; /**<bit[12 : 13] */
		uint32_t denzcddy                         :  1; /**<bit[14 : 14] */
		uint32_t dzcd_swvs                        :  3; /**<bit[15 : 17] */
		uint32_t dzcd_refs                        :  3; /**<bit[18 : 20] */
		uint32_t dzcd_manu                        :  6; /**<bit[21 : 26] */
		uint32_t azcdmsel                         :  1; /**<bit[27 : 27] */
		uint32_t psldo_swb                        :  1; /**<bit[28 : 28] */
		uint32_t vpsramsel                        :  2; /**<bit[29 : 30] */
		uint32_t enpsram                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg14_t;


typedef volatile union {
	struct {
		uint32_t gpiowken                         : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg15_t;


typedef volatile union {
	struct {
		uint32_t timer_set                        :  4; /**<bit[0 : 3] */
		uint32_t rtc_set                          :  4; /**<bit[4 : 7] */
		uint32_t nc_8_10                          :  3; /**<bit[8 : 10] */
		uint32_t vcorehssel                       :  4; /**<bit[11 : 14] */
		uint32_t vbuckhssel                       :  3; /**<bit[15 : 17] */
		uint32_t hsenfast                         :  1; /**<bit[18 : 18] */
		uint32_t enhspw                           :  1; /**<bit[19 : 19] */
		uint32_t buckhs_soft_stc                  :  4; /**<bit[20 : 23] */
		uint32_t hs_veasel                        :  2; /**<bit[24 : 25] */
		uint32_t hszcd_manual                     :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg16_t;


typedef volatile union {
	struct {
		uint32_t rtc_set                          : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg17_t;


typedef volatile union {
	struct {
		uint32_t timer_set                        : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg18_t;


typedef volatile union {
	struct {
		uint32_t hsenzcddy                        :  1; /**<bit[0 : 0] */
		uint32_t hsenzcdcalib                     :  1; /**<bit[1 : 1] */
		uint32_t hszcdswvs                        :  3; /**<bit[2 : 4] */
		uint32_t hszcdrefs                        :  3; /**<bit[5 : 7] */
		uint32_t hszcdmsel                        :  1; /**<bit[8 : 8] */
		uint32_t hspfms                           :  5; /**<bit[9 : 13] */
		uint32_t hstmpo_sel                       :  2; /**<bit[14 : 15] */
		uint32_t hsmpoen                          :  1; /**<bit[16 : 16] */
		uint32_t hsforcepfm                       :  1; /**<bit[17 : 17] */
		uint32_t hscls                            :  3; /**<bit[18 : 20] */
		uint32_t hsswrsten                        :  1; /**<bit[21 : 21] */
		uint32_t hsripc                           :  3; /**<bit[22 : 24] */
		uint32_t hsrampc                          :  4; /**<bit[25 : 28] */
		uint32_t hsrampcen                        :  1; /**<bit[29 : 29] */
		uint32_t hsenburst                        :  1; /**<bit[30 : 30] */
		uint32_t hspfmen                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg19_t;


typedef volatile union {
	struct {
		uint32_t iselaud                          :  1; /**<bit[0 : 0] */
		uint32_t audck_rlcen                      :  1; /**<bit[1 : 1] */
		uint32_t lchckinven                       :  1; /**<bit[2 : 2] */
		uint32_t enaudbias                        :  1; /**<bit[3 : 3] */
		uint32_t enadcbias                        :  1; /**<bit[4 : 4] */
		uint32_t enmicbias                        :  1; /**<bit[5 : 5] */
		uint32_t adcckinven                       :  1; /**<bit[6 : 6] */
		uint32_t spi                              :  1; /**<bit[7 : 7] */
		uint32_t adctsten                         :  1; /**<bit[8 : 8] */
		uint32_t micbias_trm                      :  2; /**<bit[9 : 10] */
		uint32_t micbias_voc                      :  5; /**<bit[11 : 15] */
		uint32_t vrefsel                          :  1; /**<bit[16 : 16] */
		uint32_t capsw                            :  5; /**<bit[17 : 21] */
		uint32_t adcref_sel                       :  2; /**<bit[22 : 23] */
		uint32_t adcvcmsel                        :  2; /**<bit[24 : 25] */
		uint32_t spi_1                            :  1; /**<bit[26 : 26] */
		uint32_t audadjref                        :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg20_t;


typedef volatile union {
	struct {
		uint32_t isel_mic1                        :  2; /**<bit[0 : 1] */
		uint32_t micirsel1_mic1                   :  1; /**<bit[2 : 2] */
		uint32_t vcmsel_mic1                      :  1; /**<bit[3 : 3] */
		uint32_t enfsr_mic1                       :  1; /**<bit[4 : 4] */
		uint32_t enopoclip_mic1                   :  1; /**<bit[5 : 5] */
		uint32_t da2aden_mic1                     :  1; /**<bit[6 : 6] */
		uint32_t imatch_mic1                      :  4; /**<bit[7 : 10] */
		uint32_t imatch_en_mic1                   :  1; /**<bit[11 : 11] */
		uint32_t dccompen_mic1                    :  1; /**<bit[12 : 12] */
		uint32_t micsingleen_mic1                 :  1; /**<bit[13 : 13] */
		uint32_t nc_14_14                         :  1; /**<bit[14 : 14] */
		uint32_t micgain_mic1                     :  4; /**<bit[15 : 18] */
		uint32_t nc_19_23                         :  5; /**<bit[19 : 23] */
		uint32_t dwamode_mic1                     :  1; /**<bit[24 : 24] */
		uint32_t nc_25_27                         :  3; /**<bit[25 : 27] */
		uint32_t micen_mic1                       :  1; /**<bit[28 : 28] */
		uint32_t rst_mic1                         :  1; /**<bit[29 : 29] */
		uint32_t bpdwa1v_mic1                     :  1; /**<bit[30 : 30] */
		uint32_t hcen1stg_mic1                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg21_t;


typedef volatile union {
	struct {
		uint32_t ictrl_dsppll                     :  1; /**<bit[0 : 0] */
		uint32_t lvref                            :  2; /**<bit[1 : 2] */
		uint32_t reserved_bit_3_3                 :  1; /**<bit[3 : 3] */
		uint32_t nc_4_18                          : 15; /**<bit[4 : 18] */
		uint32_t mode                             :  1; /**<bit[19 : 19] */
		uint32_t iamsel                           :  1; /**<bit[20 : 20] */
		uint32_t hvref                            :  2; /**<bit[21 : 22] */
		uint32_t lvref_1                          :  2; /**<bit[23 : 24] */
		uint32_t nc_25_31                         :  7; /**<bit[25 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg22_t;


typedef volatile union {
	struct {
		uint32_t camsel                           :  1; /**<bit[0 : 0] */
		uint32_t msw                              :  9; /**<bit[1 : 9] */
		uint32_t tstcken_dpll                     :  1; /**<bit[10 : 10] */
		uint32_t osccal_trig                      :  1; /**<bit[11 : 11] */
		uint32_t cnti                             :  9; /**<bit[12 : 20] */
		uint32_t nc_21_21                         :  1; /**<bit[21 : 21] */
		uint32_t spi_rst                          :  1; /**<bit[22 : 22] */
		uint32_t closeloop_en                     :  1; /**<bit[23 : 23] */
		uint32_t caltime                          :  1; /**<bit[24 : 24] */
		uint32_t lpfrz                            :  2; /**<bit[25 : 26] */
		uint32_t icp                              :  4; /**<bit[27 : 30] */
		uint32_t cp2ctrl                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg23_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                          : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg24_t;


typedef volatile union {
	struct {
		uint32_t int_mod                          :  1; /**<bit[0 : 0] */
		uint32_t nsyn                             :  1; /**<bit[1 : 1] */
		uint32_t open_enb                         :  1; /**<bit[2 : 2] */
		uint32_t reset                            :  1; /**<bit[3 : 3] */
		uint32_t ioffsetl                         :  3; /**<bit[4 : 6] */
		uint32_t lpfrz                            :  4; /**<bit[7 : 10] */
		uint32_t vsel                             :  3; /**<bit[11 : 13] */
		uint32_t vsel_cal                         :  1; /**<bit[14 : 14] */
		uint32_t pwd_lockdet                      :  1; /**<bit[15 : 15] */
		uint32_t lockdet_bypass                   :  1; /**<bit[16 : 16] */
		uint32_t ckref_loop_sel                   :  1; /**<bit[17 : 17] */
		uint32_t spi_trigger                      :  1; /**<bit[18 : 18] */
		uint32_t manual                           :  1; /**<bit[19 : 19] */
		uint32_t test_ckaudio_en                  :  1; /**<bit[20 : 20] */
		uint32_t ck2xen                           :  1; /**<bit[21 : 21] */
		uint32_t icp                              :  2; /**<bit[22 : 23] */
		uint32_t cktst_sel                        :  1; /**<bit[24 : 24] */
		uint32_t edgesel_nck                      :  1; /**<bit[25 : 25] */
		uint32_t nloaddlyen                       :  1; /**<bit[26 : 26] */
		uint32_t bypass_caldone_auto              :  1; /**<bit[27 : 27] */
		uint32_t cal_res_spi                      :  3; /**<bit[28 : 30] */
		uint32_t audioen                          :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg25_t;


typedef volatile union {
	struct {
		uint32_t n                                : 30; /**<bit[0 : 29] */
		uint32_t calres_spien                     :  1; /**<bit[30 : 30] */
		uint32_t calrefen                         :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg26_t;


typedef volatile union {
	struct {
		uint32_t isel_mic2                        :  2; /**<bit[0 : 1] */
		uint32_t micirsel1_mic2                   :  1; /**<bit[2 : 2] */
		uint32_t vcmsel_mic2                      :  1; /**<bit[3 : 3] */
		uint32_t enfsr_mic2                       :  1; /**<bit[4 : 4] */
		uint32_t enopoclip_mic2                   :  1; /**<bit[5 : 5] */
		uint32_t da2aden_mic2                     :  1; /**<bit[6 : 6] */
		uint32_t imatch_mic2                      :  4; /**<bit[7 : 10] */
		uint32_t imatch_en_mic2                   :  1; /**<bit[11 : 11] */
		uint32_t dccompen_mic2                    :  1; /**<bit[12 : 12] */
		uint32_t micsingleen_mic2                 :  1; /**<bit[13 : 13] */
		uint32_t nc_14_14                         :  1; /**<bit[14 : 14] */
		uint32_t micgain_mic2                     :  4; /**<bit[15 : 18] */
		uint32_t nc_19_23                         :  5; /**<bit[19 : 23] */
		uint32_t dwamode_mic2                     :  1; /**<bit[24 : 24] */
		uint32_t nc_25_27                         :  3; /**<bit[25 : 27] */
		uint32_t micen_mic2                       :  1; /**<bit[28 : 28] */
		uint32_t rst_mic2                         :  1; /**<bit[29 : 29] */
		uint32_t bpdwa1v_mic2                     :  1; /**<bit[30 : 30] */
		uint32_t hcen1stg_mic2                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg27_t;


typedef volatile union {
	struct {
		uint32_t isel_mic3                        :  2; /**<bit[0 : 1] */
		uint32_t micirsel1_mic3                   :  1; /**<bit[2 : 2] */
		uint32_t vcmsel_mic3                      :  1; /**<bit[3 : 3] */
		uint32_t enfsr_mic3                       :  1; /**<bit[4 : 4] */
		uint32_t enopoclip_mic3                   :  1; /**<bit[5 : 5] */
		uint32_t da2aden_mic3                     :  1; /**<bit[6 : 6] */
		uint32_t imatch_mic3                      :  4; /**<bit[7 : 10] */
		uint32_t imatch_en_mic3                   :  1; /**<bit[11 : 11] */
		uint32_t dccompen_mic3                    :  1; /**<bit[12 : 12] */
		uint32_t micsingleen_mic3                 :  1; /**<bit[13 : 13] */
		uint32_t nc_14_14                         :  1; /**<bit[14 : 14] */
		uint32_t micgain_mic3                     :  4; /**<bit[15 : 18] */
		uint32_t nc_19_23                         :  5; /**<bit[19 : 23] */
		uint32_t dwamode_mic3                     :  1; /**<bit[24 : 24] */
		uint32_t nc_25_27                         :  3; /**<bit[25 : 27] */
		uint32_t micen_mic3                       :  1; /**<bit[28 : 28] */
		uint32_t rst_mic3                         :  1; /**<bit[29 : 29] */
		uint32_t bpdwa1v_mic3                     :  1; /**<bit[30 : 30] */
		uint32_t hcen1stg_mic3                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg28_t;


typedef volatile union {
	struct {
		uint32_t hpdac                            :  1; /**<bit[0 : 0] */
		uint32_t iselstg                          :  1; /**<bit[1 : 1] */
		uint32_t oscdac                           :  2; /**<bit[2 : 3] */
		uint32_t ocendac                          :  1; /**<bit[4 : 4] */
		uint32_t vseldco                          :  1; /**<bit[5 : 5] */
		uint32_t srsel                            :  1; /**<bit[6 : 6] */
		uint32_t hpoen                            :  1; /**<bit[7 : 7] */
		uint32_t lbwen                            :  1; /**<bit[8 : 8] */
		uint32_t calsel                           :  1; /**<bit[9 : 9] */
		uint32_t bp2vldo                          :  1; /**<bit[10 : 10] */
		uint32_t dcochg                           :  2; /**<bit[11 : 12] */
		uint32_t diffen                           :  1; /**<bit[13 : 13] */
		uint32_t endaccal                         :  1; /**<bit[14 : 14] */
		uint32_t rendcoc                          :  1; /**<bit[15 : 15] */
		uint32_t lendcoc                          :  1; /**<bit[16 : 16] */
		uint32_t renvcmd                          :  1; /**<bit[17 : 17] */
		uint32_t lenvcmd                          :  1; /**<bit[18 : 18] */
		uint32_t dacdrven                         :  1; /**<bit[19 : 19] */
		uint32_t dacren                           :  1; /**<bit[20 : 20] */
		uint32_t daclen                           :  1; /**<bit[21 : 21] */
		uint32_t dacg                             :  4; /**<bit[22 : 25] */
		uint32_t dacmute                          :  1; /**<bit[26 : 26] */
		uint32_t dacdwamode_sel                   :  1; /**<bit[27 : 27] */
		uint32_t ckpsel                           :  1; /**<bit[28 : 28] */
		uint32_t nc_29_31                         :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg29_t;


typedef volatile union {
	struct {
		uint32_t lmdcin                           :  8; /**<bit[0 : 7] */
		uint32_t rmdcin                           :  8; /**<bit[8 : 15] */
		uint32_t spirst_ovc                       :  1; /**<bit[16 : 16] */
		uint32_t enidacr                          :  1; /**<bit[17 : 17] */
		uint32_t enidacl                          :  1; /**<bit[18 : 18] */
		uint32_t dac3rdhc0v9                      :  1; /**<bit[19 : 19] */
		uint32_t hc2s                             :  1; /**<bit[20 : 20] */
		uint32_t sng_fb_en                        :  1; /**<bit[21 : 21] */
		uint32_t rfb_ctrl                         :  1; /**<bit[22 : 22] */
		uint32_t enbs                             :  1; /**<bit[23 : 23] */
		uint32_t calck_sel0v9                     :  1; /**<bit[24 : 24] */
		uint32_t bpdwa0v9                         :  1; /**<bit[25 : 25] */
		uint32_t looprst0v9                       :  1; /**<bit[26 : 26] */
		uint32_t oct0v9                           :  2; /**<bit[27 : 28] */
		uint32_t sout0v9                          :  1; /**<bit[29 : 29] */
		uint32_t hc0v9                            :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg30_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                          : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg31_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg32_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg33_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg34_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg35_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg36_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg37_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg38_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg39_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg40_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg41_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg42_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg43_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg44_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg45_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg46_t;


typedef volatile union {
	struct {
		uint32_t new_ana_reg                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_aonp_ana_reg47_t;

typedef volatile struct {
	volatile sys_aonp_reg0_t reg0;
	volatile sys_aonp_reg1_t reg1;
	volatile sys_aonp_reg2_t reg2;
	volatile sys_aonp_reg3_t reg3;
	volatile sys_aonp_reg4_t reg4;
	volatile sys_aonp_reg5_t reg5;
	volatile uint32_t rsv_6_7[2];
	volatile sys_aonp_reg8_t reg8;
	volatile sys_aonp_reg9_t reg9;
	volatile sys_aonp_rega_t rega;
	volatile sys_aonp_regb_t regb;
	volatile sys_aonp_regc_t regc;
	volatile sys_aonp_regd_t regd;
	volatile uint32_t rsv_e_e[1];
	volatile sys_aonp_regf_t regf;
	volatile sys_aonp_reg10_t reg10;
	volatile sys_aonp_reg11_t reg11;
	volatile uint32_t rsv_12_13[2];
	volatile sys_aonp_reg14_t reg14;
	volatile sys_aonp_reg15_t reg15;
	volatile sys_aonp_reg16_t reg16;
	volatile sys_aonp_reg17_t reg17;
	volatile sys_aonp_reg18_t reg18;
	volatile sys_aonp_reg19_t reg19;
	volatile sys_aonp_reg1a_t reg1a;
	volatile sys_aonp_reg1b_t reg1b;
	volatile sys_aonp_reg1c_t reg1c;
	volatile uint32_t rsv_1d_1d[1];
	volatile sys_aonp_reg1e_t reg1e;
	volatile sys_aonp_reg1f_t reg1f;
	volatile sys_aonp_reg20_t reg20;
	volatile sys_aonp_reg21_t reg21;
	volatile sys_aonp_reg22_t reg22;
	volatile sys_aonp_reg23_t reg23;
	volatile sys_aonp_reg24_t reg24;
	volatile sys_aonp_reg25_t reg25;
	volatile sys_aonp_reg26_t reg26;
	volatile sys_aonp_reg27_t reg27;
	volatile sys_aonp_reg28_t reg28;
	volatile uint32_t rsv_29_29[1];
	volatile sys_aonp_reg2a_t reg2a;
	volatile sys_aonp_reg2b_t reg2b;
	volatile sys_aonp_reg2c_t reg2c;
	volatile uint32_t rsv_2d_2d[1];
	volatile sys_aonp_reg2e_t reg2e;
	volatile sys_aonp_reg2f_t reg2f;
	volatile sys_aonp_reg30_t reg30;
	volatile sys_aonp_reg31_t reg31;
	volatile sys_aonp_reg32_t reg32;
	volatile sys_aonp_reg33_t reg33;
	volatile sys_aonp_reg34_t reg34;
	volatile sys_aonp_reg35_t reg35;
	volatile sys_aonp_reg36_t reg36;
	volatile sys_aonp_reg37_t reg37;
	volatile sys_aonp_reg38_t reg38;
	volatile sys_aonp_reg39_t reg39;
	volatile sys_aonp_reg3a_t reg3a;
	volatile sys_aonp_reg3b_t reg3b;
	volatile sys_aonp_reg3c_t reg3c;
	volatile sys_aonp_reg3d_t reg3d;
	volatile sys_aonp_reg3e_t reg3e;
	volatile sys_aonp_reg3f_t reg3f;
	volatile sys_aonp_ana_reg0_t ana_reg0;
	volatile sys_aonp_ana_reg1_t ana_reg1;
	volatile sys_aonp_ana_reg2_t ana_reg2;
	volatile sys_aonp_ana_reg3_t ana_reg3;
	volatile sys_aonp_ana_reg4_t ana_reg4;
	volatile sys_aonp_ana_reg5_t ana_reg5;
	volatile sys_aonp_ana_reg6_t ana_reg6;
	volatile sys_aonp_ana_reg7_t ana_reg7;
	volatile sys_aonp_ana_reg8_t ana_reg8;
	volatile sys_aonp_ana_reg9_t ana_reg9;
	volatile sys_aonp_ana_reg10_t ana_reg10;
	volatile sys_aonp_ana_reg11_t ana_reg11;
	volatile sys_aonp_ana_reg12_t ana_reg12;
	volatile sys_aonp_ana_reg13_t ana_reg13;
	volatile sys_aonp_ana_reg14_t ana_reg14;
	volatile sys_aonp_ana_reg15_t ana_reg15;
	volatile sys_aonp_ana_reg16_t ana_reg16;
	volatile sys_aonp_ana_reg17_t ana_reg17;
	volatile sys_aonp_ana_reg18_t ana_reg18;
	volatile sys_aonp_ana_reg19_t ana_reg19;
	volatile sys_aonp_ana_reg20_t ana_reg20;
	volatile sys_aonp_ana_reg21_t ana_reg21;
	volatile sys_aonp_ana_reg22_t ana_reg22;
	volatile sys_aonp_ana_reg23_t ana_reg23;
	volatile sys_aonp_ana_reg24_t ana_reg24;
	volatile sys_aonp_ana_reg25_t ana_reg25;
	volatile sys_aonp_ana_reg26_t ana_reg26;
	volatile sys_aonp_ana_reg27_t ana_reg27;
	volatile sys_aonp_ana_reg28_t ana_reg28;
	volatile sys_aonp_ana_reg29_t ana_reg29;
	volatile sys_aonp_ana_reg30_t ana_reg30;
	volatile sys_aonp_ana_reg31_t ana_reg31;
	volatile sys_aonp_ana_reg32_t ana_reg32;
	volatile sys_aonp_ana_reg33_t ana_reg33;
	volatile sys_aonp_ana_reg34_t ana_reg34;
	volatile sys_aonp_ana_reg35_t ana_reg35;
	volatile sys_aonp_ana_reg36_t ana_reg36;
	volatile sys_aonp_ana_reg37_t ana_reg37;
	volatile sys_aonp_ana_reg38_t ana_reg38;
	volatile sys_aonp_ana_reg39_t ana_reg39;
	volatile sys_aonp_ana_reg40_t ana_reg40;
	volatile sys_aonp_ana_reg41_t ana_reg41;
	volatile sys_aonp_ana_reg42_t ana_reg42;
	volatile sys_aonp_ana_reg43_t ana_reg43;
	volatile sys_aonp_ana_reg44_t ana_reg44;
	volatile sys_aonp_ana_reg45_t ana_reg45;
	volatile sys_aonp_ana_reg46_t ana_reg46;
	volatile sys_aonp_ana_reg47_t ana_reg47;
} sys_aonp_hw_t;

#ifdef __cplusplus
}
#endif
