/* =======================================================================
 *  USB HOST mode: U-disk sequential throughput benchmark.
 *
 *  Adds the `udisk speed` sub-command body to the usb_example project:
 *    1. Reuses the udisk host bring-up / media-wait helpers that own the
 *       shared MUSB device/host state machine (udisk_switch_to_host() and
 *       udisk_wait_media(), both defined in ap_main.c).
 *    2. Mounts FatFs drive "2:" and writes total_mb of data in block_kb
 *       chunks, calling f_sync() before stopping the write timer so the
 *       number reflects flushed data instead of FatFs cache.
 *    3. Re-opens the file and reads it back off the disk, so the read
 *       figure is not served from cache.
 *    4. Reports write / read throughput, removes the test file and unmounts.
 *
 *  Usage (from the `udisk` CLI in ap_main.c):
 *    udisk speed [MB] [blockKB]   - defaults: 32 MB total, 128 KB block
 * ===================================================================== */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>

#if (CONFIG_USB_HOST && CONFIG_USBH_MSC && CONFIG_FATFS)

#include "ff.h"

#define TAG "udisk"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Host bring-up and media-ready wait live in ap_main.c; they own the shared
 * udisk device/host state machine, so reuse them here instead of duplicating. */
extern int udisk_switch_to_host(void);
extern int udisk_wait_media(uint32_t timeout_ms);

#define UDISK_DRIVE_PATH        "2:"
#define UDISK_ENUM_WAIT_MS      8000u
#define UDISK_SPEED_FILE        "2:/bk_udisk_speed.bin"
#define UDISK_SPEED_DEF_MB      32u
#define UDISK_SPEED_DEF_KB      128u

/* Print "<bytes> in <ms> -> <KB/s> (<MB/s>)" for one direction. Throughput is
 * computed in 64-bit to avoid overflow on multi-MB transfers. */
static void udisk_report_speed(const char *tag, uint32_t bytes, uint32_t ms)
{
    uint32_t kbps = ms ? (uint32_t)((uint64_t)bytes * 1000u / ((uint64_t)ms * 1024u)) : 0;
    LOGI("%s: %u KB in %u ms -> %u KB/s (%u.%02u MB/s)\n",
         tag, (unsigned)(bytes / 1024u), (unsigned)ms, (unsigned)kbps,
         (unsigned)(kbps / 1024u), (unsigned)((kbps % 1024u) * 100u / 1024u));
}

/* Sequential write + read throughput benchmark. Writes total_mb in block_kb
 * chunks (f_sync before stopping the write timer so the result reflects flushed
 * data, not FatFs cache), then re-opens and reads the file back off the disk. */
int udisk_run_speed_test(uint32_t total_mb, uint32_t block_kb)
{
    FATFS   *fs  = NULL;
    FIL     *fp  = NULL;
    uint8_t *buf = NULL;
    FRESULT  fr;
    int      mounted = 0;
    int      rc = -1;

    if (total_mb == 0) total_mb = UDISK_SPEED_DEF_MB;
    if (block_kb == 0) block_kb = UDISK_SPEED_DEF_KB;
    const uint32_t block = block_kb * 1024u;
    const uint32_t loops = (total_mb * 1024u * 1024u) / block;
    const uint32_t bytes = loops * block;

    LOGI("==== U-disk speed test BEGIN (total=%u MB, block=%u KB) ====\n",
         (unsigned)total_mb, (unsigned)block_kb);

    if (udisk_switch_to_host() != 0) {
        return -1;
    }
    if (udisk_wait_media(UDISK_ENUM_WAIT_MS) != 0) {
        LOGE("no U-disk media; abort test\n");
        return -1;
    }

    fs  = (FATFS *)os_malloc(sizeof(FATFS));
    fp  = (FIL *)os_malloc(sizeof(FIL));
    buf = (uint8_t *)os_malloc(block);
    if (!fs || !fp || !buf) {
        LOGE("os_malloc failed (block=%u)\n", (unsigned)block);
        goto out_free;
    }
    os_memset(buf, 0xA5, block);

    fr = f_mount(fs, UDISK_DRIVE_PATH, 1);
    if (fr != FR_OK) {
        LOGE("f_mount(%s) failed fr=%d (3=NOT_READY, 13=NO_FILESYSTEM)\n",
             UDISK_DRIVE_PATH, fr);
        goto out_free;
    }
    mounted = 1;

    /* sequential write */
    fr = f_open(fp, UDISK_SPEED_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        LOGE("f_open(WR) failed fr=%d\n", fr);
        goto out_unmount;
    }
    uint32_t t0 = rtos_get_time();
    for (uint32_t i = 0; i < loops; i++) {
        UINT bw = 0;
        fr = f_write(fp, buf, block, &bw);
        if (fr != FR_OK || bw != block) {
            LOGE("f_write failed at %u/%u fr=%d bw=%u\n", i, loops, fr, (unsigned)bw);
            (void)f_close(fp);
            goto out_unmount;
        }
    }
    fr = f_sync(fp);
    uint32_t wr_ms = rtos_get_time() - t0;
    (void)f_close(fp);
    if (fr != FR_OK) {
        LOGE("f_sync failed fr=%d\n", fr);
        goto out_unmount;
    }

    /* sequential read (re-open so data actually comes off the disk) */
    fr = f_open(fp, UDISK_SPEED_FILE, FA_READ);
    if (fr != FR_OK) {
        LOGE("f_open(RD) failed fr=%d\n", fr);
        goto out_unmount;
    }
    t0 = rtos_get_time();
    for (uint32_t i = 0; i < loops; i++) {
        UINT br = 0;
        fr = f_read(fp, buf, block, &br);
        if (fr != FR_OK || br != block) {
            LOGE("f_read failed at %u/%u fr=%d br=%u\n", i, loops, fr, (unsigned)br);
            (void)f_close(fp);
            goto out_unmount;
        }
    }
    uint32_t rd_ms = rtos_get_time() - t0;
    (void)f_close(fp);

    udisk_report_speed("write", bytes, wr_ms);
    udisk_report_speed("read ", bytes, rd_ms);
    rc = 0;

out_unmount:
    (void)f_unlink(UDISK_SPEED_FILE);
    if (mounted) {
        (void)f_mount(NULL, UDISK_DRIVE_PATH, 0);
    }
out_free:
    if (fs)  os_free(fs);
    if (fp)  os_free(fp);
    if (buf) os_free(buf);

    LOGI("==== U-disk speed test %s ====\n", rc == 0 ? "PASS" : "FAIL");
    return rc;
}

#endif /* CONFIG_USB_HOST && CONFIG_USBH_MSC && CONFIG_FATFS */
