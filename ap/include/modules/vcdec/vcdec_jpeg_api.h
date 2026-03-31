#pragma once

#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

vcdec_ret_e vcdec_jpeg_get_info(vcdec_handle handle_p, vcdec_info_t *info);
void vcdec_jpeg_set_rd_ptr(vcdec_handle handle_p, uint32_t rd_ptr);
void vcdec_jpeg_reset(vcdec_handle handle_p);

vcdec_ret_e vcdec_jpeg_init(vcdec_handle *handle_p, vcdec_config_t *config);

void vcdec_jpeg_deinit(vcdec_handle handle);
/**
 * Open JPEG decoder: allocates instance.
 * before decode; they must be valid for decode (filled by parser or existing library).
 */
vcdec_ret_e vcdec_jpeg_open(vcdec_handle handle);

/**
 * Decode one JPEG frame (full-frame mode). Stream and output buffers must be set.
 */
vcdec_ret_e vcdec_jpeg_decode_frame(vcdec_handle handle, vcdec_decode_config_t *config);

/**
 * Close decoder and free instance.
 */
vcdec_ret_e vcdec_jpeg_close(vcdec_handle handle);

/**
 * Abort current decode: stop HW and release blocking decode. param must be the same as used for decode.
 */
vcdec_ret_e vcdec_jpeg_abort(vcdec_handle handle);

#ifdef __cplusplus
}
#endif
