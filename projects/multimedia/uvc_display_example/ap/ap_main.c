#include <os/os.h>
#include "bk_private/bk_init.h"
#include <components/system.h>

#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include "cli.h"
#include "test_cli.h"
#include "media_service.h"
#include "avdk_monitor.h"

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
static const struct cli_command s_uvc_display_commands[] =
{
    {"uvc", " uvc open | close", cli_uvc_test_cmd},
    {"decode", "decode open | close", cli_decode_test_cmd},
    {"display", "display open | close", cli_display_test_cmd},
    {"pipeline", "pipeline open | close", cli_pipeline_test_cmd},
};

#define CMDS_COUNT  (sizeof(s_uvc_display_commands) / sizeof(struct cli_command))

int cli_uvc_display_init(void)
{
    return cli_register_commands(s_uvc_display_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();
    bk_auxldo_enable();
    bk_frame_buffer_init();
    /* Debug config for Multimedia */
    avdk_monitor_init();
    avdk_monitor_start();
    cli_uvc_display_init();
    return 0;
}
