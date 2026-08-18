/*
 * ota_secure_overwrite.c - Compressed-overwrite OTA backend (non-secure world).
 *
 * Plugs into the non-secure OTA framework as an f_ota_func_t back-end
 * (bk_ota_secure_overwrite_backend()), selected in ota_do_init_operation() when
 * CONFIG_SECURE_OTA_OVERWRITE is set (secureboot_overwrite project). The
 * transport (http_ota) streams an ota.bin here; this backend verifies the
 * header CRC, stages the (compressed + BL2-signed) payload byte-for-byte into
 * the single `ota` staging partition via non-secure flash writes (data-bus, not
 * blocked by MPC), verifies the payload CRC32, and on finish arms the update by
 * writing a {magic, OVERWRITE_CONFIRM, crc} record at the end of `ota_control`.
 * On the next reset BL2 reads that record (boot_validated_swap_type ->
 * BOOT_SWAP_TYPE_TEST), decompresses `ota` -> `primary_all` (boot_copy_region in
 * decompress_bl2.c), and does the EC-P256 signature check. No encryption in v1.
 *
 * ota.bin (LE, see tools partition.py gen_ota_bin_for_overwrite): 32B global
 * header ("BK723658" magic, ..., image_num) + image_num*32B image headers
 * (image_len, ..., checksum) + payload. For overwrite image_num==1; the payload
 * is the ota_signed.bin blob to be staged verbatim into `ota`.
 *
 * CRC32 must match the packer: init 0xFFFFFFFF, poly 0xEDB88320, NO final
 * inversion -> CheckSumUtils CRC32_* (not crc32_zlib), else downloads are
 * rejected. Mirrors ota_secure_xip.c; the only differences are the single
 * staging slot (`ota`) and the OVERWRITE_CONFIRM arm instead of a boot_param
 * TRIAL record.
 */

#include "sdkconfig.h"
#include <stdint.h>
#include <string.h>
#include <os/mem.h>
#include <os/str.h>
#include "driver/flash.h"
#include "driver/flash_partition.h"   /* bk_flash_partition_get_info + BK_PARTITION_OTA /
                                       * BK_PARTITION_OTA_CONTROL ids (via partitions_gen.h) */
#include "_ota.h"                     /* generated: OVERWRITE_CONFIRM magic (0xa16d8fb0) */
#include "common/bk_err.h"
#include "bk_private/bk_ota_private.h"
#include "CheckSumUtils.h"

#define SECURE_OW_GLOBAL_HDR_LEN    32u
#define SECURE_OW_IMG_HDR_LEN       32u
#define SECURE_OW_HDR_LEN           (SECURE_OW_GLOBAL_HDR_LEN + SECURE_OW_IMG_HDR_LEN)  /* img_num==1 */
#define SECURE_OW_MAGIC             "\x42\x4B\x37\x32\x33\x36\x35\x38"        /* "BK723658" */
/* The global-header crc (at offset 8) is computed over everything after
 * magic(8)+crc(4), i.e. the metadata + all image headers. See partition.py
 * gen_ota_global_hdr; for img_num==1 that range is hdr_buf[12 .. HDR_LEN). */
#define SECURE_OW_GLOBAL_CRC_OFF    12u

/* Confirm record retries. The confirm record lives at the end of ota_control,
 * resolved at runtime from BK_PARTITION_OTA_CONTROL; the offset must match BL2's
 * bk_boot_overwrite_confirm_off() (ota_control_offset + size - sizeof(record)). */
#define SECURE_OW_CONFIRM_RETRY     3u

/* On-flash confirm record. Instead of a bare confirm word we store
 * magic + confirm + crc so an erased/garbage/partial value can never be
 * mistaken for a valid "install" request. Layout, magic and CRC MUST match
 * BL2 bootutil_public.c (struct ota_confirm_rec / OTA_CONFIRM_REC_MAGIC /
 * ota_confirm_crc32). secure_ow_crc32() is the same CheckSumUtils CRC32. */
#define OTA_CONFIRM_REC_MAGIC       0x4F544143u   /* 'OTAC' record sentinel */

struct ota_confirm_rec {
	uint32_t magic;     /* OTA_CONFIRM_REC_MAGIC */
	uint32_t confirm;   /* OVERWRITE_CONFIRM to request the install */
	uint32_t crc;       /* crc32 over the first 8 bytes {magic, confirm} */
};

