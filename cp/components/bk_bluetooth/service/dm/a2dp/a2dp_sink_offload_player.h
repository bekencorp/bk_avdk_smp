/*
 * A2DP Sink Offload Player - CP playback driver.
 *
 * Consumes a2dp_sink_offload (Layer 4) events and drives the audio assembler
 * (a2dp_sink_audio):
 *   START  -> parse cfg -> a2dp_sink_audio_start(mcc, ..., cached volume)
 *   DATA   -> a2dp_sink_audio_handle_data(frame)
 *   STOP   -> a2dp_sink_audio_stop()
 *   VOLUME -> cache volume; apply a2dp_sink_audio_set_gain() while playing
 *
 * Owns the playback worker (queue + task). The offload callbacks fire in
 * controller context and only copy+enqueue here; decode/playback runs in the
 * player task. It also caches the last AVRCP volume (default until the first
 * VOLUME arrives) and applies it at START.
 */
#ifndef _CP_A2DP_SINK_OFFLOAD_PLAYER_H_
#define _CP_A2DP_SINK_OFFLOAD_PLAYER_H_

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Register offload callbacks, create the playback worker (queue + task).
 * Call after a2dp_sink_offload_init(). */
bk_err_t a2dp_sink_offload_player_init(void);

/* Unregister offload callbacks, stop the worker, destroy the queue. */
void a2dp_sink_offload_player_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _CP_A2DP_SINK_OFFLOAD_PLAYER_H_ */
