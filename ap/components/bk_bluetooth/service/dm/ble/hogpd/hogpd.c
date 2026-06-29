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
#include "hogpd.h"
#include <stdint.h>

enum
{
    HOGPD_DEBUG_LEVEL_ERROR,
    HOGPD_DEBUG_LEVEL_WARNING,
    HOGPD_DEBUG_LEVEL_INFO,
    HOGPD_DEBUG_LEVEL_DEBUG,
    HOGPD_DEBUG_LEVEL_VERBOSE,
};

#ifndef HOGPD_DEBUG_LEVEL
#ifdef CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_HOGPD_LOG_LEVEL
#define HOGPD_DEBUG_LEVEL CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_HOGPD_LOG_LEVEL
#else
#define HOGPD_DEBUG_LEVEL HOGPD_DEBUG_LEVEL_INFO
#endif
#endif

#define LOGE(format, ...) do{if(HOGPD_DEBUG_LEVEL >= HOGPD_DEBUG_LEVEL_ERROR)   BK_LOGE("dm_hogpd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(HOGPD_DEBUG_LEVEL >= HOGPD_DEBUG_LEVEL_WARNING) BK_LOGW("dm_hogpd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(HOGPD_DEBUG_LEVEL >= HOGPD_DEBUG_LEVEL_INFO)    BK_LOGI("dm_hogpd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(HOGPD_DEBUG_LEVEL >= HOGPD_DEBUG_LEVEL_DEBUG)   BK_LOGD("dm_hogpd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(HOGPD_DEBUG_LEVEL >= HOGPD_DEBUG_LEVEL_VERBOSE) BK_LOGV("dm_hogpd", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#if HOGPD_ENABLE

#define PROFILE_ID 2

#define MIN_VALUE(x, y) (((x) < (y)) ? (x): (y))

typedef struct
{
    uint8_t status; //0 idle 1 connected
    beken_semaphore_t server_sem;
    uint16_t send_notify_read_rsp_status;
} hogpd_app_env_t;

static uint8_t s_hogpd_is_init;
static uint8_t s_protpcol_mode = 1;
static uint8_t s_db_init;

static const uint8_t s_hid_rprtmap[] =
{
    0x05U, 0x01U, 0x09U, 0x06U, 0xA1U, 0x01U, 0x05U, 0x07U,
    0x19U, 0xE0U, 0x29U, 0xE7U, 0x15U, 0x00U, 0x25U, 0x01U,
    0x75U, 0x01U, 0x95U, 0x08U, 0x81U, 0x02U, 0x95U, 0x01U,
    0x75U, 0x08U, 0x81U, 0x03U, 0x95U, 0x05U, 0x75U, 0x01U,
    0x05U, 0x08U, 0x19U, 0x01U, 0x29U, 0x05U, 0x91U, 0x02U,
    0x95U, 0x01U, 0x75U, 0x03U, 0x91U, 0x03U, 0x95U, 0x06U,
    0x75U, 0x08U, 0x15U, 0x00U, 0x25U, 0x65U, 0x05U, 0x07U,
    0x19U, 0x00U, 0x29U, 0x65U, 0x81U, 0x00U, 0xC0U
};
static uint16_t s_report_map_desc;

static uint8_t s_input_hid_rprt[] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
static uint16_t s_input_hid_rprt_client_conf;
static const uint8_t s_input_hid_rprt_desc[] = {0, 1};

static uint8_t s_output_hid_rprt[] = {0x00U, 0x00U, 0x00U};
static const uint8_t s_output_hid_rprt_desc[] = { 0x00U, 0x02U };

static uint8_t s_feature_hid_rprt[] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
static const uint8_t s_feature_hid_rprt_desc[] = { 0x00U, 0x03U };

static const uint8_t s_hid_info[] = {0x13U, 0x02U, 0x40U, 0x01U};

static uint8_t s_boot_kbd_input_rprt[] = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
static uint16_t s_boot_kbd_input_rprt_client_conf;

static uint8_t s_boot_kbd_output_rprt[] = {0x00U, 0x00U, 0x00U};

//static uint8_t s_boot_mouse_input_rprt[] = {0x00U, 0x00U, 0x00U};


