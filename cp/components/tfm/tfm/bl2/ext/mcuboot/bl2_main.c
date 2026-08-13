/*
 * Copyright (c)     2023-2028 Wind River Systems, Inc.
 * Copyright (c)     2023-2028 Arm Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mcuboot_config/mcuboot_config.h"
#include <assert.h>
#include "target.h"
#include "tfm_hal_device_header.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "bootutil/security_cnt.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/boot_record.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"
#include "boot_hal.h"
#include "uart_stdout.h"
#include "tfm_plat_otp.h"
#include "tfm_plat_provisioning.h"
#include "sdkconfig.h"
#include "partitions_gen.h"
#include "flash_partition.h"
#include "aon_pmu_hal.h"
#include <modules/pm.h>
#include "bk_efuse.h"
#include "hal_hw_fih.h"
#include "hal_sw_fih.h"
#include "hal_efuse.h"
#include "ckmn.h"
#include "anti_tamper.h"
#include "io_matrix_driver.h"
#include "mpu.h"
#include "adc.h"
#include "cache.h"
#include "tfm_flash_partition.h"
#include "driver/flash.h"
#if CONFIG_BL2_DOWNLOAD
#include "download_flash_adapter.h"
#endif
#include "boot_param.h"
#include "bk_wdt.h"
#include "bl2_flash_map.h"

#ifdef TEST_BL2
#include "mcuboot_suites.h"
#endif /* TEST_BL2 */
#include "sys_hal.h"

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

#ifdef MCUBOOT_ENCRYPT_RSA
#define BL2_MBEDTLS_MEM_BUF_LEN 0x3000
#else
#define BL2_MBEDTLS_MEM_BUF_LEN 0x2000
#endif

#define HDR_SZ                  0x1000

/* Watchdog feed/reload value used across BL2 stages. Override from build config
 * (e.g. -DBL2_WDT_FEED_VAL=...) if the platform needs a different timeout. */
#ifndef BL2_WDT_FEED_VAL
#define BL2_WDT_FEED_VAL        0xFFFF
#endif

// Set CONFIG_DOWNLOAD_LOG to 1 to enable printf in download to debug
#define CONFIG_DOWNLOAD_LOG 1

/* Static buffer to be used by mbedtls for memory allocation */
static uint8_t mbedtls_mem_buf[BL2_MBEDTLS_MEM_BUF_LEN];
struct boot_rsp rsp;

static void do_boot(struct boot_rsp *rsp)
{
    struct boot_arm_vector_table *vt;
    uintptr_t flash_base;
    int rc;

    /* Image starts with the ARM vector table (MSP then reset vector);
     * set the stack pointer and jump to the reset vector. */
    rc = flash_device_base(rsp->br_flash_dev_id, &flash_base);
    assert(rc == 0);
    (void)rc;

    if (rsp->br_hdr->ih_flags & IMAGE_F_RAM_LOAD) {
       /* The image has been copied to SRAM, find the vector table
        * at the load address instead of image's address in flash
        */
        vt = (struct boot_arm_vector_table *)(rsp->br_hdr->ih_load_addr +
                                         rsp->br_hdr->ih_hdr_size);
    } else {
#if CONFIG_OTA_OVERWRITE || CONFIG_DIRECT_XIP
    {
        uint32_t vt_off = rsp->br_image_off;
#if CONFIG_DIRECT_XIP
        /* Slot B runs at the primary VA via HW remap: read vt there so the AES
         * tweak matches (B's physical addr decrypts to garbage -> hang). */
        if (flash_get_excute_enable()) {
            vt_off = partition_get_phy_offset(PARTITION_PRIMARY_ALL);
        }
#endif
        vt = (struct boot_arm_vector_table *)(flash_base + vt_off + rsp->br_hdr->ih_hdr_size);
    }
#endif
    }

    boot_platform_quit(vt);
}

