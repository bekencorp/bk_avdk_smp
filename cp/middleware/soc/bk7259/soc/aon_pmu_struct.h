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
		uint32_t memchk_bps                       :  1; /**<bit[0 : 0] */
		uint32_t fast_boot                        :  1; /**<bit[1 : 1] */
		uint32_t ota_finish                       :  1; /**<bit[2 : 2] */
		uint32_t bl2_deep_sleep                   :  1; /**<bit[3 : 3] */
		uint32_t dlv_startup                      :  1; /**<bit[4 : 4] */
		uint32_t aon_reg0_for_software            :  7; /**<bit[5 : 11] */
		uint32_t gpio_retention_bitmap            :  8; /**<bit[12 : 19] */
		uint32_t reset_count                      :  4 ;/**<bit[20 : 23] */
		uint32_t reset_reason                     :  7; /**<bit[24 : 30] */
		uint32_t gpio_sleep                       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r0_t;


typedef volatile union {
	struct {
		uint32_t wdt_rst_ana                      :  1; /**<bit[0 : 0] */
		uint32_t wdt_rst_top                      :  1; /**<bit[1 : 1] */
		uint32_t wdt_rst_aon                      :  1; /**<bit[2 : 2] */
		uint32_t wdt_rst_awt                      :  1; /**<bit[3 : 3] */
		uint32_t wdt_rst_gpio                     :  1; /**<bit[4 : 4] */
		uint32_t wdt_rst_rtc                      :  1; /**<bit[5 : 5] */
		uint32_t wdt_rst_wdt                      :  1; /**<bit[6 : 6] */
		uint32_t wdt_rst_pmu                      :  1; /**<bit[7 : 7] */
		uint32_t wdt_rst_blp_wlp                  :  1; /**<bit[8 : 8] */
		uint32_t reserved_bit_9_15                :  7; /**<bit[9 : 15] */
		uint32_t m55_iso_en                       :  1; /**<bit[16 : 16] */
		uint32_t m55_rstn                         :  1; /**<bit[17 : 17] */
		uint32_t m55_mem_ret                      :  1; /**<bit[18 : 18] */
		uint32_t m55_mem3_pwd                     :  1; /**<bit[19 : 19] */
		uint32_t m55_clk_en                       :  1; /**<bit[20 : 20] */
		uint32_t m55_mem4_pwd                     :  1; /**<bit[21 : 21] */
		uint32_t m55_mem5_pwd                     :  1; /**<bit[22 : 22] */
		uint32_t m55_mem6_pwd                     :  1; /**<bit[23 : 23] */
		uint32_t m55_cpu2_cache_pwd               :  1; /**<bit[24 : 24] */
		uint32_t m55_cpu3_cache_pwd               :  1; /**<bit[25 : 25] */
		uint32_t m55_cpu2_cache_init_dis_maint    :  1; /**<bit[26 : 26] */
		uint32_t m55_cpu3_cache_init_dis_maint    :  1; /**<bit[27 : 27] */
		uint32_t m55_mem_auto_set                 :  1; /**<bit[28 : 28] */
		uint32_t m55_auto_sel                     :  1; /**<bit[29 : 29] */
		uint32_t reserved_bit_30_30               :  1; /**<bit[30 : 30] */
		uint32_t otp_vdd_en                       :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r2_t;


typedef volatile union {
	struct {
		uint32_t for_sw                           : 31; /**<bit[0 : 30] */
		uint32_t shutdown_flag                    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r3_t;


typedef volatile union {
	struct {
		uint32_t program_state                    :  4; /**<bit[0 : 3] */
		uint32_t reserved_bit_4_30                : 27; /**<bit[4 : 30] */
		uint32_t aon_encp_jtag_diable             :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r4_t;


typedef volatile union {
	struct {
		uint32_t encp_spi_to_ahb_enable           :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} aon_pmu_r5_t;


typedef volatile union {
	struct {
		uint32_t reg3v_clk                        : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r25_t;


typedef volatile union {
	struct {
		uint32_t wake1_delay                      :  4; /**<bit[0 : 3] */
		uint32_t wake2_delay                      :  4; /**<bit[4 : 7] */
		uint32_t wake3_delay                      :  4; /**<bit[8 : 11] */
		uint32_t halt1_delay                      :  4; /**<bit[12 : 15] */
		uint32_t halt2_delay                      :  4; /**<bit[16 : 19] */
		uint32_t halt3_delay                      :  4; /**<bit[20 : 23] */
		uint32_t halt_volt                        :  1; /**<bit[24 : 24] */
		uint32_t halt_xtal                        :  1; /**<bit[25 : 25] */
		uint32_t halt_core                        :  1; /**<bit[26 : 26] */
		uint32_t halt_flash                       :  1; /**<bit[27 : 27] */
		uint32_t halt_rosc                        :  1; /**<bit[28 : 28] */
		uint32_t halt_resten                      :  1; /**<bit[29 : 29] */
		uint32_t halt_isolat                      :  1; /**<bit[30 : 30] */
		uint32_t halt_clkena                      :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r40_t;


typedef volatile union {
	struct {
		uint32_t lpo_config                       :  2; /**<bit[0 : 1] */
		uint32_t flshsck_iocap                    :  2; /**<bit[2 : 3] */
		uint32_t wakeup_ena                       :  7; /**<bit[4 : 10] */
		uint32_t gpio_int_clksel                  :  1; /**<bit[11 : 11] */
		uint32_t reserved_bit_12_13               :  2; /**<bit[12 : 13] */
		uint32_t xtal_sel                         :  1; /**<bit[14 : 14] */
		uint32_t gpio_func_ctrl_en                :  1; /**<bit[15 : 15] */
		uint32_t sleep_wdt_disable                :  1; /**<bit[16 : 16] */
		uint32_t reserved_bit_17_18               :  2; /**<bit[17 : 18] */
		uint32_t wlp_switch_en                    :  1; /**<bit[19 : 19] */
		uint32_t lcd_clk_inv                      :  1; /**<bit[20 : 20] */
		uint32_t reserved_bit_21_23               :  3; /**<bit[21 : 23] */
		uint32_t halt_lpo                         :  1; /**<bit[24 : 24] */
		uint32_t halt_sram0                       :  1; /**<bit[25 : 25] */
		uint32_t halt_sram1                       :  1; /**<bit[26 : 26] */
		uint32_t halt_sram2                       :  1; /**<bit[27 : 27] */
		uint32_t mem_ret_en                       :  1; /**<bit[28 : 28] */
		uint32_t halt_cache                       :  1; /**<bit[29 : 29] */
		uint32_t m52_l2_dis_cache_init_dis_maint  :  1; /**<bit[30 : 30] */
		uint32_t m52_cpu_l2_dis_cache_en_maint    :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r41_t;


typedef volatile union {
	struct {
		uint32_t pwd_blppwd                       :  1; /**<bit[0 : 0] */
		uint32_t pwd_wlppwd                       :  1; /**<bit[1 : 1] */
		uint32_t blp_isolate_state                :  1; /**<bit[2 : 2] */
		uint32_t wlp_isolate_state                :  1; /**<bit[3 : 3] */
		uint32_t reserved_bit_4_31                : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} aon_pmu_r42_t;


typedef volatile union {
	struct {
		uint32_t clr_int_touched                  : 16; /**<bit[0 : 15] */
		uint32_t clr_int_usbplug                  :  1; /**<bit[16 : 16] */
		uint32_t clr_wakeup                       :  1; /**<bit[17 : 17] */
		uint32_t reserved_bit_18_31               : 14; /**<bit[18 : 31] */
	};
	uint32_t v;
} aon_pmu_r43_t;


typedef volatile union {
	struct {
		uint32_t int_touched                      : 16; /**<bit[0 : 15] */
		uint32_t int_usbplug                      :  1; /**<bit[16 : 16] */
		uint32_t reserved_bit_17_31               : 15; /**<bit[17 : 31] */
	};
	uint32_t v;
} aon_pmu_r70_t;


typedef volatile union {
	struct {
		uint32_t touch_state                      : 16; /**<bit[0 : 15] */
		uint32_t usbplug_state                    :  1; /**<bit[16 : 16] */
		uint32_t reserved_bit_17_19               :  3; /**<bit[17 : 19] */
		uint32_t wakeup_source                    :  7; /**<bit[20 : 26] */
		uint32_t reserved_bit_27_31               :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} aon_pmu_r71_t;

typedef volatile union {
	struct {
		uint32_t td_int_status                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r72_t;


//0x73[16:0]	td_int_status<48:32>
//0x73[25:17]	cap_cal_mode1<8:0>
//0x73[26]	cal_done_mode1
typedef volatile union {
	struct {
		uint32_t td_int_status                      : 17; /**<bit[16:0]  */
		uint32_t cap_cal_mode1                      :  9; /**<bit[25:17] */
		uint32_t cal_done_mode1                     :  1; /**<bit[26]    */
		uint32_t reserved_bit_27_31                 :  5; /**<bit[31:27] */
	};
	uint32_t v;
} aon_pmu_r73_t;

typedef volatile union {
	struct {
		uint32_t por_corehs_n                     :  1; /**<bit[0 : 0] */
		uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} aon_pmu_r74_t;

typedef volatile union {
	struct {
		uint32_t ana_sta4                         : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r75_t;


typedef volatile union {
	struct {
		uint32_t ana_rtc_state                    :  4; /**<bit[0 : 3] */
		uint32_t reserved_bit_4_31                : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} aon_pmu_r78_t;


typedef volatile union {
	struct {
		uint32_t ana_rtc_value                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r79_t;


typedef volatile union {
	struct {
		uint32_t aon_mix0                         : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r7a_t;


typedef volatile union {
	struct {
		uint32_t aon3v_regdi                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r7b_t;


typedef volatile union {
	struct {
		uint32_t id                               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r7c_t;


typedef volatile union {
	struct {
		uint32_t lcal_dac                         :  8; /**<bit[0 : 7] */
		uint32_t l                                :  1; /**<bit[8 : 8] */
		uint32_t adc_cal                          :  6; /**<bit[9 : 14] */
		uint32_t bgcal                            :  6; /**<bit[15 : 20] */
		uint32_t sig_26mpll_unlock                :  1; /**<bit[21 : 21] */
		uint32_t dpll_unlock_l                    :  1; /**<bit[22 : 22] */
		uint32_t dpll_unlock_h                    :  1; /**<bit[23 : 23] */
		uint32_t apll_unlock                      :  1; /**<bit[24 : 24] */
		uint32_t btpll_unlock                     :  1; /**<bit[25 : 25] */
		uint32_t calfail_btpll                    :  1; /**<bit[26 : 26] */
		uint32_t dpll_band                        :  4; /**<bit[27 : 30] */
		uint32_t h                                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r7d_t;


typedef volatile union {
	struct {
		uint32_t cbcal                            :  5; /**<bit[0 : 4] */
		uint32_t ad_state                         :  3; /**<bit[5 : 7] */
		uint32_t bandcal                          :  8; /**<bit[8 : 15] */
		uint32_t cap_mod2_ls                      :  7; /**<bit[16 : 22] */
		uint32_t cap_mod2_hs                      :  8; /**<bit[23 : 30] */
		uint32_t h                                :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} aon_pmu_r7e_t;


typedef volatile union {
	struct {
		uint32_t td_states2                       : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} aon_pmu_r7f_t;

typedef volatile struct {
	volatile aon_pmu_r0_t r0;
	volatile aon_pmu_r2_t r2;
	volatile aon_pmu_r3_t r3;
	volatile aon_pmu_r4_t r4;
	volatile aon_pmu_r5_t r5;
	volatile uint32_t rsv_6_24[31];
	volatile aon_pmu_r25_t r25;
	volatile uint32_t rsv_26_3f[26];
	volatile aon_pmu_r40_t r40;
	volatile aon_pmu_r41_t r41;
	volatile aon_pmu_r42_t r42;
	volatile aon_pmu_r43_t r43;
	volatile uint32_t rsv_44_6f[44];
	volatile aon_pmu_r70_t r70;
	volatile aon_pmu_r71_t r71;
	volatile uint32_t rsv_72_73[2];
	volatile aon_pmu_r74_t r74;
	volatile aon_pmu_r75_t r75;
	volatile uint32_t rsv_76_77[2];
	volatile aon_pmu_r78_t r78;
	volatile aon_pmu_r79_t r79;
	volatile aon_pmu_r7a_t r7a;
	volatile aon_pmu_r7b_t r7b;
	volatile aon_pmu_r7c_t r7c;
	volatile aon_pmu_r7d_t r7d;
	volatile aon_pmu_r7e_t r7e;
	volatile aon_pmu_r7f_t r7f;
} aon_pmu_hw_t;

#ifdef __cplusplus
}
#endif
