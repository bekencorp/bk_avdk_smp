// Copyright 2024-2025 Beken
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

#include <stdbool.h>
#include <stdint.h>

#include "components/bk_decode/bk_h264_decode_types.h"
#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*bk_video_player_h264_osd_acquire_cb_t)(bk_h264_decode_osd_t *osd,
                                                       void **token,
                                                       void *user_data);
typedef void (*bk_video_player_h264_osd_release_cb_t)(void *token,
                                                       bool frame_ready,
                                                       void *user_data);

typedef struct {
    bk_video_player_h264_osd_acquire_cb_t acquire;
    bk_video_player_h264_osd_release_cb_t release;
    void *user_data;
} bk_video_player_h264_osd_provider_t;

/**
 * @brief Get hardware H264 video decoder operations (Flexa + GPU path).
 *
 * The decoder is built around bk_h264_decode_flexa_ctlr (segment mode) and a
 * bk_gpu_ctlr bonded by Flexa. The H.264 IP writes 16-line NV12 macroblock
 * segments into a small HSRAM ring buffer. The GPU drains those segments,
 * (optionally rotates / scales), and writes a tile-compressed ARGB8888 frame
 * into HSRAM when possible, with PSRAM fallback if HSRAM is exhausted. The
 * compressed ARGB frame is what the video player engine delivers to the upper
 * layer for display through the DPU's DEC400 decompressor.
 *
 * Compared to the older frame-mode + full-NV12-in-PSRAM pipeline, this path:
 *  - never lands a 1080x1920 NV12 frame in PSRAM (only 1-3 MB-line segments
 *    in HSRAM), saving ~3 MB of PSRAM write bandwidth per frame
 *  - eliminates the CPU 1088->1080 stride compaction (~3 MB read + 3 MB write)
 *  - eliminates the CPU 90-degree rotation when needed (the GPU handles it)
 *  - hands the DPU a small compressed buffer (~30% of ARGB8888 size), so DPU
 *    scan-out traffic drops accordingly
 *
 * Input bitstream is expected in Annex-B format (start codes 00 00 00 01 or
 * 00 00 01). AVCC length-prefixed samples (typical for MP4) are converted to
 * Annex-B internally with SPS/PPS injection on the first frame after init
 * and after any decode failure.
 *
 * @return video_player_video_decoder_ops_t* Pointer to H264 decoder ops template,
 *         NULL on failure.
 */
video_player_video_decoder_ops_t *bk_video_player_get_hw_h264_decoder_ops(void);

/**
 * @brief Get hardware H264 frame decoder operations (legacy full-frame NV12 path).
 *
 * This decoder uses bk_h264_decode_frame_ctlr to decode a complete NV12 frame
 * into PSRAM. It is useful when the display pipeline is configured for raw
 * NV12 instead of the Flexa + GPU compressed ARGB output path.
 *
 * @return video_player_video_decoder_ops_t* Pointer to H264 frame decoder ops template,
 *         NULL on failure.
 */
video_player_video_decoder_ops_t *bk_video_player_get_hw_h264_decoder_frame_ops(void);

/**
 * @brief Get hardware H264 zero-copy / B-frame whole-frame decoder operations.
 *
 * This decoder is built around bk_h264_decode_frame_zerocopy_ctlr, which owns
 * an internal h264d_fbpool and decodes directly into pool buffers (no
 * reference-frame backup copies), supporting B-frame decoding with POC-based
 * display reordering. Like bk_video_player_get_hw_h264_decoder_frame_ops() it
 * produces full NV12 frames, so it is a drop-in alternative for the raw-NV12
 * display pipeline whenever the content uses B-frames. The original non-B
 * frame decoder (bk_video_player_get_hw_h264_decoder_frame_ops) is kept
 * unchanged.
 *
 * Input bitstream may be Annex-B or AVCC length-prefixed (converted internally
 * with SPS/PPS injection). The controller is flushed at each IDR and at deinit
 * so trailing reordered pictures are emitted in display order.
 *
 * @return video_player_video_decoder_ops_t* Pointer to the decoder ops template,
 *         NULL on failure.
 */
video_player_video_decoder_ops_t *bk_video_player_get_hw_h264_decoder_frame_zerocopy_ops(void);

/**
 * @brief Register the OSD source used by frame-mode H.264 decoder instances.
 *
 * The decoder acquires and pins one OSD buffer immediately before each
 * synchronous decode and releases it after the VCDEC PP has completed. Passing
 * NULL unregisters the current provider. Flexa and zero-copy decoders do not
 * use this provider.
 */
avdk_err_t bk_video_player_hw_h264_frame_decoder_set_osd_provider(
    const bk_video_player_h264_osd_provider_t *provider);

/**
 * @brief Free a GPU-produced H.264 output frame using the matching allocator.
 *
 * HSRAM frames are released with hsram_free()/os_free, while PSRAM fallback
 * frames are released with bk_frame_buffer_free().
 *
 * @param frame Pixel buffer pointer returned by the H.264 decoder.
 * @return AVDK_ERR_OK.
 */
avdk_err_t bk_video_player_hw_h264_decoder_free_output_frame(void *frame);
 
#ifdef __cplusplus
}
#endif
