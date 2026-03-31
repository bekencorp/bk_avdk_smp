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

#include <components/log.h>
#include "bk_wdt.h"

#define WWDT_TAG "wwdt"
#define WWDT_LOGI(...) BK_LOGI(WWDT_TAG, ##__VA_ARGS__)
#define WWDT_LOGW(...) BK_LOGW(WWDT_TAG, ##__VA_ARGS__)
#define WWDT_LOGE(...) BK_LOGE(WWDT_TAG, ##__VA_ARGS__)
#define WWDT_LOGD(...) BK_LOGD(WWDT_TAG, ##__VA_ARGS__)
#define WWDT_LOGV(...) BK_LOGV(WWDT_TAG, ##__VA_ARGS__)

#define WWDT_BARK_TIME_MS       (1000)

void bk_wwdt_feed_handle(void);
void bk_wwdt_close(void);
void bk_wwdt_force_feed(void);
void bk_wwdt_force_reboot(void);
// eof
