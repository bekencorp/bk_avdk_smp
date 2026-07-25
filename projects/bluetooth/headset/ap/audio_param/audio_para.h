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

#include <stdint.h>
#include <components/audio_param_ctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Headset EQ coefficient presets, consumed by bt_audio_param.c and pushed into
 * the running EQ node via the param-ctrl framework. Each preset carries its
 * target stream sample rate in eq_load.samplerate; bt_audio_param picks the one
 * that matches the running stream.
 *
 * The tables in a2dp_audio_para.c / hfp_audio_para.c follow the doorbell
 * audio_para.c format (documented per-band coefficient blocks + eq_load tool
 * reflection). They ship EXAMPLE tunings; replace the coefficients with tuned
 * values per product. A2DP music runs at 44.1k/48k, HFP voice at 8k (CVSD) /
 * 16k (mSBC); A2DP has downlink EQ only, HFP has both downlink and uplink EQ.
 *
 * TODO(sys/AEC): a full customer tuning file (cf. doorbell) also carries
 *   - sys_config    : mic/spk digital+analog gains (app_aud_sys_config_t)
 *   - aec_v3_config : AEC/NS/VAD for the HFP voice uplink
 * These are intentionally omitted for now because the headset audio_play /
 * audio_record objects only expose an EQ accessor (audio_play_get_eq /
 * audio_record_get_eq). To wire them, add spk/mic/aec element accessors on the
 * SDK side, extend bt_audio_param.c adapters (get_spk_info/get_mic_info/
 * get_aec_alg) and put app_aud_sys_config_t/app_aud_aec_v3_config_t here.
 */
extern app_aud_eq_config_t g_a2dp_eq_dl_presets[];
extern const uint32_t      g_a2dp_eq_dl_preset_num;

extern app_aud_eq_config_t g_hfp_eq_dl_presets[];
extern const uint32_t      g_hfp_eq_dl_preset_num;

extern app_aud_eq_config_t g_hfp_eq_ul_presets[];
extern const uint32_t      g_hfp_eq_ul_preset_num;

#ifdef __cplusplus
}
#endif
