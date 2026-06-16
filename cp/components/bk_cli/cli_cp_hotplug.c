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
#include "sys_reg.h"

#if (CONFIG_SOC_SMP && CONFIG_CPU_HOTPLUG)

#define CPU_HOTPLUG_CMD_CNT (sizeof(s_cpu_hotplug_commands) / sizeof(struct cli_command))
#define CPU_HOTPLUG_CLI_MIGRATE_RETRY (20)
#define CPU_HOTPLUG_BUSY_TEST_STACK_SIZE (512)
#define CPU_HOTPLUG_IRQ_WORDS (3)

static uint32_t cli_cpu_irq_en_addr(uint32_t cpu, uint32_t word)
{
	uint32_t base = (cpu == CPU0_CORE_ID) ? SYS_CPU0_INT_0_31_EN_ADDR :
		SYS_CPU1_INT_0_31_EN_ADDR;

	return base + (word << 2);
}

static void cli_cpu_hotplug_help(void)
{
	CLI_LOGI("cpu list\r\n");
	CLI_LOGI("cpu state\r\n");
	CLI_LOGI("cpu offline 1\r\n");
	CLI_LOGI("cpu online 1\r\n");
	CLI_LOGI("cpu irq-affinity\r\n");
	CLI_LOGI("cpu task-affinity\r\n");
	CLI_LOGI("cpu stress 1 <loops>\r\n");
	CLI_LOGI("cpu busy-test\r\n");
}

static void cli_cpu_print_state(void)
{
	for (uint32_t cpu = CPU0_CORE_ID; cpu <= CPU1_CORE_ID; cpu++) {
		CLI_LOGI("cpu%u: state=%s online=%u active=%u domain possible=0x%x online=0x%x active=0x%x dying=0x%x offline=0x%x\r\n",
			cpu, bk_cpu_hp_get_state_name(cpu), bk_cpu_hp_is_online(cpu), bk_cpu_hp_is_active(cpu),
			bk_cpu_hp_get_domain_possible_mask(cpu), bk_cpu_hp_get_domain_online_mask(cpu),
			bk_cpu_hp_get_domain_active_mask(cpu), bk_cpu_hp_get_domain_dying_mask(cpu),
			bk_cpu_hp_get_domain_offline_mask(cpu));
	}
}

static uint32_t cli_cpu_hotplug_target_valid(uint32_t cpu)
{
	if (cpu != CPU1_CORE_ID) {
		CLI_LOGE("CP hotplug only supports cpu1, cpu%u is not allowed\r\n", cpu);
		return 0;
	}

	return 1;
}

static void cli_cpu_print_irq_affinity(void)
{
	CLI_LOGI("CP irq affinity routes: cpu0 en0=0x%x en1=0x%x en2=0x%x, cpu1 en0=0x%x en1=0x%x en2=0x%x\r\n",
		REG_READ(cli_cpu_irq_en_addr(CPU0_CORE_ID, 0)),
		REG_READ(cli_cpu_irq_en_addr(CPU0_CORE_ID, 1)),
		REG_READ(cli_cpu_irq_en_addr(CPU0_CORE_ID, 2)),
		REG_READ(cli_cpu_irq_en_addr(CPU1_CORE_ID, 0)),
		REG_READ(cli_cpu_irq_en_addr(CPU1_CORE_ID, 1)),
		REG_READ(cli_cpu_irq_en_addr(CPU1_CORE_ID, 2)));
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
	uint32_t cpu = CPU1_CORE_ID;
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
		CLI_LOGI("hard-pinned task on CP cpu1/core1: %s\r\n",
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

		ret = bk_cpu_hp_offline(CPU1_CORE_ID);
		if (ret != expected_ret) {
			recover_ret = bk_cpu_hp_online(CPU1_CORE_ID);
			CLI_LOGE("cpu busy-test recovery online ret=%d\r\n", recover_ret);
		}

		vTaskDelete(busy_task);
		CLI_LOGI("cpu busy-test %s expect=busy(%d) actual=%d state=%s\r\n",
			(ret == expected_ret) ? "PASS" : "FAIL", expected_ret, ret,
			bk_cpu_hp_get_state_name(CPU1_CORE_ID));
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
	{"cpu", "cpu {list|state|offline 1|online 1|irq-affinity|task-affinity|stress 1 <loops>|busy-test}", cli_cpu_hotplug_cmd},
};

int cli_cp_hotplug_init(void)
{
	return cli_register_commands(s_cpu_hotplug_commands, CPU_HOTPLUG_CMD_CNT);
}

#endif
