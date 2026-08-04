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

#ifndef __MPI_ISP_HIST256_H__
#define __MPI_ISP_HIST256_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond HIST256
 *
 * @defgroup mpi_isp_hist256 HIST256 Definitions
 * @{
 *
 */

#define VSI_ISP_HIST_STATIC_BLOCK_NUM 256   /**< \brief The number of bins in the histogram. */
#define HIST256_H_GRID_ITEMS 5              /**< \brief The number of horizontal grid subwindows. */
#define HIST256_V_GRID_ITEMS 5              /**< \brief The number of vertical grid subwindows. */

#define VSI_ISP_HIST256_WEIGHT_MAX 16   /**< \brief The maximum value of <tt>weight</tt> for HIST256. */

/** \brief Contains the HIST256 statistics. */
typedef struct vsiISP_HIST256_STATISTICS_S {
    vsi_u32_t histogram[VSI_ISP_HIST_STATIC_BLOCK_NUM]; /**< \brief The statistics of each bin. */
} ISP_HIST256_STATISTICS_S;

/** \brief Defines the histogram modes. */
typedef enum vsiISP_HIST256_MODE_E {
    HIST256_R_MODE = 2,    /**< \brief Red histogram mode. */
    HIST256_G_MODE = 3,    /**< \brief Green histogram mode. */
    HIST256_B_MODE = 4,    /**< \brief Blue histogram mode. */
    HIST256_Y_MODE = 5,    /**< \brief Y histogram mode. */
} ISP_HIST256_MODE_E;

/** \brief Contains the HIST256 attributes. */
typedef struct vsiISP_HIST256_ATTR_S {
    vsi_bool_t enable;        /**< \brief Whether to enable HIST256.
                                   \n Valid values:
                                   \n - 0: Disable.
                                   \n - 1: Enable. */
    RECT_S     measRect;      /**< \brief The HIST256 window. */
    vsi_u32_t  mode;          /**< \brief The histogram mode.
                                   \n Valid values: See <tt> \ref ISP_HIST256_MODE_E</tt>. */
    vsi_u8_t   weight[HIST256_H_GRID_ITEMS][HIST256_V_GRID_ITEMS];    /**< \brief The histogram weights.
                                                                           \n Valid value range: [0, 16]. */
} ISP_HIST256_ATTR_S;


/** \brief Contains the HIST256 metadata that needs to be written into registers. */
typedef struct vsiISP_HIST256_S {
    vsi_bool_t enable;        /**< \brief Whether to enable HIST256.
                                   \n Valid values:
                                   \n - 0: Disable.
                                   \n - 1: Enable. */
    RECT_S subWinRect;        /**< \brief The HIST256 window. */
    vsi_u32_t mode;           /**< \brief The histogram mode.
                                   \n Valid values: See <tt> \ref ISP_HIST256_MODE_E</tt>. */
    vsi_u8_t step;            /**< \brief The histogram step.
                                   \n Valid value range: [3, 127]. */
    vsi_u8_t weight[HIST256_H_GRID_ITEMS][HIST256_V_GRID_ITEMS];    /**< \brief The histogram weights.
                                                                           \n Valid value range: [0, 16]. */
} ISP_HIST256_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the HIST256 attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pHist256Attr        A pointer to a memory place for receiving the HIST256 attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetHist256Attr(ISP_PORT IspPort, ISP_HIST256_ATTR_S *pHist256Attr);

/*****************************************************************************/
/**
 * @brief   Sets the HIST256 attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pHist256Attr        A pointer to the HIST256 attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetHist256Attr(ISP_PORT IspPort, ISP_HIST256_ATTR_S *pHist256Attr);

/*****************************************************************************/
/**
 * @brief   Gets the HIST256 statistics of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pHistStat           A pointer to a memory place for receiving the HIST256 statistics.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetHist256Statistics(ISP_PORT IspPort, ISP_HIST256_STATISTICS_S *pHistStat);

/* @} mpi_isp_hist256 */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
