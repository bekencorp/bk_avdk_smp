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
#include <stddef.h>
#include <stdint.h>

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_check.h>
#include <components/bk_frame_buffer.h>
#include <common/bk_err.h>
#include <modules/jpeg_enc_sw.h>

#include "components/bk_encode/bk_jpeg_encode_ctlr.h"
#include "jpeg_encode_osd_priv.h"
#if CONFIG_L2_CACHE_ENABLE || CONFIG_DCACHE
#include "cache.h"
#endif

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
	beken_mutex_t osd_lock;
	jpeg_encode_osd_slot_state_t osd_slots[JPEG_ENCODE_OSD_SLOT_COUNT];
	uint8_t *osd_stage_buf;
	uint32_t osd_stage_size;
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

static void jpeg_sw_frame_osd_release_slot(jpeg_encode_osd_slot_state_t *slot)
{
	if (slot == NULL) {
		return;
	}

	if (slot->buffer != NULL && slot->buffer_free != NULL) {
		slot->buffer_free(slot->buffer, slot->free_arg);
	}
	os_memset(slot, 0, sizeof(*slot));
}

static void jpeg_sw_frame_osd_release_all(private_jpeg_encode_sw_frame_ctlr_t *control)
{
	uint32_t i;

	if (control == NULL) {
		return;
	}

	for (i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
		jpeg_sw_frame_osd_release_slot(&control->osd_slots[i]);
	}

	if (control->osd_stage_buf != NULL) {
		bk_frame_buffer_free(control->osd_stage_buf);
		control->osd_stage_buf = NULL;
		control->osd_stage_size = 0U;
	}
}

static avdk_err_t jpeg_sw_frame_osd_install(private_jpeg_encode_sw_frame_ctlr_t *control,
					    bk_jpeg_encode_osd_t *osd)
{
	jpeg_encode_osd_slot_state_t *slot;
	avdk_err_t ret = AVDK_ERR_OK;

	if (control == NULL || osd == NULL) {
		return AVDK_ERR_INVAL;
	}
	if (!control->opened) {
		LOGE("jpeg encoder is not opened\r\n");
		return AVDK_ERR_INVAL;
	}
	if (osd->index >= JPEG_ENCODE_OSD_SLOT_COUNT) {
		LOGE("invalid osd index: %u\r\n", osd->index);
		return AVDK_ERR_INVAL;
	}

	if (osd->buffer != NULL) {
		ret = jpeg_encode_osd_validate_common(control->config.width,
						      control->config.height, osd);
		if (ret != AVDK_ERR_OK) {
			return ret;
		}
	}

	rtos_lock_mutex(&control->osd_lock);
	slot = &control->osd_slots[osd->index];

	if (osd->buffer == NULL) {
		jpeg_sw_frame_osd_release_slot(slot);
	} else {
		if (slot->buffer != NULL && slot->buffer != osd->buffer) {
			jpeg_sw_frame_osd_release_slot(slot);
		}
		slot->enabled = true;
		slot->buffer = osd->buffer;
		slot->buffer_free = osd->buffer_free;
		slot->free_arg = osd->free_arg;
		slot->format = osd->format;
		slot->alpha = osd->alpha;
		slot->x = osd->x;
		slot->y = osd->y;
		slot->width = osd->width;
		slot->height = osd->height;
		slot->bitmap_y = osd->bitmap_y;
		slot->bitmap_u = osd->bitmap_u;
		slot->bitmap_v = osd->bitmap_v;
	}
	rtos_unlock_mutex(&control->osd_lock);

	return AVDK_ERR_OK;
}

static avdk_err_t jpeg_sw_frame_osd_set_boxes(private_jpeg_encode_sw_frame_ctlr_t *control,
					      bk_jpeg_encode_osd_t *osd)
{
	avdk_err_t ret = jpeg_sw_frame_osd_install(control, osd);

	if (ret != AVDK_ERR_OK && osd->buffer != NULL && osd->buffer_free != NULL) {
		osd->buffer_free(osd->buffer, osd->free_arg);
	}
	return ret;
}

/*
 * Software OSD pass. Returns the buffer that must be fed to the tiny-jpeg
 * encoder.
 *
 * config.osd_in_place == 1: composite the OSD straight into the caller's YUYV422
 * frame, no full-frame copy at all. The source is modified in place, so the
 * caller must own a writable buffer that nobody else consumes while encoding.
 *
 * config.osd_in_place == 0: keep the source untouched and composite into an
 * internal staging copy of the frame (one full-frame memcpy per frame).
 */
