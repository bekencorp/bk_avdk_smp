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

#include <avdk_check.h>
#include <common/bk_include.h>
#include <common/avdk_pixel_types.h>
#include <driver/lcd_qspi_types.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef likely
#define likely(x)      (x)
#endif
#ifndef unlikely
#define unlikely(x)    (x)
#endif



#define  USE_LCD_REGISTER_CALLBACKS  1
typedef void (*lcd_isr_t)(void);

typedef enum
{
    LCD_TYPE_RGB,     /**< lcd hardware interface is parallel RGB interface */
    LCD_TYPE_QSPI,    /**< lcd device hardware interface is QSPI interface */
    LCD_TYPE_SPI,     /**< lcd device hardware interface is SPI interface */
    LCD_TYPE_DSI      /**< lcd device hardware interface is DSI interface */
} lcd_type_t;


/**
 * @brief RGB element order
 */
typedef enum {
    LCD_RGB_DATA_ENDIAN_BIG = 0, /*!< RGB data endian: MSB first */
    LCD_RGB_DATA_ENDIAN_LITTLE,  /*!< RGB data endian: LSB first */
} lcd_rgb_data_endian_t;

/** @cond */
/// for backward compatible

/***lcd clk***/
typedef enum {
    LCD_320M = 320,
    LCD_240M = 240,
    LCD_160M = 160,
    LCD_120M = 120,
	
    LCD_106M = 106,  // PLL CLK
    LCD_100M = 100,  // PLL CLK
    LCD_96M  = 96,   //PLL CLK
    LCD_90M  = 90,   //PLL CLK
    LCD_80M  = 80,
    LCD_64M  = 64,
    LCD_60M  = 60,
    LCD_53M  = 53,   // 320/6 = 53.33M, 向下取整 = 53M
    LCD_48M  = 48,
    LCD_45M  = 45,   // 320/7 = 45.71M, 向下取整 = 45M
    LCD_40M  = 40,
    LCD_35M  = 35,   // 320/9 = 35.56M, 向下取整 = 35M
    LCD_34M  = 34,   // 240/7 = 34.29M, 向下取整 = 34M
    LCD_32M  = 32,
    LCD_30M  = 30,
    LCD_29M  = 29,   // 320/11 = 29.09M, 向下取整 = 29M
    LCD_26M  = 26,   // 240/9 = 26.67M, 向下取整 = 26M
    LCD_24M  = 24,
    LCD_22M  = 22,   // 320/14 = 22.86M, 向下取整 = 22M
    LCD_21M  = 21,   // 240/11 = 21.82M, 向下取整 = 21M
    LCD_20M  = 20,
    LCD_18M  = 18,   // 240/13 = 18.46M, 向下取整 = 18M
    LCD_17M  = 17,   // 240/14 = 17.14M, 向下取整 = 17M
    LCD_16M  = 16,
    LCD_15M  = 15,
    LCD_14M  = 14,   // 240/17 = 14.12M, 向下取整 = 14M
    LCD_13M  = 13,   // 240/18 = 13.33M, 向下取整 = 13M
    LCD_12M  = 12,
    LCD_11M  = 11,   // 240/21 = 11.43M, 向下取整 = 11M
    LCD_10M  = 10,
    LCD_9M   = 9,    // 240/26 = 9.23M, 向下取整 = 9M
    LCD_8M   = 8,
    LCD_7M   = 7     // 240/32 = 7.5M, 向下取整 = 7M
} lcd_clk_t;

typedef enum
{
    LCD_QSPI_80M = 80,
    LCD_QSPI_60M = 60,
    LCD_QSPI_53M = 53, //53.3M
    LCD_QSPI_48M = 48,
    LCD_QSPI_40M = 40,
    LCD_QSPI_32M = 32,
    LCD_QSPI_30M = 30,
    LCD_QSPI_24M = 24,
} lcd_qspi_clk_t;


/**
 * @brief Timing parameters for the video data transmission
 */
