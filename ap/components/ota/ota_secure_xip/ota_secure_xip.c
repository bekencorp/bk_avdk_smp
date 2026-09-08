/*
 * ota_secure_xip.c - Secure DIRECT_XIP A/B OTA backend (non-secure world).
 *
 * Plugs into the non-secure OTA framework as an f_ota_func_t back-end
 * (bk_ota_secure_xip_backend()): the transport streams an ota.bin here; this
 * backend verifies the header CRC, stages the ciphertext payload into the
 * inactive XIP slot via non-secure flash writes (data-bus, not blocked by MPC),
 * verifies the payload CRC32, and arms a boot_param TRIAL record. BL2 does the
 * EC-P256 signature check + rollback on next boot.
 *
 * ota.bin (LE, see tools partition.py gen_ota_bin_for_xip): 32B global header
 * ("BK723658" magic, ..., image_num) + image_num*32B image headers (image_len,
 * ..., checksum) + payload. For XIP image_num==1; payload is XTS-AES ciphertext
 * valid in either slot (BL2 remap shares the tweak).
 *
 * CRC32 must match the packer: init 0xFFFFFFFF, poly 0xEDB88320, NO final
 * inversion -> CheckSumUtils CRC32_* (not crc32_zlib), else downloads are rejected.
 */

#include "sdkconfig.h"
#include <stdint.h>
#include <string.h>
#include <os/mem.h>
#include <os/str.h>
#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "modules/ota.h"            /* bk_ota_get_current_partition() */
#include "common/bk_err.h"
#include "bk_private/bk_ota_private.h"
#include "CheckSumUtils.h"
#include "ota_boot_param.h"

#define SECURE_XIP_GLOBAL_HDR_LEN   32u
#define SECURE_XIP_IMG_HDR_LEN      32u
#define SECURE_XIP_HDR_LEN          (SECURE_XIP_GLOBAL_HDR_LEN + SECURE_XIP_IMG_HDR_LEN)  /* img_num==1 */
#define SECURE_XIP_MAGIC            "\x42\x4B\x37\x32\x33\x36\x35\x38"        /* "BK723658" */
/* The global-header crc (at offset 8) is computed over everything after
 * magic(8)+crc(4), i.e. the metadata + all image headers. See partition.py
 * gen_ota_global_hdr; for img_num==1 that range is hdr_buf[12 .. HDR_LEN). */
#define SECURE_XIP_GLOBAL_CRC_OFF   12u

typedef struct __attribute__((packed)) {
	uint8_t  magic[8];
	uint32_t crc;
	uint32_t version;
	uint16_t header_len;
	uint16_t image_num;
	uint32_t flags;
	uint32_t reserved[2];
} secure_xip_global_hdr_t;

typedef struct __attribute__((packed)) {
	uint32_t image_len;
	uint32_t image_offset;
	uint32_t flash_offset;
	uint32_t checksum;
	uint32_t version;
	uint32_t flags;
	uint32_t reserved[2];
} secure_xip_img_hdr_t;

typedef char secure_xip_hdr_size_assert[(sizeof(secure_xip_global_hdr_t) == SECURE_XIP_GLOBAL_HDR_LEN
	&& sizeof(secure_xip_img_hdr_t) == SECURE_XIP_IMG_HDR_LEN) ? 1 : -1];

typedef enum {
	SECURE_XIP_PHASE_HEADER = 0,
	SECURE_XIP_PHASE_PAYLOAD,
	SECURE_XIP_PHASE_DONE,
	SECURE_XIP_PHASE_ERROR,
} secure_xip_phase_t;

/* Single OTA in flight: parser state is a file-scope singleton. */
typedef struct {
	secure_xip_phase_t phase;
	uint8_t       hdr_buf[SECURE_XIP_HDR_LEN];
	uint32_t      hdr_recv_len;     /* header bytes received so far */
	uint32_t      payload_total;    /* image_len from the image header */
	uint32_t      payload_written;  /* payload bytes streamed to flash */
	uint32_t      expected_crc;      /* image-header checksum over payload */
	uint8_t       update_slot;       /* target (inactive) slot index, 0=A/1=B */
	uint32_t      update_slot_base;  /* target slot flash base address */
	uint32_t      update_slot_size;  /* target slot size in bytes */
	uint32_t      write_addr;        /* flash write cursor */
	int           last_log_pct;      /* last progress % printed */
	CRC32_Context crc_ctx;           /* running CRC32 over the payload */
} secure_xip_state_t;

