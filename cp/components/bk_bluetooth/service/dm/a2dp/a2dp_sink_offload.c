/*
 * A2DP Sink Offload - Layer 4: CP dm/a2dp component (synchronous pass-through).
 *
 * Thin shim between the controller parse module (Layer 3) and the consumer
 * (a2dp_sink_offload_player). It owns no task/queue: Layer 3 events are mapped
 * to the app callback types and forwarded synchronously in controller context.
 * The registered consumer callbacks therefore run in controller context and
 * MUST be light (copy + enqueue only, never block or decode).
 *
 * Responsibilities kept here: controller-header decoupling, event typing
 * (START/STOP/DATA/VOLUME -> app enum + session_t), and codec IE parsing.
 */
#include "a2dp_sink_offload.h"

#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK

#include <string.h>
#include <stdbool.h>
#include <components/system.h>
#include "components/log.h"

#define TAG "a2dp_ofl"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/*
 * Register API and callback signature exported by Layer 3 (controller parse
 * module); must match a2dp_offload.h. extern'd here to avoid a cross-component
 * dependency on the controller header path.
 */
typedef void (*a2dp_offload_event_cb_t)(uint8_t evt, uint16_t handle, uint8_t codec,
                                        const uint8_t *data, uint16_t len, uint32_t ts);
extern void a2dp_offload_register_cb(a2dp_offload_event_cb_t cb);
#define L3_EVT_START  (0u)   /* matches Layer 3 A2DP_OFFLOAD_EVT_START */
#define L3_EVT_STOP   (1u)   /* matches Layer 3 A2DP_OFFLOAD_EVT_STOP */
#define L3_EVT_DATA   (2u)   /* matches Layer 3 A2DP_OFFLOAD_EVT_DATA */
#define L3_EVT_VOLUME (3u)   /* matches Layer 3 A2DP_OFFLOAD_EVT_VOLUME */

static bool s_inited;

/* ---- app callbacks (default: drop / no notify when unregistered) ---- */
static a2dp_sink_offload_report_cb_t s_report_cb;   /* frame report */
static a2dp_sink_offload_ctrl_cb_t   s_app_ctrl_cb; /* session start/stop/volume */

void a2dp_sink_offload_register_report_cb(a2dp_sink_offload_report_cb_t cb)
{
    s_report_cb = cb;
}

void a2dp_sink_offload_register_ctrl_cb(a2dp_sink_offload_ctrl_cb_t cb)
{
    s_app_ctrl_cb = cb;
}

/* ---- raw codec IE -> bk_a2dp_mcc_t (app can feed a2dp_sink_audio_start directly) ---- */
#define A2DP_OFFLOAD_CT_SBC   (0x00u)   /* = BK_A2DP_CODEC_TYPE_SBC / CODEC_AUDIO_SBC */
#define A2DP_OFFLOAD_CT_AAC   (0x02u)   /* = BK_A2DP_CODEC_TYPE_AAC / CODEC_AUDIO_AAC */

