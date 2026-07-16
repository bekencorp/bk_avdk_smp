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

#include <stdbool.h>
#include <stdint.h>

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>
#include <common/avdk_pixel_types.h>

#include "modules/vcdec/vcdec_types.h"
#include "modules/vcdec/vcdec_pp_api.h"
#include "components/bk_decode/bk_pp_ctlr.h"

#include "private_pp_ctlr.h"
#include "bk_decode_pp_helper.h"
#include "hw_decoder_ctlr.h"

#define TAG "bk_pp"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

#define BK_PP_DEFAULT_TIMEOUT_MS 1000U

static avdk_err_t pp_map_ret(vcdec_ret_e ret)
{
	switch (ret) {
	case VCDEC_OK:
	case VCDEC_FRAME_READY:
		return AVDK_ERR_OK;
	case VCDEC_NULL_ARGUMENT:
	case VCDEC_INVALID_ARGUMENT:
		return AVDK_ERR_INVAL;
	case VCDEC_MEMORY_ERROR:
		return AVDK_ERR_NOMEM;
	case VCDEC_HW_TIMEOUT:
		return AVDK_ERR_TIMEOUT;
	case VCDEC_HW_BUS_ERROR:
	case VCDEC_HW_ERROR:
		return AVDK_ERR_HWERROR;
	default:
		return AVDK_ERR_GENERIC;
	}
}

static void pp_done_cb(int status, void *args)
{
	private_pp_ctlr_t *ctrl = (private_pp_ctlr_t *)args;

	if (ctrl == NULL) {
		return;
	}
	if (ctrl->config.done_cb != NULL) {
		ctrl->config.done_cb(status, ctrl->config.done_args);
	}
}

static void pp_ctlr_resources_deinit(private_pp_ctlr_t *ctrl)
{
	if (ctrl->process_done_sem != NULL) {
		rtos_deinit_semaphore(&ctrl->process_done_sem);
		ctrl->process_done_sem = NULL;
	}
}

static avdk_err_t pp_process_callback(void *param)
{
	private_pp_ctlr_t *ctrl = (private_pp_ctlr_t *)param;
	vcdec_ret_e vret;

	if (ctrl == NULL || ctrl->vcdec_pp_handle == NULL) {
		return AVDK_ERR_INVAL;
	}

	vret = vcdec_pp_process(ctrl->vcdec_pp_handle, &ctrl->process_req);
	ctrl->process_result = pp_map_ret(vret);
	if (ctrl->process_result != AVDK_ERR_OK) {
		LOGE("pp process failed, ret=%d\r\n", (int)vret);
	}
	return ctrl->process_result;
}

static avdk_err_t pp_ctlr_init(bk_pp_ctlr_handle_t handle)
{
	private_pp_ctlr_t *ctrl = __containerof(handle, private_pp_ctlr_t, ops);
	vcdec_pp_process_config_t cfg = {0};
	vcdec_ret_e vret;
	avdk_err_t ret;

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	ret = hw_decoder_register(HW_DECODER_TYPE_PP, ctrl);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d hw decoder register failed: %d\r\n", __func__, __LINE__, ret);
		return ret;
	}
	ctrl->hw_registered = 1U;

	ret = rtos_init_semaphore(&ctrl->process_done_sem, 1);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d init process_done_sem failed\r\n", __func__, __LINE__);
		goto error;
	}

	cfg.timeout_ms = (ctrl->config.timeout_ms != 0U) ?
		ctrl->config.timeout_ms : BK_PP_DEFAULT_TIMEOUT_MS;
	cfg.done_cb = pp_done_cb;
	cfg.args = ctrl;

	vret = vcdec_pp_process_init(&ctrl->vcdec_pp_handle, &cfg);
	if (vret != VCDEC_OK) {
		ret = pp_map_ret(vret);
		LOGE("%s %d pp process init failed: %d\r\n", __func__, __LINE__, (int)vret);
		goto error;
	}

	LOGI("%s %d PP controller initialized\r\n", __func__, __LINE__);
	return AVDK_ERR_OK;

error:
	pp_ctlr_resources_deinit(ctrl);
	if (ctrl->vcdec_pp_handle != NULL) {
		vcdec_pp_process_deinit(ctrl->vcdec_pp_handle);
		ctrl->vcdec_pp_handle = NULL;
	}
	if (ctrl->hw_registered) {
		hw_decoder_unregister(ctrl);
		ctrl->hw_registered = 0U;
	}
	return ret;
}

