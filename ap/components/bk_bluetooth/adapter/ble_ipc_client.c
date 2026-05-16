#include <stdbool.h>
#include <string.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "bt_ipc_core.h"
#include "../ipc/include/bt_ipc_vendor_opcode.h"
#include "ble_ipc_client.h"

#define TAG "ble_ipc_client"
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

#ifndef UNKNOW_ACT_IDX
#define UNKNOW_ACT_IDX 0xFFU
#endif

#define BT_IPC_BLE_QUERY_TIMEOUT_MS 2000
#define BLE_IPC_DB_VALUE_OFFSET_NONE 0xFFFFU
#define BLE_IPC_QUERY_DATA_MAX_LEN BK_BLE_GAP_BD_ADDR_LEN
#define BLE_IPC_VENDOR_PAYLOAD_MAX_LEN 253U
#define BLE_IPC_FRAG_HDR_LEN 8U
#define BLE_IPC_FRAG_CHUNK_MAX_LEN (BLE_IPC_VENDOR_PAYLOAD_MAX_LEN - BLE_IPC_FRAG_HDR_LEN)
#define BLE_IPC_FRAG_REASM_MAX_LEN 2048U
#define BLE_IPC_FRAG_FLAG_START 0x01U
#define BLE_IPC_FRAG_FLAG_END 0x02U

typedef struct __attribute__((packed)) {
    uint16_t prf_task_id;
    uint8_t uuid[16];
    uint8_t att_db_nb;
    uint16_t start_hdl;
    uint8_t svc_perm;
} ble_ipc_db_cfg_t;

typedef struct __attribute__((packed)) {
    uint8_t uuid[16];
    uint16_t perm;
    uint16_t ext_perm;
    uint16_t max_size;
    uint16_t value_len;
    uint16_t value_offset;
} ble_ipc_att_desc_t;

typedef struct __attribute__((packed)) {
    uint8_t conn_idx;
    uint16_t prf_id;
    uint16_t att_idx;
    uint16_t len;
} ble_ipc_write_req_evt_t;

typedef struct __attribute__((packed)) {
    uint8_t conn_idx;
    uint16_t prf_id;
    uint16_t att_idx;
    uint16_t size;
    uint16_t length;
} ble_ipc_read_req_evt_t;

typedef struct __attribute__((packed)) {
    uint8_t conn_idx;
    uint16_t prf_id;
    uint16_t att_idx;
    uint32_t len;
} ble_ipc_value_cmd_t;

typedef struct __attribute__((packed)) {
    uint8_t conn_idx;
    ble_conn_param_t conn_param;
} ble_ipc_update_param_cmd_t;

typedef struct __attribute__((packed)) {
    uint8_t conn_idx;
} ble_ipc_conn_idx_cmd_t;

typedef struct __attribute__((packed)) {
    uint16_t max_mtu;
} ble_ipc_set_max_mtu_cmd_t;

typedef struct {
    ble_cmd_cb_t cb;
    ble_cmd_t cmd;
    uint8_t in_use;
} ble_ipc_pending_cmd_t;

typedef struct {
    uint8_t req_id;
    uint8_t status;
    uint8_t value;
    uint8_t data_len;
    uint8_t data[BLE_IPC_QUERY_DATA_MAX_LEN];
    uint8_t done;
} ble_ipc_query_rsp_t;

typedef struct {
    uint8_t seq_id;
    uint16_t sub_opcode;
    uint16_t total_len;
    uint16_t received_len;
    uint8_t *payload;
} ble_ipc_frag_ctx_t;

static ble_ipc_pending_cmd_t s_pending_cmds[256];
static ble_ipc_query_rsp_t s_query_rsp;
/* AP only keeps callback/query context; CP does not mirror full advertising state. */
static ble_notice_cb_t s_notice_cb = NULL;
static beken_mutex_t s_pending_mutex = NULL;
static beken_mutex_t s_query_mutex = NULL;
static beken_semaphore_t s_query_sema = NULL;
static uint8_t s_next_req_id = 1;
static uint8_t s_frag_seq_id;
static ble_ipc_frag_ctx_t s_event_frag_ctx;

void ble_ipc_client_init(void)
{
    if (s_pending_mutex == NULL) {
        rtos_init_mutex(&s_pending_mutex);
    }

    if (s_query_mutex == NULL) {
        rtos_init_mutex(&s_query_mutex);
    }

    if (s_query_sema == NULL) {
        rtos_init_semaphore(&s_query_sema, 1);
    }
}

static uint8_t ble_ipc_client_alloc_req_id(void)
{
    uint16_t try_count = 0;

    rtos_lock_mutex(&s_pending_mutex);

    while (try_count < 255) {
        uint8_t req_id = s_next_req_id++;

        if (req_id == 0) {
            continue;
        }

        if (!s_pending_cmds[req_id].in_use) {
            rtos_unlock_mutex(&s_pending_mutex);
            return req_id;
        }

        try_count++;
    }

    rtos_unlock_mutex(&s_pending_mutex);
    return 0;
}

static void ble_ipc_client_register_pending(uint8_t req_id, ble_cmd_t cmd, ble_cmd_cb_t cb)
{
    if ((req_id == 0) || (cb == NULL)) {
        return;
    }

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].cb = cb;
    s_pending_cmds[req_id].cmd = cmd;
    s_pending_cmds[req_id].in_use = 1;
    rtos_unlock_mutex(&s_pending_mutex);
}

static void ble_ipc_client_complete_pending(uint8_t req_id, uint8_t actv_idx, ble_err_t status)
{
    ble_cmd_cb_t cb = NULL;
    ble_cmd_t cmd = BLE_CMD_NONE;
    ble_cmd_param_t param = {0};

    if (req_id == 0) {
        return;
    }

    rtos_lock_mutex(&s_pending_mutex);

    if (s_pending_cmds[req_id].in_use) {
        cb = s_pending_cmds[req_id].cb;
        cmd = s_pending_cmds[req_id].cmd;
        s_pending_cmds[req_id].cb = NULL;
        s_pending_cmds[req_id].cmd = BLE_CMD_NONE;
        s_pending_cmds[req_id].in_use = 0;
    }

    rtos_unlock_mutex(&s_pending_mutex);

    if (cb) {
        param.cmd_idx = actv_idx;
        param.status = status;
        cb(cmd, &param);
    }
}

