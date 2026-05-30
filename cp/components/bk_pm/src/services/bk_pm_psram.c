// Copyright 2021-2025 Beken
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
#include <modules/pm.h>
#include <sys_types.h>
#include "sys_driver.h"
#include <driver/pwr_clk.h>
#include <driver/psram.h>
#include <os/mem.h>
#include <common/bk_assert.h>
#if CONFIG_WDT_EN
#include "wdt_driver.h"
#endif

#include "pm_psram.h"
#include "pm_debug.h"


static uint32_t s_pm_psram_ctrl_state     = 0;
#if (CONFIG_CPU_CNT > 1)
static uint32_t s_pm_cp1_psram_malloc_count_state       = 0;
#endif

/* ============================================================
 * PSRAM data-retention probe.
 *
 * A multi-pattern probe block is written into PSRAM right BEFORE
 * the retention path runs (i.e. before AP power-off), and read
 * back / verified right AFTER the recovery path runs (i.e. after
 * AP power-on). This lets us see in the CP log whether the PSRAM
 * cells survived the AP power cycle while the controller was
 * gated.
 *
 * Two independent probe blocks are placed, one per PSRAM device.
 * The preferred placement is a dedicated reserved partition in
 * ram_regions.csv (4 KB each); the build then exposes
 *   CONFIG_PSRAM0_RETENTION_PROBE_ADDR / _SIZE
 *   CONFIG_PSRAM1_RETENTION_PROBE_ADDR / _SIZE
 * which this file picks up automatically. If a project does not
 * carve out those partitions, the code falls back to the tail of
 * the corresponding slab pool, which is far less likely to be hit
 * by a head-first slab allocator than a mid-pool address.
 *
 * Each block is PM_PSRAM_RETENTION_PROBE_WORDS * 4 bytes long
 * (1024 B by default).
 *
 * Per-block layout (in 32-bit words, index i):
 *   i = 0                       : MAGIC_HEAD (0xA5A5C3C3)
 *   i = 1                       : seq (monotonically incremented)
 *   i = 2                       : bank id (0 = PSRAM0, 1 = PSRAM1)
 *   i = 3                       : PAT   (0xDEADBEEF)
 *   i = 4                       : ~PAT
 *   i = 5 .. N-4                : pattern body (see probe_word_pattern)
 *   i = N-3                     : walking-1 ( 1<<(seq & 31) )
 *   i = N-2                     : checksum (xor of words 0 .. N-3)
 *   i = N-1                     : MAGIC_TAIL (0xC3C3A5A5)
 *
 * Probe is gated by PM_PSRAM_RETENTION_PROBE_ENABLE (file-local,
 * default 0). When the bigger CONFIG_PSRAM_DATA_RETENTION_ENABLE
 * is also OFF, the entire retention path is skipped and so is the
 * probe regardless of this flag.
 * ============================================================ */
#ifndef PM_PSRAM_RETENTION_PROBE_ENABLE
#define PM_PSRAM_RETENTION_PROBE_ENABLE 0
#endif

#if PM_PSRAM_RETENTION_PROBE_ENABLE && CONFIG_PSRAM_DATA_RETENTION_ENABLE

#include "ram_regions.h"   /* may provide CONFIG_PSRAM[01]_RETENTION_PROBE_ADDR/_SIZE */

/* Prefer the addresses generated from ram_regions.csv (dedicated 4KB
 * partitions named PSRAM0_RETENTION_PROBE / PSRAM1_RETENTION_PROBE).
 * If the project does not reserve those partitions, fall back to the
 * tail of the corresponding slab pool, which is far less likely to be
 * touched by a head-first slab allocator than a mid-pool address. */
#ifndef PM_PSRAM0_RETENTION_PROBE_ADDR
#ifdef  CONFIG_PSRAM0_RETENTION_PROBE_ADDR
#define PM_PSRAM0_RETENTION_PROBE_ADDR (CONFIG_PSRAM0_RETENTION_PROBE_ADDR)
#else
#define PM_PSRAM0_RETENTION_PROBE_ADDR (0x60FFFC00U)   /* last 1KB of PSRAM0 (fallback) */
#endif
#endif
#ifndef PM_PSRAM1_RETENTION_PROBE_ADDR
#ifdef  CONFIG_PSRAM1_RETENTION_PROBE_ADDR
#define PM_PSRAM1_RETENTION_PROBE_ADDR (CONFIG_PSRAM1_RETENTION_PROBE_ADDR)
#else
#define PM_PSRAM1_RETENTION_PROBE_ADDR (0x64D7FC00U)   /* last 1KB of PSRAM1 CODED slab (fallback) */
#endif
#endif

