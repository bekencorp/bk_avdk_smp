/*
 * Dhrystone 2.1 (DMIPS) CLI command - BK7259 CP core
 *
 * The command is registered by the project itself (same pattern as the AP side
 * and kvs_webrtc_example's kvs_webrtc_cli.c), so the benchmark, its sources and
 * its CLI entry move together and bk_cli knows nothing about Dhrystone.
 */
#include <stdint.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <os/os.h>

#include "bk_cli.h"
#include "cli.h"

#define TAG "dhrystone"

static void dhrystone_cli_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern void dhrystone_run(int argc, char *argv[]);

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	/* dhrystone_run() creates its own thread and blocks here until it finishes.
	 * The watchdog is left running on purpose: the 1 s AON feed in TIMER_ID2 keeps
	 * working, so a long run can no longer reset the chip. */
	dhrystone_run(argc, argv);
}

static const struct cli_command s_cmds[] = {
	{"dhrystone", "dhrystone [runs] (Dhrystone 2.1 benchmark, reports DMIPS)", dhrystone_cli_cmd},
};

int dhry_cli_init(void)
{
	return cli_register_commands(s_cmds, sizeof(s_cmds) / sizeof(s_cmds[0]));
}
