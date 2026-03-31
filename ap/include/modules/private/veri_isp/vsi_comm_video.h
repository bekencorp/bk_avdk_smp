/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2024 Vivante Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __VSI_COMM_VIDEO_H__
#define __VSI_COMM_VIDEO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include <modules/veri_isp/vsios_type.h>

/**
 * @defgroup vsi_comm_video Mpp Video Common Definitions
 * @{
 *
 *
 */

#define VIDEO_MAX_PLANES 8  /**< \brief The maximum number of planes for videos.*/

/*****************************************************************************/
/**
 * @brief   This structure contains the size parameters.
 *
 *****************************************************************************/
typedef struct vsiSIZE_S {
    vsi_u32_t width;     /**< \brief Width.*/
    vsi_u32_t height;    /**< \brief Height.*/
} SIZE_S;

/*****************************************************************************/
/**
 * @brief   This structure contains the rectangle parameters.
 *
 *****************************************************************************/
typedef struct vsiRECT_S {
    vsi_u32_t top;       /**< \brief The top position of the rectangle.*/
    vsi_u32_t left;      /**< \brief The left poistion of the rectangle.*/
    vsi_u32_t width;     /**< \brief The width of the rectangle.*/
    vsi_u32_t height;    /**< \brief The height of the rectangle.*/
} RECT_S;

/*****************************************************************************/
/**
 * @brief   This structure contains the pixel formats.
 *
 *****************************************************************************/
typedef enum vsiPIXEL_FORMAT_E {
    PIXEL_FORMAT_BGGR8 = 0, /**< \brief BGGR 8-bit. */
    PIXEL_FORMAT_GBRG8 = 1,     /**< \brief GBRG 8-bit. */
    PIXEL_FORMAT_GRBG8 = 2,     /**< \brief GRBG 8-bit. */
    PIXEL_FORMAT_RGGB8 = 3,     /**< \brief RGGB 8-bit. */

    PIXEL_FORMAT_BGGR10 = 4,    /**< \brief BGGR 10-bit. */
    PIXEL_FORMAT_GBRG10 = 5,    /**< \brief GBRG 10-bit. */
    PIXEL_FORMAT_GRBG10 = 6,    /**< \brief GRBG 10-bit. */
    PIXEL_FORMAT_RGGB10 = 7,    /**< \brief RGGB 10-bit. */

    PIXEL_FORMAT_BGGR12 = 8,    /**< \brief BGGR 12-bit. */
    PIXEL_FORMAT_GBRG12 = 9,    /**< \brief GBRG 12-bit. */
    PIXEL_FORMAT_GRBG12 = 10,   /**< \brief GRBG 12-bit. */
    PIXEL_FORMAT_RGGB12 = 11,   /**< \brief RGGB 12-bit. */

    PIXEL_FORMAT_BGGR14 = 12,   /**< \brief BGGR 14-bit. */
    PIXEL_FORMAT_GBRG14 = 13,   /**< \brief GBRG 14-bit. */
    PIXEL_FORMAT_GRBG14 = 14,   /**< \brief GRBG 14-bit. */
    PIXEL_FORMAT_RGGB14 = 15,   /**< \brief RGGB 14-bit. */

    PIXEL_FORMAT_BGGR16 = 16,   /**< \brief BGGR 16-bit. */
    PIXEL_FORMAT_GBRG16 = 17,   /**< \brief GBRG 16-bit. */
    PIXEL_FORMAT_GRBG16 = 18,   /**< \brief GRBG 16-bit. */
    PIXEL_FORMAT_RGGB16 = 19,   /**< \brief RGGB 16-bit. */

    PIXEL_FORMAT_RAW8 = 20,     /**< \brief Raw 8-bit. */
    PIXEL_FORMAT_RAW10 = 21,    /**< \brief Raw 10-bit. */
    PIXEL_FORMAT_RAW12 = 22,    /**< \brief Raw 12-bit */

    PIXEL_FORMAT_NV12 = 23,     /**< \brief YUV420sp format, y0, y1, y2, y3, y4, y5, y6, y7, u1, v1, u2, v2.*/
    PIXEL_FORMAT_NV21 = 24,     /**< \brief YUV420sp format, y0, y1, y2, y3, y4, y5, y6, y7, v1, u1, v2, u2.*/
    PIXEL_FORMAT_NV16 = 25,     /**< \brief YUV422sp format, y0, y1, y2, y3, u1, v1, u2, v2.*/
    PIXEL_FORMAT_NV61 = 26,     /**< \brief YUV422sp format, y0, y1, y2, y3, v1, u1, v2, u2.*/
    PIXEL_FORMAT_NV24 = 27,     /**< \brief YUV444sp format, y0, y1, u1, v2, u2, v2.*/
    PIXEL_FORMAT_NV42 = 28,     /**< \brief YUV444sp format, y0, y1, v1, u1, v2, u2.*/

    PIXEL_FORMAT_YUV444P = 29,   /**< \brief YUV444 planer format. */
    PIXEL_FORMAT_YUV422P = 30,   /**< \brief YUV422 planer format. */
    PIXEL_FORMAT_YUV420P = 31,   /**< \brief YUV420 planer format. */

    PIXEL_FORMAT_YUYV = 32,      /**< \brief YUV422 package format. */
    PIXEL_FORMAT_VYUY = 33,      /**< \brief YUV422 package format. */
    PIXEL_FORMAT_UYVY = 34,      /**< \brief YUV422 package format. */
    PIXEL_FORMAT_YYUV = 35,      /**< \brief YUV422 package format. */
    PIXEL_FORMAT_YUV444I = 36,    /**< \brief YUV444 package format. */

    PIXEL_FORMAT_YUV400 = 37,    /**< \brief YUV400 Y-only format. */

    PIXEL_FORMAT_RGB888 = 38,    /**< \brief RGB888 interleaved format. */
    PIXEL_FORMAT_RGB888P = 39,   /**< \brief RGB888 planar format. */
    PIXEL_FORMAT_RGB565 = 40,

    PIXEL_FORMAT_RAW420SP = 41,  /**< \brief RAW420 semi-planer format. */
    PIXEL_FORMAT_RAW422SP = 42,  /**< \brief RAW422 semi-planer format. */
    PIXEL_FORMAT_MAX,
} PIXEL_FORMAT_E;

