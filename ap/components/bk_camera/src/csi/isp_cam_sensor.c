#include "string.h"
#include <os/mem.h>
#include "isp_cam_sensor.h"
#include "csi_sensor_devices.h"
#include "dvp_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <vsi_list.h>
#include <components/bk_camera_sensor.h>

#define TAG "sensor"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

bk_camera_sensor_handle_t bk_dvp_get_sensor_auto_detect(bk_camera_bus_t *bus);


ISP_PUB_ATTR_S *csi_camera_attr = NULL;
ISP_PUB_ATTR_S *dvp_camera_attr = NULL;

static bk_camera_sensor_handle_t sensor_handle = NULL;


//actually there're four rect with different meanings and is configurable separately
//for test, simple configuration
void isp_sensor_set_rect(ISP_PUB_ATTR_S *isp_sns_attr, RECT_S *sns_rect, RECT_S *if_rect)
{
    os_memcpy(&isp_sns_attr->snsRect, sns_rect, sizeof(RECT_S));
    os_memcpy(&isp_sns_attr->inFormRect, if_rect, sizeof(RECT_S));
    os_memcpy(&isp_sns_attr->outFormRect, if_rect, sizeof(RECT_S));
    os_memcpy(&isp_sns_attr->iSRect, if_rect, sizeof(RECT_S));
}

bk_err_t isp_csi_camera_detect(isp_sensor_t *isp_sns, bk_camera_bus_t *bus)
{
    LOGW("%s, %d\n", __func__, __LINE__);

    if (sensor_handle)
    {
        bk_camera_sensor_destroy(sensor_handle);
        sensor_handle = NULL;
    }

    bk_camera_sensor_config_t config = {
        .pin_reset = 28,
        .pin_pwdn = 0xFF,
        .pin_xclk = 0,
        .bus = bus,
    };

    sensor_handle = bk_camera_sensor_auto_detect(&config, CSI_CAMERA_PORT);

    if (sensor_handle == NULL)
    {
        return BK_FAIL;
    }

    const ISP_PUB_ATTR_S *csi_camera_attr_temp = bk_camera_sensor_get_sensor_object(sensor_handle);
    if (csi_camera_attr_temp == NULL)
    {
        LOGE("%s no csi camera\r\n", __func__);
        return BK_FAIL;
    }

    if (csi_camera_attr == NULL)
    {
        csi_camera_attr = (ISP_PUB_ATTR_S *)os_malloc(sizeof(ISP_PUB_ATTR_S));
        if (csi_camera_attr == NULL)
        {
            LOGE("%s no buffer for csi_camera_attr\r\n", __func__);
            return BK_FAIL;
        }
    }
    os_memcpy(csi_camera_attr, csi_camera_attr_temp, sizeof(ISP_PUB_ATTR_S));

    csi_sensor_config_t *csi_sensor_cfg = (csi_sensor_config_t *)bk_camera_sensor_get_sensor_cfg(sensor_handle);

    if (isp_sns)
    {
        isp_sensor_set_rect(csi_camera_attr, &isp_sns->ppi, &isp_sns->acq);
        csi_camera_attr->snsFps = isp_sns->fps * ISP_SNS_FPS_ACCU;
    }
    else
    {
        RECT_S s_rect = {
            .width = csi_sensor_cfg->default_width,
            .height = csi_sensor_cfg->default_height,
            .top = 0,
            .left = 0,
        };

        isp_sensor_set_rect(csi_camera_attr, &s_rect, &s_rect);
        csi_camera_attr->snsFps = csi_sensor_cfg->default_fps * ISP_SNS_FPS_ACCU;
    }

    return BK_OK;
}

