// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#include <common/bk_include.h>
#include <os/os.h>
#include "cli.h"
#include "bk_arch.h"
#include "multicore_driver.h"

#if (CONFIG_SOC_SMP)

#define BASEPRI_STRESS_CMD_CNT (sizeof(s_basepri_stress_commands) / sizeof(struct cli_command))
#define BASEPRI_STRESS_MAX_WORKERS (4)
#define BASEPRI_STRESS_STACK_SIZE (1024)
#define BASEPRI_STRESS_DEFAULT_WORKERS (3)
#define BASEPRI_STRESS_DEFAULT_WORKER_DELAY_MS (2)
#define BASEPRI_STRESS_DEFAULT_TIMER_MS (10)
#define BASEPRI_STRESS_DEFAULT_HOTPLUG_MS (1000)
#define BASEPRI_STRESS_CRITICAL_SPINS (64)

typedef struct {
	beken_thread_t thread;
	uint32_t id;
	uint32_t loops;
	uint32_t core0_runs;
	uint32_t core1_runs;
} basepri_stress_worker_t;

typedef struct {
	volatile uint32_t running;
	volatile uint32_t stop_req;
	volatile uint32_t shared_mix;
	uint32_t started_ms;
	uint32_t worker_count;
	uint32_t worker_delay_ms;
	uint32_t timer_period_ms;
	uint32_t hotplug_period_ms;
	beken_timer_t timer;
	uint32_t timer_inited;
	beken_thread_t hotplug_thread;
	basepri_stress_worker_t workers[BASEPRI_STRESS_MAX_WORKERS];
	volatile uint32_t critical_entries;
	volatile uint32_t timer_entries;
	volatile uint32_t hotplug_loops;
	volatile uint32_t hotplug_busy;
	volatile uint32_t hotplug_failures;
	volatile uint32_t basepri_not_raised;
	volatile uint32_t primask_nonzero;
	volatile uint32_t basepri_restore_mismatch;
	volatile uint32_t basepri_leak_after_restore;
	volatile uint32_t last_basepri;
	volatile uint32_t max_basepri;
} basepri_stress_ctx_t;

static basepri_stress_ctx_t s_basepri_stress;

static void basepri_stress_help(void)
{
	CLI_LOGI("basepri_stress start [workers] [worker_delay_ms] [timer_ms] [hotplug_ms]\r\n");
	CLI_LOGI("basepri_stress stat\r\n");
	CLI_LOGI("basepri_stress stop\r\n");
	CLI_LOGI("  hotplug_ms=0 disables CPU1 online/offline loop\r\n");
	CLI_LOGI("  hotplug mode pins workers to CPU0 so CPU1 can go offline\r\n");
}

static void basepri_stress_count(volatile uint32_t *counter)
{
	(void)__sync_fetch_and_add(counter, 1);
}

static void basepri_stress_critical(uint32_t source)
{
	uint32_t old_basepri;
	uint32_t basepri;
	uint32_t primask;
	uint32_t restored_basepri;
	uint32_t i;
	volatile uint32_t mix;

	old_basepri = rtos_disable_int();
	basepri = bk_arch_get_basepri();
	primask = __get_PRIMASK();

	s_basepri_stress.last_basepri = basepri;
	if (basepri > s_basepri_stress.max_basepri) {
		s_basepri_stress.max_basepri = basepri;
	}
	if (basepri == 0UL) {
		basepri_stress_count(&s_basepri_stress.basepri_not_raised);
	}
	if (primask != 0UL) {
		basepri_stress_count(&s_basepri_stress.primask_nonzero);
	}

	mix = s_basepri_stress.shared_mix ^ source ^ rtos_get_core_id();
	for (i = 0; i < BASEPRI_STRESS_CRITICAL_SPINS; i++) {
		mix = (mix << 5) ^ (mix >> 2) ^ i ^ basepri;
	}
	s_basepri_stress.shared_mix = mix;
	basepri_stress_count(&s_basepri_stress.critical_entries);

	rtos_enable_int(old_basepri);

	restored_basepri = bk_arch_get_basepri();
	if (restored_basepri != old_basepri) {
		basepri_stress_count(&s_basepri_stress.basepri_restore_mismatch);
	}
	if ((old_basepri == 0UL) && (restored_basepri != 0UL)) {
		basepri_stress_count(&s_basepri_stress.basepri_leak_after_restore);
	}
}

