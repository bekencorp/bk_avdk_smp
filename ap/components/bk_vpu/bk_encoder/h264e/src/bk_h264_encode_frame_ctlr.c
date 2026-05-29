#include <os/os.h>
#include <os/mem.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "components/bk_encode/bk_h264_encode_types.h"
#include "h264e_driver.h"
#include "private_h264_encode_ctlr.h"
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
static void handle_encode_error(private_h264_encode_frame_ctlr_t *ctrl, void *buffer, uint32_t size)
{
    if (buffer != NULL && ctrl->config.outbuf_complete && ctrl->h264_encoder_param) {
        bk_h264_encode_outbuf_info_t info = {
            .outbuf = buffer,
            .length = size,
            .type = 0,
            .status = BK_FAIL,
            .sequence = 0,
            .args = ctrl->config.outbuf_complete_args,
        };
        ctrl->config.outbuf_complete(&info);
        ctrl->h264_encoder_param->out_buf = 0;
    }
    h264e_set_force_idr(&ctrl->h264e_handler);
}

// Handle video frames (I-frame or P-frame)
static void handle_video_frame(private_h264_encode_frame_ctlr_t *ctrl, void *buffer, uint32_t size, uint32_t type)
{
    // Pre-allocate buffer for next frame
    void *next_buffer = NULL;
    if (ctrl->config.outbuf_malloc) {
        next_buffer = ctrl->config.outbuf_malloc(CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER,
                                                 ctrl->config.outbuf_malloc_args);
        if (!next_buffer) {
            LOGD("Failed to get next buffer, force IDR\r\n");
            handle_encode_error(ctrl, buffer, size);
            return;
        }
    }

    // Notify upper layer and save buffer for next frame
    if (ctrl->config.outbuf_complete && ctrl->h264_encoder_param) {
        enc_h264_debug_t *debug_info = NULL;
        h264e_get_debug_info(&ctrl->h264e_handler, &debug_info);
        bk_h264_encode_outbuf_info_t info = {
            .outbuf = buffer,
            .length = size,
            .type = type,
            .status = BK_OK,
            .sequence = debug_info ? debug_info->all_frame_count : 0,
            .args = ctrl->config.outbuf_complete_args,
        };
        ctrl->config.outbuf_complete(&info);
        ctrl->h264_encoder_param->out_buf = (uint32_t)next_buffer;
        ctrl->h264_encoder_param->out_size = CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER;
    }
}

// Signal encoding thread that one frame encode has completed (success or failure)
static void signal_encode_done(private_h264_encode_frame_ctlr_t *ctrl)
{
    if (ctrl != NULL) {
        rtos_set_semaphore(&ctrl->enc_done_sem);
    }
}

// H.264 encoding completion callback
static void h264e_end_cb(void *buffer, uint32_t size, uint32_t type, uint32_t result, uint32_t param)
{
    ENCODE_FRAME_DONE;
    // Validate parameters
    if (!param || !buffer) {
        LOGE("Invalid parameters in h264e_end_cb\r\n");
        return;
    }

    if (type != VCENC_OUT_IFRAME && type != VCENC_OUT_PFRAME) {
        LOGE("Invalid frame type in h264e_end_cb\r\n");
        return;
    }

    private_h264_encode_frame_ctlr_t *ctrl = (private_h264_encode_frame_ctlr_t *)param;

    // Handle encoding failure
    if (result != BK_OK) {
        handle_encode_error(ctrl, buffer, size);
        signal_encode_done(ctrl);
        return;
    }

    handle_video_frame(ctrl, buffer, size, type);
    signal_encode_done(ctrl);
}

