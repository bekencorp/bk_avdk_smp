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
#include <components/bk_player_service.h>
#include <components/bk_player_service_types.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#include "audio_engine_prompt_tone.h"
#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
#include "bk_posix.h"
#endif

#define TAG "prompt_tone"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define AUDIO_PROMPT_TONE_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            BK_LOGE(TAG, "%s, %d, AUDIO_PROMPT_TONE_CHECK_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)


struct audio_engine_prompt_tone
{
    bk_player_handle_t      player_handle;      /**< the prompt tone player handle */
    audio_port_handle_t     output_port_handle; /**< the prompt tone output port handle */
    uint8_t                 port_id;            /**< the valid audio port of currently reading speaker data, 0: element->in, >=1: element->multi_in */
    uint8_t                 priority;           /**< the priority of the audio port. The lower the value, the higher the priority to be processed. The default value is 0 (highest priority). */
    audio_port_state_notify notify_cb;          /**< the audio port state notify callback function */
    void                    *user_data;         /**< the user data of the prompt tone output port state notify callback function */
    audio_element_handle_t  spk_stream;         /**< the speaker stream handle */
    uint8_t                 chl_num;            /**< speaker channel number */
    uint32_t                sample_rate;        /**< speaker sample rate */
    uint8_t                 bits;               /**< Bit width */
    player_uri_type_t       uri_type;           /**< the uri type of the prompt tone */
};


static bool gl_wait_play_finish = false;
#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
static bool gl_sdcard_is_mount = false;

/* mount sdcard */
static int vfs_mount_sd0_fatfs(void)
{
	int ret = BK_OK;

	if(!gl_sdcard_is_mount)
    {
		struct bk_fatfs_partition partition;
		char *fs_name = NULL;
		fs_name = "fatfs";
		partition.part_type = FATFS_DEVICE;
		partition.part_dev.device_name = FATFS_DEV_SDCARD;
		partition.mount_path = VFS_SD_0_PATITION_0;
		ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
		gl_sdcard_is_mount = true;
        LOGI("func %s, mount /sd0 \n", __func__);
	}

    return ret;
}

/* unmount sdcard */
static bk_err_t vfs_unmount_sd0_fatfs(void)
{
    bk_err_t ret = BK_OK;

    if (!gl_sdcard_is_mount)
    {
        return BK_OK;
    }

    LOGD("func %s, unmount /sd0 \n", __func__);
    if (BK_OK != umount(VFS_SD_0_PATITION_0))
    {
        LOGE("func %s, unmount /sd0 fail\n", __func__);
        ret = BK_FAIL;
    }
    else
    {
        gl_sdcard_is_mount = false;
    }

    return ret;
}
#endif

static int player_not_playback_port_state_notify_handler(int state, void *port_info, void *user_data)
{
    LOGD("%s, %d, ++>>>>>>>> state: %d \n", __func__, __LINE__, state);

    switch (state)
    {
        case APT_STATE_RUNNING:
            break;

        case APT_STATE_PAUSED:
            break;

        case APT_STATE_FINISHED:
            /* Check whether player finish */
            if (gl_wait_play_finish)
            {
#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
                /* Prompt tone play finish, unmount sdcard */
                vfs_unmount_sd0_fatfs();
#endif
                gl_wait_play_finish = false;
            }
            break;

        default:
            break;
    }

    return BK_OK;
}

