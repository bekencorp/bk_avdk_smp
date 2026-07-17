// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include <common/bk_include.h>
#include <os/os.h>
#include "cli.h"
#include "bk_arch.h"
#include "modules/pm.h"
#include <driver/aon_rtc.h>
#include <driver/aon_rtc_types.h>

#if (CONFIG_FREERTOS_USE_TICKLESS_IDLE >= 2) && CONFIG_PM_ENABLE

#define WFI_BP_STRESS_CMD_CNT (sizeof(s_wfi_bp_stress_commands) / sizeof(struct cli_command))
#define WFI_BP_STRESS_MAX_WORKERS (4)
#define WFI_BP_STRESS_STACK_SIZE (1536)
#define WFI_BP_STRESS_DEFAULT_WORKERS (2)
#define WFI_BP_STRESS_DEFAULT_WORKER_DELAY_MS (50)
#define WFI_BP_STRESS_DEFAULT_TIMER_MS (10)
#define WFI_BP_STRESS_DEFAULT_PM_PERIOD_MS (5000)
#define WFI_BP_STRESS_DEFAULT_WFI_PERIOD_MS (200)
#define WFI_BP_STRESS_DEFAULT_PM_RTC_MS (1000)
#define WFI_BP_STRESS_CRITICAL_SPINS (64)

typedef struct {
	beken_thread_t thread;
	uint32_t id;
	uint32_t loops;
} wfi_bp_stress_worker_t;

typedef struct {
	volatile uint32_t running;
	volatile uint32_t stop_req;
	volatile uint32_t shared_mix;
	uint32_t started_ms;
	uint32_t worker_count;
	uint32_t worker_delay_ms;
	uint32_t timer_period_ms;
	uint32_t pm_period_ms;
	uint32_t wfi_period_ms;
	uint32_t pm_rtc_ms;
	beken_timer_t timer;
	uint32_t timer_inited;
	beken_thread_t pm_thread;
	beken_thread_t wfi_thread;
	wfi_bp_stress_worker_t workers[WFI_BP_STRESS_MAX_WORKERS];
	volatile uint32_t critical_entries;
	volatile uint32_t timer_entries;
	volatile uint32_t wfi_loops;
	volatile uint32_t pm_vote_loops;
	volatile uint32_t pm_vote_failures;
	volatile uint32_t basepri_not_raised;
	volatile uint32_t primask_nonzero;
	volatile uint32_t basepri_restore_mismatch;
	volatile uint32_t basepri_leak_after_restore;
	volatile uint32_t wfi_basepri_before_sleep;
	volatile uint32_t last_basepri;
	volatile uint32_t max_basepri;
} wfi_bp_stress_ctx_t;

static wfi_bp_stress_ctx_t s_wfi_bp_stress;
static uint32_t s_wfi_bp_pm_inited;

static void wfi_bp_stress_help(void)
{
	CLI_LOGI("wfi_basepri_stress start [workers] [worker_delay_ms] [timer_ms] [pm_period_ms] [wfi_period_ms] [pm_rtc_ms]\r\n");
	CLI_LOGI("wfi_basepri_stress stat\r\n");
	CLI_LOGI("wfi_basepri_stress stop\r\n");
	CLI_LOGI("  worker_delay_ms>0 lets idle task enter tickless WFI\r\n");
	CLI_LOGI("  pm_period_ms=0 disables PM low-voltage vote loop\r\n");
	CLI_LOGI("  wfi_period_ms=0 disables direct arch_sleep probe loop\r\n");
}

static void wfi_bp_stress_count(volatile uint32_t *counter)
{
	(void)__sync_fetch_and_add(counter, 1);
}

static void wfi_bp_stress_critical(uint32_t source)
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

	s_wfi_bp_stress.last_basepri = basepri;
	if (basepri > s_wfi_bp_stress.max_basepri) {
		s_wfi_bp_stress.max_basepri = basepri;
	}
	if (basepri == 0UL) {
		wfi_bp_stress_count(&s_wfi_bp_stress.basepri_not_raised);
	}
	if (primask != 0UL) {
		wfi_bp_stress_count(&s_wfi_bp_stress.primask_nonzero);
	}

	mix = s_wfi_bp_stress.shared_mix ^ source;
	for (i = 0; i < WFI_BP_STRESS_CRITICAL_SPINS; i++) {
		mix = (mix << 5) ^ (mix >> 2) ^ i ^ basepri;
	}
	s_wfi_bp_stress.shared_mix = mix;
	wfi_bp_stress_count(&s_wfi_bp_stress.critical_entries);

	rtos_enable_int(old_basepri);

	restored_basepri = bk_arch_get_basepri();
	if (restored_basepri != old_basepri) {
		wfi_bp_stress_count(&s_wfi_bp_stress.basepri_restore_mismatch);
	}
	if ((old_basepri == 0UL) && (restored_basepri != 0UL)) {
		wfi_bp_stress_count(&s_wfi_bp_stress.basepri_leak_after_restore);
	}
}