#ifndef PM_PSRAM_RETENTION_PROBE_WORDS
#define PM_PSRAM_RETENTION_PROBE_WORDS (256U)   /* 1024 bytes */
#endif

/* Compile-time guard: probe must fit inside the reserved partition. */
#if defined(CONFIG_PSRAM0_RETENTION_PROBE_SIZE)
_Static_assert(PM_PSRAM_RETENTION_PROBE_WORDS * 4U <= CONFIG_PSRAM0_RETENTION_PROBE_SIZE,
               "PSRAM0 retention probe overflows reserved partition");
#endif
#if defined(CONFIG_PSRAM1_RETENTION_PROBE_SIZE)
_Static_assert(PM_PSRAM_RETENTION_PROBE_WORDS * 4U <= CONFIG_PSRAM1_RETENTION_PROBE_SIZE,
               "PSRAM1 retention probe overflows reserved partition");
#endif

#define PM_PSRAM_RETENTION_PROBE_MAGIC_HEAD (0xA5A5C3C3U)
#define PM_PSRAM_RETENTION_PROBE_MAGIC_TAIL (0xC3C3A5A5U)
#define PM_PSRAM_RETENTION_PROBE_PAT        (0xDEADBEEFU)
#define PM_PSRAM_RETENTION_PROBE_BANK_NUM   (2U)

static const uint32_t s_pm_psram_retention_probe_base[PM_PSRAM_RETENTION_PROBE_BANK_NUM] = {
	PM_PSRAM0_RETENTION_PROBE_ADDR,
	PM_PSRAM1_RETENTION_PROBE_ADDR,
};

static uint32_t s_pm_psram_retention_seq = 0;

/* Deterministic per-word pattern: function of (idx, seq, bank).
 * Mixes several pattern types (XOR, complement, mul-add, rotate)
 * so a single stuck-bit / line-coupling defect will surface as a
 * mismatch on at least one of the patterns. */
static inline uint32_t probe_word_pattern(uint32_t idx, uint32_t seq, uint32_t bank)
{
	uint32_t base = ((bank & 0xFU) << 28) | ((seq & 0xFFFU) << 16) | (idx & 0xFFFFU);
	switch (idx & 7U) {
		case 0U: return 0xA5A5C3C3U ^ base;
		case 1U: return 0x5A5A3C3CU ^ base;
		case 2U: return base * 0x9E3779B1U + 0xDEADBEEFU;
		case 3U: return ~base;
		case 4U: return (base << 1) | (base >> 31);          /* rotl 1  */
		case 5U: return (base >> 1) | (base << 31);          /* rotr 1  */
		case 6U: return base ^ (base << 16);
		default: return base + 0xFEEDFACEU;                  /* case 7  */
	}
}

static void bk_pm_psram_retention_probe_write_one(uint32_t bank)
{
	/* Defensive parameter check: BK_ASSERT for debug builds to fail-fast,
	 * runtime return as fallback for release builds where BK_ASSERT is a no-op. */
	BK_ASSERT(bank < PM_PSRAM_RETENTION_PROBE_BANK_NUM);
	if (bank >= PM_PSRAM_RETENTION_PROBE_BANK_NUM) {
		BK_LOGE("pm_psram", "probe_write: invalid bank %u\r\n", bank);
		return;
	}

	volatile uint32_t *p = (volatile uint32_t *)s_pm_psram_retention_probe_base[bank];
	BK_ASSERT(p != NULL);
	if (p == NULL) {
		BK_LOGE("pm_psram", "probe_write: NULL probe base bank%u\r\n", bank);
		return;
	}

	uint32_t n = PM_PSRAM_RETENTION_PROBE_WORDS;
	uint32_t seq = s_pm_psram_retention_seq;
	uint32_t cks = 0;
	uint32_t i;

	p[0] = PM_PSRAM_RETENTION_PROBE_MAGIC_HEAD;
	p[1] = seq;
	p[2] = bank;
	p[3] = PM_PSRAM_RETENTION_PROBE_PAT;
	p[4] = ~PM_PSRAM_RETENTION_PROBE_PAT;
	for (i = 5U; i < n - 3U; i++) {
		p[i] = probe_word_pattern(i, seq, bank);
	}
	p[n - 3U] = 1U << (seq & 31U);                            /* walking-1     */

	cks = 0;
	for (i = 0; i < n - 2U; i++) {
		cks ^= p[i];
	}
	p[n - 2U] = cks;
	p[n - 1U] = PM_PSRAM_RETENTION_PROBE_MAGIC_TAIL;
}

