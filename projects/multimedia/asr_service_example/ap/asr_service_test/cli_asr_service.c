// Copyright 2025-2026 Beken
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
#include <os/str.h>
#include <components/log.h>
#include "cli.h"

#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>

#if CONFIG_BEKEN_KWS
#include "bk_kws_asr.h"
#endif

#if (CONFIG_VOICE_SERVICE)
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#if CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
#include <components/bk_audio/audio_algorithms/aec_v3_algorithm_v2.h>
#endif
#if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
#include <components/bk_audio/audio_streams/onboard_mic_stream_v2.h>
#endif
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#endif
#endif

#define TAG "asr_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

static asr_handle_t gl_asr_service_handle = NULL;
static aud_asr_handle_t gl_aud_asr_service_handle = NULL;

#if CONFIG_BEKEN_KWS
static const char *g_asr_text = NULL;
static float g_asr_score = 0.0f;
#endif

#if (CONFIG_VOICE_SERVICE)
static voice_handle_t gl_voice_service_handle = NULL;
static voice_read_handle_t gl_voice_read_service_handle = NULL;
static voice_write_handle_t gl_voice_write_service_handle = NULL;
static beken_semaphore_t voice_start_sem = NULL;

int voice_service_send_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = bk_voice_write_frame_data(gl_voice_write_service_handle, (char *)data, len);
    if (ret != len)
    {
        LOGV("%s, %d, bk_voice_write_frame_data: %d != %d\n", __func__, __LINE__, ret, len);
    }

    if (voice_start_sem)
    {
        LOGD("%s, %d, get mic data, set semaphore\n", __func__, __LINE__);
        rtos_set_semaphore(&voice_start_sem);
    }
    return len;
}
#endif

static void bk_asr_service_result_handle(void *p1, void *p2)
{
#if CONFIG_BK7259_ASR_DEBUG
    return;
#endif
    const char *result = NULL;

    if (p1 != NULL) {
        result = *((char **)p1);
    }
    if (result == NULL) {
        LOGE("ASR result is NULL\n");
        return;
    }
    (void)p2;

#if CONFIG_BEKEN_KWS
    if (os_strcmp(result, "nihaobotong") == 0)
    {
        LOGI("nihaobotong, cmd: 1\r\n");
    }
    else if (os_strcmp(result, "zaijianbotong") == 0)
    {
        LOGI("zaijianbotong, cmd: 2\r\n");
    }
    else if (os_strcmp(result, "Play Music") == 0)
    {
        LOGI("play music, cmd: 3\r\n");
    }
    else if (os_strcmp(result, "Stop Play") == 0)
    {
        LOGI("stop play, cmd: 4\r\n");
    }
    else if (os_strcmp(result, "Next song") == 0)
    {
        LOGI("next song, cmd: 5\r\n");
    }
    else if (os_strcmp(result, "Volume Up") == 0)
    {
        LOGI("volume up, cmd: 6\r\n");
    }
    else if (os_strcmp(result, "Volume Down") == 0)
    {
        LOGI("volume down, cmd: 7\r\n");
    }
    else
    {
        LOGI("asr result: %s\r\n", result);
    }
#else
    LOGW("Need open CONFIG_BEKEN_KWS. result: %s\n", result);
#endif
}

