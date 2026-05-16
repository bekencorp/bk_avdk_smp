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
#include <driver/wdt.h>
#include <driver/timer.h>
#include <bk_wdt.h>
#include "wdt_driver.h"

static void cli_wdt_help(void)
{
	CLI_LOGD("wdt_driver init\n");
	CLI_LOGD("wdt_driver deinit\n");
	CLI_LOGD("wdt start [timeout]\n");
	CLI_LOGD("wdt stop\n");
	CLI_LOGD("wdt feed\n");
}

static void cli_wdt_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_wdt_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_wdt_driver_init());
		CLI_LOGD("wdt driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_wdt_driver_deinit());
		CLI_LOGD("wdt driver deinit\n");
	} else {
		cli_wdt_help();
		return;
	}
}

static void timer_isr_callback(timer_id_t chan)
{
	BK_LOG_ON_ERR(bk_wdt_feed());
}

static beken_thread_t wdt_task1_handle = NULL;
static beken_thread_t wdt_task2_handle = NULL;
void delay_ms(uint32_t ms);

static void test_task_wdt_1(void *arg) {
	uint32_t test_count = 0;
	while (test_count < 6) {
		test_count++;
		BK_LOGD(NULL, "test task wdt 1=%d\n", test_count);
		delay_ms(1000);
	}
	rtos_delete_thread(NULL);
}

static void test_task_wdt_2(void *arg) {
	uint32_t test_count = 0;
	while (test_count < 100) {
		test_count++;
		BK_LOGD(NULL, "test task wdt 2=%d\n", test_count);
		delay_ms(1000);
	}
	rtos_delete_thread(NULL);
}

static void cli_wdt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_wdt_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		uint32_t timeout = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_wdt_start(timeout));
		CLI_LOGD("wdt start, timeout=%d\n", timeout);
		CLI_LOGD("AON_WDT_REG_BASE=0x%08x\n", REG_READ(SOC_AON_WDT_REG_BASE));
	} else if (os_strcmp(argv[1], "stop") == 0) {
		BK_LOG_ON_ERR(bk_wdt_stop());
#if (CONFIG_TASK_WDT)
		bk_task_wdt_stop();
#endif
		CLI_LOGD("wdt stop\n");
	}else if (os_strcmp(argv[1], "feed") == 0) {
		BK_LOG_ON_ERR(bk_wdt_start(6000));
		BK_LOG_ON_ERR(bk_timer_start(1, 1000, timer_isr_callback));
		CLI_LOGD("wdt feed\n");
	}else if (os_strcmp(argv[1], "disable") == 0) {
		bk_wdt_stop();
#if (CONFIG_TASK_WDT)
		bk_task_wdt_stop();
#endif
		CLI_LOGD("wdt debug disabled\n");
	}else if (os_strcmp(argv[1], "enable") == 0) {
		extern void wdt_init(void);
		wdt_init();
		CLI_LOGD("wdt debug enabled\n");
	}else if (os_strcmp(argv[1], "while") == 0) {
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		CLI_LOGD("wdt enter while1\n");
		while(1);
		GLOBAL_INT_RESTORE();
	} else if (os_strcmp(argv[1], "reboot") == 0) {
		bk_wdt_force_reboot();
	} 
    else if (os_strcmp(argv[1], "single_task") == 0) {
		rtos_create_thread(&wdt_task1_handle, 5,
			"test_task_wdt_1",
			(beken_thread_function_t) test_task_wdt_1,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);

	}else if (os_strcmp(argv[1], "multi_task") == 0) {
		rtos_create_thread(&wdt_task2_handle, 6,
			"test_task_wdt_2",
			(beken_thread_function_t) test_task_wdt_2,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);

		rtos_create_thread(&wdt_task1_handle, 5,
			"test_task_wdt_1",
			(beken_thread_function_t) test_task_wdt_1,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);
    }else {
		cli_wdt_help();
		return;
	}
}

#define WDT_CMD_CNT (sizeof(s_wdt_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_wdt_commands[] = {
	{"wdt_driver", "{init|deinit}", cli_wdt_driver_cmd},
	{"wdt", "wdt {start|stop|feed} [...]", cli_wdt_cmd}
};

int bk_wdt_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_wdt_driver_init());
	return cli_register_module_test_feature(s_wdt_commands, WDT_CMD_CNT);
}

