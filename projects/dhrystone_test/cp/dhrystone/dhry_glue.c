/*
 * Dhrystone 2.1 (DMIPS) porting layer - BK7259 CP core
 *
 * The official dhry.h / dhry_1.c / dhry_2.c are kept as separate translation units
 * without modification; only dhry_1.c carries one DHRY_PORT conditional block that
 * hides the "extern int times();" prototype conflicting with newlib's sys/times.h.
 * All other adaptation is done by this file plus the per-file macros from CMake:
 *   dhry_1.c: main=dhrystone_main / printf=dhry_printf / scanf=dhry_scanf /
 *             DHRY_PORT=1 / HZ=1000000
 *   1. Enable the official TIMES timing branch (HZ = 1e6, times() returns us);
 *      times() itself is implemented here;
 *   2. Redirect printf to dhry_printf(), which logs through the SDK and captures
 *      the Dhrystones/s figure on the fly;
 *   3. Redirect scanf to dhry_scanf(), replacing the interactive run count with a
 *      fixed value (configurable with `dhrystone [runs]`);
 *   4. Rename the official main() to dhrystone_main(), called by the dhrystone CLI.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/times.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <os/os.h>

#if !CONFIG_ARCH_RISCV
#include <soc/soc.h>
#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
#endif

#include "dhry_port.h"

#define TAG "dhrystone"

extern int dhrystone_main(void);

/* Same time base as core_mark: the 26 MHz free-running counter */
static uint32_t dhry_timer_raw(void)
{
#if CONFIG_ARCH_RISCV
	extern uint64_t riscv_get_mtimer(void);
	return (uint32_t)riscv_get_mtimer();
#else
	TIMER0_REG_SET(8, 2, 3, 0);
	TIMER0_REG_SET(8, 0, 0, 1);
	while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0));

	return REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));
#endif
}

/* Extend the 32-bit counter into a 64-bit us clock, so that the 26 MHz wrap
 * (about 165 s) cannot corrupt the delta shown below */
static uint64_t s_us_total;
static uint32_t s_last_tick;

static double s_dhrystones_per_sec;
static int s_dhrystones_valid;
static int s_expect_score;

/* The official code asks for the run count through scanf, use a fixed value here */
static int s_runs = DHRY_DEFAULT_RUNS;

/* Same stack size as the coremark thread */
#define DHRY_THREAD_STACK_SIZE 4096

static beken_thread_t s_dhry_thread = NULL;
static beken_semaphore_t s_dhry_done = NULL;
static int s_dhry_argc;
static char **s_dhry_argv;

/* Check whether the format string really contains a float conversion, skipping
 * plain 'f' / 'e' letters such as the ones in "per" */
static int dhry_fmt_has_float(const char *fmt)
{
	while (*fmt) {
		if (*fmt++ != '%')
			continue;
		if (*fmt == '%') {
			fmt++;
			continue;
		}
		while (*fmt && !strchr("diouxXeEfgGaAcspn%", *fmt))
			fmt++;
		if (*fmt && strchr("eEfgG", *fmt))
			return 1;
		if (*fmt)
			fmt++;
	}

	return 0;
}

clock_t times(struct tms *buf)
{
	uint32_t raw = dhry_timer_raw();

	s_us_total += ((uint64_t)(uint32_t)(raw - s_last_tick) * 1000000ULL) / DHRY_TIMER_HZ;
	s_last_tick = raw;

	if (buf) {
		buf->tms_utime = (clock_t)s_us_total;
		buf->tms_stime = 0;
		buf->tms_cutime = 0;
		buf->tms_cstime = 0;
	}

	return (clock_t)s_us_total;
}

/* printf in dhry_1.c is redirected here (-Dprintf=dhry_printf at compile time).
 * The output goes through the SDK log, the same way core_mark's ee_printf does
 * (BK_LOGD). vprintf() is not a memory-only newlib call, it would pull in the
 * stdio stream layer and its unimplemented _fstat()/_isatty() stubs. */
