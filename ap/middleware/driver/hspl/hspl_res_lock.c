// Copyright 2020-2026 Beken
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

#include "hspl_res_lock.h"
#include "hspl_driver.h"
#include "sys_sw_regs.h"
#include <common/bk_assert.h>

#include <os/os.h>
#include "arch_interrupt.h"

#if CONFIG_AON_RTC
#include <driver/aon_rtc.h>
#endif

#define HSPL_MAX_CORES        2
#define HSPL_REC_COUNT_MAX     255
#define HSPL_RES3_TRACE_MAGIC  0x48533354U /* "HS3T" */
#define HSPL_RES3_TRACE_VERSION 1U

/* Recursive lock count: [res][core_id]. Same core locking again only increments count. */
static volatile uint8_t s_rec_count[BK_HSPL_RES_MAX][HSPL_MAX_CORES];

#if CONFIG_HSPL_LEAK_DEBUG
static volatile bk_hspl_res3_trace_buffer_t s_hspl_res3_trace = {
	.magic = HSPL_RES3_TRACE_MAGIC,
	.version = HSPL_RES3_TRACE_VERSION,
	.entry_size = sizeof(bk_hspl_res3_trace_entry_t),
	.depth = BK_HSPL_RES3_TRACE_DEPTH,
};
#endif

static inline uint8_t hspl_core_index(void)
{
	uint32_t id = portGET_CORE_ID();
	BK_ASSERT(id < HSPL_MAX_CORES);
	return (uint8_t)id;
}

static inline uint32_t hspl_get_time_ms(void)
{
#if CONFIG_AON_RTC
	return bk_aon_rtc_get_ms();
#else
	return rtos_get_time();
#endif
}

static inline void hspl_sync_barrier(void)
{
	__asm volatile ("dsb" ::: "memory");
}

#if CONFIG_HSPL_LEAK_DEBUG
static inline uint32_t hspl_res3_trace_core(void)
{
	uint32_t id = portGET_CORE_ID();

	return (id < HSPL_MAX_CORES) ? id : 0xFFU;
}

static void hspl_res3_trace_register_dump(void)
{
	if (s_hspl_res3_trace.registered != 0U) {
		return;
	}

	s_hspl_res3_trace.registered = 1U;
	bk_sys_sw_regs_update_ap_extra_dump(BK_SYS_SW_REGS_AP_EXTRA_DUMP_MAX - 1U,
		(uint32_t)(uintptr_t)&s_hspl_res3_trace, sizeof(s_hspl_res3_trace));
}

void bk_hspl_res3_trace_record(bk_hspl_res_t res, bk_hspl_res3_trace_action_t action,
	uint32_t pc, uint32_t ret)
{
	uint32_t flags;
	uint32_t index;
	uint32_t slot;
	uint32_t core;
	volatile bk_hspl_res3_trace_entry_t *entry;

	if (res != BK_HSPL_RES_SYS) {
		return;
	}

	hspl_res3_trace_register_dump();

	core = hspl_res3_trace_core();
	flags = rtos_disable_int();
	index = s_hspl_res3_trace.write_index;
	s_hspl_res3_trace.write_index = index + 1U;
	if (index >= BK_HSPL_RES3_TRACE_DEPTH) {
		s_hspl_res3_trace.wrapped = 1U;
	}

	slot = index % BK_HSPL_RES3_TRACE_DEPTH;
	entry = &s_hspl_res3_trace.entries[slot];
	entry->seq = index + 1U;
	entry->tick_ms = hspl_get_time_ms();
	entry->action = (uint32_t)action;
	entry->core = core;
	entry->pc = pc;
	entry->rec_before = (core < HSPL_MAX_CORES) ? s_rec_count[res][core] : 0xFFFFFFFFU;
	entry->ret = ret;
	hspl_sync_barrier();
	entry->rec_after = (core < HSPL_MAX_CORES) ? s_rec_count[res][core] : 0xFFFFFFFFU;
	rtos_enable_int(flags);
}

const volatile bk_hspl_res3_trace_buffer_t *bk_hspl_res3_trace_get(void)
{
	return &s_hspl_res3_trace;
}

