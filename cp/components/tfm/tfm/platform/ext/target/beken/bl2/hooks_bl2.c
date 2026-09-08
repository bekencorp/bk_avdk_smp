// Copyright 2022-2023 Beken
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

#include <assert.h>
#include <string.h>
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"
#include "components/log.h"
#include "bk_tfm_log.h"
#include "security.h"

#define TAG "hook"
#define BL2_HOOK_LOGD BK_LOGD
#define BL2_HOOK_LOGI BK_LOGD
#define BL2_HOOK_LOGW BK_LOGD
#define BL2_HOOK_LOGE BK_LOGD
/* No BK_LOG_FORCE in BL2; same style as boot_param BP_FORCE. */
#define BL2_HOOK_LOGF BK_LOGI
#define BL2_HOOK_DEBUG 0

#define BL2_HOOK_IMG_READ_DEBUG_LEN 0x100

static void dump_image_header(struct image_header *hdr)
{
	BL2_HOOK_LOGF(TAG, "magic=%x\r\n", hdr->ih_magic);
	BL2_HOOK_LOGD(TAG, "load_addr=%x\r\n", hdr->ih_load_addr);
	BL2_HOOK_LOGD(TAG, "hdr_size=%x\r\n", hdr->ih_hdr_size);
	BL2_HOOK_LOGD(TAG, "protect_tlv_size=%x\r\n", hdr->ih_protect_tlv_size);
	BL2_HOOK_LOGD(TAG, "img_size=%x\r\n", hdr->ih_img_size);
	BL2_HOOK_LOGD(TAG, "flags=%x\r\n", hdr->ih_flags);
	BL2_HOOK_LOGD(TAG, "pad=%x\r\n", hdr->_pad1);
}

static inline bool boot_u32_safe_add(uint32_t *dest, uint32_t a, uint32_t b)
{
	if (a > UINT32_MAX - b) {
		return false;
	} else {
		*dest = a + b;
		return true;
	}
}

static bool
boot_is_header_valid(const struct image_header *hdr, const struct flash_area *fap)
{
    uint32_t size;

    if (hdr->ih_magic != IMAGE_MAGIC) {
        BL2_HOOK_LOGE(TAG, "bad magic: %x\r\n", hdr->ih_magic);
        return false;
    }   

    if (!boot_u32_safe_add(&size, hdr->ih_img_size, hdr->ih_hdr_size)) {
        BL2_HOOK_LOGE(TAG, "invalid size: size=%x, img_size=%d, hdr_size=%x\r\n", size, hdr->ih_img_size, hdr->ih_hdr_size);
        return false;
    }
	BL2_HOOK_LOGD(TAG, "BB2: area_size:%x\r\n", flash_area_get_size(fap));
    if (size >= flash_area_get_size(fap)) {
        BL2_HOOK_LOGE(TAG, "invalid size: size=%x, area_size=%x\r\n", size, fap->fa_size);
        return false;
    }   

    return true;
}

uint32_t boot_get_1st_instruction_physical_off(const struct flash_area *area)
{
        uint32_t code_partition_phy_off = area->fa_off + BL2_HEADER_SIZE;
        uint32_t first_instruction_virtual_off = FLASH_PHY2VIRTUAL_CODE_START(code_partition_phy_off);
        uint32_t first_instruction_physical_off = FLASH_VIRTUAL2PHY(first_instruction_virtual_off);

        BL2_HOOK_LOGD(TAG, "code_partition_off=%x, 1st_instruction_physical_off=%x\r\n",
                code_partition_phy_off, first_instruction_physical_off);
        return first_instruction_physical_off;
}

uint32_t boot_get_1st_instruction_virtual_off(const struct flash_area *area)
{
	uint32_t code_partition_phy_off = area->fa_off + BL2_HEADER_SIZE;
	uint32_t first_instruction_virtual_off = FLASH_PHY2VIRTUAL_CODE_START(code_partition_phy_off);

	BL2_HOOK_LOGD(TAG, "code_partition_off=%x, 1st_instruction_virtual_off=%x, header=%x\r\n",
		code_partition_phy_off, first_instruction_virtual_off, BL2_HEADER_SIZE);
	return first_instruction_virtual_off;
}