static void bk_cleanup_asr_resources(void)
{
    if (gl_aud_asr_service_handle)
    {
        bk_aud_asr_stop(gl_aud_asr_service_handle);
    }
    if (gl_asr_service_handle)
    {
        bk_asr_stop(gl_asr_service_handle);
    }

    if (gl_aud_asr_service_handle)
    {
        bk_aud_asr_deinit(gl_aud_asr_service_handle);
        gl_aud_asr_service_handle = NULL;
    }
    if (gl_asr_service_handle)
    {
        bk_asr_deinit(gl_asr_service_handle);
        gl_asr_service_handle = NULL;
    }

#if (CONFIG_VOICE_SERVICE)
    if (gl_voice_read_service_handle)
    {
        bk_voice_read_stop(gl_voice_read_service_handle);
    }
    if (gl_voice_write_service_handle)
    {
        bk_voice_write_stop(gl_voice_write_service_handle);
    }
    if (gl_voice_service_handle)
    {
        bk_voice_stop(gl_voice_service_handle);
    }

    if (gl_voice_read_service_handle)
    {
        bk_voice_read_deinit(gl_voice_read_service_handle);
        gl_voice_read_service_handle = NULL;
    }
    if (gl_voice_write_service_handle)
    {
        bk_voice_write_deinit(gl_voice_write_service_handle);
        gl_voice_write_service_handle = NULL;
    }
    if (gl_voice_service_handle)
    {
        bk_voice_deinit(gl_voice_service_handle);
        gl_voice_service_handle = NULL;
    }

    if (voice_start_sem)
    {
        rtos_deinit_semaphore(&voice_start_sem);
        voice_start_sem = NULL;
    }
#endif
}

