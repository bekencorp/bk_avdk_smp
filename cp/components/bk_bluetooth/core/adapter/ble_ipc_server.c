#include <string.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "bt_ipc_core.h"
#include "../ipc/include/bt_ipc_vendor_opcode.h"
#include "bt_ipc_vendor_cmd_handler.h"
#include "ble_ipc_server.h"
#include "ble_api_5_x.h"
#include "bluetooth_legacy_include.h"

#define TAG "ble_ipc_server"
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

#ifndef UNKNOW_ACT_IDX
#define UNKNOW_ACT_IDX 0xFFU
#endif

#define BLE_IPC_DB_VALUE_OFFSET_NONE 0xFFFFU
#define BLE_IPC_VENDOR_PAYLOAD_MAX_LEN 253U
#define BLE_IPC_FRAG_HDR_LEN 8U
#define BLE_IPC_FRAG_CHUNK_MAX_LEN (BLE_IPC_VENDOR_PAYLOAD_MAX_LEN - BLE_IPC_FRAG_HDR_LEN)
#define BLE_IPC_FRAG_REASM_MAX_LEN 2048U
#define BLE_IPC_FRAG_FLAG_START 0x01U
#define BLE_IPC_FRAG_FLAG_END 0x02U
/* Max time bt_ipc_server_notice_cb() will block waiting for AP to (re)register
 * notice_cb after CP sends BT_VENDOR_SUB_OPCODE_AP_WAKEUP_TRIGGER. Must cover
 * an AP cold boot through to bk_ble_set_notice_cb() being called by AP app. */
#define BLE_IPC_NOTICE_READY_TIMEOUT_MS 5000U

typedef struct {
    uint8_t valid;
    uint8_t req_id;
    ble_cmd_t cmd;
} ble_ipc_pending_cmd_t;

typedef struct {
    uint8_t seq_id;
    uint16_t sub_opcode;
    uint16_t total_len;
    uint16_t received_len;
    uint8_t *payload;
} ble_ipc_frag_ctx_t;

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

typedef struct __attribute__((packed)) {
    uint8_t req_id;
    bd_addr_t addr;
} ble_ipc_addr_query_cmd_t;

typedef struct ble_ipc_server_db_ctx {
    struct ble_ipc_server_db_ctx *next;
    struct bk_ble_db_cfg db_cfg;
    ble_attm_desc_t *att_db;
} ble_ipc_server_db_ctx_t;

/* Minimal mode: CP only correlates one in-flight async BLE command. */
static ble_ipc_pending_cmd_t s_pending_cmd = {0};
static beken_mutex_t s_pending_mutex = NULL;
static ble_ipc_server_db_ctx_t *s_db_ctx_list = NULL;
static uint8_t s_frag_seq_id;
static ble_ipc_frag_ctx_t s_cmd_frag_ctx;

/* AP-side notice_cb registration tracking.
 *
 * s_notice_cb_ready == 1 iff AP has issued BT_VENDOR_SUB_OPCODE_BLE_SET_NOTICE
 * with payload[0] != 0 since the last clear. Cleared on:
 *   - SET_NOTICE(0) from AP (clean deregistration)
 *   - bt_ipc_on_ap_power_off_hook() (AP went down without DEINIT/SET_NOTICE(0))
 *
 * The semaphore is posted on every 0->1 transition so that any BLE-task thread
 * blocked in ble_ipc_server_notice_cb() can wake up and forward the event.
 */
static volatile uint8_t s_notice_cb_ready = 0;
static beken_semaphore_t s_notice_cb_ready_sema = NULL;

static void ble_ipc_server_release_db_ctx(uint8_t prf_id);

static void ble_ipc_server_init_pending_lock(void)
{
    if (s_pending_mutex == NULL) {
        rtos_init_mutex(&s_pending_mutex);
    }
}

static void ble_ipc_server_init_notice_ready(void)
{
    if (s_notice_cb_ready_sema == NULL) {
        rtos_init_semaphore(&s_notice_cb_ready_sema, 1);
    }
}

static void ble_ipc_server_mark_notice_cb_ready(uint8_t ready)
{
    ble_ipc_server_init_notice_ready();
    s_notice_cb_ready = ready ? 1 : 0;
    if (s_notice_cb_ready && s_notice_cb_ready_sema) {
        rtos_set_semaphore(&s_notice_cb_ready_sema);
    }
}

/* Strong override of the weak hook in bt_ipc_core.c. Called from
 * bt_ipc_notify_ap_power_off() when AP goes down outside the normal
 * SET_NOTICE(0) / DEINIT handshake. Drop the cached "AP has notice_cb"
 * flag so the next BLE notice will go through the query/wait dance.
 */
