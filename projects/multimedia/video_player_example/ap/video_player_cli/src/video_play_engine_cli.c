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

#include <common/bk_include.h>
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
#include "components/bk_video_player/bk_video_player_engine.h"

// Module ops providers (CLI registers them explicitly).
#include "components/bk_video_player/container_parser/bk_video_player_avi_parser.h"
#include "components/bk_video_player/container_parser/bk_video_player_mp4_parser.h"
#include "components/bk_video_player/video_decoder/bk_video_player_hw_jpeg_decoder.h"
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#include "components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h"
#endif
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER
#include "components/bk_video_player/audio_decoder/bk_video_player_aac_decoder.h"
#endif

#include "bk_partition.h"
#include <sys/stat.h>

#define TAG "video_play_engine_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

// Speaker digital gain
#define VIDEO_PLAY_SPK_DIG_GAIN_DEFAULT   (0x3f) // +17dB
#define VIDEO_PLAY_SPK_DIG_GAIN_MAX       (0x3F)

// Keep CLI A/V sync offset consistent with core layer validation.
#define VIDEO_PLAY_AV_SYNC_OFFSET_MS_MAX  (5000)

// Global variables for Engine layer player
static bk_video_player_engine_handle_t s_video_player_core_handle = NULL;
static bool s_video_player_core_started = false;
static bool s_video_player_core_opened = false;
static bk_display_ctlr_handle_t s_lcd_display_handle = NULL;
static audio_player_device_handle_t s_audio_player_handle = NULL;

typedef enum
{
    VIDEO_PLAY_ENGINE_H264_DECODER_FLEXA_GPU = 0,
    VIDEO_PLAY_ENGINE_H264_DECODER_FRAME,
} video_play_engine_h264_decoder_mode_t;

static video_play_engine_h264_decoder_mode_t s_h264_decoder_mode = VIDEO_PLAY_ENGINE_H264_DECODER_FLEXA_GPU;

static const char *video_play_engine_h264_decoder_mode_name(video_play_engine_h264_decoder_mode_t mode)
{
    return (mode == VIDEO_PLAY_ENGINE_H264_DECODER_FRAME) ? "frame" : "flexa_gpu";
}

static video_play_lcd_video_fmt_t video_play_engine_lcd_format_for_h264_decoder(
    video_play_engine_h264_decoder_mode_t mode)
{
    return (mode == VIDEO_PLAY_ENGINE_H264_DECODER_FRAME)
        ? VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW
        : VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED;
}

static bool video_play_engine_parse_h264_decoder_mode(int argc,
                                                      char **argv,
                                                      video_play_engine_h264_decoder_mode_t *mode)
{
    if (mode == NULL)
    {
        return false;
    }

    *mode = VIDEO_PLAY_ENGINE_H264_DECODER_FLEXA_GPU;
    if (argc < 4)
    {
        return true;
    }

    if (argc > 4)
    {
        LOGE("%s: too many start arguments, usage: start <file_path> [frame|gpu|flexa]\n", __func__);
        return false;
    }

    if (os_strcmp(argv[3], "frame") == 0)
    {
        *mode = VIDEO_PLAY_ENGINE_H264_DECODER_FRAME;
        return true;
    }

    if (os_strcmp(argv[3], "gpu") == 0 || os_strcmp(argv[3], "flexa") == 0)
    {
        *mode = VIDEO_PLAY_ENGINE_H264_DECODER_FLEXA_GPU;
        return true;
    }

    LOGE("%s: unsupported decoder mode '%s', usage: start <file_path> [frame|gpu|flexa]\n",
         __func__, argv[3]);
    return false;
}



void bk_video_player_engine_register_avi_container_parser(void *handle)
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

    avdk_err_t ret = bk_video_player_engine_register_container_parser((bk_video_player_engine_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_avi_container_parser failed, ret=%d\n", __func__, ret);
    }
}

void bk_video_player_engine_register_mp4_container_parser(void *handle)
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

    avdk_err_t ret = bk_video_player_engine_register_container_parser((bk_video_player_engine_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_mp4_container_parser failed, ret=%d\n", __func__, ret);
    }
}

