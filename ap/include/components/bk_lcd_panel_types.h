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
 * @file bk_lcd_panel_types.h
 * @brief Public LCD panel types: handles, descriptors, timing/reset
 *        structs, legacy enums, and the section-based panel registry.
 */

#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <common/bk_include.h>
#include <common/avdk_pixel_types.h>
#include <driver/lcd_qspi_types.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward typedef of the bus handle. The full opaque typedef also lives
 * in <components/bk_display_bus.h>; C11 §6.7.3 allows the redefinition. */
typedef struct bk_display_bus_ctlr_t *bk_display_bus_handle_t;

/** Opaque panel handle. Body lives in private_include/. */
typedef struct bk_avdk_lcd_panel_t bk_avdk_lcd_panel_t;
typedef struct bk_avdk_lcd_panel_t *bk_avdk_lcd_panel_handle_t;

/* ::bk_display_timing_t is defined in <driver/dpu_types.h> and re-exported
 * via the include below; placed in the driver layer so that mipi_dsi_*
 * primitives can use it without pulling component-layer headers. */

/* Default reset waveform timings (ms). Zero fields in
 * ::bk_display_reset_timing_t fall back to these.
 *
 * The waveform has three phases:
 *   IDLE    : pin parked at the inactive level, waiting for the supply
 *             rail / IO mux to settle before the active pulse.
 *   ACTIVE  : pin driven to the active reset level (the pulse width).
 *   RELEASE : pin released back to the inactive level; we wait this
 *             long for the panel's internal boot sequence to finish
 *             before the first command goes out.
 *
 * The plain ``_MS_DEFAULT`` triplet is shared by command-channel panels
 * (DSI / SPI / 8080); ``_MS_RGB_DEFAULT`` is RGB-only and tuned for the
 * longer hard-reset timing the parallel-RGB driver ICs typically require.
 */
#define BK_DISPLAY_RESET_IDLE_MS_DEFAULT          10
#define BK_DISPLAY_RESET_ACTIVE_MS_DEFAULT        10
#define BK_DISPLAY_RESET_RELEASE_MS_DEFAULT       120
#define BK_DISPLAY_RESET_IDLE_MS_RGB_DEFAULT      10
#define BK_DISPLAY_RESET_ACTIVE_MS_RGB_DEFAULT    120
#define BK_DISPLAY_RESET_RELEASE_MS_RGB_DEFAULT   60

/** Per-panel reset waveform overrides; zero fields use the defaults above. */
typedef struct {
    uint16_t idle_ms;     /**< pre-pulse idle-level wait, default ::BK_DISPLAY_RESET_IDLE_MS_DEFAULT */
    uint16_t active_ms;   /**< active reset pulse width, default ::BK_DISPLAY_RESET_ACTIVE_MS_DEFAULT */
    uint16_t release_ms;  /**< post-release boot wait, default ::BK_DISPLAY_RESET_RELEASE_MS_DEFAULT */
} bk_display_reset_timing_t;

/** LCD hardware interface family. */
typedef enum
{
    LCD_TYPE_RGB,     /**< parallel RGB */
    LCD_TYPE_QSPI,    /**< QSPI */
    LCD_TYPE_SPI,     /**< SPI */
    LCD_TYPE_DSI      /**< MIPI DSI */
} lcd_type_t;

/** Legacy LCD pixel-clock enum. Kept verbatim for SPI/QSPI panel descriptors. */
typedef enum {
    LCD_320M = 320, LCD_240M = 240, LCD_160M = 160, LCD_120M = 120,
    LCD_106M = 106, LCD_100M = 100, LCD_96M  = 96,  LCD_90M  = 90,
    LCD_80M  = 80,  LCD_64M  = 64,  LCD_60M  = 60,  LCD_53M  = 53,
    LCD_48M  = 48,  LCD_45M  = 45,  LCD_40M  = 40,  LCD_35M  = 35,
    LCD_34M  = 34,  LCD_32M  = 32,  LCD_30M  = 30,  LCD_29M  = 29,
    LCD_26M  = 26,  LCD_24M  = 24,  LCD_22M  = 22,  LCD_21M  = 21,
    LCD_20M  = 20,  LCD_18M  = 18,  LCD_17M  = 17,  LCD_16M  = 16,
    LCD_15M  = 15,  LCD_14M  = 14,  LCD_13M  = 13,  LCD_12M  = 12,
    LCD_11M  = 11,  LCD_10M  = 10,  LCD_9M   = 9,   LCD_8M   = 8,
    LCD_7M   = 7
} lcd_clk_t;

