#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_bt_types.h"
#include "components/bluetooth/bk_dm_bt.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "bluetooth_storage.h"
#include "bt_manager.h"
#if CONFIG_WIFI_COEX_SCHEME
#include "bk_coex_ext.h"
#endif

#define TAG "btm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define BT_MANAGER_DEFAULT_LOCAL_NAME       "soundbar"
#define BT_MANAGER_DEFAULT_DEVICE_CLASS     COD_SOUNDBAR
#define BT_MANAGER_DEFAULT_PAGE_SCAN_INTV   0x0800
#define BT_MANAGER_DEFAULT_PAGE_SCAN_WIN    0x00B4
#define BT_MANAGER_DEFAULT_PAGE_TIMEOUT     16000
#define BT_MANAGER_DEFAULT_RECONN_INTERVAL  2000
#define BT_MANAGER_DEFAULT_MAX_RECONN_COUNT 3
#define BT_MANAGER_IO_CAP_DEFAULT_MARKER    0xFF
#define MAX_PROFILE_NUM 10

typedef struct
{
    uint8_t inited;
    uint8_t mode;
    uint8_t connect_state;
    uint8_t manual_enter_pairing;
    uint8_t wifi_state;
    char local_name[20];
    uint32_t device_class;
    uint16_t page_scan_interval;
    uint16_t page_scan_window;
    uint16_t page_timeout;
    uint32_t reconnect_interval_ms;
    uint8_t max_reconnect_count;
    uint8_t io_capability;
    beken2_timer_t recon_tmr;
    uint8_t recon_count;
    uint8_t peer_addr[6];
    uint8_t recon_addr[6];
    uint8_t tmp_link_key[16];//BT_LINK_KEY_SIZE];
#if CONFIG_WIFI_COEX_SCHEME
    beken_queue_t msg_queue;
    beken_thread_t thread;
    coex_to_bt_func_p_t coex_cb;
#endif
} btm_env_s;

static btm_env_s btm_env={0};
static btm_callback_s btm_cbs[MAX_PROFILE_NUM] = {0};
static uint8_t s_bt_auto_accept_connection;

static void bt_manager_load_config(const bt_manager_cfg_t *cfg)
{
    const char *local_name = BT_MANAGER_DEFAULT_LOCAL_NAME;

    if (cfg && cfg->local_name)
    {
        local_name = cfg->local_name;
    }

    os_strncpy(btm_env.local_name,
               local_name,
               sizeof(btm_env.local_name) - 1);
    btm_env.local_name[sizeof(btm_env.local_name) - 1] = '\0';
    btm_env.device_class = (cfg && cfg->device_class) ?
                           cfg->device_class :
                           BT_MANAGER_DEFAULT_DEVICE_CLASS;
    btm_env.page_scan_interval = (cfg && cfg->page_scan_interval) ?
                                 cfg->page_scan_interval :
                                 BT_MANAGER_DEFAULT_PAGE_SCAN_INTV;
    btm_env.page_scan_window = (cfg && cfg->page_scan_window) ?
                               cfg->page_scan_window :
                               BT_MANAGER_DEFAULT_PAGE_SCAN_WIN;
    btm_env.page_timeout = (cfg && cfg->page_timeout) ?
                           cfg->page_timeout :
                           BT_MANAGER_DEFAULT_PAGE_TIMEOUT;
    btm_env.reconnect_interval_ms = (cfg && cfg->reconnect_interval_ms) ?
                                    cfg->reconnect_interval_ms :
                                    BT_MANAGER_DEFAULT_RECONN_INTERVAL;
    btm_env.max_reconnect_count = (cfg && cfg->max_reconnect_count) ?
                                  cfg->max_reconnect_count :
                                  BT_MANAGER_DEFAULT_MAX_RECONN_COUNT;
    btm_env.io_capability = (cfg && cfg->io_capability != BT_MANAGER_IO_CAP_DEFAULT_MARKER) ?
                            cfg->io_capability :
                            BK_BT_IO_CAP_NONE;
}

