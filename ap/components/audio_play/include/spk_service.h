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


#ifndef __SPK_SERVICE_H__
#define __SPK_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/aud_dac_types.h>

/*
 * spk_service: a single, persistent speaker/DAC owner.
 *
 * Step 1 goal: decouple the speaker/DAC lifetime from each business (A2DP /
 * HFP / prompt). The onboard speaker element and the audio DAC are created once
 * (spk_service_init) and torn down only when all audio business ends
 * (spk_service_deinit). Businesses no longer create/destroy the DAC on every
 * switch; they only attach/detach their PCM source to a DAC source FIFO.
 *
 * All inputs are PCM (decode happens on the source side). The main source is
 * fed through the persistent "raw_write -> speaker" link; auxiliary sources are
 * fed through the speaker multi-input ports and hardware-mixed by the DAC.
 */

/* Logical playback sources, 1:1 mapped to aud_dac_source_t FIFOs. */
typedef enum
{
    SPK_SERVICE_SRC_A2DP = AUD_DAC_SOURCE_A2DP,  /*!< music (main input) */
    SPK_SERVICE_SRC_CALL = AUD_DAC_SOURCE_CALL,  /*!< HFP call voice (aux) */
    SPK_SERVICE_SRC_HINT = AUD_DAC_SOURCE_HINT,  /*!< prompt tone / TTS (aux) */
} spk_service_src_t;

/* Per-source PCM format used when a business attaches its source. */
typedef struct
{
    spk_service_src_t   src;            /*!< which DAC source FIFO to feed */
    uint8_t             nChans;         /*!< 1 mono, 2 interleaved stereo */
    uint32_t            sampRate;       /*!< PCM sample rate */
    uint8_t             bitsPerSample;  /*!< PCM bit width, 16 */
    uint32_t            frame_size;     /*!< bytes of one 20ms PCM frame */
    float               volume;         /*!< initial DAC digital gain in dB */
} spk_source_cfg_t;

/**
 * @brief  Create the persistent speaker/DAC owner.
 *
 * Builds one onboard speaker element (dac_source_bitmap covering A2DP + CALL +
 * HINT, main source = A2DP fed by an internal raw_write) plus the persistent
 * play pipeline, and starts it running. Idempotent: a second call is a no-op.
 *
 * @return BK_OK on success, otherwise a bk_err_t error.
 */
bk_err_t spk_service_init(void);

/**
 * @brief  Destroy the persistent speaker/DAC owner.
 *
 * Only call this when NO business needs audio output anymore. This is the only
 * place the DAC is de-initialised.
 */
bk_err_t spk_service_deinit(void);

/**
 * @brief  Attach (open) a PCM source before writing data to it.
 *
 * The main source (A2DP) reuses the persistent raw_write input. Auxiliary
 * sources (CALL / HINT) allocate a ring-buffer input port and bind it to a
 * speaker multi-input port. Reconfigures the DAC source sample rate/channels.
 *
 * @param[in] cfg  PCM source configuration.
 *
 * @return BK_OK on success, otherwise a bk_err_t error.
 */
bk_err_t spk_service_attach(const spk_source_cfg_t *cfg);

/**
 * @brief  Detach (close) a PCM source. The speaker/DAC stays alive.
 *
 * @param[in] src  the source to detach.
 *
 * @return BK_OK on success, otherwise a bk_err_t error.
 */
bk_err_t spk_service_detach(spk_service_src_t src);

/**
 * @brief  Write one PCM frame to an attached source.
 *
 * @param[in] src     the source to write.
 * @param[in] buffer  interleaved PCM buffer.
 * @param[in] len     bytes to write.
 *
 * @return number of bytes written (>0), or <=0 on error.
 */
int spk_service_write(spk_service_src_t src, const void *buffer, uint32_t len);

/** Sentinel for spk_service_write_ex(): block until the write completes. */
#define SPK_SERVICE_WAIT_FOREVER   (0xFFFFFFFFu)

/**
 * @brief  Write one PCM frame with an explicit back-pressure timeout.
 *
 * Unlike spk_service_write(), this NEVER drops: if the auxiliary port stays full
 * for @p timeout_ms it returns 0 (nothing written) so the caller can retry and
 * thereby pace itself to the DAC's real drain rate. Use this for local file /
 * synthesised sources (prompt/TTS) that must not lose samples.
 *
 * @param[in] src         the source to write.
 * @param[in] buffer      interleaved PCM buffer.
 * @param[in] len         bytes to write.
 * @param[in] timeout_ms  max block time, or SPK_SERVICE_WAIT_FOREVER.
 *
 * @return bytes written (>0), 0 if it would block (timed out), <0 on error.
 */
int spk_service_write_ex(spk_service_src_t src, const void *buffer, uint32_t len, uint32_t timeout_ms);

/**
 * @brief  Set the (global) DAC digital gain in dB. Used for volume today; a
 *         future step will switch to per-source gain for ducking.
 *
 * @param[in] gain_db  digital gain in dB.
 *
 * @return BK_OK on success, otherwise a bk_err_t error.
 */
bk_err_t spk_service_set_volume(float gain_db);

/**
 * @brief  Mute / unmute the whole DAC output (all sources).
 *
 * @param[in] mute  1 mute, 0 unmute.
 *
 * @return BK_OK on success, otherwise a bk_err_t error.
 */
bk_err_t spk_service_set_mute(uint8_t mute);

/**
 * @brief  Mute / unmute a single source without touching the others.
 *
 * Uses the DAC per-source gain gate (silence vs 0 dB) so, for example, the
 * A2DP start-up pop suppression no longer mutes a concurrent call. This is a
 * gate only; the master volume stays on the global digital gain.
 *
 * @param[in] src   the source to mute/unmute.
 * @param[in] mute  1 mute, 0 unmute (pass at 0 dB).
 *
 * @return BK_OK on success, otherwise a bk_err_t error.
 */
bk_err_t spk_service_set_src_mute(spk_service_src_t src, uint8_t mute);

/**
 * @brief  Whether the persistent speaker/DAC is currently created.
 *
 * @return true if initialised.
 */
bool spk_service_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPK_SERVICE_H__ */
