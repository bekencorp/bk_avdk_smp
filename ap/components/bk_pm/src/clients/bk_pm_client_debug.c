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

#include <stdarg.h>
#include <stdio.h>
#include <driver/uart.h>
#include "bk_uart.h"
#include "aspl_lock.h"
#include "pm_uart_debug.h"

/*
 * Standalone UART5 log interface, guarded by CONFIG_PM_AP_UART_DEBUG
 * (default off, see bk_pm Kconfig).
 */
#if CONFIG_PM_AP_UART_DEBUG

#define BK_LOG_UART_LOCK()   uint32_t flags = bk_aspl_uart_log_enter_critical()
#define BK_LOG_UART_UNLOCK() bk_aspl_uart_log_exit_critical(flags)

#ifndef CONFIG_PRINTF_BUF_SIZE
#define CONFIG_PRINTF_BUF_SIZE    (136)
#endif

/*
 * AP<->CP functional communication keeps using the mailbox; this path only
 * writes text bytes to the dedicated UART5, so diagnostics that must not
 * depend on the mailbox log pipeline (e.g. the AP power-off prepare probe)
 * are still visible on the board's debug port.
 * CONFIG_UART5=y
 * CONFIG_UART5_TX_PIN=65
 * CONFIG_UART5_RX_PIN=64
 * CONFIG_USR_GPIO_CFG_EN=y
 */
#define BK_LOG_UART5_ID     UART_ID_5
static uint8_t s_uart5_log_inited = 0;

bk_err_t bk_pm_log_uart_init(void)
{
	const uart_config_t config =
	{
		.baud_rate = UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_NONE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_FLOWCTRL_DISABLE,
		.src_clk = UART_SCLK_APLL,
	};
	bk_err_t ret;

	if (s_uart5_log_inited) {
		return BK_OK;
	}

	ret = bk_uart_init(BK_LOG_UART5_ID, &config);
	if (ret == BK_OK) {
		s_uart5_log_inited = 1;
	}
	return ret;
}

void bk_pm_log_uart_print(const char *fmt, ...)
{
	char string[CONFIG_PRINTF_BUF_SIZE];
	va_list args;

	if (!s_uart5_log_inited) {
		return;
	}

	va_start(args, fmt);
	vsnprintf(string, sizeof(string), fmt, args);
	va_end(args);
	string[CONFIG_PRINTF_BUF_SIZE - 1] = 0;

	BK_LOG_UART_LOCK();
	uart_write_string(BK_LOG_UART5_ID, string);
	BK_LOG_UART_UNLOCK();
}

#endif /* CONFIG_PM_AP_UART_DEBUG */
