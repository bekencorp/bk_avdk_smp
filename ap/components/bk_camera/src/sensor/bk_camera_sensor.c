#include <os/os.h>
#include <os/mem.h>

#include <components/media_types.h>
#include <components/bk_isp_camera.h>
#include <driver/isp.h>
#include <driver/i2c.h>
#include <driver/io_matrix.h>
#include <avdk_check.h>
#include "isp_camera_ctlr.h"
#include "isp_cam_sensor.h"
#include <components/bk_camera_sensor.h>

#define TAG "bk_cam"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


void *bk_camera_sensor_get_sensor_object(bk_camera_sensor_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, NULL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->get_sensor_object, NULL, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->get_sensor_object(handle);
}

void *bk_camera_sensor_get_sensor_cfg(bk_camera_sensor_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, NULL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->get_sensor_cfg, NULL, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->get_sensor_cfg(handle);
}

avdk_err_t bk_camera_sensor_set_format(bk_camera_sensor_handle_t handle, bk_camera_sensor_format_t *format)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->set_format, AVDK_ERR_INVAL, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->set_format(handle, format);
}

avdk_err_t bk_camera_sensor_init(bk_camera_sensor_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_INVAL, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    LOGI("%s, %d, init=%p\n", __func__, __LINE__, handle->init);
    return handle->init(handle);
}

avdk_err_t bk_camera_sensor_query_support_formats(bk_camera_sensor_handle_t handle, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->query_support_formats, AVDK_ERR_INVAL, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->query_support_formats(handle, format_array);
}

bk_camera_sensor_handle_t bk_camera_sensor_auto_detect(bk_camera_sensor_config_t *config, bk_camera_port_t port)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    bk_camera_sensor_handle_t handle = NULL;

    for (bk_camera_sensor_detect_func_t *p = &__camera_sensor_detect_array_start; p < &__camera_sensor_detect_array_end; p++)
    {
        if (p->detect && p->port == port)
        {
            ret = p->detect(&handle, config);

            if (ret == AVDK_ERR_OK)
            {
                break;
            }
        }
    }

    return handle;
}

void bk_camera_sensor_destroy(bk_camera_sensor_handle_t handle)
{
    if (handle == NULL)
    {
        return;
    }

    /* CSI/DVP wrapper structs share the same layout; ops is the public handle. */
    bk_camera_csi_sensor_t *wrapper = __containerof(handle, bk_camera_csi_sensor_t, ops);
    os_free(wrapper);
}