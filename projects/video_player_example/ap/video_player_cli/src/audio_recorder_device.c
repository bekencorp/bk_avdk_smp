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

#include "audio_recorder_device.h"

#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_streams/onboard_mic_stream.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include <components/bk_audio/audio_encoders/g711_encoder.h>
#include <components/bk_audio/audio_encoders/g722_encoder.h>
#include <modules/bk_g722.h>
#include <components/bk_audio/audio_pipeline/audio_event_iface.h>
#include <os/mem.h>
#include <os/str.h>

#define TAG "audio_recorder_device"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/*
 * For encoded formats (AAC/G711/G722), the recorder side may become temporarily slower
 * (e.g. AVI mux + SD card writes). If the encoder output ring buffer is too small,
 * encoded frames will be dropped, resulting in shortened audio duration in the file.
 *
 * NOTE: The ring buffer between encoder -> raw_stream is sized by encoder out_block_*,
 * not by raw_stream (raw_stream is a reader and reads from its input port).
 */
#define AUDIO_RECORDER_ENC_OUT_BLOCK_NUM    (2)

// Event monitoring task stack size and priority
#define AUDIO_RECORDER_EVENT_TASK_STACK     (2048)
#define AUDIO_RECORDER_EVENT_TASK_PRIORITY  (5)

// Audio recorder device context
typedef struct
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t onboard_mic_stream;
    // Optional audio encoder element (AAC/G711/G722). NULL for PCM mode.
    audio_element_handle_t encoder;
    audio_element_handle_t raw_stream;
    audio_recorder_device_cfg_t cfg;
    bool is_started;
    bool is_packetized;   // true for encoded formats, false for PCM
    uint32_t frame_size;  // Input PCM frame size (20ms) in bytes
    // Event monitoring
    audio_event_iface_handle_t event_iface;
    beken_thread_t event_task_handle;
    bool event_task_running;
} audio_recorder_device_ctx_t;

// Event monitoring task function
static void audio_recorder_event_task(void *pvParameters)
{
    audio_recorder_device_ctx_t *ctx = (audio_recorder_device_ctx_t *)pvParameters;
    audio_event_iface_msg_t msg;

    LOGD("%s: Event monitoring task started\n", __func__);

    while (ctx->event_task_running)
    {
        // 500ms timeout for event waiting
        bk_err_t ret = audio_event_iface_listen(ctx->event_iface, &msg, 500 / portTICK_RATE_MS);
        if (ret != BK_OK)
        {
            // Timeout or error, continue to next iteration
            continue;
        }

        // Handle status report events
        if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
        {
            audio_element_status_t status = (audio_element_status_t)(intptr_t)msg.data;
            const char *element_tag = "unknown";
            
            // Get element tag for logging
            if (msg.source != NULL)
            {
                audio_element_handle_t el = (audio_element_handle_t)msg.source;
                element_tag = audio_element_get_tag(el);
            }

            switch (status)
            {
                case AEL_STATUS_ERROR_OUTPUT:
                case AEL_STATUS_ERROR_INPUT:
                    break;

                case AEL_STATUS_ERROR_PROCESS:
                    LOGE("%s: Element [%s] reported PROCESS error\n", __func__, element_tag);
                    break;
                case AEL_STATUS_STATE_STOPPED:
                    //LOGD("%s: Element [%s] stopped\n", __func__, element_tag);
                    break;
                case AEL_STATUS_STATE_FINISHED:
                    LOGD("%s: Element [%s] finished\n", __func__, element_tag);
                    break;
                default:
                    LOGD("%s: Element [%s] status: %d\n", __func__, element_tag, status);
                    break;
            }
        }
        else
        {
            // Handle other event types if needed
            LOGD("%s: Received event cmd=%d from element\n", __func__, msg.cmd);
        }
    }

    LOGD("%s: Event monitoring task stopped\n", __func__);
    // Clear thread handle before exit
    ctx->event_task_handle = NULL;
    rtos_delete_thread(NULL);
}

