#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include <components/bk_frame_buffer.h>
#include "pp_test.h"
#include "media_service.h"

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CMDS_COUNT  (sizeof(s_pp_commands) / sizeof(struct cli_command))

static const struct cli_command s_pp_commands[] =
{
    {"pp", "pp", cli_pp_cmd},
};

int cli_pp_init(void)
{
    return cli_register_commands(s_pp_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

    bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);

#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif

    cli_pp_init();

#ifdef CONFIG_BK_DECODER
	pp_run_boot_demo();
#endif
    return 0;
}
