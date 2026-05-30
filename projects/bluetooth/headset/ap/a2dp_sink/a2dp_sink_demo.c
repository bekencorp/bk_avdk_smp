#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "components/log.h"
#include "a2dp_sink_audio.h"
#include "headset_user_config.h"
#include "bk_a2dp_sink_service.h"
#include "bk_avrcp_ct_service.h"
#include "bk_avrcp_tg_service.h"
#include "bluetooth_storage.h"
#include "bt_manager.h"
#include "a2dp_sink_demo.h"

#define TAG "a2dp_sink_demo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

enum
{
    BT_AUDIO_MSG_NULL = 0,
    BT_AUDIO_A2DP_START_MSG,
    BT_AUDIO_A2DP_STOP_MSG,
    BT_AUDIO_A2DP_DATA_IND_MSG,
    BT_AUDIO_USER_START_MSG,
    BT_AUDIO_VOLUME_UPDATE_MSG,
    BT_AUDIO_AVRCP_PLAY_STATUS_CHANGED_MSG,
    BT_AUDIO_AVRCP_ELEM_ATTR_RSP_MSG,
    BT_AUDIO_EXIT_MSG,
};

static beken_queue_t s_a2dp_sink_msg_queue = NULL;
static beken_thread_t s_a2dp_sink_thread = NULL;
static beken_semaphore_t s_audio_player_en_sema = NULL;
static beken_semaphore_t s_a2dp_connect_sema = NULL;

static uint8_t s_user_spk_enable = 1;
static uint8_t s_mix_multi_channel = 1;
static uint8_t s_a2dp_sink_inited = 0;
static uint8_t s_a2dp_connected = 0;
static uint8_t s_bt_manager_index = 0xFF;

static const char *a2dp_sink_play_status_to_str(uint8_t play_status);

static int a2dp_sink_queue_push(uint8_t type, const void *data, uint16_t len, uint32_t timeout_ms)
{
    bt_audio_sink_msg_t msg = {0};
    int rc;

    if (!s_a2dp_sink_msg_queue)
    {
        LOGE("%s queue not ready\n", __func__);
        return BK_FAIL;
    }

    if (len)
    {
        if (!data)
        {
            LOGE("%s data is NULL, type %d, len %u\n", __func__, type, len);
            return BK_FAIL;
        }

        msg.data = psram_malloc(len);
        if (!msg.data)
        {
            LOGE("%s malloc failed\n", __func__);
            return BK_FAIL;
        }
        os_memcpy(msg.data, data, len);
        msg.len = len;
    }

    msg.type = type;
    rc = rtos_push_to_queue(&s_a2dp_sink_msg_queue, &msg, timeout_ms);
    if (rc != BK_OK)
    {
        if (msg.data)
        {
            psram_free(msg.data);
        }
        LOGE("%s, send queue failed, type %d, len %u, ret %d\n", __func__, type, len, rc);
        return rc;
    }

    return BK_OK;
}

static void a2dp_sink_avrcp_elem_attr_msg_free(a2dp_sink_avrcp_elem_attr_msg_t *msg)
{
    if (!msg)
    {
        return;
    }

    for (uint32_t i = 0; i < msg->attr_count; i++)
    {
        if (msg->attr_array[i].attr_text)
        {
            psram_free(msg->attr_array[i].attr_text);
            msg->attr_array[i].attr_text = NULL;
        }
    }

    psram_free(msg);
}

static void a2dp_sink_msg_release(bt_audio_sink_msg_t *msg)
{
    if (!msg || !msg->data)
    {
        return;
    }

    if (msg->type == BT_AUDIO_AVRCP_ELEM_ATTR_RSP_MSG)
    {
        a2dp_sink_avrcp_elem_attr_msg_free((a2dp_sink_avrcp_elem_attr_msg_t *)msg->data);
    }
    else
    {
        psram_free(msg->data);
    }

    msg->data = NULL;
}