typedef enum
{
    LCD_QSPI_80M = 80, LCD_QSPI_60M = 60, LCD_QSPI_53M = 53, LCD_QSPI_48M = 48,
    LCD_QSPI_40M = 40, LCD_QSPI_32M = 32, LCD_QSPI_30M = 30, LCD_QSPI_24M = 24,
} lcd_qspi_clk_t;


/** QSPI bus configuration (legacy ::lcd_device_t arm). */
typedef struct
{
    lcd_qspi_clk_t clk;
    lcd_qspi_refresh_method_t refresh_method;
    uint8_t reg_write_cmd;
    uint8_t reg_read_cmd;
    lcd_qspi_write_config_t pixel_write_config;
    lcd_qspi_reg_read_config_t reg_read_config;
    const lcd_qspi_init_cmd_t *init_cmd;
    uint32_t device_init_cmd_len;
    lcd_qspi_refresh_config_by_line_t refresh_config;
    uint32_t frame_len;
} lcd_qspi_t;

/** SPI bus configuration (legacy ::lcd_device_t arm). */
typedef struct
{
    lcd_qspi_clk_t clk;
    const lcd_qspi_init_cmd_t *init_cmd;
    uint32_t device_init_cmd_len;
    uint32_t frame_len;
} lcd_spi_t;

/**
 * @brief Per-command record for a MIPI DSI panel init sequence.
 *
 * Sentinel: ``{0, NULL, 0}``. Delay marker:
 * ``{0, (const uint8_t []){ms}, 0xFF}``.
 */
typedef struct
{
    uint8_t cmd;
    const void *data;
    uint8_t data_len;
} lcd_mipi_init_cmd_t;

/**
 * @brief RGB panel SPI register-init command. Wire format selected by
 *        ::bk_display_rgb_panel_t::spi_cmd_16bit.
 */
typedef struct {
    uint16_t cmd;           /**< 8-bit: low 8 bits; 16-bit: full 16 bits */
    const void *data;       /**< const uint8_t* (8-bit) or const uint16_t* (16-bit), NULL if no data */
    uint8_t data_len;       /**< number of data bytes/words; or delay ms when cmd==0xFF/0xFFFF && data_len==0xFF */
} lcd_rgb_spi_init_cmd_t;

/** Per-panel hardware-bring-up parameters consumed by
 *  ::bk_lcd_mipi_panel_new() / ::bk_lcd_rgb_panel_new().
 *
 * Reset polarity is now a panel-IC property and lives on the descriptor
 * (::bk_display_dsi_panel_t / ::bk_display_rgb_panel_t).
 *
 * The DPU clock source is no longer carried here. MIPI panels default
 * to ::DPU_CLK_SRC_DPHY_DPLL with automatic SYSCLK fallback when the
 * PHY PLL cannot satisfy the panel's lane:pclk ratio; RGB panels are
 * always SYSCLK. To force a specific source, call
 * ::bk_display_bus_set_clock_src() between ::bk_display_dsi_bus_new()
 * and ::bk_lcd_mipi_panel_new(). */
typedef struct bk_lcd_panel_config_t bk_lcd_panel_config_t;
struct bk_lcd_panel_config_t {
    int8_t reset_pin;                   /**< LCD reset GPIO (-1 to disable) */
};

/** Convert integer megahertz (e.g. @p mhz == 32 for 32 MHz) to pixel-clock Hz for RGB panels. */
#define BK_RGB_PIXEL_CLK_HZ(mhz)  ((uint32_t)(mhz) * 1000000U)