static uint16_t ble_ipc_get_le16(const uint8_t *data)
{
    return (uint16_t)(data[0] | (data[1] << 8));
}

static void ble_ipc_put_le16(uint8_t *data, uint16_t value)
{
    data[0] = value & 0xff;
    data[1] = value >> 8;
}

static void ble_ipc_client_send_vendor_cmd_raw(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t total_len = payload_len + 2;
    uint8_t *cmd_data = (uint8_t *)os_malloc(total_len);

    if (payload_len > BLE_IPC_VENDOR_PAYLOAD_MAX_LEN) {
        LOGW("%s, payload too long:%d\r\n", __func__, payload_len);
        return;
    }

    if (cmd_data == NULL) {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }

    cmd_data[0] = sub_opcode >> 8;
    cmd_data[1] = sub_opcode & 0xff;
    if ((payload_len > 0) && (payload != NULL)) {
        os_memcpy(&cmd_data[2], payload, payload_len);
    }

    bt_ipc_hci_send_vendor_cmd(cmd_data, total_len);
    os_free(cmd_data);
}

static void ble_ipc_client_send_vendor_cmd_ex(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t offset = 0;
    uint8_t seq_id;

    if (payload_len <= BLE_IPC_VENDOR_PAYLOAD_MAX_LEN) {
        ble_ipc_client_send_vendor_cmd_raw(sub_opcode, payload, payload_len);
        return;
    }

    if ((payload == NULL) || (payload_len > BLE_IPC_FRAG_REASM_MAX_LEN)) {
        LOGW("%s, invalid frag payload len:%d\r\n", __func__, payload_len);
        return;
    }

    seq_id = ++s_frag_seq_id;
    if (seq_id == 0) {
        seq_id = ++s_frag_seq_id;
    }

    while (offset < payload_len) {
        uint8_t frag_payload[BLE_IPC_VENDOR_PAYLOAD_MAX_LEN];
        uint16_t chunk_len = payload_len - offset;
        uint8_t flags = 0;

        if (chunk_len > BLE_IPC_FRAG_CHUNK_MAX_LEN) {
            chunk_len = BLE_IPC_FRAG_CHUNK_MAX_LEN;
        }

        if (offset == 0) {
            flags |= BLE_IPC_FRAG_FLAG_START;
        }
        if ((uint16_t)(offset + chunk_len) == payload_len) {
            flags |= BLE_IPC_FRAG_FLAG_END;
        }

        frag_payload[0] = seq_id;
        frag_payload[1] = flags;
        ble_ipc_put_le16(&frag_payload[2], sub_opcode);
        ble_ipc_put_le16(&frag_payload[4], payload_len);
        ble_ipc_put_le16(&frag_payload[6], offset);
        os_memcpy(&frag_payload[BLE_IPC_FRAG_HDR_LEN], &payload[offset], chunk_len);

        ble_ipc_client_send_vendor_cmd_raw(BT_VENDOR_SUB_OPCODE_BLE_FRAG,
                                           frag_payload,
                                           (uint16_t)(BLE_IPC_FRAG_HDR_LEN + chunk_len));
        offset += chunk_len;
    }
}

static ble_err_t ble_ipc_client_send_async_cmd(uint16_t sub_opcode,
                                               ble_cmd_t cmd,
                                               uint8_t actv_idx,
                                               const void *body,
                                               uint8_t body_len,
                                               ble_cmd_cb_t callback)
{
    uint8_t payload[2 + sizeof(ble_adv_param_t)] = {0};
    uint8_t req_id = callback ? ble_ipc_client_alloc_req_id() : 0;

    payload[0] = req_id;
    payload[1] = actv_idx;
    if ((body_len > 0) && (body != NULL)) {
        os_memcpy(&payload[2], body, body_len);
    }

    ble_ipc_client_register_pending(req_id, cmd, callback);
    ble_ipc_client_send_vendor_cmd_ex(sub_opcode, payload, body_len + 2);
    return BK_ERR_BLE_SUCCESS;
}

static uint8_t ble_ipc_client_send_sync_cmd(uint16_t sub_opcode, uint8_t default_value)
{
    uint8_t payload[1];
    uint8_t req_id = ble_ipc_client_alloc_req_id();
    bk_err_t wait_ret;

    if (req_id == 0) {
        return default_value;
    }

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].cb = NULL;
    s_pending_cmds[req_id].cmd = BLE_CMD_NONE;
    s_pending_cmds[req_id].in_use = 1;
    rtos_unlock_mutex(&s_pending_mutex);

    rtos_lock_mutex(&s_query_mutex);
    s_query_rsp.req_id = req_id;
    s_query_rsp.status = BK_ERR_BLE_FAIL;
    s_query_rsp.value = default_value;
    s_query_rsp.data_len = 0;
    s_query_rsp.done = 0;
    rtos_unlock_mutex(&s_query_mutex);

    payload[0] = req_id;

    ble_ipc_client_send_vendor_cmd_ex(sub_opcode, payload, sizeof(payload));

    wait_ret = rtos_get_semaphore(&s_query_sema, BT_IPC_BLE_QUERY_TIMEOUT_MS);
    if (wait_ret != BK_OK) {
        rtos_lock_mutex(&s_pending_mutex);
        s_pending_cmds[req_id].in_use = 0;
        rtos_unlock_mutex(&s_pending_mutex);
        return default_value;
    }

    rtos_lock_mutex(&s_query_mutex);
    if (s_query_rsp.done && (s_query_rsp.req_id == req_id) && (s_query_rsp.status == BK_ERR_BLE_SUCCESS)) {
        default_value = s_query_rsp.value;
    }

    s_query_rsp.done = 0;
    rtos_unlock_mutex(&s_query_mutex);

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].in_use = 0;
    rtos_unlock_mutex(&s_pending_mutex);

    return default_value;
}