void bk_hspl_res3_trace_clear(void)
{
	uint32_t flags = rtos_disable_int();

	for (uint32_t i = 0; i < BK_HSPL_RES3_TRACE_DEPTH; i++) {
		s_hspl_res3_trace.entries[i].seq = 0U;
		s_hspl_res3_trace.entries[i].tick_ms = 0U;
		s_hspl_res3_trace.entries[i].action = 0U;
		s_hspl_res3_trace.entries[i].core = 0U;
		s_hspl_res3_trace.entries[i].pc = 0U;
		s_hspl_res3_trace.entries[i].rec_before = 0U;
		s_hspl_res3_trace.entries[i].rec_after = 0U;
		s_hspl_res3_trace.entries[i].ret = 0U;
	}
	s_hspl_res3_trace.write_index = 0U;
	s_hspl_res3_trace.wrapped = 0U;
	s_hspl_res3_trace.magic = HSPL_RES3_TRACE_MAGIC;
	s_hspl_res3_trace.version = HSPL_RES3_TRACE_VERSION;
	s_hspl_res3_trace.entry_size = sizeof(bk_hspl_res3_trace_entry_t);
	s_hspl_res3_trace.depth = BK_HSPL_RES3_TRACE_DEPTH;
	rtos_enable_int(flags);

	hspl_res3_trace_register_dump();
}
#endif

/*
 * Resource mapping:
 * - Resources 0-15:  use HSPL_0 (BK_HSPL_ID_0) channels 0-15
 * - Resources 16-31: use HSPL_1 (BK_HSPL_ID_1) channels 0-15
 */
static inline void hspl_res_get_map_internal(bk_hspl_res_t res, uint8_t *hspl_id, uint8_t *channel)
{
	if (res < 16) {
		/* Resources 0-15: HSPL_0, channels 0-15 */
		*hspl_id = BK_HSPL_ID_0;
		*channel = (uint8_t)res;
	} else {
		/* Resources 16-31: HSPL_1, channels 0-15 */
		*hspl_id = BK_HSPL_ID_1;
		*channel = (uint8_t)(res - 16);
	}
}

bk_err_t bk_hspl_res_get_map(bk_hspl_res_t res, uint8_t *hspl_id, uint8_t *channel)
{
	if (res >= BK_HSPL_RES_MAX) {
		return BK_ERR_PARAM;
	}

	if (!hspl_id || !channel) {
		return BK_ERR_PARAM;
	}

	hspl_res_get_map_internal(res, hspl_id, channel);
	return BK_OK;
}

bk_err_t bk_hspl_res_lock(bk_hspl_res_t res, uint32_t timeout_us)
{
	uint8_t hspl_id, channel;
	uint32_t timeout_ms = 0;
	uint32_t start_ms = 0;
	bool use_timeout = false;
	uint8_t core_id;
	uint32_t flags;

	if (res >= BK_HSPL_RES_MAX) {
		return BK_ERR_PARAM;
	}

	hspl_res_get_map_internal(res, &hspl_id, &channel);

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] > 0) {
		if (s_rec_count[res][core_id] >= HSPL_REC_COUNT_MAX) {
			rtos_enable_int(flags);
			return BK_ERR_NOT_SUPPORT;
		}
		s_rec_count[res][core_id]++;
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_RECUR_ENTER, 0U, BK_OK);
#endif
		rtos_enable_int(flags);
		return BK_OK;
	}
	if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
		hspl_sync_barrier();
		s_rec_count[res][core_id] = 1;
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_ENTER_GOT, 0U, BK_OK);
#endif
		rtos_enable_int(flags);
		return BK_OK;
	}
	rtos_enable_int(flags);

	if (rtos_is_in_interrupt_context()) {
		/* ISR context: only try once, no blocking */
		return BK_ERR_TIMEOUT;
	}

	if (timeout_us == BK_HSPL_WAIT_FOREVER) {
		use_timeout = false;
	} else if (timeout_us) {
		timeout_ms = (timeout_us + 999) / 1000;
		if (timeout_ms == 0) {
			timeout_ms = 1;
		}
		start_ms = hspl_get_time_ms();
		use_timeout = true;
	}

	while (1) {
		/* Acquire and rec_count update must be atomic against local interrupts:
		 * otherwise an ISR that re-enters the same resource in the gap sees
		 * rec_count==0, re-issues a (self-owned) HW try_lock that the hardware
		 * rejects, and either dead-spins or proceeds without protection. */
		flags = rtos_disable_int();
		if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
			hspl_sync_barrier();
			s_rec_count[res][core_id] = 1;
#if CONFIG_HSPL_LEAK_DEBUG
			bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_ENTER_GOT, 0U, BK_OK);
