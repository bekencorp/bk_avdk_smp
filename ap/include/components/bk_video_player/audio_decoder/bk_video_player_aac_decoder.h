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

#pragma once

#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get AAC audio decoder operations.
 *
 * The decoder wraps the Helix AAC decoder and accepts AAC access units from
 * MP4/AVI container parsers. Codec-specific configuration is taken from the
 * stream's AudioSpecificConfig when available.
 *
 * @return Pointer to AAC decoder operations, NULL on failure.
 */
const video_player_audio_decoder_ops_t *bk_video_player_get_aac_decoder_ops(void);

#ifdef __cplusplus
}
#endif