static uint8_t ble_ipc_client_send_sync_cmd_ex(uint16_t sub_opcode,
                                               const void *body,
                                               uint8_t body_len,
                                               uint8_t default_value)
{
    uint8_t payload[1 + sizeof(bd_addr_t)] = {0};
    uint8_t req_id = ble_ipc_client_alloc_req_id();
    bk_err_t wait_ret;

    if (req_id == 0) {
        return default_value;
    }

    if (body_len > sizeof(payload) - 1) {
        return default_value;
    }

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].cb = NULL;
    s_pending_cmds[req_id].cmd = BLE_CMD_NONE;
    s_pending_cmds[req_id].in_use = 1;
    rtos_unlock_mutex(&s_pending_mutex);

    rtos_lock_mutex(&s_query_mutex);
    s_query_rsp.req_id = req_id;
    s_query_rsp.status = BK_ERR_BLE_FAIL;
    s_query_rsp.value = default_value;
    s_query_rsp.data_len = 0;
    s_query_rsp.done = 0;
    rtos_unlock_mutex(&s_query_mutex);

    payload[0] = req_id;
    if ((body_len > 0) && (body != NULL)) {
        os_memcpy(&payload[1], body, body_len);
    }

    ble_ipc_client_send_vendor_cmd_ex(sub_opcode, payload, body_len + 1);

    wait_ret = rtos_get_semaphore(&s_query_sema, BT_IPC_BLE_QUERY_TIMEOUT_MS);
    if (wait_ret != BK_OK) {
        rtos_lock_mutex(&s_pending_mutex);
        s_pending_cmds[req_id].in_use = 0;
        rtos_unlock_mutex(&s_pending_mutex);
        return default_value;
    }

    rtos_lock_mutex(&s_query_mutex);
    if (s_query_rsp.done && (s_query_rsp.req_id == req_id) && (s_query_rsp.status == BK_ERR_BLE_SUCCESS)) {
        default_value = s_query_rsp.value;
    }

    s_query_rsp.done = 0;
    rtos_unlock_mutex(&s_query_mutex);

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].in_use = 0;
    rtos_unlock_mutex(&s_pending_mutex);

    return default_value;
}

static bt_err_t ble_ipc_client_send_sync_data_cmd(uint16_t sub_opcode, uint8_t *out, uint8_t out_len)
{
    uint8_t payload[1];
    uint8_t req_id;
    bk_err_t wait_ret;
    bt_err_t ret = BT_FAIL;

    if ((out == NULL) || (out_len == 0) || (out_len > BLE_IPC_QUERY_DATA_MAX_LEN)) {
        return BK_ERR_NULL_PARAM;
    }

    req_id = ble_ipc_client_alloc_req_id();
    if (req_id == 0) {
        return BT_FAIL;
    }

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].cb = NULL;
    s_pending_cmds[req_id].cmd = BLE_CMD_NONE;
    s_pending_cmds[req_id].in_use = 1;
    rtos_unlock_mutex(&s_pending_mutex);

    rtos_lock_mutex(&s_query_mutex);
    s_query_rsp.req_id = req_id;
    s_query_rsp.status = BT_FAIL;
    s_query_rsp.value = 0;
    s_query_rsp.data_len = 0;
    s_query_rsp.done = 0;
    rtos_unlock_mutex(&s_query_mutex);

    payload[0] = req_id;
    ble_ipc_client_send_vendor_cmd_ex(sub_opcode, payload, sizeof(payload));

    wait_ret = rtos_get_semaphore(&s_query_sema, BT_IPC_BLE_QUERY_TIMEOUT_MS);
    if (wait_ret == BK_OK) {
        rtos_lock_mutex(&s_query_mutex);
        if (s_query_rsp.done &&
            (s_query_rsp.req_id == req_id) &&
            (s_query_rsp.status == BT_OK) &&
            (s_query_rsp.data_len >= out_len)) {
            os_memcpy(out, s_query_rsp.data, out_len);
            ret = BT_OK;
        }

        s_query_rsp.done = 0;
        rtos_unlock_mutex(&s_query_mutex);
    }

    rtos_lock_mutex(&s_pending_mutex);
    s_pending_cmds[req_id].in_use = 0;
    rtos_unlock_mutex(&s_pending_mutex);

    return ret;
}

