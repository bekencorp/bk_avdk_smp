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


#include "audio_para.h"

#define A2DP_EQ_TOTAL_NUM   2
#define A2DP_EQ_GAIN        16384      /* global gain, Q14 */
#define A2DP_EQ_FGAIN       0x00000000 /* front gain, IEEE-754 hex (0.0) */

//E0_freq_600_gain_n15_qval_1_type_1_LS
#define A2DP_EQ0        1
#define A2DP_EQ0FREQ    0x44160000
#define A2DP_EQ0GAIN    0xc1700000
#define A2DP_EQ0QVAL    0x3f800000
#define A2DP_EQ0FTYPE   0x01

//E1_freq_700_gain_n1_qval_1_type_0_PK
#define A2DP_EQ1        1
#define A2DP_EQ1FREQ    0x442f0000
#define A2DP_EQ1GAIN    0xbf800000
#define A2DP_EQ1QVAL    0x3f800000
#define A2DP_EQ1FTYPE   0x00


/* ---- 48000 Hz coefficients ---- */
#define A2DP_EQ0A0_48000    -1963489
#define A2DP_EQ0A1_48000    929340
#define A2DP_EQ0B0_48000    1011133
#define A2DP_EQ0B1_48000    -1969420
#define A2DP_EQ0B2_48000    960852

#define A2DP_EQ1A0_48000    -1991827
#define A2DP_EQ1A1_48000    951642
#define A2DP_EQ1B0_48000    1043305
#define A2DP_EQ1B1_48000    -1991827
#define A2DP_EQ1B2_48000    956912

/* ---- 44100 Hz coefficients ---- */
#define A2DP_EQ0A0_44100    -1951092
#define A2DP_EQ0A1_44100    919515
#define A2DP_EQ0B0_44100    1007763
#define A2DP_EQ0B1_44100    -1958081
#define A2DP_EQ0B2_44100    953339

#define A2DP_EQ1A0_44100    -1982202
#define A2DP_EQ1A1_44100    943524
#define A2DP_EQ1B0_44100    1042863
#define A2DP_EQ1B1_44100    -1982202
#define A2DP_EQ1B2_44100    949237

/* Downlink (speaker) music EQ. `rate` selects both the per-rate biquad
 * coefficient set (via token paste) and eq_load.samplerate, so bt_audio_param
 * can pick the preset matching the running A2DP stream. */
#define A2DP_EQ_DL_PRESET(rate)                             \
{                                                           \
    .app_eq_en   = 1,                                       \
    .eq_en       = 1,                                       \
    .filters     = A2DP_EQ_TOTAL_NUM,                       \
    .globle_gain = A2DP_EQ_GAIN,                            \
    .eq_para[0].a[0] = -A2DP_EQ0A0_##rate,                  \
    .eq_para[0].a[1] = -A2DP_EQ0A1_##rate,                  \
    .eq_para[0].b[0] = A2DP_EQ0B0_##rate,                   \
    .eq_para[0].b[1] = A2DP_EQ0B1_##rate,                   \
    .eq_para[0].b[2] = A2DP_EQ0B2_##rate,                   \
    .eq_para[1].a[0] = -A2DP_EQ1A0_##rate,                  \
    .eq_para[1].a[1] = -A2DP_EQ1A1_##rate,                  \
    .eq_para[1].b[0] = A2DP_EQ1B0_##rate,                   \
    .eq_para[1].b[1] = A2DP_EQ1B1_##rate,                   \
    .eq_para[1].b[2] = A2DP_EQ1B2_##rate,                   \
    .eq_load.f_gain     = A2DP_EQ_FGAIN,                    \
    .eq_load.samplerate = (rate),                           \
    .eq_load.eq_load_para[0].freq   = A2DP_EQ0FREQ,         \
    .eq_load.eq_load_para[0].gain   = A2DP_EQ0GAIN,         \
    .eq_load.eq_load_para[0].q_val  = A2DP_EQ0QVAL,         \
    .eq_load.eq_load_para[0].type   = A2DP_EQ0FTYPE,        \
    .eq_load.eq_load_para[0].enable = A2DP_EQ0,             \
    .eq_load.eq_load_para[1].freq   = A2DP_EQ1FREQ,         \
    .eq_load.eq_load_para[1].gain   = A2DP_EQ1GAIN,         \
    .eq_load.eq_load_para[1].q_val  = A2DP_EQ1QVAL,         \
    .eq_load.eq_load_para[1].type   = A2DP_EQ1FTYPE,        \
    .eq_load.eq_load_para[1].enable = A2DP_EQ1,             \
}

/* A2DP music downlink presets, one per supported stream sample rate. Each entry
 * pulls its own per-rate coefficient block. */
app_aud_eq_config_t g_a2dp_eq_dl_presets[] =
{
    A2DP_EQ_DL_PRESET(44100),
    A2DP_EQ_DL_PRESET(48000),
};

const uint32_t g_a2dp_eq_dl_preset_num =
    sizeof(g_a2dp_eq_dl_presets) / sizeof(g_a2dp_eq_dl_presets[0]);
