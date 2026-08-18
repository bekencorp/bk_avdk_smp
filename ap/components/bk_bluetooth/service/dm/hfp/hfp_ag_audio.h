#pragma once

#include <components/system.h>
#include <stdint.h>
#include "components/bluetooth/bk_dm_hfp_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Start the HFP AG SCO audio engine (speaker/mic tasks).
 * codec:      CODEC_VOICE_CVSD (8k) / CODEC_VOICE_MSBC (16k).
 * peer_addr:  6-byte SCO peer address used for uplink voice output.
 * air_tx_len: negotiated air tx packet length (bytes); reserved. */
void hfp_ag_audio_service_start(bk_hf_codec_type_t codec, const uint8_t *peer_addr, uint16_t air_tx_len);

/* Stop the HFP AG SCO audio engine and tear down the speaker/mic tasks. */
void hfp_ag_audio_service_stop(void);

/* Feed one downlink SCO voice packet (HF mic -> AG speaker) to the player.
 * The caller still owns and frees the data buffer. */
void hfp_ag_audio_service_feed_rx(const uint8_t *buf, uint32_t len);

/* Map an HFP speaker/mic volume step (0..15) to a codec gain and apply it. */
void hfp_ag_audio_service_set_spk_gain(uint8_t hfp_spk_vol);
void hfp_ag_audio_service_set_mic_gain(uint8_t hfp_mic_vol);

/* Non-zero while the audio engine is running. */
uint8_t hfp_ag_audio_service_is_running(void);

#ifdef __cplusplus
}
#endif