static ble_err_t ble_ipc_client_send_create_db_cmd(struct bk_ble_db_cfg *ble_db_cfg)
{
    ble_ipc_db_cfg_t cfg_hdr;
    uint16_t descs_len;
    uint16_t values_len = 0;
    uint16_t payload_len;
    uint8_t *payload = NULL;
    ble_ipc_att_desc_t *desc_payload;
    uint8_t *value_payload;
    uint8_t i;

    if ((ble_db_cfg == NULL) || (ble_db_cfg->att_db == NULL) || (ble_db_cfg->att_db_nb == 0)) {
        return BK_ERR_PARAM;
    }

    descs_len = (uint16_t)(ble_db_cfg->att_db_nb * sizeof(ble_ipc_att_desc_t));
    for (i = 0; i < ble_db_cfg->att_db_nb; i++) {
        if ((ble_db_cfg->att_db[i].p_value_context != NULL) && (ble_db_cfg->att_db[i].value_len > 0)) {
            if ((uint32_t)values_len + ble_db_cfg->att_db[i].value_len > BLE_IPC_DB_VALUE_OFFSET_NONE) {
                return BK_ERR_PARAM;
            }
            values_len += ble_db_cfg->att_db[i].value_len;
        }
    }

    payload_len = (uint16_t)(1 + sizeof(cfg_hdr) + descs_len + values_len);
    payload = (uint8_t *)os_malloc(payload_len);
    if (payload == NULL) {
        return BK_ERR_NO_MEM;
    }

    payload[0] = 0;
    cfg_hdr.prf_task_id = ble_db_cfg->prf_task_id;
    os_memcpy(cfg_hdr.uuid, ble_db_cfg->uuid, sizeof(cfg_hdr.uuid));
    cfg_hdr.att_db_nb = ble_db_cfg->att_db_nb;
    cfg_hdr.start_hdl = ble_db_cfg->start_hdl;
    cfg_hdr.svc_perm = ble_db_cfg->svc_perm;
    os_memcpy(&payload[1], &cfg_hdr, sizeof(cfg_hdr));

    desc_payload = (ble_ipc_att_desc_t *)&payload[1 + sizeof(cfg_hdr)];
    value_payload = &payload[1 + sizeof(cfg_hdr) + descs_len];

    for (i = 0; i < ble_db_cfg->att_db_nb; i++) {
        os_memcpy(desc_payload[i].uuid, ble_db_cfg->att_db[i].uuid, sizeof(desc_payload[i].uuid));
        desc_payload[i].perm = ble_db_cfg->att_db[i].perm;
        desc_payload[i].ext_perm = ble_db_cfg->att_db[i].ext_perm;
        desc_payload[i].max_size = ble_db_cfg->att_db[i].max_size;
        desc_payload[i].value_len = ble_db_cfg->att_db[i].value_len;
        desc_payload[i].value_offset = BLE_IPC_DB_VALUE_OFFSET_NONE;

        if ((ble_db_cfg->att_db[i].p_value_context != NULL) && (ble_db_cfg->att_db[i].value_len > 0)) {
            uint16_t value_offset = (uint16_t)(value_payload - (&payload[1 + sizeof(cfg_hdr) + descs_len]));
            desc_payload[i].value_offset = value_offset;
            os_memcpy(value_payload,
                      ble_db_cfg->att_db[i].p_value_context,
                      ble_db_cfg->att_db[i].value_len);
            value_payload += ble_db_cfg->att_db[i].value_len;
        }
    }

    ble_ipc_client_send_vendor_cmd_ex(BT_VENDOR_SUB_OPCODE_BLE_CREATE_DB, payload, payload_len);
    os_free(payload);
    return BK_ERR_BLE_SUCCESS;
}

static void ble_ipc_client_dispatch_notice_evt(uint16_t notice, const uint8_t *payload, uint16_t payload_len)
{
    ble_write_req_t write_req;
    ble_read_req_t read_req;

    if (s_notice_cb == NULL) {
        return;
    }

    switch ((ble_notice_t)notice) {
    case BLE_5_WRITE_EVENT:
        if (payload_len >= sizeof(ble_ipc_write_req_evt_t)) {
            const ble_ipc_write_req_evt_t *header = (const ble_ipc_write_req_evt_t *)payload;

            if (payload_len >= (uint16_t)(sizeof(*header) + header->len)) {
                write_req.conn_idx = header->conn_idx;
                write_req.prf_id = header->prf_id;
                write_req.att_idx = header->att_idx;
                write_req.len = header->len;
                write_req.value = (header->len > 0) ? (uint8_t *)&payload[sizeof(*header)] : NULL;
                s_notice_cb((ble_notice_t)notice, &write_req);
            }
        }
        break;

    case BLE_5_READ_EVENT:
        if (payload_len >= sizeof(ble_ipc_read_req_evt_t)) {
            const ble_ipc_read_req_evt_t *header = (const ble_ipc_read_req_evt_t *)payload;

            read_req.conn_idx = header->conn_idx;
            read_req.prf_id = header->prf_id;
            read_req.att_idx = header->att_idx;
            read_req.value = NULL;
            read_req.size = header->size;
            read_req.length = header->length;
            s_notice_cb((ble_notice_t)notice, &read_req);
        }
        break;

    default:
        s_notice_cb((ble_notice_t)notice, (void *)payload);
        break;
    }
}

static ble_err_t ble_ipc_client_send_value_cmd(uint16_t sub_opcode,
                                               uint8_t con_idx,
                                               uint32_t len,
                                               uint8_t *buf,
                                               uint16_t prf_id,
                                               uint16_t att_idx)
{
    ble_ipc_value_cmd_t header;
    uint16_t payload_len;
    uint8_t *payload;

    if ((len > 0) && (buf == NULL)) {
        return BK_ERR_PARAM;
    }

    if (len > (UINT16_MAX - sizeof(header))) {
        return BK_ERR_PARAM;
    }

    payload_len = (uint16_t)(sizeof(header) + len);
    payload = (uint8_t *)os_malloc(payload_len);
    if (payload == NULL) {
        return BK_ERR_NO_MEM;
    }

    header.conn_idx = con_idx;
    header.prf_id = prf_id;
    header.att_idx = att_idx;
    header.len = len;
    os_memcpy(payload, &header, sizeof(header));
    if (len > 0) {
        os_memcpy(&payload[sizeof(header)], buf, len);
    }

    ble_ipc_client_send_vendor_cmd_ex(sub_opcode, payload, payload_len);
    os_free(payload);
    return BK_ERR_BLE_SUCCESS;
}

static ble_err_t ble_ipc_client_send_conn_idx_cmd(uint16_t sub_opcode, uint8_t conn_idx)
{
    ble_ipc_conn_idx_cmd_t cmd;

    cmd.conn_idx = conn_idx;
    ble_ipc_client_send_vendor_cmd_ex(sub_opcode, (const uint8_t *)&cmd, sizeof(cmd));
    return BK_ERR_BLE_SUCCESS;
}

