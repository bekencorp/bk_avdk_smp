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
} sys_device_id_t;


typedef volatile union {
	struct {
		uint32_t versionid                        : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_version_id_t;


typedef volatile union {
	struct {
		uint32_t boot_mode                        :  1; /**<bit[0 : 0] */
		uint32_t clkg_bps                         :  1; /**<bit[1 : 1] */
		uint32_t reserved_bit_2_3                 :  2; /**<bit[2 : 3] */
		uint32_t rf_switch_manual_en              :  1; /**<bit[4 : 4] */
		uint32_t rf_source                        :  2; /**<bit[5 : 6] */
		uint32_t reserved_7_7                     :  1; /**<bit[7 : 7] */
		uint32_t reserved_bit_8_8                 :  1; /**<bit[8 : 8] */
		uint32_t flash_sel                        :  1; /**<bit[9 : 9] */
		uint32_t fem_bps_txen                     :  1; /**<bit[10 : 10] */
		uint32_t gpio_flash_sys_enable            :  1; /**<bit[11 : 11] */
		uint32_t boot_mode_norst                  :  1; /**<bit[12 : 12] */
		uint32_t reserved_bit_13_31               : 19; /**<bit[13 : 31] */
	};
	uint32_t v;
} sys_cpu_storage_connect_op_select_t;


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
} sys_cpu_current_run_status_t;


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
} sys_cpu0_int_halt_clk_op_t;


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
} sys_cpu1_int_halt_clk_op_t;


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
} sys_cpu_clk_div_mode1_t;


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
} sys_cpu_clk_div_mode2_t;


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
} sys_cpu_clk_div_mode3_t;


typedef volatile union {
	struct {
		uint32_t anaspi_freq                      :  6; /**<bit[0 : 5] */
		uint32_t reserved_bit_6_31                : 26; /**<bit[6 : 31] */
	};
	uint32_t v;
} sys_cpu_anaspi_freq_t;


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
} sys_cpu_device_clk_enable_t;


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
} sys_reserver_reg0xd_t;


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
} sys_reserver_reg0xf_t;


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
} sys_reserver_reg0x10_t;


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
} sys_cpu_power_sleep_wakeup_t;


