#pragma once

/**
 * @file vcenc_h264_types.h
 * @brief H.264-encoder-specific public types.
 *
 * Holds the H.264-only init configuration (geometry / GOP) and the per-frame
 * encode configuration (input/output buffer pointers). The opaque session
 * handle (@ref vcenc_handle) is produced by vcenc_h264_init() and consumed by
 * every other API entry. Profile / level enums and SYNTAX_PROFILE_H264_* are
 * module-internal and live in vcenc_h264_types_private.h; legacy users of
 * those names go through the standalone h264e library's hevcencapi.h which
 * ships its own independent copy of those identifiers. Common encoder types
 * (return codes, mode/input enums, callbacks, RC, NR, vcenc_config_t) live in
 * vcenc_types.h.
 */

#include <stdint.h>
#include "vcenc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief H.264 init configuration.
 *
 * Codec-specific one-shot configuration handed to vcenc_h264_init() together
 * with the common @ref vcenc_config_t. The geometry given here is treated as
 * the maximum picture size and is used to size reference / coeff / compress
 * buffers; subsequent encode_frame calls may pick a smaller resolution via
 * @ref vcenc_h264_frame_config_t.
 */
typedef struct vcenc_h264_config_t {
	uint16_t      width;
	uint16_t      height;
	vcenc_input_e in_type;
	uint32_t      idr_interval;
	uint32_t      slice_count;   /* only meaningful in hw/sw slice mode */
} vcenc_h264_config_t;

/**
 * @brief Per-frame H.264 encode configuration.
 *
 * Filled by the caller for every vcenc_h264_encode_frame() call to describe
 * the input picture buffer, the output bitstream buffer and any per-frame
 * GOP/RC overrides.
 */
typedef struct vcenc_h264_frame_config_t {
	uint16_t width;
	uint16_t height;
	uint32_t in_buffer;
	uint32_t in_lines;
	uint32_t out_buffer;
	uint32_t out_len;
	uint32_t update_flag;
	uint32_t force_idr_flag;
	uint32_t idr_interval;       /* 0 = inherit value from init */
} vcenc_h264_frame_config_t;

#ifdef __cplusplus
}
#endif
