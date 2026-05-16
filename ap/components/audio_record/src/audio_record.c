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

#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream_v2.h>
#include "audio_record.h"

#define AUDIO_RECORD_TAG "aud_rec"
#define AUDIO_RECORD_LISTENER_TASK_PRI   (4)
#define AUDIO_RECORD_LISTENER_TASK_STACK (2048)

typedef struct {
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t mic;
    audio_element_handle_t raw_stream;
    audio_event_iface_handle_t listener_evt;
    beken_thread_t listener_thread;
    volatile bool listener_running;
    audio_record_sta_t state;
} audio_record_ctx_t;

static void audio_record_event_listener_task(void *arg)
{
    audio_record_ctx_t *ctx = (audio_record_ctx_t *)arg;
    audio_event_iface_msg_t msg = {0};

    while (ctx && ctx->listener_running) {
        (void)audio_event_iface_listen(ctx->listener_evt, &msg, 20 / portTICK_RATE_MS);
    }

    if (ctx) {
        ctx->listener_thread = NULL;
    }
    rtos_delete_thread(NULL);
}

static void audio_record_build_mic_cfg(const audio_record_cfg_t *cfg, onboard_mic_stream_cfg_t *mic_cfg)
{
    mic_cfg->adc_cfg.chl_num     = cfg->nChans;
    mic_cfg->adc_cfg.sample_rate = cfg->sampRate;
    mic_cfg->adc_cfg.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
    mic_cfg->adc_cfg.clk_src = AUD_CLK_APLL;
    mic_cfg->adc_cfg.aec_en = 0;
    mic_cfg->adc_cfg.chl_cfg[0].bits     = cfg->bitsPerSample;
    mic_cfg->adc_cfg.chl_cfg[0].dig_gain = cfg->adc_gain;
    mic_cfg->adc_cfg.chl_cfg[0].ana_gain = 20;
    mic_cfg->adc_cfg.chl_cfg[0].adc_mode = AUD_ADC_MODE_DIFFEN;
    mic_cfg->frame_size     = cfg->frame_size;
    mic_cfg->out_block_size = cfg->frame_size;
    mic_cfg->out_block_num = 2;
    mic_cfg->ch_bitmap = (1 << AUD_ADC_CHL_0);
    mic_cfg->adc_cfg.chl_num = 0;
    for(uint32_t j = 0; j < AUD_ADC_CHL_MAX; j++)
    {
        if(mic_cfg->ch_bitmap & (1 << j))
        {
            mic_cfg->adc_cfg.chl_num++;
        }
    }
}

audio_record_t *audio_record_create(audio_record_type_t record_type, audio_record_cfg_t *config)
{
    if (!config || record_type != AUDIO_RECORD_ONBOARD_MIC) {
        return NULL;
    }

    audio_record_t *record = os_malloc(sizeof(audio_record_t));
    if (!record) {
        return NULL;
    }
    os_memset(record, 0, sizeof(audio_record_t));
    os_memcpy(&record->config, config, sizeof(audio_record_cfg_t));
    return record;
}

