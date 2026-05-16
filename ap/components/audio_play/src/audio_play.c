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


#include <os/os.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#include <components/bk_audio/audio_decoders/sbc_dec.h>
#include <components/bk_audio/audio_decoders/aac_decoder.h>
#include <components/bk_audio/audio_utils/debug_dump_util.h>
#include <driver/aud_dac.h>
#include "audio_play.h"

#define AUDIO_PLAY_TAG "aud_play"
#define LOGE(...) BK_LOGE(AUDIO_PLAY_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUDIO_PLAY_TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(AUDIO_PLAY_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUDIO_PLAY_TAG, ##__VA_ARGS__)

#define AUDIO_PLAY_LISTENER_TASK_PRI   (4)
#define AUDIO_PLAY_LISTENER_TASK_STACK (2048)

typedef struct {
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t raw_stream;
    audio_element_handle_t decoder;
    audio_element_handle_t speaker;
    audio_event_iface_handle_t listener_evt;
    beken_thread_t listener_thread;
    volatile bool listener_running;
    audio_play_sta_t state;
} audio_play_ctx_t;

static void audio_play_event_listener_task(void *arg)
{
    audio_play_ctx_t *ctx = (audio_play_ctx_t *)arg;
    audio_event_iface_msg_t msg = {0};

    while (ctx && ctx->listener_running) {
        (void)audio_event_iface_listen(ctx->listener_evt, &msg, 20 / portTICK_RATE_MS);
    }

    if (ctx) {
        ctx->listener_thread = NULL;
    }
    rtos_delete_thread(NULL);
}

static void audio_play_build_spk_cfg(const audio_play_cfg_t *cfg, onboard_speaker_stream_cfg_t *spk_cfg)
{
    spk_cfg->chl_num = cfg->nChans;
    spk_cfg->sample_rate[0] = cfg->sampRate;
    spk_cfg->sample_rate[1] = cfg->sampRate;
    spk_cfg->sample_rate[2] = cfg->sampRate;
    spk_cfg->bits = cfg->bitsPerSample;
    spk_cfg->dig_gain = cfg->volume;
    //spk_cfg->ana_gain = 0x01;
    spk_cfg->work_mode = AUD_DAC_WORK_MODE_DIFFEN;
    spk_cfg->clk_src   = AUD_CLK_APLL;
    spk_cfg->multi_in_port_num  = 0;
    spk_cfg->multi_out_port_num = 0;
    spk_cfg->frame_size[0] = cfg->frame_size;
    spk_cfg->frame_size[1] = cfg->frame_size;
    spk_cfg->frame_size[2] = cfg->frame_size;
    spk_cfg->pool_length = cfg->pool_size;
    spk_cfg->pa_ctrl_en = false;
    spk_cfg->dac_source_bitmap = cfg->dac_source_bitmap;
    spk_cfg->main_dac_source   = cfg->main_dac_source;
}

static audio_element_handle_t audio_play_create_decoder(const audio_play_cfg_t *cfg)
{
    if (cfg->decoder_type == AUDIO_PLAY_DECODER_PCM) {
        return NULL;
    }

    if (cfg->decoder_type == AUDIO_PLAY_DECODER_SBC ||
        cfg->decoder_type == AUDIO_PLAY_DECODER_MSBC) {
#if CONFIG_ADK_SBC_DECODER
        sbc_decoder_cfg_t dec_cfg = DEFAULT_SBC_DECODER_CONFIG();
        dec_cfg.out_block_size = cfg->frame_size;
        dec_cfg.out_block_num  = 2;
        dec_cfg.msbc_mode = (cfg->decoder_type == AUDIO_PLAY_DECODER_MSBC);
        return sbc_dec_init(&dec_cfg);
#else
        return NULL;
#endif
    }

    if (cfg->decoder_type == AUDIO_PLAY_DECODER_AAC) {
#if CONFIG_ADK_AAC_DECODER
        aac_decoder_cfg_t dec_cfg = DEFAULT_AAC_DECODER_CONFIG();
        return aac_decoder_init(&dec_cfg);
#else
        return NULL;
#endif
    }

    return NULL;
}

