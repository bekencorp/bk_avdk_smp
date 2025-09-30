#include <stdint.h>
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "mux_pipeline.h"
#include "uvc_pipeline_act.h"
#include "yuv_encode.h"
#include "components/media_types.h"
#include "components/bk_video_pipeline/bk_video_pipeline.h"
#include "bk_video_pipeline_ctlr.h"

#define TAG "video_pipeline_ctlr"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static avdk_err_t video_pipeline_ctlr_open_h264e(bk_video_pipeline_ctlr_handle_t handler, bk_video_pipeline_h264e_config_t *config)
{
    bk_err_t ret = BK_OK;
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    AVDK_RETURN_ON_FALSE(config->h264e_cb, AVDK_ERR_INVAL, TAG, "h264e_cb is NULL");
    AVDK_RETURN_ON_FALSE(config->h264e_cb->malloc, AVDK_ERR_INVAL, TAG, "h264e_cb->malloc is NULL");
    AVDK_RETURN_ON_FALSE(config->h264e_cb->complete, AVDK_ERR_INVAL, TAG, "h264e_cb->complete is NULL");

    if (controller->module_status.h264e_enable == VIDEO_PIPELINE_MODULE_ENABLED)
    {
        LOGE("%s, h264e is already opened\n", __func__);
        return BK_OK;
    }
    controller->h264e_config.width = config->width;
    controller->h264e_config.height = config->height;
    controller->h264e_config.fps = config->fps;
    controller->h264e_config.sw_rotate_angle = config->sw_rotate_angle;
    controller->h264e_config.h264e_cb = config->h264e_cb;
    ret = h264_jdec_pipeline_open(&controller->h264e_config,
                                controller->h264e_config.h264e_cb,
                                controller->config.jpeg_cbs,
                                controller->config.decode_cbs);
    if (ret != BK_OK)
    {
        LOGE("%s, h264_jdec_pipeline_open fail\n", __func__);
        goto error;
    }
    controller->module_status.h264e_enable = VIDEO_PIPELINE_MODULE_ENABLED;
    return AVDK_ERR_OK;
error:

    return ret;
}

static avdk_err_t video_pipeline_ctlr_close_h264e(bk_video_pipeline_ctlr_handle_t handler)
{
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (controller->module_status.h264e_enable == VIDEO_PIPELINE_MODULE_ENABLED)
    {
        controller->module_status.h264e_enable = VIDEO_PIPELINE_MODULE_DISABLED;
        h264_jdec_pipeline_close();
    }
    return AVDK_ERR_OK;
}

static avdk_err_t video_pipeline_ctlr_open_rotate(bk_video_pipeline_ctlr_handle_t handler, bk_video_pipeline_decode_config_t *config)
{
    bk_err_t ret = BK_OK;
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    if (controller->module_status.rotate_enable == VIDEO_PIPELINE_MODULE_ENABLED)
    {
        LOGE("%s, rotate is already opened\n", __func__);
        return BK_OK;
    }

    controller->decode_config.rotate_mode = config->rotate_mode;
    controller->decode_config.rotate_angle = config->rotate_angle;
    ret = lcd_jdec_pipeline_open(&controller->decode_config,
                                controller->config.jpeg_cbs,
                                controller->config.decode_cbs);
    if (ret != BK_OK)
    {
        LOGE("%s, lcd_jdec_pipeline_open fail\n", __func__);
        goto error;
    }
    controller->module_status.rotate_enable = VIDEO_PIPELINE_MODULE_ENABLED;
    return AVDK_ERR_OK;
error:

    return ret;
}