uint32_t boot_get_img_padding_len(const struct flash_area *area)
{
	uint32_t padding_len = boot_get_1st_instruction_physical_off(area) - area->fa_off - BL2_HEADER_SIZE;
	return padding_len;
}

uint32_t boot_get_off_with_padding(const struct flash_area *area, uint32_t off)
{
	if ((off < BL2_HEADER_SIZE) || (off >= (area->fa_size - BL2_TRAILER_SIZE))) {
		return off;
	}

	return (off + boot_get_img_padding_len(area));
}
 
uint32_t boot_tlv_off(const struct flash_area *fap, const struct image_header *hdr)
{
	return boot_get_1st_instruction_virtual_off(fap) + hdr->ih_img_size;
}  

int boot_read_tlv_data(const struct flash_area *fap, uint32_t virtual_off, uint8_t* data, int size)
{
        BL2_HOOK_LOGD(TAG, "LOAD fa_off=%x, start=%x, size=%x\r\n", fap->fa_off, virtual_off, size);
        memcpy(data, (void*)(SOC_FLASH_DATA_BASE + virtual_off), size);
	BK_TFM_DUMP_BUF("tlv", data, size);
	return 0;
}

#if CONFIG_DIRECT_XIP
/* Flash packing: every 32 data bytes + 2-byte BE CRC16 (poly 0x8005).
 * CBUS auto-checks CRC and can hang on mismatch; probe via DBUS first.
 * Do NOT return BOOT_EFLASH here — it equals BOOT_HOOK_REGULAR (1). */
#define BL2_FLASH_CRC_UNIT 32
#define BL2_FLASH_CRC_PKT  34

static uint16_t bl2_beken_flash_crc16(const uint8_t *data, uint32_t length)
{
	uint32_t crc = 0xFFFFFFFFu;
	uint32_t i;
	int j;

	for (i = 0; i < length; i++) {
		crc ^= ((uint32_t)data[i]) << 8;
		for (j = 0; j < 8; j++) {
			if (crc & 0x8000u) {
				crc = (crc << 1) ^ 0x8005u;
			} else {
				crc <<= 1;
			}
		}
	}
	return (uint16_t)(crc & 0xFFFFu);
}

static int bl2_header_flash_crc_precheck(const struct flash_area *fap, int slot)
{
	uint8_t pkt[BL2_FLASH_CRC_PKT];
	uint16_t expect;
	uint16_t got;
	/* Packer pads 0xFF from fa_off up to CEIL_ALIGN_34(fa_off); CRC image
	 * and CBUS logical offset 0 both start at the aligned physical address.
	 * DBUS uses physical bytes — do NOT apply FLASH_PHY2VIRTUAL here. */
	uint32_t phy_img = CEIL_ALIGN_34(fap->fa_off);
	uint32_t off = phy_img - fap->fa_off;

	if (flash_area_read_dbus(fap, off, pkt, sizeof(pkt)) != 0) {
		BK_LOGE(TAG, "s%d dbus hdr read fail off=%x\r\n", slot, off);
		return BOOT_EBADIMAGE;
	}

	expect = bl2_beken_flash_crc16(pkt, BL2_FLASH_CRC_UNIT);
	got = ((uint16_t)pkt[BL2_FLASH_CRC_UNIT] << 8) |
	      (uint16_t)pkt[BL2_FLASH_CRC_UNIT + 1];
	if (expect != got) {
		BK_LOGE(TAG, "s%d dbus hdr CRC fail off=%x expect=%04x got=%04x\r\n",
			slot, off, expect, got);
		return BOOT_EBADIMAGE;
	}
	return 0;
}
#endif /* CONFIG_DIRECT_XIP */

/* @retval 0: header was read/populated
 *         FIH_FAILURE: image is invalid,
 *         BOOT_HOOK_REGULAR if hook not implemented for the image-slot,
 *         othervise an error-code value.
 */
