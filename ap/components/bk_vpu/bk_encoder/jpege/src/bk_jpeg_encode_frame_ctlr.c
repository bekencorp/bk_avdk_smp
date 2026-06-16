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
#include <components/avdk_utils/avdk_check.h>
#include <components/avdk_utils/avdk_types.h>
#include <common/avdk_pixel_types.h>
#include <common/bk_err.h>

#include "components/bk_encode/bk_jpeg_encode_ctlr.h"
#include "hw_encoder_ctlr.h"
#include "private_jpeg_encode_ctlr.h"

#define TAG "bk_jpeg_enc_frm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static vcenc_input_e jpeg_map_input_format(uint32_t pixel_fmt)
{
	if (pixel_fmt == (uint32_t)BK_PIXEL_FORMAT_NV12)
		return VCENC_INPUT_NV12;
	return VCENC_INPUT_NV12;
}

static avdk_err_t jpeg_vcenc_ret_to_avdk(vcenc_ret_e r)
{
	if (r == VCENC_FRAME_READY || r == VCENC_OK)
		return AVDK_ERR_OK;
	if (r == VCENC_HW_TIMEOUT)
		return AVDK_ERR_TIMEOUT;
	if (r == VCENC_NULL_ARGUMENT || r == VCENC_INVALID_ARGUMENT)
		return AVDK_ERR_INVAL;
	if (r == VCENC_MEMORY_ERROR || r == VCENC_EWL_MEMORY_ERROR)
		return AVDK_ERR_NOMEM;
	return AVDK_ERR_HWERROR;
}

/* Forward: completion is signalled from hw_encoder task after vcenc_jpeg_encode_frame returns. */
static void signal_jpeg_encode_done(private_jpeg_encode_frame_ctlr_t *ctrl)
{
	if (ctrl != NULL)
		rtos_set_semaphore(&ctrl->enc_done_sem);
}

static void jpeg_encode_done_cb(void *buffer, uint32_t length, uint32_t type, uint32_t status, uint32_t args)
{
	private_jpeg_encode_frame_ctlr_t *ctrl = (private_jpeg_encode_frame_ctlr_t *)(uintptr_t)args;

	if (ctrl == NULL || ctrl->config.outbuf_complete == NULL)
		return;

	bk_jpeg_encode_outbuf_info_t info = {
		.outbuf = buffer,
		.length = length,
		.type = type,
		.status = status,
		.sequence = 0,
		.args = ctrl->config.outbuf_complete_args,
	};
	ctrl->config.outbuf_complete(&info);
}

/** Runs on hw_encoder task: execute one JPEG encode (same boundary as H264 h264_encode_msg_callback). */
static avdk_err_t jpeg_encode_msg_callback(void *param)
{
	private_jpeg_encode_frame_ctlr_t *ctrl = (private_jpeg_encode_frame_ctlr_t *)param;

	if (ctrl == NULL)
		return AVDK_ERR_INVAL;

	ctrl->last_ret = vcenc_jpeg_encode_frame(&ctrl->jpeg_param);
	signal_jpeg_encode_done(ctrl);
	return jpeg_vcenc_ret_to_avdk(ctrl->last_ret);
}

/**
 * Dedicated encoder thread (same role as h264_encoder_entry): wait for enc_start_sem,
 * push work to hw_encoder queue; caller waits on enc_done_sem after vcenc_jpeg_encode_frame finishes.
 */
