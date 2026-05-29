#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>
#include "bt_ipc_core.h"
#include "bt_ipc_vendor_cmd_handler.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>

#define TAG  "bt_ipc"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t state;
    beken_thread_t thd;
    beken_queue_t queue;
    beken_semaphore_t send_sema;
    beken_semaphore_t ap_ble_ready_sema;
}bt_ipc_t;


bt_ipc_t bt_ipc_env = {
    .state = BT_IPC_STATE_IDLE,
    .thd = NULL,
    .queue = NULL,
    .send_sema = NULL,
    .ap_ble_ready_sema = NULL,
};

static bt_hci_send_cb_t s_bt_ipc_hci_send_cb = NULL;

#define BT_IPC_CMD_CHNL     MB_CHNL_BT_CMD
#define BT_IPC_SEND_TIMEOUT_MS  4000

#define HCI_COMMAND_COMPLETE_EVT_CODE    0x0E
#define HCI_VENDOR_EVT_CODE    0xFE
#define HCI_VENDOR_OPCODE      0xFEFE

enum
{
    BT_IPC_MSG_NULL = 0,
    BT_IPC_CMD_IND_MSG = 1,
    BT_IPC_EVNET_IND_MSG = 2,
    BT_IPC_FREE_MSG = 3,
    BT_IPC_EXIT_MSG = 4,
    BT_IPC_ACL_IND_MSG = 5,
    BT_IPC_SCO_IND_MSG = 6,
};

#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
static int32_t bt_ipc_wakeup_ap(void)
{
    bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);
    return BK_OK;
}
#endif

void bt_ipc_set_state(uint8_t state)
{
    uint8_t prev = bt_ipc_env.state;

    bt_ipc_env.state = state;

    /* On any transition INTO PEEP_READY (AP just confirmed alive via INIT
     * vendor cmd), release any CP thread that voted AP boot and is now
     * blocked in bt_ipc_wait_ap_ble_ready.
     */
    if (state == BT_IPC_STATE_PEEP_READY && prev != BT_IPC_STATE_PEEP_READY) {
        if (bt_ipc_env.ap_ble_ready_sema) {
            rtos_set_semaphore(&bt_ipc_env.ap_ble_ready_sema);
        }
    }
}

uint8_t bt_ipc_get_state(void)
{
    return bt_ipc_env.state;
}

/* Weak hook invoked from bt_ipc_notify_ap_power_off(). Adapters that hold
 * per-peer cached state (e.g. ble_ipc_server's notice_cb_ready flag) can
 * override this symbol to be told about an external AP power-off without
 * having bt_ipc_core depend on them directly. Always built so generic
 * callers do not need to gate on CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL.
 */
__attribute__((weak)) void bt_ipc_on_ap_power_off_hook(void)
{
}

static void bt_ipc_notify_ap_power_off(void *arg)
{
    (void)arg;
    LOGD("bt_ipc_notify_ap_power_off\n");
    bt_ipc_on_ap_power_off_hook();
    if (bt_ipc_env.state == BT_IPC_STATE_PEEP_READY) {
        bt_ipc_set_state(BT_IPC_STATE_LOCAL_READY);
    }
}

static void bt_ipc_free_local_msg_payload(hci_hdr_t *msg)
{
    if ((msg->pkt_type != HCI_FREE_PKT) && msg->hdr_ptr) {
        os_free((void *)(uintptr_t)msg->hdr_ptr);
        msg->hdr_ptr = 0;
    }
}

