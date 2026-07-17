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

#include <stddef.h>
#include <stdint.h>

#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_check.h>
#include <common/bk_err.h>
#include <modules/jpeg_enc_sw.h>

#include "components/bk_encode/bk_jpeg_encode_ctlr.h"

#define TAG "bk_jpeg_enc_swfrm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

extern int tjpg_encoder_open(void **codec, uint16_t width, uint16_t height,
			     uint8_t *input, uint8_t *output,
			     uint16_t *header_len, uint8_t quality_factor);
extern int tjpg_reset(void *codec);
extern int tjpg_encode(void *codec, uint8_t *output, int output_buffer_left_size,
		       int *enc_size);
extern int tjpg_encoder_deinit(void **codec);

typedef struct {
	bk_jpeg_encode_sw_frame_config_t config;
	jpeg_sw_encoder_t encoder;
	uint8_t inited;
	uint8_t opened;
	bk_jpeg_encode_ctlr_t ops;
} private_jpeg_encode_sw_frame_ctlr_t;

static void jpeg_sw_frame_encoder_init(jpeg_sw_encoder_t *encoder)
{
	if (encoder == NULL) {
		return;
	}

	encoder->open = tjpg_encoder_open;
	encoder->reset = tjpg_reset;
	encoder->enc = tjpg_encode;
	encoder->deinit = tjpg_encoder_deinit;
	encoder->codec = NULL;
}

static avdk_err_t jpeg_sw_frame_ctlr_init(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	jpeg_sw_frame_encoder_init(&control->encoder);
	control->inited = 1;
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_sw_frame_ctlr_open(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(control->inited, AVDK_ERR_INVAL, TAG, "encoder not inited");

	control->opened = 1;
	LOGI("JPEG software frame encoder opened %ux%u\r\n",
	     control->config.width, control->config.height);
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_sw_frame_ctlr_encode_frame(bk_jpeg_encode_ctlr_handle_t handle,
						  bk_jpeg_encode_input_t *input)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);
	uint32_t in_base;
	uint32_t out_base;
	uint32_t out_size;
	uint16_t header_len = 0;
	int enc_size = 0;
	int ret;

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
	AVDK_RETURN_ON_FALSE(input, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(control->opened, AVDK_ERR_INVAL, TAG, "encoder not opened");

	in_base = input->pic_buf ? input->pic_buf : control->config.input_buf;
	out_base = input->out_buf;
	out_size = input->out_size;
	AVDK_RETURN_ON_FALSE(in_base && out_size, AVDK_ERR_INVAL, TAG, "invalid input/output buffer");
	if (out_base == 0 && control->config.outbuf_malloc != NULL) {
		void *out = control->config.outbuf_malloc(out_size, control->config.outbuf_malloc_args);
		out_base = (uint32_t)(uintptr_t)out;
	}
	AVDK_RETURN_ON_FALSE(out_base, AVDK_ERR_INVAL, TAG, "invalid output buffer");

	if (control->encoder.codec != NULL) {
		(void)control->encoder.deinit(&control->encoder.codec);
		control->encoder.codec = NULL;
	}
	ret = control->encoder.open(&control->encoder.codec,
				    control->config.width,
				    control->config.height,
				    (uint8_t *)(uintptr_t)in_base,
				    (uint8_t *)(uintptr_t)out_base,
				    &header_len,
				    control->config.quality);
	if (ret != BK_OK) {
		return AVDK_ERR_GENERIC;
	}

	if ((uint32_t)header_len >= out_size) {
		(void)control->encoder.deinit(&control->encoder.codec);
		control->encoder.codec = NULL;
		return AVDK_ERR_NOMEM;
	}

	ret = control->encoder.enc(control->encoder.codec,
				   (uint8_t *)(uintptr_t)(out_base + header_len),
				   (int)(out_size - header_len),
				   &enc_size);
	(void)control->encoder.deinit(&control->encoder.codec);
	control->encoder.codec = NULL;
	if (ret != BK_OK || enc_size <= 0) {
		return AVDK_ERR_GENERIC;
	}

	if (control->config.outbuf_complete != NULL) {
		bk_jpeg_encode_outbuf_info_t info = {
			.outbuf = (void *)(uintptr_t)out_base,
			.length = (uint32_t)header_len + (uint32_t)enc_size,
			.type = 0,
			.status = BK_OK,
			.sequence = 0,
			.args = control->config.outbuf_complete_args,
		};
		control->config.outbuf_complete(&info);
	}
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_sw_frame_ctlr_close(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (control->encoder.codec != NULL) {
		(void)control->encoder.deinit(&control->encoder.codec);
		control->encoder.codec = NULL;
	}
	control->opened = 0;
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_sw_frame_ctlr_deinit(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (control->encoder.codec != NULL) {
		(void)control->encoder.deinit(&control->encoder.codec);
		control->encoder.codec = NULL;
	}
	control->inited = 0;
	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_sw_frame_ctlr_ioctl(bk_jpeg_encode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	switch ((bk_jpeg_encode_ioctl_cmd_t)cmd) {
	case BK_JPEG_ENCODE_IOCTL_SET_QUALITY:
		if (arg == NULL)
			return AVDK_ERR_INVAL;
		control->config.quality = *(uint8_t *)arg;
		return AVDK_ERR_OK;
	default:
		return AVDK_ERR_UNSUPPORTED;
	}
}

static avdk_err_t jpeg_sw_frame_ctlr_del(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	os_free(control);
	return AVDK_ERR_OK;
}

avdk_err_t bk_jpeg_encode_sw_frame_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle,
					    bk_jpeg_encode_sw_frame_config_t *config)
{
	private_jpeg_encode_sw_frame_ctlr_t *ctrl;

	AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
	AVDK_RETURN_ON_FALSE(config->width && config->height, AVDK_ERR_INVAL, TAG, "invalid size");

	ctrl = (private_jpeg_encode_sw_frame_ctlr_t *)os_malloc(sizeof(*ctrl));
	if (ctrl == NULL) {
		return AVDK_ERR_NOMEM;
	}

	os_memset(ctrl, 0, sizeof(*ctrl));
	os_memcpy(&ctrl->config, config, sizeof(*config));
	ctrl->ops.init = jpeg_sw_frame_ctlr_init;
	ctrl->ops.open = jpeg_sw_frame_ctlr_open;
	ctrl->ops.encode_frame = jpeg_sw_frame_ctlr_encode_frame;
	ctrl->ops.close = jpeg_sw_frame_ctlr_close;
	ctrl->ops.deinit = jpeg_sw_frame_ctlr_deinit;
	ctrl->ops.ioctl = jpeg_sw_frame_ctlr_ioctl;
	ctrl->ops.del = jpeg_sw_frame_ctlr_del;

	*handle = &ctrl->ops;
	return AVDK_ERR_OK;
}