void bt_ipc_on_ap_power_off_hook(void)
{
    s_notice_cb_ready = 0;
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

static void ble_ipc_server_send_vendor_event_raw(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t *event = (uint8_t *)os_malloc(payload_len + 2);

    if (payload_len > BLE_IPC_VENDOR_PAYLOAD_MAX_LEN) {
        LOGW("%s, payload too long:%d\r\n", __func__, payload_len);
        return;
    }

    if (event == NULL) {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }

    event[0] = sub_opcode & 0xff;
    event[1] = sub_opcode >> 8;
    if ((payload_len > 0) && (payload != NULL)) {
        os_memcpy(&event[2], payload, payload_len);
    }

    bt_ipc_hci_send_vendor_event(event, payload_len + 2);
    os_free(event);
}

static void ble_ipc_server_send_vendor_event(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t offset = 0;
    uint8_t seq_id;

    if (payload_len <= BLE_IPC_VENDOR_PAYLOAD_MAX_LEN) {
        ble_ipc_server_send_vendor_event_raw(sub_opcode, payload, payload_len);
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

        ble_ipc_server_send_vendor_event_raw(BT_VENDOR_SUB_OPCODE_BLE_FRAG,
                                             frag_payload,
                                             (uint16_t)(BLE_IPC_FRAG_HDR_LEN + chunk_len));
        offset += chunk_len;
    }
}

static void ble_ipc_server_send_cmd_evt(uint8_t req_id, ble_cmd_t cmd, uint8_t actv_idx, ble_err_t status)
{
    uint8_t payload[3 + sizeof(status)];

    if (req_id == 0) {
        return;
    }

    payload[0] = req_id;
    payload[1] = (uint8_t)cmd;
    payload[2] = actv_idx;
    os_memcpy(&payload[3], &status, sizeof(status));
    ble_ipc_server_send_vendor_event(BT_VENDOR_SUB_OPCODE_BLE_CMD_EVT, payload, sizeof(payload));
}

static void ble_ipc_server_send_sync_rsp(uint8_t req_id, ble_err_t status, uint8_t value)
{
    uint8_t payload[3];

    payload[0] = req_id;
    payload[1] = (uint8_t)status;
    payload[2] = value;
    ble_ipc_server_send_vendor_event(BT_VENDOR_SUB_OPCODE_BLE_QUERY_RSP, payload, sizeof(payload));
}

static void ble_ipc_server_send_sync_data_rsp(uint8_t req_id, bt_err_t status, const uint8_t *data, uint8_t data_len)
{
    uint8_t payload[3 + BK_BLE_GAP_BD_ADDR_LEN] = {0};

    if (data_len > BK_BLE_GAP_BD_ADDR_LEN) {
        data_len = BK_BLE_GAP_BD_ADDR_LEN;
    }

    payload[0] = req_id;
    payload[1] = (uint8_t)status;
    payload[2] = data_len;
    if ((data_len > 0) && (data != NULL)) {
        os_memcpy(&payload[3], data, data_len);
    }

    ble_ipc_server_send_vendor_event(BT_VENDOR_SUB_OPCODE_BLE_QUERY_RSP,
                                     payload,
                                     (uint8_t)(3 + data_len));
}

static void ble_ipc_server_send_notice_evt(ble_notice_t notice, const void *data, uint16_t data_len)
{
    uint8_t *payload;

    payload = (uint8_t *)os_malloc(data_len + 4);
    if (payload == NULL) {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }

    payload[0] = notice & 0xff;
    payload[1] = notice >> 8;
    payload[2] = data_len & 0xff;
    payload[3] = data_len >> 8;
    if ((data_len > 0) && (data != NULL)) {
        os_memcpy(&payload[4], data, data_len);
    }

    ble_ipc_server_send_vendor_event(BT_VENDOR_SUB_OPCODE_BLE_NOTICE_EVT, payload, data_len + 4);
    os_free(payload);
}

static void ble_ipc_server_send_write_notice_evt(const ble_write_req_t *write_req)
{
    ble_ipc_write_req_evt_t header;
    uint8_t *payload;
    uint16_t payload_len;

    if ((write_req == NULL) || ((write_req->len > 0) && (write_req->value == NULL))) {
        return;
    }

    payload_len = (uint16_t)(sizeof(header) + write_req->len);
    payload = (uint8_t *)os_malloc(payload_len);
    if (payload == NULL) {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }

    header.conn_idx = write_req->conn_idx;
    header.prf_id = write_req->prf_id;
    header.att_idx = write_req->att_idx;
    header.len = write_req->len;
    os_memcpy(payload, &header, sizeof(header));
    if (write_req->len > 0) {
        os_memcpy(&payload[sizeof(header)], write_req->value, write_req->len);
    }

    ble_ipc_server_send_notice_evt(BLE_5_WRITE_EVENT, payload, payload_len);
    os_free(payload);
}

static void ble_ipc_server_send_read_notice_evt(const ble_read_req_t *read_req)
{
    ble_ipc_read_req_evt_t header;

    if (read_req == NULL) {
        return;
    }

    header.conn_idx = read_req->conn_idx;
    header.prf_id = read_req->prf_id;
    header.att_idx = read_req->att_idx;
    header.size = read_req->size;
    header.length = read_req->length;
    ble_ipc_server_send_notice_evt(BLE_5_READ_EVENT, &header, sizeof(header));
}


/* Block until AP has registered notice_cb on its side.
 *
 * If s_notice_cb_ready is already 1, returns BK_OK immediately. Otherwise,
 * sends BT_VENDOR_SUB_OPCODE_AP_WAKEUP_TRIGGER to AP. The send itself goes
 * through bt_ipc_mailbox_send_msg() which votes AP boot via bt_ipc_wakeup_ap()
 * when state != PEEP_READY, so a powered-down AP gets started as a side
 * effect; the opcode payload itself is intentionally a no-op on AP side.
 * Once AP's app layer (re-)calls bk_ble_set_notice_cb() as part of its
 * normal init, CP receives BT_VENDOR_SUB_OPCODE_BLE_SET_NOTICE and
 * ble_ipc_server_mark_notice_cb_ready(1) posts the semaphore.
 */
static int32_t ble_ipc_server_wait_notice_cb_ready(uint32_t timeout_ms)
{
    ble_ipc_server_init_notice_ready();

    if (s_notice_cb_ready) {
        return BK_OK;
    }

    /* Drain any stale post so the rtos_get_semaphore() below only succeeds on
     * the next genuine 0->1 transition. */
    if (s_notice_cb_ready_sema) {
        while (rtos_get_semaphore(&s_notice_cb_ready_sema, BEKEN_NO_WAIT) == BK_OK) {
        }
    }

    ble_ipc_server_send_vendor_event_raw(
        BT_VENDOR_SUB_OPCODE_AP_WAKEUP_TRIGGER, NULL, 0);

    if (s_notice_cb_ready_sema == NULL) {
        return BK_FAIL;
    }
    if (rtos_get_semaphore(&s_notice_cb_ready_sema, timeout_ms) != BK_OK) {
        return BK_FAIL;
    }
    return s_notice_cb_ready ? BK_OK : BK_FAIL;
}

static void ble_ipc_server_notice_cb(ble_notice_t notice, void *param)
{
    if (!s_notice_cb_ready) {
        if (ble_ipc_server_wait_notice_cb_ready(BLE_IPC_NOTICE_READY_TIMEOUT_MS) != BK_OK) {
            LOGW("ap notice_cb not ready, drop notice %d\n", (int)notice);
            return;
        }
    }

    switch (notice) {
    case BLE_5_CREATE_DB:
        if (param != NULL) {
            ble_create_db_t *db_ind = (ble_create_db_t *)param;
            if (db_ind->status == BK_ERR_BLE_SUCCESS) {
                ble_ipc_server_release_db_ctx(db_ind->prf_id);
            }
        }
        ble_ipc_server_send_notice_evt(notice, param, sizeof(ble_create_db_t));
        break;

    case BLE_5_CONNECT_EVENT:
        ble_ipc_server_send_notice_evt(notice, param, sizeof(ble_conn_ind_t));
        break;

    case BLE_5_DISCONNECT_EVENT:
        ble_ipc_server_send_notice_evt(notice, param, sizeof(ble_discon_ind_t));
        break;

    case BLE_5_MTU_CHANGE:
        ble_ipc_server_send_notice_evt(notice, param, sizeof(ble_mtu_change_t));
        break;

    case BLE_5_CONN_UPDATA_EVENT:
        ble_ipc_server_send_notice_evt(notice, param, sizeof(ble_conn_param_t));
        break;

    case BLE_5_GAP_CMD_CMP_EVENT:
        ble_ipc_server_send_notice_evt(notice, param, sizeof(ble_cmd_cmp_evt_t));
        break;

    case BLE_5_TX_DONE:
        ble_ipc_server_send_notice_evt(notice, param, sizeof(bk_ble_gatt_cmp_evt_t));
        break;

    case BLE_5_WRITE_EVENT:
        ble_ipc_server_send_write_notice_evt((const ble_write_req_t *)param);
        break;

    case BLE_5_READ_EVENT:
        ble_ipc_server_send_read_notice_evt((const ble_read_req_t *)param);
        break;

    default:
        LOGW("unhandled notice %d\n", (int)notice);
        break;
    }
}

static void ble_ipc_server_free_db_ctx(ble_ipc_server_db_ctx_t *db_ctx)
{
    uint8_t i;

    if (db_ctx == NULL) {
        return;
    }

    if (db_ctx->att_db != NULL) {
        for (i = 0; i < db_ctx->db_cfg.att_db_nb; i++) {
            if (db_ctx->att_db[i].p_value_context != NULL) {
                os_free(db_ctx->att_db[i].p_value_context);
            }
        }
        os_free(db_ctx->att_db);
    }

    os_free(db_ctx);
}

static ble_ipc_server_db_ctx_t *ble_ipc_server_decode_db_ctx(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_server_db_ctx_t *db_ctx;
    ble_ipc_db_cfg_t cfg_hdr;
    const uint8_t *value_base;
    const ble_ipc_att_desc_t *desc_payload;
    uint16_t descs_len;
    uint8_t i;

    if (payload_len < (uint16_t)(1 + sizeof(cfg_hdr))) {
        LOGW("%s,error 0\n", __func__);
        return NULL;
    }

    os_memcpy(&cfg_hdr, &payload[1], sizeof(cfg_hdr));
    descs_len = (uint16_t)(cfg_hdr.att_db_nb * sizeof(ble_ipc_att_desc_t));
    if ((cfg_hdr.att_db_nb == 0) || (payload_len < (uint16_t)(1 + sizeof(cfg_hdr) + descs_len))) {
        LOGW("%s,error 1\n", __func__);
        return NULL;
    }

    db_ctx = (ble_ipc_server_db_ctx_t *)os_malloc(sizeof(*db_ctx));
    if (db_ctx == NULL) {
        LOGW("%s,error 2\n", __func__);
        return NULL;
    }
    os_memset(db_ctx, 0, sizeof(*db_ctx));

    db_ctx->att_db = (ble_attm_desc_t *)os_malloc(cfg_hdr.att_db_nb * sizeof(ble_attm_desc_t));
    if (db_ctx->att_db == NULL) {
        os_free(db_ctx);
        LOGW("%s,error 3\n", __func__);
        return NULL;
    }
    os_memset(db_ctx->att_db, 0, cfg_hdr.att_db_nb * sizeof(ble_attm_desc_t));

    db_ctx->db_cfg.prf_task_id = cfg_hdr.prf_task_id;
    os_memcpy(db_ctx->db_cfg.uuid, cfg_hdr.uuid, sizeof(db_ctx->db_cfg.uuid));
    db_ctx->db_cfg.att_db_nb = cfg_hdr.att_db_nb;
    db_ctx->db_cfg.start_hdl = cfg_hdr.start_hdl;
    db_ctx->db_cfg.att_db = db_ctx->att_db;
    db_ctx->db_cfg.svc_perm = cfg_hdr.svc_perm;

    desc_payload = (const ble_ipc_att_desc_t *)&payload[1 + sizeof(cfg_hdr)];
    value_base = &payload[1 + sizeof(cfg_hdr) + descs_len];

    for (i = 0; i < cfg_hdr.att_db_nb; i++) {
        os_memcpy(db_ctx->att_db[i].uuid, desc_payload[i].uuid, sizeof(db_ctx->att_db[i].uuid));
        db_ctx->att_db[i].perm = desc_payload[i].perm;
        db_ctx->att_db[i].ext_perm = desc_payload[i].ext_perm;
        db_ctx->att_db[i].max_size = desc_payload[i].max_size;
        db_ctx->att_db[i].value_len = desc_payload[i].value_len;

        if (desc_payload[i].value_offset != BLE_IPC_DB_VALUE_OFFSET_NONE) {
            if ((desc_payload[i].value_len == 0) ||
                (desc_payload[i].value_offset > payload_len) ||
                ((uint32_t)desc_payload[i].value_offset + desc_payload[i].value_len >
                 (uint32_t)(payload_len - (1 + sizeof(cfg_hdr) + descs_len)))) {
                ble_ipc_server_free_db_ctx(db_ctx);
                LOGW("%s,error 4\n", __func__);
                return NULL;
            }

            db_ctx->att_db[i].p_value_context = os_malloc(desc_payload[i].value_len);
            if (db_ctx->att_db[i].p_value_context == NULL) {
                ble_ipc_server_free_db_ctx(db_ctx);
                LOGW("%s,error 5\n", __func__);
                return NULL;
            }

            os_memcpy(db_ctx->att_db[i].p_value_context,
                      &value_base[desc_payload[i].value_offset],
                      desc_payload[i].value_len);
        }
    }

    return db_ctx;
}

static void ble_ipc_server_track_db_ctx(ble_ipc_server_db_ctx_t *db_ctx)
{
    if (db_ctx == NULL) {
        return;
    }

    db_ctx->next = s_db_ctx_list;
    s_db_ctx_list = db_ctx;
}

static void ble_ipc_server_release_db_ctx(uint8_t prf_id)
{
    ble_ipc_server_db_ctx_t *db_ctx = s_db_ctx_list;
    ble_ipc_server_db_ctx_t *prev = NULL;

    while (db_ctx != NULL) {
        if (db_ctx->db_cfg.prf_task_id == prf_id) {
            ble_ipc_server_db_ctx_t *to_free = db_ctx;

            if (prev == NULL) {
                s_db_ctx_list = db_ctx->next;
            } else {
                prev->next = db_ctx->next;
            }

            db_ctx = db_ctx->next;
            ble_ipc_server_free_db_ctx(to_free);
            continue;
        }

        prev = db_ctx;
        db_ctx = db_ctx->next;
    }
}

static bk_err_t ble_ipc_server_prepare_pending(uint8_t req_id, ble_cmd_t cmd)
{
    if (req_id == 0) {
        return BK_OK;
    }

    ble_ipc_server_init_pending_lock();
    rtos_lock_mutex(&s_pending_mutex);
    if (s_pending_cmd.valid) {
        rtos_unlock_mutex(&s_pending_mutex);
        return BK_ERR_BLE_CMD_RUN;
    }

    s_pending_cmd.valid = 1;
    s_pending_cmd.req_id = req_id;
    s_pending_cmd.cmd = cmd;
    rtos_unlock_mutex(&s_pending_mutex);
    return BK_OK;
}

static void ble_ipc_server_clear_pending(void)
{
    ble_ipc_server_init_pending_lock();
    rtos_lock_mutex(&s_pending_mutex);
    os_memset(&s_pending_cmd, 0, sizeof(s_pending_cmd));
    rtos_unlock_mutex(&s_pending_mutex);
}

static uint8_t ble_ipc_server_take_pending_req(ble_cmd_t *cmd)
{
    uint8_t req_id = 0;

    ble_ipc_server_init_pending_lock();
    rtos_lock_mutex(&s_pending_mutex);
    if (s_pending_cmd.valid) {
        req_id = s_pending_cmd.req_id;
        if (cmd != NULL) {
            *cmd = s_pending_cmd.cmd;
        }
        os_memset(&s_pending_cmd, 0, sizeof(s_pending_cmd));
    }
    rtos_unlock_mutex(&s_pending_mutex);
    return req_id;
}

static void ble_ipc_server_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    ble_cmd_t pending_cmd = BLE_CMD_NONE;
    uint8_t req_id;

    if (param == NULL) {
        return;
    }

    req_id = ble_ipc_server_take_pending_req(&pending_cmd);
    ble_ipc_server_send_cmd_evt(req_id,
                                (pending_cmd != BLE_CMD_NONE) ? pending_cmd : cmd,
                                param->cmd_idx,
                                param->status);
}

