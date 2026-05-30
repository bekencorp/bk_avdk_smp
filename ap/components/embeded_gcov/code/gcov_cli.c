#include "gcov_public.h"
#include "cli.h"

void _fini(void) {}

static void cli_gcov_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    __gcov_exit();
}

static const struct cli_command s_gcov_cmds[] = {
    { "gcov_dump", "dump gcov data via serial", cli_gcov_dump_cmd },
};

static void gcov_cli_init(void)
{
    cli_register_commands(s_gcov_cmds, sizeof(s_gcov_cmds) / sizeof(s_gcov_cmds[0]));
}

void gcov_init_uart(unsigned int uart_id, const void *cfg)
{
    gcov_output_init_uart(uart_id, cfg);
    gcov_cli_init();
}

void gcov_init_callback(gcov_write_fn write_cb)
{
    gcov_output_init_callback(write_cb);
    gcov_cli_init();
}
