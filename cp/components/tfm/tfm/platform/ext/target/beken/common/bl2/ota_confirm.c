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

/* Shared compressed-overwrite confirm back-end (see ota_confirm.h). Compiled
 * into both platform_bl2 (BL2/MCUboot) and platform_s (TF-M SPE) from one
 * source, mirroring boot_param_ops.c, so BL2 and the SPE confirm path cannot
 * drift. Uses only flash primitives that exist in both worlds (bk_flash_*,
 * bk_flash_erase_sector, line-mode); erase is deliberately bk_flash_erase_sector
 * (not the BL2-only flash_area_erase_fast). */

/* partitions_gen.h -> _ota.h: CONFIG_OTA_CONFIRM_UPDATE, OVERWRITE_CONFIRM and
 * CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET. Kept outside the guard so the whole
 * unit compiles to nothing when the feature is off. */
#include "partitions_gen.h"

#if CONFIG_OTA_CONFIRM_UPDATE

#include <string.h>
#include "ota_confirm.h"
#include "tfm_flash_partition.h"
#include <driver/flash.h>

/* Flash must drop from QUAD continuous-read to TWO line mode or the op_sw
 * erase/write is silently ignored (same constraint as boot_param_confirm).
 * bk_flash_erase_sector + line-mode live in the flash driver / armino_min and
 * are extern'd (like boot_param_ops.c) to avoid pulling BL2-only headers into
 * the SPE build. bk_flash_read_bytes/write_bytes come from <driver/flash.h>. */
extern bk_err_t bk_flash_erase_sector(uint32_t address);
extern void bk_flash_min_unprotect_once(void);
extern void bk_flash_min_switch_line_mode_two(void);
extern void bk_flash_min_restore_line_mode(void);

/* Logging: only BL2 (MCUboot) has BOOT_LOG; the SDK log path is unsafe from the
 * SPE, so the shared build logs nothing there (matches boot_param_ops.c). */
#if defined(CONFIG_ENABLE_MCUBOOT_BL2)
#include "bootutil/bootutil_log.h"
#define OTA_CONFIRM_INF(...) BOOT_LOG_INF(__VA_ARGS__)
#define OTA_CONFIRM_ERR(...) BOOT_LOG_ERR(__VA_ARGS__)
#define OTA_CONFIRM_FORCE(...) BOOT_LOG_FORCE(__VA_ARGS__)
#else
#define OTA_CONFIRM_INF(...) ((void)0)
#define OTA_CONFIRM_ERR(...) ((void)0)
#define OTA_CONFIRM_FORCE(...) ((void)0)
#endif

#define OTA_CTRL_SECTOR_SIZE    0x1000u       /* one 4KB flash sector */
#define OTA_CONFIRM_REC_MAGIC   0x4F544143u   /* 'OTAC' record sentinel */

struct ota_confirm_rec {
    uint32_t magic;     /* OTA_CONFIRM_REC_MAGIC */
    uint32_t confirm;   /* OVERWRITE_CONFIRM to request the install */
    uint32_t crc;       /* crc32 over the first 8 bytes {magic, confirm} */
};

/* CRC32 identical to CheckSumUtils CRC32_* used by the AP writer: reflected
 * poly 0xEDB88320, init 0xFFFFFFFF, NO final inversion. Bitwise so this file
 * needs no table/driver dependency and links cleanly into both worlds. */
static uint32_t ota_confirm_crc32(const void *buf, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
        }
    }
    return crc;
}

uint32_t bk_boot_overwrite_confirm_off(void)
{
    uint32_t base = partition_get_phy_offset(PARTITION_OTA_CONTROL);
    uint32_t size = partition_get_phy_size(PARTITION_OTA_CONTROL);

    if (base == 0 || size < sizeof(struct ota_confirm_rec)) {
        return 0;
    }
    return base + size - sizeof(struct ota_confirm_rec);
}

bool bk_boot_read_ota_confirm(uint32_t value)
{
    uint32_t phy_off = bk_boot_overwrite_confirm_off();
    struct ota_confirm_rec rec = {0};

    if (phy_off == 0) {
        OTA_CONFIRM_ERR("no valid ota confirm, skip");
        return false;
    }
    bk_flash_read_bytes(phy_off, (uint8_t *)&rec, sizeof(rec));
    if (rec.magic != OTA_CONFIRM_REC_MAGIC ||
        rec.crc != ota_confirm_crc32(&rec, 2 * sizeof(uint32_t))) {
        OTA_CONFIRM_ERR("no valid ota confirm record (magic=%#x)", rec.magic);
        return false;
    }
    OTA_CONFIRM_FORCE("get ota confirm=%#x", rec.confirm);
    return (rec.confirm == value);
}

int bk_boot_write_ota_confirm(uint32_t value)
{
    uint32_t phy_off = bk_boot_overwrite_confirm_off();
    uint32_t sector;
    struct ota_confirm_rec rec;
    uint8_t retry = 3;

    if (phy_off == 0 || phy_off < CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET) {
        OTA_CONFIRM_ERR("set ota confirm: no valid ota_control");
        return BK_FAIL;
    }
    rec.magic   = OTA_CONFIRM_REC_MAGIC;
    rec.confirm = value;
    rec.crc     = ota_confirm_crc32(&rec, 2 * sizeof(uint32_t));
    /* Confirm record lives in the last ota_control sector; erase that whole
     * sector (never the first, which holds the resume journal) before writing. */
    sector = phy_off & ~(OTA_CTRL_SECTOR_SIZE - 1u);

    bk_flash_min_unprotect_once();
    bk_flash_min_switch_line_mode_two();
    while (retry--) {
        struct ota_confirm_rec check = {0};

        bk_flash_erase_sector(sector);
        bk_flash_write_bytes(phy_off, (const uint8_t *)&rec, sizeof(rec));
        bk_flash_read_bytes(phy_off, (uint8_t *)&check, sizeof(check));
        if (check.magic == rec.magic && check.confirm == rec.confirm &&
            check.crc == rec.crc) {
            bk_flash_min_restore_line_mode();
            OTA_CONFIRM_INF("set ota confirm=%#x", value);
            return BK_OK;
        }
    }
    bk_flash_min_restore_line_mode();
    OTA_CONFIRM_ERR("set ota confirm=%#x fail", value);
    return BK_FAIL;
}

void bk_ota_confirm_clear_if_armed(void)
{
    uint32_t phy_off = bk_boot_overwrite_confirm_off();
    uint32_t sector;

    if (phy_off == 0 || phy_off < CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET) {
        return;
    }
    if (!bk_boot_read_ota_confirm(OVERWRITE_CONFIRM)) {
        return;
    }
    sector = phy_off & ~(OTA_CTRL_SECTOR_SIZE - 1u);

    bk_flash_min_unprotect_once();
    bk_flash_min_switch_line_mode_two();
    bk_flash_erase_sector(sector);
    bk_flash_min_restore_line_mode();
}

#endif /* CONFIG_OTA_CONFIRM_UPDATE */