bk_err_t a2dp_sink_offload_parse_cfg(uint8_t codec, const uint8_t *cfg, uint8_t cfg_len,
                                     bk_a2dp_mcc_t *out_mcc)
{
    if (!cfg || !out_mcc) {
        return BK_FAIL;
    }
    memset(out_mcc, 0, sizeof(*out_mcc));
    out_mcc->type = codec;

    if (codec == A2DP_OFFLOAD_CT_SBC) {
        uint8_t b0, b1;
        if (cfg_len < 4) {
            return BK_FAIL;
        }
        b0 = cfg[0];
        b1 = cfg[1];
        /* sample rate: b0 high nibble */
        if      (b0 & 0x80u) out_mcc->cie.sbc_codec.sample_rate = 16000;
        else if (b0 & 0x40u) out_mcc->cie.sbc_codec.sample_rate = 32000;
        else if (b0 & 0x20u) out_mcc->cie.sbc_codec.sample_rate = 44100;
        else if (b0 & 0x10u) out_mcc->cie.sbc_codec.sample_rate = 48000;
        /* channel mode: b0 low nibble (keep raw bit value) */
        if      (b0 & 0x08u) { out_mcc->cie.sbc_codec.channel_mode = 0x08u; out_mcc->cie.sbc_codec.channels = 1; } /* mono */
        else if (b0 & 0x04u) { out_mcc->cie.sbc_codec.channel_mode = 0x04u; out_mcc->cie.sbc_codec.channels = 2; } /* dual */
        else if (b0 & 0x02u) { out_mcc->cie.sbc_codec.channel_mode = 0x02u; out_mcc->cie.sbc_codec.channels = 2; } /* stereo */
        else if (b0 & 0x01u) { out_mcc->cie.sbc_codec.channel_mode = 0x01u; out_mcc->cie.sbc_codec.channels = 2; } /* joint stereo */
        /* block length: b1 high nibble */
        if      (b1 & 0x80u) out_mcc->cie.sbc_codec.block_len = 4;
        else if (b1 & 0x40u) out_mcc->cie.sbc_codec.block_len = 8;
        else if (b1 & 0x20u) out_mcc->cie.sbc_codec.block_len = 12;
        else if (b1 & 0x10u) out_mcc->cie.sbc_codec.block_len = 16;
        /* subbands: b1 bit3-2 */
        if      (b1 & 0x08u) out_mcc->cie.sbc_codec.subbands = 4;
        else if (b1 & 0x04u) out_mcc->cie.sbc_codec.subbands = 8;
        /* allocation method: b1 bit1-0 (keep raw bit value) */
        if      (b1 & 0x02u) out_mcc->cie.sbc_codec.alloc_mode = 0x02u;
        else if (b1 & 0x01u) out_mcc->cie.sbc_codec.alloc_mode = 0x01u;
        /* bitpool: use max (matches host: codec_ie[3]) */
        out_mcc->cie.sbc_codec.bit_pool = cfg[3];
        return BK_OK;
    }

    if (codec == A2DP_OFFLOAD_CT_AAC) {
        uint16_t sf;
        if (cfg_len < 6) {
            return BK_FAIL;
        }
        /* sample rate: 16-bit field = cfg[1](hi8) | cfg[2](lo8; hi4 freq, lo2 chan) */
        sf = (uint16_t)(((uint16_t)cfg[1] << 8) | cfg[2]);
        if      (sf & 0x8000u) out_mcc->cie.aac_codec.sample_rate = 8000;
        else if (sf & 0x4000u) out_mcc->cie.aac_codec.sample_rate = 11025;
        else if (sf & 0x2000u) out_mcc->cie.aac_codec.sample_rate = 12000;
        else if (sf & 0x1000u) out_mcc->cie.aac_codec.sample_rate = 16000;
        else if (sf & 0x0800u) out_mcc->cie.aac_codec.sample_rate = 22050;
        else if (sf & 0x0400u) out_mcc->cie.aac_codec.sample_rate = 24000;
        else if (sf & 0x0200u) out_mcc->cie.aac_codec.sample_rate = 32000;
        else if (sf & 0x0100u) out_mcc->cie.aac_codec.sample_rate = 44100;
        else if (sf & 0x0080u) out_mcc->cie.aac_codec.sample_rate = 48000;
        else if (sf & 0x0040u) out_mcc->cie.aac_codec.sample_rate = 64000;
        else if (sf & 0x0020u) out_mcc->cie.aac_codec.sample_rate = 88200;
        else if (sf & 0x0010u) out_mcc->cie.aac_codec.sample_rate = 96000;
        /* channels: cfg[2] bit3-2 */
        if      (cfg[2] & 0x08u) out_mcc->cie.aac_codec.channels = 1;
        else if (cfg[2] & 0x04u) out_mcc->cie.aac_codec.channels = 2;
        return BK_OK;
    }

    return BK_FAIL;   /* unsupported codec */
}

/*
 * Layer 3 unified event callback (controller context). Synchronous pass-through:
 * map to the app callback types and forward. The consumer callbacks must be
 * light (copy + enqueue only); do not block or decode here.
 */
static void a2dp_sink_offload_on_event(uint8_t evt, uint16_t handle, uint8_t codec,
                                       const uint8_t *data, uint16_t len, uint32_t ts)
{
    a2dp_sink_offload_session_t info;

    if (!s_inited) {
        return;
    }

    if (evt == L3_EVT_DATA) {
        /* raw frame: forward pointer directly (valid only during this call) */
        if (s_report_cb && data && len) {
            s_report_cb(handle, codec, data, len, ts);
        }
        return;
    }

    if (!s_app_ctrl_cb) {
        return;
    }

    memset(&info, 0, sizeof(info));

    if (evt == L3_EVT_VOLUME) {
        /* AVRCP volume carried in len (0~127), no session context */
        info.volume = (uint8_t)len;
        s_app_ctrl_cb(A2DP_SINK_OFFLOAD_EVT_VOLUME, &info);
        return;
    }

    /* session start/stop: data=cfg (START only) / len=cfg_len */
    info.handle = handle;
    info.codec  = codec;
    if (evt == L3_EVT_START && data && len) {
        uint16_t n = len;
        if (n > A2DP_SINK_OFFLOAD_CODEC_CFG_MAX) {
            n = A2DP_SINK_OFFLOAD_CODEC_CFG_MAX;
        }
        memcpy(info.cfg, data, n);
        info.cfg_len = (uint8_t)n;
    }
    s_app_ctrl_cb((evt == L3_EVT_START) ? A2DP_SINK_OFFLOAD_EVT_START
                                        : A2DP_SINK_OFFLOAD_EVT_STOP, &info);
}

bk_err_t a2dp_sink_offload_init(void)
{
    if (s_inited) {
        return BK_OK;
    }
    s_report_cb   = NULL;
    s_app_ctrl_cb = NULL;
    s_inited      = true;

    /* register Layer 3 callback (synchronous forwarding) */
    a2dp_offload_register_cb(a2dp_sink_offload_on_event);
    LOGI("cp a2dp sink offload inited (synchronous pass-through)\n");
    return BK_OK;
}

void a2dp_sink_offload_deinit(void)
{
    if (!s_inited) {
        return;
    }
    /* detach Layer 3 callback first to stop new events/frames */
    a2dp_offload_register_cb(NULL);
    s_inited      = false;
    s_report_cb   = NULL;
    s_app_ctrl_cb = NULL;
    LOGI("cp a2dp sink offload deinited\n");
}

#endif /* CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK */
