#include "lp_ipc_msg.h"
#include "bk_ef.h"
#include <components/log.h>
#include "modules/cif_common.h"
#include "powerctrl.h"
#include "keepalive/keepalive_client.h"
#include "keepalive/keepalive_msg.h"

#define LP_IPC_TAG "LP_IPC"

#define LOGI(...) BK_LOGW(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(LP_IPC_TAG, ##__VA_ARGS__)


int lp_ipc_send_event(uint16_t event_id, uint8_t *data, uint16_t len)
{
    int ret = 0;
    lp_ipc_evt_t event = {0};

    if (len > LP_IPC_MAX_MSG_SIZE)
    {
        LOGE("%s %d error: len %d\n", __func__, __LINE__, len);
        return -1;
    }

    LOGV("%s %d event_id:%d len:%d\n", __func__, __LINE__, event_id, len);

    event.header.cid = event_id;
    event.header.len = len;
    event.header.magic = LP_IPC_PATTERN;

    if (data && len)
        os_memcpy(event.data, data, len);

    ret = cif_send_customer_event((uint8_t *)&event, sizeof(lp_ipc_prote_hdr_t) + len);

    return ret;
}

int lp_ipc_msg_handler(struct bk_msg_hdr *msg)
{
    int ret = 0;
    uint8_t *cfm_data = NULL;
    uint16_t cfm_data_len = 0;
    lp_ipc_msg_hdr_t *lp_ipc_cmd_hdr = NULL;
    char param[20];

    if (msg == NULL)
    {
        LOGI("%s data invalid\n",__func__);
        return -1;
    }

    lp_ipc_cmd_hdr = (lp_ipc_msg_hdr_t *)(msg + 1);
    LOGI("%s cmd id:0x%x\n",__func__, lp_ipc_cmd_hdr->cmd_id);
    switch (lp_ipc_cmd_hdr->cmd_id)
    {
        case LP_IPC_CMD_KEEPALIVESTART:
        {
            LOGI("LP_IPC_CMD_KEEPALIVESTART\n");
            ka_client_demo_init((uint8_t *)(lp_ipc_cmd_hdr + 1));
            break;
        }
        case LP_IPC_CMD_KEEPALIVESTOP:
        {
            LOGI("LP_IPC_CMD_KEEPALIVESTOP\n");
            ka_client_deinit();
            break;
        }
        case LP_IPC_CMD_CONTROL:
        {
            os_memcpy(param, (char *)(lp_ipc_cmd_hdr + 1), lp_ipc_cmd_hdr->len);

            LOGI("GET keep alive=%s\n",param);
            if(os_strcmp(param,"alive") == 0)
            {
                ka_reset_keepalive_with_host();
            }
            else if(os_strcmp(param,"stop") == 0)
            {
                ka_disable_keepalive_with_host();
            }

            cfm_data = (uint8_t *)param;
            cfm_data_len = strlen(param);

            break;
        }
        case LP_IPC_CMD_GET_WAKEUP_ENV_ADDR:
        {
            static uint32_t addr;
            addr = (uint32_t)(uintptr_t)&pl_wakeup_env;
            cfm_data = (uint8_t *)&addr;
            cfm_data_len = sizeof(addr);
            LOGI("LP_IPC_CMD_GET_WAKEUP_ENV_ADDR: 0x%x\n", addr);
            break;
        }
        default:
            break;
    }

    if (cfm_data && cfm_data_len)
    {
        ret = cif_send_customer_cmd_cfm(cfm_data, cfm_data_len, msg);
    }
    else
    {
        ret = cif_send_customer_cmd_cfm(NULL, 0, msg);
    }

    return ret;
}


void lp_ipc_msg_init(void)
{
    LOGI("%s\n", __func__);
    cif_register_customer_msg_handler(lp_ipc_msg_handler);
}

void lp_ipc_msg_deinit(void)
{
    LOGI("%s\n", __func__);
}