/** Panel id for legacy SPI/QSPI descriptors (::lcd_device_t). */
typedef enum {
    LCD_DEVICE_UNKNOW = 0,
    LCD_DEVICE_SH8601A,
    LCD_DEVICE_ST77903_WX20114,
    LCD_DEVICE_ST77903_SAT61478M,
    LCD_DEVICE_ST77903_H0165Y008T,
    LCD_DEVICE_SPD2010,
    LCD_DEVICE_GC9C01,
    LCD_DEVICE_JD9855,
    LCD_DEVICE_JD9855_K18XJ15,
    LCD_DEVICE_ST77916,
    LCD_DEVICE_JD9853,
    LCD_DEVICE_JD9853A,
    LCD_DEVICE_ST7796U,
    LCD_DEVICE_GC9D01,
    LCD_DEVICE_ST7789V2,
} lcd_device_id_t;

/** RGB panel descriptor. */
typedef struct
{
    uint32_t id;
    const char *name;
    /** Target parallel-RGB DPI pixel clock (Hz). Hardware picks nearest PLL/div; see ::BK_RGB_PIXEL_CLK_HZ. */
    uint32_t pixel_clock_hz;
    bk_display_timing_t timing;
    const lcd_rgb_spi_init_cmd_t *init_cmds;    /**< terminated by ``{0, NULL, 0}`` */
    /**
     * Wire format for the panel's private SW SPI register-init channel.
     *  - 0 = 9-bit SPI (ST7701S / GC9503 / AML01-class)
     *  - 1 = 4-byte packed SPI (NT35512 / NT35510-class)
     *
     * Applications copy this to bus_cfg.cmd_width:
     * ``cmd_width = panel->spi_cmd_16bit ? 16 : 8``.
     */
    uint8_t spi_cmd_16bit;
    const uint8_t *read_id_regs;     /**< ID register addresses (terminated by 0) */
    uint8_t read_id_bytes;           /**< number of bytes to read for ID (1-3, 0 = auto) */
    bool reset_active_level;         /**< true = active-high RST, false = active-low RST */
    bk_display_reset_timing_t reset_timing;     /**< zero fields fall back to ::BK_DISPLAY_RESET_*_MS_RGB_DEFAULT */
    /** Reset hook driven by ::bk_display_init(). ::bk_lcd_rgb_default_reset
     *  for the standard GPIO H/L/H, NULL to skip, or a custom function. */
    bk_err_t (*reset)(bk_avdk_lcd_panel_t *panel);
    /** Init hook driven by ::bk_display_init() (after the bus clock is set).
     *  ::bk_lcd_rgb_default_init to send @c init_cmds, NULL to skip, or
     *  a custom function (may compose by calling the default first). */
    bk_err_t (*init)(bk_avdk_lcd_panel_t *panel);
} bk_display_rgb_panel_t;

/** MIPI-DSI panel descriptor. */
typedef struct
{
    uint32_t id;
    const char *name;
    uint8_t n_lanes;                /**< MIPI active data lanes (1~4) */
    uint8_t fps;
    bk_display_timing_t timing;
    const lcd_mipi_init_cmd_t *init_cmds;       /**< terminated by ``{0, NULL, 0}`` */
    const uint8_t *read_id_regs;
    uint8_t read_id_bytes;
    bool reset_active_level;        /**< true = active-high RST, false = active-low RST */
    bk_display_reset_timing_t reset_timing;     /**< zero fields fall back to ::BK_DISPLAY_RESET_*_MS_DEFAULT */
    /** Reset hook driven by ::bk_display_init(). ::bk_lcd_mipi_default_reset
     *  for the standard GPIO H/L/H, NULL to skip, or a custom function. */
    bk_err_t (*reset)(bk_avdk_lcd_panel_t *panel);
    /** Init hook driven by ::bk_display_init() (after the bus clock is set).
     *  ::bk_lcd_mipi_default_init to send @c init_cmds, NULL to skip, or
     *  a custom function (may compose by calling the default first). */
    bk_err_t (*init)(bk_avdk_lcd_panel_t *panel);
} bk_display_dsi_panel_t;

