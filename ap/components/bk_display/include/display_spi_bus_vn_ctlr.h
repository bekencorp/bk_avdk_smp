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
#include <components/bk_display_bus.h>
#include <components/bk_lcd_types.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    bool disp_task_running;
    beken_semaphore_t disp_task_sem;
    beken_thread_t disp_task;
    beken_queue_t queue;
    beken_mutex_t lock;
    uint8_t spi_id;
    uint8_t reset_pin;
    uint8_t dc_pin;
    const bk_lcd_panel_t *device;
    void *display_frame;
    flush_free_cb_t display_frame_cb;
    bool lcd_display_flag;
} private_display_spi_context_t;

typedef struct
{
    bk_display_spi_bus_config_t config;
    bk_display_bus_ctlr_t ops;
    private_display_spi_context_t spi_context;
} spi_bus_vn_ctlr_t;

#ifdef __cplusplus
}
#endif

