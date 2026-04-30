#include <os/os.h>
#include <components/log.h>
#include <common/bk_include.h>
#include "keepalive.h"
#include "lp_ipc_msg/lp_ipc_msg.h"
#include <modules/wdrv_common.h>

#define KEEPALIVE_TAG "KEEPALIVE"

#define LOGI(...)   BK_LOGI(KEEPALIVE_TAG, ##__VA_ARGS__)
#define LOGW(...)   BK_LOGW(KEEPALIVE_TAG, ##__VA_ARGS__)
#define LOGE(...)   BK_LOGE(KEEPALIVE_TAG, ##__VA_ARGS__)
#define LOGD(...)   BK_LOGD(KEEPALIVE_TAG, ##__VA_ARGS__)
#define LOGV(...)   BK_LOGV(KEEPALIVE_TAG, ##__VA_ARGS__)


bk_err_t keepalive_send_keepalive_cmd(char *ip, char *port)
{
    int ret = BK_OK;
    lp_ipc_keepalive_cfg_t cfg = {0};

    strcpy(cfg.server, ip);
    cfg.port = (uint16_t)atoi(port);

    ret = bk_wdrv_customer_transfer(LP_IPC_CMD_KEEPALIVESTART, (uint8_t *)&cfg, sizeof(cfg));

    if (ret != 0) {
        LOGE("Failed to start keepalive: %d\n", ret);
    }

    return ret;
}

bk_err_t keepalive_stop_cp_keepalive(void)
{
    int ret;
    lp_ipc_keepalive_cfg_t cfg = {0};

    os_memset(&cfg, 0, sizeof(lp_ipc_keepalive_cfg_t));

    ret = bk_wdrv_customer_transfer(LP_IPC_CMD_KEEPALIVESTOP, (uint8_t *)&cfg, sizeof(cfg));
    if (ret != 0) {
        LOGE("%s: Failed to send: %d\n", __func__, ret);
        return BK_FAIL;
    } else {
        LOGI("%s: sent successfully\n", __func__);
        return BK_OK;
    }
}

bk_err_t keepalive_send_control_cmd(const char *control_param)
{
    int ret;
    uint16_t param_len = 0;

    if (control_param == NULL) {
        LOGE("%s: Invalid parameter (control_param is NULL)\n", __func__);
        return BK_FAIL;
    }

    param_len = (uint16_t)os_strlen(control_param);
    if (param_len == 0 || param_len >= LP_IPC_MAX_MSG_SIZE) {
        LOGE("%s: Invalid parameter length: %d\n", __func__, param_len);
        return BK_FAIL;
    }

    LOGI("%s: Sending control command - param: %s, len: %d\n", 
         __func__, control_param, param_len);

    ret = bk_wdrv_customer_transfer(LP_IPC_CMD_CONTROL, (uint8_t *)control_param, param_len);

    if (ret != 0) {
        LOGE("%s: Failed to send control command: %d\n", __func__, ret);
        return BK_FAIL;
    } else {
        LOGI("%s: Control command sent successfully\n", __func__);
        return BK_OK;
    }
}

void keepalive_handle_wakeup_reason(void)
{
    uint32_t wakeup_reason;

    // Check if pl_wakeup_env is initialized
    if (pl_wakeup_env == NULL) {
        LOGE("%s: pl_wakeup_env is NULL\n", __func__);
        return;
    }

    wakeup_reason = pl_wakeup_env->wakeup_reason;
    LOGI("%s: wakeup_reason = 0x%x\n", __func__, wakeup_reason);

    // Handle different wakeup reasons
    switch (wakeup_reason) {
        case POWERUP_POWER_WAKEUP_FLAG:
            // Normal power-on startup, no special operation needed
            LOGI("%s: Normal power-on startup\n", __func__);
            break;

        default:
            // Invalid or unknown wakeup reason
            LOGW("%s: Invalid or unknown wakeup reason: 0x%x\n", __func__, wakeup_reason);
            break;
    }
}

