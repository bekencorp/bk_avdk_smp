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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>

/* SBC frame header size */
#define SBC_HEADER_SIZE   (4)

/* Normative SBC constants / limits */
#define SBC_PROBE_SIZE    SBC_HEADER_SIZE
#define SBC_MAX_SUBBANDS  (8)
#define SBC_MAX_BLOCKS    (16)
#define SBC_MAX_SAMPLES   (SBC_MAX_BLOCKS * SBC_MAX_SUBBANDS)
#define SBC_MSBC_SAMPLES  (120)
#define SBC_MSBC_SIZE     (57)

enum sbc_freq {
    SBC_FREQ_16K,
    SBC_FREQ_32K,
    SBC_FREQ_44K1,
    SBC_FREQ_48K,
    SBC_NUM_FREQ
};

enum sbc_mode {
    SBC_MODE_MONO,
    SBC_MODE_DUAL_CHANNEL,
    SBC_MODE_STEREO,
    SBC_MODE_JOINT_STEREO,
    SBC_NUM_MODE
};

enum sbc_bam {
    SBC_BAM_LOUDNESS,
    SBC_BAM_SNR,
    SBC_NUM_BAM
};

struct sbc_frame {
    bool msbc;
    enum sbc_freq freq;
    enum sbc_mode mode;
    enum sbc_bam bam;
    int nblocks;
    int nsubbands;
    int bitpool;
};

#define SBC_DELAY_SUBBANDS(nsubbands) (10 * (nsubbands))
#define SBC_DELAY(frame) SBC_DELAY_SUBBANDS((frame)->nsubbands)

struct sbc_dstate {
    int idx;
    int16_t alignas(sizeof(int)) v[2][SBC_MAX_SUBBANDS][10];
};

struct sbc_estate {
    int idx;
    int16_t alignas(sizeof(int)) x[2][SBC_MAX_SUBBANDS][5];
    int32_t y[4];
};

typedef struct sbc {
    int nchannels;
    int nblocks;
    int nsubbands;
    union {
        struct sbc_dstate dstates[2];
        struct sbc_estate estates[2];
    };
} sbc_t;

#ifdef __cplusplus
}
#endif
