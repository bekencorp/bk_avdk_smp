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

#include <stdio.h>
#include "platform_irq.h"
#include "STAR_SE.h"
#include "core_star.h"
#include "sdkconfig.h"
#include "hal_hw_fih.h"
#include "partitions.h"

#define MPU_MAX_NUM_REGIONS     (8UL)                   /* max number of regions supported */
#define MPU_MAX_NUM_ATTRS       (8UL)                   /* max number of memory attributes supported */

/*
* #define ARM_MPU_RBAR(BASE, SH, RO, NP, XN)
*
* \brief Region Base Address Register value
* \param BASE The base address bits [31:5] of a memory region. The value is zero extended. Effective address gets 32 byte aligned.
* \param SH Defines the Shareability domain for this memory region.
* \param RO Read-Only: Set to 1 for a read-only memory region.
* \param NP Non-Privileged: Set to 1 for a non-privileged memory region.
* \oaram XN eXecute Never: Set to 1 for a non-executable memory region.
*/
/*
 * BK7259 BL2 (secure-only bootloader, CONFIG_SPE, SOC_ADDR_OFFSET=0) MPU map.
 *
 * BL2 runs entirely in the Secure world from the secure address aliases:
 *   - .text          @ 0x04004000  (Flash secure alias 0x04xxxxxx, XIP execute)
 *   - .data/.bss/
 *     .stack/.heap   @ 0x28000xxx  (SMEM secure alias 0x28xxxxxx, 512KB)
 *   - .iram/.vectors @ 0x28030000  (SMEM secure alias, executed from RAM)
 *   - M55 DTCM       @ 0x28200000  (enabled in SystemInit, XN)
 *   - Dubhe TE200    @ 0x42110000  (crypto engine, in the 0x40-0x5F device band)
 *   - flash-ctrl / UART1 / SYS / AON-PMU / PPRO / OTP  (0x44xxxxxx, device band)
 *
 * BK7259 differs from BK7236: there is no 0x00/0x02 ITCM/DTCM code region and no
 * 0x02000000 flash mirror; flash lives at 0x04xxxxxx and SMEM is a bounded
 * 0x28000000..0x280A0000 window (not the whole 0x28-0x3F range). Attribute
 * indices reference mpu_attrs[] below.
 */
static const ARM_MPU_Region_t mpu_regions[] = {
    /* region 0: Flash (secure 0x04xxxxxx) - BL2 code XIP + reading images to
     * verify/decrypt. Executable, cacheable write-through read-allocate (attr 4). */
    { ARM_MPU_RBAR(0x04000000UL, ARM_MPU_SH_NON, 1, 1, 0),
      ARM_MPU_RLAR((0x04000000UL + CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET - 0x10), 4)},

#if CONFIG_OTA_OVERWRITE
    /* region 1 (CONFIG_OTA_OVERWRITE only; XIP omits): RW+X primary flash after BL2,
     * attr 4 (WT-RA). Encrypted overwrite must use WT: bk_flash_write_cbus stores
     * plaintext via 0x04, HW XTS encrypts on write, and WT keeps stores ordered into
     * the cpu-data-write FIFO; WB lets L2 batch into out-of-order bursts so only the
     * first few lines land correctly (see flash_min.c). */
    { ARM_MPU_RBAR((0x04000000UL + CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET), ARM_MPU_SH_NON, 0, 1, 0),
      ARM_MPU_RLAR(0x04FFFFE0UL, 4) },
#endif

    /* region 2: Flash XIP-write window (secure 0x05xxxxxx), non-cacheable (attr 1),
     * execute-never - used by the packer/XIP-remap path. */
    { ARM_MPU_RBAR(0x05000000UL, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x05FFFFE0UL, 1) },

    /* region 3: BL2 RAM (0x28000000..0x2807FFFF, TOTAL_RAM_SIZE=512KB per
     * flash_layout.h / bk7259_bl2.ld). .data/.bss/.stack/.heap + IRAM copy
     * (0x28030000..0x28039FFF). RW + executable for IRAM.
     * [DIAG] attr 1 (non-cacheable), matching the known-good BK7234N BL2 map,
     * to test whether the MPU-enable MemManage is a RAM D-cache interaction. */
    { ARM_MPU_RBAR(0x28000000UL, ARM_MPU_SH_INNER, 0, 1, 0),
      ARM_MPU_RLAR(0x2807FFE0UL, 1) },

    /* region 4: peripherals / crypto engine (0x40000000..0x5FFFFFFF) - Dubhe
     * TE200 @0x42110000, flash controller, UART1, SYS, AON-PMU, PPRO, OTP, etc.
     * Device memory (attr 2), execute-never. */
    { ARM_MPU_RBAR(0x40000000UL, ARM_MPU_SH_INNER, 0, 1, 1),
      ARM_MPU_RLAR(0x5FFFFFE0UL, 2) },

    /* region 5: PSRAM0 (0x60000000..0x63FFFFFF), cacheable write-back (attr 3),
     * execute-never. */
    { ARM_MPU_RBAR(0x60000000UL, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x63FFFFE0UL, 3) },

    /* region 6: PSRAM1 (0x64000000..0x67FFFFFF). */
    { ARM_MPU_RBAR(0x64000000UL, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x67FFFFE0UL, 3) },

    /* region 7: QSPI XIP + high device/PPB (0x68000000..0xEFFFFFFF). Covers
     * SOC_QSPI0/1_DATA_BASE and the fixed PPB (NVIC/SCB/WWDT @0xE00xxxxx, no
     * S/NS alias). Device memory (attr 2), execute-never. */
    { ARM_MPU_RBAR(0x68000000UL, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0xEFFFFFE0UL, 2) }
};

