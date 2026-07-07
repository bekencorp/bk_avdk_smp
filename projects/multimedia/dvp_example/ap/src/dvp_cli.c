#include "cli.h"

#include "include/dvp_cli.h"

static const struct cli_command s_dvp_test_commands[] =
{
    {"dvp", "dvp detect | open | read | cb | close", cli_dvp_func_test_cmd},
};

#define CMDS_COUNT  (sizeof(s_dvp_test_commands) / sizeof(struct cli_command))

int cli_dvp_test_init(void)
{
    return cli_register_commands(s_dvp_test_commands, CMDS_COUNT);
}