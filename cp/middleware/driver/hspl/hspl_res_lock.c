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
#include <os/os.h>
#if CONFIG_AON_RTC
#include <driver/aon_rtc.h>
#endif

#define HSPL_MAX_CORES        2
#define HSPL_REC_COUNT_MAX     255

/* Recursive lock count: [res][core_id]. Same core locking again only increments count. */
static uint8_t s_rec_count[BK_HSPL_RES_MAX][HSPL_MAX_CORES];

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

/*
 * Resource mapping:
 * - Resources 0-15:  use HSPL_0 (BK_HSPL_ID_0) channels 0-15
 * - Resources 16-31: use HSPL_1 (BK_HSPL_ID_1) channels 0-15 (CP not supported)
 */
static inline void hspl_res_get_map_internal(bk_hspl_res_t res, uint8_t *hspl_id, uint8_t *channel)
{
	if (res < 16) {
		/* Resources 0-15: HSPL_0, channels 0-15 */
		*hspl_id = BK_HSPL_ID_0;
		*channel = (uint8_t)res;
	} else {
		/* Resources 16-31: HSPL_1, channels 0-15 (CP not supported) */
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
	if (hspl_id == BK_HSPL_ID_1) {
		/* HSPL_ID_1 is reserved for AP SMP (CPU2/CPU3), CP not supported */
		return BK_ERR_NOT_SUPPORT;
	}

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] > 0) {
		if (s_rec_count[res][core_id] >= HSPL_REC_COUNT_MAX) {
			rtos_enable_int(flags);
			return BK_ERR_NOT_SUPPORT;
		}
		s_rec_count[res][core_id]++;
		rtos_enable_int(flags);
		return BK_OK;
	}
	if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
		s_rec_count[res][core_id] = 1;
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
		if (bk_hspl_try_lock(hspl_id, channel, NULL) == BK_OK) {
			flags = rtos_disable_int();
			s_rec_count[res][core_id] = 1;
			rtos_enable_int(flags);
			return BK_OK;
		}

		if (timeout_us == 0) {
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
	if (hspl_id == BK_HSPL_ID_1) {
		return BK_ERR_NOT_SUPPORT;
	}

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] == 0) {
		rtos_enable_int(flags);
		BK_ASSERT(0);
		return BK_ERR_PARAM;
	}
	s_rec_count[res][core_id]--;
	if (s_rec_count[res][core_id] == 0) {
		ret = bk_hspl_unlock(hspl_id, channel);
	}
	rtos_enable_int(flags);
	return ret;
}

bk_err_t bk_hspl_res_must_lock(bk_hspl_res_t res)
{
	uint8_t hspl_id, channel;
	uint8_t core_id;
	uint32_t flags;

	if (res >= BK_HSPL_RES_MAX) {
		BK_ASSERT(0);
		return BK_ERR_PARAM;
	}

	hspl_res_get_map_internal(res, &hspl_id, &channel);
	if (hspl_id == BK_HSPL_ID_1) {
		return BK_ERR_NOT_SUPPORT;
	}

	core_id = hspl_core_index();
	flags = rtos_disable_int();
	if (s_rec_count[res][core_id] > 0) {
		if (s_rec_count[res][core_id] >= HSPL_REC_COUNT_MAX) {
			rtos_enable_int(flags);
			return BK_ERR_NOT_SUPPORT;
		}
		s_rec_count[res][core_id]++;
		rtos_enable_int(flags);
		return BK_OK;
	}
	rtos_enable_int(flags);

	/* Busy-wait until lock is acquired */
	while (bk_hspl_try_lock(hspl_id, channel, NULL) != BK_OK) {
		/* Spin until lock is acquired */
	}

	flags = rtos_disable_int();
	s_rec_count[res][core_id] = 1;
	rtos_enable_int(flags);
	return BK_OK;
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
	if (hspl_id == BK_HSPL_ID_1) {
		return BK_ERR_NOT_SUPPORT;
	}
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

uint32_t bk_hspl_driver_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	bk_hspl_res_must_lock(BK_HSPL_RES_SYS);
	return flags;
}

void bk_hspl_driver_exit_critical(uint32_t flags)
{
	bk_hspl_res_unlock(BK_HSPL_RES_SYS);
	rtos_enable_int(flags);
}

void bk_hspl_uart_log_lock(void)
{
	bk_hspl_res_must_lock(BK_HSPL_RES_UART_LOG);
}

void bk_hspl_uart_log_unlock(void)
{
	bk_hspl_res_unlock(BK_HSPL_RES_UART_LOG);
}

uint32_t bk_hspl_uart_log_enter_critical(void)
{
	uint32_t flags = rtos_disable_int();
	bk_hspl_res_must_lock(BK_HSPL_RES_UART_LOG);
	return flags;
}

void bk_hspl_uart_log_exit_critical(uint32_t flags)
{
	bk_hspl_res_unlock(BK_HSPL_RES_UART_LOG);
	rtos_enable_int(flags);
}

void bk_hspl_sys_sw_regs_lock(void)
{
	bk_hspl_res_must_lock(BK_HSPL_RES_SYS_SW_REGS);
}

void bk_hspl_sys_sw_regs_unlock(void)
{
	bk_hspl_res_unlock(BK_HSPL_RES_SYS_SW_REGS);
}