static int bk_init_audio_asr_service(asr_handle_t asr_handle)
{
    aud_asr_cfg_t aud_asr_cfg = (aud_asr_cfg_t)AUDIO_ASR_CFG_DEFAULT();
    aud_asr_cfg.asr_handle = asr_handle;
    aud_asr_cfg.aud_asr_result_handle = bk_asr_service_result_handle;

#if CONFIG_BEKEN_KWS
    aud_asr_cfg.aud_asr_init   = bk_tflite_asr_init;
    aud_asr_cfg.aud_asr_deinit = bk_tflite_asr_deinit;
    aud_asr_cfg.aud_asr_recog  = bk_tflite_asr_recog;
    aud_asr_cfg.p1 = (void *)&g_asr_text;
    aud_asr_cfg.p2 = (void *)&g_asr_score;
    aud_asr_cfg.max_read_size = 1280;
    aud_asr_cfg.task_stack = 25 * 1024;
    aud_asr_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
#else
    LOGW("Need open CONFIG_BEKEN_KWS. line: %d\n", __LINE__);
    return BK_FAIL;
#endif

    gl_aud_asr_service_handle = bk_aud_asr_init(&aud_asr_cfg);
    if (!gl_aud_asr_service_handle)
    {
        LOGE("aud asr init fail\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static bool bk_setup_asr_resample_config(asr_cfg_t *asr_cfg, uint32_t mic_sample_rate)
{
    asr_cfg->asr_en = true;

    if (mic_sample_rate != asr_cfg->asr_sample_rate)
    {
#if CONFIG_ADK_RSP_ALGORITHM
        asr_cfg->asr_rsp_en = true;
        asr_cfg->rsp_cfg.rsp_alg_cfg.rsp_cfg.src_rate = mic_sample_rate;
        return true;
#else
        asr_cfg->asr_rsp_en = false;
        LOGE("Need Open the aud resample Macro\n");
        return false;
#endif
    }

    asr_cfg->asr_rsp_en = false;
    return true;
}

static void bk_setup_asr_read_pool_size(asr_cfg_t *asr_cfg, uint32_t mic_sample_rate)
{
    if (mic_sample_rate == 16000) {
        asr_cfg->read_pool_size = mic_sample_rate * 2 * 20 / 1000;
    }
    else if (mic_sample_rate == 8000) {
        asr_cfg->read_pool_size = 2 * mic_sample_rate * 2 * 20 / 1000;
    }
}

static void bk_setup_onboard_mic_channels(asr_cfg_t *asr_cfg, bool aec_en)
{
#if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
    asr_cfg->mic_cfg.onboard_mic_cfg.ch_bitmap = (1 << AUD_ADC_CHL_0);
    asr_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = 0;
    for (uint32_t j = 0; j < AUD_ADC_CHL_MAX; j++)
    {
        if (asr_cfg->mic_cfg.onboard_mic_cfg.ch_bitmap & (1 << j))
        {
            asr_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.chl_num++;
        }
    }
    asr_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.aec_en = aec_en;
#else
    (void)asr_cfg;
    (void)aec_en;
#endif
}

static void bk_setup_asr_aec_config(asr_cfg_t *asr_cfg, bool aec_en, bool is_uac)
{
    if (aec_en && !is_uac)
    {
        asr_cfg->aec_en = true;
        asr_cfg->aec_cfg.aec_alg_cfg.aec_cfg.mode    = AEC_MODE_HARDWARE;
        asr_cfg->aec_cfg.aec_alg_cfg.aec_cfg.ns_type = NS_TRADITION;
        asr_cfg->aec_cfg.aec_alg_cfg.dual_ch            = 0;
        asr_cfg->aec_cfg.aec_alg_cfg.multi_in_port_num  = 0;
        asr_cfg->aec_cfg.aec_alg_cfg.vad_cfg.vad_enable = 0;
        asr_cfg->aec_cfg.aec_alg_cfg.aec_cfg.ec_only_output = 1;
        asr_cfg->aec_cfg.aec_alg_cfg.aec_cfg.multi_output_use_ec_out = 1;
        asr_cfg->aec_cfg.aec_alg_cfg.out_block_num = 4;
    }
    else
    {
        asr_cfg->aec_en = false;
    }
}

static bool bk_parse_aec_enable(int argc, char **argv, int index)
{
    if (argc <= index || argv[index] == NULL)
    {
        return false;
    }

    if (os_strcmp(argv[index], "aec") == 0)
    {
        return true;
    }

    return (os_strtoul(argv[index], NULL, 10) != 0);
}

#if (CONFIG_VOICE_SERVICE)
static bool bk_setup_voice_cfg_for_asr(voice_cfg_t *voice_cfg, bool is_uac, uint32_t mic_sample_rate, bool aec_en)
{
    uint32_t frame_bytes = mic_sample_rate * 2 * 20 / 1000;

    /* Align with player_service_example voice start path. */
    if (is_uac)
    {
        *voice_cfg = (voice_cfg_t)DEFAULT_VOICE_BY_UAC_MIC_SPK_CONFIG();
        voice_cfg->mic_cfg.uac_mic_cfg.samp_rate  = mic_sample_rate;
        voice_cfg->mic_cfg.uac_mic_cfg.frame_size = frame_bytes;
        voice_cfg->mic_cfg.uac_mic_cfg.out_block_size = frame_bytes;
        voice_cfg->mic_cfg.uac_mic_cfg.out_block_num  = 2;
        voice_cfg->spk_cfg.uac_spk_cfg.samp_rate  = mic_sample_rate;
        voice_cfg->spk_cfg.uac_spk_cfg.frame_size = frame_bytes;
    }
    else if (mic_sample_rate == 16000)
    {
        *voice_cfg = (voice_cfg_t)DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AEC_G711A_16000_CONFIG();
    }
    else
    {
        *voice_cfg = (voice_cfg_t)DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
        voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = mic_sample_rate;
        voice_cfg->mic_cfg.onboard_mic_cfg.frame_size          = frame_bytes;
        voice_cfg->mic_cfg.onboard_mic_cfg.out_block_size      = frame_bytes;
    }

    /* ASR taps mic/AEC multi-output; voice encode still uses the main path. */
    if (is_uac)
    {
        voice_cfg->mic_cfg.uac_mic_cfg.multi_out_port_num = aec_en ? 0 : 1;
        voice_cfg->spk_cfg.uac_spk_cfg.multi_out_port_num = aec_en ? 1 : 0;
    }
    else
    {
#if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
        voice_cfg->mic_cfg.onboard_mic_cfg.ch_bitmap = (1 << AUD_ADC_CHL_0);
        voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = 0;
        for (uint32_t j = 0; j < AUD_ADC_CHL_MAX; j++)
        {
            if (voice_cfg->mic_cfg.onboard_mic_cfg.ch_bitmap & (1 << j))
            {
                voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.chl_num++;
            }
        }
        voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.aec_en = aec_en;
#endif
        voice_cfg->mic_cfg.onboard_mic_cfg.multi_out_port_num = aec_en ? 0 : 1;
        voice_cfg->spk_cfg.onboard_spk_cfg.multi_out_port_num = aec_en ? 1 : 0;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
        /* Voice decode feeds CALL source (same as player_service_example). */
        voice_cfg->spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
        voice_cfg->spk_cfg.onboard_spk_cfg.main_dac_source   = AUD_DAC_SOURCE_CALL;
        voice_cfg->spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_CALL] = mic_sample_rate;
        voice_cfg->spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_CALL]  = frame_bytes;
#endif
    }

    if (aec_en)
    {
        voice_cfg->aec_en = true;
        voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
        voice_cfg->aec_cfg.aec_alg_cfg.out_block_size = frame_bytes;
        voice_cfg->aec_cfg.aec_alg_cfg.out_block_num = 1;
        /* ASR reads post-AEC PCM from AEC multi-output. */
        voice_cfg->aec_cfg.aec_alg_cfg.multi_out_port_num = 1;

        if (is_uac)
        {
            voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
            voice_cfg->aec_cfg.aec_alg_cfg.dual_ch = 0;
            voice_cfg->aec_cfg.aec_alg_cfg.multi_in_port_num = 1;
        }
        else
        {
            /* BK7259 onboard: hardware AEC + single mic, same as player_service_example. */
            voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.mode = AEC_MODE_HARDWARE;
            voice_cfg->aec_cfg.aec_alg_cfg.dual_ch = 0;
            voice_cfg->aec_cfg.aec_alg_cfg.multi_in_port_num = 0;
        }
    }
    else
    {
        voice_cfg->aec_en = false;
    }

    voice_cfg->event_handle = NULL;
    voice_cfg->args = NULL;
    return true;
}
#endif

static void cli_asr_service_print_usage(void)
{
    LOGI("Usage:\r\n");
    LOGI("  asr_service startwithmic <onboard|uac> <8000|16000> [aec_en]\r\n");
    LOGI("  asr_service startnomic <onboard|uac> <8000|16000> [aec_en]\r\n");
    LOGI("  asr_service stop\r\n");
}

void cli_asr_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    asr_cfg_t asr_cfg = {0};
    uint32_t mic_sample_rate = 0;
    bool aec_en = false;
    bool is_uac = false;

    (void)xWriteBufferLen;

    if (argc < 2)
    {
        LOGE("%s, %d, invalid argument count: %d\n", __func__, __LINE__, argc);
        cli_asr_service_print_usage();
        goto exit;
    }

    if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGD("asr stop\n");
        bk_cleanup_asr_resources();
        msg = CLI_CMD_RSP_SUCCEED;
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }

    if (argc < 4 || argv[2] == NULL || argv[3] == NULL)
    {
        LOGE("%s, %d, invalid argument count: %d\n", __func__, __LINE__, argc);
        cli_asr_service_print_usage();
        goto exit;
    }

    if (gl_asr_service_handle || gl_aud_asr_service_handle)
    {
        LOGE("asr already started, please stop first\n");
        goto exit;
    }

    mic_sample_rate = os_strtoul(argv[3], NULL, 10);
    if (mic_sample_rate != 8000 && mic_sample_rate != 16000)
    {
        LOGE("%s, %d, unsupported mic_samp_rate: %u\n", __func__, __LINE__, mic_sample_rate);
        goto exit;
    }

    aec_en = bk_parse_aec_enable(argc, argv, 4);
    is_uac = (os_strcmp(argv[2], "uac") == 0);

    if (os_strcmp(argv[1], "startwithmic") == 0)
    {
        LOGD("%s, %d, startwithmic, mic_type: %s, mic_samp_rate: %u, aec_en: %d\n",
             __func__, __LINE__, argv[2], mic_sample_rate, aec_en);

        if (os_strcmp(argv[2], "onboard") == 0)
        {
            asr_cfg_t asr_cfg_onboard = (asr_cfg_t)ASR_BY_ONBOARD_MIC_CFG_DEFAULT();
            asr_cfg_onboard.mic_type = MIC_TYPE_ONBOARD;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = mic_sample_rate;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.out_block_size = asr_cfg_onboard.mic_cfg.onboard_mic_cfg.frame_size;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.out_block_num = 4;
            asr_cfg = asr_cfg_onboard;
            bk_setup_onboard_mic_channels(&asr_cfg, aec_en);
        }
        else if (is_uac)
        {
            asr_cfg_t asr_cfg_uac = (asr_cfg_t)ASR_BY_UAC_MIC_CFG_DEFAULT();
            asr_cfg_uac.mic_type = MIC_TYPE_UAC;
            asr_cfg_uac.mic_cfg.uac_mic_cfg.samp_rate = mic_sample_rate;
            asr_cfg_uac.mic_cfg.uac_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000;
            asr_cfg_uac.mic_cfg.uac_mic_cfg.out_block_size = asr_cfg_uac.mic_cfg.uac_mic_cfg.frame_size;
            asr_cfg_uac.mic_cfg.uac_mic_cfg.out_block_num = 4;
            asr_cfg = asr_cfg_uac;
        }
        else
        {
            LOGE("%s, %d, unsupported mic_type: %s\n", __func__, __LINE__, argv[2]);
            goto exit;
        }

        if (!bk_setup_asr_resample_config(&asr_cfg, mic_sample_rate))
        {
            goto exit;
        }

        bk_setup_asr_aec_config(&asr_cfg, aec_en, is_uac);

        asr_cfg.event_handle = NULL;
        asr_cfg.args = NULL;
        gl_asr_service_handle = bk_asr_create(&asr_cfg);
        if (!gl_asr_service_handle)
        {
            LOGE("asr create fail\n");
            goto exit;
        }

        bk_setup_asr_read_pool_size(&asr_cfg, mic_sample_rate);
        bk_asr_init_with_mic(&asr_cfg, gl_asr_service_handle);

        if (bk_init_audio_asr_service(gl_asr_service_handle) != BK_OK)
        {
            goto exit;
        }

        if (BK_OK != bk_asr_start(gl_asr_service_handle))
        {
            LOGE("%s, %d, asr start fail\n", __func__, __LINE__);
            goto exit;
        }

        if (BK_OK != bk_aud_asr_start(gl_aud_asr_service_handle))
        {
            LOGE("%s, %d, aud_asr start fail\n", __func__, __LINE__);
            goto exit;
        }
    }
    else if (os_strcmp(argv[1], "startnomic") == 0)
    {
#if (CONFIG_VOICE_SERVICE)
        LOGD("%s, %d, startnomic, mic_type: %s, mic_samp_rate: %u, aec_en: %d\n",
             __func__, __LINE__, argv[2], mic_sample_rate, aec_en);

        voice_cfg_t voice_cfg = {0};

        if ((os_strcmp(argv[2], "onboard") != 0) && !is_uac)
        {
            LOGE("%s, %d, unsupported mic_type: %s\n", __func__, __LINE__, argv[2]);
            goto exit;
        }

        if (!bk_setup_voice_cfg_for_asr(&voice_cfg, is_uac, mic_sample_rate, aec_en))
        {
            goto exit;
        }

        asr_cfg = is_uac ? (asr_cfg_t)ASR_BY_UAC_MIC_CFG_DEFAULT()
                         : (asr_cfg_t)ASR_BY_ONBOARD_MIC_CFG_DEFAULT();

        if (!bk_setup_asr_resample_config(&asr_cfg, mic_sample_rate))
        {
            goto exit;
        }

        gl_voice_service_handle = bk_voice_init(&voice_cfg);
        if (!gl_voice_service_handle)
        {
            LOGE("%s, %d, voice init fail\n", __func__, __LINE__);
            goto exit;
        }

        asr_cfg.event_handle = NULL;
        asr_cfg.args = NULL;
        gl_asr_service_handle = bk_asr_create(&asr_cfg);
        if (!gl_asr_service_handle)
        {
            LOGE("asr create fail\n");
            goto exit;
        }

        gl_asr_service_handle->mic_str = (audio_element_handle_t)bk_voice_get_mic_str(gl_voice_service_handle, &voice_cfg);
        if (!gl_asr_service_handle->mic_str)
        {
            LOGE("get mic str fail\n");
            goto exit;
        }

        bk_setup_asr_read_pool_size(&asr_cfg, mic_sample_rate);
        bk_asr_init(&asr_cfg, gl_asr_service_handle);
        if (!gl_asr_service_handle)
        {
            LOGE("asr init fail\n");
            goto exit;
        }

        if (bk_init_audio_asr_service(gl_asr_service_handle) != BK_OK)
        {
            goto exit;
        }

        voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
        voice_read_cfg.voice_handle = gl_voice_service_handle;
        voice_read_cfg.voice_read_callback = voice_service_send_callback;
        gl_voice_read_service_handle = bk_voice_read_init(&voice_read_cfg);
        if (!gl_voice_read_service_handle)
        {
            LOGE("%s, %d, voice read init fail\n", __func__, __LINE__);
            goto exit;
        }

        voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
        voice_write_cfg.voice_handle = gl_voice_service_handle;
        gl_voice_write_service_handle = bk_voice_write_init(&voice_write_cfg);
        if (!gl_voice_write_service_handle)
        {
            LOGE("%s, %d, voice write init fail\n", __func__, __LINE__);
            goto exit;
        }

        if (BK_OK != bk_voice_start(gl_voice_service_handle))
        {
            LOGE("%s, %d, voice start fail\n", __func__, __LINE__);
            goto exit;
        }

        if (BK_OK != bk_voice_read_start(gl_voice_read_service_handle))
        {
            LOGE("%s, %d, voice read start fail\n", __func__, __LINE__);
            goto exit;
        }

        if (BK_OK != bk_voice_write_start(gl_voice_write_service_handle))
        {
            LOGE("%s, %d, voice write start fail\n", __func__, __LINE__);
            goto exit;
        }

        if (BK_OK != rtos_init_semaphore(&voice_start_sem, 1))
        {
            LOGE("%s, %d, create semaphore fail\n", __func__, __LINE__);
            goto exit;
        }

        LOGI("waiting for voice service to start (timeout: 5s)...\n");
        bk_err_t ret = rtos_get_semaphore(&voice_start_sem, 5000);
        if (ret == BK_OK)
        {
            LOGI("voice service started successfully!\n");
            rtos_deinit_semaphore(&voice_start_sem);
            voice_start_sem = NULL;
        }
        else
        {
            LOGE("%s, %d, voice service start timeout, callback not triggered\n", __func__, __LINE__);
            rtos_deinit_semaphore(&voice_start_sem);
            voice_start_sem = NULL;
            goto exit;
        }

        if (BK_OK != bk_asr_start(gl_asr_service_handle))
        {
            LOGE("asr start fail\n");
            goto exit;
        }

        if (BK_OK != bk_aud_asr_start(gl_aud_asr_service_handle))
        {
            LOGE("aud asr start fail\n");
            goto exit;
        }
#else
        LOGE("%s, %d, voice service not supported\n", __func__, __LINE__);
        goto exit;
#endif
    }
    else
    {
        LOGE("%s, %d, unsupported command: %s\n", __func__, __LINE__, argv[1]);
        cli_asr_service_print_usage();
        goto exit;
    }

    LOGD("%s ---complete\n", __func__);
    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

exit:
    bk_cleanup_asr_resources();
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define ASR_SERVICE_CMD_CNT   (sizeof(s_asr_service_commands) / sizeof(struct cli_command))

static const struct cli_command s_asr_service_commands[] =
{
    {"asr_service",
     "asr_service {startwithmic|startnomic|stop} [onboard|uac] [8000|16000] [aec_en]",
     cli_asr_service_test_cmd},
};

int cli_asr_service_init(void)
{
    return cli_register_commands(s_asr_service_commands, ASR_SERVICE_CMD_CNT);
}
