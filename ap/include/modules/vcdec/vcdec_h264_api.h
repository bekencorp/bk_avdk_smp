#pragma once

#include "vcdec_h264_types.h"
#include "vcdec_fb_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Get current H.264 frame information
 *
 * This API gets parsed frame information from the current decoder instance.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 * @param info output info buffer used to save H.264 frame information
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_get_info(vcdec_handle handle, vcdec_h264_info_t *info);

/**
 * @brief     Set flexa read pointer
 *
 * This API updates post-processor read pointer in flexa mode.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 * @param rd_ptr read pointer offset passed to post-processor
 *
 * @return none
 */
void vcdec_h264_set_rd_ptr(vcdec_handle handle, uint32_t rd_ptr);

/**
 * @brief     Reset H.264 decoder instance
 *
 * This API resets internal decoder state and hardware context.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 *
 * @return none
 */
void vcdec_h264_reset(vcdec_handle handle);

/**
 * @brief     Initialize H.264 decoder
 *
 * This API creates an H.264 decoder instance and initializes decoder context.
 *
 * @param handle output decoder handle
 * @param config decoder configuration, including mode/callback/timeout
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_init(vcdec_handle *handle, vcdec_config_t *config);

/**
 * @brief     Deinitialize H.264 decoder
 *
 * This API releases decoder instance and internal resources.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 *
 * @return none
 */
void vcdec_h264_deinit(vcdec_handle handle);

/**
 * @brief     Open H.264 decoder
 *
 * This API prepares decoder hardware and runtime resources before decode.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_open(vcdec_handle handle);

/**
 * @brief     Decode one H.264 access unit
 *
 * This API decodes one complete H.264 frame/access unit with input and output
 * buffers provided by user.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 * @param config decode configuration including input/output buffers
 *
 * @return VCDEC_OK or ready status for success, others for failure
 */
vcdec_ret_e vcdec_h264_decode_frame(vcdec_handle handle, vcdec_h264_decode_config_t *config);

/**
 * @brief     Register a zero-copy frame pool (frame mode only)
 *
 * Injects the external frame-pool vtable into the decoder. After registration
 * the decoder (in non-FLEXA frame mode) acquires its decode/reference/display
 * buffers from the pool, eliminating the reference backup copy and enabling
 * B-frame decoding with POC-based display reordering. Must be called after
 * vcdec_h264_open() and before vcdec_h264_decode_frame(). FLEXA mode is
 * unaffected.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 * @param ifc    decode-side frame-pool interface (from h264d_fbpool_get_if)
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_register_fb_if(vcdec_handle handle, const vcdec_fb_if_t *ifc);

/**
 * @brief     Flush trailing pictures at end of stream (pool mode)
 *
 * Emits every picture still held in the DPB in display (POC) order so the
 * application can dequeue the final reordered frames. No-op without a pool.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_flush(vcdec_handle handle);

/**
 * @brief     Close H.264 decoder
 *
 * This API closes current decoder instance and releases opened runtime
 * resources.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_close(vcdec_handle handle);

/**
 * @brief     Abort current H.264 decode
 *
 * This API stops current decode task and releases blocking decode flow.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_abort(vcdec_handle handle);

#ifdef __cplusplus
}
#endif