static int a2dp_sink_queue_push_avrcp_elem_attr_rsp(const bk_avrcp_ct_cb_param_t *param)
{
    const struct avrcp_ct_elem_attr_rsp_param *rsp = &param->elem_attr_rsp;
    uint32_t attr_count = rsp->attr_count;
    uint32_t msg_len = sizeof(a2dp_sink_avrcp_elem_attr_msg_t) +
                       attr_count * sizeof(a2dp_sink_avrcp_attr_t);
    a2dp_sink_avrcp_elem_attr_msg_t *attr_msg = NULL;
    bt_audio_sink_msg_t queue_msg = {0};
    int rc;

    if (!s_a2dp_sink_msg_queue)
    {
        LOGE("%s queue not ready\n", __func__);
        return BK_FAIL;
    }

    if (attr_count && !rsp->attr_array)
    {
        LOGE("%s attr array is NULL\n", __func__);
        return BK_FAIL;
    }

    if (msg_len > UINT16_MAX)
    {
        LOGE("%s attr msg too large %u\n", __func__, msg_len);
        return BK_FAIL;
    }

    attr_msg = psram_malloc(msg_len);
    if (!attr_msg)
    {
        LOGE("%s malloc attr msg failed\n", __func__);
        return BK_FAIL;
    }

    os_memset(attr_msg, 0, msg_len);
    attr_msg->status = rsp->status;
    attr_msg->attr_count = rsp->attr_count;
    os_memcpy(attr_msg->remote_bda, rsp->remote_bda, sizeof(attr_msg->remote_bda));

    for (uint32_t i = 0; i < attr_count; i++)
    {
        uint32_t attr_len = rsp->attr_array[i].attr_length;

        attr_msg->attr_array[i].attr_id = rsp->attr_array[i].attr_id;
        attr_msg->attr_array[i].attr_text_charset = rsp->attr_array[i].attr_text_charset;
        attr_msg->attr_array[i].attr_length = attr_len;

        if (attr_len && rsp->attr_array[i].attr_text)
        {
            attr_msg->attr_array[i].attr_text = psram_malloc(attr_len);
            if (!attr_msg->attr_array[i].attr_text)
            {
                LOGE("%s malloc attr text failed\n", __func__);
                a2dp_sink_avrcp_elem_attr_msg_free(attr_msg);
                return BK_FAIL;
            }
            os_memcpy(attr_msg->attr_array[i].attr_text, rsp->attr_array[i].attr_text, attr_len);
        }
    }

    queue_msg.type = BT_AUDIO_AVRCP_ELEM_ATTR_RSP_MSG;
    queue_msg.len = (uint16_t)msg_len;
    queue_msg.data = (uint8_t *)attr_msg;
    rc = rtos_push_to_queue(&s_a2dp_sink_msg_queue, &queue_msg, BEKEN_NO_WAIT);
    if (rc != BK_OK)
    {
        LOGE("%s, send queue failed\n", __func__);
        a2dp_sink_avrcp_elem_attr_msg_free(attr_msg);
    }

    return rc;
}

static void a2dp_sink_gap_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
        if (param)
        {
            LOGI("%s save default volume for new linkkey %02x:%02x:%02x:%02x:%02x:%02x\n",
                 __func__,
                 param->link_key_notif.bda[5],
                 param->link_key_notif.bda[4],
                 param->link_key_notif.bda[3],
                 param->link_key_notif.bda[2],
                 param->link_key_notif.bda[1],
                 param->link_key_notif.bda[0]);
            bluetooth_storage_save_volume(param->link_key_notif.bda, DEFAULT_A2DP_VOLUME);
        }
        break;

    default:
        break;
    }
}

static void a2dp_sink_bt_manager_callback_register(void)
{
    if (s_bt_manager_index == 0xFF)
    {
        btm_callback_s btm_cb = {
            .gap_cb = a2dp_sink_gap_event_cb,
        };
        s_bt_manager_index = bt_manager_register_callback(&btm_cb);
    }
}

