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

#ifndef __VSI_COMM_AF_H__
#define __VSI_COMM_AF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond AF_V10
 *
 * @defgroup mpi_isp_af AF V10 Definitions
 * @{
 *
 */

#include <modules/veri_isp/vsios_type.h>
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"

#include "mpi_isp_afm.h"

#define CAMDEV_PD_FOCAL_NUM_MAX               48    /**< \brief The number of AF focal PD. */
#define VSI_ISP_FOCUS_FOCAL_MIN               0     /**< \brief The minimum value of maximum focal point for AF. */
#define VSI_ISP_FOCUS_FOCAL_MAX               1023  /**< \brief The minimum value of maximum focal point for AF. */
#define VSI_ISP_FOCUS_WEIGHT_WINDOW_MIN       0     /**< \brief The minimum value of weight window for CDAF. */
#define VSI_ISP_FOCUS_WEIGHT_WINDOW_MAX       2550  /**< \brief The maximum value of weight window for CDAF. */
#define VSI_ISP_FOCUS_STABLE_TOLERENCE_MIN    0     /**< \brief The minimum value of stable tolerance for CDAF. */
#define VSI_ISP_FOCUS_STABLE_TOLERENCE_MAX    10    /**< \brief The maximum value of stable tolerance for CDAF. */
#define VSI_ISP_FOCUS_POINTS_OF_CURVE_MIN     3     /**< \brief The minimum value of points of curve for CDAF. */
#define VSI_ISP_FOCUS_POINTS_OF_CURVE_MAX     20   /**< \brief The maximum value of points of curve for CDAF. */

/** \brief Defines the AF statistics version. */
typedef enum vsiISP_AF_STATISTIC_TYPE_E { 
    AFMV1_STATISTIC_INVALID = 0,        /**< \brief (Reserved) Invalid. */
    AFMV1_STATISTIC         = 1,        /**< \brief AFM V10 version. */
} ISP_AF_STATISTIC_TYPE_E;

/** \brief Contains the focus attributes for use in auto mode. */
typedef struct vsiISP_AF_ATTR_S {
	vsi_u16_t    maxFocal;               /**< \brief The maximum focal length. */
    vsi_u16_t    minFocal;               /**< \brief The minimum focal length. */
    vsi_u16_t    weightWindow[3];        /**< \brief The CDAF weight of each AF window.
                                              \n Valid value range: [0, 2550], which is linearly normalized to the weight in range [0.0, 255.0]. */
    vsi_u16_t    cSharpStableTolerance;  /**< \brief The CDAF sharpness tolerance, used to determine the AF stability.
                                              \n Valid value range: [0, 10], which is linearly normalized to the tolerance in range [0.0, 1.0]. */
    vsi_u16_t    cLumStableTolerance;       /**< \brief The CDAF luminance tolerance, used to determine the AF stability.
                                              \n Valid value range: [0, 10], which is linearly normalized to the tolerance in range [0.0, 1.0]. */
    vsi_u8_t     cPointsOfCurve;         /**< \brief The number of sampling points on the CDAF curve, which determines the sampling step.
                                              \n Valid value range: [3, 20]. */
} ISP_AF_ATTR_S;

/** \brief Contains the focus attributes for use in manual mode. */
typedef struct vsiISP_MF_ATTR_S {
    vsi_u16_t focal;               /**< \brief The focal length, whose range depends on the sensor. */
} ISP_MF_ATTR_S;

/** \brief Contains the focus attributes. */
typedef struct vsiISP_FOCUS_ATTR_S {
    vsi_u32_t     opType;             /**< \brief The operation mode of focus.
                                           \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_AF_ATTR_S autoAttr;           /**< \brief The focus attributes for use in auto mode. */
    ISP_MF_ATTR_S manualAttr;         /**< \brief The focus attributes for use in manual mode. */
} ISP_FOCUS_ATTR_S;

/** \brief Focus metadata that needs to be written into registers. */
typedef ISP_MF_ATTR_S ISP_FOCUS_META_S;

/** \brief Contains the focus information. */
typedef struct vsiISP_FOCUS_INFO_S {
    vsi_u32_t  focal;              /**< \brief The optimal focal length. */
    vsi_bool_t isStable;           /**< \brief The AF status.
                                        \n Valid values:
                                        \n - 0: Unstable.
                                        \n - 1: Stable. */
} ISP_FOCUS_INFO_S;


/** \brief Defines the AF parameters. */
typedef struct vsiISP_AF_PARAM_S {
    AF_SNS_FUNC_S    afSnsFunc;     /**< \brief The functions that AF uses to control the sensor. */
    ISP_FOCUS_ATTR_S focusAttr;     /**< \brief The AF attributes. */
} ISP_AF_PARAM_S;


/* \brief Contains the AF statistics. */
typedef struct vsiISP_AF_STAT_INFO_S {
    ISP_AFM_RECT_STATISTICS_S    afmRectStat;     /**< \brief The AF statistics. */
    vsi_u8_t statType;                            /**< \brief The version of the AF statistics. */
    vsi_u8_t pdafEnable;                          /**< \brief Whether to enable PDAF.
                                                       \n Valid values:
                                                       \n - 0: Disable.
                                                       \n - 1: Enable. */
} ISP_AF_STAT_INFO_S;

/** \brief Contains the AF results. */
typedef struct vsiISP_AF_RESULT_S {
    ISP_FOCUS_POS_S focalPos;      /**< \brief The focus position. */
    // vsi_u32_t focal;            /**< \brief The optimal focal length. */
    vsi_bool_t isStable;           /**< \brief The AF status.
                                        \n Valid values:
                                        \n 0: Unstable.
                                        \n 1: Stable. */
} ISP_AF_RESULT_S;

/** \brief Defines the AF command types. */
typedef enum vsiISP_AF_CMD_E {
    ISP_AF_CMD_SET_ATTR = 0,       /**< \brief Command to set the AF attributes. */
    ISP_AF_CMD_GET_ATTR = 1,       /**< \brief Command to get the AF attributes. */
    ISP_AF_CMD_QUERY_INFO = 2,     /**< \brief Command to query the AF information. */
} ISP_AF_CMD_E;

/** \brief Defines the AF library. */
typedef struct vsiISP_AF_FUNC_S {
    int (*pfnAfInit)(ISP_PORT IspPort, const ISP_AF_PARAM_S *pParam);  /**< \brief A pointer to the function that initializes AF. */
    int (*pfnAfRun) (ISP_PORT IspPort, const ISP_AF_STAT_INFO_S *pAfStatInfo);  /**< \brief A pointer to the function that runs AF. */
    int (*pfnAfCtrl)(ISP_PORT IspPort, vsi_u32_t cmd, void *pValue);   /**< \brief A pointer to the function that controls AF. */
    int (*pfnAfExit)(ISP_PORT IspPort);                                /**< \brief A pointer to the function that exits AF. */
} ISP_AF_FUNC_S;

/* @} mpi_isp_af */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