static secure_xip_state_t s_secure_xip;

static uint32_t secure_xip_rd_u32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
	     | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* One-shot CheckSumUtils CRC32 (matches the packer); payload uses the streaming
 * CRC32_* API directly since it is fed chunk by chunk. */
static uint32_t secure_xip_crc32(const void *buf, uint32_t len)
{
	CRC32_Context ctx;
	uint32_t out;

	CRC32_Init(&ctx);
	CRC32_Update(&ctx, buf, len);
	CRC32_Final(&ctx, &out);
	return out;
}

static int secure_xip_parse_headers(void)
{
	const secure_xip_global_hdr_t *gh = (const secure_xip_global_hdr_t *)s_secure_xip.hdr_buf;
	const secure_xip_img_hdr_t    *ih = (const secure_xip_img_hdr_t *)(s_secure_xip.hdr_buf + SECURE_XIP_GLOBAL_HDR_LEN);
	uint32_t global_crc, global_calc, img_len, checksum;

	if (os_memcmp(gh->magic, SECURE_XIP_MAGIC, 8) != 0) {
		OTA_LOGE("secure xip: bad magic\r\n");
		return BK_FAIL;
	}

	/* Verify the global-header CRC (metadata + image header) before trusting any
	 * field below; catches a corrupted or truncated header. */
	global_crc  = secure_xip_rd_u32((const uint8_t *)&gh->crc);
	global_calc = secure_xip_crc32(s_secure_xip.hdr_buf + SECURE_XIP_GLOBAL_CRC_OFF,
								   SECURE_XIP_HDR_LEN - SECURE_XIP_GLOBAL_CRC_OFF);
	if (global_calc != global_crc) {
		OTA_LOGE("secure xip: header crc mismatch exp=0x%x got=0x%x\r\n", global_crc, global_calc);
		return BK_FAIL;
	}

	if (gh->image_num != 1) {
		OTA_LOGE("secure xip: image_num %u unsupported\r\n", gh->image_num);
		return BK_FAIL;
	}

	img_len  = secure_xip_rd_u32((const uint8_t *)&ih->image_len);
	checksum = secure_xip_rd_u32((const uint8_t *)&ih->checksum);
	if (img_len == 0 || img_len > s_secure_xip.update_slot_size) {
		OTA_LOGE("secure xip: img_len 0x%x out of slot 0x%x\r\n", img_len, s_secure_xip.update_slot_size);
		return BK_FAIL;
	}

	s_secure_xip.payload_total = img_len;
	s_secure_xip.expected_crc  = checksum;
	OTA_LOGI("secure xip hdr: img_len=0x%x crc=0x%x -> stage slot %u @0x%x\r\n",
			 img_len, checksum, s_secure_xip.update_slot, s_secure_xip.update_slot_base);
	return BK_OK;
}

/* Backend .wr_flash: write one buffered chunk (<=1K, never crossing a 4K
 * sector) to the inactive slot with erase-on-sector-boundary and read-back
 * verify. Registered in s_ota_secure_xip_fun and invoked via the wr_callback
 * that ota.c hands to data_process. */
static int secure_xip_wr_flash(f_ota_t *ota_ptr, uint16_t wlen)
{
	if (s_secure_xip.write_addr % FLASH_SECTOR_SIZE == 0) {
		if (bk_flash_erase_sector(s_secure_xip.write_addr) != BK_OK) {
			OTA_LOGE("secure xip: erase fail @0x%x\r\n", s_secure_xip.write_addr);
			return BK_FAIL;
		}
	}
	if (bk_flash_write_bytes(s_secure_xip.write_addr, ota_ptr->wr_buf, wlen) != BK_OK) {
		OTA_LOGE("secure xip: write fail @0x%x\r\n", s_secure_xip.write_addr);
		return BK_FAIL;
	}
	bk_flash_read_bytes(s_secure_xip.write_addr, ota_ptr->rd_buf, wlen);
	if (os_memcmp(ota_ptr->wr_buf, ota_ptr->rd_buf, wlen) != 0) {
		OTA_LOGE("secure xip: verify fail @0x%x len 0x%x\r\n", s_secure_xip.write_addr, wlen);
		return BK_FAIL;
	}
	s_secure_xip.write_addr += wlen;
	return BK_OK;
}