#if CONFIG_WIFI_COEX_SCHEME
enum
{
    BT_MNG_MSG_WIFI_STATE_UPDATE = 1,
    BT_MNG_MSG_EXIT,
};

typedef struct
{
    uint8_t type;
} bt_manager_msg_t;
#endif

static void bt_manager_handle_wifi_state_update(void)
{
    if ((btm_env.wifi_state == 0) && (btm_env.connect_state == BT_STATE_WAIT_FOR_RECONNECT))
    {
        bt_manager_start_reconnect(btm_env.recon_addr, 1);
    }

    if (btm_env.wifi_state && (btm_env.connect_state == BT_STATE_RECONNECTING))
    {
        bk_bt_gap_create_conn_cancel(btm_env.recon_addr);
    }
}

#if CONFIG_WIFI_COEX_SCHEME
static int bt_manager_queue_push(uint8_t type, uint32_t timeout_ms)
{
    bt_manager_msg_t msg = {0};

    if (!btm_env.msg_queue)
    {
        return BK_FAIL;
    }

    msg.type = type;
    return rtos_push_to_queue(&btm_env.msg_queue, &msg, timeout_ms);
}

static void bt_manager_task(void *arg)
{
    (void)arg;

    while (1)
    {
        bt_manager_msg_t msg = {0};

        if (rtos_pop_from_queue(&btm_env.msg_queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK)
        {
            continue;
        }

        switch (msg.type)
        {
        case BT_MNG_MSG_WIFI_STATE_UPDATE:
            bt_manager_handle_wifi_state_update();
            break;

        case BT_MNG_MSG_EXIT:
            rtos_delete_thread(NULL);
            return;

        default:
            break;
        }
    }
}

static int bt_manager_task_init(void)
{
    bk_err_t ret = BK_OK;

    if (btm_env.thread || btm_env.msg_queue)
    {
        return kInProgressErr;
    }

    ret = rtos_init_queue(&btm_env.msg_queue,
                          "bt_manager_msg_q",
                          sizeof(bt_manager_msg_t),
                          8);
    if (ret != BK_OK)
    {
        LOGE("%s init queue err %d\n", __func__, ret);
        return ret;
    }

    ret = rtos_create_thread(&btm_env.thread,
                             A2DP_SINK_DEMO_TASK_PRIORITY,
                             "bt_manager",
                             (beken_thread_function_t)bt_manager_task,
                             2048,
                             0);
    if (ret != BK_OK)
    {
        LOGE("%s create thread err %d\n", __func__, ret);
        rtos_deinit_queue(&btm_env.msg_queue);
        btm_env.msg_queue = NULL;
        btm_env.thread = NULL;
    }

    return ret;
}

static void bt_manager_task_deinit(void)
{
    if (btm_env.thread)
    {
        (void)bt_manager_queue_push(BT_MNG_MSG_EXIT, BEKEN_NO_WAIT);
        rtos_thread_join(&btm_env.thread);
        btm_env.thread = NULL;
    }

    if (btm_env.msg_queue)
    {
        bt_manager_msg_t msg = {0};
        while (rtos_pop_from_queue(&btm_env.msg_queue, &msg, 0) == BK_OK)
        {
        }
        rtos_deinit_queue(&btm_env.msg_queue);
        btm_env.msg_queue = NULL;
    }
}

static void bt_manager_wifi_state_callback(uint8_t status_id, uint8_t status_info)
{
    if ((status_id == COEX_WIFI_STAT_ID_SCANNING) || (status_id == COEX_WIFI_STAT_ID_CONNECTING))
    {
        if (status_info)
        {
            btm_env.wifi_state |= (1 << status_id);
        }
        else
        {
            btm_env.wifi_state &= ~(1 << status_id);
        }

        if (bt_manager_queue_push(BT_MNG_MSG_WIFI_STATE_UPDATE, BEKEN_NO_WAIT) != BK_OK)
        {
            LOGW("%s push wifi state update failed\n", __func__);
        }
    }
}

static void bt_manager_register_wifi_coex(void)
{
    os_memset(&btm_env.coex_cb, 0, sizeof(btm_env.coex_cb));
    btm_env.coex_cb.version = 0x01;
    btm_env.coex_cb.inform_wifi_status = bt_manager_wifi_state_callback;
    coex_bt_if_init(&btm_env.coex_cb);
}
#endif

static const char *bt_gap_evt_to_str(bk_gap_bt_cb_event_t event)
{
    switch (event)
    {
    case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        return "ACL_DISCONN_CMPL";
    case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        return "ACL_CONN_CMPL";
    case BK_BT_GAP_AUTH_CMPL_EVT:
        return "AUTH_CMPL";
    case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
        return "LINK_KEY_NOTIF";
    case BK_BT_GAP_LINK_KEY_REQ_EVT:
        return "LINK_KEY_REQ";
    case BK_BT_GAP_CONNECTION_REQ_EVT:
        return "CONNECTION_REQ";
    default:
        return "OTHER";
    }
}

void bt_stop_reconnect_timeout_check(void)
{
    if (rtos_is_oneshot_timer_init(&btm_env.recon_tmr))
    {
        if (rtos_is_oneshot_timer_running(&btm_env.recon_tmr))
        {
            rtos_stop_oneshot_timer(&btm_env.recon_tmr);
        }
        rtos_deinit_oneshot_timer(&btm_env.recon_tmr);
    }

    btm_env.recon_count = 0;

    for (uint8_t i = 0; i < MAX_PROFILE_NUM; i++)
    {
        if(btm_cbs[i].stop_connect_cb)
        {
            btm_cbs[i].stop_connect_cb();
        }
    }
}

void bk_bt_enter_pairing_mode(uint8_t is_visible)
{
    bt_stop_reconnect_timeout_check();

    LOGI("%s status %d\n", __func__, btm_env.connect_state);

    if (BT_STATE_RECONNECTING == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = (is_visible ? PAIRING_STATE_WAIT_CFM : PAIRING_STATE_PREPARATION);
        bk_bt_gap_create_conn_cancel(btm_env.recon_addr);
    }
    else if (BT_STATE_LINK_CONNECTED == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = (is_visible ? PAIRING_STATE_WAIT_CFM : PAIRING_STATE_PREPARATION);
        bk_bt_gap_disconnect(btm_env.peer_addr, 0x13);
    }
    else if (BT_STATE_PROFILE_CONNECTED == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = (is_visible ? PAIRING_STATE_WAIT_CFM : PAIRING_STATE_PREPARATION);
        for (int i = 0; i < MAX_PROFILE_NUM; i++)
        {
            if(btm_cbs[i].start_disconnect_cb)
            {
                btm_cbs[i].start_disconnect_cb(btm_env.peer_addr);
            }
        }
    }
    else
    {
        bt_manager_set_mode((is_visible ? BT_MNG_MODE_PAIRING : BT_MNG_MODE_IDLE));
    }

    btm_env.connect_state = BT_STATE_IDLE;
}

static char *bt_manager_mode_2_str(uint8_t mode)
{
    switch(mode)
    {
    case BT_MNG_MODE_PAIRING:
        return "paring-connable-inqable";
    case BT_MNG_MODE_RECONNECTING:
        return "reconnectin-conndisable-inqdisable";
    case BT_MNG_MODE_CONNECTEED:
        return "connected-conndisable-inqdisable";
    case BT_MNG_MODE_CONNECTABLE:
        return "connable-inqdisable";
    case BT_MNG_MODE_IDLE:
        return "idle-conndisable-inqdisable";
    case BT_MNG_MODE_DISCOVERABLE_ONLY:
        return "dis_only-conndisable-inqable";
    case BT_MNG_MODE_ALL_OFF:
        return "off-conndisable-inqdisable";
    }
    return "unknow mode";
}

uint8_t bt_manager_get_mode(void)
{
    return btm_env.mode;
}

void bt_manager_set_mode(uint8_t mode)
{
    LOGI("%s: %d -> %d\n", __func__, btm_env.mode, mode);
    LOGI("-> %s \n", bt_manager_mode_2_str(mode));

    if (btm_env.mode == mode) return;

    switch(mode)
    {
    case BT_MNG_MODE_PAIRING:
        bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        break;
    case BT_MNG_MODE_RECONNECTING:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    case BT_MNG_MODE_CONNECTEED:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    case BT_MNG_MODE_CONNECTABLE:
        bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    case BT_MNG_MODE_IDLE:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    case BT_MNG_MODE_DISCOVERABLE_ONLY:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_DISCOVERABLE);
        break;
    case BT_MNG_MODE_ALL_OFF:
        bk_bt_gap_set_visibility(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        break;
    default:
        break;
    }

    btm_env.mode = mode;
}

void link_timeout_start_reconnect_timer_hdl(void *param, unsigned int ulparam)
{
    LOGI("%s\n", __func__);

    rtos_deinit_oneshot_timer(&btm_env.recon_tmr);

    if (btm_env.wifi_state)
    {
        btm_env.connect_state = BT_STATE_WAIT_FOR_RECONNECT;
        return;
    }

    for(int i=0; i<MAX_PROFILE_NUM; i++)
    {
        if(btm_cbs[i].start_connect_cb)
        {
            LOGI("%s i %d %p\n", __func__, i, btm_cbs[i].start_connect_cb);
            btm_cbs[i].start_connect_cb(btm_env.recon_addr);
            if(btm_env.connect_state == BT_STATE_WAIT_FOR_RECONNECT)
            {
                return;
            }
        }
    }
    btm_env.connect_state = BT_STATE_RECONNECTING;
    btm_env.recon_count++;
}

static void bt_manager_notify_reconnect_fail(void)
{
    uint8_t callback_count = 0;

    for (int i = 0; i < MAX_PROFILE_NUM; i++)
    {
        if (btm_cbs[i].reconnect_fail_cb)
        {
            LOGI("%s i %d %p\n", __func__, i, btm_cbs[i].reconnect_fail_cb);
            btm_cbs[i].reconnect_fail_cb();
            callback_count++;
        }
    }

    if (!callback_count)
    {
        LOGW("%s no reconnect fail callback\n", __func__);
    }
}

void bt_manager_start_reconnect(uint8_t *addr, uint8_t immediate)
{
    uint32_t time_ms = 200;

    if (btm_env.recon_count >= btm_env.max_reconnect_count)
    {
        bt_manager_notify_reconnect_fail();
        return;
    }

    btm_env.connect_state = BT_STATE_IDLE;

    os_memcpy(btm_env.recon_addr, addr, 6);
    bt_manager_set_mode(BT_MNG_MODE_RECONNECTING);

    if (!immediate)
    {
        time_ms = btm_env.reconnect_interval_ms;
    }

    bk_err_t ret = BK_OK;
    if (!rtos_is_oneshot_timer_init(&btm_env.recon_tmr))
    {
        ret = rtos_init_oneshot_timer(&btm_env.recon_tmr, time_ms, (timer_2handler_t)link_timeout_start_reconnect_timer_hdl, NULL, 0);

        if(ret)
        {
            LOGE("%s init oneshot timer err %d\n", __func__, ret);
            return;
        }
    }
    else
    {
        LOGW("%s timer already init\n", __func__);
    }

    if(rtos_is_oneshot_timer_running(&btm_env.recon_tmr))
    {
        LOGW("%s timer already run, stop it\n", __func__);

        ret = rtos_stop_oneshot_timer(&btm_env.recon_tmr);

        if(ret)
        {
            LOGE("%s stop oneshot timer err %d\n", __func__, ret);
        }
    }

    ret = rtos_start_oneshot_timer(&btm_env.recon_tmr);

    if(ret)
    {
        LOGE("%s start oneshot timer err %d\n", __func__, ret);
        return;
    }
}

void bt_manager_clear_reconnect_info(void)
{
    os_memset(btm_env.recon_addr, 0, 6);
    btm_env.connect_state = BT_STATE_IDLE;
    btm_env.recon_count = 0;
}


void gap_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    LOGI("%s evt=%d(%s) state=%d auto_accept=0x%x\n",
         __func__, event, bt_gap_evt_to_str(event), btm_env.connect_state, s_bt_auto_accept_connection);

    switch (event)
    {
        case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        {
            uint8_t *addr = param->acl_disconn_cmpl_stat.bda;
            LOGI("Disconnected from %02x:%02x:%02x:%02x:%02x:%02x, reason 0x%02x connect_state %d\n",
                            addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
                            param->acl_disconn_cmpl_stat.reason,
                            btm_env.connect_state);

            //bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);

            if (btm_env.manual_enter_pairing)
            {
                bt_manager_set_mode(((PAIRING_STATE_PREPARATION == btm_env.manual_enter_pairing) ? BT_MNG_MODE_IDLE : BT_MNG_MODE_PAIRING));
                btm_env.manual_enter_pairing = PAIRING_STATE_IDLE;
                break;
            }

            if ((BK_BT_STATUS_REMOTE_USER_TERM_CON == param->acl_disconn_cmpl_stat.reason || BK_BT_STATUS_CON_TERM_BY_LOCAL_HOST == param->acl_disconn_cmpl_stat.reason)
                && (BT_STATE_PROFILE_CONNECTED == btm_env.connect_state || BT_STATE_KEY_MISSING == btm_env.connect_state))
            {
                os_memset(btm_env.peer_addr, 0, 6);
                bt_manager_clear_reconnect_info();
#if 1
                bluetooth_storage_sync_to_flash();
                LOGI("%s sync to flash done\n", __func__);
#endif
                if (bluetooth_storage_find_linkkey_info_index(addr, NULL) < 0)
                {
                    bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                }
                else
                {
                    bt_manager_set_mode(BT_MNG_MODE_CONNECTABLE);
                }
            }
            else //if (BK_BT_STATUS_CON_TIMEOUT == cb->acl_disconn_cmpl_stat.reason)
            {
                bt_manager_start_reconnect(addr, 1);
            }
        }
        break;

        case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        {
            uint8_t *addr = param->acl_conn_cmpl_stat.bda;
            if (0 == param->acl_conn_cmpl_stat.stat)
            {
                LOGI("Connected to %02x:%02x:%02x:%02x:%02x:%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
                os_memcpy(btm_env.peer_addr, addr, 6);
                btm_env.connect_state = BT_STATE_LINK_CONNECTED;
                bt_manager_set_mode(BT_MNG_MODE_CONNECTEED);
            }
            else
            {
                LOGI("Connect the %02x:%02x:%02x:%02x:%02x:%02x Failed, status 0x%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],param->acl_conn_cmpl_stat.stat);

                if (btm_env.manual_enter_pairing)
                {
                    bt_manager_set_mode(((PAIRING_STATE_PREPARATION == btm_env.manual_enter_pairing) ? BT_MNG_MODE_IDLE : BT_MNG_MODE_PAIRING));
                    btm_env.manual_enter_pairing = PAIRING_STATE_IDLE;
                    break;
                }

                if ((BK_BT_STATUS_PAGE_TIMEOUT == param->acl_conn_cmpl_stat.stat || BK_BT_STATUS_CON_ALREADY_EXISTS == param->acl_conn_cmpl_stat.stat || BK_BT_STATUS_UNKNOWN_CONNECTION_ID == param->acl_conn_cmpl_stat.stat)
                    && (BT_STATE_RECONNECTING == btm_env.connect_state))
                {
                    bt_manager_start_reconnect(addr, 0);
                }
                else
                {
                    bt_manager_clear_reconnect_info();
                    bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                }
            }
        }
        break;

        case BK_BT_GAP_AUTH_CMPL_EVT:
    {
        uint8_t *addr = param->auth_cmpl.bda;
        if (0 == param->auth_cmpl.stat)
        {
            LOGI("(%02x:%02x:%02x:%02x:%02x:%02x)authentication success\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        }
        else
        {
            if (BK_BT_STATUS_PIN_MISSING == param->auth_cmpl.stat || BK_BT_STATUS_AUTH_FAILURE == param->auth_cmpl.stat)
            {
                bluetooth_storage_del_linkkey_info(addr);
                btm_env.connect_state = BT_STATE_KEY_MISSING;
            }
            LOGI("(%02x:%02x:%02x:%02x:%02x:%02x)authentication failed, status: 0x%02x\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],param->auth_cmpl.stat);
        }
    }
    break;

    case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
    {
        LOGI("%s recv linkkey %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                  param->link_key_notif.bda[5],
                  param->link_key_notif.bda[4],
                  param->link_key_notif.bda[3],
                  param->link_key_notif.bda[2],
                  param->link_key_notif.bda[1],
                  param->link_key_notif.bda[0]);

        uint8_t log_buff[16 * 2 + 10] = {0};
        for (int i = 0; i < sizeof(param->link_key_notif.link_key); ++i)
        {
            sprintf((char *)(log_buff + i * 2), "%02X", param->link_key_notif.link_key[i]);
        }
        LOGW("%s %s\n", __func__, log_buff);
        int ret = bluetooth_storage_save_linkkey_info(param->link_key_notif.bda, param->link_key_notif.link_key);
        // s_a2dp_vol = DEFAULT_A2DP_VOLUME;
        // bluetooth_storage_save_volume(param->link_key_notif.bda, s_a2dp_vol);

        if (ret <= 0)
        {
            LOGE("%s save link key fail %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      param->link_key_notif.bda[5],
                      param->link_key_notif.bda[4],
                      param->link_key_notif.bda[3],
                      param->link_key_notif.bda[2],
                      param->link_key_notif.bda[1],
                      param->link_key_notif.bda[0]);
        }
        else
        {
            bluetooth_storage_update_to_newest(param->link_key_notif.bda);
#if 1
            bluetooth_storage_sync_to_flash();
            LOGI("%s sync to flash done\n", __func__);
#endif
        }

    }
    break;

    case BK_BT_GAP_LINK_KEY_REQ_EVT:
    {
        uint8_t *addr = param->link_key_req.bda;
        bk_bt_linkkey_storage_t tmp;
        int ret = 0;
        uint8_t zero_linkkey[16] = {0};
        uint8_t ff_linkkey[16] = {0};
        uint8_t found_key = 0;
        uint8_t log_buff[16 * 2 + 10] = {0};

        memset(&tmp, 0, sizeof(tmp));
        memcpy(tmp.addr, addr, sizeof(tmp.addr));

        os_memset(ff_linkkey, 0xff, sizeof(ff_linkkey));

        ret = bluetooth_storage_find_linkkey_info_index(addr, tmp.link_key);

        if (ret >= 0)
        {
            LOGI("%s found link key %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      addr[5],
                      addr[4],
                      addr[3],
                      addr[2],
                      addr[1],
                      addr[0]);

            for (int i = 0; i < sizeof(tmp.link_key); ++i)
            {
                sprintf((char *)(log_buff + i * 2), "%02X", tmp.link_key[i]);
            }

            LOGW("%s %s\n", __func__, log_buff);

            found_key = 1;
        }
        else if(os_memcmp(btm_env.tmp_link_key, zero_linkkey, sizeof(btm_env.tmp_link_key)) &&
                        os_memcmp(btm_env.tmp_link_key, ff_linkkey, sizeof(btm_env.tmp_link_key)))
        {
            LOGI("%s use tmp linkkey\n", __func__);

            os_memcpy(tmp.link_key, btm_env.tmp_link_key, sizeof(btm_env.tmp_link_key));

            for (int i = 0; i < sizeof(tmp.link_key); ++i)
            {
                sprintf((char *)(log_buff + i * 2), "%02X", tmp.link_key[i]);
            }

            LOGW("%s %s\n", __func__, log_buff);

            found_key = 1;
        }

        if(found_key)
        {
            bk_bt_gap_linkkey_reply(1, &tmp);
        }
        else
        {
            LOGI("%s not found link key in storage %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
                      addr[5],
                      addr[4],
                      addr[3],
                      addr[2],
                      addr[1],
                      addr[0]);

            memset(tmp.link_key, 0, sizeof(tmp.link_key));

            bk_bt_gap_linkkey_reply(0, &tmp);
        }
    }
    break;

    case BK_BT_GAP_CONNECTION_REQ_EVT:
    {
        struct connection_req_param *pm = (typeof(pm))param;
        const char *type_str = NULL;

        switch(pm->type)
        {
        case 0:
            type_str = "sco";
            break;
        case 1:
            type_str = "acl";
            break;
        case 2:
            type_str = "esco";
            break;
        default:
            type_str = "unknow";
            break;
        }

        LOGW("%s BK_BT_GAP_CONNECTION_REQ_EVT %s %02x:%02x:%02x:%02x:%02x:%02x cod 0x%x\n", __func__,
                        type_str,
                        pm->bda[5],
                        pm->bda[4],
                        pm->bda[3],
                        pm->bda[2],
                        pm->bda[1],
                        pm->bda[0],
                        pm->cod.cod);

        switch(pm->type)
        {
        case 1:
            pm->accept = ((s_bt_auto_accept_connection & (1 << BT_MNG_AUTO_ACCEPT_CONNECTION_ACL)) ? 1 : 0);
            pm->reject_reason = BK_BT_STATUS_CONN_REJ_SECURITY_REASONS;
            break;

        case 0:
        case 2:
            pm->accept = ((s_bt_auto_accept_connection & (1 << BT_MNG_AUTO_ACCEPT_CONNECTION_SCO)) ? 1 : 0);
            pm->reject_reason = BK_BT_STATUS_CONN_REJ_SECURITY_REASONS;
            break;

        default:
            pm->accept = 0;
            pm->reject_reason = BK_BT_STATUS_CONN_REJ_SECURITY_REASONS;
            break;
        }

        LOGI("%s conn_req type=%d accept=%d reject_reason=0x%x state=%d\n",
             __func__, pm->type, pm->accept, pm->reject_reason, btm_env.connect_state);
    }
    break;

    default:
        break;
    }

    for(int i=0;i<MAX_PROFILE_NUM;i++)
    {
        if(btm_cbs[i].gap_cb != NULL)
        {
            btm_cbs[i].gap_cb(event, param);
        }
    }
}

void bt_manager_set_auto_accept_connection(uint8_t type, uint8_t accept)
{
    if(accept)
    {
        s_bt_auto_accept_connection |= (1 << type);
    }
    else
    {
        s_bt_auto_accept_connection &= ~(1 << type);
    }

    LOGW("%s 0x%x\n", __func__, s_bt_auto_accept_connection);
}

int bt_manager_init(const bt_manager_cfg_t *cfg)
{
    LOGI("%s\n", __func__);
    int ret = 0;

    if(btm_env.inited)
    {
        LOGE("%s already init\n", __func__);
        return -1;
    }

    ret = bluetooth_storage_init();

    if (ret)
    {
        LOGE("%s bluetooth_storage_init err %d\n", __func__, ret);
        return -1;
    }
    os_memset(&btm_env, 0, sizeof(btm_env_s));
    os_memset(&btm_cbs, 0, sizeof(btm_cbs));
    bt_manager_load_config(cfg);

    s_bt_auto_accept_connection = ((1 << BT_MNG_AUTO_ACCEPT_CONNECTION_ACL) | (1 << BT_MNG_AUTO_ACCEPT_CONNECTION_SCO));

#if CONFIG_WIFI_COEX_SCHEME
    ret = bt_manager_task_init();
    if (ret != BK_OK)
    {
        LOGE("%s bt_manager_task_init err %d\n", __func__, ret);
        bluetooth_storage_deinit();
        return -1;
    }
#endif

    bk_bt_gap_register_callback(gap_event_cb);
    bk_bt_gap_set_device_class(btm_env.device_class);
    LOGI("%s local_name %s\n", __func__, btm_env.local_name);
    bk_bt_gap_set_local_name((uint8_t *)btm_env.local_name, os_strlen(btm_env.local_name));

    bt_manager_set_mode(BT_MNG_MODE_PAIRING);

    bk_bt_gap_set_page_timeout(btm_env.page_timeout);
    bk_bt_gap_set_page_scan_activity(btm_env.page_scan_interval, btm_env.page_scan_window);

    if(bk_bt_gap_set_security_param(BK_BT_SP_IOCAP_MODE,
                                    &btm_env.io_capability,
                                    sizeof(btm_env.io_capability)))
    {
        LOGE("%s set security param err\n");
    }

#if CONFIG_WIFI_COEX_SCHEME
    bt_manager_register_wifi_coex();
#endif

    rtos_delay_milliseconds(50);

    btm_env.inited = 1;
    LOGI("%s end\n", __func__);
    return 0;
}

int bt_manager_deinit()
{
    LOGI("%s\n", __func__);

    if(!btm_env.inited)
    {
        LOGE("%s already deinit\n", __func__);
        return -1;
    }

    bk_bt_gap_register_callback(NULL);
    bt_manager_set_mode(BT_MNG_MODE_ALL_OFF);

#if CONFIG_WIFI_COEX_SCHEME
    coex_bt_if_init(NULL);
    bt_manager_task_deinit();
#endif

    os_memset(&btm_cbs, 0, sizeof(btm_cbs));

    bluetooth_storage_sync_to_flash();
    bluetooth_storage_deinit();
    os_memset(&btm_env, 0, sizeof(btm_env_s));

    LOGI("%s end\n", __func__);
    return 0;
}

int bt_manager_register_callback(btm_callback_s *cb)
{
    int i=0;
    for(;i<MAX_PROFILE_NUM;i++)
    {
        if(
            btm_cbs[i].gap_cb == NULL
            && btm_cbs[i].start_connect_cb == NULL
            && btm_cbs[i].stop_connect_cb == NULL
            && btm_cbs[i].start_disconnect_cb == NULL
            && btm_cbs[i].reconnect_fail_cb == NULL
        )
        {
            LOGI("%s i %d %p\n", __func__, i, cb);
            btm_cbs[i].gap_cb = cb->gap_cb;
            btm_cbs[i].start_connect_cb = cb->start_connect_cb;
            btm_cbs[i].stop_connect_cb = cb->stop_connect_cb;
            btm_cbs[i].start_disconnect_cb = cb->start_disconnect_cb;
            btm_cbs[i].reconnect_fail_cb = cb->reconnect_fail_cb;
            return i;
        }
    }
    LOGE("%s, callback max resource, reg fail !! \n", __func__);
    return MAX_PROFILE_NUM;
}

int bt_manager_unregister_callback(uint8_t index)
{
    if (index >= MAX_PROFILE_NUM)
    {
        LOGE("%s, wrong index %d !! \n", __func__, index);
        return -1;
    }
    LOGI("%s i %d\n", __func__, index);
    os_memset(&btm_cbs[index], 0, sizeof(btm_callback_s));
    return 0;
}

uint8_t bt_manager_get_connect_state(void)
{
    return btm_env.connect_state;
}

void bt_manager_set_connect_state(uint8_t state)
{
    btm_env.connect_state = state;
}

void bt_manager_get_reconnect_device(uint8_t *addr)
{
    os_memcpy(addr, btm_env.recon_addr, sizeof(btm_env.recon_addr));
}

uint8_t* bt_manager_get_connected_device(void)
{
    return btm_env.peer_addr;
}

void bt_manager_clean_bond(void)
{
    bluetooth_storage_clean_linkkey_info();
    bluetooth_storage_sync_to_flash();
}

void bt_manager_set_tmp_linkkey(uint8_t *addr, uint8_t *linkkey)
{
    LOGI("%s set tmp linkkey\n", __func__);
    os_memcpy(btm_env.tmp_link_key, linkkey, sizeof(btm_env.tmp_link_key));
}