/**
 * @brief Legacy SPI/QSPI panel descriptor.
 *
 * RGB and MIPI-DSI panels use ::bk_display_rgb_panel_t and
 * ::bk_display_dsi_panel_t. Panel lookup uses
 * ::BK_LCD_PANEL_DEVICE_SECTION entry names, not fields here.
 */
typedef struct {
    int id;                                      /**< ::lcd_device_id_t */
    char *name;                                  /**< short name, e.g. "st77916" */
    uint8_t type;                                /**< ::lcd_type_t */
    uint16_t width;                              /**< active pixels  */
    uint16_t height;                             /**< active lines   */
    union {
        const lcd_qspi_t *qspi;                  /**< QSPI bus configuration */
        const lcd_spi_t *spi;                    /**< SPI bus configuration  */
    };
} lcd_device_t;

/** @deprecated Historical alias; new code should use ::lcd_device_t directly. */
typedef lcd_device_t bk_lcd_panel_t;

/** Bus family of a registered LCD panel descriptor. */
typedef enum {
    BK_LCD_PANEL_BUS_RGB  = 0,    /**< parallel RGB / SPI-init RGB panel */
    BK_LCD_PANEL_BUS_DSI  = 1,    /**< MIPI-DSI panel (incl. DSI bridges) */
    BK_LCD_PANEL_BUS_SPI  = 2,    /**< pure SPI panel */
    BK_LCD_PANEL_BUS_QSPI = 3,    /**< QSPI / quad-SPI panel */
    BK_LCD_PANEL_BUS_MAX,
} bk_lcd_panel_bus_type_t;

/** Linker-section entry emitted by ::BK_LCD_PANEL_DEVICE_SECTION. */
typedef struct {
    const char *name;             /**< panel name */
    const void *panel;            /**< cast based on bus_type */
    uint8_t bus_type;             /**< ::bk_lcd_panel_bus_type_t */
} bk_lcd_panel_device_entry_t;

#define _LCD_SECTION_ATTR_IMPL(SECTION, COUNTER)    __attribute__((section(SECTION "." _LCD_COUNTER_STRINGIFY(COUNTER))))
#define _LCD_COUNTER_STRINGIFY(COUNTER) #COUNTER

#define _LCD_PANEL_DEVICE_SECTION_IMPL(panel_sym, panel_name, panel_bus, unique_id)                       \
    __attribute__((used)) _LCD_SECTION_ATTR_IMPL(".lcd_panel_device_list", unique_id)                     \
    const bk_lcd_panel_device_entry_t bk_lcd_panel_device_##panel_sym = {                                 \
        .name = panel_name,                                                                               \
        .panel = &panel_sym,                                                                              \
        .bus_type = (uint8_t)(panel_bus),                                                                 \
    };

/**
 * @brief Register a panel descriptor into the linker-section registry.
 *
 * Drops a ::bk_lcd_panel_device_entry_t entry into the
 * ``.lcd_panel_device_list`` section so ::bk_lcd_get_*_panel_list() and
 * ::bk_lcd_find_*_panel_by_name() can discover it without a global table.
 *
 * @param panel_sym  Panel symbol, e.g. ``lcd_device_hx8399c_mipi_1080x1920``.
 * @param panel_name Panel name string, e.g. ``"hx8399c_mipi_1080x1920"``.
 * @param panel_bus  Bus family, ::bk_lcd_panel_bus_type_t.
 */
#define BK_LCD_PANEL_DEVICE_SECTION(panel_sym, panel_name, panel_bus)                                     \
    _LCD_PANEL_DEVICE_SECTION_IMPL(panel_sym, panel_name, panel_bus, __COUNTER__)

extern bk_lcd_panel_device_entry_t __lcd_panel_device_array_start;
extern bk_lcd_panel_device_entry_t __lcd_panel_device_array_end;

#ifdef __cplusplus
}
#endif
