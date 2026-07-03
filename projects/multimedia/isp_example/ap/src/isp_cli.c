#include "cli.h"

#include "include/isp_cli.h"

static const struct cli_command s_isp_test_commands[] =
{
    {"isp", " isp open | close | read | dvp_cb | detect", cli_isp_func_test_cmd},
    {"isp_api", "isp_api power_on | power_off | new | delete | init | deinit | open | close | suspend | resume ...", cli_isp_api_func_test_cmd},
    {"isp_tuning", "isp_tuning start | stop", cli_isp_tuning_cmd},
    {"isp_dump", "isp_dump start | stop", cli_isp_dump_cmd},
};

#define CMDS_COUNT  (sizeof(s_isp_test_commands) / sizeof(struct cli_command))

int cli_isp_test_init(void)
{
    return cli_register_commands(s_isp_test_commands, CMDS_COUNT);
}