static void wfi_bp_stress_timer_cb(void *arg)
{
	(void)arg;

	if (s_wfi_bp_stress.running == 0UL) {
		return;
	}

	wfi_bp_stress_count(&s_wfi_bp_stress.timer_entries);
	wfi_bp_stress_critical(0x54494d52UL);
}

static void wfi_bp_stress_worker(void *arg)
{
	wfi_bp_stress_worker_t *worker = (wfi_bp_stress_worker_t *)arg;

	while (s_wfi_bp_stress.stop_req == 0UL) {
		wfi_bp_stress_critical(worker->id);
		worker->loops++;

		if (s_wfi_bp_stress.worker_delay_ms > 0UL) {
			rtos_delay_milliseconds(s_wfi_bp_stress.worker_delay_ms);
		} else {
			rtos_delay_milliseconds(1);
		}
	}

	worker->thread = NULL;
	rtos_delete_thread(NULL);
}

#if CONFIG_AON_RTC
static void wfi_bp_stress_pm_rtc_cb(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	pm_ap_core_msg_t msg;

	(void)id;
	(void)name_p;
	(void)param;

	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
	msg.event = PM_CALLBACK_HANDLE_MSG;
	msg.param1 = PM_MODE_LOW_VOLTAGE;
	msg.param2 = PM_WAKEUP_SOURCE_INT_RTC;
	msg.param3 = 2;
	bk_pm_send_msg(&msg);
}
#endif

static bk_err_t wfi_bp_stress_setup_pm_once(uint32_t rtc_ms)
{
#if CONFIG_AON_RTC
	alarm_info_t alarm = {0};
	bk_err_t ret;

	if (s_wfi_bp_pm_inited != 0UL) {
		return BK_OK;
	}

	if (rtc_ms < 500UL) {
		rtc_ms = 500UL;
	}

	memcpy(alarm.name, "wfi_bp", sizeof("wfi_bp"));
	alarm.period_tick = rtc_ms * AON_RTC_MS_TICK_CNT;
	alarm.period_cnt = ALARM_LOOP_FOREVER;
	alarm.callback = wfi_bp_stress_pm_rtc_cb;
	alarm.param_p = NULL;

	bk_alarm_unregister(AON_RTC_ID_1, alarm.name);
	ret = bk_alarm_register(AON_RTC_ID_1, &alarm);
	if (ret != BK_OK) {
		CLI_LOGE("wfi_basepri_stress rtc alarm register failed, ret=%d\r\n", ret);
		return ret;
	}
#endif

	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
	s_wfi_bp_pm_inited = 1;
	return BK_OK;
}

static void wfi_bp_stress_pm_worker(void *arg)
{
	bk_err_t ret;

	(void)arg;

	while (s_wfi_bp_stress.stop_req == 0UL) {
		if (s_wfi_bp_stress.pm_period_ms == 0UL) {
			break;
		}

		rtos_delay_milliseconds(s_wfi_bp_stress.pm_period_ms);
		if (s_wfi_bp_stress.stop_req != 0UL) {
			break;
		}

		ret = wfi_bp_stress_setup_pm_once(s_wfi_bp_stress.pm_rtc_ms);
		if (ret != BK_OK) {
			wfi_bp_stress_count(&s_wfi_bp_stress.pm_vote_failures);
			continue;
		}

		ret = bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
		if (ret != BK_OK) {
			wfi_bp_stress_count(&s_wfi_bp_stress.pm_vote_failures);
			continue;
		}

		wfi_bp_stress_critical(0x504d4c56UL);

		bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);
		ret = bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);
		if (ret != BK_OK) {
			wfi_bp_stress_count(&s_wfi_bp_stress.pm_vote_failures);
			continue;
		}

		wfi_bp_stress_count(&s_wfi_bp_stress.pm_vote_loops);
	}

	s_wfi_bp_stress.pm_thread = NULL;
	rtos_delete_thread(NULL);
}