int dhry_printf(const char *fmt, ...)
{
	char buf[256];
	va_list ap, ap_fwd;
	int ret;

	va_start(ap, fmt);
	va_copy(ap_fwd, ap);

	/* The official code prints the title and the value in two separate printf
	 * calls, so remember the title line and take the next floating-point argument */
	if (strstr(fmt, "Dhrystones per Second")) {
		if (dhry_fmt_has_float(fmt)) {
			s_dhrystones_per_sec = va_arg(ap, double);
			s_dhrystones_valid = 1;
		} else {
			s_expect_score = 1;
		}
	} else if (s_expect_score) {
		s_expect_score = 0;
		if (dhry_fmt_has_float(fmt)) {
			s_dhrystones_per_sec = va_arg(ap, double);
			s_dhrystones_valid = 1;
		}
	}

	ret = vsnprintf(buf, sizeof(buf), fmt, ap_fwd);

	va_end(ap_fwd);
	va_end(ap);

	BK_LOGD(NULL, "%s", buf);

	return ret;
}

/* scanf in dhry_1.c is redirected here (-Dscanf=dhry_scanf at compile time) */
int dhry_scanf(const char *fmt, ...)
{
	va_list ap;
	int *dst;

	(void)fmt;
	va_start(ap, fmt);
	dst = va_arg(ap, int *);
	va_end(ap);

	if (dst)
		*dst = s_runs;

	return 1;
}

static int dhry_atoi(const char *s)
{
	int v = 0;

	if (!s)
		return 0;

	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');

	return v;
}

static void dhrystone_body(int argc, char *argv[])
{
	int runs;

	if (argc > 1) {
		runs = dhry_atoi(argv[1]);
		if (runs > 0)
			s_runs = runs;
	}

	s_us_total = 0;
	s_last_tick = dhry_timer_raw();
	s_dhrystones_per_sec = 0.0;
	s_dhrystones_valid = 0;
	s_expect_score = 0;

	BK_LOGI(TAG, "Dhrystone 2.1, runs %d\r\n", s_runs);

	dhrystone_main();

	if (s_dhrystones_valid && s_dhrystones_per_sec > 0.0) {
		BK_LOGI(TAG, "DMIPS: %.2f  (Dhrystones per Second %.1f / %.0f)\r\n",
		        s_dhrystones_per_sec / DHRY_DMIPS_BASE,
		        s_dhrystones_per_sec, DHRY_DMIPS_BASE);
	} else {
		BK_LOGW(TAG, "no valid Dhrystones score captured\r\n");
	}
}

static void dhrystone_task(beken_thread_arg_t arg)
{
	(void)arg;

	dhrystone_body(s_dhry_argc, s_dhry_argv);

	rtos_set_semaphore(&s_dhry_done);
	rtos_delete_thread(NULL);
}

/* Run the benchmark in a dedicated thread with the same priority and stack size the
 * coremark thread uses, instead of inside the shell task and inside a critical
 * section: with interrupts masked the 1 s AON watchdog feed (TIMER_ID2 ->
 * bk_wdt_feed_handle) can never run, and once the run lasts longer than the
 * remaining watchdog window the chip is reset in the middle of the loop.
 * Dhrystone 2.1 keeps its data in globals and is single-core by construction. */
void dhrystone_run(int argc, char *argv[])
{
	bk_err_t ret;

	s_dhry_argc = argc;
	s_dhry_argv = argv;

	ret = rtos_init_semaphore_ex(&s_dhry_done, 1, 0);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "init semaphore failed: %d\r\n", ret);
		return;
	}

	ret = rtos_create_thread(&s_dhry_thread, BEKEN_APPLICATION_PRIORITY,
	                         "dhrystone", dhrystone_task,
	                         DHRY_THREAD_STACK_SIZE, NULL);
	if (ret != kNoErr) {
		BK_LOGE(TAG, "create dhrystone thread failed: %d\r\n", ret);
		rtos_deinit_semaphore(&s_dhry_done);
		return;
	}

	rtos_get_semaphore(&s_dhry_done, BEKEN_WAIT_FOREVER);
	rtos_deinit_semaphore(&s_dhry_done);
}