static ble_err_t ble_ipc_client_send_update_param_cmd(uint8_t conn_idx, ble_conn_param_t *conn_param)
{
    ble_ipc_update_param_cmd_t cmd;

    if (conn_param == NULL) {
        return BK_ERR_PARAM;
    }

    cmd.conn_idx = conn_idx;
    cmd.conn_param = *conn_param;
    ble_ipc_client_send_vendor_cmd_ex(BT_VENDOR_SUB_OPCODE_BLE_UPDATE_PARAM,
                                      (const uint8_t *)&cmd,
                                      sizeof(cmd));
    return BK_ERR_BLE_SUCCESS;
}

static ble_err_t ble_ipc_client_send_set_max_mtu_cmd(uint16_t max_mtu)
{
    ble_ipc_set_max_mtu_cmd_t cmd;

    cmd.max_mtu = max_mtu;
    ble_ipc_client_send_vendor_cmd_ex(BT_VENDOR_SUB_OPCODE_BLE_SET_MAX_MTU,
                                      (const uint8_t *)&cmd,
                                      sizeof(cmd));
    return BK_ERR_BLE_SUCCESS;
}

static uint8_t ble_ipc_client_send_addr_query_cmd(uint16_t sub_opcode, bd_addr_t *addr, uint8_t default_value)
{
    if (addr == NULL) {
        return default_value;
    }

    return ble_ipc_client_send_sync_cmd_ex(sub_opcode, addr, sizeof(*addr), default_value);
}

static void ble_ipc_client_reset_frag_ctx(ble_ipc_frag_ctx_t *ctx)
{
    if (ctx->payload != NULL) {
        os_free(ctx->payload);
    }
    os_memset(ctx, 0, sizeof(*ctx));
}

static bool ble_ipc_client_handle_frag_event(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_frag_ctx_t *ctx = &s_event_frag_ctx;
    uint8_t seq_id;
    uint8_t flags;
    uint16_t sub_opcode;
    uint16_t total_len;
    uint16_t offset;
    uint16_t chunk_len;

    if ((payload == NULL) || (payload_len < BLE_IPC_FRAG_HDR_LEN)) {
        return false;
    }

    seq_id = payload[0];
    flags = payload[1];
    sub_opcode = ble_ipc_get_le16(&payload[2]);
    total_len = ble_ipc_get_le16(&payload[4]);
    offset = ble_ipc_get_le16(&payload[6]);
    chunk_len = payload_len - BLE_IPC_FRAG_HDR_LEN;

    if ((sub_opcode == BT_VENDOR_SUB_OPCODE_BLE_FRAG) ||
        (total_len == 0) ||
        (total_len > BLE_IPC_FRAG_REASM_MAX_LEN) ||
        ((uint32_t)offset + chunk_len > total_len)) {
        ble_ipc_client_reset_frag_ctx(ctx);
        return false;
    }

    if ((flags & BLE_IPC_FRAG_FLAG_START) != 0) {
        ble_ipc_client_reset_frag_ctx(ctx);
        ctx->payload = (uint8_t *)os_malloc(total_len);
        if (ctx->payload == NULL) {
            LOGW("%s, malloc failed\r\n", __func__);
            return false;
        }
        ctx->seq_id = seq_id;
        ctx->sub_opcode = sub_opcode;
        ctx->total_len = total_len;
        ctx->received_len = 0;
    }

    if ((ctx->payload == NULL) ||
        (ctx->seq_id != seq_id) ||
        (ctx->sub_opcode != sub_opcode) ||
        (ctx->total_len != total_len) ||
        (ctx->received_len != offset)) {
        ble_ipc_client_reset_frag_ctx(ctx);
        return false;
    }

    os_memcpy(&ctx->payload[offset], &payload[BLE_IPC_FRAG_HDR_LEN], chunk_len);
    ctx->received_len += chunk_len;

    if ((flags & BLE_IPC_FRAG_FLAG_END) != 0) {
        uint8_t *full_payload = ctx->payload;
        uint16_t full_len = ctx->received_len;
        uint16_t full_sub_opcode = ctx->sub_opcode;

        ctx->payload = NULL;
        os_memset(ctx, 0, sizeof(*ctx));

        if (full_len == total_len) {
            bool handled = ble_ipc_client_handle_vendor_event(full_sub_opcode, full_payload, full_len);
            os_free(full_payload);
            return handled;
        }

        os_free(full_payload);
        return false;
    }

    return true;
}