/*****************************************************************************/
/**
 * @brief   Plane format structure.
 *
 *****************************************************************************/
typedef struct vsiPLANE_FORMAT_S {
    vsi_u32_t size;          /**< \brief Plane format size.*/
    vsi_u32_t bytesPerLine;  /**< \brief The number of bytes per line.*/
} PLANE_FORMAT_S;

/*****************************************************************************/
/**
 * @brief   This structure contains the picture format.
 *
 *****************************************************************************/
typedef struct vsiFORMAT_S {
    vsi_u32_t      width;                    /**< \brief The image width.*/
    vsi_u32_t      height;                   /**< \brief The image height.*/
    vsi_u32_t      imageSize;                /**< \brief The image size.*/
    vsi_u32_t      pixelFormat;               /**< \brief The pixel format.*/
    vsi_u8_t       numPlanes;                 /**< \brief The number of planes.*/
    PLANE_FORMAT_S planeFmt[VIDEO_MAX_PLANES];/**< \brief The plane format.*/
    vsi_u8_t       align;		      /**< \brief The image align format.*/
    vsi_u32_t      base;		      /**< \brief Rdma channel phy address.*/
    char           buf[0];		      /**< \brief Rdma channel raw image buf*/
} FORMAT_S;

/*****************************************************************************/
/**
 * @brief   This structure contains the video buffer planes.
 *
 *****************************************************************************/
typedef struct vsiVB_PLANE_S {
    vsi_dma_t dmaPhyAddr;   /**< \brief DMA physical address.*/
    vsi_u32_t size;      /**< \brief Buffer size.*/
    void      *pUserAddr;   /**< \brief User address.*/
} VB_PLANE_S;

/*****************************************************************************/
/**
 * @brief   This structure contains the video buffer information.
 *
 *****************************************************************************/
typedef struct vsiVIDEO_BUF_S{
    vsi_u32_t index;     /**< \brief The index.*/
    vsi_u32_t imageSize; /**< \brief The image size.*/
    vsi_u64_t timeStamp; /**< \brief The time stamp.*/
    vsi_u8_t  numPlanes;  /**< \brief The number of planes.*/
    VB_PLANE_S planes[VIDEO_MAX_PLANES];  /**< \brief Video buffer configuration.*/
} VIDEO_BUF_S;

/* @} vsi_comm_video */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
