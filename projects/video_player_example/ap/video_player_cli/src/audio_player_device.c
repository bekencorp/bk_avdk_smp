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

#include "audio_player_device.h"

#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include <os/mem.h>
#include <os/str.h>

#define TAG "audio_player_device"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

// Speaker digital gain
#define AUDIO_PLAYER_DIG_GAIN_MAX       (0x3F)
#define AUDIO_PLAYER_DIG_GAIN_DEFAULT   (0x3F) // +17dB

// Audio player device context
typedef struct
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t raw_stream;
    audio_element_handle_t onboard_speaker_stream;
    audio_player_device_cfg_t cfg;
    uint8_t current_dig_gain;
    bool is_started;
    bool is_muted;
} audio_player_device_ctx_t;

/**
 * @brief Convert volume (0-100) to digital gain (0x00-0x3f)
 */
static uint8_t volume_to_dig_gain(uint8_t volume)
{
    if (volume > 100)
    {
        volume = 100;
    }

    // Map [0..100] -> [0x00..0x3f] linearly
    uint32_t gain = ((uint32_t)volume * (uint32_t)AUDIO_PLAYER_DIG_GAIN_MAX) / 100;
    if (gain > AUDIO_PLAYER_DIG_GAIN_MAX)
    {
        gain = AUDIO_PLAYER_DIG_GAIN_MAX;
    }
    return (uint8_t)gain;
}

