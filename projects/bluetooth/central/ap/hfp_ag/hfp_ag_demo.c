/**
 * @file hfp_ag_demo.c
 *
 * HFP AG (Audio Gateway) demo - control plane (application policy).
 *
 * The lifecycle (init/features/bt_manager) and the SCO audio engine are owned
 * by the componentized bk_hfp_ag_service; this demo only supplies product
 * policy: it answers the HF's AT queries (+CIND/+COPS/+CNUM/+CLCC), drives the
 * simulated call state machine, and exposes CLI helpers. It is a pure consumer
 * of the public components/bluetooth/bk_dm_hfp_ag.h API + bk_hfp_ag_service.h.
 */

#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define LOG_TAG "hfpag_app"

#include "hfp_ag_demo.h"
#include "components/log.h"
#include "components/bluetooth/bk_dm_hfp_ag.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "bk_hfp_ag_service.h"
#include "bt_manager.h"

#define HFP_GAIN_MAX 15

typedef struct
{
    int      num_active;
    int      num_held;
    char     call_number[24];

    uint8_t  signal;    /* +CIND signal strength, 0-5 */
    uint8_t  batt_lev;  /* +CIND battery charge, 0-5 */
} hfp_ag_demo_ctx_t;

static hfp_ag_demo_ctx_t s_demo =
{
    .signal   = 5,
    .batt_lev = 5,
};

/* Peer address tracked by the service (SLC peer), NULL when not connected. */
static const uint8_t *ag_peer(void)
{
    return bk_hfp_ag_service_get_peer();
}

/* -------------------------------------------------------------------------- */
/*  Apple HFP extensions (AT+XAPL / AT+IPHONEACCEV) sent by an Apple accessory  */
/* -------------------------------------------------------------------------- */

static void ag_handle_xapl(const char *s)
{
    const char *p = os_strchr((char *)s, '=');
    const char *comma;
    int feat = 0;
    int idlen;

    if (!p)
    {
        return;
    }

    p++;
    comma = os_strchr((char *)p, ',');

    if (comma)
    {
        feat  = (int)os_strtoul(comma + 1, NULL, 10);
        idlen = (int)(comma - p);
    }
    else
    {
        idlen = (int)os_strlen(p);
    }

    LOGI("HF Apple XAPL id=%.*s features=0x%x", idlen, p, feat);
}

