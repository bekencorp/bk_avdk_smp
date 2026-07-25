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
    BK_A2DP_AUDIO_OPEN_VOTE_START = 0,
    BK_A2DP_AUDIO_OPEN_VOTE_A2DP = BK_A2DP_AUDIO_OPEN_VOTE_START,
    BK_A2DP_AUDIO_OPEN_VOTE_USER,
    BK_A2DP_AUDIO_OPEN_VOTE_END,
} bk_a2dp_audio_open_vote_t;

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

#if CONFIG_AUD_PARAM_CTRL
struct audio_play;
/* Bind/unbind the A2DP downlink (music) audio_play to the param-ctrl framework
 * so the tuning tool can push EQ coefficients into its running EQ node. */
void bt_a2dp_audio_bind(struct audio_play *play, uint32_t sample_rate);
void bt_a2dp_audio_unbind(void);
struct _app_eq_t;
/* Bake the rate-matched customer A2DP downlink EQ preset into a create-time
 * eq_cal_para (no element handle needed, call before audio_play_create):
 *   - app_eq_en gates whether the preset coefficients are copied in;
 *   - the return value is the preset .eq_en, used to drive cfg.eq_enable.
 * The post-open bt_a2dp_audio_bind() still registers the running element with
 * the debug tool for live tuning / load-save. */
int bt_a2dp_audio_fill_eq(struct _app_eq_t *eq_cal_para, uint32_t sample_rate);
#endif

#ifdef __cplusplus
}
#endif
