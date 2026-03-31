// Copyright 2020-2021 Beken
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

#include <components/log.h>

#define XDAC_TAG "xdac"
#define XDAC_LOGI(...) BK_LOGI(XDAC_TAG, ##__VA_ARGS__)
#define XDAC_LOGW(...) BK_LOGW(XDAC_TAG, ##__VA_ARGS__)
#define XDAC_LOGE(...) BK_LOGE(XDAC_TAG, ##__VA_ARGS__)
#define XDAC_LOGD(...) BK_LOGD(XDAC_TAG, ##__VA_ARGS__)
#define XDAC_LOGV(...) BK_LOGV(XDAC_TAG, ##__VA_ARGS__)

/**
 * @brief AUXDAC struct defines
 * @defgroup bk_api_xdac_structs in AUXDAC
 * @ingroup bk_api_xdac
 * @{
 */
