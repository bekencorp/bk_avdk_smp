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

/*
 * Zero-copy / B-frame capable whole-frame H.264 decode controller.
 *
 * Parallel to bk_h264_decode_frame_ctlr.c (which keeps the legacy non-pool
 * path), this controller drives the reconstructed vcdec H.264 driver in its
 * zero-copy frame-pool mode: it owns an internal h264d_fbpool, injects it via
 * vcdec_h264_register_fb_if(), and a single physical buffer serves as decode
 * target / DPB reference / display output. The reference-backup and output
 * copies are eliminated and B-frame decoding with POC-based display reordering
 * is supported.
 *
 * The per-frame output buffer is no longer supplied by the caller; decoded
 * pictures are pulled in display order via BK_H264_DECODE_IOCTL_DEQUEUE and
 * returned via BK_H264_DECODE_IOCTL_RELEASE, with BK_H264_DECODE_IOCTL_FLUSH
 * emitting the trailing reordered pictures at end of stream.
 */

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>

#include "components/bk_decode/bk_h264_decode_ctlr.h"
#include "private_h264_decode_ctlr.h"
#include "hw_decoder_ctlr.h"
#include "modules/vcdec/vcdec_h264_api.h"
#include "modules/vcdec/vcdec_common.h"
#include "h264d_fbpool.h"
#include "avdk_monitor.h"

#define TAG "bk_h264_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264_DECODE_FRAME_ZC_ALIGN          64U
#define H264_DECODE_FRAME_ZC_DISP_DEPTH_DEF 1U

static void frame_done_cb(int status, void *args)
{
	DECODE_FRAME_DONE;
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = (private_h264_decode_frame_zerocopy_ctlr_t *)args;
	if (ctrl == NULL) {
		LOGE("control is NULL\r\n");
		return;
	}
	if (ctrl->config.frame_done_cb != NULL) {
		ctrl->config.frame_done_cb(status, ctrl->config.frame_done_args);
	}
}