avdk_err_t audio_recorder_device_init(const audio_recorder_device_cfg_t *cfg, audio_recorder_device_handle_t *handle)
{
    if (cfg == NULL || handle == NULL)
    {
        LOGE("%s: invalid parameter\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Validate configuration parameters
    if (cfg->audio_channels == 0 || cfg->audio_channels > 2)
    {
        LOGE("%s: invalid audio_channels: %u (must be 1 or 2)\n", __func__, cfg->audio_channels);
        return AVDK_ERR_INVAL;
    }

    if (cfg->audio_rate == 0)
    {
        LOGE("%s: invalid audio_rate: %u\n", __func__, cfg->audio_rate);
        return AVDK_ERR_INVAL;
    }

    if (cfg->audio_bits == 0)
    {
        LOGE("%s: invalid audio_bits: %u\n", __func__, cfg->audio_bits);
        return AVDK_ERR_INVAL;
    }

    // Check audio format (input is always PCM from mic, output may be encoded).
    const bool use_pcm = (cfg->audio_format == VIDEO_RECORDER_AUDIO_FORMAT_PCM);
    const bool use_aac = (cfg->audio_format == VIDEO_RECORDER_AUDIO_FORMAT_AAC);
    const bool use_alaw = (cfg->audio_format == VIDEO_RECORDER_AUDIO_FORMAT_ALAW);
    const bool use_ulaw = (cfg->audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MULAW);
    const bool use_g722 = (cfg->audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722);
    const bool use_mp3 = (cfg->audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MP3);

    if (use_mp3)
    {
        // MP3 encoder element is not available in current SDK.
        LOGE("%s: unsupported audio_format: MP3 (no encoder)\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (!(use_pcm || use_aac || use_alaw || use_ulaw || use_g722))
    {
        LOGE("%s: unsupported audio_format: 0x%08x\n", __func__, cfg->audio_format);
        return AVDK_ERR_INVAL;
    }

    if ((use_aac || use_alaw || use_ulaw || use_g722) && cfg->audio_bits != 16)
    {
        // Current encoders expect 16-bit PCM input.
        LOGE("%s: unsupported audio_bits=%u for audio_format=0x%08x (require 16-bit PCM input)\n",
             __func__, cfg->audio_bits, cfg->audio_format);
        return AVDK_ERR_INVAL;
    }

    // Allocate device context
    audio_recorder_device_ctx_t *ctx = (audio_recorder_device_ctx_t *)os_malloc(sizeof(audio_recorder_device_ctx_t));
    if (ctx == NULL)
    {
        LOGE("%s: failed to allocate context\n", __func__);
        return AVDK_ERR_NOMEM;
    }
    os_memset(ctx, 0, sizeof(audio_recorder_device_ctx_t));

    // Copy configuration
    os_memcpy(&ctx->cfg, cfg, sizeof(audio_recorder_device_cfg_t));
    ctx->is_started = false;
    ctx->is_packetized = !use_pcm;
    ctx->encoder = NULL;

    // Initialize audio pipeline
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    ctx->pipeline = audio_pipeline_init(&pipeline_cfg);
    if (ctx->pipeline == NULL)
    {
        LOGE("%s: failed to init audio pipeline\n", __func__);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    bk_err_t ret = BK_OK;

    // Calculate frame size (20ms of audio data)
    uint32_t bytes_per_sample = (cfg->audio_bits / 8);
    if (bytes_per_sample == 0)
    {
        bytes_per_sample = 2;
    }
    uint32_t frame_size = (cfg->audio_rate / 1000) * 20 * bytes_per_sample * cfg->audio_channels;
    ctx->frame_size = frame_size;

    // Initialize onboard mic stream
    onboard_mic_stream_cfg_t mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    mic_cfg.adc_cfg.chl_num = cfg->audio_channels;
    mic_cfg.adc_cfg.sample_rate = cfg->audio_rate;
    mic_cfg.adc_cfg.bits = cfg->audio_bits;
    mic_cfg.frame_size = frame_size;
    mic_cfg.out_block_size = frame_size;
    mic_cfg.out_block_num = 2;
    mic_cfg.multi_out_port_num = 0;
    ctx->onboard_mic_stream = onboard_mic_stream_init(&mic_cfg);
    if (ctx->onboard_mic_stream == NULL)
    {
        LOGE("%s: failed to init onboard mic stream\n", __func__);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Initialize encoder if needed (AAC/G711/G722)
    if (use_aac)
    {
        aac_encoder_cfg_t aac_cfg = DEFAULT_AAC_ENCODER_CONFIG();
        aac_cfg.chl_num = cfg->audio_channels;
        aac_cfg.samp_rate = cfg->audio_rate;
        aac_cfg.bits = cfg->audio_bits;
        aac_cfg.aot = 2;              // AAC-LC
        aac_cfg.sbr_mode = 0;         // Disable SBR to keep AudioSpecificConfig simple (2 bytes)
        aac_cfg.transport_type = 0;   // raw access units (no ADTS header)
        // Calculate output block size (AAC frame size is typically around 1KB)
        aac_cfg.out_block_size = 1024;
        aac_cfg.out_block_num = 2;
        // Update in_pool_len based on current settings (keep formula consistent with reference)
        aac_cfg.in_pool_len = aac_cfg.buffer_len +
                              aac_cfg.samp_rate * aac_cfg.bits / 8 * aac_cfg.chl_num / 1000 * 20;
        ctx->encoder = aac_encoder_init(&aac_cfg);
        if (ctx->encoder == NULL)
        {
            LOGE("%s: failed to init aac encoder\n", __func__);
            audio_element_deinit(ctx->onboard_mic_stream);
            audio_pipeline_deinit(ctx->pipeline);
            os_free(ctx);
            return AVDK_ERR_HWERROR;
        }
    }
    else if (use_alaw || use_ulaw)
    {
        g711_encoder_cfg_t g711_cfg = DEFAULT_G711_ENCODER_CONFIG();
        g711_cfg.enc_mode = use_ulaw ? G711_ENC_MODE_U_LOW : G711_ENC_MODE_A_LOW;
        // Increase encoder output buffering to reduce frame drops when consumer is slower.
        g711_cfg.out_block_num = AUDIO_RECORDER_ENC_OUT_BLOCK_NUM;
        ctx->encoder = g711_encoder_init(&g711_cfg);
        if (ctx->encoder == NULL)
        {
            LOGE("%s: failed to init g711 encoder\n", __func__);
            audio_element_deinit(ctx->onboard_mic_stream);
            audio_pipeline_deinit(ctx->pipeline);
            os_free(ctx);
            return AVDK_ERR_HWERROR;
        }
    }
    else if (use_g722)
    {
        g722_encoder_cfg_t g722_cfg = DEFAULT_G722_ENCODER_CONFIG();
        g722_cfg.enc_rate = G722_ENC_RATE_64000;
        g722_cfg.options = (cfg->audio_rate == 8000) ? G722_SAMPLE_RATE_8000 : 0;
        // Increase encoder output buffering to reduce frame drops when consumer is slower.
        g722_cfg.out_block_num = AUDIO_RECORDER_ENC_OUT_BLOCK_NUM;
        ctx->encoder = g722_encoder_init(&g722_cfg);
        if (ctx->encoder == NULL)
        {
            LOGE("%s: failed to init g722 encoder\n", __func__);
            audio_element_deinit(ctx->onboard_mic_stream);
            audio_pipeline_deinit(ctx->pipeline);
            os_free(ctx);
            return AVDK_ERR_HWERROR;
        }
    }

    // Initialize raw stream (reader) with PORT_TYPE_RB for reading
    // Note: raw_stream_read reads from input port, not output port
    // The output port type doesn't affect reading behavior for AUDIO_STREAM_READER
    raw_stream_cfg_t raw_stream_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_stream_cfg.type = AUDIO_STREAM_READER;
    raw_stream_cfg.output_port_type = PORT_TYPE_RB;  // Use ring buffer port (output port type doesn't affect reading)
    // Set block size based on output format.
    if (use_aac)
    {
        // AAC frame size is variable; use a larger buffer to accommodate.
        raw_stream_cfg.out_block_size = 2048;
    }
    else if (use_alaw || use_ulaw || use_g722)
    {
        // For 8kHz 20ms:
        // - G711 output: 160 bytes (8-bit)
        // - G722 output: ~160 bytes at 64kbps
        raw_stream_cfg.out_block_size = 256;
    }
    else
    {
        raw_stream_cfg.out_block_size = frame_size;
    }
    raw_stream_cfg.out_block_num = 2;  // Use 2 blocks for buffering
    ctx->raw_stream = raw_stream_init(&raw_stream_cfg);
    if (ctx->raw_stream == NULL)
    {
        LOGE("%s: failed to init raw stream\n", __func__);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    audio_element_set_input_timeout(ctx->raw_stream, 5);

    // Register elements to pipeline
    ret = audio_pipeline_register(ctx->pipeline, ctx->onboard_mic_stream, "onboard_mic");
    if (ret != BK_OK)
    {
        LOGE("%s: failed to register onboard_mic, ret=%d\n", __func__, ret);
        audio_element_deinit(ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    if (ctx->encoder != NULL)
    {
        const char *tag = use_aac ? "aac_encoder" : (use_g722 ? "g722_encoder" : "g711_encoder");
        ret = audio_pipeline_register(ctx->pipeline, ctx->encoder, tag);
        if (ret != BK_OK)
        {
            LOGE("%s: failed to register encoder, ret=%d\n", __func__, ret);
            audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
            audio_element_deinit(ctx->raw_stream);
            audio_element_deinit(ctx->encoder);
            audio_element_deinit(ctx->onboard_mic_stream);
            audio_pipeline_deinit(ctx->pipeline);
            os_free(ctx);
            return AVDK_ERR_HWERROR;
        }
    }

    ret = audio_pipeline_register(ctx->pipeline, ctx->raw_stream, "raw_stream");
    if (ret != BK_OK)
    {
        LOGE("%s: failed to register raw_stream, ret=%d\n", __func__, ret);
        if (ctx->encoder != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->encoder);
        }
        audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
        audio_element_deinit(ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Link elements based on format
    if (ctx->encoder != NULL)
    {
        const char *enc_tag = use_aac ? "aac_encoder" : (use_g722 ? "g722_encoder" : "g711_encoder");
        // onboard_mic -> encoder -> raw_stream
        const char *link_tag[] = {"onboard_mic", enc_tag, "raw_stream"};
        ret = audio_pipeline_link(ctx->pipeline, link_tag, 3);
    }
    else
    {
        // onboard_mic -> raw_stream
        const char *link_tag[] = {"onboard_mic", "raw_stream"};
        ret = audio_pipeline_link(ctx->pipeline, link_tag, 2);
    }

    if (ret != BK_OK)
    {
        LOGE("%s: failed to link pipeline, ret=%d\n", __func__, ret);
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->encoder);
        }
        audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
        audio_element_deinit(ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Initialize event interface for pipeline monitoring
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    ctx->event_iface = audio_event_iface_init(&evt_cfg);
    if (ctx->event_iface == NULL)
    {
        LOGE("%s: failed to init event interface\n", __func__);
        audio_pipeline_unlink(ctx->pipeline);
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->encoder);
        }
        audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
        audio_element_deinit(ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Set pipeline listener
    ret = audio_pipeline_set_listener(ctx->pipeline, ctx->event_iface);
    if (ret != BK_OK)
    {
        LOGE("%s: failed to set pipeline listener, ret=%d\n", __func__, ret);
        audio_event_iface_destroy(ctx->event_iface);
        ctx->event_iface = NULL;
        audio_pipeline_unlink(ctx->pipeline);
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->encoder);
        }
        audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
        audio_element_deinit(ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    // Create event monitoring task (use OS abstraction, do not use FreeRTOS API directly)
    ctx->event_task_running = true;
    bk_err_t task_ret = rtos_create_thread(
        &ctx->event_task_handle,
        AUDIO_RECORDER_EVENT_TASK_PRIORITY,
        "aud_rec_evt",
        (beken_thread_function_t)audio_recorder_event_task,
        AUDIO_RECORDER_EVENT_TASK_STACK,
        (beken_thread_arg_t)ctx
    );
    if (task_ret != BK_OK)
    {
        LOGE("%s: failed to create event task\n", __func__);
        ctx->event_task_running = false;
        audio_pipeline_remove_listener(ctx->pipeline);
        audio_event_iface_destroy(ctx->event_iface);
        ctx->event_iface = NULL;
        audio_pipeline_unlink(ctx->pipeline);
        audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->encoder);
        }
        audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
        audio_element_deinit(ctx->raw_stream);
        if (ctx->encoder != NULL)
        {
            audio_element_deinit(ctx->encoder);
        }
        audio_element_deinit(ctx->onboard_mic_stream);
        audio_pipeline_deinit(ctx->pipeline);
        os_free(ctx);
        return AVDK_ERR_HWERROR;
    }

    *handle = (audio_recorder_device_handle_t)ctx;
    LOGI("%s: initialized successfully, ch=%u, rate=%u, bits=%u, format=0x%08x (%s)\n",
         __func__, cfg->audio_channels, cfg->audio_rate, cfg->audio_bits, cfg->audio_format,
         use_aac ? "AAC" : (use_g722 ? "G722" : ((use_alaw || use_ulaw) ? "G711" : "PCM")));

    return AVDK_ERR_OK;
}

avdk_err_t audio_recorder_device_deinit(audio_recorder_device_handle_t handle)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_recorder_device_ctx_t *ctx = (audio_recorder_device_ctx_t *)handle;

    // Stop if started
    if (ctx->is_started)
    {
        audio_recorder_device_stop(handle);
    }

    // Stop event monitoring task
    if (ctx->event_task_running)
    {
        ctx->event_task_running = false;
        // Wait for task to finish (with timeout)
        if (ctx->event_task_handle != NULL)
        {
            vTaskDelay(100 / portTICK_RATE_MS);  // Give task time to exit
            // Task will delete itself, just clear the handle
            ctx->event_task_handle = NULL;
        }
    }

    // Remove pipeline listener and destroy event interface
    if (ctx->pipeline != NULL && ctx->event_iface != NULL)
    {
        audio_pipeline_remove_listener(ctx->pipeline);
    }
    if (ctx->event_iface != NULL)
    {
        audio_event_iface_destroy(ctx->event_iface);
        ctx->event_iface = NULL;
    }

    // Clean up pipeline and elements
    if (ctx->pipeline != NULL)
    {
        // Unlink pipeline first
        audio_pipeline_unlink(ctx->pipeline);

        // Unregister elements
        if (ctx->raw_stream != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->raw_stream);
        }
        if (ctx->encoder != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->encoder);
        }
        if (ctx->onboard_mic_stream != NULL)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->onboard_mic_stream);
        }

        // Deinitialize pipeline
        audio_pipeline_deinit(ctx->pipeline);
        ctx->pipeline = NULL;
    }

    // Deinitialize elements
    if (ctx->raw_stream != NULL)
    {
        audio_element_deinit(ctx->raw_stream);
        ctx->raw_stream = NULL;
    }
    if (ctx->encoder != NULL)
    {
        audio_element_deinit(ctx->encoder);
        ctx->encoder = NULL;
    }
    if (ctx->onboard_mic_stream != NULL)
    {
        audio_element_deinit(ctx->onboard_mic_stream);
        ctx->onboard_mic_stream = NULL;
    }

    os_free(ctx);
    LOGI("%s: deinitialized\n", __func__);

    return AVDK_ERR_OK;
}

avdk_err_t audio_recorder_device_start(audio_recorder_device_handle_t handle)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_recorder_device_ctx_t *ctx = (audio_recorder_device_ctx_t *)handle;

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

avdk_err_t audio_recorder_device_stop(audio_recorder_device_handle_t handle)
{
    if (handle == NULL)
    {
        LOGE("%s: invalid handle\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_recorder_device_ctx_t *ctx = (audio_recorder_device_ctx_t *)handle;

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

avdk_err_t audio_recorder_device_read(audio_recorder_device_handle_t handle, uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len)
{
    if (handle == NULL || buffer == NULL || buffer_size == 0 || data_len == NULL)
    {
        LOGE("%s: invalid parameter\n", __func__);
        return AVDK_ERR_INVAL;
    }

    audio_recorder_device_ctx_t *ctx = (audio_recorder_device_ctx_t *)handle;

    if (!ctx->is_started)
    {
        LOGE("%s: device not started\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // Determine read size based on format
    uint32_t read_size = buffer_size;
    if (!ctx->is_packetized)
    {
        // For PCM: read one frame (20ms of audio data)
        read_size = (ctx->frame_size < buffer_size) ? ctx->frame_size : buffer_size;
    }
    else
    {
        // For AAC: read one frame (typically around 1KB, but use buffer_size if smaller)
        // AAC frames are variable size, so we read what's available up to buffer_size
        read_size = buffer_size;
    }

    // Read one frame from raw_stream
    int read_len = raw_stream_read(ctx->raw_stream, (char *)buffer, (int)read_size);
    if (read_len > 0)
    {
        *data_len = (uint32_t)read_len;
        return AVDK_ERR_OK;
    }
    else if (read_len == AEL_IO_DONE || read_len == AEL_IO_OK)
    {
        // End of stream
        *data_len = 0;
        return AVDK_ERR_EOF;
    }
    else if (read_len == AEL_IO_TIMEOUT)
    {
        // Timeout, no data available
        *data_len = 0;
        return AVDK_ERR_TIMEOUT;
    }
    else
    {
        // Error
        LOGE("%s: raw_stream_read failed, ret=%d\n", __func__, read_len);
        *data_len = 0;
        return AVDK_ERR_HWERROR;
    }
}

