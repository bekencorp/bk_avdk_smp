// Copyright     2023-2028 Beken
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

#include "flash_layout.h"
/* armino_config.h provides CONFIG_TFM_RAM_SIZE generated from the project RAM
 * regions. It drives S_DATA_SIZE and the Non-Secure RAM window. */
#include "armino_config.h"

#define BL2_HEAP_SIZE           (0x0004000)
#define BL2_MSP_STACK_SIZE      (0x0004800)

#define S_HEAP_SIZE             0x6000
#define S_MSP_STACK_SIZE_INIT   (0x0000400)
#define S_MSP_STACK_SIZE        (0x0000800)
#define S_PSP_STACK_SIZE        (0x0000800)

#define NS_HEAP_SIZE            (0x0001000)
#define NS_MSP_STACK_SIZE       (0x0000800)
#define NS_PSP_STACK_SIZE       (0x0004000)

/* This size of buffer is big enough to store an attestation
 * token produced by initial attestation service
 */
#define PSA_INITIAL_ATTEST_TOKEN_MAX_SIZE   (0x250)

#define S_IMAGE_PRIMARY_PARTITION_OFFSET   CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET
#define S_IMAGE_SECONDARY_PARTITION_OFFSET CONFIG_SECONDARY_TFM_S_PHY_PARTITION_OFFSET
#define NS_IMAGE_PRIMARY_PARTITION_OFFSET  CONFIG_PRIMARY_TFM_NS_PHY_PARTITION_OFFSET

/* Alias definitions for secure and non-secure areas*/
#define S_ROM_ALIAS(x)  (S_ROM_ALIAS_BASE + (x))
#define NS_ROM_ALIAS(x) (NS_ROM_ALIAS_BASE + (x))

#define S_RAM_ALIAS(x)  (S_RAM_ALIAS_BASE + (x))
#define NS_RAM_ALIAS(x) (NS_RAM_ALIAS_BASE + (x))

#define SECUREBOOT_ENABLE 1
#if SECUREBOOT_ENABLE
/* Secure regions */
#define S_CODE_START    S_ROM_ALIAS(CONFIG_PRIMARY_TFM_S_VIRTUAL_CODE_START)
#define S_CODE_SIZE     CONFIG_PRIMARY_TFM_S_VIRTUAL_CODE_SIZE
#else
#define S_CODE_START    0x02000000
#define S_CODE_SIZE     0x80000
#endif
#define S_CODE_LIMIT    (S_CODE_START + S_CODE_SIZE - 1)

/* Size of vector table: 139 interrupt handlers + 4 bytes MPS initial value */
#define S_CODE_VECTOR_TABLE_SIZE    (0x230)

#define S_DATA_START    (S_RAM_ALIAS(0x0))

#if CONFIG_TFM_S_JUMP_TO_CPU0_APP
#define S_DATA_SIZE     CONFIG_TFM_RAM_SIZE
#else
#define S_DATA_SIZE     0x40000
#endif
#define S_DATA_LIMIT    (S_DATA_START + S_DATA_SIZE - 1)

#if SECUREBOOT_ENABLE
/* Non-secure regions */
#if CONFIG_TFM_S_JUMP_TO_TFM_NS
#define NS_CODE_START   NS_ROM_ALIAS(CONFIG_PRIMARY_TFM_NS_VIRTUAL_CODE_START)
#define NS_CODE_SIZE    CONFIG_PRIMARY_TFM_NS_VIRTUAL_CODE_SIZE
#else
#define NS_CODE_START   NS_ROM_ALIAS(CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_START)
#define NS_CODE_SIZE    CONFIG_PRIMARY_CPU0_APP_VIRTUAL_CODE_SIZE
#endif
#define NS_CODE_LIMIT   (NS_CODE_START + NS_CODE_SIZE - 1)
#else
#define NS_CODE_START   0x12080000
#define NS_CODE_SIZE    0x40000
#define NS_CODE_LIMIT   (NS_CODE_START + NS_CODE_SIZE - 1)
#endif

/*
 * BK7259 SMEM address-alias rule (from BK7259v2 User Guide):
 *   0x28xxxxxx  - SMEM, SECURE
 *   0x2Cxxxxxx  - SMEM, SECURE (CPU-only "direct" alias; the bus auto-translates
 *                 a 0x2C access back to 0x28)
 *   0x38xxxxxx  - SMEM, NON-SECURE (NS view of 0x28)
 *   0x3Cxxxxxx  - SMEM, NON-SECURE (NS view of 0x2C)
 * So the Non-Secure app MUST place its RAM/data in the 0x38/0x3C NS aliases; a
 * 0x28/0x2C access from the NS world SecureFaults.
 *
 * The NS app (cpu0_app) is built with CONFIG_SRAM_DIRECT_ADDR, so its RAM uses
 * the "direct" alias. We switch that alias to the Non-Secure form 0x3Cxxxxxx
 * (see bk7259_bsp.ld __CP_APP_RAM_OFFSET for the secure-boot case). Its RAM
 * starts immediately after the configured secure CP RAM carve-out.
 * Mark exactly that window Non-Secure in the SAU so NS data accesses are
 * allowed while the secure world retains the prefix. CONFIG_TFM_RAM_SIZE
 * drives the RAM MPC split and the NS app base.
 */

