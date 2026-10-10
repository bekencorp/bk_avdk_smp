#include <components/system.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>

#include "bk_avrcp_ct_service.h"
#include "bk_avrcp_tg_service.h"

#include "components/bluetooth/bk_dm_avrcp.h"
#include "bt_manager.h"

#define TAG "bk_avrcp_ct"

enum
{
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARNING,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_DEBUG,
    DEBUG_LEVEL_VERBOSE,
};

#define DEBUG_LEVEL DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGW(format, ...) do{if(DEBUG_LEVEL >= DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGI(format, ...) do{if(DEBUG_LEVEL >= DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGD(format, ...) do{if(DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)
#define LOGV(format, ...) do{if(DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);}while(0)

#define AVRCP_PASSTHROUGH_TIMEOUT_MS 2000

/* Playback position notification reporting interval, in seconds. */
#define AVRCP_PLAY_POS_REPORT_INTERVAL_S 1

typedef struct
{
    /* ---- common (both modes) ---- */
    uint8_t inited;
    uint8_t remote_volume_mode;       /* config: 0=media-control, 1=remote-volume */
    uint8_t connected;
    uint8_t remote_bda[6];
    uint16_t peer_cap;                /* peer registered-notification capabilities */
    bk_avrcp_ct_event_cb_t event_cb;
    void *event_user_data;

    /* ---- auto-connect after A2DP (both modes) ---- */
    uint8_t auto_connect_after_a2dp;  /* config */
    uint8_t pending_bda[6];
    beken2_timer_t auto_connect_timer;

    /* ---- media-control mode (remote_volume_mode == 0): passthrough out ---- */
    beken_semaphore_t passthrough_sema;

    /* ---- remote-volume mode (remote_volume_mode == 1): peer volume monitor ---- */
    uint8_t peer_vol;
} avrcp_ct_ctx_t;

static avrcp_ct_ctx_t s_avrcp_ct;

static void bk_avrcp_ct_emit(bk_avrcp_ct_evt_t evt, void *arg)
{
    if (s_avrcp_ct.event_cb)
    {
        s_avrcp_ct.event_cb(evt, arg, s_avrcp_ct.event_user_data);
    }
}

static uint8_t avrcp_ct_volume_step_up(uint8_t vol)
{
    uint8_t idx = (vol + 5) >> 3;
    if (idx < 16)
    {
        idx += 1;
    }
    return (idx <= 0) ? 0 : (idx >= 16) ? 0x7F : (uint8_t)((idx - 1) * 8 + 9);
}

static uint8_t avrcp_ct_volume_step_down(uint8_t vol)
{
    uint8_t idx = (vol + 5) >> 3;
    if (idx > 0)
    {
        idx -= 1;
    }
    return (idx <= 0) ? 0 : (idx >= 16) ? 0x7F : (uint8_t)((idx - 1) * 8 + 9);
}

static void avrcp_stop_auto_connect_timer(void)
{
    if (rtos_is_oneshot_timer_init(&s_avrcp_ct.auto_connect_timer))
    {
        if (rtos_is_oneshot_timer_running(&s_avrcp_ct.auto_connect_timer))
        {
            rtos_stop_oneshot_timer(&s_avrcp_ct.auto_connect_timer);
        }
        rtos_deinit_oneshot_timer(&s_avrcp_ct.auto_connect_timer);
    }
}

static void avrcp_auto_connect_timer_hdl(void *param, unsigned int ulparam)
{
    (void)param;
    (void)ulparam;
    avrcp_stop_auto_connect_timer();
    if (!s_avrcp_ct.connected && s_avrcp_ct.pending_bda[0] + s_avrcp_ct.pending_bda[1] + s_avrcp_ct.pending_bda[2] +
                                  s_avrcp_ct.pending_bda[3] + s_avrcp_ct.pending_bda[4] + s_avrcp_ct.pending_bda[5])
    {
        LOGI("%02x:%02x:%02x:%02x:%02x:%02x",
             s_avrcp_ct.pending_bda[5], s_avrcp_ct.pending_bda[4], s_avrcp_ct.pending_bda[3],
             s_avrcp_ct.pending_bda[2], s_avrcp_ct.pending_bda[1], s_avrcp_ct.pending_bda[0]);
        bk_bt_avrcp_connect(s_avrcp_ct.pending_bda);
    }
}

static void avrcp_start_auto_connect_timer(const uint8_t bda[6])
{
    if (!s_avrcp_ct.auto_connect_after_a2dp || !bda)
    {
        return;
    }

    os_memcpy(s_avrcp_ct.pending_bda, bda, sizeof(s_avrcp_ct.pending_bda));
    avrcp_stop_auto_connect_timer();
    LOGI("%02x:%02x:%02x:%02x:%02x:%02x",
         bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);
    if (rtos_init_oneshot_timer(&s_avrcp_ct.auto_connect_timer,
                                300,
                                (timer_2handler_t)avrcp_auto_connect_timer_hdl,
                                NULL,
                                0) == BK_OK)
    {
        rtos_start_oneshot_timer(&s_avrcp_ct.auto_connect_timer);
    }
}

void bk_avrcp_ct_service_notify_a2dp_state(uint8_t connected, const uint8_t bda[6])
{
    if (connected)
    {
        avrcp_start_auto_connect_timer(bda);
    }
    else
    {
        avrcp_stop_auto_connect_timer();
    }
}

static int avrcp_send_passthrough(uint8_t cmd)
{
    if (!s_avrcp_ct.connected)
    {
        return BK_FAIL;
    }

    bk_bt_avrcp_ct_send_passthrough_cmd(s_avrcp_ct.remote_bda, cmd, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if (s_avrcp_ct.passthrough_sema)
    {
        rtos_get_semaphore(&s_avrcp_ct.passthrough_sema, AVRCP_PASSTHROUGH_TIMEOUT_MS);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(s_avrcp_ct.remote_bda, cmd, BK_AVRCP_PT_CMD_STATE_RELEASED);
    return BK_OK;
}

static void avrcp_notify_event_handler(uint8_t event_id, bk_avrcp_rn_param_t *event_parameter)
{
    switch (event_id)
    {
    case BK_AVRCP_RN_PLAY_STATUS_CHANGE:
    {
        uint8_t playback = event_parameter->playback;
        LOGI("Playback status changed: 0x%x", playback);
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_PLAY_STATUS_CHANGED, &playback);
        bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_PLAY_STATUS_CHANGE, 0);
        break;
    }
    case BK_AVRCP_RN_TRACK_CHANGE:
    {
        uint64_t track = 0;
        os_memcpy(&track, event_parameter->elm_id, sizeof(event_parameter->elm_id));
        LOGI("track changed: %lld", track);
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_TRACK_CHANGED, &track);
        bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_TRACK_CHANGE, 0);
        break;
    }
    case BK_AVRCP_RN_PLAY_POS_CHANGED:
    {
        uint32_t play_pos = event_parameter->play_pos;
        LOGI("play position changed: %u ms", play_pos);
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_PLAY_POS_CHANGED, &play_pos);
        bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda,
                                                      BK_AVRCP_RN_PLAY_POS_CHANGED,
                                                      AVRCP_PLAY_POS_REPORT_INTERVAL_S);
        break;
    }
    case BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE:
        LOGI("avaliable player changed");
        bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_AVAILABLE_PLAYERS_CHANGE, 0);
        break;
    case BK_AVRCP_RN_VOLUME_CHANGE:
    {
        uint8_t vol = event_parameter->volume;
        LOGI("peer volume changed: 0x%x", vol);
        s_avrcp_ct.peer_vol = vol;
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_REMOTE_VOLUME_CHANGED, &vol);
        bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_VOLUME_CHANGE, 0);
        break;
    }
    case BK_AVRCP_RN_BATTERY_STATUS_CHANGE:
    {
        uint8_t batt = event_parameter->batt;
        LOGI("peer battery changed: %d", batt);
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_REMOTE_BATTERY_CHANGED, &batt);
        bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_BATTERY_STATUS_CHANGE, 0);
        break;
    }
    default:
        LOGW("unhandled event: %d", event_id);
        break;
    }
}

