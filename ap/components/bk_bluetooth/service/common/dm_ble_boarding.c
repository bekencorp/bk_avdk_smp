#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "components/bluetooth/bk_dm_gatt_types.h"
#include "components/bluetooth/bk_dm_gatts.h"

#include "dm_gatts.h"
#include "dm_ble_boarding.h"
#include <stdint.h>

#if CONFIG_WIFI_ENABLE
#include "bk_wifi.h"
#endif

enum
{
    BOARDING_DEBUG_LEVEL_ERROR,
    BOARDING_DEBUG_LEVEL_WARNING,
    BOARDING_DEBUG_LEVEL_INFO,
    BOARDING_DEBUG_LEVEL_DEBUG,
    BOARDING_DEBUG_LEVEL_VERBOSE,
};


#define BOARDING_DEBUG_LEVEL BOARDING_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(BOARDING_DEBUG_LEVEL >= BOARDING_DEBUG_LEVEL_ERROR)   BK_LOGE("dm_brd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(BOARDING_DEBUG_LEVEL >= BOARDING_DEBUG_LEVEL_WARNING) BK_LOGW("dm_brd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(BOARDING_DEBUG_LEVEL >= BOARDING_DEBUG_LEVEL_INFO)    BK_LOGI("dm_brd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(BOARDING_DEBUG_LEVEL >= BOARDING_DEBUG_LEVEL_DEBUG)   BK_LOGD("dm_brd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(BOARDING_DEBUG_LEVEL >= BOARDING_DEBUG_LEVEL_VERBOSE) BK_LOGV("dm_brd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#if DM_BLE_BOARDING_ENABLE

#define PROFILE_ID 3

#define MIN_VALUE(x, y) (((x) < (y)) ? (x): (y))

/* service / characteristic uuids for the Wi-Fi boarding GATT service */
#define GATT_BOARDING_SERVICE_UUID              0xFFFFU
#define GATT_BOARDING_NOTIFY_CHARACTERISTIC     0x1234U
#define GATT_BOARDING_SSID_CHARACTERISTIC       0x9ABCU
#define GATT_BOARDING_PASSWORD_CHARACTERISTIC   0xDEF0U

/* attribute table layout, the index is used to match the handle in the callback */
enum
{
    BOARDING_IDX_SVC,
    BOARDING_IDX_NOTIFY_CHAR,
    BOARDING_IDX_NOTIFY_CCC,
    BOARDING_IDX_SSID_CHAR,
    BOARDING_IDX_PASSWORD_CHAR,
    BOARDING_IDX_NB,
};

typedef struct
{
    uint8_t status; //0 idle 1 connected
} boarding_app_env_t;

static uint8_t s_boarding_is_init;
static uint8_t s_db_init;
static uint16_t s_conn_id = ~0;

static uint8_t s_boarding_notify_value[1] = {0};
static uint16_t s_boarding_notify_ccc;
static uint8_t s_boarding_ssid[64];
static uint8_t s_boarding_password[32];
static uint16_t s_boarding_ssid_len;
static uint16_t s_boarding_password_len;

