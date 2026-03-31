/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2022 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--                                                                            --
--  Abstract : API of heif writer                                             --
--                                                                            --
------------------------------------------------------------------------------*/

#ifndef VCENC_HEIF_WRITER_H
#define VCENC_HEIF_WRITER_H

#ifdef HEIF_SUPPORT

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_heif_vce HEIF Writer API
 *
 * @{
 */

/** \brief The HEIF writer handle. */
typedef const void *VCEncHeifWriterInst;

/** Defines return values of HEIF writer APIs. */
typedef enum {
  /** The API call is successful. */
  HEIF_OK = 0,
  /** An error occurs in API execution. */
  HEIF_ERROR = -1,
  /** There is no more image in the file. */
  HEIF_END = -2,
  /** There is not enough memory. */
  HEIF_MEMFAIL = -3,
  HEIF_UNSUPPORTED = -4,
} VCEncHeifRet;


/** Defines media data types. */
typedef enum {
  /** Heif image. */
  IsHevcImage = 0,
  /** Heif image sequence. */
  IsHevcSequence = 1,
  /** Decoder configuration for hevc file content. */
  IsHevcDecHeader = 2,

  /** Avif image */
  IsAv1Image = 3,
  /** Avif image sequence*/
  IsAv1Sequence = 4,
  /** Decoder configuration for av1 file content. */
  IsAv1DecHeader = 5
} VCEncHeifDecInfoType;

/** Defines auxiliary image types. */
typedef enum {
  /** Alpha plane. */
  alpha,
  /** Depth map. */
  depth
} VCEncHeifAuxiliaryType;

/** Defines mirroring types. */
typedef enum {
  /** Horizontal flip. */
  horizontal_mirror,
  /** Vertical flip. */
  vertical_mirror
} VCEncHeifMirrorType;

/** \brief Contains image cropping configurations. */
typedef struct HeifCropCfg_ {
  /** \brief The width of the cropped image. */
  uint32_t croped_width;
  /** \brief The height of the cropped image. */
  uint32_t croped_height;
  /** \brief The X offset of the clean aperture center minus (width-1)/2. */
  uint32_t horizontal_offset;
  /** \brief The Y offset of the clean aperture center minus (height-1)/2. */
  uint32_t vertical_offset;
} VCEncHeifCropCfg;

/** \brief Contains configurations of the output image derived from image grids. */
typedef struct HeifGridCfg_ {
  /** \brief The number of rows in the unit of grid.
   *  \n The field value is greater than or equal to <tt>1</tt>. */
  uint32_t rows;
  /** \brief The number of columns in the unit of grid.
   *  \n The field value is greater than or equal to <tt>1</tt>. */
  uint32_t cols;
  /** \brief The width of the output image which is composed of grids, in pixels. */
  uint32_t image_width;
  /** \brief The height of the output image which is composed of grids, in pixels. */
  uint32_t image_height;
} VCEncHeifGridCfg;

/** \brief Defines a pair of coordinate offsets. */
typedef struct HeifOffset_ {
  /** \brief The horizontal offset. */
  uint32_t horizontal;
  /** \brief The vertical offset. */
  uint32_t vertical;
} HeifOffset;

/** \brief Contains image overlay configurations. */
typedef struct HeifOverlayCfg_ {
  /** \brief The total number of input images to overlay. */
  uint32_t overlay_images;
  /** \brief The width of the output overlaid image. */
  uint32_t output_width;
  /** \brief The height of the output overlaid image. */
  uint32_t output_height;
  /** \brief The offset of each input image.
   *  \n The number of specified offsets must equal the value of <tt>VCEncHeifOverlayCfg.overlay_images</tt>. */
  HeifOffset *offsets;
} VCEncHeifOverlayCfg;

/** \brief Sample information of sequence */
typedef struct HeifSamplecCfg_ {
/** \brief Duration of sample in ImageSequence timeBase units. */
  uint64_t duration;
} VCEncHeifSamplecCfg;

/** \brief Configuration of edit list for sequence */
typedef struct HeifEditListCfg_ {
 /* \brief Times the edit list should be repeated*/
  uint32_t repeatTimes;
} VCEncHeifEditListCfg;

/** \brief Contains information of stream data for HEIF. */
typedef struct VCEncHeifStreamData_ {
  /** \brief The address of the stream data buffer. */
  uint8_t *stream_data;
  /** \brief The length of the stream data, in bytes. */
  uint64_t stream_len;
} VCEncHeifStreamData;


/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif
