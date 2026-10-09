// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_h264_encode_types.h"

/**
 * @brief Create a new hardware flexa encoder
 * @param handle Encoder handle
 * @param config Hardware flexa configuration
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_hw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);

/**
 * @brief Create a new software flexa encoder
 * @param handle Encoder handle
 * @param config Software flexa configuration
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_sw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);

/**
 * @brief Create a new frame encoder
 * @param handle Encoder handle
 * @param config Frame configuration
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_frame_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);

/**
 * @brief Initialize encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_init(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Deinitialize encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_deinit(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Open encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_open(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Close encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_close(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Execute encoding
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_start(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Control interface
 * @param handle Encoder handle
 * @param cmd Command
 * @param arg Argument
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_ioctl(bk_h264_encode_ctlr_handle_t handle, bk_h264_encode_ioctl_cmd_t cmd, void *arg);
/**
 * @brief Force IDR frame generation
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_force_idr(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Configure GOP frame count
 * @param handle Encoder handle
 * @param gop_frame_count Number of frames in one GOP
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_gop_frame_count(bk_h264_encode_ctlr_handle_t handle,
                                              uint32_t gop_frame_count);

/**
 * @brief Get GOP frame count
 * @param handle Encoder handle
 * @param gop_frame_count Output GOP frame count
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_get_gop_frame_count(bk_h264_encode_ctlr_handle_t handle,
                                              uint32_t *gop_frame_count);

/**
 * @brief Configure rate control
 * @param handle Encoder handle
 * @param rate_ctrl Rate control config
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                         bk_h264_encode_rate_ctrl_t *rate_ctrl);

/**
 * @brief Get rate control config
 * @param handle Encoder handle
 * @param rate_ctrl Output rate control config
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_get_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                         bk_h264_encode_rate_ctrl_t *rate_ctrl);

/**
 * @brief Configure hardware OSD overlay with externally owned buffer
 *
 * The caller allocates and fills @p osd->buffer, then passes ownership to the
 * encoder via @p osd->buffer_free. After a successful call the caller must not
 * modify the buffer. When a slot receives a new buffer pointer, the previous
 * buffer is released through its registered free callback. All active OSD
 * buffers are released automatically when the encoder is closed.
 *
 * @param handle Encoder handle
 * @param osd OSD overlay config (buffer_free required when buffer != NULL)
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_osd(bk_h264_encode_ctlr_handle_t handle,
                                  bk_h264_encode_osd_t *osd);

/**
 * @brief Draw hollow rectangles as a mono-color BITMAP OSD overlay
 *
 * Internally allocates one BITMAP (1bpp) covering the union of all boxes,
 * rasterizes borders, and installs it on @p boxes->index. Subsequent encode
 * frames overlay automatically.
 *
 * Color limitation: BITMAP OSD supports only one foreground color per call.
 * The color is taken from boxes->boxes[0].color (RGB888 → YUV); additional
 * per-box colors are ignored. Pass count==0 / boxes==NULL to clear the slot.
 *
 * Buffer size (PSRAM / UNCODED heap, owned by encoder until replaced/cleared):
 *   canvas = axis-aligned union of all boxes, then
 *     x/y rounded down to 2 px, width/height rounded up to 8 px;
 *   size   = (canvas_w / 8) * canvas_h  bytes  (1 bit/pixel).
 * Example: one 100x80 box → canvas 104x80 → 1040 bytes.
 *
 * Geometry tip (avoid encode mosaic): keep each box's start (x,y), size
 * (width,height), and border thickness as multiples of 16. Otherwise,
 * the encoder may introduce mosaic artifacts around the lines.
 *
 * @param handle Encoder handle
 * @param boxes BITMAP rectangle batch descriptor
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_osd_bitmap_boxes(bk_h264_encode_ctlr_handle_t handle,
                                               bk_h264_encode_osd_bitmap_boxes_t *boxes);

/**
 * @brief Draw hollow rectangles as a multi-color ARGB8888 OSD overlay
 *
 * Internally allocates one ARGB8888 buffer covering the union of all boxes,
 * rasterizes borders with per-box ARGB color (alpha blend enabled by HW), and
 * installs it on @p boxes->index. Non-border pixels are transparent.
 * Subsequent encode frames overlay automatically.
 *
 * Each box may use a different color. Color format is 0xAARRGGBB; if A==0 the
 * implementation forces opaque 0xFF. Pass count==0 / boxes==NULL to clear.
 *
 * Buffer size (PSRAM / UNCODED heap, owned by encoder until replaced/cleared):
 *   canvas = axis-aligned union of all boxes, then
 *     x/y/width/height all rounded to 2 px;
 *   size   = canvas_w * canvas_h * 4  bytes  (32 bit/pixel).
 * Example: one 100x80 box → canvas 100x80 → 32000 bytes.
 *
 * Size note: ARGB8888 needs 32x the memory of BITMAP for the same canvas
 * (32 bpp vs 1 bpp). Prefer bk_h264_encode_set_osd_bitmap_boxes() when a
 * single color is enough.
 *
 * Geometry tip (avoid encode mosaic): keep each box's start (x,y), size
 * (width,height), and border thickness as multiples of 16. Otherwise,
 * the encoder may introduce mosaic artifacts around the lines.
 *
 * @param handle Encoder handle
 * @param boxes ARGB8888 rectangle batch descriptor
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_osd_argb8888_boxes(bk_h264_encode_ctlr_handle_t handle,
                                                 bk_h264_encode_osd_argb8888_boxes_t *boxes);

/**
 * @brief Delete encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_delete(bk_h264_encode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
