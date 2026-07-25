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


#ifndef __HINT_SERVICE_H__
#define __HINT_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_err.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * hint_service: prompt-tone (HINT) source for the persistent spk_service.
 *
 * The DAC can hardware-mix three independent sources (A2DP music / CALL voice /
 * HINT prompt). This module owns the HINT source: it brings up the shared
 * spk_service (idempotent), attaches HINT as an auxiliary source and streams a
 * short 16k/mono/16bit PCM prompt tone into it, so the prompt is mixed on top of
 * any music or call already playing without disturbing them.
 *
 * A raw PCM clip is streamed straight into the HINT aux source; WAV/MP3 clips
 * are routed through the audio_play decode engine first (raw bytes -> decoder ->
 * PCM) and the decoded PCM is then streamed into HINT. The source (memory array
 * today, VFS later) is decoupled from the decoder and the output sink.
 */

/**
 * @brief  Prompt clip container format.
 */
typedef enum
{
    HINT_FMT_PCM = 0,    /*!< raw PCM (16-bit little-endian; add HINT_FMT_PCM24 later if needed) */
    HINT_FMT_WAV,        /*!< RIFF/WAVE (PCM payload), decoded via audio_play */
    HINT_FMT_MP3,        /*!< MP3, decoded via audio_play */
} hint_format_t;

/**
 * @brief  Play a prompt clip (async), auto-routing by container format.
 *
 * The buffer must stay valid until playback finishes; pass a const/static clip.
 * For WAV/MP3 the sample rate / channel count are discovered from the decoder,
 * so @p pcm_rate / @p pcm_chans are only used by HINT_FMT_PCM.
 *
 * @param[in] buf        clip bytes (PCM samples, or an encoded WAV/MP3 blob)
 * @param[in] bytes      size of @p buf in bytes
 * @param[in] fmt        container format (see hint_format_t)
 * @param[in] pcm_rate   PCM sample rate in Hz (HINT_FMT_PCM only)
 * @param[in] pcm_chans  PCM channel count 1/2 (HINT_FMT_PCM only)
 *
 * @return BK_OK if playback started, otherwise error.
 */
bk_err_t hint_service_play(const void *buf, uint32_t bytes, hint_format_t fmt,
                           uint32_t pcm_rate, uint8_t pcm_chans);

/**
 * @brief  Play a caller-supplied 16-bit mono PCM prompt clip (async).
 *         Thin wrapper over hint_service_play(HINT_FMT_PCM, mono).
 *
 * @param[in] pcm       16-bit signed mono PCM samples (little endian)
 * @param[in] bytes     size of @p pcm in bytes
 * @param[in] sampRate  PCM sample rate in Hz (e.g. 16000)
 *
 * @return BK_OK if playback started, otherwise error.
 */
bk_err_t hint_service_play_pcm(const void *pcm, uint32_t bytes, uint32_t sampRate);

/**
 * @brief  Stop the current prompt tone (if any) and detach the HINT source.
 *
 * @return BK_OK on success.
 */
bk_err_t hint_service_stop(void);

/**
 * @brief  Whether a prompt tone is currently playing.
 */
bool hint_service_is_playing(void);

#ifdef __cplusplus
}
#endif

#endif /* __HINT_SERVICE_H__ */
