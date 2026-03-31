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

#include <components/bk_display_types.h>
#include <components/bk_display_dpu_ctlr.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DISP_STATE_INIT,
    DISP_STATE_OPEN,
    DISP_STATE_CLOSE,
    DISP_STATE_DEINIT,
} display_state_t;

typedef struct
{
    display_state_t state;
    uint16_t width;    //real source output width
    uint16_t height;   //real source output height
    int (*flush)(void ** disp_ctrl, dpu_layer_t layer, void *data, flush_free_cb_t cb);
    dpu_handle_t dpu_handle;
    bk_lcd_bus_io_t *dsi_handle;
    bk_avdk_lcd_panel_handle_t dev_handle;

    bk_display_dpu_config_t config;
    bk_display_ctlr_t ops;
} dpu_vn_ctlr_t;

#ifdef __cplusplus
}
#endif