static void ag_handle_iphoneaccev(const char *s)
{
    const char *p = os_strchr(s, '=');
    int n, i;

    if (!p)
    {
        return;
    }

    p++;
    n = (int)os_strtoul(p, NULL, 10);

    for (i = 0; i < n; i++)
    {
        int key, val;

        p = os_strchr(p, ',');

        if (!p)
        {
            break;
        }

        key = (int)os_strtoul(++p, NULL, 10);

        p = os_strchr(p, ',');

        if (!p)
        {
            break;
        }

        val = (int)os_strtoul(++p, NULL, 10);

        if (key == 1)
        {
            if (val > 9) { val = 9; }
            LOGI("HF Apple battery level %d (%u%%)", val, (unsigned)((val + 1) * 10));
        }
        else if (key == 2)
        {
            LOGI("HF Apple dock/charge %d", val);
        }
        else
        {
            LOGI("HF Apple accev key %d val %d", key, val);
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  AG policy callback (invoked by bk_hfp_ag_service after its own handling)    */
/* -------------------------------------------------------------------------- */

static void hfp_ag_demo_cb(bk_hf_ag_cb_event_t event, bk_hf_ag_cb_param_t *param)
{
    switch (event)
    {
    case BK_HF_AG_CONNECTION_STATE_EVT:
    case BK_HF_AG_AUDIO_STATE_EVT:
    case BK_HF_AG_VOLUME_CONTROL_EVT:
    case BK_HF_AG_BCS_RESPONSE_EVT:
        /* lifecycle / audio / codec handled by bk_hfp_ag_service */
        break;

    case BK_HF_AG_BVRA_REQ_EVT:
        LOGI("HF requests voice recognition = %d", param->vra_req.value);
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        bk_bt_hf_ag_vra_control(param->remote_addr, param->vra_req.value);
        break;

    case BK_HF_AG_NREC_REQ_EVT:
        LOGI("HF NREC = %d", param->nrec.state);
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_VTS_REQ_EVT:
        LOGI("HF DTMF code = %s", param->vts_req.code ? param->vts_req.code : "");
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_BRSF_EVT:
        LOGI("HF supported features (BRSF) = 0x%x", (unsigned)param->brsf.peer_feat);
        break;

    case BK_HF_AG_BIEV_UPDATE_EVT:
        LOGI("HF indicator id=%u value=%u", (unsigned)param->biev.ind_id, (unsigned)param->biev.value);
        break;

    case BK_HF_AG_CHLD_REQ_EVT:
        LOGI("HF CHLD request type=%d index=%d", param->chld.type, param->chld.index);
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_BTRH_REQ_EVT:
        LOGI("HF BTRH request action=%d", param->btrh.action);
        bk_bt_hf_ag_btrh_response(param->remote_addr, (bk_hf_btrh_status_t)param->btrh.action);
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_CODEC_EVT:
        LOGI("HF codec list num=%d (0=CVSD 1=mSBC)", param->codec_info.num);
        break;

    case BK_HF_AG_UNAT_REQ_EVT:
    {
        const char *at = param->unat_req.unat ? param->unat_req.unat : "";
        param->unat_req.app_response = 1;

        if (os_strstr((char *)at, "XAPL"))
        {
            ag_handle_xapl(at);
            bk_bt_hf_ag_unknown_at_send(param->remote_addr, "+XAPL=iPhone,2");
            bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        }
        else if (os_strstr((char *)at, "IPHONEACCEV"))
        {
            ag_handle_iphoneaccev(at);
            bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        }
        else
        {
            LOGE("HF unknown AT: %s", at);
            bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_ERR, 0);
        }

        break;
    }

    /* ---- queries the AG must answer ---- */
    case BK_HF_AG_CIND_REQ_EVT:
        bk_bt_hf_ag_cind_response(param->remote_addr,
                                  s_demo.num_active ? BK_HF_CALL_STATUS_CALL_IN_PROGRESS : BK_HF_CALL_STATUS_NO_CALLS,
                                  BK_HF_CALL_SETUP_STATUS_IDLE,
                                  BK_HF_NETWORK_STATE_AVAILABLE,
                                  s_demo.signal,
                                  BK_HF_ROAMING_STATUS_INACTIVE,
                                  s_demo.batt_lev,
                                  BK_HF_CALL_HELD_STATUS_NONE);
        break;

    case BK_HF_AG_COPS_REQ_EVT:
        bk_bt_hf_ag_cops_response(param->remote_addr, "CMCC");
        break;

    case BK_HF_AG_CNUM_REQ_EVT:
        bk_bt_hf_ag_cnum_response(param->remote_addr, "10086", BK_HF_SUBSCRIBER_SERVICE_TYPE_VOICE);
        break;

    case BK_HF_AG_CLCC_REQ_EVT:
        if (s_demo.num_active > 0)
        {
            bk_bt_hf_ag_clcc_response(param->remote_addr, 1,
                                      BK_HF_CURRENT_CALL_DIRECTION_INCOMING,
                                      BK_HF_CURRENT_CALL_STATUS_ACTIVE,
                                      BK_HF_CURRENT_CALL_MODE_VOICE,
                                      BK_HF_CURRENT_CALL_MPTY_TYPE_SINGLE,
                                      s_demo.call_number[0] ? s_demo.call_number : NULL,
                                      BK_HF_CALL_ADDR_TYPE_UNKNOWN);
        }

        bk_bt_hf_ag_clcc_response(param->remote_addr, 0,
                                  BK_HF_CURRENT_CALL_DIRECTION_OUTGOING,
                                  BK_HF_CURRENT_CALL_STATUS_ACTIVE,
                                  BK_HF_CURRENT_CALL_MODE_VOICE,
                                  BK_HF_CURRENT_CALL_MPTY_TYPE_SINGLE,
                                  NULL, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
        break;

    /* ---- call control requests from HF ---- */
    case BK_HF_AG_ATA_REQ_EVT:
        LOGI("HF answered the call (ATA)");
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        hfp_ag_demo_answer();
        break;

    case BK_HF_AG_CHUP_REQ_EVT:
        LOGI("HF hung up the call (CHUP)");
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        hfp_ag_demo_hangup();
        break;

    case BK_HF_AG_DIAL_REQ_EVT:
        LOGI("HF dial request type=%d value=%s", param->out_call.type,
             param->out_call.num_or_loc ? param->out_call.num_or_loc : "(redial)");

        if (param->out_call.type == BK_HF_AG_DIAL_TYPE_MEMORY)
        {
            bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_CME,
                                  BK_HF_CME_OPERATION_NOT_SUPPORTED);
        }
        else
        {
            bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
            hfp_ag_demo_dial_out(param->out_call.num_or_loc);
        }
        break;

    default:
        LOGW("unhandled AG event %d", event);
        break;
    }
}

/* Auto-connect HFP AG once the peer is authenticated, so an HFP-capable
 * headset gets its SLC without a separate 'hfp_ag connect'. */
static void hfp_ag_demo_gap_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    if (event == BK_BT_GAP_AUTH_CMPL_EVT && param && param->auth_cmpl.stat == 0)
    {
        if (!bk_hfp_ag_service_is_connected())
        {
            LOGI("auth ok, auto-connect HFP AG");
            bk_hfp_ag_service_connect(param->auth_cmpl.bda);
        }
    }
}

int hfp_ag_demo_init(void)
{
    int ret;

    LOGI("%s", __func__);

    bk_hfp_ag_service_register_cb(hfp_ag_demo_cb);

    ret = bk_hfp_ag_service_init();
    if (ret)
    {
        LOGE("service init err %d", ret);
        return -1;
    }

    /* auto-connect the AG SLC after authentication completes */
    {
        btm_callback_s btm_cb = { .gap_cb = hfp_ag_demo_gap_cb };
        bt_manager_register_callback(&btm_cb);
    }

    return 0;
}

void hfp_ag_demo_connect(const uint8_t *addr)
{
    if (!addr)
    {
        return;
    }

    bk_hfp_ag_service_connect(addr);
}

void hfp_ag_demo_disconnect(void)
{
    bk_hfp_ag_service_disconnect();
}

void hfp_ag_demo_incoming_call(const char *number)
{
    const uint8_t *peer = ag_peer();

    if (!bk_hfp_ag_service_is_connected() || !peer)
    {
        LOGE("SLC not ready");
        return;
    }

    os_memset(s_demo.call_number, 0, sizeof(s_demo.call_number));

    if (number)
    {
        strncpy(s_demo.call_number, number, sizeof(s_demo.call_number) - 1);
    }

    bk_bt_hf_ag_devices_status_indchange((uint8_t *)peer,
                                         BK_HF_CALL_STATUS_NO_CALLS,
                                         BK_HF_CALL_SETUP_STATUS_INCOMING,
                                         BK_HF_NETWORK_STATE_AVAILABLE, 5);

    bk_bt_hf_ag_ring((uint8_t *)peer);

    if (number)
    {
        bk_bt_hf_ag_clip_report((uint8_t *)peer, number, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    }

    LOGI("incoming call from %s", number ? number : "?");
}

void hfp_ag_demo_dial_out(const char *number)
{
    const uint8_t *peer = ag_peer();

    if (!bk_hfp_ag_service_is_connected() || !peer)
    {
        return;
    }

    if (number)
    {
        os_memset(s_demo.call_number, 0, sizeof(s_demo.call_number));
        strncpy(s_demo.call_number, number, sizeof(s_demo.call_number) - 1);
    }

    char *cur = s_demo.call_number[0] ? s_demo.call_number : NULL;

    bk_bt_hf_ag_out_call((uint8_t *)peer, 0, 0,
                         BK_HF_CALL_STATUS_NO_CALLS,
                         BK_HF_CALL_SETUP_STATUS_OUTGOING_DIALING,
                         cur, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    bk_bt_hf_ag_out_call((uint8_t *)peer, 0, 0,
                         BK_HF_CALL_STATUS_NO_CALLS,
                         BK_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING,
                         cur, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    LOGI("dialing out %s", cur ? cur : "(redial)");

    /* auto-establish the SCO voice path so the intercom starts */
    bk_hfp_ag_service_audio_connect();
}

void hfp_ag_demo_answer(void)
{
    const uint8_t *peer = ag_peer();

    if (!peer)
    {
        return;
    }

    s_demo.num_active = 1;
    s_demo.num_held   = 0;
    bk_bt_hf_ag_answer_call((uint8_t *)peer, s_demo.num_active, s_demo.num_held,
                            BK_HF_CALL_STATUS_CALL_IN_PROGRESS,
                            BK_HF_CALL_SETUP_STATUS_IDLE,
                            s_demo.call_number[0] ? s_demo.call_number : NULL,
                            BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    LOGI("call active");
}

void hfp_ag_demo_hangup(void)
{
    const uint8_t *peer = ag_peer();

    if (!peer)
    {
        return;
    }

    s_demo.num_active = 0;
    s_demo.num_held   = 0;
    bk_bt_hf_ag_end_call((uint8_t *)peer, s_demo.num_active, s_demo.num_held,
                         BK_HF_CALL_STATUS_NO_CALLS,
                         BK_HF_CALL_SETUP_STATUS_IDLE,
                         NULL, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    os_memset(s_demo.call_number, 0, sizeof(s_demo.call_number));

    /* also tear down the SCO voice path (dial auto-connects audio) */
    bk_hfp_ag_service_audio_disconnect();

    LOGI("call ended");
}

void hfp_ag_demo_audio(uint8_t connect)
{
    if (connect)
    {
        if (!bk_hfp_ag_service_is_connected())
        {
            LOGE("SLC not ready, connect HF first");
            return;
        }

        bk_hfp_ag_service_audio_connect();
    }
    else
    {
        bk_hfp_ag_service_audio_disconnect();
    }
}

void hfp_ag_demo_set_codec(uint8_t msbc)
{
    bk_hfp_ag_service_set_codec(msbc);
    LOGI("codec preference set to %s", msbc ? "mSBC" : "CVSD");
}

void hfp_ag_demo_custom_cmd(const char *atcmd)
{
    const uint8_t *peer = ag_peer();

    if (peer)
    {
        bk_bt_hf_ag_unknown_at_send((uint8_t *)peer, (char *)atcmd);
    }
}

void hfp_ag_demo_send_vgs(uint8_t spk_vol)
{
    const uint8_t *peer = ag_peer();

    if (spk_vol > HFP_GAIN_MAX)
    {
        spk_vol = HFP_GAIN_MAX;
    }

    if (peer)
    {
        bk_bt_hf_ag_volume_control((uint8_t *)peer, BK_HF_VOLUME_CONTROL_TARGET_SPK, spk_vol);
    }
}

void hfp_ag_demo_send_vgm(uint8_t mic_vol)
{
    const uint8_t *peer = ag_peer();

    if (mic_vol > HFP_GAIN_MAX)
    {
        mic_vol = HFP_GAIN_MAX;
    }

    if (peer)
    {
        bk_bt_hf_ag_volume_control((uint8_t *)peer, BK_HF_VOLUME_CONTROL_TARGET_MIC, mic_vol);
    }
}

void hfp_ag_demo_set_battery(uint8_t level)
{
    const uint8_t *peer = ag_peer();

    if (level > 5)
    {
        level = 5;
    }

    s_demo.batt_lev = level;

    if (!bk_hfp_ag_service_is_connected() || !peer)
    {
        LOGW("battery=%d stored, report deferred (SLC not ready)", level);
        return;
    }

    bk_bt_hf_ag_ciev_report((uint8_t *)peer, BK_HF_AG_CIND_IDX_BATTCHG, level);
    LOGI("report battery level %d", level);
}

void hfp_ag_demo_switch_role(uint8_t master)
{
    const uint8_t *peer = ag_peer();

    if (!peer)
    {
        LOGE("no peer, connect first");
        return;
    }

    bk_bt_gap_switch_role((uint8_t *)peer, master ? BT_MASTER_ROLE : BT_SLAVE_ROLE);
    LOGI("switch role -> %s", master ? "master" : "slave");
}