bk_err_t isp_dvp_camera_detect(isp_sensor_t *isp_sns, bk_camera_bus_t *bus)
{
    if (sensor_handle)
    {
        bk_camera_sensor_destroy(sensor_handle);
        sensor_handle = NULL;
    }

    sensor_handle = bk_dvp_get_sensor_auto_detect(bus);

    const ISP_PUB_ATTR_S *dvp_camera_attr_temp = bk_camera_sensor_get_sensor_object(sensor_handle);
    if (dvp_camera_attr_temp == NULL)
    {
        LOGE("%s no dvp camera\r\n", __func__);
        return BK_FAIL;
    }

    if (dvp_camera_attr == NULL)
    {
        dvp_camera_attr = (ISP_PUB_ATTR_S *)os_malloc(sizeof(ISP_PUB_ATTR_S));
        if (dvp_camera_attr == NULL)
        {
            LOGE("%s no buffer for dvp_camera_attr\r\n", __func__);
            return BK_FAIL;
        }
    }
    os_memcpy(dvp_camera_attr, dvp_camera_attr_temp, sizeof(ISP_PUB_ATTR_S));

    dvp_sensor_config_t *dvp_sensor_cfg = (dvp_sensor_config_t *)bk_camera_sensor_get_sensor_cfg(sensor_handle);

    if (isp_sns)
    {
        isp_sensor_set_rect(dvp_camera_attr, &isp_sns->ppi, &isp_sns->acq);
        dvp_camera_attr->snsFps = isp_sns->fps * ISP_SNS_FPS_ACCU;
    }
    else
    {
        RECT_S s_rect = {
            .width = dvp_sensor_cfg->default_width,
            .height = dvp_sensor_cfg->default_height,
            .top = 0,
            .left = 0,
        };

        isp_sensor_set_rect(dvp_camera_attr, &s_rect, &s_rect);
    }

    return BK_OK;
}

bk_err_t isp_csi_camera_init()
{
    bk_err_t ret = BK_FAIL;
    if (csi_camera_attr)
    {
        csi_sensor_config_t *csi_sensor_cfg = (csi_sensor_config_t *)bk_camera_sensor_get_sensor_cfg(sensor_handle);
        uint16_t width = csi_camera_attr->snsRect.width;
        uint16_t height = csi_camera_attr->snsRect.height;

        bk_mipi_csi_ext_set_enable(0);
        bk_mipi_csi_controller_init(width, height, csi_sensor_cfg->mipi_data_type);
        //csi_sensor_cfg->reg_ctrl(CSI_SNS_STANDBY, 0, 0);
        bk_camera_sensor_init(sensor_handle);
        bk_camera_sensor_format_t format = {
            .width = width,
            .height = height,
            .fps = csi_camera_attr->snsFps / ISP_SNS_FPS_ACCU,
        };
        bk_camera_sensor_set_format(sensor_handle, &format);
        ret = BK_OK;
    }

    return ret;
}

bk_err_t isp_dvp_camera_init()
{
    bk_err_t ret = BK_FAIL;

    if (dvp_camera_attr)
    {
        //dvp_sensor_config_t *dvp_sensor_cfg = (dvp_sensor_config_t *)dvp_camera_attr->sensor_config;
        uint16_t width = dvp_camera_attr->snsRect.width;
        uint16_t height = dvp_camera_attr->snsRect.height;

        bk_mipi_csi_ext_set_enable(1);

        bk_camera_sensor_init(sensor_handle);
        bk_camera_sensor_format_t format = {
            .width = width,
            .height = height,
            .fps = dvp_camera_attr->snsFps / ISP_SNS_FPS_ACCU,
        };
        bk_camera_sensor_set_format(sensor_handle, &format);
        ret = BK_OK;
    }

    return ret;
}

ISP_PUB_ATTR_S * isp_sensor_get_main_attr()
{
    if (csi_camera_attr)
    {
        return csi_camera_attr;
    }

    if (dvp_camera_attr)
    {
        return dvp_camera_attr;
    }

    return NULL;
}

void isp_csi_sensor_reg_ctrl(uint8_t cmd, uint16_t addr, uint8_t val)
{
    if (csi_camera_attr)
    {
        csi_sensor_config_t *csi_sensor_cfg = (csi_sensor_config_t *)bk_camera_sensor_get_sensor_cfg(sensor_handle);
        csi_sensor_cfg->reg_ctrl(NULL, cmd, addr, val);
    }
}

ISP_PUB_ATTR_S *isp_sensor_get_csi_attr(void)
{
    return csi_camera_attr;
}

ISP_PUB_ATTR_S *isp_sensor_get_dvp_attr(void)
{
    return dvp_camera_attr;
}
