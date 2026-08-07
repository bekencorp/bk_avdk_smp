#include "cli.h"
#include "cli_section.h"
#include <common/bk_assert.h>
#include <driver/timer.h>
#include <os/os.h>
#include <os/str.h>
#include <stdint.h>
#include "sys_sw_regs.h"

#define APP_DUMP_CASE_ID_MAX_LEN 31U
#define APP_DUMP_TASK_STACK_SIZE 2048U
#define APP_DUMP_TIMER_DELAY_MS  10U
#define APP_DUMP_FOLLOW_WAIT_MS  30000U
/* TIMER_ID0 is permanently reserved when CONFIG_TIMER_US is enabled. */
#define APP_DUMP_TIMER_ID        TIMER_ID11

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
} app_dump_request_t;

typedef struct {
	char case_id[APP_DUMP_CASE_ID_MAX_LEN + 1U];
	const app_dump_mode_t *mode;
} app_dump_follow_request_t;

static app_dump_request_t s_request;
static volatile bool s_busy;
static volatile bool s_timer_fired;
static app_dump_follow_request_t s_follow_request;
static volatile bool s_follow_busy;

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

static void app_dump_print_begin(const app_dump_request_t *request)
{
	os_printf("DUMP_TEST_BEGIN case_id=%s target=CP core=0 mode=%s\r\n",
		request->case_id, request->mode->name);
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
	if ((timer_id != APP_DUMP_TIMER_ID) ||
		((rtos_get_core_id() & 1U) != 0U)) {
		os_printf("DUMP_TEST_REJECT case_id=%s reason=wrong_timer_isr_core\r\n",
			s_request.case_id);
		s_timer_fired = true;
		return;
	}

	app_dump_trigger(s_request.mode->fault);
	s_timer_fired = true;
}

