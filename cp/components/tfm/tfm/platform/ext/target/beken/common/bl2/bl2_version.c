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

/* bl2_version: reads the BL2 (bootloader) version stamp from the tail of the
 * running manifest slot. One copy is compiled into both BL2 (bl2_main.c) and
 * the SPE (bootloader_nsc.c) so the two sides cannot disagree. The stamp is
 * written by beken_utils/scripts/bl1_sign.py:
 *   +0  magic "BL2V"
 *   +4  version, little-endian, major<<16 | minor<<8 | patch
 *   +8  zlib crc32 over the 8 bytes [magic + version]
 *   +12 reserved
 * A zero return means a valid 0.0.0 stamp; BL2_VERSION_ABSENT means no stamp is
 * there to read (legacy bootloader, erased slot, magic mismatch or read
 * failure) and BL2_VERSION_CORRUPT means a stamp exists but its CRC is wrong. */

#include "bl2_version.h"
/* Include the generated table first: it lists only the partitions this project
 * really reserves, while partitions.h adds an #ifndef fallback for a missing
 * one, so only this ordering tells a real A/B secondary manifest apart from a
 * stub. */
#include "partitions_gen.h"
#ifdef CONFIG_SECONDARY_MANIFEST_PHY_PARTITION_OFFSET
#define BL2_HAS_SECONDARY_MANIFEST 1
#else
#define BL2_HAS_SECONDARY_MANIFEST 0
#endif
#include "partitions.h"		/* manifest / boot_flag offsets */
#include "boot_param.h"		/* boot_param_crc32 (zlib CRC32) */
#include <common/bk_err.h>	/* bk_err_t only */

extern bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size);

#define BL2_VERSION_STAMP_MAGIC   0x56324C42u	/* "BL2V" little-endian */
#define BL2_VERSION_STAMP_OFFSET  0xFF0u
#define BL2_VERSION_STAMP_SIZE    16u

/* Absolute flash offset of the running BL2 manifest. boot_flag (0x1000, magic
 * "cTrL", byte 4 = 1 primary / 2 secondary) selects the A/B slot once the
 * bootloader is upgradeable; until the layout really reserves a secondary
 * manifest this is compiled out and the primary slot is used directly. */
static uint32_t bl2_running_manifest_offset(void)
{
#if BL2_HAS_SECONDARY_MANIFEST
	uint8_t bf[8];

	if (bk_flash_read_bytes(CONFIG_BOOT_FLAG_PHY_PARTITION_OFFSET, bf, sizeof(bf)) == BK_OK &&
	    bf[0] == 0x63 && bf[1] == 0x54 && bf[2] == 0x72 && bf[3] == 0x4c && bf[4] == 2)
		return CONFIG_SECONDARY_MANIFEST_PHY_PARTITION_OFFSET;
#endif

	return CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET;
}

static uint32_t bl2_load_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint32_t bl2_get_version(void)
{
	uint8_t stamp[BL2_VERSION_STAMP_SIZE];

	if (bk_flash_read_bytes(bl2_running_manifest_offset() + BL2_VERSION_STAMP_OFFSET,
				stamp, sizeof(stamp)) != BK_OK)
		return BL2_VERSION_ABSENT;

	/* No stamp written yet: legacy bootloader, erased slot or garbage. */
	if (bl2_load_le32(stamp) != BL2_VERSION_STAMP_MAGIC)
		return BL2_VERSION_ABSENT;

	if (boot_param_crc32(stamp, 8) != bl2_load_le32(&stamp[8]))
		return BL2_VERSION_CORRUPT;

	return bl2_load_le32(&stamp[4]);
}
