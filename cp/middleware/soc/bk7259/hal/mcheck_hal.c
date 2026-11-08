// Copyright 2020-2025 Beken
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

#include <common/bk_include.h>
#include <components/log.h>
#include "hal_config.h"
#include "reg_base.h"
#if (!CONFIG_ATE_TEST)
static int mcheck_section(uint32_t* reg_addr, uint32_t base_addr, int reg_points_num, int output_points_num, int addr_len, int singe_repair_points)
{
    int reg_index, output_index;
    uint32_t reg_value;

    for (output_index=0, reg_index=0; reg_index< reg_points_num; reg_index++)
    {
        // keep the low part of register
        reg_value = reg_addr[reg_index] & 0x0000FFFF;

        // confirm whether the address is valid
        if ((reg_value >> (addr_len-1)) % 2 == 1)
        {
            // if address < 16 bits, need to convert to OTP address
            if(addr_len < 16)
            {
                reg_value |= 0x00008000;              //firstly set the highest bit
                reg_value &= ~(1 << (addr_len-1));    // clear the original valid bit
                //reg_value |=  (reg_index / singe_repair_points) << (addr_len-1);    // calculate the index and set corresbonding bits
            }

            output_index += 1;
            BK_LOGD(NULL, "%08x\r\n", base_addr + reg_value * 4);
        }
    }

    return output_index;
}

int mcheck_ate(uint16_t check_res[48])
{
    uint32_t* reg_start_addr;
    int res = -1;

    reg_start_addr = (uint32_t*)SOC_MEM_CHECK_REG_BASE;

    // SMEM2, 4 points, no share
    res = mcheck_section(reg_start_addr + 8, SOC_SRAM2_DATA_BASE, 4, 4, 16, 4);

    // SMEM3, 4 points, no share
    res += mcheck_section(reg_start_addr + 12, SOC_SRAM3_DATA_BASE, 4, 4, 16, 4);

    // SMEM4, 4 points, no share
    res += mcheck_section(reg_start_addr + 16, SOC_SRAM4_DATA_BASE, 4, 4, 16, 4);

    // SMEM5, 4 points, no share
    res += mcheck_section(reg_start_addr + 20, SOC_SRAM5_DATA_BASE, 4, 4, 16, 4);

    // h265, 4 points, no share
    res += mcheck_section(reg_start_addr + 24, SOC_SRAM0_DATA_BASE, 4, 4, 16, 4);

    // SMEM0-1 overall 8 points, share 4 points
    res += mcheck_section(reg_start_addr + 28, SOC_SRAM0_DATA_BASE, 8, 4, 15, 4);

   // cpu0_0->cpu2_0 overall 12 points, share 6 points
    res += mcheck_section(reg_start_addr + 36, 0, 12, 6, 14, 4);

   // cpu0_1->cpu1_3 overall 16 points, share 8 points
    res += mcheck_section(reg_start_addr + 48, 0, 16, 8, 13, 2);

   // cpu2_3->pram overall 14 points, share 8 points
    res += mcheck_section(reg_start_addr + 64, SOC_PSRAM_DATA_BASE, 14, 8, 13, 2);

    return res;
}

void hal_mcheck(void)
{
	uint16_t check_res[48];
	if (mcheck_ate(check_res)) {
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
	}
}
#endif

/* A Non-Secure CP (TF-M builds) cannot reach the Secure-only MEM_CHECK window,
 * so the import is absent there and the AP side disables itself on the same
 * CONFIG_SPE gate. Only a Secure CP without OTP_V1 is a misconfiguration. */
#if CONFIG_DEEP_LV && CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_SPE && !CONFIG_OTP_V1
#error "CONFIG_PM_AP_FAST_BOOT_ENABLE needs CONFIG_OTP_V1 on a Secure CP for the AP MEM_CHECK repair import"
#endif

/* AP repair-address import. Gated on CONFIG_SPE because the register window is
 * Secure Only (PPHS reg4 bit1) and the literal below is the Secure alias: a
 * Non-Secure CP build must not touch it. */
#if CONFIG_OTP_V1 && CONFIG_SPE
#include <driver/otp.h>
#include "aon_pmu_ll.h"


/* AP MEM_CHECK register window: the AP subsystem's own block, address defined
 * in reg_base.h (the CP half of the decode reads the CP window,
 * SOC_MEM_CHECK_REG_BASE, instead). */

/* AP memory-check repair record layout: one 32-bit record per repair slot,
 * packed as [valid | slot | repair data]. */
#define AP_MEM_CHECK_OTP_WORDS          58U
#define AP_MEM_CHECK_OTP_BYTES          (AP_MEM_CHECK_OTP_WORDS * 4U)
#define AP_MEM_CHECK_REPAIR_LIMIT       20U
#define AP_MEM_CHECK_REPAIR_SLOT_NUM    128U
#define AP_MEM_CHECK_REPAIR_REG_FIRST   8U
#define AP_MEM_CHECK_OTP_VALID_MASK     0x80000000U
#define AP_MEM_CHECK_OTP_SLOT_MASK      0x007F0000U
#define AP_MEM_CHECK_OTP_SLOT_SHIFT     16U
#define AP_MEM_CHECK_OTP_DATA_MASK      0x0000FFFFU

/* Low-half write masks for MCHK_CFG[8..135], generated from the MEM_CHECK
 * address-mapping sheet of BK7259v2. The secure build carries the same table in
 * tfm/.../secure/ap_lv_reserve.c. */
