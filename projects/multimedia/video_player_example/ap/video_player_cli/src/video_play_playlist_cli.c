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

#include <components/avdk_utils/avdk_error.h>
#include <components/bk_display.h>
#include "audio_player_device.h"
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>

#include "cli.h"
#include "video_player_cli.h"
#include "video_player_common.h"
#include "video_play_callbacks.h"

#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/container_parser/bk_video_player_avi_parser.h"
#include "components/bk_video_player/container_parser/bk_video_player_mp4_parser.h"
#include "components/bk_video_player/video_decoder/bk_video_player_hw_jpeg_decoder.h"
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#include "components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h"
#endif
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER
#include "components/bk_video_player/audio_decoder/bk_video_player_aac_decoder.h"
#endif
#include "modules/mp4lib.h"
#include "modules/avilib.h"
#include "bk_partition.h"
#include <sys/stat.h>
#include <limits.h>

#define TAG "video_play_playlist_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

// Speaker digital gain
#define VIDEO_PLAY_SPK_DIG_GAIN_DEFAULT   (0x3f) // +17dB
#define VIDEO_PLAY_SPK_DIG_GAIN_MAX       (0x3F)

// Keep CLI A/V sync offset consistent with core layer validation.
#define VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX  (5000)

// Global variables for Playlist layer player
static bk_video_player_playlist_handle_t s_video_player_app_handle = NULL;
static bool s_video_player_app_started = false;
static bool s_video_player_app_opened = false;
static bk_display_ctlr_handle_t s_lcd_display_handle = NULL;
static audio_player_device_handle_t s_audio_player_handle = NULL;

// Playback runtime context passed to callbacks via cfg.user_data.
static video_play_user_ctx_t s_play_user_ctx = {0};

static avdk_err_t video_play_playlist_prepare_lcd_for_file(const char *file_path)
{
    video_play_lcd_video_fmt_t fmt = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#if VIDEO_PLAY_H264_FLEXA_RAW_ARGB8888_ENABLE
    fmt = VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_RAW;
#else
    fmt = VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED;
#endif
#endif

    if (file_path != NULL && s_video_player_app_handle != NULL)
    {
        video_player_media_info_t media_info;
        os_memset(&media_info, 0, sizeof(media_info));
        if (bk_video_player_playlist_get_media_info(s_video_player_app_handle, file_path, &media_info) == AVDK_ERR_OK)
        {
            fmt = video_play_lcd_format_for_video_codec(media_info.video.format);
        }
    }

    avdk_err_t ret = video_play_lcd_ensure_open(&s_lcd_display_handle, fmt);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: video_play_lcd_ensure_open failed, ret=%d\n", __func__, ret);
        return ret;
    }

    s_play_user_ctx.lcd_handle = s_lcd_display_handle;
    video_play_lcd_runtime_format_mark(fmt);
    return AVDK_ERR_OK;
}

