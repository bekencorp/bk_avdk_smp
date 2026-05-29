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
#include "bk_wdt.h"

#define TASK_WDT_TEST_TASK_PRIORITY     BEKEN_APPLICATION_PRIORITY
#define TASK_WDT_TEST_TASK_STACK_SIZE   2048

#if CONFIG_SOC_SMP
#define TASK_WDT_TEST_CORE_NUM CONFIG_SMP_CORE_CNT
#else
#define TASK_WDT_TEST_CORE_NUM 1
#endif

static beken_thread_t s_task_wdt_busy_thread = NULL;
static beken_thread_t s_task_wdt_hang_thread = NULL;
static beken_thread_t s_task_wdt_feed_thread = NULL;

static void cli_task_wdt_help(void)
{
	CLI_LOGI("task_wdt_driver init\n");
	CLI_LOGI("task_wdt_driver deinit\n");
	CLI_LOGI("task_wdt start\n");
	CLI_LOGI("task_wdt stop\n");
	CLI_LOGI("task_wdt status\n");
	CLI_LOGI("task_wdt feed_once\n");
	CLI_LOGI("task_wdt feed_core [0|1]\n");
	CLI_LOGI("task_wdt systick_check [count]\n");
	CLI_LOGI("task_wdt skip_feed_core [0|1]\n");
	CLI_LOGI("task_wdt resume_feed_core [0|1]\n");
	CLI_LOGI("task_wdt busy_core [0|1] [seconds]\n");
	CLI_LOGI("task_wdt hang_core [0|1]\n");
}

static bk_err_t task_wdt_create_core_thread(beken_thread_t *thread, uint32_t core_id,
	const char *name, beken_thread_function_t function, beken_thread_arg_t arg)
{
#if CONFIG_SOC_SMP
	if (core_id == CPU0_CORE_ID) {
		return rtos_core0_create_thread(thread, TASK_WDT_TEST_TASK_PRIORITY, name,
			function, TASK_WDT_TEST_TASK_STACK_SIZE, arg);
	} else if (core_id == CPU1_CORE_ID) {
		return rtos_core1_create_thread(thread, TASK_WDT_TEST_TASK_PRIORITY, name,
			function, TASK_WDT_TEST_TASK_STACK_SIZE, arg);
	}

	return BK_FAIL;
#else
	if (core_id != CPU0_CORE_ID) {
		return BK_FAIL;
	}

	return rtos_create_thread(thread, TASK_WDT_TEST_TASK_PRIORITY, name,
		function, TASK_WDT_TEST_TASK_STACK_SIZE, arg);
#endif
}

static void task_wdt_status_dump(void)
{
	uint32_t core_id;

	CLI_LOGI("task_wdt status: current_core=%u, feed_bits=0x%x, skip_bits=0x%x\r\n",
		rtos_get_core_id(), bk_task_wdt_get_feed_bits(), bk_task_wdt_get_skip_feed_bits());

	for (core_id = 0; core_id < TASK_WDT_TEST_CORE_NUM; core_id++) {
		uint64_t feed_tick = bk_task_wdt_get_last_feed_tick(core_id);
		CLI_LOGI("task_wdt core%u last_feed_tick=%u:%u\r\n", core_id,
			(uint32_t)(feed_tick >> 32), (uint32_t)(feed_tick & 0xFFFFFFFF));
	}
}

static void task_wdt_feed_core_task(beken_thread_arg_t arg)
{
	uint32_t core_id = (uint32_t)arg;

	bk_task_wdt_feed();
	CLI_LOGI("task_wdt feed_core done, target_core=%u, physical_cpu=%u\r\n",
		core_id, rtos_get_core_id());

	s_task_wdt_feed_thread = NULL;
	rtos_delete_thread(NULL);
}

static void task_wdt_busy_core_task(beken_thread_arg_t arg)
{
	uint32_t seconds = (uint32_t)arg;
	uint64_t start_tick = bk_get_tick();
	uint64_t end_tick = start_tick + ((uint64_t)seconds * bk_get_ticks_per_second());
	volatile uint32_t busy_count = 0;

	CLI_LOGI("task_wdt busy start, core=%u, seconds=%u, start_tick=%u\r\n",
		rtos_get_core_id(), seconds, (uint32_t)start_tick);

	while (bk_get_tick() < end_tick) {
		busy_count++;
	}

	CLI_LOGI("task_wdt busy done, core=%u, busy_count=%u\r\n",
		rtos_get_core_id(), busy_count);

	s_task_wdt_busy_thread = NULL;
	rtos_delete_thread(NULL);
}