typedef struct __attribute__((packed)) {
	uint8_t  magic[8];
	uint32_t crc;
	uint32_t version;
	uint16_t header_len;
	uint16_t image_num;
	uint32_t flags;
	uint32_t reserved[2];
} secure_ow_global_hdr_t;

typedef struct __attribute__((packed)) {
	uint32_t image_len;
	uint32_t image_offset;
	uint32_t flash_offset;
	uint32_t checksum;
	uint32_t version;
	uint32_t flags;
	uint32_t reserved[2];
} secure_ow_img_hdr_t;

typedef char secure_ow_hdr_size_assert[(sizeof(secure_ow_global_hdr_t) == SECURE_OW_GLOBAL_HDR_LEN
	&& sizeof(secure_ow_img_hdr_t) == SECURE_OW_IMG_HDR_LEN) ? 1 : -1];

typedef enum {
	SECURE_OW_PHASE_HEADER = 0,
	SECURE_OW_PHASE_PAYLOAD,
	SECURE_OW_PHASE_DONE,
	SECURE_OW_PHASE_ERROR,
} secure_ow_phase_t;

/* Single OTA in flight: parser state is a file-scope singleton. */
typedef struct {
	secure_ow_phase_t phase;
	uint8_t       hdr_buf[SECURE_OW_HDR_LEN];
	uint32_t      hdr_recv_len;     /* header bytes received so far */
	uint32_t      payload_total;    /* image_len from the image header */
	uint32_t      payload_written;  /* payload bytes streamed to flash */
	uint32_t      expected_crc;      /* image-header checksum over payload */
	uint32_t      slot_base;         /* `ota` staging partition base address */
	uint32_t      slot_size;         /* `ota` staging partition size */
	uint32_t      confirm_off;       /* ota_control end - sizeof(rec): confirm record */
	uint32_t      ctrl_base;         /* ota_control base: BL2 resume journal sector */
	uint32_t      write_addr;        /* flash write cursor */
	int           last_log_pct;      /* last progress % printed */
	CRC32_Context crc_ctx;           /* running CRC32 over the payload */
} secure_ow_state_t;

static secure_ow_state_t s_secure_ow;

static uint32_t secure_ow_rd_u32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
	     | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* One-shot CheckSumUtils CRC32 (matches the packer); the payload uses the
 * streaming CRC32_* API directly since it is fed chunk by chunk. */
static uint32_t secure_ow_crc32(const void *buf, uint32_t len)
{
	CRC32_Context ctx;
	uint32_t out;

	CRC32_Init(&ctx);
	CRC32_Update(&ctx, buf, len);
	CRC32_Final(&ctx, &out);
	return out;
}

static int secure_ow_parse_headers(void)
{
	const secure_ow_global_hdr_t *gh = (const secure_ow_global_hdr_t *)s_secure_ow.hdr_buf;
	const secure_ow_img_hdr_t    *ih = (const secure_ow_img_hdr_t *)(s_secure_ow.hdr_buf + SECURE_OW_GLOBAL_HDR_LEN);
	uint32_t global_crc, global_calc, img_len, checksum;

	if (os_memcmp(gh->magic, SECURE_OW_MAGIC, 8) != 0) {
		OTA_LOGE("secure overwrite: bad magic\r\n");
		return BK_FAIL;
	}

	/* Verify the global-header CRC (metadata + image header) before trusting any
	 * field below; catches a corrupted or truncated header. */
	global_crc  = secure_ow_rd_u32((const uint8_t *)&gh->crc);
	global_calc = secure_ow_crc32(s_secure_ow.hdr_buf + SECURE_OW_GLOBAL_CRC_OFF,
								   SECURE_OW_HDR_LEN - SECURE_OW_GLOBAL_CRC_OFF);
	if (global_calc != global_crc) {
		OTA_LOGE("secure overwrite: header crc mismatch exp=0x%x got=0x%x\r\n", global_crc, global_calc);
		return BK_FAIL;
	}

	if (gh->image_num != 1) {
		OTA_LOGE("secure overwrite: image_num %u unsupported\r\n", gh->image_num);
		return BK_FAIL;
	}

	img_len  = secure_ow_rd_u32((const uint8_t *)&ih->image_len);
	checksum = secure_ow_rd_u32((const uint8_t *)&ih->checksum);
	if (img_len == 0 || img_len > s_secure_ow.slot_size) {
		OTA_LOGE("secure overwrite: img_len 0x%x out of ota slot 0x%x\r\n", img_len, s_secure_ow.slot_size);
		return BK_FAIL;
	}

	s_secure_ow.payload_total = img_len;
	s_secure_ow.expected_crc  = checksum;
	OTA_LOGI("secure overwrite hdr: img_len=0x%x crc=0x%x -> stage ota @0x%x\r\n",
			 img_len, checksum, s_secure_ow.slot_base);
	return BK_OK;
}

