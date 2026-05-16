#pragma once

/**
 * @file lcd_lt8912b_mipi_bridge.h
 * @brief LT8912B MIPI-DSI to HDMI bridge driver public API + video
 *        timing presets. The bridge is registered as a DSI panel via
 *        ::lcd_device_lt8912b_mipi and configured through a private
 *        software I2C side-channel owned end-to-end by this driver.
 */

#include <components/bk_lcd_panel.h>

#ifdef __cplusplus
extern "C" {
#endif
#if  CONFIG_LCD_LT8912B_MIPI_BRIDGE

/** Video timing programmed into the LT8912B HDMI output stage. */
typedef struct {
    uint16_t hfp;           /**< horizontal front porch (pixel clocks) */
    uint16_t hs;            /**< HSYNC pulse width */
    uint16_t hbp;           /**< horizontal back porch */
    uint16_t hact;          /**< horizontal active pixels */
    uint16_t htotal;        /**< horizontal total */
    uint16_t vfp;           /**< vertical front porch (lines) */
    uint16_t vs;            /**< VSYNC pulse width */
    uint16_t vbp;           /**< vertical back porch */
    uint16_t vact;          /**< vertical active lines */
    uint16_t vtotal;        /**< vertical total */
    bool     h_polarity;
    bool     v_polarity;
    uint16_t vic;           /**< CEA VIC code (0 if N/A) */
    uint8_t  aspect_ratio;  /**< 0=no data, 1=4:3, 2=16:9 */
    uint32_t pclk_mhz;      /**< pixel clock in MHz */
} lt8912b_video_timing_t;

#define LT8912B_ASPECT_RATIO_NO     0x00
#define LT8912B_ASPECT_RATIO_4_3    0x01
#define LT8912B_ASPECT_RATIO_16_9   0x02

/* 800x600@60Hz */
#define LT8912B_VIDEO_TIMING_800x600_60()   \
    {                                       \
        .hfp          = 48,                 \
        .hs           = 128,                \
        .hbp          = 88,                 \
        .hact         = 800,                \
        .htotal       = 1056,               \
        .vfp          = 1,                  \
        .vs           = 4,                  \
        .vbp          = 23,                 \
        .vact         = 600,                \
        .vtotal       = 628,                \
        .h_polarity   = 1,                  \
        .v_polarity   = 1,                  \
        .vic          = 0,                  \
        .aspect_ratio = LT8912B_ASPECT_RATIO_16_9, \
        .pclk_mhz     = 40,                 \
    }

/* 1024x768@60Hz */
#define LT8912B_VIDEO_TIMING_1024x768_60()  \
    {                                       \
        .hfp          = 48,                 \
        .hs           = 32,                 \
        .hbp          = 80,                 \
        .hact         = 1024,               \
        .htotal       = 1184,               \
        .vfp          = 3,                  \
        .vs           = 4,                  \
        .vbp          = 15,                 \
        .vact         = 768,                \
        .vtotal       = 790,                \
        .h_polarity   = 1,                  \
        .v_polarity   = 0,                  \
        .vic          = 0,                  \
        .aspect_ratio = LT8912B_ASPECT_RATIO_16_9, \
        .pclk_mhz     = 56,                 \
    }

/* 1280x720@60Hz */
#define LT8912B_VIDEO_TIMING_1280x720_60()  \
    {                                       \
        .hfp          = 48,                 \
        .hs           = 32,                 \
        .hbp          = 80,                 \
        .hact         = 1280,               \
        .htotal       = 1440,               \
        .vfp          = 3,                  \
        .vs           = 5,                  \
        .vbp          = 13,                 \
        .vact         = 720,                \
        .vtotal       = 741,                \
        .h_polarity   = 1,                  \
        .v_polarity   = 0,                  \
        .vic          = 0,                  \
        .aspect_ratio = LT8912B_ASPECT_RATIO_16_9, \
        .pclk_mhz     = 64,                 \
    }

/* 1280x800@60Hz */
#define LT8912B_VIDEO_TIMING_1280x800_60()  \
    {                                       \
        .hfp          = 48,                 \
        .hs           = 32,                 \
        .hbp          = 80,                 \
        .hact         = 1280,               \
        .htotal       = 1440,               \
        .vfp          = 3,                  \
        .vs           = 6,                  \
        .vbp          = 14,                 \
        .vact         = 800,                \
        .vtotal       = 823,                \
        .h_polarity   = 1,                  \
        .v_polarity   = 0,                  \
        .vic          = 0,                  \
        .aspect_ratio = LT8912B_ASPECT_RATIO_16_9, \
        .pclk_mhz     = 70,                 \
    }

/* 1920x1080@30Hz */
#define LT8912B_VIDEO_TIMING_1920x1080_30() \
    {                                       \
        .hfp          = 48,                 \
        .hs           = 32,                 \
        .hbp          = 80,                 \
        .hact         = 1920,               \
        .htotal       = 2080,               \
        .vfp          = 3,                  \
        .vs           = 5,                  \
        .vbp          = 8,                  \
        .vact         = 1080,               \
        .vtotal       = 1096,               \
        .h_polarity   = 1,                  \
        .v_polarity   = 0,                  \
        .vic          = 0,                  \
        .aspect_ratio = LT8912B_ASPECT_RATIO_16_9, \
        .pclk_mhz     = 80,                 \
    }

/**
 * @brief Check the LT8912B HDMI hot-plug-detect (HPD) status.
 *
 * @param[out] ready  Receives true when an HDMI sink is connected,
 *                    false when unplugged.
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM if @p ready is NULL.
 */
bk_err_t lt8912b_is_ready(bool *ready);

/**
 * @brief Drive the LT8912B internal HDMI test pattern.
 *
 * Programs the bridge to emit its built-in test pattern at the
 * currently selected ::CONFIG_LCD_LT8912B_RES_* resolution. Useful for
 * bring-up verification without a live MIPI source.
 *
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_lt8912b_send_test_pattern(void);

/** DSI panel descriptor exposed by this bridge driver. */
extern const bk_display_dsi_panel_t lcd_device_lt8912b_mipi;

/** Board-specific I2C side-channel pin assignment for the LT8912B. */
typedef struct {
    int8_t scl_pin;   /**< SCL GPIO routed to LT8912B (>=0) */
    int8_t sda_pin;   /**< SDA GPIO routed to LT8912B (>=0) */
} bk_lcd_lt8912b_io_pins_t;

/**
 * @brief Override the LT8912B private I2C pin assignment.
 *
 * Call from application bring-up (typically the place that owns the
 * board pin map, e.g. ``app_display.c``) BEFORE the DSI panel init
 * triggers the bridge custom_init. If never called, the driver falls
 * back to ``CONFIG_LCD_LT8912B_PIN_SCL`` / ``CONFIG_LCD_LT8912B_PIN_SDA``.
 *
 * @param[in] pins  Pin assignment, both fields must be >=0.
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM if @p pins is NULL.
 * @return BK_ERR_PARAM if any pin is negative.
 */
bk_err_t bk_lcd_lt8912b_set_io_pins(const bk_lcd_lt8912b_io_pins_t *pins);

#endif

#ifdef __cplusplus
}
#endif