enum
{
    HOGPD_DB_IDX_SVC,
    HOGPD_DB_IDX_PROTO_MODE,

    HOGPD_DB_IDX_REPORT_MAP,
    HOGPD_DB_IDX_REPORT_MAP_REF_DESCR,

    HOGPD_DB_IDX_INPUT_REPORT,
    HOGPD_DB_IDX_INPUT_REPORT_CLIENT_CONF,
    HOGPD_DB_IDX_INPUT_REPORT_REF_DESCR,

    HOGPD_DB_IDX_OUTPUT_REPORT,
    HOGPD_DB_IDX_OUTPUT_REPORT_REF_DESCR,

    HOGPD_DB_IDX_FEATURE_REPORT,
    HOGPD_DB_IDX_FEATURE_REPORT_REF_DESC,

    HOGPD_DB_IDX_CONTROL_POINT,

    HOGPD_DB_IDX_INFO,

    HOGPD_DB_IDX_BOOT_KBD_INPUT_REPORT,
    HOGPD_DB_IDX_BOOT_KBD_INPUT_REPORT_CLIENT_CONF,

    HOGPD_DB_IDX_BOOT_KBD_OUTPUT_REPORT,
};

static const bk_gatts_attr_db_t s_gatts_attr_db_service_hidd[] =
{
    {
        BK_GATT_PRIMARY_SERVICE_DECL(BK_GATT_UUID_HID_SVC),
    },

    //proto mode
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_PROTO_MODE,
                          sizeof(s_protpcol_mode), &s_protpcol_mode,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          //BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          BK_GATT_RSP_BY_APP),
    },

    //report map
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_REPORT_MAP,
                          sizeof(s_hid_rprtmap), (uint8_t *)s_hid_rprtmap,
                          BK_GATT_CHAR_PROP_BIT_READ,
                          BK_GATT_PERM_READ_ENCRYPTED,
                          //BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_EXT_RPT_REF_DESCR,
                               sizeof(s_report_map_desc), (uint8_t *)&s_report_map_desc,
                               BK_GATT_PERM_READ,
                               BK_GATT_AUTO_RSP),
    },

    //input report
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_REPORT,
                          sizeof(s_input_hid_rprt), s_input_hid_rprt,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_input_hid_rprt_client_conf), (uint8_t *)&s_input_hid_rprt_client_conf,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_RPT_REF_DESCR,
                               sizeof(s_input_hid_rprt_desc), (uint8_t *)s_input_hid_rprt_desc,
                               BK_GATT_PERM_READ,
                               BK_GATT_AUTO_RSP),
    },

    //output report
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_REPORT,
                          sizeof(s_output_hid_rprt), s_output_hid_rprt,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_RPT_REF_DESCR,
                               sizeof(s_output_hid_rprt_desc), (uint8_t *)s_output_hid_rprt_desc,
                               BK_GATT_PERM_READ,
                               BK_GATT_AUTO_RSP),
    },

    //feature report
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_REPORT,
                          sizeof(s_feature_hid_rprt), s_feature_hid_rprt,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_RPT_REF_DESCR,
                               sizeof(s_feature_hid_rprt_desc), (uint8_t *)s_feature_hid_rprt_desc,
                               BK_GATT_PERM_READ,
                               BK_GATT_AUTO_RSP),
    },

    //control point
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_CONTROL_POINT,
                          0, NULL,
                          BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                          BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },

    //info
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_INFORMATION,
                          sizeof(s_hid_info), (uint8_t *)s_hid_info,
                          BK_GATT_CHAR_PROP_BIT_READ,
                          BK_GATT_PERM_READ,
                          BK_GATT_AUTO_RSP),
    },

    //bootkeyboardinput report
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_BT_KB_INPUT,
                          sizeof(s_boot_kbd_input_rprt), s_boot_kbd_input_rprt,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_boot_kbd_input_rprt_client_conf), (uint8_t *)&s_boot_kbd_input_rprt_client_conf,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_AUTO_RSP),
    },

    //bootkeyboardoutput report
    {
        BK_GATT_CHAR_DECL(BK_GATT_UUID_HID_BT_KB_OUTPUT,
                          sizeof(s_boot_kbd_output_rprt), s_boot_kbd_output_rprt,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_WRITE_NR,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
};

static uint16_t s_hogpd_attr_handle_list[sizeof(s_gatts_attr_db_service_hidd) / sizeof(s_gatts_attr_db_service_hidd[0])];


static int32_t hogpd_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    dm_gatt_app_env_t *common_env_tmp = NULL;
    hogpd_app_env_t *app_env_tmp = NULL;

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

        common_env_tmp = dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))dm_ble_find_profile_data_by_profile_id(common_env_tmp, PROFILE_ID);

        if (app_env_tmp)
        {
            if (app_env_tmp->server_sem)
            {
                rtos_deinit_semaphore(&app_env_tmp->server_sem);
                app_env_tmp->server_sem = NULL;
            }
        }
    }
    break;

    case BK_GATTS_CONF_EVT:
    {
        struct gatts_conf_evt_param *param = (typeof(param))comm_param;

        LOGI("BK_GATTS_CONF_EVT");

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))dm_ble_find_profile_data_by_profile_id(common_env_tmp, PROFILE_ID);

        if (app_env_tmp)
        {
            app_env_tmp->send_notify_read_rsp_status = param->status;

            if (app_env_tmp->server_sem)
            {
                rtos_set_semaphore(&app_env_tmp->server_sem);
            }
        }
    }
    break;

    case BK_GATTS_RESPONSE_EVT:
    {
        struct gatts_rsp_evt_param *param = (typeof(param))comm_param;

        LOGI("BK_GATTS_RESPONSE_EVT");

        common_env_tmp = dm_ble_find_app_env_by_conn_id(param->conn_id);

        if (!common_env_tmp)
        {
            LOGE("cant find app env");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))dm_ble_find_profile_data_by_profile_id(common_env_tmp, PROFILE_ID);

        if (app_env_tmp)
        {
            app_env_tmp->send_notify_read_rsp_status = param->status;

            if (app_env_tmp->server_sem)
            {
                rtos_set_semaphore(&app_env_tmp->server_sem);
            }
        }
    }
    break;

    case BK_GATTS_READ_EVT:
    {
        struct gatts_read_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        os_memset(&rsp, 0, sizeof(rsp));
        LOGI("read attr handle %d need rsp %d", param->handle, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint32_t buff_size = 0;
        uint8_t valid = 1;
        uint32_t index = 0;

        if (bk_dm_prf_gatts_get_buff_from_attr_handle((bk_gatts_attr_db_t *)s_gatts_attr_db_service_hidd, s_hogpd_attr_handle_list,
                                               sizeof(s_hogpd_attr_handle_list) / sizeof(s_hogpd_attr_handle_list[0]), param->handle, &index, &tmp_buff, &buff_size))
        {
            LOGE("attr hande %d app invalid !!!", param->handle);
            valid = 0;
        }

        LOGI("index %d size %d buff %p", index, buff_size, tmp_buff);

        if (index == HOGPD_DB_IDX_PROTO_MODE)
        {
            LOGI("read proto mode");
        }
        else if (index == HOGPD_DB_IDX_REPORT_MAP)
        {
            LOGI("read report map");
        }
        else if (index == HOGPD_DB_IDX_INPUT_REPORT)
        {
            LOGI("read input report");
        }
        else if (index == HOGPD_DB_IDX_INPUT_REPORT_CLIENT_CONF)
        {
            LOGI("read input report client conf");
        }
        else if (index == HOGPD_DB_IDX_INPUT_REPORT_REF_DESCR)
        {
            LOGI("read input report desc");
        }
        else if (index == HOGPD_DB_IDX_OUTPUT_REPORT)
        {
            LOGI("read output report");
        }
        else if (index == HOGPD_DB_IDX_OUTPUT_REPORT_REF_DESCR)
        {
            LOGI("read output report desc");
        }
        else if (index == HOGPD_DB_IDX_FEATURE_REPORT)
        {
            LOGI("read feature report");
        }
        else if (index == HOGPD_DB_IDX_FEATURE_REPORT_REF_DESC)
        {
            LOGI("read feature report desc");
        }
        else if (index == HOGPD_DB_IDX_CONTROL_POINT)
        {
            LOGI("read control point");
        }
        else if (index == HOGPD_DB_IDX_INFO)
        {
            LOGI("read info");
        }
        else if (index == HOGPD_DB_IDX_BOOT_KBD_INPUT_REPORT)
        {
            LOGI("read bootkeyboardinput report");
        }
        else if (index == HOGPD_DB_IDX_BOOT_KBD_INPUT_REPORT_CLIENT_CONF)
        {
            LOGI("read bootkeyboardinput report client conf");
        }
        else if (index == HOGPD_DB_IDX_BOOT_KBD_OUTPUT_REPORT)
        {
            LOGI("read bootkeyboardoutput report");
        }
        else
        {
            valid = 0;
        }

        if (param->need_rsp)
        {
            final_len = buff_size - param->offset;

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;

            if (tmp_buff && valid)
            {
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = tmp_buff + param->offset;
            }
            else
            {
                rsp.attr_value.len = 0;
                rsp.attr_value.value = NULL;
            }

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id,
                                             (tmp_buff && valid ? BK_GATT_OK : BK_GATT_INSUF_RESOURCE), &rsp);
        }
    }
    break;

    case BK_GATTS_WRITE_EVT:
    {
        struct gatts_write_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp = {0};
        uint16_t final_len = 0;

        LOGI("write attr handle %d need rsp %d", param->handle, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint32_t buff_size = 0;
        uint32_t index = 0;
        uint8_t valid = 1;

        if (bk_dm_prf_gatts_get_buff_from_attr_handle((bk_gatts_attr_db_t *)s_gatts_attr_db_service_hidd, s_hogpd_attr_handle_list,
                                               sizeof(s_hogpd_attr_handle_list) / sizeof(s_hogpd_attr_handle_list[0]), param->handle, &index, &tmp_buff, &buff_size))
        {
            LOGI("handle invalid");
            valid = 0;
        }

        LOGI("index %d size %d buff %p", index, buff_size, tmp_buff);

        if (index == HOGPD_DB_IDX_PROTO_MODE)
        {
            LOGI("write proto mode");
        }
        else if (index == HOGPD_DB_IDX_INPUT_REPORT)
        {
            LOGI("write input report");
        }
        else if (index == HOGPD_DB_IDX_INPUT_REPORT_CLIENT_CONF)
        {
            uint16_t config = (((uint16_t)(param->value[1])) << 8) | param->value[0];

            LOGI("write input report ccc");

            if (config & 1)
            {
                LOGI("client notify open");
            }
            else
            {
                LOGI("client write invalid data 0x%x", config);
            }
        }
        else if (index == HOGPD_DB_IDX_OUTPUT_REPORT)
        {
            LOGI("write output report");
        }
        else if (index == HOGPD_DB_IDX_FEATURE_REPORT)
        {
            LOGI("write feature report");
        }
        else if (index == HOGPD_DB_IDX_CONTROL_POINT)
        {
            LOGI("write control point");
        }
        else if (index == HOGPD_DB_IDX_INFO)
        {
            LOGI("write bootkeyboardinput report");
        }

        else if (index == HOGPD_DB_IDX_BOOT_KBD_INPUT_REPORT_CLIENT_CONF)
        {
            uint16_t config = (((uint16_t)(param->value[1])) << 8) | param->value[0];

            LOGI("write bootkeyboardinput report ccc");

            if (config & 1)
            {
                LOGI("client notify open");
            }
            else
            {
                LOGI("client write invalid data 0x%x", config);
            }
        }
        else if (index == HOGPD_DB_IDX_BOOT_KBD_OUTPUT_REPORT)
        {
            LOGI("write bootkeyboardoutput report");
        }
		else
		{
			valid = 0;
		}

        if (param->need_rsp)
        {
            final_len = MIN_VALUE(param->len, buff_size - param->offset);
            memcpy(tmp_buff + param->offset, param->value, final_len);

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;
            rsp.attr_value.len = final_len;
            rsp.attr_value.value = tmp_buff + param->offset;

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, valid ? BK_GATT_OK : BK_GATT_INSUF_RESOURCE, &rsp);
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

    return 0;
}

static int32_t hogpd_reg_db(void)
{
    int32_t ret = bk_dm_prf_gatts_reg_db((bk_gatts_attr_db_t *)s_gatts_attr_db_service_hidd,
                                  sizeof(s_gatts_attr_db_service_hidd) / sizeof(s_gatts_attr_db_service_hidd[0]),
                                  s_hogpd_attr_handle_list,
                                  hogpd_gatts_cb, s_db_init ? 0 : 1);

    if (ret)
    {
        LOGE("reg db err");
        return ret;
    }

    s_db_init = 1;

    return ret;
}

#endif

int32_t bk_dm_prf_hogpd_init(void)
{
#if HOGPD_ENABLE

    if (!bk_dm_prf_gatts_is_init())
    {
        LOGE("gatts is not init");
        return -1;
    }

    if (s_hogpd_is_init)
    {
        LOGE("already init");
        return -1;
    }

    s_hogpd_is_init = 1;

    hogpd_reg_db();

    LOGI("done");
#else
    LOGE("hogpd not enable");
#endif
    return 0;
}

int32_t bk_dm_prf_hogpd_deinit(uint8_t deinit_bluetooth_future)
{
#if HOGPD_ENABLE

    if (!s_hogpd_is_init)
    {
        LOGE("already deinit");
        return -1;
    }

    LOGW("sdk can't del db service now !!!");

    bk_dm_prf_gatts_unreg_db((bk_gatts_attr_db_t *)s_gatts_attr_db_service_hidd);

    if (deinit_bluetooth_future)
    {
        s_db_init = 0;
    }

    s_hogpd_is_init = 0;
#endif
    return 0;
}

int32_t bk_dm_prf_hogpd_deinit_because_bluetooth_deinit_future(void)
{
#if HOGPD_ENABLE
    s_db_init = 0;
#endif
    return 0;
}

int32_t bk_dm_prf_hogpd_notify(uint16_t gatt_conn_handle, uint8_t *data, uint32_t len, uint8_t is_notify, uint8_t report_id)
{
    int32_t ret = 0;

    hogpd_app_env_t *app_env_tmp = NULL;
    dm_gatt_app_env_t *common_env_tmp = NULL;

    if (!s_hogpd_is_init)
    {
        LOGE("not init");
        return -1;
    }

    //rtos_init_event()

    common_env_tmp = dm_ble_find_app_env_by_conn_id(gatt_conn_handle);

    if (!common_env_tmp || !common_env_tmp->data)
    {
        LOGE("conn_id %d not found %d %p", gatt_conn_handle, common_env_tmp->data);
        ret = -1;
        goto end;
    }

    app_env_tmp = (typeof(app_env_tmp))dm_ble_find_profile_data_by_profile_id(common_env_tmp, PROFILE_ID);

    if (!app_env_tmp)
    {
        LOGE("conn_id %d not found app_env", gatt_conn_handle);
        ret = -1;
        goto end;
    }

    if (!app_env_tmp->server_sem)
    {
        ret = rtos_init_semaphore(&app_env_tmp->server_sem, 1);

        if (ret)
        {
            LOGE("init sem err %d", ret);
            ret = -1;
            goto end;
        }
    }

    ret = bk_ble_gatts_send_indicate(bk_dm_prf_gatts_get_current_if(), gatt_conn_handle, s_hogpd_attr_handle_list[HOGPD_DB_IDX_INPUT_REPORT], len, data, is_notify ? 0 : 1);

    if (ret)
    {
        LOGE("send err %d", ret);
        ret = -1;
        goto end;
    }

    ret = rtos_get_semaphore(&app_env_tmp->server_sem, SYNC_CMD_TIMEOUT_MS);

    if (ret)
    {
        LOGE("wait send completed err %d", ret);
        ret = -1;
        goto end;
    }

end:;

    if(app_env_tmp)
    {
        ret = (app_env_tmp->send_notify_read_rsp_status ? -1 : 0);

        app_env_tmp->send_notify_read_rsp_status = 0;

        if (app_env_tmp->server_sem)
        {
            rtos_deinit_semaphore(&app_env_tmp->server_sem);
            app_env_tmp->server_sem = NULL;
        }
    }

    return ret;
}
