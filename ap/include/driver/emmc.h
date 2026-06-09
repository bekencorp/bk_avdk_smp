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

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * eMMC device protocol driver.
 *
 * Built ENTIRELY on the generic SDIO host controller interface
 * (<driver/sdio_host.h>); it never touches controller registers directly, so
 * it works on either BK7259 SDIO controller (selected by CONFIG_EMMC_HOST_ID).
 *
 * @note eMMC high-speed modes (HS200/HS400) need controller delay-line tuning
 *       and 1.8V signaling that require board bring-up; this driver brings the
 *       device up in backward-compatible (high-speed SDR) mode and exposes the
 *       capacity and block read/write path. HS200/HS400 enablement is marked
 *       with TODO[HW] in the implementation.
 */

typedef struct {
	uint32_t cid[4];           /**< CID register */
	uint32_t csd[4];           /**< CSD register */
	uint16_t rca;              /**< assigned relative card address */
	uint32_t sector_count;     /**< capacity in 512-byte sectors */
	uint8_t  bus_width;        /**< active bus width (1/4/8) */
	uint8_t  ext_csd_rev;      /**< EXT_CSD revision */
	bool     is_high_capacity; /**< sector addressing (>2GB) */
} emmc_info_t;

/**
 * @brief Initialize and enumerate the eMMC device.
 *
 * Runs the full eMMC init sequence (CMD0/CMD1/CMD2/CMD3/CMD9/CMD7), reads and
 * parses EXT_CSD, switches to the configured bus width and high-speed timing.
 */
bk_err_t bk_emmc_init(void);

/**
 * @brief Deinitialize the eMMC device and release the host controller.
 */
bk_err_t bk_emmc_deinit(void);

/**
 * @brief Read @p block_num 512-byte blocks starting at @p block_addr (LBA).
 */
bk_err_t bk_emmc_read_blocks(uint8_t *data, uint32_t block_addr, uint32_t block_num);

/**
 * @brief Write @p block_num 512-byte blocks starting at @p block_addr (LBA).
 */
bk_err_t bk_emmc_write_blocks(const uint8_t *data, uint32_t block_addr, uint32_t block_num);

/**
 * @brief Get the device capacity in 512-byte sectors.
 */
uint32_t bk_emmc_get_sector_count(void);

/**
 * @brief Copy the enumerated device info.
 */
bk_err_t bk_emmc_get_info(emmc_info_t *info);

#ifdef __cplusplus
}
#endif
