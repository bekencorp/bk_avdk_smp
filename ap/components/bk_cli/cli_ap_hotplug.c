// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#include <common/bk_include.h>
#include <os/os.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"
#include "multicore_driver.h"
#include "sys_ahbp_ll.h"

#if (CONFIG_SOC_SMP && CONFIG_CPU_HOTPLUG)

#define CPU_HOTPLUG_CMD_CNT (sizeof(s_cpu_hotplug_commands) / sizeof(struct cli_command))
#define CPU_HOTPLUG_CLI_MIGRATE_RETRY (20)
#define CPU_HOTPLUG_BUSY_TEST_STACK_SIZE (512)

static void cli_cpu_hotplug_help(void)
{
	CLI_LOGI("cpu list\r\n");
	CLI_LOGI("cpu state\r\n");
	CLI_LOGI("cpu offline 3\r\n");
	CLI_LOGI("cpu online 3\r\n");
	CLI_LOGI("cpu irq-affinity\r\n");
	CLI_LOGI("cpu task-affinity\r\n");
	CLI_LOGI("cpu stress 3 <loops>\r\n");
	CLI_LOGI("cpu busy-test\r\n");
}

static void cli_cpu_print_state(void)
{
	for (uint32_t cpu = CPU2_CORE_ID; cpu <= CPU3_CORE_ID; cpu++) {
		CLI_LOGI("cpu%u: state=%s online=%u active=%u domain possible=0x%x online=0x%x active=0x%x dying=0x%x offline=0x%x\r\n",
			cpu, bk_cpu_hp_get_state_name(cpu), bk_cpu_hp_is_online(cpu), bk_cpu_hp_is_active(cpu),
			bk_cpu_hp_get_domain_possible_mask(cpu), bk_cpu_hp_get_domain_online_mask(cpu),
			bk_cpu_hp_get_domain_active_mask(cpu), bk_cpu_hp_get_domain_dying_mask(cpu),
			bk_cpu_hp_get_domain_offline_mask(cpu));
	}
}

static uint32_t cli_cpu_hotplug_target_valid(uint32_t cpu)
{
	if (cpu != CPU3_CORE_ID) {
		CLI_LOGE("AP hotplug only supports cpu3, cpu%u is not allowed\r\n", cpu);
		return 0;
	}

	return 1;
}

static void cli_cpu_print_irq_affinity(void)
{
	CLI_LOGI("AP irq affinity routes: cpu2 reg10=0x%x reg11=0x%x, cpu3 reg12=0x%x reg13=0x%x reg14=0x%x\r\n",
		sys_ahbp_ll_get_reg10_value(), sys_ahbp_ll_get_reg11_value(),
		sys_ahbp_ll_get_reg12_value(), sys_ahbp_ll_get_reg13_value(),
		sys_ahbp_ll_get_reg14_value());
}

static void cli_cpu_hotplug_busy_task(void *arg)
{
	(void)arg;

	while (1) {
		rtos_delay_milliseconds(50);
	}
}

static void cli_cpu_hotplug_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t cpu = CPU3_CORE_ID;
	uint32_t loops = 1;
	BaseType_t old_core_id = tskNO_AFFINITY;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)old_core_id;

	if (argc < 2) {
		cli_cpu_hotplug_help();
		return;
	}

	if ((os_strcmp(argv[1], "list") == 0) || (os_strcmp(argv[1], "state") == 0) ||
		(os_strcmp(argv[1], "status") == 0)) {
		cli_cpu_print_state();
		return;
	}

	if (os_strcmp(argv[1], "offline") == 0) {
		if (argc >= 3) {
			cpu = os_strtoul(argv[2], NULL, 10);
		}
		if (!cli_cpu_hotplug_target_valid(cpu)) {
			return;
		}
		ret = bk_cpu_hp_offline(cpu);
		CLI_LOGI("cpu%u offline ret=%d\r\n", cpu, ret);
		return;
	}

	if (os_strcmp(argv[1], "online") == 0) {
		if (argc >= 3) {
			cpu = os_strtoul(argv[2], NULL, 10);
		}
		if (!cli_cpu_hotplug_target_valid(cpu)) {
			return;
		}
		ret = bk_cpu_hp_online(cpu);
		CLI_LOGI("cpu%u online ret=%d\r\n", cpu, ret);
		return;
	}

	if (os_strcmp(argv[1], "irq-affinity") == 0) {
		cli_cpu_print_irq_affinity();
		return;
	}

	if (os_strcmp(argv[1], "task-affinity") == 0) {
		CLI_LOGI("hard-pinned task on AP cpu3/core1: %s\r\n",
			xTaskHasTasksPinnedToCore(SMP_CORE1_ID) ? "yes" : "no");
		return;
	}

	if (os_strcmp(argv[1], "busy-test") == 0) {
		TaskHandle_t busy_task = NULL;
		BaseType_t task_ret;
		bk_err_t recover_ret;
		const bk_err_t expected_ret = BK_ERR_BUSY;

		task_ret = xTaskCreatePinnedToCore(cli_cpu_hotplug_busy_task, "hp_busy",
			CPU_HOTPLUG_BUSY_TEST_STACK_SIZE, NULL, BEKEN_DEFAULT_WORKER_PRIORITY,
			&busy_task, SMP_CORE1_ID);
		if (task_ret != pdPASS) {
			CLI_LOGE("cpu busy-test create pinned task failed, ret=%d\r\n", task_ret);
			return;
		}

		taskYIELD();
		rtos_delay_milliseconds(2);

		ret = bk_cpu_hp_offline(CPU3_CORE_ID);
		if (ret != expected_ret) {
			recover_ret = bk_cpu_hp_online(CPU3_CORE_ID);
			CLI_LOGE("cpu busy-test recovery online ret=%d\r\n", recover_ret);
		}

		vTaskDelete(busy_task);
		CLI_LOGI("cpu busy-test %s expect=busy(%d) actual=%d state=%s\r\n",
			(ret == expected_ret) ? "PASS" : "FAIL", expected_ret, ret,
			bk_cpu_hp_get_state_name(CPU3_CORE_ID));
		return;
	}

	if (os_strcmp(argv[1], "stress") == 0) {
		if (argc >= 3) {
			cpu = os_strtoul(argv[2], NULL, 10);
		}
		if (argc >= 4) {
			loops = os_strtoul(argv[3], NULL, 10);
		}
		if (!cli_cpu_hotplug_target_valid(cpu)) {
			return;
		}


		for (uint32_t i = 0; i < loops; i++) {
			ret = bk_cpu_hp_offline(cpu);
			if (ret != BK_OK) {
				CLI_LOGE("cpu%u offline failed at loop %u, ret=%d\r\n", cpu, i, ret);
				return;
			}

			ret = bk_cpu_hp_online(cpu);
			if (ret != BK_OK) {
				CLI_LOGE("cpu%u online failed at loop %u, ret=%d\r\n", cpu, i, ret);
				return;
			}
		}

		CLI_LOGI("cpu%u hotplug stress %u loops done\r\n", cpu, loops);
		return;
	}

	cli_cpu_hotplug_help();
}

static const struct cli_command s_cpu_hotplug_commands[] = {
	{"cpu", "cpu {list|state|offline 3|online 3|irq-affinity|task-affinity|stress 3 <loops>|busy-test}", cli_cpu_hotplug_cmd},
};

int cli_ap_hotplug_init(void)
{
	return cli_register_commands(s_cpu_hotplug_commands, CPU_HOTPLUG_CMD_CNT);
}

#endif
