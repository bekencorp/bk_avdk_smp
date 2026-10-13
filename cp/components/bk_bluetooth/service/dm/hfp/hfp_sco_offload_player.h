/*
 * HFP SCO Offload Player - CP full-duplex voice driver.
 *
 * Owns the playback worker (queue + task) and the uplink mic task. Offload
 * (Layer 4) callbacks run in controller context and only copy+enqueue here; the
 * worker task dequeues and drives audio_play (downlink speaker). A separate mic
 * task captures + encodes and injects uplink frames via hfp_sco_offload_tx().
 * Volume (+VGS) is cached locally and applied at START / live while playing.
 *
 * The queue/task/event flow always builds. Real audio_play/audio_record calls are
 * gated by CONFIG_AUDIO_PLAY / CONFIG_AUDIO_RECORD; with them off, the full
 * offload->player pipeline still runs (start/stop/volume/frame count, log) so the
 * data path can be validated without the audio subsystem.
 */
#ifndef _CP_HFP_SCO_OFFLOAD_PLAYER_H_
#define _CP_HFP_SCO_OFFLOAD_PLAYER_H_

#include <common/bk_err.h>
#if CONFIG_AUDIO_PLAY
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>   /* pulls onboard_speaker; only when available */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Call during CP bluetooth init (after hfp_sco_offload_init): start the worker
 * queue+task and register consumer callbacks with Layer 4. */
bk_err_t hfp_sco_offload_player_init(void);

#if CONFIG_AUDIO_PLAY
/* Provide external-amp PA control from the product/board layer. The GPIO stays
 * owned by the caller; this module just forwards it into audio_play_cfg at open.
 * pa==NULL or pa_ctrl_en=false leaves PA control off. Call once before a call. */
void hfp_sco_offload_player_set_pa_ctrl(const onboard_speaker_pa_ctrl_t *pa);
#endif

/* Deinit: unregister from Layer 4, stop the worker + mic task, tear down audio. */
void hfp_sco_offload_player_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _CP_HFP_SCO_OFFLOAD_PLAYER_H_ */
