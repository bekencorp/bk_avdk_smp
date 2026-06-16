// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>

#include "bk_flexa_bond_types.h"
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "private_jpeg_decode_ctlr.h"
#include "hw_decoder_ctlr.h"
#include "modules/vcdec/vcdec_jpeg_api.h"
#include "avdk_monitor.h"

#define TAG "bk_jpeg_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static void frame_done_cb(int status, void *args)
{
    DECODE_FRAME_DONE;
    private_jpeg_decode_frame_ctlr_t *ctrl = (private_jpeg_decode_frame_ctlr_t *)args;
    if(ctrl == NULL) {
        LOGE("control is NULL\r\n");
        return;
    }
    if (ctrl->config.frame_done_cb != NULL)
    {
        ctrl->config.frame_done_cb(status, ctrl->config.frame_done_args);
    }
}

static avdk_err_t jpeg_decode_ctlr_init(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    avdk_err_t ret = hw_decoder_register(HW_DECODER_TYPE_JPEG, ctrl);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d register to hw decoder controller failed: %d\r\n", __func__, __LINE__, ret);
        goto error;
    }

    ret = rtos_init_semaphore(&ctrl->decode_done_sem, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d init decode_done_sem failed\r\n", __func__, __LINE__);
        goto error;
    }

    vcdec_config_t cfg = {0};
    cfg.timeout_ms = (ctrl->config.timeout_ms != 0U) ? ctrl->config.timeout_ms : 1000U;
    cfg.frame_done_cb = frame_done_cb;
    cfg.args = ctrl;

    ret = vcdec_jpeg_init(&ctrl->vcdec_handle, &cfg);
    if (ret != VCDEC_OK) {
        LOGE("vcdec_jpeg_init failed: %d\r\n", (int)ret);
        goto error;
    }

    LOGI("%s %d JPEG decoder registered to hw controller\r\n", __func__, __LINE__);
    return AVDK_ERR_OK;

error:
    if (ctrl->decode_done_sem != NULL) {
        rtos_deinit_semaphore(&ctrl->decode_done_sem);
        ctrl->decode_done_sem = NULL;
    }
    hw_decoder_unregister(ctrl);
    return ret;
}

static avdk_err_t jpeg_decode_ctlr_open(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    vcdec_ret_e ret = vcdec_jpeg_open(ctrl->vcdec_handle);
    if (ret != VCDEC_OK) {
        LOGE("vcdec_jpeg_open failed: %d\r\n", (int)ret);
        ctrl->vcdec_handle = NULL;
        return AVDK_ERR_GENERIC;
    }

    LOGI("JPEG decoder opened\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_callback(void *param)
{
    if (!param) {
        return AVDK_ERR_INVAL;
    }

    private_jpeg_decode_frame_ctlr_t *ctrl = (private_jpeg_decode_frame_ctlr_t *)param;
    if (!ctrl->vcdec_handle) {
        return AVDK_ERR_INVAL;
    }

    DECODE_FRAME_START;
    vcdec_ret_e ret = vcdec_jpeg_decode_frame(ctrl->vcdec_handle, &ctrl->decode_config);
    if (ret != VCDEC_FRAME_READY && ret != VCDEC_OK) {
        LOGE("%s %d vcdec_jpeg_decode_frame failed: %d\r\n", __func__, __LINE__, ret);
        ctrl->decode_result = AVDK_ERR_GENERIC;
        DECODE_FRAME_END;
        return AVDK_ERR_GENERIC;
    }
    DECODE_FRAME_END;
    ctrl->decode_result = BK_OK;

    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_decode_frame(bk_jpeg_decode_ctlr_handle_t handle, bk_jpeg_decode_input_t *input)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(input, AVDK_ERR_INVAL, TAG, "input is NULL");
    AVDK_RETURN_ON_FALSE(input->stream && input->stream_len > 0, AVDK_ERR_INVAL, TAG, "invalid stream");
    AVDK_RETURN_ON_FALSE(ctrl->vcdec_handle, AVDK_ERR_INVAL, TAG, "decoder not open");

    uint8_t *out_buffer = input->out_buffer;
    uint32_t out_buffer_size = input->out_buffer_size;

    const uint32_t rb_h = (uint32_t)ctrl->config.out_height;
    const uint32_t out_w = (uint32_t)ctrl->config.out_width;

    if (!out_buffer || out_buffer_size == 0U) {
        LOGE("No output buffer or size\r\n");
        return AVDK_ERR_INVAL;
    }

    if (out_buffer_size < out_w * rb_h * 3 / 2) {
        LOGE("output buffer too small: have=%u need=%u\r\n", out_buffer_size, out_w * rb_h * 3 / 2);
        return AVDK_ERR_NOMEM;
    }

    ctrl->decode_config.input_stream = input->stream;
    ctrl->decode_config.input_stream_len = input->stream_len;
    ctrl->decode_config.output_buffer = out_buffer;
    ctrl->decode_config.output_size = out_buffer_size;
    ctrl->decode_config.width = ctrl->config.out_width;
    ctrl->decode_config.height = ctrl->config.out_height;
    ctrl->decode_config.segment_height = 1U;
    ctrl->decode_config.segment_number = 1U;

    hw_decoder_msg_t msg = {
        .decoder_type = HW_DECODER_TYPE_JPEG,
        .type = HW_DECODER_MSG_DECODE,
        .callback = jpeg_decode_callback,
        .param = ctrl,
        .sem = &ctrl->decode_done_sem,
    };

    avdk_err_t ret = hw_decoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    ret = rtos_get_semaphore(&ctrl->decode_done_sem, 2000);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d rtos_get_semaphore failed: %d\r\n", __func__, __LINE__, ret);
        if (ctrl->config.frame_done_cb != NULL) {
            ctrl->config.frame_done_cb(BK_FAIL, ctrl->config.frame_done_args);
        }
        return AVDK_ERR_GENERIC;
    }

    if(ctrl->decode_result != BK_OK) {
        if (ctrl->config.frame_done_cb != NULL) {
            ctrl->config.frame_done_cb(BK_FAIL, ctrl->config.frame_done_args);
        }
        return ctrl->decode_result;
    }
    return AVDK_ERR_OK;
}

static void jpeg_decode_resources_deinit(private_jpeg_decode_frame_ctlr_t *ctrl)
{
    if (ctrl->decode_done_sem != NULL) {
        rtos_deinit_semaphore(&ctrl->decode_done_sem);
        ctrl->decode_done_sem = NULL;
    }
}

static avdk_err_t jpeg_decode_ctlr_close(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (ctrl->vcdec_handle) {
        vcdec_jpeg_close(ctrl->vcdec_handle);
    }
    LOGI("JPEG decoder closed\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_deinit(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    avdk_err_t ret = hw_decoder_unregister(ctrl);
    if (ret != AVDK_ERR_OK) {
        LOGE("Unregister from hw decoder controller failed: %d\r\n", ret);
        return ret;
    }
    if (ctrl->vcdec_handle) {
        vcdec_jpeg_deinit(ctrl->vcdec_handle);
        ctrl->vcdec_handle = NULL;
    }
    jpeg_decode_resources_deinit(ctrl);
    LOGI("JPEG decoder unregistered from hw controller\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_ioctl(bk_jpeg_decode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (cmd) {
    case BK_JPEG_DECODE_IOCTL_GET_INFO: {
        vcdec_ret_e ret = bk_jpeg_decode_get_img_info((bk_jpeg_decode_img_info_t *)arg);
        if (ret != VCDEC_OK) {
            LOGE("%s %d get_img_info failed: %d\r\n", __func__, __LINE__, (int)ret);
            return AVDK_ERR_GENERIC;
        }
        break;
    }
    case BK_JPEG_DECODE_IOCTL_ABORT:
        vcdec_jpeg_abort(ctrl->vcdec_handle);
        break;

    case BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR:
    case BK_JPEG_DECODE_IOCTL_REGISTER_BOND:
    case BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND:
    case BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE:
    {
        LOGI("%s %d cmd %u is unsupported in frame mode\r\n", __func__, __LINE__, cmd);
        break;
    }

    default:
        LOGE("Unknown ioctl: %u\r\n", (unsigned)cmd);
        return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_delete(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_frame_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_frame_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
    os_free(ctrl);
    LOGI("JPEG decoder deleted\r\n");
    return AVDK_ERR_OK;
}

avdk_err_t bk_jpeg_decode_frame_ctlr_new(bk_jpeg_decode_ctlr_handle_t *handle, bk_jpeg_decode_frame_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    private_jpeg_decode_frame_ctlr_t *ctrl = (private_jpeg_decode_frame_ctlr_t *)os_malloc(sizeof(private_jpeg_decode_frame_ctlr_t));
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(ctrl, 0, sizeof(private_jpeg_decode_frame_ctlr_t));
    os_memcpy(&ctrl->config, config, sizeof(bk_jpeg_decode_frame_config_t));

    ctrl->mode = BK_JPEG_DECODE_FLEXA_MODE_NONE;

    ctrl->ops.init = jpeg_decode_ctlr_init;
    ctrl->ops.open = jpeg_decode_ctlr_open;
    ctrl->ops.decode_frame = jpeg_decode_ctlr_decode_frame;
    ctrl->ops.close = jpeg_decode_ctlr_close;
    ctrl->ops.deinit = jpeg_decode_ctlr_deinit;
    ctrl->ops.ioctl = jpeg_decode_ctlr_ioctl;
    ctrl->ops.del = jpeg_decode_ctlr_delete;

    *handle = &ctrl->ops;
    LOGI("JPEG decoder controller created\r\n");
    return AVDK_ERR_OK;
}