static void basepri_stress_timer_cb(void *arg)
{
	(void)arg;

	if (s_basepri_stress.running == 0UL) {
		return;
	}

	basepri_stress_count(&s_basepri_stress.timer_entries);
	basepri_stress_critical(0x54494d52UL);
}

static void basepri_stress_worker(void *arg)
{
	basepri_stress_worker_t *worker = (basepri_stress_worker_t *)arg;
	uint32_t core;

	while (s_basepri_stress.stop_req == 0UL) {
		basepri_stress_critical(worker->id);
		worker->loops++;

		core = rtos_get_core_id() & 0x1UL;
		if (core == SMP_CORE0_ID) {
			worker->core0_runs++;
		} else {
			worker->core1_runs++;
		}

		if (s_basepri_stress.worker_delay_ms > 0UL) {
			rtos_delay_milliseconds(s_basepri_stress.worker_delay_ms);
		} else {
			rtos_delay_milliseconds(1);
		}
	}

	worker->thread = NULL;
	rtos_delete_thread(NULL);
}

static void basepri_stress_hotplug_worker(void *arg)
{
	bk_err_t ret;
	uint32_t delay_ms;

	(void)arg;

	while (s_basepri_stress.stop_req == 0UL) {
		delay_ms = s_basepri_stress.hotplug_period_ms;
		if (delay_ms == 0UL) {
			break;
		}

		rtos_delay_milliseconds(delay_ms);
		if (s_basepri_stress.stop_req != 0UL) {
			break;
		}

#if CONFIG_CPU_HOTPLUG
		ret = bk_cpu_hp_offline(CPU1_CORE_ID);
		if (ret != BK_OK) {
			if (ret == BK_ERR_BUSY) {
				basepri_stress_count(&s_basepri_stress.hotplug_busy);
			} else {
				basepri_stress_count(&s_basepri_stress.hotplug_failures);
			}
			continue;
		}

		basepri_stress_critical(0x48504f46UL);

		ret = bk_cpu_hp_online(CPU1_CORE_ID);
		if (ret != BK_OK) {
			if (ret == BK_ERR_BUSY) {
				basepri_stress_count(&s_basepri_stress.hotplug_busy);
			} else {
				basepri_stress_count(&s_basepri_stress.hotplug_failures);
			}
			continue;
		}

		basepri_stress_count(&s_basepri_stress.hotplug_loops);
#else
		basepri_stress_count(&s_basepri_stress.hotplug_failures);
		break;
#endif
	}

	s_basepri_stress.hotplug_thread = NULL;
	rtos_delete_thread(NULL);
}

static void basepri_stress_print_stat(void)
{
	uint32_t i;
	uint32_t now_ms;
	uint32_t elapsed_ms;

	now_ms = rtos_get_time();
	elapsed_ms = now_ms - s_basepri_stress.started_ms;

	CLI_LOGI("basepri_stress running=%u elapsed_ms=%u workers=%u delay_ms=%u timer_ms=%u hotplug_ms=%u\r\n",
		s_basepri_stress.running, elapsed_ms, s_basepri_stress.worker_count,
		s_basepri_stress.worker_delay_ms, s_basepri_stress.timer_period_ms,
		s_basepri_stress.hotplug_period_ms);
	CLI_LOGI("basepri_stress critical=%u timer=%u hotplug_loops=%u hotplug_busy=%u hotplug_fail=%u mix=0x%x\r\n",
		s_basepri_stress.critical_entries, s_basepri_stress.timer_entries,
		s_basepri_stress.hotplug_loops, s_basepri_stress.hotplug_busy,
		s_basepri_stress.hotplug_failures, s_basepri_stress.shared_mix);
	CLI_LOGI("basepri_stress basepri_not_raised=%u primask_nonzero=%u restore_mismatch=%u leak_after_restore=%u last=0x%x max=0x%x\r\n",
		s_basepri_stress.basepri_not_raised, s_basepri_stress.primask_nonzero,
		s_basepri_stress.basepri_restore_mismatch,
		s_basepri_stress.basepri_leak_after_restore,
		s_basepri_stress.last_basepri, s_basepri_stress.max_basepri);

	for (i = 0; i < s_basepri_stress.worker_count; i++) {
		CLI_LOGI("basepri_stress worker%u loops=%u core0=%u core1=%u active=%u\r\n",
			i, s_basepri_stress.workers[i].loops,
			s_basepri_stress.workers[i].core0_runs,
			s_basepri_stress.workers[i].core1_runs,
			(s_basepri_stress.workers[i].thread != NULL));
	}
}

