#include "cli.h"
#include "cli_section.h"
#include <common/bk_assert.h>
#include <driver/aon_rtc.h>
#include <driver/flash.h>
#include <driver/ipi_driver.h>
#include <driver/timer.h>
#include <os/os.h>
#include <os/str.h>
#include <stdint.h>
#include "sys_sw_regs.h"

#define APP_DUMP_CASE_ID_MAX_LEN 31U
#define APP_DUMP_TASK_STACK_SIZE 2048U
#define APP_DUMP_TIMER_DELAY_MS  10U
#define APP_DUMP_IPI_WAIT_MS     1000U
#define APP_DUMP_IPI_EVENT       0xD1U
#define APP_DUMP_IPI_PAYLOAD     0x7259U
#define APP_DUMP_FLASH_SECTOR_SIZE 4096U
#define APP_DUMP_FLASH_PAGE_SIZE   256U
#define APP_DUMP_FLASH_MAX_LEN     (64U * 1024U)
#define APP_DUMP_FLASH_MAX_CYCLES  16U
#define APP_DUMP_FOLLOW_WAIT_MS    30000U
#define APP_DUMP_FOLLOW_PRIORITY   BEKEN_APPLICATION_PRIORITY
#define APP_DUMP_RACE_READY_MS     1000U
#define APP_DUMP_RACE_FOLLOW_DELAY_US 1000ULL

typedef enum {
	APP_DUMP_CONTEXT_TASK = 0,
	APP_DUMP_CONTEXT_ISR,
	APP_DUMP_CONTEXT_CRITICAL,
} app_dump_context_t;

typedef enum {
	APP_DUMP_FAULT_ASSERT = 0,
	APP_DUMP_FAULT_CRASH,
	APP_DUMP_FAULT_BADPC,
	APP_DUMP_FAULT_UDF,
	APP_DUMP_FAULT_DIVZERO,
} app_dump_fault_t;

typedef struct {
	const char *name;
	app_dump_context_t context;
	app_dump_fault_t fault;
} app_dump_mode_t;

typedef struct {
	char case_id[APP_DUMP_CASE_ID_MAX_LEN + 1U];
	const app_dump_mode_t *mode;
	uint8_t core;
} app_dump_request_t;

typedef struct {
	char case_id[APP_DUMP_CASE_ID_MAX_LEN + 1U];
	uint32_t address;
	uint32_t length;
} app_dump_flash_request_t;

typedef struct {
	char case_id[APP_DUMP_CASE_ID_MAX_LEN + 1U];
	const app_dump_mode_t *mode;
	uint8_t core;
	uint8_t scenario;
} app_dump_follow_request_t;

typedef struct {
	char case_id[APP_DUMP_CASE_ID_MAX_LEN + 1U];
	const app_dump_mode_t *mode;
	uint8_t preferred_core;
	uint8_t scenario;
} app_dump_race_request_t;

typedef enum {
	APP_DUMP_IPI_WAITING = 0,
	APP_DUMP_IPI_ENTERED,
	APP_DUMP_IPI_ARMED,
	APP_DUMP_IPI_ABORTED,
	APP_DUMP_IPI_BAD_CORE,
	APP_DUMP_IPI_DONE,
} app_dump_ipi_state_t;

static app_dump_request_t s_request;
static volatile bool s_busy;
static volatile bool s_timer_fired;
static volatile app_dump_ipi_state_t s_ipi_state;
static app_dump_flash_request_t s_flash_request;
static volatile bool s_flash_busy;
static uint8_t s_flash_write_buffer[APP_DUMP_FLASH_PAGE_SIZE];
static uint8_t s_flash_read_buffer[APP_DUMP_FLASH_PAGE_SIZE];
static app_dump_follow_request_t s_follow_request;
static app_dump_race_request_t s_race_request;
static volatile bool s_orch_busy;
static volatile uint32_t s_race_ready;
static volatile bool s_race_start;
static volatile bool s_race_abort;

