/*
 * A2DP Sink Offload - Layer 4: CP dm/a2dp component (synchronous pass-through).
 *
 * At init, registers with Layer 3 (controller parse module) and forwards its
 * events (frame / start / stop / volume) synchronously to the registered
 * consumer (a2dp_sink_offload_player). It owns no task/queue.
 *
 * NOTE: because forwarding is synchronous, all consumer callbacks run in
 * controller context and MUST be light (copy + enqueue only, never block/decode).
 */
#ifndef _CP_A2DP_SINK_OFFLOAD_H_
#define _CP_A2DP_SINK_OFFLOAD_H_

#include <stdint.h>
#include <common/bk_err.h>
#include "components/bluetooth/bk_dm_a2dp_types.h"   /* bk_a2dp_mcc_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Max A2DP codec-specific config IE length (matches Layer 3). */
#define A2DP_SINK_OFFLOAD_CODEC_CFG_MAX  (16u)

/*
 * Raw codec frame report callback. Runs in controller context (synchronous
 * forward), so it MUST be light: copy the frame and enqueue, never block/decode.
 * frame is valid only during the callback. Dropped by default when unregistered.
 */
typedef void (*a2dp_sink_offload_report_cb_t)(uint16_t handle, uint8_t codec,
                                              const uint8_t *frame, uint16_t len, uint32_t ts);

/* Register the frame report callback; NULL/unregistered means drop. */
void a2dp_sink_offload_register_report_cb(a2dp_sink_offload_report_cb_t cb);

/* Session start/stop/volume events (mirror AP offload Start/Stop/SetVolume). */
typedef enum {
    A2DP_SINK_OFFLOAD_EVT_START  = 0,   /* session up: decode params in info valid */
    A2DP_SINK_OFFLOAD_EVT_STOP   = 1,   /* session down: stop audio */
    A2DP_SINK_OFFLOAD_EVT_VOLUME = 2,   /* AVRCP absolute volume in info->volume */
} a2dp_sink_offload_evt_t;

/* Session decode params (carried by START). Only what playback needs: codec
 * selects the decoder; cfg (codec IE) yields sample rate / channels / bitpool.
 * handle matches the frame callback for identification.
 * volume: valid on VOLUME events (0~127); the app caches it and applies it at
 * START / while playing. */
typedef struct {
    uint16_t handle;                                    /* ACL handle, matches frame cb */
    uint8_t  codec;                                     /* 0=SBC / 2=AAC */
    uint8_t  cfg_len;
    uint8_t  cfg[A2DP_SINK_OFFLOAD_CODEC_CFG_MAX];       /* A2DP codec-specific IE */
    uint8_t  volume;                                    /* VOLUME event: AVRCP volume 0~127 */
} a2dp_sink_offload_session_t;

/*
 * Session control callback. Runs in controller context (synchronous forward),
 * so it MUST be light: copy/enqueue only, never block/decode. START -> info
 * carries codec/cfg; STOP -> tear down; VOLUME -> read info->volume. The heavy
 * work (create decoder, apply gain) belongs in the consumer's own task.
 * info is valid only during the callback.
 */
typedef void (*a2dp_sink_offload_ctrl_cb_t)(a2dp_sink_offload_evt_t evt,
                                            const a2dp_sink_offload_session_t *info);

/* Register the session start/stop callback; NULL/unregistered means no notify. */
void a2dp_sink_offload_register_ctrl_cb(a2dp_sink_offload_ctrl_cb_t cb);

/*
 * Parse the raw codec IE (cfg) from a START event into bk_a2dp_mcc_t, ready for
 * a2dp_sink_audio_start(const bk_a2dp_mcc_t *, ...). Bit mapping matches the host
 * appl_a2dp_codec_ie_decode (channel_mode keeps raw bits, bit_pool takes max).
 *   codec      : codec from the session callback (0=SBC / 2=AAC)
 *   cfg/cfg_len: raw IE from the session callback (SBC>=4B / AAC>=6B)
 *   out_mcc    : parsed output
 * Returns BK_OK; BK_FAIL on invalid params or unsupported codec.
 */
bk_err_t a2dp_sink_offload_parse_cfg(uint8_t codec, const uint8_t *cfg, uint8_t cfg_len,
                                     bk_a2dp_mcc_t *out_mcc);

/* Call during CP bluetooth init: register callbacks with Layer 3, start the
 * dispatch task and frame queue. */
bk_err_t a2dp_sink_offload_init(void);

/* Deinit: stop the task, destroy the queue, unregister Layer 3 callback (optional). */
void a2dp_sink_offload_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _CP_A2DP_SINK_OFFLOAD_H_ */
