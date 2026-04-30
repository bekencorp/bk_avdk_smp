#pragma once

#include "vcdec_h264_types.h"

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
 * @brief     Register memory allocator for H.264 decoder
 *
 * This API registers malloc and free callbacks used by decoder internal
 * frame/reconstruction buffer allocation.
 *
 * @param handle decoder handle returned by vcdec_h264_init
 * @param pmalloc memory allocation callback
 * @param pfree memory free callback
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_h264_memalloc_register(vcdec_handle handle, void* (*pmalloc)(uint32_t), void (*pfree)(void*));

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
