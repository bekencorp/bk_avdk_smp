// Copyright 2022-2025 Beken
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
		uint32_t deviceid                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sdio1_dev_id_t;


typedef volatile union {
	struct {
		uint32_t versionid                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sdio1_ver_id_t;


typedef volatile union {
	struct {
		uint32_t soft_resetn              :  1; /**<bit[0 : 0] */
		uint32_t bps_clkgate              :  1; /**<bit[1 : 1] */
		uint32_t reserved_bit_2_31        : 30; /**<bit[2 : 31] */
	};
	uint32_t v;
} sdio1_clkg_reset_t;


typedef volatile union {
	struct {
		uint32_t globalstatus             : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} sdio1_status_t;


typedef volatile union {
	struct {
		uint32_t tmclk_div                :  8; /**<bit[0 : 7] */
		uint32_t cqet_mclk_div            :  8; /**<bit[8 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} sdio1_div_ctrl_t;


typedef volatile union {
	struct {
		uint32_t card_write_prot          :  1; /**<bit[0 : 0] */
		uint32_t card_detect_n            :  1; /**<bit[1 : 1] */
		uint32_t led_control              :  1; /**<bit[2 : 2] */
		uint32_t sd_datxfer_width         :  2; /**<bit[3 : 4] */
		uint32_t tuning_rx_sel0           :  3; /**<bit[5 : 7] */
		uint32_t tuning_rx_sel1           :  3; /**<bit[8 : 10] */
		uint32_t reserved_11_13           :  3; /**<bit[11 : 13] */
		uint32_t sample_rx_sel0           :  1; /**<bit[14 : 14] */
		uint32_t sample_rx_sel1           :  1; /**<bit[15 : 15] */
		uint32_t reserved_16_16           :  1; /**<bit[16 : 16] */
		uint32_t tuning_tx_sel0           :  3; /**<bit[17 : 19] */
		uint32_t tuning_tx_sel1           :  3; /**<bit[20 : 22] */
		uint32_t reserved_23_25           :  3; /**<bit[23 : 25] */
		uint32_t sample_tx_sel0           :  1; /**<bit[26 : 26] */
		uint32_t sample_tx_sel1           :  1; /**<bit[27 : 27] */
		uint32_t reserved_28_28           :  1; /**<bit[28 : 28] */
		uint32_t clk_drv_negedge_sel      :  1; /**<bit[29 : 29] */
		uint32_t reserved_30_31           :  2; /**<bit[30 : 31] */
	};
	uint32_t v;
} sdio1_sdio_ctrl_t;


typedef volatile union {
	struct {
		uint32_t mhprot                   :  4; /**<bit[0 : 3] */
		uint32_t mhprot_sel               :  1; /**<bit[4 : 4] */
		uint32_t reserved_5_31            : 27; /**<bit[5 : 31] */
	};
	uint32_t v;
} sdio1_prot_ctrl_t;

typedef volatile struct {
	volatile sdio1_dev_id_t dev_id;
	volatile sdio1_ver_id_t ver_id;
	volatile sdio1_clkg_reset_t clkg_reset;
	volatile sdio1_status_t status;
	volatile sdio1_div_ctrl_t div_ctrl;
	volatile sdio1_sdio_ctrl_t sdio_ctrl;
	volatile sdio1_prot_ctrl_t prot_ctrl;
} sdio1_hw_t;

#ifdef __cplusplus
}
#endif