static void jpeg_encoder_entry(void *arg)
{
	private_jpeg_encode_frame_ctlr_t *ctrl = (private_jpeg_encode_frame_ctlr_t *)arg;

	if (ctrl == NULL) {
		LOGE("JPEG encoder thread started with NULL context\r\n");
		return;
	}

	rtos_set_semaphore(&ctrl->sem);

	while (ctrl->enc_status) {
		rtos_get_semaphore(&ctrl->enc_start_sem, BEKEN_WAIT_FOREVER);
		if (!ctrl->enc_status)
			break;

		uint32_t out_cap = CONFIG_BK_ENCODER_MJPEG_MAX_OUTPUT_BUFFER;
		void *out_ptr = NULL;
		uint32_t in_base = ctrl->pending_input.pic_buf ?
					   ctrl->pending_input.pic_buf :
					   ctrl->config.input_buf;

		if (in_base == 0) {
			LOGE("invalid input buffer\r\n");
			ctrl->last_ret = VCENC_INVALID_ARGUMENT;
			signal_jpeg_encode_done(ctrl);
			continue;
		}

		if (ctrl->pending_input.out_buf != 0) {
			ctrl->jpeg_param.out_buffer = ctrl->pending_input.out_buf;
			ctrl->jpeg_param.out_len = ctrl->pending_input.out_size ?
							   ctrl->pending_input.out_size :
							   out_cap;
		} else if (ctrl->config.outbuf_malloc) {
			out_ptr = ctrl->config.outbuf_malloc(out_cap, ctrl->config.outbuf_malloc_args);
			if (out_ptr == NULL) {
				LOGE("outbuf_malloc failed\r\n");
				ctrl->last_ret = VCENC_MEMORY_ERROR;
				signal_jpeg_encode_done(ctrl);
				continue;
			}
			ctrl->jpeg_param.out_buffer = (uint32_t)(uintptr_t)out_ptr;
			ctrl->jpeg_param.out_len = out_cap;
		} else {
			LOGE("no output buffer\r\n");
			ctrl->last_ret = VCENC_INVALID_ARGUMENT;
			signal_jpeg_encode_done(ctrl);
			continue;
		}

		ctrl->jpeg_param.in_buffer = in_base;

		hw_encoder_msg_t msg = {
			.type = HW_ENCODER_MSG_ENCODE,
			.encoder_type = HW_ENCODER_TYPE_JPEG,
			.callback = jpeg_encode_msg_callback,
			.param = ctrl,
			.sem = NULL,
		};

		avdk_err_t ret = hw_encoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
		if (ret != AVDK_ERR_OK) {
			LOGE("hw_encoder_send_msg failed: %d\r\n", ret);
			ctrl->last_ret = VCENC_ERROR;
			if (out_ptr != NULL && ctrl->config.outbuf_complete) {
				bk_jpeg_encode_outbuf_info_t info = {
					.outbuf = out_ptr,
					.length = 0,
					.type = 0,
					.status = BK_FAIL,
					.sequence = 0,
					.args = ctrl->config.outbuf_complete_args,
				};
				ctrl->config.outbuf_complete(&info);
			}
			signal_jpeg_encode_done(ctrl);
			continue;
		}

		(void)ret;
	}

	rtos_set_semaphore(&ctrl->sem);
	rtos_delete_thread(NULL);
}

