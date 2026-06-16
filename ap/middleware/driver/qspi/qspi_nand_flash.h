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
/* ZB35Q01CYIG 9Fh returns 2 valid ID bytes after dummy (Table 3-1) */
#define FLASH_READ_ID_SIZE         2
#define FLASH_PAGE_MASK            (NAND_PAGE_SIZE_BYTES - 1)
#define FLASH_SECTOR_MASK          (NAND_BLOCK_SIZE_BYTES - 1)

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

#define NAND_PAGE_SIZE_BYTES         2048U
#define NAND_SPARE_SIZE_BYTES        64U
#define NAND_BLOCK_PAGE_COUNT        64U
#define NAND_BLOCK_SIZE_BYTES        (NAND_PAGE_SIZE_BYTES * NAND_BLOCK_PAGE_COUNT)
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

#define NAND_DEFAULT_TIMEOUT_MS      100U
#define NAND_ERASE_TIMEOUT_MS        3000U

/* ZB35Q01CYIG JEDEC ID (Table 3-1) */
#define NAND_JEDEC_MFG_ID_ZBIT       0x5E
#define NAND_JEDEC_DEV_ID_ZB35Q01    0xC1

/* A0h Protection Register bits */
#define NAND_PROT_WP_E_BIT           BIT(1)

/* B0h Configuration Register: ECC-E defaults to 1 at power-up */
#define NAND_CFG_ECC_E_BIT           BIT(4)
#define NAND_CFG_QE_BIT              BIT(0)

#ifdef __cplusplus
}
#endif
