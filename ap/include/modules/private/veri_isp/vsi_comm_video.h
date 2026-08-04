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
  * @defgroup vsi_comm_video Video Definitions
  * @{
  *
  *
  */
 
 #define VIDEO_MAX_PLANES 8  /**< \brief The maximum number of planes for video formats. */
 
 /*****************************************************************************/
 /**
  * @brief Specifies a size.
  *
  *****************************************************************************/
 typedef struct vsiSIZE_S {
     vsi_u32_t width;     /**< \brief The width. */
     vsi_u32_t height;    /**< \brief The height. */
 } SIZE_S;
 
 /*****************************************************************************/
 /**
  * @brief Specifies a rectangle.
  *
  *****************************************************************************/
 typedef struct vsiRECT_S {
     vsi_u32_t top;       /**< \brief The top border of the rectangle. */
     vsi_u32_t left;      /**< \brief The left border of the rectangle. */
     vsi_u32_t width;     /**< \brief The width of the rectangle. */
     vsi_u32_t height;    /**< \brief The height of the rectangle. */
 } RECT_S;
 
 /*****************************************************************************/
 /**
  * @brief Defines the pixel data formats.
  *
  *****************************************************************************/
 typedef enum vsiPIXEL_FORMAT_E {
     PIXEL_FORMAT_BGGR8 = 0, /**< \brief BGGR 8-bit. */
     PIXEL_FORMAT_GBRG8,     /**< \brief GBRG 8-bit. */
     PIXEL_FORMAT_GRBG8,     /**< \brief GRBG 8-bit. */
     PIXEL_FORMAT_RGGB8,     /**< \brief RGGB 8-bit. */
 
     PIXEL_FORMAT_BGGR10,    /**< \brief BGGR 10-bit. */
     PIXEL_FORMAT_GBRG10,    /**< \brief GBRG 10-bit. */
     PIXEL_FORMAT_GRBG10,    /**< \brief GRBG 10-bit. */
     PIXEL_FORMAT_RGGB10,    /**< \brief RGGB 10-bit. */
 
     PIXEL_FORMAT_BGGR12,    /**< \brief BGGR 12-bit. */
     PIXEL_FORMAT_GBRG12,    /**< \brief GBRG 12-bit. */
     PIXEL_FORMAT_GRBG12,   /**< \brief GRBG 12-bit. */
     PIXEL_FORMAT_RGGB12,   /**< \brief RGGB 12-bit. */
 
     PIXEL_FORMAT_BGGR14,   /**< \brief BGGR 14-bit. */
     PIXEL_FORMAT_GBRG14,   /**< \brief GBRG 14-bit. */
     PIXEL_FORMAT_GRBG14,   /**< \brief GRBG 14-bit. */
     PIXEL_FORMAT_RGGB14,   /**< \brief RGGB 14-bit. */
 
     PIXEL_FORMAT_BGGR16,   /**< \brief BGGR 16-bit. */
     PIXEL_FORMAT_GBRG16,   /**< \brief GBRG 16-bit. */
     PIXEL_FORMAT_GRBG16,   /**< \brief GRBG 16-bit. */
     PIXEL_FORMAT_RGGB16,   /**< \brief RGGB 16-bit. */
 
     PIXEL_FORMAT_RAW8,     /**< \brief Raw 8-bit. */
     PIXEL_FORMAT_RAW10,    /**< \brief Raw 10-bit. */
     PIXEL_FORMAT_RAW12,    /**< \brief Raw 12-bit */
     PIXEL_FORMAT_RAW14,    /**< \brief Raw 14-bit */
     PIXEL_FORMAT_RAW16,    /**< \brief Raw 16-bit */
 
     PIXEL_FORMAT_NV12,     /**< \brief YUV420 semi-planar format: y0, y1, y2, y3, y4, y5, y6, y7; u1, v1, u2, v2.*/
     PIXEL_FORMAT_NV21,     /**< \brief YUV420 semi-planar format: y0, y1, y2, y3, y4, y5, y6, y7; v1, u1, v2, u2.*/
     PIXEL_FORMAT_NV16,     /**< \brief YUV422 semi-planar format: y0, y1, y2, y3; u1, v1, u2, v2.*/
     PIXEL_FORMAT_NV61,     /**< \brief YUV422 semi-planar format: y0, y1, y2, y3; v1, u1, v2, u2.*/
     PIXEL_FORMAT_NV24,     /**< \brief YUV444 semi-planar format: y0, y1; u1, v2, u2, v2.*/
     PIXEL_FORMAT_NV42,     /**< \brief YUV444 semi-planar format: y0, y1; v1, u1, v2, u2.*/
 
     PIXEL_FORMAT_YUV444P,   /**< \brief YUV444 planar format. */
     PIXEL_FORMAT_YUV422P,   /**< \brief YUV422 planar format. */
     PIXEL_FORMAT_YUV420P,   /**< \brief YUV420 planar format. */
 
     PIXEL_FORMAT_YUYV,      /**< \brief YUV422 packed format. */
     PIXEL_FORMAT_VYUY,      /**< \brief YUV422 packed format. */
     PIXEL_FORMAT_UYVY,      /**< \brief YUV422 packed format. */
     PIXEL_FORMAT_YYUV,      /**< \brief YUV422 packed format. */
     PIXEL_FORMAT_YUV444I,    /**< \brief YUV444 packed format. */
 
     PIXEL_FORMAT_YUV400,    /**< \brief YUV400 monochrome Y-only format. */
 
     PIXEL_FORMAT_NV12_10BIT,    /**< \brief YUV420 semi-planar format: y0, y1, y2, y3, y4, y5, y6, y7; u1, v1, u2, v2.*/
     PIXEL_FORMAT_NV16_10BIT,    /**< \brief YUV422 semi-planar format: y0, y1, y2, y3; u1, v1, u2, v2.*/
     PIXEL_FORMAT_YUYV_10BIT,    /**< \brief YUV422 packed format. */
     PIXEL_FORMAT_YUV444I_10BIT, /**< \brief YUV444 packed format. */
 
     PIXEL_FORMAT_RGB888,    /**< \brief RGB888 interleaved format. */
     PIXEL_FORMAT_RGB888P,   /**< \brief RGB888 planar format. */
 
     PIXEL_FORMAT_RAW420SP,  /**< \brief RAW420 semi-planar format. Take point directly when RAW to 420SP. */
     PIXEL_FORMAT_RAW422SP,  /**< \brief RAW422 semi-planar format. Take point directly when RAW to 422SP. */
     PIXEL_FORMAT_RAW420SP_1,  /**< \brief RAW420 semi-planar format. Even rows take points on average when RAW to 420SP. */
     PIXEL_FORMAT_RAW422SP_1,  /**< \brief RAW422 semi-planar format. Even rows take points on average when RAW to 420SP. */
     PIXEL_FORMAT_MAX,
 } PIXEL_FORMAT_E;
 
 /*****************************************************************************/
 /**
  * @brief Specifies the format of a plane.
  *
  *****************************************************************************/
 typedef struct vsiPLANE_FORMAT_S {
     vsi_u32_t size;          /**< \brief The format size of the plane. */
     vsi_u32_t bytesPerLine;  /**< \brief The number of bytes per line in the plane. */
     vsi_u32_t lineCnt;       /**< \brief The number of lines per plane. */
 } PLANE_FORMAT_S;
 
 /*****************************************************************************/
 /**
  * @brief Defines the MI data alignment modes.
  *
  *****************************************************************************/
 typedef enum vsiMI_DATA_ALIGN_MODE_E {
     MI_DATA_ALIGN_MODE_INVALID = -1, /* (Reserved) Invalid. */
     MI_DATA_UNALIGN_MODE = 0,        /* Does not align pixel data. */
     MI_DATA_ALIGN_MODE0 = 1,         /* Aligns raw pixel data. Aligns RAW10 with double words. Aligns RAW12 with quad words. */
     MI_DATA_ALIGN_DOUBLE_WORD = 1,   /* Aligns YUV pixel data with double words. */
     MI_DATA_ALIGN_WORD = 2,          /* Aligns YUV pixel data with words. */
     MI_DATA_ALIGN_MODE1 = 2,         /* Aligns raw pixel data with 16 bits. */
     MI_DATA_ALIGN_MODE_MAX           /* (Reserved) The number of MI data alignment modes. */
 } MI_DATA_ALIGN_MODE_E;
 
 /*****************************************************************************/
 /**
  * @brief Contains the picture format.
  *
  *****************************************************************************/
 typedef struct vsiFORMAT_S {
     vsi_u32_t      width;                     /**< \brief The image width. */
     vsi_u32_t      height;                    /**< \brief The image height. */
     vsi_u32_t      imageSize;                 /**< \brief The image size. */
     vsi_u32_t      pixelFormat;               /**< \brief The pixel format. */
     vsi_u8_t       numPlanes;                 /**< \brief The number of planes. */
     PLANE_FORMAT_S planeFmt[VIDEO_MAX_PLANES];/**< \brief The plane format. */
     vsi_u8_t       align;              /**< \brief The image alignment, in bytes. */
     vsi_dma_t      base;               /**< \brief The physical address of the RDMA channel. */
     vsi_u16_t      rdmaFrameNum;           /**< \brief The number of frames in the RDMA channel. */
     vsi_u16_t      outFrameNum;            /**< \brief The number of the ISP out frame.
                                                 \n Valid values:
                                                 \n - 0: output continuously.
                                                 \n - n: output n frames. */
     char           buf[0];             /**< \brief The raw image buffer of the RDMA channel. */
 } FORMAT_S;
 
 /*****************************************************************************/
 /**
  * @brief Contains the configurations of a plane.
  *
  *****************************************************************************/
 typedef struct vsiVB_PLANE_S {
     vsi_dma_t dmaPhyAddr;   /**< \brief The DMA physical address of the plane. */
     vsi_u32_t size;         /**< \brief The buffer size of the plane. */
     void      *pUserAddr;   /**< \brief The user address of the plane. */
 } VB_PLANE_S;
 
 /*****************************************************************************/
 /**
  * @brief Contains the information of a video buffer.
  *
  *****************************************************************************/
 typedef struct vsiVIDEO_BUF_S{
     vsi_u32_t index;     /**< \brief The index of the buffer. */
     vsi_u32_t imageSize; /**< \brief The image size in the buffer. */
     vsi_u64_t timeStamp; /**< \brief The timestamp of the buffer.*/
     vsi_u8_t  numPlanes; /**< \brief The number of planes in the buffer. */
     VB_PLANE_S planes[VIDEO_MAX_PLANES];  /**< \brief The configurations of each plane in the buffer. */
 } VIDEO_BUF_S;
 
 /* @} vsi_comm_video */
 /* @endcond */
 
 #ifdef __cplusplus
 #if __cplusplus
 }
 #endif
 #endif
 
 #endif
 