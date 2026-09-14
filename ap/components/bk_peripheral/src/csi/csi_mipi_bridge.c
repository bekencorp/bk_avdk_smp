// Copyright 2024-2025 Beken
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
#include <os/mem.h>
#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <components/bk_camera_sensor.h>
#include <avdk_check.h>

#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <vsi_list.h>
#include <driver/isp_types.h>
#include <components/bk_isp_camera_types.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include "vsios_i2c.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_calib.h"

#define TAG "mipi bridge"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

avdk_err_t mipi_bridge_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

static int MipiBridge_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    return 0;
}

static int MipiBridge_Exit(ISP_PORT IspPort)
{
    return 0;
}

static int MipiBridge_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    return 0;
}

static int MipiBridge_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    return 0;
}

static int MipiBridge_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    return 0;
}

static int MipiBridge_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    return 0;
}

static int MipiBridge_SetIspDefault(ISP_PORT IspPort)
{
    return 0;
}

static int MipiBridge_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = MipiBridge_Init;
    pIspSnsFunc->pfnSensorExit    = MipiBridge_Exit;
    pIspSnsFunc->pfnWriteReg      = MipiBridge_WriteReg;
    pIspSnsFunc->pfnReadReg       = MipiBridge_ReadReg;
    pIspSnsFunc->pfnSetMode       = MipiBridge_SetMode;
    pIspSnsFunc->pfnSetStream     = MipiBridge_SetStream;
    pIspSnsFunc->pfnSetIspDefault = MipiBridge_SetIspDefault;
    return 0;
}

static int MipiBridge_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    return 0;
}

const ISP_SNS_OBJ_S snsMipiBridgeObj = {
    .pfnInitIspSnsFunc = MipiBridge_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = MipiBridge_InitAeSnsFunc,
};

static avdk_err_t mipi_bridge_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_printf("%s\n", __func__);

    bk_mipi_csi_ext_set_enable(0);
    //bk_mipi_csi_enable_debug_pin();

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static avdk_err_t mipi_bridge_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_printf("%s, width: %d, height: %d\n", __func__, width, height);

    bk_mipi_csi_controller_init(width, height, 0x1E);

    return 0;
}

static avdk_err_t mipi_bridge_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    return 0;
}

static avdk_err_t mipi_bridge_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_printf("%s, width: %d, height: %d\n", __func__, format->width, format->height);

    mipi_bridge_set_ppi(controller, format->width, format->height);
    mipi_bridge_set_fps(controller, format->fps);

    bk_mipi_csi_controller_reset();

    *((volatile uint32_t*)(0x4c040000 + 0x0000155C)) = (6 << 1);
    *((volatile uint32_t*)(0x4c050000 + 0x000000AC)) = ((1 << 0) | (0x2A << 8));

    return AVDK_ERR_OK;
}

static avdk_err_t mipi_bridge_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    return 0;
}

static avdk_err_t mipi_bridge_set_hmirror(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    return 0;
}

static avdk_err_t mipi_bridge_set_vflip(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    return 0;
}

static const csi_sensor_config_t csi_sensor_mipi_bridge =
{
    .name = "mipi_bridge",
    .clk = MCLK_24M,
    .mipi_data_type = 0x1E,
    .default_width = 1280,
    .default_height = 720,
    .default_fps = 30,
    .id = 0,
    .address = (0 >> 1),
    .init = mipi_bridge_init,
    .detect = mipi_bridge_detect,
    .set_ppi = mipi_bridge_set_ppi,
    .set_fps = mipi_bridge_set_fps,
    .reg_ctrl = mipi_bridge_ctrl,
};

static const ISP_PUB_ATTR_S mipi_bridge_mipi_linear_attr = {
    .pSnsObj      = (void*)&snsMipiBridgeObj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB8,
    .snsFps       = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t mipi_bridge_format_array[] = {
    {
        .width = 1280,
        .height = 720,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_YUYV,
    },
};

static avdk_err_t mipi_bridge_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &mipi_bridge_format_array[0];
    format_array->size = ARRAY_SIZE(mipi_bridge_format_array);
    return AVDK_ERR_OK;
}

static void *mipi_bridge_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsMipiBridgeObj;
}

static void *mipi_bridge_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

avdk_err_t mipi_bridge_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    config->bus->write_address = 0;

    AVDK_RETURN_ON_FALSE(config->bus, AVDK_ERR_GENERIC, TAG, "bus is NULL");

    LOGI("%s, rest_pin: %d, pwdn_pin: %d\n", __func__, config->pin_reset, config->pin_pwdn);
    LOGI("%s detect success\n", TAG);

    bk_camera_csi_sensor_t *csi_sensor = os_malloc(sizeof(bk_camera_csi_sensor_t));
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(csi_sensor, 0, sizeof(bk_camera_csi_sensor_t));
    config->bus->write_address = 0;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = mipi_bridge_init;
    csi_sensor->ops.set_format = mipi_bridge_set_format;
    csi_sensor->ops.reg_ctrl = mipi_bridge_ctrl;
    csi_sensor->ops.set_hmirror = mipi_bridge_set_hmirror;
    csi_sensor->ops.set_vflip = mipi_bridge_set_vflip;
    csi_sensor->ops.get_sensor_object = mipi_bridge_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = mipi_bridge_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = mipi_bridge_query_support_formats;

    csi_sensor->isp_pub_attr = &mipi_bridge_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_mipi_bridge;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}

BK_CAMERA_SENSOR_DETECT_SECTION(mipi_bridge_detect, CSI_CAMERA_PORT);
