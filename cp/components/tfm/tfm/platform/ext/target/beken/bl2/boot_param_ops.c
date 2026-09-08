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

/* boot_param flash/CRC back-end for platform_bl2. CRC must match Python zlib.crc32. */

#include "boot_param.h"
#include "flash_partition.h"
#include "partitions.h"
#include "aon_pmu_hal.h"	/* aon_pmu_ll_get_r7b / set_r0 / set_r25 */
#include <common/bk_err.h>	/* bk_err_t only */

extern bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size);
extern bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size);
extern bk_err_t bk_flash_erase_sector(uint32_t address);

uint32_t boot_param_crc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFFFFFFu;

	for (uint32_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int b = 0; b < 8; b++) {
			uint32_t mask = -(int32_t)(crc & 1u);
			crc = (crc >> 1) ^ (0xEDB88320u & mask);
		}
	}

	return crc ^ 0xFFFFFFFFu;
}

static int bp_flash_read(uint32_t off, void *buf, uint32_t len)
{
	return bk_flash_read_bytes(off, (uint8_t *)buf, len);
}

static int bp_flash_erase(uint32_t off)
{
	return bk_flash_erase_sector(off);
}

static int bp_flash_write(uint32_t off, const void *buf, uint32_t len)
{
	return bk_flash_write_bytes(off, (const uint8_t *)buf, len);
}

static uint32_t bp_crc32(const void *buf, uint32_t len)
{
	return boot_param_crc32((const uint8_t *)buf, len);
}

const ab_flag_ops_t boot_param_ops = {
	.read = bp_flash_read,
	.erase_sector = bp_flash_erase,
	.write = bp_flash_write,
	.crc32 = bp_crc32,
};

uint32_t boot_param_partition_base(void)
{
	uint32_t base = partition_get_phy_offset(PARTITION_BOOT_PARAM);

	if (base == 0) {
		base = CONFIG_BOOT_PARAM_PHY_PARTITION_OFFSET;
	}
	return base;
}

/* AON_PMU try counter (bit[20:23]): CRC hang + TRIAL. Warm-reset sticky; BIT/MASK in boot_param.h. */

static void boot_param_pmu_latch_r0(uint32_t r0_val)
{
	aon_pmu_ll_set_r0(r0_val);
	aon_pmu_ll_set_r25(0x424B55AAu);
	aon_pmu_ll_set_r25(0xBDB4AA55u);
}

uint8_t boot_param_pmu_try_get(void)
{
	uint32_t r7b = aon_pmu_ll_get_r7b();

	return (uint8_t)((r7b >> BOOT_PARAM_PMU_TRY_BIT) & BOOT_PARAM_PMU_TRY_MASK);
}

void boot_param_pmu_try_inc(void)
{
	uint32_t r7b = aon_pmu_ll_get_r7b();
	uint32_t cnt = (r7b >> BOOT_PARAM_PMU_TRY_BIT) & BOOT_PARAM_PMU_TRY_MASK;

	/* Saturate at 15. Do not clear here: TRIAL may use try_max up to 14
	 * (rollback when cnt > try_max). NORMAL CRC recovery clears try>LIMIT
	 * in loader find_slot_with_highest_version instead. */
	if (cnt >= BOOT_PARAM_PMU_TRY_MASK) {
		return;
	}
	cnt++;
	r7b &= ~(BOOT_PARAM_PMU_TRY_MASK << BOOT_PARAM_PMU_TRY_BIT);
	r7b |= (cnt << BOOT_PARAM_PMU_TRY_BIT);
	boot_param_pmu_latch_r0(r7b);
}

void boot_param_pmu_try_clear(void)
{
	uint32_t r7b = aon_pmu_ll_get_r7b();

	r7b &= ~(BOOT_PARAM_PMU_TRY_MASK << BOOT_PARAM_PMU_TRY_BIT);
	boot_param_pmu_latch_r0(r7b);
}
