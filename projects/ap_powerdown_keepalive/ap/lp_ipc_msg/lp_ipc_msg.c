#include "lp_ipc_msg/lp_ipc_msg.h"
#include <string.h>
#include <common/sys_config.h>
#include "bk_uart.h"
#include "bk_private/bk_wifi.h"
#include "bk_wifi_private.h"
#include "bk_cli.h"
#include "cli.h"
#include <components/event.h>
#include <components/netif.h>
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#include <modules/wdrv_common.h>
#include "keepalive/keepalive.h"

#define LP_IPC_TAG "LP_IPC"

#define LOGI(...)       BK_LOGI(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGW(...)       BK_LOGW(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGE(...)       BK_LOGE(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGD(...)       BK_LOGD(LP_IPC_TAG, ##__VA_ARGS__)
#define LOGV(...)       BK_LOGV(LP_IPC_TAG, ##__VA_ARGS__)


#define LP_IPC_CLI_CMD_CNT (sizeof(s_lp_ipc_commands) / sizeof(struct cli_command))

pl_wakeup_t *pl_wakeup_env = NULL;


int lp_ipc_wakeup_env_init(void)
{
    int ret;
    uint8_t response_buf[64] = {0};
    uint16_t response_len = 0;

    ret = bk_wdrv_customer_transfer_rsp(LP_IPC_CMD_GET_WAKEUP_ENV_ADDR, NULL, 0, 
                                                   response_buf, sizeof(response_buf), &response_len);
    if (ret != 0) {
        LOGE("%s %d Failed ret %d\n",__func__,__LINE__, ret);
        return -1;
    }

    if (response_len >= sizeof(uint32_t)) {
        lp_ipc_wakeup_env_response(response_buf, response_len);
        if (pl_wakeup_env != NULL) {
            LOGV("%s %d Wakeup env address: 0x%p\n",__func__,__LINE__, pl_wakeup_env);
            return 0;
        } else {
            LOGE("%s %d Failed\n",__func__,__LINE__);
            return -1;
        }
    } else {
        LOGE("Invalid response length: %d \n", response_len);
        return -1;
    }
}

void lp_ipc_wakeup_env_response(uint8_t *data, uint16_t len)
{
    if (data == NULL || len < sizeof(uint32_t)) {
        LOGE("Invalid wakeup env response, len: %d \n", len);
        pl_wakeup_env = NULL;
        return;
    }

    uint32_t addr = *(uint32_t *)data;
    pl_wakeup_env = (pl_wakeup_t *)(uintptr_t)addr;

    if (pl_wakeup_env != NULL) {
        LOGV("wakeup_reason: 0x%x\n", pl_wakeup_env->wakeup_reason);
    }
}

static void lp_ipc_cli_help(void)
{
    BK_LOG_RAW("lp <arg1> <arg2> ...\r\n");
    BK_LOG_RAW("-----------------------LP_IPC COMMAND---------------------------------\r\n");
    BK_LOG_RAW("lp                                      - help infomation\r\n");
    BK_LOG_RAW("lp start_ka                             - start keep alive with server demo\r\n");
    BK_LOG_RAW("lp stop_ka                              - stop keep alive with server demo\r\n");
    BK_LOG_RAW("lp control [para]                       - start keep avlive with host(eg. lp control alive/stop)\r\n");

}

static void lp_ipc_cli_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    void *rsp_data = NULL;
    //uint16_t rsp_len = 0;

    if (argc == 2) {
        lp_ipc_cli_help();
        return;
    }

    rsp_data = (lp_ipc_msg_hdr_t *)malloc(LP_IPC_MAX_MSG_SIZE);

    if (rsp_data == NULL){
        LOGE("%s malloc failed,line:%d\n", __FUNCTION__,__LINE__);
        goto EXIT;
    }
    memset(rsp_data,0,LP_IPC_MAX_MSG_SIZE);

    if ((os_strcmp(argv[1], "start_ka") == 0))
    {
        keepalive_send_keepalive_cmd(argv[2], argv[3]);
    }
    else if ((os_strcmp(argv[1], "stop_ka") == 0))
    {
        keepalive_stop_cp_keepalive();
    }
    else if ((os_strcmp(argv[1], "control") == 0))
    {
        const char *control_param = (argc >= 3) ? argv[2] : NULL;
        keepalive_send_control_cmd(control_param);
    }

EXIT:
    if (rsp_data)
        free(rsp_data);
}

static const struct cli_command s_lp_ipc_commands[] = {
	{"lp", "lp ipc CLI commands", lp_ipc_cli_cmd},
};

int lp_ipc_cli_init(void)
{
	return cli_register_commands(s_lp_ipc_commands, LP_IPC_CLI_CMD_CNT);
}
