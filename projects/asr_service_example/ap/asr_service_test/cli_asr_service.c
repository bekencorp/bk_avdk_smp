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

#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>

#if (CONFIG_WANSON_ASR || CONFIG_WANSON_ARMINO_ASR)
#include "bk_wanson_asr_intf.h"
#endif

#if (CONFIG_VOICE_SERVICE)
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#endif

#define TAG "asr_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

// Global handle definitions
static asr_handle_t gl_asr_service_handle = NULL;
static aud_asr_handle_t gl_aud_asr_service_handle = NULL;

#if (CONFIG_VOICE_SERVICE)
static voice_handle_t gl_voice_service_handle = NULL;
static voice_read_handle_t gl_voice_read_service_handle = NULL;
static voice_write_handle_t gl_voice_write_service_handle = NULL;
static beken_semaphore_t voice_start_sem = NULL;

/**
 * @brief Voice service send callback function
 * @param data Audio data
 * @param len Data length
 * @param args Additional arguments
 * @return Processed data length
 */
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

    // Semaphore only needs to be released once, indicating voice service has successfully started and begun receiving data
    if (voice_start_sem)
    {
        LOGD("%s, %d, get mic data, set semaphore\n", __func__, __LINE__);
        rtos_set_semaphore(&voice_start_sem);
    }
    return len;
}
#endif

/**
 * @brief ASR result processing function
 * @param param Recognition result string pointer
 */
static void bk_asr_service_result_handle(uint32_t param)
{
    char *result = (char *)param;

#if CONFIG_WANSON_ASR
    if (os_strcmp(result, "小叮小叮") == 0)
    {
        LOGI("%s \n", "XiaodingXiaoding");
    }
    // No processing for other cases
#elif CONFIG_WANSON_ARMINO_ASR
#if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V1)
    if (os_strcmp(result, "嗨阿米诺") == 0)   // Wake-up word "Hi Armino" recognized
    {
        LOGI("%s \n", "hi armino, cmd: 0 ");
    }
    else if (os_strcmp(result, "嘿阿米楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 1 ");
    }
    else if (os_strcmp(result, "嘿儿米楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 2 ");
    }
    else if (os_strcmp(result, "嘿鹅迷楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 3 ");
    }
    else if (os_strcmp(result, "拜拜阿米诺") == 0)   // "Bye-bye Armino" recognized
    {
        LOGI("%s \n", "byebye armino, cmd: 0 ");
    }
    else if (os_strcmp(result, "拜拜阿米楼") == 0)
    {
        LOGI("%s \n", "byebye armino, cmd: 1 ");
    }
    // No processing for other cases
#else
    if (os_strcmp(result, "你好阿米诺") == 0)   // Wake-up word "Hello Armino" recognized
    {
        LOGI("%s \n", "nihao armino, cmd: 0 ");
    }
    else if (os_strcmp(result, "再见阿米诺") == 0)
    {
        LOGI("%s \n", "zaijian armino, cmd: 1 ");
    }
    else if (os_strcmp(result, "小叮小叮") == 0)
    {
        LOGI("%s \n", "XiaodingXiaoding");
    }
    // No processing for other cases
#endif
#endif
}

/**
 * @brief Clean up ASR related resources
 */
static void bk_cleanup_asr_resources(void)
{
    // Stop services
    if (gl_aud_asr_service_handle)
    {
        bk_aud_asr_stop(gl_aud_asr_service_handle);
    }
    if (gl_asr_service_handle)
    {
        bk_asr_stop(gl_asr_service_handle);
    }

    // Release resources
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
    // Clean up voice service resources
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

    // Clean up semaphore
    if (voice_start_sem)
    {
        rtos_deinit_semaphore(&voice_start_sem);
        voice_start_sem = NULL;
    }
#endif
}

/**
 * @brief Initialize audio ASR service
 * @param asr_handle ASR service handle
 * @return BK_OK on success, error code on failure
 */
static int bk_init_audio_asr_service(asr_handle_t asr_handle)
{
    aud_asr_cfg_t aud_asr_cfg = (aud_asr_cfg_t)AUDIO_ASR_CFG_DEFAULT();
    aud_asr_cfg.asr_handle = asr_handle;
    aud_asr_cfg.aud_asr_result_handle = bk_asr_service_result_handle;

#if (CONFIG_WANSON_ARMINO_ASR || CONFIG_WANSON_ASR)
    aud_asr_cfg.aud_asr_init = bk_wanson_asr_common_init;
    aud_asr_cfg.aud_asr_deinit = bk_wanson_asr_common_deinit;
    aud_asr_cfg.aud_asr_recog = bk_wanson_asr_recog;
#endif

    gl_aud_asr_service_handle = bk_aud_asr_init(&aud_asr_cfg);
    if (!gl_aud_asr_service_handle)
    {
        LOGE("aud asr init fail\n");
        return BK_FAIL;
    }

    return BK_OK;
}

