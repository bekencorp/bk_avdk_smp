// Copyright 2024-2025 Beken
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
#include <stdbool.h>
#include <common/bk_err.h>
#include <driver/hal/hal_qspi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Max JEDEC ID bytes compared during Read ID (9Fh) probing. */
#define QSPI_NAND_ID_MAX_LEN         4U

/* Device capacity and page/block geometry. */
typedef struct {
	uint32_t total_size;        /**< device capacity in bytes */
	uint32_t page_size;         /**< main-array bytes per page (e.g. 2048) */
	uint32_t spare_size;        /**< spare/OOB bytes per page (e.g. 64) */
	uint32_t pages_per_block;   /**< pages per erase block (e.g. 64) */
} qspi_nand_geometry_t;

/* Command opcodes and command-phase field widths. */
typedef struct {
	uint8_t reset;
	uint8_t write_enable;
	uint8_t get_feature;
	uint8_t set_feature;
	uint8_t block_erase;
	uint8_t program_load;
	uint8_t program_load_random;
	uint8_t program_execute;
	uint8_t page_read;
	uint8_t read_cache;
	uint8_t read_cache_x2;
	uint8_t read_cache_x4;
	uint8_t read_cache_quad;
	uint8_t program_load_x4;
	uint8_t program_load_random_x4;
	uint8_t feature_addr_len;
	uint8_t column_addr_len;
	uint8_t row_addr_len;
	uint8_t feature_data_len;
	uint8_t read_cache_dummy_cycles;
	uint8_t read_cache_dummy_mode;
} qspi_nand_cmd_t;

/* Feature-register addresses. */
typedef struct {
	uint8_t block_lock;         /**< A0h protection register */
	uint8_t configuration;      /**< B0h configuration register */
	uint8_t status;             /**< C0h status register */
	uint8_t block_unlock_value; /**< value written to unlock all blocks */
} qspi_nand_reg_t;

/* Status/feature bit positions and ECC-status field encoding. */
typedef struct {
	uint8_t oip;                /**< operation-in-progress */
	uint8_t wel;                /**< write-enable-latch */
	uint8_t erase_fail;
	uint8_t program_fail;
	uint8_t ecc_enable;         /**< B0h ECC-E bit */
	uint8_t quad_enable;        /**< B0h QE bit */
	uint8_t wp_enable;          /**< A0h WP-E bit, part specific */
	uint8_t ecc_status_mask;    /**< C0h ECCS field mask */
	uint8_t ecc_status_shift;   /**< C0h ECCS field shift */
	uint8_t ecc_no_error;       /**< ECCS value: no error */
	uint8_t ecc_corrected;      /**< ECCS value: corrected */
	uint8_t ecc_uncorrectable;  /**< ECCS value: uncorrectable */
} qspi_nand_bit_t;

/* One supported SPI-NAND part. Selected at init by matching Read ID (9Fh). */
typedef struct {
	const char *name;
	uint8_t id[QSPI_NAND_ID_MAX_LEN];
	uint8_t id_len;
	qspi_nand_geometry_t geometry;
	qspi_nand_cmd_t cmd;
	qspi_nand_reg_t reg;
	qspi_nand_bit_t bit;
} qspi_nand_dev_t;

/* Linker-section entry emitted by ::QSPI_NAND_DEVICE_SECTION. Only a fixed-size
 * pointer is placed in the section so iteration and alignment stay simple. */
typedef struct {
	const qspi_nand_dev_t *dev;
} qspi_nand_device_entry_t;

#define _QSPI_NAND_STRINGIFY(x)          #x
#define _QSPI_NAND_SECTION_ATTR(sec, uid) \
	__attribute__((used, section(sec "." _QSPI_NAND_STRINGIFY(uid))))

#define _QSPI_NAND_DEVICE_SECTION_IMPL(dev_sym, uid)                                \
	static _QSPI_NAND_SECTION_ATTR(".qspi_nand_device_list", uid)                   \
	const qspi_nand_device_entry_t qspi_nand_entry_##dev_sym = {                     \
		.dev = &(dev_sym),                                                          \
	}

/**
 * @brief Register a SPI-NAND descriptor into the linker-section registry.
 *
 * Drops a ::qspi_nand_device_entry_t pointing at @p dev_sym into the
 * ``.qspi_nand_device_list`` section, so the driver can discover it at init
 * without any central registration function.
 *
 * @param dev_sym  A ::qspi_nand_dev_t object, e.g. ``qspi_nand_zb35q01_dev``.
 */
#define QSPI_NAND_DEVICE_SECTION(dev_sym)                                           \
	_QSPI_NAND_DEVICE_SECTION_IMPL(dev_sym, __COUNTER__)

extern qspi_nand_device_entry_t __qspi_nand_device_array_start;
extern qspi_nand_device_entry_t __qspi_nand_device_array_end;

/**
 * @brief Get the descriptor selected for @p id (probed at init).
 *
 * @return the active descriptor, or a built-in default when @p id was not
 *         identified. Never returns NULL for a valid id.
 */
const qspi_nand_dev_t *bk_qspi_nand_dev(qspi_id_t id);

/**
 * @brief Total device capacity in bytes for @p id (from the active descriptor).
 */
uint32_t bk_qspi_flash_nand_total_size(qspi_id_t id);

#ifdef __cplusplus
}
#endif
