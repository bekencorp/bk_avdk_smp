#include <os/os.h>
#include <os/mem.h>
#include <components/avdk_utils/avdk_check.h>

#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "h264_encode_vcenc_rate_ctrl_priv.h"

#define TAG "bk_h264e"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


avdk_err_t bk_h264_encode_init(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->init(handle);
}

avdk_err_t bk_h264_encode_deinit(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->deinit, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->deinit(handle);
}

avdk_err_t bk_h264_encode_open(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->open(handle);
}

avdk_err_t bk_h264_encode_close(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->close(handle);
}

avdk_err_t bk_h264_encode_start(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->encode, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->encode(handle);
}

avdk_err_t bk_h264_encode_ioctl(bk_h264_encode_ctlr_handle_t handle, bk_h264_encode_ioctl_cmd_t cmd, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->ioctl(handle, cmd, arg);
}

avdk_err_t bk_h264_encode_force_idr(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->force_idr, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->force_idr(handle);
}

avdk_err_t bk_h264_encode_set_gop_frame_count(bk_h264_encode_ctlr_handle_t handle,
                                              uint32_t gop_frame_count)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_SET_GOP_FRAME_COUNT, &gop_frame_count);
}

avdk_err_t bk_h264_encode_get_gop_frame_count(bk_h264_encode_ctlr_handle_t handle,
                                              uint32_t *gop_frame_count)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(gop_frame_count, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_GET_GOP_FRAME_COUNT, gop_frame_count);
}

avdk_err_t bk_h264_encode_set_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                         bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(rate_ctrl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_SET_RATE_CTRL, rate_ctrl);
}

avdk_err_t bk_h264_encode_get_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                         bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(rate_ctrl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_GET_RATE_CTRL, rate_ctrl);
}

avdk_err_t bk_h264_encode_set_osd(bk_h264_encode_ctlr_handle_t handle,
                                  bk_h264_encode_osd_t *osd)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(osd, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_SET_OSD, osd);
}

avdk_err_t h264e_stream_encode_set_vcenc_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                                   bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(rate_ctrl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return handle->ioctl(handle, H264_ENCODE_IOCTL_SET_VCENC_RATE_CTRL_PRIV, rate_ctrl);
}

avdk_err_t h264e_stream_encode_get_vcenc_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                                   bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(rate_ctrl, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return handle->ioctl(handle, H264_ENCODE_IOCTL_GET_VCENC_RATE_CTRL_PRIV, rate_ctrl);
}

avdk_err_t bk_h264_encode_delete(bk_h264_encode_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->del(handle);
}

avdk_err_t bk_h264_encode_hw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    extern avdk_err_t bk_h264_encode_hw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);
    return bk_h264_encode_hw_flexa_ctlr_new(handle, config);
}

avdk_err_t bk_h264_encode_sw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    extern avdk_err_t bk_h264_encode_sw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);
    return bk_h264_encode_sw_flexa_ctlr_new(handle, config);
}

avdk_err_t bk_h264_encode_frame_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    extern avdk_err_t bk_h264_encode_frame_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);
    return bk_h264_encode_frame_ctlr_new(handle, config);
}