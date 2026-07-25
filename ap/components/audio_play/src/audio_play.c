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
#if CONFIG_ADK_MP3_DECODER
#include <components/bk_audio/audio_decoders/mp3_decoder.h>
#endif
#if CONFIG_ADK_WAV_DECODER
#include <components/bk_audio/audio_decoders/wav_decoder.h>
#endif
#include <components/bk_audio/audio_utils/debug_dump_util.h>
#if CONFIG_ADK_EQ_ALGORITHM
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>
#endif
#include <driver/aud_dac.h>
#include "audio_play.h"
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
#include "spk_service.h"
#endif

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
#if CONFIG_ADK_EQ_ALGORITHM
    audio_element_handle_t eq;
#endif
    audio_element_handle_t speaker;
    audio_element_handle_t reader;      /*!< external-sink mode: PCM tail reader (no speaker) */
    beken_thread_t pump_thread;         /*!< external-sink mode: PCM pump task */
    volatile bool pump_running;
    volatile bool pump_eos;             /*!< external-sink mode: tail reader hit end-of-stream */
    uint8_t *pump_buf;
    uint32_t pump_frame_size;
    struct audio_play *owner;           /*!< back-pointer for the pump task */
    audio_event_iface_handle_t listener_evt;
    beken_thread_t listener_thread;
    volatile bool listener_running;
    audio_play_sta_t state;
} audio_play_ctx_t;

#define AUDIO_PLAY_PUMP_TASK_PRI   (4)
#define AUDIO_PLAY_PUMP_TASK_STACK (2048)

/* external-sink mode: pull decoded PCM from the tail reader and hand each frame
 * to the caller supplied sink (e.g. spk_service). No onboard speaker is used. */
static void audio_play_pcm_pump_task(void *arg)
{
    audio_play_ctx_t *ctx = (audio_play_ctx_t *)arg;
    audio_play_t *play = ctx ? ctx->owner : NULL;

    while (ctx && ctx->pump_running) {
        int r = raw_stream_read(ctx->reader, (char *)ctx->pump_buf, (int)ctx->pump_frame_size);
        if (r == AEL_IO_DONE) {
            /* End-of-stream: decoder drained all PCM. Stop polling so we don't
             * spin on repeated DONE reads (log flood) and let the caller finish. */
            ctx->pump_eos = true;
            break;
        }
        if (r <= 0) {
            /* AEL_IO_TIMEOUT / transient: no PCM yet, re-check and retry. */
            rtos_delay_milliseconds(2);
            continue;
        }
        if (play && play->config.pcm_sink) {
            (void)play->config.pcm_sink(play->config.pcm_sink_user, ctx->pump_buf, (uint32_t)r);
        }
    }

    if (ctx) {
        ctx->pump_thread = NULL;
    }
    rtos_delete_thread(NULL);
}

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

    if (cfg->decoder_type == AUDIO_PLAY_DECODER_MP3) {
#if CONFIG_ADK_MP3_DECODER
        mp3_decoder_cfg_t dec_cfg = DEFAULT_MP3_DECODER_CONFIG();
        return mp3_decoder_init(&dec_cfg);
#else
        return NULL;
#endif
    }

    if (cfg->decoder_type == AUDIO_PLAY_DECODER_WAV) {
#if CONFIG_ADK_WAV_DECODER
        wav_decoder_cfg_t dec_cfg = DEFAULT_WAV_DECODER_CONFIG();
        return wav_decoder_init(&dec_cfg);
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
    ctx->owner = play;

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

    if (play->config.pcm_sink) {
        /* external-sink mode: a PCM tail reader replaces the onboard speaker */
        raw_stream_cfg_t rd_cfg = {
            .type = AUDIO_STREAM_READER,
            .out_block_size = play->config.frame_size,
            .out_block_num  = 8,
            .output_port_type = PORT_TYPE_RB,
        };
        ctx->reader = raw_stream_init(&rd_cfg);
        if (!ctx->reader) {
            goto fail;
        }
        ctx->pump_frame_size = play->config.frame_size;
        ctx->pump_buf = os_malloc(ctx->pump_frame_size);
        if (!ctx->pump_buf) {
            goto fail;
        }
    } else {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        if (spk_service_is_running()) {
            LOGE("%s, multi-source: pcm_sink required (spk_service owns the DAC)\n", __func__);
            goto fail;
        }
#endif
        onboard_speaker_stream_cfg_t spk_cfg = DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG();
        audio_play_build_spk_cfg(&play->config, &spk_cfg);
        ctx->speaker = onboard_speaker_stream_init(&spk_cfg);
        if (!ctx->speaker) {
            goto fail;
        }
    }

#if CONFIG_ADK_EQ_ALGORITHM
    if (play->config.eq_enable) {
        ctx->eq = eq_algorithm_init(&play->config.eq_cfg);
        if (!ctx->eq) {
            LOGE("%s, eq init failed\n", __func__);
            goto fail;
        }
    }
#endif

    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->raw_stream, "raw")) {
        goto fail;
    }
    if (ctx->decoder && BK_OK != audio_pipeline_register(ctx->pipeline, ctx->decoder, "decoder")) {
        goto fail;
    }
