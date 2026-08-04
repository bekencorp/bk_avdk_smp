// Copyright 2023-2028 Beken
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

#include "os/os.h"
#include "common/bk_assert.h"
#include "sdkconfig.h"
#include "bk_arch.h"
#include "soc/bk7259/reg_base.h"

/*
* #define ARM_MPU_RBAR(BASE, SH, RO, NP, XN)
*
* \brief Region Base Address Register value
* \param BASE The base address bits [31:5] of a memory region. The value is zero extended bits [4:0]. Effective address gets 32 byte aligned.
* \param SH Defines the Shareability domain for this memory region.
* \param RO Read-Only: Set to 1 for a read-only memory region.
* \param NP Non-Privileged: Set to 1 for a non-privileged memory region.
* \param XN eXecute Never: Set to 1 for a non-executable memory region.
*/
/*
* #define ARM_MPU_RLAR(LIMIT, IDX)
*
* \brief Region Limit Address Register value
* \param LIMIT The limit address bits [31:5] for this memory region. The value is one extended bits [4:0].
* \param IDX The attribute index to be associated with this memory region.
*/

#define NS_MEM_OFFSET  SOC_S_NS_ADDR_DIFF

ARM_MPU_Region_t mpu_regions[] = {
    /* MPU region 1, RO-code/RO-data.
    	Flash_ns: 0x1400 0000-----------0x14FF FFFF
     */
    { ARM_MPU_RBAR(0x04000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 1, 1, 0),
      ARM_MPU_RLAR(0x04FFFFE0UL + NS_MEM_OFFSET, 4) },

    { ARM_MPU_RBAR(0x05000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 0),
      ARM_MPU_RLAR(0x05FFFFE0UL + NS_MEM_OFFSET, 1) },

    /* MPU region 2
    	iram_ns: 0x1800 0000-----------0x1805 FFFF
     */
    { ARM_MPU_RBAR(0x08000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 1, 1, 0),
      ARM_MPU_RLAR(0x0805FFE0UL + NS_MEM_OFFSET, 1) },

    /* MPU region 3
        shared memory(smem0) 0x3800 0000-----------0x3801 FFFF
        shared memory(smem1) 0x3802 0000-----------0x3803 FFFF
        shared memory(smem2) 0x3804 0000-----------0x3805 FFFF
        shared memory(smem3) 0x3810 0000-----------0x3813 FFFF
        shared memory(smem4) 0x3814 0000-----------0x3817 FFFF
        shared memory(smem5) 0x3818 0000-----------0x381B FFFF
        shared memory(smem6) 0x381C 0000-----------0x381D FFFF
     */
    { ARM_MPU_RBAR(0x28000000UL + NS_MEM_OFFSET, ARM_MPU_SH_INNER, 0, 1, 0),
      ARM_MPU_RLAR(0x2805FFE0UL + NS_MEM_OFFSET, 1) },
    { ARM_MPU_RBAR(0x2C000000UL + NS_MEM_OFFSET, ARM_MPU_SH_INNER, 0, 1, 0),
      ARM_MPU_RLAR(0x2C05FFE0UL + NS_MEM_OFFSET, 1) },
    { ARM_MPU_RBAR(0x28100000UL + NS_MEM_OFFSET, ARM_MPU_SH_INNER, 0, 1, 0),
      ARM_MPU_RLAR(0x281DFFE0UL + NS_MEM_OFFSET, 1) },
    { ARM_MPU_RBAR(0x2C100000UL + NS_MEM_OFFSET, ARM_MPU_SH_INNER, 0, 1, 0),
      ARM_MPU_RLAR(0x2C1DFFE0UL + NS_MEM_OFFSET, 1) },

   /* MPU region
    	m55_dtcm_s:  0x2820 0000-----------0x2820 FFFF
    	m55_dtcm_ns: 0x3820 0000-----------0x3820 FFFF
     */
    { ARM_MPU_RBAR(0x28200000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x2820FFE0UL + NS_MEM_OFFSET, 1) },
    /* MPU region 5 periphral, device memory
        device memory is shareable, and must not be cached.
        please refer to the document:star_user_guide_reference_material.pdf page50
	 */
    { ARM_MPU_RBAR(0x40000000UL + NS_MEM_OFFSET, ARM_MPU_SH_INNER, 0, 1, 1),
      ARM_MPU_RLAR(0x5FFFFFE0UL + NS_MEM_OFFSET, 2) },

    /* MPU region 6 psram0 */
    { ARM_MPU_RBAR(0x60000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x63FFFFE0UL + NS_MEM_OFFSET, 1) },

    /* MPU region 7 psram1 */
    { ARM_MPU_RBAR(0x64000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0x67FFFFE0UL + NS_MEM_OFFSET, 1) },

     /* MPU region 8 qspi1 */
     { ARM_MPU_RBAR(0x68000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
       ARM_MPU_RLAR(0x6FFFFFE0UL + NS_MEM_OFFSET, 3) },

#if CONFIG_PSRAM_INTERLEAVE
     /* MPU region 10a interleaved psram (0x90000000~0x97FFFFE0) */
     { ARM_MPU_RBAR(0x80000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
        ARM_MPU_RLAR(0x87FFFFE0UL + NS_MEM_OFFSET, 1) },

     /* MPU region 11b ppb and other (0x88000000~0xEFFFFFE0) - device memory */
     { ARM_MPU_RBAR(0x88000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
       ARM_MPU_RLAR(0xEFFFFFE0UL, 2) }
  #else
     /* MPU region 10 ppb and other */
     { ARM_MPU_RBAR(0x80000000UL + NS_MEM_OFFSET, ARM_MPU_SH_NON, 0, 1, 1),
      ARM_MPU_RLAR(0xEFFFFFE0UL, 2) }
 #endif
};

_Static_assert(sizeof(mpu_regions) / sizeof(mpu_regions[0]) <= 16,
               "mpu_regions exceeds the 16 hardware MPU regions (extra ones are silently dropped)");

/*
 For the star processor, only two combinations of these attributes are valid:Device-nGnRnE/Device-nGnRE
 please refer to the document:star_user_guide_reference_material.pdf page50
 */
 /*
 For BK7259, when configuring the regions in the MAIR(Memory Attribute Indirection Registers),
 the L2 Cache corresponds to the Outer region, and the L1 Cache corresponds to the Inner region.
 */
uint8_t mpu_attrs[] = {
    ARM_MPU_ATTR(0xb, 0xb), // Normal memory, cacheable write through, read allocate, write allocate
    ARM_MPU_ATTR(0x4, 0x4), // Normal memory, non-cacheable
    ARM_MPU_ATTR(0x0, 0x0), // Device memory, bit[3:4]:nGnRnE-00,nGnRE-01
    ARM_MPU_ATTR(0xf, 0xf), // Normal memory, cacheable write back, read allocate, write allocate
    ARM_MPU_ATTR(0xa, 0xa), // Normal memory, cacheable write through, read allocate, (RO) no WA.
    ARM_MPU_ATTR(0xb, 0x4), // L2 Cache, cacheable write back, L1 Cache: non-cacheable.
};

void mpu_register_regions(ARM_MPU_Region_t *regions, uint32_t region_cnt);
void mpu_register_attrs(uint8_t *attrs, uint32_t cnt);

void soc_mpu_cfg(void)
{
	mpu_register_regions(mpu_regions, sizeof(mpu_regions)/sizeof(mpu_regions[0]));
	mpu_register_attrs(mpu_attrs, sizeof(mpu_attrs)/sizeof(mpu_attrs[0]));
}
// eof
