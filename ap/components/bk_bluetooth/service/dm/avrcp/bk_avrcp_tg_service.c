#include <components/system.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>

#include "bk_avrcp_tg_service.h"

#include "components/bluetooth/bk_dm_avrcp.h"
#include "bluetooth_storage.h"
#include "bt_manager.h"

#define TAG "bk_avrcp_tg"

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


#define AVRCP_GAIN_MAX            (128 - 1)
#define PLATFORM_SPK_GAIN_MAX     0x3f
#define PLATFORM_SPK_GAIN_DEFAULT 0x2d

/* player_mode: passthrough commands are deferred to a worker thread so the
 * stack callback never blocks on the application music policy. */
#define AVRCP_TG_CTX_MSG_COUNT    5

enum
{
    AVRCP_TG_MSG_PASSTHROUGH,
};

typedef struct
{
    uint8_t type;
    uint8_t key;
} avrcp_tg_ctx_msg_t;

typedef struct
{
    /* ---- common (both modes) ---- */
    uint8_t inited;
    uint8_t connected;
    uint8_t player_mode;              /* config: 0=volume-sink, 1=media-player */
    uint16_t registered_noti;         /* peer-registered notification bitmask */
    uint8_t remote_bda[6];
    bk_avrcp_tg_event_cb_t event_cb;
    void *event_user_data;

    /* ---- volume-sink mode (player_mode == 0): local speaker volume ---- */
    uint8_t default_volume;
    uint8_t local_volume;

    /* ---- player mode (player_mode == 1): media-player state ---- */
    uint8_t last_playback;            /* cached to answer INTERIM registered notifications */
    uint32_t last_play_pos;
    uint64_t last_track_id;           /* opaque 8-byte track id for TRACK_CHANGE */
    beken_timer_t pos_timer;          /* periodic play-position report */
    beken_thread_t ctx_thread;        /* passthrough worker */
    beken_queue_t ctx_queue;
} avrcp_tg_ctx_t;

static avrcp_tg_ctx_t s_avrcp_tg;

static void avrcp_save_volume_for_addr(const uint8_t *bda, uint8_t vol)
{
    uint8_t addr[6] = {0};

    if (bda)
    {
        os_memcpy(addr, bda, sizeof(addr));
    }
    else if (bt_manager_get_connected_device())
    {
        os_memcpy(addr, bt_manager_get_connected_device(), sizeof(addr));
    }
    else
    {
        LOGW("skip save volume %d, no device address", vol);
        return;
    }

    LOGI("save volume %d %02x:%02x:%02x:%02x:%02x:%02x",
         vol, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    bluetooth_storage_save_volume(addr, vol);
}

static uint8_t avrcp_clamp_volume(uint8_t vol)
{
    return vol > 0x7F ? 0x7F : vol;
}

static void avrcp_tg_restore_initial_volume(void)
{
    uint8_t addr[6] = {0};

    s_avrcp_tg.local_volume = s_avrcp_tg.default_volume;
    if (bluetooth_storage_get_newest_linkkey_info(addr, NULL) >= 0)
    {
        uint8_t stored = 0;
        if (bluetooth_storage_find_volume_by_addr(addr, &stored) < 0)
        {
            s_avrcp_tg.local_volume = 1.0 * PLATFORM_SPK_GAIN_DEFAULT / PLATFORM_SPK_GAIN_MAX * AVRCP_GAIN_MAX;
        }
        else
        {
            s_avrcp_tg.local_volume = stored;
        }

        if (s_avrcp_tg.local_volume == 0)
        {
            s_avrcp_tg.local_volume = 1.0 * PLATFORM_SPK_GAIN_DEFAULT / PLATFORM_SPK_GAIN_MAX * AVRCP_GAIN_MAX;
        }

        LOGI("initial volume %d %02x:%02x:%02x:%02x:%02x:%02x",
             s_avrcp_tg.local_volume, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    }
    else
    {
        LOGI("can't find linkkey info");
    }
}

static void bk_avrcp_tg_emit(bk_avrcp_tg_evt_t evt, void *arg)
{
    if (s_avrcp_tg.event_cb)
    {
        s_avrcp_tg.event_cb(evt, arg, s_avrcp_tg.event_user_data);
    }
}

void bk_avrcp_tg_emit_current_volume(void)
{
    uint8_t local_volume = bk_avrcp_tg_get_local_volume_value();
    bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_VOLUME_CHANGED, &local_volume);
}

/* ---- player_mode: passthrough worker ---- */

static void avrcp_tg_ctx_task(void *arg)
{
    avrcp_tg_ctx_msg_t msg;
    (void)arg;

    while (1)
    {
        if (rtos_pop_from_queue(&s_avrcp_tg.ctx_queue, &msg, BEKEN_WAIT_FOREVER))
        {
            break;
        }

        switch (msg.type)
        {
        case AVRCP_TG_MSG_PASSTHROUGH:
        {
            bk_avrcp_tg_passthrough_t pt = { .key_code = msg.key, .key_state = 1 };
            bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_PASSTHROUGH, &pt);
            break;
        }

        default:
            break;
        }
    }

    rtos_delete_thread(NULL);
}

static int avrcp_tg_create_ctx_task(void)
{
    int ret;

    if (s_avrcp_tg.ctx_thread)
    {
        return BK_OK;
    }

    ret = rtos_init_queue(&s_avrcp_tg.ctx_queue, "avrcp_tg_q",
                          sizeof(avrcp_tg_ctx_msg_t), AVRCP_TG_CTX_MSG_COUNT);
    if (ret != BK_OK)
    {
        LOGE("queue init err %d", ret);
        s_avrcp_tg.ctx_queue = NULL;
        return ret;
    }

    ret = rtos_create_thread(&s_avrcp_tg.ctx_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 2,
                             "avrcp_tg_ctx",
                             (beken_thread_function_t)avrcp_tg_ctx_task,
                             1024 * 3,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("thread create err %d", ret);
        s_avrcp_tg.ctx_thread = NULL;
        rtos_deinit_queue(&s_avrcp_tg.ctx_queue);
        s_avrcp_tg.ctx_queue = NULL;
        return ret;
    }

    return BK_OK;
}

static void avrcp_tg_delete_ctx_task(void)
{
    if (s_avrcp_tg.ctx_queue)
    {
        /* task is blocked on pop; deinit makes it return an error and self-exit */
        rtos_deinit_queue(&s_avrcp_tg.ctx_queue);
        s_avrcp_tg.ctx_queue = NULL;
    }
    s_avrcp_tg.ctx_thread = NULL;
}

/* ---- player_mode: play position periodic report ---- */

static void avrcp_tg_pos_stop_timer(void)
{
    if (rtos_is_timer_init(&s_avrcp_tg.pos_timer))
    {
        if (rtos_is_timer_running(&s_avrcp_tg.pos_timer))
        {
            rtos_stop_timer(&s_avrcp_tg.pos_timer);
        }
        rtos_deinit_timer(&s_avrcp_tg.pos_timer);
    }
}

static void avrcp_tg_pos_timer_hdl(void *param)
{
    static uint32_t last_play_pos = 0;
    (void)param;
    if (last_play_pos != s_avrcp_tg.last_play_pos)
    {
        last_play_pos = s_avrcp_tg.last_play_pos;
        bk_avrcp_tg_notify_play_pos(s_avrcp_tg.last_play_pos);
    }
}

static int avrcp_tg_pos_start_timer(uint32_t ms)
{
    int ret;

    avrcp_tg_pos_stop_timer();

    ret = rtos_init_timer(&s_avrcp_tg.pos_timer, ms, avrcp_tg_pos_timer_hdl, NULL);
    if (ret != BK_OK)
    {
        LOGE("init pos timer err %d", ret);
        return ret;
    }
    ret = rtos_start_timer(&s_avrcp_tg.pos_timer);
    if (ret != BK_OK)
    {
        LOGE("start pos timer err %d", ret);
    }
    return ret;
}

