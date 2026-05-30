#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>
#include <components/bk_display.h>
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "video_player_common.h"
#include "video_play_callbacks.h"
#include "bk_partition.h"
#include "bk_posix.h"
#include "app_display.h"
#if CONFIG_SDCARD
#include <driver/sd_card.h>
#endif

#define TAG "video_player_common"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SD_CARD_MOUNT_MAX_RETRIES           (3U)
#define SD_CARD_POST_UNMOUNT_SETTLE_MS     (100U)

// SD card mount status (shared by recording and SD card test)
static bool sd_card_mounted = false;

#if CONFIG_SDCARD
static void sd_card_reset_hw(void)
{
    (void)bk_sd_card_deinit();
}
#else
static void sd_card_reset_hw(void)
{
}
#endif

static int sd_card_do_mount(void)
{
    struct bk_fatfs_partition partition;
    const char *fs_name = "fatfs";

    os_memset(&partition, 0, sizeof(partition));
    partition.part_type = FATFS_DEVICE;
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = VFS_SD_0_PATITION_0;

    LOGD("%s: Calling mount() with path=%s, fs_name=%s\n", __func__, partition.mount_path, fs_name);
    return mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
}

// Mount SD card to /sd0
int sd_card_mount(void)
{
    LOGD("%s: Starting SD card mount, current status: %s\n", __func__, sd_card_mounted ? "mounted" : "not mounted");

    if (sd_card_mounted)
    {
        LOGD("%s: SD card already mounted\n", __func__);
        return BK_OK;
    }

    int ret = BK_FAIL;
    ret = sd_card_do_mount();
    if (ret == BK_OK)
    {
        sd_card_mounted = true;
        LOGI("%s: SD card mounted to /sd0 successfully\n", __func__);
        return BK_OK;
    }
    sd_card_reset_hw();
    LOGE("%s: Failed to mount SD card after %u attempts, ret=%d\n",
         __func__, (unsigned)SD_CARD_MOUNT_MAX_RETRIES, ret);
    return ret;
}

// Unmount SD card from /sd0
int sd_card_unmount(void)
{
    int ret = BK_OK;

    if (sd_card_mounted)
    {
        ret = umount(VFS_SD_0_PATITION_0);
        if (ret == BK_OK)
        {
            sd_card_mounted = false;
            LOGI("%s: SD card unmounted from /sd0 successfully\n", __func__);
        }
        else
        {
            sd_card_mounted = false;
            LOGE("%s: Failed to unmount SD card, ret=%d\n", __func__, ret);
        }
    }
    else
    {
        LOGD("%s: SD card not mounted\n", __func__);
    }

    /* Reset SDIO so the next mount() re-initializes cleanly (avoids stale
     * "sd card has inited" state after heavy playback + umount). */
    sd_card_reset_hw();
    rtos_delay_milliseconds(SD_CARD_POST_UNMOUNT_SETTLE_MS);

    return ret;
}

// Check if SD card is mounted
bool sd_card_is_mounted(void)
{
    return sd_card_mounted;
}

static avdk_err_t video_play_lcd_apply_video_format(bk_display_ctlr_handle_t handle,
                                                    video_play_lcd_video_fmt_t fmt)
{
    if (handle == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    if (fmt == VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED)
    {
        const bk_display_pixel_format_config_t cfg = {
            .format = BK_PIXEL_FORMAT_ARGB8888,
            .decompress = true,
        };

        avdk_err_t ret = bk_display_pixel_format_set(handle, &cfg);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: DPU runtime switch to ARGB8888+decompress=true failed, ret=%d\n", __func__, ret);
            return ret;
        }

        LOGI("%s: DPU runtime switched to ARGB8888 (decompress=true, GPU-H264 path)\n", __func__);
        return AVDK_ERR_OK;
    }

    const bk_display_pixel_format_config_t cfg = {
        .format = BK_PIXEL_FORMAT_NV12,
        .decompress = false,
    };

    avdk_err_t ret = bk_display_pixel_format_set(handle, &cfg);
    if (ret != AVDK_ERR_OK)
    {
        /* Worth shouting about: the engine will hand the DPU NV12 frames
         * even if the runtime switch failed, in which case we get rainbow
         * tearing because the DPU is still parsing the bytes as compressed
         * ARGB8888. Surface ret so the caller can decide whether to abort. */
        LOGE("%s: DPU runtime switch to NV12+decompress=false failed, ret=%d\n", __func__, ret);
        return ret;
    }

    LOGI("%s: DPU runtime switched to NV12 (decompress=false)\n", __func__);
    return ret;
}

video_play_lcd_video_fmt_t video_play_lcd_format_for_video_codec(video_player_video_format_t format)
{
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    if (format == VIDEO_PLAYER_VIDEO_FORMAT_H264)
    {
        return VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED;
    }
#else
    (void)format;
#endif

    return VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
}

avdk_err_t video_play_lcd_apply_format(bk_display_ctlr_handle_t handle,
                                       video_play_lcd_video_fmt_t fmt)
{
    return video_play_lcd_apply_video_format(handle, fmt);
}

avdk_err_t video_play_lcd_ensure_open(bk_display_ctlr_handle_t *out_handle,
                                      video_play_lcd_video_fmt_t fmt)
{
    if (out_handle == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    if (*out_handle == NULL)
    {
        return video_play_lcd_open_with_format(out_handle, fmt);
    }

    return video_play_lcd_apply_format(*out_handle, fmt);
}

avdk_err_t video_play_lcd_open(bk_display_ctlr_handle_t *out_handle)
{
    return video_play_lcd_open_with_format(out_handle, VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW);
}

avdk_err_t video_play_lcd_open_with_format(bk_display_ctlr_handle_t *out_handle,
                                           video_play_lcd_video_fmt_t fmt)
{
    avdk_err_t ret = app_mipi_lcd_turn_on(app_display_board_config_get());
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: app_display_open failed, ret:%d\n", __func__, ret);
        return ret;
    }

    /*
     * Without this, s_lcd_display_handle stays NULL, the per-frame callback
     * sees lcd_handle == NULL and silently frees every decoded frame instead
     * of pushing it to the DPU.
     */
    bk_display_ctlr_handle_t handle = (bk_display_ctlr_handle_t)app_mipi_lcd_handle_get();
    if (handle == NULL)
    {
        LOGE("%s: app_mipi_lcd_handle_get returned NULL\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    /* Bring the DPU video layer in line with what the active video decoder
     * will produce. Failure is logged but not fatal so the upper layer can
     * still bring the LCD up; expect garbled frames in that case. */
    (void)video_play_lcd_apply_video_format(handle, fmt);

    if (out_handle != NULL)
    {
        *out_handle = handle;
    }

    return AVDK_ERR_OK;
}

avdk_err_t video_play_lcd_close(void)
{
    video_play_lcd_runtime_format_reset();
    return app_mipi_lcd_turn_off();
}
