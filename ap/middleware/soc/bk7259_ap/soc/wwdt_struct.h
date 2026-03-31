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
} wwdt_smb_devid_t;


typedef volatile union {
	struct {
		uint32_t version_id               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} wwdt_smb_verid_t;


typedef volatile union {
	struct {
		uint32_t soft_reset               :  1; /**<bit[0 : 0] */
		uint32_t clkg_bypass              :  1; /**<bit[1 : 1] */
		uint32_t resv                     : 30; /**<bit[2 : 31] */
	};
	uint32_t v;
} wwdt_smb_clkrst_t;


typedef volatile union {
	struct {
		uint32_t dev_status               : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} wwdt_smb_state_t;


typedef volatile union {
	struct {
		uint32_t period                   : 16; /**<bit[0 : 15] */
		uint32_t key                      :  8; /**<bit[16 : 23] */
		uint32_t reserved_bit_24_31       :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} wwdt_wdt_config_t;


typedef volatile union {
	struct {
		uint32_t count                    : 16; /**<bit[0 : 15] */
		uint32_t reserved_bit_16_31       : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} wwdt_wdt_cnt_t;


typedef volatile union {
	struct {
		uint32_t win_val                  : 16; /**<bit[0 : 15] */
		uint32_t win_key                  :  8; /**<bit[16 : 23] */
		uint32_t win_en                   :  1; /**<bit[24 : 24] */
		uint32_t reserved_bit_25_31       :  7; /**<bit[25 : 31] */
	};
	uint32_t v;
} wwdt_wdt_win_set_t;


typedef volatile union {
	struct {
		uint32_t cpu_id                   :  4; /**<bit[0 : 3] */
		uint32_t magic_word               :  4; /**<bit[4 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} wwdt_cpuid_t;

typedef volatile struct {
	volatile wwdt_smb_devid_t smb_devid;
	volatile wwdt_smb_verid_t smb_verid;
	volatile wwdt_smb_clkrst_t smb_clkrst;
	volatile wwdt_smb_state_t smb_state;
	volatile wwdt_wdt_config_t wdt_config;
	volatile wwdt_wdt_cnt_t wdt_cnt;
	volatile wwdt_wdt_win_set_t wdt_win_set;
	volatile wwdt_cpuid_t cpuid;
} wwdt_hw_t;

#ifdef __cplusplus
}
#endif