static void avrcp_tg_cb(bk_avrcp_tg_cb_event_t event, bk_avrcp_tg_cb_param_t *param)
{
    LOGI("event: %d", event);

    switch (event)
    {
    case BK_AVRCP_TG_CONNECTION_STATE_EVT:
        s_avrcp_tg.connected = param->conn_stat.connected;
        LOGI("avrcp tg connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
             s_avrcp_tg.connected,
             param->conn_stat.remote_bda[5], param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[3],
             param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[1], param->conn_stat.remote_bda[0]);
        if (s_avrcp_tg.connected)
        {
            os_memcpy(s_avrcp_tg.remote_bda, param->conn_stat.remote_bda, sizeof(s_avrcp_tg.remote_bda));
            bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_CONNECTED, s_avrcp_tg.remote_bda);
        }
        else
        {
            s_avrcp_tg.registered_noti = 0;
            /* peer's notification registrations are gone on link loss: stop the
             * periodic play-position report so it doesn't fire on a dead link */
            avrcp_tg_pos_stop_timer();
            bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_DISCONNECTED, s_avrcp_tg.remote_bda);
            os_memset(s_avrcp_tg.remote_bda, 0, sizeof(s_avrcp_tg.remote_bda));
        }
        break;

    case BK_AVRCP_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
        LOGI("recv abs vol 0x%x %02x:%02x:%02x:%02x:%02x:%02x",
             param->set_abs_vol.volume,
             param->set_abs_vol.remote_bda[5], param->set_abs_vol.remote_bda[4], param->set_abs_vol.remote_bda[3],
             param->set_abs_vol.remote_bda[2], param->set_abs_vol.remote_bda[1], param->set_abs_vol.remote_bda[0]);
        bk_avrcp_tg_set_local_volume(param->set_abs_vol.volume, param->set_abs_vol.remote_bda);
        break;

    case BK_AVRCP_TG_PASSTHROUGH_CMD_EVT:
        LOGI("recv pt key 0x%02x state %d",
             param->psth_cmd.key_code, param->psth_cmd.key_state);
        /* forward only key-press events, and only in player_mode, deferred to worker */
        if (s_avrcp_tg.player_mode && param->psth_cmd.key_state && s_avrcp_tg.ctx_queue)
        {
            avrcp_tg_ctx_msg_t msg = { .type = AVRCP_TG_MSG_PASSTHROUGH, .key = param->psth_cmd.key_code };
            if (rtos_push_to_queue(&s_avrcp_tg.ctx_queue, &msg, BEKEN_NO_WAIT))
            {
                LOGW("passthrough queue full");
            }
        }
        break;

    case BK_AVRCP_TG_REGISTER_NOTIFICATION_EVT:
    {
        bk_avrcp_rn_param_t cmd;
        uint8_t send = 1;
        s_avrcp_tg.registered_noti |= (1 << param->reg_ntf.event_id);
        LOGI("recv reg evt 0x%x param %d %02x:%02x:%02x:%02x:%02x:%02x",
             param->reg_ntf.event_id,
             param->reg_ntf.event_parameter,
             param->reg_ntf.remote_bda[5], param->reg_ntf.remote_bda[4], param->reg_ntf.remote_bda[3],
             param->reg_ntf.remote_bda[2], param->reg_ntf.remote_bda[1], param->reg_ntf.remote_bda[0]);

        os_memset(&cmd, 0, sizeof(cmd));

        switch (param->reg_ntf.event_id)
        {
        case BK_AVRCP_RN_VOLUME_CHANGE:
            cmd.volume = bk_avrcp_tg_get_local_volume_value();
            break;

        case BK_AVRCP_RN_PLAY_STATUS_CHANGE:
            cmd.playback = s_avrcp_tg.last_playback;
            break;

        case BK_AVRCP_RN_TRACK_CHANGE:
            os_memcpy(cmd.elm_id, &s_avrcp_tg.last_track_id, sizeof(cmd.elm_id));
            break;

        case BK_AVRCP_RN_PLAY_POS_CHANGED:
            if (param->reg_ntf.event_parameter)
            {
                avrcp_tg_pos_start_timer(param->reg_ntf.event_parameter * 1000);
            }
            cmd.play_pos = s_avrcp_tg.last_play_pos;
            break;

        case BK_AVRCP_RN_BATTERY_STATUS_CHANGE:
            cmd.batt = BK_AVRCP_BATT_EXTERNAL;
            break;

        default:
            LOGW("unknow reg event 0x%x", param->reg_ntf.event_id);
            send = 0;
            break;
        }

        if (send)
        {
            bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                       param->reg_ntf.event_id,
                                       BK_AVRCP_RN_RSP_INTERIM,
                                       &cmd);
        }
        break;
    }

    default:
        LOGW("unknow event 0x%x", event);
        break;
    }
}

int bk_avrcp_tg_register_event_cb(bk_avrcp_tg_event_cb_t cb, void *user_data)
{
    s_avrcp_tg.event_cb = cb;
    s_avrcp_tg.event_user_data = user_data;
    return BK_OK;
}

int bk_avrcp_tg_set_local_volume(uint8_t vol, const uint8_t *bda)
{
    s_avrcp_tg.local_volume = avrcp_clamp_volume(vol);
    avrcp_save_volume_for_addr(bda, s_avrcp_tg.local_volume);
    bk_avrcp_tg_emit_current_volume();
    return BK_OK;
}

