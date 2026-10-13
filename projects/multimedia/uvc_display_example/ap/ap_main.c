#include <os/os.h>
#include "bk_private/bk_init.h"
#include <components/system.h>

#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include "cli.h"
#include "test_cli.h"
#include "media_service.h"
#include "avdk_monitor.h"

static const struct cli_command s_uvc_display_commands[] =
{
    {"uvc", "uvc open <port> <w> <h> <mjpeg|...> | close <port>", cli_uvc_test_cmd},
    {"decode", "decode open | close", cli_decode_test_cmd},
    {"display", "display open | close", cli_display_test_cmd},
    {"pipeline", "pipeline open [port w h fps] | close", cli_pipeline_test_cmd},
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
    bk_frame_buffer_init();
    /* Debug config for Multimedia */
    avdk_monitor_init();
    avdk_monitor_start();
    cli_uvc_display_init();
    return 0;
}
