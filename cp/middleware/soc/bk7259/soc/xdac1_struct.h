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
		uint32_t deviceid                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} xdac1_reg0_t;


typedef volatile union {
	struct {
		uint32_t versionid                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} xdac1_reg1_t;


typedef volatile union {
	struct {
		uint32_t soft_reset               :  1; /**<bit[0 : 0] */
		uint32_t clkg_bypass              :  1; /**<bit[1 : 1] */
		uint32_t reserved_bit_2_31        : 30; /**<bit[2 : 31] */
	};
	uint32_t v;
} xdac1_reg2_t;


typedef volatile union {
	struct {
		uint32_t devstatus                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} xdac1_reg3_t;


typedef volatile union {
	struct {
		uint32_t dac_enable               :  1; /**<bit[0 : 0] */
		uint32_t dac_clk_en               :  1; /**<bit[1 : 1] */
		uint32_t dac_mode                 :  1; /**<bit[2 : 2] */
		uint32_t fifo_enable              :  1; /**<bit[3 : 3] */
		uint32_t reserved_4_15            : 12; /**<bit[4 : 15] */
		uint32_t dac_clk_div              : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} xdac1_reg4_t;


typedef volatile union {
	struct {
		uint32_t fifo_empty_int           :  1; /**<bit[0 : 0] */
		uint32_t fifo_full_int            :  1; /**<bit[1 : 1] */
		uint32_t fifo_near_full_int       :  1; /**<bit[2 : 2] */
		uint32_t fifo_near_empty_int      :  1; /**<bit[3 : 3] */
		uint32_t reserved_4_31            : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} xdac1_reg5_t;


typedef volatile union {
	struct {
		uint32_t fifo_empty_int_en        :  1; /**<bit[0 : 0] */
		uint32_t fifo_full_int_en         :  1; /**<bit[1 : 1] */
		uint32_t fifo_near_full_int_en    :  1; /**<bit[2 : 2] */
		uint32_t fifo_near_empty_int_en   :  1; /**<bit[3 : 3] */
		uint32_t reserved_4_31            : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} xdac1_reg6_t;


typedef volatile union {
	struct {
		uint32_t dac_rthrd                :  5; /**<bit[0 : 4] */
		uint32_t dac_wthrd                :  5; /**<bit[5 : 9] */
		uint32_t reserved_10_31           : 22; /**<bit[10 : 31] */
	};
	uint32_t v;
} xdac1_reg7_t;


typedef volatile union {
	struct {
		uint32_t tx_fifo_wr_data          : 12; /**<bit[0 : 11] */
		uint32_t reserved_12_31           : 20; /**<bit[12 : 31] */
	};
	uint32_t v;
} xdac1_reg8_t;

typedef volatile struct {
	volatile xdac1_reg0_t reg0;
	volatile xdac1_reg1_t reg1;
	volatile xdac1_reg2_t reg2;
	volatile xdac1_reg3_t reg3;
	volatile xdac1_reg4_t reg4;
	volatile xdac1_reg5_t reg5;
	volatile xdac1_reg6_t reg6;
	volatile xdac1_reg7_t reg7;
	volatile xdac1_reg8_t reg8;
} xdac1_hw_t;

#ifdef __cplusplus
}
#endif
