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


#ifndef __AUDIO_PLAY_H__
#define __AUDIO_PLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>
#include <driver/aud_dac_types.h>


typedef enum
{
    AUDIO_PLAY_UNKNOWN = 0,

    //AUDIO_PLAY_DEVICE,
    //AUDIO_PLAY_FILE,
    AUDIO_PLAY_ONBOARD_SPEAKER,
    //AUDIO_SINK_NET,
} audio_play_type_t;

typedef enum
{
    AUDIO_PLAY_DECODER_PCM = 0,
    AUDIO_PLAY_DECODER_SBC,
    AUDIO_PLAY_DECODER_AAC,
    AUDIO_PLAY_DECODER_MSBC,
    AUDIO_PLAY_DECODER_MP3,
    AUDIO_PLAY_DECODER_WAV,
} audio_play_decoder_t;

typedef enum {
	AUDIO_PLAY_MODE_DIFFEN = 0,
	AUDIO_PLAY_MODE_SIGNAL_END,
	AUDIO_PLAY_MODE_MAX,
} audio_play_mode_t;

typedef enum
{
    AUDIO_PLAY_STA_IDLE = 0,
    AUDIO_PLAY_STA_RUNNING,
    AUDIO_PLAY_STA_PAUSED,
} audio_play_sta_t;

typedef enum
{
    AUDIO_PLAY_PAUSE,
    AUDIO_PLAY_RESUME,
    AUDIO_PLAY_MUTE,
    AUDIO_PLAY_UNMUTE,
    AUDIO_PLAY_SET_VOLUME,
} audio_play_ctl_t;

/**
 * @brief  PCM sink callback for "external sink" mode.
 *
 * When set in audio_play_cfg_t, audio_play does NOT create its own onboard
 * speaker. Instead the pipeline runs raw -> [decoder] -> raw_read and a pump
 * task delivers each decoded PCM frame to this callback, so the caller can
 * route it to a shared/persistent speaker (e.g. spk_service).
 *
 * @param[in] user    user pointer from audio_play_cfg_t.pcm_sink_user
 * @param[in] pcm     interleaved PCM buffer
 * @param[in] len     bytes available in pcm
 *
 * @return  number of bytes consumed (normally len), or <0 on fatal error
 */
typedef int (*audio_play_pcm_sink_t)(void *user, void *pcm, uint32_t len);

typedef struct
{
    uint8_t port;                   /*!< select port when connect multiple speaker, default 0 when connect one device */
    uint8_t nChans;
    uint32_t sampRate;
    uint8_t bitsPerSample;
    float volume;                  /*!< speaker digital gain in dB */
    audio_play_mode_t play_mode;
    uint32_t frame_size;            /*!< frame size unit byte */
    uint32_t pool_size;             /*!< the size (unit byte) of ringbuffer pool saved speaker data need to play */
    uint32_t                dac_source_bitmap;  /*!< bitmap of active dac source,bit[x]:0:source_x inactive;1:source_x active*/
    aud_dac_source_t        main_dac_source;    /*!< main input source mapped to element->in */
    audio_play_decoder_t    decoder_type;       /*!< decoder type, PCM means input data is pcm stream */
    uint8_t                 eq_enable;          /*!< insert an EQ node before the speaker when non-zero */
    eq_algorithm_cfg_t      eq_cfg;             /*!< EQ node config, only used when eq_enable is set */
    audio_play_pcm_sink_t   pcm_sink;           /*!< non-NULL: external-sink mode, no onboard speaker is created */
    void                    *pcm_sink_user;     /*!< user pointer passed back to pcm_sink */
    bool                    pa_ctrl_en;         /*!< drive external amp GPIO around dac open/close */
    uint16_t                pa_ctrl_gpio;       /*!< PA enable GPIO id */
    uint8_t                 pa_on_level;        /*!< 0: low turns PA on, 1: high turns PA on */
    uint32_t                pa_on_delay;        /*!< ms after dac enable before PA on */
    uint32_t                pa_off_delay;       /*!< ms after PA off before dac deinit */
} audio_play_cfg_t;

#define DEFAULT_AUDIO_PLAY_CONFIG() {       \
    .port = 0,                              \
    .nChans = 1,                            \
    .sampRate = 8000,                       \
    .bitsPerSample = 16,                    \
    .volume = -7.0f,                        \
    .play_mode = AUDIO_PLAY_MODE_DIFFEN,    \
    .frame_size = 320,                      \
    .pool_size = 640,                       \
    .dac_source_bitmap = DEFAULT_ACTIVE_DAC_SOURCE_BITMAP, \
    .main_dac_source   = DEFAULT_DAC_SOURCE,               \
    .decoder_type = AUDIO_PLAY_DECODER_PCM, \
    .eq_enable = 0,                         \
    .pa_ctrl_en = false,                    \
    .pa_ctrl_gpio = 0,                      \
    .pa_on_level = 0,                       \
    .pa_on_delay = 0,                       \
    .pa_off_delay = 0,                      \
}

