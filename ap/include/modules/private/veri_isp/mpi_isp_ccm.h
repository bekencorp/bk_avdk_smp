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

#define VSI_ISP_CCM_MATRIX_SIZE        9           /**< \brief The matrix size of color correction . */

#define VSI_ISP_CCM_MATRIX_MIN         -1024       /**< \brief The minimum value of color matrix coefficient. */
#define VSI_ISP_CCM_MATRIX_MAX         1023        /**< \brief The maximum value of color matrix coefficient. */

#define VSI_ISP_CCM_OFFSET_MIN         -2048       /**< \brief The minimum value of color offset coefficient. */
#define VSI_ISP_CCM_OFFSET_MAX         2047        /**< \brief The maximum value of color offset coefficient. */

#define VSI_ISP_COLOR_TEMP_MIN         2000        /**< \brief The minimum value of color temperature. */
#define VSI_ISP_COLOR_TEMP_MAX         10000       /**< \brief The maximum value of color temperature. */

/** \brief   CCM manual attributes. */
typedef struct vsiISP_CCM_MANUAL_ATTR_S {
    vsi_s16_t colorMatrix[VSI_ISP_CCM_MATRIX_SIZE]; /**< \brief Color correction matrix coefficient. The value range is [-1024, 1023].
                                                    \n The value range for the 7-bit fractional part is (-8, 7.996,).
                                                    \n (-1024 / 128) = -8
                                                    \n (1023 / 128) = 7.996*/
    vsi_s16_t rOffset;   /**< \brief Red color offset coefficient.
                                \n Range [-2048, 2047]*/
    vsi_s16_t gOffset;   /**< \brief Green color offset coefficient.
                                \n Range [-2048, 2047]*/
    vsi_s16_t bOffset;   /**< \brief Blue color offset coefficient.
                                \n Range [-2048, 2047]*/
} ISP_CCM_MANUAL_ATTR_S;

/** \brief   CCM illuminant attributes. */
typedef struct vsiISP_ILLUMINANT_CCM_S
{
    vsi_s16_t colorMatrix[VSI_ISP_CCM_MATRIX_SIZE]; /**< \brief Color correction matrix coefficient. The value range is [-1024 1023].
                                                    \n The value range for the 7-bit fractional part is (-8, 7.996).
                                                    \n (-1024 / 128) = -8
                                                    \n (1023 / 128) = 7.996*/
    vsi_s16_t rOffset;   /**< \brief Red color offset coefficient.
                                \n Range [-2048, 2047]*/
    vsi_s16_t gOffset;   /**< \brief Green color offset coefficient.
                                \n Range [-2048, 2047]*/
    vsi_s16_t bOffset;   /**< \brief Blue color offset coefficient.
                                \n Range [-2048, 2047]*/
    vsi_u32_t colorTemp;   /**< \brief Color temperature value.
                                \n Range [2000, 10000]*/
} ISP_ILLUMINANT_CCM_S;

/** \brief   CCM auto attributes. */
typedef struct vsiISP_CCM_AUTO_ATTR_S {
    ISP_ILLUMINANT_CCM_S illuminantCCM[ILLUMINANT_TYPE_CNT]; /**< \brief CCM illuminant attributes */
} ISP_CCM_AUTO_ATTR_S;

/** \brief   CCM attributes. */
typedef struct vsiISP_CCM_ATTR_S {
    vsi_u32_t             opType; /**< \brief CCM mode attributes. For details, see <tt>ISP_OP_TYPE_E</tt>. */
    ISP_CCM_MANUAL_ATTR_S manualAttr; /**< \brief CCM manual attributes */
    ISP_CCM_AUTO_ATTR_S   autoAttr;   /**< \brief CCM auto attributes */
} ISP_CCM_ATTR_S;

/** \brief CCM metadata structure that need to be written into registers. */
typedef ISP_CCM_MANUAL_ATTR_S ISP_CCM_META_S;

/*****************************************************************************/
/**
 * @brief   Gets CCM attributes.
 *
 * @param   IspPort             Port ID.
 * @param   pCcmAttr          Pointer to the CCM attributes.
 *
 * @retval  VSI_SUCCESS         Operation succeeded.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetCcmAttr(ISP_PORT IspPort, ISP_CCM_ATTR_S *pCcmAttr);

/*****************************************************************************/
/**
 * @brief   Sets CCM configuration parameters.
 *
 * @param   IspPort             Port ID.
 * @param   pCcmAttr          Pointer to the CCM attributes.
 *
 * @retval  VSI_SUCCESS         Operation succeeded.
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
