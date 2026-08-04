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
 * @defgroup mpi_isp_ae AE V10 Definitions
 * @{
 *
 */

#include <modules/veri_isp/vsios_type.h>
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"

#include "mpi_isp_expm.h"
#ifdef ISP_HIST256
#include "mpi_isp_hist256.h"
#elif defined(ISP_HIST64)
#include "mpi_isp_hist64.h"
#endif

/** \brief Contains the auto exposure (AE) attributes. */
typedef struct vsiISP_AE_ATTR_S {
    ISP_AE_RANGE_S expTimeRange;    /**< \brief The range of exposure time, depending on the sensor. */
    ISP_AE_RANGE_S aGainRange;      /**< \brief The range of analog gain, depending on the sensor. */
    ISP_AE_RANGE_S dGainRange;      /**< \brief The range of digital gain, depending on the sensor. */
    vsi_u8_t  runInterval;        /**< \brief The interval at which AE is run, in frames.
                                         \n Default value: 1. */
    vsi_u8_t  target;             /**< \brief The target luminance of AE.
                                         \n Valid value range: [0, 255]. */
    vsi_u8_t  dampOver;             /**< \brief When the current luminance exceeds the target, the greater the field value, the slower the luminance approaches the target.
                                         \n Valid value range: [0, 255]. */
    vsi_u8_t  dampUnder;            /**< \brief When the current luminance is below the target, the greater the field value, the slower the luminance approaches the target.
                                         \n Valid value range: [0, 255]. */
    vsi_u8_t  tolerance;            /**< \brief If the actual luminance falls in this range, it is considered that the target luminance is reached.
                                         \n Valid value range: [0, 100]. */
    ISP_ANTIFLICKER_S antiFlicker;  /**< \brief The anti-flicker configurations. */
    vsi_u32_t         aeMode;       /**< \brief The mode of AE.
                                         \n Valid values: See <tt> \ref ISP_AE_MODE_E</tt>. */
    vsi_u32_t         gainThreshold;    /**< \brief The gain threshold in AE slow shutter mode. */
    ISP_AE_ROUTE_S    aeRoute;          /**< \brief The route of AE. */
    ISP_AE_DELAY_S    delayAttr;      /**< \brief The delay frames when the scene luminance changes. */
    vsi_u8_t          weight[5][5];     /**< \brief The weight matrix of mean luminance.
                                             \n Valid value range: [0, 255]. */
    vsi_u8_t          reserver[128];    /**< \brief Reserved. */
} ISP_AE_ATTR_S;

/** \brief Contains the exposure attributes for use in manual mode. */
typedef struct vsiISP_ME_ATTR_S {
    vsi_u32_t intTime;               /**< \brief The exposure time, whose range depends on the sensor. */
    vsi_u32_t aGain;                 /**< \brief The analog gain, whose range depends on the sensor. */
    vsi_u32_t dGain;                 /**< \brief The digital gain, whose range depends on the sensor. */
} ISP_ME_ATTR_S;

/** \brief Contains the exposure attributes. */
typedef struct vsiISP_EXPOSURE_ATTR_S {
    vsi_u32_t     opType;             /**< \brief The operation mode of exposure.
                                           \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_AE_ATTR_S autoAttr;               /**< \brief The exposure attributes for use in auto mode. */
    ISP_ME_ATTR_S manualAttr;             /**< \brief The exposure attributes for use in manual mode. */
} ISP_EXPOSURE_ATTR_S;

/** \brief Exposure metadata that needs to be written into registers. */
typedef ISP_ME_ATTR_S ISP_EXPOSURE_META_S;

/** \brief (Reserved) Contains the HDR exposure attributes. */
typedef struct vsiISP_HDR_EXPOSURE_ATTR_S {
    vsi_u32_t opType;                  /**< \brief The operation mode of exposure.
                                            \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    vsi_u32_t ratio[HDR_EXPOSURE_RATIO_MAX_NUM];  /**< \brief The HDR ratio. */
    vsi_u32_t minRatio;                   /**< \brief The minimum ratio. */
    vsi_u32_t maxRatio;                   /**< \brief The maximum ratio. */
} ISP_HDR_EXPOSURE_ATTR_S;

/** \brief Contains the exposure information. */
typedef struct vsiISP_EXPOSURE_INFO_S {
    ISP_AE_RANGE_S expTimeRange;        /**< \brief The initial range of exposure time, depending on the sensor. */
    ISP_AE_RANGE_S aGainRange;          /**< \brief The initial range of analog gain, depending on the sensor. */
    ISP_AE_RANGE_S dGainRange;          /**< \brief The initial range of digital gain, depending on the sensor. */
    vsi_u32_t expTime[HDR_FRAME_MAX];   /**< \brief The exposure time. */
    vsi_u32_t aGain[HDR_FRAME_MAX];     /**< \brief The analog gain. */
    vsi_u32_t dGain[HDR_FRAME_MAX];     /**< \brief The digital gain. */
    vsi_u32_t exposure[HDR_FRAME_MAX];  /**< \brief The exposure. */
    vsi_u32_t iso;                      /**< \brief The ISO. The larger the value, the more sensitive the camera or sensor to light.*/
    vsi_u32_t ratio[HDR_EXPOSURE_RATIO_MAX_NUM]; /**< \brief Reserved. */
    vsi_bool_t isStable;                /**< \brief The status of AE.
                                             \n Valid values:
                                             \n - 0: Unstable.
                                             \n - 1: Stable. */
} ISP_EXPOSURE_INFO_S;

/** \brief Contains the AE parameters. */
typedef struct vsiISP_AE_PARAM_S
{
    vsi_u32_t hdrMode;              /**< \brief The HDR mode.
                                         \n Valid values: See <tt> \ref ISP_HDR_MODE_E</tt>. */
    vsi_u32_t stichMode;            /**< \brief The stitching mode.
                                         \n Valid values: See <tt> \ref ISP_STICH_MODE_E</tt>. */
    AE_SNS_FUNC_S aeSnsFunc;        /**< \brief The functions that AE uses to control the sensor. */
    ISP_EXPOSURE_ATTR_S expAttr;    /**< \brief The exposure attributes. */
} ISP_AE_PARAM_S;

/** \brief Contains the exposure statistics. */
typedef struct vsiISP_AE_STAT_INFO_S
{
    ISP_EXPM_STATISTICS_S    expmStat;    /**< \brief The statistics of mean luminance. */
#ifdef ISP_HIST256
    ISP_HIST256_STATISTICS_S histStat;    /**< \brief The statistics of histogram. */
#elif defined(ISP_HIST64)
    ISP_HIST64_STATISTICS_S histStat;    /**< \brief The statistics of histogram. */
#endif
} ISP_AE_STAT_INFO_S;

/** \brief Contains the AE results. */
typedef struct vsiISP_AE_RESULT_S {
    vsi_u32_t intLine;             /**< \brief The number of exposure lines. */
    vsi_u32_t aGain;               /**< \brief The analog gain. */
    vsi_u32_t dGain;               /**< \brief The digital gain. */
    vsi_bool_t isStable;           /**< \brief The status of AE.
                                        \n Valid values:
                                        \n - 0: Unstable.
                                        \n - 1: Stable. */
} ISP_AE_RESULT_S;

/** \brief Defines the AE commands. */
typedef enum vsiISP_AE_CMD_E {
    ISP_AE_CMD_SET_ATTR = 0,       /**< \brief Command to set the AE attributes. */
    ISP_AE_CMD_GET_ATTR = 1,       /**< \brief Command to get the AE attributes. */
    ISP_AE_CMD_SET_HDR_ATTR = 2,   /**< \brief Command to set the HDR attributes. */
    ISP_AE_CMD_GET_HDR_ATTR = 3,   /**< \brief Command to get the HDR attributes. */
    ISP_AE_CMD_QUERY_INFO = 4,     /**< \brief Command to query the AE information. */
} ISP_AE_CMD_E;

/** \brief Defines the AE library. */
typedef struct vsiISP_AE_FUNC_S {
    int (*pfnAeInit)(ISP_PORT IspPort, const ISP_AE_PARAM_S *pParam);  /**< \brief A pointer to the function that initializes AE. */
    int (*pfnAeRun) (ISP_PORT IspPort, const ISP_AE_STAT_INFO_S *pAeStatInfo);  /**< \brief A pointer to the function that runs AE. */
    int (*pfnAeCtrl)(ISP_PORT IspPort, vsi_u32_t cmd, void *pValue);   /**< \brief A pointer to the function that controls AE. */
    int (*pfnAeExit)(ISP_PORT IspPort);                                /**< \brief A pointer to the function that exits AE. */
} ISP_AE_FUNC_S;

/* @} mpi_isp_ae */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
