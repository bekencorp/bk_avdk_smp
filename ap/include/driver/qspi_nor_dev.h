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

/* Max JEDEC ID bytes compared during Read ID (9Fh) probing:
 * MID + ID15-8 + ID7-0. */
#define QSPI_NOR_ID_MAX_LEN          3U

/* Device capacity and program/erase geometry. */
typedef struct {
	uint32_t total_size;        /**< device capacity in bytes */
	uint32_t sector_size;       /**< erase sector size (e.g. 4096) */
	uint32_t page_size;         /**< program page size (e.g. 256) */
} qspi_nor_geometry_t;

/* Command opcodes, dummy timing and address width. */
typedef struct {
	uint8_t read_id;                 /**< 0x9F */
	uint8_t write_enable;            /**< 0x06 */
	uint8_t read_sr1;                /**< 0x05 read S0-S7 */
	uint8_t read_sr2;                /**< 0x35 read S8-S15 */
	uint8_t read_sr3;                /**< 0x15 read S16-S23 */
	uint8_t write_sr1;               /**< 0x01 write S0-S7 */
	uint8_t write_sr2;               /**< 0x31 write S8-S15 */
	uint8_t write_sr3;               /**< 0x11 write S16-S23 */
	/* Address-phase opcodes below. For parts > 16MB that use 4-byte
	 * addressing (addr_bytes == 4), populate these with the dedicated
	 * 4-byte-address opcode variants (e.g. 0x13/0x12/0x21/0xDC/0xEC) instead
	 * of the 3-byte ones. The driver emits addr_bytes address bytes with the
	 * opcode as-is, so no mode switch (Enter-4B) is required and operation is
	 * reset-safe. Keep enter_4byte = 0 in that case. */
	uint8_t page_program;            /**< 0x02 PP (3B) / 0x12 PP4B */
	uint8_t quad_page_program;       /**< 0x32 QPP (3B) / 0x34 QPP4B */
	uint8_t read;                    /**< 0x03 READ (3B) / 0x13 READ4B */
	uint8_t dual_read;               /**< 0x3B DOFR (3B) / 0x3C DOFR4B */
	uint8_t quad_read;               /**< 0xEB QIOR (3B) / 0xEC QIOR4B */
	uint8_t erase_sector;            /**< 0x20 SE (3B) / 0x21 SE4B (4KB) */
	uint8_t erase_32k;               /**< 0x52 BE32 (3B) / 0x5C BE32-4B */
	uint8_t erase_64k;               /**< 0xD8 BE64 (3B) / 0xDC BE64-4B */
	uint8_t enter_4byte;             /**< 0xB7 enter 4-byte addr mode; 0 = none (use 4B opcodes) */
	uint8_t addr_bytes;              /**< 3 or 4 address bytes */
	uint8_t dual_read_dummy_cycle;   /**< dummy cycles for dual read */
	uint8_t dual_read_dummy_mode;    /**< dummy mode for dual read */
	uint8_t quad_read_dummy_cycle;   /**< dummy cycles for quad read */
	uint8_t quad_read_dummy_mode;    /**< dummy mode for quad read */
	uint8_t quad_read_addr_1wire;    /**< 1 = quad read sends address on 1 wire
	                                  *   (Quad Output Read, e.g. 0x6B/0x6C: no
	                                  *   4-wire mode byte); 0 = 4 wires (Quad I/O
	                                  *   Read, e.g. 0xEB/0xEC). Use 1-wire for
	                                  *   4-byte-address parts where the controller
	                                  *   quad-I/O address framing misbehaves. */
	uint8_t read_force_single;       /**< 1 = route all reads (incl. the quad-read
	                                  *   entry point) through the single-line read
	                                  *   opcode. The BK7259 QSPI controller's quad
	                                  *   read is unreliable with 4-byte addressing
	                                  *   (data corruption on multi-chunk sector
	                                  *   reads), so 4-byte parts read single-line.
	                                  *   Writes still use the quad page program. */
} qspi_nor_cmd_t;

/* Status-register layout: which SR carries QE, and the protection bits.
 *
 * qe_sr_index selects the status register that carries the QE bit:
 *   0 = SR1 (S0-S7), 1 = SR2 (S8-S15), 2 = SR3 (S16-S23).
 * The engine reads/writes that SR via the matching read_srN/write_srN opcode.
 */
typedef struct {
	uint8_t qe_sr_index;        /**< SR index holding QE (0/1/2) */
	uint8_t qe_bit;             /**< QE bit mask within that SR, e.g. BIT(1) */
	uint8_t srp0_bit;           /**< SRP0 mask in SR1, e.g. BIT(7) */
	uint8_t srp1_bit;           /**< SRP1 mask in SR2, e.g. BIT(0) */
	uint8_t sr1_preserve_mask;  /**< SR1 bits to keep while clearing BP, e.g. 0x83 */
	uint8_t bp_shift;           /**< shift of the BP field in SR1, e.g. 2 */
	uint8_t bp_mask;            /**< BP field mask after shift, e.g. 0x07 */
} qspi_nor_reg_t;

/* One supported SPI-NOR part. Selected at init by matching Read ID (9Fh). */
typedef struct {
	const char *name;
	uint8_t id[QSPI_NOR_ID_MAX_LEN];
	uint8_t id_len;
	qspi_nor_geometry_t geometry;
	qspi_nor_cmd_t cmd;
	qspi_nor_reg_t reg;
} qspi_nor_dev_t;

/* Linker-section entry emitted by ::QSPI_NOR_DEVICE_SECTION. Only a fixed-size
 * pointer is placed in the section so iteration and alignment stay simple. */
typedef struct {
	const qspi_nor_dev_t *dev;
} qspi_nor_device_entry_t;

#define _QSPI_NOR_STRINGIFY(x)           #x
#define _QSPI_NOR_SECTION_ATTR(sec, uid) \
	__attribute__((used, section(sec "." _QSPI_NOR_STRINGIFY(uid))))

#define _QSPI_NOR_DEVICE_SECTION_IMPL(dev_sym, uid)                                \
	static _QSPI_NOR_SECTION_ATTR(".qspi_nor_device_list", uid)                     \
	const qspi_nor_device_entry_t qspi_nor_entry_##dev_sym = {                       \
		.dev = &(dev_sym),                                                          \
	}

/**
 * @brief Register a SPI-NOR descriptor into the linker-section registry.
 *
 * Drops a ::qspi_nor_device_entry_t pointing at @p dev_sym into the
 * ``.qspi_nor_device_list`` section, so the driver can discover it at init
 * without any central registration function.
 *
 * @param dev_sym  A ::qspi_nor_dev_t object, e.g. ``qspi_nor_gd25_dev``.
 */
#define QSPI_NOR_DEVICE_SECTION(dev_sym)                                           \
	_QSPI_NOR_DEVICE_SECTION_IMPL(dev_sym, __COUNTER__)

extern qspi_nor_device_entry_t __qspi_nor_device_array_start;
extern qspi_nor_device_entry_t __qspi_nor_device_array_end;

/**
 * @brief Get the descriptor selected for @p id (probed at init).
 *
 * @return the active descriptor, or a built-in default when @p id was not
 *         identified. Never returns NULL for a valid id.
 */
const qspi_nor_dev_t *bk_qspi_nor_dev(qspi_id_t id);

/**
 * @brief Total device capacity in bytes for @p id (from the active descriptor).
 */
uint32_t bk_qspi_flash_nor_total_size(qspi_id_t id);

#ifdef __cplusplus
}
#endif
