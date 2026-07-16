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

/*
 * Fixed decode-target acquire wait budget: when the frame pool is momentarily
 * exhausted (the application still holds display frames) the decoder waits up to
 * this long for a slot to be released before dropping the frame, trading frame
 * rate for a bounded peak memory footprint. Must stay well below the decode-call
 * semaphore wait (2000ms) so an exhausted pool degrades to a soft frame drop
 * rather than a hard decode-call timeout.
 */
#define H264_DECODE_FRAME_ZC_ACQ_TMO_MS     200U

/*
 * Upper bound on how far into an access unit we scan for the first VCL slice
 * NAL. Any SPS/PPS/SEI that may precede an IDR slice is tiny (tens of bytes),
 * so a valid AU always exposes its first slice header well within this window.
 * Capping the scan lets a corrupted/misframed AU fail fast instead of walking
 * the entire (possibly hundreds-of-KB) buffer.
 */
#define H264_ZC_PEEK_MAX_SCAN               2048U

/*
 * Peek the frame type of an Annex-B access unit without fully parsing it: scan
 * (up to H264_ZC_PEEK_MAX_SCAN bytes) for the first VCL slice NAL
 * (nal_unit_type 1..5) and read its 1-byte NAL header. type == 5 marks an IDR;
 * nal_ref_idc (top 2 bits) != 0 marks a reference picture. This is all the
 * controller's frame-drop policy needs, and keeps that policy out of the codec
 * core. Returns true when a VCL slice header is found within the scan window,
 * false otherwise (empty/corrupt/misframed AU).
 */
