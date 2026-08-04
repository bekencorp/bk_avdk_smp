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

#ifndef __MPI_ISP_DPF_H__
#define __MPI_ISP_DPF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond DPF_V10
 *
 * @defgroup mpi_isp_dpf DPF V10 Definitions
 * @{
 *
 */

#define DPF_NOISE_CURVE_SIZE 17    /**< \brief The number of control points on the DPF noise curve. */

#define VSI_ISP_DPF_STRENGTH_MAX    65    /**< \brief The maximum value of <tt>pinvStrength</tt> for DPF. */
#define VSI_ISP_DPF_SIGMA_G_MIN     16    /**< \brief The minimum value of <tt>sigmaG</tt> for DPF. */
#define VSI_ISP_DPF_SIGMA_G_MAX     2048  /**< \brief The maximum value of <tt>sigmaG</tt> for DPF. */
#define VSI_ISP_DPF_SIGMA_RB_MIN    16    /**< \brief The minimum value of <tt>sigmaRb</tt> for DPF. */
#define VSI_ISP_DPF_SIGMA_RB_MAX    2048  /**< \brief The maximum value of <tt>sigmaRb</tt> for DPF. */
#define VSI_ISP_DPF_NOISE_CURVE_MAX 1023  /**< \brief The maximum value of <tt>noiseCurve</tt> for DPF. */

/** \brief Contains the DPF attributes for use in manual mode. */
typedef struct vsiISP_DPF_MANUAL_ATTR_S {
    vsi_u8_t   pinvStrength;                   /**< \brief The DPF strength.
                                                    \n A greater value indicates weaker denoising effects.
                                                    \n Valid value range: (0, 65]. */
    vsi_u16_t  sigmaG;                         /**< \brief The Gaussian sigma for the green channel.
                                                    \n A greater value indicates larger weights of neighboring pixels.
                                                    \n Valid value range: [16, 2048].
                                                    \n Value range for the 4-bit fractional part: [1.0, 128.0]. */
    vsi_u16_t  sigmaRb;                        /**< \brief The Gaussian sigma for the red and blue channels.
                                                    \n A greater value indicates larger weights of neighboring pixels.
                                                    \n Valid value range: [16, 2048].
                                                    \n Value range for the 4-bit fractional part: [1.0 128.0]. */
    vsi_u16_t  noiseCurve[DPF_NOISE_CURVE_SIZE]; /**< \brief The NUV noise curve.
                                                      \n A smaller value indicates stronger denoising effects.
                                                      \n Valid value range: [0, 1023].
                                                      \n Value range for the 8-bit fractional part: [0 4095.0]. */
} ISP_DPF_MANUAL_ATTR_S;

/** \brief Contains the DPF attributes for use in auto mode. */
typedef struct vsiISP_DPF_AUTO_ATTR_S {
    vsi_u8_t   pinvStrength[ISP_AUTO_STRENGTH_NUM];  /**< \brief The DPF strength.
                                                          \n A greater value indicates weaker denoising effects.
                                                          \n Valid value range: (0, 65]. */
    vsi_u16_t  sigmaG[ISP_AUTO_STRENGTH_NUM];        /**< \brief The Gaussian sigma for the green channel.
                                                          \n A greater value indicates larger weights of neighboring pixels.
                                                          \n Valid value range: [16, 2048].
                                                          \n Value range for the 4-bit fractional part: [1.0, 128.0]. */
    vsi_u16_t  sigmaRb[ISP_AUTO_STRENGTH_NUM];       /**< \brief The Gaussian sigma for the red and blue channels.
                                                          \n A greater value indicates larger weights of neighboring pixels.
                                                          \n Valid value range: [16, 2048].
                                                          \n Value range for the 4-bit fractional part: [1.0 128.0]. */
    vsi_u16_t  noiseCurve[ISP_AUTO_STRENGTH_NUM][DPF_NOISE_CURVE_SIZE];  /**< \brief The NUV noise curve.
                                                                              \n A smaller value indicates stronger denoising effects.
                                                                              \n Valid value range: [0, 1023].
                                                                              \n Value range for the 8-bit fractional part: [0 4095.0]. */
} ISP_DPF_AUTO_ATTR_S;

/** \brief Contains the DPF attributes. */
typedef struct  vsiISP_DPF_ATTR_S {
    vsi_bool_t enable;                 /**< \brief Whether to enable DPF.
                                            \n Valid values;
                                            \n - 0: Disable.
                                            \n - 1: Enable. */
    vsi_u32_t  opType;                 /**< \brief The operation mode of DPF.
                                            \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_DPF_MANUAL_ATTR_S manualAttr;  /**< \brief The DPF attributes for use in manual mode. */
    ISP_DPF_AUTO_ATTR_S autoAttr;      /**< \brief The DPF attributes for use in auto mode. */
} ISP_DPF_ATTR_S;

/** \brief Contains the DPF metadata that needs to be written into registers. */
typedef struct vsiISP_DPF_S {
    vsi_bool_t enable;        /**< \brief Whether to enable DPF.
                                   \n Valid values;
                                   \n - 0: Disable.
                                   \n - 1: Enable. */
    vsi_u8_t   pinvStrength;  /**< \brief The DPF strength.
                                   \n A greater value indicates weaker denoising effects.
                                   \n Valid value range: (0, 65]. */
    vsi_u16_t  sigmaG;        /**< \brief The Gaussian sigma for the green channel.
                                   \n A greater value indicates larger weights of neighboring pixels.
                                   \n Valid value range: [16, 2048].
                                   \n Value range for the 4-bit fractional part: [1.0, 128.0]. */
    vsi_u16_t  sigmaRb;       /**< \brief The Gaussian sigma for the red and blue channels.
                                   \n A greater value indicates larger weights of neighboring pixels.
                                   \n Valid value range: [16, 2048].
                                   \n Value range for the 4-bit fractional part: [1.0 128.0]. */
    vsi_u16_t  noiseCurve[DPF_NOISE_CURVE_SIZE]; /**< \brief The NUV noise curve.
                                                      \n A smaller value indicates stronger denoising effects.
                                                      \n Valid value range: [0, 1023].
                                                      \n Value range for the 8-bit fractional part: [0 4095.0]. */
} ISP_DPF_META_S;


/*****************************************************************************/
/**
 * @brief   Gets the DPF attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pDpfAttr            A pointer to a memory place for receiving the DPF attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetDpfAttr(ISP_PORT IspPort, ISP_DPF_ATTR_S *pDpfAttr);


/*****************************************************************************/
/**
 * @brief   Sets the DPF attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pDpfAttr            A pointer to the DPF attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetDpfAttr(ISP_PORT IspPort, ISP_DPF_ATTR_S *pDpfAttr);

/* @} mpi_isp_dpf */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
