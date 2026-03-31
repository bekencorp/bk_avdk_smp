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
#include <string.h>
#include <os/mem.h>
#include <common/bk_err.h>
#include <common/avdk_pixel_types.h>
#include <driver/lcd_types.h>
#include <driver/sim_spi.h>
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Define all image resolutions in pixels
 * @{
 */
#define PIXEL_160   (160)  /**< 160 pixels */
#define PIXEL_170   (170)  /**< 170 pixels */
#define PIXEL_240   (240)  /**< 240 pixels */
#define PIXEL_256   (256)  /**< 256 pixels */
#define PIXEL_272   (272)  /**< 272 pixels */
#define PIXEL_288   (288)  /**< 288 pixels */
#define PIXEL_320   (320)  /**< 320 pixels */
#define PIXEL_360   (360)  /**< 360 pixels */
#define PIXEL_400   (400)  /**< 400 pixels */
#define PIXEL_412   (412)  /**< 412 pixels */
#define PIXEL_454   (454)  /**< 454 pixels */
#define PIXEL_480   (480)  /**< 480 pixels */
#define PIXEL_576   (576)  /**< 576 pixels */
#define PIXEL_600   (600)  /**< 600 pixels */
#define PIXEL_640   (640)  /**< 640 pixels */
#define PIXEL_720   (720)  /**< 720 pixels */
#define PIXEL_800   (800)  /**< 800 pixels */
#define PIXEL_854   (854)  /**< 854 pixels */
#define PIXEL_864   (864)  /**< 864 pixels */
#define PIXEL_960   (960)  /**< 960 pixels */
#define PIXEL_1024 (1024)  /**< 1024 pixels */
#define PIXEL_1080 (1080)  /**< 1080 pixels */
#define PIXEL_1200 (1200)  /**< 1200 pixels */
#define PIXEL_1280 (1280)  /**< 1280 pixels */
#define PIXEL_1296 (1296)  /**< 1296 pixels */
#define PIXEL_1600 (1600)  /**< 1600 pixels */
#define PIXEL_1920 (1920)  /**< 1920 pixels */
#define PIXEL_2304 (2304)  /**< 2304 pixels */
#define PIXEL_4320 (4320)  /**< 4320 pixels */
#define PIXEL_7680 (7680)  /**< 7680 pixels */
/** @brief Media debug timer interval in seconds */
#define MEDIA_DEBUG_TIMER_INTERVAL    (6)
#ifdef CONFIG_MEDIA_DEBUG_TIMER_ENABLE
/** @brief Media debug timer enable flag */
#define MEDIA_DEBUG_TIMER_ENABLE      (1)
#else
/** @brief Media debug timer disable flag */
#define MEDIA_DEBUG_TIMER_ENABLE      (0)
#endif

/**
 * @brief Define camera types
 * @{
 */


/**
 * @brief Media rotation modes
 * Defines the different methods of image rotation
 */
typedef enum{
    NONE_ROTATE = 0,  /**< No rotation mode */
    HW_ROTATE,        /**< Hardware-based rotation */
    SW_ROTATE,        /**< Software-based rotation */
}media_rotate_mode_t;
/**
 * @brief Media decoding modes
 * Defines the different methods of media decoding
 */
typedef enum
{
    NONE_DECODE = 0,              /**< No decoding */
    SOFTWARE_DECODING_MAJOR,      /**< Software decoding for major frames */
    SOFTWARE_DECODING_MINOR,      /**< Software decoding for minor frames */
    HARDWARE_DECODING,            /**< Hardware decoding */
    JPEGDEC_HW_MODE = HARDWARE_DECODING, /**< JPEG hardware decoding mode */
    JPEGDEC_SW_MODE               /**< JPEG software decoding mode */
} media_decode_mode_t;
/**
 * @brief Media decoding types
 * Defines how JPEG data is decoded
 */
typedef enum
{
    JPEGDEC_BY_LINE = 0,  /**< Decode JPEG data line by line */
    JPEGDEC_BY_FRAME,     /**< Decode JPEG data frame by frame */
} media_decode_type_t;
/**
 * @brief Image processing order
 * Defines the order in which image processing operations are performed
 */
typedef enum
{
    IMG_PROC_ROTATE_FIRST = 0,  /**< Rotate image first, then scale */
    IMG_PROC_SCALE_FIRST,       /**< Scale image first, then rotate */
}img_proc_order_t;
/**
 * @brief Media PPI (Pixels Per Inch) definitions
 * Defines various screen resolutions with width and height in pixels
 */
typedef enum
{
    PPI_DEFAULT     = 0,          /**< Default PPI setting */
    PPI_160X160     = (PIXEL_160 << 16) | PIXEL_160,   /**< 160x160 resolution */
    PPI_170X320     = (PIXEL_170 << 16) | PIXEL_320,   /**< 170x320 resolution */
    PPI_256X320     = (PIXEL_256 << 16) | PIXEL_320,   /**< 256x320 resolution */
    PPI_240X320     = (PIXEL_240 << 16) | PIXEL_320,   /**< 240x320 resolution */
    PPI_320X240     = (PIXEL_320 << 16) | PIXEL_240,   /**< 320x240 resolution */
    PPI_320X480     = (PIXEL_320 << 16) | PIXEL_480,   /**< 320x480 resolution */
    PPI_360X360     = (PIXEL_360 << 16) | PIXEL_360,   /**< 360x360 resolution */
    PPI_360X480     = (PIXEL_360 << 16) | PIXEL_480,   /**< 360x480 resolution */
    PPI_400X400     = (PIXEL_400 << 16) | PIXEL_400,   /**< 400x400 resolution */
    PPI_412X412     = (PIXEL_412 << 16) | PIXEL_412,   /**< 412x412 resolution */
    PPI_454X454     = (PIXEL_454 << 16) | PIXEL_454,   /**< 454x454 resolution */
    PPI_480X272     = (PIXEL_480 << 16) | PIXEL_272,   /**< 480x272 resolution */
    PPI_480X320     = (PIXEL_480 << 16) | PIXEL_320,   /**< 480x320 resolution */
    PPI_480X480     = (PIXEL_480 << 16) | PIXEL_480,   /**< 480x480 resolution */
    PPI_640X480     = (PIXEL_640 << 16) | PIXEL_480,   /**< 640x480 resolution */
    PPI_480X800     = (PIXEL_480 << 16) | PIXEL_800,   /**< 480x800 resolution */
    PPI_480X854     = (PIXEL_480 << 16) | PIXEL_854,   /**< 480x854 resolution */
    PPI_480X864     = (PIXEL_480 << 16) | PIXEL_864,   /**< 480x864 resolution */
    PPI_720X288     = (PIXEL_720 << 16) | PIXEL_288,   /**< 720x288 resolution */
    PPI_720X576     = (PIXEL_720 << 16) | PIXEL_576,   /**< 720x576 resolution */
    PPI_720X1280    = (PIXEL_720 << 16) | PIXEL_1280,  /**< 720x1280 resolution */
    PPI_854X480     = (PIXEL_854 << 16) | PIXEL_480,   /**< 854x480 resolution */
    PPI_800X480     = (PIXEL_800 << 16) | PIXEL_480,   /**< 800x480 resolution */
    PPI_864X480     = (PIXEL_864 << 16) | PIXEL_480,   /**< 864x480 resolution */
    PPI_960X480     = (PIXEL_960 << 16) | PIXEL_480,   /**< 960x480 resolution */
    PPI_800X600     = (PIXEL_800 << 16) | PIXEL_600,   /**< 800x600 resolution */
    PPI_1024X600    = (PIXEL_1024 << 16) | PIXEL_600,  /**< 1024x600 resolution */
    PPI_1280X720    = (PIXEL_1280 << 16) | PIXEL_720,  /**< 1280x720 resolution */
    PPI_1600X1200   = (PIXEL_1600 << 16) | PIXEL_1200, /**< 1600x1200 resolution */
    PPI_1920X1080   = (PIXEL_1920 << 16) | PIXEL_1080, /**< 1920x1080 resolution */
    PPI_2304X1296   = (PIXEL_2304 << 16) | PIXEL_1296, /**< 2304x1296 resolution */
    PPI_7680X4320   = (PIXEL_7680 << 16) | PIXEL_4320, /**< 7680x4320 resolution */
} media_ppi_t;
/**
 * @brief Media PPI capabilities
 * Defines the supported PPI capabilities as bit flags
 */
typedef enum
{
    PPI_CAP_UNKNOW      = 0,              /**< Unknown PPI capability */
    PPI_CAP_320X240     = (1 << 0),       /**< 320x240 resolution capability */
    PPI_CAP_320X480     = (1 << 1),       /**< 320x480 resolution capability */
    PPI_CAP_480X272     = (1 << 2),       /**< 480x272 resolution capability */
    PPI_CAP_480X320     = (1 << 3),       /**< 480x320 resolution capability */
    PPI_CAP_640X480     = (1 << 4),       /**< 640x480 resolution capability */
    PPI_CAP_480X800     = (1 << 5),       /**< 480x800 resolution capability */
    PPI_CAP_800X480     = (1 << 6),       /**< 800x480 resolution capability */
    PPI_CAP_800X600     = (1 << 7),       /**< 800x600 resolution capability */
    PPI_CAP_864X480     = (1 << 8),       /**< 864x480 resolution capability */
    PPI_CAP_1024X600    = (1 << 9),       /**< 1024x600 resolution capability */
    PPI_CAP_1280X720    = (1 << 10),      /**< 1280x720 resolution capability */
    PPI_CAP_1600X1200   = (1 << 11),      /**< 1600x1200 resolution capability */
    PPI_CAP_480X480     = (1 << 12),      /**< 480x480 resolution capability */
    PPI_CAP_720X288     = (1 << 13),      /**< 720x288 resolution capability */
    PPI_CAP_720X576     = (1 << 14),      /**< 720x576 resolution capability */
    PPI_CAP_480X854     = (1 << 15),      /**< 480x854 resolution capability */
    PPI_CAP_170X320     = (1 << 16),      /**< 170x320 resolution capability */
} media_ppi_cap_t;

/**
 * @brief Frame rate enumeration
 * Defines the supported frame rates as bit flags
 */
typedef enum
{
    FPS0    = 0,        /**< 0 FPS (no frame rate) */
    FPS5    = (1 << 0), /**< 5 FPS (frames per second) */
    FPS10   = (1 << 1), /**< 10 FPS (frames per second) */
    FPS15   = (1 << 2), /**< 15 FPS (frames per second) */
    FPS20   = (1 << 3), /**< 20 FPS (frames per second) */
    FPS25   = (1 << 4), /**< 25 FPS (frames per second) */
    FPS30   = (1 << 5), /**< 30 FPS (frames per second) */
} frame_fps_t;

/** @brief Camera handle type */
typedef void *camera_handle_t;

/**
 * @brief LCD configuration structure
 * Contains configuration parameters for LCD device initialization
 */
typedef struct {
    uint32_t device_ppi;     /**< Device PPI (Pixels Per Inch) for LCD initialization */
    char * device_name;      /**< LCD driver IC name for device initialization */
    const void * lcd_device; /**< LCD device handle */
    lcd_spi_io_t spi_io;     /**< SPI I/O configuration for LCD communication */
} lcd_config_t;
/** @brief Deprecated LCD open structure (use lcd_config_t instead) */
typedef lcd_config_t lcd_open_t __attribute__((deprecated("Use lcd_config_t instead")));
/**
 * @brief Rotation configuration structure
 * Contains configuration parameters for image rotation
 */
typedef struct {
    media_rotate_mode_t mode;  /**< Rotation mode (hardware or software) */
    rott_angle_t angle;      /**< Rotation angle */
    pixel_format_t fmt;        /**< Pixel format */
    uint8_t scale_en;          /**< Scale enable flag */
} rot_open_t;
/**
 * @brief LCD scaling structure
 * Contains scaling parameters for LCD display
 */
typedef struct {
    uint32_t src_ppi;  /**< Source PPI (width and height) */
    uint32_t dst_ppi;  /**< Destination PPI */
} lcd_scale_t;

/**
 * @brief Media debug structure
 * Contains debugging information for media processing
 */
typedef struct
{
    uint16_t isr_h264;      /**< H.264 frame count */
    uint16_t isr_decoder;   /**< JPEG decode frame count */
    uint16_t err_dec;       /**< JPEG decode error count */
    uint16_t isr_lcd;       /**< LCD display count */
    uint16_t fps_lcd;       /**< LCD display FPS */
    uint16_t lvgl_draw;     /**< LVGL draw frame count */
    uint16_t isr_rotate;    /**< LCD rotate frame count */
    uint16_t err_rot;       /**< LCD rotate error count */
    uint16_t fps_wifi;      /**< WiFi transfer FPS */
    uint32_t jpeg_length;   /**< JPEG frame length */
    uint32_t h264_length;   /**< H.264 frame length */
    uint32_t jpeg_kbps;     /**< JPEG data bit stream rate (Kbps) */
    uint32_t h264_kbps;     /**< H.264 data bit stream rate (Kbps) */
    uint32_t wifi_kbps;     /**< WiFi transfer data bit stream rate (Kbps) */
    uint32_t meantimes;     /**< WiFi transfer frame buffer times */
    uint16_t begin_trs : 1; /**< WiFi begin transfer a frame buffer */
    uint16_t end_trs : 1;   /**< WiFi end transfer a frame buffer */
} media_debug_t;
/**
 * @brief Get pixel width from PPI value
 * @param ppi PPI value containing width and height
 * @return Pixel width (high 16 bits of ppi)
 */
static inline uint16_t ppi_to_pixel_x(media_ppi_t ppi)
{
    return ppi >> 16;
}
/**
 * @brief Get pixel height from PPI value
 * @param ppi PPI value containing width and height
 * @return Pixel height (low 16 bits of ppi)
 */
static inline uint16_t ppi_to_pixel_y(media_ppi_t ppi)
{
    return ppi & 0xFFFF;
}
/**
 * @brief Get block pixel width from PPI value
 * @param ppi PPI value containing width and height
 * @return Block pixel width (high 16 bits of ppi divided by 8)
 */
static inline uint16_t ppi_to_pixel_x_block(media_ppi_t ppi)
{
    return (ppi >> 16) / 8;
}
/**
 * @brief Get block pixel height from PPI value
 * @param ppi PPI value containing width and height
 * @return Block pixel height (low 16 bits of ppi divided by 8)
 */
static inline uint16_t ppi_to_pixel_y_block(media_ppi_t ppi)
{
    return (ppi & 0xFFFF) / 8;
}
/**
 * @brief Get YUV422 image size from PPI value
 * @param ppi PPI value containing width and height
 * @return YUV422 image size in bytes (width * height * 2)
 */
static inline uint32_t get_ppi_size(media_ppi_t ppi)
{
    return (ppi >> 16) * (ppi & 0xFFFF) * 2;
}
/*
 * @}
 */
#ifdef __cplusplus
}
#endif