static void bk_pm_psram_retention_probe_write(void)
{
	uint32_t bank;
	s_pm_psram_retention_seq++;
	for (bank = 0; bank < PM_PSRAM_RETENTION_PROBE_BANK_NUM; bank++) {
		bk_pm_psram_retention_probe_write_one(bank);
		BK_LOGI("pm_psram",
				"retention_probe write bank%u @0x%08x seq=%u words=%u\r\n",
				bank, s_pm_psram_retention_probe_base[bank],
				s_pm_psram_retention_seq, PM_PSRAM_RETENTION_PROBE_WORDS);
	}
}

static bool bk_pm_psram_retention_probe_verify_one(uint32_t bank)
{
	/* Defensive parameter check: BK_ASSERT for debug builds to fail-fast,
	 * runtime return as fallback for release builds where BK_ASSERT is a no-op. */
	BK_ASSERT(bank < PM_PSRAM_RETENTION_PROBE_BANK_NUM);
	if (bank >= PM_PSRAM_RETENTION_PROBE_BANK_NUM) {
		BK_LOGE("pm_psram", "probe_verify: invalid bank %u\r\n", bank);
		return false;
	}

	volatile uint32_t *p = (volatile uint32_t *)s_pm_psram_retention_probe_base[bank];
	BK_ASSERT(p != NULL);
	if (p == NULL) {
		BK_LOGE("pm_psram", "probe_verify: NULL probe base bank%u\r\n", bank);
		return false;
	}

	uint32_t n = PM_PSRAM_RETENTION_PROBE_WORDS;
	uint32_t seq = s_pm_psram_retention_seq;
	uint32_t cks = 0;
	uint32_t mismatch = 0;
	uint32_t first_bad_idx = 0;
	uint32_t first_bad_exp = 0;
	uint32_t first_bad_got = 0;
	uint32_t i;
	uint32_t exp;

	bool magic_head_ok = (p[0] == PM_PSRAM_RETENTION_PROBE_MAGIC_HEAD);
	bool seq_ok        = (p[1] == seq);
	bool bank_ok       = (p[2] == bank);
	bool pat_ok        = ((p[3] == PM_PSRAM_RETENTION_PROBE_PAT) &&
						  (p[4] == ~PM_PSRAM_RETENTION_PROBE_PAT));
	bool walk1_ok      = (p[n - 3U] == (1U << (seq & 31U)));
	bool magic_tail_ok = (p[n - 1U] == PM_PSRAM_RETENTION_PROBE_MAGIC_TAIL);

	for (i = 5U; i < n - 3U; i++) {
		exp = probe_word_pattern(i, seq, bank);
		if (p[i] != exp) {
			if (mismatch == 0) {
				first_bad_idx = i;
				first_bad_exp = exp;
				first_bad_got = p[i];
			}
			mismatch++;
		}
	}

	cks = 0;
	for (i = 0; i < n - 2U; i++) {
		cks ^= p[i];
	}
	bool cks_ok = (p[n - 2U] == cks);

	bool all_ok = magic_head_ok && seq_ok && bank_ok && pat_ok &&
				  walk1_ok && magic_tail_ok && cks_ok && (mismatch == 0);

	if (all_ok) {
		BK_LOGI("pm_psram",
				"retention_probe verify PASS bank%u @0x%08x seq=%u words=%u\r\n",
				bank, s_pm_psram_retention_probe_base[bank], seq,
				PM_PSRAM_RETENTION_PROBE_WORDS);
	} else {
		BK_LOGE("pm_psram",
				"retention_probe verify FAIL bank%u @0x%08x seq_exp=%u "
				"head=%d seq=%d bank=%d pat=%d walk1=%d tail=%d cks=%d mismatch=%u\r\n",
				bank, s_pm_psram_retention_probe_base[bank], seq,
				magic_head_ok, seq_ok, bank_ok, pat_ok, walk1_ok,
				magic_tail_ok, cks_ok, mismatch);
		if (mismatch != 0) {
			BK_LOGE("pm_psram",
					"retention_probe first_bad bank%u idx=%u exp=0x%08x got=0x%08x\r\n",
					bank, first_bad_idx, first_bad_exp, first_bad_got);
		}
	}
	return all_ok;
}

static void bk_pm_psram_retention_probe_verify(void)
{
	uint32_t bank;
	for (bank = 0; bank < PM_PSRAM_RETENTION_PROBE_BANK_NUM; bank++) {
		(void)bk_pm_psram_retention_probe_verify_one(bank);
	}
}

#endif /* PM_PSRAM_RETENTION_PROBE_ENABLE && CONFIG_PSRAM_DATA_RETENTION_ENABLE */

static bk_err_t bk_pm_psram_init_and_check(void)
{
	bk_err_t ret = BK_OK;
	ret = bk_psram_init();
	if (ret != BK_OK)
	{
		return ret;
	}
	return BK_OK;
}

/*
 * NOTE: do NOT add BK_LOGI / BK_LOGE / BK_LOGD (or any path going through
 * the async shell logger) inside this function.
 *
 * When CONFIG_PSRAM_AS_SYS_MEMORY=y, the shell dynamic-log allocator
 * LOG_MALLOC == psram_malloc. The first vote-on happens *before* the
 * PSRAM heap is initialized, so psram_malloc_cm() calls back into
 * bk_pm_module_vote_psram_ctrl(... STATE_ON) to bring PSRAM up. Any
 * BK_LOGx in this function would then need a dynamic-log block, which
 * re-enters this same function and causes unbounded recursion / stack
 * overflow at boot.
 *
 * If absolutely needed, use BK_DUMP_OUT() instead -- it uses a fixed
 * shell_assert_buff[] and writes synchronously to UART, bypassing
 * LOG_MALLOC. Otherwise put diagnostics in the callee (bk_psram_init /
 * bk_psram_data_retention_recover) where the heap is already up.
 */
bk_err_t bk_pm_module_vote_psram_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state)
{
	bk_err_t ret = BK_OK;

	GLOBAL_INT_DECLARATION();
    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
        GLOBAL_INT_DISABLE();
        s_pm_psram_ctrl_state |= 0x1 << (module);
        GLOBAL_INT_RESTORE();

#if CONFIG_PSRAM_DATA_RETENTION_ENABLE
		/* Branch on AP first-boot flag (PM_AP_WORK_STATE_FIRST_BOOT):
		 *   - cold boot                -> external PSRAM cells were just
		 *                                 powered on, full bk_psram_init()
		 *                                 (chip-id detection + cal +
		 *                                  mode-reg program) is required;
		 *   - sleep / AP-power-cycle  -> PSRAM cells were kept alive by
		 *                                 the retention path; we only need
		 *                                 the light-weight controller
		 *                                 recovery (ckg-bypass + clk +
		 *                                  soft-reset + mode-reg restore).
		 *
		 * The flag is set to true in reset_reason.c on RESET_SOURCE_POWERON
		 * and cleared in pm_module_shutdown_cpu1() right after the first
		 * AP power-off, so the very first ON after cold boot lands in the
		 * init branch and every subsequent ON lands in the recover branch. */
		bool is_first_boot = bk_pm_ap_first_boot_get();
		if (is_first_boot) {
			ret = bk_pm_psram_init_and_check();
		} else {
			ret = bk_psram_data_retention_recover();
			if (ret != BK_OK) {
				/* Controller-side recovery failed (PSRAM cells may still
				 * hold the data, but the controller is in an unknown
				 * state). Fall back to full re-init so the controller is
				 * at least usable and AP can boot; cell data is lost in
				 * this case. No log here -- see WARN at function head. */
				ret = bk_pm_psram_init_and_check();
			}
		}
#else  /* !CONFIG_PSRAM_DATA_RETENTION_ENABLE: original init-only path */
		ret = bk_pm_psram_init_and_check();
#endif /* CONFIG_PSRAM_DATA_RETENTION_ENABLE */

		if(ret != BK_OK)
		{
			LOGE("Psram_I err0:%d",ret);
			// bk_psram_deinit();
			// ret = bk_pm_psram_init_and_check();
			// if(ret != BK_OK)
			// {
			// 	LOGE("Psram_I err1:%d",ret);
			// 	bk_psram_deinit();
			// 	ret = bk_pm_psram_init_and_check();
			// 	if(ret != BK_OK)
			// 	{
			// 		LOGE("Psram_I err2:%d",ret);
			// 		#if CONFIG_WDT_EN
			// 		bk_wdt_force_reboot();//try 3 times, if fail ,reboot.
			// 		#endif
			// 	}
			// }
		}

#if PM_PSRAM_RETENTION_PROBE_ENABLE && CONFIG_PSRAM_DATA_RETENTION_ENABLE
		/* The probe only carries meaningful content after the first
		 * retention write; on cold boot PSRAM contains garbage and the
		 * verify would always fail, so skip it. */
		if (!is_first_boot && (ret == BK_OK)) {
			bk_pm_psram_retention_probe_verify();
		}
#endif
	}
    else //power down
    {
		//if(s_pm_psram_ctrl_state&(0x1 << (module)))
		{
			GLOBAL_INT_DISABLE();
			s_pm_psram_ctrl_state &= ~(0x1 << (module));
			GLOBAL_INT_RESTORE();
			//if(0x0 == s_pm_psram_ctrl_state)
			{
#if CONFIG_PSRAM_DATA_RETENTION_ENABLE
#if PM_PSRAM_RETENTION_PROBE_ENABLE
				/* Write a probe pattern BEFORE the retention path so that we
				 * can verify after power-up whether the PSRAM cells held
				 * their content while the AP power-domain was off. */
				bk_pm_psram_retention_probe_write();
#endif
				/* Replace bk_psram_deinit() with the retention path: flush
				 * the controller, latch PSRAM I/O pads at 3V, keep the PSRAM
				 * voltage rail ON so cells stay alive while AHBP/M55 power
				 * goes away. */
				ret = bk_psram_data_retention();
				if (ret != BK_OK) {
					/* Retention setup failed; the controller may be left in
					 * an inconsistent state. Fall back to a clean deinit so
					 * the next vote_on starts from a known state via
					 * bk_psram_init(). Cell data WILL be lost in this case.
					 * No log here -- see WARN at function head. */
					bk_psram_deinit();
				}
#else  /* !CONFIG_PSRAM_DATA_RETENTION_ENABLE: original deinit path */
				bk_psram_deinit();
#endif /* CONFIG_PSRAM_DATA_RETENTION_ENABLE */
			}
		}
	}
	return BK_OK;
}

