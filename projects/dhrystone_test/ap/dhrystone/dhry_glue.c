/*
 * Dhrystone 2.1 (DMIPS) porting layer - BK7259 AP core
 *
 * The official dhry.h / dhry_1.c / dhry_2.c are kept as separate translation units
 * without modification; only dhry_1.c carries one DHRY_PORT conditional block that
 * hides the "extern int times();" prototype conflicting with newlib's sys/times.h.
 * All other adaptation is done by this file plus the per-file macros from CMake:
 *   dhry_1.c: main=dhrystone_main / printf=dhry_printf / scanf=dhry_scanf /
 *             DHRY_PORT=1 / HZ=1000000
 *   1. Enable the official TIMES timing branch (HZ = 1e6, times() returns us);
 *      times() itself is implemented here;
 *   2. Redirect printf to dhry_printf(), which logs through the SDK;
 *   3. Redirect scanf to dhry_scanf(), replacing the interactive run count with a
 *      fixed value (configurable with `dhrystone [runs]`);
 *   4. Rename the official main() to dhrystone_main(), called by the dhrystone CLI.
 *
 * SMP dual-core mode (DHRYSTONE_CONTEXTS=2, the default, same knob style as
 * core_mark's COREMARK_CONTEXTS): the official benchmark keeps every mutable
 * variable in file scope, so a single instance cannot be shared by two cores.
 * dhry_1_ctx1.c / dhry_2_ctx1.c include the official sources a second time and
 * CMake renames all of their globals and Proc_/Func_ symbols with a "_1" suffix,
 * which gives the second core a fully independent copy (its own .bss, its own
 * records, its own entry point). Both instances are then started simultaneously
 * from a barrier on core 0 and core 1.
 *
 * Reporting follows core_mark's whole-chip convention: one parallel wall-clock
 * window is taken around the two contexts (start before they are created, stop
 * after the last one finishes) and the score is
 *     Dhrystones/s = (contexts * runs) / wall_seconds
 * i.e. total work divided by the parallel elapsed time, exactly like
 * core_main.c's "Iterations/Sec = default_num_contexts * iterations / total_time".
 * The per-instance score lines of the official program are suppressed in SMP mode
 * so the log carries a single whole-chip result.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/times.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <os/os.h>
#include <os/mem.h>
#include "l2_cache.h"

#if !CONFIG_ARCH_RISCV
#include <soc/soc.h>
#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
#endif

#include "dhry_port.h"

#define TAG "dhrystone"

#ifndef DHRY_CONTEXTS
#define DHRY_CONTEXTS 1
#endif

/* Two AP cores at most; without SMP there is only one instance. This must stay in
 * sync with the source list CMake builds (dhry_1_ctx1.c / dhry_2_ctx1.c). */
#if (DHRY_CONTEXTS > 1) && CONFIG_SOC_SMP
#define DHRY_CTX_COUNT 2
#else
#define DHRY_CTX_COUNT 1
#endif

extern int dhrystone_main(void);
extern float Dhrystones_Per_Second;
extern long User_Time;

/* Final values the official program checks by hand; validating them in code gives
 * the same "Correct operation validated" proof core_mark prints. */
extern int Int_Glob;
extern int Bool_Glob;
extern char Ch_1_Glob, Ch_2_Glob;
extern int Arr_1_Glob[50];
extern int Arr_2_Glob[50][50];

#if DHRY_CTX_COUNT > 1
/* Second instance, see dhry_1_ctx1.c / dhry_2_ctx1.c */
extern int dhrystone_main1(void);
extern void *dhry_malloc1(unsigned int size);
extern clock_t dhry_times1(struct tms *buf);
extern float Dhrystones_Per_Second_1;
extern long User_Time_1;
extern int Int_Glob_1;
extern int Bool_Glob_1;
extern char Ch_1_Glob_1, Ch_2_Glob_1;
extern int Arr_1_Glob_1[50];
extern int Arr_2_Glob_1[50][50];
#endif

/* The official code asks for the run count through scanf, use a fixed value here */
static int s_runs = DHRY_DEFAULT_RUNS;

/* Address of each context's benchmark records. Printed as placement evidence, the
 * same way core_mark prints "CoreMark data addr[n]", so a SRAM3 / SRAM4 / DTCM run
 * can be told apart from a plain heap run. */
static void *s_data_addr[DHRY_CTX_COUNT];

