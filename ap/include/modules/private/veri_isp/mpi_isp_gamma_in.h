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

#ifndef __MPI_ISP_GAMMA_IN_H__
#define __MPI_ISP_GAMMA_IN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond GAMMA_IN_V10
 *
 * @defgroup mpi_isp_gamma_in Gamma In V10 Definitions
 * @{
 *
 */


#define VSI_ISP_GAMMA_IN_PX_MAX 7             /**< \brief The maximum value of <tt>pX</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_PX_MIN 0             /**< \brief The minimum value of <tt>pX</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_RY_MAX 4095          /**< \brief The maximum value of <tt>rY</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_RY_MIN 0             /**< \brief The minimum value of <tt>rY</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_GY_MAX 4095          /**< \brief The maximum value of <tt>gY</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_GY_MIN 0             /**< \brief The minimum value of <tt>gY</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_BY_MAX 4095          /**< \brief The maximum value of <tt>bY</tt> for gamma in. */
#define VSI_ISP_GAMMA_IN_BY_MIN 0             /**< \brief The minimum value of <tt>bY</tt> for gamma in. */

/** \brief Contains the gamma in attributes. */
typedef struct vsiISP_GAMMA_IN_ATTR_S {
    vsi_bool_t enable;  /**< \brief Whether to enable gamma in.
                             \n Valid values:
                             \n - 0: Disable.
                             \n - 1: Enable. */
    vsi_u8_t  pX[16];   /**< \brief The X-axis distance between adjacent points on the gamma curve.
                             \n dxn = 1 << (pxn + 4)
                             \n Valid value range: [0, 7]. */
    vsi_u16_t rY[17];   /**< \brief The Y coordinate for red value on the gamma curve.
                             \n Valid value range: [0, 4095]. */
    vsi_u16_t gY[17];   /**< \brief The Y coordinate for green value on the gamma curve.
                             \n Valid value range: [0, 4095]. */
    vsi_u16_t bY[17];   /**< \brief The Y coordinate for blue value on the gamma curve.
                             \n Valid value range: [0, 4095]. */
} ISP_GAMMA_IN_ATTR_S;

/** \brief Gamma in metadata that needs to be written into registers. */
typedef ISP_GAMMA_IN_ATTR_S ISP_GAMMAIN_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the gamma in attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pGammaInAttr        A pointer to a memory place for receiving the gamma in attributes
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetGammaInAttr(ISP_PORT IspPort, ISP_GAMMA_IN_ATTR_S *pGammaInAttr);

/*****************************************************************************/
/**
 * @brief   Sets the gamma in attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pGammaInAttr        A pointer to the gamma in attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetGammaInAttr(ISP_PORT IspPort, ISP_GAMMA_IN_ATTR_S *pGammaInAttr);

/* @} mpi_isp_gamma_in */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
