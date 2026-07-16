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
#include <driver/hal/hal_qspi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief QSPI NAND bad-block management (BBM) translation layer.
 *
 * Sits between the QSPI NAND driver and the filesystem. It scans factory bad
 * blocks, maintains a persistent bad-block table (BBT) plus a logical->physical
 * remap onto a reserved spare-block pool, and exposes a contiguous "good block"
 * logical address space. Chip-agnostic and filesystem-agnostic: it only calls
 * the bk_qspi_flash_* driver APIs.
 */

/**
 * @brief  Initialize the BBM for a device (idempotent per id).
 *
 * Loads the persisted BBT if present and valid, otherwise scans factory bad
 * blocks, builds the remap and persists a fresh BBT.
 *
 * @return BK_OK on success, otherwise an error code.
 */
bk_err_t bk_qspi_nand_bbm_init(qspi_id_t id);

/**
 * @brief  Size in bytes of the good-block logical address space exposed upward.
 *
 * @return logical size in bytes, or 0 if not initialized.
 */
uint32_t bk_qspi_nand_bbm_logical_size(qspi_id_t id);

/**
 * @brief  Read from the logical (good-block) address space.
 *
 * @param log_addr logical byte address
 * @param buf      destination buffer
 * @param len      number of bytes
 * @return BK_OK on success; BK_ERR_QSPI_NAND_ECC_FAIL on uncorrectable ECC.
 */
bk_err_t bk_qspi_nand_bbm_read(qspi_id_t id, uint32_t log_addr, void *buf, uint32_t len);

/**
 * @brief  Program the logical (good-block) address space.
 *
 * On a program failure the underlying physical block is retired and the logical
 * block is remapped to a spare; the original error is still returned so the
 * upper filesystem can relocate.
 *
 * @return BK_OK on success, otherwise an error code.
 */
bk_err_t bk_qspi_nand_bbm_prog(qspi_id_t id, uint32_t log_addr, const void *buf, uint32_t len);

/**
 * @brief  Erase one or more logical blocks (addr/size must be block-aligned).
 *
 * On an erase failure the physical block is retired and the logical block is
 * remapped to a spare, then the erase is retried transparently.
 *
 * @return BK_OK on success, otherwise an error code.
 */
bk_err_t bk_qspi_nand_bbm_erase(qspi_id_t id, uint32_t log_addr, uint32_t size);

/**
 * @brief  Dump the current BBM state (layout, sequence, bad blocks, remaps).
 *
 * Intended for board bring-up / verification via CLI.
 */
void bk_qspi_nand_bbm_dump(qspi_id_t id);

/**
 * @brief  Test hook: force-retire the physical block backing a logical block.
 *
 * Marks the backing physical block bad, remaps the logical block to a spare and
 * persists the BBT - the software equivalent of a runtime bad-block event, so
 * remap/persistence can be exercised without a real program/erase failure.
 *
 * @param log_block logical block index to retire
 * @return BK_OK on success, otherwise an error code.
 */
bk_err_t bk_qspi_nand_bbm_inject_bad(qspi_id_t id, uint32_t log_block);

#ifdef __cplusplus
}
#endif