static void wfi_bp_stress_wfi_worker(void *arg)
{
	uint32_t saved_primask;
	uint32_t basepri;

	(void)arg;

	while (s_wfi_bp_stress.stop_req == 0UL) {
		if (s_wfi_bp_stress.wfi_period_ms == 0UL) {
			break;
		}

		rtos_delay_milliseconds(s_wfi_bp_stress.wfi_period_ms);
		if (s_wfi_bp_stress.stop_req != 0UL) {
			break;
		}

		saved_primask = __get_PRIMASK();
		__disable_irq();
		basepri = bk_arch_get_basepri();
		if ((saved_primask == 0UL) && (basepri != 0UL)) {
			wfi_bp_stress_count(&s_wfi_bp_stress.wfi_basepri_before_sleep);
		}

		arch_sleep();

		if (bk_arch_get_basepri() != 0UL) {
			wfi_bp_stress_count(&s_wfi_bp_stress.basepri_leak_after_restore);
		}

		__set_PRIMASK(saved_primask);
		wfi_bp_stress_count(&s_wfi_bp_stress.wfi_loops);
	}

	s_wfi_bp_stress.wfi_thread = NULL;
	rtos_delete_thread(NULL);
}

static void wfi_bp_stress_print_stat(void)
{
	uint32_t i;
	uint32_t now_ms;
	uint32_t elapsed_ms;

	now_ms = rtos_get_time();
	elapsed_ms = now_ms - s_wfi_bp_stress.started_ms;

	CLI_LOGI("wfi_basepri_stress running=%u elapsed_ms=%u workers=%u delay_ms=%u timer_ms=%u pm_ms=%u wfi_ms=%u pm_rtc_ms=%u\r\n",
		s_wfi_bp_stress.running, elapsed_ms, s_wfi_bp_stress.worker_count,
		s_wfi_bp_stress.worker_delay_ms, s_wfi_bp_stress.timer_period_ms,
		s_wfi_bp_stress.pm_period_ms, s_wfi_bp_stress.wfi_period_ms,
		s_wfi_bp_stress.pm_rtc_ms);
	CLI_LOGI("wfi_basepri_stress critical=%u timer=%u wfi_loops=%u pm_vote=%u pm_fail=%u mix=0x%x\r\n",
		s_wfi_bp_stress.critical_entries, s_wfi_bp_stress.timer_entries,
		s_wfi_bp_stress.wfi_loops, s_wfi_bp_stress.pm_vote_loops,
		s_wfi_bp_stress.pm_vote_failures, s_wfi_bp_stress.shared_mix);
	CLI_LOGI("wfi_basepri_stress basepri_not_raised=%u primask_nonzero=%u restore_mismatch=%u leak_after_restore=%u wfi_basepri=%u last=0x%x max=0x%x\r\n",
		s_wfi_bp_stress.basepri_not_raised, s_wfi_bp_stress.primask_nonzero,
		s_wfi_bp_stress.basepri_restore_mismatch,
		s_wfi_bp_stress.basepri_leak_after_restore,
		s_wfi_bp_stress.wfi_basepri_before_sleep,
		s_wfi_bp_stress.last_basepri, s_wfi_bp_stress.max_basepri);

	for (i = 0; i < s_wfi_bp_stress.worker_count; i++) {
		CLI_LOGI("wfi_basepri_stress worker%u loops=%u active=%u\r\n",
			i, s_wfi_bp_stress.workers[i].loops,
			(s_wfi_bp_stress.workers[i].thread != NULL));
	}
}

static bk_err_t wfi_bp_stress_start_workers(void)
{
	bk_err_t ret;
	uint32_t i;
	wfi_bp_stress_worker_t *worker;

	for (i = 0; i < s_wfi_bp_stress.worker_count; i++) {
		worker = &s_wfi_bp_stress.workers[i];
		worker->id = i + 1;
		worker->loops = 0;

		ret = rtos_create_thread(&worker->thread, BEKEN_DEFAULT_WORKER_PRIORITY,
			"wfi_bp", wfi_bp_stress_worker, WFI_BP_STRESS_STACK_SIZE, worker);
		if (ret != BK_OK) {
			CLI_LOGE("wfi_basepri_stress create worker%u failed, ret=%d\r\n", i, ret);
			return ret;
		}
	}

	return BK_OK;
}

