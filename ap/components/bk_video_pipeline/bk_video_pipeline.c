#include <stdint.h>
#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "mux_pipeline.h"
#include "uvc_pipeline_act.h"
#include "components/media_types.h"
#include "components/bk_video_pipeline/bk_video_pipeline.h"
#include "bk_video_pipeline_ctlr.h"

#define TAG "video_pipeline"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t bk_video_pipeline_open_h264e(bk_video_pipeline_handle_t handler, bk_video_pipeline_h264e_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(config && handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open_h264e, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->open_h264e(handler, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "open_h264e failed");

    return ret;
}

avdk_err_t bk_video_pipeline_close_h264e(bk_video_pipeline_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close_h264e, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->close_h264e(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "close_h264e failed");

    return ret;
}

avdk_err_t bk_video_pipeline_open_rotate(bk_video_pipeline_handle_t handler, bk_video_pipeline_decode_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(config && handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->open_rotate, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    switch (config->rotate_mode) {
        case NONE_ROTATE:
        case HW_ROTATE:
        case SW_ROTATE:
            break;
        default:
            LOGE("Invalid rotate mode: %d\n", config->rotate_mode);
            return AVDK_ERR_INVAL;
    }
    switch (config->rotate_angle) {
        case 0:
        case 90:
        case 180:
        case 270:
            break;
        default:
            LOGE("Invalid rotate angle: %d\n", config->rotate_angle);
            return AVDK_ERR_INVAL;
    }
    ret = handler->open_rotate(handler, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "open_rotate failed");

    return ret;
}

avdk_err_t bk_video_pipeline_close_rotate(bk_video_pipeline_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->close_rotate, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->close_rotate(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "close_rotate failed");

    return ret;
}

avdk_err_t bk_video_pipeline_get_module_status(bk_video_pipeline_handle_t handler, video_pipeline_module_t module, video_pipeline_module_status_t *status)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->get_module_status, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(status, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->get_module_status(handler, module, status);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "get_module_status failed");

    return ret;
}

avdk_err_t bk_video_pipeline_reset_decode(bk_video_pipeline_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->reset_decode, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->reset_decode(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "reset_decode failed");

    return ret;
}

avdk_err_t bk_video_pipeline_delete(bk_video_pipeline_handle_t handler)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->delete, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->delete(handler);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "delete failed");

    return ret;
}

avdk_err_t bk_video_pipeline_ioctl(bk_video_pipeline_handle_t handler, video_pipeline_ioctl_cmd_t cmd, void *param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handler, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handler->ioctl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    ret = handler->ioctl(handler, cmd, param);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "ioctl failed");

    return ret;
}

avdk_err_t bk_video_pipeline_new(bk_video_pipeline_handle_t *handle, bk_video_pipeline_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL, it may have already been created");

    ret = bk_video_pipeline_ctlr_new(handle, config);
    AVDK_RETURN_ON_FALSE(ret == BK_OK, ret, TAG, "new failed");

    return ret;
}