uint8_t bk_avrcp_tg_get_local_volume_value(void)
{
    return s_avrcp_tg.local_volume;
}

int bk_avrcp_tg_service_init(const bk_avrcp_tg_cfg_t *cfg)
{
    bk_avrcp_rn_evt_cap_mask_t tmp_cap = {0};
    bk_avrcp_rn_evt_cap_mask_t final_cap = {0};
    int ret;

    LOGI("");

    if (s_avrcp_tg.inited)
    {
        LOGE("already init");
        return BK_OK;
    }

    s_avrcp_tg.player_mode = cfg ? cfg->player_mode : 0;

    s_avrcp_tg.default_volume = cfg ? avrcp_clamp_volume(cfg->default_volume) : 0x5A;
    if (s_avrcp_tg.default_volume == 0)
    {
        s_avrcp_tg.default_volume = 0x5A;
    }

    avrcp_tg_restore_initial_volume();

    /* player_mode (A2DP source) additionally advertises player-side notifications */
    if (s_avrcp_tg.player_mode)
    {
        final_cap.bits |= (1 << BK_AVRCP_RN_PLAY_STATUS_CHANGE)
                          | (1 << BK_AVRCP_RN_TRACK_CHANGE)
                          | (1 << BK_AVRCP_RN_PLAY_POS_CHANGED)
                          | (1 << BK_AVRCP_RN_BATTERY_STATUS_CHANGE);
    }
    else
    {
        final_cap.bits |= (1 << BK_AVRCP_RN_VOLUME_CHANGE);
    }

    bk_bt_avrcp_tg_init();
    bk_bt_avrcp_tg_get_rn_evt_cap(BK_AVRCP_RN_CAP_API_METHOD_ALLOWED, &tmp_cap);
    final_cap.bits &= tmp_cap.bits;
    LOGI("set rn cap 0x%x", final_cap.bits);
    ret = bk_bt_avrcp_tg_set_rn_evt_cap(&final_cap);
    if (ret != BK_OK)
    {
        LOGE("set rn cap err %d", ret);
        bk_bt_avrcp_tg_deinit();
        return ret;
    }

    /* player_mode accepts remote passthrough keys and defers them to a worker */
    if (s_avrcp_tg.player_mode)
    {
        bk_avrcp_psth_bit_mask_t sup_filter = {0};
        bk_avrcp_psth_bit_mask_t final_filter = {0};
        const uint8_t try_filter[] =
        {
            BK_AVRCP_PT_CMD_VOL_UP,
            BK_AVRCP_PT_CMD_VOL_DOWN,
            BK_AVRCP_PT_CMD_PLAY,
            BK_AVRCP_PT_CMD_STOP,
            BK_AVRCP_PT_CMD_PAUSE,
            BK_AVRCP_PT_CMD_REWIND,
            BK_AVRCP_PT_CMD_FAST_FORWARD,
            BK_AVRCP_PT_CMD_FORWARD,
            BK_AVRCP_PT_CMD_BACKWARD,
        };

        bk_bt_avrcp_tg_get_psth_cmd_filter(BK_AVRCP_PSTH_FILTER_METHOD_ALLOWED, &sup_filter);
        for (uint32_t i = 0; i < sizeof(try_filter) / sizeof(try_filter[0]); ++i)
        {
            if (bk_bt_avrcp_psth_bit_mask_operation(BK_AVRCP_BIT_MASK_OP_TEST, &sup_filter, try_filter[i]))
            {
                bk_bt_avrcp_psth_bit_mask_operation(BK_AVRCP_BIT_MASK_OP_SET, &final_filter, try_filter[i]);
            }
        }

        ret = bk_bt_avrcp_tg_set_psth_cmd_filter(BK_AVRCP_PSTH_FILTER_METHOD_CURRENT_ENABLE, &final_filter);
        if (ret != BK_OK)
        {
            LOGE("set psth filter err %d", ret);
        }

        avrcp_tg_create_ctx_task();
    }

    bk_bt_avrcp_tg_register_callback(avrcp_tg_cb);

    s_avrcp_tg.inited = 1;
    LOGI("end");
    return BK_OK;
}

int bk_avrcp_tg_service_deinit(void)
{
    LOGI("");

    avrcp_tg_pos_stop_timer();
    avrcp_tg_delete_ctx_task();
    bk_bt_avrcp_tg_register_callback(NULL);
    bk_bt_avrcp_tg_deinit();
    os_memset(&s_avrcp_tg, 0, sizeof(s_avrcp_tg));

    LOGI("end");
    return BK_OK;
}

