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
		uint32_t gpio_input               :  1; /**<bit[0 : 0] */
		uint32_t gpio_output              :  1; /**<bit[1 : 1] */
		uint32_t gpio_input_ena           :  1; /**<bit[2 : 2] */
		uint32_t gpio_output_ena          :  1; /**<bit[3 : 3] */
		uint32_t gpio_pull_mode           :  1; /**<bit[4 : 4] */
		uint32_t gpio_pull_ena            :  1; /**<bit[5 : 5] */
		uint32_t gpio_fun_ena             :  1; /**<bit[6 : 6] */
		uint32_t input_monitor            :  1; /**<bit[7 : 7] */
		uint32_t gpio_capacity            :  2; /**<bit[8 : 9] */
		uint32_t gpio_int_type            :  2; /**<bit[10 : 11] */
		uint32_t gpio_int_ena             :  1; /**<bit[12 : 12] */
		uint32_t gpio_int_clear           :  1; /**<bit[13 : 13] */
		uint32_t reserved_14_15           :  2; /**<bit[14 : 15] */
		uint32_t gpio_state               :  5; /**<bit[16 : 20] */
		uint32_t reserved_21_23           :  3; /**<bit[21 : 23] */
		uint32_t gpio_fun_sel             :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} gpio_cfg_t;


typedef volatile union {
	struct {
		uint32_t gpio_intsta0             : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} gpio_intsta0_t;


typedef volatile union {
	struct {
		uint32_t gpio_intsta1             : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} gpio_intsta1_t;


typedef volatile union {
	struct {
		uint32_t gpio_intsta2             :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} gpio_intsta2_t;


typedef volatile union {
	struct {
		uint32_t m52_gpoup_int_mask       :  9; /**<bit[0 : 8] */
		uint32_t m52_even_int_mask        :  1; /**<bit[9 : 9] */
		uint32_t m52_odd_int_mask         :  1; /**<bit[10 : 10] */
		uint32_t reserved_11_15           :  5; /**<bit[11 : 15] */
		uint32_t m55_gpoup_int_mask       :  9; /**<bit[16 : 24] */
		uint32_t m55_even_int_mask        :  1; /**<bit[25 : 25] */
		uint32_t m55_odd_int_mask         :  1; /**<bit[26 : 26] */
		uint32_t reserved_27_31           :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} gpio_int_mask_t;

typedef volatile struct {
	volatile gpio_cfg_t cfg[SOC_GPIO_NUM];
	volatile uint32_t rsv_48_77[48];
	volatile gpio_intsta0_t intsta0;
	volatile gpio_intsta1_t intsta1;
	volatile gpio_intsta2_t intsta2;
	volatile uint32_t rsv_7b_7b[1];
	volatile gpio_int_mask_t int_mask;
} gpio_hw_t;

#ifdef __cplusplus
}
#endif
