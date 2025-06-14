// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include <driver/pwr_clk.h>
#include "media_app.h"
#include "camera_act.h"
#include "img_service.h"
#include "camera_handle_list.h"
#include "driver/lcd.h"
#include "uvc_pipeline_act.h"
#include "lcd_display_service.h"
#if CONFIG_BLUETOOTH_AP
#include "components/bluetooth/bk_dm_bluetooth.h"
#endif

#define TAG "media_app"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static media_modules_state_t *media_modules_state = NULL;


uint32_t media_app_get_lcd_status(void)
{
    bool lcd_status = false;
    lcd_status = check_lcd_task_is_open();

    return lcd_status;
}

uint32_t media_app_get_lcd_devices_num(void)
{
    uint32_t num;
    num = get_lcd_devices_num();
    return num;
}

uint32_t media_app_get_lcd_devices_list(void)
{
    const lcd_device_t **device_addr = get_lcd_devices_list();
    LOGI("%s, lcd device addr = %p\n", __func__, device_addr);
    return (uint32_t)device_addr;
}

uint32_t media_app_get_lcd_device_by_id(uint32_t id)
{
    int ret = BK_FAIL;

    const lcd_device_t *device = get_lcd_device_by_id(id);
    if (ret != BK_OK)
    {
        LOGE("%s error\n", __func__);
    }
    return (uint32_t)device;
}

bk_err_t media_app_lcd_fmt(pixel_format_t fmt)
{
    lcd_set_fmt(fmt);
    return 0;
}

bk_err_t media_app_set_rotate(media_rotate_t rotate)
{
    int ret = BK_FAIL;
    ret = pipeline_set_rotate(rotate);
    ret = image_rotate_set(rotate);
    LOGI("%s %d %d(0:0, 1:90, 2:180,3:270)\n", __func__, __LINE__, rotate);
    return ret;
}

bk_err_t media_app_get_main_camera_stream(frame_list_node_t *node)
{
    int ret = BK_FAIL;

    if (list_empty(&media_modules_state->cam_list))
    {
        LOGI("%s camera not open!\n", __func__);
        return ret;
    }

    node = frame_buffer_list_get_main_stream();

    LOGI("%s complete %p %x\n", __func__, node, node->camera_id);

    return ret;
}

bk_err_t media_app_lcd_disp_open(void *config)
{
    int ret = BK_OK;

    if (config == NULL)
    {
        LOGE("malloc lcd_open_t failed\r\n");
        return BK_FAIL;
    }

#if (CONFIG_BT_REUSE_MEDIA_MEMORY && CONFIG_BLUETOOTH_AP)
    bk_bluetooth_deinit();
#endif

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD,PM_POWER_MODULE_STATE_ON);

    ret = lcd_display_open(config);

    LOGI("%s complete %x\n", __func__, ret);

    return ret;
}

bk_err_t media_app_lcd_disp_close(void)
{
    int ret = BK_OK;

    ret = lcd_display_close();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);

    LOGI("%s complete %x\n", __func__, ret);

    return ret;
}

bk_err_t media_app_camera_open(camera_handle_t *handle, media_camera_device_t *device)
{
    int ret = BK_FAIL;

    LOGI("%s, type:%d, id:%d, W*H:%d*%d, format:%d\n",
         __func__, device->type, device->port, device->width,
         device->height, device->format);

    if (device == NULL || device->port >= CAMERA_MAX_NUM)
    {
        LOGE("%s, device (%p) null or port over valied range\n", __func__, device);
        return ret;
    }

#if (CONFIG_BT_REUSE_MEDIA_MEMORY && CONFIG_BLUETOOTH_AP)
    bk_bluetooth_deinit();
#endif

    camera_handle_t tmp = bk_camera_handle_node_get_by_id_and_fomat(device->port, device->format);
    if (tmp)
    {
        ret = BK_OK;
        LOGI("%s already opened, %p\n", __func__, tmp);
        *handle = tmp;
        return ret;
    }

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_ON);

    media_device_t media_device = {0};
    media_device.param1 = (uint32_t)handle;
    media_device.param2 = (uint32_t)device;

    ret = camera_open_handle(&media_device);

    if (ret == BK_OK)
    {
        media_camera_node_t *node = bk_camera_handle_node_init(device->port, device->format);
        if (node == NULL)
        {
            LOGE("%s, %d\n", __func__, __LINE__);
        }
        else
        {
            node->cam_handle = *handle;
        }
    }
    else
    {
        if (list_empty(&media_modules_state->cam_list))
        {
            LOGE("%s list_empty \n", __func__);
        }

        bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_OFF);
        LOGW("%s, %d, open failed...\n", __func__, __LINE__);
    }

    LOGI("%s complete\n", __func__);

    return ret;
}

bk_err_t media_app_camera_close(camera_handle_t *handle)
{
    int ret = BK_FAIL;

    if (*handle == NULL)
    {
        LOGI("%s already closed\n", __func__);
        return BK_OK;
    }

    camera_config_t *config = (camera_config_t *)*handle;

    camera_handle_t tmp = bk_camera_handle_node_get_by_id_and_fomat(config->id, config->image_format);
    if (tmp == NULL)
    {
        LOGI("%s already closed\n", __func__);
        return BK_OK;
    }
    ret = camera_close_handle(handle);

    if (ret == BK_OK)
    {
        bk_camera_handle_node_deinit(tmp);
    }

    if (list_empty(&media_modules_state->cam_list))
    {
        LOGI("%s list_empty \n", __func__);
        bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_JPEG_EN,PM_POWER_MODULE_STATE_OFF);
    }

    LOGI("%s complete %d\n", __func__, ret);

    return ret;
}


bk_err_t media_app_init(void)
{
    bk_err_t ret = BK_OK;

    if (media_modules_state == NULL)
    {
        media_modules_state = (media_modules_state_t *)os_malloc(sizeof(media_modules_state_t));
        if (media_modules_state == NULL)
        {
            LOGE("%s, media_modules_state malloc failed!\n", __func__);
            return BK_ERR_NO_MEM;
        }
    }

    media_modules_state->aud_state = AUDIO_STATE_DISABLED;
    INIT_LIST_HEAD(&media_modules_state->cam_list);
    media_modules_state->lcd_state = LCD_STATE_DISABLED;
    media_modules_state->stor_state = STORAGE_STATE_DISABLED;
    media_modules_state->trs_state = TRS_STATE_DISABLED;
    ret = bk_camera_handle_list_init((void *)&media_modules_state->cam_list);

    if (ret != kNoErr)
    {
        goto error;
    }

    LOGI("%s complete\n", __func__);

    return ret;

error:

    if (media_modules_state)
    {
        os_free(media_modules_state);
        media_modules_state = NULL;
    }

    LOGI("%s error\n");
    return ret;
}
