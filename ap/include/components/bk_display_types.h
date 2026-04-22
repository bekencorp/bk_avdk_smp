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

#include <avdk_error.h>
#include <driver/dpu_types.h>
#include <components/bk_lcd_types.h>
// #include <dpu_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/** MIPI DPU / DSI clock root: DPU register mux and DSI PHY init path (see mipi_dsi_clock_set). */
typedef enum {
    DPU_CLK_SRC_UNKNOWN = 0,                     /**< default: Naneng DPHY internal PLL + byte-cycle VID timing */
    DPU_CLK_SRC_320M_480M = 1,                   /**< legacy: fixed DPHY table from dsi_dphy_bitrate_calc + hal_dsi_dphy_init */
    DPU_CLK_SRC_NANENG_DPHY_INTERNAL_DPLL = 2,   /**< Naneng DPHY internal PLL + hal_dsi_dphy_init_for_panel */
} dpu_clk_src_t;

/*
* Bus Types Start
*/
typedef enum
{
    DSI_DISPLAY_PORT = 1,     /**< dsi port */
    RGB_DISPLAY_PORT,         /**< rgb port */
} bk_display_dsi_port_t;

typedef struct
{
    /** Same value as bk_display_dpu_config_t.clk_src; NULL config to bk_display_dsi_bus_new → DPU_CLK_SRC_UNKNOWN */
    dpu_clk_src_t clk_src;
} bk_display_dsi_bus_config_t;

typedef struct
{
    uint8_t reset_pin;   /**< lcd reset io */
    uint8_t clk_pin;     /**< lcd clk io */
    uint8_t csx_pin;     /**< lcd csx io */
    uint8_t sda_pin;     /**< lcd sda io */
} bk_display_rgb_bus_config_t;

typedef struct
{
    uint8_t scl_pin;     /**< i2c scl io */
    uint8_t sda_pin;     /**< i2c sda io */
} bk_display_i2c_bus_config_t;

typedef struct
{
    const bk_lcd_panel_t *lcd_panel;
    uint8_t spi_id;      /**< spi id */
    uint8_t reset_pin;   /**< lcd reset io */
    uint8_t dc_pin;      /**< lcd data or command io */
    uint8_t te_pin;      /**< lcd te io */
} bk_display_spi_bus_config_t;

typedef struct
{
    uint32_t clk;                /**< mipi lcd clock */
    uint8_t  n_lanes;            /**< mipi lcd active data lanes (1~4) */
    uint8_t  fps;                /**< frame rate;  according to fps calculate dsi rate, should not over max dsi lane clock rate*/
    dpu_clk_src_t clk_src;       /**< @see dpu_clk_src_t — selects DSI PHY / VID timing path */
    bk_display_timing_t timing;    /**< dpu video timing */
} bk_panel_clock_config_t;


typedef struct
{
    uint32_t id;   
    const char *name;               /**< rgb panel name */
    bk_display_timing_t timing;       /**< rgb panel timing */
    const lcd_rgb_spi_init_cmd_t *init_cmds;  /**< initialization command sequence (8-bit or 16-bit, determined by spi_cmd_16bit flag) */
    uint8_t spi_cmd_16bit;          /**< 0=8-bit command format (use lower 8 bits of cmd), 1=16-bit command format (use full 16 bits) */
    const uint8_t *read_id_regs;     /**< ID register addresses (array, terminated by 0) */
    uint8_t read_id_bytes;           /**< number of bytes to read for ID (1-3, 0 means auto) */
    bk_err_t (*custom_reset)(bk_avdk_lcd_panel_t *panel, void *priv); /**< optional custom reset */
} bk_display_rgb_panel_t;

typedef struct
{
    uint32_t id;                    /**< dsi panel id */
    const char *name;               /**< dsi panel name */
    uint8_t n_lanes;                /**< mipi lcd active data lanes (1~4) */
    uint8_t fps;                    /**< mipi lcd fps */
    bk_display_timing_t timing;     /**< dsi panel timing */
    const lcd_mipi_init_cmd_t *init_cmds; /**< initialization command sequence */
    const uint8_t *read_id_regs;    /**< ID register addresses (array, terminated by 0) */
    uint8_t read_id_bytes;          /**< number of bytes to read for ID (1-3, 0 means auto) */
    bk_err_t (*custom_reset)(bk_avdk_lcd_panel_t *panel, void *priv); /**< optional custom reset */
    bk_err_t (*custom_init)(bk_avdk_lcd_panel_t *panel, void *priv);  /**< optional custom init, called in panel init */
} bk_display_dsi_panel_t;

typedef struct
{
    dpu_clk_src_t clk_src;  /**< DPU clock mux + MIPI DSI PHY init path */
    /* Video timing configuration */
    bk_display_timing_t       timing;     /**< dpu timing */
    dpu_video_layer_config_t video;    /**< dpu layer config */
    dpu_graphic_layer_config_t graphic;    /**< dpu graphic layer config */
} bk_display_dpu_config_t;

typedef struct
{
    bk_pixel_format_t format;
    bool decompress;
} bk_display_pixel_format_config_t;

/** Display ioctl*/
typedef enum {
    BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT = 0,
} bk_display_ioctl_cmd_t;

typedef struct bk_display_ctlr_t *bk_display_ctlr_handle_t;
typedef struct bk_display_ctlr_t bk_display_ctlr_t;
struct bk_display_ctlr_t
{
    avdk_err_t (*init)(bk_display_ctlr_t *controller);
    avdk_err_t (*open)(bk_display_ctlr_t *controller);
    avdk_err_t (*close)(bk_display_ctlr_t *controller);
    avdk_err_t (*deinit)(bk_display_ctlr_t *controller);
    avdk_err_t (*suspend)(bk_display_ctlr_t *controller);
    avdk_err_t (*resume)(bk_display_ctlr_t *controller);
    avdk_err_t (*flush)(bk_display_ctlr_t *controller, uint8_t *frame, flush_free_cb_t cb);
    avdk_err_t (*layer_flush)(bk_display_ctlr_t *controller, dpu_layer_t layer, uint8_t *frame, flush_free_cb_t cb);
    avdk_err_t (*ioctl)(bk_display_ctlr_t *controller, bk_display_ioctl_cmd_t cmd, void *arg);
    avdk_err_t (*del)(bk_display_ctlr_t *controller);
};

#ifdef __cplusplus
}
#endif
