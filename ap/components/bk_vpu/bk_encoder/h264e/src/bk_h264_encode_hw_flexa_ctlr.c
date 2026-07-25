#include <os/os.h>
#include <os/mem.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "components/bk_encode/bk_h264_encode_types.h"
#include "modules/vcenc/vcenc_types.h"
#include "modules/vcenc/vcenc_h264_api.h"
#include "private_h264_encode_ctlr.h"
#include "h264_encode_vcenc_rate_ctrl_priv.h"
#include "hw_encoder_ctlr.h"
#include <components/bk_frame_buffer.h>
#include "avdk_monitor.h"

#define TAG "bk_h264_encode_ctlr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Handle encoding failure
static void handle_encode_error(private_h264_encode_hw_flexa_ctlr_t *ctrl, void *buffer, uint32_t size)
{
    if (buffer != NULL && ctrl->config.outbuf_complete && ctrl->pending_valid) {
        bk_h264_encode_outbuf_info_t info = {
            .outbuf = buffer,
            .length = size,
            .type = 0,
            .status = BK_FAIL,
            .sequence = 0,
            .args = ctrl->config.outbuf_complete_args,
        };
        ctrl->config.outbuf_complete(&info);
        ctrl->pending_out_buf = 0;
    }
    ctrl->force_idr = true;
}

// Handle video frames (I-frame or P-frame)
static void handle_video_frame(private_h264_encode_hw_flexa_ctlr_t *ctrl, void *buffer, uint32_t size, uint32_t type)
{
    void *next_buffer = NULL;
    if (ctrl->config.outbuf_malloc) {
        next_buffer = ctrl->config.outbuf_malloc(CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER, ctrl->config.outbuf_malloc_args);
        if (!next_buffer) {
            handle_encode_error(ctrl, buffer, size);
            return;
        }
    }

    if (ctrl->config.outbuf_complete && ctrl->pending_valid) {
        bk_h264_encode_outbuf_info_t info = {
            .outbuf = buffer,
            .length = size,
            .type = type,
            .status = BK_OK,
            .sequence = ctrl->debug_info.all_frame_count,
            .args = ctrl->config.outbuf_complete_args,
        };
        ctrl->config.outbuf_complete(&info);
        ctrl->pending_out_buf = (uint32_t)next_buffer;
        ctrl->pending_out_size = CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER;
    }
}

/*
 * H.264 frame-done callback, registered directly on enc_param.frame_done_cb.
 * Debug-stats accumulation that used to live in h264e_driver's
 * h264e_frame_end_cb is folded in here.
 */
static void h264e_end_cb(void *buffer, uint32_t size, uint32_t type, uint32_t result, uint32_t param)
{
    ENCODE_FRAME_DONE;
    if (!param || !buffer) {
        LOGE("Invalid parameters in h264e_end_cb\r\n");
        return;
    }

    if (type != VCENC_OUT_IFRAME && type != VCENC_OUT_PFRAME) {
        LOGE("Invalid frame type in h264e_end_cb\r\n");
        return;
    }

    private_h264_encode_hw_flexa_ctlr_t *ctrl = (private_h264_encode_hw_flexa_ctlr_t *)param;

    if (ctrl->debug_time_ms != 0) {
        if (result == BK_OK) {
            ctrl->debug_info.enc_frame_ok_cnt++;
            if (type == VCENC_OUT_IFRAME) {
                if (ctrl->debug_info.max_i_frame_size < size) {
                    ctrl->debug_info.max_i_frame_size = size;
                }
                ctrl->debug_info.last_i_frame_size = size;
            } else if (type == VCENC_OUT_PFRAME) {
                if (ctrl->debug_info.max_p_frame_size < size) {
                    ctrl->debug_info.max_p_frame_size = size;
                }
                ctrl->debug_info.last_p_frame_size = size;
            }
            ctrl->debug_info.all_frame_size += size;
            h264_encode_debug_update_qp(&ctrl->debug_info, type,
                                        vcenc_h264_get_current_qp(&ctrl->enc_param));
        } else {
            ctrl->debug_info.enc_frame_err_cnt++;
        }
    }

    if (result != BK_OK) {
        handle_encode_error(ctrl, buffer, size);
        return;
    }

    handle_video_frame(ctrl, buffer, size, type);
}

