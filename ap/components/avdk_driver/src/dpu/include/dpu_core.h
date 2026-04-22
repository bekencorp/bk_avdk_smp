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
#include "components/bk_lcd_types.h"
#include <driver/dpu_types.h>


typedef void * dpu_handle_t;


#define EVENT_BIT_AVAILABLE (1 << 0)

/* dpu_clk_src_t is defined in components/bk_display_types.h */

/**
 * @brief MIPI DSI DPI panel configuration structure
 */
typedef struct {
    dpu_clk_src_t dpu_clk_src;
    uint8_t virtual_channel;                   /*!< Virtual channel ID, index from 0 */
    uint32_t dpi_clock_freq_mhz;               /*!< DPI clock frequency in MHz */
    bk_display_timing_t video_timing;       /*!< Video timing */

    dpu_video_layer_config_t video;
    dpu_graphic_layer_config_t graphic;

    //bk_pixel_format_t pixel_format; /*!< Pixel format that used by the MIPI LCD device */
    //uint8_t dpu_decompress_en : 1;
    //bool dpu_blend_en : 1;
    //uint8_t dpu_blend_mode;
} dpu_config_t;

typedef struct  {
    //bool dpu_decompress_en;
    //bool dpu_blend_en : 1;

    /* layer parameters */
    bool display_dirty;
    beken_event_t dpu_event_handle;
    void *display_frame[DPU_LAYER_MAX];
    flush_free_cb_t display_cb[DPU_LAYER_MAX];
    void *update_frame[DPU_LAYER_MAX];
    flush_free_cb_t update_cb[DPU_LAYER_MAX];
    beken_mutex_t  flush_mutex;
    uint32_t refresh_rate;
    uint32_t frame_rate[DPU_LAYER_MAX];

    //uint8_t dpu_blend_mode;
    uint32_t h_pixels;            // Horizontal pixels
    uint32_t v_pixels;            // Vertical pixels
    uint32_t bits_per_pixel;        // Bits per pixel
    dpu_config_t current_config;
    //bk_pixel_format_t pixel_format; // RGB Pixel format
    int (*draw)(dpu_handle_t *handle, dpu_layer_t layer, void *data, flush_free_cb_t free_cb);
#if CONFIG_DPU_FLUSH_TIMER_DEBUG
    struct {
        beken_timer_t timer;                         /*!< Periodic timer for refresh statistics */
        uint32_t last_refresh_rate;                  /*!< Accumulated refresh count snapshot */
        uint32_t last_frame_rate[DPU_LAYER_MAX];     /*!< Per-layer frame count snapshot */
        float refresh_rps;                           /*!< Refresh operations per second */
        float layer_fps[DPU_LAYER_MAX];              /*!< Per-layer frames per second */
        bool timer_started;                          /*!< Timer running flag */
        bool first_statistics;                       /*!< Flag to skip first statistics */
    } debug;
#endif
}dpu_context_t;


bk_err_t dpu_core_init(dpu_config_t * dpu_config, dpu_handle_t *handle);

bk_err_t dpu_core_deinit(dpu_handle_t *handle);

bk_err_t dpu_core_layer_config(dpu_config_t * dpu_config, dpu_handle_t *handle);

bk_err_t dpu_core_flush(dpu_handle_t *handle, dpu_layer_t layer, void *buff, flush_free_cb_t free_cb);

uint32_t dpu_core_get_flush_addr(dpu_handle_t *handle);

bk_err_t dpu_core_flush_stop(dpu_handle_t *handle);

bk_err_t dpu_core_flush_restart(dpu_handle_t *handle);

bk_err_t dpu_core_runtime_switch(dpu_handle_t *handle, const bk_display_pixel_format_config_t *config);


