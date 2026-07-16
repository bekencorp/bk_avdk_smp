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

#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <driver/qspi_types.h>
#include <driver/qspi_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NAND FTL block device.
 *
 * A thin 512-byte logical-sector block device on top of the Dhara flash
 * translation layer (page-mapped log, wear levelling, power-fail-safe recovery,
 * bad-block handling). It is the shared backing for FatFS-on-NAND and the USB
 * MSC device LUN, so a PC sees a normal removable FAT volume.
 *
 * Dhara maps in units of a NAND page (2048B); this layer read-modify-writes the
 * page to present 512B sectors for host compatibility.
 */

#define BK_NAND_FTL_SECTOR_SIZE   512U

/* Bring up the FTL on the given QSPI id: init the QSPI NAND driver, attach the
 * Dhara map to the configured partition and resume any persisted state. Safe to
 * call more than once (subsequent calls are no-ops). */
bk_err_t bk_nand_ftl_init(qspi_id_t id);

/* Whether the FTL for this id has been initialised. */
bool bk_nand_ftl_is_inited(qspi_id_t id);

/* Wipe the FTL: clear the map and persist the empty state. The caller must
 * ensure no filesystem is mounted on top. */
bk_err_t bk_nand_ftl_format(qspi_id_t id);

/* Logical sector size (always BK_NAND_FTL_SECTOR_SIZE). */
uint32_t bk_nand_ftl_sector_size(qspi_id_t id);

/* Number of addressable 512B logical sectors. */
uint32_t bk_nand_ftl_sector_count(qspi_id_t id);

/* Read `count` logical sectors starting at `sector` into `buf`. */
bk_err_t bk_nand_ftl_read(qspi_id_t id, uint32_t sector, uint8_t *buf, uint32_t count);

/* Write `count` logical sectors starting at `sector` from `buf`. */
bk_err_t bk_nand_ftl_write(qspi_id_t id, uint32_t sector, const uint8_t *buf, uint32_t count);

/* Flush all pending changes so they are persistent and durable. */
bk_err_t bk_nand_ftl_sync(qspi_id_t id);

/* Test hook: force a logical block bad and let Dhara relocate its contents. */
bk_err_t bk_nand_ftl_inject_bad(qspi_id_t id, uint32_t block);

/* Print FTL geometry / capacity / bad-block stats to the log. */
void bk_nand_ftl_dump(qspi_id_t id);

/* Scan the FTL partition's physical blocks for *factory* bad-block markers
 * (independent of the runtime dhara cache) and log each bad block plus totals.
 * Useful as a baseline to tell factory bad blocks apart from ones injected or
 * retired at runtime. */
void bk_nand_ftl_scan_factory_bad(qspi_id_t id);

#ifdef __cplusplus
}
#endif
