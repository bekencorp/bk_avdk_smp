// Copyright 2020-2021 Beken
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

#include "flash_core_config.h"

/*
 * Merged single copy of the bk7259 flash table. The three former copies
 * (cp/ap middleware flash_driver.c and TF-M flash_min.c) were byte-identical;
 * this is that content, verbatim, now maintained in exactly one place.
 */
const flash_config_t flash_config[] = {
	/* flash_id, flash_size,    status_reg_size, line_mode,            cmp_post, protect_post, protect_mask, protect_all, protect_none, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val   */
	{0xC84016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //gd_25q32c
	{0xC86516,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //gd_25wq32e
	{0xC86517,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //gd_25wq64e
	{0xCD6017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //th_25q64ha
	{0xCD7017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //th_25q64ub
	{0xC86518,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //gd_25wq128e
	{0x204018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,	               9,            1,           0x20,                         }, //xm_25qh128d
	{0x852018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x0E,                9,            1,           0xA0,                         }, //py_25q129ha
	{0x000000,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_TWO,    0,        2,            0x1F,         0x00,        0x00,         0x00,                0,            0,           0x00,                         }, //default
};

const uint32_t flash_config_num = sizeof(flash_config) / sizeof(flash_config[0]);
