// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include "cli.h"
#include <driver/wwdt.h>
#include <driver/timer.h>
#include "wwdt_driver.h"

static void cli_wwdt_help(void)
{
	CLI_LOGI("wwdt_driver init\n");
	CLI_LOGI("wwdt_driver deinit\n");
	CLI_LOGI("wwdt start [timeout]\n");
	CLI_LOGI("wwdt stop\n");
	CLI_LOGI("wwdt feed\n");
	CLI_LOGI("wwdt feed_in_win\n");
	CLI_LOGI("wwdt feed_out_win\n");
}

static void cli_wwdt_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_wwdt_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_driver_init());
		CLI_LOGI("wwdt driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_driver_deinit());
		CLI_LOGI("wwdt driver deinit\n");
	} else {
		cli_wwdt_help();
		return;
	}
}

static void timer_isr_callback(timer_id_t chan)
{
	BK_LOG_ON_ERR(bk_wwdt_feed());
}

static void cli_wwdt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_wwdt_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		uint32_t win_val;
		uint32_t timeout;
		uint32_t is_enable_window = false;

		timeout = os_strtoul(argv[2], NULL, 10);
		win_val = timeout / 2;
		if (argc > 3) {
			is_enable_window = os_strtoul(argv[3], NULL, 10);
		}
		if (argc > 4) {
			win_val = os_strtoul(argv[4], NULL, 10);
		}
		BK_LOG_ON_ERR(bk_wwdt_start(timeout, (is_enable_window ? true : false), win_val));
		CLI_LOGI("wwdt start, timeout=%d, is_enable_window:%d\n", timeout, is_enable_window);
	} else if (os_strcmp(argv[1], "stop") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_stop());
		CLI_LOGI("wwdt stop\n");
	}else if (os_strcmp(argv[1], "feed") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_start(6000, false, 0));
		BK_LOG_ON_ERR(bk_timer_start(1, 1000, timer_isr_callback));
		CLI_LOGI("wwdt feed\n");
	} else if (os_strcmp(argv[1], "feed_in_win") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_start(6000, true, 3000));
		bk_timer_start(1, 2, timer_isr_callback);
		CLI_LOGI("wwdt window feed\n");
	} else if (os_strcmp(argv[1], "feed_out_win") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_start(6000, true, 3000));
		bk_timer_start(1, 4000, timer_isr_callback);
		CLI_LOGI("wwdt window feed\n");
	}else if (os_strcmp(argv[1], "disable") == 0) {
		bk_wwdt_stop();
		CLI_LOGI("wwdt debug disabled\n");
	}else if (os_strcmp(argv[1], "while") == 0) {
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		CLI_LOGI("wwdt enter while_1\n");
		while(1);
		GLOBAL_INT_RESTORE();
	} else if (os_strcmp(argv[1], "reboot") == 0) {
		bk_wwdt_force_reboot();
	} else if (os_strcmp(argv[1], "get_cpu_id") == 0) {
		uint32_t cpu_id = bk_wwdt_get_cpu_id();
		CLI_LOGI("wwdt get cpu_id:%x\r\n", cpu_id);
	} else {
		cli_wwdt_help();
		return;
	}
}

#define WDT_CMD_CNT (sizeof(s_wwdt_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_wwdt_commands[] = {
	{"wwdt_driver", "{init|deinit}", cli_wwdt_driver_cmd},
	{"wwdt", "wwdt {start|stop|feed} [...]", cli_wwdt_cmd}
};

int bk_wwdt_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_wwdt_driver_init());
	return cli_register_module_test_feature(s_wwdt_commands, WDT_CMD_CNT);
}
// eof

