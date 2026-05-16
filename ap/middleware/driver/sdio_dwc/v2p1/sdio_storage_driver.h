// Copyright 2020-2024 Beken
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

#include <components/log.h>

#define SDIOD_TAG "sdio_dwc"

/*
 * Global debug-level log switch for the whole SDIO_DWC driver.
 *   0 - disable all SDIOD_LOGD prints (default, less noise)
 *   1 - enable SDIOD_LOGD prints for full debugging
 * Only the verbose D level is gated; SDIOD_LOGI/SDIOD_LOGW/SDIOD_LOGE
 * (key events, warnings and errors) are always kept.
 */
#ifndef CONFIG_SDIO_DWC_LOG_EN
#define CONFIG_SDIO_DWC_LOG_EN 0
#endif

#define SDIOD_LOGI(...) BK_LOGI(SDIOD_TAG, ##__VA_ARGS__)
#define SDIOD_LOGW(...) BK_LOGW(SDIOD_TAG, ##__VA_ARGS__)
#define SDIOD_LOGE(...) BK_LOGE(SDIOD_TAG, ##__VA_ARGS__)
#if CONFIG_SDIO_DWC_LOG_EN
#define SDIOD_LOGD(...) BK_LOGD(SDIOD_TAG, ##__VA_ARGS__)
#else
#define SDIOD_LOGD(...) do {} while (0)
#endif

// eof

