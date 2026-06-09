#pragma once

#include "vcdec_jpeg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize JPEG decoder: allocates instance.
 * before decode; they must be valid for decode (filled by parser or existing library).
 */
vcdec_ret_e vcdec_jpeg_init(vcdec_handle *handle_p, vcdec_config_t *config);

/**
 * Deinitialize JPEG decoder: frees instance.
 */
void vcdec_jpeg_deinit(vcdec_handle handle);

/**
 * Register the per-instance buffer allocator hooks used for large
 * frame / line-buffer allocations done by the JPEG decoder driver.
 *
 * Must be called after @ref vcdec_jpeg_init and before any @ref vcdec_jpeg_open
 * / @ref vcdec_jpeg_decode_frame. Mirrors @ref vcdec_h264_memalloc_register so
 * that both codec paths share a per-instance allocator contract (the encoder
 * side has the equivalent vcenc_jpeg_memalloc_register entry).
 *
 * @param handle  decoder handle returned by vcdec_jpeg_init
 * @param pmalloc memory allocation callback (non-NULL)
 * @param pfree   memory free callback (non-NULL)
 *
 * @return VCDEC_OK on success; VCDEC_INVALID_ARGUMENT if any argument is NULL.
 */
vcdec_ret_e vcdec_jpeg_memalloc_register(vcdec_handle handle,
					 void *(*pmalloc)(uint32_t),
					 void  (*pfree)(void *));

/**
 * Open JPEG decoder: allocates instance.
 * before decode; they must be valid for decode (filled by parser or existing library).
 */
vcdec_ret_e vcdec_jpeg_open(vcdec_handle handle);

/**
 * Decode one JPEG frame (full-frame mode). Stream and output buffers must be set.
 */
vcdec_ret_e vcdec_jpeg_decode_frame(vcdec_handle handle, vcdec_jpeg_decode_config_t *config);

/**
 * Close decoder and free instance.
 */
vcdec_ret_e vcdec_jpeg_close(vcdec_handle handle);

/**
 * Abort current decode: stop HW and release blocking decode. param must be the same as used for decode.
 */
vcdec_ret_e vcdec_jpeg_abort(vcdec_handle handle);

/**
 * Set the read pointer for the JPEG decoder.
 */
void vcdec_jpeg_set_rd_ptr(vcdec_handle handle_p, uint32_t rd_ptr);

/**
 * Reset the JPEG decoder.
 */
void vcdec_jpeg_reset(vcdec_handle handle_p);

#ifdef __cplusplus
}
#endif
