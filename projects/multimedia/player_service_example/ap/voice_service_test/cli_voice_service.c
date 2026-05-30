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
#include <components/log.h>
#include "cli.h"

#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>


#define TAG "voc_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

voice_handle_t gl_voice_service_handle = NULL;
static voice_read_handle_t gl_voice_read_service_handle = NULL;
static voice_write_handle_t gl_voice_write_service_handle = NULL;
static beken_semaphore_t voice_start_sem = NULL;
/* Max 16kHz/20ms mono samples; stereo interleaved needs 2x int16 samples */
#define VOICE_SPK_STEREO_INTERLEAVE_SAMPLES_MAX   2048//(16000 * 20 / 1000)
static bool gl_voice_spk_stereo_dup = false;
static int16_t gl_voice_spk_stereo_buf[VOICE_SPK_STEREO_INTERLEAVE_SAMPLES_MAX * 2];

/* +10 dB: 10^(10/20) = sqrt(10) ¡Ö 3.162278, Q14 fixed-point */
#define VOICE_SPK_GAIN_10DB_Q14        51805

static inline int16_t voice_spk_apply_gain_10db(int16_t sample)
{
    int32_t v = ((int32_t)sample * VOICE_SPK_GAIN_10DB_Q14) >> 14;

    if (v > 32767)
    {
        return 32767;
    }
    if (v < -32768)
    {
        return -32768;
    }
    return (int16_t)v;
}

int voice_service_send_callback(unsigned char *data, unsigned int len, void *args)
{
    char *write_buf = (char *)data;
    uint32_t write_len = len;

    if (gl_voice_spk_stereo_dup && (len > 0))
    {
        uint32_t mono_samples = len / sizeof(int16_t);
        int16_t *mono = (int16_t *)data;
        int16_t *stereo = gl_voice_spk_stereo_buf;

        if (mono_samples > VOICE_SPK_STEREO_INTERLEAVE_SAMPLES_MAX)
        {
            LOGE("%s, %d, mono_samples:%u overflow\n", __func__, __LINE__, mono_samples);
            return (int)len;
        }

        for (uint32_t i = 0; i < mono_samples; i++)
        {
            stereo[2 * i + 0] = voice_spk_apply_gain_10db(mono[i]);
            stereo[2 * i + 1] = mono[i];
        }
        write_buf = (char *)stereo;
        write_len = len * 2;
    }
    int ret = bk_voice_write_frame_data(gl_voice_write_service_handle, write_buf, write_len);
    if (ret != (int)write_len)
    {
        LOGV("%s, %d, bk_voice_write_frame_data: %d != %u\n", __func__, __LINE__, ret, write_len);
    }
    else
    {
        //LOGD("%s, %d, len: %d\n", __func__, __LINE__, len);
    }

    // The semaphore only needs to be released once, indicating that the voice service has successfully started and started receiving data
    if (voice_start_sem)
    {
        LOGD("%s, %d, get mic data, set semaphore\n", __func__, __LINE__);
        rtos_set_semaphore(&voice_start_sem);
    }

    return len;
}

