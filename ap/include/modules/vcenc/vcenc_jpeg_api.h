#pragma once

#include "modules/vcenc/vcenc_common.h"

typedef struct jpeg_enc_param_s {
	vcenc_handle instance;
	uint16_t width;
	uint16_t height;
	vcenc_mode_e enc_mode;
	vcenc_input_e in_type;
	uint32_t in_buffer;
	uint32_t in_lines;
	uint32_t out_buffer;
	uint32_t out_len;
	uint8_t quality; /* 0..10 → EncJpeg QuantLuminance index */
	uint32_t input_linebuf_depth;
	uint32_t input_linebuf_loopback_en;
	uint32_t input_linebuf_hw_mode_en;
	uint32_t amount_per_loopback;
	uint32_t linebuf_wr_cnt;
	vcenc_frame_done_cb frame_done_cb;
	vcenc_slice_done_cb slice_done_cb;
	uint32_t args;
} jpeg_enc_param_t;

void jpeg_vcenc_memalloc_register(void *(*pmalloc)(size_t), void (*pfree)(void *));

vcenc_ret_e jpeg_vcencoder_init(jpeg_enc_param_t *enc_param);

vcenc_ret_e jpeg_vcencoder_encode(jpeg_enc_param_t *enc_param);

vcenc_ret_e jpeg_vcencoder_stop_encode(jpeg_enc_param_t *enc_param);

vcenc_ret_e jpeg_vcencoder_flexa_input_linebuf_wrcnt_set(jpeg_enc_param_t *enc_param,
							  uint32_t wrcnt);

uint32_t jpeg_vcencoder_get_encoded_lines(void);

int jpeg_vcencoder_memfree(jpeg_enc_param_t *param);

vcenc_ret_e jpeg_vcencoder_deinit(jpeg_enc_param_t *enc_param);

/** Shared encoder IRQ entry (HW JPEG path); must stay compatible with hw_encoder_ctlr dispatch. */
void jpeg_vcenc_isr(void);
