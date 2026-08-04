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

#ifndef __MPI_ISP_CCM_H__
#define __MPI_ISP_CCM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond CCM_V10
 *
 * @defgroup mpi_isp_ccm CCM V10 Definitions
 * @{
 *
 */

#define VSI_ISP_CCM_MATRIX_SIZE        9           /**< \brief The size of the CCM. */

#define VSI_ISP_CCM_MATRIX_MIN         -1024       /**< \brief The minimum value of <tt>colorMatrix</tt> for CCM. */
#define VSI_ISP_CCM_MATRIX_MAX         1023        /**< \brief The maximum value of <tt>colorMatrix</tt> for CCM. */

#define VSI_ISP_CCM_OFFSET_MIN         -2048       /**< \brief The minimum value of each color offset parameter for CCM. */
#define VSI_ISP_CCM_OFFSET_MAX         2047        /**< \brief The maximum value of each color offset parameter for CCM. */

#define VSI_ISP_COLOR_TEMP_MIN         2000        /**< \brief The minimum value of <tt>colorTemp</tt> for CCM. */
#define VSI_ISP_COLOR_TEMP_MAX         10000       /**< \brief The maximum value of <tt>colorTemp</tt> for CCM. */

/** \brief Contains the CCM attributes for use in manual mode. */
typedef struct vsiISP_CCM_MANUAL_ATTR_S {
    vsi_s16_t colorMatrix[VSI_ISP_CCM_MATRIX_SIZE]; /**< \brief The CCM coefficients.
                                                         \n Valid value range: [-1024, 1023].
                                                         \n Value range for the 7-bit fractional part: (-8, 7.996). */
    vsi_s16_t rOffset;   /**< \brief The red color offset.
                              \n Valid value range: [-2048, 2047]. */
    vsi_s16_t gOffset;   /**< \brief The green color offset.
                              \n Valid value range: [-2048, 2047]. */
    vsi_s16_t bOffset;   /**< \brief The blue color offset.
                              \n Valid value range: [-2048, 2047]. */
} ISP_CCM_MANUAL_ATTR_S;

/** \brief Contains the CCM attributes of an illuminant. */
typedef struct vsiISP_ILLUMINANT_CCM_S
{
    vsi_s16_t colorMatrix[VSI_ISP_CCM_MATRIX_SIZE]; /**< \brief The CCM coefficients.
                                                         \n Valid value range: [-1024, 1023].
                                                         \n Value range for the 7-bit fractional part: (-8, 7.996). */
    vsi_s16_t rOffset;   /**< \brief The red color offset.
                              \n Valid value range: [-2048, 2047]. */
    vsi_s16_t gOffset;   /**< \brief The green color offset.
                              \n Valid value range: [-2048, 2047]. */
    vsi_s16_t bOffset;   /**< \brief The blue color offset.
                              \n Valid value range: [-2048, 2047]. */
    vsi_u32_t colorTemp; /**< \brief The color temperature.
                              \n Valid value range: [2000, 10000]. */
} ISP_ILLUMINANT_CCM_S;

/** \brief Contains the CCM attributes for use in auto mode. */
typedef struct vsiISP_CCM_AUTO_ATTR_S {
    ISP_ILLUMINANT_CCM_S illuminantCCM[ILLUMINANT_TYPE_CNT]; /**< \brief The CCM attributes of each illuminant. */
} ISP_CCM_AUTO_ATTR_S;

/** \brief Contains the CCM attributes. */
typedef struct vsiISP_CCM_ATTR_S {
    vsi_u32_t             opType;     /**< \brief The operation mode of CCM.
                                           \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_CCM_MANUAL_ATTR_S manualAttr; /**< \brief The CCM attributes for use in manual mode. */
    ISP_CCM_AUTO_ATTR_S   autoAttr;   /**< \brief The CCM attributes for use in auto mode. */
} ISP_CCM_ATTR_S;

/** \brief CCM metadata that needs to be written into registers. */
typedef ISP_CCM_MANUAL_ATTR_S ISP_CCM_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the CCM attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pCcmAttr            A pointer to a memory place for receiving the CCM attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetCcmAttr(ISP_PORT IspPort, ISP_CCM_ATTR_S *pCcmAttr);

/*****************************************************************************/
/**
 * @brief   Sets the CCM attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pCcmAttr            A pointer to the CCM attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetCcmAttr(ISP_PORT IspPort, ISP_CCM_ATTR_S *pCcmAttr);

/* @} mpi_isp_ccm */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