static bk_err_t wfi_bp_stress_start_timer(void)
{
	bk_err_t ret;

	ret = rtos_init_timer(&s_wfi_bp_stress.timer, s_wfi_bp_stress.timer_period_ms,
		wfi_bp_stress_timer_cb, NULL);
	if (ret != BK_OK) {
		CLI_LOGE("wfi_basepri_stress init timer failed, ret=%d\r\n", ret);
		return ret;
	}

	s_wfi_bp_stress.timer_inited = 1;
	ret = rtos_start_timer(&s_wfi_bp_stress.timer);
	if (ret != BK_OK) {
		CLI_LOGE("wfi_basepri_stress start timer failed, ret=%d\r\n", ret);
		return ret;
	}

	return BK_OK;
}

static bk_err_t wfi_bp_stress_start_pm(void)
{
	bk_err_t ret;

	if (s_wfi_bp_stress.pm_period_ms == 0UL) {
		return BK_OK;
	}

	ret = rtos_create_thread(&s_wfi_bp_stress.pm_thread, BEKEN_DEFAULT_WORKER_PRIORITY,
		"wfi_pm", wfi_bp_stress_pm_worker, WFI_BP_STRESS_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		CLI_LOGE("wfi_basepri_stress create pm thread failed, ret=%d\r\n", ret);
	}

	return ret;
}

static bk_err_t wfi_bp_stress_start_wfi(void)
{
	bk_err_t ret;

	if (s_wfi_bp_stress.wfi_period_ms == 0UL) {
		return BK_OK;
	}

	ret = rtos_create_thread(&s_wfi_bp_stress.wfi_thread, BEKEN_DEFAULT_WORKER_PRIORITY,
		"wfi_arc", wfi_bp_stress_wfi_worker, WFI_BP_STRESS_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		CLI_LOGE("wfi_basepri_stress create wfi thread failed, ret=%d\r\n", ret);
	}

	return ret;
}

static void wfi_bp_stress_stop(void)
{
	uint32_t i;

	if (s_wfi_bp_stress.running == 0UL) {
		CLI_LOGI("wfi_basepri_stress already stopped\r\n");
		return;
	}

	s_wfi_bp_stress.stop_req = 1;

	if (s_wfi_bp_stress.timer_inited != 0UL) {
		(void)rtos_stop_timer(&s_wfi_bp_stress.timer);
		(void)rtos_deinit_timer(&s_wfi_bp_stress.timer);
		s_wfi_bp_stress.timer_inited = 0;
	}

#if CONFIG_AON_RTC
	(void)bk_alarm_unregister(AON_RTC_ID_1, (uint8_t *)"wfi_bp");
#endif
	s_wfi_bp_pm_inited = 0;
	(void)bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
	bk_pm_sleep_mode_set(PM_MODE_DEFAULT);

	for (i = 0; i < 30; i++) {
		uint32_t active = 0;
		uint32_t j;

		if (s_wfi_bp_stress.pm_thread != NULL) {
			active++;
		}
		if (s_wfi_bp_stress.wfi_thread != NULL) {
			active++;
		}
		for (j = 0; j < s_wfi_bp_stress.worker_count; j++) {
			if (s_wfi_bp_stress.workers[j].thread != NULL) {
				active++;
			}
		}
		if (active == 0UL) {
			break;
		}
		rtos_delay_milliseconds(20);
	}

	s_wfi_bp_stress.running = 0;
	CLI_LOGI("wfi_basepri_stress stopped\r\n");
	wfi_bp_stress_print_stat();
}