bk_err_t pm_cp1_psram_malloc_count_state_set(uint32_t value)
{
	#if (CONFIG_CPU_CNT > 1)
	s_pm_cp1_psram_malloc_count_state = value;
	#endif
	return BK_OK;
}

bk_err_t pm_cp1_psram_malloc_state_get()
{
	#if (CONFIG_CPU_CNT > 1)
	s_pm_cp1_psram_malloc_count_state = bk_pm_get_cp1_psram_malloc_count(0);
	#endif
	return BK_OK;
}

__IRAM_SEC bk_err_t pm_psram_malloc_state_and_power_ctrl()
{
#if CONFIG_PSRAM_AS_SYS_MEMORY
	uint32_t cp0_psram_malloc_count = 0;
	uint32_t cp1_psram_malloc_count = 0;
	/*get the cp1 psram malloc count*/
	#if (CONFIG_CPU_CNT > 1)
	cp1_psram_malloc_count = s_pm_cp1_psram_malloc_count_state;
	if (pm_debug_mode() == 64)
	{
		if(s_pm_cp1_psram_malloc_count_state > 0)
		{
			LOGV("CP1 psram malloc count[%d] > 0\r\n",cp1_psram_malloc_count);
			LOGV("Power consumption will get higher,free them\r\n");

			bk_pm_dump_cp1_psram_malloc_info();
		}
	}
	#endif
	/*get the cp0 psram malloc count*/
	cp0_psram_malloc_count = bk_psram_heap_get_used_count();
	if (pm_debug_mode() == 64)
	{
		if(cp0_psram_malloc_count > 0)
		{
			LOGV("CP0 psram malloc count[%d] > 0\r\n",cp0_psram_malloc_count);
			LOGV("Power consumption will get higher,free them\r\n");

			bk_psram_heap_get_used_state();
		}
	}
	if((cp0_psram_malloc_count == 0)&&(cp1_psram_malloc_count == 0))
	{
		bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_AS_MEM,PM_POWER_MODULE_STATE_OFF);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM, PM_POWER_MODULE_STATE_OFF);
	}

#endif
	return BK_OK;
}

void pm_debug_psram()
{
	uint32_t cp0_psram_malloc_count = 0;

	/*get the cp1 psram malloc count*/
	#if (CONFIG_CPU_CNT > 1)
	uint32_t cp1_psram_malloc_count = s_pm_cp1_psram_malloc_count_state;

	if(cp1_psram_malloc_count > 0)
	{
		LOGI("CP1 psram malloc count[%d] > 0\r\n",cp1_psram_malloc_count);
		LOGI("Power consumption will get higher, please free them\r\n");
		bk_pm_dump_cp1_psram_malloc_info();
	}
	#endif

	/*get the cp0 psram malloc count*/
	cp0_psram_malloc_count = bk_psram_heap_get_used_count();
	if(cp0_psram_malloc_count > 0)
	{
		LOGI("CP0 psram malloc count[%d] > 0\r\n",cp0_psram_malloc_count);
		LOGI("power consumption will get higher,free them\r\n");
		bk_psram_heap_get_used_state();
	}
}

void pm_debug_psram_state()
{
	LOGI("pm_psram:0x%x 0x%x\r\n",s_pm_psram_ctrl_state,bk_psram_heap_init_flag_get());
}

