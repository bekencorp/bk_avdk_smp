#pragma once

/**
 * @file vcdec_types.h
 * @brief Common public types shared by all vcdec codec front-ends (JPEG / H264 / ...).
 *
 * This header only contains types that are codec-agnostic:
 *   - return codes (vcdec_ret_e)
 *   - flexa working mode (vcdec_flexa_mode_e)
 *   - opaque decoder handle (vcdec_handle)
 *   - completion/event callbacks
 *   - decoder init configuration (vcdec_config_t) used by every codec's _init()
 *
 * Codec-specific public types live in dedicated headers:
 *   - vcdec_jpeg_types.h  (JPEG-only types, e.g. vcdec_jpeg_decode_config_t)
 *   - vcdec_h264_types.h  (H264-only types, e.g. vcdec_h264_decode_config_t)
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return / status codes used by every vcdec API.
 *
 * Non-negative values indicate success or a specific ready event reported
 * out-of-band, negative values indicate errors.
 */
typedef enum {
	VCDEC_OK               = 0,
	VCDEC_FRAME_READY      = (1 << 0),
	VCDEC_SLICE_READY      = (1 << 1),
	VCDEC_ERROR            = -1,
	VCDEC_NULL_ARGUMENT    = -2,
	VCDEC_INVALID_ARGUMENT = -3,
	VCDEC_MEMORY_ERROR     = -4,
	VCDEC_HW_TIMEOUT       = -5,
	VCDEC_HW_ERROR         = -6,
	VCDEC_HW_BUS_ERROR     = -7,
	VCDEC_SW_ABORT         = -8,
} vcdec_ret_e;

/**
 * @brief Flexa working mode for the post-processor.
 */
typedef enum {
	VCDEC_FLEXA_MODE_NONE = 0,
	VCDEC_FLEXA_MODE_FLEXA,
} vcdec_flexa_mode_e;

/** Opaque decoder instance handle returned by every codec's _init(). */
typedef void *vcdec_handle;

/** Frame-done callback. Invoked from the decoder ISR/worker context. */
typedef void (*vcdec_frame_done_cb)(int status, void *args);

/** Flexa ring-buffer line-update callback. Invoked from the PP ISR context. */
typedef void (*vcdec_flexa_done_cb)(uint32_t line_cnt, void *args);

/**
 * @brief Decoder init configuration, passed to every codec's _init().
 */
typedef struct vcdec_config_t {
	vcdec_flexa_mode_e  mode;
	uint32_t            timeout_ms;
	vcdec_frame_done_cb frame_done_cb;
	vcdec_flexa_done_cb flexa_done_cb;
	void               *args;
} vcdec_config_t;

/** PP output pixel format (shared by H.264 and JPEG decoders). */
typedef enum {
	VCDEC_PP_OUT_NV12 = 0,
	VCDEC_PP_OUT_RGB565,
	VCDEC_PP_OUT_RGB888,
} vcdec_pp_out_format_e;

#ifdef __cplusplus
}
#endif
