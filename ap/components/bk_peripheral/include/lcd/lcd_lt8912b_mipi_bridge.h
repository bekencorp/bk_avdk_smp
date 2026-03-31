#pragma once

#include <components/bk_display_types.h>

#ifdef __cplusplus
extern "C" {
#endif
#if  CONFIG_LCD_LT8912B_MIPI_BRIDGE
/* LT8912B HDMI bridge common types and presets (similar to ESP lt8912b header). */

/* Video timing structure for LT8912B HDMI output.
 * lcd_mipi_lt8912b_1920x1080.c uses the macros below for s_lt8912b_timing_current (single source of truth).
 */
typedef struct {
    uint16_t hfp;
    uint16_t hs;
    uint16_t hbp;
    uint16_t hact;
    uint16_t htotal;
    uint16_t vfp;
    uint16_t vs;
    uint16_t vbp;
    uint16_t vact;
    uint16_t vtotal;
    bool     h_polarity;
    bool     v_polarity;
    uint16_t vic;
    uint8_t  aspect_ratio;  /* 0=no data, 1=4:3, 2=16:9, 3=no data (reserved). */
    uint32_t pclk_mhz;
} lt8912b_video_timing_t;

/* Aspect ratio encoding used in LT8912B AVI infoframe. */
#define LT8912B_ASPECT_RATIO_NO     0x00
#define LT8912B_ASPECT_RATIO_4_3    0x01
#define LT8912B_ASPECT_RATIO_16_9   0x02

/* Predefined video timing presets, aligned with esp_lcd_lt8912b.h */

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

/* Check LT8912B HDMI ready status (HPD) via register 0xC1[7]. */
bk_err_t lt8912b_is_ready(bool *ready);

/* Enable LT8912B internal HDMI test pattern using current resolution.
 * This API is only available when CONFIG_LCD_LT8912B_MIPI_BRIDGE and ENABLE_TEST_PATTERN are enabled.
 */
bk_err_t bk_lcd_lt8912b_send_test_pattern(void);

extern const bk_display_dsi_panel_t lcd_device_lt8912b_mipi;
#endif

#ifdef __cplusplus
}
#endif

