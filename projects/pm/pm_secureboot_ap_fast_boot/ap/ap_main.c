// Copyright 2025 Beken
//
// pm_secureboot_ap_fast_boot - AP NS application entry.
// Cold boot runs once; after AP OFF/ON the saved CPU2 execution point resumes.

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <modules/pm.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdint.h>

#define TAG "ap_fast_boot"

#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
#define PSRAM_RETENTION_TEST_SIZE (1024U * 1024U)
#define PSRAM_RETENTION_TEST_WORDS \
	(PSRAM_RETENTION_TEST_SIZE / sizeof(uint32_t))
#define PSRAM_RETENTION_MAX_MISMATCH_LOGS 64U
#define PSRAM_RETENTION_MAX_REREAD_LOGS 8U
#define PSRAM_RETENTION_REREAD_COUNT 100U

typedef struct {
	uint32_t mismatch_count;
	uint32_t first_index;
	uint32_t last_index;
	uint32_t logged_index[PSRAM_RETENTION_MAX_MISMATCH_LOGS];
	uint32_t logged_actual[PSRAM_RETENTION_MAX_MISMATCH_LOGS];
} psram_retention_scan_result_t;
#endif

static volatile uint32_t s_ap_main_entry_count;
static beken_thread_t s_resume_test_thread;
#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
static uint32_t s_psram_retention_data[PSRAM_RETENTION_TEST_WORDS]
	__attribute__((section(".psram.bss"), aligned(32)));
static volatile uint32_t s_psram_test_initialized;
static volatile uint32_t s_psram_crc_before;
static volatile uint32_t s_psram_crc_ready;
static volatile uint32_t s_psram_crc_valid;
static volatile uint32_t s_psram_check_pending;
static volatile uint32_t s_psram_retention_cycle;

static uint32_t psram_retention_crc32(volatile const uint32_t *data,
	uint32_t word_count)
{
	static const uint32_t crc32_nibble_table[16] = {
		0x00000000U, 0x1DB71064U, 0x3B6E20C8U, 0x26D930ACU,
		0x76DC4190U, 0x6B6B51F4U, 0x4DB26158U, 0x5005713CU,
		0xEDB88320U, 0xF00F9344U, 0xD6D6A3E8U, 0xCB61B38CU,
		0x9B64C2B0U, 0x86D3D2D4U, 0xA00AE278U, 0xBDBDF21CU,
	};
	uint32_t crc = 0xFFFFFFFFU;

	/*
	 * Read PSRAM one word at a time. The test region is non-cacheable, so
	 * byte accesses make a 1 MiB scan too slow for the AP suspend path.
	 * BK7259 is little-endian; eight reflected nibble steps per word produce
	 * the same IEEE CRC32 as processing its four bytes in address order.
	 */
	for (uint32_t i = 0; i < word_count; i++) {
		uint32_t actual = data[i];

		crc ^= actual;
		for (uint32_t nibble = 0; nibble < 8U; nibble++) {
			crc = (crc >> 4) ^ crc32_nibble_table[crc & 0xFU];
		}
	}

	return ~crc;
}

static void psram_retention_scan_incrementing_pattern(
	volatile const uint32_t *data, uint32_t word_count,
	psram_retention_scan_result_t *scan_result)
{
	scan_result->mismatch_count = 0U;
	scan_result->first_index = 0U;
	scan_result->last_index = 0U;

	/*
	 * This is intentionally a separate PSRAM pass from CRC verification.
	 * It verifies that every 32-bit word still contains the incrementing
	 * value written before AP power-off.
	 */
	for (uint32_t i = 0; i < word_count; i++) {
		uint32_t actual = data[i];

		if (actual != i) {
			uint32_t mismatch = scan_result->mismatch_count;

			if (mismatch == 0U) {
				scan_result->first_index = i;
			}
			if (mismatch < PSRAM_RETENTION_MAX_MISMATCH_LOGS) {
				scan_result->logged_index[mismatch] = i;
				scan_result->logged_actual[mismatch] = actual;
			}
			scan_result->last_index = i;
			scan_result->mismatch_count = mismatch + 1U;
		}
	}
}

static bk_err_t psram_retention_quiesce(void *arg)
{
	(void)arg;

	/*
	 * Never scan 1 MiB of non-cacheable PSRAM in this callback. The PM
	 * framework waits for quiesce before it can power AP off, so a full scan
	 * here causes CP's suspend timeout. The pattern is immutable; its CRC was
	 * calculated after initialization and is still the pre-sleep CRC.
	 */
	s_psram_crc_valid =
		(s_psram_test_initialized != 0U) && (s_psram_crc_ready != 0U);

	return BK_OK;
}

static bk_err_t psram_retention_resume(void *arg)
{
	(void)arg;

	/*
	 * This callback also runs when an AP power-off attempt is rolled back.
	 * Leave the CRC untouched and defer verification to app_resume, which
	 * runs only after a real AP power-off and wake-up.
	 */
	return BK_OK;
}

static bk_err_t psram_retention_app_resume(void *arg)
{
	(void)arg;
	if (s_psram_crc_valid != 0U) {
		/*
		 * app_resume is reached only after a real AP power-off/wake-up.
		 * Defer the expensive PSRAM scan to the test task so the PM task and
		 * CP startup handshake are not blocked.
		 */
		s_psram_crc_valid = 0U;
		s_psram_check_pending = 1U;
	}

	return BK_OK;
}

static void psram_retention_reread_mismatch(uint32_t cycle,
	uint32_t word_index, uint32_t scan_actual)
{
	volatile const uint32_t *addr = &s_psram_retention_data[word_index];
	uint32_t first = 0U;
	uint32_t last = 0U;
	uint32_t observed_or = 0U;
	uint32_t observed_and = 0xFFFFFFFFU;
	uint32_t expected_reads = 0U;
	uint32_t different_from_first = 0U;
	uint32_t variation_mask;
	const char *diagnosis;

	for (uint32_t i = 0; i < PSRAM_RETENTION_REREAD_COUNT; i++) {
		uint32_t actual = *addr;

		if (i == 0U) {
			first = actual;
		} else if (actual != first) {
			different_from_first++;
		}
		if (actual == word_index) {
			expected_reads++;
		}
		observed_or |= actual;
		observed_and &= actual;
		last = actual;
	}

	variation_mask = observed_or ^ observed_and;
	if (variation_mask != 0U) {
		diagnosis = "UNSTABLE_READ";
	} else if (first == word_index) {
		diagnosis = "TRANSIENT_READ_ERROR";
	} else {
		diagnosis = "STABLE_DATA_ERROR";
	}

	BK_LOGE(TAG,
		"PSRAM_REREAD cycle=%u addr=%p word=%u samples=%u "
		"expected=0x%08x scan_actual=0x%08x first=0x%08x last=0x%08x "
		"expected_reads=%u different_from_first=%u variation_mask=0x%08x "
		"diagnosis=%s\r\n",
		cycle,
		(void *)addr,
		word_index,
		PSRAM_RETENTION_REREAD_COUNT,
		word_index,
		scan_actual,
		first,
		last,
		expected_reads,
		different_from_first,
		variation_mask,
		diagnosis);
}

static void psram_retention_rearm_pattern(uint32_t cycle,
	uint32_t expected_crc)
{
	uint32_t rearm_crc;

	/*
	 * Do not carry corruption from one retention cycle into the next one.
	 * Rebuild the complete pattern so each cycle reports only newly-created
	 * errors, then read it back once through CRC before arming the next test.
	 */
	s_psram_crc_ready = 0U;
	for (uint32_t i = 0; i < PSRAM_RETENTION_TEST_WORDS; i++) {
		s_psram_retention_data[i] = i;
	}
	__asm volatile("dsb sy" ::: "memory");

	rearm_crc = psram_retention_crc32(
		(volatile const uint32_t *)s_psram_retention_data,
		PSRAM_RETENTION_TEST_WORDS);
	if (rearm_crc == expected_crc) {
		s_psram_crc_ready = 1U;
		BK_LOGI(TAG,
			"PSRAM_REARM PASS cycle=%u addr=%p size=%u crc=0x%08x\r\n",
			cycle,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			rearm_crc);
	} else {
		BK_LOGE(TAG,
			"PSRAM_REARM FAIL cycle=%u addr=%p size=%u "
			"expected_crc=0x%08x actual_crc=0x%08x; next test disabled\r\n",
			cycle,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			expected_crc,
			rearm_crc);
	}
}

