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

#pragma once

#include <common/bk_include.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prompt tone URI information structure 
 * 
 * This structure holds the URI information for a prompt tone.
 */
typedef struct
{
    char *uri;
    uint32_t total_len;
} prompt_tone_uri_info_t;

/**
 * @brief Audio engine prompt tone configuration structure
 */
typedef struct {
    uint8_t                 port_id;            /*!< The audio input port ID, must be >= 1 (port 0 is reserved) */
    uint8_t                 priority;           /*!< the priority of the audio port. The lower the value, the higher the priority to be processed. The default value is 0 (highest priority). */
    uint32_t                port_rb_size;       /*!< the size of the prompt tone output port ring buffer in bytes */
    audio_port_state_notify notify_cb;          /*!< the audio port state notify callback function */
    void                    *user_data;         /*!< the user data of the prompt tone output port state notify callback function */
    audio_element_handle_t  spk_stream;         /*!< the speaker stream handle */
    uint8_t                 chl_num;            /*!< speaker channel number, effective when the input audio data is raw PCM data */
    uint32_t                sample_rate;        /*!< speaker sample rate，effective when the input audio data is raw PCM data */
    uint8_t                 bits;               /*!< Bit width，effective when the input audio data is raw PCM data */
} audio_engine_prompt_tone_cfg_t;

/**
 * @brief Default prompt tone configuration
 *
 * This configuration provides default settings for the audio engine prompt tone.
 * It uses port 1, mono channel, 16kHz sample rate, and 16-bit audio format.
 */
#define DEFAULT_AUDIO_ENGINE_PROMPT_TONE_CONFIG() {     \
    .port_id = 1,                                       \
    .priority = 1,                                      \
    .port_rb_size = 4096,                               \
    .notify_cb = NULL,                                  \
    .user_data = NULL,                                  \
    .spk_stream = NULL,                                 \
    .chl_num = 1,                                       \
    .sample_rate = 16000,                               \
    .bits = 16,                                         \
}


/**
 * @brief Audio engine prompt tone handle type
 *
 * This typedef defines a pointer to the audio engine prompt tone structure.
 */
typedef struct audio_engine_prompt_tone *audio_engine_prompt_tone_handle_t;

/**
 * @brief      Initialize the audio engine prompt tone
 *
 * This function creates and initializes a prompt tone instance. The prompt tone can
 * play audio files or raw PCM data through the specified speaker stream. For default
 * configuration, refer to DEFAULT_AUDIO_ENGINE_PROMPT_TONE_CONFIG().
 *
 * @param[in]  cfg  The prompt tone configuration
 *
 * @return     The prompt tone handle
 *             - Not NULL: Success
 *             - NULL: Failed
 */
audio_engine_prompt_tone_handle_t audio_engine_prompt_tone_init(audio_engine_prompt_tone_cfg_t *cfg);

/**
 * @brief      Stop the audio engine prompt tone
 *
 * This function stops the currently playing prompt tone and releases the associated
 * audio port.
 *
 * @param[in]  prompt_tone  The prompt tone handle
 *
 * @return
 *             - BK_OK: Success
 *             - BK_FAIL: Failed
 */
bk_err_t audio_engine_prompt_tone_stop(audio_engine_prompt_tone_handle_t prompt_tone);

/**
 * @brief      Start playing the audio engine prompt tone
 *
 * This function starts playing a prompt tone from the specified URI. The URI can
 * point to an audio file (e.g., MP3, WAV) or raw PCM data. The audio data will be
 * decoded (if necessary) and sent to the speaker stream for playback.
 *
 * @param[in]  prompt_tone  The prompt tone handle
 * @param[in]  uri_info     The URI information of the prompt tone to play
 *
 * @return
 *             - BK_OK: Success
 *             - BK_FAIL: Failed
 */
bk_err_t audio_engine_prompt_tone_start(audio_engine_prompt_tone_handle_t prompt_tone, prompt_tone_uri_info_t *uri_info);

/**
 * @brief      Deinitialize the audio engine prompt tone
 *
 * This function stops the prompt tone playback (if active), releases all allocated
 * resources, and destroys the prompt tone instance. After calling this function,
 * the prompt_tone handle becomes invalid and should not be used.
 *
 * @param[in]  prompt_tone  The prompt tone handle
 *
 * @return
 *             - BK_OK: Success
 *             - BK_FAIL: Failed
 */
bk_err_t audio_engine_prompt_tone_deinit(audio_engine_prompt_tone_handle_t prompt_tone);


#ifdef __cplusplus
}
#endif
