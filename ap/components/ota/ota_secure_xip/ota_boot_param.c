/*
 * ota_boot_param.c - AP-side writer for the boot_param TRIAL record.
 *
 * See ota_boot_param.h. The ping-pong algorithm and record layout come from the
 * shared ab_flag.h; this file only supplies the non-secure flash back-end and
 * targets the boot_param partition so the committed record is consumed by the
 * CP-side BL2/SPE (boot_param.h) unchanged.
 */

#include <stdint.h>
#include <string.h>
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "modules/ota.h"          /* bk_ota_get_current_partition() */
#include "bk_private/bk_ota_private.h"
#include "ab_flag.h"
#include "ota_boot_param.h"

/* boot_param ping-pong back-end (AP side): SDK flash driver + inline zlib CRC32.
 * See ab_flag.h for the shared record layout and algorithm. */

static void ota_bp_read(uint32_t off, void *buf, uint32_t len)
{
	bk_flash_read_bytes(off, (uint8_t *)buf, len);
}

static void ota_bp_erase(uint32_t off)
{
	bk_flash_erase_sector(off);
}

static void ota_bp_write(uint32_t off, const void *buf, uint32_t len)
{
	bk_flash_write_bytes(off, (uint8_t *)buf, len);
}

/* zlib/PKZIP CRC32 (init 0xFFFFFFFF, poly 0xEDB88320, final inversion), inlined
 * to avoid the crc32_zlib dependency (only linked on CONFIG_HTTP_AB_PARTITION).
 * Bit-identical to the CP boot_param_ops.c and the packer. */
static uint32_t ota_bp_crc32(const void *buf, uint32_t len)
{
	const uint8_t *d = (const uint8_t *)buf;
	uint32_t crc = 0xFFFFFFFFu;
	uint32_t i;

	for (i = 0; i < len; i++) {
		crc ^= d[i];
		for (int b = 0; b < 8; b++) {
			uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
			crc = (crc >> 1) ^ (0xEDB88320u & mask);
		}
	}
	return crc ^ 0xFFFFFFFFu;
}

static const ab_flag_ops_t s_ota_bp_ops = {
	.read = ota_bp_read,
	.erase_sector = ota_bp_erase,
	.write = ota_bp_write,
	.crc32 = ota_bp_crc32,
};

static uint32_t ota_bp_partition_base(void)
{
	bk_logic_partition_t *part = bk_flash_partition_get_info(BK_PARTITION_BOOT_PARAM);

	if (part == NULL) {
		OTA_LOGE("boot_param partition missing\r\n");
		return 0;
	}
	return part->partition_start_addr;
}

int ota_boot_param_read_latest(ab_flag_record_t *rec)
{
	uint32_t base;

	if (rec == NULL) {
		return -1;
	}
	base = ota_bp_partition_base();
	if (base == 0) {
		return -1;
	}
	return (ab_record_read_latest(base, &s_ota_bp_ops, rec) < 0) ? -1 : 0;
}

int ota_boot_param_set_trial(uint8_t update_slot)
{
	ab_flag_record_t rec;
	uint32_t base = ota_bp_partition_base();
	flash_protect_type_t protect_type;

	if (base == 0) {
		return -1;
	}

	/* Start from the freshest record (keep try_max); default on a virgin part. */
	if (ab_record_read_latest(base, &s_ota_bp_ops, &rec) < 0) {
		memset(&rec, 0, sizeof(rec));
		rec.try_max = AB_FLAG_DEFAULT_TRY_MAX;
	}

	/* exec_slot = slot actually running (HW XIP remap), not the stale record. */
	rec.exec_slot   = (uint8_t)bk_ota_get_current_partition();
	rec.update_slot = update_slot;
	rec.boot_state  = (uint8_t)AB_STATE_TRIAL;
	rec.dl_state    = (uint8_t)AB_DL_DONE;
	if (rec.try_max == 0u) {
		rec.try_max = AB_FLAG_DEFAULT_TRY_MAX;
	}

	/* Zero rsvd0 for a fresh trial budget: byte 0x11 is the boot_param try_count
	 * (CP boot_param.h), which a prior BL2 write may have left non-zero. */
	memset(rec.rsvd0, 0, sizeof(rec.rsvd0));

	protect_type = bk_flash_get_protect_type();
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	(void)ab_record_commit(base, &s_ota_bp_ops, &rec);
	bk_flash_set_protect_type(protect_type);

	OTA_LOGI("boot_param trial armed: exec=%u update=%u try_max=%u\r\n",
			 rec.exec_slot, rec.update_slot, rec.try_max);
	return 0;
}