#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
static int32_t bt_ipc_wait_ap_ble_ready(uint32_t timeout_ms)
{
    int32_t ret;

    /* Drain any stale post on the ready sema so the rtos_get_semaphore()
     * below only returns when the next state-machine transition to
     * BT_IPC_STATE_PEEP_READY actually happens.
     */
    if (bt_ipc_env.ap_ble_ready_sema) {
        while (rtos_get_semaphore(&bt_ipc_env.ap_ble_ready_sema, BEKEN_NO_WAIT) == BK_OK) {
        }
    }

    ret = bt_ipc_wakeup_ap();
    if (ret != BK_OK) {
        LOGW("wake AP BLE failed, ret:%d\n", ret);
        return ret;
    }

    ret = rtos_get_semaphore(&bt_ipc_env.ap_ble_ready_sema, timeout_ms);
    if (ret != BK_OK) {
        LOGW("wait AP BLE ready timeout, ret:%d\n", ret);
        return ret;
    }

    return BK_OK;
}
#endif

static void bt_ipc_mailbox_rx_isr(void *param, void *cmd_buf)
{
    hci_hdr_t *hci_hdr = (hci_hdr_t *)cmd_buf;
    switch(hci_hdr->pkt_type) {
        case HCI_COMMAND_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_CMD_IND_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGW("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        case HCI_EVENT_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_EVNET_IND_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGW("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        case HCI_ACL_DATA_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_ACL_IND_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGW("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        case HCI_SCO_DATA_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_SCO_IND_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGW("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        case HCI_FREE_PKT:
        {
            bt_ipc_msg_t bt_ipc_msg;

            bt_ipc_msg.type = BT_IPC_FREE_MSG;
            bt_ipc_msg.param = hci_hdr->hdr_ptr;

            int rc = rtos_push_to_queue(&bt_ipc_env.queue, &bt_ipc_msg, BEKEN_NO_WAIT);

            if (kNoErr != rc)
            {
                LOGW("%s, send queue failed\r\n", __func__);
            }
        }
        break;

        default:
            LOGW("%s, unknown type %d\r\n", __func__,hci_hdr->pkt_type);
            break;

    }

}

static void bt_ipc_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
    rtos_set_semaphore(&bt_ipc_env.send_sema);
}

static void bt_ipc_mailbox_send_msg(hci_hdr_t *msg)
{
    /* Fast path: peer (AP) has already confirmed it is alive via INIT vendor
     * cmd, mailbox can be written directly.
     *
     * Slow path: state is IDLE (bt_ipc_init not yet done) or LOCAL_READY
     * (AP has never talked to us yet since bt_ipc_init, or has DEINIT-ed):
     *   - AP_PWD_ALL build  : vote AP boot and block until the state machine
     *                         transitions to PEEP_READY (driven by AP's INIT
     *                         vendor cmd in bt_ipc_vendor_cmd_init_cb).
     *   - non-AP_PWD_ALL    : no wakeup mechanism available, just drop.
     */
    if (bt_ipc_env.state != BT_IPC_STATE_PEEP_READY)
    {
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
        int32_t ret = bt_ipc_wait_ap_ble_ready(BT_IPC_SEND_TIMEOUT_MS);
        if (ret != BK_OK)
        {
            LOGW("%s BLE AP is not ready, drop pkt type %d\r\n", __func__, msg->pkt_type);
            bt_ipc_free_local_msg_payload(msg);
            return;
        }
#else
        LOGW("%s bt ipc not ready (state %d), drop pkt type %d\r\n", __func__, bt_ipc_env.state, msg->pkt_type);
        bt_ipc_free_local_msg_payload(msg);
        return;
#endif
    }

    bt_ipc_cmd_t bt_ipc_cmd;
    int ret = BK_OK;

    bt_ipc_cmd.hci_hdr = *msg;

    ret = rtos_get_semaphore(&bt_ipc_env.send_sema, BT_IPC_SEND_TIMEOUT_MS);
    if (ret != BK_OK)
    {
        LOGW("get bt ipc send_sema failed\n");
    }

    ret = mb_chnl_write(BT_IPC_CMD_CHNL, (mb_chnl_cmd_t*)&bt_ipc_cmd);
    if (ret != BK_OK)
    {
        LOGW("mb_chnl_write failed\n");
        bt_ipc_free_local_msg_payload(msg);
        return;
    }
}

void bt_ipc_hci_send_vendor_cmd(uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(cmd_hdr_t) + len;
    cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)os_malloc(data_len);

    //LOGD("malloc ptr %p\n",cmd_hdr);

    if (cmd_hdr == NULL)
    {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }
    cmd_hdr->opcode = HCI_VENDOR_OPCODE;
    cmd_hdr->param_len = len;
    os_memcpy(cmd_hdr->param, data, len);

    msg.pkt_type = HCI_COMMAND_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)cmd_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_send_vendor_event(uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(event_hdr_t) + len;
    event_hdr_t *event_hdr = (event_hdr_t *)os_malloc(data_len);

    //LOGD("malloc ptr %p\n",event_hdr);

    if (event_hdr == NULL)
    {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }
    event_hdr->event_code = HCI_VENDOR_EVT_CODE;
    event_hdr->param_len = len;
    os_memcpy(event_hdr->param, data, len);

    msg.pkt_type = HCI_EVENT_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)event_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_send_event(uint8_t event_code, uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(event_hdr_t) + len;
    event_hdr_t *event_hdr = (event_hdr_t *)os_malloc(data_len);

    //LOGD("malloc ptr %p\n",event_hdr);

    if (event_hdr == NULL)
    {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }
    event_hdr->event_code = event_code;
    event_hdr->param_len = len;
    os_memcpy(event_hdr->param, data, len);

    msg.pkt_type = HCI_EVENT_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)event_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_send_acl_data(uint16_t hdl_flags, uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(acl_hdr_t) + len;
    acl_hdr_t *acl_hdr = (acl_hdr_t *)os_malloc(data_len);

    //LOGD("malloc ptr %p\n",acl_hdr);

    if (acl_hdr == NULL)
    {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }
    acl_hdr->hdl_flags = hdl_flags;
    acl_hdr->datalen = len;
    os_memcpy(acl_hdr->param, data, len);

    msg.pkt_type = HCI_ACL_DATA_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)acl_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_send_sco_data(uint16_t hdl_flags, uint8_t *data, uint16_t len)
{
    hci_hdr_t msg;

    uint16_t data_len = sizeof(sco_hdr_t) + len;
    sco_hdr_t *sco_hdr = (sco_hdr_t *)os_malloc(data_len);

    //LOGD("malloc ptr %p\n",sco_hdr);

    if (sco_hdr == NULL)
    {
        LOGW("%s, malloc failed\r\n", __func__);
        return;
    }
    sco_hdr->conhdl_psf = hdl_flags;
    sco_hdr->datalen = len;
    os_memcpy(sco_hdr->param, data, len);

    msg.pkt_type = HCI_SCO_DATA_PKT;
    msg.hdr_ptr = (uint32_t)(uintptr_t)sco_hdr;

    bt_ipc_mailbox_send_msg(&msg);
}

