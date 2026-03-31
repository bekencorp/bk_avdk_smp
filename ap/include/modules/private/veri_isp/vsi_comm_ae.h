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

#ifndef __VSI_COMM_AE_H__
#define __VSI_COMM_AE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond AE_V10
 *
 * @defgroup mpi_isp_ae AE V10 Common Definitions
 * @{
 *
 */

#include "vsi_comm_def.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"

#include "mpi_isp_expm.h"
#ifdef ISP_HIST256
#include "mpi_isp_hist256.h"
#endif

/** \brief   This structure contains the auto exposure (AE) attributes. */
typedef struct vsiISP_AE_ATTR_S {
    ISP_AE_RANGE_S expTimeRange;    /**< \brief The maximum and minimum values of exposure time, depending on the sensor. */
    ISP_AE_RANGE_S againRange;      /**< \brief The maximum and minimum values of analog gain, depending on the sensor. */
    ISP_AE_RANGE_S dgainRange;      /**< \brief The maximum and minimum values of digital gain, depending on the sensor. */
    vsi_u8_t  aeRunInterval;        /**< \brief The number of interval frames to run AE. The default value is 1. */
    vsi_u8_t  aeTarget;             /**< \brief The target luminance of AE.
                                            \n Range [0, 255] */
    vsi_u8_t  dampOver;             /**< \brief When the current luminance exceeds the target, the larger this member value,
                                            the slower the luminance approaches the target.
                                            \n Range [0, 255] */
    vsi_u8_t  dampUnder;            /**< \brief When the current luminance is below the target,
                                            the larger this member value, the slower the luminance approaches the target.
                                            \n Range [0, 255] */
    vsi_u8_t  tolerance;            /**< \brief If the actual luminance falls in this range,
                                            it is considered that the target luminance is reached.
                                            \n Range [0, 100] */
    ISP_ANTIFLICKER_S antiflicker;  /**< \brief The anti-flicker configuration. */
    vsi_u32_t         aeMode;       /**< \brief The mode of AE. For detals, see <tt>ISP_AE_MODE_E</tt>. */
    vsi_u32_t         gainThreshold;    /**< \brief The threshold of gain in AE slow shutter mode. */
    ISP_AE_ROUTE_S    aeRoute;          /**< \brief The route of AE. */
    ISP_AE_DELAY_S    aeDelayAttr;      /**< \brief The delay frames when the scene luminance changes. */
    vsi_u8_t          weight[5][5];     /**< \brief The weight matrix of mean luminance.
                                                \n Range [0, 255] */
    vsi_u8_t          reserver[128];    /**< \brief Reserved */
} ISP_AE_ATTR_S;

/** \brief   This structure contains the manual exposure attributes. */
typedef struct vsiISP_ME_ATTR_S {
    vsi_u32_t intTime;               /**< \brief The exposure time, whose range depends on the sensor. */
    vsi_u32_t again;                 /**< \brief The analog gain, whose range depends on the sensor. */
    vsi_u32_t dgain;                 /**< \brief The digital gain, whose range depends on the sensor. */
} ISP_ME_ATTR_S;

/** \brief   This structure contains the exposure attributes. */
typedef struct vsiISP_EXPOSURE_ATTR_S {
    vsi_u32_t     opType;             /**< \brief The exposure running mode. For details, see <tt>ISP_OP_TYPE_E</tt>.
    \n <tt>0</tt>: Automatic mode. \n <tt>1</tt>: Manual mode. */
    ISP_AE_ATTR_S autoAttr;               /**< \brief Auto exposure attributes. */
    ISP_ME_ATTR_S manualAttr;             /**< \brief Manual exposure attributes. */
} ISP_EXPOSURE_ATTR_S;

/** \brief EXPOSURE metadata structure that need to be written into registers. */
typedef ISP_ME_ATTR_S ISP_EXPOSURE_META_S;

/** \brief   (Reserved) This structure contains the HDR exposure attributes. */
typedef struct vsiISP_HDR_EXPOSURE_ATTR_S {
    vsi_u32_t opType;                  /**< \brief The HDR exposure running mode. For details, see <tt>ISP_OP_TYPE_E</tt>.
    \n <tt>0</tt>: Automatic mode. \n <tt>1</tt>: Manual mode. */
    vsi_u32_t ratio[HDR_FRAME_MAX - 1];  /**< \brief The HDR ratio. */
    vsi_u32_t minRatio;                   /**< \brief The minimum ratio. */
    vsi_u32_t maxRatio;                   /**< \brief The maximum ratio. */
} ISP_HDR_EXPOSURE_ATTR_S;

/** \brief   This structure contains the exposure information. */
typedef struct vsiISP_EXPOSURE_INFO_S {
    vsi_u32_t expTime[HDR_FRAME_MAX];   /**< \brief The exposure time. */
    vsi_u32_t again[HDR_FRAME_MAX];     /**< \brief The analog gain. */
    vsi_u32_t dgain[HDR_FRAME_MAX];     /**< \brief The digital gain. */
    vsi_u32_t exposure[HDR_FRAME_MAX];  /**< \brief The exposure. */
    vsi_u32_t iso;                       /**< \brief The ISO. The larger the value, the more sensitive the camera or sensor to light.*/
    vsi_u32_t ratio[HDR_FRAME_MAX - 1]; /**< \brief Reserved. */
} ISP_EXPOSURE_INFO_S;

/** \brief   This structure contains the AE parameters. */
typedef struct vsiISP_AE_PARAM_S
{
    vsi_u32_t hdrMode;              /**< \brief The HDR mode. For details, see <tt>ISP_HDR_MODE_E</tt>. */
    vsi_u32_t stichMode;            /**< \brief The stitching mode. For details, see <tt>ISP_STICH_MODE_E</tt>. */
    AE_SNS_FUNC_S aeSnsFunc;        /**< \brief The structure of functions that AE used to control the sensor. */
    ISP_EXPOSURE_ATTR_S expAttr;    /**< \brief The AE attributes. */
} ISP_AE_PARAM_S;

/** \brief   This structure contains the exposure statistics. */
typedef struct vsiISP_AE_STAT_INFO_S
{
    ISP_EXPM_STATISTICS_S    expmStat;    /**< \brief The statistics of mean luminance. */
#ifdef ISP_HIST256
    ISP_HIST256_STATISTICS_S histStat;    /**< \brief The statistics of histogram. */
#endif
} ISP_AE_STAT_INFO_S;

/** \brief   This structure contains the AE result. */
typedef struct vsiISP_AE_RESULT_S {
    vsi_u32_t intLine;             /**< \brief The number of exposure lines. */
    vsi_u32_t again;               /**< \brief The analog gain. */
    vsi_u32_t dgain;               /**< \brief The digital gain. */
} ISP_AE_RESULT_S;

/** \brief   This enumeration specifies the indexes for implementing the AE commands. */
typedef enum vsiISP_AE_CMD_E {
    ISP_AE_CMD_SET_ATTR = 0,       /**< \brief The index for setting AE attributes. */
    ISP_AE_CMD_GET_ATTR = 1,       /**< \brief The index for getting AE attributes. */
    ISP_AE_CMD_SET_HDR_ATTR = 2,   /**< \brief The index for setting HDR attributes. */
    ISP_AE_CMD_GET_HDR_ATTR = 3,   /**< \brief The index for getting HDR attributes. */
    ISP_AE_CMD_QUERY_INFO = 4,     /**< \brief The index for querying AE information. */
} ISP_AE_CMD_E;

/** \brief   This structure contains the AE function pointers. */
typedef struct vsiISP_AE_FUNC_S {
    int (*pfnAeInit)(ISP_PORT IspPort, const ISP_AE_PARAM_S *pParam);  /**< \brief The function pointer of initializing AE. */
    int (*pfnAeRun) (ISP_PORT IspPort, const ISP_AE_STAT_INFO_S *pAeStatInfo);  /**< \brief The function pointer of running AE. */
    int (*pfnAeCtrl)(ISP_PORT IspPort, vsi_u32_t cmd, void *pValue);   /**< \brief The function pointer of controlling AE. */
    int (*pfnAeExit)(ISP_PORT IspPort);                                /**< \brief The function pointer of exiting AE. */
} ISP_AE_FUNC_S;

/* @} mpi_isp_ae */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
