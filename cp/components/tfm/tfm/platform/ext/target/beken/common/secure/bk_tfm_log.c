// Copyright     2023-2028 Beken
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

/* BL2-only printf retarget. MCUboot core logs (MCUBOOT_LOG_*) call printf; route
 * it to the secure UART sink so the bootloader output shares the same path as
 * the rest of the unified log. In the secure runtime the beken code uses the
 * SPM log helpers directly, so no printf override is compiled there. */

#ifdef BK_TFM_BL2_LOG

#include <stdio.h>
#include <stdarg.h>
#include "uart_stdout.h"

#define BK_BL2_PRINTF_BUF_SIZE 128

int printf(const char *fmt, ...)
{
	char string[BK_BL2_PRINTF_BUF_SIZE] = {0};
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(string, sizeof(string) - 1, fmt, ap);
	va_end(ap);

	string[BK_BL2_PRINTF_BUF_SIZE - 1] = 0;
	if (len > BK_BL2_PRINTF_BUF_SIZE - 1) {
		len = BK_BL2_PRINTF_BUF_SIZE - 1;
	}

	stdio_output_string((const unsigned char *)string, len);
	return len;
}

int printf_dummy(const char *fmt, ...)
{
	(void)fmt;
	return 0;
}

#endif /* BK_TFM_BL2_LOG */
