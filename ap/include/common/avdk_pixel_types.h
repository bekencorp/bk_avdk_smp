// Copyright 2023-2024 Beken
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


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BK_PIXEL_FORMAT_INVALID	(0xFFFFFFFF)


#define PIXEL_170   (170)
#define PIXEL_240   (240)
#define PIXEL_272   (272)
#define PIXEL_288   (288)
#define PIXEL_320   (320)
#define PIXEL_360   (360)
#define PIXEL_390   (390)
#define PIXEL_400   (400)
#define PIXEL_412   (412)
#define PIXEL_432   (432)
#define PIXEL_454   (454)
#define PIXEL_480   (480)
#define PIXEL_576   (576)
#define PIXEL_600   (600)
#define PIXEL_640   (640)
#define PIXEL_720   (720)
#define PIXEL_800   (800)
#define PIXEL_854   (854)
#define PIXEL_864   (864)
#define PIXEL_960   (960)
#define PIXEL_1024 (1024)
#define PIXEL_1080 (1080)
#define PIXEL_1200 (1200)
#define PIXEL_1280 (1280)
#define PIXEL_1600 (1600)
#define PIXEL_1920 (1920)

typedef enum {
    BK_IMAGE_FORMAT_UNKNOW,
    BK_IMAGE_FORMAT_MJPEG,
    BK_IMAGE_FORMAT_H264,
    BK_IMAGE_FORMAT_H265,
    BK_IMAGE_FORMAT_PNG,
    BK_IMAGE_FORMAT_YUV,
} bk_image_format_t;

// need optimize
typedef enum
{
    IMAGE_UNKNOW = 0,
    IMAGE_YUV    = (1 << 0),
    IMAGE_RGB    = (1 << 1),
    IMAGE_MJPEG  = (1 << 2),
    IMAGE_H264   = (1 << 3),
    IMAGE_H265   = (1 << 4),
} image_format_t;

/**
 * @brief Pixel format identifiers.
 *
 * Naming / byte-order convention used in this enum:
 *   - The characters in the name are read LEFT -> RIGHT in increasing memory
 *     address order (i.e. byte-order naming, the same convention used by
 *     OpenGL/Vulkan and most LCD/DPU controllers, NOT the "high-bit-first
 *     32-bit integer" convention).
 *   - On a little-endian CPU (BK series), packing the four characters of e.g.
 *     ARGB8888 into a uint32_t directly will give a different byte layout in
 *     memory; always think in terms of bytes when interfacing with hardware.
 *   - Numeric values are kept in the original (implicit) order so existing
 *     switch/case code is unaffected.
 */