/* NS "direct" SMEM alias base: 0x3C = the Non-Secure (0x38) view of the secure
 * CPU-direct 0x2C alias (0x2C = 0x28 + SMEM_DIRECT_ALIAS_OFFSET). cpu0_app runs
 * its RAM here because it is built with CONFIG_SRAM_DIRECT_ADDR. */
#define SMEM_DIRECT_ALIAS_OFFSET   (0x04000000)
#define NS_RAM_DIRECT_ALIAS_BASE   (NS_RAM_ALIAS_BASE + SMEM_DIRECT_ALIAS_OFFSET)   /* 0x3C000000 */

/* End of the CP RAM window (physical SMEM offset). The CP swap area starts here
 * (bk7259_bsp.ld __CP_SWAP_BASE = 0x2805F000), so the NS RAM must stay below. */
#define CP_RAM_WINDOW_END          (0x5F000)

/* SPE (secure) owns physical [0, CONFIG_TFM_RAM_SIZE); the NS app RAM is the
 * remainder of the CP RAM window, seen through the NS direct alias. */
#define NS_DATA_START   (NS_RAM_DIRECT_ALIAS_BASE + CONFIG_TFM_RAM_SIZE)   /* 0x3C014000 */
#define NS_DATA_SIZE    (CP_RAM_WINDOW_END - CONFIG_TFM_RAM_SIZE)

#define NS_DATA_LIMIT   (NS_DATA_START + NS_DATA_SIZE - 1)

/* NS partition information is used for MPC and SAU configuration */
#define NS_PARTITION_START NS_CODE_START
#define NS_PARTITION_SIZE (FLASH_NS_PARTITION_SIZE)

/* Secondary partition for new images in case of firmware upgrade */
#define SECONDARY_PARTITION_START \
            (NS_ROM_ALIAS(S_IMAGE_SECONDARY_PARTITION_OFFSET))
#define SECONDARY_PARTITION_SIZE (FLASH_S_PARTITION_SIZE + \
                                  FLASH_NS_PARTITION_SIZE)

#ifdef BL2
/* Bootloader regions */
#define BL2_CODE_START    (0x04000000 + CONFIG_BL2_VIRTUAL_CODE_START)
#define BL2_CODE_SIZE     CONFIG_BL2_VIRTUAL_CODE_SIZE
#define BL2_CODE_LIMIT    (BL2_CODE_START + BL2_CODE_SIZE - 1)

#define BL2_DATA_START    (S_RAM_ALIAS(0x0))
#define BL2_DATA_SIZE     (TOTAL_RAM_SIZE)
#define BL2_DATA_LIMIT    (BL2_DATA_START + BL2_DATA_SIZE - 1)
#endif /* BL2 */



/* Shared symbol area between bootloader and runtime firmware. Global variables
 * in the shared code can be placed here.
 */
#ifdef CODE_SHARING
#define SHARED_SYMBOL_AREA_BASE S_RAM_ALIAS_BASE
#define SHARED_SYMBOL_AREA_SIZE 0x20
#else
#define SHARED_SYMBOL_AREA_BASE S_RAM_ALIAS_BASE
#define SHARED_SYMBOL_AREA_SIZE 0x0
#endif /* CODE_SHARING */

/* Shared data area between bootloader and runtime firmware.
 * These areas are allocated at the beginning of the RAM, it is overlapping
 * with TF-M Secure code's MSP stack
 */
#define BOOT_TFM_SHARED_DATA_BASE (SHARED_SYMBOL_AREA_BASE + \
                                   SHARED_SYMBOL_AREA_SIZE)
#define BOOT_TFM_SHARED_DATA_SIZE (0x400)       /* May be reduced during future RAM optimization. */
#define BOOT_TFM_SHARED_DATA_LIMIT (BOOT_TFM_SHARED_DATA_BASE + \
                                    BOOT_TFM_SHARED_DATA_SIZE - 1)

/* Fixed BL2/TF-M fastboot ABI: directly after the shared-data area. */
#define BL2_DS_RETENTION_ADDR (BOOT_TFM_SHARED_DATA_LIMIT + 1)
#define BL2_DS_RETENTION_SIZE (0x20)
#define TFM_SLEEP_CONTEXT_ADDR (BL2_DS_RETENTION_ADDR + BL2_DS_RETENTION_SIZE)
#define TFM_SLEEP_CONTEXT_MAX_SIZE (0x400)

#define ENABLE_HEAP 1