static const uint32_t s_ap_mem_check_restore_mask[AP_MEM_CHECK_REPAIR_SLOT_NUM] = {
	0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU,
	0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x00001FFFU, 0x00001FFFU, 0x000007FFU, 0x000007FFU,
	0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU,
	0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU,
	0x00007FFFU, 0x00007FFFU, 0x00007FFFU, 0x00007FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU,
	0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00007FFFU, 0x00007FFFU, 0x00003FFFU, 0x00003FFFU,
	0x00003FFFU, 0x00003FFFU, 0x00001FFFU, 0x00001FFFU, 0x00003FFFU, 0x00003FFFU, 0x00001FFFU, 0x00001FFFU,
	0x00001FFFU, 0x00001FFFU, 0x00000FFFU, 0x00000FFFU, 0x00001FFFU, 0x00001FFFU, 0x0000FFFFU, 0x0000FFFFU,
	0x0000FFFFU, 0x0000FFFFU, 0x00001FFFU, 0x00001FFFU, 0x00003FFFU, 0x00003FFFU, 0x00003FFFU, 0x00003FFFU,
	0x00003FFFU, 0x00003FFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x00001FFFU, 0x00001FFFU,
	0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU,
};

static uint32_t load_le32(const uint8_t *data)
{
	return (uint32_t)data[0]
		| ((uint32_t)data[1] << 8)
		| ((uint32_t)data[2] << 16)
		| ((uint32_t)data[3] << 24);
}

#define AP_MEM_CHECK_PLAN_MAGIC         0x414D434AU /* "JCMA" */
#define AP_MEM_CHECK_PLAN_DATA_SHIFT    16U
#define AP_MEM_CHECK_PLAN_SLOT_MASK     0x0000007FU

static uint32_t s_ap_mem_check_plan[AP_MEM_CHECK_REPAIR_LIMIT];
static uint32_t s_ap_mem_check_plan_num;
static uint32_t s_ap_mem_check_plan_magic;

static void ap_mem_check_plan_build(const uint8_t *otp)
{
	uint32_t num = 0;

	s_ap_mem_check_plan_magic = 0U;

	for (uint32_t off = 0; off + 4U <= AP_MEM_CHECK_OTP_BYTES; off += 4U) {
		uint32_t word = load_le32(&otp[off]);
		uint32_t slot;

		if ((word & AP_MEM_CHECK_OTP_VALID_MASK) == 0U) {
			continue;
		}

		slot = (word & AP_MEM_CHECK_OTP_SLOT_MASK) >> AP_MEM_CHECK_OTP_SLOT_SHIFT;
		if (slot >= AP_MEM_CHECK_REPAIR_SLOT_NUM) {
			BK_LOGW(NULL, "skip invalid AP memcheck word 0x%08lx\r\n",
				(unsigned long)word);
			continue;
		}

		s_ap_mem_check_plan[num] = (slot & AP_MEM_CHECK_PLAN_SLOT_MASK) |
			((word & AP_MEM_CHECK_OTP_DATA_MASK) << AP_MEM_CHECK_PLAN_DATA_SHIFT);

		if (++num >= AP_MEM_CHECK_REPAIR_LIMIT) {
			break;
		}
	}

	s_ap_mem_check_plan_num = num;
	/* Magic last: a partially built plan must never be replayed. */
	s_ap_mem_check_plan_magic = AP_MEM_CHECK_PLAN_MAGIC;
}

static __attribute__((noinline)) bk_err_t ap_mem_check_plan_rebuild(void)
{
	uint8_t otp[AP_MEM_CHECK_OTP_BYTES];

	if (bk_otp_apb_read(OTP_M55SUB_MEMCHECK, otp, sizeof(otp)) != BK_OK) {
		BK_LOGE(NULL, "read the AP memory-check OTP failed\r\n");
		return BK_FAIL;
	}

	ap_mem_check_plan_build(otp);
	return BK_OK;
}

static const uint32_t *ap_mem_check_plan_get(uint32_t *num)
{
	if (s_ap_mem_check_plan_magic != AP_MEM_CHECK_PLAN_MAGIC) {
		if (ap_mem_check_plan_rebuild() != BK_OK) {
			return NULL;
		}
	}

	*num = s_ap_mem_check_plan_num;
	return s_ap_mem_check_plan;
}

bk_err_t hal_ap_mem_check_restore_from_otp(void)
{
	uint32_t num;
	const uint32_t *plan = ap_mem_check_plan_get(&num);

	if (plan == NULL) {
		return BK_FAIL;
	}

	/* Make sure the memory-check path is not bypassed before AP starts. */
	aon_pmu_ll_set_r0_memchk_bps(0);

	for (uint32_t i = 0; i < num; i++) {
		uint32_t record = plan[i];
		uint32_t slot = record & AP_MEM_CHECK_PLAN_SLOT_MASK;
		uint32_t value = (record >> AP_MEM_CHECK_PLAN_DATA_SHIFT) &
			AP_MEM_CHECK_OTP_DATA_MASK;
		uint32_t mask = s_ap_mem_check_restore_mask[slot];
		volatile uint32_t *reg = (volatile uint32_t *)SOC_AP_MEM_CHECK_REG_BASE +
			AP_MEM_CHECK_REPAIR_REG_FIRST + slot;

		*reg = (*reg & ~mask) | (value & mask);
	}

	/* Silent on a die with no repair record, which is every AP power-up on a
	 * clean part: the user has nothing to act on. Failures above stay loud. */
	if (num != 0U) {
		BK_LOGD(NULL, "restored %lu AP memcheck repair words\r\n",
			(unsigned long)num);
	}

	return BK_OK;
}

#endif
