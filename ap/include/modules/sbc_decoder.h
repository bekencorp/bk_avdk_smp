// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "sbc_decoder_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert SBC frequency enum to Hz.
 *
 * @param freq SBC frequency index.
 *
 * @return Frequency in Hz, or a negative value on error.
 */
int sbc_get_freq_hz(enum sbc_freq freq);

/**
 * @brief Calculate SBC frame size in bytes.
 *
 * @param frame SBC frame parameters.
 *
 * @return Frame size in bytes.
 */
unsigned int sbc_get_frame_size(const struct sbc_frame *frame);

/**
 * @brief Calculate SBC frame bitrate.
 *
 * @param frame SBC frame parameters.
 *
 * @return Bitrate in bps.
 */
unsigned int sbc_get_frame_bitrate(const struct sbc_frame *frame);

/**
 * @brief Reset SBC codec state.
 *
 * @param sbc SBC codec runtime state.
 */
void sbc_reset(sbc_t *sbc);

/**
 * @brief Probe and parse SBC frame header.
 *
 * @param data  Input frame data.
 * @param frame Parsed frame info output.
 *
 * @return 0 on success, negative value on failure.
 */
int sbc_probe(const void *data, struct sbc_frame *frame);

/**
 * @brief Decode one SBC frame to PCM.
 *
 * @param sbc    SBC decoder runtime state.
 * @param data   Encoded SBC frame buffer.
 * @param size   Encoded SBC frame size.
 * @param frame  SBC frame parameters.
 * @param pcml   Left channel PCM output.
 * @param pitchl Left channel sample stride.
 * @param pcmr   Right channel PCM output.
 * @param pitchr Right channel sample stride.
 *
 * @return Number of bytes consumed on success, negative value on failure.
 */
int sbc_decode(sbc_t *sbc,
    const void *data, unsigned int size, struct sbc_frame *frame,
    int16_t *pcml, int pitchl, int16_t *pcmr, int pitchr);

/**
 * @brief Encode one PCM frame to SBC.
 *
 * @param sbc    SBC encoder runtime state.
 * @param pcml   Left channel PCM input.
 * @param pitchl Left channel sample stride.
 * @param pcmr   Right channel PCM input.
 * @param pitchr Right channel sample stride.
 * @param frame  SBC frame parameters.
 * @param data   Encoded SBC output buffer.
 * @param size   Encoded SBC output buffer size.
 *
 * @return Number of bytes written on success, negative value on failure.
 */
int sbc_encode(sbc_t *sbc,
    const int16_t *pcml, int pitchl, const int16_t *pcmr, int pitchr,
    const struct sbc_frame *frame, void *data, unsigned int size);

#ifdef __cplusplus
}
#endif
