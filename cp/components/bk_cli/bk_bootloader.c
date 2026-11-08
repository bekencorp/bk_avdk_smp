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

/* Bootloader version reader, exposed through bk_bootloader.h. Kept apart from the
 * CLI (cli_bootloader.c) so other modules such as pm can query the version
 * without depending on the CLI.
 *
 * Security firmware (TF-M) projects keep the stamp at the tail of the BL2
 * manifest slot, in the flash region TF-M marks Secure on the DBUS, so it is read
 * through the CMSE gateway bk_bootloader_version_nsc(). Projects without security
 * firmware run aboot instead: no TrustZone, and the stamp sits right after the
 * aboot vector table in the bootloader partition, read straight from flash. Both
 * stamps share the same 16-byte layout, only the source differs.
 *
 * Layout: magic "BL2V" | version (LE, major<<16 | minor<<8 | patch) |
 * crc32(magic + version, zlib) | reserved. Written by bl1_sign.py (BL2) and
 * arm_bootloader/tools/bootloader.py (aboot); parsing matches the secure side
 * bl2_version.c. */

#include <os/os.h>
#include <os/str.h>
#include "bk_bootloader.h"

#define BK_BOOTLOADER_STAMP_MAGIC    0x56324C42u	/* "BL2V" little-endian */
#define BK_BOOTLOADER_STAMP_SIZE     16u
#define BK_BOOTLOADER_STAMP_CRC_LEN  8u			/* magic + version */

#if CONFIG_TFM_BOOTLOADER_VERSION_NSC

#include "tfm_bootloader_nsc.h"
#include "tfm_ns_interface.h"

static uint32_t bootloader_version_read(void)
{
	uint32_t ver;

	tfm_ns_secure_call_lock();
	ver = bk_bootloader_version_nsc();
	tfm_ns_secure_call_unlock();

	return ver;
}

#else /* aboot: the stamp lives in the bootloader partition, read it directly */

#include "driver/flash.h"
#include <driver/flash_partition.h>

/* Offset reserved by aboot right after its vector table, see __bl_version in
 * aboot/arm_bootloader/bootloader/platform/bk7259/ld/bootloader.ld. */
#define BK_BOOTLOADER_ABOOT_STAMP_OFFSET  0x148u

static uint32_t load_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* zlib CRC32 (poly 0xEDB88320, init 0xFFFFFFFF, final xor) over 8 bytes. */
static uint32_t stamp_crc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFFFFFFu;
	uint32_t i;
	int bit;

	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(crc & 1u)));
	}
	return crc ^ 0xFFFFFFFFu;
}

static uint32_t bootloader_version_read(void)
{
	uint8_t stamp[BK_BOOTLOADER_STAMP_SIZE];
	bk_logic_partition_t *part = bk_flash_partition_get_info(BK_PARTITION_BOOTLOADER);

	if (part == NULL)
		return BK_BOOTLOADER_VERSION_ABSENT;
	if (bk_flash_read_bytes(part->partition_start_addr + BK_BOOTLOADER_ABOOT_STAMP_OFFSET,
				stamp, sizeof(stamp)) != BK_OK)
		return BK_BOOTLOADER_VERSION_ABSENT;

	/* No stamp written yet: legacy bootloader or erased slot. */
	if (load_le32(stamp) != BK_BOOTLOADER_STAMP_MAGIC)
		return BK_BOOTLOADER_VERSION_ABSENT;
	if (stamp_crc32(stamp, BK_BOOTLOADER_STAMP_CRC_LEN) != load_le32(&stamp[8]))
		return BK_BOOTLOADER_VERSION_CORRUPT;

	return load_le32(&stamp[4]);
}

#endif

uint32_t bk_bootloader_get_version(void)
{
	return bootloader_version_read();
}

bk_err_t bk_bootloader_get_version_string(char *buf, uint32_t len)
{
	uint32_t ver;

	/* The longest form "255.255.255" needs len >= 12 with the terminator. */
	if (buf == NULL || len < 12)
		return BK_FAIL;

	ver = bootloader_version_read();
	if (ver == BK_BOOTLOADER_VERSION_ABSENT || ver == BK_BOOTLOADER_VERSION_CORRUPT)
		return BK_FAIL;

	os_snprintf(buf, len, "%u.%u.%u",
		    (unsigned)((ver >> 16) & 0xFFu),
		    (unsigned)((ver >> 8) & 0xFFu),
		    (unsigned)(ver & 0xFFu));
	return BK_OK;
}
