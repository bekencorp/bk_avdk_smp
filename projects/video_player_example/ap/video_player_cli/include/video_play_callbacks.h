#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <components/bk_display_types.h>

#include "components/bk_video_player/bk_video_player_types.h"

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

// ====== Buffer callbacks ======
avdk_err_t video_play_audio_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_audio_buffer_free_cb(void *user_data, video_player_buffer_t *buffer);

avdk_err_t video_play_video_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_video_buffer_free_cb(void *user_data, video_player_buffer_t *buffer);

avdk_err_t video_play_video_buffer_alloc_yuv_cb(void *user_data, video_player_buffer_t *buffer);
void video_play_video_buffer_free_yuv_cb(void *user_data, video_player_buffer_t *buffer);

// ====== Decode complete callbacks ======
void video_play_video_decode_complete_cb(void *user_data, const video_player_video_frame_meta_t *meta, video_player_buffer_t *buffer);
void video_play_audio_decode_complete_cb(void *user_data, const video_player_audio_packet_meta_t *meta, video_player_buffer_t *buffer);

#ifdef __cplusplus
}
#endif


