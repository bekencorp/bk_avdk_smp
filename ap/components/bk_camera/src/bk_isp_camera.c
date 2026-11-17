#include <os/os.h>
#include <os/mem.h>

#include <common/avdk_pixel_types.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <components/bk_frame_buffer.h>
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

avdk_err_t bk_isp_camera_port_select(bk_isp_camera_ctlr_handle_t handle, uint8_t port_id)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->port_select, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->port_select(handle, port_id);
}

avdk_err_t bk_isp_camera_port_change(bk_isp_camera_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->port_change, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
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

avdk_err_t bk_isp_camera_multi_port_read(
    bk_isp_camera_ctlr_handle_t handle,
    const multi_port_read_param_t *param,
    multi_port_read_result_t *result)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(param, AVDK_ERR_INVAL, TAG, "param is NULL");
    AVDK_RETURN_ON_FALSE(result, AVDK_ERR_INVAL, TAG, "result is NULL");
    AVDK_RETURN_ON_FALSE(handle->multi_port_read, AVDK_ERR_UNSUPPORTED, TAG,
                         AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->multi_port_read(handle, param, result);
}

avdk_err_t bk_isp_camera_vc_mux_start(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->start, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->start(handle, config);
}

avdk_err_t bk_isp_camera_vc_mux_stop(bk_isp_camera_vc_mux_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->stop, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->stop(handle);
}

avdk_err_t bk_isp_camera_vc_mux_vc_enable(bk_isp_camera_vc_mux_handle_t handle, uint8_t vc, uint8_t discard_frames)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->vc_enable, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->vc_enable(handle, vc, discard_frames);
}

avdk_err_t bk_isp_camera_vc_mux_vc_disable(bk_isp_camera_vc_mux_handle_t handle, uint8_t vc)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->vc_disable, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->vc_disable(handle, vc);
}

avdk_err_t bk_isp_camera_vc_mux_peek(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_frame_ref_t *frame)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->peek, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->peek(handle, frame);
}

avdk_err_t bk_isp_camera_vc_mux_release(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_frame_ref_t *frame)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->release, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->release(handle, frame);
}

avdk_err_t bk_isp_camera_vc_mux_delete(bk_isp_camera_vc_mux_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->del(handle);
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

avdk_err_t bk_isp_camera_frame_pool_alloc(
    bk_isp_camera_channel_config_t *config,
    bk_isp_camera_frame_pool_t *pool)
{
    AVDK_RETURN_ON_FALSE(config && pool, AVDK_ERR_INVAL, TAG, "frame pool arg is NULL");
    AVDK_RETURN_ON_FALSE(config->work_mode == 0 && config->enable_flexa == 0,
                         AVDK_ERR_INVAL, TAG, "frame pool requires frame mode");
    AVDK_RETURN_ON_FALSE(config->buf_cnt >= 2 && config->buf_cnt <= ISP_FRAME_CNT_MAX,
                         AVDK_ERR_INVAL, TAG, "frame mode requires at least two buffers");
    for (uint8_t i = 0; i < ISP_FRAME_CNT_MAX; i++)
    {
        AVDK_RETURN_ON_FALSE(pool->buffers[i].addr == 0,
                             AVDK_ERR_BUSY, TAG, "previous frame pool is still allocated");
    }

    os_memset(pool, 0, sizeof(*pool));

    uint32_t frame_size;
    if (config->format == BK_PIXEL_FORMAT_RGB888 ||
        config->format == BK_PIXEL_FORMAT_BGR888)
    {
        /* ISP RGB888/BGR888 output is RGBX, four bytes per pixel. */
        frame_size = (uint32_t)config->width * config->height * 4U;
    }
    else
    {
        frame_size = bk_image_size_get(config->width, config->height,
                                       (bk_pixel_format_t)config->format);
    }
    AVDK_RETURN_ON_FALSE(frame_size > 0, AVDK_ERR_INVAL, TAG, "invalid frame geometry");

    for (uint8_t i = 0; i < config->buf_cnt; i++)
    {
#ifdef CONFIG_FRAME_BUFFER
        void *mem = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
#else
        void *mem = NULL;
#endif
        if (mem == NULL)
        {
            bk_isp_camera_frame_pool_free(pool);
            return AVDK_ERR_NOMEM;
        }
        pool->buffers[i].addr = (uint32_t)(uintptr_t)mem;
        pool->buffers[i].size = frame_size;
    }

    return AVDK_ERR_OK;
}

void bk_isp_camera_frame_pool_free(bk_isp_camera_frame_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < ISP_FRAME_CNT_MAX; i++)
    {
        if (pool->buffers[i].addr != 0)
        {
#ifdef CONFIG_FRAME_BUFFER
            bk_frame_buffer_free((void *)(uintptr_t)pool->buffers[i].addr);
#endif
            pool->buffers[i].addr = 0;
            pool->buffers[i].size = 0;
        }
    }
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
