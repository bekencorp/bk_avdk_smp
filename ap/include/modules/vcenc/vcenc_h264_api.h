#pragma once

/**
 * @file vcenc_h264_api.h
 * @brief Public API for the H.264 encoder front-end.
 *
 * Lifecycle: vcenc_h264_init() -> vcenc_h264_open() -> vcenc_h264_encode_frame()*
 *           -> vcenc_h264_close() -> vcenc_h264_deinit().
 */

#include "modules/vcenc/vcenc_types.h"
#include "modules/vcenc/vcenc_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ENC_INSTANCE 2

/** Register memory allocator hooks shared by all H.264 encoder instances. */
void vcenc_h264_memalloc_register(void *(*pmalloc)(size_t), void (*pfree)(void *));

/** Allocate an instance and bind it to the caller-owned @ref h264_enc_param_t. */
vcenc_ret_e vcenc_h264_init(h264_enc_param_t *enc_param);

/** Open the encoder session: arms IRQ, prepares per-instance semaphore. */
vcenc_ret_e vcenc_h264_open(h264_enc_param_t *enc_param);

/** Encode one frame; blocks the caller on the per-instance encode semaphore. */
vcenc_ret_e vcenc_h264_encode_frame(h264_enc_param_t *enc_param);

/** Close the encoder session: stops HW, masks IRQ, releases per-instance sem. */
vcenc_ret_e vcenc_h264_close(h264_enc_param_t *enc_param);

/**
 * Abort an in-flight encode_frame: forces the engine to stop and wakes the
 * waiter with VCENC error. Equivalent to the legacy stop_encode entry point.
 */
vcenc_ret_e vcenc_h264_abort(h264_enc_param_t *enc_param);

/** Tear down the instance and free its associated buffers. */
vcenc_ret_e vcenc_h264_deinit(h264_enc_param_t *enc_param);

vcenc_ret_e vcenc_h264_set_osd(h264_enc_param_t *enc_param, uint32_t index, void *buffer,
				uint32_t format, uint8_t alpha, uint32_t x, uint32_t y,
				uint32_t width, uint32_t height);

vcenc_ret_e vcenc_h264_set_mosaic(h264_enc_param_t *enc_param, uint32_t index, uint32_t enable,
				   uint32_t x, uint32_t y, uint32_t width, uint32_t height);

vcenc_ret_e vcenc_h264_set_rate_ctrl(h264_enc_param_t *enc_param, vcenc_rate_ctrl_t *rc);

vcenc_ret_e vcenc_h264_get_rate_ctrl(h264_enc_param_t *enc_param, vcenc_rate_ctrl_t *rc);

vcenc_ret_e vcenc_h264_set_nr(h264_enc_param_t *enc_param, vcenc_nr_t *nr);

vcenc_ret_e vcenc_h264_update_slice_wr_cnt(h264_enc_param_t *enc_param, uint32_t slice_id);

uint32_t vcenc_h264_get_encoded_lines(void);

uint32_t vcenc_h264_get_current_qp(h264_enc_param_t *enc_param);

#ifdef __cplusplus
}
#endif
