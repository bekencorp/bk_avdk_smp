#pragma once

/**
 * @file vcenc_jpeg_api.h
 * @brief Public API for the JPEG encoder front-end.
 *
 * Lifecycle: vcenc_jpeg_init() -> vcenc_jpeg_open() -> vcenc_jpeg_encode_frame()*
 *           -> vcenc_jpeg_close() -> vcenc_jpeg_deinit().
 *
 * vcenc_jpeg_init() returns an opaque @ref vcenc_handle through an out-param;
 * every other API consumes that handle and a per-call configuration where
 * applicable.
 */

#include "modules/vcenc/vcenc_types.h"
#include "modules/vcenc/vcenc_jpeg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate an instance and return its opaque handle through @p out_handle.
 *
 * @param out_handle on success, receives the new encoder handle.
 * @param config     common (codec-agnostic) init configuration.
 * @param jpeg_config JPEG-specific init configuration (geometry + flexa).
 */
vcenc_ret_e vcenc_jpeg_init(vcenc_handle *out_handle,
			    const vcenc_config_t *config,
			    const vcenc_jpeg_config_t *jpeg_config);

/**
 * Register the per-instance buffer allocator hooks.
 *
 * Mirrors the vcdec design: caller must register a (malloc, free) pair after
 * vcenc_jpeg_init() but before vcenc_jpeg_open(); coeff / refer buffers are
 * allocated lazily inside vcenc_jpeg_open() from the registered allocator
 * (typically the caller's frame-buffer slab).
 *
 * @param handle  encoder handle returned by vcenc_jpeg_init.
 * @param pmalloc memory allocation callback (non-NULL).
 * @param pfree   memory free callback (non-NULL).
 *
 * @return VCENC_OK on success; VCENC_NULL_ARGUMENT / VCENC_INSTANCE_ERROR
 *         if @p handle or callbacks are NULL.
 */
vcenc_ret_e vcenc_jpeg_memalloc_register(vcenc_handle handle,
					 void *(*pmalloc)(size_t),
					 void  (*pfree)(void *));

/**
 * Open the encoder session: arms IRQ, prepares per-instance semaphore, and
 * allocates coeff / refer buffers via the allocator registered with
 * vcenc_jpeg_memalloc_register(). Must be called after the allocator is
 * registered.
 */
vcenc_ret_e vcenc_jpeg_open(vcenc_handle handle);

/** Encode one frame; blocks the caller on the per-instance encode semaphore. */
vcenc_ret_e vcenc_jpeg_encode_frame(vcenc_handle handle,
				    const vcenc_jpeg_frame_config_t *frame_config);

/** Close the encoder session: stops HW, masks IRQ, releases per-instance sem. */
vcenc_ret_e vcenc_jpeg_close(vcenc_handle handle);

/**
 * Abort an in-flight encode_frame: forces the engine to stop and wakes the
 * waiter with VCENC_SW_ABORT-equivalent error path. May also be used as a
 * synchronous "stop encode" entry point.
 */
vcenc_ret_e vcenc_jpeg_abort(vcenc_handle handle);

/** Tear down the instance and free its associated buffers. */
vcenc_ret_e vcenc_jpeg_deinit(vcenc_handle handle);

/** Update the FLEXA input line-buffer write counter at runtime. */
vcenc_ret_e vcenc_jpeg_flexa_input_linebuf_wrcnt_set(vcenc_handle handle, uint32_t wrcnt);

/** Read the encoded slice count (rdcnt) from hardware (FLEXA). */
uint32_t vcenc_jpeg_get_encoded_lines(void);

#ifdef __cplusplus
}
#endif