void bt_ipc_hci_free_pkt(uint32_t ptr)
{
    hci_hdr_t msg;

    msg.pkt_type = HCI_FREE_PKT;
    msg.hdr_ptr = ptr;

    bt_ipc_mailbox_send_msg(&msg);
}

static void bt_ipc_mailbox_config(uint8_t channel)
{
    bk_err_t ret;

    LOGD("open channel: %d on CPU\n", channel);
    /* reigster a mailbox logical channel */
    ret = mb_chnl_open(channel, NULL);
    if (ret != BK_OK) {
        LOGW(" mb_chnl_open open fail \r\n");
        return;
    }
    /* register mailbox logical channel rx callbcak */
    mb_chnl_ctrl(channel, MB_CHNL_SET_RX_ISR, bt_ipc_mailbox_rx_isr);
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_ISR, NULL);
    /* register mailbox logical channel rx complete callback */
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_CMPL_ISR, bt_ipc_mailbox_tx_cmpl_isr);
}

static void bt_ipc_message_handle(void)
{
    bk_err_t ret = BK_OK;
    bt_ipc_msg_t msg;

    while (1)
    {

        ret = rtos_pop_from_queue(&bt_ipc_env.queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            switch (msg.type)
            {
                case BT_IPC_CMD_IND_MSG:
                {
                    //LOGD("BT_IPC_CMD_IND_MSG\n");
                    cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)(uintptr_t)msg.param;
                    //LOGD("opcode 0x%04x, param_len %d\n",cmd_hdr->opcode, cmd_hdr->param_len);
                    if (cmd_hdr->opcode == HCI_VENDOR_OPCODE)
                    {
                        bk_err_t dispatch_ret = bt_ipc_vendor_cmd_handler_dispatch(cmd_hdr);
                        if (dispatch_ret != BK_OK)
                        {
                            LOGW("%s, vendor command dispatch failed, ret:%d\r\n", __func__, dispatch_ret);
                        }
                    }
                    else
                    {
                        if (s_bt_ipc_hci_send_cb)
                        {
                            uint16_t cmd_len = sizeof(cmd_hdr_t) + cmd_hdr->param_len + 1;
                            uint8_t *p_cmd_data = (uint8_t *)os_malloc(cmd_len);
                            if (!p_cmd_data)
                            {
                                LOGW("%s, malloc p_cmd_data failed\r\n", __func__);
                            }
                            else
                            {
                                p_cmd_data[0] = HCI_COMMAND_PKT;
                                os_memcpy(p_cmd_data + 1, (uint8_t *)(uintptr_t)msg.param, cmd_len - 1);
                                s_bt_ipc_hci_send_cb(p_cmd_data, cmd_len);
                                os_free(p_cmd_data);
                            }
                        }
                    }
                    bt_ipc_hci_free_pkt(msg.param);
                }
                break;

                case BT_IPC_EVNET_IND_MSG:
                {
                    //LOGD("BT_IPC_EVNET_IND_MSG\n");
                    //event_hdr_t *event_hdr = (event_hdr_t *)(uintptr_t)msg.param;
                    //LOGD("evt_code 0x%02x, param_len %d\n",event_hdr->event_code, event_hdr->param_len);
                    bt_ipc_hci_free_pkt(msg.param);
                }
                break;

                case BT_IPC_ACL_IND_MSG:
                {
                    //LOGD("BT_IPC_ACL_IND_MSG\n");
                    acl_hdr_t *acl_hdr = (acl_hdr_t *)(uintptr_t)msg.param;
                    //LOGD("hdl_flags 0x%04x, param_len %d\n",acl_hdr->hdl_flags, acl_hdr->datalen);
                    if (s_bt_ipc_hci_send_cb)
                    {
                        uint16_t acl_data_len = sizeof(acl_hdr_t) + acl_hdr->datalen + 1;
                        uint8_t *p_acl_data = (uint8_t *)os_malloc(acl_data_len);
                        if (!p_acl_data)
                        {
                            LOGW("%s, malloc p_acl_data failed\r\n", __func__);
                        }
                        else
                        {
                            p_acl_data[0] = HCI_ACL_DATA_PKT;
                            os_memcpy(p_acl_data + 1, (uint8_t *)(uintptr_t)msg.param, acl_data_len - 1);
                            s_bt_ipc_hci_send_cb(p_acl_data, acl_data_len);
                            os_free(p_acl_data);
                        }
                    }
                    bt_ipc_hci_free_pkt(msg.param);
                }
                break;

                case BT_IPC_SCO_IND_MSG:
                {
                    //LOGD("BT_IPC_SCO_IND_MSG\n");
                    sco_hdr_t *sco_hdr = (sco_hdr_t *)(uintptr_t)msg.param;
                    //LOGD("hdl_flags 0x%04x, param_len %d\n",sco_hdr->conhdl_psf, sco_hdr->datalen);
                    if (s_bt_ipc_hci_send_cb)
                    {
                        uint16_t sco_data_len = sizeof(sco_hdr_t) + sco_hdr->datalen + 1;
                        uint8_t *p_sco_data = (uint8_t *)os_malloc(sco_data_len);
                        if (!p_sco_data)
                        {
                            LOGW("%s, malloc p_sco_data failed\r\n", __func__);
                        }
                        else
                        {
                            p_sco_data[0] = HCI_SCO_DATA_PKT;
                            os_memcpy(p_sco_data + 1, (uint8_t *)(uintptr_t)msg.param, sco_data_len - 1);
                            s_bt_ipc_hci_send_cb(p_sco_data, sco_data_len);
                            os_free(p_sco_data);
                        }
                    }
                    bt_ipc_hci_free_pkt(msg.param);
                }
                break;

                case BT_IPC_FREE_MSG:
                {
                    //LOGD("BT_IPC_FREE_MSG\n");
                    //LOGD("free ptr %p\n",msg.param);
                    os_free((void*)(uintptr_t)msg.param);
                }
                break;

                case BT_IPC_EXIT_MSG:
                    goto exit;

                default:
                    break;
            }
        }
    }