typedef struct {
    uint32_t clk;
    uint16_t h_size;            /*!< Horizontal resolution, i.e. the number of pixels in a line */
    uint16_t v_size;            /*!< Vertical resolution, i.e. the number of lines in the frame  */
    uint16_t hsync_pulse_width; /*!< Horizontal sync width, in pixel clock */
    uint16_t vsync_pulse_width; /*!< Vertical sync width, in number of lines */
    uint16_t hsync_back_porch;  /*!< Horizontal back porch, number of pixel clock between hsync and start of line active data */
    uint16_t hsync_front_porch; /*!< Horizontal front porch, number of pixel clock between the end of active data and the next hsync */
    uint16_t vsync_back_porch;  /*!< Vertical back porch, number of invalid lines between vsync and start of frame */
    uint16_t vsync_front_porch; /*!< Vertical front porch, number of invalid lines between the end of frame and the next vsync */
} bk_display_timing_t;

/** qspi interface config param */
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

/** spi interface config param */
typedef struct
{
	lcd_qspi_clk_t clk;
	const lcd_qspi_init_cmd_t *init_cmd;
	uint32_t device_init_cmd_len;
	uint32_t frame_len;
} lcd_spi_t;

typedef struct
{
    int id;           /**< lcd device type, user can add if you want to add another lcd device */
    char *name;                  /**< lcd device name */
    lcd_type_t type;             /**< lcd device hw interface */
    uint16_t width;              /**< lcd device x y size */
    uint16_t height;
    bk_pixel_format_t src_fmt;      /**< source data format: input to display module data format(rgb565/rgb888/yuv)*/
    bk_pixel_format_t out_fmt;      /**< display module output data format(rgb565/rgb666/rgb888), input to lcd device,*/
    union
    {
        const lcd_qspi_t *qspi;  /**< QSPI interface lcd device config */
        const lcd_spi_t *spi;    /**< SPI interface lcd device config */
    };
    void (*init)(void);          /**< lcd device initial function */
    void (*deinit)(void);        /**< lcd device deinitial function */
    bk_err_t (*lcd_off)(void);   /**< lcd off */
} bk_lcd_panel_t;

typedef struct {
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
} lcd_display_area_t;

//////////////////////==================new add============================///////////////////////////////


typedef struct
{
    uint8_t cmd;
    const void *data;       /*<! Buffer that holds the command specific data */
    uint8_t data_len;       /**< Payload length; or cmd==0 and data_len==0xFF for delay ms in ((uint8_t *)data)[0] */
} lcd_mipi_init_cmd_t;

/**
 * @brief RGB panel SPI initialization command structure (unified for 8-bit and 16-bit)
 *
 * For RGB panels using SPI interface, commands are sent as CMD followed by DATA bytes/words.
 * The command format (8-bit or 16-bit) is determined by the spi_cmd_16bit flag in bk_display_rgb_panel_t.
 * Special command values:
 * - 8-bit: cmd = 0xFF, data_len = 0xFF: delay in milliseconds (data[0] contains delay value)
 * - 16-bit: cmd = 0xFFFF, data_len = 0xFF: delay in milliseconds (data[0] contains delay value)
 */
typedef struct {
    uint16_t cmd;                   /**< Command value (8-bit: use lower 8 bits like 0x0011, 16-bit: use full 16 bits like 0xF000) */
    const void *data;               /**< Data array: const uint8_t* for 8-bit, const uint16_t* for 16-bit (NULL if no data) */
    uint8_t data_len;               /**< Number of data bytes (8-bit) or words (16-bit), or delay ms if cmd=0xFF/0xFFFF and data_len=0xFF */
} lcd_rgb_spi_init_cmd_t;


/**
 * @brief RGB element order
 */
typedef enum {
    COLOR_RGB_ELEMENT_ORDER_RGB, /*!< RGB element order: RGB */
    COLOR_RGB_ELEMENT_ORDER_BGR, /*!< RGB element order: BGR */
} color_rgb_element_order_t;

/**
 * @brief Configuration structure for panel device
 */
