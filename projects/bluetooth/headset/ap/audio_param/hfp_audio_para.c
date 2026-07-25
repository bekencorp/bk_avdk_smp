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

#define HFP_EQ_TOTAL_NUM    2
#define HFP_EQ_GAIN         16384      /* global gain, Q14 (unity) */

/* ============================ sampleRate 8K EQ params ============================ */
//E0_freq_600_gain_n15_qval_1_type_1_LS
#define HFP_EQ0_8K      1
#define HFP_EQ0A0_8K    -1201893
#define HFP_EQ0A1_8K    533883
#define HFP_EQ0B0_8K    821528
#define HFP_EQ0B1_8K    -1358339
#define HFP_EQ0B2_8K    604484
#define HFP_EQ0FREQ_8K  0x44160000
#define HFP_EQ0GAIN_8K  0xc1700000
#define HFP_EQ0QVAL_8K  0x3f800000
#define HFP_EQ0FTYPE_8K 0x01

//E1_freq_700_gain_n1_qval_1_type_0_PK
#define HFP_EQ1_8K      1
#define HFP_EQ1A0_8K    -1400545
#define HFP_EQ1A1_8K    594021
#define HFP_EQ1B0_8K    1023859
#define HFP_EQ1B1_8K    -1400545
#define HFP_EQ1B2_8K    618737
#define HFP_EQ1FREQ_8K  0x442f0000
#define HFP_EQ1GAIN_8K  0xbf800000
#define HFP_EQ1QVAL_8K  0x3f800000
#define HFP_EQ1FTYPE_8K 0x00

#define HFP_EQSAMP_8K   0x1f40
#define HFP_EQFGAIN_8K  0x00000000

/* ============================ sampleRate 16K EQ params =========================== */
//E0_freq_600_gain_n15_qval_1_type_1_LS
#define HFP_EQ0_16K      1
#define HFP_EQ0A0_16K    -1668051
#define HFP_EQ0A1_16K    734106
#define HFP_EQ0B0_16K    934084
#define HFP_EQ0B1_16K    -1715175
#define HFP_EQ0B2_16K    801474
#define HFP_EQ0FREQ_16K  0x44160000
#define HFP_EQ0GAIN_16K  0xc1700000
#define HFP_EQ0QVAL_16K  0x3f800000
#define HFP_EQ0FTYPE_16K 0x01

//E1_freq_700_gain_n1_qval_1_type_0_PK
#define HFP_EQ1_16K      1
#define HFP_EQ1A0_16K    -1764716
#define HFP_EQ1A1_16K    784980
#define HFP_EQ1B0_16K    1034243
#define HFP_EQ1B1_16K    -1764716
#define HFP_EQ1B2_16K    799312
#define HFP_EQ1FREQ_16K  0x442f0000
#define HFP_EQ1GAIN_16K  0xbf800000
#define HFP_EQ1QVAL_16K  0x3f800000
#define HFP_EQ1FTYPE_16K 0x00

#define HFP_EQSAMP_16K   0x3e80
#define HFP_EQFGAIN_16K  0x00000000

/* app_eq_en selects whether this EQ family feeds the debug tool's active view;
 * pass it in so downlink and uplink presets can be distinguished. */
