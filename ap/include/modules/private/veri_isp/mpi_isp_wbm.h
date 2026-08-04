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

#ifndef __MPI_ISP_WBM_H__
#define __MPI_ISP_WBM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond WBM_V10
 *
 * @defgroup mpi_isp_wbm WBM V10 Definitions
 * @{
 *
 */

/** \brief Defines the WBM modes. */
typedef enum vsiISP_WBM_MODE_E {
    ISP_AWB_MEAS_MODE_YCBCR    = 0,      /**< \brief YCbCr-based near-white discrimination mode. */
    ISP_AWB_MEAS_MODE_RGB      = 1,      /**< \brief RGB-based measurement mode. */
} ISP_WBM_MODE_E;

/** \brief Contains the WBM statistics. */
typedef struct vsiISP_WBM_STATISTICS_S {
    vsi_u32_t       measMode;              /**< \brief The WBM mode.
                                                \n Valid values: See <tt> \ref ISP_WBM_MODE_E</tt>. */
    vsi_u32_t       noWhitePixel;          /**< \brief The number of white pixels. */
    vsi_u8_t        meanY_G;               /**< \brief The Y value in YCbCr mode or G value in RGB mode. */
    vsi_u8_t        meanCb_B;              /**< \brief The Cb value in YCbCr mode or B value in RGB mode. */
    vsi_u8_t        meanCr_R;              /**< \brief The Cr value in YCbCr mode or R value in RGB mode. */
} ISP_WBM_STATISTICS_S;


/** \brief Contains the WBM configurations. */
typedef struct vsiISP_WBM_WP_RANGE_S {
    vsi_u8_t maxY;           /**< \brief YCbCr mode: The upper threshold of Y.
                                  \n A pixel contributes to WBM only if its Y component is no greater than this threshold.
                                  \n Value 0 indicates to disable the threshold.
                                  \n RGB mode: Unused. */
    vsi_u8_t refCr_MaxR;     /**< \brief YCbCr mode: The Cr reference value for confirming white pixel measurement.
                                  \n RGB mode: The upper threshold of R.
                                  \n A pixel contributes to WBM only if its R component is less than this threshold. */
    vsi_u8_t minY_MaxG;      /**< \brief YCbCr mode: The lower treshold of Y.
                                  \n A pixel contributes to WBM only if its Y component is no less than this threshold.
                                  \n RGB mode: The upper threshold of G.
                                  \n A pixel contributes to WBM only if its G component is less than this threshold. */
    vsi_u8_t refCb_MaxB;     /**< \brief YCbCr mode: The Cb reference value for confirming white pixel measurement.
                                  \n RGB mode: The upper threshold of B.
                                  \n A pixel contributes to WBM only if its B component is less than this threshold. */
    vsi_u8_t maxCSum;        /**< \brief YCbCr mode: The upper threshold of (Cb + Cr).
                                  \n A pixel contributes to WBM only if the sum of its Cb and Cr components is less than this threshold.
                                  \n RGB mode: Unused. */
    vsi_u8_t minC;           /**< \brief YCbCr mode: The lower treshold of Cb and Cr.
                                  \n A pixel contributes to WBM only if both its Cb component and Cr component is greater than this threshold.
                                  \n RGB mode: Unused. */
} ISP_WBM_WP_RANGE_S;

/** \brief Contains the WBM attributes. */
typedef struct vsiISP_WBM_ATTR_S {
    vsi_bool_t            enable;          /**< \brief Whether to enable WBM.
                                                \n Valid values:
                                                \n - <tt>0</tt>: Disable.
                                                \n - <tt>1</tt>: Enable. */
    vsi_u32_t             measMode;        /**< \brief The operation mode of WBM.
                                                \n Valid values: See <tt> \ref ISP_WBM_MODE_E</tt>. */
    RECT_S                measRect;        /**< \brief The WBM window. */
    ISP_WBM_WP_RANGE_S    wpRange;         /**< \brief The measurement configurations. */
} ISP_WBM_ATTR_S;

/** \brief WBM metadata that needs to be written into registers. */
typedef ISP_WBM_ATTR_S ISP_WBM_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the WBM attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pWbmAttr            A pointer to a memory place for receiving the WBM attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetWbmAttr(ISP_PORT IspPort, ISP_WBM_ATTR_S *pWbmAttr);

/*****************************************************************************/
/**
 * @brief   Sets the WBM attributes of an ISP device port.
 *
 * @param   IspPort                  The ID of the port.
 * @param   pWbmAttr                 A pointer to the WBM attributes.

 *
 * @retval  VSI_SUCCESS              The operation succeeds.
 * @retval  VSI_ERR_ILLEGAL_PARAM    Parameters are invalid.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetWbmAttr(ISP_PORT IspPort, ISP_WBM_ATTR_S *pWbmAttr);

/*****************************************************************************/
/**
 * @brief   Gets the WBM statistics of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pWbmStat            A pointer to a memory place for receiving the WBM statistics.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetWbmStatistics(ISP_PORT IspPort, ISP_WBM_STATISTICS_S *pWbmStat);

/* @} mpi_isp_wbm */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
