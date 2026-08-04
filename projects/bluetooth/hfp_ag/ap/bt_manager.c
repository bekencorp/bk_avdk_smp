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
#include "bluetooth_storage.h"
#include "bt_manager.h"
#include "bluetooth_user_config.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#define TAG "btm"

enum
{
    BT_MNG_DEBUG_LEVEL_ERROR,
    BT_MNG_DEBUG_LEVEL_WARNING,
    BT_MNG_DEBUG_LEVEL_INFO,
    BT_MNG_DEBUG_LEVEL_DEBUG,
    BT_MNG_DEBUG_LEVEL_VERBOSE,
};

#define BT_MNG_DEBUG_LEVEL BT_MNG_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(BT_MNG_DEBUG_LEVEL >= BT_MNG_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(BT_MNG_DEBUG_LEVEL >= BT_MNG_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(BT_MNG_DEBUG_LEVEL >= BT_MNG_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(BT_MNG_DEBUG_LEVEL >= BT_MNG_DEBUG_LEVEL_DEBUG)   BK_LOGD(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(BT_MNG_DEBUG_LEVEL >= BT_MNG_DEBUG_LEVEL_VERBOSE) BK_LOGV(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)


#define MAX_PROFILE_NUM 10

typedef struct
{
    uint8_t inited;
    uint8_t mode;
    uint8_t connect_state;
    uint8_t manual_enter_pairing;
    beken2_timer_t recon_tmr;
    uint8_t peer_addr[6];
    uint8_t recon_addr[6];
    uint8_t tmp_link_key[16];//BT_LINK_KEY_SIZE];
} btm_env_s;

static btm_env_s btm_env = {0};
static btm_callback_s btm_cbs[MAX_PROFILE_NUM] = {0};
static uint8_t s_bt_auto_accept_connection;

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

    for (uint8_t i = 0; i < MAX_PROFILE_NUM; i++)
    {
        if (btm_cbs[i].stop_connect_cb)
        {
            btm_cbs[i].stop_connect_cb();
        }
    }
}

void bk_bt_enter_pairing_mode(void)
{
    bt_stop_reconnect_timeout_check();

    LOGI("status %d", btm_env.connect_state);

    if (BT_STATE_RECONNECTING == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = 1;
        bk_bt_gap_create_conn_cancel(btm_env.recon_addr);
    }
    else if (BT_STATE_LINK_CONNECTED == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = 1;
        bk_bt_gap_disconnect(btm_env.peer_addr, 0x13);
    }
    else if (BT_STATE_PROFILE_CONNECTED == btm_env.connect_state)
    {
        btm_env.manual_enter_pairing = 1;

        for (int i = 0; i < MAX_PROFILE_NUM; i++)
        {
            if (btm_cbs[i].start_disconnect_cb)
            {
                btm_cbs[i].start_disconnect_cb(btm_env.peer_addr);
            }
        }
    }
    else
    {
        bt_manager_set_mode(BT_MNG_MODE_PAIRING);
    }

    btm_env.connect_state = BT_STATE_IDLE;
}

static char *bt_manager_mode_2_str(uint8_t mode)
{
    switch (mode)
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
    LOGI("%d -> %d", btm_env.mode, mode);
    LOGI("-> %s", bt_manager_mode_2_str(mode));

    if (btm_env.mode == mode) { return; }

    switch (mode)
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
    LOGI("");
    rtos_deinit_oneshot_timer(&btm_env.recon_tmr);

    for (int i = 0; i < MAX_PROFILE_NUM; i++)
    {
        if (btm_cbs[i].start_connect_cb)
        {
            LOGI("i %d %p", i, btm_cbs[i].start_connect_cb);
            btm_cbs[i].start_connect_cb(btm_env.recon_addr);

            if (btm_env.connect_state == BT_STATE_WAIT_FOR_RECONNECT)
            {
                return;
            }
        }
    }

    btm_env.connect_state = BT_STATE_RECONNECTING;
}

void bt_manager_start_reconnect(uint8_t *addr, uint8_t immediate)
{
    uint32_t time_ms = 200;
    int32_t ret = 0;

    btm_env.connect_state = BT_STATE_IDLE;

    os_memcpy(btm_env.recon_addr, addr, 6);
    bt_manager_set_mode(BT_MNG_MODE_RECONNECTING);

    if (!immediate)
    {
        time_ms = CONFIG_RECONN_INTERVAL;
    }

    if (!rtos_is_oneshot_timer_init(&btm_env.recon_tmr))
    {
        ret = rtos_init_oneshot_timer(&btm_env.recon_tmr, time_ms, (timer_2handler_t)link_timeout_start_reconnect_timer_hdl, NULL, 0);

        if (ret)
        {
            LOGE("init oneshot timer err %d", ret);
            return;
        }
    }
    else
    {
        LOGW("timer already init");
    }

    if (rtos_is_oneshot_timer_running(&btm_env.recon_tmr))
    {
        LOGW("timer already run, stop it");

        ret = rtos_stop_oneshot_timer(&btm_env.recon_tmr);

        if (ret)
        {
            LOGE("stop oneshot timer err %d", ret);
        }
    }

    ret = rtos_start_oneshot_timer(&btm_env.recon_tmr);

    if (ret)
    {
        LOGE("start oneshot timer err %d", ret);
        return;
    }
}

