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

#ifndef __MPI_ISP_GAMMA_OUT_V20_H__
#define __MPI_ISP_GAMMA_OUT_V20_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond GAMMA_OUT_V20
 *
 * @defgroup mpi_isp_gamma_out Gamma Out V20 Definitions
 * @{
 *
 */

#define ISP_GAMMA_OUT_NODE_PX_NUM 64               /**< \brief The number of Px values of the gamma out curve. */
#define ISP_GAMMA_OUT_NODE_X_NUM 63                /**< \brief The number of X values of the gamma out curve. */
#define ISP_GAMMA_OUT_NODE_Y_NUM 64                /**< \brief The number of Y values of the gamma out curve. */

#define VSI_ISP_GAMMA_OUT_RED_PX_MAX 12            /**< \brief The maximum value of <tt>redPx</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_RED_PX_MIN 0             /**< \brief The minimum value of <tt>redPx</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_RED_X_MAX  4095          /**< \brief The maximum value of <tt>redX</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_RED_X_MIN  0             /**< \brief The minimum value of <tt>redX</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_RED_Y_MAX  1023          /**< \brief The maximum value of <tt>redY</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_RED_Y_MIN  0             /**< \brief The minimum value of <tt>redY</tt> for gamma out. */

#define VSI_ISP_GAMMA_OUT_GREEN_PX_MAX 12          /**< \brief The maximum value of <tt>greenPx</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_GREEN_PX_MIN 0           /**< \brief The minimum value of <tt>greenPx</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_GREEN_X_MAX  4095        /**< \brief The maximum value of <tt>greenX</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_GREEN_X_MIN  0           /**< \brief The minimum value of <tt>greenX</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_GREEN_Y_MAX  1023        /**< \brief The maximum value of <tt>greenY</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_GREEN_Y_MIN  0           /**< \brief The minimum value of <tt>greenY</tt> for gamma out. */

#define VSI_ISP_GAMMA_OUT_BLUE_PX_MAX 12           /**< \brief The maximum value of <tt>bluePx</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_BLUE_PX_MIN 0            /**< \brief The minimum value of <tt>bluePx</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_BLUE_X_MAX  4095         /**< \brief The maximum value of <tt>blueX</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_BLUE_X_MIN  0            /**< \brief The minimum value of <tt>blueX</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_BLUE_Y_MAX  1023         /**< \brief The maximum value of <tt>blueY</tt> for gamma out. */
#define VSI_ISP_GAMMA_OUT_BLUE_Y_MIN  0            /**< \brief The minimum value of <tt>blueY</tt> for gamma out. */


// 12bits to 10bits gamma


/** \brief Contains the gamma curve attributes of gamma out. */
typedef struct vsiISP_GAMMA_OUT_MANUAL_ATTR_S {
    vsi_u8_t  redPx[ISP_GAMMA_OUT_NODE_PX_NUM];    /**< \brief The distance along the X-axis between every two adjacent control points on the luma curve for the R channel.
                                                     \n The value of this field equals the base-2 logarithm of the distance.
                                                     \n Valid value range: [0, 12]. */
    vsi_u16_t redX[ISP_GAMMA_OUT_NODE_X_NUM];     /**< \brief The X coorindate of each control point on the gamma curve for the R channel.
                                                     \n <tt>redX[0]</tt> to <tt>redX[63]</tt> specify the X coordinates of control points 1 to 64. The X coordinate of control point 0 is fixed to 0.
                                                     \n Valid value range: [0, 4095]. */
    vsi_u16_t redY[ISP_GAMMA_OUT_NODE_Y_NUM];     /**< \brief The Y coorindate of each control point on the gamma curve for the R channel.
                                                     \n <tt>redX[0]</tt> to <tt>redX[63]</tt> specify the Y coordinates of control points 1 to 64. The Y coordinate of control point 0 is fixed to 0.
                                                     \n Valid value range: [0, 1023]. */

    vsi_u8_t  greenPx[ISP_GAMMA_OUT_NODE_PX_NUM];  /**< \brief The distance along the X-axis between every two adjacent control points on the luma curve for the G channel.
                                                     \n The value of this field equals the base-2 logarithm of the distance.
                                                     \n Valid value range: [0, 12]. */
    vsi_u16_t greenX[ISP_GAMMA_OUT_NODE_X_NUM];   /**< \brief The X coorindate of each control point on the gamma curve for the G channel.
                                                     \n <tt>greenX[0]</tt> to <tt>greenX[63]</tt> specify the X coordinates of control points 1 to 64. The X coordinate of control point 0 is fixed to 0.
                                                     \n Valid value range: [0, 4095]. */
    vsi_u16_t greenY[ISP_GAMMA_OUT_NODE_Y_NUM];   /**< \brief The Y coorindate of each control point on the gamma curve for the R channel.
                                                     \n <tt>greenY[0]</tt> to <tt>greenY[63]</tt> specify the Y coordinates of control points 1 to 64. The Y coordinate of control point 0 is fixed to 0.
                                                     \n Valid value range: [0, 1023]. */

    vsi_u8_t  bluePx[ISP_GAMMA_OUT_NODE_PX_NUM];   /**< \brief The distance along the X-axis between every two adjacent control points on the luma curve for the B channel.
                                                     \n The value of this field equals the base-2 logarithm of the distance.
                                                     \n Valid value range: [0, 12]. */
    vsi_u16_t blueX[ISP_GAMMA_OUT_NODE_X_NUM];    /**< \brief The X coorindate of each control point on the gamma curve for the B channel.
                                                     \n <tt>blueX[0]</tt> to <tt>blueX[63]</tt> specify the X coordinates of control points 1 to 64. The X coordinate of control point 0 is fixed to 0.
                                                     \n Valid value range: [0, 4095]. */
    vsi_u16_t blueY[ISP_GAMMA_OUT_NODE_Y_NUM];    /**< \brief The Y coorindate of each control point on the gamma curve for the R channel.
                                                     \n <tt>blueY[0]</tt> to <tt>blueY[63]</tt> specify the Y coordinates of control points 1 to 64. The Y coordinate of control point 0 is fixed to 0.
                                                     \n Valid value range: [0, 1023]. */
} ISP_GAMMA_OUT_CURVE_ATTR_S;


/** \brief Contains the gamma out attributes. */
typedef struct vsiISP_GAMMA_OUT_V20_ATTR_S {
    vsi_bool_t enable;                    /**< \brief Whether to enable gamma out.
                                               \n Valid values:
                                               \n - 0: Disable.
                                               \n - 1: Enable. */
    ISP_GAMMA_OUT_CURVE_ATTR_S curve;     /**< \brief The gamma curve attributes of gamma out. */
} ISP_GAMMA_OUT_V20_ATTR_S;

/** \brief Gamma out metadata that needs to be written into registers. */
typedef ISP_GAMMA_OUT_V20_ATTR_S ISP_GAMMA_OUT_V20_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the gamma out attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pGammaOutAttr       A pointer to a memory place for receiving the gamma out attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetGammaOutV20Attr(ISP_PORT IspPort, ISP_GAMMA_OUT_V20_ATTR_S *pGammaOutAttr);

/*****************************************************************************/
/**
 * @brief   Sets the gamma out attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pGammaOutAttr       A pointer to the gamma out attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetGammaOutV20Attr(ISP_PORT IspPort, ISP_GAMMA_OUT_V20_ATTR_S *pGammaOutAttr);

/* @} mpi_isp_gamma_out */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