static void task_wdt_hang_core_task(beken_thread_arg_t arg)
{
	(void)arg;

	CLI_LOGI("task_wdt hang start, physical_cpu=%u, smp_core=%u\r\n",
		rtos_get_core_id(), portGET_CORE_ID());

	while (1) {
		;
	}
}

static void cli_task_wdt_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_task_wdt_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_task_wdt_driver_init());
		CLI_LOGI("task_wdt driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_task_wdt_driver_deinit());
		CLI_LOGI("task_wdt driver deinit\n");
	} else {
		cli_task_wdt_help();
	}
}

static void cli_task_wdt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_task_wdt_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		bk_task_wdt_start();
		CLI_LOGI("task_wdt start\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		bk_task_wdt_stop();
		CLI_LOGI("task_wdt stop\r\n");
	} else if (os_strcmp(argv[1], "status") == 0) {
		task_wdt_status_dump();
	} else if (os_strcmp(argv[1], "feed_once") == 0) {
		bk_task_wdt_feed();
		CLI_LOGI("task_wdt feed_once, core=%u\r\n", rtos_get_core_id());
	} else if (os_strcmp(argv[1], "feed_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_task_wdt_help();
			return;
		}

		if (s_task_wdt_feed_thread) {
			CLI_LOGI("task_wdt feed task already running\r\n");
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(task_wdt_create_core_thread(&s_task_wdt_feed_thread, core_id,
			"task_wdt_feed", task_wdt_feed_core_task, (beken_thread_arg_t)core_id));
	} else if (os_strcmp(argv[1], "systick_check") == 0) {
		uint32_t count = 1;
		uint32_t i;

		if (argc >= 3) {
			count = os_strtoul(argv[2], NULL, 10);
		}

		for (i = 0; i < count; i++) {
			bk_task_wdt_systick_check();
		}
		CLI_LOGI("task_wdt systick_check count=%u\r\n", count);
	} else if (os_strcmp(argv[1], "skip_feed_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_task_wdt_help();
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_task_wdt_set_skip_feed_core(core_id, true));
		CLI_LOGI("task_wdt skip feed core=%u, skip_bits=0x%x\r\n",
			core_id, bk_task_wdt_get_skip_feed_bits());
	} else if (os_strcmp(argv[1], "resume_feed_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_task_wdt_help();
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(bk_task_wdt_set_skip_feed_core(core_id, false));
		CLI_LOGI("task_wdt resume feed core=%u, skip_bits=0x%x\r\n",
			core_id, bk_task_wdt_get_skip_feed_bits());
	} else if (os_strcmp(argv[1], "busy_core") == 0) {
		uint32_t core_id;
		uint32_t seconds;

		if (argc < 4) {
			cli_task_wdt_help();
			return;
		}

		if (s_task_wdt_busy_thread) {
			CLI_LOGI("task_wdt busy task already running\r\n");
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		seconds = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(task_wdt_create_core_thread(&s_task_wdt_busy_thread, core_id,
			"task_wdt_busy", task_wdt_busy_core_task, (beken_thread_arg_t)seconds));
		CLI_LOGI("task_wdt busy_core scheduled, core=%u, seconds=%u\r\n",
			core_id, seconds);
	} else if (os_strcmp(argv[1], "hang_core") == 0) {
		uint32_t core_id;

		if (argc < 3) {
			cli_task_wdt_help();
			return;
		}

		if (s_task_wdt_hang_thread) {
			CLI_LOGI("task_wdt hang task already running\r\n");
			return;
		}

		core_id = os_strtoul(argv[2], NULL, 10);
		BK_LOG_ON_ERR(task_wdt_create_core_thread(&s_task_wdt_hang_thread, core_id,
			"task_wdt_hang", task_wdt_hang_core_task, (beken_thread_arg_t)0));
		CLI_LOGI("task_wdt hang_core scheduled, core=%u\r\n", core_id);
	} else {
		cli_task_wdt_help();
	}
}

#define TASK_WDT_CMD_CNT (sizeof(s_task_wdt_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_task_wdt_commands[] = {
	{"task_wdt_driver", "{init|deinit}", cli_task_wdt_driver_cmd},
	{"task_wdt", "task_wdt {start|stop|status|feed_once|feed_core|systick_check|skip_feed_core|resume_feed_core|busy_core|hang_core} [...]", cli_task_wdt_cmd}
};

int bk_task_wdt_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_task_wdt_driver_init());
	return cli_register_module_test_feature(s_task_wdt_commands, TASK_WDT_CMD_CNT);
}

// eof
