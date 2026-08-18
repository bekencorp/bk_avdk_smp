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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "bk_tfm_log.h"
#include "tfm_hal_spm_logdev.h"

#define BK_TFM_LOG_BUF_SIZE 128

static void bk_tfm_voutput(const char *fmt, va_list ap)
{
	char buf[BK_TFM_LOG_BUF_SIZE];
	int len;

	len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	if (len <= 0) {
		return;
	}
	if (len > (int)sizeof(buf) - 1) {
		len = (int)sizeof(buf) - 1;
	}
	buf[len] = '\0';
	(void)tfm_hal_output_spm_log(buf, (uint32_t)len);
}

void bk_tfm_log_vtag(const char *tag, const char *fmt, va_list ap)
{
	if (tag && tag[0]) {
		(void)tfm_hal_output_spm_log(tag, (uint32_t)strlen(tag));
		(void)tfm_hal_output_spm_log(": ", 2);
	}
	bk_tfm_voutput(fmt, ap);
}

void bk_tfm_log_tag(const char *tag, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bk_tfm_log_vtag(tag, fmt, ap);
	va_end(ap);
}

/* Map the SDK log level (components/log.h: ERROR=1 .. VERBOSE=5) onto the
 * per-domain TFM level gate so logs emitted through the SDK backend honour the
 * same control as the native BK_LOG* macros. */
int bk_tfm_sdk_log_enabled(int sdk_level)
{
	switch (sdk_level) {
	case 1: /* ERROR */
		return (BK_TFM_LVL_CUR >= BK_TFM_LVL_ERR);
	case 2: /* WARNING */
		return (BK_TFM_LVL_CUR >= BK_TFM_LVL_WRN);
	case 4: /* DEBUG */
	case 5: /* VERBOSE */
		return (BK_TFM_LVL_CUR >= BK_TFM_LVL_DBG);
	case 3: /* INFO */
	default:
		return (BK_TFM_LVL_CUR >= BK_TFM_LVL_INF);
	}
}

void bk_tfm_log_raw(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bk_tfm_voutput(fmt, ap);
	va_end(ap);
}

void bk_tfm_force_log(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bk_tfm_voutput(fmt, ap);
	va_end(ap);
}

#if CONFIG_BK_TFM_DUMP_BUF
void bk_tfm_dump_buf(const char *str, const uint8_t *buf, uint32_t len)
{
	bk_tfm_log_raw("%s\r\n", str);
	for (uint32_t i = 0; i < len; i++) {
		if (i && (i % 32) == 0) {
			bk_tfm_log_raw("\r\n");
		}
		bk_tfm_log_raw("%02x ", buf[i]);
	}
	bk_tfm_log_raw("\r\n");
}
#endif