static uint32_t jpeg_sw_frame_osd_apply(private_jpeg_encode_sw_frame_ctlr_t *control,
					uint32_t in_base)
{
	uint32_t frame_size = control->config.width * control->config.height * 2U;
	bool has_osd = false;
	uint32_t i;

	if (frame_size == 0U) {
		return in_base;
	}

	rtos_lock_mutex(&control->osd_lock);
	for (i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
		if (control->osd_slots[i].enabled && control->osd_slots[i].buffer != NULL) {
			has_osd = true;
			break;
		}
	}

	if (!has_osd) {
		rtos_unlock_mutex(&control->osd_lock);
		return in_base;
	}

	if (control->config.osd_in_place) {
		/* No staging copy: the source buffer is writable and owned by us. */
		jpeg_encode_osd_draw_yuyv((uint8_t *)(uintptr_t)in_base, control->config.width,
					  control->config.height, control->osd_slots);
		rtos_unlock_mutex(&control->osd_lock);
		return in_base;
	}

	if (control->osd_stage_size < frame_size) {
		if (control->osd_stage_buf != NULL) {
			bk_frame_buffer_free(control->osd_stage_buf);
			control->osd_stage_buf = NULL;
			control->osd_stage_size = 0U;
		}
		control->osd_stage_buf =
			(uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);
		if (control->osd_stage_buf == NULL) {
			rtos_unlock_mutex(&control->osd_lock);
			LOGE("alloc osd staging buffer %u bytes failed\r\n", frame_size);
			return in_base;
		}
		control->osd_stage_size = frame_size;
	}

	os_memcpy(control->osd_stage_buf, (void *)(uintptr_t)in_base, frame_size);
	jpeg_encode_osd_draw_yuyv(control->osd_stage_buf, control->config.width,
				  control->config.height, control->osd_slots);
	rtos_unlock_mutex(&control->osd_lock);

	return (uint32_t)(uintptr_t)control->osd_stage_buf;
}

static avdk_err_t jpeg_sw_frame_ctlr_init(bk_jpeg_encode_ctlr_handle_t handle)
{
	private_jpeg_encode_sw_frame_ctlr_t *control =
		__containerof(handle, private_jpeg_encode_sw_frame_ctlr_t, ops);

	AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

	if (rtos_init_mutex(&control->osd_lock) != BK_OK) {
		LOGE("init osd mutex failed\r\n");
		return AVDK_ERR_GENERIC;
	}

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

#if CONFIG_L2_CACHE_ENABLE || CONFIG_DCACHE
	{
		/* Packed YUYV422: two bytes per pixel. */
		uint32_t flush_sz = control->config.width * control->config.height * 2U;

		if (flush_sz > 0U) {
			flush_dcache((void *)(uintptr_t)in_base, (long)flush_sz);
		}
	}
#endif

	in_base = jpeg_sw_frame_osd_apply(control, in_base);

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
#if CONFIG_L2_CACHE_ENABLE || CONFIG_DCACHE
		flush_dcache((void *)(uintptr_t)out_base,
			     (long)((size_t)header_len + (size_t)enc_size));
#endif
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
	jpeg_sw_frame_osd_release_all(control);
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
	jpeg_sw_frame_osd_release_all(control);
	if (control->inited) {
		rtos_deinit_mutex(&control->osd_lock);
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
	case BK_JPEG_ENCODE_IOCTL_SET_OSD:
		return jpeg_sw_frame_osd_install(control, (bk_jpeg_encode_osd_t *)arg);
	case BK_JPEG_ENCODE_IOCTL_SET_OSD_BITMAP_BOXES: {
		bk_jpeg_encode_osd_t osd = {0};
		avdk_err_t ret = jpeg_encode_build_osd_bitmap_boxes(control->config.width,
								    control->config.height,
								    (bk_jpeg_encode_osd_bitmap_boxes_t *)arg,
								    &osd);
		if (ret != AVDK_ERR_OK)
			return ret;
		return jpeg_sw_frame_osd_set_boxes(control, &osd);
	}
	case BK_JPEG_ENCODE_IOCTL_SET_OSD_ARGB8888_BOXES: {
		bk_jpeg_encode_osd_t osd = {0};
		avdk_err_t ret = jpeg_encode_build_osd_argb8888_boxes(control->config.width,
								      control->config.height,
								      (bk_jpeg_encode_osd_argb8888_boxes_t *)arg,
								      &osd);
		if (ret != AVDK_ERR_OK)
			return ret;
		return jpeg_sw_frame_osd_set_boxes(control, &osd);
	}
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