static void bt_clear_reconnect_info(void)
{
    os_memset(btm_env.recon_addr, 0, 6);
    btm_env.connect_state = BT_STATE_IDLE;
}


void gap_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
    {
        uint8_t *addr = param->acl_disconn_cmpl_stat.bda;
        LOGI("BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT Disconnected from %02x:%02x:%02x:%02x:%02x:%02x, reason 0x%02x connect_state %d",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0],
             param->acl_disconn_cmpl_stat.reason,
             btm_env.connect_state);

        //bk_bt_gap_set_visibility(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);

        if (btm_env.manual_enter_pairing)
        {
            bt_manager_set_mode(BT_MNG_MODE_PAIRING);
            btm_env.manual_enter_pairing = 0;
            break;
        }

        if ((BK_BT_STATUS_REMOTE_USER_TERM_CON == param->acl_disconn_cmpl_stat.reason || BK_BT_STATUS_CON_TERM_BY_LOCAL_HOST == param->acl_disconn_cmpl_stat.reason)
                && (BT_STATE_LINK_CONNECTED == btm_env.connect_state ||
                    BT_STATE_PROFILE_CONNECTED == btm_env.connect_state ||
                    BT_STATE_KEY_MISSING == btm_env.connect_state))
        {
            os_memset(btm_env.peer_addr, 0, 6);
            bt_clear_reconnect_info();
#if 1
            bluetooth_storage_sync_to_flash();
            LOGI("sync to flash done");
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
        else if (0) //if (BK_BT_STATUS_CON_TIMEOUT == cb->acl_disconn_cmpl_stat.reason)
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
            LOGI("BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT Connected to %02x:%02x:%02x:%02x:%02x:%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
            os_memcpy(btm_env.peer_addr, addr, 6);
            btm_env.connect_state = BT_STATE_LINK_CONNECTED;
            bt_manager_set_mode(BT_MNG_MODE_CONNECTEED);
            bk_bt_gap_switch_role(addr, 0);
        }
        else
        {
            LOGI("BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT Connect the %02x:%02x:%02x:%02x:%02x:%02x Failed, status 0x%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], param->acl_conn_cmpl_stat.stat);

            if (btm_env.manual_enter_pairing)
            {
                bt_manager_set_mode(BT_MNG_MODE_PAIRING);
                btm_env.manual_enter_pairing = 0;
                break;
            }

            if ((BK_BT_STATUS_PAGE_TIMEOUT == param->acl_conn_cmpl_stat.stat || BK_BT_STATUS_CON_ALREADY_EXISTS == param->acl_conn_cmpl_stat.stat || BK_BT_STATUS_UNKNOWN_CONNECTION_ID == param->acl_conn_cmpl_stat.stat)
                    && (BT_STATE_RECONNECTING == btm_env.connect_state))
            {
                bt_manager_start_reconnect(addr, 0);
            }
            else
            {
                bt_clear_reconnect_info();
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
            LOGI("BK_BT_GAP_AUTH_CMPL_EVT (%02x:%02x:%02x:%02x:%02x:%02x)authentication success", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        }
        else
        {
            if (BK_BT_STATUS_PIN_MISSING == param->auth_cmpl.stat || BK_BT_STATUS_AUTH_FAILURE == param->auth_cmpl.stat)
            {
                bluetooth_storage_del_linkkey_info(addr);
                btm_env.connect_state = BT_STATE_KEY_MISSING;
            }

            LOGI("BK_BT_GAP_AUTH_CMPL_EVT (%02x:%02x:%02x:%02x:%02x:%02x)authentication failed, status: 0x%02x", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], param->auth_cmpl.stat);
        }
    }
    break;

    case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
    {
        LOGI("BK_BT_GAP_LINK_KEY_NOTIF_EVT recv linkkey %02X:%02X:%02X:%02X:%02X:%02X",
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

        LOGI("%s", log_buff);

        int ret = bluetooth_storage_save_linkkey_info(param->link_key_notif.bda, param->link_key_notif.link_key);

        // s_a2dp_vol = DEFAULT_A2DP_VOLUME;
        // bluetooth_storage_save_volume(param->link_key_notif.bda, s_a2dp_vol);

        if (ret <= 0)
        {
            LOGE("save link key fail %02X:%02X:%02X:%02X:%02X:%02X",
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
            LOGI("sync to flash done");
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
            LOGI("found link key %02X:%02X:%02X:%02X:%02X:%02X",
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

            LOGI("%s", log_buff);

            found_key = 1;
        }
        else if (os_memcmp(btm_env.tmp_link_key, zero_linkkey, sizeof(btm_env.tmp_link_key)) &&
                 os_memcmp(btm_env.tmp_link_key, ff_linkkey, sizeof(btm_env.tmp_link_key)))
        {
            LOGI("use tmp linkkey");

            os_memcpy(tmp.link_key, btm_env.tmp_link_key, sizeof(btm_env.tmp_link_key));

            for (int i = 0; i < sizeof(tmp.link_key); ++i)
            {
                sprintf((char *)(log_buff + i * 2), "%02X", tmp.link_key[i]);
            }

            LOGW("%s", log_buff);

            found_key = 1;
        }

        if (found_key)
        {
            bk_bt_gap_linkkey_reply(1, &tmp);
        }
        else
        {
            LOGI("not found link key in storage %02X:%02X:%02X:%02X:%02X:%02X",
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

        switch (pm->type)
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

        LOGW("BK_BT_GAP_CONNECTION_REQ_EVT %s %02x:%02x:%02x:%02x:%02x:%02x cod 0x%x",
             type_str,
             pm->bda[5],
             pm->bda[4],
             pm->bda[3],
             pm->bda[2],
             pm->bda[1],
             pm->bda[0],
             pm->cod.cod);

        switch (pm->type)
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
    }
    break;

    default:
        break;
    }

    for (int i = 0; i < MAX_PROFILE_NUM; i++)
    {
        if (btm_cbs[i].gap_cb != NULL)
        {
            btm_cbs[i].gap_cb(event, param);
        }
    }
}

void bt_manager_set_auto_accept_connection(uint8_t type, uint8_t accept)
{
    if (accept)
    {
        s_bt_auto_accept_connection |= (1 << type);
    }
    else
    {
        s_bt_auto_accept_connection &= ~(1 << type);
    }

    LOGW("0x%x", s_bt_auto_accept_connection);
}

int bt_manager_init()
{
    LOGI("");
    int ret = 0;

    if (btm_env.inited)
    {
        LOGE("already init");
        return -1;
    }

    ret = bluetooth_storage_init();

    if (ret)
    {
        LOGE("bluetooth_storage_init err %d", ret);
        return -1;
    }

    os_memset(&btm_env, 0, sizeof(btm_env_s));
    os_memset(&btm_cbs, 0, sizeof(btm_cbs));
    s_bt_auto_accept_connection = ((1 << BT_MNG_AUTO_ACCEPT_CONNECTION_ACL) | (1 << BT_MNG_AUTO_ACCEPT_CONNECTION_SCO));
    bk_bt_gap_register_callback(gap_event_cb);
    bk_bt_gap_set_device_class(COD_PHONE);
    uint8_t bt_mac[6];
    char local_name[30] = {0};
    bk_bluetooth_get_address((uint8_t *)bt_mac);
    snprintf(local_name, 30, "%s_%02x%02x%02x", LOCAL_NAME, bt_mac[2], bt_mac[1], bt_mac[0]);
    bk_bt_gap_set_local_name((uint8_t *)local_name, os_strlen(local_name));

    bt_manager_set_mode(BT_MNG_MODE_CONNECTABLE);

    bk_bt_gap_set_page_timeout(CONFIG_PAGE_TIMEOUT);
    bk_bt_gap_set_page_scan_activity(PAGE_SCAN_INTV, PAGE_SCAN_WIN);

    uint8_t iocap = BK_BT_IO_CAP_NONE;

    if (bk_bt_gap_set_security_param(BK_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap)))
    {
        LOGE("set security param err");
    }

    rtos_delay_milliseconds(50);

    btm_env.inited = 1;
    LOGI("end");
    return 0;
}

int bt_manager_deinit()
{
    LOGI("");

    if (!btm_env.inited)
    {
        LOGE("already deinit");
        return -1;
    }

    bk_bt_gap_register_callback(NULL);
    bt_manager_set_mode(BT_MNG_MODE_ALL_OFF);

    os_memset(&btm_cbs, 0, sizeof(btm_cbs));

    bluetooth_storage_sync_to_flash();
    bluetooth_storage_deinit();
    os_memset(&btm_env, 0, sizeof(btm_env_s));

    LOGI("end");
    return 0;
}

int bt_manager_register_callback(btm_callback_s *cb)
{
    int i = 0;

    for (; i < MAX_PROFILE_NUM; i++)
    {
        if (
            btm_cbs[i].gap_cb == NULL
            && btm_cbs[i].start_connect_cb == NULL
            && btm_cbs[i].stop_connect_cb == NULL
            && btm_cbs[i].start_disconnect_cb == NULL
        )
        {
            btm_cbs[i].gap_cb = cb->gap_cb;
            btm_cbs[i].start_connect_cb = cb->start_connect_cb;
            btm_cbs[i].stop_connect_cb = cb->stop_connect_cb;
            btm_cbs[i].start_disconnect_cb = cb->start_disconnect_cb;
            return i;
        }
    }

    LOGE("callback max resource, reg fail !!");
    return MAX_PROFILE_NUM;
}

int bt_manager_unregister_callback(uint8_t index)
{
    if (index >= MAX_PROFILE_NUM)
    {
        LOGE("wrong index %d !!", index);
        return -1;
    }

    LOGI("i %d", index);
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

uint8_t *bt_manager_get_connected_device(void)
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
    LOGI("set tmp linkkey");
    os_memcpy(btm_env.tmp_link_key, linkkey, sizeof(btm_env.tmp_link_key));
}
