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
#include <driver/hpdma_types.h>

#define HPDMA_TAG "hpdma"
#define HPDMA_LOGI(...) BK_LOGI(HPDMA_TAG, ##__VA_ARGS__)
#define HPDMA_LOGW(...) BK_LOGW(HPDMA_TAG, ##__VA_ARGS__)
#define HPDMA_LOGE(...) BK_LOGE(HPDMA_TAG, ##__VA_ARGS__)
#define HPDMA_LOGD(...) BK_LOGD(HPDMA_TAG, ##__VA_ARGS__)
#define HPDMA_LOGV(...)

typedef struct
{
	u32		chnl_bitmap;
	u32		chnl_user[HPDMA_ID_MAX];
} hpdma_chnl_pool_t;


