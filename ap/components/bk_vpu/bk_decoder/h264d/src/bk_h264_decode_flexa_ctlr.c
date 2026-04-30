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
#include "components/bk_decode/bk_h264_decode_ctlr.h"
#include "private_h264_decode_ctlr.h"
#include "hw_decoder_ctlr.h"
#include "modules/vcdec/vcdec_h264_api.h"
#include "avdk_monitor.h"

#define TAG "bk_h264_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264_DECODE_PORT_DONE_BIT(id) (1U << (id))

static void h264_decode_apply_min_rd_to_hw(private_h264_decode_flexa_ctlr_t *ctrl)
{
	uint32_t min_rd = 0xFFFFFFFFU;

	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond != NULL && ctrl->port[i].first_bond == 0U) {
			if (ctrl->port[i].rd_blocks < min_rd) {
				min_rd = ctrl->port[i].rd_blocks;
			}
		}
	}

	if (min_rd != 0xFFFFFFFFU) {
		if (min_rd > ctrl->all_ports_min_rd) {
			ctrl->all_ports_min_rd = min_rd;
			DECODE_LINE_START;
			vcdec_h264_set_rd_ptr(ctrl->vcdec_handle, min_rd);
		} else if (min_rd < ctrl->all_ports_min_rd) {
			LOGW("%s %d min_rd %u is less than all_ports_min_rd %u\r\n", __func__, __LINE__, min_rd, ctrl->all_ports_min_rd);
		}
	}
}

static void h264_decode_flexa_clear_port_done_events(private_h264_decode_flexa_ctlr_t *ctrl)
{
	uint32_t mask = 0U;

	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond != NULL) {
			mask |= H264_DECODE_PORT_DONE_BIT(i);
		}
	}
	if (mask != 0U && ctrl->port_done_events != NULL) {
		(void)rtos_clear_event_flags(&ctrl->port_done_events, mask);
	}
}

static avdk_err_t h264_decode_wait_flexa_registered_ports_done(private_h264_decode_flexa_ctlr_t *ctrl)
{
	uint32_t mask = 0U;

	if (ctrl->port_done_events == NULL) {
		LOGE("%s %d port_done_events not initialized\r\n", __func__, __LINE__);
		return AVDK_ERR_GENERIC;
	}

	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond != NULL && ctrl->port[i].first_bond == 0U) {
			mask |= H264_DECODE_PORT_DONE_BIT(i);
		}
	}

	if (mask == 0U) {
		return AVDK_ERR_OK;
	}

	beken_event_flags_t ux = rtos_wait_for_event_flags(&ctrl->port_done_events, mask, true, WAIT_FOR_ALL_EVENTS, 800U);
	if ((ux & mask) != mask) {
		for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			uint32_t bit = H264_DECODE_PORT_DONE_BIT(i);
			if ((mask & bit) != 0U && (ux & bit) == 0U) {
				LOGW("%s %d flexa port[%u] decode timeout\r\n", __func__, __LINE__, (unsigned)i);
			}
		}
		return AVDK_ERR_GENERIC;
	}

	return AVDK_ERR_OK;
}

static void h264_decode_notify_flexa_bonds_error(private_h264_decode_flexa_ctlr_t *ctrl)
{
	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond == NULL) {
			continue;
		}
		bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
		if (b->error != NULL) {
			b->error(BK_FAIL, b);
		}
	}
}

static void frame_done_cb(int status, void *args)
{
	DECODE_FRAME_DONE;
	private_h264_decode_flexa_ctlr_t *ctrl = (private_h264_decode_flexa_ctlr_t *)args;
	if (ctrl == NULL) {
		LOGE("control is NULL\r\n");
		return;
	}
	if (ctrl->config.frame_done_cb != NULL) {
		ctrl->config.frame_done_cb(status, ctrl->config.frame_done_args);
	}
}