static bool h264_zc_peek_frame_type(const uint8_t *buf, uint32_t len,
				    uint8_t *is_idr, uint8_t *is_ref)
{
	uint32_t i = 0U;
	uint32_t scan_len;

	*is_idr = 0U;
	*is_ref = 0U;
	if (buf == NULL || len < 4U) {
		return false;
	}
	/* Bound the search: a valid AU exposes its first slice header within the
	 * leading SPS/PPS/SEI, which never approaches this window. */
	scan_len = (len < H264_ZC_PEEK_MAX_SCAN) ? len : H264_ZC_PEEK_MAX_SCAN;
	/* A 3-byte start code (00 00 01) is a suffix of the 4-byte one, so
	 * scanning for 00 00 01 matches both. */
	while (i + 3U < scan_len) {
		if (buf[i] == 0U && buf[i + 1U] == 0U && buf[i + 2U] == 1U) {
			uint8_t nal_hdr = buf[i + 3U];
			uint8_t type = nal_hdr & 0x1FU;

			if (type >= 1U && type <= 5U) {
				*is_idr = (type == 5U) ? 1U : 0U;
				*is_ref = ((nal_hdr >> 5U) & 0x3U) ? 1U : 0U;
				return true;
			}
			i += 3U;
		} else {
			i++;
		}
	}
	return false;
}

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
				h264_decode_mem_malloc_align, h264_decode_mem_free) != AVDK_ERR_OK ||
	    ctrl->pool == NULL) {
		LOGE("%s %d h264d_fbpool_create failed\r\n", __func__, __LINE__);
		ret = AVDK_ERR_GENERIC;
		goto error;
	}
	if (h264d_fbpool_get_if(ctrl->pool, &ctrl->fbif) != AVDK_ERR_OK) {
		LOGE("%s %d h264d_fbpool_get_if failed\r\n", __func__, __LINE__);
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	/* Fixed acquire wait budget: on transient pool exhaustion the decoder waits
	 * up to this long for the application to release a display frame, then drops
	 * the frame (skip-until-IDR for reference/IDR frames) instead of failing. */
	h264d_fbpool_set_acquire_timeout(ctrl->pool, H264_DECODE_FRAME_ZC_ACQ_TMO_MS);

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

	LOGI("%s %d H264 frame-zerocopy decoder registered (disp_depth=%u, acquire_tmo=%ums)\r\n",
	     __func__, __LINE__, (unsigned)disp_depth, (unsigned)H264_DECODE_FRAME_ZC_ACQ_TMO_MS);
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
	uint8_t is_idr = 0U;
	uint8_t is_ref = 0U;

	if (ctrl == NULL || ctrl->vcdec_handle == NULL) {
		return AVDK_ERR_INVAL;
	}

	/*
	 * No VCL slice NAL within the first H264_ZC_PEEK_MAX_SCAN bytes: the
	 * access unit is empty, corrupt, or misframed. Don't hand it to the HW
	 * decoder (it can't be decoded and only wastes a decode cycle). Reclaim
	 * the pool (keeps active SPS/PPS) and resync at the next IDR, then report
	 * the failure for this AU.
	 */
	if (!h264_zc_peek_frame_type(ctrl->decode_config.input_stream,
				     ctrl->decode_config.input_stream_len,
				     &is_idr, &is_ref)) {
		LOGE("peek: no VCL NAL within %uB (au_len=%u), drop AU and resync at next IDR\r\n",
		     (unsigned)H264_ZC_PEEK_MAX_SCAN,
		     (unsigned)ctrl->decode_config.input_stream_len);
		vcdec_h264_recycle(ctrl->vcdec_handle);
		ctrl->skip_until_idr = 1U;
		ctrl->decode_result = AVDK_ERR_GENERIC;
		return AVDK_ERR_GENERIC;
	}

	/*
	 * Resync gate: while recovering from a broken reference chain, drop every
	 * non-IDR access unit (undecodable) and resume only at the next IDR. A
	 * dropped frame is a consumed-but-no-output result, not an error.
	 */
	if (ctrl->skip_until_idr) {
		if (!is_idr) {
			ctrl->decode_result = BK_OK;
			return AVDK_ERR_OK;
		}
		ctrl->skip_until_idr = 0U;
	}

	DECODE_FRAME_START;
	ret = vcdec_h264_decode_frame(ctrl->vcdec_handle, &ctrl->decode_config);
	DECODE_FRAME_END;

	if (ret == VCDEC_FRAME_READY || ret == VCDEC_OK) {
		ctrl->decode_result = BK_OK;
		return AVDK_ERR_OK;
	}

	if (ret == VCDEC_HW_TIMEOUT) {
		/*
		 * No decode target within the acquire wait budget: drop this frame,
		 * trading frame rate for a bounded peak memory footprint. A
		 * non-reference frame can be dropped in isolation (nothing references
		 * it). Dropping a reference/IDR breaks the chain, so reclaim the pool
		 * (vcdec_h264_recycle keeps the active SPS/PPS, so out-of-band
		 * parameter-set streams still recover) and skip until the next IDR.
		 */
		if (is_ref || is_idr) {
			vcdec_h264_recycle(ctrl->vcdec_handle);
			ctrl->skip_until_idr = 1U;
			LOGW("frame drop: reference/IDR lost (acquire timeout), skip until next IDR\r\n");
		} else {
			LOGW("frame drop: non-reference frame dropped (acquire timeout)\r\n");
		}
		ctrl->decode_result = BK_OK;
		return AVDK_ERR_OK;
	}

	LOGE("%s %d vcdec_h264_decode_frame failed: %d\r\n", __func__, __LINE__, (int)ret);
	ctrl->decode_result = AVDK_ERR_GENERIC;
	return AVDK_ERR_GENERIC;
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
	avdk_err_t vret;

	AVDK_RETURN_ON_FALSE(dq, AVDK_ERR_INVAL, TAG, "dequeue arg is NULL");
	AVDK_RETURN_ON_FALSE(ctrl->pool, AVDK_ERR_INVAL, TAG, "pool is NULL");

	vret = h264d_fbpool_dequeue(ctrl->pool, &frm, dq->timeout_ms);
	if (vret != AVDK_ERR_OK) {
		/* Timeout / empty: not a hard error, lets the caller stop draining. */
		return vret;
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

	if (h264d_fbpool_release(ctrl->pool, &frm) != AVDK_ERR_OK) {
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
		ctrl->skip_until_idr = 0U;
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