static void wfi_bp_stress_start(uint32_t worker_count, uint32_t worker_delay_ms,
	uint32_t timer_period_ms, uint32_t pm_period_ms, uint32_t wfi_period_ms,
	uint32_t pm_rtc_ms)
{
	bk_err_t ret;

	if (s_wfi_bp_stress.running != 0UL) {
		CLI_LOGI("wfi_basepri_stress already running\r\n");
		wfi_bp_stress_print_stat();
		return;
	}

	if (worker_count == 0UL) {
		worker_count = WFI_BP_STRESS_DEFAULT_WORKERS;
	}
	if (worker_count > WFI_BP_STRESS_MAX_WORKERS) {
		worker_count = WFI_BP_STRESS_MAX_WORKERS;
	}
	if (timer_period_ms == 0UL) {
		timer_period_ms = WFI_BP_STRESS_DEFAULT_TIMER_MS;
	}
	if (pm_period_ms == 0UL) {
		pm_period_ms = WFI_BP_STRESS_DEFAULT_PM_PERIOD_MS;
	}
	if (wfi_period_ms == 0UL) {
		wfi_period_ms = WFI_BP_STRESS_DEFAULT_WFI_PERIOD_MS;
	}
	if (pm_rtc_ms == 0UL) {
		pm_rtc_ms = WFI_BP_STRESS_DEFAULT_PM_RTC_MS;
	}

	os_memset(&s_wfi_bp_stress, 0, sizeof(s_wfi_bp_stress));
	s_wfi_bp_pm_inited = 0;
	s_wfi_bp_stress.running = 1;
	s_wfi_bp_stress.worker_count = worker_count;
	s_wfi_bp_stress.worker_delay_ms = worker_delay_ms;
	s_wfi_bp_stress.timer_period_ms = timer_period_ms;
	s_wfi_bp_stress.pm_period_ms = pm_period_ms;
	s_wfi_bp_stress.wfi_period_ms = wfi_period_ms;
	s_wfi_bp_stress.pm_rtc_ms = pm_rtc_ms;
	s_wfi_bp_stress.started_ms = rtos_get_time();

	ret = wfi_bp_stress_start_timer();
	if (ret != BK_OK) {
		wfi_bp_stress_stop();
		return;
	}

	ret = wfi_bp_stress_start_workers();
	if (ret != BK_OK) {
		wfi_bp_stress_stop();
		return;
	}

	ret = wfi_bp_stress_start_pm();
	if (ret != BK_OK) {
		CLI_LOGW("wfi_basepri_stress pm loop disabled, ret=%d\r\n", ret);
		s_wfi_bp_stress.pm_period_ms = 0;
	}

	ret = wfi_bp_stress_start_wfi();
	if (ret != BK_OK) {
		wfi_bp_stress_stop();
		return;
	}

	CLI_LOGI("wfi_basepri_stress started workers=%u delay_ms=%u timer_ms=%u pm_ms=%u wfi_ms=%u pm_rtc_ms=%u\r\n",
		worker_count, worker_delay_ms, timer_period_ms, pm_period_ms, wfi_period_ms, pm_rtc_ms);
}

static void wfi_bp_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t workers = WFI_BP_STRESS_DEFAULT_WORKERS;
	uint32_t worker_delay_ms = WFI_BP_STRESS_DEFAULT_WORKER_DELAY_MS;
	uint32_t timer_ms = WFI_BP_STRESS_DEFAULT_TIMER_MS;
	uint32_t pm_ms = WFI_BP_STRESS_DEFAULT_PM_PERIOD_MS;
	uint32_t wfi_ms = WFI_BP_STRESS_DEFAULT_WFI_PERIOD_MS;
	uint32_t pm_rtc_ms = WFI_BP_STRESS_DEFAULT_PM_RTC_MS;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		wfi_bp_stress_help();
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
			pm_ms = os_strtoul(argv[5], NULL, 10);
		}
		if (argc >= 7) {
			wfi_ms = os_strtoul(argv[6], NULL, 10);
		}
		if (argc >= 8) {
			pm_rtc_ms = os_strtoul(argv[7], NULL, 10);
		}

		wfi_bp_stress_start(workers, worker_delay_ms, timer_ms, pm_ms, wfi_ms, pm_rtc_ms);
		return;
	}

	if (os_strcmp(argv[1], "stat") == 0) {
		wfi_bp_stress_print_stat();
		return;
	}

	if (os_strcmp(argv[1], "stop") == 0) {
		wfi_bp_stress_stop();
		return;
	}

	wfi_bp_stress_help();
}

static const struct cli_command s_wfi_bp_stress_commands[] = {
	{"wfi_basepri_stress", "wfi_basepri_stress {start|stat|stop}", wfi_bp_stress_cmd},
};

int cli_wfi_basepri_stress_init(void)
{
	return cli_register_commands(s_wfi_bp_stress_commands, WFI_BP_STRESS_CMD_CNT);
}

#endif