static avdk_err_t h264_decode_ctlr_init(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);
	uint16_t disp_depth;

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	avdk_err_t ret = hw_decoder_register(HW_DECODER_TYPE_H264, ctrl);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d register to hw decoder controller failed: %d\r\n", __func__, __LINE__, ret);
		goto error;
	}

	ret = rtos_init_semaphore(&ctrl->decode_done_sem, 1);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d init decode_done_sem failed\r\n", __func__, __LINE__);
		goto error;
	}

	disp_depth = (ctrl->config.disp_depth != 0U) ? ctrl->config.disp_depth : H264_DECODE_FRAME_ZC_DISP_DEPTH_DEF;
	if (h264d_fbpool_create(&ctrl->pool, H264_DECODE_FRAME_ZC_ALIGN, disp_depth,
				h264_decode_mem_malloc_align, h264_decode_mem_free) != VCDEC_OK ||
	    ctrl->pool == NULL) {
		LOGE("%s %d h264d_fbpool_create failed\r\n", __func__, __LINE__);
		ret = AVDK_ERR_GENERIC;
		goto error;
	}
	if (h264d_fbpool_get_if(ctrl->pool, &ctrl->fbif) != VCDEC_OK) {
		LOGE("%s %d h264d_fbpool_get_if failed\r\n", __func__, __LINE__);
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	vcdec_config_t cfg = {0};
	cfg.mode = VCDEC_FLEXA_MODE_NONE;
	cfg.timeout_ms = (ctrl->config.timeout_ms != 0U) ? ctrl->config.timeout_ms : 1000U;
	cfg.frame_done_cb = frame_done_cb;
	cfg.args = ctrl;

	if (vcdec_h264_init(&ctrl->vcdec_handle, &cfg) != VCDEC_OK) {
		LOGE("vcdec_h264_init failed\r\n");
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	if (vcdec_register_memalloc(ctrl->vcdec_handle, h264_decode_mem_malloc, h264_decode_mem_free) != VCDEC_OK) {
		LOGE("vcdec_register_memalloc failed\r\n");
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	LOGI("%s %d H264 frame-zerocopy decoder registered (disp_depth=%u)\r\n", __func__, __LINE__, (unsigned)disp_depth);
	return AVDK_ERR_OK;

error:
	if (ctrl->vcdec_handle != NULL) {
		vcdec_h264_deinit(ctrl->vcdec_handle);
		ctrl->vcdec_handle = NULL;
	}
	if (ctrl->pool != NULL) {
		h264d_fbpool_destroy(ctrl->pool);
		ctrl->pool = NULL;
	}
	if (ctrl->decode_done_sem != NULL) {
		rtos_deinit_semaphore(&ctrl->decode_done_sem);
		ctrl->decode_done_sem = NULL;
	}
	hw_decoder_unregister(ctrl);
	return ret;
}

static avdk_err_t h264_decode_ctlr_open(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (vcdec_h264_open(ctrl->vcdec_handle) != VCDEC_OK) {
		LOGE("vcdec_h264_open failed\r\n");
		ctrl->vcdec_handle = NULL;
		return AVDK_ERR_GENERIC;
	}

	/* Zero-copy pool path is selected by registering the fb interface after open. */
	if (vcdec_h264_register_fb_if(ctrl->vcdec_handle, &ctrl->fbif) != VCDEC_OK) {
		LOGE("vcdec_h264_register_fb_if failed\r\n");
		return AVDK_ERR_GENERIC;
	}

	LOGI("H264 frame-zerocopy decoder opened\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_callback(void *param)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = (private_h264_decode_frame_zerocopy_ctlr_t *)param;
	vcdec_ret_e ret;

	if (ctrl == NULL || ctrl->vcdec_handle == NULL) {
		return AVDK_ERR_INVAL;
	}

	DECODE_FRAME_START;
	ret = vcdec_h264_decode_frame(ctrl->vcdec_handle, &ctrl->decode_config);
	if (ret != VCDEC_FRAME_READY && ret != VCDEC_OK) {
		LOGE("%s %d vcdec_h264_decode_frame failed: %d\r\n", __func__, __LINE__, (int)ret);
		ctrl->decode_result = AVDK_ERR_GENERIC;
		DECODE_FRAME_END;
		return AVDK_ERR_GENERIC;
	}
	DECODE_FRAME_END;
	ctrl->decode_result = BK_OK;
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_decode_frame(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_input_t *input)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(input, AVDK_ERR_INVAL, TAG, "input is NULL");
	AVDK_RETURN_ON_FALSE(input->stream && input->stream_len > 0U, AVDK_ERR_INVAL, TAG, "invalid stream");
	AVDK_RETURN_ON_FALSE(ctrl->vcdec_handle, AVDK_ERR_INVAL, TAG, "decoder not open");

	/* Pool mode supplies the decode target itself; the user output buffer is unused. */
	ctrl->decode_config.input_stream = input->stream;
	ctrl->decode_config.input_stream_len = input->stream_len;
	ctrl->decode_config.output_buffer = NULL;
	ctrl->decode_config.output_size = 0U;
	ctrl->decode_config.segment_height = 0U;
	ctrl->decode_config.segment_number = 0U;

	hw_decoder_msg_t msg = {
		.decoder_type = HW_DECODER_TYPE_H264,
		.type = HW_DECODER_MSG_DECODE,
		.callback = h264_decode_callback,
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

	if (ctrl->decode_result != BK_OK) {
		if (ctrl->config.frame_done_cb != NULL) {
			ctrl->config.frame_done_cb(BK_FAIL, ctrl->config.frame_done_args);
		}
		return ctrl->decode_result;
	}

	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_dequeue(private_h264_decode_frame_zerocopy_ctlr_t *ctrl, void *arg)
{
	bk_h264_decode_dequeue_t *dq = (bk_h264_decode_dequeue_t *)arg;
	vcdec_frame_t frm = {0};
	vcdec_ret_e vret;

	AVDK_RETURN_ON_FALSE(dq, AVDK_ERR_INVAL, TAG, "dequeue arg is NULL");
	AVDK_RETURN_ON_FALSE(ctrl->pool, AVDK_ERR_INVAL, TAG, "pool is NULL");

	vret = h264d_fbpool_dequeue(ctrl->pool, &frm, dq->timeout_ms);
	if (vret != VCDEC_OK) {
		/* Timeout / empty: not a hard error, lets the caller stop draining. */
		return AVDK_ERR_TIMEOUT;
	}

	dq->frame.data = frm.data;
	dq->frame.data_len = frm.data_len;
	dq->frame.capacity = frm.capacity;
	dq->frame.width = frm.width;
	dq->frame.height = frm.height;
	dq->frame.format = frm.format;
	dq->frame.frame_type = frm.frame_type;
	dq->frame.poc = frm.poc;
	dq->frame.token = frm.token;
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_release(private_h264_decode_frame_zerocopy_ctlr_t *ctrl, void *arg)
{
	bk_h264_decode_out_frame_t *of = (bk_h264_decode_out_frame_t *)arg;
	vcdec_frame_t frm = {0};

	AVDK_RETURN_ON_FALSE(of, AVDK_ERR_INVAL, TAG, "release arg is NULL");
	AVDK_RETURN_ON_FALSE(of->token, AVDK_ERR_INVAL, TAG, "release token is NULL");
	AVDK_RETURN_ON_FALSE(ctrl->pool, AVDK_ERR_INVAL, TAG, "pool is NULL");

	frm.data = of->data;
	frm.data_len = of->data_len;
	frm.capacity = of->capacity;
	frm.width = of->width;
	frm.height = of->height;
	frm.format = of->format;
	frm.frame_type = of->frame_type;
	frm.poc = of->poc;
	frm.token = of->token;

	if (h264d_fbpool_release(ctrl->pool, &frm) != VCDEC_OK) {
		LOGE("%s %d h264d_fbpool_release failed\r\n", __func__, __LINE__);
		return AVDK_ERR_GENERIC;
	}
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_ioctl(bk_h264_decode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	switch (cmd) {
	case BK_H264_DECODE_IOCTL_GET_INFO:
		AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "arg is NULL");
		if (vcdec_h264_get_info(ctrl->vcdec_handle, (bk_h264_decode_info_t *)arg) != VCDEC_OK) {
			LOGE("%s %d get_info failed\r\n", __func__, __LINE__);
			return AVDK_ERR_GENERIC;
		}
		break;
	case BK_H264_DECODE_IOCTL_ABORT:
		vcdec_h264_abort(ctrl->vcdec_handle);
		break;
	case BK_H264_DECODE_IOCTL_RESET:
		vcdec_h264_reset(ctrl->vcdec_handle);
		break;
	case BK_H264_DECODE_IOCTL_DEQUEUE:
		return h264_decode_ctlr_dequeue(ctrl, arg);
	case BK_H264_DECODE_IOCTL_RELEASE:
		return h264_decode_ctlr_release(ctrl, arg);
	case BK_H264_DECODE_IOCTL_FLUSH:
		if (vcdec_h264_flush(ctrl->vcdec_handle) != VCDEC_OK) {
			LOGE("%s %d vcdec_h264_flush failed\r\n", __func__, __LINE__);
			return AVDK_ERR_GENERIC;
		}
		break;
	case BK_H264_DECODE_IOCTL_PORT_SET_RD_PTR:
	case BK_H264_DECODE_IOCTL_REGISTER_BOND:
	case BK_H264_DECODE_IOCTL_UNREGISTER_BOND:
	case BK_H264_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE:
		LOGI("%s %d cmd %u is unsupported in frame-zerocopy mode\r\n", __func__, __LINE__, cmd);
		break;
	default:
		LOGE("Unknown ioctl: %u\r\n", (unsigned)cmd);
		return AVDK_ERR_INVAL;
	}

	return AVDK_ERR_OK;
}

static void h264_decode_resources_deinit(private_h264_decode_frame_zerocopy_ctlr_t *ctrl)
{
	if (ctrl->decode_done_sem != NULL) {
		rtos_deinit_semaphore(&ctrl->decode_done_sem);
		ctrl->decode_done_sem = NULL;
	}
}

static avdk_err_t h264_decode_ctlr_close(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (ctrl->vcdec_handle != NULL) {
		vcdec_h264_close(ctrl->vcdec_handle);
	}
	LOGI("H264 frame-zerocopy decoder closed\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_deinit(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	avdk_err_t ret = hw_decoder_unregister(ctrl);
	if (ret != AVDK_ERR_OK) {
		LOGE("Unregister from hw decoder controller failed: %d\r\n", ret);
		return ret;
	}
	if (ctrl->vcdec_handle != NULL) {
		vcdec_h264_deinit(ctrl->vcdec_handle);
		ctrl->vcdec_handle = NULL;
	}
	if (ctrl->pool != NULL) {
		h264d_fbpool_destroy(ctrl->pool);
		ctrl->pool = NULL;
	}
	h264_decode_resources_deinit(ctrl);
	LOGI("H264 frame-zerocopy decoder unregistered from hw controller\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_delete(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl = __containerof(handle, private_h264_decode_frame_zerocopy_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	os_free(ctrl);
	LOGI("H264 frame-zerocopy decoder deleted\r\n");
	return AVDK_ERR_OK;
}

avdk_err_t bk_h264_decode_frame_zerocopy_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_frame_zerocopy_config_t *config)
{
	private_h264_decode_frame_zerocopy_ctlr_t *ctrl;

	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
	AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

	ctrl = (private_h264_decode_frame_zerocopy_ctlr_t *)os_malloc(sizeof(private_h264_decode_frame_zerocopy_ctlr_t));
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

	os_memset(ctrl, 0, sizeof(private_h264_decode_frame_zerocopy_ctlr_t));
	os_memcpy(&ctrl->config, config, sizeof(bk_h264_decode_frame_zerocopy_config_t));

	ctrl->mode = VCDEC_FLEXA_MODE_NONE;
	ctrl->ops.init = h264_decode_ctlr_init;
	ctrl->ops.open = h264_decode_ctlr_open;
	ctrl->ops.decode_frame = h264_decode_ctlr_decode_frame;
	ctrl->ops.close = h264_decode_ctlr_close;
	ctrl->ops.deinit = h264_decode_ctlr_deinit;
	ctrl->ops.ioctl = h264_decode_ctlr_ioctl;
	ctrl->ops.del = h264_decode_ctlr_delete;

	*handle = &ctrl->ops;
	LOGI("H264 frame-zerocopy decoder controller created\r\n");
	return AVDK_ERR_OK;
}
