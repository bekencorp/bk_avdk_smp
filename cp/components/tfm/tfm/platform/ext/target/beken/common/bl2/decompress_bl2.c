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

/*
 * BK7259 compressed-overwrite BL2 image install (secureboot_overwrite).
 *
 * This is the BK7259 rewrite of the BK7234 decompress_bl2.c. Key differences
 * versus BK7234:
 *   - BK7259 flash has NO 34/32 CRC interleave (crc_en=FALSE), so the physical
 *     and virtual flash offsets are IDENTITY. All the TOVIRTURE/TOPHY/
 *     CEIL_ALIGN_34 conversions are removed; a 64KB decompressed block maps to
 *     a 64KB physical span at primary_all + 64KB*index.
 *   - v1 is PLAINTEXT (flash_aes_type=NONE, ota.csv encrypt=FALSE), so the
 *     decompressed block is written with bk_flash_write_bytes() (direct/DBUS)
 *     instead of the HW-AES CBUS path (bk_flash_write_primary_cbus).
 *   - The 64KB input/output buffers are fixed static arrays (BL2 has a 512KB
 *     RAM window but only a 16KB heap, so os_malloc cannot serve 64KB); the
 *     LZMA probability table still comes from the compress component's own
 *     32KB static fallback (see cp/components/compress/lzma.c).
 *   - Power-fail resume is journaled in the dedicated ota_control partition
 *     (PARTITION_OTA_CONTROL). Because blocks are 64KB/identity aligned there is
 *     no cross-block tail to back up, so the journal only records the index of
 *     the last fully-written+committed block.
 *
 * boot_copy_region() overrides the built-in MCUboot copy (loader.c guards its
 * own definition with #ifndef CONFIG_OTA_OVERWRITE). The compressed source is
 * the ota staging partition (mapped as MCUboot's secondary slot, flash_map
 * index FLASH_MAP_IMAGE_SECONDARY_ALL == 1); the destination is primary_all
 * (index 0). The install is armed by the device receiver writing
 * OVERWRITE_CONFIRM into ota_control (see loader.c boot_validated_swap_type).
 */

#include "partitions_gen.h"

#if CONFIG_OTA_OVERWRITE

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <driver/flash.h>
#include "tfm_flash_partition.h"
#include "bl2_flash_map.h"
#include "decompress.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "flash_map_backend/flash_map_backend.h"

#define TAG "decompress"

extern void update_wdt(uint32_t val);

#define COMPRESS_BLOCK_SIZE (64 * 1024)
/* ota_control: resume journal in first sector(s), OVERWRITE_CONFIRM in last
 * sector (separate so the confirm flag survives a redo, power-fail safe). 4KB = sector. */
#define OTA_CTRL_SECTOR_SIZE (4 * 1024)
#define OTA_WDT_FEED_VAL 0xFFFFu

/* Only the pointer is used (copy is driven by flash_map indices), so a forward decl suffices. */
struct boot_loader_state;

/* Fixed decode buffers: two 64KB static arrays fit BL2's 512KB RAM and avoid a
 * 64KB os_malloc against the 16KB BL2 heap. */
static uint8_t s_compressed_buf[COMPRESS_BLOCK_SIZE + 64];
static uint8_t s_decompressed_buf[COMPRESS_BLOCK_SIZE + 64];

typedef struct {
	uint8_t crc;
} CRC8_Context;

/* 35-byte journal record; data[] is unused on BK7259 (no interleave tail to back
 * up) but kept so the on-flash layout matches the proven BK7234 record. */
typedef struct {
	uint8_t index;
	uint8_t data[33];
	CRC8_Context crc8;
} resume_block_t;

static uint8_t UpdateCRC8(uint8_t crcIn, uint8_t byte)
{
	uint8_t crc = crcIn;
	uint8_t i;

	crc ^= byte;
	for (i = 0; i < 8; i++) {
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		} else {
			crc >>= 1;
		}
	}
	return crc;
}

static void CRC8_Init(CRC8_Context *inContext)
{
	inContext->crc = 0;
}

static void CRC8_Update(CRC8_Context *inContext, const void *inSrc, size_t inLen)
{
	const uint8_t *src = (const uint8_t *)inSrc;
	const uint8_t *srcEnd = src + inLen;
	while (src < srcEnd) {
		inContext->crc = UpdateCRC8(inContext->crc, *src++);
	}
}

static uint32_t get_resume_base_address(void)
{
	return partition_get_phy_offset(PARTITION_OTA_CONTROL);
}

/* Valid record count = committed blocks (record at slot[index]); stop at first
 * 0xFF. Returns 0xFF on CRC corruption to force a full redo. */
