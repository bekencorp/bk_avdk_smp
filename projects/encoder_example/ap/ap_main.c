#include <os/os.h>
#include <os/mem.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <components/shell_task.h>
#include <media_service.h>
#include "cli.h"
#include "encoder_cli.h"

#define CMDS_COUNT  (sizeof(s_encoder_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_encoder_test_commands[] =
{
    {"jpeg_encode", "jpeg_encode open|close|encode", cli_jpeg_encoder_test_cmd},
};

int cli_encoder_test_init(void)
{
    return cli_register_commands(s_encoder_test_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();

    cli_encoder_test_init();
    return 0;
}
