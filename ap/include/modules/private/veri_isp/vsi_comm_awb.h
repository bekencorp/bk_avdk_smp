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
 * @defgroup mpi_isp_wb WB V10 Common Definitions
 * @{
 *
 */

#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_wbm.h"

#define VSI_ISP_AWB_CURVE_CNT      16     /**< \brief The value of WB quadratic curve count. */
#define VSI_ISP_WB_GAIN_MIN        256    /**< \brief The minimum value of WB gain. */
#define VSI_ISP_WB_GAIN_MAX        1023   /**< \brief The maximum value of WB gain. */
#define VSI_ISP_AWB_SPEED_MIN      0      /**< \brief The minimum value of AWB speed. */
#define VSI_ISP_AWB_SPEED_MAX      255    /**< \brief The maximum value of AWB speed. */
#define VSI_ISP_AWB_TOLERANCE_MIN  0      /**< \brief The minimum value of AWB tolerance. */
#define VSI_ISP_AWB_TOLERANCE_MAX  100    /**< \brief The maximum value of AWB tolerance. */

/** \brief   WB gain. */
typedef struct vsiISP_AWB_GAIN_S {
    vsi_u16_t rGain;    /**< \brief The gain value for the red channel.
                             \n Range [256, 1023]. */
    vsi_u16_t grGain;   /**< \brief The gain value for the green channel in red lines.
                             \n Range [256, 1023]. */
    vsi_u16_t gbGain;   /**< \brief The gain value for the green channel in blue lines.
                             \n Range [256, 1023]. */
    vsi_u16_t bGain;    /**< \brief The gain value for the blue channel.
                             \n Range [256, 1023]. */
} ISP_WB_GAIN_S;

/** \brief   Calibration data for AWB center line. */
typedef struct vsISP_AWB_CALIB_CENTER_LINE_S {
    vsi_s32_t rgParam;   /**< \brief The ratio of red channel gain to green channel gain of center line. */
    vsi_s32_t bgParam;   /**< \brief The ratio of blue channel gain to green channel gain of center line. */
    vsi_s32_t distParam; /**< \brief The distance parameter of center line. */
} ISP_AWB_CALIB_CENTER_LINE_S;

/** \brief   Calibration data for AWB white pixel curve. */
typedef struct vsiISP_AWB_CALIB_WP_CURVE_S {
    vsi_s32_t rg[VSI_ISP_AWB_CURVE_CNT];        /**< \brief Horizontal coordinate of the projection of the edge of the orange box along the sampling point to the center line during AWB calibration. */
    vsi_s32_t dist[VSI_ISP_AWB_CURVE_CNT];      /**< \brief Distance between the horizontal coordinate of the sampling point on the edge of the orange box and Rg during AWB calibration. */
} ISP_AWB_CALIB_WP_CURVE_S;

/** \brief   Calibration data for AWB white pixel range. */
typedef struct vsiISP_AWB_CALIB_WP_RANGE_S {
    ISP_AWB_CALIB_WP_CURVE_S wpLCurve;  /**< \brief The left edge of the orange box. */
    ISP_AWB_CALIB_WP_CURVE_S wpRCurve;  /**< \brief The right edge of the orange box. */
} ISP_AWB_CALIB_WP_RANGE_S;

/** \brief   Calibration data for AWB illuminance. */
typedef struct vsiISP_AWB_CALIB_ILLUMINANT_S {
    vsi_u32_t illuType;                 /**< \brief The index of color temperature, reference ISP_ILLUMINANT_TYPE_E. */
    vsi_u32_t colorTemp;                /**< \brief The value of color temperature. */
    ISP_WB_GAIN_S wbGain;               /**< \brief The WB gain. */
} ISP_AWB_CALIB_ILLUMINANT_S;

/** \brief   Calibration data for AWB. */
typedef struct vsiISP_AWB_CALIB_PARAM_S {
    ISP_AWB_CALIB_CENTER_LINE_S centLine;  /**< \brief AWB center line. */
    vsi_s32_t rgMin;                       /**< \brief Minimum Rg value in indoor scenes when the AWB clip box is calibrated. */
    vsi_s32_t rgMax;                       /**< \brief Maximum Rg boundary value of the orange box when the AWB clip box is calibrated. */
    ISP_AWB_CALIB_WP_RANGE_S wpRange0;     /**< \brief The white pixel range zero. */
    ISP_AWB_CALIB_WP_RANGE_S wpRange1;     /**< \brief The white pixel range one. */
    ISP_AWB_CALIB_ILLUMINANT_S illuminant[ILLUMINANT_TYPE_CNT]; /**< \brief  The illuminance. */
} ISP_AWB_CALIB_PARAM_S;

/** \brief   AWB information. */
typedef struct vsiISP_AWB_INFO_S {
    vsi_u32_t colorTemp;      /**< \brief The color temperature. */
    ISP_WB_GAIN_S wbGain;     /**< \brief The WB gain. */
} ISP_AWB_INFO_S;


/** \brief   Manual WB attributes. */
typedef struct vsiISP_MWB_ATTR_S {
    ISP_WB_GAIN_S wbGain;     /**< \brief The WB gain. */
} ISP_MWB_ATTR_S;

/** \brief   Auto WB attributes. */
typedef struct vsiISP_AWB_ATTR_S {
    vsi_u8_t  runInterval;      /**< \brief The number of interval frames to run AWB. */
    vsi_u8_t  speed;            /**< \brief The speed of approaching the target.
                                     \n Range [0, 255]. */
    vsi_u8_t  tolerance;        /**< \brief The tolerance range.
                                      If the actual luminance falls in this range, it is considered that the target luminance is reached.
                                     \n Range [0, 100].  */
    vsi_u32_t  initColorTemp;   /**< \brief The initial color temperature. */
    ISP_AWB_CALIB_PARAM_S calibParam; /**< \brief The AWB calibration data. */
    vsi_u8_t  reserver[256];          /**< \brief Reserved */
} ISP_AWB_ATTR_S;

/** \brief   WB attributes. */
typedef struct vsiISP_WB_ATTR_S {
    vsi_bool_t enable;      /**< \brief Whether to enable WB. \n 0: Disable WB. \n 1: Enable WB. */
    vsi_u32_t opType;       /**< \brief WB mode attributes. For details, see <tt>ISP_OP_TYPE_E</tt> */
    ISP_MWB_ATTR_S manualAttr; /**< \brief WB manual attributes */
    ISP_AWB_ATTR_S autoAttr;   /**< \brief WB auto attributes */
} ISP_WB_ATTR_S;

/** \brief   WB metadata structure that need to be written into registers. */
typedef struct vsiISP_WB_S {
    vsi_bool_t enable;  /**< \brief Whether to enable WB. \n 0: Disable WB. \n 1: Enable WB. */
    vsi_u16_t rGain;    /**< \brief The gain value for the red channel. */
    vsi_u16_t grGain;   /**< \brief The gain value for the green channel in red lines. */
    vsi_u16_t gbGain;   /**< \brief The gain value for the green channel in blue lines. */
    vsi_u16_t bGain;    /**< \brief The gain value for the blue channel. */
} ISP_AWB_META_S;

/** \brief   AWB parameters. */
typedef struct vsiISP_AWB_PARAM_S {
    vsi_u32_t hdrMode;          /**< \brief The HDR mode. For details, see <tt>ISP_HDR_MODE_E</tt>. */
    vsi_u32_t stichMode;        /**< \brief The stitching mode. For details, see <tt>ISP_STICH_MODE_E</tt>. */
    ISP_WB_ATTR_S   wbAttr;     /**< \brief WB attributes */
} ISP_AWB_PARAM_S;

/** \brief   WB statistics. */
typedef struct vsiISP_AWB_STAT_INFO_S {
    ISP_WBM_STATISTICS_S    wbmStat;  /**< \brief The statistic of WB. */
} ISP_AWB_STAT_INFO_S;

/** \brief   AWB result. */
typedef struct vsiISP_AWB_RESULT_S {
    ISP_WB_GAIN_S wbGain;    /**< \brief WB gain. */
    vsi_u32_t colorTemp;     /**< \brief The color temperature. */
} ISP_AWB_RESULT_S;

/** \brief   AWB commands. */
typedef enum vsiISP_AWB_CMD_E {
    ISP_AWB_CMD_SET_ATTR    = 0,   /**< \brief The index of setting AWB attributes. */
    ISP_AWB_CMD_GET_ATTR    = 1,   /**< \brief The index of getting AWB attributes. */
    ISP_AWB_CMD_QUERY_INFO  = 2,   /**< \brief The index of querying AWB information. */
} ISP_AWB_CMD_E;

/** \brief   AWB functions. */
typedef struct vsiISP_AWB_FUNC_S {
    int (*pfnAwbInit)(ISP_PORT IspPort, const ISP_AWB_PARAM_S *pParam,
                        ISP_AWB_RESULT_S *pAwbResult);         /**< \brief The function pointer of initializing AWB. */
    int (*pfnAwbRun) (ISP_PORT IspPort, const ISP_AWB_STAT_INFO_S *pAwbStatInfo,
                        ISP_AWB_RESULT_S *pAwbResult);         /**< \brief The function pointer of running AWB. */
    int (*pfnAwbCtrl)(ISP_PORT IspPort, vsi_u32_t cmd, void *pValue); /**< \brief The function pointer of controlling AWB. */
    int (*pfnAwbExit)(ISP_PORT IspPort);                              /**< \brief The function pointer of exiting AWB. */
} ISP_AWB_FUNC_S;

/* @} mpi_isp_wb */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
