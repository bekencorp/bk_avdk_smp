#include <stdint.h>
#include <string.h>
#include "hal_hci_core.h"
#include <os/mem.h>
#include <os/str.h>
#include "components/bluetooth/bk_ble_types.h"
#include "components/bluetooth/bk_ble.h"

#include "bt_ipc_core.h"
#include "hci_distinguish.h"
#include "hal_hci_internal.h"
#include "hci_parse.h"

#define LOG_TAG "hal_hci"
#define LOG_LEVEL LOG_LEVEL_INFO

enum
{
    MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE,
    MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL,
    MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT,
    MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE,
    MULTI_CONTROLLER_MODE_COUNT,
};

enum
{
    MULTI_CONTROLLER_DIR_PRI,
    MULTI_CONTROLLER_DIR_SEC,
    MULTI_CONTROLLER_DIR_ALL,
    MULTI_CONTROLLER_DIR_NOT_SURE,
    MULTI_CONTROLLER_DIR_COUNT,
};

enum
{
    MULTI_CONTROLLER_VOTE_START = 0,
    MULTI_CONTROLLER_VOTE_PRI = MULTI_CONTROLLER_VOTE_START,
    MULTI_CONTROLLER_VOTE_SEC,

    MULTI_CONTROLLER_VOTE_END,
};

typedef struct
{
    uint16_t opcode;
    uint8_t vote;
    uint8_t ret_status;
} multi_ct_cmd_t;


static const uint8_t s_controller_mode =
#if CONFIG_BLUETOOTH_MULTI_CONTROLLER

    #if CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE
        MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE
    #elif CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL
        MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL
    #elif CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT
        MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT
    #elif CONFIG_BLUETOOTH_MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE
        MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE
    #endif

#else
    MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE
    //MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL
#endif
    ;

static bk_bluetooth_secondary_callback_t *s_secondary_cb;
static beken_queue_t s_multi_controller_cmd_queue = NULL;
static beken_mutex_t s_multi_controller_mutex = NULL;