static int player_not_playback_event_handler(int data, void *params, void *args)
{
    LOGD("%s, %d, data: %d\n", __func__, __LINE__, data);

    audio_engine_prompt_tone_handle_t prompt_tone = (audio_engine_prompt_tone_handle_t)args;

    if (data == PLAYER_EVENT_MUSIC_INFO)
    {
        audio_element_info_t *music_info = (audio_element_info_t *)params;
        if (prompt_tone->spk_stream == NULL)
        {
            LOGE("%s, %d, spk_stream is NULL\n", __func__, __LINE__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = DEFAULT_AUDIO_PORT_INFO();
        port_info.chl_num = music_info->channels;
        port_info.sample_rate = music_info->sample_rates;
        port_info.bits = music_info->bits;
        port_info.port_id = 1;
        port_info.priority = 1;
        port_info.port = prompt_tone->output_port_handle;
        port_info.notify_cb = player_not_playback_port_state_notify_handler;
        port_info.user_data = prompt_tone;
        if (BK_OK != onboard_speaker_stream_set_input_port_info(prompt_tone->spk_stream, &port_info))
        {
            LOGE("%s, %d, onboard_speaker_stream_set_input_port_info fail\n", __func__, __LINE__);
            return BK_FAIL;
        }
        LOGD("[%s] PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", __func__, music_info->sample_rates, music_info->bits, music_info->channels);
        LOGD("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
    }
    else if (data == PLAYER_EVENT_FINISH)
    {
        LOGD("[%s] PLAYER_EVENT_FINISH\n", __func__);
        gl_wait_play_finish = true;
    }
    else
    {
        //nothing todo
    }

    return BK_OK;
}

audio_engine_prompt_tone_handle_t audio_engine_prompt_tone_init(audio_engine_prompt_tone_cfg_t *cfg)
{
    if (cfg == NULL)
    {
        LOGE("%s, %d, cfg is NULL\n", __func__, __LINE__);
        return NULL;
    }

    LOGD("%s \n", __func__);

    audio_engine_prompt_tone_handle_t prompt_tone = (audio_engine_prompt_tone_handle_t)psram_malloc(sizeof(struct audio_engine_prompt_tone));
    AUDIO_PROMPT_TONE_CHECK_NULL(prompt_tone, return NULL);
    os_memset(prompt_tone, 0, sizeof(struct audio_engine_prompt_tone));

    prompt_tone->port_id    = cfg->port_id;
    prompt_tone->priority   = cfg->priority;
    prompt_tone->notify_cb  = cfg->notify_cb;
    prompt_tone->user_data  = cfg->user_data;
    prompt_tone->spk_stream = cfg->spk_stream;
    prompt_tone->chl_num    = cfg->chl_num;
    prompt_tone->sample_rate = cfg->sample_rate;
    prompt_tone->bits        = cfg->bits;

    /* step 1: create player */
    bk_player_cfg_t player_cfg = DEFAULT_PLAYER_NOT_PLAYBACK_CONFIG();
    player_cfg.event_handle = player_not_playback_event_handler;
    player_cfg.args = prompt_tone;
    prompt_tone->player_handle = bk_player_create(&player_cfg);
    if (!prompt_tone->player_handle)
    {
        LOGE("%s, %d, bk_player_init fail\n", __func__, __LINE__);
        goto fail;
    }

    /* step 2: set source type and decoder type */
#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
    prompt_tone->uri_type = PLAYER_URI_TYPE_VFS;
#else
    prompt_tone->uri_type = PLAYER_URI_TYPE_ARRAY;
#endif

#if CONFIG_AE_PROMPT_TONE_DECODER_PCM
    bk_player_set_decode_type(prompt_tone->player_handle, AUDIO_DEC_TYPE_PCM);
#elif CONFIG_AE_PROMPT_TONE_DECODER_MP3
    bk_player_set_decode_type(prompt_tone->player_handle, AUDIO_DEC_TYPE_MP3);
#else
    bk_player_set_decode_type(prompt_tone->player_handle, AUDIO_DEC_TYPE_WAV);
#endif

    /* step 3: set uri to init play_pipeline */
    player_uri_info_t temp_player_uri_info = {0};
    temp_player_uri_info.uri_type = prompt_tone->uri_type;
#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
#if CONFIG_AE_PROMPT_TONE_DECODER_PCM
    temp_player_uri_info.uri = "temp.pcm";
#elif CONFIG_AE_PROMPT_TONE_DECODER_MP3
    temp_player_uri_info.uri = "temp.mp3";
#else
    temp_player_uri_info.uri = "temp.wav";
#endif
#else
    temp_player_uri_info.uri = NULL;
#endif
    temp_player_uri_info.total_len = 0;
    if (BK_OK != bk_player_set_uri(prompt_tone->player_handle, &temp_player_uri_info))
    {
        LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
        goto fail;
    }

    /* step 3: set output port */
    ringbuf_port_cfg_t port_cfg     = RINGBUF_PORT_CFG_DEFAULT();
    port_cfg.ringbuf_size           = cfg->port_rb_size;
    prompt_tone->output_port_handle = ringbuf_port_init(&port_cfg);
    if (prompt_tone->output_port_handle == NULL)
    {
        LOGE("%s, %d, ringbuf_port_init fail\n", __func__, __LINE__);
        goto fail;
    }

    if (BK_OK != bk_player_set_output_port(prompt_tone->player_handle, prompt_tone->output_port_handle))
    {
        LOGE("%s, %d, bk_player_set_output_port fail\n", __func__, __LINE__);
        goto fail;
    }

#if CONFIG_AE_PROMPT_TONE_DECODER_PCM && CONFIG_AE_PROMPT_TONE_SOURCE_ARRAY
    if (!prompt_tone->spk_stream)
    {
        LOGE("%s, %d, spk_stream is NULL\n", __func__, __LINE__);
        goto fail;
    }

    audio_port_info_t port_info = DEFAULT_AUDIO_PORT_INFO();
    port_info.chl_num     = prompt_tone->chl_num;
    port_info.sample_rate = prompt_tone->sample_rate;
    port_info.bits        = prompt_tone->bits;
    port_info.port        = prompt_tone->output_port_handle;
    port_info.port_id     = prompt_tone->port_id;
    port_info.priority    = prompt_tone->priority;
    port_info.notify_cb   = player_not_playback_port_state_notify_handler;
    port_info.user_data   = prompt_tone;
    if (BK_OK != onboard_speaker_stream_set_input_port_info(prompt_tone->spk_stream, &port_info))
    {
        LOGE("%s, %d, onboard_speaker_stream_set_input_port_info fail\n", __func__, __LINE__);
        goto fail;
    }
    LOGD("[%s] PLAYER_EVENT_MUSIC_INFO, sample_rates: %d, bits: %d, channels: %d\n", __func__, prompt_tone->sample_rate, prompt_tone->bits, prompt_tone->chl_num);
    LOGD("port_info, port_id: %d, priority: %d, port: %p\n", port_info.port_id, port_info.priority, port_info.port);
#endif

    LOGD("%s complete\n", __func__);
    return prompt_tone;

fail:
    if (prompt_tone->player_handle)
    {
        if (prompt_tone->output_port_handle)
        {
            audio_port_deinit(prompt_tone->output_port_handle);
            prompt_tone->output_port_handle = NULL;
        }

        if (prompt_tone->player_handle)
        {
            bk_player_destroy(prompt_tone->player_handle);
            prompt_tone->player_handle = NULL;
        }

        psram_free(prompt_tone);
    }

    LOGD("%s fail\n", __func__);
    return NULL;
}

bk_err_t audio_engine_prompt_tone_stop(audio_engine_prompt_tone_handle_t prompt_tone)
{
    AUDIO_PROMPT_TONE_CHECK_NULL(prompt_tone, return BK_FAIL);

    if (!prompt_tone->player_handle)
    {
        LOGE("%s player_handle is NULL\n", __func__);
        return BK_FAIL;
    }

    if (BK_OK != bk_player_stop(prompt_tone->player_handle))
    {
        LOGE("%s, %d, bk_player_stop fail\n", __func__, __LINE__);
        //return BK_FAIL;
    }

#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
    vfs_unmount_sd0_fatfs();
#endif
    gl_wait_play_finish = false;

    return BK_OK;
}

bk_err_t audio_engine_prompt_tone_start(audio_engine_prompt_tone_handle_t prompt_tone, prompt_tone_uri_info_t *uri_info)
{
    AUDIO_PROMPT_TONE_CHECK_NULL(prompt_tone, return BK_FAIL);
    AUDIO_PROMPT_TONE_CHECK_NULL(uri_info, return BK_FAIL);
    AUDIO_PROMPT_TONE_CHECK_NULL(uri_info->uri, return BK_FAIL);

    bk_player_state_t state = PLAYER_STATE_NONE;
    bk_player_get_state(prompt_tone->player_handle, &state);
    LOGD("%s, %d, bk_player_state: %d\n", __func__, __LINE__, state);

    bk_player_stop(prompt_tone->player_handle);

#if CONFIG_AE_PROMPT_TONE_SOURCE_VFS
    if (BK_OK != vfs_mount_sd0_fatfs())
    {
        LOGE("%s, %d, vfs mount fail\n", __func__, __LINE__);
        return BK_FAIL;
    }
#endif

    player_uri_info_t player_uri_info = {0};
    player_uri_info.uri_type  = prompt_tone->uri_type;
    player_uri_info.uri       = uri_info->uri;
    player_uri_info.total_len = uri_info->total_len;

    if (BK_OK != bk_player_set_uri(prompt_tone->player_handle, &player_uri_info))
    {
        LOGE("%s, %d, bk_player_set_uri fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (BK_OK != bk_player_start(prompt_tone->player_handle))
    {
        LOGE("%s, %d, bk_player_start fail\n", __func__, __LINE__);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t audio_engine_prompt_tone_deinit(audio_engine_prompt_tone_handle_t prompt_tone)
{
    if (prompt_tone == NULL)
    {
        LOGE("%s, %d, prompt_tone already deinit\n", __func__, __LINE__);
        return BK_OK;
    }

    if (prompt_tone->player_handle)
    {
        audio_engine_prompt_tone_stop(prompt_tone);

        bk_player_set_output_port(prompt_tone->player_handle, NULL);

        bk_player_destroy(prompt_tone->player_handle);
        prompt_tone->player_handle = NULL;
    }

    if (prompt_tone->output_port_handle)
    {
        audio_port_info_t port_info = DEFAULT_AUDIO_PORT_INFO();
        port_info.chl_num     = prompt_tone->chl_num;
        port_info.sample_rate = prompt_tone->sample_rate;
        port_info.bits        = prompt_tone->bits;
        port_info.port        = NULL;
        port_info.port_id     = prompt_tone->port_id;
        port_info.priority    = prompt_tone->priority;
        port_info.notify_cb   = NULL;
        port_info.user_data   = prompt_tone;
        onboard_speaker_stream_set_input_port_info(prompt_tone->spk_stream, &port_info);

        audio_port_deinit(prompt_tone->output_port_handle);
        prompt_tone->output_port_handle = NULL;
    }
    psram_free(prompt_tone);
    LOGD("%s complete\n", __func__);
    return BK_OK;
}