int boot_read_image_header_hook(int img_index, int slot,
                                struct image_header *img_hed)
{
	const struct flash_area *fap;
	int area_id = 0;
	int rc;

	area_id = flash_area_id_from_multi_image_slot(img_index, slot);
	rc = flash_area_open(area_id, &fap);
	if (rc != 0) {
		BL2_HOOK_LOGE(TAG, "failed to open flash\r\n");
		return BOOT_EBADIMAGE;
	}

	/* Soft-CRC via DBUS; only then CBUS (AES decrypt) for plaintext hdr. */
	rc = bl2_header_flash_crc_precheck(fap, slot);
	if (rc != 0) {
		memset(img_hed, 0, sizeof(*img_hed));
		flash_area_close(fap);
		return rc;
	}

	rc = flash_area_read(fap, 0, img_hed, sizeof(*img_hed));
	if (rc != 0) {
		BL2_HOOK_LOGE(TAG, "cbus hdr read fail slot=%d\r\n", slot);
		memset(img_hed, 0, sizeof(*img_hed));
		flash_area_close(fap);
		return BOOT_EBADIMAGE;
	}

	if (boot_is_header_valid(img_hed, fap) == false) {
		BL2_HOOK_LOGE(TAG, "bad hdr s=%d\r\n", slot);
		memset(img_hed, 0, sizeof(*img_hed));
		flash_area_close(fap);
		return BOOT_EBADIMAGE;
	}

	BL2_HOOK_LOGD(TAG, "read image=%d, slot=%d hdr\r\n", img_index, slot);
	dump_image_header(img_hed);
	BL2_HOOK_LOGD(TAG, "tlv off=%x\r\n", boot_tlv_off(fap, img_hed));
	flash_area_close(fap);
	return 0;
}

/* @retval FIH_SUCCESS: image is valid,
 *         FIH_FAILURE: image is invalid,
 *         fih encoded BOOT_HOOK_REGULAR if hook not implemented for
 *         the image-slot.
 */
fih_int boot_image_check_hook(int img_index, int slot)
{
	FIH_RET(fih_int_encode(BOOT_HOOK_REGULAR));
}

int boot_perform_update_hook(int img_index, struct image_header *img_head,
                             const struct flash_area *area)
{
	return BOOT_HOOK_REGULAR;
}

static void dump_swap_state(int image_index, struct boot_swap_state *state)

{
	BL2_HOOK_LOGD(TAG, "magic=%x\r\n", state->magic);
	BL2_HOOK_LOGD(TAG, "swap_type=%x\r\n", state->swap_type);
	BL2_HOOK_LOGD(TAG, "image_num=%x\r\n", state->image_num);
	BL2_HOOK_LOGD(TAG, "copy_done=%x\r\n", state->copy_done);
	BL2_HOOK_LOGD(TAG, "image_ok=%x\r\n", state->image_ok);
}
	
int boot_read_swap_state_primary_slot_hook(int image_index,
                                           struct boot_swap_state *state)
{
	dump_swap_state(image_index, state);
	return BOOT_HOOK_REGULAR;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area,
                               size_t size)
{
	return 0;
}

int boot_serial_uploaded_hook(int img_index, const struct flash_area *area,
                               size_t size)
{
	return 0;
}

int boot_img_install_stat_hook(int image_index, int slot, int *img_install_stat)
{
	return BOOT_HOOK_REGULAR;
}

int flash_area_read_post_hook(const struct flash_area *area, uint32_t off, void *dst, uint32_t len)
{
#if BL2_HOOK_DEBUG
	if ((len > 0) && (len < BL2_HOOK_IMG_READ_DEBUG_LEN)) {
		uint8_t *buf = (uint8_t*)dst;

        	BK_LOG_RAW("area%d read: flash_off=%x off=%x len=%x\r\n", area->fa_id, area->fa_off, off, len);
		for (int i = 0; i < len; i++) {
			BK_LOG_RAW("%02x ", buf[i]);

			if (i && (i % 16 == 0)) {
				BK_LOG_RAW("\r\n");
			}
		}

        	BK_LOG_RAW("\r\n");
	}
#endif

	return 0;
}