void cli_voice_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s +++\n", __func__);
    char *msg = CLI_CMD_RSP_ERROR;
    bool is_g711a_codec = false;
    bool is_pcm_codec = false;
    uint32_t aec_mode = 0;
    bool is_supported_aec_mode = false;


    if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGD("voice stop\n");
        msg = CLI_CMD_RSP_SUCCEED;
        goto exit;
    }
    else if (os_strcmp(argv[1], "start") == 0)
    {
        if ((argc != 9) && (argc != 10))
        {
            LOGE("%s, %d, agc: %d not right\n", __func__, __LINE__, argc);
            //goto exit;
        }
    
        voice_cfg_t voice_cfg = {0};
        aec_mode = os_strtoul(argv[4], NULL, 10);
        is_supported_aec_mode = (aec_mode >= 1 && aec_mode <= 3);
        is_g711a_codec = (os_strcmp(argv[5], "g711a") == 0 && os_strcmp(argv[6], "g711a") == 0);
        is_pcm_codec = (os_strcmp(argv[5], "pcm") == 0 && os_strcmp(argv[6], "pcm") == 0);
    
         LOGD("%s, %d, argc: %d, mic_type: %s, mic_samp_rate: %s, aec_version: %s, enc_type: %s, dec_type: %s, spk_type: %s, spk_samp_rate: %s, spk_chl: %s\n",
            __func__, __LINE__, argc,
            argv[2],
            argv[3],
            argv[4],
            argv[5],
            argv[6],
            argv[7],
            argv[8],
            argv[9]);

        uint32_t spk_chl = os_strtoul(argv[9], NULL, 10);

        if (spk_chl != 1 && spk_chl != 2)
        {
            LOGE("%s, %d, spk_chl:%d invalid, use 1=mono, 2=stereo\n", __func__, __LINE__, spk_chl);
            goto exit;
        }

        if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 8000
            && is_supported_aec_mode
            && (is_g711a_codec || is_pcm_codec)
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 8000)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
            voice_temp_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
            voice_cfg = voice_temp_cfg;
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 16000
            && is_supported_aec_mode
            && (is_g711a_codec || is_pcm_codec)
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 16000)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AEC_G711A_16000_CONFIG();
            voice_temp_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
            voice_cfg = voice_temp_cfg;
        }
        else if (os_strcmp(argv[2], "uac") == 0
            && os_strtoul(argv[3], NULL, 10) == 8000
            && is_supported_aec_mode
            && (is_g711a_codec || is_pcm_codec)
            && os_strcmp(argv[7], "uac") == 0
            && os_strtoul(argv[8], NULL, 10) == 8000)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_UAC_MIC_SPK_CONFIG();
            voice_cfg = voice_temp_cfg;
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 8000
            && is_supported_aec_mode
            && os_strcmp(argv[5], "aac") == 0
            && os_strcmp(argv[6], "aac") == 0
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 8000)
        {
#if (CONFIG_VOICE_SERVICE_AAC_ENCODER && CONFIG_VOICE_SERVICE_AAC_DECODER)
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AAC_CONFIG();
            voice_temp_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
            voice_cfg = voice_temp_cfg;
#else
            LOGW("%s, %d, aac encoder or decoder not support, please config: CONFIG_VOICE_SERVICE_AAC_ENCODER=y CONFIG_VOICE_SERVICE_AAC_DECODER=y\n", __func__, __LINE__);
            goto exit;
#endif
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 16000
            && is_supported_aec_mode
            && os_strcmp(argv[5], "g722") == 0
            && os_strcmp(argv[6], "g722") == 0
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 16000)
        {
#if (CONFIG_VOICE_SERVICE_G722_ENCODER && CONFIG_VOICE_SERVICE_G722_DECODER)
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_G722_CONFIG();
            voice_temp_cfg.spk_cfg.onboard_spk_cfg.multi_in_port_num = 2;
            voice_cfg = voice_temp_cfg;
#else
            LOGW("%s, %d, g722 encoder or decoder not support, please config: CONFIG_VOICE_SERVICE_G722_ENCODER=y CONFIG_VOICE_SERVICE_G722_DECODER=y\n", __func__, __LINE__);
            goto exit;
#endif
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 16000
            && is_supported_aec_mode
            && (is_g711a_codec || is_pcm_codec)
            && os_strcmp(argv[7], "i2s") == 0
            && os_strtoul(argv[8], NULL, 10) == 16000)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_I2S_SPK_AEC_G711A_16000_CONFIG();
            voice_temp_cfg.aec_en = false;
            voice_temp_cfg.spk_cfg.i2s_cfg.multi_in_port_num = 2;
            voice_cfg = voice_temp_cfg;
        }
        else
        {
            LOGE("%s, %d, test command not support\n", __func__, __LINE__);
            goto exit;
        }

        uint32_t mic_sample_rate = os_strtoul(argv[3], NULL, 10);
        uint32_t spk_sample_rate = os_strtoul(argv[8], NULL, 10);
        uint8_t aec_en = (uint8_t)(aec_mode & 0x3);
        bool voice_onboard_spk_stereo = false;

        if (voice_cfg.spk_type == SPK_TYPE_ONBOARD && spk_chl == 2)
        {
            voice_onboard_spk_stereo = true;
        }

        if (is_pcm_codec)
        {
            voice_cfg.enc_type = AUDIO_ENC_TYPE_PCM;
            voice_cfg.dec_type = AUDIO_DEC_TYPE_PCM;
            voice_cfg.read_pool_size  = mic_sample_rate * 2 * 20 / 1000 * 2;
            voice_cfg.write_pool_size = spk_sample_rate * 2 * 20 / 1000 * 2;
            if (voice_onboard_spk_stereo)
            {
                voice_cfg.write_pool_size *= 2;
            }
        }

        uint8_t dac_source = AUD_DAC_SOURCE_A2DP;
        bool prompt_mix_enable = false;
        if (argv[10])
        {
            if (os_strcmp(argv[10], "call") == 0)
            {
                dac_source = AUD_DAC_SOURCE_CALL;
            }
            else if (os_strcmp(argv[10], "a2dp") == 0)
            {
                dac_source = AUD_DAC_SOURCE_A2DP;
            }
            else if (os_strcmp(argv[10], "hint") == 0)
            {
                dac_source = AUD_DAC_SOURCE_HINT;
            }
            else if (os_strcmp(argv[10], "prompt") == 0)
            {
                /* A2DP as main source, prompt tone mixed from CALL source */
                prompt_mix_enable = true;
                dac_source = AUD_DAC_SOURCE_A2DP;
            }
        } else
		{
            prompt_mix_enable = true;
            dac_source = AUD_DAC_SOURCE_A2DP;
		}

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
        if (voice_cfg.spk_type == SPK_TYPE_ONBOARD)
        {
            if (spk_sample_rate == 8000)
            {
                voice_cfg.spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT
                                                                      | ONBOARD_SPEAKER_STREAM_DAC_SOURCE_HINT_BIT;
                voice_cfg.spk_cfg.onboard_spk_cfg.main_dac_source = AUD_DAC_SOURCE_CALL;
                voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_CALL] = spk_sample_rate;
                voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_CALL] = spk_sample_rate * 2 * 20 / 1000; 
                voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_HINT] = 16000;
                voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_HINT] = 16000 * 2 * 20 / 1000; 
            }
            else
            {
                /* 16k voice call: use CALL DAC at native rate (A2DP path resamples to 48k) */
                if (prompt_mix_enable)
                {
                    voice_cfg.spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT
                                                                          | ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
                    voice_cfg.spk_cfg.onboard_spk_cfg.main_dac_source = AUD_DAC_SOURCE_A2DP;
                    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_A2DP] = spk_sample_rate;
                    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_A2DP]  = spk_sample_rate * 2 * 20 / 1000;
                    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_CALL] = 16000;
                    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_CALL]  = 16000 * 2 * 20 / 1000;
                }
                else if (dac_source == AUD_DAC_SOURCE_CALL)
                {
                    voice_cfg.spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
                    voice_cfg.spk_cfg.onboard_spk_cfg.main_dac_source   = AUD_DAC_SOURCE_CALL;
                    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_CALL] = spk_sample_rate;
                    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_CALL]  = spk_sample_rate * 2 * 20 / 1000;
                }
                else if (dac_source == AUD_DAC_SOURCE_A2DP)
                {
                    voice_cfg.spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT;
                    voice_cfg.spk_cfg.onboard_spk_cfg.main_dac_source   = AUD_DAC_SOURCE_A2DP;
                    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_A2DP] = spk_sample_rate;
                    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_A2DP]  = spk_sample_rate * 2 * 20 / 1000;
                }
                else if (dac_source == AUD_DAC_SOURCE_HINT)
                {
                    voice_cfg.spk_cfg.onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_HINT_BIT;
                    voice_cfg.spk_cfg.onboard_spk_cfg.main_dac_source   = AUD_DAC_SOURCE_HINT;
                    voice_cfg.spk_cfg.onboard_spk_cfg.sample_rate[AUD_DAC_SOURCE_HINT] = 16000;
                    voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_HINT]  = 16000 * 2 * 20 / 1000;
                }
                else
                {
                    LOGE("%s, %d, dac_source:%d not support\n", __func__, __LINE__, dac_source);
                    goto exit;
                }
            }
        }
