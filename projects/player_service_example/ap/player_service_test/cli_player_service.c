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

#include <components/bk_player_service.h>
#include <components/bk_player_service_types.h>
#include "prompt_tone_test.h"

#if CONFIG_VOICE_SERVICE_TEST
#include <components/bk_voice_service.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#endif

#define TAG "player_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
#define OUTPUT_RB_SIZE  (4 * 1024)
static audio_port_handle_t gl_output_port_handle = NULL;
static audio_port_handle_t gl_output_port1_handle = NULL;
extern voice_handle_t gl_voice_service_handle;
#endif


#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"


static bk_player_handle_t gl_player_handle = NULL;

#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
static bk_player_handle_t gl_player1_handle = NULL;

static int player_not_playback_port_state_notify_handler(int state, void *port_info, void *user_data)
{
    LOGD("%s, %d, ++>>>>>>>> state: %d \n", __func__, __LINE__, state);

    return BK_OK;
}

int player_not_playback_event_handler(int data, void *params, void *args)
{
    LOGD("%s, %d, data: %d\n", __func__, __LINE__, data);

    if (data == PLAYER_EVENT_MUSIC_INFO)
    {
#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        audio_element_info_t *music_info = (audio_element_info_t *)params;
        audio_element_handle_t spk_element = bk_voice_get_spk_element(gl_voice_service_handle);
        if (spk_element == NULL)
        {
            LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = {0};
        port_info.chl_num = music_info->channels;
        port_info.sample_rate = music_info->sample_rates;
        port_info.dig_gain = 0x2d;
        port_info.ana_gain = 0x01;
        port_info.bits = music_info->bits;
        port_info.port_id = 1;
        port_info.priority = 1;
        port_info.port = gl_output_port_handle;
        port_info.notify_cb = player_not_playback_port_state_notify_handler;
        port_info.user_data = NULL;

        spk_type_t spk_type = SPK_TYPE_INVALID;
        if (BK_OK != bk_voice_get_spkstr_type(gl_voice_service_handle, &spk_type))
        {
            LOGE("%s, %d, bk_voice_get_spkstr_type fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        switch (spk_type)
        {
            case SPK_TYPE_ONBOARD:
                if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
                {
                    LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                    return BK_FAIL;
                }
                break;

            case SPK_TYPE_UAC:
                LOGE("%s, %d, SPK_TYPE_UAC is not support\n", __func__, __LINE__);
                return BK_FAIL;
                break;

            case SPK_TYPE_I2S:
                if (BK_OK != i2s_stream_set_input_port_info(spk_element, &port_info))
                {
                    LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                    return BK_FAIL;
                }
                break;

            default:
                LOGE("%s, %d, spk_type: %d is not support\n", __func__, __LINE__, spk_type);
                return BK_FAIL;
                break;
        }

        LOGD("[%s] PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", __func__, music_info->sample_rates, music_info->bits, music_info->channels);
        LOGD("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
#endif
    }
    else if (data == PLAYER_EVENT_FINISH)
    {
        LOGD("[%s] PLAYER_EVENT_FINISH\n", __func__);
    }
    else
    {
        //nothing todo
    }

    return BK_OK;
}

static int player1_not_playback_port_state_notify_handler(int state, void *port_info, void *user_data)
{
    LOGD("%s, %d, ++>>>>>>>> state: %d \n", __func__, __LINE__, state);

    return BK_OK;
}

int player1_not_playback_event_handler(int data, void *params, void *args)
{
    LOGD("%s data: %d\n", __func__, data);

    if (data == PLAYER_EVENT_MUSIC_INFO)
    {
        audio_element_info_t *music_info = (audio_element_info_t *)params;
        audio_element_handle_t spk_element = bk_voice_get_spk_element(gl_voice_service_handle);
        if (spk_element == NULL)
        {
            LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = {0};
        port_info.chl_num = music_info->channels;
        port_info.sample_rate = music_info->sample_rates;
        port_info.dig_gain = 0x2d;
        port_info.ana_gain = 0x01;
        port_info.bits = music_info->bits;
        port_info.port_id = 2;
        port_info.priority = 2;
        port_info.port = gl_output_port1_handle;
        port_info.notify_cb = player1_not_playback_port_state_notify_handler;
        port_info.user_data = NULL;

        spk_type_t spk_type = SPK_TYPE_INVALID;
        if (BK_OK != bk_voice_get_spkstr_type(gl_voice_service_handle, &spk_type))
        {
            LOGE("%s, %d, bk_voice_get_spkstr_type fail\n", __func__, __LINE__);
            return BK_FAIL;
        }

        switch (spk_type)
        {
            case SPK_TYPE_ONBOARD:
                if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
                {
                    LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                    return BK_FAIL;
                }
                break;

            case SPK_TYPE_UAC:
                LOGE("%s, %d, SPK_TYPE_UAC is not support\n", __func__, __LINE__);
                return BK_FAIL;
                break;

            case SPK_TYPE_I2S:
                if (BK_OK != i2s_stream_set_input_port_info(spk_element, &port_info))
                {
                    LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                    return BK_FAIL;
                }
                break;

            default:
                LOGE("%s, %d, spk_type: %d is not support\n", __func__, __LINE__, spk_type);
                return BK_FAIL;
                break;
        }
        LOGD("[%s] PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", __func__, music_info->sample_rates, music_info->bits, music_info->channels);
        LOGD("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
    }
    else if (data == PLAYER_EVENT_FINISH)
    {
        LOGD("[%s] PLAYER_EVENT_FINISH\n", __func__);
    }
    else
    {
        //nothing todo
    }

    return BK_OK;
}
#endif

int player_with_playback_event_handler(int data, void *params, void *args)
{
    LOGD("%s data: %d\n", __func__, data);

    if (data == PLAYER_EVENT_MUSIC_INFO)
    {
        LOGD("[%s] PLAYER_EVENT_MUSIC_INFO\n", __func__);
    }
    else if (data == PLAYER_EVENT_FINISH)
    {
        LOGD("[%s] PLAYER_EVENT_FINISH\n", __func__);
    }
    else
    {
        //nothing todo
    }

    return BK_OK;
}

void cli_player_service_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s +++\n", __func__);
    bk_err_t ret = BK_OK;
    player_uri_info_t uri_info = {0};
    char *msg = CLI_CMD_RSP_ERROR;

#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    audio_element_handle_t spk_element = NULL;
#endif

    if (argc < 2)
    {
        LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
        return;
    }

    if (os_strcmp(argv[1], "playback") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
            return;
        }

        if (os_strcmp(argv[2], "start") == 0)
        {
            if (argc < 5)
            {
                LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                return;
            }

            bk_player_cfg_t player_cfg = DEFAULT_PLAYER_WITH_PLAYBACK_CONFIG();
            player_cfg.spk_cfg.onboard_spk_cfg.sample_rate = 16000;
            player_cfg.args = NULL;
            player_cfg.event_handle = player_with_playback_event_handler;
            gl_player_handle = bk_player_create(&player_cfg);
            if (!gl_player_handle)
            {
                LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                return;
            }

            if (os_strcmp(argv[3], "array") == 0)
            {
                int array_id = os_strtoul(argv[4], NULL, 10);
                uri_info.uri_type = PLAYER_URI_TYPE_ARRAY;
                if (array_id == 0)
                {
                    uri_info.uri = (char *)asr_wakeup_prompt_tone_array;
                    uri_info.total_len = sizeof(asr_wakeup_prompt_tone_array);
                    bk_player_set_decode_type(gl_player_handle, AUDIO_DEC_TYPE_PCM);
                }
                else if (array_id == 1)
                {
                    uri_info.uri = (char *)network_provision_prompt_tone_array;
                    uri_info.total_len = sizeof(network_provision_prompt_tone_array);
                    bk_player_set_decode_type(gl_player_handle, AUDIO_DEC_TYPE_MP3);
                }
                else if (array_id == 2)
                {
                    uri_info.uri = (char *)low_voltage_prompt_tone_array;
                    uri_info.total_len = sizeof(low_voltage_prompt_tone_array);
                    bk_player_set_decode_type(gl_player_handle, AUDIO_DEC_TYPE_WAV);
                }
                else
                {
                    LOGE("%s, %d, array_id: %d not right\n", __func__, __LINE__, array_id);
                    goto exit;
                }

                if (BK_OK != bk_player_set_uri(gl_player_handle, &uri_info))
                {
                    LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
                    goto exit;
                }
            }
            else if (os_strcmp(argv[3], "vfs") == 0)
            {
                uri_info.uri_type = PLAYER_URI_TYPE_VFS;
                uri_info.uri = argv[4];

                if (BK_OK != bk_player_set_uri(gl_player_handle, &uri_info))
                {
                    LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
                    goto exit;
                }
            }
            else
            {
                LOGE("%s, %d, uri type: %s is not support\n", __func__, __LINE__, argv[3]);
                goto exit;
            }

            ret = bk_player_start(gl_player_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_start fail\n", __func__, __LINE__);
                goto exit;
            }
        }
        else if (os_strcmp(argv[2], "stop") == 0)
        {
            msg = CLI_CMD_RSP_SUCCEED;
            goto exit;
        }
        else
        {
            LOGE("%s, %d, cmd: %s is not support\n", __func__, __LINE__, argv[2]);
            goto exit;
        }
    }
#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    else if (os_strcmp(argv[1], "prompt_tone") == 0)
    {
        if (argc < 3)
        {
            LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
            return;
        }

        uint32_t tone_id = 0;

        if (os_strcmp(argv[2], "start") == 0)
        {
            if (argc < 6)
            {
                LOGE("%s, %d, argc: %d not right\n", __func__, __LINE__, argc);
                return;
            }

            tone_id = os_strtoul(argv[3], NULL, 10);

            /* step 1: create player */
            bk_player_cfg_t player_cfg = DEFAULT_PLAYER_NOT_PLAYBACK_CONFIG();
            if (tone_id == 1)
            {
                player_cfg.event_handle = player1_not_playback_event_handler;
            }
            else
            {
                player_cfg.event_handle = player_not_playback_event_handler;
            }
            player_cfg.args = NULL;
            bk_player_handle_t tmp_player_handle = NULL;
            if (tone_id == 1)
            {
                gl_player1_handle = bk_player_create(&player_cfg);
                if (!gl_player1_handle)
                {
                    LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                    goto exit;
                }
                tmp_player_handle = gl_player1_handle;
            }
            else
            {
                gl_player_handle = bk_player_create(&player_cfg);
                if (!gl_player_handle)
                {
                    LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
                    goto exit;
                }
                tmp_player_handle = gl_player_handle;
            }

            /* step 2: set uri */
            if (os_strcmp(argv[4], "array") == 0)
            {
                int array_id = os_strtoul(argv[5], NULL, 10);
                uri_info.uri_type = PLAYER_URI_TYPE_ARRAY;
                if (array_id == 0)
                {
                    uri_info.uri = (char *)asr_wakeup_prompt_tone_array;
                    uri_info.total_len = sizeof(asr_wakeup_prompt_tone_array);
                    bk_player_set_decode_type(tmp_player_handle, AUDIO_DEC_TYPE_PCM);
                }
                else if (array_id == 1)
                {
                    uri_info.uri = (char *)network_provision_prompt_tone_array;
                    uri_info.total_len = sizeof(network_provision_prompt_tone_array);
                    bk_player_set_decode_type(tmp_player_handle, AUDIO_DEC_TYPE_MP3);
                }
                else if (array_id == 2)
                {
                    uri_info.uri = (char *)low_voltage_prompt_tone_array;
                    uri_info.total_len = sizeof(low_voltage_prompt_tone_array);
                    bk_player_set_decode_type(tmp_player_handle, AUDIO_DEC_TYPE_WAV);
                }
                else
                {
                    LOGE("%s, %d, array_id: %d not right\n", __func__, __LINE__, array_id);
                    goto exit;
                }

                if (BK_OK != bk_player_set_uri(tmp_player_handle, &uri_info))
                {
                    LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
                    goto exit;
                }
            }
            else if (os_strcmp(argv[4], "vfs") == 0)
            {
                uri_info.uri_type = PLAYER_URI_TYPE_VFS;
                uri_info.uri = argv[5];

                if (BK_OK != bk_player_set_uri(tmp_player_handle, &uri_info))
                {
                    LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
                    goto exit;
                }
            }
            else
            {
                LOGE("%s, %d, uri type: %s is not support\n", __func__, __LINE__, argv[4]);
                goto exit;
            }

            /* step 3: set output port */
            ringbuf_port_cfg_t cfg = RINGBUF_PORT_CFG_DEFAULT();
            cfg.ringbuf_size = OUTPUT_RB_SIZE;
            audio_port_handle_t tmp_output_port_handle = NULL;
            if (tone_id == 1)
            {
                gl_output_port1_handle = ringbuf_port_init(&cfg);
                if (gl_output_port1_handle == NULL)
                {
                    LOGE("%s, %d, ringbuf_port_init fail\n", __func__, __LINE__);
                    goto exit;
                }
                tmp_output_port_handle = gl_output_port1_handle;
            }
            else
            {
                gl_output_port_handle = ringbuf_port_init(&cfg);
                if (gl_output_port_handle == NULL)
                {
                    LOGE("%s, %d, ringbuf_port_init fail\n", __func__, __LINE__);
                    goto exit;
                }
                tmp_output_port_handle = gl_output_port_handle;
            }

            ret = bk_player_set_output_port(tmp_player_handle, tmp_output_port_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_set_output_port fail\n", __func__, __LINE__);
                goto exit;
            }

            if (!gl_voice_service_handle)
            {
                LOGE("%s, %d, gl_voice_service_handle is NULL\n", __func__, __LINE__);
                goto exit;
            }

            /* If pipeline not has decoder, set speaker port info */
            if (os_strcmp(argv[4], "array") == 0 && os_strtoul(argv[5], NULL, 10) == 0)
            {
                spk_element = bk_voice_get_spk_element(gl_voice_service_handle);
                if (spk_element == NULL)
                {
                    LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
                    goto exit;
                }

                audio_port_info_t port_info = {0};
                port_info.chl_num = 1;
                port_info.sample_rate = 16000;
                port_info.dig_gain = 0x2d;
                port_info.ana_gain = 0x01;
                port_info.bits = 16;
                port_info.port = tmp_output_port_handle;
                if (tone_id == 1)
                {
                    port_info.port_id = 2;
                    port_info.priority = 2;
                    port_info.notify_cb = player1_not_playback_port_state_notify_handler;
                }
                else
                {
                    port_info.port_id = 1;
                    port_info.priority = 1;
                    port_info.notify_cb = player_not_playback_port_state_notify_handler;
                }
                port_info.user_data = NULL;
                if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
                {
                    LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                    goto exit;
                }
                LOGD("[%s] PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", __func__, 16000, 16, 1);
                LOGD("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
            }

            /* step 4: start player */
            ret = bk_player_start(tmp_player_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_start fail\n", __func__, __LINE__);
                goto exit;
            }
        }
        else if (os_strcmp(argv[2], "stop") == 0)
        {
            bk_player_handle_t tmp_player_handle = NULL;
            audio_port_handle_t tmp_output_port_handle = NULL;
            uint32_t tone_id = os_strtoul(argv[3], NULL, 10);
            if (tone_id == 1)
            {
                tmp_player_handle = gl_player1_handle;
                tmp_output_port_handle = gl_output_port1_handle;
            }
            else
            {
                tmp_player_handle = gl_player_handle;
                tmp_output_port_handle = gl_output_port_handle;
            }

            ret = bk_player_stop(tmp_player_handle);
            if (ret != BK_OK)
            {
                LOGE("%s, %d, bk_player_stop fail\n", __func__, __LINE__);
            }

            spk_element = bk_voice_get_spk_element(gl_voice_service_handle);
            if (spk_element == NULL)
            {
                LOGE("%s, %d, bk_voice_get_spk_element fail\n", __func__, __LINE__);
                return;
            }

            audio_port_info_t port_info = {0};
            port_info.chl_num = 1;
            port_info.sample_rate = 8000;
            port_info.dig_gain = 0x2d;
            port_info.ana_gain = 0x01;
            port_info.bits = 16;
            if (tone_id == 1)
            {
                port_info.port_id = 2;
                port_info.priority = 2;
            }
            else
            {
                port_info.port_id = 1;
                port_info.priority = 1;
            }
            port_info.port = NULL;
            if (BK_OK != onboard_speaker_stream_set_input_port_info(spk_element, &port_info))
            {
                LOGE("%s, %d, audio_element_set_multi_input_port fail\n", __func__, __LINE__);
                goto exit;
            }

            if (BK_OK != bk_player_set_output_port(tmp_player_handle, NULL))
            {
                LOGE("%s, %d, bk_player_set_output_port fail\n", __func__, __LINE__);
                goto exit;
            }

            if (BK_OK != audio_port_deinit(tmp_output_port_handle))
            {
                LOGE("%s, %d, audio_port_deinit fail\n", __func__, __LINE__);
                goto exit;
            }

            if (tone_id == 1)
            {
                gl_output_port1_handle = NULL;
            }
            else
            {
                gl_output_port_handle = NULL;
            }

            if (BK_OK != bk_player_destroy(tmp_player_handle))
            {
                LOGE("%s, %d, bk_player_destroy fail\n", __func__, __LINE__);
                goto exit;
            }

            if (tone_id == 1)
            {
                gl_player1_handle = NULL;
            }
            else
            {
                gl_player_handle = NULL;
            }
        }
        else
        {
            LOGE("%s, %d, cmd: %s is not support fail\n", __func__, __LINE__, argv[2]);
            goto exit;
        }
    }
#endif
    else
    {
        LOGE("%s, %d, cmd: %s is not support fail\n", __func__, __LINE__, argv[1]);
    }

    LOGD("%s ---complete\n", __func__);
    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

    return;

exit:

    if (gl_player_handle)
    {
        bk_player_stop(gl_player_handle);
#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        if (gl_output_port_handle)
        {
            audio_port_info_t port_info = {0};
            port_info.port_id = 1;
            port_info.priority = 1;
            port_info.port = NULL;
            onboard_speaker_stream_set_input_port_info(spk_element, &port_info);
            bk_player_set_output_port(gl_player_handle, NULL);
            audio_port_deinit(gl_output_port_handle);
        }
#endif
        bk_player_destroy(gl_player_handle);
        gl_player_handle = NULL;
    }

#if CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (gl_player1_handle)
    {
        bk_player_stop(gl_player1_handle);
        if (gl_output_port1_handle)
        {
            audio_port_info_t port_info = {0};
            port_info.port_id = 2;
            port_info.priority = 2;
            port_info.port = NULL;
            onboard_speaker_stream_set_input_port_info(spk_element, &port_info);
            bk_player_set_output_port(gl_player1_handle, NULL);
            audio_port_deinit(gl_output_port1_handle);
        }
        bk_player_destroy(gl_player1_handle);
        gl_player1_handle = NULL;
    }
#endif

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

    return;
}

#define PLAYER_SERVICE_CMD_CNT   (sizeof(s_player_service_commands) / sizeof(struct cli_command))

static const struct cli_command s_player_service_commands[] =
{
    /* player_service {playback cmd source_type info}
     *
     * [cmd]            start/stop
     * [source_type]    array|vfs
     * [info]           array_id|url
     */
    {"player_service", "player_service playback cmd source_type info", cli_player_service_test_cmd},

    /* player_service {prompt_tone cmd tone_id source_type info}
     *
     * [cmd]            start/stop
     * [tone_id]        1/2
     * [source_type]    array|vfs
     * [info]           array_id|url
     */
    {"player_service", "player_service prompt_tone cmd tone_id source_type info", cli_player_service_test_cmd},
};

int cli_player_service_init(void)
{
    return cli_register_commands(s_player_service_commands, PLAYER_SERVICE_CMD_CNT);
}
