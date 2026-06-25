#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>

#include "bk_a2dp_sink_service.h"
#include "bk_avrcp_ct_service.h"
#include "components/bluetooth/bk_dm_a2dp.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "bt_manager.h"

#define TAG "bk_a2dp_srv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t inited;
    uint8_t a2dp_connected;
    uint8_t auto_accept_conn;
    uint8_t bt_manager_index;
    bk_a2dp_audio_state_t audio_state;
    bk_a2dp_mcc_t codec;
    beken_semaphore_t api_sync_sema;
    beken_semaphore_t disconnect_sema;
    bk_a2dp_sink_event_cb_t event_cb;
    void *event_user_data;
} a2dp_service_ctx_t;

static a2dp_service_ctx_t s_a2dp;

static void a2dp_service_emit(bk_a2dp_sink_evt_t evt, void *arg)
{
    if (s_a2dp.event_cb)
    {
        s_a2dp.event_cb(evt, arg, s_a2dp.event_user_data);
    }
}

static void a2dp_service_emit_connection_event(bk_a2dp_sink_evt_t evt, const uint8_t remote_bda[6])
{
    bk_a2dp_sink_conn_t conn = {0};

    if (remote_bda)
    {
        os_memcpy(conn.remote_bda, remote_bda, sizeof(conn.remote_bda));
    }
    a2dp_service_emit(evt, &conn);
}

static void a2dp_service_media_cb(const uint8_t *data, uint16_t data_len)
{
    bk_a2dp_media_data_t media = {0};

    if (!data || !data_len)
    {
        return;
    }

    media.data = data;
    media.len = data_len;
    a2dp_service_emit(BK_A2DP_SINK_EVT_MEDIA_DATA, &media);
}

static void a2dp_service_stream_suspend(void)
{
    LOGI("%s\n", __func__);
    a2dp_service_emit(BK_A2DP_SINK_EVT_STREAM_SUSPEND, NULL);
}

static void a2dp_service_stream_start(const bk_a2dp_mcc_t *codec)
{
    if (!codec)
    {
        return;
    }

    LOGI("%s codec_id %d\n", __func__, codec->type);
    a2dp_service_emit(BK_A2DP_SINK_EVT_STREAM_START, (void *)codec);
}

static void a2dp_service_bt_connect(uint8_t *remote_addr)
{
    if (!remote_addr)
    {
        LOGE("%s null remote addr\n", __func__);
        return;
    }

    if (!s_a2dp.auto_accept_conn)
    {
        LOGW("%s auto_accept disabled, skip reconnect\n", __func__);
        return;
    }

    LOGI("%s %02x:%02x:%02x:%02x:%02x:%02x\n",
         __func__,
         remote_addr[5],
         remote_addr[4],
         remote_addr[3],
         remote_addr[2],
         remote_addr[1],
         remote_addr[0]);
    bk_a2dp_sink_connect(remote_addr);
}

static void a2dp_service_bt_disconnect(uint8_t *remote_addr)
{
    if (!remote_addr)
    {
        LOGE("%s null remote addr\n", __func__);
        return;
    }

    LOGI("%s %02x:%02x:%02x:%02x:%02x:%02x\n",
         __func__,
         remote_addr[5],
         remote_addr[4],
         remote_addr[3],
         remote_addr[2],
         remote_addr[1],
         remote_addr[0]);
    bk_a2dp_sink_disconnect(remote_addr);
}

static void a2dp_service_stop_connect(void)
{
    LOGI("%s\n", __func__);
    bk_avrcp_ct_service_notify_a2dp_state(0, NULL);
}

