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
#include "bk_partition.h"
#include "bk_posix.h"
#include "app_display.h"

#define TAG "video_player_common"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

// SD card mount status (shared by recording and SD card test)
static bool sd_card_mounted = false;

// Mount SD card to /sd0
int sd_card_mount(void)
{
    int ret = BK_OK;

    LOGD("%s: Starting SD card mount, current status: %s\n", __func__, sd_card_mounted ? "mounted" : "not mounted");

    if (!sd_card_mounted)
    {
        struct bk_fatfs_partition partition;
        char *fs_name = NULL;
        fs_name = "fatfs";
        partition.part_type = FATFS_DEVICE;
        partition.part_dev.device_name = FATFS_DEV_SDCARD;
        partition.mount_path = VFS_SD_0_PATITION_0;

        LOGD("%s: Calling mount() with path=%s, fs_name=%s\n", __func__, partition.mount_path, fs_name);
        ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
        LOGD("%s: mount() returned: %d\n", __func__, ret);

        if (ret == BK_OK)
        {
            sd_card_mounted = true;
            LOGI("%s: SD card mounted to /sd0 successfully\n", __func__);
        }
        else
        {
            LOGE("%s: Failed to mount SD card, ret=%d\n", __func__, ret);
        }
    }
    else
    {
        LOGD("%s: SD card already mounted\n", __func__);
    }

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
            LOGE("%s: Failed to unmount SD card, ret=%d\n", __func__, ret);
        }
    }
    else
    {
        LOGD("%s: SD card not mounted\n", __func__);
    }

    return ret;
}

// Check if SD card is mounted
bool sd_card_is_mounted(void)
{
    return sd_card_mounted;
}

avdk_err_t video_play_lcd_open(bk_display_ctlr_handle_t *out_handle)
{
    avdk_err_t ret = app_mipi_lcd_turn_on(app_display_board_config_get());
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: app_display_open failed, ret:%d\n", __func__, ret);
        return ret;
    }

    return AVDK_ERR_OK;
}

avdk_err_t video_play_lcd_close(void)
{
    return app_mipi_lcd_turn_off();
}
