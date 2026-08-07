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

/* Shared boot_param back-end: the single flash driver + zlib-CRC32 + partition
 * base feeding the ping-pong algorithm in boot_param.h. Compiled into both
 * platform_bl2 (boot_param.c) and platform_s (boot_param_confirm.c) so the two
 * on-chip consumers cannot drift. Keep boot_param_crc32() bit-identical to the
 * Python packer's zlib.crc32.
 *
 * Links into the secure image, so the ops never call BK_LOG (the SDK log path is
 * unsafe from the SPE); flash errors surface via the record CRC/validity check. */

#include <soc/soc.h>		/* REG_READ/REG_WRITE, SOC_AON_PMU_REG_BASE */
#include "boot_param.h"
#include "tfm_flash_partition.h"
#include "partitions.h"
#include "components/log.h"	/* bk_err_t only */

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

static void bp_flash_read(uint32_t off, void *buf, uint32_t len)
{
	(void)bk_flash_read_bytes(off, (uint8_t *)buf, len);
}

static void bp_flash_erase(uint32_t off)
{
	(void)bk_flash_erase_sector(off);
}

static void bp_flash_write(uint32_t off, const void *buf, uint32_t len)
{
	(void)bk_flash_write_bytes(off, (const uint8_t *)buf, len);
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

/* AON_PMU trial-boot counter: keeps the runtime attempt count out of flash. Held in
 * R0 bit[14:12] (3-bit), latched via the R25 magic handshake, read from R7A;
 * survives warm reset, cleared by cold power-on. Ported from aboot driver_ab.c. */
#define BOOT_PARAM_PMU_R0        (0x0u)
#define BOOT_PARAM_PMU_R25       (0x25u)
#define BOOT_PARAM_PMU_R7A       (0x7Au)
#define BOOT_PARAM_PMU_TRY_BIT   (12u)
#define BOOT_PARAM_PMU_TRY_MASK  (0x7u)   /* 3-bit reboot counter field */

/* Write R0 then latch it via the magic sequence. */
static void boot_param_pmu_latch_r0(uint32_t r0_val)
{
	REG_WRITE(SOC_AON_PMU_REG_BASE + BOOT_PARAM_PMU_R0 * 4, r0_val);
	REG_WRITE(SOC_AON_PMU_REG_BASE + BOOT_PARAM_PMU_R25 * 4, 0x424B55AAu);
	REG_WRITE(SOC_AON_PMU_REG_BASE + BOOT_PARAM_PMU_R25 * 4, 0xBDB4AA55u);
}

uint8_t boot_param_pmu_try_get(void)
{
	uint32_t r7a = REG_READ(SOC_AON_PMU_REG_BASE + BOOT_PARAM_PMU_R7A * 4);

	return (uint8_t)((r7a >> BOOT_PARAM_PMU_TRY_BIT) & BOOT_PARAM_PMU_TRY_MASK);
}

void boot_param_pmu_try_inc(void)
{
	uint32_t r7a = REG_READ(SOC_AON_PMU_REG_BASE + BOOT_PARAM_PMU_R7A * 4);
	uint32_t cnt = (r7a >> BOOT_PARAM_PMU_TRY_BIT) & BOOT_PARAM_PMU_TRY_MASK;

	/* Saturate at the 3-bit max (never wrap to 0) so a stuck crash loop stays
	 * over threshold. */
	if (cnt >= BOOT_PARAM_PMU_TRY_MASK) {
		return;
	}
	cnt++;
	r7a &= ~(BOOT_PARAM_PMU_TRY_MASK << BOOT_PARAM_PMU_TRY_BIT);
	r7a |= (cnt << BOOT_PARAM_PMU_TRY_BIT);
	boot_param_pmu_latch_r0(r7a);
}

void boot_param_pmu_try_clear(void)
{
	uint32_t r7a = REG_READ(SOC_AON_PMU_REG_BASE + BOOT_PARAM_PMU_R7A * 4);

	r7a &= ~(BOOT_PARAM_PMU_TRY_MASK << BOOT_PARAM_PMU_TRY_BIT);
	boot_param_pmu_latch_r0(r7a);
}