avdk_err_t audio_player_device_init(const audio_player_device_cfg_t *cfg, audio_player_device_handle_t *handle)
{
    if (cfg == NULL || handle == NULL)
    {
        LOGE("%s: invalid parameter\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Validate configuration parameters
    if (cfg->channels == 0 || cfg->channels > 2)
    {
        LOGE("%s: invalid channels: %u (must be 1 or 2)\n", __func__, cfg->channels);
        return AVDK_ERR_INVAL;
    }

    if (cfg->sample_rate == 0)
    {
        LOGE("%s: invalid sample_rate: %u\n", __func__, cfg->sample_rate);
        return AVDK_ERR_INVAL;
    }

    if (cfg->bits_per_sample == 0)
    {
        LOGE("%s: invalid bits_per_sample: %u\n", __func__, cfg->bits_per_sample);
        return AVDK_ERR_INVAL;
    }

    if (cfg->frame_size == 0)
    {
        LOGE("%s: invalid frame_size: %u\n", __func__, cfg->frame_size);
        return AVDK_ERR_INVAL;
    }

    // Allocate device context
    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)os_malloc(sizeof(audio_player_device_ctx_t));
    if (ctx == NULL)
    {
        LOGE("%s: failed to allocate context\n", __func__);
        return AVDK_ERR_NOMEM;
    }
    os_memset(ctx, 0, sizeof(audio_player_device_ctx_t));

    // Copy configuration
    os_memcpy(&ctx->cfg, cfg, sizeof(audio_player_device_cfg_t));
    ctx->current_dig_gain = AUDIO_PLAYER_DIG_GAIN_DEFAULT;
    ctx->is_started = false;
    ctx->is_muted = false;

    // Initialize audio pipeline
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    ctx->pipeline = audio_pipeline_init(&pipeline_cfg);
    if (ctx->pipeline == NULL)
    {
        LOGE("%s: failed to init audio pipeline\n", __func__);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Initialize raw stream (writer)
    raw_stream_cfg_t raw_stream_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_stream_cfg.type = AUDIO_STREAM_WRITER;
    raw_stream_cfg.output_port_type = PORT_TYPE_RB;
    // Calculate block size and number based on frame_size
    raw_stream_cfg.out_block_size = cfg->frame_size;
    raw_stream_cfg.out_block_num = 2; // Use 2 blocks for buffering
    ctx->raw_stream = raw_stream_init(&raw_stream_cfg);
    if (ctx->raw_stream == NULL)
    {
        LOGE("%s: failed to init raw stream\n", __func__);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Initialize onboard speaker stream
    onboard_speaker_stream_cfg_t speaker_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
    speaker_cfg.chl_num = cfg->channels;
    speaker_cfg.sample_rate = cfg->sample_rate;
    speaker_cfg.bits = cfg->bits_per_sample;
    speaker_cfg.dig_gain = ctx->current_dig_gain;
    speaker_cfg.frame_size = cfg->frame_size;
    speaker_cfg.multi_in_port_num = 0;
    speaker_cfg.multi_out_port_num = 0;
    ctx->onboard_speaker_stream = onboard_speaker_stream_init(&speaker_cfg);
    if (ctx->onboard_speaker_stream == NULL)
    {
        LOGE("%s: failed to init onboard speaker stream\n", __func__);
        // Clean up raw_stream element
        audio_element_deinit(ctx->raw_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Register elements to pipeline
    bk_err_t ret = audio_pipeline_register(ctx->pipeline, ctx->raw_stream, "raw_stream");
    if (ret != BK_OK)
    {
        LOGE("%s: failed to register raw_stream, ret=%d\n", __func__, ret);
        // Clean up elements
        audio_element_deinit(ctx->onboard_speaker_stream);
        audio_element_deinit(ctx->raw_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    ret = audio_pipeline_register(ctx->pipeline, ctx->onboard_speaker_stream, "onboard_speaker");
    if (ret != BK_OK)
    {
        LOGE("%s: failed to register onboard_speaker, ret=%d\n", __func__, ret);
        // Unregister raw_stream and clean up
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        audio_element_deinit(ctx->onboard_speaker_stream);
        audio_element_deinit(ctx->raw_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Link elements: raw_stream -> onboard_speaker
    const char *link_tag[] = {"raw_stream", "onboard_speaker"};
    ret = audio_pipeline_link(ctx->pipeline, link_tag, 2);
    if (ret != BK_OK)
    {
        LOGE("%s: failed to link pipeline, ret=%d\n", __func__, ret);
        // Unregister elements and clean up
        audio_pipeline_unregister(ctx->pipeline, ctx->onboard_speaker_stream);
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        audio_element_deinit(ctx->onboard_speaker_stream);
        audio_element_deinit(ctx->raw_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    *handle = (audio_player_device_handle_t)ctx;
    LOGI("%s: initialized successfully, ch=%u, rate=%u, bits=%u, frame_size=%u\n",
         __func__, cfg->channels, cfg->sample_rate, cfg->bits_per_sample, cfg->frame_size);

    return AVDK_ERR_OK;
}

avdk_err_t audio_player_device_deinit(audio_player_device_handle_t handle)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)handle;

    // Stop if started
    if (ctx->is_started)
    {
        audio_player_device_stop(handle);
    }

    // Clean up pipeline and elements
    if (ctx->pipeline != NULL)
    {
        // Unlink pipeline first
        audio_pipeline_unlink(ctx->pipeline);

        // Unregister elements
        if (ctx->onboard_speaker_stream != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->onboard_speaker_stream);
        }
        if (ctx->raw_stream != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        }

        // Deinitialize pipeline
        audio_pipeline_deinit(ctx->pipeline);
        ctx->pipeline = NULL;
    }

    // Deinitialize elements
    if (ctx->onboard_speaker_stream != NULL)
    {
        audio_element_deinit(ctx->onboard_speaker_stream);
        ctx->onboard_speaker_stream = NULL;
    }
    if (ctx->raw_stream != NULL)
    {
        audio_element_deinit(ctx->raw_stream);
        ctx->raw_stream = NULL;
    }

    os_free(ctx);
    LOGI("%s: deinitialized\n", __func__);

    return AVDK_ERR_OK;
}

avdk_err_t audio_player_device_start(audio_player_device_handle_t handle)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)handle;

    if (ctx->is_started)
    {
        LOGW("%s: already started\n", __func__);
        return AVDK_ERR_OK;
    }

    // Start pipeline
    bk_err_t ret = audio_pipeline_run(ctx->pipeline);
    if (ret != BK_OK)
    {
        LOGE("%s: failed to run pipeline, ret=%d\n", __func__, ret);
        return AVDK_ERR_HWERROR;
    }

    ctx->is_started = true;
    LOGI("%s: started\n", __func__);

    return AVDK_ERR_OK;
}

avdk_err_t audio_player_device_stop(audio_player_device_handle_t handle)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)handle;

    if (!ctx->is_started)
    {
        LOGW("%s: not started\n", __func__);
        return AVDK_ERR_OK;
    }

    // Stop pipeline
    bk_err_t ret = audio_pipeline_stop(ctx->pipeline);
    if (ret != BK_OK)
    {
        LOGE("%s: failed to stop pipeline, ret=%d\n", __func__, ret);
        return AVDK_ERR_HWERROR;
    }

    ret = audio_pipeline_wait_for_stop(ctx->pipeline);
    if (ret != BK_OK)
    {
        LOGE("%s: failed to wait for pipeline stop, ret=%d\n", __func__, ret);
        return AVDK_ERR_HWERROR;
    }

    ctx->is_started = false;
    LOGI("%s: stopped\n", __func__);

    return AVDK_ERR_OK;
}

int32_t audio_player_device_write_frame_data(audio_player_device_handle_t handle, const char *data, uint32_t len)
{
    if (handle == NULL || data == NULL || len == 0)
    {
        LOGE("%s: invalid parameter\n", __func__);
        return -1;
    }

    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)handle;

    if (!ctx->is_started)
    {
        LOGW("%s: device not started\n", __func__);
        return -1;
    }

    // Write data to raw stream
    int written = raw_stream_write(ctx->raw_stream, (char *)data, (int)len);
    if (written < 0)
    {
        LOGW("%s: raw_stream_write failed, ret=%d\n", __func__, written);
        return written;
    }

    return written;
}

avdk_err_t audio_player_device_set_volume(audio_player_device_handle_t handle, uint8_t volume)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)handle;

    uint8_t dig_gain = volume_to_dig_gain(volume);
    bk_err_t ret = onboard_speaker_stream_set_digital_gain(ctx->onboard_speaker_stream, dig_gain);
    if (ret != BK_OK)
    {
        LOGE("%s: onboard_speaker_stream_set_digital_gain failed, ret=%d\n", __func__, ret);
        return AVDK_ERR_HWERROR;
    }

    ctx->current_dig_gain = dig_gain;
    LOGI("%s: volume=%u -> dig_gain=0x%02x\n", __func__, volume, dig_gain);

    return AVDK_ERR_OK;
}

avdk_err_t audio_player_device_set_mute(audio_player_device_handle_t handle, bool mute)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_player_device_ctx_t *ctx = (audio_player_device_ctx_t *)handle;

    uint8_t dig_gain = mute ? 0 : ctx->current_dig_gain;
    bk_err_t ret = onboard_speaker_stream_set_digital_gain(ctx->onboard_speaker_stream, dig_gain);
    if (ret != BK_OK)
    {
        LOGE("%s: onboard_speaker_stream_set_digital_gain failed, ret=%d\n", __func__, ret);
        return AVDK_ERR_HWERROR;
    }

    ctx->is_muted = mute;
    LOGI("%s: mute=%u, dig_gain=0x%02x\n", __func__, mute, dig_gain);

    return AVDK_ERR_OK;
}