static void psram_retention_verify_after_wakeup(void)
{
	psram_retention_scan_result_t scan_result;
	uint32_t crc_after;
	uint32_t crc_before;
	uint32_t crc_pass;
	uint32_t pattern_pass;

	crc_before = s_psram_crc_before;
	crc_after = psram_retention_crc32(
		(volatile const uint32_t *)s_psram_retention_data,
		PSRAM_RETENTION_TEST_WORDS);
	psram_retention_scan_incrementing_pattern(
		(volatile const uint32_t *)s_psram_retention_data,
		PSRAM_RETENTION_TEST_WORDS,
		&scan_result);
	s_psram_retention_cycle++;
	crc_pass = (crc_after == crc_before);
	pattern_pass = (scan_result.mismatch_count == 0U);

	if (crc_pass != 0U) {
		BK_LOGI(TAG,
			"PSRAM_RETENTION PASS cycle=%u addr=%p size=%u "
			"crc_before=0x%08x crc_after=0x%08x\r\n",
			s_psram_retention_cycle,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			crc_before,
			crc_after);
	} else {
		BK_LOGE(TAG,
			"PSRAM_RETENTION FAIL cycle=%u addr=%p size=%u "
			"crc_before=0x%08x crc_after=0x%08x\r\n",
			s_psram_retention_cycle,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			crc_before,
			crc_after);
	}

	if (pattern_pass != 0U) {
		BK_LOGI(TAG,
			"PSRAM_INCREMENT_PATTERN PASS cycle=%u addr=%p size=%u "
			"words=%u\r\n",
			s_psram_retention_cycle,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			PSRAM_RETENTION_TEST_WORDS);
	} else {
		uint32_t shown = scan_result.mismatch_count;

		if (shown > PSRAM_RETENTION_MAX_MISMATCH_LOGS) {
			shown = PSRAM_RETENTION_MAX_MISMATCH_LOGS;
		}
		BK_LOGE(TAG,
			"PSRAM_INCREMENT_PATTERN FAIL cycle=%u addr=%p size=%u "
			"words=%u mismatches=%u\r\n",
			s_psram_retention_cycle,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			PSRAM_RETENTION_TEST_WORDS,
			scan_result.mismatch_count);
		for (uint32_t i = 0; i < shown; i++) {
			uint32_t word_index = scan_result.logged_index[i];
			uint32_t actual = scan_result.logged_actual[i];

			BK_LOGE(TAG,
				"PSRAM_INCREMENT_MISMATCH cycle=%u index=%u addr=%p word=%u "
				"expected=0x%08x actual=0x%08x xor=0x%08x\r\n",
				s_psram_retention_cycle,
				i,
				(void *)&s_psram_retention_data[word_index],
				word_index,
				word_index,
				actual,
				word_index ^ actual);
			if (i < PSRAM_RETENTION_MAX_REREAD_LOGS) {
				psram_retention_reread_mismatch(
					s_psram_retention_cycle, word_index, actual);
			}
		}
		BK_LOGE(TAG,
			"PSRAM_INCREMENT_MISMATCH summary cycle=%u total_words=%u "
			"shown=%u first_addr=%p last_addr=%p omitted=%u\r\n",
			s_psram_retention_cycle,
			scan_result.mismatch_count,
			shown,
			(void *)&s_psram_retention_data[scan_result.first_index],
			(void *)&s_psram_retention_data[scan_result.last_index],
			scan_result.mismatch_count - shown);
	}

	if (crc_pass == pattern_pass) {
		BK_LOGI(TAG,
			"PSRAM_RETENTION_COMPARE PASS cycle=%u result=%s "
			"crc_pass=%u pattern_pass=%u\r\n",
			s_psram_retention_cycle,
			(crc_pass != 0U) ? "DATA_OK" : "DATA_CORRUPTED",
			crc_pass,
			pattern_pass);
	} else {
		BK_LOGE(TAG,
			"PSRAM_RETENTION_COMPARE FAIL cycle=%u crc_pass=%u "
			"pattern_pass=%u; independent scans disagree, "
			"PSRAM reads may be unstable\r\n",
			s_psram_retention_cycle,
			crc_pass,
			pattern_pass);
	}

	if ((crc_pass == 0U) || (pattern_pass == 0U)) {
		psram_retention_rearm_pattern(
			s_psram_retention_cycle, crc_before);
	}
}

static const pm_ap_power_ops_t s_psram_retention_pm_ops = {
	.name = "psram_retention_test",
	.quiesce = psram_retention_quiesce,
	.resume = psram_retention_resume,
	.app_resume = psram_retention_app_resume,
	.priority = PM_AP_POWER_PRIORITY_APPLICATION,
};
#endif /* CONFIG_PM_AP_PSRAM_RETENTION_CHECK */