// Callback run in hw_encoder task: start one frame encode
static avdk_err_t h264_encode_msg_callback(void *param)
{
    if (param == NULL) {
        LOGE("Encode callback param is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    private_h264_encode_hw_flexa_ctlr_t *ctrl = (private_h264_encode_hw_flexa_ctlr_t *)param;
    if (!ctrl->pending_valid) {
        LOGW("No encoder parameters available\r\n");
        return AVDK_ERR_INVAL;
    }
    ENCODE_FRAME_START;
    ctrl->enc_param.in_buffer = ctrl->pending_in_buf;
    ctrl->enc_param.in_lines = ctrl->pending_in_lines;
    ctrl->enc_param.out_buffer = ctrl->pending_out_buf;
    ctrl->enc_param.out_len = ctrl->pending_out_size;
    ctrl->enc_param.force_idr_flag = ctrl->force_idr ? 1U : 0U;
    ctrl->force_idr = false;
    ctrl->debug_info.all_frame_count++;
    vcenc_ret_e venc_ret = vcenc_h264_encode_frame(&ctrl->enc_param);
    ctrl->enc_param.update_flag = 0;
    if (venc_ret != VCENC_FRAME_READY && venc_ret != VCENC_OK) {
        LOGE("vcenc_h264_encode_frame failed: %d\r\n", venc_ret);
        ctrl->encode_result = (uint32_t)BK_FAIL;
        ENCODE_FRAME_END;
        /*
         * Drain a possibly-posted enc_start_sem to keep the encoder thread
         * from spinning on a frame whose msg dispatch already failed. Matches
         * the legacy h264e_driver-backed behaviour.
         */
        rtos_get_semaphore(&ctrl->enc_start_sem, BEKEN_NO_WAIT);
        return AVDK_ERR_GENERIC;
    }
    ENCODE_FRAME_END;
    ctrl->encode_result = BK_OK;
    return AVDK_ERR_OK;
}

// Encoding thread entry
static void h264_encoder_entry(void *arg)
{
    private_h264_encode_hw_flexa_ctlr_t *ctrl = (private_h264_encode_hw_flexa_ctlr_t *)arg;
    if (ctrl == NULL) {
        LOGE("Encoder thread started with NULL context\r\n");
        return;
    }

    rtos_set_semaphore(&ctrl->sem);

    ctrl->pending_in_buf = 0;
    ctrl->pending_in_lines = 0;
    ctrl->pending_out_buf = 0;
    ctrl->pending_out_size = 0;
    ctrl->pending_valid = true;
    while (ctrl->enc_status) {
        rtos_get_semaphore(&ctrl->enc_start_sem, BEKEN_WAIT_FOREVER);
        if (!ctrl->enc_status) {
            break;
        }
        if (ctrl->pending_out_buf == 0 && ctrl->config.outbuf_malloc != NULL) {
            void *temp_buffer = ctrl->config.outbuf_malloc(CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER, ctrl->config.outbuf_malloc_args);
            if (temp_buffer != NULL) {
                ctrl->pending_out_buf = (uint32_t)temp_buffer;
                ctrl->pending_out_size = CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER;
            } else {
                ctrl->pending_out_buf = 0;
                ctrl->pending_out_size = 0;
            }
        }
        if (ctrl->pending_out_buf == 0) {
            LOGW("Failed to get output buffer, skip this frame\r\n");
            handle_encode_error(ctrl, (void *)ctrl->pending_out_buf, 0);
            if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL) {
                ctrl->bond->frame_done(BK_FAIL, ctrl->bond);
            }
            continue;
        }
        ctrl->pending_in_buf = ctrl->config.input_buf;
        ctrl->pending_in_lines = ctrl->config.input_size;

        hw_encoder_msg_t msg = {
            .type = HW_ENCODER_MSG_ENCODE,
            .encoder_type = HW_ENCODER_TYPE_H264,
            .callback = h264_encode_msg_callback,
            .param = ctrl,
            .sem = &ctrl->enc_done_sem
        };
        avdk_err_t ret = hw_encoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
        if (ret != AVDK_ERR_OK) {
            LOGE("hw_encoder_send_msg failed: %d\r\n", ret);
            handle_encode_error(ctrl, (void *)ctrl->pending_out_buf, 0);
            continue;
        }
        ret = rtos_get_semaphore(&ctrl->enc_done_sem, 1000);
        if (ret != BK_OK) {
            LOGE("get semaphore failed: %d\r\n", ret);
            handle_encode_error(ctrl, (void *)ctrl->pending_out_buf, 0);
            if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL) {
                ctrl->bond->frame_done(BK_FAIL, ctrl->bond);
            }
            continue;
        }
        if (ctrl->bond != NULL && ctrl->bond->frame_done != NULL) {
            if (ctrl->encode_result != BK_OK) {
                ctrl->bond->frame_done(BK_FAIL, ctrl->bond);
            } else {
                ctrl->bond->frame_done(BK_OK, ctrl->bond);
            }
        }
    }
    // Clean up resources
    if (ctrl->pending_out_buf != 0) {
        handle_encode_error(ctrl, (void *)ctrl->pending_out_buf, 0);
    }
    ctrl->pending_valid = false;
    rtos_set_semaphore(&ctrl->sem);
    rtos_delete_thread(NULL);
}