typedef struct audio_play audio_play_t;

typedef struct
{
    int (*open)(audio_play_t *play, audio_play_cfg_t *config);
    int (*write)(audio_play_t *play, char *buffer, uint32_t len);
    int (*control)(audio_play_t *play, audio_play_ctl_t ctl);
    int (*close)(audio_play_t *play);
} audio_play_ops_t;

struct audio_play
{
    audio_play_ops_t *ops;

    audio_play_cfg_t config;

    void *play_ctx;
};



/**
 * @brief     Create audio play with config
 *
 * This API create audio play handle according to play type and config.
 * This API should be called before other api.
 *
 * @param[in] play_type The type of play
 * @param[in] config    Play config used in audio_play_open api
 *
 * @return
 *    - Not NULL: success
 *    - NULL: failed
 */
audio_play_t *audio_play_create(  audio_play_type_t play_type, audio_play_cfg_t *config);

/**
 * @brief      Destroy audio play
 *
 * This API Destroy audio play according to audio play handle.
 *
 *
 * @param[in] play  The audio play handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_destroy(audio_play_t *play);

/**
 * @brief      Open audio play
 *
 * This API open audio play and start play.
 *
 *
 * @param[in] play  The audio play handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_open(audio_play_t *play);

/**
 * @brief      Close audio play
 *
 * This API stop play and close audio play.
 *
 *
 * @param[in] play  The audio play handle
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_close(audio_play_t *play);

/**
 * @brief      Write speaker data to audio play
 *
 * This API write speaker data to pool.
 * If memory in pool is not enough, wait until the pool has enough memory.
 *
 *
 * @param[in] play      The audio play handle
 * @param[in] buffer    The speaker data buffer
 * @param[in] len       The length (byte) of speaker data
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_write_data(audio_play_t *play, char *buffer, uint32_t len);

/**
 * @brief  Signal end-of-stream to the decode pipeline.
 *
 * For a finite in-memory clip (WAV/MP3/...) the caller pushes all encoded bytes
 * via audio_play_write_data() and then calls this once. It marks the raw source
 * element's output done so the decoder receives AEL_IO_DONE, flushes its final
 * frames and stops cleanly instead of looping on input-read timeouts.
 *
 * @param[in] play  The audio play handle
 *
 * @return BK_OK on success, otherwise error.
 */
bk_err_t audio_play_write_eos(audio_play_t *play);

/**
 * @brief  Query whether the external-sink PCM tail reader hit end-of-stream.
 *
 * In external-sink (pcm_sink) mode the pump stops as soon as the decoder has
 * drained all PCM (AEL_IO_DONE). Callers can poll this to finish promptly
 * instead of waiting on an idle timeout.
 *
 * @param[in] play  The audio play handle
 *
 * @return true once all decoded PCM has been delivered, false otherwise.
 */
bool audio_play_pcm_ended(audio_play_t *play);

/**
 * @brief      Control audio play
 *
 * This API can control audio play, such as pause, resume and so on.
 *
 *
 * @param[in] play  The audio play handle
 * @param[in] ctl   The control opcode
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_control(audio_play_t *play, audio_play_ctl_t ctl);

/**
 * @brief      Open audio play
 *
 * This API open audio play and start play.
 *
 *
 * @param[in] play      The audio play handle
 * @param[in] volume    The volume value
 *
 * @return
 *    - BK_OK: success
 *    - NULL: failed
 */
bk_err_t audio_play_set_volume(audio_play_t *play, float volume);

/**
 * @brief  Get the EQ audio element of a running audio_play (external-sink or
 *         speaker mode). Returns NULL when EQ is disabled or not created.
 *         Used by the param-ctrl framework to push tuned EQ coefficients.
 */
void *audio_play_get_eq(audio_play_t *play);

/**
 * @brief  Query the decoded PCM format discovered by the decoder element.
 *
 * Only meaningful in external-sink mode with a real decoder (MP3/WAV/...): the
 * decoder fills its output info once it has parsed the first frame, so this is
 * intended to be called from the pcm_sink callback (i.e. after the first PCM
 * frame is produced). Any out pointer may be NULL.
 *
 * @param[in]  play      audio play handle
 * @param[out] sampRate  decoded sample rate in Hz
 * @param[out] nChans    decoded channel count (1 mono / 2 stereo)
 * @param[out] bits      decoded bit width
 *
 * @return BK_OK if a valid (non-zero rate/channels) format is available.
 */
bk_err_t audio_play_get_pcm_info(audio_play_t *play, uint32_t *sampRate, uint8_t *nChans, uint8_t *bits);

#ifdef __cplusplus
}
#endif
#endif /* __AUDIO_PLAY_H__ */

