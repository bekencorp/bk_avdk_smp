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

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    BK_DISPLAY_BUS_RW_DSI_CMD,      /**<DSI command*/
    BK_DISPLAY_BUS_RW_SPI_CMD,      /**<SPI 8 bit command*/
    BK_DISPLAY_BUS_RW_SPI_DATA,     /**<SPI 8 bit data*/
    BK_DISPLAY_BUS_RW_SPI_HF_CMD,   /**<SPI 16 bit command*/
    BK_DISPLAY_BUS_RW_SPI_HF_DATA,  /**<SPI 16 bit data*/
    BK_DISPLAY_BUS_RW_I2C_REG,      /**<I2C register access, cmd = (dev_addr<<8)|reg */
} bk_display_bus_rw_type_t;

typedef struct bk_display_bus_ctlr_t *bk_display_bus_handle_t;
typedef struct bk_display_bus_ctlr_t bk_display_bus_ctlr_t;
struct bk_display_bus_ctlr_t
{
    avdk_err_t (*enable)(bk_display_bus_ctlr_t *controller);
    avdk_err_t (*disable)(bk_display_bus_ctlr_t *controller);
    avdk_err_t (*read)(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, void *param, size_t size);
    avdk_err_t (*write)(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, const void *param, size_t size);
    avdk_err_t (*delete)(bk_display_bus_ctlr_t *controller);
    avdk_err_t (*set_clock)(bk_display_bus_ctlr_t *controller, bk_panel_clock_config_t *porch);
    avdk_err_t (*flush)(bk_display_bus_ctlr_t *controller, uint8_t *frame, flush_free_cb_t cb);
};

avdk_err_t bk_display_bus_enable(bk_display_bus_handle_t handle);
avdk_err_t bk_display_bus_disable(bk_display_bus_handle_t handle);
avdk_err_t bk_display_bus_set_clock(bk_display_bus_handle_t handle, bk_panel_clock_config_t *clock);
avdk_err_t bk_display_bus_read(bk_display_bus_handle_t handle, bk_display_bus_rw_type_t type, uint32_t cmd, void *param, size_t size);
avdk_err_t bk_display_bus_write(bk_display_bus_handle_t handle, bk_display_bus_rw_type_t type, uint32_t cmd, const void *param, size_t size);
avdk_err_t bk_display_bus_flush(bk_display_bus_handle_t handle, uint8_t *frame, flush_free_cb_t cb);
avdk_err_t bk_display_bus_delete(bk_display_bus_handle_t handle);

avdk_err_t bk_display_dsi_bus_new(bk_display_bus_handle_t *handle, bk_display_dsi_bus_config_t *config);
avdk_err_t bk_display_rgb_bus_new(bk_display_bus_handle_t *handle, bk_display_rgb_bus_config_t *config);
avdk_err_t bk_display_spi_bus_new(bk_display_bus_handle_t *handle, bk_display_spi_bus_config_t *config);
avdk_err_t bk_display_i2c_bus_new(bk_display_bus_handle_t *handle, const bk_display_i2c_bus_config_t *config);