static const app_dump_mode_t s_app_dump_modes[] = {
	{"task_assert", APP_DUMP_CONTEXT_TASK, APP_DUMP_FAULT_ASSERT},
	{"isr_assert", APP_DUMP_CONTEXT_ISR, APP_DUMP_FAULT_ASSERT},
	{"isr_crash", APP_DUMP_CONTEXT_ISR, APP_DUMP_FAULT_CRASH},
	{"critical_assert", APP_DUMP_CONTEXT_CRITICAL, APP_DUMP_FAULT_ASSERT},
	{"critical_crash", APP_DUMP_CONTEXT_CRITICAL, APP_DUMP_FAULT_CRASH},
	{"task_badpc", APP_DUMP_CONTEXT_TASK, APP_DUMP_FAULT_BADPC},
	{"task_udf", APP_DUMP_CONTEXT_TASK, APP_DUMP_FAULT_UDF},
	{"task_divzero", APP_DUMP_CONTEXT_TASK, APP_DUMP_FAULT_DIVZERO},
	{"isr_badpc", APP_DUMP_CONTEXT_ISR, APP_DUMP_FAULT_BADPC},
	{"isr_udf", APP_DUMP_CONTEXT_ISR, APP_DUMP_FAULT_UDF},
	{"isr_divzero", APP_DUMP_CONTEXT_ISR, APP_DUMP_FAULT_DIVZERO},
	{"critical_badpc", APP_DUMP_CONTEXT_CRITICAL, APP_DUMP_FAULT_BADPC},
	{"critical_udf", APP_DUMP_CONTEXT_CRITICAL, APP_DUMP_FAULT_UDF},
	{"critical_divzero", APP_DUMP_CONTEXT_CRITICAL, APP_DUMP_FAULT_DIVZERO},
};

static bool app_dump_get_case_id_len(const char *case_id, size_t *length)
{
	size_t len = 0;

	while ((len <= APP_DUMP_CASE_ID_MAX_LEN) && (case_id[len] != '\0')) {
		len++;
	}
	if ((len == 0U) || (len > APP_DUMP_CASE_ID_MAX_LEN)) {
		return false;
	}

	*length = len;
	return true;
}

static const app_dump_mode_t *app_dump_parse_mode(const char *name)
{
	size_t index;

	for (index = 0; index < (sizeof(s_app_dump_modes) /
		sizeof(s_app_dump_modes[0])); index++) {
		if (os_strcmp(name, s_app_dump_modes[index].name) == 0) {
			return &s_app_dump_modes[index];
		}
	}
	return NULL;
}

static uint32_t app_dump_parse_follow_scenario(const char *name,
	uint8_t core)
{
	if ((os_strcmp(name, "cp_ap") == 0) &&
		((core == 0U) || (core == 1U))) {
		return BK_SYS_SW_REGS_DUMP_TEST_CP_OWNER_AP_FOLLOWER;
	}
	if ((os_strcmp(name, "ap0_ap1") == 0) && (core == 1U)) {
		return BK_SYS_SW_REGS_DUMP_TEST_AP0_OWNER_AP1_FOLLOWER;
	}
	if ((os_strcmp(name, "ap1_ap0") == 0) && (core == 0U)) {
		return BK_SYS_SW_REGS_DUMP_TEST_AP1_OWNER_AP0_FOLLOWER;
	}
	return BK_SYS_SW_REGS_DUMP_TEST_SCENARIO_NONE;
}

static uint32_t app_dump_ap_test_core(uint8_t core)
{
	return (core == 0U) ? BK_SYS_SW_REGS_DUMP_TEST_CORE_AP0 :
		BK_SYS_SW_REGS_DUMP_TEST_CORE_AP1;
}

static void app_dump_print_begin(const app_dump_request_t *request)
{
	os_printf("DUMP_TEST_BEGIN case_id=%s target=AP core=%u mode=%s\r\n",
		request->case_id, (unsigned)request->core,
		request->mode->name);
}

static void app_dump_print_reject(const char *case_id, const char *reason)
{
	os_printf("DUMP_TEST_REJECT case_id=%s reason=%s\r\n", case_id, reason);
}

static void app_dump_print_reject_ret(const char *case_id, const char *reason,
	bk_err_t ret)
{
	os_printf("DUMP_TEST_REJECT case_id=%s reason=%s ret=%d\r\n",
		case_id, reason, (int)ret);
}

static void app_dump_release_request(void)
{
	uint32_t flags = rtos_enter_critical();
	s_busy = false;
	rtos_exit_critical(flags);
}

static void app_dump_crash(void)
{
	/* Deliberate data-access fault for dump testing. Keep the invalid access
	 * in assembly so static analysis does not report a C null dereference. */
	__asm volatile(
		"movs r0, #0\n"
		"ldr r1, =0xD00F7259\n"
		"str r1, [r0]\n"
		:
		:
		: "r0", "r1", "memory");
}

static void app_dump_badpc(void)
{
	/* Deliberate invalid-state branch for dump testing. */
	__asm volatile(
		"movs r0, #0\n"
		"bx r0\n"
		:
		:
		: "r0", "memory");
}

static void app_dump_udf(void)
{
	__asm volatile("udf #0");
}

static void app_dump_divzero(void)
{
	volatile int32_t numerator = 0x7259;
	volatile int32_t denominator = 0;
	volatile int32_t result = numerator / denominator;
	(void)result;
}

static void app_dump_trigger(app_dump_fault_t fault)
{
	switch (fault) {
	case APP_DUMP_FAULT_ASSERT:
		BK_ASSERT(false);
		break;
	case APP_DUMP_FAULT_CRASH:
		app_dump_crash();
		break;
	case APP_DUMP_FAULT_BADPC:
		app_dump_badpc();
		break;
	case APP_DUMP_FAULT_UDF:
		app_dump_udf();
		break;
	case APP_DUMP_FAULT_DIVZERO:
		app_dump_divzero();
		break;
	default:
		BK_ASSERT(false);
		break;
	}
}

static void app_dump_timer_isr(timer_id_t timer_id)
{
	if ((timer_id != TIMER_ID17) ||
		((rtos_get_core_id() & 1U) != (uint32_t)s_request.core)) {
		os_printf("DUMP_TEST_REJECT case_id=%s reason=wrong_timer_isr_core\r\n",
			s_request.case_id);
		s_timer_fired = true;
		return;
	}

	app_dump_trigger(s_request.mode->fault);
	s_timer_fired = true;
}

static void app_dump_ipi_callback(ipi_core_id_t core_id, uint32_t value,
	uint8_t src_cpu, uint8_t event, uint16_t payload, void *param)
{
	app_dump_ipi_state_t expected;

	(void)value;
	(void)src_cpu;
	(void)param;

	if ((core_id != IPI_AP_CORE1) || ((rtos_get_core_id() & 1U) != 1U) ||
		(event != APP_DUMP_IPI_EVENT) || (payload != APP_DUMP_IPI_PAYLOAD)) {
		expected = APP_DUMP_IPI_WAITING;
		(void)__atomic_compare_exchange_n(&s_ipi_state, &expected,
			APP_DUMP_IPI_BAD_CORE, false, __ATOMIC_ACQ_REL,
			__ATOMIC_ACQUIRE);
		return;
	}

	expected = APP_DUMP_IPI_WAITING;
	if (!__atomic_compare_exchange_n(&s_ipi_state, &expected,
		APP_DUMP_IPI_ENTERED, false, __ATOMIC_ACQ_REL,
		__ATOMIC_ACQUIRE)) {
		return;
	}

	while (__atomic_load_n(&s_ipi_state, __ATOMIC_ACQUIRE) ==
		APP_DUMP_IPI_ENTERED) {
		;
	}

	if (__atomic_load_n(&s_ipi_state, __ATOMIC_ACQUIRE) !=
		APP_DUMP_IPI_ARMED) {
		return;
	}

	app_dump_trigger(s_request.mode->fault);
	__atomic_store_n(&s_ipi_state, APP_DUMP_IPI_DONE, __ATOMIC_RELEASE);
}

static void app_dump_task(beken_thread_arg_t arg)
{
	const app_dump_request_t *request = (const app_dump_request_t *)arg;
	bk_err_t ret;
	uint32_t critical_flags;

	if ((rtos_get_core_id() & 1U) != (uint32_t)request->core) {
		app_dump_print_reject(request->case_id, "wrong_task_core");
		goto exit_task;
	}

	switch (request->mode->context) {
	case APP_DUMP_CONTEXT_TASK:
		app_dump_print_begin(request);
		app_dump_trigger(request->mode->fault);
		break;

	case APP_DUMP_CONTEXT_CRITICAL:
		app_dump_print_begin(request);
		/* Fault injection only needs local IRQ masking; avoid adding scheduler
		 * synchronization to the condition this case is intended to test. */
		critical_flags = rtos_disable_int();
		app_dump_trigger(request->mode->fault);
		rtos_enable_int(critical_flags);
		break;

	case APP_DUMP_CONTEXT_ISR:
		if (request->core != 0U) {
			app_dump_print_reject(request->case_id, "invalid_timer_core");
			break;
		}

		ret = bk_timer_driver_init();
		if (ret != BK_OK) {
			app_dump_print_reject_ret(request->case_id,
				"timer_init_failed", ret);
			break;
		}
		if ((bk_timer_get_enable_status() &
			(1U << (TIMER_ID17 - TIMER_ID12))) != 0U) {
			app_dump_print_reject(request->case_id, "timer_busy");
			break;
		}

		s_timer_fired = false;
		ret = bk_timer_start(TIMER_ID17, APP_DUMP_TIMER_DELAY_MS,
			app_dump_timer_isr);
		if (ret != BK_OK) {
			app_dump_print_reject_ret(request->case_id,
				"timer_start_failed", ret);
			break;
		}

		app_dump_print_begin(request);
		while (!s_timer_fired) {
			rtos_delay_milliseconds(1);
		}
		break;

	default:
		app_dump_print_reject(request->case_id, "invalid_mode");
		break;
	}

exit_task:
	app_dump_release_request();
	rtos_delete_thread(NULL);
}

static void app_dump_ipi_task(beken_thread_arg_t arg)
{
	const app_dump_request_t *request = (const app_dump_request_t *)arg;
	bk_err_t ret;
	uint32_t waited_ms = 0;
	bool callback_registered = false;
	app_dump_ipi_state_t state;
	app_dump_ipi_state_t expected;

	if ((rtos_get_core_id() & 1U) != 0U) {
		app_dump_print_reject(request->case_id, "wrong_ipi_sender_core");
		goto exit_task;
	}

	ret = bk_ipi_driver_init();
	if (ret != BK_OK) {
		app_dump_print_reject_ret(request->case_id, "ipi_init_failed", ret);
		goto exit_task;
	}

	ret = bk_ipi_register_domain_callback(IPI_DOMAIN_TEST,
		app_dump_ipi_callback, NULL);
	if (ret != BK_OK) {
		app_dump_print_reject_ret(request->case_id,
			"ipi_register_failed", ret);
		goto exit_task;
	}
	callback_registered = true;

	ret = bk_ipi_enable(IPI_AP_CORE1);
	if (ret != BK_OK) {
		app_dump_print_reject_ret(request->case_id, "ipi_enable_failed", ret);
		goto exit_task;
	}

	__atomic_store_n(&s_ipi_state, APP_DUMP_IPI_WAITING, __ATOMIC_RELEASE);
	ret = bk_ipi_send_domain(IPI_AP_CORE1, IPI_DOMAIN_TEST,
		APP_DUMP_IPI_EVENT, APP_DUMP_IPI_PAYLOAD);
	if (ret != BK_OK) {
		app_dump_print_reject_ret(request->case_id, "ipi_send_failed", ret);
		goto exit_task;
	}

	while ((__atomic_load_n(&s_ipi_state, __ATOMIC_ACQUIRE) ==
		APP_DUMP_IPI_WAITING) &&
		(waited_ms < APP_DUMP_IPI_WAIT_MS)) {
		rtos_delay_milliseconds(1);
		waited_ms++;
	}

	state = __atomic_load_n(&s_ipi_state, __ATOMIC_ACQUIRE);
	if (state == APP_DUMP_IPI_WAITING) {
		expected = APP_DUMP_IPI_WAITING;
		if (__atomic_compare_exchange_n(&s_ipi_state, &expected,
			APP_DUMP_IPI_ABORTED, false, __ATOMIC_ACQ_REL,
			__ATOMIC_ACQUIRE)) {
			state = APP_DUMP_IPI_ABORTED;
		} else {
			state = expected;
		}
	}

	if (state == APP_DUMP_IPI_BAD_CORE) {
		app_dump_print_reject(request->case_id, "wrong_ipi_callback_core");
		goto exit_task;
	}
	if (state != APP_DUMP_IPI_ENTERED) {
		app_dump_print_reject(request->case_id, "ipi_callback_timeout");
		goto exit_task;
	}

	app_dump_print_begin(request);
	rtos_delay_milliseconds(1);
	__atomic_store_n(&s_ipi_state, APP_DUMP_IPI_ARMED, __ATOMIC_RELEASE);
	while (__atomic_load_n(&s_ipi_state, __ATOMIC_ACQUIRE) !=
		APP_DUMP_IPI_DONE) {
		rtos_delay_milliseconds(1);
	}

exit_task:
	if (callback_registered) {
		(void)bk_ipi_unregister_domain_callback(IPI_DOMAIN_TEST);
	}
	app_dump_release_request();
	rtos_delete_thread(NULL);
}

static void app_dump_orch_release(void)
{
	uint32_t flags = rtos_enter_critical();

	s_orch_busy = false;
	rtos_exit_critical(flags);
}

