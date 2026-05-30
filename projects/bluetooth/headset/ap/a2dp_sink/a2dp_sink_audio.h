#pragma once

#include <components/system.h>
#include <stdint.h>

#include "bk_a2dp_sink_service.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    HEADSET_AUDIO_OPEN_VOTE_START = 0,
    HEADSET_AUDIO_OPEN_VOTE_A2DP = HEADSET_AUDIO_OPEN_VOTE_START,
    HEADSET_AUDIO_OPEN_VOTE_USER,
    HEADSET_AUDIO_OPEN_VOTE_END,
} headset_a2dp_audio_open_vote_t;

void a2dp_sink_audio_set_config(const bk_a2dp_mcc_t *codec);
bk_err_t a2dp_sink_audio_start(const bk_a2dp_mcc_t *codec,
                               uint32_t open_vote,
                               uint8_t mix_multi_channel,
                               uint8_t volume);
bk_err_t a2dp_sink_audio_open(uint32_t open_vote,
                              uint8_t mix_multi_channel,
                              uint8_t volume);
void a2dp_sink_audio_stop(void);
void a2dp_sink_audio_handle_data(uint8_t *data,
                                 uint16_t len);
float a2dp_sink_audio_set_gain(uint8_t avrcp_vol);

int32_t a2dp_sink_audio_wait_player_end(void);

#ifdef __cplusplus
}
#endif
