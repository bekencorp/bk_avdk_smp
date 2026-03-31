/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VeriSilicon Inc                       --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/
/*!\file
 * \brief Describes the av1 image descriptor and associated operations
 *
 */
#ifndef VCENC_AV1_IMAGE_H_
#define VCENC_AV1_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vcenc_av1_integer.h"

#define VCENC_AV1_IMG_FMT_PLANAR 0x100  /**< Image is a planar format. */
#define VCENC_AV1_IMG_FMT_UV_FLIP 0x200 /**< V plane precedes U in memory. */
/** 0x400 used to signal alpha channel, skipping for backwards compatibility. */
#define VCENC_AV1_IMG_FMT_HIGHBITDEPTH 0x800 /**< Image uses 16bit framebuffer. */

/*!\brief List of supported image formats */
typedef enum vcenc_av1_img_fmt {
  VCENC_AV1_IMG_FMT_NONE,
  VCENC_AV1_IMG_FMT_YV12 =
      VCENC_AV1_IMG_FMT_PLANAR | VCENC_AV1_IMG_FMT_UV_FLIP | 1, /**< planar YVU */
  VCENC_AV1_IMG_FMT_I420 = VCENC_AV1_IMG_FMT_PLANAR | 2,
  VCENC_AV1_IMG_FMT_AV1YV12 = VCENC_AV1_IMG_FMT_PLANAR | VCENC_AV1_IMG_FMT_UV_FLIP |
                        3, /** < planar 4:2:0 format with av1 color space */
  VCENC_AV1_IMG_FMT_AV1I420 = VCENC_AV1_IMG_FMT_PLANAR | 4,
  VCENC_AV1_IMG_FMT_I422 = VCENC_AV1_IMG_FMT_PLANAR | 5,
  VCENC_AV1_IMG_FMT_I444 = VCENC_AV1_IMG_FMT_PLANAR | 6,
  VCENC_AV1_IMG_FMT_I42016 = VCENC_AV1_IMG_FMT_I420 | VCENC_AV1_IMG_FMT_HIGHBITDEPTH,
  VCENC_AV1_IMG_FMT_YV1216 = VCENC_AV1_IMG_FMT_YV12 | VCENC_AV1_IMG_FMT_HIGHBITDEPTH,
  VCENC_AV1_IMG_FMT_I42216 = VCENC_AV1_IMG_FMT_I422 | VCENC_AV1_IMG_FMT_HIGHBITDEPTH,
  VCENC_AV1_IMG_FMT_I44416 = VCENC_AV1_IMG_FMT_I444 | VCENC_AV1_IMG_FMT_HIGHBITDEPTH,
} vcenc_av1_img_fmt_t; /**< alias for enum vcenc_av1_img_fmt */

/*!\brief List of supported color primaries */
typedef enum vcenc_av1_color_primaries {
  VCENC_AV1_CICP_CP_RESERVED_0 = 0,  /**< For future use */
  VCENC_AV1_CICP_CP_BT_709 = 1,      /**< BT.709 */
  VCENC_AV1_CICP_CP_UNSPECIFIED = 2, /**< Unspecified */
  VCENC_AV1_CICP_CP_RESERVED_3 = 3,  /**< For future use */
  VCENC_AV1_CICP_CP_BT_470_M = 4,    /**< BT.470 System M (historical) */
  VCENC_AV1_CICP_CP_BT_470_B_G = 5,  /**< BT.470 System B, G (historical) */
  VCENC_AV1_CICP_CP_BT_601 = 6,      /**< BT.601 */
  VCENC_AV1_CICP_CP_SMPTE_240 = 7,   /**< SMPTE 240 */
  VCENC_AV1_CICP_CP_GENERIC_FILM =
      8, /**< Generic film (color filters using illuminant C) */
  VCENC_AV1_CICP_CP_BT_2020 = 9,      /**< BT.2020, BT.2100 */
  VCENC_AV1_CICP_CP_XYZ = 10,         /**< SMPTE 428 (CIE 1921 XYZ) */
  VCENC_AV1_CICP_CP_SMPTE_431 = 11,   /**< SMPTE RP 431-2 */
  VCENC_AV1_CICP_CP_SMPTE_432 = 12,   /**< SMPTE EG 432-1  */
  VCENC_AV1_CICP_CP_RESERVED_13 = 13, /**< For future use (values 13 - 21)  */
  VCENC_AV1_CICP_CP_EBU_3213 = 22,    /**< EBU Tech. 3213-E  */
  VCENC_AV1_CICP_CP_RESERVED_23 = 23  /**< For future use (values 23 - 255)  */
} vcenc_av1_color_primaries_t;        /**< alias for enum vcenc_av1_color_primaries */

