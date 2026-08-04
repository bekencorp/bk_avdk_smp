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

#ifndef __MPI_ISP_AFM_H__
#define __MPI_ISP_AFM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond AFM_V10
 *
 * @defgroup mpi_isp_afm AFM V10 Definitions
 * @{
 *
 */

/** \brief Defines the AF window IDs. */
typedef enum vsiISP_AFM_WINDOW_ID_E
{
    AFM_WINDOW_A        = 0,    /**< \brief Window A. */
    AFM_WINDOW_B        = 1,    /**< \brief Window B. */
    AFM_WINDOW_C        = 2,    /**< \brief Window C. */
    AFM_WINDOW_MAX,             /**< \brief The number of AF window IDs. */
} ISP_AFM_WINDOW_ID_E;

/** \brief Contains the statistics of an AF window. */
typedef struct vsiISP_AFM_STATISTICS_S {
    vsi_u32_t    sharpness;         /**< \brief The sharpness of the window. */
    vsi_u32_t    luminance;         /**< \brief The luminance of the window. */
    vsi_u32_t    pixelCnt;          /**< \brief The number of pixels in the window. */
} ISP_AFM_STATISTICS_S;


/** \brief Contains the statistics of all AF windows. */
typedef struct vsiISP_AFM_RECT_STATISTICS_S {
    ISP_AFM_STATISTICS_S    afmStat[3];         /**< \brief The statistics of each window. */
} ISP_AFM_RECT_STATISTICS_S;


/** \brief Contains the AFM attributes. */
typedef struct vsiISP_AFM_ATTR_S {
    vsi_bool_t   enable;           /**< \brief Whether to enable AFM.
                                        \n Valid values:
                                        \n - 0: Disable.
                                        \n - 1: Enable. */
    vsi_u32_t    threshold;
    vsi_u32_t    lumShift;
    vsi_u32_t    sharpnessShift;
    RECT_S       measRect[3];       /**< \brief The ROI of each AF window. */
} ISP_AFM_ATTR_S;

/** \brief  AFM metadata that needs to be written into registers. */
typedef ISP_AFM_ATTR_S ISP_AFM_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the AFM attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAfmAttr            A pointer to a memory place for receiving the AFM attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetAfmAttr(ISP_PORT IspPort, ISP_AFM_ATTR_S *pAfmAttr);

/*****************************************************************************/
/**
 * @brief   Sets the AFM attributes of an ISP device port.
 *
 * @param   IspPort               The ID of the port.
 * @param   pAfmAttr              A pointer to the AFM attributes.

 *
 * @retval  VSI_SUCCESS              The operation succeeds.
 * @retval  VSI_ERR_ILLEGAL_PARAM    Parameters are invalid.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetAfmAttr(ISP_PORT IspPort, ISP_AFM_ATTR_S *pAfmAttr);

/*****************************************************************************/
/**
 * @brief   Gets the AFM statistics of an ISP device port.
 *
 * @param   IspPort              The ID of the port.
 * @param   pAfmRectStat         A pointer to a memory place for receiving the AFM statistics.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetAfmStatistics(ISP_PORT IspPort, ISP_AFM_RECT_STATISTICS_S *pAfmRectStat);

/* @} mpi_isp_afm */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