// Callback run in hw_encoder task: start one frame encode
static avdk_err_t h264_encode_msg_callback(void *param)
{
    if (param == NULL) {
        LOGE("Encode callback param is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    private_h264_encode_frame_ctlr_t *ctrl = (private_h264_encode_frame_ctlr_t *)param;
    if (ctrl->h264_encoder_param == NULL) {
        LOGW("No encoder parameters available\r\n");
        signal_encode_done(ctrl);
        return AVDK_ERR_INVAL;
    }
    ENCODE_FRAME_START;
    bk_err_t ret = h264e_start_encode(&ctrl->h264e_handler, ctrl->h264_encoder_param);
    ENCODE_FRAME_END;
    if (ret != BK_OK) {
        LOGE("h264e_start_encode failed: %d\r\n", ret);
        signal_encode_done(ctrl);
        return AVDK_ERR_GENERIC;
    }
    return AVDK_ERR_OK;
}

// Encoding thread entry
static void h264_encoder_entry(void *arg)
{
    private_h264_encode_frame_ctlr_t *ctrl = (private_h264_encode_frame_ctlr_t *)arg;
    if (ctrl == NULL) {
        LOGE("Encoder thread started with NULL context\r\n");
        return;
    }

    rtos_set_semaphore(&ctrl->sem);

    h264_encoder_parameters_t param = {0};
    ctrl->h264_encoder_param = &param;
    while (ctrl->enc_status) {
        rtos_get_semaphore(&ctrl->enc_start_sem, BEKEN_WAIT_FOREVER);
        if (!ctrl->enc_status) {
            break;
        }
        if (param.out_buf == 0 && ctrl->config.outbuf_malloc != NULL) {
            void *temp_buffer = ctrl->config.outbuf_malloc(CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER,
                                                           ctrl->config.outbuf_malloc_args);
            if (temp_buffer != NULL) {
                param.out_buf = (uint32_t)temp_buffer;
                param.out_size = CONFIG_BK_ENCODER_H264_MAX_OUTPUT_BUFFER;
            } else {
                param.out_buf = 0;
                param.out_size = 0;
            }
        }
        if (param.out_buf == 0) {
            LOGW("Failed to get output buffer, skip this frame\r\n");
            handle_encode_error(ctrl, (void *)param.out_buf, 0);
            signal_encode_done(ctrl);
            continue;
        }
        param.pic_buf = ctrl->config.input_buf;
        param.pic_lines = ctrl->config.input_size;

        hw_encoder_msg_t msg = {
            .type = HW_ENCODER_MSG_ENCODE,
            .callback = h264_encode_msg_callback,
            .param = ctrl,
            .sem = NULL
        };
        avdk_err_t ret = hw_encoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
        if (ret != AVDK_ERR_OK) {
            LOGE("hw_encoder_send_msg failed: %d\r\n", ret);
            handle_encode_error(ctrl, (void *)param.out_buf, 0);
            continue;
        }
    }
    // Clean up resources
    if (param.out_buf != 0) {
        handle_encode_error(ctrl, (void *)param.out_buf, 0);
    }
    ctrl->h264_encoder_param = NULL;
    rtos_set_semaphore(&ctrl->sem);
    rtos_delete_thread(NULL);
}

static avdk_err_t h264_encode_ctlr_init(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    // Register to hardware controller
    avdk_err_t ret = hw_encoder_register(HW_ENCODER_TYPE_H264, control);
    if (ret != AVDK_ERR_OK) {
        LOGE("Register to hw controller failed: %d\r\n", ret);
        return ret;
    }
    // Initialize semaphores
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
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    // Configure H.264 encoder
    h264_encoder_config_t config = {0};
    config.width = control->config.width;
    config.height = control->config.height;
    config.flexa_mode = BK_H264_ENCODE_FLEXA_MODE_NONE;
    config.input_type = control->config.input_format;
    config.buf_cnt = control->config.input_flexa_cnt;
    config.idr_interval = control->config.gop_frame_count;
    LOGI("Start H.264 encoder %dx%d, mode=%d\r\n", config.width, config.height, config.flexa_mode);
    // Initialize H.264 encoder
    bk_err_t ret = h264e_init(&control->h264e_handler, &config);
    if (ret != BK_OK) {
        LOGE("h264e_init failed: %d\r\n", ret);
        return AVDK_ERR_GENERIC;
    }
    // Register encoding completion callback
    h264_encoder_callback_t callback = {0};
    callback.fcb = NULL;
    callback.ocb = h264e_end_cb;
    callback.param = (uint32_t)control;
    ret = h264e_register_callback(&control->h264e_handler, &callback);
    if (ret != BK_OK) {
        LOGE("h264e_register_callback failed: %d\r\n", ret);
        h264e_deinit(&control->h264e_handler);
        return AVDK_ERR_GENERIC;
    }
    // Open encoder
    ret = h264e_open(&control->h264e_handler);
    if (ret != BK_OK) {
        LOGE("h264e_open failed: %d\r\n", ret);
        h264e_deregister_callback(&control->h264e_handler);
        h264e_deinit(&control->h264e_handler);
        return AVDK_ERR_GENERIC;
    }
    // Set encoding status
    control->enc_line_cnt = 0;
    control->enc_status = 1;
    control->enc_start_first = 1;
    // Create encoding thread
    ret = rtos_create_hsram_thread(&control->thread,
                           BEKEN_DEFAULT_WORKER_PRIORITY,
                           "h264e_encoder",
                           (beken_thread_function_t)h264_encoder_entry,
                           CONFIG_BK_ENCODER_H264_TASK_SIZE,
                           control);
    if (ret != BK_OK) {
        LOGE("Create thread failed: %d\r\n", ret);
        control->enc_status = 0;
        h264e_close(&control->h264e_handler);
        h264e_deregister_callback(&control->h264e_handler);
        h264e_deinit(&control->h264e_handler);
        return AVDK_ERR_GENERIC;
    }
    rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);

    LOGI("H.264 encoder opened successfully\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_encode(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    rtos_set_semaphore(&control->enc_start_sem);
    rtos_get_semaphore(&control->enc_done_sem, BEKEN_WAIT_FOREVER);
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_close(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    // Stop encoding
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

    // Deregister encoder callbacks
    h264e_deregister_callback(&control->h264e_handler);
    // Close and deinitialize encoder
    h264e_close(&control->h264e_handler);
    h264e_deinit(&control->h264e_handler);
    LOGI("H.264 encoder closed\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_deinit(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    // Clean up semaphores
    rtos_deinit_semaphore(&control->sem);
    rtos_deinit_semaphore(&control->enc_start_sem);
    rtos_deinit_semaphore(&control->enc_done_sem);
    // Unregister from hardware controller
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
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    // Force next frame to be IDR
    bk_err_t ret = h264e_set_force_idr(&control->h264e_handler);
    if (ret != BK_OK) {
        LOGE("Force IDR failed: %d\r\n", ret);
        return AVDK_ERR_GENERIC;
    }
    // LOGI("Force IDR frame requested\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_delete(bk_h264_encode_ctlr_handle_t handle)
{
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    // Free controller memory
    os_free(control);
    LOGI("H.264 encoder deleted\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_set_gop_frame_count(private_h264_encode_frame_ctlr_t *control,
                                                       uint32_t gop_frame_count)
{
    if (gop_frame_count == 0) {
        LOGE("invalid GOP frame count: %u\r\n", gop_frame_count);
        return AVDK_ERR_INVAL;
    }

    control->config.gop_frame_count = gop_frame_count;
    if (control->h264e_handler != NULL) {
        bk_err_t ret = h264e_set_gop_frame_count(&control->h264e_handler, gop_frame_count);
        if (ret != BK_OK) {
            LOGE("set gop config failed: %d\r\n", ret);
            return AVDK_ERR_GENERIC;
        }
    }

    LOGI("H.264 GOP frame count set, count=%u\r\n", gop_frame_count);
    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_get_gop_frame_count(private_h264_encode_frame_ctlr_t *control,
                                                       uint32_t *gop_frame_count)
{
    if (gop_frame_count == NULL) {
        LOGE("GOP frame count arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    if (control->h264e_handler != NULL) {
        bk_err_t ret = h264e_get_gop_frame_count(&control->h264e_handler, gop_frame_count);
        if (ret != BK_OK) {
            LOGE("get gop frame count failed: %d\r\n", ret);
            return AVDK_ERR_GENERIC;
        }
    } else {
        *gop_frame_count = control->config.gop_frame_count;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_set_rate_ctrl(private_h264_encode_frame_ctlr_t *control,
                                                 bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    if (rate_ctrl == NULL) {
        LOGE("rate_ctrl arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (control->h264e_handler == NULL) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }

    h264_encoder_rate_ctrl_t h264e_rc = {
        .bitrate = rate_ctrl->bitrate,
        .qp_min_i = rate_ctrl->qp_min_i,
        .qp_max_i = rate_ctrl->qp_max_i,
        .qp_min_p = rate_ctrl->qp_min_p,
        .qp_max_p = rate_ctrl->qp_max_p,
    };
    bk_err_t ret = h264e_set_rate_ctrl(&control->h264e_handler, &h264e_rc);
    if (ret != BK_OK) {
        LOGE("set rate ctrl failed: %d\r\n", ret);
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t h264_encode_ctlr_get_rate_ctrl(private_h264_encode_frame_ctlr_t *control,
                                                 bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    if (rate_ctrl == NULL) {
        LOGE("rate_ctrl arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (control->h264e_handler == NULL) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }

    h264_encoder_rate_ctrl_t h264e_rc = {0};
    bk_err_t ret = h264e_get_rate_ctrl(&control->h264e_handler, &h264e_rc);
    if (ret != BK_OK) {
        LOGE("get rate ctrl failed: %d\r\n", ret);
        return AVDK_ERR_GENERIC;
    }

    rate_ctrl->qp_min_i = h264e_rc.qp_min_i;
    rate_ctrl->qp_max_i = h264e_rc.qp_max_i;
    rate_ctrl->qp_min_p = h264e_rc.qp_min_p;
    rate_ctrl->qp_max_p = h264e_rc.qp_max_p;
    rate_ctrl->bitrate = h264e_rc.bitrate;
    return AVDK_ERR_OK;
}

// Debug timer callback
static void h264e_debug_callback(void *arg)
{
    private_h264_encode_frame_ctlr_t *ctrl = (private_h264_encode_frame_ctlr_t *)arg;
    if (ctrl == NULL) {
        return;
    }
    enc_h264_debug_t *debug = NULL;
    bk_err_t ret = h264e_get_debug_info(&ctrl->h264e_handler, &debug);
    if (ret != BK_OK || debug == NULL) {
        LOGE("Failed to get debug info\r\n");
        return;
    }
    if (ctrl->debug_time_ms == 0) {
        LOGE("Invalid debug_time_ms\r\n");
        return;
    }
    uint32_t frame_count = (debug->all_frame_count - ctrl->last_debug_info.all_frame_count) * 1000 / ctrl->debug_time_ms;
    uint32_t bytes_per_second = (debug->all_frame_size - ctrl->last_debug_info.all_frame_size) * 1000 / ctrl->debug_time_ms;
    uint32_t bit_rate_kbps = bytes_per_second * 8 / 1024;
    LOGI("%s %d(fps:%d\t%dBytes/s\tbit_rate:%dkbps\tmax_i:%d\tmax_p:%d)\n", __func__, __LINE__, frame_count, bytes_per_second, bit_rate_kbps, debug->max_i_frame_size, debug->max_p_frame_size);
    os_memcpy(&ctrl->last_debug_info, debug, sizeof(enc_h264_debug_t));
}

static avdk_err_t h264_encode_ctlr_ioctl(bk_h264_encode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
    private_h264_encode_frame_ctlr_t *control =  __containerof(handle, private_h264_encode_frame_ctlr_t, ops);
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
            // Initialize debug info
            os_memset(&control->last_debug_info, 0, sizeof(enc_h264_debug_t));
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
        case BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY: {
            h264e_flexa_input_linebuf_wrcnt_set(&control->h264e_handler, (uint32_t)arg);
            break;
        }
        case BK_H264_ENCODE_IOCTL_SET_FRAME_READY: {
            rtos_set_semaphore(&control->enc_start_sem);
            break;
        }
        default:
            LOGE("Unknown ioctl command: %d\r\n", cmd);
            return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

avdk_err_t bk_h264_encode_frame_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    if (config->outbuf_malloc == NULL || config->outbuf_complete == NULL) {
        LOGE("Buffer callbacks are required\r\n");
        return AVDK_ERR_INVAL;
    }
    private_h264_encode_frame_ctlr_t *controller = (private_h264_encode_frame_ctlr_t *)os_malloc(sizeof(private_h264_encode_frame_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_h264_encode_frame_ctlr_t));
    os_memcpy(&controller->config, config, sizeof(bk_h264_encode_frame_config_t));
    // Initialize operation function pointers
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