/* Backend .wr_flash: write one buffered chunk (<=1K, never crossing a 4K
 * sector) to the `ota` slot with erase-on-sector-boundary and read-back verify.
 * Registered in s_ota_secure_ow_fun and invoked via the wr_callback that ota.c
 * hands to data_process. */
static int secure_ow_wr_flash(f_ota_t *ota_ptr, uint16_t wlen)
{
	if (s_secure_ow.write_addr % FLASH_SECTOR_SIZE == 0) {
		if (bk_flash_erase_sector(s_secure_ow.write_addr) != BK_OK) {
			OTA_LOGE("secure overwrite: erase fail @0x%x\r\n", s_secure_ow.write_addr);
			return BK_FAIL;
		}
	}
	if (bk_flash_write_bytes(s_secure_ow.write_addr, ota_ptr->wr_buf, wlen) != BK_OK) {
		OTA_LOGE("secure overwrite: write fail @0x%x\r\n", s_secure_ow.write_addr);
		return BK_FAIL;
	}
	bk_flash_read_bytes(s_secure_ow.write_addr, ota_ptr->rd_buf, wlen);
	if (os_memcmp(ota_ptr->wr_buf, ota_ptr->rd_buf, wlen) != 0) {
		OTA_LOGE("secure overwrite: verify fail @0x%x len 0x%x\r\n", s_secure_ow.write_addr, wlen);
		return BK_FAIL;
	}
	s_secure_ow.write_addr += wlen;
	return BK_OK;
}

static int secure_ow_stage_payload(f_ota_t *ota_ptr, ota_wr_callback wr_flash,
									const uint8_t *payload, uint32_t len)
{
	uint32_t off = 0;

	CRC32_Update(&s_secure_ow.crc_ctx, payload, len);
	s_secure_ow.payload_written += len;

	while (off < len) {
		uint32_t buf_space = OTA_FLASH_BUFFER_LENGTH - ota_ptr->wr_last_len;
		uint32_t copy_len  = MIN(len - off, buf_space);

		os_memcpy(ota_ptr->wr_buf + ota_ptr->wr_last_len, payload + off, copy_len);
		ota_ptr->wr_last_len += copy_len;
		off += copy_len;

		if (ota_ptr->wr_last_len == OTA_FLASH_BUFFER_LENGTH) {
			if (wr_flash(ota_ptr, OTA_FLASH_BUFFER_LENGTH) != BK_OK) {
				return BK_FAIL;
			}
			ota_ptr->wr_last_len = 0;
		}
	}
	return BK_OK;
}

static void secure_ow_log_progress(void)
{
	int pct;

	if (s_secure_ow.payload_total == 0) {
		return;
	}
	pct = (int)(((uint64_t)s_secure_ow.payload_written * 100) / s_secure_ow.payload_total);
	if ((pct - s_secure_ow.last_log_pct) >= 10 || pct == 100) {
		OTA_LOGI("secure overwrite: staged %d%%\r\n", pct);
		s_secure_ow.last_log_pct = pct;
	}
}

/* Arm the install: write the {magic, confirm, crc} record at the end of
 * ota_control (erase -> write -> read-back verify, retried). Must match the
 * record BL2 reads in bk_boot_read_ota_confirm(). Direct NS data-bus write. */
