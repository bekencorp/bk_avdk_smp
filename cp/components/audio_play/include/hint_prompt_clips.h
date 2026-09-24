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

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HINT prompt-tone test clips (all 16k/16bit/mono), used to exercise the
 * hint_service decode path. Data-only; no dependency on bk_player_service.
 *
 *   - asr_wakeup ......... raw PCM
 *   - network_provision .. MP3 (ID3 + MPEG frames)
 *   - low_voltage ........ WAV (RIFF + PCM payload)
 */
extern const char        asr_wakeup_prompt_tone_array[];
extern const unsigned int asr_wakeup_prompt_tone_array_len;

extern const char        network_provision_prompt_tone_array[];
extern const unsigned int network_provision_prompt_tone_array_len;

extern const char        low_voltage_prompt_tone_array[];
extern const unsigned int low_voltage_prompt_tone_array_len;

#ifdef __cplusplus
}
#endif