audio_play_t *audio_play_create(audio_play_type_t play_type, audio_play_cfg_t *config)
{
    if (!config || play_type != AUDIO_PLAY_ONBOARD_SPEAKER) {
        return NULL;
    }

    audio_play_t *play = os_malloc(sizeof(audio_play_t));
    if (!play) {
        return NULL;
    }
    os_memset(play, 0x00, sizeof(audio_play_t));
    os_memcpy(&play->config, config, sizeof(audio_play_cfg_t));
    return play;
}

bk_err_t audio_play_open(audio_play_t *play)
{
    if (!play) {
        return BK_FAIL;
    }

    if (play->play_ctx) {
        return BK_OK;
    }

    audio_play_ctx_t *ctx = os_malloc(sizeof(audio_play_ctx_t));
    if (!ctx) {
        return BK_FAIL;
    }
    os_memset(ctx, 0x00, sizeof(audio_play_ctx_t));

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    ctx->pipeline = audio_pipeline_init(&pipeline_cfg);
    if (!ctx->pipeline) {
        goto fail;
    }

    raw_stream_cfg_t raw_cfg = {
        .type = AUDIO_STREAM_WRITER,
        .out_block_size = play->config.frame_size,
        .out_block_num  = 8,
        .output_port_type = PORT_TYPE_RB,
    };
    ctx->raw_stream = raw_stream_init(&raw_cfg);
    if (!ctx->raw_stream) {
        goto fail;
    }

    ctx->decoder = audio_play_create_decoder(&play->config);
    if (play->config.decoder_type != AUDIO_PLAY_DECODER_PCM && !ctx->decoder) {
        LOGE("%s, decoder init failed type:%d\n", __func__, play->config.decoder_type);
        goto fail;
    }

    onboard_speaker_stream_cfg_t spk_cfg = DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG();
    audio_play_build_spk_cfg(&play->config, &spk_cfg);
    ctx->speaker = onboard_speaker_stream_init(&spk_cfg);
    if (!ctx->speaker) {
        goto fail;
    }

    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->raw_stream, "raw")) {
        goto fail;
    }
    if (ctx->decoder && BK_OK != audio_pipeline_register(ctx->pipeline, ctx->decoder, "decoder")) {
        goto fail;
    }
    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->speaker, "speaker")) {
        goto fail;
    }

    if (ctx->decoder) {
        if (BK_OK != audio_pipeline_link(ctx->pipeline, (const char *[]) {"raw", "decoder", "speaker"}, 3)) {
            goto fail;
        }
    } else {
        if (BK_OK != audio_pipeline_link(ctx->pipeline, (const char *[]) {"raw", "speaker"}, 2)) {
            goto fail;
        }
    }

    if (BK_OK != audio_pipeline_run(ctx->pipeline)) {
        goto fail;
    }

#if CONFIG_ADK_DEBUG_DUMP_UTIL
   aud_dump_cli_init();
#endif

    ctx->state = AUDIO_PLAY_STA_RUNNING;
    play->play_ctx = ctx;
    return BK_OK;

fail:
    if (ctx) {
        if (ctx->listener_thread) {
            ctx->listener_running = false;
            while (ctx->listener_thread) {
                rtos_delay_milliseconds(5);
            }
        }
        if (ctx->pipeline && ctx->listener_evt) {
            audio_pipeline_remove_listener(ctx->pipeline);
        }
        if (ctx->listener_evt) {
            audio_event_iface_destroy(ctx->listener_evt);
        }
        if (ctx->pipeline) {
            audio_pipeline_deinit(ctx->pipeline);
        }
        if (ctx->speaker) {
            audio_element_deinit(ctx->speaker);
        }
        if (ctx->decoder) {
            audio_element_deinit(ctx->decoder);
        }
        if (ctx->raw_stream) {
            audio_element_deinit(ctx->raw_stream);
        }
        os_free(ctx);
    }
    return BK_FAIL;
}

