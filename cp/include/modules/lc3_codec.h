// Copyright 2020-2021 Beken
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
{
#endif


    /**
     * Frame duration 7.5ms or 10ms
     */

    enum lc3_dt {
        LC3_DT_7M5,
        LC3_DT_10M,

        LC3_NUM_DT
    };

    /**
     * Sampling frequency
     */

    enum lc3_srate {
        LC3_SRATE_8K,
        LC3_SRATE_16K,
        LC3_SRATE_24K,
        LC3_SRATE_32K,
        LC3_SRATE_48K,

        LC3_NUM_SRATE,
    };


    /**
     * Encoder state and memory
     */

    typedef struct lc3_attdet_analysis {
        int32_t en1, an1;
        int p_att;
    } lc3_attdet_analysis_t;

    struct lc3_ltpf_hp50_state {
        int64_t s1, s2;
    };

    typedef struct lc3_ltpf_analysis {
        bool active;
        int pitch;
        float nc[2];

        struct lc3_ltpf_hp50_state hp50;
        int16_t x_12k8[384];
        int16_t x_6k4[178];
        int tc;
    } lc3_ltpf_analysis_t;

    typedef struct lc3_spec_analysis {
        float nbits_off;
        int nbits_spare;
    } lc3_spec_analysis_t;

    struct lc3_encoder {
        enum lc3_dt dt;
        enum lc3_srate sr, sr_pcm;

        lc3_attdet_analysis_t attdet;
        lc3_ltpf_analysis_t ltpf;
        lc3_spec_analysis_t spec;

        int xt_off, xs_off, xd_off;
        float x[1];
    };


    /**
     * Handle
     */

    typedef struct lc3_encoder *lc3_encoder_t;
    typedef struct lc3_decoder *lc3_decoder_t;


#define MAXCH  2
    typedef struct
    {
        int16_t channel;
        int16_t frame_us;
        int16_t frame_samples;
        int16_t frame_bytes;
        int32_t bitrate;
        int32_t enc_srate_hz;
        int32_t pcm_sbytes;
        int32_t frame_cnt;
        lc3_encoder_t encoder[MAXCH];
    } lc3_enc_info_t;

    typedef struct
    {
        int16_t channel;
        int16_t frame_us;
        int16_t frame_samples;
        int16_t frame_bytes;
        int32_t bitrate;
        int32_t dec_srate_hz;
        int32_t pcm_sbytes;
        int32_t frame_cnt;
        lc3_decoder_t decoder[MAXCH];
    } lc3_dec_info_t;

    void lc3_encoder_init(lc3_enc_info_t *lc3_enc_info_t, uint8_t *buff);
    void lc3_encoder_proc(lc3_enc_info_t *lc3_enc_info_t, uint8_t *pcm_data, uint8_t *compress_data);

    void lc3_decoder_init(lc3_dec_info_t *lc3_dec_info_t, uint8_t *buff);
    void lc3_decoder_proc(lc3_dec_info_t *lc3_dec_info_t, uint8_t *compress_data, uint8_t *pcm_data);

    uint32_t lc3_codec_ver();

#ifdef __cplusplus
}
#endif