#if CONFIG_ADK_EQ_ALGORITHM
    if (ctx->eq && BK_OK != audio_pipeline_register(ctx->pipeline, ctx->eq, "eq")) {
        goto fail;
    }
#endif
    if (ctx->speaker && BK_OK != audio_pipeline_register(ctx->pipeline, ctx->speaker, "speaker")) {
        goto fail;
    }
    if (ctx->reader && BK_OK != audio_pipeline_register(ctx->pipeline, ctx->reader, "reader")) {
        goto fail;
    }

    {
        /* raw -> [decoder] -> [eq] -> speaker|reader */
        const char *link_tag[5];
        int link_num = 0;
        link_tag[link_num++] = "raw";
        if (ctx->decoder) {
            link_tag[link_num++] = "decoder";
        }
#if CONFIG_ADK_EQ_ALGORITHM
        if (ctx->eq) {
            link_tag[link_num++] = "eq";
        }
#endif
        link_tag[link_num++] = ctx->reader ? "reader" : "speaker";
        if (BK_OK != audio_pipeline_link(ctx->pipeline, link_tag, link_num)) {
            goto fail;
        }
    }

    /* Register a listener + drain thread on the pipeline. Elements keep reporting
     * status/info events to the pipeline external queue; if nobody consumes it the
     * queue fills up and floods "no space in external queue". */
    {
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        ctx->listener_evt = audio_event_iface_init(&evt_cfg);
        if (!ctx->listener_evt) {
            goto fail;
        }
        if (BK_OK != audio_pipeline_set_listener(ctx->pipeline, ctx->listener_evt)) {
            goto fail;
        }
        ctx->listener_running = true;
        if (BK_OK != rtos_create_thread(&ctx->listener_thread, AUDIO_PLAY_LISTENER_TASK_PRI,
                                        "aud_play_evt", audio_play_event_listener_task,
                                        AUDIO_PLAY_LISTENER_TASK_STACK, ctx)) {
            ctx->listener_running = false;
            goto fail;
        }
    }

    if (BK_OK != audio_pipeline_run(ctx->pipeline)) {
        goto fail;
    }

    if (ctx->reader) {
        /* Bound the tail read so the pump periodically re-checks pump_running and
         * can exit cleanly even if the upstream source is suspended (no data). */
        audio_element_set_input_timeout(ctx->reader, 30 / portTICK_RATE_MS);
        ctx->pump_eos = false;
        ctx->pump_running = true;
        if (BK_OK != rtos_create_thread(&ctx->pump_thread, AUDIO_PLAY_PUMP_TASK_PRI,
                                        "aud_play_pump", audio_play_pcm_pump_task,
                                        AUDIO_PLAY_PUMP_TASK_STACK, ctx)) {
            ctx->pump_running = false;
            goto fail;
        }
    }

#if CONFIG_ADK_DEBUG_DUMP_UTIL
   aud_dump_cli_init();
#endif

    ctx->state = AUDIO_PLAY_STA_RUNNING;
    play->play_ctx = ctx;
    return BK_OK;