static void ap_resume_test_task(void *arg)
{
	uint32_t sequence = 0;
	uint32_t stack_canary = 0x12345678U;
	uint32_t *heap_canary = os_malloc(sizeof(*heap_canary));
#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
	bk_err_t ret;
#endif

	(void)arg;
	if (heap_canary != NULL) {
		*heap_canary = 0xA55A5AA5U;
	}
#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
	if (s_psram_test_initialized == 0U) {
		for (uint32_t i = 0; i < PSRAM_RETENTION_TEST_WORDS; i++) {
			s_psram_retention_data[i] = i;
		}
		s_psram_test_initialized = 1U;
		BK_LOGI(TAG,
			"PSRAM_RETENTION init addr=%p size=%u words=%u\r\n",
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			PSRAM_RETENTION_TEST_WORDS);
		s_psram_crc_before = psram_retention_crc32(
			(volatile const uint32_t *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_WORDS);
		s_psram_crc_ready = 1U;
		BK_LOGI(TAG, "PSRAM_RETENTION baseline_crc=0x%08x\r\n",
			s_psram_crc_before);
	}
	ret = bk_pm_ap_power_ops_register(&s_psram_retention_pm_ops);
	if (ret != BK_OK) {
		BK_LOGE(TAG,
			"PSRAM_RETENTION register PM callbacks failed: %d\r\n",
			ret);
	}
#endif

	/*
	 * Print once at INFO so retention canaries are visible after cold boot.
	 * Keep the 2s proof at DEBUG: INFO on the shared UART collides with
	 * CP's power-on callback window and inflates AP_TIME startup_success.
	 */
	sequence++;
#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
	BK_LOGI(TAG,
		"RESUME_PROOF seq=%u stack=0x%08x heap=%p heap_value=0x%08x "
		"psram=%p psram_size=%u main_entries=%u core=%u\r\n",
		sequence,
		stack_canary,
		heap_canary,
		(heap_canary != NULL) ? *heap_canary : 0U,
		(void *)s_psram_retention_data,
		PSRAM_RETENTION_TEST_SIZE,
		s_ap_main_entry_count,
		rtos_get_core_id());
#else
	BK_LOGI(TAG,
		"RESUME_PROOF seq=%u stack=0x%08x heap=%p heap_value=0x%08x "
		"main_entries=%u core=%u\r\n",
		sequence,
		stack_canary,
		heap_canary,
		(heap_canary != NULL) ? *heap_canary : 0U,
		s_ap_main_entry_count,
		rtos_get_core_id());
#endif

	for (;;) {
		rtos_delay_milliseconds(2000);
#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
		if (s_psram_check_pending != 0U) {
			s_psram_check_pending = 0U;
			psram_retention_verify_after_wakeup();
		}
#endif
		sequence++;
#if CONFIG_PM_AP_PSRAM_RETENTION_CHECK
		BK_LOGD(TAG,
			"RESUME_PROOF seq=%u stack=0x%08x heap=%p heap_value=0x%08x "
			"psram=%p psram_size=%u main_entries=%u core=%u\r\n",
			sequence,
			stack_canary,
			heap_canary,
			(heap_canary != NULL) ? *heap_canary : 0U,
			(void *)s_psram_retention_data,
			PSRAM_RETENTION_TEST_SIZE,
			s_ap_main_entry_count,
			rtos_get_core_id());
#else
		BK_LOGD(TAG,
			"RESUME_PROOF seq=%u stack=0x%08x heap=%p heap_value=0x%08x "
			"main_entries=%u core=%u\r\n",
			sequence,
			stack_canary,
			heap_canary,
			(heap_canary != NULL) ? *heap_canary : 0U,
			s_ap_main_entry_count,
			rtos_get_core_id());
#endif
	}
}

int main(void)
{
	bk_err_t ret;

	bk_init();
	s_ap_main_entry_count++;
	BK_LOGI(TAG, "AP main entry count=%u\r\n", s_ap_main_entry_count);

	ret = rtos_create_thread(&s_resume_test_thread,
		BEKEN_DEFAULT_WORKER_PRIORITY,
		"ap_resume_test",
		ap_resume_test_task,
		2048,
		NULL);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "create resume test task failed: %d\r\n", ret);
	}

	return 0;
}
