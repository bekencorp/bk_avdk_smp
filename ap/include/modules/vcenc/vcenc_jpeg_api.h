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