static const bk_gatts_attr_db_t s_gatts_attr_db_service_boarding[BOARDING_IDX_NB] =
{
    //service
    [BOARDING_IDX_SVC] =
    {
        BK_GATT_PRIMARY_SERVICE_DECL(GATT_BOARDING_SERVICE_UUID),
    },

    //notify
    [BOARDING_IDX_NOTIFY_CHAR] =
    {
        BK_GATT_CHAR_DECL(GATT_BOARDING_NOTIFY_CHARACTERISTIC,
                          sizeof(s_boarding_notify_value), s_boarding_notify_value,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ,
                          BK_GATT_AUTO_RSP),
    },
    [BOARDING_IDX_NOTIFY_CCC] =
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_boarding_notify_ccc), (uint8_t *)&s_boarding_notify_ccc,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_AUTO_RSP),
    },

    //ssid
    [BOARDING_IDX_SSID_CHAR] =
    {
        BK_GATT_CHAR_DECL(GATT_BOARDING_SSID_CHARACTERISTIC,
                          sizeof(s_boarding_ssid), s_boarding_ssid,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },

    //password
    [BOARDING_IDX_PASSWORD_CHAR] =
    {
        BK_GATT_CHAR_DECL(GATT_BOARDING_PASSWORD_CHARACTERISTIC,
                          sizeof(s_boarding_password), s_boarding_password,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
};

static uint16_t s_boarding_attr_handle_list[sizeof(s_gatts_attr_db_service_boarding) / sizeof(s_gatts_attr_db_service_boarding[0])];


static int32_t dm_ble_boarding_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    dm_gatt_app_env_t *common_env_tmp = NULL;
    boarding_app_env_t *app_env_tmp = NULL;

    switch (event)
    {
    case BK_GATTS_CONNECT_EVT:
    {
        struct gatts_connect_evt_param *param = (typeof(param))comm_param;

        LOGI("BK_GATTS_CONNECT_EVT %d role %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id, param->link_role,
                 param->remote_bda[5],
                 param->remote_bda[4],
                 param->remote_bda[3],
                 param->remote_bda[2],
                 param->remote_bda[1],
                 param->remote_bda[0]);

        s_conn_id = param->conn_id;

        common_env_tmp = dm_ble_alloc_profile_data_by_addr(PROFILE_ID, param->remote_bda, sizeof(*app_env_tmp), (uint8_t **)&app_env_tmp);

        if (!common_env_tmp)
        {
            LOGE("alloc profile data err !!!!");
            break;
        }

        app_env_tmp->status = 1;
    }
    break;

    case BK_GATTS_DISCONNECT_EVT:
    {
        struct gatts_disconnect_evt_param *param = (typeof(param))comm_param;

        LOGI("BK_GATTS_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
                 param->remote_bda[5],
                 param->remote_bda[4],
                 param->remote_bda[3],
                 param->remote_bda[2],
                 param->remote_bda[1],
                 param->remote_bda[0]);

        s_conn_id = ~0;

        common_env_tmp = dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))dm_ble_find_profile_data_by_profile_id(common_env_tmp, PROFILE_ID);

        if (app_env_tmp)
        {
            app_env_tmp->status = 0;
        }
    }
    break;

    case BK_GATTS_CONF_EVT:
    {
        LOGI("BK_GATTS_CONF_EVT");
    }
    break;

    case BK_GATTS_RESPONSE_EVT:
    {
        LOGI("BK_GATTS_RESPONSE_EVT");
    }
    break;

    case BK_GATTS_READ_EVT:
    {
        struct gatts_read_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        memset(&rsp, 0, sizeof(rsp));
        LOGI("read attr handle %d need rsp %d", param->handle, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint32_t buff_size = 0;
        uint32_t index = 0;

        if (bk_dm_prf_gatts_get_buff_from_attr_handle((bk_gatts_attr_db_t *)s_gatts_attr_db_service_boarding, s_boarding_attr_handle_list,
                                               sizeof(s_boarding_attr_handle_list) / sizeof(s_boarding_attr_handle_list[0]), param->handle, &index, &tmp_buff, &buff_size))
        {
            LOGI("handle invalid");
            break;
        }

        if (index == BOARDING_IDX_SSID_CHAR)
        {
            buff_size = s_boarding_ssid_len;
        }
        else if (index == BOARDING_IDX_PASSWORD_CHAR)
        {
            buff_size = s_boarding_password_len;
        }

        LOGI("index %d size %d buff %p", index, buff_size, tmp_buff);

        if (param->need_rsp)
        {
            final_len = buff_size - param->offset;

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;
            rsp.attr_value.len = final_len;
            rsp.attr_value.value = tmp_buff + param->offset;

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
        }
    }
    break;

    case BK_GATTS_WRITE_EVT:
    {
        struct gatts_write_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        memset(&rsp, 0, sizeof(rsp));

        LOGI("write attr handle %d len %d need rsp %d", param->handle, param->len, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint32_t buff_size = 0;
        uint32_t index = 0;

        if (bk_dm_prf_gatts_get_buff_from_attr_handle((bk_gatts_attr_db_t *)s_gatts_attr_db_service_boarding, s_boarding_attr_handle_list,
                                               sizeof(s_boarding_attr_handle_list) / sizeof(s_boarding_attr_handle_list[0]), param->handle, &index, &tmp_buff, &buff_size))
        {
            LOGI("handle invalid");
            break;
        }

        LOGI("index %d size %d buff %p", index, buff_size, tmp_buff);

        if (index == BOARDING_IDX_NOTIFY_CCC)
        {
            uint16_t config = (((uint16_t)(param->value[1])) << 8) | param->value[0];

            if (config & 1)
            {
                LOGI("client notify open");
            }
            else
            {
                LOGI("client notify close 0x%x", config);
            }
        }
        else if (index == BOARDING_IDX_SSID_CHAR)
        {
            s_boarding_ssid_len = MIN_VALUE(param->len, sizeof(s_boarding_ssid));
            os_memset(s_boarding_ssid, 0, sizeof(s_boarding_ssid));
            os_memcpy(s_boarding_ssid, param->value, s_boarding_ssid_len);
            LOGI("boarding write SSID:%s, %d", s_boarding_ssid, s_boarding_ssid_len);
        }
        else if (index == BOARDING_IDX_PASSWORD_CHAR)
        {
            s_boarding_password_len = MIN_VALUE(param->len, sizeof(s_boarding_password));
            os_memset(s_boarding_password, 0, sizeof(s_boarding_password));
            os_memcpy(s_boarding_password, param->value, s_boarding_password_len);
            LOGI("boarding write PASS:%s, %d", s_boarding_password, s_boarding_password_len);

            /* password is the last field written, trigger the Wi-Fi connection */
#if CONFIG_WIFI_ENABLE
            demo_sta_app_init((char *)s_boarding_ssid, (char *)s_boarding_password);
#endif
        }

        if (param->need_rsp)
        {
            final_len = MIN_VALUE(param->len, buff_size - param->offset);

            if (tmp_buff && index != BOARDING_IDX_SSID_CHAR && index != BOARDING_IDX_PASSWORD_CHAR)
            {
                memcpy(tmp_buff + param->offset, param->value, final_len);
            }

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;
            rsp.attr_value.len = final_len;
            rsp.attr_value.value = param->value;

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, BK_GATT_OK, &rsp);
        }
    }
    break;

    case BK_GATTS_EXEC_WRITE_EVT:
    {
        struct gatts_exec_write_evt_param *param = (typeof(param))comm_param;
        LOGI("exec write");
    }
    break;

    default:
        break;
    }

    return ret;
}

static int32_t dm_ble_boarding_reg_db(void)
{
    int32_t ret = bk_dm_prf_gatts_reg_db((bk_gatts_attr_db_t *)s_gatts_attr_db_service_boarding,
                                  sizeof(s_gatts_attr_db_service_boarding) / sizeof(s_gatts_attr_db_service_boarding[0]),
                                  s_boarding_attr_handle_list,
                                  dm_ble_boarding_gatts_cb, s_db_init ? 0 : 1);

    if (ret)
    {
        LOGE("reg db err");
        return ret;
    }

    s_db_init = 1;

    return ret;
}

#endif

int32_t dm_ble_boarding_init(void)
{
#if DM_BLE_BOARDING_ENABLE

    if (!bk_dm_prf_gatts_is_init())
    {
        LOGE("gatts is not init");
        return -1;
    }

    if (s_boarding_is_init)
    {
        LOGE("already init");
        return -1;
    }

    s_boarding_is_init = 1;

    dm_ble_boarding_reg_db();

    LOGI("done");
#else
    LOGE("boarding not enable");
#endif
    return 0;
}

int32_t dm_ble_boarding_deinit(uint8_t deinit_bluetooth_future)
{
#if DM_BLE_BOARDING_ENABLE

    if (!s_boarding_is_init)
    {
        LOGE("already deinit");
        return -1;
    }

    LOGW("sdk can't del db service now !!!");

    bk_dm_prf_gatts_unreg_db((bk_gatts_attr_db_t *)s_gatts_attr_db_service_boarding);

    if (deinit_bluetooth_future)
    {
        s_db_init = 0;
    }

    s_boarding_is_init = 0;
#endif
    return 0;
}

int32_t dm_ble_boarding_deinit_because_bluetooth_deinit_future(void)
{
#if DM_BLE_BOARDING_ENABLE
    s_db_init = 0;
#endif
    return 0;
}

int32_t dm_ble_boarding_notify(uint8_t *data, uint16_t len)
{
#if DM_BLE_BOARDING_ENABLE

    if (s_conn_id == (uint16_t)~0)
    {
        LOGE("not connected, can not notify");
        return -1;
    }

    return bk_dm_prf_gatts_send_notify(s_conn_id, s_boarding_attr_handle_list[BOARDING_IDX_NOTIFY_CHAR], data, len, 1);
#else
    return -1;
#endif
}
