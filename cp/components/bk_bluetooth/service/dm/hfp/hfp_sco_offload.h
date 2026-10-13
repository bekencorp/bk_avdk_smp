/*
 * HFP SCO Offload - Layer 4: CP dm/hfp component (synchronous pass-through).
 *
 * At init, registers with Layer 3 (controller parse module sco_offload) and
 * forwards its events (frame / start / stop / volume) synchronously to the
 * registered consumer (hfp_sco_offload_player). It owns no task/queue.
 *
 * Full-duplex: besides the downlink (frame report), it exposes the uplink helper
 * hfp_sco_offload_tx(), which packs an encoded frame into an HCI-SCO packet and
 * injects it straight into this core's controller (bk_ble_hci_to_controller),
 * reusing the SCO handle cached by Layer 3.
 *
 * NOTE: because forwarding is synchronous, all consumer callbacks run in
 * controller context and MUST be light (copy + enqueue only, never block/decode).
 */
#ifndef _CP_HFP_SCO_OFFLOAD_H_
#define _CP_HFP_SCO_OFFLOAD_H_

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Raw codec frame report callback (downlink). Runs in controller context
 * (synchronous forward), so it MUST be light: copy the frame and enqueue, never
 * block/decode. frame is valid only during the callback. Dropped when unregistered.
 *   codec: CODEC_VOICE_CVSD(0) / CODEC_VOICE_MSBC(1)
 */
typedef void (*hfp_sco_offload_report_cb_t)(uint16_t handle, uint8_t codec,
                                            const uint8_t *frame, uint16_t len);

/* Register the downlink frame report callback; NULL/unregistered means drop. */
void hfp_sco_offload_register_report_cb(hfp_sco_offload_report_cb_t cb);

/* Session start/stop/volume events (mirror AP SCO offload Start/Stop/SetVolume). */
typedef enum {
    HFP_SCO_OFFLOAD_EVT_START  = 0,   /* session up: codec valid, start down+up */
    HFP_SCO_OFFLOAD_EVT_STOP   = 1,   /* session down: stop audio */
    HFP_SCO_OFFLOAD_EVT_VOLUME = 2,   /* HFP +VGS in info->volume (0~15) */
} hfp_sco_offload_evt_t;

/* Session params (carried by START); VOLUME carries info->volume. */
typedef struct {
    uint16_t handle;   /* SCO connection handle */
    uint8_t  codec;    /* CODEC_VOICE_CVSD(0) / CODEC_VOICE_MSBC(1) */
    uint8_t  volume;   /* VOLUME event: HFP +VGS 0~15 */
} hfp_sco_offload_session_t;

/*
 * Session control callback. Runs in controller context (synchronous forward),
 * so it MUST be light: copy/enqueue only, never block/decode. START -> info
 * carries codec; STOP -> tear down; VOLUME -> read info->volume. The heavy work
 * (open player/recorder, apply gain) belongs in the consumer's own task.
 * info is valid only during the callback.
 */
typedef void (*hfp_sco_offload_ctrl_cb_t)(hfp_sco_offload_evt_t evt,
                                          const hfp_sco_offload_session_t *info);

/* Register the session start/stop/volume callback; NULL/unregistered means no notify. */
void hfp_sco_offload_register_ctrl_cb(hfp_sco_offload_ctrl_cb_t cb);

/*
 * Uplink: pack an encoded SCO frame into an HCI-SCO data packet
 *   [ conn_handle(2, LE, packet-status-flag=0) | data_total_length(1) | frame[] ]
 * and inject it into this core's controller via bk_ble_hci_to_controller().
 * The SCO handle is taken from Layer 3's cached session (no handle -> BK_FAIL).
 * Pacing (per-frame sizing) is the caller's responsibility.
 */
bk_err_t hfp_sco_offload_tx(const uint8_t *frame, uint16_t len);

/* Call during CP bluetooth init: register callbacks with Layer 3 (sync forward). */
bk_err_t hfp_sco_offload_init(void);

/* Deinit: unregister Layer 3 callback, detach consumers. */
void hfp_sco_offload_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _CP_HFP_SCO_OFFLOAD_H_ */
