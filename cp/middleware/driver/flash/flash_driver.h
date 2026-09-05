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

#include <components/log.h>
#include <driver/flash_types.h>
#include "flash_core_config.h"
#include "bk_flash_core.h"
#include "bk_flash_port.h"

#define FLASH_TAG "flash"
#define FLASH_LOGI(...) BK_LOGI(FLASH_TAG, ##__VA_ARGS__)
#define FLASH_LOGW(...) BK_LOGW(FLASH_TAG, ##__VA_ARGS__)
#define FLASH_LOGE(...) BK_LOGE(FLASH_TAG, ##__VA_ARGS__)
#define FLASH_LOGD(...) BK_LOGD(FLASH_TAG, ##__VA_ARGS__)
#define FLASH_LOGV(...) BK_LOGV(FLASH_TAG, ##__VA_ARGS__)

/* FLASH_SIZE_* and flash_config_t now live in the shared flash_core_config.h. */
#define FLASH_STATUS_REG_PROTECT_MASK    0xff
#define FLASH_STATUS_REG_PROTECT_OFFSET  8
#define FLASH_CMP_MASK                   0x1
#define FLASH_BUFFER_LEN                 8
#define FLASH_BYTES_CNT                  32
#define FLASH_ADDRESS_MASK               0x1f
#define FLASH_ERASE_SECTOR_MASK          0xFFF000
#define FLASH_ERASE_BLOCK_MASK           0xFF0000
#define FLASH_SECTOR_SIZE_MASK           0x000FFF
#define FLASH_SECTOR_SIZE                0x1000 /* each sector has 4K bytes */
#define FLASH_SECTOR_SIZE_OFFSET         12
#define FLASH_PAGE_SIZE                  256 /* each page has 256 bytes */
#define FLASH_DPLL_DIV_VALUE_TEN         3
#define FLASH_DPLL_DIV_VALUE_SIX         1
#define FLASH_ManuFacID_POSI             (16)
#define FLASH_ManuFacID_GD               (0xC8)
#define FLASH_ManuFacID_TH               (0xCD)

#define FLASH_BLOCK32_SIZE               (0x8000)
#define FLASH_BLOCK_SIZE                 (0x10000)
#define FLASH_MAX_SIZE                   (FLASH_SIZE_16M)
#define FLASH_API_MAGIC_CODE             (0x12345678)

/* Test-only helpers to toggle flash protection for CLI/verify. */
void test_flash_set_protect_type_none(void);
void test_flash_set_protect_type_all(void);
