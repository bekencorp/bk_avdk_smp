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

    if ((argc != 9) && (argc != 10))
    {
        LOGE("%s, %d, agc: %d not right\n", __func__, __LINE__, argc);
        goto exit;
    }

    voice_cfg_t voice_cfg = {0};

     LOGD("%s, %d, argc: %d, mic_type: %s, mic_samp_rate: %s, aec_version: %s, enc_type: %s, dec_type: %s, spk_type: %s, spk_samp_rate: %s, eq_type: %s\n", 
        __func__, __LINE__, argc, 
        argv[2], 
        argv[3], 
        argv[4], 
        argv[5], 
        argv[6], 
        argv[7], 
        argv[8], 
        argv[9]);

    if (os_strcmp(argv[1], "start") == 0)
    {
        if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 8000
            && os_strtoul(argv[4], NULL, 10) == 1
            && os_strcmp(argv[5], "g711a") == 0
            && os_strcmp(argv[6], "g711a") == 0
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 8000
            && os_strtoul(argv[9], NULL, 10) == 0)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
            voice_cfg = voice_temp_cfg;
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 16000
            && os_strtoul(argv[4], NULL, 10) == 1
            && os_strcmp(argv[5], "g711a") == 0
            && os_strcmp(argv[6], "g711a") == 0
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 16000
            && os_strtoul(argv[9], NULL, 10) == 0)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AEC_G711A_16000_CONFIG();
            voice_cfg = voice_temp_cfg;
        }
        else if (os_strcmp(argv[2], "uac") == 0
            && os_strtoul(argv[3], NULL, 10) == 8000
            && os_strtoul(argv[4], NULL, 10) == 1
            && os_strcmp(argv[5], "g711a") == 0
            && os_strcmp(argv[6], "g711a") == 0
            && os_strcmp(argv[7], "uac") == 0
            && os_strtoul(argv[8], NULL, 10) == 8000
            && os_strtoul(argv[9], NULL, 10) == 0)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_UAC_MIC_SPK_CONFIG();
            voice_cfg = voice_temp_cfg;
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 8000
            && os_strtoul(argv[4], NULL, 10) == 1
            && os_strcmp(argv[5], "aac") == 0
            && os_strcmp(argv[6], "aac") == 0
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 8000
            && os_strtoul(argv[9], NULL, 10) == 0)
        {
#if (CONFIG_VOICE_SERVICE_AAC_ENCODER && CONFIG_VOICE_SERVICE_AAC_DECODER)
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_AAC_CONFIG();
            voice_cfg = voice_temp_cfg;
#else
            LOGW("%s, %d, aac encoder or decoder not support, please config: CONFIG_VOICE_SERVICE_AAC_ENCODER=y CONFIG_VOICE_SERVICE_AAC_DECODER=y\n", __func__, __LINE__);
            goto exit;
#endif
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 16000
            && os_strtoul(argv[4], NULL, 10) == 1
            && os_strcmp(argv[5], "g722") == 0
            && os_strcmp(argv[6], "g722") == 0
            && os_strcmp(argv[7], "onboard") == 0
            && os_strtoul(argv[8], NULL, 10) == 16000
            && os_strtoul(argv[9], NULL, 10) == 0)
        {
#if (CONFIG_VOICE_SERVICE_G722_ENCODER && CONFIG_VOICE_SERVICE_G722_DECODER)
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_G722_CONFIG();
            voice_cfg = voice_temp_cfg;
#else
            LOGW("%s, %d, g722 encoder or decoder not support, please config: CONFIG_VOICE_SERVICE_G722_ENCODER=y CONFIG_VOICE_SERVICE_G722_DECODER=y\n", __func__, __LINE__);
            goto exit;
#endif
        }
        else if (os_strcmp(argv[2], "onboard") == 0
            && os_strtoul(argv[3], NULL, 10) == 16000
            && os_strtoul(argv[4], NULL, 10) == 1
            && os_strcmp(argv[5], "g711a") == 0
            && os_strcmp(argv[6], "g711a") == 0
            && os_strcmp(argv[7], "i2s") == 0
            && os_strtoul(argv[8], NULL, 10) == 16000
            && os_strtoul(argv[9], NULL, 10) == 0)
        {
            voice_cfg_t voice_temp_cfg = DEFAULT_VOICE_BY_ONBOARD_MIC_I2S_SPK_AEC_G711A_16000_CONFIG();
            voice_cfg = voice_temp_cfg;
        }
        else
        {
            LOGE("%s, %d, test command not support\n", __func__, __LINE__);
            goto exit;
        }

        /* start voice */
        gl_voice_service_handle = bk_voice_init(&voice_cfg);
        if (!gl_voice_service_handle)
        {
            LOGE("%s, %d, voice init fail\n", __func__, __LINE__);
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
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGD("voice stop\n");
        msg = CLI_CMD_RSP_SUCCEED;
        goto exit;
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
