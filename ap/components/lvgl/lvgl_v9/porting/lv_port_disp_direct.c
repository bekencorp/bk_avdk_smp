/**
 * @file lv_port_disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp_private.h"
#include "../src/misc/lv_area_private.h"

#define TAG "LVGL_DIRECT"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static bk_err_t lv_direct_frame_buffer_free_cb(void *frame)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    rtos_set_semaphore(&vnd_data->lv_disp_sem);

    return BK_OK;
}

void lv_disp_flush_for_direct_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    static bool first_flush = true;
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(disp_drv);

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        vnd_data->config.flush_cb(vnd_data->config.args, px_map, lv_direct_frame_buffer_free_cb);

        if (first_flush) {
            first_flush = false;
        } else {
            bk_err_t ret = rtos_get_semaphore(&vnd_data->lv_disp_sem, 1000);
            if (ret != BK_OK) {
                LOGE("%s rtos_get_semaphore failed\n", __func__);
            }
        }
    }
}