static void a2dp_service_cb(bk_a2dp_cb_event_t event, bk_a2dp_cb_param_t *param)
{
    bk_a2dp_cb_param_t *a2dp = param;

    LOGI("%s event: %d\n", __func__, event);
    if (!param)
    {
        LOGE("%s null param\n", __func__);
        return;
    }

    switch (event)
    {
    case BK_A2DP_PROF_STATE_EVT:
        LOGI("a2dp prof init action %d status %d reason %d\n",
             param->a2dp_prof_stat.action,
             param->a2dp_prof_stat.status,
             param->a2dp_prof_stat.reason);
        if (param && !param->a2dp_prof_stat.status && s_a2dp.api_sync_sema)
        {
            rtos_set_semaphore(&s_a2dp.api_sync_sema);
        }
        break;

    case BK_A2DP_CONNECTION_STATE_EVT:
        LOGI("A2DP connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
             a2dp->conn_state.state,
             a2dp->conn_state.remote_bda[5],
             a2dp->conn_state.remote_bda[4],
             a2dp->conn_state.remote_bda[3],
             a2dp->conn_state.remote_bda[2],
             a2dp->conn_state.remote_bda[1],
             a2dp->conn_state.remote_bda[0]);
        if (a2dp->conn_state.state == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
        {
            s_a2dp.a2dp_connected = 0;
            if (s_a2dp.audio_state == BK_A2DP_AUDIO_STATE_STARTED)
            {
                s_a2dp.audio_state = BK_A2DP_AUDIO_STATE_SUSPEND;
                a2dp_service_stream_suspend();
            }
            bk_avrcp_ct_service_notify_a2dp_state(0, a2dp->conn_state.remote_bda);
            a2dp_service_emit_connection_event(BK_A2DP_SINK_EVT_DISCONNECTED, a2dp->conn_state.remote_bda);
            if (s_a2dp.disconnect_sema)
            {
                rtos_set_semaphore(&s_a2dp.disconnect_sema);
            }
        }
        else if (a2dp->conn_state.state == BK_A2DP_CONNECTION_STATE_CONNECTED)
        {
            bt_manager_set_connect_state(BT_STATE_PROFILE_CONNECTED);
            s_a2dp.a2dp_connected = 1;
            bk_avrcp_ct_service_notify_a2dp_state(1, a2dp->conn_state.remote_bda);
            a2dp_service_emit_connection_event(BK_A2DP_SINK_EVT_CONNECTED, a2dp->conn_state.remote_bda);
        }
        break;

    case BK_A2DP_AUDIO_STATE_EVT:
        LOGI("A2DP audio state: %d\n", a2dp->audio_state.state);
        if (a2dp->audio_state.state == BK_A2DP_AUDIO_STATE_STARTED)
        {
            s_a2dp.audio_state = a2dp->audio_state.state;
            a2dp_service_stream_start(&s_a2dp.codec);
        }
        else if (a2dp->audio_state.state == BK_A2DP_AUDIO_STATE_SUSPEND &&
                 s_a2dp.audio_state == BK_A2DP_AUDIO_STATE_STARTED)
        {
            s_a2dp.audio_state = a2dp->audio_state.state;
            a2dp_service_stream_suspend();
        }
        break;

    case BK_A2DP_AUDIO_CFG_EVT:
        s_a2dp.codec = a2dp->audio_cfg.mcc;
        LOGI("%s codec_id %d\n", __func__, s_a2dp.codec.type);
        a2dp_service_emit(BK_A2DP_SINK_EVT_AUDIO_CFG, &s_a2dp.codec);
        break;

    case BK_A2DP_L2CAP_CONNECT_REQ_EVT:
    {
        struct a2dp_l2cap_connect_req_param *req = &a2dp->a2dp_l2cap_connect_req;
        req->accept = s_a2dp.auto_accept_conn;
        LOGI("%s BK_A2DP_L2CAP_CONNECT_REQ_EVT %02x:%02x:%02x:%02x:%02x:%02x, %s\n",
             __func__,
             req->remote_bda[5],
             req->remote_bda[4],
             req->remote_bda[3],
             req->remote_bda[2],
             req->remote_bda[1],
             req->remote_bda[0],
             req->accept ? "accept" : "reject");
        if (!req->accept)
        {
            bt_manager_set_connect_state(BT_STATE_PROFILE_CONNECTED);
        }
        break;
    }

    case BK_A2DP_SET_CAP_COMPLETED_EVT:
        LOGI("%s set cap status 0x%x\n", __func__, param->a2dp_set_cap_completed.status);
        if (s_a2dp.api_sync_sema)
        {
            rtos_set_semaphore(&s_a2dp.api_sync_sema);
        }
        break;

    default:
        LOGW("Invalid A2DP event: %d\n", event);
        break;
    }
}

int bk_a2dp_sink_register_event_cb(bk_a2dp_sink_event_cb_t cb, void *user_data)
{
    s_a2dp.event_cb = cb;
    s_a2dp.event_user_data = user_data;
    return BK_OK;
}

int bk_a2dp_sink_connect(const uint8_t bda[6])
{
    if (bda)
    {
        LOGI("%s %02x:%02x:%02x:%02x:%02x:%02x\n",
             __func__, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);
    }
    return bda ? bk_bt_a2dp_sink_connect((uint8_t *)bda) : BK_FAIL;
}

int bk_a2dp_sink_disconnect(const uint8_t bda[6])
{
    if (bda)
    {
        LOGI("%s %02x:%02x:%02x:%02x:%02x:%02x\n",
             __func__, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);
    }
    return bda ? bk_bt_a2dp_sink_disconnect((uint8_t *)bda) : BK_FAIL;
}

int bk_a2dp_sink_service_init(const bk_a2dp_sink_cfg_t *cfg)
{
    int ret;
    bk_a2dp_codec_cap_t cap = {
        .type = BK_A2DP_CODEC_TYPE_SBC,
        .param.sbc_codec_cap.channel_mode =
            BK_A2DP_SBC_CHANNEL_MODE_MONO |
            BK_A2DP_SBC_CHANNEL_MODE_DUAL |
            BK_A2DP_SBC_CHANNEL_MODE_STEREO |
            BK_A2DP_SBC_CHANNEL_MODE_JOINT_STEREO,
        .param.sbc_codec_cap.bit_pool_max = 35,
    };

    LOGI("%s\n", __func__);

    if (s_a2dp.inited)
    {
        LOGE("%s already init\n", __func__);
        return BK_OK;
    }

    s_a2dp.auto_accept_conn = cfg ? cfg->auto_accept_conn : 1;
    s_a2dp.audio_state = BK_A2DP_AUDIO_STATE_SUSPEND;
    s_a2dp.bt_manager_index = 0xFF;

    ret = rtos_init_semaphore(&s_a2dp.api_sync_sema, 1);
    if (ret != BK_OK)
    {
        LOGE("%s sem init err %d\n", __func__, ret);
        return ret;
    }

    {
        btm_callback_s btm_cb = {
            .start_connect_cb = a2dp_service_bt_connect,
            .stop_connect_cb = a2dp_service_stop_connect,
            .start_disconnect_cb = a2dp_service_bt_disconnect,
        };
        s_a2dp.bt_manager_index = bt_manager_register_callback(&btm_cb);
    }

    ret = bk_bt_a2dp_register_callback(a2dp_service_cb);
    if (ret != BK_OK)
    {
        LOGE("%s bk_bt_a2dp_register_callback err %d\n", __func__, ret);
        goto fail;
    }

    ret = bk_bt_a2dp_sink_init(cfg ? cfg->aac_supported : 0);
    if (ret != BK_OK)
    {
        LOGE("%s a2dp sink init err %d\n", __func__, ret);
        goto fail;
    }

    ret = rtos_get_semaphore(&s_a2dp.api_sync_sema, 6000);
    if (ret != BK_OK)
    {
        LOGE("%s get sem for a2dp sink init err\n", __func__);
        goto fail;
    }

    LOGI("%s set cap codec %d bit_pool_max %d\n",
         __func__,
         cap.type,
         cap.param.sbc_codec_cap.bit_pool_max);
    ret = bk_bt_a2dp_set_cap(1, &cap);
    if (ret != BK_OK)
    {
        LOGE("%s bk_bt_a2dp_set_cap err %d\n", __func__, ret);
        goto fail;
    }

    ret = rtos_get_semaphore(&s_a2dp.api_sync_sema, 6000);
    if (ret != BK_OK)
    {
        LOGE("%s get sem for bk_bt_a2dp_set_cap err\n", __func__);
        goto fail;
    }

    ret = bk_bt_a2dp_sink_register_data_callback(a2dp_service_media_cb);
    if (ret != BK_OK)
    {
        LOGE("%s bk_bt_a2dp_sink_register_data_callback err %d\n", __func__, ret);
        goto fail;
    }

    s_a2dp.inited = 1;
    LOGI("%s end\n", __func__);
    return BK_OK;

fail:
    LOGE("%s failed %d\n", __func__, ret);
    bk_a2dp_sink_service_deinit();
    return ret ? ret : BK_FAIL;
}

int bk_a2dp_sink_service_deinit(void)
{
    LOGI("%s\n", __func__);

    if (!s_a2dp.inited && !s_a2dp.api_sync_sema)
    {
        LOGE("%s already deinit\n", __func__);
        return BK_OK;
    }

    if (s_a2dp.a2dp_connected)
    {
        if (!s_a2dp.disconnect_sema)
        {
            if (rtos_init_semaphore(&s_a2dp.disconnect_sema, 1) != BK_OK)
            {
                LOGE("%s disconnect sema init failed\n", __func__);
            }
        }

        bk_bt_a2dp_sink_disconnect(bt_manager_get_connected_device());
        if (s_a2dp.disconnect_sema)
        {
            rtos_get_semaphore(&s_a2dp.disconnect_sema, 5000);
        }
    }

    bk_bt_a2dp_sink_register_data_callback(NULL);
    bk_bt_a2dp_register_callback(NULL);
    bk_bt_a2dp_sink_deinit();

    if (s_a2dp.bt_manager_index != 0xFF)
    {
        bt_manager_unregister_callback(s_a2dp.bt_manager_index);
        s_a2dp.bt_manager_index = 0xFF;
    }

    if (s_a2dp.api_sync_sema)
    {
        rtos_deinit_semaphore(&s_a2dp.api_sync_sema);
        s_a2dp.api_sync_sema = NULL;
    }

    if (s_a2dp.disconnect_sema)
    {
        rtos_deinit_semaphore(&s_a2dp.disconnect_sema);
        s_a2dp.disconnect_sema = NULL;
    }

    os_memset(&s_a2dp, 0, sizeof(s_a2dp));
    s_a2dp.bt_manager_index = 0xFF;
    LOGI("%s end\n", __func__);
    return BK_OK;
}