void bk_video_player_engine_register_hw_jpeg_video_decoder(void *handle)
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

    avdk_err_t ret = bk_video_player_engine_register_video_decoder((bk_video_player_engine_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_hw_jpeg_video_decoder failed, ret=%d\n", __func__, ret);
    }
}

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
void bk_video_player_engine_register_hw_h264_video_decoder(void *handle)
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

    avdk_err_t ret = bk_video_player_engine_register_video_decoder((bk_video_player_engine_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_hw_h264_video_decoder failed, ret=%d\n", __func__, ret);
    }
}

void bk_video_player_engine_register_hw_h264_frame_video_decoder(void *handle)
{
    if (handle == NULL)
    {
        return;
    }

    video_player_video_decoder_ops_t *ops = bk_video_player_get_hw_h264_decoder_frame_ops();
    if (ops == NULL)
    {
        LOGE("%s: get_hw_h264_decoder_frame_ops failed\n", __func__);
        return;
    }

    avdk_err_t ret = bk_video_player_engine_register_video_decoder((bk_video_player_engine_handle_t)handle, ops);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: register_hw_h264_frame_video_decoder failed, ret=%d\n", __func__, ret);
    }
}

#endif

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER
void bk_video_player_engine_register_aac_audio_decoder(void *handle)
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

    avdk_err_t ret = bk_video_player_engine_register_audio_decoder((bk_video_player_engine_handle_t)handle, ops);
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

static avdk_err_t video_play_audio_set_volume_cb(void *user_data, uint8_t volume)
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
        // Audio output may not be initialized yet (e.g. probe media info first).
        // Treat it as best-effort and return OK to avoid failing engine_open().
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

static avdk_err_t video_play_audio_set_mute_cb(void *user_data, bool mute)
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
        // Audio output may not be initialized yet (e.g. probe media info first).
        // Treat it as best-effort and return OK to avoid failing engine_open().
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

