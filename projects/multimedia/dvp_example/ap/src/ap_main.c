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

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)


static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

int main(void)
{
    bk_init();
    media_service_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

    bk_printf("%s, %d, m55 running...\r\n", __func__, __LINE__);

    bk_printf("lodoen enable start...\r\n");

    bk_auxldo_enable();

    bk_printf("lodoen enable...\r\n");
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
