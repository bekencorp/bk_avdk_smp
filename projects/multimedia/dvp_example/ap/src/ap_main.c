#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/psram.h>
#include "cli.h"
#include "sys_driver.h"
#include "media_service.h"
#include "avdk_monitor.h"
#include "dvp_cli.h"

#include <components/bk_frame_buffer.h>

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

    int ret = cli_dvp_test_init();

    bk_printf("%s, %d, m55 running..., ret: %d\r\n", __func__, __LINE__, ret);

    bk_printf("m55 running...\r\n");

    avdk_monitor_init();
    avdk_monitor_start();

    return 0;
}
