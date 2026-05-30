#include <os/os.h>
#include <os/mem.h>

#include <common/avdk_pixel_types.h>
#include <components/bk_isp_camera.h>
#include <driver/isp.h>
#include <avdk_check.h>
#include "isp_camera_ctlr.h"

#define TAG "bk_cam"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


avdk_err_t bk_isp_camera_dev_init(bk_isp_camera_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->dev_init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->dev_init(handle);
}

avdk_err_t bk_isp_camera_port_init(bk_isp_camera_ctlr_handle_t handle, void *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->port_init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->port_init(handle, config);
}

avdk_err_t bk_isp_camera_port_change(bk_isp_camera_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->dev_init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->port_change(handle);
}

avdk_err_t bk_isp_camera_deinit(bk_isp_camera_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->deinit, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->deinit(handle);
}

avdk_err_t bk_isp_camera_open(bk_isp_camera_ctlr_handle_t handle, void *parameter)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->open(handle, parameter);
}

avdk_err_t bk_isp_camera_close(bk_isp_camera_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->close(handle);
}

avdk_err_t bk_isp_camera_read(bk_isp_camera_ctlr_handle_t handle, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->read, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->read(handle, id, frame, size, timeout);
}

avdk_err_t bk_isp_camera_register_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, bk_camera_isr_t cb, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->register_isr_callback, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->register_isr_callback(handle, type, cb, arg);
}

avdk_err_t bk_isp_camera_deregister_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->deregister_isr_callback, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->deregister_isr_callback(handle, type, arg);
}

avdk_err_t bk_isp_camera_ctlr_ioctl(bk_isp_camera_ctlr_handle_t handle, bk_cam_interface_ioctl_t ioctl, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->ioctl(handle, ioctl, arg);
}

avdk_err_t bk_isp_camera_delete(bk_isp_camera_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->del(handle);
}

avdk_err_t bk_isp_camera_channel_open(bk_isp_camera_ctlr_handle_t handle, uint8_t channel, bk_isp_camera_channel_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->channel_open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->channel_open(handle, channel, config);
}

avdk_err_t bk_isp_camera_channel_close(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->channel_close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->channel_close(handle, channel);
}

bk_isp_camera_channel_state_t bk_isp_camera_channel_state_get(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->channel_state_get, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->channel_state_get(handle, channel);
}