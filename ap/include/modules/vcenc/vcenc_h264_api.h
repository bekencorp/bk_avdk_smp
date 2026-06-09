#pragma once

/**
 * @file vcenc_h264_api.h
 * @brief Public API for the H.264 encoder front-end.
 *
 * Lifecycle: vcenc_h264_init() -> vcenc_h264_open() -> vcenc_h264_encode_frame()*
 *           -> vcenc_h264_close() -> vcenc_h264_deinit().
 *
 * vcenc_h264_init() returns an opaque @ref vcenc_handle through an out-param;
 * every other API consumes that handle and a per-call configuration where
 * applicable. This mirrors the vcdec front-end so that handle ownership and
 * per-frame parameters are no longer mixed in a single struct.
 */

#include "modules/vcenc/vcenc_types.h"
#include "modules/vcenc/vcenc_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ENC_INSTANCE 2

/**
 * Allocate an instance and return its opaque handle through @p out_handle.
 *
 * @param out_handle  on success, receives the new encoder handle.
 * @param config      common (codec-agnostic) init configuration.
 * @param h264_config H.264-specific init configuration (geometry, GOP...).
 */
vcenc_ret_e vcenc_h264_init(vcenc_handle *out_handle,
			    const vcenc_config_t *config,
			    const vcenc_h264_config_t *h264_config);

/**
 * Register the per-instance buffer allocator hooks.
 *
 * Mirrors the vcdec design: caller must register a (malloc, free) pair after
 * vcenc_h264_init() but before vcenc_h264_open(); the large reference / coeff
 * / compress-table buffers are allocated lazily inside vcenc_h264_open() from
 * the registered allocator (typically the caller's frame-buffer slab).
 *
 * @param handle  encoder handle returned by vcenc_h264_init.
 * @param pmalloc memory allocation callback (non-NULL).
 * @param pfree   memory free callback (non-NULL).
 *
 * @return VCENC_OK on success; VCENC_NULL_ARGUMENT / VCENC_INSTANCE_ERROR
 *         if @p handle or callbacks are NULL.
 */
vcenc_ret_e vcenc_h264_memalloc_register(vcenc_handle handle,
					 void *(*pmalloc)(size_t),
					 void  (*pfree)(void *));

/**
 * Open the encoder session: arms IRQ, prepares per-instance semaphore, and
 * allocates ref / coeff / compress-table buffers via the allocator registered
 * with vcenc_h264_memalloc_register(). Must be called after the allocator is
 * registered.
 */
vcenc_ret_e vcenc_h264_open(vcenc_handle handle);

/** Encode one frame; blocks the caller on the per-instance encode semaphore. */
vcenc_ret_e vcenc_h264_encode_frame(vcenc_handle handle,
				    const vcenc_h264_frame_config_t *frame_config);

/** Close the encoder session: stops HW, masks IRQ, releases per-instance sem. */
vcenc_ret_e vcenc_h264_close(vcenc_handle handle);

/**
 * Abort an in-flight encode_frame: forces the engine to stop and wakes the
 * waiter with VCENC error. Equivalent to the legacy stop_encode entry point.
 */
vcenc_ret_e vcenc_h264_abort(vcenc_handle handle);

/** Tear down the instance and free its associated buffers. */
vcenc_ret_e vcenc_h264_deinit(vcenc_handle handle);

vcenc_ret_e vcenc_h264_set_osd(vcenc_handle handle, uint32_t index, void *buffer,
				uint32_t format, uint8_t alpha, uint32_t x, uint32_t y,
				uint32_t width, uint32_t height);

vcenc_ret_e vcenc_h264_set_mosaic(vcenc_handle handle, uint32_t index, uint32_t enable,
				   uint32_t x, uint32_t y, uint32_t width, uint32_t height);

vcenc_ret_e vcenc_h264_set_rate_ctrl(vcenc_handle handle, vcenc_rate_ctrl_t *rc);

vcenc_ret_e vcenc_h264_get_rate_ctrl(vcenc_handle handle, vcenc_rate_ctrl_t *rc);

vcenc_ret_e vcenc_h264_set_nr(vcenc_handle handle, vcenc_nr_t *nr);

vcenc_ret_e vcenc_h264_update_slice_wr_cnt(vcenc_handle handle, uint32_t slice_id);

uint32_t vcenc_h264_get_encoded_lines(void);

uint32_t vcenc_h264_get_current_qp(vcenc_handle handle);

#ifdef __cplusplus
}
#endif