static bk_err_t basepri_stress_start_workers(void)
{
	bk_err_t ret;
	uint32_t i;
	basepri_stress_worker_t *worker;

	for (i = 0; i < s_basepri_stress.worker_count; i++) {
		worker = &s_basepri_stress.workers[i];
		worker->id = i + 1;
		worker->loops = 0;
		worker->core0_runs = 0;
		worker->core1_runs = 0;

		if ((i == 0UL) || (s_basepri_stress.hotplug_period_ms != 0UL)) {
			ret = rtos_core0_create_thread(&worker->thread, BEKEN_DEFAULT_WORKER_PRIORITY,
				"bpstr_c0", basepri_stress_worker, BASEPRI_STRESS_STACK_SIZE, worker);
		} else {
			ret = rtos_smp_create_thread(&worker->thread, BEKEN_DEFAULT_WORKER_PRIORITY,
				"bpstr_smp", basepri_stress_worker, BASEPRI_STRESS_STACK_SIZE, worker);
		}

		if (ret != BK_OK) {
			CLI_LOGE("basepri_stress create worker%u failed, ret=%d\r\n", i, ret);
			return ret;
		}
	}

	return BK_OK;
}

static bk_err_t basepri_stress_start_timer(void)
{
	bk_err_t ret;

	ret = rtos_init_timer(&s_basepri_stress.timer, s_basepri_stress.timer_period_ms,
		basepri_stress_timer_cb, NULL);
	if (ret != BK_OK) {
		CLI_LOGE("basepri_stress init timer failed, ret=%d\r\n", ret);
		return ret;
	}

	s_basepri_stress.timer_inited = 1;
	ret = rtos_start_timer(&s_basepri_stress.timer);
	if (ret != BK_OK) {
		CLI_LOGE("basepri_stress start timer failed, ret=%d\r\n", ret);
		return ret;
	}

	return BK_OK;
}

static bk_err_t basepri_stress_start_hotplug(void)
{
	bk_err_t ret;

	if (s_basepri_stress.hotplug_period_ms == 0UL) {
		return BK_OK;
	}

	ret = rtos_core0_create_thread(&s_basepri_stress.hotplug_thread,
		BEKEN_DEFAULT_WORKER_PRIORITY, "bpstr_hp", basepri_stress_hotplug_worker,
		BASEPRI_STRESS_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		CLI_LOGE("basepri_stress create hotplug thread failed, ret=%d\r\n", ret);
	}

	return ret;
}

static void basepri_stress_stop(void)
{
	uint32_t i;

	if (s_basepri_stress.running == 0UL) {
		CLI_LOGI("basepri_stress already stopped\r\n");
		return;
	}

	s_basepri_stress.stop_req = 1;

	if (s_basepri_stress.timer_inited != 0UL) {
		(void)rtos_stop_timer(&s_basepri_stress.timer);
		(void)rtos_deinit_timer(&s_basepri_stress.timer);
		s_basepri_stress.timer_inited = 0;
	}

	for (i = 0; i < 20; i++) {
		uint32_t active = 0;
		uint32_t j;

		if (s_basepri_stress.hotplug_thread != NULL) {
			active++;
		}
		for (j = 0; j < s_basepri_stress.worker_count; j++) {
			if (s_basepri_stress.workers[j].thread != NULL) {
				active++;
			}
		}
		if (active == 0UL) {
			break;
		}
		rtos_delay_milliseconds(20);
	}

	s_basepri_stress.running = 0;
	CLI_LOGI("basepri_stress stopped\r\n");
	basepri_stress_print_stat();
}