static avdk_err_t h264_encode_ctlr_init(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    avdk_err_t ret = hw_encoder_register(HW_ENCODER_TYPE_H264, control);
    if (ret != AVDK_ERR_OK) {
        LOGE("Register to hw controller failed: %d\r\n", ret);
        return ret;
    }
    ret = rtos_init_semaphore(&control->sem, 1);
    if (ret != BK_OK) {
        LOGE("Init semaphore failed\r\n");
        hw_encoder_unregister(control);
        return AVDK_ERR_GENERIC;
    }
    ret = rtos_init_semaphore(&control->enc_start_sem, 1);
    if (ret != BK_OK) {
        LOGE("Init enc_start_sem failed\r\n");
        rtos_deinit_semaphore(&control->sem);
        hw_encoder_unregister(control);
        return AVDK_ERR_GENERIC;
    }
    ret = rtos_init_semaphore(&control->enc_done_sem, 1);
    if (ret != BK_OK) {
        LOGE("Init enc_done_sem failed\r\n");
        rtos_deinit_semaphore(&control->enc_start_sem);
        rtos_deinit_semaphore(&control->sem);
        hw_encoder_unregister(control);
        return AVDK_ERR_GENERIC;
    }
    LOGI("H.264 encoder registered to hw controller\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_open(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    LOGI("Start H.264 encoder %dx%d, mode=%d\r\n",
         control->config.width, control->config.height, BK_H264_ENCODE_FLEXA_MODE_HARDWARE);

    os_memset(&control->enc_param, 0, sizeof(control->enc_param));
    control->enc_param.enc_mode = (vcenc_mode_e)BK_H264_ENCODE_FLEXA_MODE_HARDWARE;
    control->enc_param.idr_interval = control->config.gop_frame_count;
    control->enc_param.slice_count = control->config.input_flexa_cnt;
    control->enc_param.width = (uint16_t)control->config.width;
    control->enc_param.height = (uint16_t)control->config.height;
    control->enc_param.in_type = VCENC_INPUT_NV12;
    control->enc_param.slice_done_cb = NULL;
    control->enc_param.frame_done_cb = h264e_end_cb;
    control->enc_param.args = (uint32_t)control;

    vcenc_ret_e venc_ret = vcenc_h264_init(&control->enc_param);
    if (venc_ret != VCENC_OK) {
        LOGE("vcenc_h264_init failed: %d\r\n", venc_ret);
        return AVDK_ERR_GENERIC;
    }
    venc_ret = vcenc_h264_open(&control->enc_param);
    if (venc_ret != VCENC_OK) {
        LOGE("vcenc_h264_open failed: %d\r\n", venc_ret);
        (void)vcenc_h264_deinit(&control->enc_param);
        return AVDK_ERR_GENERIC;
    }
    control->encoder_inited = true;

    /* Apply legacy open-time fixed-QP defaults. */
    vcenc_rate_ctrl_t rc;
    if (vcenc_h264_get_rate_ctrl(&control->enc_param, &rc) == VCENC_OK) {
        rc.qp_min_i = H264_ENCODE_DEFAULT_OPEN_QP_I;
        rc.qp_max_i = H264_ENCODE_DEFAULT_OPEN_QP_I;
        rc.qp_min_pb = H264_ENCODE_DEFAULT_OPEN_QP_P;
        rc.qp_max_pb = H264_ENCODE_DEFAULT_OPEN_QP_P;
        rc.qp_hdr = (int)rc.qp_min_i;
        rc.picture_rc = 0;
        rc.bit_per_second = 0;
        (void)vcenc_h264_set_rate_ctrl(&control->enc_param, &rc);
    }

    control->enc_line_cnt = 0;
    control->enc_status = 1;
    control->enc_start_first = 1;
    bk_err_t ret = rtos_create_hsram_thread(&control->thread,
                           2,
                           "h264e_encoder",
                           (beken_thread_function_t)h264_encoder_entry,
                           CONFIG_BK_ENCODER_H264_TASK_SIZE,
                           control);
    if (ret != BK_OK) {
        LOGE("Create thread failed: %d\r\n", ret);
        control->enc_status = 0;
        (void)vcenc_h264_close(&control->enc_param);
        (void)vcenc_h264_deinit(&control->enc_param);
        control->encoder_inited = false;
        return AVDK_ERR_GENERIC;
    }
    rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);

    LOGI("H.264 encoder opened successfully\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_encode(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_close(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    control->enc_status = 0;
    rtos_set_semaphore(&control->enc_start_sem);
    rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);

    if (rtos_is_timer_init(&control->debug_timer)) {
        bk_err_t ret = rtos_stop_timer(&control->debug_timer);
        if (ret != BK_OK) {
            LOGE("Stop timer failed: %d\r\n", ret);
        }

        ret = rtos_deinit_timer(&control->debug_timer);
        if (ret != BK_OK) {
            LOGE("Deinit timer failed: %d\r\n", ret);
        }

        control->debug_time_ms = 0;
        LOGI("H.264 debug stopped\r\n");
    }

    if (control->encoder_inited) {
        (void)vcenc_h264_close(&control->enc_param);
        (void)vcenc_h264_deinit(&control->enc_param);
        control->encoder_inited = false;
    }
    LOGI("H.264 encoder closed\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_deinit(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    rtos_deinit_semaphore(&control->sem);
    rtos_deinit_semaphore(&control->enc_start_sem);
    rtos_deinit_semaphore(&control->enc_done_sem);
    avdk_err_t ret = hw_encoder_unregister(control);
    if (ret != AVDK_ERR_OK) {
        LOGE("Unregister from hw controller failed: %d\r\n", ret);
        return ret;
    }
    LOGI("H.264 encoder unregistered from hw controller\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_force_idr(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    control->force_idr = true;
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_delete(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    os_free(control);
    LOGI("H.264 encoder deleted\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_set_gop_frame_count(private_h264_encode_hw_flexa_ctlr_t *control,
                                                       uint32_t gop_frame_count)
{
    if (gop_frame_count == 0) {
        LOGE("invalid GOP frame count: %u\r\n", gop_frame_count);
        return AVDK_ERR_INVAL;
    }

    control->config.gop_frame_count = gop_frame_count;
    if (control->encoder_inited) {
        control->enc_param.idr_interval = gop_frame_count;
        control->enc_param.update_flag = 1;
    }

    LOGI("H.264 GOP frame count set, count=%u\r\n", gop_frame_count);
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_get_gop_frame_count(private_h264_encode_hw_flexa_ctlr_t *control,
                                                       uint32_t *gop_frame_count)
{
    if (gop_frame_count == NULL) {
        LOGE("GOP frame count arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    if (control->encoder_inited) {
        *gop_frame_count = control->enc_param.idr_interval;
    } else {
        *gop_frame_count = control->config.gop_frame_count;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_set_rate_ctrl(private_h264_encode_hw_flexa_ctlr_t *control,
                                                 bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    if (rate_ctrl == NULL) {
        LOGE("rate_ctrl arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!control->encoder_inited) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }
    if (rate_ctrl->qp_min_i > 51 || rate_ctrl->qp_max_i > 51 ||
        rate_ctrl->qp_min_p > 51 || rate_ctrl->qp_max_p > 51 ||
        (rate_ctrl->qp_min_i && rate_ctrl->qp_max_i && rate_ctrl->qp_min_i > rate_ctrl->qp_max_i) ||
        (rate_ctrl->qp_min_p && rate_ctrl->qp_max_p && rate_ctrl->qp_min_p > rate_ctrl->qp_max_p)) {
        LOGE("invalid rate_ctrl, bitrate=%u i=[%u,%u] p=[%u,%u]\r\n",
             rate_ctrl->bitrate,
             rate_ctrl->qp_min_i, rate_ctrl->qp_max_i,
             rate_ctrl->qp_min_p, rate_ctrl->qp_max_p);
        return AVDK_ERR_INVAL;
    }

    vcenc_rate_ctrl_t vcenc_rc;
    if (vcenc_h264_get_rate_ctrl(&control->enc_param, &vcenc_rc) != VCENC_OK) {
        LOGE("vcenc_h264_get_rate_ctrl failed\r\n");
        return AVDK_ERR_GENERIC;
    }

    uint8_t has_qp_config = rate_ctrl->qp_min_i || rate_ctrl->qp_max_i ||
                            rate_ctrl->qp_min_p || rate_ctrl->qp_max_p;
    uint8_t fixed_qp = (rate_ctrl->bitrate == 0) ||
                       (has_qp_config &&
                        rate_ctrl->qp_min_i == rate_ctrl->qp_max_i &&
                        rate_ctrl->qp_min_p == rate_ctrl->qp_max_p);

    if (fixed_qp) {
        uint8_t qp_i = rate_ctrl->qp_min_i;
        uint8_t qp_p = rate_ctrl->qp_min_p;
        vcenc_rc.qp_min_i = qp_i;
        vcenc_rc.qp_max_i = qp_i;
        vcenc_rc.qp_min_pb = qp_p;
        vcenc_rc.qp_max_pb = qp_p;
        vcenc_rc.qp_hdr = (int)qp_i;
        vcenc_rc.picture_rc = 0;
        vcenc_rc.bit_per_second = 0;
    } else {
        vcenc_rc.qp_min_i = rate_ctrl->qp_min_i;
        vcenc_rc.qp_max_i = rate_ctrl->qp_max_i;
        vcenc_rc.qp_min_pb = rate_ctrl->qp_min_p;
        vcenc_rc.qp_max_pb = rate_ctrl->qp_max_p;
        if (vcenc_rc.qp_min_i > vcenc_rc.qp_max_i || vcenc_rc.qp_min_pb > vcenc_rc.qp_max_pb) {
            LOGE("invalid effective rate_ctrl, bitrate=%u i=[%u,%u] p=[%u,%u]\r\n",
                 rate_ctrl->bitrate,
                 vcenc_rc.qp_min_i, vcenc_rc.qp_max_i,
                 vcenc_rc.qp_min_pb, vcenc_rc.qp_max_pb);
            return AVDK_ERR_INVAL;
        }
        vcenc_rc.qp_hdr = -1;
        vcenc_rc.picture_rc = 1;
        vcenc_rc.bit_per_second = rate_ctrl->bitrate;
    }

    if (vcenc_h264_set_rate_ctrl(&control->enc_param, &vcenc_rc) != VCENC_OK) {
        LOGE("vcenc_h264_set_rate_ctrl failed\r\n");
        return AVDK_ERR_GENERIC;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_get_rate_ctrl(private_h264_encode_hw_flexa_ctlr_t *control,
                                                 bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    if (rate_ctrl == NULL) {
        LOGE("rate_ctrl arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!control->encoder_inited) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }

    vcenc_rate_ctrl_t vcenc_rc;
    if (vcenc_h264_get_rate_ctrl(&control->enc_param, &vcenc_rc) != VCENC_OK) {
        LOGE("vcenc_h264_get_rate_ctrl failed\r\n");
        return AVDK_ERR_GENERIC;
    }

    rate_ctrl->qp_min_i = (uint8_t)vcenc_rc.qp_min_i;
    rate_ctrl->qp_max_i = (uint8_t)vcenc_rc.qp_max_i;
    rate_ctrl->qp_min_p = (uint8_t)vcenc_rc.qp_min_pb;
    rate_ctrl->qp_max_p = (uint8_t)vcenc_rc.qp_max_pb;
    rate_ctrl->bitrate = vcenc_rc.picture_rc ? vcenc_rc.bit_per_second : 0U;
    return AVDK_ERR_OK;
}


// Debug timer callback
static void h264e_debug_callback(void *arg)
{
    private_h264_encode_hw_flexa_ctlr_t *ctrl = (private_h264_encode_hw_flexa_ctlr_t *)arg;
    if (ctrl == NULL) {
        return;
    }
    if (ctrl->debug_time_ms == 0) {
        LOGE("Invalid debug_time_ms\r\n");
        return;
    }
    h264_encode_debug_info_t *debug = &ctrl->debug_info;
    uint32_t frame_count = (debug->all_frame_count - ctrl->last_debug_info.all_frame_count) * 1000 / ctrl->debug_time_ms;
    uint32_t bytes_per_second = (debug->all_frame_size - ctrl->last_debug_info.all_frame_size) * 1000 / ctrl->debug_time_ms;
    uint32_t bit_rate_kbps = bytes_per_second * 8 / 1024;
    uint32_t enc_err_cnt = debug->enc_frame_err_cnt - ctrl->last_debug_info.enc_frame_err_cnt;
    uint32_t enc_ok_cnt = debug->enc_frame_ok_cnt - ctrl->last_debug_info.enc_frame_ok_cnt;
    LOGI("%s %d(fps:%d\t%dBytes/s\tbit_rate:%dkbps\tmax_i:%d\tmax_p:%d\t"
         "i_qp[%u-%u]\tp_qp[%u-%u]\tlast_qp:%u\tenc_ok:%u\tenc_err:%u)\n", __func__, __LINE__,
         frame_count, bytes_per_second, bit_rate_kbps,
         debug->max_i_frame_size, debug->max_p_frame_size,
         debug->min_i_qp <= 51U ? debug->min_i_qp : 0U, debug->max_i_qp,
         debug->min_p_qp <= 51U ? debug->min_p_qp : 0U, debug->max_p_qp,
         debug->last_frame_qp, enc_ok_cnt, enc_err_cnt);
    os_memcpy(&ctrl->last_debug_info, debug, sizeof(*debug));
    h264_encode_debug_info_reset_qp(debug);
}

static avdk_err_t h264_encode_ctlr_ioctl(bk_h264_encode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
    private_h264_encode_hw_flexa_ctlr_t *control =  __containerof(handle, private_h264_encode_hw_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    switch (cmd) {
        case BK_H264_ENCODE_IOCTL_DEBUG_START: {
            uint32_t time_ms = arg ? *(uint32_t *)arg : 1000;
            if (time_ms == 0 || time_ms > 60000) {
                LOGE("Invalid debug interval: %dms (valid range: 1-60000)\r\n", time_ms);
                return AVDK_ERR_INVAL;
            }
            if (rtos_is_timer_init(&control->debug_timer)) {
                LOGW("Debug timer already running\r\n");
                return AVDK_ERR_OK;
            }
            bk_err_t ret = rtos_init_timer(&control->debug_timer, time_ms, h264e_debug_callback, control);
            if (ret != BK_OK) {
                LOGE("Init timer failed: %d\r\n", ret);
                return AVDK_ERR_INVAL;
            }
            ret = rtos_start_timer(&control->debug_timer);
            if (ret != BK_OK) {
                LOGE("Start timer failed: %d\r\n", ret);
                rtos_deinit_timer(&control->debug_timer);
                return AVDK_ERR_INVAL;
            }
            control->debug_time_ms = time_ms;
            os_memset(&control->last_debug_info, 0, sizeof(control->last_debug_info));
            LOGI("H.264 debug started, interval=%dms\r\n", time_ms);
            break;
        }
        case BK_H264_ENCODE_IOCTL_DEBUG_STOP: {
            if (!rtos_is_timer_init(&control->debug_timer)) {
                LOGW("Debug timer not running\r\n");
                return AVDK_ERR_OK;
            }
            bk_err_t ret = rtos_stop_timer(&control->debug_timer);
            if (ret != BK_OK) {
                LOGE("Stop timer failed: %d\r\n", ret);
            }
            ret = rtos_deinit_timer(&control->debug_timer);
            if (ret != BK_OK) {
                LOGE("Deinit timer failed: %d\r\n", ret);
            }
            control->debug_time_ms = 0;
            LOGI("H.264 debug stopped\r\n");
            break;
        }
        case BK_H264_ENCODE_IOCTL_SET_GOP_FRAME_COUNT:
            if (arg == NULL) {
                LOGE("GOP frame count arg is NULL\r\n");
                return AVDK_ERR_INVAL;
            }
            return h264_encode_ctlr_set_gop_frame_count(control, *(uint32_t *)arg);
        case BK_H264_ENCODE_IOCTL_GET_GOP_FRAME_COUNT:
            return h264_encode_ctlr_get_gop_frame_count(control, (uint32_t *)arg);
        case BK_H264_ENCODE_IOCTL_SET_RATE_CTRL:
            return h264_encode_ctlr_set_rate_ctrl(control, (bk_h264_encode_rate_ctrl_t *)arg);
        case BK_H264_ENCODE_IOCTL_GET_RATE_CTRL:
            return h264_encode_ctlr_get_rate_ctrl(control, (bk_h264_encode_rate_ctrl_t *)arg);
        case H264_ENCODE_IOCTL_SET_VCENC_RATE_CTRL_PRIV:
            return h264_encode_set_vcenc_rate_ctrl_common(&control->enc_param, control->encoder_inited, (bk_h264_encode_vcenc_rate_ctrl_t *)arg);
        case H264_ENCODE_IOCTL_GET_VCENC_RATE_CTRL_PRIV:
            return h264_encode_get_vcenc_rate_ctrl_common(&control->enc_param, control->encoder_inited, (bk_h264_encode_vcenc_rate_ctrl_t *)arg);
        case BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY: {
            if (control->encoder_inited) {
                (void)vcenc_h264_update_slice_wr_cnt(&control->enc_param, (uint32_t)arg);
            }
            break;
        }
        case BK_H264_ENCODE_IOCTL_SET_FRAME_READY: {
            rtos_set_semaphore(&control->enc_start_sem);
            break;
        }
        case BK_H264_ENCODE_IOCTL_REGISTER_BOND: {
            bk_flexa_bond_t *bond = (bk_flexa_bond_t *)arg;
            if (bond == NULL) {
                LOGW("%s %d bond is NULL\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            else if (control->bond != NULL) {
                LOGW("%s %d bond is already registered\r\n", __func__, __LINE__);
                return AVDK_ERR_OK;
            }
            else {
                control->bond = bond;
            }
            break;
        }
        case BK_H264_ENCODE_IOCTL_UNREGISTER_BOND: {
            bk_flexa_bond_t *bond = (bk_flexa_bond_t *)arg;
            if (bond == NULL) {
                LOGE("%s %d bond is NULL\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            if (control->bond == bond) {
                control->bond = NULL;
            } else {
                LOGW("%s %d bond is not registered\r\n", __func__, __LINE__);
                return AVDK_ERR_INVAL;
            }
            break;
        }
        default:
            LOGW("Unknown ioctl command: %d\r\n", cmd);
            return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

avdk_err_t bk_h264_encode_hw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    if (config->outbuf_malloc == NULL || config->outbuf_complete == NULL) {
        LOGE("Buffer callbacks are required\r\n");
        return AVDK_ERR_INVAL;
    }
    private_h264_encode_hw_flexa_ctlr_t *controller = (private_h264_encode_hw_flexa_ctlr_t *)os_malloc(sizeof(private_h264_encode_hw_flexa_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_h264_encode_hw_flexa_ctlr_t));
    h264_encode_debug_info_reset_qp(&controller->debug_info);
    os_memcpy(&controller->config, config, sizeof(bk_h264_encode_hw_flexa_config_t));
    controller->ops.init = h264_encode_ctlr_init;
    controller->ops.open = h264_encode_ctlr_open;
    controller->ops.encode = h264_encode_ctlr_encode;
    controller->ops.close = h264_encode_ctlr_close;
    controller->ops.ioctl = h264_encode_ctlr_ioctl;
    controller->ops.force_idr = h264_encode_ctlr_force_idr;
    controller->ops.deinit = h264_encode_ctlr_deinit;
    controller->ops.del = h264_encode_ctlr_delete;
    *handle = &(controller->ops);
    LOGI("H.264 encoder controller created\r\n");
    return AVDK_ERR_OK;
}