int bk_avrcp_tg_disconnect(const uint8_t bda[6])
{
    (void)bda;
    LOGW("disconnect API not exposed by current SDK");
    return BK_FAIL;
}

int bk_avrcp_tg_is_connected(void)
{
    return s_avrcp_tg.connected;
}

int bk_avrcp_tg_notify_volume_change(uint8_t vol_0_7f)
{
    bk_avrcp_rn_param_t cmd = {0};

    if (!(s_avrcp_tg.registered_noti & (1 << BK_AVRCP_RN_VOLUME_CHANGE)))
    {
        return BK_FAIL;
    }

    cmd.volume = vol_0_7f > 0x7F ? 0x7F : vol_0_7f;
    return bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                      BK_AVRCP_RN_VOLUME_CHANGE,
                                      BK_AVRCP_RN_RSP_CHANGED,
                                      &cmd);
}

int bk_avrcp_tg_notify_playback_status(uint8_t playback)
{
    bk_avrcp_rn_param_t cmd = {0};

    s_avrcp_tg.last_playback = playback;

    if (!s_avrcp_tg.connected)
    {
        return BK_FAIL;
    }
    if (!(s_avrcp_tg.registered_noti & (1 << BK_AVRCP_RN_PLAY_STATUS_CHANGE)))
    {
        return BK_OK;
    }

    cmd.playback = playback;
    s_avrcp_tg.registered_noti &= ~(1 << BK_AVRCP_RN_PLAY_STATUS_CHANGE);
    LOGI("report playback %d", playback);
    return bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                      BK_AVRCP_RN_PLAY_STATUS_CHANGE,
                                      BK_AVRCP_RN_RSP_CHANGED,
                                      &cmd);
}

int bk_avrcp_tg_notify_track_change(uint64_t track_id)
{
    bk_avrcp_rn_param_t cmd = {0};

    s_avrcp_tg.last_track_id = track_id;

    if (!s_avrcp_tg.connected)
    {
        return BK_FAIL;
    }
    if (!(s_avrcp_tg.registered_noti & (1 << BK_AVRCP_RN_TRACK_CHANGE)))
    {
        return BK_OK;
    }

    os_memcpy(cmd.elm_id, &track_id, sizeof(cmd.elm_id));
    s_avrcp_tg.registered_noti &= ~(1 << BK_AVRCP_RN_TRACK_CHANGE);
    LOGI("report track change %u", (uint32_t)track_id);
    return bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                      BK_AVRCP_RN_TRACK_CHANGE,
                                      BK_AVRCP_RN_RSP_CHANGED,
                                      &cmd);
}

int bk_avrcp_tg_notify_play_pos(uint32_t pos_ms)
{
    bk_avrcp_rn_param_t cmd = {0};

    s_avrcp_tg.last_play_pos = pos_ms;

    if (!s_avrcp_tg.connected)
    {
        return BK_FAIL;
    }
    if (!(s_avrcp_tg.registered_noti & (1 << BK_AVRCP_RN_PLAY_POS_CHANGED)))
    {
        return BK_OK;
    }

    cmd.play_pos = pos_ms;
    s_avrcp_tg.registered_noti &= ~(1 << BK_AVRCP_RN_PLAY_POS_CHANGED);
    LOGI("report play pos %u", pos_ms);
    return bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                      BK_AVRCP_RN_PLAY_POS_CHANGED,
                                      BK_AVRCP_RN_RSP_CHANGED,
                                      &cmd);
}

int bk_avrcp_tg_notify_battery_status(uint8_t batt)
{
    bk_avrcp_rn_param_t cmd = {0};

    if (!s_avrcp_tg.connected)
    {
        return BK_FAIL;
    }
    if (!(s_avrcp_tg.registered_noti & (1 << BK_AVRCP_RN_BATTERY_STATUS_CHANGE)))
    {
        return BK_OK;
    }

    cmd.batt = batt;
    s_avrcp_tg.registered_noti &= ~(1 << BK_AVRCP_RN_BATTERY_STATUS_CHANGE);
    LOGI("report batt %d", batt);
    return bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                      BK_AVRCP_RN_BATTERY_STATUS_CHANGE,
                                      BK_AVRCP_RN_RSP_CHANGED,
                                      &cmd);
}