static void avrcp_ct_cb(bk_avrcp_ct_cb_event_t event, bk_avrcp_ct_cb_param_t *param)
{
    bk_avrcp_ct_cb_param_t *avrcp = param;

    LOGI("event: %d", event);

    switch (event)
    {
    case BK_AVRCP_CT_CONNECTION_STATE_EVT:
        s_avrcp_ct.connected = avrcp->conn_state.connected;
        LOGI("AVRCP CT connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
             s_avrcp_ct.connected,
             avrcp->conn_state.remote_bda[5], avrcp->conn_state.remote_bda[4], avrcp->conn_state.remote_bda[3],
             avrcp->conn_state.remote_bda[2], avrcp->conn_state.remote_bda[1], avrcp->conn_state.remote_bda[0]);
        if (s_avrcp_ct.connected)
        {
            os_memcpy(s_avrcp_ct.remote_bda, avrcp->conn_state.remote_bda, sizeof(s_avrcp_ct.remote_bda));
            avrcp_stop_auto_connect_timer();
            bk_bt_avrcp_ct_send_get_rn_capabilities_cmd(s_avrcp_ct.remote_bda);
            bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_CONNECTED, s_avrcp_ct.remote_bda);
        }
        else
        {
            bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_DISCONNECTED, s_avrcp_ct.remote_bda);
            os_memset(s_avrcp_ct.remote_bda, 0, sizeof(s_avrcp_ct.remote_bda));
        }
        break;

    case BK_AVRCP_CT_PASSTHROUGH_RSP_EVT:
    {
        struct avrcp_ct_psth_rsp_param *rsp = &avrcp->psth_rsp;
        LOGI("AVRCP psth rsp 0x%x op 0x%x release %d tl %d %02x:%02x:%02x:%02x:%02x:%02x",
             rsp->rsp_code, rsp->key_code, rsp->key_state, rsp->tl,
             rsp->remote_bda[5], rsp->remote_bda[4], rsp->remote_bda[3],
             rsp->remote_bda[2], rsp->remote_bda[1], rsp->remote_bda[0]);
        if (s_avrcp_ct.passthrough_sema &&
            rsp->key_state == BK_AVRCP_PT_CMD_STATE_PRESSED &&
            (rsp->key_code == BK_AVRCP_PT_CMD_PLAY ||
             rsp->key_code == BK_AVRCP_PT_CMD_PAUSE ||
             rsp->key_code == BK_AVRCP_PT_CMD_FORWARD ||
             rsp->key_code == BK_AVRCP_PT_CMD_BACKWARD ||
             rsp->key_code == BK_AVRCP_PT_CMD_VOL_DOWN ||
             rsp->key_code == BK_AVRCP_PT_CMD_VOL_UP))
        {
            rtos_set_semaphore(&s_avrcp_ct.passthrough_sema);
        }
        break;
    }

    case BK_AVRCP_CT_GET_RN_CAPABILITIES_RSP_EVT:
        LOGI("AVRCP peer supported notification events 0x%x %02x:%02x:%02x:%02x:%02x:%02x",
             avrcp->get_rn_caps_rsp.evt_set.bits,
             avrcp->get_rn_caps_rsp.remote_bda[5], avrcp->get_rn_caps_rsp.remote_bda[4], avrcp->get_rn_caps_rsp.remote_bda[3],
             avrcp->get_rn_caps_rsp.remote_bda[2], avrcp->get_rn_caps_rsp.remote_bda[1], avrcp->get_rn_caps_rsp.remote_bda[0]);
        s_avrcp_ct.peer_cap = avrcp->get_rn_caps_rsp.evt_set.bits;
        if (s_avrcp_ct.remote_volume_mode)
        {
            /* A2DP source: monitor peer speaker volume and battery */
            if (s_avrcp_ct.peer_cap & (0x01 << BK_AVRCP_RN_VOLUME_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_VOLUME_CHANGE, 0);
            }
            if (s_avrcp_ct.peer_cap & (0x01 << BK_AVRCP_RN_BATTERY_STATUS_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_BATTERY_STATUS_CHANGE, 0);
            }
        }
        else
        {
            if (s_avrcp_ct.peer_cap & (0x01 << BK_AVRCP_RN_PLAY_STATUS_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_PLAY_STATUS_CHANGE, 0);
            }
            if (s_avrcp_ct.peer_cap & (0x01 << BK_AVRCP_RN_TRACK_CHANGE))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_RN_TRACK_CHANGE, 0);
            }
            if (s_avrcp_ct.peer_cap & (0x01 << BK_AVRCP_RN_PLAY_POS_CHANGED))
            {
                bk_bt_avrcp_ct_send_register_notification_cmd(s_avrcp_ct.remote_bda,
                                                              BK_AVRCP_RN_PLAY_POS_CHANGED,
                                                              AVRCP_PLAY_POS_REPORT_INTERVAL_S);
            }
        }
        break;

    case BK_AVRCP_CT_CHANGE_NOTIFY_EVT:
        LOGI("AVRCP event notification: %d %02x:%02x:%02x:%02x:%02x:%02x",
             avrcp->change_ntf.event_id,
             avrcp->change_ntf.remote_bda[5], avrcp->change_ntf.remote_bda[4], avrcp->change_ntf.remote_bda[3],
             avrcp->change_ntf.remote_bda[2], avrcp->change_ntf.remote_bda[1], avrcp->change_ntf.remote_bda[0]);
        avrcp_notify_event_handler(avrcp->change_ntf.event_id, &avrcp->change_ntf.event_parameter);
        break;

    case BK_AVRCP_CT_GET_ELEM_ATTR_RSP_EVT:
        LOGI("get elem rsp status %d count %d",
             avrcp->elem_attr_rsp.status, avrcp->elem_attr_rsp.attr_count);
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_ELEM_ATTR_RSP, param);
        break;

    case BK_AVRCP_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
    {
        bk_avrcp_ct_abs_vol_rsp_t rsp =
        {
            .status = avrcp->set_volume_rsp.status,
            .volume = avrcp->set_volume_rsp.volume,
        };
        LOGI("set abs vol rsp status %d vol %d", rsp.status, rsp.volume);
        bk_avrcp_ct_emit(BK_AVRCP_CT_EVT_SET_ABS_VOLUME_RSP, &rsp);
        break;
    }

    default:
        LOGW("Invalid AVRCP event: %d", event);
        break;
    }
}