/*
 For the star processor, only two combinations of these attributes are valid:Device-nGnRnE/Device-nGnRE
 please refer to the document:star_user_guide_reference_material.pdf page50
 */
static const uint8_t mpu_attrs[] = {
    ARM_MPU_ATTR(0xb, 0xb), // [0] Normal memory, cacheable write through, read allocate, write allocate
    ARM_MPU_ATTR(0x4, 0x4), // [1] Normal memory, non-cacheable
    ARM_MPU_ATTR(0x0, 0x0), // [2] Device memory, bit[3:4]:nGnRnE-00,nGnRE-01
    ARM_MPU_ATTR(0xf, 0xf), // [3] Normal memory, cacheable write back, read allocate, write allocate
    ARM_MPU_ATTR(0xa, 0xa)  // [4] Normal memory, cacheable write through, read allocate, (RO) no write allocate
};

void mpu_clear(uint32_t rnr)
{
    ARM_MPU_Disable();
    ARM_MPU_ClrRegion(rnr);
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);
}

void mpu_cfg(uint32_t rnr, uint32_t rbar, uint32_t rlar)
{
    ARM_MPU_Disable();
    ARM_MPU_SetRegion(rnr, rbar, rlar);
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);
    SCB_CleanInvalidateDCache();
}

void mpu_enable(void)
{
    const uint32_t region_num = sizeof(mpu_regions)/sizeof(mpu_regions[0]);
    const uint32_t attr_num = sizeof(mpu_attrs)/sizeof(mpu_attrs[0]);
    uint32_t region_count;
    uint32_t attr_count;
    uint32_t verify_count;
    uint32_t pos;
    uint32_t mair_val;

    if (region_num > MPU_MAX_NUM_REGIONS){
        FIH_ASSERT4(0);
        return;
    }

    if (attr_num > MPU_MAX_NUM_ATTRS){
        FIH_ASSERT4(0);
        return;
    }

    ARM_MPU_Disable();

    region_count = region_num;
    for (int i = 0; i < region_count; i++) {
        ARM_MPU_SetRegion(i, mpu_regions[i].RBAR, mpu_regions[i].RLAR);
        MPU->RNR = i;
        __DMB();
        if (MPU->RBAR != mpu_regions[i].RBAR || MPU->RLAR != mpu_regions[i].RLAR) {
            ARM_MPU_Disable();
            FIH_ASSERT4(0);
            return;
        }
    }

    verify_count = 0;
    for (int i = 0; i < region_count; i++) {
        MPU->RNR = i;
        __DMB();
        if (MPU->RBAR == mpu_regions[i].RBAR && MPU->RLAR == mpu_regions[i].RLAR) {
            verify_count++;
        }
    }
    if (verify_count != region_count) {
        ARM_MPU_Disable();
        FIH_ASSERT4(0);
        return;
    }

    attr_count = attr_num;
    for (int j = 0; j < attr_count; j++) {
        ARM_MPU_SetMemAttr(j, mpu_attrs[j]);
    }

    verify_count = 0;
    for (int j = 0; j < attr_count; j++) {
        pos = (j % 4U) * 8U;
        __DMB();
        if (j < 4) {
            mair_val = MPU->MAIR0;
        } else {
            mair_val = MPU->MAIR1;
        }
        __DMB();
        if (((mair_val >> pos) & 0xFF) == mpu_attrs[j]) {
            verify_count++;
        }
    }
    if (verify_count != attr_count) {
        ARM_MPU_Disable();
        FIH_ASSERT4(0);
        return;
    }

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);

    __DMB();
    if ((MPU->CTRL & (MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk | MPU_CTRL_ENABLE_Msk)) !=
        (MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk | MPU_CTRL_ENABLE_Msk)) {
        ARM_MPU_Disable();
        FIH_ASSERT4(0);
        return;
    }

    verify_count = 0;
    for (int i = 0; i < region_count; i++) {
        MPU->RNR = i;
        __DMB();
        if (MPU->RBAR == mpu_regions[i].RBAR && MPU->RLAR == mpu_regions[i].RLAR) {
            verify_count++;
        }
    }
    if (verify_count != region_count) {
        ARM_MPU_Disable();
        FIH_ASSERT4(0);
        return;
    }
}