static uint8_t read_resume_block(uint32_t back_address)
{
	resume_block_t curr;
	CRC8_Context crc_8;
	uint32_t max = 4096 / sizeof(resume_block_t);
	uint8_t idx = 0;

	for (; idx < max; ++idx) {
		CRC8_Init(&crc_8);
		memset(&curr, 0xFF, sizeof(curr));
		bk_flash_read_bytes(back_address + idx * sizeof(resume_block_t),(uint8_t *)&curr, sizeof(curr));
		if (curr.index != 0xFF) {
			CRC8_Update(&crc_8, &curr.index, sizeof(curr.index));
			CRC8_Update(&crc_8, curr.data, sizeof(curr.data));
			if (crc_8.crc != curr.crc8.crc) {
				BOOT_LOG_ERR("resume block=%d crc8 error!", idx);
				return 0xffu;
			}
		} else {
			break;
		}
	}
	return idx;
}

static void write_resume_block(uint8_t idx, uint32_t back_address)
{
	resume_block_t resume_block;
	CRC8_Context crc_8;

	memset(&resume_block, 0xFF, sizeof(resume_block));
	CRC8_Init(&crc_8);
	CRC8_Update(&crc_8, &idx, sizeof(idx));
	CRC8_Update(&crc_8, resume_block.data, sizeof(resume_block.data));
	resume_block.index = idx;
	resume_block.crc8 = crc_8;
	bk_flash_write_bytes(back_address + idx * sizeof(resume_block_t),(uint8_t *)&resume_block, sizeof(resume_block));
}

static uint32_t idx_sum(uint16_t *buffer, size_t idx)
{
	uint32_t sum = 0;
	for (size_t i = 0; i < idx; ++i) {
		sum += buffer[i];
	}
	return sum;
}

#define ERASE_VERIFY_BUF_SIZE (4 * 1024)
#define ERASE_RETRY_COUNT     3

/* Read the just-erased span back and confirm it is all 0xFF. Returns 0 on pass. */
static int verify_erase(uint32_t offset, uint32_t size)
{
	static uint8_t verify_buf[ERASE_VERIFY_BUF_SIZE];
	uint32_t remaining = size;
	uint32_t cur = offset;

	while (remaining > 0) {
		update_wdt(OTA_WDT_FEED_VAL);
		uint32_t chunk = (remaining > ERASE_VERIFY_BUF_SIZE) ?
				 ERASE_VERIFY_BUF_SIZE : remaining;
		if (bk_flash_read_bytes(cur, verify_buf, chunk) != BK_OK) {
			BOOT_LOG_ERR("erase-verify read failed off=0x%x size=0x%x", cur, chunk);
			return -1;
		}
		for (uint32_t i = 0; i < chunk; i++) {
			if (verify_buf[i] != 0xFF) {
				BOOT_LOG_ERR("erase-verify mismatch off=0x%x byte[%u]=0x%02x",
					     cur + i, (unsigned)i, verify_buf[i]);
				return -1;
			}
		}
		cur += chunk;
		remaining -= chunk;
	}
	return 0;
}

/* Erase + readback-verify with retries; -1 on persistent failure (caller aborts,
 * bl2_main.c then re-arms OVERWRITE_CONFIRM to rerun the install). */
static int flash_area_erase_fast_verify(uint32_t erase_off, uint32_t len)
{
	for (int retry = 0; retry < ERASE_RETRY_COUNT; retry++) {
		/* Erase in 64KB chunks, feeding the WDT before each. A single multi-MB
		 * flash_area_erase_fast() call never feeds the WDT internally, so erasing
		 * primary_all (~3.5MB, several seconds) trips the ~1s WWDT mid-erase and
		 * resets BL2 with primary_all half-erased -> confirm still set -> reinstall
		 * loop. primary_all is 64KB-aligned so chunks keep the fast 64k-erase path. */
		uint32_t off = erase_off;
		uint32_t remaining = len;
		while (remaining > 0) {
			uint32_t chunk = (remaining > COMPRESS_BLOCK_SIZE) ? COMPRESS_BLOCK_SIZE : remaining;
			update_wdt(OTA_WDT_FEED_VAL);
			flash_area_erase_fast(off, chunk);
			off += chunk;
			remaining -= chunk;
		}
		if (verify_erase(erase_off, len) == 0) {
			if (retry > 0) {
				BOOT_LOG_INF("erase ok after %d retries off=0x%x size=0x%x",retry, erase_off, len);
			}
			return 0;
		}
		BOOT_LOG_ERR("erase verify failed (%d/%d) off=0x%x size=0x%x",
			     retry + 1, ERASE_RETRY_COUNT, erase_off, len);
	}
	BOOT_LOG_ERR("erase failed after %d retries off=0x%x size=0x%x, abort OTA copy",
		     ERASE_RETRY_COUNT, erase_off, len);
	return -1;
}

/* Sanity-check the ota_control base before erasing: a bogus offset (0, or below
 * primary_all) would erase the bootloader at flash offset 0 and brick the device. */
