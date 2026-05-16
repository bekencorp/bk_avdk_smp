#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_frame_buffer.h>
#include "h264e_driver.h"
#include "private_h264_encode_ctlr.h"
#define TAG "h264e_drv"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define H264E_QP_LOG_TAG "bk_h264_encode_ctlr"
#define H264E_QP_LOGI(...) BK_LOGI(H264E_QP_LOG_TAG, ##__VA_ARGS__)
#define CHECK_ENC_HANDLE(handle) \
    do { \
        if((handle) == NULL || *((uint32_t *)(handle)) != H264_ENC_TAG_INIT) {\
            LOGE("%s %d handle is invalid\r\n", __func__, __LINE__);\
            return BK_FAIL; \
        } \
    } while(0)
#define CHECK_ENC_HANDLE_NO_RETURN(handle) \
    do { \
        if((handle) == NULL || *((uint32_t *)(handle)) != H264_ENC_TAG_INIT) {\
            LOGE("%s %d handle is invalid\r\n", __func__, __LINE__);\
            return ; \
        } \
    } while(0)
extern vcenc_ret_e h264_vcencoder_init(h264_enc_param_t *enc_param);
extern vcenc_ret_e h264_vcencoder_encode(h264_enc_param_t *enc_param);
extern vcenc_ret_e h264_vcencoder_stop_encode(h264_enc_param_t *enc_param);
extern vcenc_ret_e h264_vcencoder_deinit(h264_enc_param_t *enc_param);
extern vcenc_ret_e h264_vcencoder_osd_config(h264_enc_param_t *enc_param, uint32_t index, void* buffer, uint32_t format, uint8_t alpha, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern vcenc_ret_e h264_vcencoder_rate_ctrl(h264_enc_param_t *enc_param, vcenc_rate_ctrl_t *rc);
extern vcenc_ret_e h264_vcencoder_get_rate_ctrl(h264_enc_param_t *enc_param, vcenc_rate_ctrl_t *rc);
extern uint32_t h264_vcencoder_get_encoded_lines(void);

uint32_t h264e_get_encoded_lines(void)
{
    return h264_vcencoder_get_encoded_lines();
}

static void h264e_frame_end_cb(void *buffer, uint32_t size, uint32_t type, uint32_t result, uint32_t param)
{
    CHECK_ENC_HANDLE_NO_RETURN((void *)param);
    h264_encoder_context *context = (h264_encoder_context *)param;
    if (context->callback.ocb != NULL)
    {
        if (result == BK_OK)
        {
            if (type == VCENC_OUT_IFRAME)
            {
                if (context->debug_info.max_i_frame_size < size)
                {
                    context->debug_info.max_i_frame_size = size;
                }
                context->debug_info.last_i_frame_size = size;
            }
            if (type == VCENC_OUT_PFRAME)
            {
                if (context->debug_info.max_p_frame_size < size)
                {
                    context->debug_info.max_p_frame_size = size;
                }
                context->debug_info.last_p_frame_size = size;
            }
            context->debug_info.all_frame_size += size;
        }
        else
        {
            context->debug_info.dec_frame_err_cnt++;
        }
        context->callback.ocb(buffer, size, type, result, context->callback.param);
    }
}

static uint32_t h264e_slice_end_cb(uint8_t *yDst, uint8_t *uDst, uint8_t *vDst, uint32_t param)
{
    CHECK_ENC_HANDLE((void *)param);
    h264_encoder_context *context = (h264_encoder_context *)param;
    uint32_t ret = BK_OK;
    if (context->callback.fcb != NULL)
    {
        ret = context->callback.fcb(yDst, uDst, vDst, context->callback.param);
    }
    return ret;
}

static bk_err_t h264e_validate_gop_frame_count(uint32_t gop_frame_count)
{
    if (gop_frame_count == 0) {
        LOGE("%s %d invalid GOP frame count: %u\r\n",
             __func__, __LINE__, gop_frame_count);
        return BK_ERR_PARAM;
    }
    return BK_OK;
}

bk_err_t h264e_init(h264_encoder_handle_t* handle, h264_encoder_config_t* in_config)
{
    if (handle == NULL || in_config == NULL)
    {
        LOGE("%s %d invalid parameters\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }
    if (*handle != NULL)
    {
        LOGE("%s %d handle is already initialized %x\r\n", __func__, __LINE__, *handle);
        return BK_FAIL;
    }
    if (in_config->width == 0 || in_config->height == 0)
    {
        LOGE("%s %d invalid width or height\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }
    h264_encoder_context *context = (h264_encoder_context*)os_zalloc(sizeof(h264_encoder_context));
    if (context == NULL)
    {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        return BK_ERR_NO_MEM;
    }
    // os_zalloc already zeroes the memory, no need for os_memset
    context->debug_info.all_frame_count = 0;
    context->debug_info.dec_frame_err_cnt = 0;
    context->param.enc_mode = in_config->flexa_mode;
	context->param.idr_interval = in_config->idr_interval;
	context->param.slice_count = in_config->buf_cnt;
	context->param.width = in_config->width;
	context->param.height = in_config->height;
	context->param.in_type = VCENC_INPUT_NV12;
    context->param.slice_done_cb = h264e_slice_end_cb;
    context->param.frame_done_cb = h264e_frame_end_cb;
    context->param.args = (uint32_t)context;
    vcenc_ret_e ret = h264_vcencoder_init(&context->param);
    if (ret != VCENC_OK)
    {
        LOGE("%s %d h264_vcencoder_init failed with error %d\r\n", __func__, __LINE__, ret);
        os_free(context);
        return BK_FAIL;
    }
    os_memcpy(&context->config, in_config, sizeof(h264_encoder_config_t));
    context->enc_tag = H264_ENC_TAG_INIT;
    *handle = context;
    return BK_OK;
}

bk_err_t h264e_deinit(h264_encoder_handle_t* handle)
{
    bk_err_t ret = BK_OK;
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_context *context = (h264_encoder_context*)*handle;
    context->enc_tag = H264_ENC_TAG_DEINIT;
    h264_vcencoder_deinit(&context->param);
    os_free(context);
    *handle = NULL;
    return ret;
}

bk_err_t h264e_register_callback(h264_encoder_handle_t* handle, h264_encoder_callback_t *callback)
{
    CHECK_ENC_HANDLE(*handle);
    
    if (callback == NULL)
    {
        LOGE("%s %d callback is NULL\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }
    
    h264_encoder_context *context = (h264_encoder_context*)*handle;
    os_memcpy(&context->callback, callback, sizeof(h264_encoder_callback_t));
    return BK_OK;
}

bk_err_t h264e_deregister_callback(h264_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_context *context = (h264_encoder_context*)*handle;
    os_memset(&context->callback, 0, sizeof(h264_encoder_callback_t));
    return BK_OK;
}

bk_err_t h264e_open(h264_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_rate_ctrl_t h264e_rate_ctrl = {0};
    h264e_rate_ctrl.bitrate = 0;
    h264e_rate_ctrl.qp_min_i = 23;
    h264e_rate_ctrl.qp_max_i = 23;
    h264e_rate_ctrl.qp_min_p = 26;
    h264e_rate_ctrl.qp_max_p = 26;
    h264e_set_rate_ctrl(handle, &h264e_rate_ctrl);
    return BK_OK;
}

bk_err_t h264e_close(h264_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);
    // Reserved for future implementation
    return BK_OK;
}

bk_err_t h264e_set_force_idr(h264_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_context* context = (h264_encoder_context*)*handle;
    context->force_idr = true;
    return BK_OK;
}

bk_err_t h264e_start_encode(h264_encoder_handle_t* handle, h264_encoder_parameters_t* para)
{
    CHECK_ENC_HANDLE(*handle);
    
    if (para == NULL)
    {
        LOGE("%s %d parameters is NULL\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }
    if (para->out_buf == 0 || para->pic_buf == 0)
    {
        LOGE("%s %d invalid buffers %p %p\r\n", __func__, __LINE__, para->out_buf, para->pic_buf);
        return BK_ERR_PARAM;
    }
    h264_encoder_context* context = (h264_encoder_context*)*handle;
    context->param.out_buffer = (uint32_t)para->out_buf;
    context->param.out_len = para->out_size;
    context->param.in_buffer = para->pic_buf;
    context->param.in_lines = para->pic_lines;
    context->param.force_idr_flag = context->force_idr;
    context->force_idr = 0;
    context->debug_info.all_frame_count++;
    vcenc_ret_e ret = h264_vcencoder_encode(&context->param);
    context->param.update_flag = 0;
    if (ret == VCENC_FRAME_READY)
    {
        return BK_OK;
    }
    return ret;
}

bk_err_t h264e_stop_encode(h264_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);

    h264_encoder_context* context = (h264_encoder_context*)*handle;
    vcenc_ret_e ret = h264_vcencoder_stop_encode(&context->param);
    if (ret != VCENC_OK)
    {
        LOGE("%s %d h264_vcencoder_stop_encode failed with error %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }
    return BK_OK;
}

bk_err_t h264e_set_gop_frame_count(h264_encoder_handle_t* handle, uint32_t gop_frame_count)
{
    CHECK_ENC_HANDLE(*handle);
    bk_err_t ret = h264e_validate_gop_frame_count(gop_frame_count);
    if (ret != BK_OK) {
        return ret;
    }

    h264_encoder_context* context = (h264_encoder_context*)*handle;
    context->param.idr_interval = gop_frame_count;
    context->param.update_flag = 1;
    return BK_OK;
}

bk_err_t h264e_get_gop_frame_count(h264_encoder_handle_t* handle, uint32_t *gop_frame_count)
{
    CHECK_ENC_HANDLE(*handle);
    if (gop_frame_count == NULL) {
        LOGE("%s %d gop_frame_count is NULL\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }

    h264_encoder_context* context = (h264_encoder_context*)*handle;
    *gop_frame_count = context->param.idr_interval;
    return BK_OK;
}

bk_err_t h264e_set_rate_ctrl(h264_encoder_handle_t* handle, h264_encoder_rate_ctrl_t* rate_ctrl)
{
    CHECK_ENC_HANDLE(*handle);

    if (rate_ctrl == NULL)
    {
        LOGE("%s %d rate_ctrl is NULL\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }

    if (rate_ctrl->qp_min_i > 51 || rate_ctrl->qp_max_i > 51 ||
        rate_ctrl->qp_min_p > 51 || rate_ctrl->qp_max_p > 51 ||
        (rate_ctrl->qp_min_i && rate_ctrl->qp_max_i && rate_ctrl->qp_min_i > rate_ctrl->qp_max_i) ||
        (rate_ctrl->qp_min_p && rate_ctrl->qp_max_p && rate_ctrl->qp_min_p > rate_ctrl->qp_max_p))
    {
        LOGE("%s %d invalid rate_ctrl, bitrate=%u i=[%u,%u] p=[%u,%u]\r\n",
             __func__, __LINE__, rate_ctrl->bitrate,
             rate_ctrl->qp_min_i, rate_ctrl->qp_max_i,
             rate_ctrl->qp_min_p, rate_ctrl->qp_max_p);
        return BK_ERR_PARAM;
    }

    h264_encoder_context* context = (h264_encoder_context*)*handle;
    vcenc_rate_ctrl_t vcenc_rc;
    vcenc_ret_e ret = h264_vcencoder_get_rate_ctrl(&context->param, &vcenc_rc);
    if (ret != VCENC_OK)
    {
        LOGE("%s %d h264_vcencoder_get_rate_ctrl failed with error %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    uint8_t has_qp_config = rate_ctrl->qp_min_i || rate_ctrl->qp_max_i ||
                            rate_ctrl->qp_min_p || rate_ctrl->qp_max_p;
    uint8_t fixed_qp = (rate_ctrl->bitrate == 0) ||
                       (has_qp_config &&
                        rate_ctrl->qp_min_i == rate_ctrl->qp_max_i &&
                        rate_ctrl->qp_min_p == rate_ctrl->qp_max_p);

    if (fixed_qp)
    {
        uint8_t qp_i = rate_ctrl->qp_min_i;
        uint8_t qp_p = rate_ctrl->qp_min_p;
        vcenc_rc.qp_min_i = qp_i;
        vcenc_rc.qp_max_i = qp_i;
        vcenc_rc.qp_min_pb = qp_p;
        vcenc_rc.qp_max_pb = qp_p;
    }
    else
    {
        vcenc_rc.qp_min_i = rate_ctrl->qp_min_i;
        vcenc_rc.qp_max_i = rate_ctrl->qp_max_i;
        vcenc_rc.qp_min_pb = rate_ctrl->qp_min_p;
        vcenc_rc.qp_max_pb = rate_ctrl->qp_max_p;
        if (vcenc_rc.qp_min_i > vcenc_rc.qp_max_i || vcenc_rc.qp_min_pb > vcenc_rc.qp_max_pb)
        {
            LOGE("%s %d invalid effective rate_ctrl, bitrate=%u i=[%u,%u] p=[%u,%u]\r\n",
                 __func__, __LINE__, rate_ctrl->bitrate,
                 vcenc_rc.qp_min_i, vcenc_rc.qp_max_i,
                 vcenc_rc.qp_min_pb, vcenc_rc.qp_max_pb);
            return BK_ERR_PARAM;
        }
    }
    if (fixed_qp)
    {
        vcenc_rc.qp_hdr = vcenc_rc.qp_min_i;
        vcenc_rc.picture_rc = 0;
        vcenc_rc.bit_per_second = 0;
    }
    else
    {
        vcenc_rc.qp_hdr = -1;
        vcenc_rc.picture_rc = 1;
        vcenc_rc.bit_per_second = rate_ctrl->bitrate;
    }

    ret = h264_vcencoder_rate_ctrl(&context->param, &vcenc_rc);
    if (ret != VCENC_OK)
    {
        LOGE("%s %d h264_vcencoder_rate_ctrl failed with error %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t h264e_get_rate_ctrl(h264_encoder_handle_t* handle, h264_encoder_rate_ctrl_t* rate_ctrl)
{
    CHECK_ENC_HANDLE(*handle);

    if (rate_ctrl == NULL)
    {
        LOGE("%s %d rate_ctrl is NULL\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }

    h264_encoder_context* context = (h264_encoder_context*)*handle;
    vcenc_rate_ctrl_t vcenc_rc;
    vcenc_ret_e ret = h264_vcencoder_get_rate_ctrl(&context->param, &vcenc_rc);
    if (ret != VCENC_OK)
    {
        LOGE("%s %d h264_vcencoder_get_rate_ctrl failed with error %d\r\n", __func__, __LINE__, ret);
        return BK_FAIL;
    }

    rate_ctrl->qp_min_i = vcenc_rc.qp_min_i;
    rate_ctrl->qp_max_i = vcenc_rc.qp_max_i;
    rate_ctrl->qp_min_p = vcenc_rc.qp_min_pb;
    rate_ctrl->qp_max_p = vcenc_rc.qp_max_pb;
    rate_ctrl->bitrate = vcenc_rc.picture_rc ? vcenc_rc.bit_per_second : 0;

    return BK_OK;
}

uint32_t h264e_get_flexa_mode(h264_encoder_handle_t* handle)
{
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_context* context = (h264_encoder_context*)*handle;
    return context->config.flexa_mode;
}

bk_err_t h264e_get_debug_info(h264_encoder_handle_t* handle, enc_h264_debug_t **debug_info)
{
    CHECK_ENC_HANDLE(*handle);
    
    if (debug_info == NULL)
    {
        LOGE("%s %d debug_info is NULL\r\n", __func__, __LINE__);
        return BK_ERR_PARAM;
    }
    
    h264_encoder_context* context = (h264_encoder_context*)*handle;
    *debug_info = &context->debug_info;
    return BK_OK;
}

extern vcenc_ret_e h264_vcencoder_update_slice_wr(h264_enc_param_t *enc_param, uint32_t slice_id);
bk_err_t h264e_flexa_input_linebuf_wrcnt_set(h264_encoder_handle_t* handle, uint32_t wrcnt)
{
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_context* context = (h264_encoder_context *)*handle;
    h264_vcencoder_update_slice_wr(&context->param, wrcnt);
    return BK_OK;
}

bk_err_t h264e_osd_config(h264_encoder_handle_t* handle, uint32_t index, void* buffer, uint32_t format, uint8_t alpha, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    CHECK_ENC_HANDLE(*handle);
    h264_encoder_context* context = (h264_encoder_context *)*handle;
    h264_vcencoder_osd_config(&context->param, index, buffer, format, alpha, x, y, width, height);
    return BK_OK;
}