bool ble_ipc_client_handle_vendor_event(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    switch (sub_opcode) {
    case BT_VENDOR_SUB_OPCODE_BLE_FRAG:
        return ble_ipc_client_handle_frag_event(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_CMD_EVT:
        if (payload_len >= (uint16_t)(3 + sizeof(ble_err_t))) {
            ble_err_t status = BK_ERR_BLE_FAIL;
            os_memcpy(&status, &payload[3], sizeof(status));
            ble_ipc_client_complete_pending(payload[0], payload[2], status);
        }
        return true;

    case BT_VENDOR_SUB_OPCODE_BLE_NOTICE_EVT:
        if ((payload_len >= 4) && (s_notice_cb != NULL)) {
            uint16_t notice = payload[0] | (payload[1] << 8);
            uint16_t data_len = payload[2] | (payload[3] << 8);

            if (payload_len >= (uint16_t)(4 + data_len)) {
                ble_ipc_client_dispatch_notice_evt(notice, &payload[4], data_len);
            }
        }
        return true;

    case BT_VENDOR_SUB_OPCODE_BLE_QUERY_RSP:
        if (payload_len >= 3) {
            rtos_lock_mutex(&s_query_mutex);
            if (s_query_rsp.req_id == payload[0]) {
                s_query_rsp.status = payload[1];
                s_query_rsp.value = payload[2];
                s_query_rsp.data_len = 0;
                if (payload_len > 3) {
                    uint8_t data_len = payload[2];
                    if (data_len > BLE_IPC_QUERY_DATA_MAX_LEN) {
                        data_len = BLE_IPC_QUERY_DATA_MAX_LEN;
                    }
                    if ((uint16_t)(3 + data_len) <= payload_len) {
                        s_query_rsp.data_len = data_len;
                        os_memcpy(s_query_rsp.data, &payload[3], data_len);
                    }
                }
                s_query_rsp.done = 1;
                rtos_set_semaphore(&s_query_sema);
            }
            rtos_unlock_mutex(&s_query_mutex);
        }
        return true;

    default:
        return false;
    }
}

void bk_ble_set_notice_cb(ble_notice_cb_t func)
{
    uint8_t enable = (func != NULL);

    s_notice_cb = func;
    ble_ipc_client_send_vendor_cmd_ex(BT_VENDOR_SUB_OPCODE_BLE_SET_NOTICE, &enable, sizeof(enable));
    if (enable) {
        bt_ipc_notify_ap_ble_ready();
    }
}

ble_err_t bk_ble_create_db(struct bk_ble_db_cfg *ble_db_cfg)
{
    return ble_ipc_client_send_create_db_cmd(ble_db_cfg);
}

ble_err_t bk_ble_read_response_value(uint8_t con_idx, uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx)
{
    return ble_ipc_client_send_value_cmd(BT_VENDOR_SUB_OPCODE_BLE_READ_RESPONSE_VALUE,
                                         con_idx,
                                         len,
                                         buf,
                                         prf_id,
                                         att_idx);
}

ble_err_t bk_ble_update_param(uint8_t conn_idx, ble_conn_param_t *conn_param)
{
    return ble_ipc_client_send_update_param_cmd(conn_idx, conn_param);
}

ble_err_t bk_ble_gatt_mtu_change(uint8_t conn_idx)
{
    return ble_ipc_client_send_conn_idx_cmd(BT_VENDOR_SUB_OPCODE_BLE_GATT_MTU_CHANGE, conn_idx);
}

ble_err_t bk_ble_set_max_mtu(uint16_t max_mtu)
{
    return ble_ipc_client_send_set_max_mtu_cmd(max_mtu);
}

ble_err_t bk_ble_disconnect(uint8_t conn_idx)
{
    return ble_ipc_client_send_conn_idx_cmd(BT_VENDOR_SUB_OPCODE_BLE_DISCONNECT, conn_idx);
}

ble_err_t bk_ble_send_noti_value(uint8_t con_idx, uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx)
{
    return ble_ipc_client_send_value_cmd(BT_VENDOR_SUB_OPCODE_BLE_SEND_NOTI_VALUE,
                                         con_idx,
                                         len,
                                         buf,
                                         prf_id,
                                         att_idx);
}

ble_err_t bk_ble_send_ind_value(uint8_t con_idx, uint32_t len, uint8_t *buf, uint16_t prf_id, uint16_t att_idx)
{
    return ble_ipc_client_send_value_cmd(BT_VENDOR_SUB_OPCODE_BLE_SEND_IND_VALUE,
                                         con_idx,
                                         len,
                                         buf,
                                         prf_id,
                                         att_idx);
}

ble_err_t bk_ble_create_advertising(uint8_t actv_idx, ble_adv_param_t *adv_param, ble_cmd_cb_t callback)
{
    if (adv_param == NULL) {
        return BK_ERR_BLE_FAIL;
    }

    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_CREATE_ADV,
                                         BLE_CREATE_ADV,
                                         actv_idx,
                                         adv_param,
                                         sizeof(*adv_param),
                                         callback);
}

ble_err_t bk_ble_start_advertising(uint8_t actv_idx, uint16 duration, ble_cmd_cb_t callback)
{
    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_START_ADV,
                                         BLE_START_ADV,
                                         actv_idx,
                                         &duration,
                                         sizeof(duration),
                                         callback);
}

ble_err_t bk_ble_stop_advertising(uint8_t actv_idx, ble_cmd_cb_t callback)
{
    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_STOP_ADV,
                                         BLE_STOP_ADV,
                                         actv_idx,
                                         NULL,
                                         0,
                                         callback);
}

ble_err_t bk_ble_delete_advertising(uint8_t actv_idx, ble_cmd_cb_t callback)
{
    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_DELETE_ADV,
                                         BLE_DELETE_ADV,
                                         actv_idx,
                                         NULL,
                                         0,
                                         callback);
}

ble_err_t bk_ble_set_adv_data(uint8_t actv_idx, unsigned char *adv_buff, unsigned char adv_len, ble_cmd_cb_t callback)
{
    uint8_t payload[1 + BK_BLE_MAX_ADV_DATA_LEN] = {0};

    if ((adv_len > BK_BLE_MAX_ADV_DATA_LEN) || ((adv_len > 0) && (adv_buff == NULL))) {
        return BK_ERR_BLE_ADV_DATA;
    }

    payload[0] = adv_len;
    if (adv_len > 0) {
        os_memcpy(&payload[1], adv_buff, adv_len);
    }

    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_SET_ADV_DATA,
                                         BLE_SET_ADV_DATA,
                                         actv_idx,
                                         payload,
                                         adv_len + 1,
                                         callback);
}

ble_err_t bk_ble_set_scan_rsp_data(uint8_t actv_idx, unsigned char *scan_buff, unsigned char scan_len, ble_cmd_cb_t callback)
{
    uint8_t payload[1 + BK_BLE_MAX_ADV_DATA_LEN] = {0};

    if ((scan_len > BK_BLE_MAX_ADV_DATA_LEN) || ((scan_len > 0) && (scan_buff == NULL))) {
        return BK_ERR_BLE_ADV_DATA;
    }

    payload[0] = scan_len;
    if (scan_len > 0) {
        os_memcpy(&payload[1], scan_buff, scan_len);
    }

    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_SET_SCAN_RSP_DATA,
                                         BLE_SET_RSP_DATA,
                                         actv_idx,
                                         payload,
                                         scan_len + 1,
                                         callback);
}