static void app_dump_follow_task(beken_thread_arg_t arg)
{
	const app_dump_follow_request_t *request =
		(const app_dump_follow_request_t *)arg;
	bk_sys_sw_regs_dump_test_state_t state;
	uint64_t deadline_us;
	uint32_t test_core = app_dump_ap_test_core(request->core);

	if ((rtos_get_core_id() & 1U) != (uint32_t)request->core) {
		app_dump_print_reject(request->case_id, "wrong_follower_core");
		goto exit_task;
	}

	os_printf("DUMP_ORCH_FOLLOW_READY case_id=%s follower=AP%u\r\n",
		request->case_id, (unsigned)request->core);
	deadline_us = bk_aon_rtc_get_us()
		+ ((uint64_t)APP_DUMP_FOLLOW_WAIT_MS * 1000ULL);
	while (bk_aon_rtc_get_us() < deadline_us) {
		if (bk_sys_sw_regs_get_dump_test_state(&state) &&
			(state.scenario == request->scenario) &&
			(state.phase == BK_SYS_SW_REGS_DUMP_TEST_PHASE_OWNER) &&
			(state.follower == test_core)) {
			app_dump_trigger(request->mode->fault);
			goto exit_task;
		}
		/* Keep the follower executing on its low-priority core. Once the owner
		 * enters exception context, the SMP scheduler may no longer wake a
		 * task blocked on rtos_delay_milliseconds(). Higher-priority CLI and
		 * system work can still preempt this test-only spin. */
		__asm volatile("nop");
	}

	app_dump_print_reject(request->case_id, "follower_owner_timeout");
	bk_sys_sw_regs_dump_test_reset();

exit_task:
	app_dump_orch_release();
	rtos_delete_thread(NULL);
}

static void app_dump_race_task(beken_thread_arg_t arg)
{
	const app_dump_race_request_t *request =
		(const app_dump_race_request_t *)arg;
	uint32_t core = rtos_get_core_id() & 1U;
	uint64_t follower_deadline_us;

	(void)__atomic_add_fetch(&s_race_ready, 1U, __ATOMIC_ACQ_REL);
	while (!__atomic_load_n(&s_race_start, __ATOMIC_ACQUIRE) &&
		!__atomic_load_n(&s_race_abort, __ATOMIC_ACQUIRE)) {
		__asm volatile("nop");
	}
	if (!__atomic_load_n(&s_race_abort, __ATOMIC_ACQUIRE)) {
		if ((request->preferred_core <= 1U) &&
			(core != request->preferred_core)) {
			follower_deadline_us = bk_aon_rtc_get_us()
				+ APP_DUMP_RACE_FOLLOW_DELAY_US;
			while (bk_aon_rtc_get_us() < follower_deadline_us) {
				__asm volatile("nop");
			}
		}
		app_dump_trigger(request->mode->fault);
	}
	app_dump_orch_release();
	rtos_delete_thread(NULL);
}

static void app_dump_follow_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	const app_dump_mode_t *mode;
	bk_err_t ret;
	uint32_t flags;
	uint32_t scenario;
	uint32_t follower;
	size_t case_id_len;
	uint8_t core;

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if ((argc != 5) || (argv == NULL) || (argv[1] == NULL) ||
		(argv[2] == NULL) || (argv[3] == NULL) || (argv[4] == NULL)) {
		app_dump_print_reject("-", "follow_invalid_args");
		return;
	}
	if (!app_dump_get_case_id_len(argv[1], &case_id_len)) {
		app_dump_print_reject("-", "invalid_case_id");
		return;
	}
	mode = app_dump_parse_mode(argv[3]);
	if ((mode == NULL) || (mode->context != APP_DUMP_CONTEXT_TASK)) {
		app_dump_print_reject(argv[1], "follower_requires_task_mode");
		return;
	}
	if (os_strcmp(argv[4], "0") == 0) {
		core = 0U;
	} else if (os_strcmp(argv[4], "1") == 0) {
		core = 1U;
	} else {
		app_dump_print_reject(argv[1], "invalid_core");
		return;
	}
	scenario = app_dump_parse_follow_scenario(argv[2], core);
	if (scenario == BK_SYS_SW_REGS_DUMP_TEST_SCENARIO_NONE) {
		app_dump_print_reject(argv[1], "invalid_follow_scenario");
		return;
	}

	flags = rtos_enter_critical();
	if (s_orch_busy) {
		rtos_exit_critical(flags);
		app_dump_print_reject(argv[1], "orchestration_busy");
		return;
	}
	s_orch_busy = true;
	os_memcpy(s_follow_request.case_id, argv[1], case_id_len + 1U);
	s_follow_request.mode = mode;
	s_follow_request.core = core;
	s_follow_request.scenario = (uint8_t)scenario;
	rtos_exit_critical(flags);

	follower = app_dump_ap_test_core(core);
	if (bk_sys_sw_regs_dump_test_arm(scenario, follower) == 0U) {
		app_dump_print_reject(argv[1], "state_arm_failed");
		app_dump_orch_release();
		return;
	}
	if (core == 0U) {
		ret = rtos_core0_create_thread(NULL, APP_DUMP_FOLLOW_PRIORITY,
			"dump_follow0", app_dump_follow_task, APP_DUMP_TASK_STACK_SIZE,
			&s_follow_request);
	} else {
		ret = rtos_core1_create_thread(NULL, APP_DUMP_FOLLOW_PRIORITY,
			"dump_follow1", app_dump_follow_task, APP_DUMP_TASK_STACK_SIZE,
			&s_follow_request);
	}
	if (ret != BK_OK) {
		app_dump_print_reject_ret(argv[1], "follow_create_failed", ret);
		bk_sys_sw_regs_dump_test_reset();
		app_dump_orch_release();
		return;
	}
	os_printf("DUMP_ORCH_ARMED case_id=%s scenario=%s follower=AP%u mode=%s\r\n",
		argv[1], argv[2], (unsigned)core, mode->name);
}

