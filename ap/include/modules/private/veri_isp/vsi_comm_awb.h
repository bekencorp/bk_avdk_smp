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

#ifndef __VSI_COMM_AWB_H__
#define __VSI_COMM_AWB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond WB_V10
 *
 * @defgroup mpi_isp_wb WB V10 Definitions
 * @{
 *
 */

#include <modules/veri_isp/vsios_type.h>
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_wbm.h"

#define VSI_ISP_AWB_CURVE_CNT      16     /**< \brief The number of WB quadratic curves. */
#define VSI_ISP_WB_GAIN_MIN        256    /**< \brief The minimum value of each gain parameter for WB. */
#define VSI_ISP_WB_GAIN_MAX        1023   /**< \brief The maximum value of each gain parameter for WB. */
#define VSI_ISP_AWB_SPEED_MIN      0      /**< \brief The minimum value of <tt>speed</tt> for AWB. */
#define VSI_ISP_AWB_SPEED_MAX      255    /**< \brief The maximum value of <tt>speed</tt> for AWB. */
#define VSI_ISP_AWB_TOLERANCE_MIN  0      /**< \brief The minimum value of <tt>tolerance</tt> for AWB. */
#define VSI_ISP_AWB_TOLERANCE_MAX  100    /**< \brief The maximum value of <tt>tolerance</tt> for AWB. */

/** \brief Contains the WB gains. */
typedef struct vsiISP_AWB_GAIN_S {
    vsi_u16_t rGain;    /**< \brief The R gains.
                             \n Valid value range: [256, 1023]. */
    vsi_u16_t grGain;   /**< \brief The Gr gains.
                             \n Valid value range: [256, 1023]. */
    vsi_u16_t gbGain;   /**< \brief The Gb gains.
                             \n Valid value range: [256, 1023]. */
    vsi_u16_t bGain;    /**< \brief The B gains.
                             \n Valid value range: [256, 1023]. */
} ISP_WB_GAIN_S;

/** \brief Contains the AWB calibration data of the center line. */
typedef struct vsISP_AWB_CALIB_CENTER_LINE_S {
    vsi_s32_t rgParam;   /**< \brief The ratio of R gain to G gain of the center line. */
    vsi_s32_t bgParam;   /**< \brief The ratio of B gain to G gain of the center line. */
    vsi_s32_t distParam; /**< \brief The distance parameter of the center line. */
} ISP_AWB_CALIB_CENTER_LINE_S;

/** \brief Contains the AWB calibration data of the white pixel curve. */
typedef struct vsiISP_AWB_CALIB_WP_CURVE_S {
    vsi_s32_t rg[VSI_ISP_AWB_CURVE_CNT];        /**< \brief The horizontal coordinate of the projection of the edge of the orange box along the sampling point to the center line. */
    vsi_s32_t dist[VSI_ISP_AWB_CURVE_CNT];      /**< \brief The distance between the horizontal coordinate of the sampling point on the edge of the orange box and Rg. */
} ISP_AWB_CALIB_WP_CURVE_S;

/** \brief Contains the AWB calibration data of the white pixel range. */
typedef struct vsiISP_AWB_CALIB_WP_RANGE_S {
    ISP_AWB_CALIB_WP_CURVE_S wpLCurve;  /**< \brief The left edge of the orange box. */
    ISP_AWB_CALIB_WP_CURVE_S wpRCurve;  /**< \brief The right edge of the orange box. */
} ISP_AWB_CALIB_WP_RANGE_S;

/** \brief Contains the AWB calibration data of an illuminant. */
typedef struct vsiISP_AWB_CALIB_ILLUMINANT_S {
    vsi_u32_t illuType;                 /**< \brief The type of the illuminant.
                                             \n Valid values: See <tt> \ref ISP_ILLUMINANT_TYPE_E</tt>. */
    vsi_u32_t colorTemp;                /**< \brief The color temperature of the illuminant. */
    ISP_WB_GAIN_S wbGain;               /**< \brief The WB gains of the illuminant. */
} ISP_AWB_CALIB_ILLUMINANT_S;

/** \brief Contains the AWB calibration data. */
typedef struct vsiISP_AWB_CALIB_PARAM_S {
    ISP_AWB_CALIB_CENTER_LINE_S centLine;  /**< \brief The AWB center line. */
    vsi_s32_t rgMin;                       /**< \brief The minimum Rg value in indoor scenes when the AWB clip box is calibrated. */
    vsi_s32_t rgMax;                       /**< \brief The maximum Rg value of the orange box when the AWB clip box is calibrated. */
    ISP_AWB_CALIB_WP_RANGE_S wpRange0;     /**< \brief The white pixel range 0. */
    ISP_AWB_CALIB_WP_RANGE_S wpRange1;     /**< \brief The white pixel range 1. */
    ISP_AWB_CALIB_ILLUMINANT_S illuminant[ILLUMINANT_TYPE_CNT]; /**< \brief  The illuminant. */
} ISP_AWB_CALIB_PARAM_S;

/** \brief Contains the AWB information. */
typedef struct vsiISP_AWB_INFO_S {
    vsi_u32_t colorTemp;      /**< \brief The color temperature. */
    ISP_WB_GAIN_S wbGain;     /**< \brief The WB gains. */
    vsi_bool_t isStable;      /**< \brief The status of AWB.
                                   \n Valid values:
                                   \n - 0: Unstable.
                                   \n - 1: Stable. */
} ISP_AWB_INFO_S;


/** \brief Contains the WB attributes for use in manual mode. */
typedef struct vsiISP_MWB_ATTR_S {
    ISP_WB_GAIN_S wbGain;     /**< \brief The WB gains. */
} ISP_MWB_ATTR_S;

/** \brief Contains the WB attributes for use in auto mode. */
typedef struct vsiISP_AWB_ATTR_S {
    vsi_u8_t  runInterval;      /**< \brief The number of interval frames to run AWB. */
    vsi_u8_t  speed;            /**< \brief The speed of approaching the target.
                                     \n Valid value range: [0, 255]. */
    vsi_u8_t  tolerance;        /**< \brief The tolerance range.
                                     \n If the actual luminance falls in this range, it is considered that the target luminance is reached.
                                     \n Valid value range: [0, 100].  */
    vsi_u32_t  initColorTemp;   /**< \brief The initial color temperature. */
    ISP_AWB_CALIB_PARAM_S calibParam; /**< \brief The AWB calibration data. */
    vsi_u8_t  reserver[256];          /**< \brief Reserved. */
} ISP_AWB_ATTR_S;

/** \brief Contains the WB attributes. */
typedef struct vsiISP_WB_ATTR_S {
    vsi_bool_t enable;      /**< \brief Whether to enable WB.
                                 \n Valid values:
                                 \n - 0: Disable.
                                 \n - 1: Enable. */
    vsi_u32_t opType;       /**< \brief The operation mode of WB.
                                 \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_MWB_ATTR_S manualAttr; /**< \brief The WB attributes for use in manual mode. */
    ISP_AWB_ATTR_S autoAttr;   /**< \brief The WB attributes for use in auto mode. */
} ISP_WB_ATTR_S;

/** \brief Contains the WB metadata that needs to be written into registers. */
typedef struct vsiISP_WB_S {
    vsi_bool_t enable;  /**< \brief Whether to enable WB.
                             \n Valid values:
                             \n - 0: Disable.
                             \n - 1: Enable. */
    vsi_u16_t rGain;    /**< \brief The R gains. */
    vsi_u16_t grGain;   /**< \brief The Gr gains. */
    vsi_u16_t gbGain;   /**< \brief The Gb gains. */
    vsi_u16_t bGain;    /**< \brief The B gains. */
} ISP_AWB_META_S;

/** \brief Contains the AWB parameters. */
typedef struct vsiISP_AWB_PARAM_S {
    vsi_u32_t hdrMode;          /**< \brief The HDR mode.
                                     \n Valid values: See <tt> \ref ISP_HDR_MODE_E</tt>. */
    vsi_u32_t stichMode;        /**< \brief The stitching mode.
                                     \n Valid values: See <tt> \ref ISP_STICH_MODE_E</tt>. */
    ISP_WB_ATTR_S   wbAttr;     /**< \brief The WB attributes. */
} ISP_AWB_PARAM_S;

/** \brief Contains the WB statistics. */
typedef struct vsiISP_AWB_STAT_INFO_S {
    ISP_WBM_STATISTICS_S    wbmStat;  /**< \brief The WB statistic. */
} ISP_AWB_STAT_INFO_S;

/** \brief Contains the AWB results. */
typedef struct vsiISP_AWB_RESULT_S {
    ISP_WB_GAIN_S wbGain;    /**< \brief The WB gains. */
    vsi_u32_t colorTemp;     /**< \brief The color temperature. */
} ISP_AWB_RESULT_S;

/** \brief Defines the AWB commands. */
typedef enum vsiISP_AWB_CMD_E {
    ISP_AWB_CMD_SET_ATTR    = 0,   /**< \brief Command to set the AWB attributes. */
    ISP_AWB_CMD_GET_ATTR    = 1,   /**< \brief Command to get the AWB attributes. */
    ISP_AWB_CMD_QUERY_INFO  = 2,   /**< \brief Command to query the AWB information. */
} ISP_AWB_CMD_E;

/** \brief Defines the AWB library. */
typedef struct vsiISP_AWB_FUNC_S {
    int (*pfnAwbInit)(ISP_PORT IspPort, const ISP_AWB_PARAM_S *pParam,
                        ISP_AWB_RESULT_S *pAwbResult);         /**< \brief A pointer to the function that initializes AWB. */
    int (*pfnAwbRun) (ISP_PORT IspPort, const ISP_AWB_STAT_INFO_S *pAwbStatInfo,
                        ISP_AWB_RESULT_S *pAwbResult);         /**< \brief A pointer to the function that runs AWB. */
    int (*pfnAwbCtrl)(ISP_PORT IspPort, vsi_u32_t cmd, void *pValue); /**< \brief A pointer to the function that controls AWB. */
    int (*pfnAwbExit)(ISP_PORT IspPort);                              /**< \brief A pointer to the function that exits AWB. */
} ISP_AWB_FUNC_S;

/* @} mpi_isp_wb */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