ble_err_t bk_ble_set_adv_random_addr(uint8_t actv_idx, uint8_t *addr, ble_cmd_cb_t callback)
{
    if (addr == NULL) {
        return BK_ERR_BLE_FAIL;
    }

    return ble_ipc_client_send_async_cmd(BT_VENDOR_SUB_OPCODE_BLE_SET_ADV_RANDOM_ADDR,
                                         BLE_SET_ADV_RANDOM_ADDR,
                                         actv_idx,
                                         addr,
                                         BK_BLE_GAP_BD_ADDR_LEN,
                                         callback);
}

uint8_t bk_ble_get_idle_actv_idx_handle(void)
{
    return ble_ipc_client_send_sync_cmd(BT_VENDOR_SUB_OPCODE_BLE_GET_IDLE_ACTV_IDX,
                                        UNKNOW_ACT_IDX);
}

uint8_t bk_ble_get_max_actv_idx_count(void)
{
    return ble_ipc_client_send_sync_cmd(BT_VENDOR_SUB_OPCODE_BLE_GET_MAX_ACTV_IDX,
                                        0);
}

uint8_t bk_ble_get_max_conn_idx_count(void)
{
    return ble_ipc_client_send_sync_cmd(BT_VENDOR_SUB_OPCODE_BLE_GET_MAX_CONN_IDX,
                                        0);
}

uint8_t bk_ble_find_actv_state_idx_handle(uint8_t state)
{
    return ble_ipc_client_send_sync_cmd_ex(BT_VENDOR_SUB_OPCODE_BLE_FIND_ACTV_STATE_IDX,
                                           &state,
                                           sizeof(state),
                                           bk_ble_get_max_actv_idx_count());
}

uint8_t bk_ble_find_master_state_idx_handle(uint8_t state)
{
    return ble_ipc_client_send_sync_cmd_ex(BT_VENDOR_SUB_OPCODE_BLE_FIND_MASTER_STATE_IDX,
                                           &state,
                                           sizeof(state),
                                           bk_ble_get_max_conn_idx_count());
}

uint8_t bk_ble_find_conn_idx_from_addr(bd_addr_t *connt_addr)
{
    return ble_ipc_client_send_addr_query_cmd(BT_VENDOR_SUB_OPCODE_BLE_FIND_CONN_IDX_FROM_ADDR,
                                              connt_addr,
                                              bk_ble_get_max_conn_idx_count());
}

uint8_t bk_ble_get_connect_state(bd_addr_t *connt_addr)
{
    return ble_ipc_client_send_addr_query_cmd(BT_VENDOR_SUB_OPCODE_BLE_GET_CONNECT_STATE,
                                              connt_addr,
                                              0);
}

bt_err_t bk_bluetooth_get_address(uint8_t *addr)
{
    return ble_ipc_client_send_sync_data_cmd(BT_VENDOR_SUB_OPCODE_BLE_GET_BT_ADDRESS,
                                             addr,
                                             BK_BLE_GAP_BD_ADDR_LEN);
}

int bk_bt_feature_enable_fuzz(uint8_t enable)
{
    (void)enable;
    return BK_ERR_BLE_CMD_NOT_SUPPORT;
}

BK_BLE_CONTROLLER_STACK_TYPE bk_ble_get_controller_stack_type(void)
{
    return BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2;
}

BK_BLE_HOST_STACK_TYPE bk_ble_get_host_stack_type(void)
{
    return BK_BLE_HOST_STACK_TYPE_RW_5_2;
}

/*
 * Keep weak fallbacks for public APIs that are not proxied yet. If another
 * target provides a strong implementation, the linker will prefer that one.
 */
#define BLE_IPC_WEAK_STUB_ERR(name, args) \
    __attribute__((weak)) ble_err_t name args \
    { \
        LOGW("%s not implemented\r\n", __func__); \
        return BK_ERR_BLE_CMD_NOT_SUPPORT; \
    }

#define BLE_IPC_WEAK_STUB_U8(name, args, value) \
    __attribute__((weak)) uint8_t name args \
    { \
        LOGW("%s not implemented\r\n", __func__); \
        return (value); \
    }

#define BLE_IPC_WEAK_STUB_VOID(name, args) \
    __attribute__((weak)) void name args \
    { \
        LOGW("%s not implemented\r\n", __func__); \
    }

