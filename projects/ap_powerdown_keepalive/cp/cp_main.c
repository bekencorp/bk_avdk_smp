#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <components/ate.h>
#include "powerctrl.h"
#include "lp_ipc_msg.h"
#include "keepalive_client.h"
#include "keepalive_msg.h"
#include "keepalive_server.h"


#define CP_TAG "CP"

#define LOGI(...)   BK_LOGI(CP_TAG, ##__VA_ARGS__)
#define LOGW(...)   BK_LOGW(CP_TAG, ##__VA_ARGS__)
#define LOGE(...)   BK_LOGE(CP_TAG, ##__VA_ARGS__)
#define LOGD(...)   BK_LOGD(CP_TAG, ##__VA_ARGS__)
#define LOGV(...)   BK_LOGV(CP_TAG, ##__VA_ARGS__)


extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);


#define LP_KA_CMD_CNT (sizeof(s_lp_ka_commands) / sizeof(struct cli_command))

void lp_ka_cli_usage(void)
{
    BK_LOG_RAW("Usage: lp client/server [option]\n");
}

void lp_ka_demo_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (os_strcmp(argv[1], "client") == 0) {
        ka_client_hanlde_cli(argc, argv);
    }
    else if (os_strcmp(argv[1], "server") == 0) {
        ka_server_hanlde_cli(argc, argv);
    }
    else
    {
        LOGI("invalid paramter\n");
        goto error;
    }

    return;

error:
    lp_ka_cli_usage();
}

static const struct cli_command s_lp_ka_commands[] = {
    {"lp", "lowpower keepalive demo cmd", lp_ka_demo_cmd},
};

int lp_ka_cli_init(void)
{
    return cli_register_commands(s_lp_ka_commands, LP_KA_CMD_CNT);
}

void user_app_main(void) {
    pl_wakeup_host(POWERUP_POWER_WAKEUP_FLAG);

    if (!ate_is_enabled())
    {
        lp_ipc_msg_init();
    }

    lp_ka_cli_init();
    ka_common_init();
}

int main(void)
{
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    bk_init();

    return 0;
}