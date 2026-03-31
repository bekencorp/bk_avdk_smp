#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include "aov_camera.h"

#include <driver/mipi_csi.h>
#include <components/bk_isp_camera.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu.h>
#include <components/bk_camera_isp_ctlr.h>
#include <components/bk_camera_configs.h>
#include <components/bk_frame_buffer.h>
#include <avdk_check.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include <driver/i2c.h>
#include <components/bk_camera_sensor.h>

#include "app_camera.h"

#define TAG "db-avo"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    isp_handle_t isp_handle;
    mipi_csi_handle_t csi_handle;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;
} isp_csi_cam_handle_t;

static isp_csi_cam_handle_t isp_csi_cam_handle = {0};

void *aov_isp_handle_get(void)
{
    return isp_csi_cam_handle.isp_handle;
}

int aov_isp_camera_turn_off(void)
{
    LOGI("%s\n", __func__);

    // step 1: close sensor
    if (isp_csi_cam_handle.camera_ctlr_handle)
    {
        bk_isp_camera_close(isp_csi_cam_handle.camera_ctlr_handle);
    }

    if (isp_csi_cam_handle.camera_ctlr_handle)
    {
        bk_isp_camera_deinit(isp_csi_cam_handle.camera_ctlr_handle);
    }

    return BK_OK;
}


int aov_isp_camera_turn_on(uint16_t width, uint16_t height, uint16_t pixel_format)
{
    LOGI("%s\n", __func__);

    app_isp_mipi_camera_turn_on(app_camera_board_config_get());

    app_isp_camera_sp_channel_turn_on(app_camera_board_config_get());
#if CONFIG_VG_LITE_GPU
#include "app_gpu.h"
    app_gpu_turn_on(app_gpu_board_config_get());
#endif

    return BK_OK;

}

bool aov_isp_camera_state_get(void)
{
    return 0;
}

int aov_isp_camera_frame_get(uint8_t *frame, uint32_t size)
{
    int ret = BK_FAIL;
    // current support sp channel
    if (frame == NULL)
    {
        LOGE("%s, %d frame malloc failed\n", __func__, __LINE__);
        return ret;
    }

    return app_isp_camera_channel_read(APP_ISP_SP_CHN_ID, frame, size, -1);
}