static avdk_err_t video_play_audio_output_config_cb(void *user_data, const video_player_audio_params_t *params)
{
    if (params == NULL)
    {
        LOGE("%s: params is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Note: audio output might not be opened yet (probe media info first).
    // Treat it as best-effort and return OK to avoid failing playback when audio output is disabled.
    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    if (ctx == NULL || ctx->audio_player_handle == NULL)
    {
        return AVDK_ERR_OK;
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
     * Clear buffered old audio on file switching by recreating audio output:
     * deinit -> init -> start.
     */
    audio_player_device_handle_t old_handle = ctx->audio_player_handle;
    ctx->audio_player_handle = NULL;
    s_audio_player_handle = NULL;

    avdk_err_t ret = audio_player_device_deinit(old_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_player_device_deinit failed, ret=%d\n", __func__, ret);
        // Restore pointer on failure to avoid dropping audio permanently.
        ctx->audio_player_handle = old_handle;
        s_audio_player_handle = old_handle;
        return ret;
    }

    /*
     * Calculate PCM frame size for AUDIO OUTPUT buffering (20ms of audio).
     *
     * IMPORTANT:
     * Keep this as a small and stable pacing unit (suggest 20ms), do NOT enlarge it to match
     * decoder output packet size (e.g. AAC can output 2048 bytes per packet at 8kHz).
     * Enlarging frame_size increases the sink pre-buffer depth and causes fixed A/V desync
     * (video appears ahead of audio by the buffered duration).
     */
    uint32_t bytes_per_sample = bits / 8;
    if (bytes_per_sample == 0)
    {
        bytes_per_sample = 2;
    }
    uint32_t samples_20ms = (params->sample_rate * 20 + 999) / 1000;
    uint32_t frame_size = samples_20ms * bytes_per_sample * params->channels;

    audio_player_device_cfg_t audio_cfg = {0};
    audio_cfg.channels = params->channels;
    audio_cfg.sample_rate = params->sample_rate;
    audio_cfg.bits_per_sample = bits;
    audio_cfg.format = params->format;
    audio_cfg.frame_size = frame_size;

    audio_player_device_handle_t new_handle = NULL;
    ret = audio_player_device_init(&audio_cfg, &new_handle);
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

// Playback runtime context passed to callbacks via cfg.user_data.
static video_play_user_ctx_t s_play_user_ctx = {0};

static void video_play_engine_destroy_runtime(bool close_lcd)
{
    if (s_video_player_core_handle != NULL)
    {
        if (s_video_player_core_opened)
        {
            avdk_err_t ret = bk_video_player_engine_stop(s_video_player_core_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGW("%s: video player engine stop failed, ret=%d\n", __func__, ret);
            }

            ret = bk_video_player_engine_close(s_video_player_core_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGW("%s: video player engine close failed, ret=%d\n", __func__, ret);
            }
        }

        avdk_err_t ret = bk_video_player_engine_delete(s_video_player_core_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s: video player engine delete failed, ret=%d\n", __func__, ret);
        }

        s_video_player_core_handle = NULL;
        s_video_player_core_opened = false;
        s_video_player_core_started = false;
        video_play_mark_engine_active(false);
    }

    if (s_audio_player_handle != NULL)
    {
        audio_player_device_stop(s_audio_player_handle);
        audio_player_device_deinit(s_audio_player_handle);
        s_audio_player_handle = NULL;
        s_play_user_ctx.audio_player_handle = NULL;
    }

    if (close_lcd && s_lcd_display_handle != NULL)
    {
        avdk_err_t ret = video_play_lcd_close();
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s: video_play_lcd_close failed, ret=%d\n", __func__, ret);
        }
        s_lcd_display_handle = NULL;
        s_play_user_ctx.lcd_handle = NULL;
        video_play_lcd_runtime_format_reset();
    }
}

void video_play_engine_runtime_shutdown(void)
{
    video_play_engine_destroy_runtime(true);
    video_play_mark_engine_active(false);
}

// CLI command:
// video_play_engine start <file_path> [frame|gpu|flexa] / stop / pause / resume / seek [time_ms] / ff [time_ms] / rewind [time_ms] /
// volume [0-100] / vol_up [step] / vol_down [step] / mute [on|off] / avsync [offset_ms] / status / info
void cli_video_play_engine_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
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
        video_play_engine_h264_decoder_mode_t requested_h264_decoder_mode = VIDEO_PLAY_ENGINE_H264_DECODER_FLEXA_GPU;

        if (file_path == NULL)
        {
            LOGE("%s: file_path is required\n", __func__);
            goto exit;
        }

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
        if (!video_play_engine_parse_h264_decoder_mode(argc, argv, &requested_h264_decoder_mode))
        {
            goto exit;
        }
#else
        if (argc >= 4)
        {
            LOGE("%s: H264 decoder mode argument requires CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER\n",
                 __func__);
            goto exit;
        }
#endif

        if (s_video_player_core_handle != NULL &&
            s_video_player_core_opened &&
            requested_h264_decoder_mode != s_h264_decoder_mode)
        {
            LOGI("%s: switching H264 decoder mode %s -> %s, recreating engine\n",
                 __func__,
                 video_play_engine_h264_decoder_mode_name(s_h264_decoder_mode),
                 video_play_engine_h264_decoder_mode_name(requested_h264_decoder_mode));
            video_play_engine_destroy_runtime(true);
        }

        /*
         * Allow re-play after EOF without re-creating the whole engine.
         * If engine is already started, just restart playback:
         * The restart sequence is handled inside bk_video_player_engine_play_file().
         */
        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            ret = bk_video_player_engine_play_file(s_video_player_core_handle, file_path);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: video player engine restart failed, ret=%d\n", __func__, ret);
                goto exit;
            }

            LOGI("%s: Video playback restarted (Engine layer), file: %s\n", __func__, file_path);
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

        // Initialize LCD display
        if (s_lcd_display_handle == NULL)
        {
            ret = video_play_lcd_open_with_format(
                &s_lcd_display_handle,
                video_play_engine_lcd_format_for_h264_decoder(requested_h264_decoder_mode));
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: video_play_lcd_open failed, ret:%d\n", __func__, ret);
                goto exit;
            }

            LOGI("%s: LCD display opened successfully\n", __func__);
        }

        // Initialize video player engine configuration
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

        /* H264 frame mode uses NV12 output and switches the DPU to NV12.
         * Flexa GPU mode ignores this requested format and emits compressed
         * ARGB8888 for the board-default DPU DEC400 path. */
        cfg.video.output_format = PIXEL_FMT_NV12;
        s_play_user_ctx.lcd_handle = s_lcd_display_handle;
        // Audio output may be opened later after probing media info.
        s_play_user_ctx.audio_player_handle = NULL;
        s_play_user_ctx.audio_volume = 100;
        s_play_user_ctx.audio_muted = false;
        cfg.user_data = &s_play_user_ctx;

        // Volume/mute control callbacks must be provided at init time (no dynamic registration).
        cfg.audio.audio_set_volume_cb = video_play_audio_set_volume_cb;
        cfg.audio.audio_set_mute_cb = video_play_audio_set_mute_cb;
        cfg.audio.audio_ctrl_user_data = NULL; // Use cfg.user_data

        // Audio output parameter (re)configuration callback (sample rate/bits/channels).
        cfg.audio.audio_output_config_cb = video_play_audio_output_config_cb;
        cfg.audio.audio_output_config_user_data = NULL; // Use cfg.user_data

        // Create video player engine instance
        LOGI("%s: Creating video player engine instance, h264_decoder=%s\n",
             __func__, video_play_engine_h264_decoder_mode_name(requested_h264_decoder_mode));
        ret = bk_video_player_engine_new(&s_video_player_core_handle, &cfg);
        if (ret != AVDK_ERR_OK || s_video_player_core_handle == NULL)
        {
            LOGE("%s: bk_video_player_engine_new failed, ret=%d\n", __func__, ret);
            s_video_player_core_handle = NULL;
            goto exit;
        }

        bk_video_player_engine_register_avi_container_parser(s_video_player_core_handle);
        bk_video_player_engine_register_mp4_container_parser(s_video_player_core_handle);
        bk_video_player_engine_register_hw_jpeg_video_decoder(s_video_player_core_handle);
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
        if (requested_h264_decoder_mode == VIDEO_PLAY_ENGINE_H264_DECODER_FRAME)
        {
            bk_video_player_engine_register_hw_h264_frame_video_decoder(s_video_player_core_handle);
        }
        else
        {
            bk_video_player_engine_register_hw_h264_video_decoder(s_video_player_core_handle);
        }
#endif
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER
        bk_video_player_engine_register_aac_audio_decoder(s_video_player_core_handle);
#endif

        // Open player engine
        ret = bk_video_player_engine_open(s_video_player_core_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: video player engine open failed, ret=%d\n", __func__, ret);
            bk_video_player_engine_delete(s_video_player_core_handle);
            s_video_player_core_handle = NULL;
            goto exit;
        }
        s_video_player_core_opened = true;
        video_play_mark_engine_active(true);
        s_h264_decoder_mode = requested_h264_decoder_mode;

        // Probe media info first, then decide whether to open audio output.
        video_player_media_info_t start_media_info;
        os_memset(&start_media_info, 0, sizeof(start_media_info));
        ret = bk_video_player_engine_get_media_info(s_video_player_core_handle, file_path, &start_media_info);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: get_media_info failed, ret=%d, file=%s\n", __func__, ret, file_path);
            bk_video_player_engine_close(s_video_player_core_handle);
            bk_video_player_engine_delete(s_video_player_core_handle);
            s_video_player_core_handle = NULL;
            s_video_player_core_opened = false;
            video_play_mark_engine_active(false);
            goto exit;
        }

        // Some containers/codecs (e.g. AAC in AVI) may not provide bits_per_sample in header.
        // For audio output, we only require channel count and sample rate. Bits will fallback to 16-bit PCM.
        bool has_audio = (start_media_info.audio.channels > 0 &&
                          start_media_info.audio.sample_rate > 0);
        if (has_audio && s_audio_player_handle == NULL)
        {
            uint32_t in_audio_rate = start_media_info.audio.sample_rate;
            uint32_t in_audio_channels = start_media_info.audio.channels;
            uint32_t in_audio_bits = start_media_info.audio.bits_per_sample;

            /*
             * Calculate PCM frame size for AUDIO OUTPUT buffering (20ms of audio).
             *
             * IMPORTANT:
             * - frame_size MUST represent the sink's steady pacing unit (suggest 20ms of PCM),
             *   not the decoder's "packet" size.
             * - For AAC, a decoded packet can be 2048 bytes at 8kHz mono 16-bit (128ms audio).
             *   If we set sink frame_size=2048, the audio pipeline will pre-buffer multiple frames
             *   (raw_stream blocks + speaker DMA ring), easily introducing ~0.5s fixed latency.
             * - Upper layer can still write larger chunks (e.g. 2048 bytes), it will be buffered and
             *   drained as 20ms frames by the sink.
             */
            uint32_t eff_bits = (in_audio_bits > 0) ? in_audio_bits : 16;
            uint32_t bytes_per_sample = eff_bits / 8;
            if (bytes_per_sample == 0)
            {
                bytes_per_sample = 2;
            }
            uint32_t samples_20ms = (in_audio_rate * 20 + 999) / 1000;
            uint32_t frame_size = samples_20ms * bytes_per_sample * in_audio_channels;

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
                has_audio = false;
                goto audio_init_done;
            }

            // Start audio player device
            ret = audio_player_device_start(s_audio_player_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGW("%s: audio_player_device_start failed, ret=%d, continue without audio output\n", __func__, ret);
                audio_player_device_deinit(s_audio_player_handle);
                s_audio_player_handle = NULL;
                has_audio = false;
                goto audio_init_done;
            }

            LOGI("%s: Audio output opened successfully\n", __func__);
        }

