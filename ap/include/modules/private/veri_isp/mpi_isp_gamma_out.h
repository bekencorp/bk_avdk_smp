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

#ifndef __MPI_ISP_GAMMA_OUT_H__
#define __MPI_ISP_GAMMA_OUT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond GAMMA_OUT_V20
 *
 * @defgroup mpi_isp_gamma_out GAMMA_OUT V20 Definitions
 * @{
 *
 */

#define ISP_GAMMA_OUT_NODE_NUM 64                  /**< \brief The number of GAMMA OUT nodes. */

#define VSI_ISP_GAMMA_OUT_RED_PX_MAX 12            /**< \brief The maximum value of GAMMA OUT red px. */
#define VSI_ISP_GAMMA_OUT_RED_PX_MIN 0             /**< \brief The minimum value of GAMMA OUT red px. */
#define VSI_ISP_GAMMA_OUT_RED_X_MAX  4095          /**< \brief The maximum value of GAMMA OUT red x. */
#define VSI_ISP_GAMMA_OUT_RED_X_MIN  0             /**< \brief The minimum value of GAMMA OUT red x. */
#define VSI_ISP_GAMMA_OUT_RED_Y_MAX  1023          /**< \brief The maximum value of GAMMA OUT red y. */
#define VSI_ISP_GAMMA_OUT_RED_Y_MIN  0             /**< \brief The minimum value of GAMMA OUT red y. */

#define VSI_ISP_GAMMA_OUT_GREEN_PX_MAX 12            /**< \brief The maximum value of GAMMA OUT green px. */
#define VSI_ISP_GAMMA_OUT_GREEN_PX_MIN 0             /**< \brief The minimum value of GAMMA OUT green px. */
#define VSI_ISP_GAMMA_OUT_GREEN_X_MAX  4095          /**< \brief The maximum value of GAMMA OUT green x. */
#define VSI_ISP_GAMMA_OUT_GREEN_X_MIN  0             /**< \brief The minimum value of GAMMA OUT green x. */
#define VSI_ISP_GAMMA_OUT_GREEN_Y_MAX  1023          /**< \brief The maximum value of GAMMA OUT green y. */
#define VSI_ISP_GAMMA_OUT_GREEN_Y_MIN  0             /**< \brief The minimum value of GAMMA OUT green y. */

#define VSI_ISP_GAMMA_OUT_BLUE_PX_MAX 12            /**< \brief The maximum value of GAMMA OUT blue px. */
#define VSI_ISP_GAMMA_OUT_BLUE_PX_MIN 0             /**< \brief The minimum value of GAMMA OUT blue px. */
#define VSI_ISP_GAMMA_OUT_BLUE_X_MAX  4095          /**< \brief The maximum value of GAMMA OUT blue x. */
#define VSI_ISP_GAMMA_OUT_BLUE_X_MIN  0             /**< \brief The minimum value of GAMMA OUT blue x. */
#define VSI_ISP_GAMMA_OUT_BLUE_Y_MAX  1023          /**< \brief The maximum value of GAMMA OUT blue y. */
#define VSI_ISP_GAMMA_OUT_BLUE_Y_MIN  0             /**< \brief The minimum value of GAMMA OUT blue y. */


// 12bits to 10bits gamma


/** \brief   GAMMA out manual attributes. */
typedef struct vsiISP_GAMMA_OUT_MANUAL_ATTR_S {
    vsi_u8_t  redPx[ISP_GAMMA_OUT_NODE_NUM];     /**< \brief Distance of x in gamma for red.
                                                        \n Range: [0, 12]
                                                        \n dxn = 1 << pxn*/
    vsi_u16_t redX[ISP_GAMMA_OUT_NODE_NUM];     /**< \brief X data in gamma curve for red.
                                                        \n Range: [0, 4095],
                                                        \n x[0] is 0, and x[1] to x[64] are assigned to redX[0] to redX[63]. */
    vsi_u16_t redY[ISP_GAMMA_OUT_NODE_NUM];     /**< \brief Y data in gamma curve for red.
                                                         \n Range: [0, 1023]
                                                         \n y[0] is 0, and y[1] to y[64] are assigned to redY[0] to redY[63]. */

    vsi_u8_t  greenPx[ISP_GAMMA_OUT_NODE_NUM];   /**< \brief Distance of x in Gamma for green.
                                                        \n Range: [0, 12]
                                                        \n dxn = 1 << pxn*/
    vsi_u16_t greenX[ISP_GAMMA_OUT_NODE_NUM];   /**< \brief X data in gamma curve for green.
                                                        \n Range: [0, 4095],
                                                        \n x[0] is 0, and x[1] to x[64] are assigned to greenX[0] to greenX[63]. */
    vsi_u16_t greenY[ISP_GAMMA_OUT_NODE_NUM];   /**< \brief Y data in gamma curve for green.
                                                         \n Range: [0, 1023]
                                                         \n y[0] is 0, and y[1] to y[64] are assigned to greenY[0] to greenY[63]. */

    vsi_u8_t  bluePx[ISP_GAMMA_OUT_NODE_NUM];    /**< \brief Distance of x in Gamma for blue.
                                                        \n Range: [0, 12],
                                                        \n dxn = 1 << pxn*/
    vsi_u16_t blueX[ISP_GAMMA_OUT_NODE_NUM];    /**< \brief X data in gamma curve for blue.
                                                        \n Range: [0, 4095],
                                                        \n x[0] is 0, and x[1] to x[64] are assigned to blueX[0] to blueX[63]. */
    vsi_u16_t blueY[ISP_GAMMA_OUT_NODE_NUM];    /**< \brief Y data in gamma curve for blue.
                                                         \n Range: [0, 1023]
                                                         \n y[0] is 0, and y[1] to y[64] are assigned to blueY[0] to blueY[63]. */
} ISP_GAMMA_OUT_CURVE_ATTR_S;


/** \brief   GAMMA out attributes. */
typedef struct vsiISP_GAMMA_OUT_ATTR_S {
    vsi_bool_t enable;                     /**< \brief Whether to enable GAMMA OUT. \n 0: Disable GAMMA OUT. \n 1: Enable GAMMA OUT. */
    ISP_GAMMA_OUT_CURVE_ATTR_S curve;     /**< \brief GAMMA OUT curve attributes */
} ISP_GAMMA_OUT_ATTR_S;

/** \brief GAMMA_OUT metadata structure that need to be written into registers. */
typedef ISP_GAMMA_OUT_ATTR_S ISP_GAMMA_OUT_META_S;

/*****************************************************************************/
/**
 * @brief   Gets GAMMA OUT attributes.
 *
 * @param   IspPort             Port ID
 * @param   pGammaOutAttr     Pointer to the GAMMA OUT attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetGammaOutAttr(ISP_PORT IspPort, ISP_GAMMA_OUT_ATTR_S *pGammaOutAttr);

/*****************************************************************************/
/**
 * @brief   Sets GAMMA OUT attributes.
 *
 * @param   IspPort             Port ID
 * @param   pGammaOutAttr     Pointer to the GAMMA OUT attributes

 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetGammaOutAttr(ISP_PORT IspPort, ISP_GAMMA_OUT_ATTR_S *pGammaOutAttr);

/* @} mpi_isp_gamma_out */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
