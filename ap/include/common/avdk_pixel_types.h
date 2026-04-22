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

typedef enum {
    BK_PIXEL_FORMAT_UNKNOW,         /**< unknow image format */

    /* Pixel format for RAW8 */
    BK_PIXEL_FORMAT_BGGR8,
    BK_PIXEL_FORMAT_GBRG8,
    BK_PIXEL_FORMAT_GRBG8,
    BK_PIXEL_FORMAT_RGGB8,
    BK_PIXEL_FORMAT_RAW8,

    /* Pixel format for RAW10 */
    BK_PIXEL_FORMAT_BGGR10,
    BK_PIXEL_FORMAT_GBRG10,
    BK_PIXEL_FORMAT_GRBG10,
    BK_PIXEL_FORMAT_RGGB10,
    BK_PIXEL_FORMAT_RAW10,
    /* Pixel format for RGB565 */
    BK_PIXEL_FORMAT_RGB565,
    BK_PIXEL_FORMAT_BGR565,

    /* Pixel format for ARGB565 */
    BK_PIXEL_FORMAT_ARGB8565,
    BK_PIXEL_FORMAT_ABGR8565,
    BK_PIXEL_FORMAT_RGBA5658,
    BK_PIXEL_FORMAT_BGRA5658, 

    /* Pixel format for RGB888 */
    BK_PIXEL_FORMAT_RGB888,
    BK_PIXEL_FORMAT_BGR888,

    /* Pixel format for ARGB888 */
    BK_PIXEL_FORMAT_ARGB8888,
    BK_PIXEL_FORMAT_ABGR8888,
    BK_PIXEL_FORMAT_RGBA8888,
    BK_PIXEL_FORMAT_BGRA8888,

    /* Pixel format for YUV */
    BK_PIXEL_FORMAT_NV12,        /**< \brief Yuv420sp format, y0, y1, y2, y3, y4, y5, y6, y7, u1, v1, u2, v2.*/
    BK_PIXEL_FORMAT_NV21,        /**< \brief Yuv420sp format, y0, y1, y2, y3, y4, y5, y6, y7, v1, u1, v2, u2.*/
    BK_PIXEL_FORMAT_YUYV,        /**< \brief YUV422 package format. */
    BK_PIXEL_FORMAT_VYUY,        /**< \brief YUV422 package format. */
    BK_PIXEL_FORMAT_UYVY,        /**< \brief YUV422 package format. */
    BK_PIXEL_FORMAT_YYUV,        /**< \brief YUV422 package format. */
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