#endif


        if (voice_cfg.mic_type == MIC_TYPE_ONBOARD)
        {
            voice_cfg.mic_cfg.onboard_mic_cfg.ch_bitmap = (1 << AUD_ADC_CHL_0);
            voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num = 0;
            for (uint32_t j = 0; j < AUD_ADC_CHL_MAX; j++)
            {
                if (voice_cfg.mic_cfg.onboard_mic_cfg.ch_bitmap & (1 << j))
                {
                    voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.chl_num++;
                }
            }
            voice_cfg.mic_cfg.onboard_mic_cfg.adc_cfg.aec_en = aec_en;
        }


        if (voice_cfg.spk_type == SPK_TYPE_ONBOARD)
        {
            voice_cfg.spk_cfg.onboard_spk_cfg.chl_num = spk_chl;
            voice_cfg.spk_cfg.onboard_spk_cfg.dac_chl = (spk_chl == 2) ? AUD_DAC_CHL_LR : AUD_DAC_CHL_L;
        }

        gl_voice_spk_stereo_dup = false;
        if (voice_onboard_spk_stereo)
        {
            uint32_t mono_frame_bytes = spk_sample_rate * 2 * 20 / 1000;

            gl_voice_spk_stereo_dup = true;

            if (spk_sample_rate == 8000)
            {
                voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_CALL] = mono_frame_bytes * 2;
            }
            else
            {
                voice_cfg.spk_cfg.onboard_spk_cfg.frame_size[AUD_DAC_SOURCE_CALL] = mono_frame_bytes * 2;
            }

        }

        if (aec_en && voice_cfg.mic_type == MIC_TYPE_UAC && voice_cfg.spk_type == SPK_TYPE_UAC)
        {
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
            voice_cfg.aec_cfg.aec_alg_cfg.dual_ch = 0;
            voice_cfg.aec_cfg.aec_alg_cfg.multi_in_port_num = 1;
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
        }