/**
 * @brief Set ASR resample configuration
 * @param asr_cfg ASR configuration structure
 * @param mic_sample_rate Microphone sample rate
 * @return true on success, false on failure
 */
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

/**
 * @brief Set ASR read pool size
 * @param asr_cfg ASR configuration structure
 * @param mic_sample_rate Microphone sample rate
 */
static void bk_setup_asr_read_pool_size(asr_cfg_t *asr_cfg, uint32_t mic_sample_rate)
{
    if (mic_sample_rate == 16000) {
        asr_cfg->read_pool_size = mic_sample_rate * 2 * 20 / 1000;  // 16kHz, 20ms, 16bit
    }
    else if (mic_sample_rate == 8000) {
        asr_cfg->read_pool_size = 2 * mic_sample_rate * 2 * 20 / 1000;  // 8kHz, double buffer
    }
}

void cli_asr_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;

    // Parameter validity check
    if (argc < 4)
    {
        LOGE("%s, %d, invalid argument count: %d\n", __func__, __LINE__, argc);
        goto exit;
    }

    asr_cfg_t asr_cfg = {0};
    uint32_t mic_sample_rate = 0;

    if (argv[3] == NULL)
    {
        LOGE("%s, %d, mic_samp_rate is NULL\n", __func__, __LINE__, argv[3]);
        goto exit;
    }

    mic_sample_rate = os_strtoul(argv[3], NULL, 10);

    // Process start with microphone command
    if (os_strcmp(argv[1], "startwithmic") == 0)
    {
        LOGD("%s, %d, startwithmic, mic_type: %s, mic_samp_rate: %u\n", 
             __func__, __LINE__, argv[2], mic_sample_rate);

        // Configure based on microphone type
        if (os_strcmp(argv[2], "onboard") == 0)
        {
            asr_cfg_t asr_cfg_onboard = (asr_cfg_t)ASR_BY_ONBOARD_MIC_CFG_DEFAULT();
            asr_cfg_onboard.mic_type = MIC_TYPE_ONBOARD;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.adc_cfg.sample_rate = mic_sample_rate;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.frame_size = mic_sample_rate * 2 * 20 / 1000; 
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.out_block_size = asr_cfg_onboard.mic_cfg.onboard_mic_cfg.frame_size;
            asr_cfg_onboard.mic_cfg.onboard_mic_cfg.out_block_num = 4;
            asr_cfg = asr_cfg_onboard;
        }
        else if (os_strcmp(argv[2], "uac") == 0)
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

        // Set resample configuration
        if (!bk_setup_asr_resample_config(&asr_cfg, mic_sample_rate))
        {
            goto exit;
        }

        // Create ASR service
        asr_cfg.event_handle = NULL;
        asr_cfg.args = NULL;
        gl_asr_service_handle = bk_asr_create(&asr_cfg);
        if (!gl_asr_service_handle)
        {
            LOGE("asr create fail\n");
            goto exit;
        }

        // Set read pool size
        bk_setup_asr_read_pool_size(&asr_cfg, mic_sample_rate);

        // Initialize ASR
        if (argv[4])
        {
            asr_cfg.aec_en = os_strtoul(argv[4], NULL, 10);
            asr_cfg.aec_cfg.aec_alg_cfg.aec_cfg.fs = mic_sample_rate;
        }
        bk_asr_init_with_mic(&asr_cfg, gl_asr_service_handle);
        
        // Initialize audio ASR service
        if (bk_init_audio_asr_service(gl_asr_service_handle) != BK_OK)
        {
            goto exit;
        }

        // Start ASR service
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
    // Process start without microphone command
    else if (os_strcmp(argv[1], "startnomic") == 0)
    {
#if (CONFIG_VOICE_SERVICE)
        LOGD("%s, %d, startnomic, mic_type: %s, mic_samp_rate: %u, aec_on: %s\n", 
             __func__, __LINE__, argv[2], mic_sample_rate, argv[4] ? argv[4] : "NULL");

        voice_cfg_t voice_cfg = {0};
        bool aec_enabled = (argv[4] && os_strcmp(argv[4], "aec") == 0);

        // Configure voice service based on microphone type
        if (os_strcmp(argv[2], "onboard") == 0)
        {
            voice_cfg = (voice_cfg_t)DEFAULT_VOICE_BY_ONBOARD_MIC_SPK_CONFIG();
            voice_cfg.aec_en = aec_enabled;

            if (aec_enabled)
            {
                voice_cfg.aec_cfg.aec_alg_cfg.multi_out_port_num = 1;
            }
            else
            {
                voice_cfg.mic_cfg.onboard_mic_cfg.multi_out_port_num = 1;
            }

            asr_cfg = (asr_cfg_t)ASR_BY_ONBOARD_MIC_CFG_DEFAULT();
        }
        else if (os_strcmp(argv[2], "uac") == 0)
        {
            voice_cfg = (voice_cfg_t)DEFAULT_VOICE_BY_UAC_MIC_SPK_CONFIG();
            voice_cfg.aec_en = aec_enabled;
            
            if (aec_enabled)
            {
                voice_cfg.aec_cfg.aec_alg_cfg.multi_out_port_num = 1;
            }
            else
            {
                voice_cfg.mic_cfg.uac_mic_cfg.multi_out_port_num = 1;
            }

            asr_cfg = (asr_cfg_t)ASR_BY_UAC_MIC_CFG_DEFAULT();
        }
        else
        {
            LOGE("%s, %d, unsupported mic_type: %s\n", __func__, __LINE__, argv[2]);
            goto exit;
        }

        // Set ASR resample configuration
        if (!bk_setup_asr_resample_config(&asr_cfg, mic_sample_rate))
        {
            goto exit;
        }

        // Initialize voice service
        gl_voice_service_handle = bk_voice_init(&voice_cfg);
        if (!gl_voice_service_handle)
        {
            LOGE("%s, %d, voice init fail\n", __func__, __LINE__);
            goto exit;
        }

        // Initialize ASR service
        asr_cfg.event_handle = NULL;
        asr_cfg.args = NULL;
        gl_asr_service_handle = bk_asr_create(&asr_cfg);
        if (!gl_asr_service_handle)
        {
            LOGE("asr create fail\n");
            goto exit;
        }

        // Get microphone stream
        gl_asr_service_handle->mic_str = (audio_element_handle_t)bk_voice_get_mic_str(gl_voice_service_handle, &voice_cfg);
        if (!gl_asr_service_handle->mic_str)
        {
            LOGE("get mic str fail\n");
            goto exit;
        }

        // Set read pool size
        bk_setup_asr_read_pool_size(&asr_cfg, mic_sample_rate);
        
        // Initialize ASR
        bk_asr_init(&asr_cfg, gl_asr_service_handle);
        if (!gl_asr_service_handle)
        {
            LOGE("asr init fail\n");
            goto exit;
        }

        // Initialize audio ASR service
        if (bk_init_audio_asr_service(gl_asr_service_handle) != BK_OK)
        {
            goto exit;
        }

        // Initialize voice read/write services
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

        // Start voice service
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

        // Create semaphore to check if voice service started successfully
        if (BK_OK != rtos_init_semaphore(&voice_start_sem, 1))
        {
            LOGE("%s, %d, create semaphore fail\n", __func__, __LINE__);
            goto exit;
        }

        // Wait for 5 seconds timeout, check if callback function is called
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

        // Start ASR service
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
    // Process stop command
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGD("asr stop\n");
        msg = CLI_CMD_RSP_SUCCEED;
        bk_cleanup_asr_resources();
        goto exit;
    }
    else
    {
        LOGE("%s, %d, unsupported command: %s\n", __func__, __LINE__, argv[1]);
        goto exit;
    }

    LOGD("%s ---complete\n", __func__);
    msg = CLI_CMD_RSP_SUCCEED;

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

 exit:
    // Clean up resources
    bk_cleanup_asr_resources();
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define ASR_SERVICE_CMD_CNT   (sizeof(s_asr_service_commands) / sizeof(struct cli_command))

static const struct cli_command s_asr_service_commands[] =
{
    /* asr_service {cmd mic_type mic_samp_rate}
     *
     * [cmd]            start/stop
     * [mic_type]       onboard/uac/
     * [mic_samp_rate]  8000/16000
     */

    {"asr_service", "asr_service {start|stop onboard|uac|onboard_dual_dmic_mic 8000|16000}", cli_asr_service_test_cmd},
};

int cli_asr_service_init(void)
{
    return cli_register_commands(s_asr_service_commands, ASR_SERVICE_CMD_CNT);
}