static void a2dp_sink_bt_manager_callback_unregister(void)
{
    if (s_bt_manager_index != 0xFF)
    {
        bt_manager_unregister_callback(s_bt_manager_index);
        s_bt_manager_index = 0xFF;
    }
}

static uint8_t a2dp_sink_get_local_volume(void)
{
    return bk_avrcp_tg_get_local_volume_value();
}

static void a2dp_sink_demo_task(void *arg)
{
    uint32_t spk_task_start_vote = (1 << HEADSET_AUDIO_OPEN_VOTE_USER);
    (void)arg;

    while (1)
    {
        bt_audio_sink_msg_t msg = {0};
        if (rtos_pop_from_queue(&s_a2dp_sink_msg_queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK)
        {
            continue;
        }

        switch (msg.type)
        {
        case BT_AUDIO_A2DP_START_MSG:
            if (msg.data && msg.len == sizeof(bk_a2dp_mcc_t))
            {
                spk_task_start_vote |= (1 << HEADSET_AUDIO_OPEN_VOTE_A2DP);
                LOGI("BT_AUDIO_A2DP_START_MSG\n");

                if (a2dp_sink_audio_start((bk_a2dp_mcc_t *)msg.data,
                                          spk_task_start_vote,
                                          s_mix_multi_channel,
                                          a2dp_sink_get_local_volume()) != BK_OK)
                {
                    LOGE("audio player open failed\n");
                }
            }
            break;

        case BT_AUDIO_A2DP_STOP_MSG:
            LOGI("BT_AUDIO_A2DP_STOP_MSG\n");
            spk_task_start_vote &= ~(1 << HEADSET_AUDIO_OPEN_VOTE_A2DP);
            a2dp_sink_audio_stop();
            break;

        case BT_AUDIO_A2DP_DATA_IND_MSG:
            // LOGI("BT_AUDIO_A2DP_DATA_IND_MSG dl: %d\n", msg.len);
            a2dp_sink_audio_handle_data(msg.data, msg.len);
            break;

        case BT_AUDIO_USER_START_MSG:
            if (msg.data && msg.len == sizeof(uint8_t))
            {
                uint8_t enable = *msg.data;
                LOGI("BT_AUDIO_USER_START_MSG %d\n", enable);
                if (enable)
                {
                    spk_task_start_vote |= (1 << HEADSET_AUDIO_OPEN_VOTE_USER);
                    a2dp_sink_audio_open(spk_task_start_vote,
                                         s_mix_multi_channel,
                                         a2dp_sink_get_local_volume());
                }
                else
                {
                    spk_task_start_vote &= ~(1 << HEADSET_AUDIO_OPEN_VOTE_USER);
                    a2dp_sink_audio_stop();
                }
            }
            if (s_audio_player_en_sema)
            {
                rtos_set_semaphore(&s_audio_player_en_sema);
            }
            break;

        case BT_AUDIO_VOLUME_UPDATE_MSG:
            if (msg.data && msg.len == sizeof(uint8_t))
            {
                LOGI("BT_AUDIO_VOLUME_UPDATE_MSG vol: %d\n", *(uint8_t *)msg.data);
                a2dp_sink_audio_set_gain(*(uint8_t *)msg.data);
            }
            break;

        case BT_AUDIO_AVRCP_PLAY_STATUS_CHANGED_MSG:
            if (msg.data && msg.len == sizeof(uint8_t))
            {
                uint8_t play_status = *(uint8_t *)msg.data;
                LOGI("AVRCP CT play status changed: %s(0x%x)\n",
                     a2dp_sink_play_status_to_str(play_status),
                     play_status);
            }
            break;

        case BT_AUDIO_AVRCP_ELEM_ATTR_RSP_MSG:
            if (msg.data && msg.len >= sizeof(a2dp_sink_avrcp_elem_attr_msg_t))
            {
                a2dp_sink_avrcp_elem_attr_msg_t *rsp = (a2dp_sink_avrcp_elem_attr_msg_t *)msg.data;
                LOGI("AVRCP CT elem attr rsp status %d count %d %02x:%02x:%02x:%02x:%02x:%02x\n",
                     rsp->status,
                     rsp->attr_count,
                     rsp->remote_bda[5],
                     rsp->remote_bda[4],
                     rsp->remote_bda[3],
                     rsp->remote_bda[2],
                     rsp->remote_bda[1],
                     rsp->remote_bda[0]);

                for (uint32_t i = 0; i < rsp->attr_count; ++i)
                {
                    LOGI("AVRCP CT elem attr 0x%x charset 0x%x len %u\n",
                         rsp->attr_array[i].attr_id,
                         rsp->attr_array[i].attr_text_charset,
                         rsp->attr_array[i].attr_length);
                    if (rsp->attr_array[i].attr_text && rsp->attr_array[i].attr_length)
                    {
                        LOGI("AVRCP CT elem attr text: %.*s\n",
                             (int)rsp->attr_array[i].attr_length,
                             (char *)rsp->attr_array[i].attr_text);
                    }
                }

                a2dp_sink_avrcp_elem_attr_msg_free(rsp);
                msg.data = NULL;
            }
            break;

        case BT_AUDIO_EXIT_MSG:
            LOGI("BT_AUDIO_EXIT_MSG\n");
            a2dp_sink_msg_release(&msg);
            spk_task_start_vote &= ~(1 << HEADSET_AUDIO_OPEN_VOTE_A2DP);
            a2dp_sink_audio_stop();
            rtos_delete_thread(NULL);
            return;

        default:
            break;
        }

        a2dp_sink_msg_release(&msg);
    }
}

static int a2dp_sink_task_init(void)
{
    bk_err_t ret;

    LOGI("%s\n", __func__);

    if (s_a2dp_sink_thread || s_a2dp_sink_msg_queue)
    {
        return BK_OK;
    }

    ret = rtos_init_queue(&s_a2dp_sink_msg_queue,
                          "headset_demo_q",
                          sizeof(bt_audio_sink_msg_t),
                          BT_AUDIO_SINK_DEMO_MSG_COUNT);
    if (ret != BK_OK)
    {
        LOGE("bt_audio sink demo msg queue failed\n");
        return ret;
    }

    ret = rtos_create_thread(&s_a2dp_sink_thread,
                             A2DP_SINK_DEMO_TASK_PRIORITY,
                             "headset_demo",
                             (beken_thread_function_t)a2dp_sink_demo_task,
                             4096,
                             0);
    if (ret != BK_OK)
    {
        LOGE("bt_audio sink demo task fail\n");
        rtos_deinit_queue(&s_a2dp_sink_msg_queue);
        s_a2dp_sink_msg_queue = NULL;
        s_a2dp_sink_thread = NULL;
    }

    LOGI("%s end\n", __func__);
    return ret;
}

static void a2dp_sink_task_deinit(void)
{
    if (s_a2dp_sink_thread)
    {
        LOGI("%s wait demo task end\n", __func__);
        a2dp_sink_queue_push(BT_AUDIO_EXIT_MSG, NULL, 0, BEKEN_WAIT_FOREVER);
        rtos_thread_join(&s_a2dp_sink_thread);
        s_a2dp_sink_thread = NULL;
        LOGI("%s demo task end !!!\n", __func__);
    }

    if (s_a2dp_sink_msg_queue)
    {
        bt_audio_sink_msg_t msg = {0};
        while (rtos_pop_from_queue(&s_a2dp_sink_msg_queue, &msg, 0) == BK_OK)
        {
            a2dp_sink_msg_release(&msg);
        }
        rtos_deinit_queue(&s_a2dp_sink_msg_queue);
        s_a2dp_sink_msg_queue = NULL;
    }
}

static void on_a2dp_evt(bk_a2dp_sink_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    // LOGI("%s event: %d\n", __func__, evt);

    switch (evt)
    {
    case BK_A2DP_SINK_EVT_CONNECTED:
        s_a2dp_connected = 1;
        break;
    case BK_A2DP_SINK_EVT_STREAM_START:
        a2dp_sink_queue_push(BT_AUDIO_A2DP_START_MSG, arg, sizeof(bk_a2dp_mcc_t), BEKEN_NO_WAIT);
        break;
    case BK_A2DP_SINK_EVT_STREAM_SUSPEND:
    case BK_A2DP_SINK_EVT_DISCONNECTED:
        if (evt == BK_A2DP_SINK_EVT_DISCONNECTED)
        {
            LOGI("A2DP disconnected\n");
            s_a2dp_connected = 0;
            if (s_a2dp_connect_sema)
            {
                rtos_set_semaphore(&s_a2dp_connect_sema);
            }
        }
        a2dp_sink_queue_push(BT_AUDIO_A2DP_STOP_MSG, NULL, 0, BEKEN_NO_WAIT);
        break;
    case BK_A2DP_SINK_EVT_MEDIA_DATA:
    {
        const bk_a2dp_media_data_t *media = arg;
        if (media && media->data && media->len)
        {
            a2dp_sink_queue_push(BT_AUDIO_A2DP_DATA_IND_MSG, media->data, media->len, 2);
        }
        break;
    }
    case BK_A2DP_SINK_EVT_AUDIO_CFG:
        if (arg)
        {
            a2dp_sink_audio_set_config((bk_a2dp_mcc_t *)arg);
        }
        break;
    default:
        break;
    }
}

static void on_avrcp_tg_evt(bk_avrcp_tg_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    LOGI("%s event: %d\n", __func__, evt);

    switch (evt)
    {
    case BK_AVRCP_TG_EVT_VOLUME_CHANGED:
        a2dp_sink_queue_push(BT_AUDIO_VOLUME_UPDATE_MSG, arg, sizeof(uint8_t), BEKEN_NO_WAIT);
        break;
    default:
        break;
    }
}

static const char *a2dp_sink_play_status_to_str(uint8_t play_status)
{
    switch (play_status)
    {
    case BK_AVRCP_PLAYBACK_STOPPED:
        return "stopped";
    case BK_AVRCP_PLAYBACK_PLAYING:
        return "playing";
    case BK_AVRCP_PLAYBACK_PAUSED:
        return "paused";
    case BK_AVRCP_PLAYBACK_FWD_SEEK:
        return "forward seek";
    case BK_AVRCP_PLAYBACK_REV_SEEK:
        return "reverse seek";
    case BK_AVRCP_PLAYBACK_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static void on_avrcp_ct_evt(bk_avrcp_ct_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    LOGI("%s event: %d\n", __func__, evt);

    switch (evt)
    {
    case BK_AVRCP_CT_EVT_PLAY_STATUS_CHANGED:
    {
        uint8_t play_status = arg ? *(uint8_t *)arg : BK_AVRCP_PLAYBACK_ERROR;
        a2dp_sink_queue_push(BT_AUDIO_AVRCP_PLAY_STATUS_CHANGED_MSG,
                             &play_status,
                             sizeof(play_status),
                             BEKEN_NO_WAIT);
        break;
    }

    case BK_AVRCP_CT_EVT_TRACK_CHANGED:
    {
        uint64_t track_id = 0;
        if (arg)
        {
            track_id = *(uint64_t *)arg;
        }
        LOGI("AVRCP CT track changed: %llu\n", (unsigned long long)track_id);
        bk_avrcp_ct_get_attr(BK_AVRCP_MEDIA_ATTR_ID_TITLE);
        break;
    }

    case BK_AVRCP_CT_EVT_ELEM_ATTR_RSP:
    {
        bk_avrcp_ct_cb_param_t *param = (bk_avrcp_ct_cb_param_t *)arg;
        if (!param)
        {
            break;
        }

        a2dp_sink_queue_push_avrcp_elem_attr_rsp(param);
        break;
    }

    default:
        LOGW("Unhandled AVRCP CT event: %d\n", evt);
        break;
    }
}

int a2dp_sink_demo_init(uint8_t aac_supported, uint8_t auto_accept_conn)
{
    bk_a2dp_sink_cfg_t sink_cfg;
    bk_avrcp_ct_cfg_t avrcp_ct_cfg;
    bk_avrcp_tg_cfg_t avrcp_tg_cfg;

    LOGI("%s\n", __func__);

    if (aac_supported)
    {
#if (!CONFIG_ADK_AAC_DECODER)
        LOGE("%s AAC is not supported!\n", __func__);
        return BK_FAIL;
#endif
    }

    if (s_a2dp_sink_inited)
    {
        LOGE("%s already init\n", __func__);
        return BK_OK;
    }

    if (!s_audio_player_en_sema)
    {
        if (rtos_init_semaphore(&s_audio_player_en_sema, 1) != BK_OK)
        {
            LOGE("%s audio player semaphore init err\n", __func__);
            return BK_FAIL;
        }
    }

    if (a2dp_sink_task_init() != BK_OK)
    {
        LOGE("%s a2dp_sink_task_init err\n", __func__);
        return BK_FAIL;
    }

    bk_a2dp_sink_register_event_cb(on_a2dp_evt, NULL);
    bk_avrcp_ct_register_event_cb(on_avrcp_ct_evt, NULL);
    bk_avrcp_tg_register_event_cb(on_avrcp_tg_evt, NULL);
    a2dp_sink_bt_manager_callback_register();

    sink_cfg.aac_supported = aac_supported;
    sink_cfg.auto_accept_conn = auto_accept_conn;
    if (bk_a2dp_sink_service_init(&sink_cfg) != BK_OK)
    {
        LOGE("%s bk_a2dp_sink_service_init err\n", __func__);
        a2dp_sink_bt_manager_callback_unregister();
        a2dp_sink_task_deinit();
        return BK_FAIL;
    }

    avrcp_ct_cfg.auto_ct_connect_after_a2dp = 1;
    if (bk_avrcp_ct_service_init(&avrcp_ct_cfg) != BK_OK)
    {
        LOGE("%s bk_avrcp_ct_service_init err\n", __func__);
        bk_a2dp_sink_service_deinit();
        a2dp_sink_bt_manager_callback_unregister();
        a2dp_sink_task_deinit();
        return BK_FAIL;
    }

    avrcp_tg_cfg.default_volume = DEFAULT_A2DP_VOLUME;
    if (bk_avrcp_tg_service_init(&avrcp_tg_cfg) != BK_OK)
    {
        LOGE("%s bk_avrcp_tg_service_init err\n", __func__);
        bk_avrcp_ct_service_deinit();
        bk_a2dp_sink_service_deinit();
        a2dp_sink_bt_manager_callback_unregister();
        a2dp_sink_task_deinit();
        return BK_FAIL;
    }
    bk_avrcp_tg_emit_current_volume();

    LOGI("%s initial volume %d\n", __func__, a2dp_sink_get_local_volume());
    s_a2dp_sink_inited = 1;
    LOGI("%s end\n", __func__);
    return BK_OK;
}

int a2dp_sink_demo_deinit(void)
{
    LOGI("%s\n", __func__);

    if (!s_a2dp_sink_inited)
    {
        LOGE("%s already deinit\n", __func__);
        return BK_OK;
    }

    bk_avrcp_tg_service_deinit();
    bk_avrcp_ct_service_deinit();
    bk_a2dp_sink_service_deinit();
    a2dp_sink_bt_manager_callback_unregister();
    a2dp_sink_task_deinit();

    if (s_audio_player_en_sema)
    {
        rtos_deinit_semaphore(&s_audio_player_en_sema);
        s_audio_player_en_sema = NULL;
    }

    if (s_a2dp_connect_sema)
    {
        rtos_deinit_semaphore(&s_a2dp_connect_sema);
        s_a2dp_connect_sema = NULL;
    }

    s_a2dp_connected = 0;
    s_a2dp_sink_inited = 0;
    LOGI("%s end\n", __func__);
    return BK_OK;
}

int32_t a2dp_sink_demo_wait_player_end(void)
{
    return a2dp_sink_audio_wait_player_end();
}

void a2dp_sink_demo_audio_spk_enable(uint8_t enable)
{
    LOGI("%s %d\n", __func__, enable);
    s_user_spk_enable = enable;
    if (a2dp_sink_queue_push(BT_AUDIO_USER_START_MSG, &enable, sizeof(enable), BEKEN_NO_WAIT) == BK_OK &&
        s_audio_player_en_sema)
    {
        rtos_get_semaphore(&s_audio_player_en_sema, BEKEN_WAIT_FOREVER);
    }
    LOGI("%s end\n", __func__);
}

void a2dp_sink_demo_set_mix(uint8_t enable)
{
    LOGI("%s %d\n", __func__, enable);
    s_mix_multi_channel = enable;
}

int32_t a2dp_sink_demo_try_connect(void)
{
    int32_t ret = BK_OK;

    if (!s_a2dp_sink_inited)
    {
        LOGE("%s already deinit\n", __func__);
        return BK_FAIL;
    }

    LOGW("%s current bt manager status %d %d\n",
         __func__, bt_manager_get_connect_state(), s_a2dp_connected);

    if ((bt_manager_get_connect_state() == BT_STATE_LINK_CONNECTED ||
         bt_manager_get_connect_state() == BT_STATE_PROFILE_CONNECTED) &&
        !s_a2dp_connected)
    {
        const uint8_t *device = bt_manager_get_connected_device();
        if (device)
        {
            LOGW("%s start connect a2dp profile %02x:%02x:%02x:%02x:%02x:%02x\n",
                 __func__,
                 device[5],
                 device[4],
                 device[3],
                 device[2],
                 device[1],
                 device[0]);
        }
        ret = bk_a2dp_sink_connect(bt_manager_get_connected_device());
        if (ret != BK_OK)
        {
            LOGE("%s bk_a2dp_sink_connect err %d\n", __func__, ret);
        }
    }

    return ret;
}

int32_t a2dp_sink_demo_try_disconnect_current(void)
{
    int32_t ret = BK_OK;

    if (s_a2dp_connected)
    {
        if (!s_a2dp_connect_sema)
        {
            if (rtos_init_semaphore(&s_a2dp_connect_sema, 1) != BK_OK)
            {
                LOGE("%s init connect sema fail\n", __func__);
                return BK_FAIL;
            }
        }

        LOGW("%s disconnecting a2dp\n", __func__);
        ret = bk_a2dp_sink_disconnect(bt_manager_get_connected_device());
        if (ret != BK_OK)
        {
            LOGE("%s bk_a2dp_sink_disconnect err %d\n", __func__, ret);
        }
        else
        {
            LOGW("%s wait disconnect a2dp sem\n", __func__);
            ret = rtos_get_semaphore(&s_a2dp_connect_sema, 5000);
            if (ret != BK_OK)
            {
                LOGE("%s wait disconnect a2dp sem err %d\n", __func__, ret);
            }
            else
            {
                LOGW("%s wait disconnect a2dp success\n", __func__);
            }
        }
    }

    if (s_a2dp_connect_sema)
    {
        if (rtos_deinit_semaphore(&s_a2dp_connect_sema) != BK_OK)
        {
            LOGE("%s deinit connect sema fail\n", __func__);
        }
        s_a2dp_connect_sema = NULL;
    }

    LOGW("%s end\n", __func__);
    return ret;
}