static void app_dump_race_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	const app_dump_mode_t *mode;
	bk_err_t ret0;
	bk_err_t ret1;
	uint32_t flags;
	uint32_t follower;
	uint32_t scenario;
	uint32_t waited_ms;
	size_t case_id_len;

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if (((argc != 3) && (argc != 4)) || (argv == NULL) ||
		(argv[1] == NULL) || (argv[2] == NULL) ||
		((argc == 4) && (argv[3] == NULL))) {
		app_dump_print_reject("-", "race_invalid_args");
		return;
	}
	if (!app_dump_get_case_id_len(argv[1], &case_id_len)) {
		app_dump_print_reject("-", "invalid_case_id");
		return;
	}
	mode = app_dump_parse_mode(argv[2]);
	if ((mode == NULL) || (mode->context != APP_DUMP_CONTEXT_TASK)) {
		app_dump_print_reject(argv[1], "race_requires_task_mode");
		return;
	}

	flags = rtos_enter_critical();
	if (s_orch_busy) {
		rtos_exit_critical(flags);
		app_dump_print_reject(argv[1], "orchestration_busy");
		return;
	}
	s_orch_busy = true;
	os_memcpy(s_race_request.case_id, argv[1], case_id_len + 1U);
	s_race_request.mode = mode;
	s_race_request.preferred_core = UINT8_MAX;
	s_race_request.scenario =
		BK_SYS_SW_REGS_DUMP_TEST_AP_FIRST_OWNER_RACE;
	if (argc == 4) {
		if (os_strcmp(argv[3], "0") == 0) {
			s_race_request.preferred_core = 0U;
			s_race_request.scenario =
				BK_SYS_SW_REGS_DUMP_TEST_AP0_OWNER_AP1_FOLLOWER;
		} else if (os_strcmp(argv[3], "1") == 0) {
			s_race_request.preferred_core = 1U;
			s_race_request.scenario =
				BK_SYS_SW_REGS_DUMP_TEST_AP1_OWNER_AP0_FOLLOWER;
		} else {
			s_orch_busy = false;
			rtos_exit_critical(flags);
			app_dump_print_reject(argv[1], "invalid_preferred_core");
			return;
		}
	}
	s_race_ready = 0U;
	s_race_start = false;
	s_race_abort = false;
	rtos_exit_critical(flags);

	scenario = s_race_request.scenario;
	follower = (s_race_request.preferred_core == 0U) ?
		BK_SYS_SW_REGS_DUMP_TEST_CORE_AP1 :
		((s_race_request.preferred_core == 1U) ?
			BK_SYS_SW_REGS_DUMP_TEST_CORE_AP0 :
			BK_SYS_SW_REGS_DUMP_TEST_CORE_NONE);
	if (bk_sys_sw_regs_dump_test_arm(
		scenario, follower) == 0U) {
		app_dump_print_reject(argv[1], "state_arm_failed");
		app_dump_orch_release();
		return;
	}
	ret0 = rtos_core0_create_thread(NULL, APP_DUMP_FOLLOW_PRIORITY,
		"dump_race0", app_dump_race_task, APP_DUMP_TASK_STACK_SIZE,
		&s_race_request);
	ret1 = rtos_core1_create_thread(NULL, APP_DUMP_FOLLOW_PRIORITY,
		"dump_race1", app_dump_race_task, APP_DUMP_TASK_STACK_SIZE,
		&s_race_request);
	if ((ret0 != BK_OK) || (ret1 != BK_OK)) {
		__atomic_store_n(&s_race_abort, true, __ATOMIC_RELEASE);
		app_dump_print_reject(argv[1], "race_create_failed");
		bk_sys_sw_regs_dump_test_reset();
		app_dump_orch_release();
		return;
	}

	for (waited_ms = 0U; waited_ms < APP_DUMP_RACE_READY_MS; waited_ms++) {
		if (__atomic_load_n(&s_race_ready, __ATOMIC_ACQUIRE) == 2U) {
			break;
		}
		rtos_delay_milliseconds(1);
	}
	if (__atomic_load_n(&s_race_ready, __ATOMIC_ACQUIRE) != 2U) {
		__atomic_store_n(&s_race_abort, true, __ATOMIC_RELEASE);
		app_dump_print_reject(argv[1], "race_ready_timeout");
		bk_sys_sw_regs_dump_test_reset();
		return;
	}

	os_printf("DUMP_ORCH_ARMED case_id=%s scenario=%u preferred=AP%u mode=%s\r\n",
		argv[1], (unsigned)scenario,
		(unsigned)s_race_request.preferred_core, mode->name);
	__atomic_store_n(&s_race_start, true, __ATOMIC_RELEASE);
}