typedef struct bk_lcd_panel_dev_config_t bk_lcd_panel_dev_config_t;
struct bk_lcd_panel_dev_config_t {
    int8_t reset_pin;                   ///< lcd reset io
    int8_t clk_pin;
    int8_t csx_pin;
    int8_t sda_pin;
    color_rgb_element_order_t rgb_ele_order;   /*!< @deprecated Set RGB data endian, please use rgb_ele_order instead */
    lcd_rgb_data_endian_t data_endian;         /*!< Set the data endian for color data larger than 1 byte */
    uint32_t bits_per_pixel;                   /*!< Color depth, in bpp */
    struct {
        uint32_t reset_active_level: 1; /*!< Setting 1 if the panel reset is high level active，0 if low level active */
    } flags;                           /*!< LCD panel config flags */
    void *vendor_config; /*!< vendor specific configuration, optional, left as NULL if not used */
};


typedef struct bk_avdk_lcd_panel_t bk_avdk_lcd_panel_t;  /*!< Type of LCD panel */
typedef struct bk_avdk_lcd_panel_t *bk_avdk_lcd_panel_handle_t;       /*!< Type of LCD panel handle */

/**
 * @brief LCD panel interface
 *
 * Only the operations that are actually exercised by the current MIPI/RGB
 * code paths are exposed here. Optional/legacy hooks (mirror, swap_xy,
 * set_gap, invert_color, disp_on_off, disp_sleep, draw_bitmap) were
 * removed: they were declared but never called by application code.
 * Re-introduce them as a separate `bk_display_panel_ops_t` table when a
 * concrete use-case appears.
 */
 struct bk_avdk_lcd_panel_t{
    /**
     * @brief Reset LCD panel
     *
     * @param[in] panel LCD panel handle
     * @return BK_OK on success
     */
    bk_err_t (*reset)(bk_avdk_lcd_panel_t *panel);

    /**
     * @brief Initialize LCD panel (clock, init command sequence, ...)
     *
     * @param[in] panel LCD panel handle
     * @return BK_OK on success
     */
    bk_err_t (*init)(bk_avdk_lcd_panel_t *panel);

    /**
     * @brief Destroy/free the LCD panel object
     *
     * @param[in] panel LCD panel handle
     * @return BK_OK on success
     */
    bk_err_t (*del)(bk_avdk_lcd_panel_t *panel);

    /**
     * @brief Read LCD panel IC id (optional)
     *
     * @param[in]  panel LCD panel handle
     * @param[out] id    LCD panel IC id
     * @return BK_OK on success, BK_ERR_NOT_SUPPORT if unsupported
     */
    bk_err_t (*read_id)(bk_avdk_lcd_panel_t *panel, uint32_t* id);

    /**
     * @brief Get LCD panel display timing snapshot
     */
    bk_err_t (*get_disp_timing)(bk_avdk_lcd_panel_t *panel, bk_display_timing_t *timing);

    void *user_data;    /*!< User data, used to store externally customized data */
};


typedef struct bk_lcd_bus_io_t bk_lcd_bus_io_t; /*!< Type of LCD panel IO */

/**
 * @brief LCD panel IO interface
 */
struct bk_lcd_bus_io_t {
    /**
     * @brief Destroy LCD panel
     *
     * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
     * @return
     *          - BK_OK on success
     */
    bk_err_t (*del)(bk_lcd_bus_io_t *panel);

    /**
     * @brief Transmit LCD command and receive corresponding parameters
     *
     * @note This is the panel-specific interface called by function `esp_lcd_panel_io_rx_param()`.
     *
     * @param[in]  io LCD panel IO handle, which is created by other factory API like `esp_lcd_new_panel_io_spi()`
     * @param[in]  lcd_cmd The specific LCD command, set to -1 if no command needed
     * @param[out] param Buffer for the command data
     * @param[in]  param_size Size of `param` buffer
     * @return
     *          - BK_ERR_INVALID_ARG   if parameter is invalid
     *          - BK_ERR_NOT_SUPPORTED if read is not supported by transport
     *          - BK_OK                on success
     */
    bk_err_t (*rx_param)(bk_lcd_bus_io_t *io, int lcd_cmd, void *param, uint16_t param_size);