static avdk_err_t video_pipeline_ctlr_close_rotate(bk_video_pipeline_ctlr_handle_t handler)
{
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (controller->module_status.rotate_enable == VIDEO_PIPELINE_MODULE_ENABLED)
    {
        lcd_jdec_pipeline_close();
        controller->module_status.rotate_enable = VIDEO_PIPELINE_MODULE_DISABLED;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t video_pipeline_ctlr_reset_decode(bk_video_pipeline_ctlr_handle_t handler)
{
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    video_pipeline_reset_decode();
    return AVDK_ERR_OK;
}

static avdk_err_t video_pipeline_ctlr_get_module_status(bk_video_pipeline_ctlr_handle_t handler, video_pipeline_module_t module, video_pipeline_module_status_t *status)
{
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(status, AVDK_ERR_INVAL, TAG, "status is NULL");

    switch (module)
    {
        case VIDEO_PIPELINE_MODULE_H264E:
            *status = controller->module_status.h264e_enable;
            break;
        case VIDEO_PIPELINE_MODULE_ROTATE:
            *status = controller->module_status.rotate_enable;
            break;
        default:
            break;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t video_pipeline_ctlr_delete(bk_video_pipeline_ctlr_handle_t handler)
{
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (controller->module_status.h264e_enable == VIDEO_PIPELINE_MODULE_ENABLED)
    {
        controller->module_status.h264e_enable = VIDEO_PIPELINE_MODULE_DISABLED;
        h264_jdec_pipeline_close();
    }
    if (controller->module_status.rotate_enable == VIDEO_PIPELINE_MODULE_ENABLED)
    {
        controller->module_status.rotate_enable = VIDEO_PIPELINE_MODULE_DISABLED;
        lcd_jdec_pipeline_close();
    }
    os_free(controller);
    return AVDK_ERR_OK;
}

static avdk_err_t video_pipeline_ctlr_ioctl(bk_video_pipeline_ctlr_handle_t handler, video_pipeline_ioctl_cmd_t cmd, void *param)
{
    private_video_pipeline_ctlr_t *controller = __containerof(handler, private_video_pipeline_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    //TODO
    switch (cmd)
    {
    case VIDEO_PIPELINE_IOCTL_CMD_BASE:
        break;
    default:
        break;
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_pipeline_ctlr_new(bk_video_pipeline_ctlr_handle_t *handle, bk_video_pipeline_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    AVDK_RETURN_ON_FALSE(config->decode_cbs, AVDK_ERR_INVAL, TAG, "decode_cbs is NULL");
    AVDK_RETURN_ON_FALSE(config->decode_cbs->malloc, AVDK_ERR_INVAL, TAG, "decode_cbs->malloc is NULL");
    AVDK_RETURN_ON_FALSE(config->decode_cbs->free, AVDK_ERR_INVAL, TAG, "decode_cbs->free is NULL");
    AVDK_RETURN_ON_FALSE(config->decode_cbs->complete, AVDK_ERR_INVAL, TAG, "decode_cbs->complete is NULL");
    AVDK_RETURN_ON_FALSE(config->jpeg_cbs, AVDK_ERR_INVAL, TAG, "jpeg_cbs is NULL");
    AVDK_RETURN_ON_FALSE(config->jpeg_cbs->read, AVDK_ERR_INVAL, TAG, "jpeg_cbs->read is NULL");
    AVDK_RETURN_ON_FALSE(config->jpeg_cbs->complete, AVDK_ERR_INVAL, TAG, "jpeg_cbs->complete is NULL");

    private_video_pipeline_ctlr_t *controller = os_malloc(sizeof(private_video_pipeline_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_video_pipeline_ctlr_t));

    os_memcpy(&controller->config, config, sizeof(bk_video_pipeline_ctlr_config_t));
    controller->ops.open_h264e = video_pipeline_ctlr_open_h264e;
    controller->ops.close_h264e = video_pipeline_ctlr_close_h264e;
    controller->ops.open_rotate = video_pipeline_ctlr_open_rotate;
    controller->ops.close_rotate = video_pipeline_ctlr_close_rotate;
    controller->ops.reset_decode = video_pipeline_ctlr_reset_decode;
    controller->ops.get_module_status = video_pipeline_ctlr_get_module_status;
    controller->ops.delete = video_pipeline_ctlr_delete;
    controller->ops.ioctl = video_pipeline_ctlr_ioctl;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}