int bk_avrcp_ct_register_event_cb(bk_avrcp_ct_event_cb_t cb, void *user_data)
{
    s_avrcp_ct.event_cb = cb;
    s_avrcp_ct.event_user_data = user_data;
    return BK_OK;
}

int bk_avrcp_ct_service_init(const bk_avrcp_ct_cfg_t *cfg)
{
    int ret;

    LOGI("");

    if (s_avrcp_ct.inited)
    {
        LOGE("already init");
        return BK_OK;
    }

    s_avrcp_ct.auto_connect_after_a2dp = cfg ? cfg->auto_ct_connect_after_a2dp : 1;
    s_avrcp_ct.remote_volume_mode = cfg ? cfg->remote_volume_mode : 0;

    ret = rtos_init_semaphore(&s_avrcp_ct.passthrough_sema, 1);
    if (ret != BK_OK)
    {
        LOGE("avrcp evt sem init err %d", ret);
        return ret;
    }

    bk_bt_avrcp_ct_init();
    bk_bt_avrcp_ct_register_callback(avrcp_ct_cb);

    s_avrcp_ct.inited = 1;
    LOGI("end");
    return BK_OK;
}

int bk_avrcp_ct_service_deinit(void)
{
    LOGI("");

    avrcp_stop_auto_connect_timer();
    bk_bt_avrcp_ct_register_callback(NULL);
    bk_bt_avrcp_ct_deinit();

    if (s_avrcp_ct.passthrough_sema)
    {
        rtos_deinit_semaphore(&s_avrcp_ct.passthrough_sema);
    }

    os_memset(&s_avrcp_ct, 0, sizeof(s_avrcp_ct));
    LOGI("end");
    return BK_OK;
}

int bk_avrcp_ct_connect(const uint8_t bda[6])
{
    if (!bda)
    {
        return BK_FAIL;
    }
    LOGI("%02x:%02x:%02x:%02x:%02x:%02x",
         bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);
    avrcp_stop_auto_connect_timer();
    return bk_bt_avrcp_connect((uint8_t *)bda);
}

int bk_avrcp_ct_disconnect(const uint8_t bda[6])
{
    (void)bda;
    avrcp_stop_auto_connect_timer();
    LOGW("disconnect API not exposed by current SDK");
    return BK_FAIL;
}

