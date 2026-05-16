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

/**
 * @file display_dpu_vn_ctlr.h
 * @brief Default DPU virtual node controller. Internal to bk_display;
 *        implements the public ::bk_display_ctlr_t op-set on top of
 *        the avdk_driver DPU handle.
 */

#include <os/os.h>
#include <components/bk_display.h>
#include <driver/dpu_types.h>
#include "bk_display_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Display controller lifecycle state.
 *  Transitions: DEINIT -> INITED -> ACTIVE -> CLOSED -> INITED -> ... -> DEINIT. */
typedef enum
{
    DISP_STATE_DEINIT     = 0,    /**< handle exists, no HW programmed */
    DISP_STATE_INITED     = 1,    /**< registers programmed, panel ON, no refresh */
    DISP_STATE_ACTIVE     = 2,    /**< accepts ::bk_display_flush() */
    DISP_STATE_CLOSED     = 3,    /**< quiesced; re-openable */
    DISP_STATE_DEINITING  = 4,    /**< transient: blocking new flushes during deinit */
} display_state_t;

/** Default DPU virtual node controller body. First member must remain
 *  the public ops table for the cast at the dispatcher boundary. */
typedef struct
{
    display_state_t state;
    beken_mutex_t lock;
    beken_semaphore_t flush_idle_sem;
    uint32_t inflight_flush;
    dpu_handle_t dpu_handle;
    bk_display_dpu_config_t config;
    bk_display_ctlr_t ops;
    bool flush_lazy_promoted;     /**< true once a flush implicitly armed the bus */
} dpu_vn_ctlr_t;

#ifdef __cplusplus
}
#endif
