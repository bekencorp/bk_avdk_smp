#include <components/avdk_utils/avdk_error.h>
#include <components/bk_camera_ctlr.h>
#include "cli.h"
#include "dvp_cli.h"
#include "media_utils.h"

bool cmd_contain(int argc, char **argv, char *string)
{
    int i;
    bool ret = false;

    for (i = 0; i < argc; i++)
    {
        if (os_strcmp(argv[i], string) == 0)
        {
            ret = true;
        }
    }

    return ret;
}

#define CMDS_COUNT  (sizeof(s_dvp_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_dvp_test_commands[] =
{
    {"dvp", "dvp open|close|suspend|resume|regener_idr [ppi] [type]", cli_dvp_func_test_cmd},
    {"dvp_api", "dvp api_test", cli_dvp_api_test_cmd},
};

int cli_dvp_test_init(void)
{
    return cli_register_commands(s_dvp_test_commands, CMDS_COUNT);
}
