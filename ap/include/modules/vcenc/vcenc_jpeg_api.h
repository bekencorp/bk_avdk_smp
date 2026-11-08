#pragma once

/**
 * @file vcenc_jpeg_api.h
 * @brief Public API for the JPEG encoder front-end.
 *
 * Lifecycle: vcenc_jpeg_init() -> vcenc_jpeg_open() -> vcenc_jpeg_encode_frame()*
 *           -> vcenc_jpeg_close() -> vcenc_jpeg_deinit().
 */

#include "modules/vcenc/vcenc_types.h"
#include "modules/vcenc/vcenc_jpeg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Register memory allocator hooks shared by all JPEG encoder instances. */
void vcenc_jpeg_memalloc_register(void *(*pmalloc)(size_t), void (*pfree)(void *));

/** Allocate an instance and bind it to the caller-owned @ref jpeg_enc_param_t. */
vcenc_ret_e vcenc_jpeg_init(jpeg_enc_param_t *enc_param);

/** Open the encoder session: arms IRQ, prepares per-instance semaphore. */
vcenc_ret_e vcenc_jpeg_open(jpeg_enc_param_t *enc_param);

/** Encode one frame; blocks the caller on the per-instance encode semaphore. */
vcenc_ret_e vcenc_jpeg_encode_frame(jpeg_enc_param_t *enc_param);

/** Close the encoder session: stops HW, masks IRQ, releases per-instance sem. */
vcenc_ret_e vcenc_jpeg_close(jpeg_enc_param_t *enc_param);

/**
 * Abort an in-flight encode_frame: forces the engine to stop and wakes the
 * waiter with VCENC_SW_ABORT-equivalent error path. May also be used as a
 * synchronous "stop encode" entry point.
 */
vcenc_ret_e vcenc_jpeg_abort(jpeg_enc_param_t *enc_param);

/** Tear down the instance and free its associated buffers. */
vcenc_ret_e vcenc_jpeg_deinit(jpeg_enc_param_t *enc_param);

/**
 * Configure one OSD overlay slot for JPEG encoding. The overlay is applied on
 * the next vcenc_jpeg_encode_frame() call. Pass @p buffer == NULL to clear the
 * slot. Format: 0=ARGB8888, 1=NV12, 2=bitmap (uses bitmap_y/u/v).
 */
vcenc_ret_e vcenc_jpeg_set_osd(jpeg_enc_param_t *enc_param, uint32_t index, void *buffer,
			       uint32_t format, uint8_t alpha, uint32_t x, uint32_t y,
			       uint32_t width, uint32_t height,
			       uint8_t bitmap_y, uint8_t bitmap_u, uint8_t bitmap_v);

/** Configure a mosaic overlay window (format=3) for JPEG encoding. */
vcenc_ret_e vcenc_jpeg_set_mosaic(jpeg_enc_param_t *enc_param, uint32_t index, uint32_t enable,
				  uint32_t x, uint32_t y, uint32_t width, uint32_t height);

/** Update the FLEXA input line-buffer write counter at runtime. */
vcenc_ret_e vcenc_jpeg_flexa_input_linebuf_wrcnt_set(jpeg_enc_param_t *enc_param,
						      uint32_t wrcnt);

/** Read the encoded slice count (rdcnt) from hardware (FLEXA). */
uint32_t vcenc_jpeg_get_encoded_lines(void);

/** Free codec-owned memory buffers allocated during init. Used by deinit. */
int vcenc_jpeg_memfree(jpeg_enc_param_t *param);

#ifdef __cplusplus
}
#endif