static ble_cmd_cb_t ble_ipc_server_get_cmd_cb(uint8_t req_id)
{
    return req_id ? ble_ipc_server_cmd_cb : NULL;
}

static bk_err_t ble_ipc_server_finish_immediate(uint8_t req_id, ble_cmd_t cmd, uint8_t actv_idx, ble_err_t status)
{
    ble_ipc_server_send_cmd_evt(req_id, cmd, actv_idx, status);
    return status;
}

static bk_err_t ble_ipc_server_handle_create_adv(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    ble_adv_param_t adv_param;
    uint8_t actv_idx;
    ble_err_t ret;

    if (payload_len < (uint16_t)(2 + sizeof(ble_adv_param_t))) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];
    os_memcpy(&adv_param, &payload[2], sizeof(adv_param));

    ret = ble_ipc_server_prepare_pending(req_id, BLE_CREATE_ADV);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_CREATE_ADV, actv_idx, ret);
    }

    ret = bk_ble_create_advertising(actv_idx, &adv_param, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_CREATE_ADV, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_create_db(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_server_db_ctx_t *db_ctx;
    ble_err_t ret;

    db_ctx = ble_ipc_server_decode_db_ctx(payload, payload_len);
    if (db_ctx == NULL) {
        LOGW("ble_ipc_server_handle_create_db decode_db_ctx failed\n");
        return BK_ERR_PARAM;
    }

    ret = bk_ble_create_db(&db_ctx->db_cfg);
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_create_db_t db_evt = {0};

        db_evt.status = (uint8_t)ret;
        db_evt.prf_id = (uint8_t)db_ctx->db_cfg.prf_task_id;
        db_evt.start_hdl = 0;
        ble_ipc_server_send_notice_evt(BLE_5_CREATE_DB, &db_evt, sizeof(db_evt));
        ble_ipc_server_free_db_ctx(db_ctx);
        return ret;
    }

    ble_ipc_server_track_db_ctx(db_ctx);
    return BK_ERR_BLE_SUCCESS;
}