fail:
    if (ctx) {
        if (ctx->pump_thread) {
            ctx->pump_running = false;
            while (ctx->pump_thread) {
                rtos_delay_milliseconds(5);
            }
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
        if (ctx->pipeline) {
            audio_pipeline_deinit(ctx->pipeline);
        }
        if (ctx->speaker) {
            audio_element_deinit(ctx->speaker);
        }
#if CONFIG_ADK_EQ_ALGORITHM
        if (ctx->eq) {
            audio_element_deinit(ctx->eq);
        }
#endif
        if (ctx->reader) {
            audio_element_deinit(ctx->reader);
        }
        if (ctx->decoder) {
            audio_element_deinit(ctx->decoder);
        }
        if (ctx->raw_stream) {
            audio_element_deinit(ctx->raw_stream);
        }
        if (ctx->pump_buf) {
            os_free(ctx->pump_buf);
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

    if (ctx->pump_thread) {
        ctx->pump_running = false;
    }
    if (ctx->pipeline) {
        audio_pipeline_stop(ctx->pipeline);
        audio_pipeline_wait_for_stop(ctx->pipeline);
    }
    if (ctx->pump_thread) {
        while (ctx->pump_thread) {
            rtos_delay_milliseconds(5);
        }
    }
    if (ctx->pipeline) {
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
#if CONFIG_ADK_EQ_ALGORITHM
    if (ctx->pipeline && ctx->eq) {
        audio_pipeline_unregister(ctx->pipeline, ctx->eq);
    }
#endif
    if (ctx->pipeline && ctx->decoder) {
        audio_pipeline_unregister(ctx->pipeline, ctx->decoder);
    }
    if (ctx->pipeline && ctx->reader) {
        audio_pipeline_unregister(ctx->pipeline, ctx->reader);
    }
    if (ctx->pipeline && ctx->raw_stream) {
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
    }

    if (ctx->speaker) {
        audio_element_deinit(ctx->speaker);
    }
#if CONFIG_ADK_EQ_ALGORITHM
    if (ctx->eq) {
        audio_element_deinit(ctx->eq);
    }
#endif
    if (ctx->reader) {
        audio_element_deinit(ctx->reader);
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
    if (ctx->pump_buf) {
        os_free(ctx->pump_buf);
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

bk_err_t audio_play_write_eos(audio_play_t *play)
{
    audio_play_ctx_t *ctx = play ? (audio_play_ctx_t *)play->play_ctx : NULL;

    if (!ctx || !ctx->raw_stream) {
        return BK_FAIL;
    }

    /* Mark the raw source's output done -> downstream decoder gets AEL_IO_DONE,
     * flushes its tail and finishes instead of retrying input-read timeouts. */
    return audio_element_set_port_done(ctx->raw_stream);
}

bool audio_play_pcm_ended(audio_play_t *play)
{
    audio_play_ctx_t *ctx = play ? (audio_play_ctx_t *)play->play_ctx : NULL;

    return ctx ? ctx->pump_eos : false;
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
            return ctx->speaker ? onboard_speaker_stream_dac_mute_en(ctx->speaker, 1) : BK_OK;
        case AUDIO_PLAY_UNMUTE:
            return ctx->speaker ? onboard_speaker_stream_dac_mute_en(ctx->speaker, 0) : BK_OK;
        case AUDIO_PLAY_SET_VOLUME:
            return ctx->speaker ? bk_aud_dac_set_dig_gain_db((float)play->config.volume) : BK_OK;
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

void *audio_play_get_eq(audio_play_t *play)
{
#if CONFIG_ADK_EQ_ALGORITHM
    audio_play_ctx_t *ctx = play ? (audio_play_ctx_t *)play->play_ctx : NULL;
    return ctx ? (void *)ctx->eq : NULL;
#else
    (void)play;
    return NULL;
#endif
}

bk_err_t audio_play_get_pcm_info(audio_play_t *play, uint32_t *sampRate, uint8_t *nChans, uint8_t *bits)
{
    audio_play_ctx_t *ctx = play ? (audio_play_ctx_t *)play->play_ctx : NULL;
    audio_element_info_t info = {0};

    if (!ctx || !ctx->decoder) {
        return BK_FAIL;
    }
    if (audio_element_getinfo(ctx->decoder, &info) != BK_OK) {
        return BK_FAIL;
    }
    if (info.sample_rates <= 0 || info.channels <= 0) {
        return BK_FAIL;
    }

    if (sampRate) {
        *sampRate = (uint32_t)info.sample_rates;
    }
    if (nChans) {
        *nChans = (uint8_t)info.channels;
    }
    if (bits) {
        *bits = (uint8_t)(info.bits ? info.bits : 16);
    }
    return BK_OK;
}