/*!\brief List of supported transfer functions */
typedef enum vcenc_av1_transfer_characteristics {
  VCENC_AV1_CICP_TC_RESERVED_0 = 0,  /**< For future use */
  VCENC_AV1_CICP_TC_BT_709 = 1,      /**< BT.709 */
  VCENC_AV1_CICP_TC_UNSPECIFIED = 2, /**< Unspecified */
  VCENC_AV1_CICP_TC_RESERVED_3 = 3,  /**< For future use */
  VCENC_AV1_CICP_TC_BT_470_M = 4,    /**< BT.470 System M (historical)  */
  VCENC_AV1_CICP_TC_BT_470_B_G = 5,  /**< BT.470 System B, G (historical) */
  VCENC_AV1_CICP_TC_BT_601 = 6,      /**< BT.601 */
  VCENC_AV1_CICP_TC_SMPTE_240 = 7,   /**< SMPTE 240 M */
  VCENC_AV1_CICP_TC_LINEAR = 8,      /**< Linear */
  VCENC_AV1_CICP_TC_LOG_100 = 9,     /**< Logarithmic (100 : 1 range) */
  VCENC_AV1_CICP_TC_LOG_100_SQRT10 =
      10,                     /**< Logarithmic (100 * Sqrt(10) : 1 range) */
  VCENC_AV1_CICP_TC_IEC_61966 = 11, /**< IEC 61966-2-4 */
  VCENC_AV1_CICP_TC_BT_1361 = 12,   /**< BT.1361 */
  VCENC_AV1_CICP_TC_SRGB = 13,      /**< sRGB or sYCC*/
  VCENC_AV1_CICP_TC_BT_2020_10_BIT = 14, /**< BT.2020 10-bit systems */
  VCENC_AV1_CICP_TC_BT_2020_12_BIT = 15, /**< BT.2020 12-bit systems */
  VCENC_AV1_CICP_TC_SMPTE_2084 = 16,     /**< SMPTE ST 2084, ITU BT.2100 PQ */
  VCENC_AV1_CICP_TC_SMPTE_428 = 17,      /**< SMPTE ST 428 */
  VCENC_AV1_CICP_TC_HLG = 18,            /**< BT.2100 HLG, ARIB STD-B67 */
  VCENC_AV1_CICP_TC_RESERVED_19 = 19     /**< For future use (values 19-255) */
} vcenc_av1_transfer_characteristics_t;  /**< alias for enum vcenc_av1_transfer_function */

/*!\brief List of supported matrix coefficients */
typedef enum vcenc_av1_matrix_coefficients {
  VCENC_AV1_CICP_MC_IDENTITY = 0,    /**< Identity matrix */
  VCENC_AV1_CICP_MC_BT_709 = 1,      /**< BT.709 */
  VCENC_AV1_CICP_MC_UNSPECIFIED = 2, /**< Unspecified */
  VCENC_AV1_CICP_MC_RESERVED_3 = 3,  /**< For future use */
  VCENC_AV1_CICP_MC_FCC = 4,         /**< US FCC 73.628 */
  VCENC_AV1_CICP_MC_BT_470_B_G = 5,  /**< BT.470 System B, G (historical) */
  VCENC_AV1_CICP_MC_BT_601 = 6,      /**< BT.601 */
  VCENC_AV1_CICP_MC_SMPTE_240 = 7,   /**< SMPTE 240 M */
  VCENC_AV1_CICP_MC_SMPTE_YCGCO = 8, /**< YCgCo */
  VCENC_AV1_CICP_MC_BT_2020_NCL =
      9, /**< BT.2020 non-constant luminance, BT.2100 YCbCr  */
  VCENC_AV1_CICP_MC_BT_2020_CL = 10, /**< BT.2020 constant luminance */
  VCENC_AV1_CICP_MC_SMPTE_2085 = 11, /**< SMPTE ST 2085 YDzDx */
  VCENC_AV1_CICP_MC_CHROMAT_NCL =
      12, /**< Chromaticity-derived non-constant luminance */
  VCENC_AV1_CICP_MC_CHROMAT_CL = 13, /**< Chromaticity-derived constant luminance */
  VCENC_AV1_CICP_MC_ICTCP = 14,      /**< BT.2100 ICtCp */
  VCENC_AV1_CICP_MC_RESERVED_15 = 15 /**< For future use (values 15-255)  */
} vcenc_av1_matrix_coefficients_t;

/*!\brief List of supported color range */
typedef enum vcenc_av1_color_range {
  VCENC_AV1_CR_STUDIO_RANGE = 0, /**< Y [16..235], UV [16..240] */
  VCENC_AV1_CR_FULL_RANGE = 1    /**< YUV/RGB [0..255] */
} vcenc_av1_color_range_t;       /**< alias for enum vcenc_av1_color_range */

/*!\brief List of chroma sample positions */
typedef enum vcenc_av1_chroma_sample_position {
  VCENC_AV1_CSP_UNKNOWN = 0,          /**< Unknown */
  VCENC_AV1_CSP_VERTICAL = 1,         /**< Horizontally co-located with luma(0, 0)*/
                                /**< sample, between two vertical samples */
  VCENC_AV1_CSP_COLOCATED = 2,        /**< Co-located with luma(0, 0) sample */
  VCENC_AV1_CSP_RESERVED = 3          /**< Reserved value */
} vcenc_av1_chroma_sample_position_t; /**< alias for enum vcenc_av1_transfer_function */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VCENC_AV1_IMAGE_H_