bk_err_t audio_record_open(audio_record_t *record)
{
    if (!record) {
        BK_LOGD(AUDIO_RECORD_TAG, "%s, %d, record is NULL.\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (record->record_ctx) {
        BK_LOGD(AUDIO_RECORD_TAG, "%s, %d, audio_record_open already opened.\n", __func__, __LINE__);
        return BK_OK;
    }

    audio_record_ctx_t *ctx = os_malloc(sizeof(audio_record_ctx_t));
    if (!ctx) {
        return BK_FAIL;
    }
    os_memset(ctx, 0x00, sizeof(audio_record_ctx_t));

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    ctx->pipeline = audio_pipeline_init(&pipeline_cfg);
    if (!ctx->pipeline) {
        goto fail;
    }

    onboard_mic_stream_cfg_t mic_cfg = DEFAULT_ONBOARD_MIC_ADC_STREAM_CONFIG();
    audio_record_build_mic_cfg(&record->config, &mic_cfg);
    BK_LOGI(AUDIO_RECORD_TAG, "mic cfg: rate=%d, ch_num=%d, bits0=%d, frame=%d, ch_bitmap=0x%x\n",
            mic_cfg.adc_cfg.sample_rate, mic_cfg.adc_cfg.chl_num, mic_cfg.adc_cfg.chl_cfg[0].bits,
            mic_cfg.frame_size, mic_cfg.ch_bitmap);

    ctx->mic = onboard_mic_stream_init(&mic_cfg);
    if (!ctx->mic) {
        BK_LOGE(AUDIO_RECORD_TAG, "onboard_mic_stream_init failed");
        goto fail;
    }

    raw_stream_cfg_t raw_cfg = {
        .type = AUDIO_STREAM_READER,
        .out_block_size = record->config.frame_size,
        .out_block_num = 4,
        .output_port_type = PORT_TYPE_FB,
    };
    ctx->raw_stream = raw_stream_init(&raw_cfg);
    if (!ctx->raw_stream) {
        goto fail;
    }
    /*
     * Avoid forever blocking in raw_stream_read().
     * HFP stop path waits thread exit by semaphore, so reader must wake up
     * periodically to observe hf_auido_start == 0 and break its loop.
     */
    //audio_element_set_input_timeout(ctx->raw_stream, 20 / portTICK_RATE_MS);

    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->mic, "mic")) {
        goto fail;
    }
    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->raw_stream, "raw")) {
        goto fail;
    }
    if (BK_OK != audio_pipeline_link(ctx->pipeline, (const char *[]) {"mic", "raw"}, 2)) {
        goto fail;
    }
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    ctx->listener_evt = audio_event_iface_init(&evt_cfg);
    if (!ctx->listener_evt) {
        goto fail;
    }
    if (BK_OK != audio_pipeline_set_listener(ctx->pipeline, ctx->listener_evt)) {
        goto fail;
    }
    ctx->listener_running = true;
    if (kNoErr != rtos_create_thread(&ctx->listener_thread,
                                     AUDIO_RECORD_LISTENER_TASK_PRI,
                                     "aud_rec_evt",
                                     (beken_thread_function_t)audio_record_event_listener_task,
                                     AUDIO_RECORD_LISTENER_TASK_STACK,
                                     ctx)) {
        ctx->listener_running = false;
        goto fail;
    }
    if (BK_OK != audio_pipeline_run(ctx->pipeline)) {
        BK_LOGE(AUDIO_RECORD_TAG, "audio_pipeline_run failed\n");
        goto fail;
    }

    ctx->state = AUDIO_RECORD_STA_RUNNING;
    record->record_ctx = ctx;
    BK_LOGI(AUDIO_RECORD_TAG, "audio_record_open success\n");
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
        if (ctx->mic) {
            audio_element_deinit(ctx->mic);
        }
        if (ctx->raw_stream) {
            audio_element_deinit(ctx->raw_stream);
        }
        os_free(ctx);
    }
    return BK_FAIL;
}

bk_err_t audio_record_close(audio_record_t *record)
{
    if (!record) {
        return BK_FAIL;
    }

    audio_record_ctx_t *ctx = (audio_record_ctx_t *)record->record_ctx;
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

    if (ctx->pipeline && ctx->raw_stream) {
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
    }
    if (ctx->pipeline && ctx->mic) {
        audio_pipeline_unregister(ctx->pipeline, ctx->mic);
    }

    if (ctx->raw_stream) {
        audio_element_deinit(ctx->raw_stream);
    }
    if (ctx->mic) {
        audio_element_deinit(ctx->mic);
    }
    if (ctx->pipeline) {
        audio_pipeline_deinit(ctx->pipeline);
    }

    os_free(ctx);
    record->record_ctx = NULL;
    return BK_OK;
}

bk_err_t audio_record_destroy(audio_record_t *record)
{
    if (!record) {
        return BK_OK;
    }

    audio_record_close(record);
    os_free(record);
    return BK_OK;
}

bk_err_t audio_record_read_data(audio_record_t *record, char *buffer, uint32_t len)
{
    if (!record || !buffer || !len) {
        return BK_FAIL;
    }

    audio_record_ctx_t *ctx = (audio_record_ctx_t *)record->record_ctx;
    if (!ctx || !ctx->raw_stream) {
        return BK_FAIL;
    }

    return raw_stream_read(ctx->raw_stream, buffer, len);
}

bk_err_t audio_record_control(audio_record_t *record, audio_record_ctl_t ctl)
{
    if (!record) {
        return BK_FAIL;
    }

    audio_record_ctx_t *ctx = (audio_record_ctx_t *)record->record_ctx;
    if (!ctx) {
        return BK_FAIL;
    }

    switch (ctl) {
        case AUDIO_RECORD_PAUSE:
            return audio_pipeline_pause(ctx->pipeline);
        case AUDIO_RECORD_RESUME:
            return audio_pipeline_resume(ctx->pipeline);
        case AUDIO_RECORD_SET_ADC_GAIN:
            return onboard_mic_stream_set_digital_gain(ctx->mic, record->config.adc_gain, AUD_ADC_CHL_0);
        default:
            return BK_OK;
    }
}

bk_err_t audio_play_set_adc_gain(audio_record_t *record, float value)
{
    if (!record) {
        return BK_FAIL;
    }

    record->config.adc_gain = value;
    return audio_record_control(record, AUDIO_RECORD_SET_ADC_GAIN);
}


