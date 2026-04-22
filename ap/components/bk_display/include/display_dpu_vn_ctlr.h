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

#include <os/os.h>
#include <components/bk_display_types.h>
#include <components/bk_display_dpu_ctlr.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DISP_STATE_DEINIT,
    DISP_STATE_READY,
    DISP_STATE_CLOSED,
    DISP_STATE_DEINITING,
} display_state_t;

typedef struct
{
    display_state_t state;
    beken_mutex_t lock;
    beken_semaphore_t flush_idle_sem;
    uint32_t inflight_flush;
    dpu_handle_t dpu_handle;
    bk_display_dpu_config_t config;
    bk_display_ctlr_t ops;
} dpu_vn_ctlr_t;

#ifdef __cplusplus
}
#endif

