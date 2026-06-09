#include "bk_cli.h"
#include <common/sys_config.h>

extern void bk24_cli_parser(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);//TODO

static const struct cli_command s_commands[] = {
#if CONFIG_BK24G
	{"bk_24g", "see -h", bk24_cli_parser},
#endif
};

int cli_24g_init(void)
{
	return cli_register_commands(s_commands, sizeof(s_commands) / sizeof(s_commands[0]));
}