static int ota_ctrl_erase_ok(uint32_t back_address, uint32_t primary_all_phy_offset)
{
	if (back_address != 0 && back_address >= primary_all_phy_offset) {
		return 1;
	}
	BOOT_LOG_ERR("ota_control addr 0x%x invalid, skip erase (protect bootloader)",
		     back_address);
	return 0;
}

/* Decide the (re)start block index and erase the primary_all region to be
 * (re)written; identity mapping keeps committed blocks [0, restart) intact.
 * Returns that index, or -1 if a primary_all erase couldn't be verified after
 * retries (caller must abort the copy). */
static int resume_flash(uint32_t block_num)
{
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);
	uint32_t primary_all_phy_size = get_flash_map_phy_size(0);
	uint32_t back_address = get_resume_base_address();
	uint32_t ota_ctrl_size = partition_get_phy_size(PARTITION_OTA_CONTROL);
	uint32_t primary_magic = 0xffffffffu;

	uint8_t restart_block_idx = read_resume_block(back_address);
	BOOT_LOG_INF("total block=%u, resume block=%u", block_num, restart_block_idx);

	/* Stale-journal guard: a crash after primary erase but before journal clear
	 * (or a protect-era half install) can leave primary_all=0xFF while the
	 * journal still claims resume==block_num. That skips every full block
	 * (for-loop is empty) and bricks in a re-arm loop. If primary has no
	 * IMAGE_MAGIC, discard the journal and force a full redo. */
	bk_flash_read_bytes(primary_all_phy_offset, (uint8_t *)&primary_magic,
			    sizeof(primary_magic));
	if (primary_magic != IMAGE_MAGIC &&
	    restart_block_idx != 0 && restart_block_idx != 0xffu) {
		BOOT_LOG_ERR("primary magic=0x%x but resume=%u, discard stale journal",
			     primary_magic, restart_block_idx);
		restart_block_idx = 0;
	}

	if ((restart_block_idx == 0) || (restart_block_idx == 0xffu) ||
	    (restart_block_idx > block_num)) {
		BOOT_LOG_INF("Erasing primary and resume journal");
		/* Clear journal FIRST. If we erase primary then lose power before
		 * clearing the journal, the next boot resumes mid-image against an
		 * empty primary (see stale-journal guard above). Journal-first means
		 * a crash restarts from block 0. Confirm lives in the last sector and
		 * is intentionally preserved. */
		if (ota_ctrl_erase_ok(back_address, primary_all_phy_offset)) {
			flash_area_erase_fast(back_address, ota_ctrl_size - OTA_CTRL_SECTOR_SIZE);
		}
		if (flash_area_erase_fast_verify(primary_all_phy_offset, primary_all_phy_size) != 0) {
			return -1;
		}
		return 0;
	}

	uint32_t restart_block_offset = primary_all_phy_offset + COMPRESS_BLOCK_SIZE * restart_block_idx;
	uint32_t erase_size = primary_all_phy_size - COMPRESS_BLOCK_SIZE * restart_block_idx;
	BOOT_LOG_INF("Resume: erase primary off=0x%x size=0x%x",
		     restart_block_offset, erase_size);
	if (flash_area_erase_fast_verify(restart_block_offset, erase_size) != 0) {
		return -1;
	}
	return restart_block_idx;
}

static void clean_buf(void)
{
	memset(s_decompressed_buf, 0, COMPRESS_BLOCK_SIZE);
	memset(s_compressed_buf, 0, COMPRESS_BLOCK_SIZE);
}

/* ota slot layout: [hdr(ih_hdr_size)][u32 block_num][u16 block_list[block_num+2]][blocks...]
 * block_list: [0..block_num-1] = compressed size of each full 64KB block;
 * [block_num]/[block_num+1] = compressed/decompressed size of the last partial block.
 * block_num is in the signed payload (host = floor(signed_size/64KB)), so it's covered
 * by boot_validate_slot(). It's read via the image's own ih_hdr_size (offset 8), with
 * BL2_HEADER_SIZE only as fallback when that field is blank/erased. */