#endif
			rtos_enable_int(flags);
			return BK_OK;
		}
		rtos_enable_int(flags);

		if (timeout_us == 0) {
			return BK_ERR_TIMEOUT;
		}

		/* A stopped peer core may hold this lock forever during coredump; do not
		 * block the dump/reboot flow. */
		if (arch_is_enter_exception()) {
			return BK_ERR_TIMEOUT;
		}

		if (use_timeout) {
			uint32_t current_ms = hspl_get_time_ms();
			uint32_t elapsed_ms;

			/* Handle time overflow (wrap-around) */
			if (current_ms >= start_ms) {
				elapsed_ms = current_ms - start_ms;
			} else {
				elapsed_ms = timeout_ms;
			}

			if (elapsed_ms >= timeout_ms) {
				return BK_ERR_TIMEOUT;
			}
		}

		rtos_delay_milliseconds(1);
	}
}

bk_err_t bk_hspl_res_try_lock(bk_hspl_res_t res)
{
	return bk_hspl_res_lock(res, 0);
}

bk_err_t bk_hspl_res_unlock(bk_hspl_res_t res)
{
	uint8_t hspl_id, channel;
	uint8_t core_id;
	uint32_t flags;
	bk_err_t ret = BK_OK;

	if (res >= BK_HSPL_RES_MAX) {
		BK_ASSERT(0);
		return BK_ERR_PARAM;
	}

	hspl_res_get_map_internal(res, &hspl_id, &channel);

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] == 0) {
		rtos_enable_int(flags);
		BK_ASSERT(0);
		return BK_ERR_PARAM;
	}
	s_rec_count[res][core_id]--;
	if (s_rec_count[res][core_id] == 0) {
		hspl_sync_barrier();
		ret = bk_hspl_unlock(hspl_id, channel);
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_EXIT_DONE, 0U, (uint32_t)ret);
#endif
	} else {
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_RECUR_EXIT, 0U, BK_OK);
#endif
	}
	rtos_enable_int(flags);
	return ret;
}

static inline uint32_t hspl_res_must_lock_timeout_ms(bk_hspl_res_t res)
{
	if (res == BK_HSPL_RES_FLASH) {
		return (uint32_t)CONFIG_HSPL_MUST_LOCK_TIMEOUT_MS_FLASH;
	}

	return (uint32_t)CONFIG_HSPL_MUST_LOCK_TIMEOUT_MS_DEFAULT;
}

static void hspl_res_must_lock_assert_timeout(bk_hspl_res_t res, uint32_t timeout_ms)
{
	/* In exception/coredump context, do NOT assert: a peer core may have been
	 * stopped while holding this lock and can never release it, so asserting
	 * here would only trigger a secondary exception. Give up hardware mutual
	 * exclusion (interrupts are already disabled and other cores stopped) and
	 * let the dump/reboot flow continue. */
	if (arch_is_enter_exception()) {
		return;
	}
	BK_ASSERT_EX(0, "HSPL res %u must_lock timeout %ums\r\n", (unsigned int)res, timeout_ms);
}

static inline uint32_t hspl_must_lock_elapsed_ms(uint32_t start_ms, uint32_t now_ms, uint32_t cap_ms)
{
	if (now_ms >= start_ms) {
		return now_ms - start_ms;
	}

	return cap_ms;
}

