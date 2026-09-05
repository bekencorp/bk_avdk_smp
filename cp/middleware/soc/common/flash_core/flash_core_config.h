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

#pragma once

#include <stdint.h>

/*
 * flash_line_mode_t / flash_protect_type_t / BK_ERR_FLASH_ADDR_OUT_OF_RANGE
 * normally come from the SDK's <driver/flash_types.h>. The aboot bootloader
 * builds bare-metal (no SDK include tree), so when it defines
 * FLASH_CORE_FREESTANDING we provide the same definitions locally. Pulling
 * <driver/flash_types.h> here would drag <common/bk_include.h> and
 * <common/bk_err.h> into every bootloader translation unit that includes
 * driver_flash.h, so the freestanding branch stays self-contained. The values
 * MUST stay in sync with cp/include/driver/flash_types.h.
 */
#if defined(FLASH_CORE_FREESTANDING)
typedef enum {
	FLASH_LINE_MODE_TWO = 2,
	FLASH_LINE_MODE_FOUR = 4,
} flash_line_mode_t;

typedef enum {
	FLASH_PROTECT_NONE = 0,
	FLASH_PROTECT_ALL,
	FLASH_PROTECT_HALF,
	FLASH_UNPROTECT_LAST_BLOCK,
} flash_protect_type_t;

#ifndef BK_ERR_FLASH_ADDR_OUT_OF_RANGE
#define BK_ERR_FLASH_ADDR_OUT_OF_RANGE (-0x3702) /* BK_ERR_FLASH_BASE(-0x3700) - 2 */
#endif
#else
#include <driver/flash_types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Single source of truth for the bk7259 flash configuration table.
 *
 * This replaces the three byte-identical copies that used to live in
 * cp/middleware/.../flash_driver.c, ap/middleware/.../flash_driver.c and the
 * TF-M armino_min/hal/flash_min.c. The struct layout is unchanged (13 fields).
 */

#ifndef FLASH_SIZE_1M
#define FLASH_SIZE_1M                    0x100000
#endif
#ifndef FLASH_SIZE_2M
#define FLASH_SIZE_2M                    0x200000
#endif
#ifndef FLASH_SIZE_4M
#define FLASH_SIZE_4M                    0x400000
#endif
#ifndef FLASH_SIZE_8M
#define FLASH_SIZE_8M                    0x800000
#endif
#ifndef FLASH_SIZE_16M
#define FLASH_SIZE_16M                   0x1000000
#endif

typedef struct {
	uint32_t flash_id;
	uint32_t flash_size;
	uint8_t status_reg_size; /**< the byte count of status register */
	flash_line_mode_t line_mode;
	uint8_t cmp_post; /**< CMP bit position in status register */
	uint8_t protect_post; /**< block protect bits position in status register */
	uint8_t protect_mask; /**< block protect bits mask value in status register */
	uint16_t protect_all;
	uint16_t protect_none;
	uint16_t unprotect_last_block;
	uint8_t quad_en_post; /**< quad enable bit position in status register */
	uint8_t quad_en_val; /**< When the QE pin is set to quad_en_val(1 or 0), the Quad IO2 and IO3 pins are enabled */
	uint8_t coutinuous_read_mode_bits_val;
} flash_config_t;

/** The single shared table. Last entry (flash_id == 0) is the catch-all default. */
extern const flash_config_t flash_config[];

/** Number of entries in flash_config[], including the trailing default entry. */
extern const uint32_t flash_config_num;

#ifdef __cplusplus
}
#endif