static uint8_t hal_hci_arb_cmd_dir(uint8_t current_controller_mode, uint16_t opcode)
{
    static const uint8_t s_cmd_dir_map[MULTI_CONTROLLER_MODE_COUNT][HCI_CMD_TYPE_COUNT] =
    {
        [MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE] = {MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
        [MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL] = {MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
        [MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT] = {MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_ALL, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
        [MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE] = {MULTI_CONTROLLER_DIR_PRI, MULTI_CONTROLLER_DIR_SEC, MULTI_CONTROLLER_DIR_ALL, MULTI_CONTROLLER_DIR_NOT_SURE, MULTI_CONTROLLER_DIR_PRI},
    };

    return s_cmd_dir_map[current_controller_mode][hci_cmd_get_type(opcode)];
}

static ble_err_t ble_hci_to_host_evt_cb_ext(uint8_t from, uint8_t *buf, uint16_t len)
{
    int32_t ret = 0;
    event_hdr_t *event_hdr = (event_hdr_t *)buf;
    multi_ct_cmd_t msg = {0};
    uint8_t need_rep = 0;

    if (s_controller_mode == MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE ||
            s_controller_mode == MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL)
    {
        need_rep = 1;
        goto end;
    }

    rtos_lock_mutex(&s_multi_controller_mutex);
    ret = rtos_pop_from_queue(&s_multi_controller_cmd_queue, &msg, 0);

    if(ret)
    {
        rtos_unlock_mutex(&s_multi_controller_mutex);
        need_rep = 1;
        goto end;
    }


    uint16_t opcode = 0;
    uint8_t status = 0;
    uint8_t status_pos = 0;

    if (event_hdr->event_code == 0xe) //cmd compl
    {
        status_pos = 5;
        opcode = (((uint16_t)buf[4]) << 8 | buf[3]);
        status = buf[status_pos];
    }
    else if (event_hdr->event_code == 0xf) //cmd status
    {
        status_pos = 2;
        opcode = (((uint16_t)buf[5]) << 8 | buf[4]);
        status = buf[status_pos];
    }

    if (status)
    {
        LOGW("evt 0x%x op 0x%04x status 0x%x from %d dir %d type %d !!!", event_hdr->event_code, opcode, status, from,
             hal_hci_arb_cmd_dir(s_controller_mode, opcode), hci_cmd_get_type(opcode));
    }

    if (msg.opcode == opcode)
    {
        msg.ret_status |= status;
        msg.vote |= (1 << from);

        uint8_t all_vote_mask = 0;

        for (uint32_t i = MULTI_CONTROLLER_VOTE_START; i < MULTI_CONTROLLER_VOTE_END; i++)
        {
            all_vote_mask |= (1 << i);
        }

        if (all_vote_mask != msg.vote)
        {
            ret = rtos_push_to_queue_front(&s_multi_controller_cmd_queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret)
            {
                LOGE("push to queue front err !!!");
                BK_ASSERT(0);
            }

            LOGD("prevent report because not full, op 0x%04x from %d status 0x%x", opcode, from, status);
        }
        else
        {
            buf[status_pos] = msg.ret_status;
            need_rep = 1;
            LOGD("report because full, op 0x%04x status 0x%x", opcode, msg.ret_status);
        }
    }

    rtos_unlock_mutex(&s_multi_controller_mutex);

end:;

    if (need_rep)
    {
        if(s_multi_controller_mutex)
        {
            rtos_lock_mutex(&s_multi_controller_mutex);
            bt_ipc_hci_send_event(event_hdr->event_code, event_hdr->param, event_hdr->param_len);
            rtos_unlock_mutex(&s_multi_controller_mutex);
        }
        else
        {
            bt_ipc_hci_send_event(event_hdr->event_code, event_hdr->param, event_hdr->param_len);
        }
    }

    return 0;
}

static ble_err_t ble_hci_to_host_evt_cb(uint8_t *buf, uint16_t len)
{
    return ble_hci_to_host_evt_cb_ext(MULTI_CONTROLLER_VOTE_PRI, buf, len);
}

static ble_err_t ble_hci_to_host_acl_cb(uint8_t *buf, uint16_t len)
{
    acl_hdr_t *acl_hdr = (acl_hdr_t *)buf;
    bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
    return 0;
}

static ble_err_t ble_hci_to_host_sco_cb(uint8_t *buf, uint16_t len)
{
    sco_hdr_t *sco_hdr = (sco_hdr_t *)buf;
    bt_ipc_hci_send_sco_data(sco_hdr->conhdl_psf, sco_hdr->param, sco_hdr->datalen);
    return 0;
}

static ble_err_t ble_hci_to_secondary_controller(uint8_t *buf, uint16_t len)
{
    int32_t ret = 0;

    if (hci_parser_get_interface()->do_encode)
    {
        hci_parser_get_interface()->do_encode(buf, len);

        if (HCI_PARSER_ENCODE_RET_NO_NEED == ret)
        {
            if (s_secondary_cb && s_secondary_cb->send)
            {
                s_secondary_cb->send(buf, len);
            }
        }
        else if (ret == HCI_PARSER_ENCODE_RET_PENDING)
        {

        }
        else if (ret == HCI_PARSER_ENCODE_RET_ERROR)
        {
            ret = -1;
        }
    }
    else
    {
        if (s_secondary_cb && s_secondary_cb->send)
        {
            s_secondary_cb->send(buf, len);
        }
    }

    return ret;
}

static void hal_hci_driver_send(uint8_t *buf, uint16_t len)
{
    uint8_t type = buf[0];
    int32_t ret = 0;
    //LOGD("type %d,len %d", type, len);

    switch (type)
    {
    case DATA_TYPE_COMMAND:
    {
        uint16_t opcode = (((uint16_t)buf[2]) << 8 | buf[1]);
        uint8_t dir = hal_hci_arb_cmd_dir(s_controller_mode, opcode);

        switch (dir)
        {
        case MULTI_CONTROLLER_DIR_PRI:
            bk_ble_hci_cmd_to_controller(&buf[1], len - 1);
            break;

        case MULTI_CONTROLLER_DIR_SEC:
            ble_hci_to_secondary_controller(buf, len);
            break;

        case MULTI_CONTROLLER_DIR_ALL:
        {
            //todo: add to pending list
            multi_ct_cmd_t msg = {0};
            msg.opcode = opcode;
            msg.vote = 0;

            ret = rtos_push_to_queue(&s_multi_controller_cmd_queue, &msg, BEKEN_WAIT_FOREVER);

            if (ret)
            {
                LOGE("push to queue err 0x%04x", opcode);
                BK_ASSERT(0);
            }

            bk_ble_hci_cmd_to_controller(&buf[1], len - 1);
            ble_hci_to_secondary_controller(buf, len);
        }
        break;

        case MULTI_CONTROLLER_DIR_NOT_SURE:
            if (opcode == APP_HCI_DISCONNECT_CMD_OPCODE
                    || opcode == APP_HCI_VENDOR_SET_ACL_PRIORITY_CMD_OPCODE
                    || opcode == APP_HCI_RD_FAIL_CONTACT_CNT_CMD_OPCODE
                    || opcode == APP_HCI_RST_FAIL_CONTACT_CNT_CMD_OPCODE
                    || opcode == APP_HCI_RD_LINK_QUAL_CMD_OPCODE
                    || opcode == APP_HCI_RD_RSSI_CMD_OPCODE
                    || opcode == APP_HCI_RD_AFH_CH_MAP_CMD_OPCODE
                    || opcode == APP_HCI_RD_CLK_CMD_OPCODE
                    || opcode == APP_HCI_RD_ENC_KEY_SIZE_CMD_OPCODE)
            {
                uint16_t acl_handle = (((uint16_t)buf[5]) << 8 | buf[4]);

                if (s_secondary_cb && s_secondary_cb->send &&
                        acl_handle >= s_secondary_cb->acl_handle_threshold_min && acl_handle <= s_secondary_cb->acl_handle_threshold_max)
                {
                    ble_hci_to_secondary_controller(buf, len);
                }
                else
                {
                    bk_ble_hci_cmd_to_controller(&buf[1], len - 1);
                }
            }
            else
            {
                LOGE("unknow how to send op 0x%x dir %d type %d !!!", opcode, dir, hci_cmd_get_type(opcode));
            }

            break;
        }
    }
    break;

    case DATA_TYPE_ACL:
    {
        switch (s_controller_mode)
        {
        case MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE:
            bk_ble_hci_acl_to_controller(&buf[1], len - 1);
            break;

        case MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL:
             ble_hci_to_secondary_controller(buf, len);

            break;

        case MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT:
        case MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE:
        {
            uint16_t acl_handle = (((uint16_t)buf[2]) << 8 | buf[1]);

            if (s_secondary_cb && s_secondary_cb->send &&
                    acl_handle >= s_secondary_cb->acl_handle_threshold_min && acl_handle <= s_secondary_cb->acl_handle_threshold_max)
            {
                ble_hci_to_secondary_controller(buf, len);
            }
            else
            {
                bk_ble_hci_acl_to_controller(&buf[1], len - 1);
            }
        }
        break;
        }
    }
    break;

    case DATA_TYPE_SCO:
    {
        switch (s_controller_mode)
        {
        case MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE:
        case MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE:
            bk_ble_hci_to_controller(DATA_TYPE_SCO, &buf[1], len - 1);
            break;

        case MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL:
        case MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT:
            ble_hci_to_secondary_controller(buf, len);
            break;
        }
    }
    break;

    default:
        LOGE("unknown type (0x%x)", type);
        break;
    }
}

static int32_t hci_evt_secondary_report(uint8_t *data, uint32_t len)
{
    switch (s_controller_mode)
    {
    case MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE:
        LOGE("can't recv evt in mode MULTI_CONTROLLER_MODE_PRI_ALL_SEC_NONE");
        return 0;
        break;

    case MULTI_CONTROLLER_MODE_PRI_NONE_SEC_ALL:

        break;

    case MULTI_CONTROLLER_MODE_PRI_BLE_SEC_BT:
    case MULTI_CONTROLLER_MODE_PRI_BT_SEC_BLE:
    {

    }
    break;
    }

    hci_parser_get_interface()->do_parse(data, len);

    return 0;
}

static void notify_parse_packet_ready_ext_cb(uint8_t type, void *data, uint16_t len)
{
    uint8_t *buff = data;

    switch (type)
    {
    case DATA_TYPE_ACL:
        ble_hci_to_host_acl_cb(data, len);
        break;

    case DATA_TYPE_SCO:
        ble_hci_to_host_sco_cb(data, len);
        break;

    case DATA_TYPE_EVENT:
    {
        uint8_t evt = buff[0];

        switch (evt)
        {
        case 0x10: //hardware err
        {
            if (buff[2] == 0x8)
            {
                //ignore
                return;
            }
            else
            {
                LOGE("secondary controller hardware err 0x%x !!!", buff[2]);
                BK_ASSERT(0);
            }
        }
        break;
        }

        ble_hci_to_host_evt_cb_ext(MULTI_CONTROLLER_VOTE_SEC, data, len);
    }
    break;
    }
}

static void notify_encode_packet_ready_cb(void *data, uint16_t len)
{

}

int hal_hci_driver_secondary_controller_init(bk_bluetooth_secondary_callback_t *cb)
{
    int32_t ret = 0;

    ret = rtos_init_queue(&s_multi_controller_cmd_queue,
                          "multi_controller_cmd_queue",
                          sizeof(multi_ct_cmd_t),
                          20);

    if (ret)
    {
        LOGE("init queue err !!!");
        return -1;
    }

    ret = rtos_init_mutex(&s_multi_controller_mutex);

    if (ret)
    {
        LOGE("init mutex err !!!");
        return -1;
    }

    static const hci_parser_callbacks_t s_hci_parser_cb =
    {
        .notify_parse_packet_ready_ext_cb = notify_parse_packet_ready_ext_cb,
        .notify_encode_packet_ready_cb = notify_encode_packet_ready_cb,
        // .notify_uart_change_cb = uart_controller_only_uart_change_cb,
        // .notify_uart_reset_cb = uart_controller_only_uart_reset_cb,
        // .read_byte = uart_controller_only_read_byte,
    };

    if (cb->init(hci_evt_secondary_report))
    {
        LOGE("init err !!!");
        return -1;
    }

    ret = hci_parser_get_interface()->init((hci_parser_callbacks_t *)&s_hci_parser_cb);

    if (ret)
    {
        LOGE("hci parser init err");
        return -1;
    }

    s_secondary_cb = cb;

    return 0;
}

int hal_hci_driver_secondary_controller_deinit(void)
{
    hci_parser_get_interface()->deinit();

    s_secondary_cb->deinit();
    s_secondary_cb = NULL;

    rtos_deinit_queue(&s_multi_controller_cmd_queue);
    s_multi_controller_cmd_queue = NULL;

    rtos_deinit_mutex(&s_multi_controller_mutex);
    s_multi_controller_mutex = NULL;

    return 0;
}

int hal_hci_driver_open(void)
{
    int ret;

    ret = bk_ble_reg_hci_recv_callback(ble_hci_to_host_evt_cb, ble_hci_to_host_acl_cb);
    bk_ble_reg_sco_hci_recv_callback(ble_hci_to_host_sco_cb);
    bt_ipc_register_hci_send_callback(hal_hci_driver_send);
    return ret;
}

int hal_hci_driver_close(void)
{
    int ret;

    bt_ipc_register_hci_send_callback(NULL);
    ret = bk_ble_reg_hci_recv_callback(NULL, NULL);
    bk_ble_reg_sco_hci_recv_callback(NULL);

    return ret;
}