bk_err_t bk_hspl_res_must_lock(bk_hspl_res_t res)
{
	uint8_t hspl_id, channel;
	uint8_t core_id;
	uint32_t flags;
	uint32_t timeout_ms;
	uint32_t start_ms;
	bool use_timeout;

	if (res >= BK_HSPL_RES_MAX) {
		BK_ASSERT(0);
		return BK_ERR_PARAM;
	}

	hspl_res_get_map_internal(res, &hspl_id, &channel);

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] > 0) {
		if (s_rec_count[res][core_id] >= HSPL_REC_COUNT_MAX) {
			rtos_enable_int(flags);
			return BK_ERR_NOT_SUPPORT;
		}
		s_rec_count[res][core_id]++;
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_RECUR_ENTER, 0U, BK_OK);
#endif
		rtos_enable_int(flags);
		return BK_OK;
	}
	/* First attempt while local IRQ is still disabled, so the HW acquire and the
	 * rec_count update are atomic against same-core ISR re-entry of this resource. */
	if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
		hspl_sync_barrier();
		s_rec_count[res][core_id] = 1;
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_ENTER_GOT, 0U, BK_OK);
#endif
		rtos_enable_int(flags);
		return BK_OK;
	}
	rtos_enable_int(flags);

	timeout_ms = hspl_res_must_lock_timeout_ms(res);
	use_timeout = (timeout_ms != 0U);
	start_ms = hspl_get_time_ms();

	while (1) {
		/* Never block the dump/reboot flow on a stopped lock-holder. */
		if (arch_is_enter_exception()) {
			return BK_ERR_TIMEOUT;
		}

		flags = rtos_disable_int();
		if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
			hspl_sync_barrier();
			s_rec_count[res][core_id] = 1;
#if CONFIG_HSPL_LEAK_DEBUG
			bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_ENTER_GOT, 0U, BK_OK);
#endif
			rtos_enable_int(flags);
			return BK_OK;
		}
		rtos_enable_int(flags);

		if (use_timeout) {
			uint32_t now_ms = hspl_get_time_ms();

			if (hspl_must_lock_elapsed_ms(start_ms, now_ms, timeout_ms) >= timeout_ms) {
#if CONFIG_HSPL_LEAK_DEBUG
				bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_TIMEOUT, 0U, BK_ERR_TIMEOUT);
#endif
				hspl_res_must_lock_assert_timeout(res, timeout_ms);
				return BK_ERR_TIMEOUT;
			}
		}
	}
}

void bk_hspl_res_dbg_set_owner(bk_hspl_res_t res, uint8_t core, uint32_t pc)
{
#if CONFIG_HSPL_LEAK_DEBUG
	uint8_t core_id;
	uint32_t flags;

	if ((res >= BK_HSPL_RES_MAX) || (pc == 0U)) {
		return;
	}

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] == 1U) {
		bk_sys_sw_regs_set_hspl_owner((uint8_t)res, core, pc);
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_OWNER_SET, pc, BK_OK);
#endif
	}
	rtos_enable_int(flags);
#else
	(void)res;
	(void)core;
	(void)pc;
#endif
}

void bk_hspl_res_dbg_clear_owner(bk_hspl_res_t res)
{
#if CONFIG_HSPL_LEAK_DEBUG
	uint8_t core_id;
	uint32_t flags;

	if (res >= BK_HSPL_RES_MAX) {
		return;
	}

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] == 1U) {
#if CONFIG_HSPL_LEAK_DEBUG
		bk_hspl_res3_trace_record(res, BK_HSPL_RES3_TRACE_OWNER_CLEAR, 0U, BK_OK);
#endif
		bk_sys_sw_regs_clear_hspl_owner((uint8_t)res);
	}
	rtos_enable_int(flags);
#else
	(void)res;
#endif
}

bk_err_t bk_hspl_res_lock_irqsave(bk_hspl_res_t res, uint32_t *flags)
{
	uint8_t hspl_id, channel;
	uint8_t core_id;

	if (!flags) {
		return BK_ERR_PARAM;
	}

	if (rtos_is_in_interrupt_context()) {
		return BK_ERR_PARAM;
	}

	if (res >= BK_HSPL_RES_MAX) {
		return BK_ERR_PARAM;
	}

	hspl_res_get_map_internal(res, &hspl_id, &channel);
	*flags = rtos_disable_int();
	core_id = hspl_core_index();

	if (s_rec_count[res][core_id] > 0) {
		if (s_rec_count[res][core_id] >= HSPL_REC_COUNT_MAX) {
			rtos_enable_int(*flags);
			return BK_ERR_NOT_SUPPORT;
		}
		s_rec_count[res][core_id]++;
		return BK_OK;
	}
	if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
		hspl_sync_barrier();
		s_rec_count[res][core_id] = 1;
		return BK_OK;
	}

	rtos_enable_int(*flags);
	return BK_ERR_TIMEOUT;
}

bk_err_t bk_hspl_res_unlock_irqrestore(bk_hspl_res_t res, uint32_t flags)
{
	bk_err_t ret = bk_hspl_res_unlock(res);
	rtos_enable_int(flags);
	return ret;
}


