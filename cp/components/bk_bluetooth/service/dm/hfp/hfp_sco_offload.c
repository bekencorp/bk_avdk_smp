/*
 * HFP SCO Offload - Layer 4: CP dm/hfp component (synchronous pass-through).
 *
 * Thin shim between the controller parse module (Layer 3 sco_offload) and the
 * consumer (hfp_sco_offload_player). It owns no task/queue: Layer 3 events are
 * mapped to the app callback types and forwarded synchronously in controller
 * context. The registered consumer callbacks therefore run in controller context
 * and MUST be light (copy + enqueue only, never block or decode).
 *
 * It also owns the uplink helper hfp_sco_offload_tx(): pack an encoded frame into
 * an HCI-SCO packet and inject it into this core's controller.
 */
#include "hfp_sco_offload.h"

#if CONFIG_BLUETOOTH_HFP_SCO_OFFLOAD

#include <string.h>
#include <stdbool.h>
#include <components/system.h>
#include "components/log.h"
#include "components/bluetooth/bk_ble.h"   /* bk_ble_hci_to_controller */

#define TAG "sco_ofl"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/*
 * Register API / callback signature / handle accessor exported by Layer 3
 * (controller parse module); must match sco_offload.h. extern'd here to avoid a
 * cross-component dependency on the controller header path.
 */
typedef void (*sco_offload_event_cb_t)(uint8_t evt, uint16_t handle, uint8_t codec,
                                       const uint8_t *data, uint16_t len);
extern void     sco_offload_register_cb(sco_offload_event_cb_t cb);
extern uint16_t sco_offload_get_handle(void);
#define L3_EVT_START  (0u)   /* matches Layer 3 SCO_OFFLOAD_EVT_START */
#define L3_EVT_STOP   (1u)   /* matches Layer 3 SCO_OFFLOAD_EVT_STOP */
#define L3_EVT_DATA   (2u)   /* matches Layer 3 SCO_OFFLOAD_EVT_DATA */
#define L3_EVT_VOLUME (3u)   /* matches Layer 3 SCO_OFFLOAD_EVT_VOLUME */

/* HCI packet indicator for SCO data (== hci_parse.h DATA_TYPE_SCO). */
#define HCI_PKT_TYPE_SCO   (3u)
/* Max encoded uplink frame (CVSD send unit *2 / mSBC 60): keep some headroom. */
#define SCO_TX_FRAME_MAX   (256u)

static bool s_inited;

/* ---- app callbacks (default: drop / no notify when unregistered) ---- */
static hfp_sco_offload_report_cb_t s_report_cb;   /* downlink frame report */
static hfp_sco_offload_ctrl_cb_t   s_app_ctrl_cb; /* session start/stop/volume */

void hfp_sco_offload_register_report_cb(hfp_sco_offload_report_cb_t cb)
{
    s_report_cb = cb;
}

void hfp_sco_offload_register_ctrl_cb(hfp_sco_offload_ctrl_cb_t cb)
{
    s_app_ctrl_cb = cb;
}

/*
 * Layer 3 unified event callback (controller context). Synchronous pass-through:
 * map to the app callback types and forward. The consumer callbacks must be
 * light (copy + enqueue only); do not block or decode here.
 */
static void hfp_sco_offload_on_event(uint8_t evt, uint16_t handle, uint8_t codec,
                                     const uint8_t *data, uint16_t len)
{
    hfp_sco_offload_session_t info;

    if (!s_inited) {
        return;
    }

    if (evt == L3_EVT_DATA) {
        /* raw frame: forward pointer directly (valid only during this call) */
        if (s_report_cb && data && len) {
            s_report_cb(handle, codec, data, len);
        }
        return;
    }

    if (!s_app_ctrl_cb) {
        return;
    }

    memset(&info, 0, sizeof(info));

    if (evt == L3_EVT_VOLUME) {
        /* HFP +VGS carried in len (0~15), no session context */
        info.volume = (uint8_t)len;
        s_app_ctrl_cb(HFP_SCO_OFFLOAD_EVT_VOLUME, &info);
        return;
    }

    /* session start/stop: codec valid on START */
    info.handle = handle;
    info.codec  = codec;
    s_app_ctrl_cb((evt == L3_EVT_START) ? HFP_SCO_OFFLOAD_EVT_START
                                        : HFP_SCO_OFFLOAD_EVT_STOP, &info);
}

/* ---- uplink: pack HCI-SCO packet + inject into controller ---- */
bk_err_t hfp_sco_offload_tx(const uint8_t *frame, uint16_t len)
{
    uint8_t  pkt[3 + SCO_TX_FRAME_MAX];
    uint16_t handle;

    if (!s_inited || !frame || len == 0 || len > SCO_TX_FRAME_MAX) {
        return BK_FAIL;
    }
    handle = sco_offload_get_handle();
    if (handle == 0) {
        return BK_FAIL;   /* no active SCO offload session */
    }

    /* HCI SCO data packet body: conn_handle(2, LE) | data_total_length(1) | frame[].
     * packet-status-flag (bits 12-13) = 0 on TX. */
    pkt[0] = (uint8_t)(handle & 0xFFu);
    pkt[1] = (uint8_t)((handle >> 8) & 0x0Fu);
    pkt[2] = (uint8_t)len;
    memcpy(&pkt[3], frame, len);

    return (bk_ble_hci_to_controller(HCI_PKT_TYPE_SCO, pkt, (uint16_t)(3 + len)) == BK_ERR_BLE_SUCCESS)
           ? BK_OK : BK_FAIL;
}

bk_err_t hfp_sco_offload_init(void)
{
    if (s_inited) {
        return BK_OK;
    }
    s_report_cb   = NULL;
    s_app_ctrl_cb = NULL;
    s_inited      = true;

    /* register Layer 3 callback (synchronous forwarding) */
    sco_offload_register_cb(hfp_sco_offload_on_event);
    LOGI("cp hfp sco offload inited (synchronous pass-through)\n");
    return BK_OK;
}

void hfp_sco_offload_deinit(void)
{
    if (!s_inited) {
        return;
    }
    /* detach Layer 3 callback first to stop new events/frames */
    sco_offload_register_cb(NULL);
    s_inited      = false;
    s_report_cb   = NULL;
    s_app_ctrl_cb = NULL;
    LOGI("cp hfp sco offload deinited\n");
}

#endif /* CONFIG_BLUETOOTH_HFP_SCO_OFFLOAD */