int boot_copy_region(struct boot_loader_state *state,
		 const struct flash_area *fap_src,
		 const struct flash_area *fap_dst,
		 uint32_t off_src, uint32_t off_dst, uint32_t sz)
{
	int rc = -1;
	uint16_t ih_hdr_size = 0;
	uint32_t block_num = 0;

	(void)state;
	(void)fap_dst;
	(void)off_dst;
	(void)sz;

	uint32_t primary_all_vir_size = get_flash_map_size(0);
	uint32_t primary_all_phy_offset = get_flash_map_offset(0);

	flash_area_read(fap_src, off_src + 8, &ih_hdr_size, sizeof(ih_hdr_size));
	if (ih_hdr_size == 0 || ih_hdr_size == 0xffffu) {
		ih_hdr_size = BL2_HEADER_SIZE;
	}

	flash_area_read(fap_src, off_src + ih_hdr_size, &block_num, sizeof(block_num));

	/* Guard a corrupt count before it sizes the VLA / drives the loop
	 * (the image may be unverified when secure boot is off). */
	uint32_t max_blocks = primary_all_vir_size / COMPRESS_BLOCK_SIZE;
	if (block_num == 0 || block_num > max_blocks) {
		BOOT_LOG_ERR("OTA bad block_num=%u (max=%u), abort copy", block_num, max_blocks);
		return -1;
	}

	uint16_t block_list[block_num + 2];
	uint32_t back_address = get_resume_base_address();
	uint32_t bytes_copied  = 0;
	uint8_t block_idx = 0;

	/* BK7259SW-2937 defers unprotect out of flash init so the read-only
	 * secure-boot path stays protected. That is correct for DIRECT_XIP (BL2
	 * never erases/programs), but compressed-overwrite MUST write primary_all
	 * here. Without unprotect, erase is a no-op (status protect) and
	 * erase-verify still sees IMAGE_MAGIC byte 0x3d at primary_all. Mirror
	 * the serial-download handshake: unprotect once, then stay in two-line
	 * for the erase/program session. */
	update_wdt(OTA_WDT_FEED_VAL);
	bk_flash_min_unprotect_once();
	bk_flash_min_switch_line_mode_two();

	int restart_block_idx = resume_flash(block_num);
	if (restart_block_idx < 0) {
		BOOT_LOG_ERR("OTA copy: primary_all erase unverified, abort");
		goto out;
	}

	BOOT_LOG_INF("OTA copy: resume_flash done, restart_block=%u", restart_block_idx);
	int rate_process = (block_num >= 5) ? (int)(block_num / 5) : 1;

	/* Skip [header][uint32 block_num], then read the block_list. */
	bytes_copied = ih_hdr_size + (uint32_t)sizeof(block_num);
	flash_area_read(fap_src, off_src + bytes_copied, block_list, 2 * (block_num + 2));
	bytes_copied += 2 * (block_num + 2);
	bytes_copied += idx_sum(block_list, restart_block_idx);

	for (block_idx = restart_block_idx; block_idx < block_num; block_idx++) {
		update_wdt(OTA_WDT_FEED_VAL);
		clean_buf();
		flash_area_read(fap_src, off_src + bytes_copied, s_compressed_buf,
				block_list[block_idx]);

		uint8_t *r = decompress_in_memory(s_compressed_buf, s_decompressed_buf,
						  COMPRESS_BLOCK_SIZE, DECOMPRESS_BY_LZMA);
		if (r == NULL) {
			BOOT_LOG_ERR("OTA decompress failed at block %d", block_idx);
			goto out;
		}

		bk_flash_write_bytes(primary_all_phy_offset + COMPRESS_BLOCK_SIZE * block_idx,
				     s_decompressed_buf, COMPRESS_BLOCK_SIZE);
		/* Commit: record that block_idx is fully written. */
		write_resume_block(block_idx, back_address);

		bytes_copied += block_list[block_idx];
		if (((block_idx + 1) % rate_process) == 0) {
			BOOT_LOG_INF("OTA %d%%", (block_idx / rate_process + 1) * 20);
		}
	}

	/* Final partial block. */
	uint16_t last_block_before_size = block_list[block_idx + 1]; /* decompressed */
	uint16_t last_block_after_size  = block_list[block_idx];     /* compressed   */

	if (last_block_before_size > 0) {
		update_wdt(OTA_WDT_FEED_VAL);
		clean_buf();
		flash_area_read(fap_src, off_src + bytes_copied, s_compressed_buf, last_block_after_size);
		uint8_t *r = decompress_in_memory(s_compressed_buf, s_decompressed_buf,
						  last_block_after_size, DECOMPRESS_BY_LZMA);
		if (r == NULL) {
			BOOT_LOG_ERR("OTA decompress failed at last block");
			goto out;
		}

		uint16_t write_size = (last_block_before_size + 31) / 32 * 32;
		bk_flash_write_bytes(primary_all_phy_offset + COMPRESS_BLOCK_SIZE * block_idx, s_decompressed_buf, write_size);
	}

	/* Do NOT clear ota_control here. OVERWRITE_CONFIRM + the resume journal are
	 * cleared by the SPE (tfm_hal_platform_init) only after the freshly installed
	 * image has booted into the secure world -- MCUboot's confirmed pattern -- so a
	 * power loss before that point safely re-runs the install on the next boot. */
	rc = 0;

out:
	bk_flash_min_restore_line_mode();
	return rc;
}

#endif /* CONFIG_OTA_OVERWRITE */