static avdk_err_t pp_ctlr_open(bk_pp_ctlr_handle_t handle)
{
	private_pp_ctlr_t *ctrl = __containerof(handle, private_pp_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(ctrl->vcdec_pp_handle, AVDK_ERR_INVAL, TAG, "PP not initialized");
	return AVDK_ERR_OK;
}

static avdk_err_t pp_ctlr_process(bk_pp_ctlr_handle_t handle, const bk_pp_process_req_t *req)
{
	private_pp_ctlr_t *ctrl = __containerof(handle, private_pp_ctlr_t, ops);
	vcdec_pp_out_format_e pp_fmt;
	vcdec_pp_process_req_t vreq = {0};
	uint16_t out_w;
	uint16_t out_h;
	uint16_t out_h_storage;
	uint32_t need_size;
	avdk_err_t ret;

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(ctrl->vcdec_pp_handle, AVDK_ERR_INVAL, TAG, "PP not initialized");
	AVDK_RETURN_ON_FALSE(req, AVDK_ERR_INVAL, TAG, "req is NULL");

	if (req->in_y == NULL || req->in_c == NULL || req->out_buffer == NULL ||
	    req->in_width == 0U || req->in_height == 0U) {
		LOGE("invalid pp process request\r\n");
		return AVDK_ERR_INVAL;
	}

	if (req->in_format != BK_PIXEL_FORMAT_NV12) {
		LOGE("unsupported in_format=%u (NV12 only)\r\n", (unsigned)req->in_format);
		return AVDK_ERR_UNSUPPORTED;
	}

	pp_fmt = bk_decode_pp_map_out_format(req->out_format);
	if (pp_fmt != VCDEC_PP_OUT_NV12 && pp_fmt != VCDEC_PP_OUT_RGB565 &&
	    pp_fmt != VCDEC_PP_OUT_RGB888) {
		LOGE("unsupported out_format=%u (NV12/RGB565/RGB888 only)\r\n",
		     (unsigned)req->out_format);
		return AVDK_ERR_UNSUPPORTED;
	}

	out_w = (req->out_width != 0U) ? req->out_width : req->in_width;
	out_h = (req->out_height != 0U) ? req->out_height : req->in_height;
	out_h_storage = (uint16_t)((out_h + 1U) & ~1U);
	if (req->out_format == BK_PIXEL_FORMAT_RGB565 ||
	    req->out_format == BK_PIXEL_FORMAT_RGB888) {
		out_h_storage = (uint16_t)((out_h + 15U) & ~15U);
	}

	need_size = bk_decode_pp_output_size(req->out_format, out_w, out_h_storage);
	if (req->out_size < need_size) {
		LOGE("out buffer too small: size=%u need=%u\r\n",
		     (unsigned)req->out_size, (unsigned)need_size);
		return AVDK_ERR_INVAL;
	}

	vreq.in_y_bus = (uint32_t)(uintptr_t)req->in_y;
	vreq.in_c_bus = (uint32_t)(uintptr_t)req->in_c;
	vreq.in_width = req->in_width;
	vreq.in_height = req->in_height;
	vreq.out_width = req->out_width;
	vreq.out_height = req->out_height;
	vreq.out_buffer = req->out_buffer;
	vreq.out_size = req->out_size;
	vreq.out_format = pp_fmt;

	os_memcpy(&ctrl->process_req, &vreq, sizeof(vreq));

	{
		uint32_t wait_ms = (ctrl->config.timeout_ms != 0U) ?
			ctrl->config.timeout_ms : BK_PP_DEFAULT_TIMEOUT_MS;
		hw_decoder_msg_t msg = {
			.decoder_type = HW_DECODER_TYPE_PP,
			.type = HW_DECODER_MSG_DECODE,
			.callback = pp_process_callback,
			.param = ctrl,
			.sem = &ctrl->process_done_sem,
		};

		ret = hw_decoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
		if (ret != AVDK_ERR_OK) {
			return ret;
		}

		ret = rtos_get_semaphore(&ctrl->process_done_sem, wait_ms + 2000U);
		if (ret != AVDK_ERR_OK) {
			LOGE("%s %d process_done_sem timeout\r\n", __func__, __LINE__);
			return AVDK_ERR_TIMEOUT;
		}
	}

	return ctrl->process_result;
}

static avdk_err_t pp_ctlr_close(bk_pp_ctlr_handle_t handle)
{
	private_pp_ctlr_t *ctrl = __containerof(handle, private_pp_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
	return AVDK_ERR_OK;
}

static avdk_err_t pp_ctlr_deinit(bk_pp_ctlr_handle_t handle)
{
	private_pp_ctlr_t *ctrl = __containerof(handle, private_pp_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (ctrl->vcdec_pp_handle != NULL) {
		vcdec_pp_process_deinit(ctrl->vcdec_pp_handle);
		ctrl->vcdec_pp_handle = NULL;
	}
	pp_ctlr_resources_deinit(ctrl);
	if (ctrl->hw_registered) {
		avdk_err_t ret = hw_decoder_unregister(ctrl);
		if (ret != AVDK_ERR_OK) {
			LOGE("hw decoder unregister failed: %d\r\n", ret);
			return ret;
		}
		ctrl->hw_registered = 0U;
	}
	LOGI("%s %d PP controller deinitialized\r\n", __func__, __LINE__);
	return AVDK_ERR_OK;
}

static avdk_err_t pp_ctlr_delete(bk_pp_ctlr_handle_t handle)
{
	private_pp_ctlr_t *ctrl = __containerof(handle, private_pp_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	os_free(ctrl);
	LOGI("%s %d PP controller deleted\r\n", __func__, __LINE__);
	return AVDK_ERR_OK;
}

avdk_err_t bk_pp_ctlr_new(bk_pp_ctlr_handle_t *handle, bk_pp_config_t *config)
{
	private_pp_ctlr_t *ctrl;

	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
	AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

	ctrl = (private_pp_ctlr_t *)os_malloc(sizeof(private_pp_ctlr_t));
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

	os_memset(ctrl, 0, sizeof(private_pp_ctlr_t));
	os_memcpy(&ctrl->config, config, sizeof(bk_pp_config_t));

	ctrl->ops.init = pp_ctlr_init;
	ctrl->ops.open = pp_ctlr_open;
	ctrl->ops.process = pp_ctlr_process;
	ctrl->ops.close = pp_ctlr_close;
	ctrl->ops.deinit = pp_ctlr_deinit;
	ctrl->ops.del = pp_ctlr_delete;

	*handle = &ctrl->ops;
	LOGI("%s %d PP controller created\r\n", __func__, __LINE__);
	return AVDK_ERR_OK;
}