static void basepri_stress_start(uint32_t worker_count, uint32_t worker_delay_ms,
	uint32_t timer_period_ms, uint32_t hotplug_period_ms)
{
	bk_err_t ret;

	if (s_basepri_stress.running != 0UL) {
		CLI_LOGI("basepri_stress already running\r\n");
		basepri_stress_print_stat();
		return;
	}

	if (worker_count == 0UL) {
		worker_count = BASEPRI_STRESS_DEFAULT_WORKERS;
	}
	if (worker_count > BASEPRI_STRESS_MAX_WORKERS) {
		worker_count = BASEPRI_STRESS_MAX_WORKERS;
	}
	if (timer_period_ms == 0UL) {
		timer_period_ms = BASEPRI_STRESS_DEFAULT_TIMER_MS;
	}

	os_memset(&s_basepri_stress, 0, sizeof(s_basepri_stress));
	s_basepri_stress.running = 1;
	s_basepri_stress.worker_count = worker_count;
	s_basepri_stress.worker_delay_ms = worker_delay_ms;
	s_basepri_stress.timer_period_ms = timer_period_ms;
	s_basepri_stress.hotplug_period_ms = hotplug_period_ms;
	s_basepri_stress.started_ms = rtos_get_time();

	ret = basepri_stress_start_timer();
	if (ret != BK_OK) {
		basepri_stress_stop();
		return;
	}

	ret = basepri_stress_start_workers();
	if (ret != BK_OK) {
		basepri_stress_stop();
		return;
	}

	ret = basepri_stress_start_hotplug();
	if (ret != BK_OK) {
		basepri_stress_stop();
		return;
	}

	CLI_LOGI("basepri_stress started workers=%u delay_ms=%u timer_ms=%u hotplug_ms=%u\r\n",
		worker_count, worker_delay_ms, timer_period_ms, hotplug_period_ms);
}

static void basepri_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t workers = BASEPRI_STRESS_DEFAULT_WORKERS;
	uint32_t worker_delay_ms = BASEPRI_STRESS_DEFAULT_WORKER_DELAY_MS;
	uint32_t timer_ms = BASEPRI_STRESS_DEFAULT_TIMER_MS;
	uint32_t hotplug_ms = BASEPRI_STRESS_DEFAULT_HOTPLUG_MS;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		basepri_stress_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (argc >= 3) {
			workers = os_strtoul(argv[2], NULL, 10);
		}
		if (argc >= 4) {
			worker_delay_ms = os_strtoul(argv[3], NULL, 10);
		}
		if (argc >= 5) {
			timer_ms = os_strtoul(argv[4], NULL, 10);
		}
		if (argc >= 6) {
			hotplug_ms = os_strtoul(argv[5], NULL, 10);
		}

		basepri_stress_start(workers, worker_delay_ms, timer_ms, hotplug_ms);
		return;
	}

	if (os_strcmp(argv[1], "stat") == 0) {
		basepri_stress_print_stat();
		return;
	}

	if (os_strcmp(argv[1], "stop") == 0) {
		basepri_stress_stop();
		return;
	}

	basepri_stress_help();
}

static const struct cli_command s_basepri_stress_commands[] = {
	{"basepri_stress", "basepri_stress {start|stat|stop}", basepri_stress_cmd},
};

int cli_basepri_stress_init(void)
{
	return cli_register_commands(s_basepri_stress_commands, BASEPRI_STRESS_CMD_CNT);
}

#endif