#if CONFIG_SOC_BK7259
        if (aec_en && voice_cfg.mic_type == MIC_TYPE_ONBOARD && voice_cfg.spk_type == SPK_TYPE_ONBOARD)
        {
            voice_cfg.aec_cfg.aec_alg_cfg.dual_ch = 0;
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.mode = AEC_MODE_HARDWARE;
            voice_cfg.aec_cfg.aec_alg_cfg.multi_in_port_num = 0;
            voice_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
        }
#endif

        /* start voice */
        gl_voice_service_handle = bk_voice_init(&voice_cfg);
        if (!gl_voice_service_handle)
        {
            LOGE("%s, %d, voice init fail\n", __func__, __LINE__);
            goto exit;
        }

        voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
        voice_read_cfg.voice_handle = gl_voice_service_handle;
        voice_read_cfg.max_read_size = 640; ////
        voice_read_cfg.voice_read_callback = voice_service_send_callback;
        gl_voice_read_service_handle = bk_voice_read_init(&voice_read_cfg);
        if (!gl_voice_read_service_handle)
        {
            LOGE("%s, %d, voice read init fail\n", __func__, __LINE__);
            goto exit;
        }

        voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
        voice_write_cfg.voice_handle = gl_voice_service_handle;
        if (is_pcm_codec)
        {
            /* One PCM spk frame per pump; must match onboard_spk frame_size (stereo = 2x mono) */
            //voice_write_cfg.frame_size = spk_sample_rate * 2 * 20 / 1000;
            if (voice_onboard_spk_stereo)
            {
                //voice_write_cfg.frame_size *= 2;
            }
        }
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

        // Create a semaphore to check if the voice service has successfully started
        if (BK_OK != rtos_init_semaphore(&voice_start_sem, 1))
        {
            LOGE("%s, %d, create semaphore fail\n", __func__, __LINE__);
            goto exit;
        }

        // Wait for 5 seconds timeout, check if the callback function is called
        LOGI("waiting for voice service to start (timeout: 5s)...\n");
        bk_err_t ret = rtos_get_semaphore(&voice_start_sem, 5000);  // 5 seconds timeout
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
    }
    else
    {
        LOGE("%s, %d, cmd not support\n", __func__, __LINE__);
    }

    LOGD("%s ---complete\n", __func__);

    msg = CLI_CMD_RSP_SUCCEED;

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

    return;

exit:
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
    }

    if (gl_voice_write_service_handle)
    {
        bk_voice_write_deinit(gl_voice_write_service_handle);
    }

    if (gl_voice_service_handle)
    {
        bk_voice_deinit(gl_voice_service_handle);
    }
    gl_voice_read_service_handle = NULL;
    gl_voice_write_service_handle = NULL;
    gl_voice_service_handle  = NULL;
    gl_voice_spk_stereo_dup = false;

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define VOICE_SERVICE_CMD_CNT   (sizeof(s_voice_service_commands) / sizeof(struct cli_command))

static const struct cli_command s_voice_service_commands[] =
{
    /* voice_service {cmd mic_type mic_samp_rate aec_en enc_type dec_type spk_type eq_type}
     *
     * [cmd]            start/stop
     * [mic_type]       onboard/uac/onboard_dual_dmic_mic
     * [mic_samp_rate]  8000/16000
     * [aec_en]         bit0:0 aec disable/1 aec enable,
                        bit1:0 AEC_MODE_SOFTWARE/1 AEC_MODE_HARDWARE,
                        bit2:0 DUAL_MIC_CH_0_DEGREE/1 DUAL_MIC_CH_90_DEGREE
                        bit3:0 no mic swap/1 mic swap
                        bit4:0 no ec ooutput/1 ecoutput
     * [enc_type]       pcm/g711a/g711u/aac/g722
     * [dec_type]       pcm/g711a/g711u/aac/g722
     * [spk_type]       onboard/uac/i2s
     * [spk_samp_rate]  8000/16000
     * [eq_type]        0: diabale, 1: eq_mono, 2: eq_stereo
     */

    {"voice_service", "voice_service {start|stop onboard|uac|onboard_dual_dmic_mic 8000|16000 0|1|3 pcm|g711a|g711u|aac|g722 pcm|g711a|g711u|aac|g722 onboard|uac|i2s 8000|16000 0|1|2}", cli_voice_service_test_cmd},
};

int cli_voice_service_init(void)
{
    return cli_register_commands(s_voice_service_commands, VOICE_SERVICE_CMD_CNT);
}