int bk_avrcp_stop_reconnect(void)
{
    avrcp_stop_auto_connect_timer();
    return BK_OK;
}

int bk_avrcp_ct_is_connected(void)
{
    return s_avrcp_ct.connected;
}

int bk_avrcp_ct_play(void)
{
    return avrcp_send_passthrough(BK_AVRCP_PT_CMD_PLAY);
}

int bk_avrcp_ct_pause(void)
{
    return avrcp_send_passthrough(BK_AVRCP_PT_CMD_PAUSE);
}

int bk_avrcp_ct_next(void)
{
    return avrcp_send_passthrough(BK_AVRCP_PT_CMD_FORWARD);
}

int bk_avrcp_ct_prev(void)
{
    return avrcp_send_passthrough(BK_AVRCP_PT_CMD_BACKWARD);
}

int bk_avrcp_ct_rewind(uint32_t ms)
{
    if (!s_avrcp_ct.connected)
    {
        return BK_FAIL;
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_PT_CMD_REWIND, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if (ms)
    {
        rtos_delay_milliseconds(ms);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_PT_CMD_REWIND, BK_AVRCP_PT_CMD_STATE_RELEASED);
    return BK_OK;
}

int bk_avrcp_ct_fast_forward(uint32_t ms)
{
    if (!s_avrcp_ct.connected)
    {
        return BK_FAIL;
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_PT_CMD_FAST_FORWARD, BK_AVRCP_PT_CMD_STATE_PRESSED);
    if (ms)
    {
        rtos_delay_milliseconds(ms);
    }
    bk_bt_avrcp_ct_send_passthrough_cmd(s_avrcp_ct.remote_bda, BK_AVRCP_PT_CMD_FAST_FORWARD, BK_AVRCP_PT_CMD_STATE_RELEASED);
    return BK_OK;
}

int bk_avrcp_ct_vol_up(void)
{
    uint8_t old = bk_avrcp_tg_get_local_volume_value();
    uint8_t next = avrcp_ct_volume_step_up(old);

    if (next == old)
    {
        LOGI("vol already max %d %d", old, next);
        return BK_OK;
    }

    bk_avrcp_tg_set_local_volume(next, s_avrcp_ct.remote_bda);
    if (bk_avrcp_tg_notify_volume_change(next) != BK_OK && s_avrcp_ct.connected)
    {
        LOGE("peer not reg vol change, adjust local only !!!");
        avrcp_send_passthrough(BK_AVRCP_PT_CMD_VOL_UP);
    }
    LOGI("vol_up, vol: %d -> %d", old, bk_avrcp_tg_get_local_volume_value());
    return BK_OK;
}

int bk_avrcp_ct_vol_down(void)
{
    uint8_t old = bk_avrcp_tg_get_local_volume_value();
    uint8_t next = avrcp_ct_volume_step_down(old);

    if (next == old)
    {
        LOGI("vol already min %d %d", old, next);
        return BK_OK;
    }

    bk_avrcp_tg_set_local_volume(next, s_avrcp_ct.remote_bda);
    if (bk_avrcp_tg_notify_volume_change(next) != BK_OK && s_avrcp_ct.connected)
    {
        LOGE("peer not reg vol change, adjust local only !!!");
        avrcp_send_passthrough(BK_AVRCP_PT_CMD_VOL_DOWN);
    }
    LOGI("vol_down, vol: %d -> %d", old, bk_avrcp_tg_get_local_volume_value());
    return BK_OK;
}

int bk_avrcp_ct_get_attr(uint32_t attr_id)
{
    uint32_t media_attr_id_mask = (1 << BK_AVRCP_MEDIA_ATTR_ID_TITLE);

    if (attr_id)
    {
        media_attr_id_mask = (1 << attr_id);
    }

    return bk_bt_avrcp_ct_send_get_elem_attribute_cmd(s_avrcp_ct.remote_bda, media_attr_id_mask);
}

int bk_avrcp_ct_send_absolute_volume(uint8_t vol_0_7f)
{
    if (!s_avrcp_ct.connected)
    {
        return BK_FAIL;
    }

    return bk_bt_avrcp_ct_send_absolute_volume_cmd(s_avrcp_ct.remote_bda, vol_0_7f > 0x7F ? 0x7F : vol_0_7f);
}