exit:

    /* delate msg queue */
    ret = rtos_deinit_queue(&bt_ipc_env.queue);

    if (ret != kNoErr)
    {
        LOGW("delete message queue fail\n");
    }

    bt_ipc_env.queue = NULL;

    LOGW("delete message queue complete\n");

    /* delate task */
    rtos_delete_thread(NULL);

    bt_ipc_env.thd = NULL;

    LOGW("delete task complete\n");
}

void bt_ipc_register_hci_send_callback(bt_hci_send_cb_t cb)
{
    s_bt_ipc_hci_send_cb = cb;
}

int32_t bt_ipc_init(void)
{
    if (BT_IPC_STATE_IDLE != bt_ipc_env.state)
    {
        LOGW("%s bt ipc already initialised\r\n", __func__);
        return 1;
    }

    bk_err_t ret;

    ret = rtos_init_queue(&bt_ipc_env.queue,
                          "bt_ipc_queue",
                          sizeof(bt_ipc_msg_t),
                          BT_IPC_QUEUE_LEN);

    if (ret != BK_OK)
    {
        LOGW("%s, create bt ipc queue failed\n", __func__);
        return -1;
    }

    ret = rtos_create_thread(&bt_ipc_env.thd,
                             BT_IPC_TASK_PRIO,
                             "bt_ipc_thd",
                             (beken_thread_function_t)bt_ipc_message_handle,
                             2048,
                             NULL);

    if (ret != BK_OK)
    {
        LOGW("create bt ipc thread fail\n");
        rtos_deinit_queue(&bt_ipc_env.queue);
        bt_ipc_env.queue = NULL;
        return -1;
    }

    /* register a mailbox channel */
    bt_ipc_mailbox_config(BT_IPC_CMD_CHNL);

    /* init semaphore */
    ret = rtos_init_semaphore_ex(&bt_ipc_env.send_sema, 1, 1);
    if (ret != BK_OK) {
        LOGW("init send_sema fail!\r\n");
        rtos_deinit_queue(&bt_ipc_env.queue);
        bt_ipc_env.queue = NULL;
        rtos_delete_thread(bt_ipc_env.thd);
        bt_ipc_env.thd = NULL;
        return -1;
    }

    ret = rtos_init_semaphore(&bt_ipc_env.ap_ble_ready_sema, 1);
    if (ret != BK_OK) {
        LOGW("init ap_ble_ready_sema fail!\r\n");
        rtos_deinit_queue(&bt_ipc_env.queue);
        bt_ipc_env.queue = NULL;
        rtos_delete_thread(bt_ipc_env.thd);
        bt_ipc_env.thd = NULL;
        rtos_deinit_semaphore(&bt_ipc_env.send_sema);
        bt_ipc_env.send_sema = NULL;
        return -1;
    }

    bt_ipc_env.state = BT_IPC_STATE_LOCAL_READY;

    #if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
    bk_pm_ap_ctrl_callback_register(bt_ipc_notify_ap_power_off, NULL);
    #endif

    LOGD("%s success\n", __func__);

    return 0;
}