#define HFP_EQ_8K_PRESET(app_en)                        \
{                                                       \
    .app_eq_en   = (app_en),                            \
    .eq_en       = 1,                                   \
    .filters     = HFP_EQ_TOTAL_NUM,                    \
    .globle_gain = HFP_EQ_GAIN,                         \
    .eq_para[0].a[0] = -HFP_EQ0A0_8K,                   \
    .eq_para[0].a[1] = -HFP_EQ0A1_8K,                   \
    .eq_para[0].b[0] = HFP_EQ0B0_8K,                    \
    .eq_para[0].b[1] = HFP_EQ0B1_8K,                    \
    .eq_para[0].b[2] = HFP_EQ0B2_8K,                    \
    .eq_para[1].a[0] = -HFP_EQ1A0_8K,                   \
    .eq_para[1].a[1] = -HFP_EQ1A1_8K,                   \
    .eq_para[1].b[0] = HFP_EQ1B0_8K,                    \
    .eq_para[1].b[1] = HFP_EQ1B1_8K,                    \
    .eq_para[1].b[2] = HFP_EQ1B2_8K,                    \
    .eq_load.f_gain     = HFP_EQFGAIN_8K,               \
    .eq_load.samplerate = 8000,                         \
    .eq_load.eq_load_para[0].freq   = HFP_EQ0FREQ_8K,   \
    .eq_load.eq_load_para[0].gain   = HFP_EQ0GAIN_8K,   \
    .eq_load.eq_load_para[0].q_val  = HFP_EQ0QVAL_8K,   \
    .eq_load.eq_load_para[0].type   = HFP_EQ0FTYPE_8K,  \
    .eq_load.eq_load_para[0].enable = HFP_EQ0_8K,       \
    .eq_load.eq_load_para[1].freq   = HFP_EQ1FREQ_8K,   \
    .eq_load.eq_load_para[1].gain   = HFP_EQ1GAIN_8K,   \
    .eq_load.eq_load_para[1].q_val  = HFP_EQ1QVAL_8K,   \
    .eq_load.eq_load_para[1].type   = HFP_EQ1FTYPE_8K,  \
    .eq_load.eq_load_para[1].enable = HFP_EQ1_8K,       \
}

#define HFP_EQ_16K_PRESET(app_en)                       \
{                                                       \
    .app_eq_en   = (app_en),                            \
    .eq_en       = 1,                                   \
    .filters     = HFP_EQ_TOTAL_NUM,                    \
    .globle_gain = HFP_EQ_GAIN,                         \
    .eq_para[0].a[0] = -HFP_EQ0A0_16K,                  \
    .eq_para[0].a[1] = -HFP_EQ0A1_16K,                  \
    .eq_para[0].b[0] = HFP_EQ0B0_16K,                   \
    .eq_para[0].b[1] = HFP_EQ0B1_16K,                   \
    .eq_para[0].b[2] = HFP_EQ0B2_16K,                   \
    .eq_para[1].a[0] = -HFP_EQ1A0_16K,                  \
    .eq_para[1].a[1] = -HFP_EQ1A1_16K,                  \
    .eq_para[1].b[0] = HFP_EQ1B0_16K,                   \
    .eq_para[1].b[1] = HFP_EQ1B1_16K,                   \
    .eq_para[1].b[2] = HFP_EQ1B2_16K,                   \
    .eq_load.f_gain     = HFP_EQFGAIN_16K,              \
    .eq_load.samplerate = 16000,                        \
    .eq_load.eq_load_para[0].freq   = HFP_EQ0FREQ_16K,  \
    .eq_load.eq_load_para[0].gain   = HFP_EQ0GAIN_16K,  \
    .eq_load.eq_load_para[0].q_val  = HFP_EQ0QVAL_16K,  \
    .eq_load.eq_load_para[0].type   = HFP_EQ0FTYPE_16K, \
    .eq_load.eq_load_para[0].enable = HFP_EQ0_16K,      \
    .eq_load.eq_load_para[1].freq   = HFP_EQ1FREQ_16K,  \
    .eq_load.eq_load_para[1].gain   = HFP_EQ1GAIN_16K,  \
    .eq_load.eq_load_para[1].q_val  = HFP_EQ1QVAL_16K,  \
    .eq_load.eq_load_para[1].type   = HFP_EQ1FTYPE_16K, \
    .eq_load.eq_load_para[1].enable = HFP_EQ1_16K,      \
}

/* HFP voice downlink (speaker) presets: 8k CVSD / 16k mSBC. */
app_aud_eq_config_t g_hfp_eq_dl_presets[] =
{
    HFP_EQ_8K_PRESET(1),
    HFP_EQ_16K_PRESET(1),
};

const uint32_t g_hfp_eq_dl_preset_num =
    sizeof(g_hfp_eq_dl_presets) / sizeof(g_hfp_eq_dl_presets[0]);

app_aud_eq_config_t g_hfp_eq_ul_presets[] =
{
    HFP_EQ_8K_PRESET(1),
    HFP_EQ_16K_PRESET(1),
};

const uint32_t g_hfp_eq_ul_preset_num =
    sizeof(g_hfp_eq_ul_presets) / sizeof(g_hfp_eq_ul_presets[0]);