static int secure_ow_arm_confirm(void)
{
	uint32_t retry = SECURE_OW_CONFIRM_RETRY;
	uint32_t off   = s_secure_ow.confirm_off;
	struct ota_confirm_rec rec;

	rec.magic   = OTA_CONFIRM_REC_MAGIC;
	rec.confirm = OVERWRITE_CONFIRM;
	rec.crc     = secure_ow_crc32(&rec, 2 * sizeof(uint32_t));

	/* Clear the BL2 resume journal (first sector) BEFORE arming so every install
	 * starts from block 0. Otherwise stale journal records from a power-failed
	 * prior install could make BL2 wrongly "resume" this fresh image. */
	if (bk_flash_erase_sector(s_secure_ow.ctrl_base) != BK_OK) {
		OTA_LOGE("secure overwrite: journal erase fail @0x%x\r\n", s_secure_ow.ctrl_base);
	}

	while (retry--) {
		struct ota_confirm_rec check;

		os_memset(&check, 0, sizeof(check));
		if (bk_flash_erase_sector(off) != BK_OK) {
			OTA_LOGE("secure overwrite: confirm erase fail @0x%x\r\n", off);
			continue;
		}
		if (bk_flash_write_bytes(off, (uint8_t *)&rec, sizeof(rec)) != BK_OK) {
			OTA_LOGE("secure overwrite: confirm write fail @0x%x\r\n", off);
			continue;
		}
		bk_flash_read_bytes(off, (uint8_t *)&check, sizeof(check));
		if (check.magic == rec.magic && check.confirm == rec.confirm &&
			check.crc == rec.crc) {
			return BK_OK;
		}
		OTA_LOGE("secure overwrite: confirm verify mismatch\r\n");
	}
	return BK_FAIL;
}

static int secure_ow_init(f_ota_t *ota_ptr)
{
	bk_logic_partition_t *ota_pt;
	bk_logic_partition_t *ctrl_pt;

	OTA_CHECK_POINTER(ota_ptr);

	OTA_MALLOC(ota_ptr->wr_buf, OTA_FLASH_BUFFER_LENGTH);
	OTA_MALLOC(ota_ptr->wr_tmp_buf, OTA_TEMP_FLASH_BUFFER_LENGTH);
	OTA_MALLOC(ota_ptr->rd_buf, OTA_FLASH_BUFFER_LENGTH);

	/* Resolve the `ota` staging and `ota_control` partitions from the flash
	 * partition table (layout-agnostic; BK7259 CRC disabled -> phys==logical). */
	ota_pt  = bk_flash_partition_get_info(BK_PARTITION_OTA);
	ctrl_pt = bk_flash_partition_get_info(BK_PARTITION_OTA_CONTROL);
	if (ota_pt == NULL || ctrl_pt == NULL) {
		OTA_LOGE("secure overwrite: ota/ota_control partition missing\r\n");
		return BK_FAIL;   /* buffers freed by deinit on the caller's failure path */
	}

	os_memset(&s_secure_ow, 0, sizeof(s_secure_ow));
	s_secure_ow.phase        = SECURE_OW_PHASE_HEADER;
	s_secure_ow.last_log_pct = -10;
	s_secure_ow.slot_base    = ota_pt->partition_start_addr;
	s_secure_ow.slot_size    = ota_pt->partition_length;
	s_secure_ow.confirm_off  = ctrl_pt->partition_start_addr + ctrl_pt->partition_length
	                           - sizeof(struct ota_confirm_rec);
	s_secure_ow.ctrl_base    = ctrl_pt->partition_start_addr;
	s_secure_ow.write_addr   = s_secure_ow.slot_base;

	ota_ptr->wr_last_len         = 0;
	ota_ptr->wr_err              = 0;
	ota_ptr->wr_flash_flag       = 0;
	ota_ptr->received_total_size = 0;
	ota_ptr->fd                  = -1;
	ota_ptr->init_flag           = 1;
	/* No explicit flash write-protect management here: the AP flash driver
	 * unprotects before each erase/program and restores afterwards internally
	 * (flash_driver.c), and it exposes no bk_flash_get/set_protect_type API. */

	OTA_LOGI("secure overwrite: stage ota @0x%x size 0x%x\r\n",
			 s_secure_ow.slot_base, s_secure_ow.slot_size);
	return BK_OK;
}

