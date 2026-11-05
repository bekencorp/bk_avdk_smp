// Copyright 2026 Beken
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

#pragma once

#include <common/sys_config.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standalone UART5 log interface, independent of the mailbox log forwarding.
 * AP<->CP functional communication is not affected.
 *
 * The implementation lives in bk_pm/src/clients/bk_pm_client_debug.c and is
 * only compiled when CONFIG_PM_AP_UART_DEBUG is enabled; otherwise the
 * inline stubs below compile all call sites away.
 */
#if CONFIG_PM_AP_UART_DEBUG

bk_err_t bk_pm_log_uart_init(void);

void bk_pm_log_uart_print(const char *fmt, ...);

#else

static inline bk_err_t bk_pm_log_uart_init(void)
{
	return BK_OK;
}

static inline void bk_pm_log_uart_print(const char *fmt, ...)
{
	(void)fmt;
}

#endif /* CONFIG_PM_AP_UART_DEBUG */

#ifdef __cplusplus
}
#endif
