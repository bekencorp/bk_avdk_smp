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

#ifndef __MPI_ISP_EXPM_H__
#define __MPI_ISP_EXPM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond EXPM_V10
 *
 * @defgroup mpi_isp_expm EXPM V10 Definitions
 * @{
 *
 */

#define VSI_ISP_EXPM_STATIC_BLOCK_NUM    25   /**< \brief The statistics block number. */

#define VSI_ISP_EXPM_ALT_MODE_0          0     /**< \brief Luminance calculation according to Y=16+0.25R+0.5G+0.1094B. */
#define VSI_ISP_EXPM_ALT_MODE_1          1     /**< \brief Luminance calculation according to Y=(R+G+B) x 0.332. */


/** \brief   EXPM statistics structure. */
typedef struct vsiISP_EXPM_STATISTICS_S {
    vsi_u8_t meanLum[VSI_ISP_EXPM_STATIC_BLOCK_NUM];   /**< \brief Exposure green and green blue mean values. */
} ISP_EXPM_STATISTICS_S;

/** \brief   Exposure measurement attributes. */
typedef struct vsiISP_EXPM_ATTR_S {
    vsi_bool_t enable;         /**< \brief Whether to enable EXPM.
                                    \n 0: Disable EXPM.
                                    \n 1: Enable EXPM. */
    vsi_u8_t   expAltMode;     /**< \brief Luminance calculation mode.
                                    \n 0: Luminance calculation according to Y=16+0.25R+0.5G+0.1094B.
                                    \n 1: Luminance calculation according to Y=(R+G+B) x 0.332. */
    RECT_S     measRect;       /**< \brief The EXPM rectangle. */
} ISP_EXPM_ATTR_S;

/** \brief   EXPM metadata structure that need to be written into registers. */
typedef struct vsiISP_EXPM_S {
    vsi_bool_t enable;         /**< \brief Whether to enable EXPM.
                                    \n 0: Disable EXPM.
                                    \n 1: Enable EXPM. */
    vsi_bool_t altStop;        /**< \brief Luminance measurement mode.
                                    \n 0: Continuous measurement.
                                    \n 1: Stop measuring after a complete frame. */
    vsi_u8_t   expAltMode;     /**< \brief Luminance calculation mode.
                                    \n 0: Luminance calculation according to Y=16+0.25R+0.5G+0.1094B.
                                    \n 1: Luminance calculation according to Y=(R+G+B) x 0.332. */
    RECT_S     blockRect;      /**< \brief The EXPM rectangle. */
} ISP_EXPM_META_S;

/*****************************************************************************/
/**
 * @brief   Gets EXPM configurations.
 *
 * @param   IspPort             Port ID
 * @param   pExpmAttr         Pointer to the EXPM configurations
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetExpmAttr(ISP_PORT IspPort, ISP_EXPM_ATTR_S *pExpmAttr);

/*****************************************************************************/
/**
 * @brief   Sets EXPM configurations.
 *
 * @param   IspPort                  Port ID
 * @param   pExpmAttr              Pointer to the EXPM configurations

 *
 * @retval  VSI_SUCCESS              Operation succeeded
 * @retval  VSI_ERR_ILLEGAL_PARAM    Invalid parameter
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetExpmAttr(ISP_PORT IspPort, ISP_EXPM_ATTR_S *pExpmAttr);

/*****************************************************************************/
/**
 * @brief   Gets EXPM statistics.
 *
 * @param   IspPort             Port ID
 * @param   pExpmStat         Pointer to the EXPM statistics
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetExpmStatistics(ISP_PORT IspPort, ISP_EXPM_STATISTICS_S *pExpmStat);

/* @} mpi_isp_expm */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