static bk_err_t ble_ipc_server_handle_set_adv_data(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t actv_idx;
    uint8_t data_len;
    uint8_t adv_data[BK_BLE_MAX_ADV_DATA_LEN];
    ble_err_t ret;

    if (payload_len < 3) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];
    data_len = payload[2];
    if ((data_len > BK_BLE_MAX_ADV_DATA_LEN) || (payload_len < (uint16_t)(3 + data_len))) {
        return BK_ERR_PARAM;
    }

    if (data_len > 0) {
        os_memcpy(adv_data, &payload[3], data_len);
    }

    ret = ble_ipc_server_prepare_pending(req_id, BLE_SET_ADV_DATA);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_SET_ADV_DATA, actv_idx, ret);
    }

    ret = bk_ble_set_adv_data(actv_idx, adv_data, data_len, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_SET_ADV_DATA, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_set_scan_rsp_data(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t actv_idx;
    uint8_t data_len;
    uint8_t scan_rsp_data[BK_BLE_MAX_ADV_DATA_LEN];
    ble_err_t ret;

    if (payload_len < 3) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];
    data_len = payload[2];
    if ((data_len > BK_BLE_MAX_ADV_DATA_LEN) || (payload_len < (uint16_t)(3 + data_len))) {
        return BK_ERR_PARAM;
    }

    if (data_len > 0) {
        os_memcpy(scan_rsp_data, &payload[3], data_len);
    }

    ret = ble_ipc_server_prepare_pending(req_id, BLE_SET_RSP_DATA);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_SET_RSP_DATA, actv_idx, ret);
    }

    ret = bk_ble_set_scan_rsp_data(actv_idx, scan_rsp_data, data_len, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_SET_RSP_DATA, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_start_adv(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t actv_idx;
    uint16_t duration;
    ble_err_t ret;

    if (payload_len < 4) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];
    os_memcpy(&duration, &payload[2], sizeof(duration));

    ret = ble_ipc_server_prepare_pending(req_id, BLE_START_ADV);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_START_ADV, actv_idx, ret);
    }

    ret = bk_ble_start_advertising(actv_idx, duration, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_START_ADV, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_stop_adv(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t actv_idx;
    ble_err_t ret;

    if (payload_len < 2) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];

    ret = ble_ipc_server_prepare_pending(req_id, BLE_STOP_ADV);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_STOP_ADV, actv_idx, ret);
    }

    ret = bk_ble_stop_advertising(actv_idx, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_STOP_ADV, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_delete_adv(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t actv_idx;
    ble_err_t ret;

    if (payload_len < 2) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];

    ret = ble_ipc_server_prepare_pending(req_id, BLE_DELETE_ADV);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_DELETE_ADV, actv_idx, ret);
    }

    ret = bk_ble_delete_advertising(actv_idx, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_DELETE_ADV, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_set_random_addr(uint8_t req_id, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t actv_idx;
    uint8_t addr[BK_BLE_GAP_BD_ADDR_LEN];
    ble_err_t ret;

    if (payload_len < (uint16_t)(2 + BK_BLE_GAP_BD_ADDR_LEN)) {
        return BK_ERR_PARAM;
    }

    actv_idx = payload[1];
    os_memcpy(addr, &payload[2], sizeof(addr));

    ret = ble_ipc_server_prepare_pending(req_id, BLE_SET_ADV_RANDOM_ADDR);
    if (ret != BK_OK) {
        return ble_ipc_server_finish_immediate(req_id, BLE_SET_ADV_RANDOM_ADDR, actv_idx, ret);
    }

    ret = bk_ble_set_adv_random_addr(actv_idx, addr, ble_ipc_server_get_cmd_cb(req_id));
    if (ret != BK_ERR_BLE_SUCCESS) {
        ble_ipc_server_clear_pending();
        ble_ipc_server_send_cmd_evt(req_id, BLE_SET_ADV_RANDOM_ADDR, actv_idx, ret);
    }

    return ret;
}

static bk_err_t ble_ipc_server_handle_set_notice(const uint8_t *payload, uint16_t payload_len)
{
    if (payload_len < 1) {
        LOGW("ble_ipc_server_handle_set_notice payload_len < 1\n");
        return BK_ERR_PARAM;
    }

    bk_ble_set_notice_cb(payload[0] ? ble_ipc_server_notice_cb : NULL);
    ble_ipc_server_mark_notice_cb_ready(payload[0]);
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_get_idle_actv_idx(const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id;

    if (payload_len < 1) {
        return BK_ERR_PARAM;
    }

    req_id = payload[0];
    /* Query the stack directly instead of relying on a CP-side cached state. */
    ble_ipc_server_send_sync_rsp(req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_get_idle_actv_idx_handle());
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_get_max_actv_idx(const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id;

    if (payload_len < 1) {
        return BK_ERR_PARAM;
    }

    req_id = payload[0];
    /* Query the stack directly instead of relying on a CP-side cached state. */
    ble_ipc_server_send_sync_rsp(req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_get_max_actv_idx_count());
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_get_max_conn_idx(const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id;

    if (payload_len < 1) {
        return BK_ERR_PARAM;
    }

    req_id = payload[0];
    ble_ipc_server_send_sync_rsp(req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_get_max_conn_idx_count());
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_find_conn_idx_from_addr(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_addr_query_cmd_t cmd;

    if (payload_len < sizeof(cmd)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&cmd, payload, sizeof(cmd));
    ble_ipc_server_send_sync_rsp(cmd.req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_find_conn_idx_from_addr(&cmd.addr));
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_find_actv_state_idx(const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id;
    uint8_t state;

    if (payload_len < 2) {
        return BK_ERR_PARAM;
    }

    req_id = payload[0];
    state = payload[1];
    ble_ipc_server_send_sync_rsp(req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_find_actv_state_idx_handle(state));
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_find_master_state_idx(const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id;
    uint8_t state;

    if (payload_len < 2) {
        return BK_ERR_PARAM;
    }

    req_id = payload[0];
    state = payload[1];
    ble_ipc_server_send_sync_rsp(req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_find_master_state_idx_handle(state));
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_get_bt_address(const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id;
    uint8_t addr[BK_BLE_GAP_BD_ADDR_LEN] = {0};
    bt_err_t status;

    if (payload_len < 1) {
        return BK_ERR_PARAM;
    }

    req_id = payload[0];
    status = bk_bluetooth_get_address(addr);
    ble_ipc_server_send_sync_data_rsp(req_id,
                                      status,
                                      (status == BT_OK) ? addr : NULL,
                                      (status == BT_OK) ? BK_BLE_GAP_BD_ADDR_LEN : 0);
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_get_connect_state(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_addr_query_cmd_t cmd;

    if (payload_len < sizeof(cmd)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&cmd, payload, sizeof(cmd));
    ble_ipc_server_send_sync_rsp(cmd.req_id,
                                 BK_ERR_BLE_SUCCESS,
                                 bk_ble_get_connect_state(&cmd.addr));
    return BK_OK;
}

static bk_err_t ble_ipc_server_handle_value_cmd(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_value_cmd_t header;

    if (payload_len < sizeof(header)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&header, payload, sizeof(header));
    if ((header.len > 0) && (payload_len < (uint16_t)(sizeof(header) + header.len))) {
        return BK_ERR_PARAM;
    }

    switch (sub_opcode) {
    case BT_VENDOR_SUB_OPCODE_BLE_READ_RESPONSE_VALUE:
        return bk_ble_read_response_value(header.conn_idx,
                                          header.len,
                                          (header.len > 0) ? (uint8_t *)&payload[sizeof(header)] : NULL,
                                          header.prf_id,
                                          header.att_idx);

    case BT_VENDOR_SUB_OPCODE_BLE_SEND_NOTI_VALUE:
        return bk_ble_send_noti_value(header.conn_idx,
                                      header.len,
                                      (header.len > 0) ? (uint8_t *)&payload[sizeof(header)] : NULL,
                                      header.prf_id,
                                      header.att_idx);

    case BT_VENDOR_SUB_OPCODE_BLE_SEND_IND_VALUE:
        return bk_ble_send_ind_value(header.conn_idx,
                                     header.len,
                                     (header.len > 0) ? (uint8_t *)&payload[sizeof(header)] : NULL,
                                     header.prf_id,
                                     header.att_idx);

    default:
        return BK_ERR_NOT_FOUND;
    }
}

static bk_err_t ble_ipc_server_handle_update_param(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_update_param_cmd_t cmd;

    if (payload_len < sizeof(cmd)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&cmd, payload, sizeof(cmd));
    return bk_ble_update_param(cmd.conn_idx, &cmd.conn_param);
}

static bk_err_t ble_ipc_server_handle_gatt_mtu_change(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_conn_idx_cmd_t cmd;

    if (payload_len < sizeof(cmd)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&cmd, payload, sizeof(cmd));
    return bk_ble_gatt_mtu_change(cmd.conn_idx);
}

static bk_err_t ble_ipc_server_handle_set_max_mtu(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_set_max_mtu_cmd_t cmd;

    if (payload_len < sizeof(cmd)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&cmd, payload, sizeof(cmd));
    return bk_ble_set_max_mtu(cmd.max_mtu);
}

static bk_err_t ble_ipc_server_handle_disconnect(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_conn_idx_cmd_t cmd;

    if (payload_len < sizeof(cmd)) {
        return BK_ERR_PARAM;
    }

    os_memcpy(&cmd, payload, sizeof(cmd));
    return bk_ble_disconnect(cmd.conn_idx);
}

static void ble_ipc_server_reset_frag_ctx(ble_ipc_frag_ctx_t *ctx)
{
    if (ctx->payload != NULL) {
        os_free(ctx->payload);
    }
    os_memset(ctx, 0, sizeof(*ctx));
}

static bk_err_t ble_ipc_server_handle_frag_cmd(const uint8_t *payload, uint16_t payload_len)
{
    ble_ipc_frag_ctx_t *ctx = &s_cmd_frag_ctx;
    uint8_t seq_id;
    uint8_t flags;
    uint16_t sub_opcode;
    uint16_t total_len;
    uint16_t offset;
    uint16_t chunk_len;

    if ((payload == NULL) || (payload_len < BLE_IPC_FRAG_HDR_LEN)) {
        return BK_ERR_PARAM;
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
        ble_ipc_server_reset_frag_ctx(ctx);
        return BK_ERR_PARAM;
    }

    if ((flags & BLE_IPC_FRAG_FLAG_START) != 0) {
        ble_ipc_server_reset_frag_ctx(ctx);
        ctx->payload = (uint8_t *)os_malloc(total_len);
        if (ctx->payload == NULL) {
            LOGW("%s, malloc failed\r\n", __func__);
            return BK_ERR_NO_MEM;
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
        ble_ipc_server_reset_frag_ctx(ctx);
        return BK_ERR_PARAM;
    }

    os_memcpy(&ctx->payload[offset], &payload[BLE_IPC_FRAG_HDR_LEN], chunk_len);
    ctx->received_len += chunk_len;

    if ((flags & BLE_IPC_FRAG_FLAG_END) != 0) {
        uint8_t *full_payload = ctx->payload;
        uint16_t full_len = ctx->received_len;
        uint16_t full_sub_opcode = ctx->sub_opcode;
        bk_err_t ret = BK_ERR_PARAM;

        ctx->payload = NULL;
        os_memset(ctx, 0, sizeof(*ctx));

        if (full_len == total_len) {
            ret = ble_ipc_server_dispatch_vendor_cmd(full_sub_opcode, full_payload, full_len);
        }

        os_free(full_payload);
        return ret;
    }

    return BK_OK;
}

bk_err_t ble_ipc_server_dispatch_vendor_cmd(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t req_id = (payload_len > 0) ? payload[0] : 0;

    switch (sub_opcode) {
    case BT_VENDOR_SUB_OPCODE_BLE_FRAG:
        return ble_ipc_server_handle_frag_cmd(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_CREATE_DB:
        return ble_ipc_server_handle_create_db(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_CREATE_ADV:
        return ble_ipc_server_handle_create_adv(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_SET_ADV_DATA:
        return ble_ipc_server_handle_set_adv_data(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_SET_SCAN_RSP_DATA:
        return ble_ipc_server_handle_set_scan_rsp_data(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_START_ADV:
        return ble_ipc_server_handle_start_adv(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_STOP_ADV:
        return ble_ipc_server_handle_stop_adv(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_DELETE_ADV:
        return ble_ipc_server_handle_delete_adv(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_SET_ADV_RANDOM_ADDR:
        return ble_ipc_server_handle_set_random_addr(req_id, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_SET_NOTICE:
        return ble_ipc_server_handle_set_notice(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_GET_IDLE_ACTV_IDX:
        return ble_ipc_server_handle_get_idle_actv_idx(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_GET_MAX_ACTV_IDX:
        return ble_ipc_server_handle_get_max_actv_idx(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_GET_MAX_CONN_IDX:
        return ble_ipc_server_handle_get_max_conn_idx(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_FIND_CONN_IDX_FROM_ADDR:
        return ble_ipc_server_handle_find_conn_idx_from_addr(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_FIND_ACTV_STATE_IDX:
        return ble_ipc_server_handle_find_actv_state_idx(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_FIND_MASTER_STATE_IDX:
        return ble_ipc_server_handle_find_master_state_idx(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_GET_BT_ADDRESS:
        return ble_ipc_server_handle_get_bt_address(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_GET_CONNECT_STATE:
        return ble_ipc_server_handle_get_connect_state(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_READ_RESPONSE_VALUE:
    case BT_VENDOR_SUB_OPCODE_BLE_SEND_NOTI_VALUE:
    case BT_VENDOR_SUB_OPCODE_BLE_SEND_IND_VALUE:
        return ble_ipc_server_handle_value_cmd(sub_opcode, payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_UPDATE_PARAM:
        return ble_ipc_server_handle_update_param(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_GATT_MTU_CHANGE:
        return ble_ipc_server_handle_gatt_mtu_change(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_SET_MAX_MTU:
        return ble_ipc_server_handle_set_max_mtu(payload, payload_len);

    case BT_VENDOR_SUB_OPCODE_BLE_DISCONNECT:
        return ble_ipc_server_handle_disconnect(payload, payload_len);

    default:
        return BK_ERR_NOT_FOUND;
    }
}