typedef volatile union {
	struct {
		uint32_t cpu0_dma0_nsec_int_en            :  1; /**<bit[0 : 0] */
		uint32_t cpu0_encp_sec_intr_int_en        :  1; /**<bit[1 : 1] */
		uint32_t cpu0_encp_nsec_intr_int_en       :  1; /**<bit[2 : 2] */
		uint32_t cpu0_timer_int_en                :  1; /**<bit[3 : 3] */
		uint32_t cpu0_uart_int_en                 :  1; /**<bit[4 : 4] */
		uint32_t cpu0_pwm0_int_en                 :  1; /**<bit[5 : 5] */
		uint32_t cpu0_i2c0_int_en                 :  1; /**<bit[6 : 6] */
		uint32_t cpu0_spi0_int_en                 :  1; /**<bit[7 : 7] */
		uint32_t cpu0_sadc_int_en                 :  1; /**<bit[8 : 8] */
		uint32_t cpu0_irda_int_en                 :  1; /**<bit[9 : 9] */
		uint32_t cpu0_l2_sec_int_en               :  1; /**<bit[10 : 10] */
		uint32_t cpu0_dma0_sec_int_en             :  1; /**<bit[11 : 11] */
		uint32_t cpu0_la_int_en                   :  1; /**<bit[12 : 12] */
		uint32_t cpu0_acomp0_int_en               :  1; /**<bit[13 : 13] */
		uint32_t cpu0_acomp1_int_en               :  1; /**<bit[14 : 14] */
		uint32_t cpu0_uart1_int_en                :  1; /**<bit[15 : 15] */
		uint32_t cpu0_cpu0_fpu_int_en             :  1; /**<bit[16 : 16] */
		uint32_t cpu0_cpu1_fpu_int_en             :  1; /**<bit[17 : 17] */
		uint32_t cpu0_can_int_en                  :  1; /**<bit[18 : 18] */
		uint32_t cpu0_l2_nsec_int_en              :  1; /**<bit[19 : 19] */
		uint32_t cpu0_vid_disp0_int_en            :  1; /**<bit[20 : 20] */
		uint32_t cpu0_ckmn_int_en                 :  1; /**<bit[21 : 21] */
		uint32_t cpu0_vid_disp1_int_en            :  1; /**<bit[22 : 22] */
		uint32_t cpu0_aud_int_en                  :  1; /**<bit[23 : 23] */
		uint32_t cpu0_i2s0_int_en                 :  1; /**<bit[24 : 24] */
		uint32_t cpu0_i2s1_int_en                 :  1; /**<bit[25 : 25] */
		uint32_t cpu0_vid_disp2_int_en            :  1; /**<bit[26 : 26] */
		uint32_t cpu0_ipchecksum_int_en           :  1; /**<bit[27 : 27] */
		uint32_t cpu0_thread_int_en               :  1; /**<bit[28 : 28] */
		uint32_t cpu0_phy_mbp_int_en              :  1; /**<bit[29 : 29] */
		uint32_t cpu0_phy_riu_int_en              :  1; /**<bit[30 : 30] */
		uint32_t cpu0_mac_int_tx_rx_timer_n_int_en :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu0_int_0_31_en_t;


typedef volatile union {
	struct {
		uint32_t cpu0_mac_int_tx_rx_misc_n_int_en :  1; /**<bit[0 : 0] */
		uint32_t cpu0_mac_int_rx_trigger_n_int_en :  1; /**<bit[1 : 1] */
		uint32_t cpu0_mac_int_tx_trigger_n_int_en :  1; /**<bit[2 : 2] */
		uint32_t cpu0_mac_int_port_trigger_n_int_en :  1; /**<bit[3 : 3] */
		uint32_t cpu0_mac_int_gen_n_int_en        :  1; /**<bit[4 : 4] */
		uint32_t cpu0_gpio_ns_int_en              :  1; /**<bit[5 : 5] */
		uint32_t cpu0_int_mac_wakeup_int_en       :  1; /**<bit[6 : 6] */
		uint32_t cpu0_dm_irq_int_en               :  1; /**<bit[7 : 7] */
		uint32_t cpu0_ble_irq_int_en              :  1; /**<bit[8 : 8] */
		uint32_t cpu0_bt_irq_int_en               :  1; /**<bit[9 : 9] */
		uint32_t cpu0_btdm_wake_up_int_en         :  1; /**<bit[10 : 10] */
		uint32_t cpu0_touched_int_en              :  1; /**<bit[11 : 11] */
		uint32_t cpu0_i2s2_int_en                 :  1; /**<bit[12 : 12] */
		uint32_t cpu0_i2s3_int_en                 :  1; /**<bit[13 : 13] */
		uint32_t cpu0_spdif0_int_en               :  1; /**<bit[14 : 14] */
		uint32_t cpu0_cec_int_en                  :  1; /**<bit[15 : 15] */
		uint32_t cpu0_xdac0_int_en                :  1; /**<bit[16 : 16] */
		uint32_t cpu0_xdac1_int_en                :  1; /**<bit[17 : 17] */
		uint32_t cpu0_otp_int_en                  :  1; /**<bit[18 : 18] */
		uint32_t cpu0_dpll_unlock_int_en          :  1; /**<bit[19 : 19] */
		uint32_t cpu0_dco_unlock_int_en           :  1; /**<bit[20 : 20] */
		uint32_t cpu0_usbplug_int_en              :  1; /**<bit[21 : 21] */
		uint32_t cpu0_rtc_int_en                  :  1; /**<bit[22 : 22] */
		uint32_t cpu0_gpio_s_int_en               :  1; /**<bit[23 : 23] */
		uint32_t cpu0_uart2_int_en                :  1; /**<bit[24 : 24] */
		uint32_t cpu0_spi1_int_en                 :  1; /**<bit[25 : 25] */
		uint32_t cpu0_timer1_int_en               :  1; /**<bit[26 : 26] */
		uint32_t cpu0_spi3_int_en                 :  1; /**<bit[27 : 27] */
		uint32_t cpu0_scr_int_en                  :  1; /**<bit[28 : 28] */
		uint32_t cpu0_lin_int_en                  :  1; /**<bit[29 : 29] */
		uint32_t cpu0_can1_int_en                 :  1; /**<bit[30 : 30] */
		uint32_t cpu0_timer2_int_en               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu0_int_32_63_en_t;


typedef volatile union {
	struct {
		uint32_t cpu0_timer3_int_en               :  1; /**<bit[0 : 0] */
		uint32_t cpu0_uart3_int_en                :  1; /**<bit[1 : 1] */
		uint32_t cpu0_spi2_int_en                 :  1; /**<bit[2 : 2] */
		uint32_t cpu0_uart4_int_en                :  1; /**<bit[3 : 3] */
		uint32_t cpu0_i2c3_int_en                 :  1; /**<bit[4 : 4] */
		uint32_t cpu0_hspl_int_en                 :  1; /**<bit[5 : 5] */
		uint32_t cpu0_bk24_int_en                 :  1; /**<bit[6 : 6] */
		uint32_t cpu0_irda1_int_en                :  1; /**<bit[7 : 7] */
		uint32_t cpu0_irda2_int_en                :  1; /**<bit[8 : 8] */
		uint32_t cpu0_irda3_int_en                :  1; /**<bit[9 : 9] */
		uint32_t cpu0_i3c_int_en                  :  1; /**<bit[10 : 10] */
		uint32_t cpu0_i2s4_int_en                 :  1; /**<bit[11 : 11] */
		uint32_t cpu0_spdif1_int_en               :  1; /**<bit[12 : 12] */
		uint32_t cpu0_int_m55sub_int_en           :  1; /**<bit[13 : 13] */
		uint32_t cpu0_mailbox_int_en              :  1; /**<bit[14 : 14] */
		uint32_t cpu0_ipi_int_en                  :  1; /**<bit[15 : 15] */
		uint32_t cpu0_vid_disp3_int_en            :  1; /**<bit[16 : 16] */
		uint32_t cpu0_vad_int_en                  :  1; /**<bit[17 : 17] */
		uint32_t cpu0_resv82_int_en               :  1; /**<bit[18 : 18] */
		uint32_t cpu0_resv83_int_en               :  1; /**<bit[19 : 19] */
		uint32_t cpu0_resv84_int_en               :  1; /**<bit[20 : 20] */
		uint32_t cpu0_resv85_int_en               :  1; /**<bit[21 : 21] */
		uint32_t cpu0_resv86_int_en               :  1; /**<bit[22 : 22] */
		uint32_t cpu0_resv87_int_en               :  1; /**<bit[23 : 23] */
		uint32_t cpu0_resv88_int_en               :  1; /**<bit[24 : 24] */
		uint32_t cpu0_resv89_int_en               :  1; /**<bit[25 : 25] */
		uint32_t cpu0_resv90_int_en               :  1; /**<bit[26 : 26] */
		uint32_t cpu0_resv91_int_en               :  1; /**<bit[27 : 27] */
		uint32_t cpu0_resv92_int_en               :  1; /**<bit[28 : 28] */
		uint32_t cpu0_resv93_int_en               :  1; /**<bit[29 : 29] */
		uint32_t cpu0_resv94_int_en               :  1; /**<bit[30 : 30] */
		uint32_t cpu0_resv95_int_en               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu0_int_64_95_en_t;


typedef volatile union {
	struct {
		uint32_t cpu1_dma0_nsec_int_en            :  1; /**<bit[0 : 0] */
		uint32_t cpu1_encp_sec_intr_int_en        :  1; /**<bit[1 : 1] */
		uint32_t cpu1_encp_nsec_intr_int_en       :  1; /**<bit[2 : 2] */
		uint32_t cpu1_timer_int_en                :  1; /**<bit[3 : 3] */
		uint32_t cpu1_uart_int_en                 :  1; /**<bit[4 : 4] */
		uint32_t cpu1_pwm0_int_en                 :  1; /**<bit[5 : 5] */
		uint32_t cpu1_i2c0_int_en                 :  1; /**<bit[6 : 6] */
		uint32_t cpu1_spi0_int_en                 :  1; /**<bit[7 : 7] */
		uint32_t cpu1_sadc_int_en                 :  1; /**<bit[8 : 8] */
		uint32_t cpu1_irda_int_en                 :  1; /**<bit[9 : 9] */
		uint32_t cpu1_l2_sec_int_en               :  1; /**<bit[10 : 10] */
		uint32_t cpu1_dma0_sec_int_en             :  1; /**<bit[11 : 11] */
		uint32_t cpu1_la_int_en                   :  1; /**<bit[12 : 12] */
		uint32_t cpu1_acomp0_int_en               :  1; /**<bit[13 : 13] */
		uint32_t cpu1_acomp1_int_en               :  1; /**<bit[14 : 14] */
		uint32_t cpu1_uart1_int_en                :  1; /**<bit[15 : 15] */
		uint32_t cpu1_cpu0_fpu_int_en             :  1; /**<bit[16 : 16] */
		uint32_t cpu1_cpu1_fpu_int_en             :  1; /**<bit[17 : 17] */
		uint32_t cpu1_can_int_en                  :  1; /**<bit[18 : 18] */
		uint32_t cpu1_l2_nsec_int_en              :  1; /**<bit[19 : 19] */
		uint32_t cpu1_vid_disp0_int_en            :  1; /**<bit[20 : 20] */
		uint32_t cpu1_ckmn_int_en                 :  1; /**<bit[21 : 21] */
		uint32_t cpu1_vid_disp1_int_en            :  1; /**<bit[22 : 22] */
		uint32_t cpu1_aud_int_en                  :  1; /**<bit[23 : 23] */
		uint32_t cpu1_i2s0_int_en                 :  1; /**<bit[24 : 24] */
		uint32_t cpu1_i2s1_int_en                 :  1; /**<bit[25 : 25] */
		uint32_t cpu1_vid_disp2_int_en            :  1; /**<bit[26 : 26] */
		uint32_t cpu1_ipchecksum_int_en           :  1; /**<bit[27 : 27] */
		uint32_t cpu1_thread_int_en               :  1; /**<bit[28 : 28] */
		uint32_t cpu1_phy_mbp_int_en              :  1; /**<bit[29 : 29] */
		uint32_t cpu1_phy_riu_int_en              :  1; /**<bit[30 : 30] */
		uint32_t cpu1_mac_int_tx_rx_timer_n_int_en :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu1_int_0_31_en_t;


typedef volatile union {
	struct {
		uint32_t cpu1_mac_int_tx_rx_misc_n_int_en :  1; /**<bit[0 : 0] */
		uint32_t cpu1_mac_int_rx_trigger_n_int_en :  1; /**<bit[1 : 1] */
		uint32_t cpu1_mac_int_tx_trigger_n_int_en :  1; /**<bit[2 : 2] */
		uint32_t cpu1_mac_int_port_trigger_n_int_en :  1; /**<bit[3 : 3] */
		uint32_t cpu1_mac_int_gen_n_int_en        :  1; /**<bit[4 : 4] */
		uint32_t cpu1_gpio_ns_int_en              :  1; /**<bit[5 : 5] */
		uint32_t cpu1_int_mac_wakeup_int_en       :  1; /**<bit[6 : 6] */
		uint32_t cpu1_dm_irq_int_en               :  1; /**<bit[7 : 7] */
		uint32_t cpu1_ble_irq_int_en              :  1; /**<bit[8 : 8] */
		uint32_t cpu1_bt_irq_int_en               :  1; /**<bit[9 : 9] */
		uint32_t cpu1_btdm_wake_up_int_en         :  1; /**<bit[10 : 10] */
		uint32_t cpu1_touched_int_en              :  1; /**<bit[11 : 11] */
		uint32_t cpu1_i2s2_int_en                 :  1; /**<bit[12 : 12] */
		uint32_t cpu1_i2s3_int_en                 :  1; /**<bit[13 : 13] */
		uint32_t cpu1_spdif0_int_en               :  1; /**<bit[14 : 14] */
		uint32_t cpu1_cec_int_en                  :  1; /**<bit[15 : 15] */
		uint32_t cpu1_xdac0_int_en                :  1; /**<bit[16 : 16] */
		uint32_t cpu1_xdac1_int_en                :  1; /**<bit[17 : 17] */
		uint32_t cpu1_otp_int_en                  :  1; /**<bit[18 : 18] */
		uint32_t cpu1_dpll_unlock_int_en          :  1; /**<bit[19 : 19] */
		uint32_t cpu1_dco_unlock_int_en           :  1; /**<bit[20 : 20] */
		uint32_t cpu1_usbplug_int_en              :  1; /**<bit[21 : 21] */
		uint32_t cpu1_rtc_int_en                  :  1; /**<bit[22 : 22] */
		uint32_t cpu1_gpio_s_int_en               :  1; /**<bit[23 : 23] */
		uint32_t cpu1_uart2_int_en                :  1; /**<bit[24 : 24] */
		uint32_t cpu1_spi1_int_en                 :  1; /**<bit[25 : 25] */
		uint32_t cpu1_timer1_int_en               :  1; /**<bit[26 : 26] */
		uint32_t cpu1_spi3_int_en                 :  1; /**<bit[27 : 27] */
		uint32_t cpu1_scr_int_en                  :  1; /**<bit[28 : 28] */
		uint32_t cpu1_lin_int_en                  :  1; /**<bit[29 : 29] */
		uint32_t cpu1_can1_int_en                 :  1; /**<bit[30 : 30] */
		uint32_t cpu1_timer2_int_en               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu1_int_32_63_en_t;


typedef volatile union {
	struct {
		uint32_t cpu1_timer3_int_en               :  1; /**<bit[0 : 0] */
		uint32_t cpu1_uart3_int_en                :  1; /**<bit[1 : 1] */
		uint32_t cpu1_spi2_int_en                 :  1; /**<bit[2 : 2] */
		uint32_t cpu1_uart4_int_en                :  1; /**<bit[3 : 3] */
		uint32_t cpu1_i2c3_int_en                 :  1; /**<bit[4 : 4] */
		uint32_t cpu1_hspl_int_en                 :  1; /**<bit[5 : 5] */
		uint32_t cpu1_bk24_int_en                 :  1; /**<bit[6 : 6] */
		uint32_t cpu1_irda1_int_en                :  1; /**<bit[7 : 7] */
		uint32_t cpu1_irda2_int_en                :  1; /**<bit[8 : 8] */
		uint32_t cpu1_irda3_int_en                :  1; /**<bit[9 : 9] */
		uint32_t cpu1_i3c_int_en                  :  1; /**<bit[10 : 10] */
		uint32_t cpu1_i2s4_int_en                 :  1; /**<bit[11 : 11] */
		uint32_t cpu1_spdif1_int_en               :  1; /**<bit[12 : 12] */
		uint32_t cpu1_int_m55sub_int_en           :  1; /**<bit[13 : 13] */
		uint32_t cpu1_mailbox_int_en              :  1; /**<bit[14 : 14] */
		uint32_t cpu1_ipi_int_en                  :  1; /**<bit[15 : 15] */
		uint32_t cpu1_vid_disp3_int_en            :  1; /**<bit[16 : 16] */
		uint32_t cpu1_vad_int_en                  :  1; /**<bit[17 : 17] */
		uint32_t cpu1_resv82_int_en               :  1; /**<bit[18 : 18] */
		uint32_t cpu1_resv83_int_en               :  1; /**<bit[19 : 19] */
		uint32_t cpu1_resv84_int_en               :  1; /**<bit[20 : 20] */
		uint32_t cpu1_resv85_int_en               :  1; /**<bit[21 : 21] */
		uint32_t cpu1_resv86_int_en               :  1; /**<bit[22 : 22] */
		uint32_t cpu1_resv87_int_en               :  1; /**<bit[23 : 23] */
		uint32_t cpu1_resv88_int_en               :  1; /**<bit[24 : 24] */
		uint32_t cpu1_resv89_int_en               :  1; /**<bit[25 : 25] */
		uint32_t cpu1_resv90_int_en               :  1; /**<bit[26 : 26] */
		uint32_t cpu1_resv91_int_en               :  1; /**<bit[27 : 27] */
		uint32_t cpu1_resv92_int_en               :  1; /**<bit[28 : 28] */
		uint32_t cpu1_resv93_int_en               :  1; /**<bit[29 : 29] */
		uint32_t cpu1_resv94_int_en               :  1; /**<bit[30 : 30] */
		uint32_t cpu1_resv95_int_en               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu1_int_64_95_en_t;


typedef volatile union {
	struct {
		uint32_t m55sub_m52s_int_en               :  1; /**<bit[0 : 0] */
		uint32_t m55sub_gdma1_int_en              :  1; /**<bit[1 : 1] */
		uint32_t m55sub_mbox_int_en               :  1; /**<bit[2 : 2] */
		uint32_t m55sub_ipi_int_en                :  1; /**<bit[3 : 3] */
		uint32_t m55sub_gdma0_int_en              :  1; /**<bit[4 : 4] */
		uint32_t m55sub_cpu_fpu_int_int_en        :  1; /**<bit[5 : 5] */
		uint32_t m55sub_npu_int_en                :  1; /**<bit[6 : 6] */
		uint32_t m55sub_usb_fs_int_int_en         :  1; /**<bit[7 : 7] */
		uint32_t m55sub_usb_hs_int_int_en         :  1; /**<bit[8 : 8] */
		uint32_t m55sub_usb_plug_int_en           :  1; /**<bit[9 : 9] */
		uint32_t m55sub_uart5_int_en              :  1; /**<bit[10 : 10] */
		uint32_t m55sub_wwdt_int_en               :  1; /**<bit[11 : 11] */
		uint32_t m55sub_sdio0_int_en              :  1; /**<bit[12 : 12] */
		uint32_t m55sub_sdio1_int_en              :  1; /**<bit[13 : 13] */
		uint32_t m55sub_enet0_int_en              :  1; /**<bit[14 : 14] */
		uint32_t m55sub_enet1_int_en              :  1; /**<bit[15 : 15] */
		uint32_t m55sub_qspi0_int_en              :  1; /**<bit[16 : 16] */
		uint32_t m55sub_qspi1_int_en              :  1; /**<bit[17 : 17] */
		uint32_t m55sub_hspl_int_en               :  1; /**<bit[18 : 18] */
		uint32_t m55sub_isp_mi_int_en             :  1; /**<bit[19 : 19] */
		uint32_t m55sub_isp_fe_int_en             :  1; /**<bit[20 : 20] */
		uint32_t m55sub_isp_isp_int_en            :  1; /**<bit[21 : 21] */
		uint32_t m55sub_csi_int_en                :  1; /**<bit[22 : 22] */
		uint32_t m55sub_h26e_int_en               :  1; /**<bit[23 : 23] */
		uint32_t m55sub_vid_disp0_int_en          :  1; /**<bit[24 : 24] */
		uint32_t m55sub_vid_disp1_int_en          :  1; /**<bit[25 : 25] */
		uint32_t m55sub_vid_disp2_int_en          :  1; /**<bit[26 : 26] */
		uint32_t m55sub_vid_disp3_int_en          :  1; /**<bit[27 : 27] */
		uint32_t m55sub_vid_disp4_int_en          :  1; /**<bit[28 : 28] */
		uint32_t m55sub_psram0_err_int_en         :  1; /**<bit[29 : 29] */
		uint32_t m55sub_psram1_err_int_en         :  1; /**<bit[30 : 30] */
		uint32_t m55sub_mpc_int_en                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_m55sub_int_0_31_en_t;


typedef volatile union {
	struct {
		uint32_t m55sub_timer4_int_en             :  1; /**<bit[0 : 0] */
		uint32_t m55sub_timer5_int_en             :  1; /**<bit[1 : 1] */
		uint32_t m55sub_int_gpio_ns_int_en        :  1; /**<bit[2 : 2] */
		uint32_t m55sub_int_gpio_s_int_en         :  1; /**<bit[3 : 3] */
		uint32_t m55sub_int_audio_int_en          :  1; /**<bit[4 : 4] */
		uint32_t m55sub_int_i2s0_int_en           :  1; /**<bit[5 : 5] */
		uint32_t m55sub_int_i2s1_int_en           :  1; /**<bit[6 : 6] */
		uint32_t m55sub_int_i2s2_int_en           :  1; /**<bit[7 : 7] */
		uint32_t m55sub_int_i2s3_int_en           :  1; /**<bit[8 : 8] */
		uint32_t m55sub_int_i2s4_int_en           :  1; /**<bit[9 : 9] */
		uint32_t m55sub_int_spdif0_int_en         :  1; /**<bit[10 : 10] */
		uint32_t m55sub_int_spdif1_int_en         :  1; /**<bit[11 : 11] */
		uint32_t m55sub_int_cec_int_en            :  1; /**<bit[12 : 12] */
		uint32_t m55sub_int_i2c_0_int_en          :  1; /**<bit[13 : 13] */
		uint32_t m55sub_int_i2c_3_int_en          :  1; /**<bit[14 : 14] */
		uint32_t m55sub_int_i3c_int_en            :  1; /**<bit[15 : 15] */
		uint32_t m55sub_int_uart0_int_en          :  1; /**<bit[16 : 16] */
		uint32_t m55sub_int_uart1_int_en          :  1; /**<bit[17 : 17] */
		uint32_t m55sub_int_uart2_int_en          :  1; /**<bit[18 : 18] */
		uint32_t m55sub_int_uart3_int_en          :  1; /**<bit[19 : 19] */
		uint32_t m55sub_int_uart4_int_en          :  1; /**<bit[20 : 20] */
		uint32_t m55sub_int_l2cache_int_en        :  1; /**<bit[21 : 21] */
		uint32_t m55sub_resv54_int_en             :  1; /**<bit[22 : 22] */
		uint32_t m55sub_resv55_int_en             :  1; /**<bit[23 : 23] */
		uint32_t m55sub_resv56_int_en             :  1; /**<bit[24 : 24] */
		uint32_t m55sub_resv57_int_en             :  1; /**<bit[25 : 25] */
		uint32_t m55sub_resv58_int_en             :  1; /**<bit[26 : 26] */
		uint32_t m55sub_resv59_int_en             :  1; /**<bit[27 : 27] */
		uint32_t m55sub_resv60_int_en             :  1; /**<bit[28 : 28] */
		uint32_t m55sub_resv61_int_en             :  1; /**<bit[29 : 29] */
		uint32_t m55sub_resv62_int_en             :  1; /**<bit[30 : 30] */
		uint32_t m55sub_resv63_int_en             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_m55sub_int_32_63_en_t;


typedef volatile union {
	struct {
		uint32_t m55sub_resv64_int_en             :  1; /**<bit[0 : 0] */
		uint32_t m55sub_resv65_int_en             :  1; /**<bit[1 : 1] */
		uint32_t m55sub_resv66_int_en             :  1; /**<bit[2 : 2] */
		uint32_t m55sub_resv67_int_en             :  1; /**<bit[3 : 3] */
		uint32_t m55sub_resv68_int_en             :  1; /**<bit[4 : 4] */
		uint32_t m55sub_resv69_int_en             :  1; /**<bit[5 : 5] */
		uint32_t m55sub_resv70_int_en             :  1; /**<bit[6 : 6] */
		uint32_t m55sub_resv71_int_en             :  1; /**<bit[7 : 7] */
		uint32_t m55sub_resv72_int_en             :  1; /**<bit[8 : 8] */
		uint32_t m55sub_resv73_int_en             :  1; /**<bit[9 : 9] */
		uint32_t m55sub_resv74_int_en             :  1; /**<bit[10 : 10] */
		uint32_t m55sub_resv75_int_en             :  1; /**<bit[11 : 11] */
		uint32_t m55sub_resv76_int_en             :  1; /**<bit[12 : 12] */
		uint32_t m55sub_resv77_int_en             :  1; /**<bit[13 : 13] */
		uint32_t m55sub_resv78_int_en             :  1; /**<bit[14 : 14] */
		uint32_t m55sub_resv79_int_en             :  1; /**<bit[15 : 15] */
		uint32_t m55sub_resv80_int_en             :  1; /**<bit[16 : 16] */
		uint32_t m55sub_resv81_int_en             :  1; /**<bit[17 : 17] */
		uint32_t m55sub_resv82_int_en             :  1; /**<bit[18 : 18] */
		uint32_t m55sub_resv83_int_en             :  1; /**<bit[19 : 19] */
		uint32_t m55sub_resv84_int_en             :  1; /**<bit[20 : 20] */
		uint32_t m55sub_resv85_int_en             :  1; /**<bit[21 : 21] */
		uint32_t m55sub_resv86_int_en             :  1; /**<bit[22 : 22] */
		uint32_t m55sub_resv87_int_en             :  1; /**<bit[23 : 23] */
		uint32_t m55sub_resv88_int_en             :  1; /**<bit[24 : 24] */
		uint32_t m55sub_resv89_int_en             :  1; /**<bit[25 : 25] */
		uint32_t m55sub_resv90_int_en             :  1; /**<bit[26 : 26] */
		uint32_t m55sub_resv91_int_en             :  1; /**<bit[27 : 27] */
		uint32_t m55sub_resv92_int_en             :  1; /**<bit[28 : 28] */
		uint32_t m55sub_resv93_int_en             :  1; /**<bit[29 : 29] */
		uint32_t m55sub_resv94_int_en             :  1; /**<bit[30 : 30] */
		uint32_t m55sub_wakeup                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_m55sub_int_64_95_en_t;


typedef volatile union {
	struct {
		uint32_t spsh_cfg                         : 10; /**<bit[0 : 9] */
		uint32_t spbh_cfg                         : 11; /**<bit[10 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x1e_t;


typedef volatile union {
	struct {
		uint32_t stph_cfg                         : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_23               : 12; /**<bit[12 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x1f_t;


typedef volatile union {
	struct {
		uint32_t cpu0_dma0_nsec_int_st            :  1; /**<bit[0 : 0] */
		uint32_t cpu0_encp_sec_intr_int_st        :  1; /**<bit[1 : 1] */
		uint32_t cpu0_encp_nsec_intr_int_st       :  1; /**<bit[2 : 2] */
		uint32_t cpu0_timer_int_st                :  1; /**<bit[3 : 3] */
		uint32_t cpu0_uart_int_st                 :  1; /**<bit[4 : 4] */
		uint32_t cpu0_pwm0_int_st                 :  1; /**<bit[5 : 5] */
		uint32_t cpu0_i2c0_int_st                 :  1; /**<bit[6 : 6] */
		uint32_t cpu0_spi0_int_st                 :  1; /**<bit[7 : 7] */
		uint32_t cpu0_sadc_int_st                 :  1; /**<bit[8 : 8] */
		uint32_t cpu0_irda_int_st                 :  1; /**<bit[9 : 9] */
		uint32_t cpu0_l2_sec_int_st               :  1; /**<bit[10 : 10] */
		uint32_t cpu0_dma0_sec_int_st             :  1; /**<bit[11 : 11] */
		uint32_t cpu0_la_int_st                   :  1; /**<bit[12 : 12] */
		uint32_t cpu0_acomp0_int_st               :  1; /**<bit[13 : 13] */
		uint32_t cpu0_acomp1_int_st               :  1; /**<bit[14 : 14] */
		uint32_t cpu0_uart1_int_st                :  1; /**<bit[15 : 15] */
		uint32_t cpu0_cpu0_fpu_int_st             :  1; /**<bit[16 : 16] */
		uint32_t cpu0_cpu1_fpu_int_st             :  1; /**<bit[17 : 17] */
		uint32_t cpu0_can_int_st                  :  1; /**<bit[18 : 18] */
		uint32_t cpu0_l2_nsec_int_st              :  1; /**<bit[19 : 19] */
		uint32_t cpu0_vid_disp0_int_st            :  1; /**<bit[20 : 20] */
		uint32_t cpu0_ckmn_int_st                 :  1; /**<bit[21 : 21] */
		uint32_t cpu0_vid_disp1_int_st            :  1; /**<bit[22 : 22] */
		uint32_t cpu0_aud_int_st                  :  1; /**<bit[23 : 23] */
		uint32_t cpu0_i2s0_int_st                 :  1; /**<bit[24 : 24] */
		uint32_t cpu0_i2s1_int_st                 :  1; /**<bit[25 : 25] */
		uint32_t cpu0_vid_disp2_int_st            :  1; /**<bit[26 : 26] */
		uint32_t cpu0_ipchecksum_int_st           :  1; /**<bit[27 : 27] */
		uint32_t cpu0_thread_int_st               :  1; /**<bit[28 : 28] */
		uint32_t cpu0_phy_mbp_int_st              :  1; /**<bit[29 : 29] */
		uint32_t cpu0_phy_riu_int_st              :  1; /**<bit[30 : 30] */
		uint32_t cpu0_mac_int_tx_rx_timer_n_int_st :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu0_int_0_31_status_t;


typedef volatile union {
	struct {
		uint32_t cpu0_mac_int_tx_rx_misc_n_int_st :  1; /**<bit[0 : 0] */
		uint32_t cpu0_mac_int_rx_trigger_n_int_st :  1; /**<bit[1 : 1] */
		uint32_t cpu0_mac_int_tx_trigger_n_int_st :  1; /**<bit[2 : 2] */
		uint32_t cpu0_mac_int_port_trigger_n_int_st :  1; /**<bit[3 : 3] */
		uint32_t cpu0_mac_int_gen_n_int_st        :  1; /**<bit[4 : 4] */
		uint32_t cpu0_gpio_ns_int_st              :  1; /**<bit[5 : 5] */
		uint32_t cpu0_int_mac_wakeup_int_st       :  1; /**<bit[6 : 6] */
		uint32_t cpu0_dm_irq_int_st               :  1; /**<bit[7 : 7] */
		uint32_t cpu0_ble_irq_int_st              :  1; /**<bit[8 : 8] */
		uint32_t cpu0_bt_irq_int_st               :  1; /**<bit[9 : 9] */
		uint32_t cpu0_btdm_wake_up_int_st         :  1; /**<bit[10 : 10] */
		uint32_t cpu0_touched_int_st              :  1; /**<bit[11 : 11] */
		uint32_t cpu0_i2s2_int_st                 :  1; /**<bit[12 : 12] */
		uint32_t cpu0_i2s3_int_st                 :  1; /**<bit[13 : 13] */
		uint32_t cpu0_spdif0_int_st               :  1; /**<bit[14 : 14] */
		uint32_t cpu0_cec_int_st                  :  1; /**<bit[15 : 15] */
		uint32_t cpu0_xdac0_int_st                :  1; /**<bit[16 : 16] */
		uint32_t cpu0_xdac1_int_st                :  1; /**<bit[17 : 17] */
		uint32_t cpu0_otp_int_st                  :  1; /**<bit[18 : 18] */
		uint32_t cpu0_dpll_unlock_int_st          :  1; /**<bit[19 : 19] */
		uint32_t cpu0_dco_unlock_int_st           :  1; /**<bit[20 : 20] */
		uint32_t cpu0_usbplug_int_st              :  1; /**<bit[21 : 21] */
		uint32_t cpu0_rtc_int_st                  :  1; /**<bit[22 : 22] */
		uint32_t cpu0_gpio_s_int_st               :  1; /**<bit[23 : 23] */
		uint32_t cpu0_uart2_int_st                :  1; /**<bit[24 : 24] */
		uint32_t cpu0_spi1_int_st                 :  1; /**<bit[25 : 25] */
		uint32_t cpu0_timer1_int_st               :  1; /**<bit[26 : 26] */
		uint32_t cpu0_spi3_int_st                 :  1; /**<bit[27 : 27] */
		uint32_t cpu0_scr_int_st                  :  1; /**<bit[28 : 28] */
		uint32_t cpu0_lin_int_st                  :  1; /**<bit[29 : 29] */
		uint32_t cpu0_can1_int_st                 :  1; /**<bit[30 : 30] */
		uint32_t cpu0_timer2_int_st               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu0_int_32_63_status_t;


typedef volatile union {
	struct {
		uint32_t cpu0_timer3_int_st               :  1; /**<bit[0 : 0] */
		uint32_t cpu0_uart3_int_st                :  1; /**<bit[1 : 1] */
		uint32_t cpu0_spi2_int_st                 :  1; /**<bit[2 : 2] */
		uint32_t cpu0_uart4_int_st                :  1; /**<bit[3 : 3] */
		uint32_t cpu0_i2c3_int_st                 :  1; /**<bit[4 : 4] */
		uint32_t cpu0_hspl_int_st                 :  1; /**<bit[5 : 5] */
		uint32_t cpu0_bk24_int_st                 :  1; /**<bit[6 : 6] */
		uint32_t cpu0_irda1_int_st                :  1; /**<bit[7 : 7] */
		uint32_t cpu0_irda2_int_st                :  1; /**<bit[8 : 8] */
		uint32_t cpu0_irda3_int_st                :  1; /**<bit[9 : 9] */
		uint32_t cpu0_i3c_int_st                  :  1; /**<bit[10 : 10] */
		uint32_t cpu0_i2s4_int_st                 :  1; /**<bit[11 : 11] */
		uint32_t cpu0_spdif1_int_st               :  1; /**<bit[12 : 12] */
		uint32_t cpu0_int_m55sub_int_st           :  1; /**<bit[13 : 13] */
		uint32_t cpu0_mailbox_int_st              :  1; /**<bit[14 : 14] */
		uint32_t cpu0_ipi_int_st                  :  1; /**<bit[15 : 15] */
		uint32_t cpu0_vid_disp3_int_st            :  1; /**<bit[16 : 16] */
		uint32_t cpu0_vad_int_st                  :  1; /**<bit[17 : 17] */
		uint32_t cpu0_resv82_int_st               :  1; /**<bit[18 : 18] */
		uint32_t cpu0_resv83_int_st               :  1; /**<bit[19 : 19] */
		uint32_t cpu0_resv84_int_st               :  1; /**<bit[20 : 20] */
		uint32_t cpu0_resv85_int_st               :  1; /**<bit[21 : 21] */
		uint32_t cpu0_resv86_int_st               :  1; /**<bit[22 : 22] */
		uint32_t cpu0_resv87_int_st               :  1; /**<bit[23 : 23] */
		uint32_t cpu0_resv88_int_st               :  1; /**<bit[24 : 24] */
		uint32_t cpu0_resv89_int_st               :  1; /**<bit[25 : 25] */
		uint32_t cpu0_resv90_int_st               :  1; /**<bit[26 : 26] */
		uint32_t cpu0_resv91_int_st               :  1; /**<bit[27 : 27] */
		uint32_t cpu0_resv92_int_st               :  1; /**<bit[28 : 28] */
		uint32_t cpu0_resv93_int_st               :  1; /**<bit[29 : 29] */
		uint32_t cpu0_resv94_int_st               :  1; /**<bit[30 : 30] */
		uint32_t cpu0_resv95_int_st               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu0_int_64_95_status_t;


typedef volatile union {
	struct {
		uint32_t cpu1_dma0_nsec_int_st            :  1; /**<bit[0 : 0] */
		uint32_t cpu1_encp_sec_intr_int_st        :  1; /**<bit[1 : 1] */
		uint32_t cpu1_encp_nsec_intr_int_st       :  1; /**<bit[2 : 2] */
		uint32_t cpu1_timer_int_st                :  1; /**<bit[3 : 3] */
		uint32_t cpu1_uart_int_st                 :  1; /**<bit[4 : 4] */
		uint32_t cpu1_pwm0_int_st                 :  1; /**<bit[5 : 5] */
		uint32_t cpu1_i2c0_int_st                 :  1; /**<bit[6 : 6] */
		uint32_t cpu1_spi0_int_st                 :  1; /**<bit[7 : 7] */
		uint32_t cpu1_sadc_int_st                 :  1; /**<bit[8 : 8] */
		uint32_t cpu1_irda_int_st                 :  1; /**<bit[9 : 9] */
		uint32_t cpu1_l2_sec_int_st               :  1; /**<bit[10 : 10] */
		uint32_t cpu1_dma0_sec_int_st             :  1; /**<bit[11 : 11] */
		uint32_t cpu1_la_int_st                   :  1; /**<bit[12 : 12] */
		uint32_t cpu1_acomp0_int_st               :  1; /**<bit[13 : 13] */
		uint32_t cpu1_acomp1_int_st               :  1; /**<bit[14 : 14] */
		uint32_t cpu1_uart1_int_st                :  1; /**<bit[15 : 15] */
		uint32_t cpu1_cpu0_fpu_int_st             :  1; /**<bit[16 : 16] */
		uint32_t cpu1_cpu1_fpu_int_st             :  1; /**<bit[17 : 17] */
		uint32_t cpu1_can_int_st                  :  1; /**<bit[18 : 18] */
		uint32_t cpu1_l2_nsec_int_st              :  1; /**<bit[19 : 19] */
		uint32_t cpu1_vid_disp0_int_st            :  1; /**<bit[20 : 20] */
		uint32_t cpu1_ckmn_int_st                 :  1; /**<bit[21 : 21] */
		uint32_t cpu1_vid_disp1_int_st            :  1; /**<bit[22 : 22] */
		uint32_t cpu1_aud_int_st                  :  1; /**<bit[23 : 23] */
		uint32_t cpu1_i2s0_int_st                 :  1; /**<bit[24 : 24] */
		uint32_t cpu1_i2s1_int_st                 :  1; /**<bit[25 : 25] */
		uint32_t cpu1_vid_disp2_int_st            :  1; /**<bit[26 : 26] */
		uint32_t cpu1_ipchecksum_int_st           :  1; /**<bit[27 : 27] */
		uint32_t cpu1_thread_int_st               :  1; /**<bit[28 : 28] */
		uint32_t cpu1_phy_mbp_int_st              :  1; /**<bit[29 : 29] */
		uint32_t cpu1_phy_riu_int_st              :  1; /**<bit[30 : 30] */
		uint32_t cpu1_mac_int_tx_rx_timer_n_int_st :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu1_int_0_31_status_t;


typedef volatile union {
	struct {
		uint32_t cpu1_mac_int_tx_rx_misc_n_int_st :  1; /**<bit[0 : 0] */
		uint32_t cpu1_mac_int_rx_trigger_n_int_st :  1; /**<bit[1 : 1] */
		uint32_t cpu1_mac_int_tx_trigger_n_int_st :  1; /**<bit[2 : 2] */
		uint32_t cpu1_mac_int_port_trigger_n_int_st :  1; /**<bit[3 : 3] */
		uint32_t cpu1_mac_int_gen_n_int_st        :  1; /**<bit[4 : 4] */
		uint32_t cpu1_gpio_ns_int_st              :  1; /**<bit[5 : 5] */
		uint32_t cpu1_int_mac_wakeup_int_st       :  1; /**<bit[6 : 6] */
		uint32_t cpu1_dm_irq_int_st               :  1; /**<bit[7 : 7] */
		uint32_t cpu1_ble_irq_int_st              :  1; /**<bit[8 : 8] */
		uint32_t cpu1_bt_irq_int_st               :  1; /**<bit[9 : 9] */
		uint32_t cpu1_btdm_wake_up_int_st         :  1; /**<bit[10 : 10] */
		uint32_t cpu1_touched_int_st              :  1; /**<bit[11 : 11] */
		uint32_t cpu1_i2s2_int_st                 :  1; /**<bit[12 : 12] */
		uint32_t cpu1_i2s3_int_st                 :  1; /**<bit[13 : 13] */
		uint32_t cpu1_spdif0_int_st               :  1; /**<bit[14 : 14] */
		uint32_t cpu1_cec_int_st                  :  1; /**<bit[15 : 15] */
		uint32_t cpu1_xdac0_int_st                :  1; /**<bit[16 : 16] */
		uint32_t cpu1_xdac1_int_st                :  1; /**<bit[17 : 17] */
		uint32_t cpu1_otp_int_st                  :  1; /**<bit[18 : 18] */
		uint32_t cpu1_dpll_unlock_int_st          :  1; /**<bit[19 : 19] */
		uint32_t cpu1_dco_unlock_int_st           :  1; /**<bit[20 : 20] */
		uint32_t cpu1_usbplug_int_st              :  1; /**<bit[21 : 21] */
		uint32_t cpu1_rtc_int_st                  :  1; /**<bit[22 : 22] */
		uint32_t cpu1_gpio_s_int_st               :  1; /**<bit[23 : 23] */
		uint32_t cpu1_uart2_int_st                :  1; /**<bit[24 : 24] */
		uint32_t cpu1_spi1_int_st                 :  1; /**<bit[25 : 25] */
		uint32_t cpu1_timer1_int_st               :  1; /**<bit[26 : 26] */
		uint32_t cpu1_spi3_int_st                 :  1; /**<bit[27 : 27] */
		uint32_t cpu1_scr_int_st                  :  1; /**<bit[28 : 28] */
		uint32_t cpu1_lin_int_st                  :  1; /**<bit[29 : 29] */
		uint32_t cpu1_can1_int_st                 :  1; /**<bit[30 : 30] */
		uint32_t cpu1_timer2_int_st               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu1_int_32_63_status_t;


typedef volatile union {
	struct {
		uint32_t cpu1_timer3_int_st               :  1; /**<bit[0 : 0] */
		uint32_t cpu1_uart3_int_st                :  1; /**<bit[1 : 1] */
		uint32_t cpu1_spi2_int_st                 :  1; /**<bit[2 : 2] */
		uint32_t cpu1_uart4_int_st                :  1; /**<bit[3 : 3] */
		uint32_t cpu1_i2c3_int_st                 :  1; /**<bit[4 : 4] */
		uint32_t cpu1_hspl_int_st                 :  1; /**<bit[5 : 5] */
		uint32_t cpu1_bk24_int_st                 :  1; /**<bit[6 : 6] */
		uint32_t cpu1_irda1_int_st                :  1; /**<bit[7 : 7] */
		uint32_t cpu1_irda2_int_st                :  1; /**<bit[8 : 8] */
		uint32_t cpu1_irda3_int_st                :  1; /**<bit[9 : 9] */
		uint32_t cpu1_i3c_int_st                  :  1; /**<bit[10 : 10] */
		uint32_t cpu1_i2s4_int_st                 :  1; /**<bit[11 : 11] */
		uint32_t cpu1_spdif1_int_st               :  1; /**<bit[12 : 12] */
		uint32_t cpu1_int_m55sub_int_st           :  1; /**<bit[13 : 13] */
		uint32_t cpu1_mailbox_int_st              :  1; /**<bit[14 : 14] */
		uint32_t cpu1_ipi_int_st                  :  1; /**<bit[15 : 15] */
		uint32_t cpu1_vid_disp3_int_st            :  1; /**<bit[16 : 16] */
		uint32_t cpu1_vad_int_st                  :  1; /**<bit[17 : 17] */
		uint32_t cpu1_resv82_int_st               :  1; /**<bit[18 : 18] */
		uint32_t cpu1_resv83_int_st               :  1; /**<bit[19 : 19] */
		uint32_t cpu1_resv84_int_st               :  1; /**<bit[20 : 20] */
		uint32_t cpu1_resv85_int_st               :  1; /**<bit[21 : 21] */
		uint32_t cpu1_resv86_int_st               :  1; /**<bit[22 : 22] */
		uint32_t cpu1_resv87_int_st               :  1; /**<bit[23 : 23] */
		uint32_t cpu1_resv88_int_st               :  1; /**<bit[24 : 24] */
		uint32_t cpu1_resv89_int_st               :  1; /**<bit[25 : 25] */
		uint32_t cpu1_resv90_int_st               :  1; /**<bit[26 : 26] */
		uint32_t cpu1_resv91_int_st               :  1; /**<bit[27 : 27] */
		uint32_t cpu1_resv92_int_st               :  1; /**<bit[28 : 28] */
		uint32_t cpu1_resv93_int_st               :  1; /**<bit[29 : 29] */
		uint32_t cpu1_resv94_int_st               :  1; /**<bit[30 : 30] */
		uint32_t cpu1_resv95_int_st               :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_cpu1_int_64_95_status_t;


typedef volatile union {
	struct {
		uint32_t m55sub_m52s_int_st               :  1; /**<bit[0 : 0] */
		uint32_t m55sub_gdma1_int_st              :  1; /**<bit[1 : 1] */
		uint32_t m55sub_mbox_int_st               :  1; /**<bit[2 : 2] */
		uint32_t m55sub_ipi_int_st                :  1; /**<bit[3 : 3] */
		uint32_t m55sub_gdma0_int_st              :  1; /**<bit[4 : 4] */
		uint32_t m55sub_cpu_fpu_int_int_st        :  1; /**<bit[5 : 5] */
		uint32_t m55sub_npu_int_st                :  1; /**<bit[6 : 6] */
		uint32_t m55sub_usb_fs_int_int_st         :  1; /**<bit[7 : 7] */
		uint32_t m55sub_usb_hs_int_int_st         :  1; /**<bit[8 : 8] */
		uint32_t m55sub_usb_plug_int_st           :  1; /**<bit[9 : 9] */
		uint32_t m55sub_uart5_int_st              :  1; /**<bit[10 : 10] */
		uint32_t m55sub_wwdt_int_st               :  1; /**<bit[11 : 11] */
		uint32_t m55sub_sdio0_int_st              :  1; /**<bit[12 : 12] */
		uint32_t m55sub_sdio1_int_st              :  1; /**<bit[13 : 13] */
		uint32_t m55sub_enet0_int_st              :  1; /**<bit[14 : 14] */
		uint32_t m55sub_enet1_int_st              :  1; /**<bit[15 : 15] */
		uint32_t m55sub_qspi0_int_st              :  1; /**<bit[16 : 16] */
		uint32_t m55sub_qspi1_int_st              :  1; /**<bit[17 : 17] */
		uint32_t m55sub_hspl_int_st               :  1; /**<bit[18 : 18] */
		uint32_t m55sub_isp_mi_int_st             :  1; /**<bit[19 : 19] */
		uint32_t m55sub_isp_fe_int_st             :  1; /**<bit[20 : 20] */
		uint32_t m55sub_isp_isp_int_st            :  1; /**<bit[21 : 21] */
		uint32_t m55sub_csi_int_st                :  1; /**<bit[22 : 22] */
		uint32_t m55sub_h26e_int_st               :  1; /**<bit[23 : 23] */
		uint32_t m55sub_vid_disp0_int_st          :  1; /**<bit[24 : 24] */
		uint32_t m55sub_vid_disp1_int_st          :  1; /**<bit[25 : 25] */
		uint32_t m55sub_vid_disp2_int_st          :  1; /**<bit[26 : 26] */
		uint32_t m55sub_vid_disp3_int_st          :  1; /**<bit[27 : 27] */
		uint32_t m55sub_vid_disp4_int_st          :  1; /**<bit[28 : 28] */
		uint32_t m55sub_psram0_err_int_st         :  1; /**<bit[29 : 29] */
		uint32_t m55sub_psram1_err_int_st         :  1; /**<bit[30 : 30] */
		uint32_t m55sub_mpc_int_st                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_m55sub_int_0_31_status_t;


typedef volatile union {
	struct {
		uint32_t m55sub_timer4_int_st             :  1; /**<bit[0 : 0] */
		uint32_t m55sub_timer5_int_st             :  1; /**<bit[1 : 1] */
		uint32_t m55sub_int_gpio_ns_int_st        :  1; /**<bit[2 : 2] */
		uint32_t m55sub_int_gpio_s_int_st         :  1; /**<bit[3 : 3] */
		uint32_t m55sub_int_audio_int_st          :  1; /**<bit[4 : 4] */
		uint32_t m55sub_int_i2s0_int_st           :  1; /**<bit[5 : 5] */
		uint32_t m55sub_int_i2s1_int_st           :  1; /**<bit[6 : 6] */
		uint32_t m55sub_int_i2s2_int_st           :  1; /**<bit[7 : 7] */
		uint32_t m55sub_int_i2s3_int_st           :  1; /**<bit[8 : 8] */
		uint32_t m55sub_int_i2s4_int_st           :  1; /**<bit[9 : 9] */
		uint32_t m55sub_int_spdif0_int_st         :  1; /**<bit[10 : 10] */
		uint32_t m55sub_int_spdif1_int_st         :  1; /**<bit[11 : 11] */
		uint32_t m55sub_int_cec_int_st            :  1; /**<bit[12 : 12] */
		uint32_t m55sub_int_i2c_0_int_st          :  1; /**<bit[13 : 13] */
		uint32_t m55sub_int_i2c_3_int_st          :  1; /**<bit[14 : 14] */
		uint32_t m55sub_int_i3c_int_st            :  1; /**<bit[15 : 15] */
		uint32_t m55sub_int_uart0_int_st          :  1; /**<bit[16 : 16] */
		uint32_t m55sub_int_uart1_int_st          :  1; /**<bit[17 : 17] */
		uint32_t m55sub_int_uart2_int_st          :  1; /**<bit[18 : 18] */
		uint32_t m55sub_int_uart3_int_st          :  1; /**<bit[19 : 19] */
		uint32_t m55sub_int_uart4_int_st          :  1; /**<bit[20 : 20] */
		uint32_t m55sub_int_l2cache_int_st        :  1; /**<bit[21 : 21] */
		uint32_t m55sub_resv54_int_st             :  1; /**<bit[22 : 22] */
		uint32_t m55sub_resv55_int_st             :  1; /**<bit[23 : 23] */
		uint32_t m55sub_resv56_int_st             :  1; /**<bit[24 : 24] */
		uint32_t m55sub_resv57_int_st             :  1; /**<bit[25 : 25] */
		uint32_t m55sub_resv58_int_st             :  1; /**<bit[26 : 26] */
		uint32_t m55sub_resv59_int_st             :  1; /**<bit[27 : 27] */
		uint32_t m55sub_resv60_int_st             :  1; /**<bit[28 : 28] */
		uint32_t m55sub_resv61_int_st             :  1; /**<bit[29 : 29] */
		uint32_t m55sub_resv62_int_st             :  1; /**<bit[30 : 30] */
		uint32_t m55sub_resv63_int_st             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_m55sub_int_32_63_status_t;


typedef volatile union {
	struct {
		uint32_t m55sub_resv64_int_st             :  1; /**<bit[0 : 0] */
		uint32_t m55sub_resv65_int_st             :  1; /**<bit[1 : 1] */
		uint32_t m55sub_resv66_int_st             :  1; /**<bit[2 : 2] */
		uint32_t m55sub_resv67_int_st             :  1; /**<bit[3 : 3] */
		uint32_t m55sub_resv68_int_st             :  1; /**<bit[4 : 4] */
		uint32_t m55sub_resv69_int_st             :  1; /**<bit[5 : 5] */
		uint32_t m55sub_resv70_int_st             :  1; /**<bit[6 : 6] */
		uint32_t m55sub_resv71_int_st             :  1; /**<bit[7 : 7] */
		uint32_t m55sub_resv72_int_st             :  1; /**<bit[8 : 8] */
		uint32_t m55sub_resv73_int_st             :  1; /**<bit[9 : 9] */
		uint32_t m55sub_resv74_int_st             :  1; /**<bit[10 : 10] */
		uint32_t m55sub_resv75_int_st             :  1; /**<bit[11 : 11] */
		uint32_t m55sub_resv76_int_st             :  1; /**<bit[12 : 12] */
		uint32_t m55sub_resv77_int_st             :  1; /**<bit[13 : 13] */
		uint32_t m55sub_resv78_int_st             :  1; /**<bit[14 : 14] */
		uint32_t m55sub_resv79_int_st             :  1; /**<bit[15 : 15] */
		uint32_t m55sub_resv80_int_st             :  1; /**<bit[16 : 16] */
		uint32_t m55sub_resv81_int_st             :  1; /**<bit[17 : 17] */
		uint32_t m55sub_resv82_int_st             :  1; /**<bit[18 : 18] */
		uint32_t m55sub_resv83_int_st             :  1; /**<bit[19 : 19] */
		uint32_t m55sub_resv84_int_st             :  1; /**<bit[20 : 20] */
		uint32_t m55sub_resv85_int_st             :  1; /**<bit[21 : 21] */
		uint32_t m55sub_resv86_int_st             :  1; /**<bit[22 : 22] */
		uint32_t m55sub_resv87_int_st             :  1; /**<bit[23 : 23] */
		uint32_t m55sub_resv88_int_st             :  1; /**<bit[24 : 24] */
		uint32_t m55sub_resv89_int_st             :  1; /**<bit[25 : 25] */
		uint32_t m55sub_resv90_int_st             :  1; /**<bit[26 : 26] */
		uint32_t m55sub_resv91_int_st             :  1; /**<bit[27 : 27] */
		uint32_t m55sub_resv92_int_st             :  1; /**<bit[28 : 28] */
		uint32_t m55sub_resv93_int_st             :  1; /**<bit[29 : 29] */
		uint32_t m55sub_resv94_int_st             :  1; /**<bit[30 : 30] */
		uint32_t m55sub_resv95_int_st             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_m55sub_int_64_95_status_t;


typedef volatile union {
	struct {
		uint32_t debug_gpio_ie                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x2a_t;


typedef volatile union {
	struct {
		uint32_t debug_gpio_i                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x2b_t;


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
} sys_reserver_reg0x2c_t;


typedef volatile union {
	struct {
		uint32_t spsl_cfg                         : 10; /**<bit[0 : 9] */
		uint32_t spbl_cfg                         : 11; /**<bit[10 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x2e_t;


typedef volatile union {
	struct {
		uint32_t stpl_cfg                         : 12; /**<bit[0 : 11] */
		uint32_t reserved_bit_12_23               : 12; /**<bit[12 : 23] */
		uint32_t set_key                          :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x2f_t;


typedef volatile union {
	struct {
		uint32_t gpio_input_status0               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_gpio_input_status0_t;


typedef volatile union {
	struct {
		uint32_t gpio_input_status1               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_gpio_input_status1_t;


typedef volatile union {
	struct {
		uint32_t gpio_input_status2               :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_30                : 23; /**<bit[8 : 30] */
		uint32_t gpio_input_status_en             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_gpio_input_status2_t;


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
} sys_reserver_reg0x33_t;


typedef volatile union {
	struct {
		uint32_t cpu0_curpc                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_cpu0_curpc_t;


typedef volatile union {
	struct {
		uint32_t cpu0_faultstat_h                 : 11; /**<bit[0 : 10] */
		uint32_t reserved_11_31                   : 21; /**<bit[11 : 31] */
	};
	uint32_t v;
} sys_cpu0_faultstat_H_t;


typedef volatile union {
	struct {
		uint32_t cpu0_faultstat_l                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_cpu0_faultstat_L_t;


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
} sys_cpu0_info_t;


typedef volatile union {
	struct {
		uint32_t dbug_config0                     : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_sys_debug_config0_t;


typedef volatile union {
	struct {
		uint32_t dbug_cfg1                        :  5; /**<bit[0 : 4] */
		uint32_t reserved_bit_5_31                : 27; /**<bit[5 : 31] */
	};
	uint32_t v;
} sys_sys_debug_config1_t;


typedef volatile union {
	struct {
		uint32_t anareg_stat                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_anareg_stat_t;


typedef volatile union {
	struct {
		uint32_t coresight_chn_gate_en            : 24; /**<bit[0 : 23] */
		uint32_t coresight_tpmaxdatasize          :  5; /**<bit[24 : 28] */
		uint32_t coresight_valid                  :  1; /**<bit[29 : 29] */
		uint32_t reserved_30_30                   :  1; /**<bit[30 : 30] */
		uint32_t anaregb_stat                     :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_reserver_reg0x3b_t;


typedef volatile union {
	struct {
		uint32_t cpu1_curpc                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_cpu1_curpc_t;


typedef volatile union {
	struct {
		uint32_t cpu1_faultstat_h                 : 11; /**<bit[0 : 10] */
		uint32_t reserved_11_31                   : 21; /**<bit[11 : 31] */
	};
	uint32_t v;
} sys_cpu1_faultstat_H_t;


typedef volatile union {
	struct {
		uint32_t cpu1_faultstat_l                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_cpu1_faultstat_L_t;


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
} sys_cpu1_info_t;


typedef volatile union {
	struct {
		uint32_t dpll_tsten               :  1; /**<bit[0 : 0] */
		uint32_t cp                       :  3; /**<bit[1 : 3] */
		uint32_t spideten                 :  1; /**<bit[4 : 4] */
		uint32_t hvref                    :  2; /**<bit[5 : 6] */
		uint32_t lvref                    :  2; /**<bit[7 : 8] */
		uint32_t rzctrl26m                :  1; /**<bit[9 : 9] */
		uint32_t looprzctrl               :  4; /**<bit[10 : 13] */
		uint32_t rpc                      :  2; /**<bit[14 : 15] */
		uint32_t openloop_en              :  1; /**<bit[16 : 16] */
		uint32_t unlock_sel               :  1; /**<bit[17 : 17] */
		uint32_t rst_unlock               :  1; /**<bit[18 : 18] */
		uint32_t spitrig                  :  1; /**<bit[19 : 19] */
		uint32_t band                     :  1; /**<bit[20 : 20] */
		uint32_t band_1                   :  1; /**<bit[21 : 21] */
		uint32_t band_2                   :  3; /**<bit[22 : 24] */
		uint32_t bandmanual               :  1; /**<bit[25 : 25] */
		uint32_t dsptrig                  :  1; /**<bit[26 : 26] */
		uint32_t lpen_dpll                :  1; /**<bit[27 : 27] */
		uint32_t cksel                    :  2; /**<bit[28 : 29] */
		uint32_t bp_caldone               :  1; /**<bit[30 : 30] */
		uint32_t vselldo                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg0_t;


typedef volatile union {
	struct {
		uint32_t vcooffset                :  1; /**<bit[0 : 0] */
		uint32_t selpol                   :  1; /**<bit[1 : 1] */
		uint32_t dlysel                   :  2; /**<bit[2 : 3] */
		uint32_t edgesel_nck              :  1; /**<bit[4 : 4] */
		uint32_t nload_dlyen              :  1; /**<bit[5 : 5] */
		uint32_t cp                       :  3; /**<bit[6 : 8] */
		uint32_t spideten                 :  1; /**<bit[9 : 9] */
		uint32_t cben                     :  1; /**<bit[10 : 10] */
		uint32_t hvref                    :  2; /**<bit[11 : 12] */
		uint32_t lvref                    :  2; /**<bit[13 : 14] */
		uint32_t rzctrl26m                :  1; /**<bit[15 : 15] */
		uint32_t lpfrz                    :  4; /**<bit[16 : 19] */
		uint32_t rpc                      :  3; /**<bit[20 : 22] */
		uint32_t dpll_tsten               :  1; /**<bit[23 : 23] */
		uint32_t kctrl                    :  2; /**<bit[24 : 25] */
		uint32_t vsel_ldo                 :  2; /**<bit[26 : 27] */
		uint32_t div_sw                   :  1; /**<bit[28 : 28] */
		uint32_t bp_caldone               :  1; /**<bit[29 : 29] */
		uint32_t ck2xen                   :  1; /**<bit[30 : 30] */
		uint32_t int_mod                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg1_t;


typedef volatile union {
	struct {
		uint32_t nc_0_1                   :  2; /**<bit[0 : 1] */
		uint32_t vctrl_vsel               :  3; /**<bit[2 : 4] */
		uint32_t nc_5_7                   :  3; /**<bit[5 : 7] */
		uint32_t ck_tst_en                :  1; /**<bit[8 : 8] */
		uint32_t cktst_sel                :  2; /**<bit[9 : 10] */
		uint32_t dco_modecal              :  1; /**<bit[11 : 11] */
		uint32_t dco_modecal_1            :  1; /**<bit[12 : 12] */
		uint32_t anabufsel_rx             :  1; /**<bit[13 : 13] */
		uint32_t nc_14_15                 :  2; /**<bit[14 : 15] */
		uint32_t cktdinven                :  1; /**<bit[16 : 16] */
		uint32_t cktden                   :  1; /**<bit[17 : 17] */
		uint32_t nc_18_20                 :  3; /**<bit[18 : 20] */
		uint32_t xtal32k_dgliten          :  1; /**<bit[21 : 21] */
		uint32_t xtal32k_diven            :  1; /**<bit[22 : 22] */
		uint32_t rc32k_dgliten            :  1; /**<bit[23 : 23] */
		uint32_t rc32k_diven              :  1; /**<bit[24 : 24] */
		uint32_t nc_25_27                 :  3; /**<bit[25 : 27] */
		uint32_t unlock_sel_dco           :  1; /**<bit[28 : 28] */
		uint32_t rst_unlock_dco           :  1; /**<bit[29 : 29] */
		uint32_t refsamen                 :  1; /**<bit[30 : 30] */
		uint32_t adcdcsel                 :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg2_t;


typedef volatile union {
	struct {
		uint32_t ctune                    :  8; /**<bit[0 : 7] */
		uint32_t core_hpen                :  1; /**<bit[8 : 8] */
		uint32_t ck_sel                   :  1; /**<bit[9 : 9] */
		uint32_t anabuf_sel_tx            :  1; /**<bit[10 : 10] */
		uint32_t pwd_xtalldo              :  1; /**<bit[11 : 11] */
		uint32_t iamp                     :  1; /**<bit[12 : 12] */
		uint32_t vddren                   :  1; /**<bit[13 : 13] */
		uint32_t xamp                     :  6; /**<bit[14 : 19] */
		uint32_t vosel                    :  5; /**<bit[20 : 24] */
		uint32_t en_xtalh_sleep           :  1; /**<bit[25 : 25] */
		uint32_t xtal40_en                :  1; /**<bit[26 : 26] */
		uint32_t bufictrl                 :  1; /**<bit[27 : 27] */
		uint32_t ibias_ctrl               :  2; /**<bit[28 : 29] */
		uint32_t icore_ctrl               :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sys_ana_reg3_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg4_t;


typedef volatile union {
	struct {
		uint32_t vselldo1_dpll            :  1; /**<bit[0 : 0] */
		uint32_t en_xtall                 :  1; /**<bit[1 : 1] */
		uint32_t en_dco                   :  1; /**<bit[2 : 2] */
		uint32_t temp_gsel                :  1; /**<bit[3 : 3] */
		uint32_t en_temp                  :  1; /**<bit[4 : 4] */
		uint32_t en_dpll                  :  1; /**<bit[5 : 5] */
		uint32_t en_cb                    :  1; /**<bit[6 : 6] */
		uint32_t gpio_latch               :  1; /**<bit[7 : 7] */
		uint32_t bypassen                 :  1; /**<bit[8 : 8] */
		uint32_t nc_9_9                   :  1; /**<bit[9 : 9] */
		uint32_t rc32k_refclk_en          :  1; /**<bit[10 : 10] */
		uint32_t spilatchb_rc32k          :  1; /**<bit[11 : 11] */
		uint32_t rosc_disable             :  1; /**<bit[12 : 12] */
		uint32_t pwdaudpll                :  1; /**<bit[13 : 13] */
		uint32_t pwd_rosc_spi             :  1; /**<bit[14 : 14] */
		uint32_t nc_15_15                 :  1; /**<bit[15 : 15] */
		uint32_t itune_xtall              :  4; /**<bit[16 : 19] */
		uint32_t xtall_tsten              :  1; /**<bit[20 : 20] */
		uint32_t rosc_ten                 :  1; /**<bit[21 : 21] */
		uint32_t bcal_start               :  1; /**<bit[22 : 22] */
		uint32_t bcal_en                  :  1; /**<bit[23 : 23] */
		uint32_t bcal_sel                 :  3; /**<bit[24 : 26] */
		uint32_t vbias                    :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} sys_ana_reg5_t;


typedef volatile union {
	struct {
		uint32_t calib_interval           : 10; /**<bit[0 : 9] */
		uint32_t modify_interval          :  6; /**<bit[10 : 15] */
		uint32_t xtal_wakeup_time         :  4; /**<bit[16 : 19] */
		uint32_t spi_trig                 :  1; /**<bit[20 : 20] */
		uint32_t modifi_auto              :  1; /**<bit[21 : 21] */
		uint32_t calib_auto               :  1; /**<bit[22 : 22] */
		uint32_t cal_mode                 :  1; /**<bit[23 : 23] */
		uint32_t manu_ena                 :  1; /**<bit[24 : 24] */
		uint32_t manu_cin                 :  7; /**<bit[25 : 31] */
	};
	uint32_t v;
} sys_ana_reg6_t;


typedef volatile union {
	struct {
		uint32_t nsyn                     :  1; /**<bit[0 : 0] */
		uint32_t bandmanual               :  6; /**<bit[1 : 6] */
		uint32_t ckref_loop_sel           :  1; /**<bit[7 : 7] */
		uint32_t ioffs                    :  3; /**<bit[8 : 10] */
		uint32_t reset_nload              :  1; /**<bit[11 : 11] */
		uint32_t closeloop_en             :  1; /**<bit[12 : 12] */
		uint32_t ictrlm                   :  1; /**<bit[13 : 13] */
		uint32_t spi_rstn                 :  1; /**<bit[14 : 14] */
		uint32_t osccal_trig              :  1; /**<bit[15 : 15] */
		uint32_t manual                   :  1; /**<bit[16 : 16] */
		uint32_t diff                     :  3; /**<bit[17 : 19] */
		uint32_t ictrlmanual              :  3; /**<bit[20 : 22] */
		uint32_t cnti                     :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} sys_ana_reg7_t;


typedef volatile union {
	struct {
		uint32_t reserved_bit_0_30        : 31; /**<bit[0 : 30] */
		uint32_t n                        :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg8_t;


typedef volatile union {
	struct {
		uint32_t clk_sel                  :  1; /**<bit[0 : 0] */
		uint32_t coreldo_hp               :  1; /**<bit[1 : 1] */
		uint32_t dldohp                   :  1; /**<bit[2 : 2] */
		uint32_t t_vanaldosel             :  3; /**<bit[3 : 5] */
		uint32_t r_vanaldosel             :  3; /**<bit[6 : 8] */
		uint32_t en_trsw                  :  1; /**<bit[9 : 9] */
		uint32_t aldohp                   :  1; /**<bit[10 : 10] */
		uint32_t anacurlim                :  1; /**<bit[11 : 11] */
		uint32_t hsldo_hp                 :  1; /**<bit[12 : 12] */
		uint32_t pwd_hsldo                :  1; /**<bit[13 : 13] */
		uint32_t enfast_hsldo             :  1; /**<bit[14 : 14] */
		uint32_t vporsel                  :  1; /**<bit[15 : 15] */
		uint32_t valoldosel               :  3; /**<bit[16 : 18] */
		uint32_t alopowsel                :  1; /**<bit[19 : 19] */
		uint32_t en_fast_aloldo           :  1; /**<bit[20 : 20] */
		uint32_t aloldohp                 :  1; /**<bit[21 : 21] */
		uint32_t bgcal                    :  6; /**<bit[22 : 27] */
		uint32_t vbgcalmode               :  1; /**<bit[28 : 28] */
		uint32_t vbgcalstart              :  1; /**<bit[29 : 29] */
		uint32_t pwd_bgcal                :  1; /**<bit[30 : 30] */
		uint32_t spi_envbg                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg9_t;


typedef volatile union {
	struct {
		uint32_t azcd_manual              :  6; /**<bit[0 : 5] */
		uint32_t azcdrefs                 :  3; /**<bit[6 : 8] */
		uint32_t spi_latch1v              :  1; /**<bit[9 : 9] */
		uint32_t digcurlim                :  1; /**<bit[10 : 10] */
		uint32_t rtc_wkrstn               :  1; /**<bit[11 : 11] */
		uint32_t rst_wks                  :  1; /**<bit[12 : 12] */
		uint32_t d_veasel1v               :  2; /**<bit[13 : 14] */
		uint32_t ensfsdd                  :  1; /**<bit[15 : 15] */
		uint32_t vcorehsel                :  4; /**<bit[16 : 19] */
		uint32_t vcorelsel                :  3; /**<bit[20 : 22] */
		uint32_t vlden                    :  1; /**<bit[23 : 23] */
		uint32_t en_fast_coreldo          :  1; /**<bit[24 : 24] */
		uint32_t pwdcoreldo               :  1; /**<bit[25 : 25] */
		uint32_t vdighsel                 :  3; /**<bit[26 : 28] */
		uint32_t vdiglsel                 :  2; /**<bit[29 : 30] */
		uint32_t vdd12lden                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg10_t;


typedef volatile union {
	struct {
		uint32_t aldo_czsel               :  3; /**<bit[0 : 2] */
		uint32_t zldo_rzsel               :  2; /**<bit[3 : 4] */
		uint32_t azcdswvs                 :  3; /**<bit[5 : 7] */
		uint32_t aenzcddy                 :  1; /**<bit[8 : 8] */
		uint32_t aenzcdmsel               :  1; /**<bit[9 : 9] */
		uint32_t aenzcdcalib              :  1; /**<bit[10 : 10] */
		uint32_t en_corepsw               :  1; /**<bit[11 : 11] */
		uint32_t en_alopsw                :  1; /**<bit[12 : 12] */
		uint32_t vbatdetsel               :  2; /**<bit[13 : 14] */
		uint32_t spi_timerwken            :  1; /**<bit[15 : 15] */
		uint32_t spi_byp32pwd             :  1; /**<bit[16 : 16] */
		uint32_t sd                       :  1; /**<bit[17 : 17] */
		uint32_t timer_wkrstn             :  1; /**<bit[18 : 18] */
		uint32_t gpio_wkrst1v             :  1; /**<bit[19 : 19] */
		uint32_t ckfs                     :  2; /**<bit[20 : 21] */
		uint32_t ckintsel                 :  1; /**<bit[22 : 22] */
		uint32_t osccaltrig               :  1; /**<bit[23 : 23] */
		uint32_t mroscsel                 :  1; /**<bit[24 : 24] */
		uint32_t mrosci_cal               :  3; /**<bit[25 : 27] */
		uint32_t mrosccap_cal             :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} sys_ana_reg11_t;


typedef volatile union {
	struct {
		uint32_t sfsr                     :  4; /**<bit[0 : 3] */
		uint32_t ensfsaa                  :  1; /**<bit[4 : 4] */
		uint32_t apfms                    :  5; /**<bit[5 : 9] */
		uint32_t atmpo_sel                :  2; /**<bit[10 : 11] */
		uint32_t ampoen                   :  1; /**<bit[12 : 12] */
		uint32_t enpowa                   :  1; /**<bit[13 : 13] */
		uint32_t avea_sel                 :  2; /**<bit[14 : 15] */
		uint32_t aforcepfm                :  1; /**<bit[16 : 16] */
		uint32_t acls                     :  3; /**<bit[17 : 19] */
		uint32_t aswrsten                 :  1; /**<bit[20 : 20] */
		uint32_t aripc                    :  3; /**<bit[21 : 23] */
		uint32_t arampc                   :  4; /**<bit[24 : 27] */
		uint32_t arampcen                 :  1; /**<bit[28 : 28] */
		uint32_t aenburst                 :  1; /**<bit[29 : 29] */
		uint32_t apfmen                   :  1; /**<bit[30 : 30] */
		uint32_t aldosel                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg12_t;


typedef volatile union {
	struct {
		uint32_t buckd_softst             :  4; /**<bit[0 : 3] */
		uint32_t denzcdcalib              :  1; /**<bit[4 : 4] */
		uint32_t dzcdmsel                 :  1; /**<bit[5 : 5] */
		uint32_t vbd_rstrtc_en            :  1; /**<bit[6 : 6] */
		uint32_t vddgpio_sel              :  1; /**<bit[7 : 7] */
		uint32_t dpfms                    :  5; /**<bit[8 : 12] */
		uint32_t dtmpo_sel                :  2; /**<bit[13 : 14] */
		uint32_t dmpoen                   :  1; /**<bit[15 : 15] */
		uint32_t dforcepfm                :  1; /**<bit[16 : 16] */
		uint32_t dcls                     :  3; /**<bit[17 : 19] */
		uint32_t dswrsten                 :  1; /**<bit[20 : 20] */
		uint32_t dripc                    :  3; /**<bit[21 : 23] */
		uint32_t drampc                   :  4; /**<bit[24 : 27] */
		uint32_t drampcen                 :  1; /**<bit[28 : 28] */
		uint32_t denburst                 :  1; /**<bit[29 : 29] */
		uint32_t dpfmen                   :  1; /**<bit[30 : 30] */
		uint32_t dldosel                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg13_t;


typedef volatile union {
	struct {
		uint32_t en_alo2corepsw           :  1; /**<bit[0 : 0] */
		uint32_t asoft_stc                :  4; /**<bit[1 : 4] */
		uint32_t dldo_czsel               :  3; /**<bit[5 : 7] */
		uint32_t dldo_rzsel               :  2; /**<bit[8 : 9] */
		uint32_t en_usbvcc18              :  1; /**<bit[10 : 10] */
		uint32_t en_usbvcc3v              :  1; /**<bit[11 : 11] */
		uint32_t vtrxspisel               :  2; /**<bit[12 : 13] */
		uint32_t denzcddy                 :  1; /**<bit[14 : 14] */
		uint32_t dzcd_swvs                :  3; /**<bit[15 : 17] */
		uint32_t dzcd_refs                :  3; /**<bit[18 : 20] */
		uint32_t dzcd_manu                :  6; /**<bit[21 : 26] */
		uint32_t vpsramsel                :  4; /**<bit[27 : 30] */
		uint32_t enpsram                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg14_t;


typedef volatile union {
	struct {
		uint32_t gpiowken                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg15_t;


typedef volatile union {
	struct {
		uint32_t rtc_set                  :  8; /**<bit[0 : 7] */
		uint32_t nc_8_10                  :  3; /**<bit[8 : 10] */
		uint32_t vcorehssel               :  4; /**<bit[11 : 14] */
		uint32_t vbuckhssel               :  3; /**<bit[15 : 17] */
		uint32_t hsenfast                 :  1; /**<bit[18 : 18] */
		uint32_t enhspw                   :  1; /**<bit[19 : 19] */
		uint32_t buckhs_soft_stc          :  4; /**<bit[20 : 23] */
		uint32_t hs_veasel                :  2; /**<bit[24 : 25] */
		uint32_t hszcd_manual             :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} sys_ana_reg16_t;


typedef volatile union {
	struct {
		uint32_t rtc_set                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg17_t;


typedef volatile union {
	struct {
		uint32_t timer_set                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg18_t;


typedef volatile union {
	struct {
		uint32_t hsenzcddy                :  1; /**<bit[0 : 0] */
		uint32_t hsenzcdcalib             :  1; /**<bit[1 : 1] */
		uint32_t hszcdswvs                :  3; /**<bit[2 : 4] */
		uint32_t hszcdrefs                :  3; /**<bit[5 : 7] */
		uint32_t hszcdmsel                :  1; /**<bit[8 : 8] */
		uint32_t hspfms                   :  5; /**<bit[9 : 13] */
		uint32_t hstmpo_sel               :  2; /**<bit[14 : 15] */
		uint32_t hsmpoen                  :  1; /**<bit[16 : 16] */
		uint32_t hsforcepfm               :  1; /**<bit[17 : 17] */
		uint32_t hscls                    :  3; /**<bit[18 : 20] */
		uint32_t hsswrsten                :  1; /**<bit[21 : 21] */
		uint32_t hsripc                   :  3; /**<bit[22 : 24] */
		uint32_t hsrampc                  :  4; /**<bit[25 : 28] */
		uint32_t hsrampcen                :  1; /**<bit[29 : 29] */
		uint32_t hsenburst                :  1; /**<bit[30 : 30] */
		uint32_t hspfmen                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg19_t;


typedef volatile union {
	struct {
		uint32_t iselaud                  :  1; /**<bit[0 : 0] */
		uint32_t audck_rlcen              :  1; /**<bit[1 : 1] */
		uint32_t lchckinven               :  1; /**<bit[2 : 2] */
		uint32_t enaudbias                :  1; /**<bit[3 : 3] */
		uint32_t enadcbias                :  1; /**<bit[4 : 4] */
		uint32_t enmicbias                :  1; /**<bit[5 : 5] */
		uint32_t adcckinven               :  1; /**<bit[6 : 6] */
		uint32_t spi                      :  1; /**<bit[7 : 7] */
		uint32_t adctsten                 :  1; /**<bit[8 : 8] */
		uint32_t micbias_trm              :  2; /**<bit[9 : 10] */
		uint32_t micbias_voc              :  5; /**<bit[11 : 15] */
		uint32_t vrefsel                  :  1; /**<bit[16 : 16] */
		uint32_t capsw                    :  5; /**<bit[17 : 21] */
		uint32_t adcref_sel               :  2; /**<bit[22 : 23] */
		uint32_t adcvcmsel                :  2; /**<bit[24 : 25] */
		uint32_t spi_1                    :  1; /**<bit[26 : 26] */
		uint32_t audadjref                :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} sys_ana_reg20_t;


typedef volatile union {
	struct {
		uint32_t isel_mic1                :  2; /**<bit[0 : 1] */
		uint32_t micirsel1_mic1           :  1; /**<bit[2 : 2] */
		uint32_t vcmsel_mic1              :  1; /**<bit[3 : 3] */
		uint32_t enfsr_mic1               :  1; /**<bit[4 : 4] */
		uint32_t enopoclip_mic1           :  1; /**<bit[5 : 5] */
		uint32_t da2aden_mic1             :  1; /**<bit[6 : 6] */
		uint32_t imatch_mic1              :  4; /**<bit[7 : 10] */
		uint32_t imatch_en_mic1           :  1; /**<bit[11 : 11] */
		uint32_t dccompen_mic1            :  1; /**<bit[12 : 12] */
		uint32_t micsingleen_mic1         :  1; /**<bit[13 : 13] */
		uint32_t nc_14_14                 :  1; /**<bit[14 : 14] */
		uint32_t micgain_mic1             :  4; /**<bit[15 : 18] */
		uint32_t nc_19_23                 :  5; /**<bit[19 : 23] */
		uint32_t dwamode_mic1             :  1; /**<bit[24 : 24] */
		uint32_t nc_25_26                 :  2; /**<bit[25 : 26] */
		uint32_t rstsel_mic1              :  1; /**<bit[27 : 27] */
		uint32_t micen_mic1               :  1; /**<bit[28 : 28] */
		uint32_t rst_mic1                 :  1; /**<bit[29 : 29] */
		uint32_t bpdwa1v_mic1             :  1; /**<bit[30 : 30] */
		uint32_t hcen1stg_mic1            :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg21_t;


typedef volatile union {
	struct {
		uint32_t inbuffer_isel            :  2; /**<bit[0 : 1] */
		uint32_t gadc_offset_en           :  1; /**<bit[2 : 2] */
		uint32_t gadc_vref_sel            :  1; /**<bit[3 : 3] */
		uint32_t gadc_bufamp_isel         :  3; /**<bit[4 : 6] */
		uint32_t gadc_preamp_isel         :  3; /**<bit[7 : 9] */
		uint32_t gadc_comp_isel           :  3; /**<bit[10 : 12] */
		uint32_t gadc_bscalsaw            :  3; /**<bit[13 : 15] */
		uint32_t gadc_vncalsaw            :  3; /**<bit[16 : 18] */
		uint32_t gadc_vpcalsaw            :  3; /**<bit[19 : 21] */
		uint32_t irefen                   :  1; /**<bit[22 : 22] */
		uint32_t gadc_vbg_sel             :  1; /**<bit[23 : 23] */
		uint32_t gadc_clk_rlten           :  1; /**<bit[24 : 24] */
		uint32_t gadc_calintsaw_en        :  1; /**<bit[25 : 25] */
		uint32_t gadc_clk_sel             :  1; /**<bit[26 : 26] */
		uint32_t gadc_clk_in              :  1; /**<bit[27 : 27] */
		uint32_t gadc_calcap_ch           :  2; /**<bit[28 : 29] */
		uint32_t gadc_inbuf_en            :  1; /**<bit[30 : 30] */
		uint32_t gadc_en_spi              :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg22_t;


typedef volatile union {
	struct {
		uint32_t vadckinven               :  1; /**<bit[0 : 0] */
		uint32_t vadrefsel                :  1; /**<bit[1 : 1] */
		uint32_t vad_rstn                 :  1; /**<bit[2 : 2] */
		uint32_t vad_viniset              :  1; /**<bit[3 : 3] */
		uint32_t vad_cstrm                :  2; /**<bit[4 : 5] */
		uint32_t vad_cftrm                :  2; /**<bit[6 : 7] */
		uint32_t vad_en                   :  1; /**<bit[8 : 8] */
		uint32_t vad_ctrl0                : 23; /**<bit[9 : 31] */
	};
	uint32_t v;
} sys_ana_reg23_t;


typedef volatile union {
	struct {
		uint32_t vad_ctrl1                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg24_t;


typedef volatile union {
	struct {
		uint32_t int_mod                  :  1; /**<bit[0 : 0] */
		uint32_t nsyn                     :  1; /**<bit[1 : 1] */
		uint32_t open_enb                 :  1; /**<bit[2 : 2] */
		uint32_t reset                    :  1; /**<bit[3 : 3] */
		uint32_t ioffsetl                 :  3; /**<bit[4 : 6] */
		uint32_t lpfrz                    :  4; /**<bit[7 : 10] */
		uint32_t vsel                     :  3; /**<bit[11 : 13] */
		uint32_t vsel_cal                 :  1; /**<bit[14 : 14] */
		uint32_t pwd_lockdet              :  1; /**<bit[15 : 15] */
		uint32_t lockdet_bypass           :  1; /**<bit[16 : 16] */
		uint32_t ckref_loop_sel           :  1; /**<bit[17 : 17] */
		uint32_t spi_trigger              :  1; /**<bit[18 : 18] */
		uint32_t manual                   :  1; /**<bit[19 : 19] */
		uint32_t test_ckaudio_en          :  1; /**<bit[20 : 20] */
		uint32_t ck2xen                   :  1; /**<bit[21 : 21] */
		uint32_t icp                      :  2; /**<bit[22 : 23] */
		uint32_t cktst_sel                :  1; /**<bit[24 : 24] */
		uint32_t edgesel_nck              :  1; /**<bit[25 : 25] */
		uint32_t nloaddlyen               :  1; /**<bit[26 : 26] */
		uint32_t bypass_caldone_auto      :  1; /**<bit[27 : 27] */
		uint32_t cal_res_spi              :  3; /**<bit[28 : 30] */
		uint32_t audioen                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg25_t;


typedef volatile union {
	struct {
		uint32_t n                        : 30; /**<bit[0 : 29] */
		uint32_t calres_spien             :  1; /**<bit[30 : 30] */
		uint32_t calrefen                 :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg26_t;


typedef volatile union {
	struct {
		uint32_t isel_mic2                :  2; /**<bit[0 : 1] */
		uint32_t micirsel1_mic2           :  1; /**<bit[2 : 2] */
		uint32_t vcmsel_mic2              :  1; /**<bit[3 : 3] */
		uint32_t enfsr_mic2               :  1; /**<bit[4 : 4] */
		uint32_t enopoclip_mic2           :  1; /**<bit[5 : 5] */
		uint32_t da2aden_mic2             :  1; /**<bit[6 : 6] */
		uint32_t imatch_mic2              :  4; /**<bit[7 : 10] */
		uint32_t imatch_en_mic2           :  1; /**<bit[11 : 11] */
		uint32_t dccompen_mic2            :  1; /**<bit[12 : 12] */
		uint32_t micsingleen_mic2         :  1; /**<bit[13 : 13] */
		uint32_t nc_14_14                 :  1; /**<bit[14 : 14] */
		uint32_t micgain_mic2             :  4; /**<bit[15 : 18] */
		uint32_t nc_19_23                 :  5; /**<bit[19 : 23] */
		uint32_t dwamode_mic2             :  1; /**<bit[24 : 24] */
		uint32_t nc_25_26                 :  2; /**<bit[25 : 26] */
		uint32_t rstsel_mic2              :  1; /**<bit[27 : 27] */
		uint32_t micen_mic2               :  1; /**<bit[28 : 28] */
		uint32_t rst_mic2                 :  1; /**<bit[29 : 29] */
		uint32_t bpdwa1v_mic2             :  1; /**<bit[30 : 30] */
		uint32_t hcen1stg_mic2            :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg27_t;


typedef volatile union {
	struct {
		uint32_t isel_mic3                :  2; /**<bit[0 : 1] */
		uint32_t micirsel1_mic3           :  1; /**<bit[2 : 2] */
		uint32_t vcmsel_mic3              :  1; /**<bit[3 : 3] */
		uint32_t enfsr_mic3               :  1; /**<bit[4 : 4] */
		uint32_t enopoclip_mic3           :  1; /**<bit[5 : 5] */
		uint32_t da2aden_mic3             :  1; /**<bit[6 : 6] */
		uint32_t imatch_mic3              :  4; /**<bit[7 : 10] */
		uint32_t imatch_en_mic3           :  1; /**<bit[11 : 11] */
		uint32_t dccompen_mic3            :  1; /**<bit[12 : 12] */
		uint32_t micsingleen_mic3         :  1; /**<bit[13 : 13] */
		uint32_t nc_14_14                 :  1; /**<bit[14 : 14] */
		uint32_t micgain_mic3             :  4; /**<bit[15 : 18] */
		uint32_t nc_19_23                 :  5; /**<bit[19 : 23] */
		uint32_t dwamode_mic3             :  1; /**<bit[24 : 24] */
		uint32_t nc_25_26                 :  2; /**<bit[25 : 26] */
		uint32_t rstsel_mic3              :  1; /**<bit[27 : 27] */
		uint32_t micen_mic3               :  1; /**<bit[28 : 28] */
		uint32_t rst_mic3                 :  1; /**<bit[29 : 29] */
		uint32_t bpdwa1v_mic3             :  1; /**<bit[30 : 30] */
		uint32_t hcen1stg_mic3            :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg28_t;


typedef volatile union {
	struct {
		uint32_t hpdac                    :  1; /**<bit[0 : 0] */
		uint32_t iselstg                  :  1; /**<bit[1 : 1] */
		uint32_t oscdac                   :  2; /**<bit[2 : 3] */
		uint32_t ocendac                  :  1; /**<bit[4 : 4] */
		uint32_t vseldco                  :  1; /**<bit[5 : 5] */
		uint32_t srsel                    :  1; /**<bit[6 : 6] */
		uint32_t hpoen                    :  1; /**<bit[7 : 7] */
		uint32_t lbwen                    :  1; /**<bit[8 : 8] */
		uint32_t calsel                   :  1; /**<bit[9 : 9] */
		uint32_t bp2vldo                  :  1; /**<bit[10 : 10] */
		uint32_t dcochg                   :  2; /**<bit[11 : 12] */
		uint32_t diffen                   :  1; /**<bit[13 : 13] */
		uint32_t endaccal                 :  1; /**<bit[14 : 14] */
		uint32_t rendcoc                  :  1; /**<bit[15 : 15] */
		uint32_t lendcoc                  :  1; /**<bit[16 : 16] */
		uint32_t renvcmd                  :  1; /**<bit[17 : 17] */
		uint32_t lenvcmd                  :  1; /**<bit[18 : 18] */
		uint32_t dacdrven                 :  1; /**<bit[19 : 19] */
		uint32_t dacren                   :  1; /**<bit[20 : 20] */
		uint32_t daclen                   :  1; /**<bit[21 : 21] */
		uint32_t dacg                     :  4; /**<bit[22 : 25] */
		uint32_t dacmute                  :  1; /**<bit[26 : 26] */
		uint32_t dacdwamode_sel           :  1; /**<bit[27 : 27] */
		uint32_t ckpsel                   :  1; /**<bit[28 : 28] */
		uint32_t nc_29_31                 :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} sys_ana_reg29_t;


typedef volatile union {
	struct {
		uint32_t lmdcin                   :  8; /**<bit[0 : 7] */
		uint32_t rmdcin                   :  8; /**<bit[8 : 15] */
		uint32_t spirst_ovc               :  1; /**<bit[16 : 16] */
		uint32_t enidacr                  :  1; /**<bit[17 : 17] */
		uint32_t enidacl                  :  1; /**<bit[18 : 18] */
		uint32_t dac3rdhc0v9              :  1; /**<bit[19 : 19] */
		uint32_t hc2s                     :  1; /**<bit[20 : 20] */
		uint32_t sng_fb_en                :  1; /**<bit[21 : 21] */
		uint32_t rfb_ctrl                 :  1; /**<bit[22 : 22] */
		uint32_t enbs                     :  1; /**<bit[23 : 23] */
		uint32_t calck_sel0v9             :  1; /**<bit[24 : 24] */
		uint32_t bpdwa0v9                 :  1; /**<bit[25 : 25] */
		uint32_t looprst0v9               :  1; /**<bit[26 : 26] */
		uint32_t oct0v9                   :  2; /**<bit[27 : 28] */
		uint32_t sout0v9                  :  1; /**<bit[29 : 29] */
		uint32_t hc0v9                    :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sys_ana_reg30_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg31_t;


typedef volatile union {
	struct {
		uint32_t nc_0_2                   :  3; /**<bit[0 : 2] */
		uint32_t vad_ck_sel               :  1; /**<bit[3 : 3] */
		uint32_t int_sel                  :  1; /**<bit[4 : 4] */
		uint32_t ldoen                    :  1; /**<bit[5 : 5] */
		uint32_t ldoctrl                  :  1; /**<bit[6 : 6] */
		uint32_t ibctrl                   :  1; /**<bit[7 : 7] */
		uint32_t rstb_dig                 :  1; /**<bit[8 : 8] */
		uint32_t en_vtest_sel             :  1; /**<bit[9 : 9] */
		uint32_t en_adcmod                :  1; /**<bit[10 : 10] */
		uint32_t en_out_test              :  1; /**<bit[11 : 11] */
		uint32_t nc_12_12                 :  1; /**<bit[12 : 12] */
		uint32_t sel_seri_cap             :  1; /**<bit[13 : 13] */
		uint32_t en_seri_cap              :  1; /**<bit[14 : 14] */
		uint32_t cal_ctrl                 :  2; /**<bit[15 : 16] */
		uint32_t cal_vth                  :  3; /**<bit[17 : 19] */
		uint32_t crg                      :  2; /**<bit[20 : 21] */
		uint32_t vrefs                    :  4; /**<bit[22 : 25] */
		uint32_t gain_s                   :  4; /**<bit[26 : 29] */
		uint32_t td_latch                 :  1; /**<bit[30 : 30] */
		uint32_t pwd_td                   :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg32_t;


typedef volatile union {
	struct {
		uint32_t test_number              :  4; /**<bit[0 : 3] */
		uint32_t test_period              :  4; /**<bit[4 : 7] */
		uint32_t chs                      : 16; /**<bit[8 : 23] */
		uint32_t chs_sel_cal              :  4; /**<bit[24 : 27] */
		uint32_t cal_done_clr             :  1; /**<bit[28 : 28] */
		uint32_t en_cal_force             :  1; /**<bit[29 : 29] */
		uint32_t en_cal_auto              :  1; /**<bit[30 : 30] */
		uint32_t en_scan                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg33_t;


typedef volatile union {
	struct {
		uint32_t int_en                   : 17; /**<bit[0 : 16] */
		uint32_t nc_17_17                 :  1; /**<bit[17 : 17] */
		uint32_t modsel_spi               :  1; /**<bit[18 : 18] */
		uint32_t cal_number               :  4; /**<bit[19 : 22] */
		uint32_t cl_period                :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} sys_ana_reg34_t;


typedef volatile union {
	struct {
		uint32_t int_clr                  : 17; /**<bit[0 : 16] */
		uint32_t nc_17_17                 :  1; /**<bit[17 : 17] */
		uint32_t int_clr_sel              :  1; /**<bit[18 : 18] */
		uint32_t en_lpmod                 :  1; /**<bit[19 : 19] */
		uint32_t en_testcmp               :  1; /**<bit[20 : 20] */
		uint32_t en_man_wr                :  1; /**<bit[21 : 21] */
		uint32_t en_manmode               :  1; /**<bit[22 : 22] */
		uint32_t cap_calspi               :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} sys_ana_reg35_t;


typedef volatile union {
	struct {
		uint32_t int_clr_cal              : 16; /**<bit[0 : 15] */
		uint32_t int_en_cal               : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} sys_ana_reg36_t;


typedef volatile union {
	struct {
		uint32_t reset                    :  1; /**<bit[0 : 0] */
		uint32_t clear_int                :  1; /**<bit[1 : 1] */
		uint32_t xmin_init                :  1; /**<bit[2 : 2] */
		uint32_t gain                     :  4; /**<bit[3 : 6] */
		uint32_t alphal                   :  3; /**<bit[7 : 9] */
		uint32_t alphas                   :  3; /**<bit[10 : 12] */
		uint32_t lpfn                     :  3; /**<bit[13 : 15] */
		uint32_t hpfn                     :  3; /**<bit[16 : 18] */
		uint32_t lpf_bypass               :  1; /**<bit[19 : 19] */
		uint32_t hpf_bypass               :  1; /**<bit[20 : 20] */
		uint32_t is_abs                   :  1; /**<bit[21 : 21] */
		uint32_t dir                      :  1; /**<bit[22 : 22] */
		uint32_t nc_23_31                 :  9; /**<bit[23 : 31] */
	};
	uint32_t v;
} sys_ana_reg37_t;


typedef volatile union {
	struct {
		uint32_t alpha                    :  8; /**<bit[0 : 7] */
		uint32_t comp                     : 12; /**<bit[8 : 19] */
		uint32_t cntn                     : 12; /**<bit[20 : 31] */
	};
	uint32_t v;
} sys_ana_reg38_t;


typedef volatile union {
	struct {
		uint32_t enspi_i                  :  1; /**<bit[0 : 0] */
		uint32_t ck_edge_i                :  1; /**<bit[1 : 1] */
		uint32_t outbuff_isel_i           :  3; /**<bit[2 : 4] */
		uint32_t refbuff_isel_i           :  3; /**<bit[5 : 7] */
		uint32_t cap_fb_sel_i             :  4; /**<bit[8 : 11] */
		uint32_t gain_sel_i               :  3; /**<bit[12 : 14] */
		uint32_t vref_cal_sel_i           :  5; /**<bit[15 : 19] */
		uint32_t cap_cal_sel_i            :  4; /**<bit[20 : 23] */
		uint32_t cal_direction_i          :  1; /**<bit[24 : 24] */
		uint32_t endigspi_sel_i           :  1; /**<bit[25 : 25] */
		uint32_t nc_26_31                 :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} sys_ana_reg39_t;


typedef volatile union {
	struct {
		uint32_t enspi_q                  :  1; /**<bit[0 : 0] */
		uint32_t ck_edge_q                :  1; /**<bit[1 : 1] */
		uint32_t outbuff_qsel_q           :  3; /**<bit[2 : 4] */
		uint32_t refbuff_qsel_q           :  3; /**<bit[5 : 7] */
		uint32_t cap_fb_sel_q             :  4; /**<bit[8 : 11] */
		uint32_t gain_sel_q               :  3; /**<bit[12 : 14] */
		uint32_t vref_cal_sel_q           :  5; /**<bit[15 : 19] */
		uint32_t cap_cal_sel_q            :  4; /**<bit[20 : 23] */
		uint32_t cal_direction_q          :  1; /**<bit[24 : 24] */
		uint32_t endigspi_sel_q           :  1; /**<bit[25 : 25] */
		uint32_t nc_26_31                 :  6; /**<bit[26 : 31] */
	};
	uint32_t v;
} sys_ana_reg40_t;


typedef volatile union {
	struct {
		uint32_t nc_0_10                  : 11; /**<bit[0 : 10] */
		uint32_t vsel_auxldo3v            :  4; /**<bit[11 : 14] */
		uint32_t vsel_auxldo2p8v          :  4; /**<bit[15 : 18] */
		uint32_t vsel_auxldo1p8v          :  4; /**<bit[19 : 22] */
		uint32_t vsel_auxldo1p2v          :  3; /**<bit[23 : 25] */
		uint32_t swb_auxldo3v             :  1; /**<bit[26 : 26] */
		uint32_t swb_auxldo2p8v           :  1; /**<bit[27 : 27] */
		uint32_t en_auxldo3v              :  1; /**<bit[28 : 28] */
		uint32_t en_auxldo2p8v            :  1; /**<bit[29 : 29] */
		uint32_t en_auxldo_1p8v           :  1; /**<bit[30 : 30] */
		uint32_t en_auxldo_1p2v           :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg41_t;


typedef volatile union {
	struct {
		uint32_t nc_0_9                   : 10; /**<bit[0 : 9] */
		uint32_t dslep_disable            :  1; /**<bit[10 : 10] */
		uint32_t en_vout                  :  1; /**<bit[11 : 11] */
		uint32_t vusbsel                  :  2; /**<bit[12 : 13] */
		uint32_t usbnen_dn                :  4; /**<bit[14 : 17] */
		uint32_t usbpen_dn                :  4; /**<bit[18 : 21] */
		uint32_t usbnen_dp                :  4; /**<bit[22 : 25] */
		uint32_t usbpen_dp                :  4; /**<bit[26 : 29] */
		uint32_t usb_speed                :  1; /**<bit[30 : 30] */
		uint32_t pwd_usb                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg42_t;


typedef volatile union {
	struct {
		uint32_t en_lpdac_a_votest        :  1; /**<bit[0 : 0] */
		uint32_t acmp_clr_out_a           :  1; /**<bit[1 : 1] */
		uint32_t acmp_chsel_a             :  4; /**<bit[2 : 5] */
		uint32_t acmp_hystsel_a           :  2; /**<bit[6 : 7] */
		uint32_t acmp_npmd_a              :  1; /**<bit[8 : 8] */
		uint32_t acmp_hpmd_a              :  1; /**<bit[9 : 9] */
		uint32_t en_anacomp_a             :  1; /**<bit[10 : 10] */
		uint32_t vbg_sel_lpdac_a          :  1; /**<bit[11 : 11] */
		uint32_t en_lpdac_a               :  1; /**<bit[12 : 12] */
		uint32_t comp_a_vosel             :  2; /**<bit[13 : 14] */
		uint32_t nc_15_15                 :  1; /**<bit[15 : 15] */
		uint32_t en_lpdac_b_votest        :  1; /**<bit[16 : 16] */
		uint32_t acmp_clr_out_b           :  1; /**<bit[17 : 17] */
		uint32_t acmp_chsel_b             :  4; /**<bit[18 : 21] */
		uint32_t acmp_hystsel_b           :  2; /**<bit[22 : 23] */
		uint32_t acmp_npmd_b              :  1; /**<bit[24 : 24] */
		uint32_t acmp_hpmd_b              :  1; /**<bit[25 : 25] */
		uint32_t en_anacomp_b             :  1; /**<bit[26 : 26] */
		uint32_t vbg_sel_lpdac_b          :  1; /**<bit[27 : 27] */
		uint32_t en_lpdac_b               :  1; /**<bit[28 : 28] */
		uint32_t comp_b_vosel             :  2; /**<bit[29 : 30] */
		uint32_t nc_31_31                 :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} sys_ana_reg43_t;


typedef volatile union {
	struct {
		uint32_t din_lpdac_a              :  8; /**<bit[0 : 7] */
		uint32_t din_lpdac_b              :  8; /**<bit[8 : 15] */
		uint32_t nc_16_31                 : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} sys_ana_reg44_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg45_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg46_t;


typedef volatile union {
	struct {
		uint32_t nc_0_31                  : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sys_ana_reg47_t;

typedef volatile struct {
	volatile sys_device_id_t device_id;
	volatile sys_version_id_t version_id;
	volatile sys_cpu_storage_connect_op_select_t cpu_storage_connect_op_select;
	volatile sys_cpu_current_run_status_t cpu_current_run_status;
	volatile sys_cpu0_int_halt_clk_op_t cpu0_int_halt_clk_op;
	volatile sys_cpu1_int_halt_clk_op_t cpu1_int_halt_clk_op;
	volatile uint32_t rsv_6_7[2];
	volatile sys_cpu_clk_div_mode1_t cpu_clk_div_mode1;
	volatile sys_cpu_clk_div_mode2_t cpu_clk_div_mode2;
	volatile sys_cpu_clk_div_mode3_t cpu_clk_div_mode3;
	volatile sys_cpu_anaspi_freq_t cpu_anaspi_freq;
	volatile sys_cpu_device_clk_enable_t cpu_device_clk_enable;
	volatile sys_reserver_reg0xd_t reserver_reg0xd;
	volatile uint32_t rsv_e_e[1];
	volatile sys_reserver_reg0xf_t reserver_reg0xf;
	volatile sys_reserver_reg0x10_t reserver_reg0x10;
	volatile sys_cpu_power_sleep_wakeup_t cpu_power_sleep_wakeup;
	volatile uint32_t rsv_12_13[2];
	volatile sys_cpu0_int_0_31_en_t cpu0_int_0_31_en;
	volatile sys_cpu0_int_32_63_en_t cpu0_int_32_63_en;
	volatile sys_cpu0_int_64_95_en_t cpu0_int_64_95_en;
	volatile sys_cpu1_int_0_31_en_t cpu1_int_0_31_en;
	volatile sys_cpu1_int_32_63_en_t cpu1_int_32_63_en;
	volatile sys_cpu1_int_64_95_en_t cpu1_int_64_95_en;
	volatile sys_m55sub_int_0_31_en_t m55sub_int_0_31_en;
	volatile sys_m55sub_int_32_63_en_t m55sub_int_32_63_en;
	volatile sys_m55sub_int_64_95_en_t m55sub_int_64_95_en;
	volatile uint32_t rsv_1d_1d[1];
	volatile sys_reserver_reg0x1e_t reserver_reg0x1e;
	volatile sys_reserver_reg0x1f_t reserver_reg0x1f;
	volatile sys_cpu0_int_0_31_status_t cpu0_int_0_31_status;
	volatile sys_cpu0_int_32_63_status_t cpu0_int_32_63_status;
	volatile sys_cpu0_int_64_95_status_t cpu0_int_64_95_status;
	volatile sys_cpu1_int_0_31_status_t cpu1_int_0_31_status;
	volatile sys_cpu1_int_32_63_status_t cpu1_int_32_63_status;
	volatile sys_cpu1_int_64_95_status_t cpu1_int_64_95_status;
	volatile sys_m55sub_int_0_31_status_t m55sub_int_0_31_status;
	volatile sys_m55sub_int_32_63_status_t m55sub_int_32_63_status;
	volatile sys_m55sub_int_64_95_status_t m55sub_int_64_95_status;
	volatile uint32_t rsv_29_29[1];
	volatile sys_reserver_reg0x2a_t reserver_reg0x2a;
	volatile sys_reserver_reg0x2b_t reserver_reg0x2b;
	volatile sys_reserver_reg0x2c_t reserver_reg0x2c;
	volatile uint32_t rsv_2d_2d[1];
	volatile sys_reserver_reg0x2e_t reserver_reg0x2e;
	volatile sys_reserver_reg0x2f_t reserver_reg0x2f;
	volatile sys_gpio_input_status0_t gpio_input_status0;
	volatile sys_gpio_input_status1_t gpio_input_status1;
	volatile sys_gpio_input_status2_t gpio_input_status2;
	volatile sys_reserver_reg0x33_t reserver_reg0x33;
	volatile sys_cpu0_curpc_t cpu0_curpc;
	volatile sys_cpu0_faultstat_H_t cpu0_faultstat_H;
	volatile sys_cpu0_faultstat_L_t cpu0_faultstat_L;
	volatile sys_cpu0_info_t cpu0_info;
	volatile sys_sys_debug_config0_t sys_debug_config0;
	volatile sys_sys_debug_config1_t sys_debug_config1;
	volatile sys_anareg_stat_t anareg_stat;
	volatile sys_reserver_reg0x3b_t reserver_reg0x3b;
	volatile sys_cpu1_curpc_t cpu1_curpc;
	volatile sys_cpu1_faultstat_H_t cpu1_faultstat_H;
	volatile sys_cpu1_faultstat_L_t cpu1_faultstat_L;
	volatile sys_cpu1_info_t cpu1_info;
	volatile sys_ana_reg0_t ana_reg0;
	volatile sys_ana_reg1_t ana_reg1;
	volatile sys_ana_reg2_t ana_reg2;
	volatile sys_ana_reg3_t ana_reg3;
	volatile sys_ana_reg4_t ana_reg4;
	volatile sys_ana_reg5_t ana_reg5;
	volatile sys_ana_reg6_t ana_reg6;
	volatile sys_ana_reg7_t ana_reg7;
	volatile sys_ana_reg8_t ana_reg8;
	volatile sys_ana_reg9_t ana_reg9;
	volatile sys_ana_reg10_t ana_reg10;
	volatile sys_ana_reg11_t ana_reg11;
	volatile sys_ana_reg12_t ana_reg12;
	volatile sys_ana_reg13_t ana_reg13;
	volatile sys_ana_reg14_t ana_reg14;
	volatile sys_ana_reg15_t ana_reg15;
	volatile sys_ana_reg16_t ana_reg16;
	volatile sys_ana_reg17_t ana_reg17;
	volatile sys_ana_reg18_t ana_reg18;
	volatile sys_ana_reg19_t ana_reg19;
	volatile sys_ana_reg20_t ana_reg20;
	volatile sys_ana_reg21_t ana_reg21;
	volatile sys_ana_reg22_t ana_reg22;
	volatile sys_ana_reg23_t ana_reg23;
	volatile sys_ana_reg24_t ana_reg24;
	volatile sys_ana_reg25_t ana_reg25;
	volatile sys_ana_reg26_t ana_reg26;
	volatile sys_ana_reg27_t ana_reg27;
	volatile sys_ana_reg28_t ana_reg28;
	volatile sys_ana_reg29_t ana_reg29;
	volatile sys_ana_reg30_t ana_reg30;
	volatile sys_ana_reg31_t ana_reg31;
	volatile sys_ana_reg32_t ana_reg32;
	volatile sys_ana_reg33_t ana_reg33;
	volatile sys_ana_reg34_t ana_reg34;
	volatile sys_ana_reg35_t ana_reg35;
	volatile sys_ana_reg36_t ana_reg36;
	volatile sys_ana_reg37_t ana_reg37;
	volatile sys_ana_reg38_t ana_reg38;
	volatile sys_ana_reg39_t ana_reg39;
	volatile sys_ana_reg40_t ana_reg40;
	volatile sys_ana_reg41_t ana_reg41;
	volatile sys_ana_reg42_t ana_reg42;
	volatile sys_ana_reg43_t ana_reg43;
	volatile sys_ana_reg44_t ana_reg44;
	volatile sys_ana_reg45_t ana_reg45;
	volatile sys_ana_reg46_t ana_reg46;
	volatile sys_ana_reg47_t ana_reg47;
} sys_hw_t;

#ifdef __cplusplus
}
#endif
