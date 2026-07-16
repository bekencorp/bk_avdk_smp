#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "components/bk_video_player/bk_video_player_types.h"
#include "video_player_common.h"

// Forward declaration
typedef void *audio_player_device_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

// Runtime context passed via bk_video_player_config_t.user_data for CLI playback.
// This avoids using cross-file globals.
typedef struct video_play_user_ctx_s
{
    bk_display_ctlr_handle_t lcd_handle;
    audio_player_device_handle_t audio_player_handle;
    // Cached audio control states (requested by player) for audio output recreation scenarios.
    uint8_t audio_volume; // 0-100
    bool audio_muted;
} video_play_user_ctx_t;

typedef enum
{
    VIDEO_PLAY_ROTATE_NONE = 0,
    VIDEO_PLAY_ROTATE_90,
    VIDEO_PLAY_ROTATE_270,
} video_play_rotate_mode_t;

// ====== Buffer callbacks ======
avdk_err_t video_play_audio_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_audio_buffer_free_cb(void *user_data, video_player_buffer_t *buffer);

avdk_err_t video_play_video_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_video_buffer_free_cb(void *user_data, video_player_buffer_t *buffer);

avdk_err_t video_play_video_buffer_alloc_yuv_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_video_buffer_free_yuv_cb(void *user_data, video_player_buffer_t *buffer);

/* NV12 output buffers placed in the CODED slab (PSRAM1) instead of UNCODED
 * (PSRAM0). Used by the frame-zerocopy H.264 decoder so the large zero-copy
 * decode pool can keep PSRAM0 to itself while the displayable NV12 frames live
 * on PSRAM1. Both slabs are non-cacheable, so DPU display stays coherent. */
avdk_err_t video_play_video_buffer_alloc_yuv_coded_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_video_buffer_free_yuv_coded_cb(void *user_data, video_player_buffer_t *buffer);

// ====== Decode complete callbacks ======
void video_play_video_decode_complete_cb(void *user_data, const video_player_video_frame_meta_t *meta, video_player_buffer_t *buffer);
void video_play_audio_decode_complete_cb(void *user_data, const video_player_audio_packet_meta_t *meta, video_player_buffer_t *buffer);

// ====== Display worker lifecycle ======
// The video decode-complete callback offloads GPU rotate / LCD format sync /
// display flush to a dedicated worker thread instead of running them inline on
// the decode thread. Create the worker before playback starts, and tear it down
// after the engine (and thus the decode thread) has been stopped.
avdk_err_t video_play_display_worker_init(void);
void video_play_display_worker_deinit(void);

void video_play_lcd_runtime_format_reset(void);
void video_play_lcd_runtime_format_mark(video_play_lcd_video_fmt_t fmt);
void video_play_video_set_rotate_mode(video_play_rotate_mode_t mode);
video_play_rotate_mode_t video_play_video_get_rotate_mode(void);
uint32_t video_play_video_get_rotate_degree(void);

#ifdef __cplusplus
}
#endif