typedef enum {
    BK_PIXEL_FORMAT_UNKNOW       = 0,    /**< unknown / invalid format */

    /* =========================================================================
     *  RAW8 - Bayer mosaic, 1 byte per pixel.
     *  The four characters describe the colour of each cell in a 2x2 block:
     *      +----+----+
     *      | C0 | C1 |   row 0  (top)
     *      +----+----+
     *      | C2 | C3 |   row 1  (bottom)
     *      +----+----+
     *  Bytes in memory go row-major: C0, C1, ..., next row, ...
     * ========================================================================= */
    BK_PIXEL_FORMAT_BGGR8        = 1,    /**< 2x2: B G / G R   (8-bit per cell) */
    BK_PIXEL_FORMAT_GBRG8        = 2,    /**< 2x2: G B / R G   (8-bit per cell) */
    BK_PIXEL_FORMAT_GRBG8        = 3,    /**< 2x2: G R / B G   (8-bit per cell) */
    BK_PIXEL_FORMAT_RGGB8        = 4,    /**< 2x2: R G / G B   (8-bit per cell) */
    BK_PIXEL_FORMAT_RAW8         = 5,    /**< Generic 8-bit raw, pattern unspecified */

    /* =========================================================================
     *  RAW10 - same Bayer mosaic as above but each cell is 10-bit, packed in
     *  2 bytes (LSB-aligned in a uint16_t). 2 bytes per pixel.
     * ========================================================================= */
    BK_PIXEL_FORMAT_BGGR10       = 6,    /**< 2x2: B G / G R   (10-bit per cell) */
    BK_PIXEL_FORMAT_GBRG10       = 7,    /**< 2x2: G B / R G   (10-bit per cell) */
    BK_PIXEL_FORMAT_GRBG10       = 8,    /**< 2x2: G R / B G   (10-bit per cell) */
    BK_PIXEL_FORMAT_RGGB10       = 9,    /**< 2x2: R G / G B   (10-bit per cell) */
    BK_PIXEL_FORMAT_RAW10        = 10,   /**< Generic 10-bit raw, pattern unspecified */

    /* =========================================================================
     *  RGB565 - 2 bytes per pixel, no alpha. Bit layout inside the 16-bit
     *  pixel value (MSB ... LSB):
     *      RGB565: [15:11]=R5 [10:5]=G6 [4:0]=B5
     *      BGR565: [15:11]=B5 [10:5]=G6 [4:0]=R5
     *  Stored little-endian in memory: low 8 bits at +0, high 8 bits at +1.
     * ========================================================================= */
    BK_PIXEL_FORMAT_RGB565       = 11,   /**< 16-bit:  R5 G6 B5  (high -> low) */
    BK_PIXEL_FORMAT_BGR565       = 12,   /**< 16-bit:  B5 G6 R5  (high -> low) */

    /* =========================================================================
     *  RGB565 + 8-bit Alpha - 3 bytes per pixel. Memory order (low addr first):
     *      ARGB8565:  [+0]=A8     [+1..+2]=RGB565
     *      ABGR8565:  [+0]=A8     [+1..+2]=BGR565
     *      RGBA5658:  [+0..+1]=RGB565   [+2]=A8
     *      BGRA5658:  [+0..+1]=BGR565   [+2]=A8
     * ========================================================================= */
    BK_PIXEL_FORMAT_ARGB8565     = 13,   /**< +0=A   +1..+2=RGB565 */
    BK_PIXEL_FORMAT_ABGR8565     = 14,   /**< +0=A   +1..+2=BGR565 */
    BK_PIXEL_FORMAT_RGBA5658     = 15,   /**< +0..+1=RGB565  +2=A  */
    BK_PIXEL_FORMAT_BGRA5658     = 16,   /**< +0..+1=BGR565  +2=A  */

    /* =========================================================================
     *  RGB888 - 3 bytes per pixel, no alpha. Memory order (low addr first):
     *      RGB888:  +0=R  +1=G  +2=B
     *      BGR888:  +0=B  +1=G  +2=R
     * ========================================================================= */
    BK_PIXEL_FORMAT_RGB888       = 17,   /**< +0=R  +1=G  +2=B */
    BK_PIXEL_FORMAT_BGR888       = 18,   /**< +0=B  +1=G  +2=R */

    /* =========================================================================
     *  RGB888 + 8-bit Alpha - 4 bytes per pixel. Memory order (low addr first).
     *
     *  Example for an opaque red pixel (A=0xFF, R=0xFF, G=0x00, B=0x00):
     *
     *           +0    +1    +2    +3
     *           +-----+-----+-----+-----+
     *  ARGB8888 | FF  | FF  | 00  | 00  |   (+0=A +1=R +2=G +3=B)
     *           +-----+-----+-----+-----+
     *  ABGR8888 | FF  | 00  | 00  | FF  |   (+0=A +1=B +2=G +3=R)
     *           +-----+-----+-----+-----+
     *  RGBA8888 | FF  | 00  | 00  | FF  |   (+0=R +1=G +2=B +3=A)
     *           +-----+-----+-----+-----+
     *  BGRA8888 | 00  | 00  | FF  | FF  |   (+0=B +1=G +2=R +3=A)
     *           +-----+-----+-----+-----+
     *
     *  CPU-pack helpers (little-endian, so the lowest byte ends up at +0):
     *      ARGB8888:  pixel = (B<<24) | (G<<16) | (R<<8) | A
     *      ABGR8888:  pixel = (R<<24) | (G<<16) | (B<<8) | A
     *      RGBA8888:  pixel = (A<<24) | (B<<16) | (G<<8) | R
     *      BGRA8888:  pixel = (A<<24) | (R<<16) | (G<<8) | B
     *
     *  Note: ARGB <-> BGRA and RGBA <-> ABGR are byte-reverses of each other.
     * ========================================================================= */
    BK_PIXEL_FORMAT_ARGB8888     = 19,   /**< +0=A  +1=R  +2=G  +3=B */
    BK_PIXEL_FORMAT_ABGR8888     = 20,   /**< +0=A  +1=B  +2=G  +3=R */
    BK_PIXEL_FORMAT_RGBA8888     = 21,   /**< +0=R  +1=G  +2=B  +3=A */
    BK_PIXEL_FORMAT_BGRA8888     = 22,   /**< +0=B  +1=G  +2=R  +3=A */

    /* =========================================================================
     *  YUV
     *
     *  NV12 / NV21 are PLANAR 4:2:0 (1.5 bytes per pixel):
     *      [ Y plane: width*height bytes                  ]
     *      [ UV plane: width*height/2 bytes, interleaved  ]
     *
     *  YUYV / VYUY / UYVY / YYUV are PACKED 4:2:2 (2 bytes per pixel). The
     *  four characters give the byte order for every pair of adjacent pixels
     *  (occupying 4 bytes total).
     * ========================================================================= */
    BK_PIXEL_FORMAT_NV12         = 23,   /**< 4:2:0 planar; UV-interleaved (U V U V ...) after Y plane */
    BK_PIXEL_FORMAT_NV21         = 24,   /**< 4:2:0 planar; VU-interleaved (V U V U ...) after Y plane */
    BK_PIXEL_FORMAT_YUYV         = 25,   /**< 4:2:2 packed; bytes per 2 pixels:  Y0  U   Y1  V  */
    BK_PIXEL_FORMAT_VYUY         = 26,   /**< 4:2:2 packed; bytes per 2 pixels:  V   Y0  U   Y1 */
    BK_PIXEL_FORMAT_UYVY         = 27,   /**< 4:2:2 packed; bytes per 2 pixels:  U   Y0  V   Y1 */
    BK_PIXEL_FORMAT_YYUV         = 28,   /**< 4:2:2 packed; bytes per 2 pixels:  Y0  Y1  U   V  */
} bk_pixel_format_t;

