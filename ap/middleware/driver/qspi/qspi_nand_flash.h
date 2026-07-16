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

#ifdef __cplusplus
extern "C" {
#endif

#define QSPI_CMD1_LEN              8
/* QSPI max configurable SCK is 80MHz */
#define QSPI_FLASH_MAX_SCK_HZ      80000000
/* ZB35Q01CYIG 9Fh returns 2 valid ID bytes after dummy (Table 3-1) */
#define FLASH_READ_ID_SIZE         2
#define FLASH_PAGE_MASK            (NAND_PAGE_SIZE_BYTES - 1)
#define FLASH_SECTOR_MASK          (NAND_BLOCK_SIZE_BYTES - 1)

/* ZB35Q01CYIG Reset command (Table 3-2) */
#define NAND_CMD_RESET               0xFF
#define NAND_CMD_WRITE_ENABLE        0x06
#define NAND_CMD_GET_FEATURE         0x0F
#define NAND_CMD_SET_FEATURE         0x1F
#define NAND_CMD_BLOCK_ERASE         0xD8
#define NAND_CMD_PROGRAM_LOAD        0x02
#define NAND_CMD_PROGRAM_LOAD_RANDOM 0x84
#define NAND_CMD_PROGRAM_EXECUTE     0x10
#define NAND_CMD_PAGE_READ           0x13
#define NAND_CMD_READ_FROM_CACHE     0x03
#define NAND_CMD_READ_FROM_CACHE_X2  0x3B
#define NAND_CMD_READ_FROM_CACHE_X4  0x6B
#define NAND_CMD_READ_FROM_CACHE_QUAD 0xEB
#define NAND_CMD_PRORAM_LOAD_QUAD        0x32
#define NAND_CMD_PRORAM_LOAD_RANDOM_QUAD 0x34

#define NAND_FEATURE_ADDR_BLOCK_LOCK 0xA0
#define NAND_FEATURE_ADDR_DRIVE      0xB0
#define NAND_FEATURE_ADDR_STATUS     0xC0

/* Command phase field widths */
#define NAND_ADDR_LEN_FEATURE        1    // 1-byte feature/ID address
#define NAND_ADDR_LEN_COLUMN         2    // 2-byte in-page column address
#define NAND_ADDR_LEN_ROW            3    // 3-byte page/block row address
#define NAND_FEATURE_DATA_LEN        1    // feature register is 1 byte
#define NAND_READ_DUMMY_CYCLE        8    // read-from-cache dummy cycles
#define NAND_READ_DUMMY_MODE         3    // read-from-cache dummy mode

#define NAND_PAGE_SIZE_BYTES         2048U
#define NAND_SPARE_SIZE_BYTES        64U
#define NAND_PAGE_PLUS_SPARE_BYTES   (NAND_PAGE_SIZE_BYTES + NAND_SPARE_SIZE_BYTES)
#define NAND_BLOCK_PAGE_COUNT        64U
#define NAND_BLOCK_SIZE_BYTES        (NAND_PAGE_SIZE_BYTES * NAND_BLOCK_PAGE_COUNT)

/* Factory bad-block marker: manufacturer writes a non-FFh value into the first
 * spare byte of page 0 (and page 1) of a bad block, with on-die ECC disabled.
 * Confirm the exact column/page against the ZB35Q01CYIG datasheet before relying
 * on a single page. */
#define NAND_BAD_MARKER_COLUMN       NAND_PAGE_SIZE_BYTES
#define NAND_BAD_MARKER_GOOD         0xFFU
#ifdef CONFIG_QSPI_NAND_FLASH_SIZE
#define NAND_DEVICE_TOTAL_SIZE       (CONFIG_QSPI_NAND_FLASH_SIZE)
#else
#define NAND_DEVICE_TOTAL_SIZE       (128U * 1024U * 1024U)
#endif
#define NAND_TOTAL_PAGE_COUNT        (NAND_DEVICE_TOTAL_SIZE / NAND_PAGE_SIZE_BYTES)

#define NAND_STATUS_OIP              BIT(0)
#define NAND_STATUS_WEL              BIT(1)
#define NAND_STATUS_E_FAIL           BIT(2)
#define NAND_STATUS_P_FAIL           BIT(3)

/* C0h status register ECC status field (ECCS1:ECCS0).
 * 00 = no error, 01 = corrected, 10 = uncorrectable, 11 = corrected w/ rewrite.
 * Confirm the field position against the ZB35Q01CYIG datasheet. */
#define NAND_STATUS_ECC_MASK         (BIT(4) | BIT(5))
#define NAND_STATUS_ECC_POS          4
#define NAND_ECC_NO_ERROR            0x0U
#define NAND_ECC_CORRECTED           0x1U
#define NAND_ECC_UNCORRECTABLE       0x2U

#define NAND_DEFAULT_TIMEOUT_MS      100U
#define NAND_ERASE_TIMEOUT_MS        3000U

/* ZB35Q01CYIG JEDEC ID (Table 3-1) */
#define NAND_JEDEC_MFG_ID_ZBIT       0x5E
#define NAND_JEDEC_DEV_ID_ZB35Q01    0xC1

/* A0h Protection Register bits */
#define NAND_PROT_WP_E_BIT           BIT(1)
#define NAND_BLOCK_LOCK_NONE         0x00

/* B0h Configuration Register: ECC-E defaults to 1 at power-up */
#define NAND_CFG_ECC_E_BIT           BIT(4)
#define NAND_CFG_QE_BIT              BIT(0)

#ifdef __cplusplus
}
#endif