audio_init_done:
        // Update callback context after audio output init (or keep NULL to drop audio).
        s_play_user_ctx.audio_player_handle = s_audio_player_handle;
        if (has_audio && s_audio_player_handle != NULL)
        {
            (void)bk_video_player_engine_set_volume(s_video_player_core_handle, 100);
            (void)bk_video_player_engine_set_mute(s_video_player_core_handle, false);
        }


        // Set file path and play
        ret = bk_video_player_engine_set_file_path(s_video_player_core_handle, file_path);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_player_engine_set_file_path failed, ret=%d\n", __func__, ret);
            bk_video_player_engine_close(s_video_player_core_handle);
            bk_video_player_engine_delete(s_video_player_core_handle);
            s_video_player_core_handle = NULL;
            s_video_player_core_opened = false;
            video_play_mark_engine_active(false);
            goto exit;
        }

        // Always reset playback time to 0 for "start" command.
        ret = bk_video_player_engine_play(s_video_player_core_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: video player engine start(seek 0) failed, ret=%d\n", __func__, ret);
            bk_video_player_engine_close(s_video_player_core_handle);
            bk_video_player_engine_delete(s_video_player_core_handle);
            s_video_player_core_handle = NULL;
            s_video_player_core_opened = false;
            video_play_mark_engine_active(false);
            goto exit;
        }

        s_video_player_core_started = true;
        LOGI("%s: Video playback started (Engine layer), file: %s, h264_decoder=%s\n",
             __func__, file_path, video_play_engine_h264_decoder_mode_name(s_h264_decoder_mode));
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "info") == 0)
    {
        video_player_media_info_t media_info = {0};
        const char *file_path = (argc >= 3) ? argv[2] : NULL;

        if (s_video_player_core_handle == NULL || !s_video_player_core_opened)
        {
            LOGE("%s: engine is not opened, please run start first\n", __func__);
            goto exit;
        }

        ret = bk_video_player_engine_get_media_info(s_video_player_core_handle, file_path, &media_info);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_player_engine_get_media_info failed, ret=%d\n", __func__, ret);
            goto exit;
        }

        LOGI("%s: Media info (duration=%llu ms, file_size=%llu bytes)\n",
             __func__, (unsigned long long)media_info.duration_ms, (unsigned long long)media_info.file_size_bytes);
        LOGI("%s: Video: width=%u height=%u fps=%u format=%u jpeg_subsampling=%u\n",
             __func__,
             (unsigned)media_info.video.width, (unsigned)media_info.video.height, (unsigned)media_info.video.fps,
             (unsigned)media_info.video.format, (unsigned)media_info.video.jpeg_subsampling);
        LOGI("%s: Audio: channels=%u sample_rate=%u bits_per_sample=%u format=%u\n",
             __func__,
             (unsigned)media_info.audio.channels, (unsigned)media_info.audio.sample_rate,
             (unsigned)media_info.audio.bits_per_sample, (unsigned)media_info.audio.format);

        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        video_play_stop_all_and_unmount_sd();
        LOGI("%s: Video playback stopped (Engine CLI, all layers)\n", __func__);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "pause") == 0)
    {
        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            ret = bk_video_player_engine_set_pause(s_video_player_core_handle, true);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "resume") == 0)
    {
        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            ret = bk_video_player_engine_set_pause(s_video_player_core_handle, false);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "seek") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: seek command requires time_ms argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            uint64_t time_ms = (uint64_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_engine_seek(s_video_player_core_handle, time_ms);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "ff") == 0 || os_strcmp(argv[1], "fast_forward") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: ff command requires time_ms argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            uint32_t time_ms = (uint32_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_engine_fast_forward(s_video_player_core_handle, time_ms);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "rewind") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: fb command requires time_ms argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            uint32_t time_ms = (uint32_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_engine_rewind(s_video_player_core_handle, time_ms);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "volume") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: volume command requires level (0-100) argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            uint8_t volume = (uint8_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_engine_set_volume(s_video_player_core_handle, volume);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "vol_up") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: vol_up command requires step (0-100) argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            uint8_t step = (uint8_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_engine_volume_up(s_video_player_core_handle, step);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "vol_down") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: vol_down command requires step (0-100) argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            uint8_t step = (uint8_t)os_strtoul(argv[2], NULL, 10);
            ret = bk_video_player_engine_volume_down(s_video_player_core_handle, step);
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
            LOGE("%s: Video player engine not started\n", __func__);
        }
    }
    else if (os_strcmp(argv[1], "mute") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s: mute command requires on/off argument\n", __func__);
            goto exit;
        }

        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            bool mute = (os_strcmp(argv[2], "on") == 0);
            ret = bk_video_player_engine_set_mute(s_video_player_core_handle, mute);
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
            LOGE("%s: Video player engine not started\n", __func__);
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

        if (s_video_player_core_handle == NULL || !s_video_player_core_opened)
        {
            LOGE("%s: engine is not opened, please run start first\n", __func__);
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

        ret = bk_video_player_engine_set_av_sync_offset_ms(s_video_player_core_handle, offset_ms);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: set_av_sync_offset_ms failed, ret=%d, offset_ms=%d\n",
                 __func__, ret, (int)offset_ms);
            goto exit;
        }

        LOGI("%s: A/V sync offset set to %d ms\n", __func__, (int)offset_ms);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "status") == 0)
    {
        if (s_video_player_core_handle != NULL && s_video_player_core_opened)
        {
            video_player_status_t status;
            uint64_t current_time = 0;
            video_player_media_info_t info = {0};

            ret = bk_video_player_engine_get_status(s_video_player_core_handle, &status);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Status: %d\n", __func__, status);
            }

            ret = bk_video_player_engine_get_current_time(s_video_player_core_handle, &current_time);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Current time: %llu ms\n", __func__, (unsigned long long)current_time);
            }

            ret = bk_video_player_engine_get_media_info(s_video_player_core_handle, NULL, &info);
            if (ret == AVDK_ERR_OK)
            {
                LOGI("%s: Duration: %llu ms\n", __func__, (unsigned long long)info.duration_ms);
            }

            msg = CLI_CMD_RSP_SUCCEED;
        }
        else
        {
            LOGE("%s: Video player engine not started\n", __func__);
        }
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