void bk_video_player_playlist_register_avi_container_parser(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    video_player_container_parser_ops_t *ops = bk_video_player_get_avi_parser_ops();
    if (ops == NULL)
    {
        LOGE("%s: get_avi_parser_ops failed\n", __func__);
        return;
    }

    avdk_err_t ret = bk_video_player_playlist_register_container_parser((bk_video_player_playlist_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_avi_container_parser failed, ret=%d\n", __func__, ret);
    }
}

void bk_video_player_playlist_register_mp4_container_parser(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    video_player_container_parser_ops_t *ops = bk_video_player_get_mp4_parser_ops();
    if (ops == NULL)
    {
        LOGE("%s: get_mp4_parser_ops failed\n", __func__);
        return;
    }

    avdk_err_t ret = bk_video_player_playlist_register_container_parser((bk_video_player_playlist_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_mp4_container_parser failed, ret=%d\n", __func__, ret);
    }
}

void bk_video_player_playlist_register_hw_jpeg_video_decoder(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    video_player_video_decoder_ops_t *ops = bk_video_player_get_hw_jpeg_decoder_ops();
    if (ops == NULL)
    {
        LOGE("%s: get_hw_jpeg_decoder_ops failed\n", __func__);
        return;
    }

    avdk_err_t ret = bk_video_player_playlist_register_video_decoder((bk_video_player_playlist_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_hw_jpeg_video_decoder failed, ret=%d\n", __func__, ret);
    }
}

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
void bk_video_player_playlist_register_hw_h264_video_decoder(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    video_player_video_decoder_ops_t *ops = bk_video_player_get_hw_h264_decoder_ops();
    if (ops == NULL)
    {
        LOGE("%s: get_hw_h264_decoder_ops failed\n", __func__);
        return;
    }

    avdk_err_t ret = bk_video_player_playlist_register_video_decoder((bk_video_player_playlist_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_hw_h264_video_decoder failed, ret=%d\n", __func__, ret);
    }
}
#endif

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER
void bk_video_player_playlist_register_aac_audio_decoder(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    const video_player_audio_decoder_ops_t *ops = bk_video_player_get_aac_decoder_ops();
    if (ops == NULL)
    {
        LOGE("%s: get_aac_decoder_ops failed\n", __func__);
        return;
    }

    avdk_err_t ret = bk_video_player_playlist_register_audio_decoder((bk_video_player_playlist_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_aac_audio_decoder failed, ret=%d\n", __func__, ret);
    }
}
#endif

static bool video_play_parse_int32_dec_range(const char *s, int32_t min_v, int32_t max_v, int32_t *out_v)
{
    if (out_v == NULL)
    {
        return false;
    }
    *out_v = 0;

    if (s == NULL || s[0] == '\0')
    {
        return false;
    }

    // Parse optional sign.
    bool negative = false;
    uint32_t i = 0;
    if (s[0] == '+')
    {
        i = 1;
    }
    else if (s[0] == '-')
    {
        negative = true;
        i = 1;
    }

    if (s[i] == '\0')
    {
        return false;
    }

    // Parse base-10 digits with overflow check.
    int64_t acc = 0;
    for (; s[i] != '\0'; i++)
    {
        char c = s[i];
        if (c < '0' || c > '9')
        {
            return false;
        }

        int32_t digit = (int32_t)(c - '0');
        acc = acc * 10 + (int64_t)digit;

        // Early overflow guard for int32.
        if (!negative && acc > (int64_t)INT32_MAX)
        {
            return false;
        }
        if (negative && -acc < (int64_t)INT32_MIN)
        {
            return false;
        }
    }

    int64_t v = negative ? -acc : acc;
    if (v < (int64_t)min_v || v > (int64_t)max_v)
    {
        return false;
    }

    *out_v = (int32_t)v;
    return true;
}

static void video_play_playlist_release_player_handle(void)
{
    if (s_video_player_app_handle == NULL)
    {
        return;
    }

    if (s_video_player_app_opened)
    {
        (void)bk_video_player_playlist_close(s_video_player_app_handle);
    }

    (void)bk_video_player_playlist_delete(s_video_player_app_handle);
    s_video_player_app_handle = NULL;
    s_video_player_app_opened = false;
    s_video_player_app_started = false;
}

static void video_play_playlist_release_media_outputs(void)
{
    if (s_audio_player_handle != NULL)
    {
        audio_player_device_stop(s_audio_player_handle);
        audio_player_device_deinit(s_audio_player_handle);
        s_audio_player_handle = NULL;
        s_play_user_ctx.audio_player_handle = NULL;
    }

    if (s_lcd_display_handle != NULL)
    {
        (void)video_play_lcd_close();
        s_lcd_display_handle = NULL;
        s_play_user_ctx.lcd_handle = NULL;
        video_play_lcd_runtime_format_reset();
    }
}

void video_play_playlist_runtime_shutdown(void)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_video_player_app_handle != NULL && s_video_player_app_opened)
    {
        ret = bk_video_player_playlist_stop(s_video_player_app_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s: playlist stop failed, ret=%d\n", __func__, ret);
        }
    }

    video_play_playlist_release_player_handle();
    video_play_playlist_release_media_outputs();

}

static void video_play_app_reset_audio_output(void)
{
    /*
     * When switching playlist items rapidly (next/prev/play), stale audio may remain buffered in
     * audio player device internal buffers. This causes perceived audio lag behind video.
     * Stop and restart audio player device to drop pending old audio and keep A/V aligned.
     */
    if (s_audio_player_handle != NULL)
    {
        avdk_err_t ret = audio_player_device_stop(s_audio_player_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s: audio_player_device_stop failed, ret=%d\n", __func__, ret);
        }

        ret = audio_player_device_start(s_audio_player_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s: audio_player_device_start failed, ret=%d\n", __func__, ret);
        }
    }
}

static avdk_err_t video_play_app_audio_output_reset_cb(void *user_data)
{
    (void)user_data;
    video_play_app_reset_audio_output();
    return AVDK_ERR_OK;
}

static avdk_err_t video_play_app_audio_set_volume_cb(void *user_data, uint8_t volume)
{
    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    if (ctx == NULL)
    {
        LOGE("%s: user_data is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Cache the requested state even if audio output is not opened yet.
    ctx->audio_volume = volume;

    if (ctx->audio_player_handle == NULL)
    {
        // Audio output may not be initialized yet (probe media info first).
        // Treat it as best-effort and return OK to avoid failing playlist_open().
        return AVDK_ERR_OK;
    }

    avdk_err_t ret = audio_player_device_set_volume(ctx->audio_player_handle, volume);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_player_device_set_volume failed, ret=%d\n", __func__, ret);
        return ret;
    }

    LOGI("%s: applied volume=%u\n", __func__, (unsigned)volume);
    return AVDK_ERR_OK;
}

static avdk_err_t video_play_app_audio_set_mute_cb(void *user_data, bool mute)
{
    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    if (ctx == NULL)
    {
        LOGE("%s: user_data is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Cache the requested state even if audio output is not opened yet.
    ctx->audio_muted = mute;

    if (ctx->audio_player_handle == NULL)
    {
        // Audio output may not be initialized yet (probe media info first).
        // Treat it as best-effort and return OK to avoid failing playlist_open().
        return AVDK_ERR_OK;
    }

    avdk_err_t ret = audio_player_device_set_mute(ctx->audio_player_handle, mute);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_player_device_set_mute failed, ret=%d\n", __func__, ret);
        return ret;
    }

    LOGI("%s: mute=%u\n", __func__, (unsigned)mute);
    return AVDK_ERR_OK;
}

static avdk_err_t video_play_app_audio_output_config_cb(void *user_data, const video_player_audio_params_t *params)
{
    if (params == NULL)
    {
        LOGE("%s: params is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    if (ctx == NULL)
    {
        LOGE("%s: user_data is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (params->channels == 0 || params->channels > 2 || params->sample_rate == 0)
    {
        LOGE("%s: invalid audio params: ch=%u rate=%u\n",
             __func__, (unsigned)params->channels, (unsigned)params->sample_rate);
        return AVDK_ERR_INVAL;
    }

    uint32_t bits = (params->bits_per_sample > 0) ? params->bits_per_sample : 16;
    if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
    {
        LOGE("%s: invalid bits_per_sample: %u (must be 8/16/24/32)\n", __func__, (unsigned)bits);
        return AVDK_ERR_INVAL;
    }

    /*
     * Root fix:
     * - When "start" is called without a file_path, audio output is not pre-opened.
     * - The first "play" relies on audio_output_config_cb to create the audio device.
     * Therefore this callback must support BOTH:
     * - first-time creation (init -> start)
     * - reconfiguration on file switching (deinit -> init -> start)
     */
    audio_player_device_handle_t old_handle = ctx->audio_player_handle;
    if (old_handle != NULL)
    {
        avdk_err_t ret = audio_player_device_deinit(old_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: audio_player_device_deinit failed, ret=%d\n", __func__, ret);
            return ret;
        }

        ctx->audio_player_handle = NULL;
        if (s_audio_player_handle == old_handle)
        {
            s_audio_player_handle = NULL;
        }
    }

    // Calculate PCM frame size for buffering (20ms of audio).
    // Use ceil to avoid zero frame size on low sample rates.
    uint32_t bytes_per_sample = bits / 8;
    if (bytes_per_sample == 0)
    {
        bytes_per_sample = 2;
    }
    uint32_t samples_20ms = (params->sample_rate * 20 + 999) / 1000;
    uint32_t frame_size = samples_20ms * bytes_per_sample * params->channels;

    // AAC decoded PCM packets are often larger; use a safe minimum to avoid underrun.
    if (params->format == VIDEO_PLAYER_AUDIO_FORMAT_AAC && frame_size < 2048)
    {
        frame_size = 2048;
    }

    audio_player_device_cfg_t audio_cfg = {0};
    audio_cfg.channels = params->channels;
    audio_cfg.sample_rate = params->sample_rate;
    audio_cfg.bits_per_sample = bits;
    audio_cfg.format = params->format;
    audio_cfg.frame_size = frame_size;

    audio_player_device_handle_t new_handle = NULL;
    avdk_err_t ret = audio_player_device_init(&audio_cfg, &new_handle);
    if (ret != AVDK_ERR_OK || new_handle == NULL)
    {
        LOGE("%s: audio_player_device_init failed, ret=%d\n", __func__, ret);
        return (ret == AVDK_ERR_OK) ? AVDK_ERR_HWERROR : ret;
    }

    ret = audio_player_device_start(new_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_player_device_start failed, ret=%d\n", __func__, ret);
        audio_player_device_deinit(new_handle);
        return ret;
    }

    // Restore cached audio control states after recreating audio output.
    ret = audio_player_device_set_volume(new_handle, ctx->audio_volume);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_player_device_set_volume failed, ret=%d\n", __func__, ret);
        audio_player_device_deinit(new_handle);
        return ret;
    }

    ret = audio_player_device_set_mute(new_handle, ctx->audio_muted);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_player_device_set_mute failed, ret=%d\n", __func__, ret);
        audio_player_device_deinit(new_handle);
        return ret;
    }

    ctx->audio_player_handle = new_handle;
    s_audio_player_handle = new_handle;

    return AVDK_ERR_OK;
}

// CLI command: video_play_playlist start [file_path] / stop / pause / resume / add [file_path] / remove [file_path] /
//              next / prev / play [index|file] / clear / list / status / seek [time_ms] / volume [0-100] /
//              vol_up [step] / vol_down [step] / mute [on|off] / avsync [offset_ms] / ff [time_ms] / rewind [time_ms] / info [file_path] /
//              play_mode <stop|repeat|loop>
void cli_video_play_playlist_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    (void)xWriteBufferLen;
    avdk_err_t ret = AVDK_ERR_OK;

    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        const char *file_path = (argc >= 3) ? argv[2] : NULL;

        // Allow re-start if playlist is still open (same model as video_play_engine).
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            if (file_path != NULL)
            {
                ret = video_play_playlist_prepare_lcd_for_file(file_path);
                if (ret != AVDK_ERR_OK)
                {
                    goto exit;
                }

                ret = bk_video_player_playlist_play_file(s_video_player_app_handle, file_path);
                if (ret != AVDK_ERR_OK)
                {
                    LOGE("%s: bk_video_player_playlist_play_file failed, ret=%d\n", __func__, ret);
                    goto exit;
                }

                s_video_player_app_started = true;
                LOGI("%s: Video playback restarted (Playlist layer), file: %s\n", __func__, file_path);
                msg = CLI_CMD_RSP_SUCCEED;
                goto exit;
            }

            LOGW("%s: video player playlist already started\n", __func__);
            msg = CLI_CMD_RSP_SUCCEED;
            goto exit;
        }

        // Mount SD card if not mounted
        if (!sd_card_is_mounted())
        {
            int sd_ret = sd_card_mount();
            if (sd_ret != BK_OK)
            {
                LOGE("%s: sd_card_mount failed, ret=%d\n", __func__, sd_ret);
                goto exit;
            }
        }

        // Initialize video player playlist configuration
        bk_video_player_config_t cfg;
        os_memset(&cfg, 0, sizeof(cfg));

        cfg.video.parser_to_decode_buffer_count = 2;
        cfg.video.decode_to_output_buffer_count = 2;
        cfg.audio.parser_to_decode_buffer_count = 2;
        cfg.audio.decode_to_output_buffer_count = 2;

        cfg.audio.buffer_alloc_cb = video_play_audio_buffer_alloc_cb;
        cfg.audio.buffer_free_cb = video_play_audio_buffer_free_cb;
        cfg.video.buffer_alloc_cb = video_play_video_buffer_alloc_yuv_cb;
        cfg.video.buffer_free_cb = video_play_video_buffer_free_yuv_cb;

        cfg.video.packet_buffer_alloc_cb = video_play_video_buffer_alloc_cb;
        cfg.video.packet_buffer_free_cb = video_play_video_buffer_free_cb;

        cfg.audio.decode_complete_cb = video_play_audio_decode_complete_cb;
        cfg.video.decode_complete_cb = video_play_video_decode_complete_cb;

        /* MJPEG/JPEG uses NV12 into the DPU; H264 flexa GPU emits compressed
         * ARGB8888 and needs the matching DPU runtime (see
         * video_play_playlist_prepare_lcd_for_file()). */
        cfg.video.output_format = PIXEL_FMT_NV12;
        cfg.video.rotate_degree = video_play_video_get_rotate_degree();
        s_play_user_ctx.lcd_handle = NULL;
        // Audio output may be opened later after probing media info.
        s_play_user_ctx.audio_player_handle = NULL;
        s_play_user_ctx.audio_volume = 100;
        s_play_user_ctx.audio_muted = false;
        cfg.user_data = &s_play_user_ctx;

        // Volume/mute control callbacks must be provided at init time (no dynamic registration).
        cfg.audio.audio_set_volume_cb = video_play_app_audio_set_volume_cb;
        cfg.audio.audio_set_mute_cb = video_play_app_audio_set_mute_cb;
        cfg.audio.audio_ctrl_user_data = NULL; // Use cfg.user_data

        // Track switching audio reset callback (next/prev/play).
        cfg.audio.audio_output_reset_cb = video_play_app_audio_output_reset_cb;
        cfg.audio.audio_output_reset_user_data = NULL; // Use cfg.user_data

        // Audio output parameter (re)configuration callback (sample rate/bits/channels).
        cfg.audio.audio_output_config_cb = video_play_app_audio_output_config_cb;
        cfg.audio.audio_output_config_user_data = NULL; // Use cfg.user_data

        // Create video player playlist instance
        LOGI("%s: Creating video player playlist instance\n", __func__);
        ret = bk_video_player_playlist_new(&s_video_player_app_handle, &cfg);
        if (ret != AVDK_ERR_OK || s_video_player_app_handle == NULL)
        {
            LOGE("%s: bk_video_player_playlist_new failed, ret=%d\n", __func__, ret);
            s_video_player_app_handle = NULL;
            goto exit;
        }

        bk_video_player_playlist_register_avi_container_parser(s_video_player_app_handle);
        bk_video_player_playlist_register_mp4_container_parser(s_video_player_app_handle);
        bk_video_player_playlist_register_hw_jpeg_video_decoder(s_video_player_app_handle);
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
        bk_video_player_playlist_register_hw_h264_video_decoder(s_video_player_app_handle);
#endif
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER
        bk_video_player_playlist_register_aac_audio_decoder(s_video_player_app_handle);
#endif
        // Open player playlist
        ret = bk_video_player_playlist_open(s_video_player_app_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: video player playlist open failed, ret=%d\n", __func__, ret);
            bk_video_player_playlist_delete(s_video_player_app_handle);
            s_video_player_app_handle = NULL;
            goto exit;
        }
        s_video_player_app_opened = true;

        ret = video_play_playlist_prepare_lcd_for_file(file_path);
        if (ret != AVDK_ERR_OK)
        {
            video_play_playlist_release_player_handle();
            video_play_playlist_release_media_outputs();
            goto exit;
        }

        LOGI("%s: LCD display opened successfully\n", __func__);

        // Probe media info first (if file_path is provided), then decide whether to open audio output.
        if (file_path != NULL && s_audio_player_handle == NULL)
        {
            video_player_media_info_t start_media_info;
            os_memset(&start_media_info, 0, sizeof(start_media_info));
            ret = bk_video_player_playlist_get_media_info(s_video_player_app_handle, file_path, &start_media_info);
            // Some containers/codecs (e.g. AAC in AVI) may not provide bits_per_sample in header.
            // For audio output, we only require channel count and sample rate. Bits will fallback to 16-bit PCM.
            if (ret == AVDK_ERR_OK &&
                start_media_info.audio.channels > 0 &&
                start_media_info.audio.sample_rate > 0)
            {
                uint32_t in_audio_rate = start_media_info.audio.sample_rate;
                uint32_t in_audio_channels = start_media_info.audio.channels;
                uint32_t in_audio_bits = start_media_info.audio.bits_per_sample;

                // Calculate frame size (20ms of audio data)
                uint32_t eff_bits = (in_audio_bits > 0) ? in_audio_bits : 16;
                uint32_t bytes_per_sample = eff_bits / 8;
                if (bytes_per_sample == 0)
                {
                    bytes_per_sample = 2;
                }
                uint32_t frame_size = (in_audio_rate / 1000) * 20 * bytes_per_sample * in_audio_channels;
                // Only AAC commonly outputs large decoded PCM packets (e.g. 8kHz AAC often outputs 2048 bytes per packet).
                if (start_media_info.audio.format == VIDEO_PLAYER_AUDIO_FORMAT_AAC && frame_size < 2048)
                {
                    frame_size = 2048;
                }

                // Initialize audio player device
                audio_player_device_cfg_t audio_cfg = {0};
                audio_cfg.channels = (in_audio_channels > 0) ? in_audio_channels : 1;
                audio_cfg.sample_rate = in_audio_rate;
                audio_cfg.bits_per_sample = (in_audio_bits > 0) ? in_audio_bits : 16;
                audio_cfg.format = start_media_info.audio.format;
                audio_cfg.frame_size = frame_size;

                ret = audio_player_device_init(&audio_cfg, &s_audio_player_handle);
                if (ret != AVDK_ERR_OK || s_audio_player_handle == NULL)
                {
                    LOGW("%s: audio_player_device_init failed, ret=%d, continue without audio output\n", __func__, ret);
                }
                else
                {
                    // Start audio player device
                    ret = audio_player_device_start(s_audio_player_handle);
                    if (ret != AVDK_ERR_OK)
                    {
                        LOGW("%s: audio_player_device_start failed, ret=%d, continue without audio output\n", __func__, ret);
                        audio_player_device_deinit(s_audio_player_handle);
                        s_audio_player_handle = NULL;
                    }
                    else
                    {
                        LOGI("%s: Audio output opened successfully\n", __func__);
                    }
                }
            }
        }

        // Update callback context after audio output init (or keep NULL to drop audio).
        s_play_user_ctx.audio_player_handle = s_audio_player_handle;


        // If file_path is provided, add it to playlist and play
        if (file_path != NULL)
        {
            ret = bk_video_player_playlist_play_file(s_video_player_app_handle, file_path);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: video player playlist play failed, ret=%d\n", __func__, ret);
                video_play_playlist_release_player_handle();
                video_play_playlist_release_media_outputs();
                goto exit;
            }
        }

        s_video_player_app_started = true;
        LOGI("%s: Video playback started (Playlist layer), file: %s\n", __func__,
             file_path ? file_path : "(none, use add/play to add files)");
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        video_play_playlist_runtime_shutdown();
        LOGI("%s: Video playback stopped (Playlist CLI, all layers)\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "pause") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_set_pause(s_video_player_app_handle, true);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Video playback paused\n", __func__);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to pause video, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "resume") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_set_pause(s_video_player_app_handle, false);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Video playback resumed\n", __func__);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to resume video, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "add") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: add command requires file_path argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_add_file(s_video_player_app_handle, argv[2]);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: File added to playlist: %s\n", __func__, argv[2]);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to add file, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "remove") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: remove command requires file_path argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_remove_file(s_video_player_app_handle, argv[2]);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: File removed from playlist: %s\n", __func__, argv[2]);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to remove file, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "clear") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_clear(s_video_player_app_handle);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Playlist cleared\n", __func__);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to clear playlist, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "next") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_play_next(s_video_player_app_handle);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Playing next file\n", __func__);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to play next file, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "prev") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            ret = bk_video_player_playlist_play_prev(s_video_player_app_handle);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Playing previous file\n", __func__);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to play previous file, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "play") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: play command requires file_path or index argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            // Check if argument is a number (index) or file path
            char *endptr;
            uint32_t index = (uint32_t)os_strtoul(argv[2], &endptr, 10);
            if (*endptr == '\0')
            {
                // It's a number, treat as index
                ret = bk_video_player_playlist_play_at_index(s_video_player_app_handle, index);
                if (ret == AVDK_ERR_OK)
                {
                    LOGI("%s: Playing file at index %u\n", __func__, index);
                    msg = CLI_CMD_RSP_SUCCEED;
                }
                else
                {
                    LOGE("%s: Failed to play file at index %u, ret=%d\n", __func__, index, ret);
                }
            }
            else
            {
                // It's a file path
                ret = bk_video_player_playlist_play_file(s_video_player_app_handle, argv[2]);
                if (ret == AVDK_ERR_OK)
                {
                    LOGI("%s: Playing file: %s\n", __func__, argv[2]);
                    msg = CLI_CMD_RSP_SUCCEED;
                }
                else
                {
                    LOGE("%s: Failed to play file, ret=%d\n", __func__, ret);
                }
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "list") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint32_t count = 0;
            ret = bk_video_player_playlist_get_file_count(s_video_player_app_handle, &count);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Playlist has %u files\n", __func__, count);
            }

            char current_file[256] = {0};
            uint32_t current_index = 0;
            ret = bk_video_player_playlist_get_current_file(s_video_player_app_handle,
                                                            current_file,
                                                            sizeof(current_file),
                                                            &current_index);
            if (ret == AVDK_ERR_OK && current_file[0] != '\0')
            {
                LOGI("%s: Current file[%u]: %s\n", __func__, (unsigned int)current_index, current_file);
            }
            else
            {
                LOGI("%s: No current file\n", __func__);
            }

            msg = CLI_CMD_RSP_SUCCEED;
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "status") == 0)
    {
        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            video_player_status_t status;
            uint64_t current_time = 0;
            video_player_media_info_t info = {0};

            ret = bk_video_player_playlist_get_status(s_video_player_app_handle, &status);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Status: %d\n", __func__, status);
            }

            ret = bk_video_player_playlist_get_current_time(s_video_player_app_handle, &current_time);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Current time: %llu ms\n", __func__, (unsigned long long)current_time);
            }

            ret = bk_video_player_playlist_get_media_info(s_video_player_app_handle, NULL, &info);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Duration: %llu ms\n", __func__, (unsigned long long)info.duration_ms);
            }

            uint32_t count = 0;
            ret = bk_video_player_playlist_get_file_count(s_video_player_app_handle, &count);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Playlist count: %u\n", __func__, count);
            }

            msg = CLI_CMD_RSP_SUCCEED;
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "seek") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: seek command requires time_ms argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint64_t time_ms = (uint64_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_playlist_seek(s_video_player_app_handle, time_ms);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Seek to %llu ms\n", __func__, (unsigned long long)time_ms);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to seek, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "ff") == 0 || os_strcmp(argv[1], "fast_forward") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: ff command requires time_ms argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint32_t time_ms = (uint32_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_playlist_fast_forward(s_video_player_app_handle, time_ms);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Fast forward %u ms\n", __func__, (unsigned)time_ms);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to fast forward, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "rewind") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: fb command requires time_ms argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint32_t time_ms = (uint32_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_playlist_rewind(s_video_player_app_handle, time_ms);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Rewind %u ms\n", __func__, (unsigned)time_ms);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to rewind, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "info") == 0)
    {
        if (s_video_player_app_handle == NULL || !s_video_player_app_opened)
        {
            LOGE("%s: Video player playlist not started\n", __func__);
            goto exit;
        }

        const char *path = (argc >= 3) ? argv[2] : NULL;
        video_player_media_info_t media_info;
        os_memset(&media_info, 0, sizeof(media_info));

        ret = bk_video_player_playlist_get_media_info(s_video_player_app_handle, path, &media_info);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_player_playlist_get_media_info failed, ret=%d\n", __func__, ret);
            goto exit;
        }

        LOGI("%s: Media info (duration=%llu ms, file_size=%llu bytes)\n",
             __func__,
             (unsigned long long)media_info.duration_ms,
             (unsigned long long)media_info.file_size_bytes);
        LOGI("%s: Video: width=%u height=%u fps=%u format=%u jpeg_subsampling=%u\n",
             __func__,
             (unsigned)media_info.video.width,
             (unsigned)media_info.video.height,
             (unsigned)media_info.video.fps,
             (unsigned)media_info.video.format,
             (unsigned)media_info.video.jpeg_subsampling);
        LOGI("%s: Audio: channels=%u sample_rate=%u bits_per_sample=%u format=%u\n",
             __func__,
             (unsigned)media_info.audio.channels,
             (unsigned)media_info.audio.sample_rate,
             (unsigned)media_info.audio.bits_per_sample,
             (unsigned)media_info.audio.format);

        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "play_mode") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: play_mode requires one argument: stop|repeat|loop\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle == NULL || !s_video_player_app_opened)
        {
            LOGE("%s: Video player playlist not started\n", __func__);
            goto exit;
        }

        bk_video_player_playlist_play_mode_t mode = BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_STOP;
        if (os_strcmp(argv[2], "stop") == 0)
        {
            mode = BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_STOP;
        }
        else if (os_strcmp(argv[2], "repeat") == 0)
        {
            mode = BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_REPEAT;
        }
        else if (os_strcmp(argv[2], "loop") == 0)
        {
            mode = BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_LOOP;
        }
        else
        {
            LOGE("%s: invalid play_mode '%s', expected: stop|repeat|loop\n", __func__, argv[2]);
            goto exit;
        }

        ret = bk_video_player_playlist_set_play_mode(s_video_player_app_handle, mode);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_player_playlist_set_play_mode failed, ret=%d\n", __func__, ret);
            goto exit;
        }

        LOGI("%s: play_mode set to %d (%s)\n", __func__, (int)mode, argv[2]);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "volume") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: volume command requires level (0-100) argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint8_t volume = (uint8_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_playlist_set_volume(s_video_player_app_handle, volume);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Volume set to %u%%\n", __func__, volume);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to set volume, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "vol_up") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: vol_up command requires step (0-100) argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint8_t step = (uint8_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_playlist_volume_up(s_video_player_app_handle, step);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Volume up by %u\n", __func__, (unsigned)step);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to volume up, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "vol_down") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: vol_down command requires step (0-100) argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            uint8_t step = (uint8_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_playlist_volume_down(s_video_player_app_handle, step);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Volume down by %u\n", __func__, (unsigned)step);
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to volume down, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "mute") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: mute command requires on/off argument\n", __func__);
            goto exit;
        }

        if (s_video_player_app_handle != NULL && s_video_player_app_opened)
        {
            bool mute = (os_strcmp(argv[2], "on") == 0);
            ret = bk_video_player_playlist_set_mute(s_video_player_app_handle, mute);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Audio mute set to %s\n", __func__, mute ? "on" : "off");
                msg = CLI_CMD_RSP_SUCCEED;
            }
            else
            {
                LOGE("%s: Failed to set mute, ret=%d\n", __func__, ret);
            }
        }
        else
        {
            LOGE("%s: Video player playlist not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "avsync") == 0 || os_strcmp(argv[1], "av_sync") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: avsync requires offset_ms (range=[%d,%d])\n",
                 __func__, -VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX, VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX);
            goto exit;
        }

        if (s_video_player_app_handle == NULL || !s_video_player_app_opened)
        {
            LOGE("%s: Video player playlist not started\n", __func__);
            goto exit;
        }

        int32_t offset_ms = 0;
        if (!video_play_parse_int32_dec_range(argv[2],
                                              -VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX,
                                              VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX,
                                              &offset_ms))
        {
            LOGE("%s: invalid offset_ms: %s (range=[%d,%d])\n",
                 __func__, argv[2], -VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX, VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX);
            goto exit;
        }

        ret = bk_video_player_playlist_set_av_sync_offset_ms(s_video_player_app_handle, offset_ms);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: set_av_sync_offset_ms failed, ret=%d, offset_ms=%d\n",
                 __func__, ret, (int)offset_ms);
            goto exit;
        }

        LOGI("%s: A/V sync offset set to %d ms\n", __func__, (int)offset_ms);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }

exit:
    os_strncpy(pcWriteBuffer, msg, xWriteBufferLen - 1);
    pcWriteBuffer[xWriteBufferLen - 1] = '\0';
}

