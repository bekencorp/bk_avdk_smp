#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t aec;
    uint8_t uac;
    uint8_t rmt_recorder_fmt; /* codec_format_t */
    uint8_t rmt_player_fmt;   /* codec_format_t */
    uint32_t rmt_recorder_sample_rate;
    uint32_t rmt_player_sample_rate;
    uint8_t asr;
} audio_parameters_t;


#ifdef __cplusplus
}
#endif