/* Returns 1 when every documented final value matches. */
static int dhry_check_values(int ctx)
{
#if DHRY_CTX_COUNT > 1
	if (ctx == 1)
		return Int_Glob_1 == 5 && Bool_Glob_1 == 1 && Ch_1_Glob_1 == 'A' &&
		       Ch_2_Glob_1 == 'B' && Arr_1_Glob_1[8] == 7 &&
		       Arr_2_Glob_1[8][7] == s_runs + 10;
#endif
	(void)ctx;

	return Int_Glob == 5 && Bool_Glob == 1 && Ch_1_Glob == 'A' &&
	       Ch_2_Glob == 'B' && Arr_1_Glob[8] == 7 &&
	       Arr_2_Glob[8][7] == s_runs + 10;
}

/* Serialises the Timer0 capture sequence, see dhry_timer_raw() */
static volatile int s_timer_lock;

/* Same time base as core_mark: the 26 MHz free-running counter */
static uint32_t dhry_timer_raw(void)
{
#if CONFIG_ARCH_RISCV
	extern uint64_t riscv_get_mtimer(void);
	return (uint32_t)riscv_get_mtimer();
#else
	uint32_t raw;

	/* Timer0 is a shared peripheral: when both contexts take their Begin_Time /
	 * End_Time at (nearly) the same moment, the capture trigger and the capture
	 * register read must not interleave, so serialise the short sequence. */
	while (__atomic_test_and_set(&s_timer_lock, __ATOMIC_ACQUIRE))
		;

	TIMER0_REG_SET(8, 2, 3, 0);
	TIMER0_REG_SET(8, 0, 0, 1);
	while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0))
		;

	raw = REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));

	__atomic_clear(&s_timer_lock, __ATOMIC_RELEASE);

	return raw;
#endif
}

/* Per-context 64-bit us clock: the 26 MHz counter wraps every ~165 s, so the
 * 32-bit delta is accumulated into a 64-bit total */
typedef struct {
	uint64_t us_total;
	uint32_t last_tick;
} dhry_clock_t;

static dhry_clock_t s_clk[DHRY_CTX_COUNT];

#if DHRY_CTX_COUNT > 1
/* Whole-chip parallel window, the counterpart of core_mark's start_time() /
 * stop_time(): started before the contexts are created and stopped after the last
 * one has finished, so it covers exactly the same span as core_mark's total_time. */
static dhry_clock_t s_wall_clk;
static uint64_t s_wall_us;

static uint64_t dhry_wall_now(void)
{
	uint32_t raw = dhry_timer_raw();

	s_wall_clk.us_total += ((uint64_t)(uint32_t)(raw - s_wall_clk.last_tick) * 1000000ULL) / DHRY_TIMER_HZ;
	s_wall_clk.last_tick = raw;

	return s_wall_clk.us_total;
}

/* The official program prints Microseconds/run and Dhrystones/s for its own
 * instance. In SMP mode only the whole-chip figure is reported, so both the title
 * line and the value line that follows it are dropped (the two printf calls are
 * separate, hence the per-context "skip next" flag). Final values are kept: they
 * are the correctness proof. */
static int s_skip_score_value[DHRY_CTX_COUNT];

static int dhry_is_per_ctx_score(int ctx, const char *buf)
{
	static const char *const titles[] = {
		"Microseconds for one run through Dhrystone:",
		"Dhrystones per Second:",
	};
	unsigned int i;

	if (s_skip_score_value[ctx]) {
		s_skip_score_value[ctx] = 0;
		return 1;
	}

	for (i = 0; i < sizeof(titles) / sizeof(titles[0]); i++) {
		if (strncmp(buf, titles[i], strlen(titles[i])) == 0) {
			s_skip_score_value[ctx] = 1;
			return 1;
		}
	}

	return 0;
}
#endif

/* The official code asks for the run count through scanf, use a fixed value here */

/* Same stack size as the coremark0 / coremark1 threads */
#define DHRY_THREAD_STACK_SIZE 4096

typedef struct {
	beken_thread_t thread;
	beken_semaphore_t done;
} dhry_ctx_t;

static dhry_ctx_t s_ctx[DHRY_CTX_COUNT];

/* Start barrier, so both contexts enter the timed loop at the same time */
static volatile uint32_t s_ready_count;
static volatile uint32_t s_ctx_total;