static void flexa_done_cb(uint32_t wr_ptr, void *args)
{
	DECODE_LINE_END;
	private_h264_decode_flexa_ctlr_t *ctrl = (private_h264_decode_flexa_ctlr_t *)args;
	if (ctrl == NULL) {
		LOGE("control is NULL\r\n");
		return;
	}

	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond == NULL) {
			continue;
		}
		bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
		if (ctrl->port[i].first_bond != 0U) {
			if (wr_ptr == 1U) {
				ctrl->port[i].first_bond = 0U;
				if (b->flexa_done != NULL) {
					b->flexa_done(wr_ptr, b);
				}
			}
		} else if (b->flexa_done != NULL) {
			b->flexa_done(wr_ptr, b);
		}
	}

	if (ctrl->config.flexa_done_cb != NULL) {
		ctrl->config.flexa_done_cb(wr_ptr, ctrl->config.flexa_done_args);
	}
}

static avdk_err_t h264_decode_ctlr_init(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);
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

	ret = rtos_init_event_flags(&ctrl->port_done_events);
	if (ret != kNoErr) {
		LOGE("%s %d init port_done_events failed\r\n", __func__, __LINE__);
		goto error;
	}

	vcdec_config_t cfg = {0};
	cfg.mode = ctrl->mode;
	cfg.timeout_ms = (ctrl->config.timeout_ms != 0U) ? ctrl->config.timeout_ms : 1000U;
	cfg.frame_done_cb = frame_done_cb;
	cfg.flexa_done_cb = flexa_done_cb;
	cfg.args = ctrl;

	if (vcdec_h264_init(&ctrl->vcdec_handle, &cfg) != VCDEC_OK) {
		LOGE("vcdec_h264_init failed\r\n");
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	if (vcdec_h264_memalloc_register(ctrl->vcdec_handle, h264_decode_mem_malloc, h264_decode_mem_free) != VCDEC_OK) {
		LOGE("vcdec_h264_memalloc_register failed\r\n");
		ret = AVDK_ERR_GENERIC;
		goto error;
	}

	LOGI("%s %d H264 decoder registered to hw controller\r\n", __func__, __LINE__);
	return AVDK_ERR_OK;

error:
	if (ctrl->vcdec_handle != NULL) {
		vcdec_h264_deinit(ctrl->vcdec_handle);
		ctrl->vcdec_handle = NULL;
	}
	if (ctrl->port_done_events != NULL) {
		(void)rtos_deinit_event_flags(&ctrl->port_done_events);
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
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (vcdec_h264_open(ctrl->vcdec_handle) != VCDEC_OK) {
		LOGE("vcdec_h264_open failed\r\n");
		ctrl->vcdec_handle = NULL;
		return AVDK_ERR_GENERIC;
	}

	LOGI("H264 decoder opened\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_callback(void *param)
{
	private_h264_decode_flexa_ctlr_t *ctrl = (private_h264_decode_flexa_ctlr_t *)param;
	vcdec_ret_e ret;

	if (ctrl == NULL || ctrl->vcdec_handle == NULL) {
		return AVDK_ERR_INVAL;
	}

	h264_decode_flexa_clear_port_done_events(ctrl);
	ctrl->all_ports_min_rd = 0U;
	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond != NULL) {
			ctrl->port[i].rd_blocks = 0U;
		}
	}

	DECODE_FRAME_START;
	DECODE_LINE_START;
	ret = vcdec_h264_decode_frame(ctrl->vcdec_handle, &ctrl->decode_config);
	if (ret != VCDEC_FRAME_READY && ret != VCDEC_OK) {
		LOGE("%s %d vcdec_h264_decode_frame failed: %d\r\n", __func__, __LINE__, (int)ret);
		ctrl->decode_result = AVDK_ERR_GENERIC;
		DECODE_FRAME_END;
		return AVDK_ERR_GENERIC;
	}
	DECODE_FRAME_END;
	ctrl->decode_result = BK_OK;

	ret = h264_decode_wait_flexa_registered_ports_done(ctrl);
	if (ret != AVDK_ERR_OK) {
		LOGE("%s %d wait flexa registered ports done failed: %d\r\n", __func__, __LINE__, ret);
	}

	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_decode_frame(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_input_t *input)
{
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);
	uint32_t rb_h;
	uint32_t out_w;

	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(input, AVDK_ERR_INVAL, TAG, "input is NULL");
	AVDK_RETURN_ON_FALSE(input->stream && input->stream_len > 0U, AVDK_ERR_INVAL, TAG, "invalid stream");
	AVDK_RETURN_ON_FALSE(ctrl->vcdec_handle, AVDK_ERR_INVAL, TAG, "decoder not open");

	out_w = (uint32_t)ctrl->config.out_width;
	rb_h = 16U * (uint32_t)ctrl->config.segment_height * (uint32_t)ctrl->config.segment_number;
	if (out_w == 0U) {
		LOGE("Flexa mode requires out_width set\r\n");
		return AVDK_ERR_INVAL;
	}
	if (input->out_buffer == NULL || input->out_buffer_size == 0U) {
		LOGE("No output buffer or size\r\n");
		return AVDK_ERR_INVAL;
	}
	if (input->out_buffer_size < out_w * rb_h * 3U / 2U) {
		LOGE("output buffer too small: have=%u need=%u\r\n", input->out_buffer_size, out_w * rb_h * 3U / 2U);
		return AVDK_ERR_NOMEM;
	}

	ctrl->decode_config.input_stream = input->stream;
	ctrl->decode_config.input_stream_len = input->stream_len;
	ctrl->decode_config.output_buffer = input->out_buffer;
	ctrl->decode_config.output_size = input->out_buffer_size;
	ctrl->decode_config.segment_height = ctrl->config.segment_height;
	ctrl->decode_config.segment_number = ctrl->config.segment_number;

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
		for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			if (ctrl->port[i].bond != NULL) {
				bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
				if (b->frame_done != NULL) {
					b->frame_done(BK_FAIL, b);
				}
			}
		}
		h264_decode_notify_flexa_bonds_error(ctrl);
		return AVDK_ERR_GENERIC;
	}

	if (ctrl->decode_result != BK_OK) {
		if (ctrl->config.frame_done_cb != NULL) {
			ctrl->config.frame_done_cb(BK_FAIL, ctrl->config.frame_done_args);
		}
		for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			if (ctrl->port[i].bond != NULL) {
				bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
				if (b->frame_done != NULL) {
					b->frame_done(BK_FAIL, b);
				}
			}
		}
		h264_decode_notify_flexa_bonds_error(ctrl);
		return AVDK_ERR_GENERIC;
	}

	for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
		if (ctrl->port[i].bond != NULL) {
			bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
			if (b->frame_done != NULL) {
				b->frame_done(BK_OK, b);
			}
		}
	}
	if (ctrl->config.frame_done_cb != NULL) {
		ctrl->config.frame_done_cb(BK_OK, ctrl->config.frame_done_args);
	}

	return AVDK_ERR_OK;
}

