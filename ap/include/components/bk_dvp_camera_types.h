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

#include <common/bk_err.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_camera_sensor.h>
#include <components/bk_isp_camera_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief dvp sensor configure
 * @{
 */
typedef struct
{
    char *name;  /**< sensor name */
    mclk_freq_t  clk;  /**< sensor work clk in config fps and ppi */
    bk_pixel_format_t fmt; /**< sensor input data format */
    sync_level_t vsync; /**< sensor vsync active level  */
    sync_level_t hsync; /**< sensor hsync active level  */
    uint16_t default_width;
    uint16_t default_height;
    uint16_t default_fps;
    uint16_t id;  /**< sensor type, sensor_id_t */
    uint16_t address;  /**< sensor write register address by i2c */
    avdk_err_t (*detect)(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);  /**< auto detect used dvp sensor */
    avdk_err_t (*init)(bk_camera_sensor_ctlr_t *controller);  /**< init dvp sensor */
    avdk_err_t (*set_ppi)(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height);  /**< set resolution of sensor */
    avdk_err_t (*set_fps)(bk_camera_sensor_ctlr_t *controller, uint16_t fps);  /**< set fps of sensor */
    avdk_err_t (*power_down)(bk_camera_sensor_ctlr_t *controller);  /**< power down or reset of sensor */
} dvp_sensor_config_t;

typedef struct
{
    uint16_t id;
    uint16 img_fomat; // image_format_t
    uint16_t width;
    uint16_t height;
    uint8_t fps;
} bk_dvp_config_t;

typedef struct
{
    void (*frame_complete)(image_format_t format, frame_buffer_t *buffer);
} bk_dvp_callback_t;


/*
 * @}
 */

#ifdef __cplusplus
}
#endif