static clock_t dhry_times_common(int ctx, struct tms *buf)
{
	dhry_clock_t *clk = &s_clk[ctx];
	uint32_t raw = dhry_timer_raw();

	clk->us_total += ((uint64_t)(uint32_t)(raw - clk->last_tick) * 1000000ULL) / DHRY_TIMER_HZ;
	clk->last_tick = raw;

	if (buf) {
		buf->tms_utime = (clock_t)clk->us_total;
		buf->tms_stime = 0;
		buf->tms_cutime = 0;
		buf->tms_cstime = 0;
	}

	return (clock_t)clk->us_total;
}

clock_t times(struct tms *buf)
{
	return dhry_times_common(0, buf);
}

#if DHRY_CTX_COUNT > 1
clock_t dhry_times1(struct tms *buf)
{
	return dhry_times_common(1, buf);
}
#endif

static void dhry_clock_reset(int ctx)
{
	s_clk[ctx].us_total = 0;
	s_clk[ctx].last_tick = dhry_timer_raw();
}

/* printf in dhry_1.c is redirected here (-Dprintf=dhry_printf at compile time).
 * The output goes through the SDK log, the same way core_mark's ee_printf does
 * (BK_LOGD). vprintf() itself is not in the SDK --wrap list, so it would fall
 * into newlib's global_stdio_init() and then hit the asserting _fstat()/_isatty()
 * stubs of platform_stub.c, which dumps the system.
 * In SMP mode every line is tagged with its context so the two interleaved
 * reports stay readable. */
static int dhry_printf_common(int ctx, const char *fmt, va_list ap)
{
	char buf[256];
	int ret;

	ret = vsnprintf(buf, sizeof(buf), fmt, ap);

#if DHRY_CTX_COUNT > 1
	/* Only the whole-chip score is reported in SMP mode */
	if (dhry_is_per_ctx_score(ctx, buf))
		return ret;

	if (buf[0] != '\n')
		BK_LOGD(NULL, "[c%d] %s", ctx, buf);
	else
		BK_LOGD(NULL, "%s", buf);
#else
	BK_LOGD(NULL, "%s", buf);
#endif

	return ret;
}

int dhry_printf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = dhry_printf_common(0, fmt, ap);
	va_end(ap);

	return ret;
}

#if DHRY_CTX_COUNT > 1
int dhry_printf1(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = dhry_printf_common(1, fmt, ap);
	va_end(ap);

	return ret;
}
#endif

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

#if DHRY_CTX_COUNT > 1
int dhry_scanf1(const char *fmt, ...)
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
#endif

/* Allocator used by the official malloc() calls (redirected in dhry_1.c with
 * `#define malloc(sz) dhry_malloc(sz)`, renamed to dhry_malloc1 in the second
 * instance). The two Rec_Type records are written on every iteration, so where
 * they live matters for the score:
 *   default        -> os_malloc(), the internal SRAM heap core_mark also uses
 *                     (malloc() itself would be wrapped to psram_malloc())
 *   DHRY_SRAM3_DATA -> fixed arena at 0x2c100000 (same as core_mark)
 *   DHRY_SRAM4_DATA -> fixed arena at 0x2c140000 (same as core_mark)
 *   DHRY_DTCM_DATA  -> .dtcm_noinit, placed in DTCM by the linker script
 * In SMP mode every context owns a separate arena region. */
#define DHRY_ARENA_SIZE 512

#if defined(DHRY_SRAM3_DATA)
#define DHRY_FIXED_ARENA ((unsigned char *)0x2c100000UL)
#elif defined(DHRY_SRAM4_DATA)
#define DHRY_FIXED_ARENA ((unsigned char *)0x2c140000UL)
#elif defined(DHRY_DTCM_DATA)
static unsigned char s_dhry_arena[DHRY_CTX_COUNT][DHRY_ARENA_SIZE] __attribute__((section(".dtcm_noinit"), aligned(32)));
#endif

#if defined(DHRY_SRAM3_DATA) || defined(DHRY_SRAM4_DATA) || defined(DHRY_DTCM_DATA)
static unsigned int s_dhry_arena_used[DHRY_CTX_COUNT];

/* The official code allocates its two records on every run and never frees them,
 * so the fixed arena has to be rewound before each run. */
static void dhry_arena_reset(int ctx)
{
	s_dhry_arena_used[ctx] = 0;
}

static void *dhry_malloc_common(int ctx, unsigned int size)
{
	unsigned char *base;
	unsigned int len = (size + 7u) & ~7u;

#if defined(DHRY_FIXED_ARENA)
	base = DHRY_FIXED_ARENA + (ctx * DHRY_ARENA_SIZE);
#else
	base = s_dhry_arena[ctx];
#endif

	if (s_dhry_arena_used[ctx] + len > DHRY_ARENA_SIZE)
		return NULL;

	base += s_dhry_arena_used[ctx];
	s_dhry_arena_used[ctx] += len;

	if (!s_data_addr[ctx])
		s_data_addr[ctx] = base;

	return base;
}
#else
static void dhry_arena_reset(int ctx)
{
	(void)ctx;
}

static void *dhry_malloc_common(int ctx, unsigned int size)
{
	void *p = os_malloc(size);

	if (p && !s_data_addr[ctx])
		s_data_addr[ctx] = p;

	return p;
}
#endif

void *dhry_malloc(unsigned int size)
{
	return dhry_malloc_common(0, size);
}

#if DHRY_CTX_COUNT > 1
void *dhry_malloc1(unsigned int size)
{
	return dhry_malloc_common(1, size);
}
#endif

static int dhry_atoi(const char *s)
{
	int v = 0;

	if (!s)
		return 0;

	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');

	return v;
}

/* Per-context score. The official code stores the result in a file-scope global
 * (Dhrystones_Per_Second), which is why printf interception is no longer needed
 * to learn the score: reading the global after main() returns is race free and
 * works for both instances. */
static float dhry_score_of(int ctx)
{
#if DHRY_CTX_COUNT > 1
	return (ctx == 1) ? Dhrystones_Per_Second_1 : Dhrystones_Per_Second;
#else
	(void)ctx;
	return Dhrystones_Per_Second;
#endif
}

static long dhry_wall_us_of(int ctx)
{
#if DHRY_CTX_COUNT > 1
	return (ctx == 1) ? User_Time_1 : User_Time;
#else
	(void)ctx;
	return User_Time;
#endif
}

/* Report one context: window, cycles/run and DMIPS. The single-context format is
 * kept identical to the previous release. */
static void dhry_report_ctx(int ctx)
{
	float score = dhry_score_of(ctx);
	long wall_us = dhry_wall_us_of(ctx);
#if DHRY_CTX_COUNT > 1
	char prefix[8];
#else
	const char *prefix = "";
#endif

	if (wall_us < (2 * (long)DHRY_HZ)) {
		BK_LOGW(TAG, "context %d: measured time too small (%ld us), increase the run count\r\n",
		        ctx, wall_us);
		return;
	}

#if DHRY_CTX_COUNT > 1
	snprintf(prefix, sizeof(prefix), "[c%d] ", ctx);
#endif

#if defined(DHRY_CPU_FREQ_MHZ)
	{
		double us_per_run = (double)wall_us / (double)s_runs;

		BK_LOGI(TAG, "%swindow %.3f s, %.2f us/run, %.0f cycles/run (@%uMHz)\r\n",
		        prefix, (double)wall_us / 1000000.0, us_per_run,
		        us_per_run * (double)DHRY_CPU_FREQ_MHZ,
		        (unsigned)DHRY_CPU_FREQ_MHZ);
	}
#else
	BK_LOGI(TAG, "%swindow %.3f s, %.2f us/run\r\n",
	        prefix, (double)wall_us / 1000000.0, (double)wall_us / (double)s_runs);
#endif

	BK_LOGI(TAG, "%sDMIPS: %.2f  (Dhrystones per Second %.1f / %.0f)\r\n",
	        prefix, (double)score / DHRY_DMIPS_BASE, (double)score, DHRY_DMIPS_BASE);
	BK_LOGI(TAG, "%sdata addr 0x%08lx, final values %s\r\n", prefix,
	        (unsigned long)(uintptr_t)s_data_addr[ctx],
	        dhry_check_values(ctx) ? "verified" : "MISMATCH");
}

static void dhry_report_all(void)
{
#if DHRY_CTX_COUNT > 1
	int ctx;
	double runs_total = 0.0;
	double cyc_sum = 0.0;
	double wall_s;
	double rate;

	for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++) {
		long wall_us = dhry_wall_us_of(ctx);

		if (wall_us < (2L * DHRY_HZ)) {
			BK_LOGW(TAG, "context %d: measured time too small (%ld us), increase the run count\r\n",
			        ctx, wall_us);
			return;
		}

		runs_total += (double)s_runs;
		cyc_sum += ((double)wall_us / (double)s_runs) * (double)DHRY_CPU_FREQ_MHZ;
	}

	if (s_wall_us == 0) {
		BK_LOGE(TAG, "parallel wall clock unavailable, falling back to per-context report\r\n");
		for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++)
			dhry_report_ctx(ctx);
		return;
	}

	/* Same formula as core_main.c: total work / parallel elapsed time */
	wall_s = (double)s_wall_us / 1000000.0;
	rate = runs_total / wall_s;

	BK_LOGI(TAG, "Parallel %d cores: %d x %d runs in %.3f s wall\r\n",
	        DHRY_CTX_COUNT, DHRY_CTX_COUNT, s_runs, wall_s);
	BK_LOGI(TAG, "Dhrystones per Second (%d cores): %.1f\r\n", DHRY_CTX_COUNT, rate);
	BK_LOGI(TAG, "DMIPS (%d cores): %.2f   DMIPS/MHz (%d cores): %.3f   avg cycles/run: %.0f\r\n",
	        DHRY_CTX_COUNT, rate / DHRY_DMIPS_BASE,
	        DHRY_CTX_COUNT, rate / DHRY_DMIPS_BASE / (double)DHRY_CPU_FREQ_MHZ,
	        cyc_sum / (double)DHRY_CTX_COUNT);

	/* Correctness proof and placement evidence, mirroring core_mark's
	 * "Correct operation validated" / "CoreMark data addr[n]" lines */
	{
		int verified = 1;

		for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++) {
			BK_LOGI(TAG, "Dhrystone data addr[%d]: 0x%08lx\r\n", ctx,
			        (unsigned long)(uintptr_t)s_data_addr[ctx]);
			if (!dhry_check_values(ctx))
				verified = 0;
		}

		BK_LOGI(TAG, "Final values %s (Int_Glob=5 Bool_Glob=1 Ch_1=A Ch_2=B "
		        "Arr_1[8]=7 Arr_2[8][7]=%d)\r\n",
		        verified ? "verified" : "MISMATCH", s_runs + 10);
		BK_LOGI(TAG, "Dhrystone 2.1 : %.1f Dhrystones/s, %.2f DMIPS, %.3f DMIPS/MHz / %s / %s / %d:FreeRTOS\r\n",
		        rate, rate / DHRY_DMIPS_BASE,
		        rate / DHRY_DMIPS_BASE / (double)DHRY_CPU_FREQ_MHZ,
		        DHRY_OPT_LABEL, DHRY_MEM_LOCATION, DHRY_CTX_COUNT);
	}
#else
	dhry_report_ctx(0);
#endif
}

static void dhry_context_body(int ctx)
{
	dhry_clock_reset(ctx);
	dhry_arena_reset(ctx);
	s_data_addr[ctx] = NULL;

	if (ctx == 0) {
#if DHRY_CTX_COUNT > 1
		BK_LOGI(TAG, "Dhrystone 2.1, runs %d, SMP %d cores (ctx0 on AP core 0, ctx1 on AP core 1)\r\n",
		        s_runs, DHRY_CTX_COUNT);
#else
		/* rtos_get_core_id() is the global CPU number (CPU2 on the AP main core) */
		BK_LOGI(TAG, "Dhrystone 2.1, runs %d, CPU%u (AP core %u, single core)\r\n",
		        s_runs, rtos_get_core_id(), rtos_get_core_id() & 0x1u);
#endif
#if defined(DHRY_OPT_LABEL) && defined(DHRY_CPU_FREQ_MHZ)
		/* Benchmark build options, code/data placement and CPU frequency of this run. */
		BK_LOGI(TAG, "Config: dhrystone %s, code %s, data %s, %u MHz, contexts %d\r\n",
		        DHRY_OPT_LABEL, DHRY_CODE_PLACEMENT, DHRY_MEM_LOCATION,
		        (unsigned)DHRY_CPU_FREQ_MHZ, DHRY_CTX_COUNT);
#endif
		/* Flash cache dimension evidence: L1 I+D cache is enabled in both
		 * variants, only the shared L2 controller differs (0 = L1 only). */
		BK_LOGI(TAG, "L2 cache: enabled %d\r\n", (int)l2_cache_is_enabled());
	}

	/* Start barrier: both contexts leave together, like core_mark's parallel tasks */
	__atomic_fetch_add(&s_ready_count, 1u, __ATOMIC_SEQ_CST);
	while (s_ready_count < s_ctx_total)
		__asm volatile("yield");

#if DHRY_CTX_COUNT > 1
	if (ctx == 1)
		dhrystone_main1();
	else
		dhrystone_main();
#else
	dhrystone_main();
#endif
}

static void dhrystone_task(beken_thread_arg_t arg)
{
	int ctx = (int)(intptr_t)arg;

	dhry_context_body(ctx);

	rtos_set_semaphore(&s_ctx[ctx].done);
	rtos_delete_thread(NULL);
}

/* Run the benchmark in dedicated threads, with the same priority and stack size
 * core_mark uses for coremark0 / coremark1. Running it inside the shell task would
 * let every other task preempt the measurement. With DHRY_CTX_COUNT > 1 the
 * second core runs an independent copy of the benchmark (see the file header). */
void dhrystone_run(int argc, char *argv[])
{
	bk_err_t ret;
	int ctx;
	int started = 0;

	if (argc > 1) {
		int runs = dhry_atoi(argv[1]);

		if (runs > 0)
			s_runs = runs;
	}

	s_ready_count = 0;
	s_ctx_total = DHRY_CTX_COUNT;

#if DHRY_CTX_COUNT > 1
	/* core_mark's start_time(): taken before the contexts are created */
	s_wall_clk.us_total = 0;
	s_wall_clk.last_tick = dhry_timer_raw();
	s_wall_us = 0;
	memset(s_skip_score_value, 0, sizeof(s_skip_score_value));
#endif

	for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++) {
		s_ctx[ctx].thread = NULL;
		ret = rtos_init_semaphore_ex(&s_ctx[ctx].done, 1, 0);
		if (ret != kNoErr) {
			BK_LOGE(TAG, "init semaphore failed: %d\r\n", ret);
			goto fail;
		}
	}

	for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++) {
		const char *name = (DHRY_CTX_COUNT > 1) ? ((ctx == 0) ? "dhrystone0" : "dhrystone1") : "dhrystone";

#if CONFIG_SOC_SMP
		if (ctx == 0)
			ret = rtos_core0_create_thread(&s_ctx[ctx].thread, BEKEN_APPLICATION_PRIORITY,
			                               name, dhrystone_task,
			                               DHRY_THREAD_STACK_SIZE, (beken_thread_arg_t)(intptr_t)ctx);
		else
			ret = rtos_core1_create_thread(&s_ctx[ctx].thread, BEKEN_APPLICATION_PRIORITY,
			                               name, dhrystone_task,
			                               DHRY_THREAD_STACK_SIZE, (beken_thread_arg_t)(intptr_t)ctx);
#else
		ret = rtos_create_thread(&s_ctx[ctx].thread, BEKEN_APPLICATION_PRIORITY,
		                         name, dhrystone_task,
		                         DHRY_THREAD_STACK_SIZE, (beken_thread_arg_t)(intptr_t)ctx);
#endif
		if (ret != kNoErr) {
			BK_LOGE(TAG, "create dhrystone thread %d failed: %d\r\n", ctx, ret);
			/* Do not leave the other contexts spinning at the barrier */
			s_ctx_total = (uint32_t)ctx;
			if (started > 0)
				s_ctx_total = 1;
			break;
		}

		started++;
	}

	if (started == 0)
		goto fail;

	for (ctx = 0; ctx < started; ctx++)
		rtos_get_semaphore(&s_ctx[ctx].done, BEKEN_WAIT_FOREVER);

#if DHRY_CTX_COUNT > 1
	/* core_mark's stop_time(): read after the last context has finished */
	s_wall_us = dhry_wall_now();
#endif

	for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++)
		rtos_deinit_semaphore(&s_ctx[ctx].done);

	dhry_report_all();

	return;

fail:
	for (ctx = 0; ctx < DHRY_CTX_COUNT; ctx++) {
		if (s_ctx[ctx].done)
			rtos_deinit_semaphore(&s_ctx[ctx].done);
	}
}