static avdk_err_t jpeg_encode_ctlr_init(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	avdk_err_t ret = hw_encoder_register(HW_ENCODER_TYPE_JPEG, control);
	if (ret != AVDK_ERR_OK) {
		LOGE("Register to hw encoder controller failed: %d\r\n", ret);
		return ret;
	}

	bk_err_t sret = rtos_init_semaphore(&control->sem, 1);
	if (sret != BK_OK) {
		hw_encoder_unregister(control);
		return AVDK_ERR_GENERIC;
	}

	sret = rtos_init_semaphore(&control->enc_start_sem, 1);
	if (sret != BK_OK) {
		rtos_deinit_semaphore(&control->sem);
		hw_encoder_unregister(control);
		return AVDK_ERR_GENERIC;
	}

	sret = rtos_init_semaphore(&control->enc_done_sem, 1);
	if (sret != BK_OK) {
		rtos_deinit_semaphore(&control->enc_start_sem);
		rtos_deinit_semaphore(&control->sem);
		hw_encoder_unregister(control);
		return AVDK_ERR_GENERIC;
	}

	LOGI("JPEG encoder registered to hw controller\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_encode_ctlr_open(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	os_memset(&control->jpeg_param, 0, sizeof(control->jpeg_param));
	control->jpeg_param.width = (uint16_t)control->config.width;
	control->jpeg_param.height = (uint16_t)control->config.height;
	control->jpeg_param.in_type = jpeg_map_input_format(control->config.input_format);
	control->jpeg_param.quality = control->config.quality;
	control->jpeg_param.frame_done_cb = jpeg_encode_done_cb;
	control->jpeg_param.args = (uint32_t)(uintptr_t)control;

	vcenc_ret_e jr = vcenc_jpeg_init(&control->jpeg_param);
	if (jr != VCENC_OK) {
		LOGE("vcenc_jpeg_init failed: %d\r\n", jr);
		return jpeg_vcenc_ret_to_avdk(jr);
	}

	jr = vcenc_jpeg_open(&control->jpeg_param);
	if (jr != VCENC_OK) {
		LOGE("vcenc_jpeg_open failed: %d\r\n", jr);
		(void)vcenc_jpeg_deinit(&control->jpeg_param);
		return jpeg_vcenc_ret_to_avdk(jr);
	}

	control->enc_status = 1;
	control->opened = 1;

	bk_err_t tr = rtos_create_hsram_thread(&control->thread,
					       BEKEN_DEFAULT_WORKER_PRIORITY,
					       "jpeg_encoder",
					       (beken_thread_function_t)jpeg_encoder_entry,
					       CONFIG_BK_ENCODER_H264_TASK_SIZE,
					       control);
	if (tr != BK_OK) {
		LOGE("Create JPEG encoder thread failed: %d\r\n", tr);
		control->enc_status = 0;
		control->opened = 0;
		(void)vcenc_jpeg_close(&control->jpeg_param);
		(void)vcenc_jpeg_deinit(&control->jpeg_param);
		return AVDK_ERR_GENERIC;
	}

	rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);

	LOGI("JPEG encoder opened %ux%u (worker thread ready)\r\n",
	     control->config.width, control->config.height);
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_encode_ctlr_close(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	control->enc_status = 0;
	rtos_set_semaphore(&control->enc_start_sem);
	rtos_get_semaphore(&control->sem, BEKEN_WAIT_FOREVER);

	if (control->opened) {
		(void)vcenc_jpeg_close(&control->jpeg_param);
		(void)vcenc_jpeg_deinit(&control->jpeg_param);
		control->opened = 0;
	}

	LOGI("JPEG encoder closed\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_encode_ctlr_deinit(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	rtos_deinit_semaphore(&control->sem);
	rtos_deinit_semaphore(&control->enc_start_sem);
	rtos_deinit_semaphore(&control->enc_done_sem);

	avdk_err_t ret = hw_encoder_unregister(control);
	if (ret != AVDK_ERR_OK)
		LOGE("Unregister from hw encoder controller failed: %d\r\n", ret);
	else
		LOGI("JPEG encoder unregistered\r\n");

	return ret;
}

static avdk_err_t jpeg_encode_ctlr_ioctl(bk_jpeg_encode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	switch ((bk_jpeg_encode_ioctl_cmd_t)cmd) {
	case BK_JPEG_ENCODE_IOCTL_SET_QUALITY:
		if (arg == NULL)
			return AVDK_ERR_INVAL;
		control->config.quality = *(uint8_t *)arg;
		control->jpeg_param.quality = control->config.quality;
		return AVDK_ERR_OK;
	default:
		return AVDK_ERR_UNSUPPORTED;
	}
}

static avdk_err_t jpeg_encode_ctlr_del(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	os_free(control);
	return AVDK_ERR_OK;
}

/**
 * Same handshake as bk_h264_encode_start: wake internal encoder thread, wait until this frame completes.
 */
static avdk_err_t jpeg_encode_ctlr_encode_frame(bk_jpeg_encode_ctlr_handle_t handle, bk_jpeg_encode_input_t *input)
{
	private_jpeg_encode_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(input, AVDK_ERR_INVAL, TAG, "input is NULL");
	AVDK_RETURN_ON_FALSE(control->opened && control->enc_status, AVDK_ERR_INVAL, TAG, "encoder not running");

	uint32_t in_base = input->pic_buf ? input->pic_buf : control->config.input_buf;

	if (in_base == 0) {
		LOGE("invalid input buffer\r\n");
		return AVDK_ERR_INVAL;
	}

	os_memcpy(&control->pending_input, input, sizeof(*input));

	rtos_set_semaphore(&control->enc_start_sem);
	rtos_get_semaphore(&control->enc_done_sem, BEKEN_WAIT_FOREVER);

	return jpeg_vcenc_ret_to_avdk(control->last_ret);
}

avdk_err_t bk_jpeg_encode_frame_new(bk_jpeg_encode_ctlr_handle_t *handle, bk_jpeg_encode_frame_config_t *config)
{
	AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(config->width && config->height, AVDK_ERR_INVAL, TAG, "invalid size");

	private_jpeg_encode_frame_ctlr_t *ctrl =
		(private_jpeg_encode_frame_ctlr_t *)os_malloc(sizeof(private_jpeg_encode_frame_ctlr_t));
	if (ctrl == NULL)
		return AVDK_ERR_NOMEM;

	os_memset(ctrl, 0, sizeof(*ctrl));
	os_memcpy(&ctrl->config, config, sizeof(*config));

	ctrl->ops.init = jpeg_encode_ctlr_init;
	ctrl->ops.open = jpeg_encode_ctlr_open;
	ctrl->ops.encode_frame = jpeg_encode_ctlr_encode_frame;
	ctrl->ops.close = jpeg_encode_ctlr_close;
	ctrl->ops.deinit = jpeg_encode_ctlr_deinit;
	ctrl->ops.ioctl = jpeg_encode_ctlr_ioctl;
	ctrl->ops.del = jpeg_encode_ctlr_del;

	*handle = &ctrl->ops;
	return AVDK_ERR_OK;
}