// need optimize
typedef enum {
    PIXEL_FMT_UNKNOW,         /**< unknow image format */
    PIXEL_FMT_JPEG,           /**< image foramt jpeg */
    PIXEL_FMT_H264,
    PIXEL_FMT_H265,
    PIXEL_FMT_YUV444,
    PIXEL_FMT_YUYV,  /**< lcd/jpeg_decode support */
    PIXEL_FMT_VYUY,  /**< jpeg_decode support */
    PIXEL_FMT_UYVY,
    PIXEL_FMT_YYUV,   /**< jpeg_decode support */
    PIXEL_FMT_VUYY,   /**< jpeg_decode support */
    PIXEL_FMT_UVYY,
    PIXEL_FMT_YUV422,
    PIXEL_FMT_I420,
    PIXEL_FMT_YV12,
    PIEXL_FMT_YUV420P,
    PIXEL_FMT_NV12,
    PIXEL_FMT_NV21,
    PIXEL_FMT_YUV420SP,
    PIXEL_FMT_YUV420,
    PIXEL_FMT_RGB444,
    PIXEL_FMT_RGB555,
    PIXEL_FMT_RGB565,     /**< input data format is rgb565(big endian), high pixel is bit[31-16], low pixel is bit[15-0] (PIXEL BIG ENDIAN)*/
    PIXEL_FMT_RGB565_LE,  /**< input data format is rgb565(big endian), high pixel is bit[15-0], low pixel is bit[31-16] (PIXEL little ENDIAN)*/
    PIXEL_FMT_BGR565,
    PIXEL_FMT_RGB666,
    PIXEL_FMT_RGB888,
    PIXEL_FMT_BGR888,
    PIXEL_FMT_ARGB8888,
    PIXEL_FMT_GRAY,
    PIXEL_FMT_RAW,
    PIXEL_FMT_PNG,
} pixel_format_t;

typedef struct {
    uint16_t top;       /**< \brief Rectange top poistion.*/
    uint16_t left;      /**< \brief Rectange left poistion.*/
    uint16_t width;     /**< \brief Rectange width.*/
    uint16_t height;    /**< \brief Rectange height.*/
} bk_rect_t;


typedef enum {
    ROTATE_NONE = 0, /**< no rotate */
    ROTATE_90, /**< Image rotaged 90 degress*/
    ROTATE_180, /**< No support yet, reserved for the future*/
    ROTATE_270, /**< Image rotaged 270 degress*/
} rott_angle_t;




typedef struct frame_buffer_t frame_buffer_t;

typedef struct
{
    void (*free)(frame_buffer_t *frame_buffer);
} frame_buffer_callback_t;


typedef void (*frame_cb_t)(frame_buffer_t *frame);

/**
 * @brief Frame buffer structure
 * Contains all information about a video frame buffer
 * @{
 */
struct frame_buffer_t
{
    uint32_t fmt;
    uint8_t *frame;
    uint32_t timestamp;
    uint16_t width;
    uint16_t height;
    uint32_t length;
    uint32_t size;
    uint32_t sequence;
    uint32_t h264_type;
    frame_buffer_callback_t *cb;
};

uint32_t bk_image_size_get(uint16_t width, uint16_t height, bk_pixel_format_t format);
uint32_t bk_pixel_size_get(bk_pixel_format_t format);

#ifdef __cplusplus
}
#endif