static void app_dump_orch_status_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	bk_sys_sw_regs_dump_test_state_t state = {0};

	(void)pc_write_buffer;
	(void)write_buffer_len;
	(void)argv;
	if (argc != 1) {
		app_dump_print_reject("-", "status_invalid_args");
		return;
	}
	(void)bk_sys_sw_regs_get_dump_test_state(&state);
	os_printf("DUMP_ORCH_STATUS raw=0x%08x valid=%u scenario=%u phase=%u winner=%u follower=%u\r\n",
		(unsigned)state.raw, (unsigned)state.valid,
		(unsigned)state.scenario, (unsigned)state.phase,
		(unsigned)state.winner, (unsigned)state.follower);
}

static void app_dump_orch_reset_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	(void)pc_write_buffer;
	(void)write_buffer_len;
	(void)argv;
	if (argc != 1) {
		app_dump_print_reject("-", "reset_invalid_args");
		return;
	}
	bk_sys_sw_regs_dump_test_reset();
	os_printf("DUMP_ORCH_RESET raw=0x00000000\r\n");
}

static void app_dump_flash_release(void)
{
	uint32_t flags = rtos_enter_critical();
	s_flash_busy = false;
	rtos_exit_critical(flags);
}

static void app_dump_flash_task(beken_thread_arg_t arg)
{
	const app_dump_flash_request_t *request =
		(const app_dump_flash_request_t *)arg;
	bk_err_t ret = BK_OK;
	uint32_t cycle;
	uint32_t offset;
	uint32_t index;

	os_printf("DUMP_LOAD_READY case_id=%s type=flash address=0x%08x length=0x%x\r\n",
		request->case_id, (unsigned)request->address,
		(unsigned)request->length);

	for (cycle = 0; cycle < APP_DUMP_FLASH_MAX_CYCLES; cycle++) {
		ret = bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		if (ret != BK_OK) {
			break;
		}

		for (offset = 0; offset < request->length;
			offset += APP_DUMP_FLASH_SECTOR_SIZE) {
			ret = bk_flash_erase_sector(request->address + offset);
			if (ret != BK_OK) {
				break;
			}
		}

		for (offset = 0; (ret == BK_OK) && (offset < request->length);
			offset += APP_DUMP_FLASH_PAGE_SIZE) {
			for (index = 0; index < APP_DUMP_FLASH_PAGE_SIZE; index++) {
				s_flash_write_buffer[index] =
					(uint8_t)(cycle + offset + index);
			}
			ret = bk_flash_write_bytes(request->address + offset,
				s_flash_write_buffer, APP_DUMP_FLASH_PAGE_SIZE);
			if (ret == BK_OK) {
				ret = bk_flash_read_bytes(request->address + offset,
					s_flash_read_buffer, APP_DUMP_FLASH_PAGE_SIZE);
			}
			if ((ret == BK_OK) &&
				(os_memcmp(s_flash_write_buffer, s_flash_read_buffer,
				APP_DUMP_FLASH_PAGE_SIZE) != 0)) {
				ret = BK_FAIL;
			}
		}

		(void)bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		if (ret != BK_OK) {
			break;
		}
		rtos_delay_milliseconds(1);
	}

	if (ret == BK_OK) {
		os_printf("DUMP_LOAD_DONE case_id=%s type=flash cycles=%u\r\n",
			request->case_id, (unsigned)APP_DUMP_FLASH_MAX_CYCLES);
	} else {
		os_printf("DUMP_LOAD_ERROR case_id=%s type=flash ret=%d\r\n",
			request->case_id, (int)ret);
	}
	app_dump_flash_release();
	rtos_delete_thread(NULL);
}

