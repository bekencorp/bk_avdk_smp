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
#include <components/system.h>
#include "cli.h"
#include <driver/wwdt.h>
#include <driver/timer.h>
#include "wwdt_driver.h"

#define WWDT_TEST_TASK_PRIORITY     BEKEN_APPLICATION_PRIORITY
#define WWDT_TEST_TASK_STACK_SIZE   2048

static beken_thread_t s_wwdt_busy_thread = NULL;
static beken_thread_t s_wwdt_hang_thread = NULL;

static void cli_wwdt_help(void)
{
	CLI_LOGI("wwdt_driver init\n");
	CLI_LOGI("wwdt_driver deinit\n");
	CLI_LOGI("wwdt start [timeout]\n");
	CLI_LOGI("wwdt stop\n");
	CLI_LOGI("wwdt status\n");
	CLI_LOGI("wwdt feed_once\n");
	CLI_LOGI("wwdt feed_timer\n");
	CLI_LOGI("wwdt set_feed_time [tick]\n");
	CLI_LOGI("wwdt skip_feed_core [0|1]\n");
	CLI_LOGI("wwdt resume_feed_core [0|1]\n");
	CLI_LOGI("wwdt busy_core [0|1] [seconds]\n");
	CLI_LOGI("wwdt hang_core [0|1]\n");
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

static bk_err_t wwdt_create_core_thread(beken_thread_t *thread, uint32_t core_id,
	const char *name, beken_thread_function_t function, beken_thread_arg_t arg)
{
#if CONFIG_SOC_SMP
	if (core_id == CPU0_CORE_ID) {
		return rtos_core0_create_thread(thread, WWDT_TEST_TASK_PRIORITY, name,
			function, WWDT_TEST_TASK_STACK_SIZE, arg);
	} else if (core_id == CPU1_CORE_ID) {
		return rtos_core1_create_thread(thread, WWDT_TEST_TASK_PRIORITY, name,
			function, WWDT_TEST_TASK_STACK_SIZE, arg);
	}

	return BK_FAIL;
#else
	if (core_id != CPU0_CORE_ID) {
		return BK_FAIL;
	}

	return rtos_create_thread(thread, WWDT_TEST_TASK_PRIORITY, name,
		function, WWDT_TEST_TASK_STACK_SIZE, arg);
#endif
}

static void wwdt_busy_core_task(beken_thread_arg_t arg)
{
	uint32_t seconds = (uint32_t)arg;
	uint64_t start_tick = bk_get_tick();
	uint64_t end_tick = start_tick + ((uint64_t)seconds * bk_get_ticks_per_second());
	volatile uint32_t busy_count = 0;

	CLI_LOGI("wwdt busy start, core=%u, seconds=%u, start_tick=%u\r\n",
		rtos_get_core_id(), seconds, (uint32_t)start_tick);

	while (bk_get_tick() < end_tick) {
		busy_count++;
	}

	CLI_LOGI("wwdt busy done, core=%u, busy_count=%u\r\n",
		rtos_get_core_id(), busy_count);
	s_wwdt_busy_thread = NULL;
	rtos_delete_thread(NULL);
}

static void wwdt_hang_core_task(beken_thread_arg_t arg)
{
	(void)arg;
	GLOBAL_INT_DECLARATION();

	CLI_LOGI("wwdt hang start, core=%u, wwdt_cpu=%u\r\n",
		rtos_get_core_id(), bk_wwdt_get_cpu_id());

	GLOBAL_INT_DISABLE();
	while (1) {
		;
	}
	GLOBAL_INT_RESTORE();
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

		if (argc < 3) {
			cli_wwdt_help();
			return;
		}
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
	} else if (os_strcmp(argv[1], "status") == 0) {
		CLI_LOGI("wwdt status: core=%u, wwdt_cpu=%u, driver_inited=%u, feed_time=%u, skip_bits=0x%x\r\n",
			rtos_get_core_id(), bk_wwdt_get_cpu_id(),
			bk_wwdt_is_driver_inited(), bk_wwdt_get_feed_time(),
			bk_wwdt_get_skip_feed_bits());
	} else if (os_strcmp(argv[1], "feed_once") == 0) {
		BK_LOG_ON_ERR(bk_wwdt_feed());
		CLI_LOGI("wwdt feed once, core=%u, wwdt_cpu=%u\r\n",
			rtos_get_core_id(), bk_wwdt_get_cpu_id());
	} else if (os_strcmp(argv[1], "set_feed_time") == 0) {
		uint32_t feed_time;

		if (argc < 3) {
			cli_wwdt_help();
			return;
		}

		feed_time = os_strtoul(argv[2], NULL, 10);
		bk_wwdt_set_feed_time(feed_time);
		CLI_LOGI("wwdt set feed_time=%u\r\n", bk_wwdt_get_feed_time());
	} else if (os_strcmp(argv[1], "skip_feed_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_wwdt_help();
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_wwdt_set_skip_feed_core(core_id, true));
		CLI_LOGI("wwdt skip feed core=%u, skip_bits=0x%x\r\n",
			core_id, bk_wwdt_get_skip_feed_bits());
	} else if (os_strcmp(argv[1], "resume_feed_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_wwdt_help();
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_wwdt_set_skip_feed_core(core_id, false));
		CLI_LOGI("wwdt resume feed core=%u, skip_bits=0x%x\r\n",
			core_id, bk_wwdt_get_skip_feed_bits());
	} else if ((os_strcmp(argv[1], "feed_timer") == 0) ||
		(os_strcmp(argv[1], "feed") == 0)) {
		BK_LOG_ON_ERR(bk_wwdt_start(6000, false, 0));
		BK_LOG_ON_ERR(bk_timer_start(1, 1000, timer_isr_callback));
		CLI_LOGI("wwdt feed by legacy timer, core=%u\n", rtos_get_core_id());
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
	} else if (os_strcmp(argv[1], "busy_core") == 0) {
		uint32_t core_id;
		uint32_t seconds;

		if (argc < 4) {
			cli_wwdt_help();
			return;
		}

		if (s_wwdt_busy_thread) {
			CLI_LOGI("wwdt busy task already running\r\n");
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		seconds = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(wwdt_create_core_thread(&s_wwdt_busy_thread, core_id,
			"wwdt_busy", wwdt_busy_core_task, (beken_thread_arg_t)seconds));
		CLI_LOGI("wwdt busy_core scheduled, core=%u, seconds=%u\r\n",
			core_id, seconds);
	} else if (os_strcmp(argv[1], "hang_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_wwdt_help();
			return;
		}

		if (s_wwdt_hang_thread) {
			CLI_LOGI("wwdt hang task already running\r\n");
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(wwdt_create_core_thread(&s_wwdt_hang_thread, core_id,
			"wwdt_hang", wwdt_hang_core_task, (beken_thread_arg_t)0));
		CLI_LOGI("wwdt hang_core scheduled, core=%u\r\n", core_id);
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
	{"wwdt", "wwdt {start|stop|status|feed_once|feed_timer|skip_feed_core|resume_feed_core|busy_core|hang_core} [...]", cli_wwdt_cmd}
};

int bk_wwdt_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_wwdt_driver_init());
	return cli_register_module_test_feature(s_wwdt_commands, WDT_CMD_CNT);
}
// eof