static void app_dump_task(beken_thread_arg_t arg)
{
	const app_dump_request_t *request = (const app_dump_request_t *)arg;
	bk_err_t ret;
	uint32_t critical_flags;

	if ((rtos_get_core_id() & 1U) != 0U) {
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
		critical_flags = rtos_enter_critical();
		app_dump_trigger(request->mode->fault);
		rtos_exit_critical(critical_flags);
		break;

	case APP_DUMP_CONTEXT_ISR:
		ret = bk_timer_driver_init();
		if (ret != BK_OK) {
			app_dump_print_reject_ret(request->case_id,
				"timer_init_failed", ret);
			break;
		}
		if ((bk_timer_get_enable_status() &
			(1U << (uint32_t)APP_DUMP_TIMER_ID)) != 0U) {
			app_dump_print_reject(request->case_id, "timer_busy");
			break;
		}

		s_timer_fired = false;
		ret = bk_timer_start(APP_DUMP_TIMER_ID, APP_DUMP_TIMER_DELAY_MS,
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

static void app_dump_follow_release(void)
{
	uint32_t flags = rtos_enter_critical();

	s_follow_busy = false;
	rtos_exit_critical(flags);
}

static void app_dump_follow_task(beken_thread_arg_t arg)
{
	const app_dump_follow_request_t *request =
		(const app_dump_follow_request_t *)arg;
	bk_sys_sw_regs_dump_test_state_t state;
	uint32_t waited_ms;

	if ((rtos_get_core_id() & 1U) != 0U) {
		app_dump_print_reject(request->case_id, "wrong_follower_core");
		goto exit_task;
	}
	for (waited_ms = 0U; waited_ms < APP_DUMP_FOLLOW_WAIT_MS; waited_ms++) {
		if (bk_sys_sw_regs_get_dump_test_state(&state) &&
			(state.scenario ==
			 BK_SYS_SW_REGS_DUMP_TEST_AP_OWNER_CP_FOLLOWER) &&
			(state.phase == BK_SYS_SW_REGS_DUMP_TEST_PHASE_OWNER) &&
			(state.follower == BK_SYS_SW_REGS_DUMP_TEST_CORE_CP0)) {
			app_dump_trigger(request->mode->fault);
			goto exit_task;
		}
		rtos_delay_milliseconds(1);
	}

	app_dump_print_reject(request->case_id, "follower_owner_timeout");
	bk_sys_sw_regs_dump_test_reset();

exit_task:
	app_dump_follow_release();
	rtos_delete_thread(NULL);
}

static void app_dump_follow_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	const app_dump_mode_t *mode;
	bk_err_t ret;
	uint32_t flags;
	size_t case_id_len;

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if ((argc != 4) || (argv == NULL) || (argv[1] == NULL) ||
		(argv[2] == NULL) || (argv[3] == NULL)) {
		app_dump_print_reject("-", "follow_invalid_args");
		return;
	}
	if (!app_dump_get_case_id_len(argv[1], &case_id_len)) {
		app_dump_print_reject("-", "invalid_case_id");
		return;
	}
	if (os_strcmp(argv[2], "ap_cp") != 0) {
		app_dump_print_reject(argv[1], "invalid_follow_scenario");
		return;
	}
	mode = app_dump_parse_mode(argv[3]);
	if ((mode == NULL) || (mode->context != APP_DUMP_CONTEXT_TASK)) {
		app_dump_print_reject(argv[1], "follower_requires_task_mode");
		return;
	}

	flags = rtos_enter_critical();
	if (s_follow_busy) {
		rtos_exit_critical(flags);
		app_dump_print_reject(argv[1], "orchestration_busy");
		return;
	}
	s_follow_busy = true;
	os_memcpy(s_follow_request.case_id, argv[1], case_id_len + 1U);
	s_follow_request.mode = mode;
	rtos_exit_critical(flags);

	if (bk_sys_sw_regs_dump_test_arm(
		BK_SYS_SW_REGS_DUMP_TEST_AP_OWNER_CP_FOLLOWER,
		BK_SYS_SW_REGS_DUMP_TEST_CORE_CP0) == 0U) {
		app_dump_print_reject(argv[1], "state_arm_failed");
		app_dump_follow_release();
		return;
	}
	ret = rtos_core0_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
		"dump_follow", app_dump_follow_task, APP_DUMP_TASK_STACK_SIZE,
		&s_follow_request);
	if (ret != BK_OK) {
		app_dump_print_reject_ret(argv[1], "follow_create_failed", ret);
		bk_sys_sw_regs_dump_test_reset();
		app_dump_follow_release();
		return;
	}
	os_printf("DUMP_ORCH_ARMED case_id=%s scenario=ap_cp follower=CP0 mode=%s\r\n",
		argv[1], mode->name);
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

static void app_dump_test_command(char *pc_write_buffer, int write_buffer_len,
	int argc, char **argv)
{
	const app_dump_mode_t *mode;
	bk_err_t ret;
	uint32_t flags;
	size_t case_id_len;

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if ((argc != 3) || (argv == NULL)) {
		app_dump_print_reject("-", "invalid_argc");
		return;
	}
	if ((argv[1] == NULL) || (argv[2] == NULL)) {
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

	flags = rtos_enter_critical();
	if (s_busy) {
		rtos_exit_critical(flags);
		app_dump_print_reject(argv[1], "busy");
		return;
	}
	s_busy = true;
	os_memcpy(s_request.case_id, argv[1], case_id_len + 1U);
	s_request.mode = mode;
	rtos_exit_critical(flags);

	ret = rtos_core0_create_thread(NULL, BEKEN_APPLICATION_PRIORITY,
		"dump_cp", app_dump_task, APP_DUMP_TASK_STACK_SIZE, &s_request);
	if (ret != BK_OK) {
		app_dump_print_reject_ret(s_request.case_id,
			"create_thread_failed", ret);
		app_dump_release_request();
	}
}

#if CONFIG_CP_HANG_DUMP_BY_AP
static void cp_hang_hb_test_command(char *pc_write_buffer,
	int write_buffer_len, int argc, char **argv)
{
	extern void bk_cp_hang_debug_heartbeat_pause(uint32_t pause);

	(void)pc_write_buffer;
	(void)write_buffer_len;

	if ((argc != 2) || (argv == NULL) || (argv[1] == NULL)) {
		os_printf("usage: cp_hang_hb_test <stop|start>\r\n");
		return;
	}
	if (os_strcmp(argv[1], "stop") == 0) {
		bk_cp_hang_debug_heartbeat_pause(1U);
		os_printf("DUMP_HB_TEST direction=CP_TO_AP state=stopped\r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		bk_cp_hang_debug_heartbeat_pause(0U);
		os_printf("DUMP_HB_TEST direction=CP_TO_AP state=started\r\n");
	} else {
		os_printf("DUMP_TEST_REJECT case_id=- reason=invalid_hb_action\r\n");
	}
}
#endif

COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_app_dump_test_commands[] = {
	{"cp_dump_test", "cp_dump_test <case_id> <mode>",
		app_dump_test_command},
	{"cp_dump_follow", "cp_dump_follow <case_id> ap_cp <task_mode>",
		app_dump_follow_command},
	{"cp_dump_orch_status", "cp_dump_orch_status",
		app_dump_orch_status_command},
	{"cp_dump_orch_reset", "cp_dump_orch_reset",
		app_dump_orch_reset_command},
#if CONFIG_CP_HANG_DUMP_BY_AP
	{"cp_hang_hb_test", "cp_hang_hb_test <stop|start>",
		cp_hang_hb_test_command},
#endif
};