BLE_IPC_WEAK_STUB_U8(bk_ble_appm_get_dev_name, (uint8_t *name, uint32_t buf_len), 0)
BLE_IPC_WEAK_STUB_U8(bk_ble_appm_set_dev_name, (uint8_t len, uint8_t *name), 0)
BLE_IPC_WEAK_STUB_ERR(bk_ble_set_per_adv_data, (uint8_t actv_idx, uint8_t *per_adv_buff, uint8_t per_adv_len, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_read_phy, (uint8_t conn_idx))
BLE_IPC_WEAK_STUB_ERR(bk_ble_set_phy, (uint8_t conn_idx, ble_set_phy_t *phy_info))
BLE_IPC_WEAK_STUB_ERR(bk_ble_create_scaning, (uint8_t actv_idx, ble_scan_param_t *scan_param, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_start_scaning, (uint8_t actv_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_start_scaning_ex, (uint8_t actv_idx, uint8_t filt_duplicate, uint16_t duration, uint16_t period, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_stop_scaning, (uint8_t actv_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_delete_scaning, (uint8_t actv_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_create_init, (uint8_t con_idx, ble_conn_param_t *conn_param, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_init_start_conn, (uint8_t con_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_init_stop_conn, (uint8_t con_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_init_set_connect_dev_addr, (uint8_t connidx, bd_addr_t *bdaddr, uint8_t addr_type))
BLE_IPC_WEAK_STUB_ERR(bk_ble_create_periodic_sync, (uint8_t actv_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_start_periodic_sync, (uint8_t actv_idx, ble_periodic_param_t *param, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_stop_periodic_sync, (uint8_t actv_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_ERR(bk_ble_delete_periodic_sync, (uint8_t actv_idx, ble_cmd_cb_t callback))
BLE_IPC_WEAK_STUB_U8(bk_ble_get_idle_conn_idx_handle, (void), 0xFFU)
BLE_IPC_WEAK_STUB_ERR(bk_ble_get_mac, (uint8_t *mac))
BLE_IPC_WEAK_STUB_ERR(bk_ble_reg_hci_recv_callback, (ble_hci_to_host_cb evt_cb, ble_hci_to_host_cb acl_cb))
BLE_IPC_WEAK_STUB_ERR(bk_ble_hci_to_controller, (uint8_t type, uint8_t *buf, uint16_t len))
BLE_IPC_WEAK_STUB_ERR(bk_ble_hci_cmd_to_controller, (uint8_t *buf, uint16_t len))
BLE_IPC_WEAK_STUB_ERR(bk_ble_hci_acl_to_controller, (uint8_t *buf, uint16_t len))
BLE_IPC_WEAK_STUB_ERR(bk_ble_set_task_stack_size, (uint16_t size))
BLE_IPC_WEAK_STUB_VOID(bk_ble_register_app_sdp_charac_callback, (app_sdp_charac_callback cb))
BLE_IPC_WEAK_STUB_VOID(bk_ble_register_app_sdp_common_callback, (app_sdp_comm_callback cb))
BLE_IPC_WEAK_STUB_U8(bk_ble_gatt_write_ccc, (uint8_t con_idx, uint16_t ccc_handle, uint16_t ccc_value), 0xFFU)
BLE_IPC_WEAK_STUB_ERR(bk_ble_gatt_write_value, (uint8_t con_idx, uint16_t att_handle, uint16_t len, uint8_t *data))
BLE_IPC_WEAK_STUB_ERR(bk_ble_sec_send_auth_mode, (uint8_t con_idx, uint8_t mode, uint8_t iocap, uint8_t sec_req, uint8_t oob))
BLE_IPC_WEAK_STUB_U8(bk_ble_get_env_state, (void), 0)
BLE_IPC_WEAK_STUB_ERR(bk_ble_init, (void))
BLE_IPC_WEAK_STUB_ERR(bk_ble_deinit, (void))
BLE_IPC_WEAK_STUB_ERR(bk_ble_delete_service, (struct bk_ble_db_cfg *ble_db_cfg))
BLE_IPC_WEAK_STUB_ERR(bk_ble_att_read, (uint8_t con_idx, uint16_t att_handle))
BLE_IPC_WEAK_STUB_ERR(bk_ble_create_bond, (uint8_t con_idx, uint8_t auth, uint8_t iocap, uint8_t sec_req, uint8_t oob))
BLE_IPC_WEAK_STUB_ERR(bk_ble_create_bond_ext, (uint8_t con_idx, uint8_t auth, uint8_t iocap, uint8_t sec_req, uint8_t oob, uint8_t initiator_key_distr, uint8_t responder_key_distr))
BLE_IPC_WEAK_STUB_ERR(bk_ble_passkey_send, (uint8_t con_idx, uint8_t accept, uint32_t passkey))
BLE_IPC_WEAK_STUB_ERR(bk_ble_number_compare_send, (uint8_t con_idx, uint8_t accept))
BLE_IPC_WEAK_STUB_ERR(bk_ble_read_rssi, (uint8_t conn_idx))
BLE_IPC_WEAK_STUB_ERR(bk_ble_config_local_appearance, (uint16_t appearance))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_primary_service, (uint8_t conn_id, uint16_t sh, uint16_t eh))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_primary_service_by_uuid, (uint8_t conn_id, uint16_t sh, uint16_t eh, uint16_t uuid))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_primary_service_by_128uuid, (uint8_t conn_id, uint16_t sh, uint16_t eh, uint8_t *uuid))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_characteristic, (uint8_t conn_id, uint16_t sh, uint16_t eh))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_characteristic_by_uuid, (uint8_t conn_id, uint16_t sh, uint16_t eh, uint16_t uuid))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_characteristic_by_128uuid, (uint8_t conn_id, uint16_t sh, uint16_t eh, uint8_t *uuid))
BLE_IPC_WEAK_STUB_ERR(bk_ble_discover_characteristic_descriptor, (uint8_t conn_id, uint16_t sh, uint16_t eh))
BLE_IPC_WEAK_STUB_ERR(bk_ble_gattc_read, (uint8_t con_idx, uint16_t att_handle, uint16_t offset))
BLE_IPC_WEAK_STUB_ERR(bk_ble_gattc_read_by_uuid, (uint8_t conn_id, uint16_t sh, uint16_t eh, uint8_t *uuid, uint8_t uuid_len))
BLE_IPC_WEAK_STUB_ERR(bk_ble_gattc_write, (uint8_t con_idx, uint16_t att_handle, uint8_t *data, uint16_t len, uint8_t is_write_cmd))
BLE_IPC_WEAK_STUB_ERR(bk_ble_clear_white_list, (void))
BLE_IPC_WEAK_STUB_ERR(bk_ble_add_devices_to_while_list, (bd_addr_t *addr, uint8_t addr_type))
BLE_IPC_WEAK_STUB_ERR(bk_ble_remove_devices_from_while_list, (bd_addr_t *addr, uint8_t addr_type))
BLE_IPC_WEAK_STUB_ERR(bk_ble_sec_send_auth_mode_ext, (uint8_t con_idx, uint8_t mode, uint8_t iocap, uint8_t sec_req, uint8_t oob, uint8_t initiator_key_distr, uint8_t responder_key_distr))
BLE_IPC_WEAK_STUB_ERR(bk_ble_host_register_hci_callback, (ble_hci_to_cp_cb cb))

__attribute__((weak)) uint8_t bk_ble_if_support_central(uint8_t *count)
{
    LOGW("%s not implemented\r\n", __func__);
    if (count != NULL) {
        *count = 0;
    }
    return 0;
}

#undef BLE_IPC_WEAK_STUB_ERR
#undef BLE_IPC_WEAK_STUB_U8
#undef BLE_IPC_WEAK_STUB_VOID
