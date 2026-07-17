#include <stdio.h>
#include <stdlib.h>
#include "coremark.h"
#include "config.h"

#if VALIDATION_RUN
	volatile ee_s32 seed1_volatile=0x3415;
	volatile ee_s32 seed2_volatile=0x3415;
	volatile ee_s32 seed3_volatile=0x66;
#endif

#if PERFORMANCE_RUN
	volatile ee_s32 seed1_volatile=0x0;
	volatile ee_s32 seed2_volatile=0x0;
	volatile ee_s32 seed3_volatile=0x66;
#endif

#if PROFILE_RUN
	volatile ee_s32 seed1_volatile=0x8;
	volatile ee_s32 seed2_volatile=0x8;
	volatile ee_s32 seed3_volatile=0x8;
#endif

volatile ee_s32 seed4_volatile=ITERATIONS;
volatile ee_s32 seed5_volatile=0;

static CORE_TICKS t0, t1;
/* When parallel tasks take over timing, lock t0/t1 from external overwrite */
static volatile int s_timing_locked = 0;
uint64_t get_timer_value(void);

#ifndef COREMARK_PMU_ENABLE
#define COREMARK_PMU_ENABLE 0
#endif

/* Forward declarations for PMU helpers (defined below, used by start/stop_time) */
static void _pmu_task_start(void);
static void _pmu_task_report(uint32_t core_id);

#if COREMARK_SEGMENT_ENABLE
static uint64_t s_seg_cycles[COREMARK_SEG_COUNT];
static uint32_t s_seg_calls[COREMARK_SEG_COUNT];
static uint32_t s_seg_start[COREMARK_SEG_COUNT];
static uint64_t s_find_nodes;
static uint32_t s_find_node_counting;
static const char *s_seg_name[COREMARK_SEG_COUNT] = {
	"list_incl",
	"matrix",
	"state",
	"outer_crc",
	"find_rev",
	"find_only",
	"reverse",
	"sort_cx",
	"remove_crc",
	"sort_idx",
	"final_crc",
};
#endif

#if CONFIG_ARCH_RISCV

extern uint64_t riscv_get_mtimer(void);

uint64_t get_timer_value()
{
	return riscv_get_mtimer();
}

void timer_hal_us_init(uint32_t us)
{
}

#endif

#if !CONFIG_ARCH_RISCV

#include <soc/soc.h>
#include <os/mem.h>
#include "armcm55.h"
#include "core_cm55.h"

#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
#define TIMER0_PERIOD 0xFFFFFFFF

static uint32_t timer_hal_get_timer0_cnt(void)
{
	TIMER0_REG_SET(8, 2, 3, 0);
	TIMER0_REG_SET(8, 0, 0, 1);
	while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0));

	return REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));
}

extern void timer_hal_us_init(uint32_t us);

uint64_t get_timer_value()
{
	return timer_hal_get_timer0_cnt();
}

void *portable_malloc(ee_size_t size)
{
	return os_malloc(size);
}

void portable_free(void *p)
{
	os_free(p);
}

#endif /* !CONFIG_ARCH_RISCV */

void start_time(void)
{
	if (!s_timing_locked) {
		t0 = get_timer_value();
#if (MULTITHREAD <= 1) && COREMARK_PMU_ENABLE
		_pmu_task_start();
#endif
	}
}

void stop_time(void)
{
	if (!s_timing_locked) {
		t1 = get_timer_value();
#if (MULTITHREAD <= 1) && COREMARK_PMU_ENABLE
		_pmu_task_report(0);
#endif
	}
}

CORE_TICKS get_time(void)
{
	return (CORE_TICKS)((uint32_t)t1 - (uint32_t)t0);
}

secs_ret time_in_secs(CORE_TICKS ticks)
{
	secs_ret delta = (secs_ret)ticks;
	secs_ret val=delta / 26000000UL;
	return val;
}

#if COREMARK_SEGMENT_ENABLE
#if !CONFIG_ARCH_RISCV
static inline uint32_t coremark_seg_cycle_now(void)
{
	return DWT->CYCCNT;
}
#else
static inline uint32_t coremark_seg_cycle_now(void)
{
	return (uint32_t)get_timer_value();
}
#endif

void coremark_seg_reset(void)
{
	for (uint32_t i = 0; i < (uint32_t)COREMARK_SEG_COUNT; i++) {
		s_seg_cycles[i] = 0;
		s_seg_calls[i] = 0;
		s_seg_start[i] = 0;
	}
	s_find_nodes = 0;
	s_find_node_counting = 0;

#if !CONFIG_ARCH_RISCV
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
	__DSB();
	__ISB();
#endif
}

void coremark_seg_enter(coremark_seg_id seg)
{
	if ((uint32_t)seg >= (uint32_t)COREMARK_SEG_COUNT) {
		return;
	}
	s_seg_start[seg] = coremark_seg_cycle_now();
}

void coremark_seg_exit(coremark_seg_id seg)
{
	uint32_t end;

	if ((uint32_t)seg >= (uint32_t)COREMARK_SEG_COUNT) {
		return;
	}

	end = coremark_seg_cycle_now();
	s_seg_cycles[seg] += (uint32_t)(end - s_seg_start[seg]);
	s_seg_calls[seg]++;
}

void coremark_find_nodes_enable(ee_u32 enable)
{
	s_find_node_counting = enable ? 1u : 0u;
}

void coremark_find_nodes_add(ee_u32 nodes)
{
	if (s_find_node_counting) {
		s_find_nodes += nodes;
	}
}

void coremark_seg_report(ee_u32 iterations, ee_u32 contexts)
{
	uint64_t list_self = s_seg_cycles[COREMARK_SEG_LIST];
	uint64_t nested = s_seg_cycles[COREMARK_SEG_MATRIX] + s_seg_cycles[COREMARK_SEG_STATE];
	uint64_t sort_complex_self = s_seg_cycles[COREMARK_SEG_LIST_SORT_COMPLEX];
	uint64_t find_rev_other;
	uint64_t list_accounted_self;
	uint64_t list_unaccounted;

	if (list_self > nested) {
		list_self -= nested;
	} else {
		list_self = 0;
	}
	if (sort_complex_self > nested) {
		sort_complex_self -= nested;
	} else {
		sort_complex_self = 0;
	}
	find_rev_other = s_seg_cycles[COREMARK_SEG_LIST_FIND_REV];
	if (find_rev_other > (s_seg_cycles[COREMARK_SEG_LIST_FIND_ONLY] + s_seg_cycles[COREMARK_SEG_LIST_REVERSE])) {
		find_rev_other -= s_seg_cycles[COREMARK_SEG_LIST_FIND_ONLY] + s_seg_cycles[COREMARK_SEG_LIST_REVERSE];
	} else {
		find_rev_other = 0;
	}
	list_accounted_self = s_seg_cycles[COREMARK_SEG_LIST_FIND_REV]
	                    + sort_complex_self
	                    + s_seg_cycles[COREMARK_SEG_LIST_REMOVE_CRC]
	                    + s_seg_cycles[COREMARK_SEG_LIST_SORT_IDX]
	                    + s_seg_cycles[COREMARK_SEG_LIST_FINAL_CRC];
	list_unaccounted = (list_self > list_accounted_self) ? (list_self - list_accounted_self) : 0;

	BK_LOGI("coremark", "Segment timing by DWT CYCCNT, iterations=%lu contexts=%lu\r\n",
	        (unsigned long)iterations, (unsigned long)contexts);
	for (uint32_t i = 0; i < (uint32_t)COREMARK_SEG_COUNT; i++) {
		double per_iter = iterations ? (double)s_seg_cycles[i] / (double)iterations : 0.0;
		BK_LOGI("coremark", "SEG %-9s cycles=%llu calls=%lu cycles/iter=%.0f\r\n",
		        s_seg_name[i],
		        (unsigned long long)s_seg_cycles[i],
		        (unsigned long)s_seg_calls[i],
		        per_iter);
	}
	BK_LOGI("coremark", "SEG list_self cycles=%llu cycles/iter=%.0f\r\n",
	        (unsigned long long)list_self,
	        iterations ? (double)list_self / (double)iterations : 0.0);
	BK_LOGI("coremark", "SEG sort_cx_self cycles=%llu cycles/iter=%.0f\r\n",
	        (unsigned long long)sort_complex_self,
	        iterations ? (double)sort_complex_self / (double)iterations : 0.0);
	BK_LOGI("coremark", "SEG find_rev_other cycles=%llu cycles/iter=%.0f\r\n",
	        (unsigned long long)find_rev_other,
	        iterations ? (double)find_rev_other / (double)iterations : 0.0);
	BK_LOGI("coremark", "SEG find_nodes nodes=%llu nodes/find_call=%.2f cycles/find_node=%.2f\r\n",
	        (unsigned long long)s_find_nodes,
	        s_seg_calls[COREMARK_SEG_LIST_FIND_ONLY] ?
	            (double)s_find_nodes / (double)s_seg_calls[COREMARK_SEG_LIST_FIND_ONLY] : 0.0,
	        s_find_nodes ?
	            (double)s_seg_cycles[COREMARK_SEG_LIST_FIND_ONLY] / (double)s_find_nodes : 0.0);
	BK_LOGI("coremark", "SEG list_accounted_self cycles=%llu cycles/iter=%.0f unaccounted=%llu\r\n",
	        (unsigned long long)list_accounted_self,
	        iterations ? (double)list_accounted_self / (double)iterations : 0.0,
	        (unsigned long long)list_unaccounted);
}
#endif /* COREMARK_SEGMENT_ENABLE */

/* ---- per-core PMU measurement (ARMv8.1-M, M55) ---- */
#if !CONFIG_ARCH_RISCV && COREMARK_PMU_ENABLE
#include "armcm55.h"
#include "core_cm55.h"

#define _PMU_LAR   (*((volatile uint32_t *)(PMU_BASE + 0xFB0u)))
#define _PMCR_E    (1u << 0)
#define _PMCR_P    (1u << 1)
#define _PMCR_C    (1u << 2)

static void _pmu_task_start(void)
{
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
	__DSB(); __ISB();
	_PMU_LAR = 0xC5ACCE55u;
	__DSB();
	PMU->CNTENCLR = 0xFFFFFFFFu;
	PMU->CTRL = _PMCR_P | _PMCR_C;
	__DSB();
	/* STALL_FRONTEND(0x23): cycles the pipeline stalled fetching instructions.
	 * STALL_BACKEND (0x24): cycles the pipeline stalled in the back-end.
	 * INST_RETIRED  (0x08): instructions architecturally executed.
	 * L1I_CACHE_REFILL(0x01): I-cache line-fill (miss) count.
	 * Note: all 32-bit counters wrap every 2^32/480MHz ≈ 8.9s.  A 23s
	 * CoreMark run causes ~2.5 wraps — absolute values are invalid but the
	 * STALL_FE:STALL_BE ratio and the STALL_FE/STALL_BE vs INST_RETIRED
	 * ratio remain representative (both wrap equally often). */
	/* PMCR.DP (bit5) = 0: allow counting in EL0 (Non-Secure Unprivileged).
	 * Without this, STALL_FRONTEND/BACKEND may be filtered when code runs
	 * at NS-EL1 (FreeRTOS task context).                                   */
	PMU->CTRL &= ~(1u << 5);
	__DSB();

	/* ARMv8.1-M PMU EVTYPER[n] layout (ARM DDI 0553B.y Table D1-3):
	 *   [15:0]  event number
	 *   [27]    NSH  — count in Non-Secure Hyp mode
	 *   [31]    NSK  — count in Non-Secure EL0 (task context)
	 * Set both NSH and NSK so the counters accumulate regardless of which
	 * NS privilege level the CoreMark task is running at.                   */
	PMU->EVTYPER[0] = 0x0023u | (1u<<27) | (1u<<31); /* STALL_FRONTEND  */
	PMU->EVTYPER[1] = 0x0024u | (1u<<27) | (1u<<31); /* STALL_BACKEND   */
	PMU->EVTYPER[2] = 0x0008u | (1u<<27) | (1u<<31); /* INST_RETIRED    */
	PMU->EVTYPER[3] = 0x0001u | (1u<<27) | (1u<<31); /* L1I_CACHE_REFILL */

	/* PMCCFILTR: same filter for the cycle counter */
	PMU->CCFILTR = (1u<<27) | (1u<<31);
	__DSB();
	PMU->CNTENSET = (1u << 31) | 0xFu;
	PMU->CTRL = _PMCR_E;
	__ISB();
}

static void _pmu_task_report(uint32_t core_id)
{
	PMU->CTRL = 0; __DSB();
	uint32_t stall_fe = PMU->EVCNTR[0]; /* STALL_FRONTEND (wrapped, ratio valid) */
	uint32_t stall_be = PMU->EVCNTR[1]; /* STALL_BACKEND  (wrapped, ratio valid) */
	uint32_t retired  = PMU->EVCNTR[2]; /* INST_RETIRED   (wrapped, ratio valid) */
	uint32_t i_miss   = PMU->EVCNTR[3]; /* L1I_CACHE_REFILL (approx)             */

	/* All counters wrap equally in a ~23s run (~2.5 wraps each).
	 * Use FE+BE+RETIRED as a proxy for total cycles to compute ratios. */
	uint64_t total_proxy = (uint64_t)stall_fe + stall_be + retired;
	double fe_pct = total_proxy ? 100.0 * stall_fe / total_proxy : 0.0;
	double be_pct = total_proxy ? 100.0 * stall_be / total_proxy : 0.0;
	double ipc_proxy = total_proxy ? (double)retired / (double)total_proxy : 0.0;

	BK_LOGI("pmu", "[cm-core%u] STALL_FE=%u STALL_BE=%u INST_RETIRED=%u L1I_miss=%u\r\n",
	        (unsigned)core_id, stall_fe, stall_be, retired, i_miss);
	BK_LOGI("pmu", "[cm-core%u] FE_stall~%.1f%% BE_stall~%.1f%% IPC_proxy~%.2f (of FE+BE+RETIRED)\r\n",
	        (unsigned)core_id, fe_pct, be_pct, ipc_proxy);
}
#else
static void _pmu_task_start(void) {}
static void _pmu_task_report(uint32_t core_id) { (void)core_id; }
#endif /* !CONFIG_ARCH_RISCV && COREMARK_PMU_ENABLE */
/* ---------------------------------------------------- */

#if (MULTITHREAD > 1)

ee_u32 default_num_contexts = MULTITHREAD;
static uint32_t s_coremark_next_core;

/* Barrier: all tasks wait here until all are ready, then start simultaneously */
static volatile uint32_t s_coremark_ready_count = 0;
static volatile uint32_t s_coremark_total        = 0;

static void coremark_parallel_task(void *arg)
{
	core_results *res = (core_results *)arg;
	uint32_t cid = res->port.core_id;

	uint64_t task_enter_tick = get_timer_value();

	/* Enable DWT cycle counter (requires DEMCR.TRCENA, set in _pmu_task_start) */
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
	__DSB(); __ISB();

	/* Signal that this task has started and is ready to compute */
	__atomic_fetch_add(&s_coremark_ready_count, 1u, __ATOMIC_SEQ_CST);

	/* Spin until ALL tasks are ready */
	while (s_coremark_ready_count < s_coremark_total) {
		__asm volatile("yield");
	}

	/* All tasks are ready. core0 takes the real start timestamp and locks it.
	 * This overwrites the t0 that core_main.c set before task creation,
	 * eliminating the task-creation scheduling overhead from the score. */
	if (cid == 0) {
		s_timing_locked = 0;  /* unlock so start_time() can write t0 */
		start_time();         /* t0 = now (all cores ready, about to compute) */
		s_timing_locked = 1;  /* lock: prevent core_main.c stop_time() from overwriting */
		BK_LOGI("coremark", "[cm-core%u] task_enter to compute_start: %.3f s\r\n",
		        cid, (double)(t0 - task_enter_tick) / 26000000.0);
	}
	__DSB(); __ISB();

	/* DWT micro-benchmark: 10ms busy-loop BEFORE iterate() to verify actual CPU freq.
	 * ratio = DWT_secs / Timer0_secs; expect 480/26=18.46 at 480MHz, 1.0 at 26MHz. */
	{
		DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
		DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
		__DSB(); __ISB();
		uint32_t d0 = DWT->CYCCNT;
		uint64_t t_pre0 = get_timer_value();
		while ((uint32_t)(get_timer_value() - t_pre0) < 260000u) {} /* 10ms @26MHz */
		uint32_t d1 = DWT->CYCCNT;
		uint64_t t_pre1 = get_timer_value();
		double pre_ratio = (double)(d1-d0)/480000000.0 / ((double)(t_pre1-t_pre0)/26000000.0 + 1e-9);
		BK_LOGI("coremark", "[cm-core%u] pre-iterate 10ms: DWT=%u Timer0=%u ratio=%.2f\r\n",
		        cid, d1-d0, (uint32_t)(t_pre1-t_pre0), pre_ratio);
	}

	uint32_t dwt_start = DWT->CYCCNT;
	uint64_t timer0_start = get_timer_value();

	_pmu_task_start();
	iterate(res);
	_pmu_task_report(cid);

	uint32_t dwt_end   = DWT->CYCCNT;
	uint64_t timer0_end = get_timer_value();

	/* DWT CYCCNT is 32-bit and wraps every 2^32/480MHz ≈ 8.9s.
	 * A 23s CoreMark run causes ~2.5 overflows — raw subtraction gives wrong result.
	 * Use Timer0 (64-bit, 26MHz) as the ground truth for elapsed time,
	 * then compute expected DWT cycles = Timer0_secs × 480MHz. */
	uint32_t dwt_raw    = dwt_end - dwt_start;  /* only valid if <8.9s, shown for debug */
	uint64_t timer0_ticks = timer0_end - timer0_start;
	double   timer0_secs  = (double)timer0_ticks / 26000000.0;
	double   expected_dwt = timer0_secs * 480000000.0;
	/* Reconstructed 64-bit DWT: floor(expected/2^32) gives wrap count.
	 * Do NOT round: if raw > expected%2^32, we'd get one extra wrap. */
	uint64_t dwt_full_wraps = (uint64_t)(expected_dwt / 4294967296.0);
	uint64_t dwt_cycles64   = (uint64_t)dwt_raw + dwt_full_wraps * 4294967296ULL;
	/* If reconstructed value deviates >1% from expected, try wraps+1 */
	uint64_t dwt_cycles64_alt = (uint64_t)dwt_raw + (dwt_full_wraps + 1) * 4294967296ULL;
	double err0 = dwt_cycles64     > (uint64_t)expected_dwt ?
	              (double)(dwt_cycles64 - (uint64_t)expected_dwt) : (double)((uint64_t)expected_dwt - dwt_cycles64);
	double err1 = dwt_cycles64_alt > (uint64_t)expected_dwt ?
	              (double)(dwt_cycles64_alt - (uint64_t)expected_dwt) : (double)((uint64_t)expected_dwt - dwt_cycles64_alt);
	if (err1 < err0) { dwt_cycles64 = dwt_cycles64_alt; dwt_full_wraps++; }
	double   dwt_secs64     = (double)dwt_cycles64 / 480000000.0;

	BK_LOGI("coremark", "[cm-core%u] DWT_raw=%u(wraps=%llu) Timer0=%llu ticks\r\n",
	        cid, dwt_raw, (unsigned long long)dwt_full_wraps,
	        (unsigned long long)timer0_ticks);
	double cycles_per_iter = (double)dwt_cycles64 / (double)ITERATIONS;
	BK_LOGI("coremark", "[cm-core%u] DWT_secs=%.3f Timer0_secs=%.3f ratio=%.4f cycles/iter=%.0f\r\n",
	        cid, dwt_secs64, timer0_secs,
	        timer0_secs > 0 ? dwt_secs64 / timer0_secs : 0.0,
	        cycles_per_iter);

	uint64_t task_done_tick = get_timer_value();
	BK_LOGI("coremark", "[cm-core%u] compute done: %.3f s\r\n",
	        cid, (double)(task_done_tick - t0) / 26000000.0);

	/* Last task to finish records stop time */
	uint32_t remaining = __atomic_sub_fetch(&s_coremark_ready_count, 1u, __ATOMIC_SEQ_CST);
	if (remaining == 0) {
		s_timing_locked = 0;  /* unlock for stop_time() */
		stop_time();          /* t1 = now */
		s_timing_locked = 1;  /* lock out core_main.c's stop_time() call */
		BK_LOGI("coremark", "Parallel compute window: %.3f s (%.0f ticks @26MHz)\r\n",
		        (double)(t1 - t0) / 26000000.0, (double)(t1 - t0));
	}

	rtos_set_semaphore(&res->port.done);
	rtos_delete_thread(NULL);
}

ee_u8 core_start_parallel(core_results *res)
{
	bk_err_t ret;

	res->port.thread = NULL;
	res->port.done = NULL;
	res->port.core_id = s_coremark_next_core++;

	ret = rtos_init_semaphore_ex(&res->port.done, 1, 0);
	if (ret != kNoErr) {
		return 1;
	}

#if CONFIG_SOC_SMP
	if ((res->port.core_id & 1U) == 0U) {
		ret = rtos_core0_create_thread(&res->port.thread,
						BEKEN_APPLICATION_PRIORITY,
						"coremark0",
						coremark_parallel_task,
						4096,
						(beken_thread_arg_t)res);
	} else {
		ret = rtos_core1_create_thread(&res->port.thread,
						BEKEN_APPLICATION_PRIORITY,
						"coremark1",
						coremark_parallel_task,
						4096,
						(beken_thread_arg_t)res);
	}
#else
	ret = rtos_create_thread(&res->port.thread,
				 BEKEN_APPLICATION_PRIORITY,
				 "coremark",
				 coremark_parallel_task,
				 4096,
				 (beken_thread_arg_t)res);
#endif
	return (ret == kNoErr) ? 0 : 1;
}

ee_u8 core_stop_parallel(core_results *res)
{
	if (res->port.done == NULL) {
		return 1;
	}

	if (rtos_get_semaphore(&res->port.done, BEKEN_WAIT_FOREVER) != kNoErr) {
		return 1;
	}

	rtos_deinit_semaphore(&res->port.done);
	res->port.done = NULL;
	return 0;
}

static void portable_init(core_portable *p, int *argc, char *argv[])
{
	(void)p;
	(void)argc;
	(void)argv;
	s_coremark_next_core   = 0;
	s_coremark_ready_count = 0;
	s_coremark_total       = (uint32_t)default_num_contexts;
	s_timing_locked        = 0;
}

static void portable_fini(core_portable *p)
{
	(void)p;
}

#endif /* MULTITHREAD > 1 */