bk_err_t audio_play_close(audio_play_t *play)
{
    if (!play) {
        return BK_FAIL;
    }

    audio_play_ctx_t *ctx = (audio_play_ctx_t *)play->play_ctx;
    if (!ctx) {
        return BK_OK;
    }

    if (ctx->pipeline) {
        audio_pipeline_stop(ctx->pipeline);
        audio_pipeline_wait_for_stop(ctx->pipeline);
        audio_pipeline_terminate(ctx->pipeline);
    }
    if (ctx->listener_thread) {
        ctx->listener_running = false;
        while (ctx->listener_thread) {
            rtos_delay_milliseconds(5);
        }
    }
    if (ctx->pipeline && ctx->listener_evt) {
        audio_pipeline_remove_listener(ctx->pipeline);
    }
    if (ctx->listener_evt) {
        audio_event_iface_destroy(ctx->listener_evt);
    }

    if (ctx->pipeline && ctx->speaker) {
        audio_pipeline_unregister(ctx->pipeline, ctx->speaker);
    }
    if (ctx->pipeline && ctx->decoder) {
        audio_pipeline_unregister(ctx->pipeline, ctx->decoder);
    }
    if (ctx->pipeline && ctx->raw_stream) {
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
    }

    if (ctx->speaker) {
        audio_element_deinit(ctx->speaker);
    }
    if (ctx->decoder) {
        audio_element_deinit(ctx->decoder);
    }
    if (ctx->raw_stream) {
        audio_element_deinit(ctx->raw_stream);
    }
    if (ctx->pipeline) {
        audio_pipeline_deinit(ctx->pipeline);
    }

    os_free(ctx);
    play->play_ctx = NULL;
    return BK_OK;
}

bk_err_t audio_play_destroy(audio_play_t *play)
{
    if (!play) {
        return BK_OK;
    }

    audio_play_close(play);
    os_free(play);
    return BK_OK;
}

bk_err_t audio_play_write_data(audio_play_t *play, char *buffer, uint32_t len)
{
    if (!play || !buffer || !len) {
        return BK_FAIL;
    }

    audio_play_ctx_t *ctx = (audio_play_ctx_t *)play->play_ctx;
    if (!ctx || !ctx->raw_stream) {
        return BK_FAIL;
    }

    return raw_stream_write(ctx->raw_stream, buffer, len);
}

bk_err_t audio_play_control(audio_play_t *play, audio_play_ctl_t ctl)
{
    if (!play) {
        return BK_FAIL;
    }

    audio_play_ctx_t *ctx = (audio_play_ctx_t *)play->play_ctx;
    if (!ctx) {
        return BK_FAIL;
    }

    switch (ctl) {
        case AUDIO_PLAY_PAUSE:
            return audio_pipeline_pause(ctx->pipeline);
        case AUDIO_PLAY_RESUME:
            return audio_pipeline_resume(ctx->pipeline);
        case AUDIO_PLAY_MUTE:
            return onboard_speaker_stream_dac_mute_en(ctx->speaker, 1);
        case AUDIO_PLAY_UNMUTE:
            return onboard_speaker_stream_dac_mute_en(ctx->speaker, 0);
        case AUDIO_PLAY_SET_VOLUME:
            return bk_aud_dac_set_dig_gain_db((float)play->config.volume);
        default:
            return BK_OK;
    }
}

bk_err_t audio_play_set_volume(audio_play_t *play, float volume)
{
    if (!play) {
        return BK_FAIL;
    }
    play->config.volume = volume;
    return audio_play_control(play, AUDIO_PLAY_SET_VOLUME);
}