static int secure_ow_data_process(f_ota_t *ota_ptr, uint16_t len,
							 ota_update_type_t ota_type, ota_wr_callback wr_callback)
{
	uint8_t *read_ptr;
	uint32_t remaining;

	(void)ota_type;   /* transport-agnostic: same staging for every OTA type */
	OTA_CHECK_POINTER(ota_ptr);
	OTA_CHECK_POINTER(ota_ptr->wr_tmp_buf);
	OTA_CHECK_POINTER(wr_callback);   /* backend .wr_flash drives the flash write */

	if (s_secure_ow.phase == SECURE_OW_PHASE_ERROR) {
		return BK_FAIL;
	}

	read_ptr  = ota_ptr->wr_tmp_buf;
	remaining = len;

	if (s_secure_ow.phase == SECURE_OW_PHASE_HEADER) {
		uint32_t hdr_need = SECURE_OW_HDR_LEN - s_secure_ow.hdr_recv_len;
		uint32_t hdr_take = MIN(remaining, hdr_need);

		os_memcpy(s_secure_ow.hdr_buf + s_secure_ow.hdr_recv_len, read_ptr, hdr_take);
		s_secure_ow.hdr_recv_len += hdr_take;
		read_ptr  += hdr_take;
		remaining -= hdr_take;
		ota_ptr->received_total_size += hdr_take;

		if (s_secure_ow.hdr_recv_len < SECURE_OW_HDR_LEN) {
			return BK_OK;  /* header split across chunks, wait for more */
		}
		if (secure_ow_parse_headers() != BK_OK) {
			s_secure_ow.phase = SECURE_OW_PHASE_ERROR;
			return BK_FAIL;
		}
		CRC32_Init(&s_secure_ow.crc_ctx);
		s_secure_ow.phase = SECURE_OW_PHASE_PAYLOAD;
	}

	if (s_secure_ow.phase == SECURE_OW_PHASE_PAYLOAD && remaining > 0) {
		uint32_t payload_left = s_secure_ow.payload_total - s_secure_ow.payload_written;
		uint32_t payload_take = MIN(remaining, payload_left);

		if (secure_ow_stage_payload(ota_ptr, wr_callback, read_ptr, payload_take) != BK_OK) {
			s_secure_ow.phase = SECURE_OW_PHASE_ERROR;
			return BK_FAIL;
		}
		read_ptr  += payload_take;
		remaining -= payload_take;
		ota_ptr->received_total_size += payload_take;
		secure_ow_log_progress();

		if (s_secure_ow.payload_written == s_secure_ow.payload_total) {
			uint32_t crc_calc;

			if (ota_ptr->wr_last_len > 0) {  /* flush final partial chunk */
				if (wr_callback(ota_ptr, ota_ptr->wr_last_len) != BK_OK) {
					s_secure_ow.phase = SECURE_OW_PHASE_ERROR;
					return BK_FAIL;
				}
				ota_ptr->wr_last_len = 0;
			}
			CRC32_Final(&s_secure_ow.crc_ctx, &crc_calc);
			if (crc_calc != s_secure_ow.expected_crc) {
				OTA_LOGE("secure overwrite: crc mismatch exp=0x%x got=0x%x\r\n",
						 s_secure_ow.expected_crc, crc_calc);
				s_secure_ow.phase = SECURE_OW_PHASE_ERROR;
				return BK_FAIL;
			}
			s_secure_ow.phase = SECURE_OW_PHASE_DONE;
			OTA_LOGI("secure overwrite: payload verified (0x%x bytes)\r\n", s_secure_ow.payload_total);
		}
	}

	/* Any bytes past the declared payload (there should be none) are ignored. */
	return BK_OK;
}

static int secure_ow_finish(f_ota_t *ota_ptr)
{
	(void)ota_ptr;

	if (s_secure_ow.phase != SECURE_OW_PHASE_DONE) {
		OTA_LOGE("secure overwrite: incomplete (phase=%d written=0x%x/0x%x)\r\n",
				 s_secure_ow.phase, s_secure_ow.payload_written, s_secure_ow.payload_total);
		return BK_FAIL;
	}
	if (secure_ow_arm_confirm() != BK_OK) {
		OTA_LOGE("secure overwrite: arm OVERWRITE_CONFIRM failed\r\n");
		return BK_FAIL;
	}
	OTA_LOGI("secure overwrite: armed OVERWRITE_CONFIRM @0x%x, reboot to install\r\n",
			 s_secure_ow.confirm_off);
	return BK_OK;
}

static int secure_ow_deinit(f_ota_t *ota_ptr)
{
	OTA_CHECK_POINTER(ota_ptr);

	OTA_FREE(ota_ptr->wr_buf);
	OTA_FREE(ota_ptr->wr_tmp_buf);
	OTA_FREE(ota_ptr->rd_buf);
	ota_ptr->init_flag = 0;
	return BK_OK;
}

static const f_ota_func_t s_ota_secure_ow_fun = {
	.init         = secure_ow_init,
	.wr_flash     = secure_ow_wr_flash,
	.data_process = secure_ow_data_process,
	.crc          = NULL,             /* verification is folded into data_process */
	.deinit       = secure_ow_deinit,
	.finish       = secure_ow_finish,
};

const f_ota_func_t *bk_ota_secure_overwrite_backend(void)
{
	return &s_ota_secure_ow_fun;
}
