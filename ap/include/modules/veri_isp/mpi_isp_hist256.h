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

#define VSI_ISP_HIST_STATIC_BLOCK_NUM 256   /**< \brief The number of bins */
#define HIST256_H_GRID_ITEMS 5               /**< \brief The number of horizontal grid sub windows */
#define HIST256_V_GRID_ITEMS 5               /**< \brief The number of vertical grid sub windows */

#define VSI_ISP_HIST256_STEP_MIN 3      /**< \brief The minimum value of HIST256 step. */
#define VSI_ISP_HIST256_STEP_MAX 127    /**< \brief The maximum value of HIST256 step. */
#define VSI_ISP_HIST256_WEIGHT_MAX 16   /**< \brief The maximum value of HIST256 weight. */

/** \brief   HIST256 attributes. */
typedef struct vsiISP_HIST256_STATISTICS_S {
    vsi_u32_t histogram[VSI_ISP_HIST_STATIC_BLOCK_NUM]; /**< \brief Histogram statistics */
} ISP_HIST256_STATISTICS_S;

/** \brief   HIST256 mode. */
typedef enum vsiISP_HIST256_MODE_E {
    HIST256_MODE_MIN = 1,  /**< \brief The minimum value of Histogram mode */
    HIST256_R_MODE = 2,    /**< \brief Histogram red mode */
    HIST256_G_MODE = 3,    /**< \brief Histogram green mode */
    HIST256_B_MODE = 4,    /**< \brief Histogram blue mode */
    HIST256_Y_MODE = 5,    /**< \brief Histogram y mode */
    HIST256_MODE_MAX,      /**< \brief The maximum value of Histogram mode */
} ISP_HIST256_MODE_E;

/** \brief   HIST256 attributes. */
typedef struct vsiISP_HIST256_ATTR_S {
    vsi_bool_t enable;        /**< \brief Whether to enable histogram. \n 0: Disable histogram. \n 1: Enable histogram. */
    RECT_S     measRect;      /**< \brief Rectangle of HIST256 measurement */
    vsi_u32_t  mode;          /**< \brief Histogram mode, reference ISP_HIST256_MODE_E */
    vsi_u8_t   step;          /**< \brief Histogram step set. Range [3, 127] */
    vsi_u8_t   weight[HIST256_H_GRID_ITEMS][HIST256_V_GRID_ITEMS];    /**< \brief Histogram weight. Range [0, 16] */
} ISP_HIST256_ATTR_S;


/** \brief   HIST256 metadata structure that need to be written into registers. */
typedef struct vsiISP_HIST256_S {
    vsi_bool_t enable;        /**< \brief Whether to enable histogram. \n 0: Disable histogram. \n 1: Enable histogram. */
    RECT_S subWinRect;        /**< \brief Sub-window rectangle of HIST256 measurement */
    ISP_HIST256_MODE_E mode;  /**< \brief Histogram mode */
    vsi_u8_t step;            /**< \brief Histogram step set. Range [3, 127] */
    vsi_u8_t weight[HIST256_H_GRID_ITEMS][HIST256_V_GRID_ITEMS];    /**< \brief Histogram weight. Range [0, 16] */
} ISP_HIST256_META_S;

/*****************************************************************************/
/**
 * @brief   Gets HIST256 configurations.
 *
 * @param   IspPort             Port ID
 * @param   pHist256Attr      Pointer to the HIST256 configurations
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetHist256Attr(ISP_PORT IspPort, ISP_HIST256_ATTR_S *pHist256Attr);

/*****************************************************************************/
/**
 * @brief   Sets HIST256 configurations.
 *
 * @param   IspPort             Port ID
 * @param   pHist256Attr      Pointer to the HIST256 configurations

 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetHist256Attr(ISP_PORT IspPort, ISP_HIST256_ATTR_S *pHist256Attr);

/*****************************************************************************/
/**
 * @brief   Gets HIST256 statistics.
 *
 * @param   IspPort             Port ID
 * @param   pHistStat         Pointer to the HIST256 statistics
 *
 * @retval  VSI_SUCCESS         Operation succeeded
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