void mpu_disable(void)
{
    ARM_MPU_Disable();
    for (uint32_t i = 0; i < MPU_MAX_NUM_REGIONS; i++) {
      ARM_MPU_ClrRegion(i);
    }
}

void mpu_dump(void)
{
    BK_DUMP_OUT("******************** Dump mpu config begin ********************\r\n");
    BK_DUMP_OUT("MPU->TYPE: 0x%08X.\r\n", MPU->TYPE);
    BK_DUMP_OUT("MPU->CTRL: 0x%08X.\r\n", MPU->CTRL);
    for (uint32_t i = 0; i < MPU_MAX_NUM_REGIONS; i++) {
        MPU->RNR = i;
        BK_DUMP_OUT("MPU->RNR: %d.\r\n", MPU->RNR);
        BK_DUMP_OUT("MPU->RBAR: 0x%08X.\r\n", MPU->RBAR);
        BK_DUMP_OUT("MPU->RLAR: 0x%08X.\r\n", MPU->RLAR);
        BK_DUMP_OUT("MPU->RBAR_A1: 0x%08X.\r\n", MPU->RBAR_A1);
        BK_DUMP_OUT("MPU->RLAR_A1: 0x%08X.\r\n", MPU->RLAR_A1);
        BK_DUMP_OUT("MPU->RBAR_A2: 0x%08X.\r\n", MPU->RBAR_A2);
        BK_DUMP_OUT("MPU->RLAR_A2: 0x%08X.\r\n", MPU->RLAR_A2);
        BK_DUMP_OUT("MPU->RBAR_A3: 0x%08X.\r\n", MPU->RBAR_A3);
        BK_DUMP_OUT("MPU->RLAR_A3: 0x%08X.\r\n", MPU->RLAR_A3);
        BK_DUMP_OUT("MPU->MAIR0: 0x%08X.\r\n", MPU->MAIR0);
        BK_DUMP_OUT("MPU->MAIR1: 0x%08X.\r\n", MPU->MAIR1);
    }
    BK_DUMP_OUT("******************** Dump mpu config begin ********************\r\n");
}

