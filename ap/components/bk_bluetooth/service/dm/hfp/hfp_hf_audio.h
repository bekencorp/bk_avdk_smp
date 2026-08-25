#pragma once

#include <components/system.h>
#include <stdint.h>
#include <stdbool.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Start the HFP SCO audio engine (speaker/mic tasks + ring buffer).
 * codec: CODEC_VOICE_CVSD / CODEC_VOICE_MSBC (see bk_dm_hfp_types.h).
 * peer_addr: 6-byte SCO peer address used for uplink voice output. */
void hfp_hf_audio_start(uint8_t codec, const uint8_t *peer_addr);

/* Stop the HFP SCO audio engine and tear down the speaker/mic tasks. */
void hfp_hf_audio_stop(void);

/* Feed one downlink SCO voice packet to the speaker ring buffer.
 * The caller still owns and frees the data buffer. */
void hfp_hf_audio_handle_data(const uint8_t *data, uint16_t len);

/* Map an HFP speaker volume step to a DAC gain and apply it. Returns the gain in dB. */
float hfp_hf_audio_set_gain(uint8_t hfp_vol);

/* Provide external-amp PA control from the product/board layer. The GPIO stays
 * owned by the caller; this module just forwards it into audio_play_cfg at open.
 * pa==NULL or pa_ctrl_en=false leaves PA control off. Call once before a call. */
void hfp_hf_audio_set_pa_ctrl(const onboard_speaker_pa_ctrl_t *pa);

/* Block until the speaker/mic tasks have fully exited. */
int32_t hfp_hf_audio_wait_player_end(void);

#if CONFIG_AUD_PARAM_CTRL
struct audio_play;
struct audio_record;
/* Bind/unbind the HFP downlink (speaker) / uplink (mic) to the param-ctrl
 * framework so the tuning tool can push EQ coefficients into the EQ nodes. */
void bt_hfp_audio_dl_bind(struct audio_play *play, uint32_t sample_rate);
void bt_hfp_audio_dl_unbind(void);
void bt_hfp_audio_ul_bind(struct audio_record *record, uint32_t sample_rate);
void bt_hfp_audio_ul_unbind(void);
struct _app_eq_t;
/* Bake the rate-matched HFP downlink (speaker) / uplink (mic) EQ preset into a
 * create-time eq_cal_para (call before audio_play_create / audio_record_create):
 *   - app_eq_en gates whether the preset coefficients are copied in;
 *   - the return value is the preset .eq_en, used to drive cfg.eq_enable.
 * The post-open bind still registers the running element with the debug tool. */
int bt_hfp_audio_dl_fill_eq(struct _app_eq_t *eq_cal_para, uint32_t sample_rate);
int bt_hfp_audio_ul_fill_eq(struct _app_eq_t *eq_cal_para, uint32_t sample_rate);
#endif

#ifdef __cplusplus
}
#endif