static void app_dump_flash_load_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	bk_err_t ret;
	uint32_t flags;
	uint32_t address;
	uint32_t length;
	size_t case_id_len;

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if ((argc != 4) || (argv == NULL) || (argv[1] == NULL) ||
		(argv[2] == NULL) || (argv[3] == NULL)) {
		app_dump_print_reject("-", "flash_load_invalid_args");
		return;
	}
	if (!app_dump_get_case_id_len(argv[1], &case_id_len)) {
		app_dump_print_reject("-", "invalid_case_id");
		return;
	}

	address = os_strtoul(argv[2], NULL, 16);
	length = os_strtoul(argv[3], NULL, 16);
	if ((address == 0U) || ((address & (APP_DUMP_FLASH_SECTOR_SIZE - 1U)) != 0U) ||
		(length == 0U) || (length > APP_DUMP_FLASH_MAX_LEN) ||
		((length & (APP_DUMP_FLASH_SECTOR_SIZE - 1U)) != 0U) ||
		((address + length) < address)) {
		app_dump_print_reject(argv[1], "unsafe_flash_range");
		return;
	}

	flags = rtos_enter_critical();
	if (s_flash_busy) {
		rtos_exit_critical(flags);
		app_dump_print_reject(argv[1], "flash_load_busy");
		return;
	}
	s_flash_busy = true;
	os_memcpy(s_flash_request.case_id, argv[1], case_id_len + 1U);
	s_flash_request.address = address;
	s_flash_request.length = length;
	rtos_exit_critical(flags);

	ret = rtos_core0_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY,
		"dump_flash", app_dump_flash_task, APP_DUMP_TASK_STACK_SIZE,
		&s_flash_request);
	if (ret != BK_OK) {
		app_dump_print_reject_ret(s_flash_request.case_id,
			"flash_load_create_failed", ret);
		app_dump_flash_release();
	}
}

static void app_dump_test_command(char *pc_write_buffer, int write_buffer_len,
	int argc, char **argv)
{
	const app_dump_mode_t *mode;
	bk_err_t ret;
	uint32_t flags;
	size_t case_id_len;
	uint8_t core;
	beken_thread_function_t task_function;

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if ((argc != 4) || (argv == NULL)) {
		app_dump_print_reject("-", "invalid_argc");
		return;
	}
	if ((argv[1] == NULL) || (argv[2] == NULL) || (argv[3] == NULL)) {
		app_dump_print_reject("-", "null_argument");
		return;
	}

	if (!app_dump_get_case_id_len(argv[1], &case_id_len)) {
		app_dump_print_reject("-", "invalid_case_id");
		return;
	}
	mode = app_dump_parse_mode(argv[2]);
	if (mode == NULL) {
		app_dump_print_reject(argv[1], "invalid_mode");
		return;
	}
	if (os_strcmp(argv[3], "0") == 0) {
		core = 0U;
	} else if (os_strcmp(argv[3], "1") == 0) {
		core = 1U;
	} else {
		app_dump_print_reject(argv[1], "invalid_core");
		return;
	}

	flags = rtos_enter_critical();
	if (s_busy) {
		rtos_exit_critical(flags);
		app_dump_print_reject(argv[1], "busy");
		return;
	}
	s_busy = true;
	os_memcpy(s_request.case_id, argv[1], case_id_len + 1U);
	s_request.mode = mode;
	s_request.core = core;
	rtos_exit_critical(flags);

	task_function = app_dump_task;
	if ((mode->context == APP_DUMP_CONTEXT_ISR) && (core == 1U)) {
		task_function = app_dump_ipi_task;
		ret = rtos_core0_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
			"dump_ipi", task_function, APP_DUMP_TASK_STACK_SIZE, &s_request);
	} else if (core == 0U) {
		ret = rtos_core0_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
			"dump_ap0", task_function, APP_DUMP_TASK_STACK_SIZE, &s_request);
	} else {
		ret = rtos_core1_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
			"dump_ap1", task_function, APP_DUMP_TASK_STACK_SIZE, &s_request);
	}

	if (ret != BK_OK) {
		app_dump_print_reject_ret(s_request.case_id,
			"create_thread_failed", ret);
		app_dump_release_request();
	}
}

COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_app_dump_test_commands[] = {
	{"ap_dump_test", "ap_dump_test <case_id> <mode> <core>",
		app_dump_test_command},
	{"ap_dump_flash_load",
		"ap_dump_flash_load <case_id> <safe_addr> <length>",
		app_dump_flash_load_command},
	{"ap_dump_follow",
		"ap_dump_follow <case_id> <cp_ap|ap0_ap1|ap1_ap0> <task_mode> <core>",
		app_dump_follow_command},
	{"ap_dump_race", "ap_dump_race <case_id> <task_mode> [preferred_core]",
		app_dump_race_command},
	{"ap_dump_orch_status", "ap_dump_orch_status",
		app_dump_orch_status_command},
	{"ap_dump_orch_reset", "ap_dump_orch_reset",
		app_dump_orch_reset_command},
};