static void h264_decode_resources_deinit(private_h264_decode_flexa_ctlr_t *ctrl)
{
	if (ctrl->port_done_events != NULL) {
		(void)rtos_deinit_event_flags(&ctrl->port_done_events);
	}
	if (ctrl->decode_done_sem != NULL) {
		rtos_deinit_semaphore(&ctrl->decode_done_sem);
		ctrl->decode_done_sem = NULL;
	}
}

static avdk_err_t h264_decode_ctlr_close(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (ctrl->vcdec_handle != NULL) {
		vcdec_h264_close(ctrl->vcdec_handle);
	}
	LOGI("H264 decoder closed\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_deinit(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);
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
	h264_decode_resources_deinit(ctrl);
	LOGI("H264 decoder unregistered from hw controller\r\n");
	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_ioctl(bk_h264_decode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);

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
	case BK_H264_DECODE_IOCTL_PORT_SET_RD_PTR: {
		uint32_t flags = rtos_enter_critical();
		bk_h264_decode_port_rd_t *p = (bk_h264_decode_port_rd_t *)arg;
		uint32_t i = 0U;

		AVDK_RETURN_ON_FALSE(p, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
		for (i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			if (ctrl->port[i].bond == p->port_ptr) {
				ctrl->port[i].rd_blocks = p->rd_blocks;
				break;
			}
		}
		if (i == BK_H264_DECODE_RD_PORT_MAX) {
			LOGE("%s %d no free port\r\n", __func__, __LINE__);
			rtos_exit_critical(flags);
			return AVDK_ERR_NOMEM;
		}
		h264_decode_apply_min_rd_to_hw(ctrl);
		rtos_exit_critical(flags);
		break;
	}
	case BK_H264_DECODE_IOCTL_REGISTER_BOND: {
		void *bond_ptr = arg;
		uint32_t i = 0U;

		AVDK_RETURN_ON_FALSE(bond_ptr, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
		for (i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			if (ctrl->port[i].bond == NULL) {
				ctrl->port[i].bond = bond_ptr;
				ctrl->port[i].first_bond = 1U;
				break;
			}
		}
		if (i == BK_H264_DECODE_RD_PORT_MAX) {
			LOGE("%s %d no free port\r\n", __func__, __LINE__);
			return AVDK_ERR_NOMEM;
		}
		break;
	}
	case BK_H264_DECODE_IOCTL_UNREGISTER_BOND: {
		void *bond_ptr = arg;

		AVDK_RETURN_ON_FALSE(bond_ptr, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
		for (uint32_t i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			if (ctrl->port[i].bond == bond_ptr) {
				ctrl->port[i].bond = NULL;
				ctrl->port[i].first_bond = 0U;
				break;
			}
		}
		break;
	}
	case BK_H264_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE: {
		void *bond_ptr = arg;
		uint32_t i = 0U;

		AVDK_RETURN_ON_FALSE(bond_ptr, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
		for (i = 0; i < BK_H264_DECODE_RD_PORT_MAX; i++) {
			if (ctrl->port[i].bond == bond_ptr) {
				break;
			}
		}
		if (i == BK_H264_DECODE_RD_PORT_MAX) {
			LOGE("%s %d no found port\r\n", __func__, __LINE__);
			return AVDK_ERR_NOMEM;
		}
		if (ctrl->port_done_events == NULL) {
			LOGE("%s %d port_done_events is NULL\r\n", __func__, __LINE__);
			break;
		}
		rtos_set_event_flags(&ctrl->port_done_events, H264_DECODE_PORT_DONE_BIT(i));
		break;
	}
	default:
		LOGE("Unknown ioctl: %u\r\n", (unsigned)cmd);
		return AVDK_ERR_INVAL;
	}

	return AVDK_ERR_OK;
}

static avdk_err_t h264_decode_ctlr_delete(bk_h264_decode_ctlr_handle_t handle)
{
	private_h264_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_h264_decode_flexa_ctlr_t, ops);
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

	os_free(ctrl);
	LOGI("H264 decoder deleted\r\n");
	return AVDK_ERR_OK;
}

avdk_err_t bk_h264_decode_flexa_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_flexa_config_t *config)
{
	private_h264_decode_flexa_ctlr_t *ctrl;

	AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
	AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

	ctrl = (private_h264_decode_flexa_ctlr_t *)os_malloc(sizeof(private_h264_decode_flexa_ctlr_t));
	AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

	os_memset(ctrl, 0, sizeof(private_h264_decode_flexa_ctlr_t));
	os_memcpy(&ctrl->config, config, sizeof(bk_h264_decode_flexa_config_t));

	ctrl->mode = VCDEC_FLEXA_MODE_FLEXA;
	ctrl->ops.init = h264_decode_ctlr_init;
	ctrl->ops.open = h264_decode_ctlr_open;
	ctrl->ops.decode_frame = h264_decode_ctlr_decode_frame;
	ctrl->ops.close = h264_decode_ctlr_close;
	ctrl->ops.deinit = h264_decode_ctlr_deinit;
	ctrl->ops.ioctl = h264_decode_ctlr_ioctl;
	ctrl->ops.del = h264_decode_ctlr_delete;

	*handle = &ctrl->ops;
	LOGI("H264 decoder controller created\r\n");
	return AVDK_ERR_OK;
}
