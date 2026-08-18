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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <common/bk_err.h>

/* Kept identical to the SoC definition so a later soc_impl.h include is a
 * benign (identical) redefinition rather than a conflict. */
#ifndef BIT
#define BIT(i) (1 << (i))
#endif

/* Independent TFM/BL2 log interface. It does not depend on the SDK
 * components/log.h nor CONFIG_LOG_LEVEL. The output sink is the SPM log HAL
 * (tfm_hal_output_spm_log -> UART). Level control is split by domain:
 *   - secure runtime (SPE) : TFM_SPM_LOG_LEVEL
 *   - bootloader     (BL2) : MCUBOOT_LOG_LEVEL
 * BK_LOG_FORCE always outputs regardless of the level. */

/* BK_TFM_BL2_LOG is injected only on the bootloader target (platform_bl2). The
 * generic BL2 macro is defined image-wide whenever MCUboot is enabled, so it
 * cannot tell the bootloader image apart from the secure runtime. */
#ifdef BK_TFM_BL2_LOG
#include "mcuboot_config/mcuboot_config.h"
#include "mcuboot_config/mcuboot_logging.h"
#define BK_TFM_LVL_ERR   MCUBOOT_LOG_LEVEL_ERROR
#define BK_TFM_LVL_WRN   MCUBOOT_LOG_LEVEL_WARNING
#define BK_TFM_LVL_INF   MCUBOOT_LOG_LEVEL_INFO
#define BK_TFM_LVL_DBG   MCUBOOT_LOG_LEVEL_DEBUG
#define BK_TFM_LVL_CUR   MCUBOOT_LOG_LEVEL
#else
#ifndef TFM_SPM_LOG_LEVEL
/* Fallback for translation units built without the SPM level definition. */
#define TFM_SPM_LOG_LEVEL 2
#endif
#include "tfm_spm_log.h"
#define BK_TFM_LVL_ERR   TFM_SPM_LOG_LEVEL_ERROR
/* The SPM level set has no WARNING tier; map it onto INFO. */
#define BK_TFM_LVL_WRN   TFM_SPM_LOG_LEVEL_INFO
#define BK_TFM_LVL_INF   TFM_SPM_LOG_LEVEL_INFO
#define BK_TFM_LVL_DBG   TFM_SPM_LOG_LEVEL_DEBUG
#define BK_TFM_LVL_CUR   TFM_SPM_LOG_LEVEL
#endif

#ifdef __cplusplus
extern "C" {
#endif

void bk_tfm_log_tag(const char *tag, const char *fmt, ...);
void bk_tfm_log_vtag(const char *tag, const char *fmt, va_list ap);
void bk_tfm_log_raw(const char *fmt, ...);
void bk_tfm_force_log(const char *fmt, ...);

/* Bridge for the SDK log backend (bk_printf_ext): returns non-zero when a line
 * at the given SDK level should be emitted under the current TFM log level. */
int bk_tfm_sdk_log_enabled(int sdk_level);

#ifdef __cplusplus
}
#endif

#define BK_LOG_FORCE(fmt, ...)  bk_tfm_force_log(fmt, ##__VA_ARGS__)

#if (BK_TFM_LVL_CUR >= BK_TFM_LVL_ERR)
#define BK_LOGE(tag, fmt, ...)  bk_tfm_log_tag((tag), (fmt), ##__VA_ARGS__)
#else
#define BK_LOGE(tag, fmt, ...)  ((void)0)
#endif

#if (BK_TFM_LVL_CUR >= BK_TFM_LVL_WRN)
#define BK_LOGW(tag, fmt, ...)  bk_tfm_log_tag((tag), (fmt), ##__VA_ARGS__)
#else
#define BK_LOGW(tag, fmt, ...)  ((void)0)
#endif

#if (BK_TFM_LVL_CUR >= BK_TFM_LVL_INF)
#define BK_LOGI(tag, fmt, ...)  bk_tfm_log_tag((tag), (fmt), ##__VA_ARGS__)
#define BK_LOG_RAW(fmt, ...)    bk_tfm_log_raw((fmt), ##__VA_ARGS__)
#else
#define BK_LOGI(tag, fmt, ...)  ((void)0)
#define BK_LOG_RAW(fmt, ...)    ((void)0)
#endif

#if (BK_TFM_LVL_CUR >= BK_TFM_LVL_DBG)
#define BK_LOGD(tag, fmt, ...)  bk_tfm_log_tag((tag), (fmt), ##__VA_ARGS__)
#define BK_LOGV(tag, fmt, ...)  bk_tfm_log_tag((tag), (fmt), ##__VA_ARGS__)
#else
#define BK_LOGD(tag, fmt, ...)  ((void)0)
#define BK_LOGV(tag, fmt, ...)  ((void)0)
#endif

#if CONFIG_BK_TFM_DUMP_BUF
void bk_tfm_dump_buf(const char *str, const uint8_t *buf, uint32_t len);
#define BK_TFM_DUMP_BUF(prompt, buf, len) bk_tfm_dump_buf((prompt), (buf), (len))
#else
#define BK_TFM_DUMP_BUF(prompt, buf, len)
#endif