static int secure_xip_stage_payload(f_ota_t *ota_ptr, ota_wr_callback wr_flash,
									const uint8_t *payload, uint32_t len)
{
	uint32_t off = 0;

	CRC32_Update(&s_secure_xip.crc_ctx, payload, len);
	s_secure_xip.payload_written += len;

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

static void secure_xip_log_progress(void)
{
	int pct;

	if (s_secure_xip.payload_total == 0) {
		return;
	}
	pct = (int)(((uint64_t)s_secure_xip.payload_written * 100) / s_secure_xip.payload_total);
	/* Coarse 10% steps for the bulk of the download; switch to fine 1% steps from
	 * 95% onward so testers get frequent feedback near the end (95/96/.../100). */
	int step = (pct >= 95) ? 1 : 10;
	if ((pct - s_secure_xip.last_log_pct) >= step || pct == 100) {
		OTA_LOGI("secure xip: staged %d%%\r\n", pct);
		s_secure_xip.last_log_pct = pct;
	}
}

/* Resolve the staging (inactive) slot into s_secure_xip: it is the slot opposite
 * the one currently running (HW XIP remap). The two XIP slots are contiguous and
 * equal-sized, so the gap between their bases is the per-slot size.
 * @return BK_OK, filling *running with the live slot; BK_FAIL if partitions miss. */
static int secure_xip_resolve_slot(uint8_t *running)
{
	bk_logic_partition_t *primary   = bk_flash_partition_get_info(BK_PARTITION_PRIMARY_TFM_S);
	bk_logic_partition_t *secondary = bk_flash_partition_get_info(BK_PARTITION_SECONDARY_TFM_S);
	uint8_t run, inactive;

	if (primary == NULL || secondary == NULL) {
		OTA_LOGE("secure xip: slot partitions missing\r\n");
		return BK_FAIL;
	}

	run      = (uint8_t)(bk_ota_get_current_partition() & 0x1);   /* 0=A, 1=B */
	inactive = run ^ 1u;

	s_secure_xip.update_slot      = inactive;
	s_secure_xip.update_slot_size = secondary->partition_start_addr - primary->partition_start_addr;
	s_secure_xip.update_slot_base = (inactive == 0) ? primary->partition_start_addr
													: secondary->partition_start_addr;
	*running = run;
	return BK_OK;
}

static int secure_xip_init(f_ota_t *ota_ptr)
{
	uint8_t running;

	OTA_CHECK_POINTER(ota_ptr);

	OTA_MALLOC(ota_ptr->wr_buf, OTA_FLASH_BUFFER_LENGTH);
	OTA_MALLOC(ota_ptr->wr_tmp_buf, OTA_TEMP_FLASH_BUFFER_LENGTH);
	OTA_MALLOC(ota_ptr->rd_buf, OTA_FLASH_BUFFER_LENGTH);

	os_memset(&s_secure_xip, 0, sizeof(s_secure_xip));
	s_secure_xip.phase        = SECURE_XIP_PHASE_HEADER;
	s_secure_xip.last_log_pct = -10;
	if (secure_xip_resolve_slot(&running) != BK_OK) {
		return BK_FAIL;
	}
	s_secure_xip.write_addr = s_secure_xip.update_slot_base;

	ota_ptr->wr_last_len         = 0;
	ota_ptr->wr_err              = 0;
	ota_ptr->wr_flash_flag       = 0;
	ota_ptr->received_total_size = 0;
	ota_ptr->fd                  = -1;
	ota_ptr->init_flag           = 1;

	OTA_LOGI("secure xip: run slot %u -> stage slot %u @0x%x size 0x%x\r\n",
			 running, s_secure_xip.update_slot,
			 s_secure_xip.update_slot_base, s_secure_xip.update_slot_size);
	return BK_OK;
}

static int secure_xip_data_process(f_ota_t *ota_ptr, uint16_t len,
							 ota_update_type_t ota_type, ota_wr_callback wr_callback)
{
	uint8_t *read_ptr;
	uint32_t remaining;

	(void)ota_type;   /* transport-agnostic: same staging for every OTA type */
	OTA_CHECK_POINTER(ota_ptr);
	OTA_CHECK_POINTER(ota_ptr->wr_tmp_buf);
	OTA_CHECK_POINTER(wr_callback);   /* backend .wr_flash drives the flash write */

	if (s_secure_xip.phase == SECURE_XIP_PHASE_ERROR) {
		return BK_FAIL;
	}

	read_ptr  = ota_ptr->wr_tmp_buf;
	remaining = len;

	if (s_secure_xip.phase == SECURE_XIP_PHASE_HEADER) {
		uint32_t hdr_need = SECURE_XIP_HDR_LEN - s_secure_xip.hdr_recv_len;
		uint32_t hdr_take = MIN(remaining, hdr_need);

		os_memcpy(s_secure_xip.hdr_buf + s_secure_xip.hdr_recv_len, read_ptr, hdr_take);
		s_secure_xip.hdr_recv_len += hdr_take;
		read_ptr  += hdr_take;
		remaining -= hdr_take;
		ota_ptr->received_total_size += hdr_take;

		if (s_secure_xip.hdr_recv_len < SECURE_XIP_HDR_LEN) {
			return BK_OK;  /* header split across chunks, wait for more */
		}
		if (secure_xip_parse_headers() != BK_OK) {
			s_secure_xip.phase = SECURE_XIP_PHASE_ERROR;
			return BK_FAIL;
		}
		CRC32_Init(&s_secure_xip.crc_ctx);
		s_secure_xip.phase = SECURE_XIP_PHASE_PAYLOAD;
	}

	if (s_secure_xip.phase == SECURE_XIP_PHASE_PAYLOAD && remaining > 0) {
		uint32_t payload_left = s_secure_xip.payload_total - s_secure_xip.payload_written;
		uint32_t payload_take = MIN(remaining, payload_left);

		if (secure_xip_stage_payload(ota_ptr, wr_callback, read_ptr, payload_take) != BK_OK) {
			s_secure_xip.phase = SECURE_XIP_PHASE_ERROR;
			return BK_FAIL;
		}
		read_ptr  += payload_take;
		remaining -= payload_take;
		ota_ptr->received_total_size += payload_take;
		secure_xip_log_progress();

		if (s_secure_xip.payload_written == s_secure_xip.payload_total) {
			uint32_t crc_calc;

			if (ota_ptr->wr_last_len > 0) {  /* flush final partial chunk */
				if (wr_callback(ota_ptr, ota_ptr->wr_last_len) != BK_OK) {
					s_secure_xip.phase = SECURE_XIP_PHASE_ERROR;
					return BK_FAIL;
				}
				ota_ptr->wr_last_len = 0;
			}
			CRC32_Final(&s_secure_xip.crc_ctx, &crc_calc);
			if (crc_calc != s_secure_xip.expected_crc) {
				OTA_LOGE("secure xip: crc mismatch exp=0x%x got=0x%x\r\n",
						 s_secure_xip.expected_crc, crc_calc);
				s_secure_xip.phase = SECURE_XIP_PHASE_ERROR;
				return BK_FAIL;
			}
			s_secure_xip.phase = SECURE_XIP_PHASE_DONE;
			OTA_LOGI("secure xip: payload verified (0x%x bytes)\r\n", s_secure_xip.payload_total);
		}
	}

	/* Any bytes past the declared payload (there should be none) are ignored. */
	return BK_OK;
}

static int secure_xip_finish(f_ota_t *ota_ptr)
{
	(void)ota_ptr;

	if (s_secure_xip.phase != SECURE_XIP_PHASE_DONE) {
		OTA_LOGE("secure xip: incomplete (phase=%d written=0x%x/0x%x)\r\n",
				 s_secure_xip.phase, s_secure_xip.payload_written, s_secure_xip.payload_total);
		return BK_FAIL;
	}
	if (ota_boot_param_set_trial(s_secure_xip.update_slot) != BK_OK) {
		OTA_LOGE("secure xip: arm trial failed\r\n");
		return BK_FAIL;
	}
	OTA_LOGI("secure xip: armed trial for slot %u\r\n", s_secure_xip.update_slot);
	return BK_OK;
}

static int secure_xip_deinit(f_ota_t *ota_ptr)
{
	OTA_CHECK_POINTER(ota_ptr);

	OTA_FREE(ota_ptr->wr_buf);
	OTA_FREE(ota_ptr->wr_tmp_buf);
	OTA_FREE(ota_ptr->rd_buf);
	ota_ptr->init_flag = 0;
	return BK_OK;
}

static const f_ota_func_t s_ota_secure_xip_fun = {
	.init         = secure_xip_init,
	.wr_flash     = secure_xip_wr_flash,
	.data_process = secure_xip_data_process,
	.crc          = NULL,             /* verification is folded into data_process */
	.deinit       = secure_xip_deinit,
	.finish       = secure_xip_finish,
};

const f_ota_func_t *bk_ota_secure_xip_backend(void)
{
	return &s_ota_secure_xip_fun;
}
