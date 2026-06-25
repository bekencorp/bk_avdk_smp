#pragma once

#include <components/system.h>
#include <stdint.h>

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

/* Block until the speaker/mic tasks have fully exited. */
int32_t hfp_hf_audio_wait_player_end(void);

#ifdef __cplusplus
}
#endif