    /**
     * @brief Transmit LCD command and corresponding parameters
     *
     * @note This is the panel-specific interface called by function `bk_lcd_bus_io_tx_param()`.
     *
     * @param[in] io LCD panel IO handle, which is created by other factory API like `esp_lcd_new_panel_io_spi()`
     * @param[in] lcd_cmd The specific LCD command
     * @param[in] param Buffer that holds the command specific parameters, set to NULL if no parameter is needed for the command
     * @param[in] param_size Size of `param` in memory, in bytes, set to zero if no parameter is needed for the command
     * @return
     *          - BK_ERR_INVALID_ARG   if parameter is invalid
     *          - BK_OK                on success
     */
    bk_err_t (*tx_param)(bk_lcd_bus_io_t *io, int lcd_cmd, const void *param, uint16_t param_size);
};



/**
 * @brief LCD panel device entry structure for section registration
 */
typedef struct {
    const char *name;                    /**< Panel name */
    const void *panel;                   /**< Panel structure pointer (bk_display_dsi_panel_t* or bk_display_rgb_panel_t*) */
    uint8_t type;                        /**< Panel type: 0=RGB, 1=DSI */
} bk_lcd_panel_device_entry_t;

/**
 * @brief Section attribute macro implementation
 * @param SECTION Section name
 * @param COUNTER Counter
 */
#define _LCD_SECTION_ATTR_IMPL(SECTION, COUNTER)    __attribute__((section(SECTION "." _LCD_COUNTER_STRINGIFY(COUNTER))))

/**
 * @brief Stringify macro
 * @param COUNTER Counter
 */
#define _LCD_COUNTER_STRINGIFY(COUNTER) #COUNTER

/**
 * @brief Internal macro implementation
 *
 * Note:
 * - @p panel_sym must be the symbol of the panel structure (e.g., lcd_device_hx8399c_mipi_1080x1920)
 * - Use @p panel_sym to generate variable names, thus keeping unique across different .c files
 * - @p unique_id (from __COUNTER__) is only used for section name suffix, not for variable names, to avoid repetition across compilation units
 */
#define _LCD_PANEL_DEVICE_SECTION_IMPL(panel_sym, panel_name, panel_type, unique_id)                      \
    __attribute__((used)) _LCD_SECTION_ATTR_IMPL(".lcd_panel_device_list", unique_id)                   \
    const bk_lcd_panel_device_entry_t bk_lcd_panel_device_##panel_sym = {                                 \
        .name = panel_name,                                                                               \
        .panel = &panel_sym,                                                                              \
        .type = panel_type,                                                                               \
    };

/**
 * @brief LCD panel device section registration macro
 * @param panel_sym Panel symbol (e.g., lcd_device_hx8399c_mipi_1080x1920)
 * @param panel_name Panel name string (e.g., "hx8399c_mipi_1080x1920")
 * @param panel_type Panel type: 0=RGB, 1=DSI
 *
 * This macro registers a LCD panel device into a dedicated section,
 * enabling the system to automatically discover all registered panels.
 *
 * Example:
 *   BK_LCD_PANEL_DEVICE_SECTION(lcd_device_hx8399c_mipi_1080x1920, "hx8399c_mipi_1080x1920", 1);
 */
#define BK_LCD_PANEL_DEVICE_SECTION(panel_sym, panel_name, panel_type)                                    \
    _LCD_PANEL_DEVICE_SECTION_IMPL(panel_sym, panel_name, panel_type, __COUNTER__)

/**
 * @brief External symbols for section boundaries
 * These are defined in the linker script to mark the start and end of the section
 */
extern bk_lcd_panel_device_entry_t __lcd_panel_device_array_start;
extern bk_lcd_panel_device_entry_t __lcd_panel_device_array_end;

/*
 * @}
 */

#ifdef __cplusplus
}
#endif