int main(void)
{
    fih_ret fih_rc = FIH_FAILURE;
    enum tfm_plat_err_t plat_err;

    /* BK7259 bring-up: minimal BL2, FIH hardening (anti-tamper / CKMN /
     * bk_sw_fih_* / bk_fih_set_src) dropped for now; re-add once stable. */
    bk_efuse_init();

    update_wdt(BL2_WDT_FEED_VAL);
#if CONFIG_BL2_SECURE_DEBUG
    extern void hal_secure_debug(void);
    hal_secure_debug();
#endif

    /* Initialise the mbedtls static memory allocator so that mbedtls allocates
     * memory from the provided static buffer instead of from the heap.
     */
    mbedtls_memory_buffer_alloc_init(mbedtls_mem_buf, BL2_MBEDTLS_MEM_BUF_LEN);

#if CONFIG_DOWNLOAD_LOG
    stdio_init();
#endif
    /* Perform platform specific initialization */
    if (boot_platform_init() != 0) {
        BOOT_LOG_ERR("Platform init failed");
        FIH_PANIC;
    }
#if CONFIG_BL2_DOWNLOAD
    if (efuse_is_secure_download_enabled()) {
        BOOT_LOG_INF("BB2: download start");
        flash_switch_to_line_mode_two();
        void legacy_boot_main(void);
        legacy_boot_main();
        flash_restore_line_mode();
    }
#endif

    BOOT_LOG_INF("Starting bootloader");
    dump_efuse();
    int part_rc = -1;
    part_rc = partition_init();
    if (part_rc != 0) {
        BOOT_LOG_ERR("Partition init failed");
        FIH_PANIC;
    }

    flash_map_init();
    dump_partition();
    /* Force-A builds ignore the retained boot_param A/B record completely. */
#if !defined(CONFIG_XIP_FORCE_SLOT_A)
    /* Compute preferred A/B slot from boot_param; fed to MCUboot via
     * boot_get_active_slot_hook(). MCUboot still validates and falls back on
     * a bad signature. */
    (void)boot_param_load();
    uint8_t ab_pref = boot_param_decide_slot();
    BOOT_LOG_INF("boot_param preferred slot: %d", ab_pref);
#else
    BOOT_LOG_INF("XIP force-A: skip boot_param slot selection");
#endif

    plat_err = tfm_plat_otp_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        BOOT_LOG_ERR("OTP system initialization failed");
        FIH_PANIC;
    }

    FIH_CALL(boot_nv_security_counter_init, fih_rc);
    if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        BOOT_LOG_ERR("Error while initializing the security counter");
        FIH_PANIC;
    }

    /* Perform platform specific post-initialization */
    if (boot_platform_post_init() != 0) {
        BOOT_LOG_ERR("Platform post init failed");
        FIH_PANIC;
    }

#ifdef TEST_BL2
    (void)run_mcuboot_testsuite();
#endif /* TEST_BL2 */

#if CONFIG_DIRECT_XIP
    if (arch_dcache_invd_all() != 0) {
        BOOT_LOG_ERR("L1/L2 cache invalidate failed");
        FIH_PANIC;
    }
#endif

    update_wdt(BL2_WDT_FEED_VAL);
    FIH_CALL(boot_go, fih_rc, &rsp);
    if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
        BOOT_LOG_ERR("Unable to find bootable image");
        FIH_PANIC;
    }
    /* Force-A cannot reconcile or persist a fallback to the placeholder B. */
#if !defined(CONFIG_XIP_FORCE_SLOT_A)
    /* If MCUboot fell back off our preferred slot (it failed validation), persist
     * the slot actually booted so the next reset goes straight to the good one. */
    boot_param_reconcile_booted(rsp.br_image_off);
#endif
    do_boot(&rsp);

    BOOT_LOG_ERR("Never should get here");
    FIH_PANIC;

    /* Dummy return to be compatible with some check tools */
    return FIH_FAILURE;
}
// eof

