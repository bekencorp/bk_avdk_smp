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

#ifndef __MPI_ISP_CSM_H__
#define __MPI_ISP_CSM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond CSM_V10
 *
 * @defgroup mpi_isp_csm CSM V10 Definitions
 * @{
 *
 */

/** \brief Defines the CSM types. */
typedef enum vsiISP_CSM_TYPE_E {
    ISP_CSM_TYPE_601 = 0,   /**< \brief CSM based on ITU-R BT.601. */
    ISP_CSM_TYPE_709,       /**< \brief CSM based on ITU-R BT.709. */
    ISP_CSM_TYPE_USER,      /**< \brief User-defined CSM. */
} ISP_CSM_TYPE_E;

/** \brief Defines the CSM quantization ranges. */
typedef enum vsiISP_CSM_QUANTIZATION_E {
    ISP_CSM_LIM_RANGE  = 0, /**< \brief Limited range. */
    ISP_CSM_FULL_RANGE = 1, /**< \brief Full range. */
} ISP_CSM_QUANTIZATION_E;

/** \brief Contains the CSM attributes. */
typedef struct vsiISP_CSM_ATTR_S {
    vsi_u32_t type;          /**< \brief The CSM type.
                                  \n Valid values: See <tt> \ref ISP_CSM_TYPE_E</tt>. */
    vsi_u32_t quantization;  /**< \brief The CSM quantization range.
                                  \n Valid values: See <tt> \ref ISP_CSM_QUANTIZATION_E</tt>. */
    vsi_s16_t coef[9];       /**< \brief The CSM coefficients.
                                  \n Valid value range: [-256, 255].
                                  \n Value range for the 7-bit fractional part: (-2, 1.996). */
} ISP_CSM_ATTR_S;

/** \brief Contains the CSM metadata that needs to be written into registers. */
typedef struct vsiISP_CSM_S {
    vsi_u32_t quantization; /**< \brief The CSM quantization range.
                                 \n Valid values: See <tt> \ref ISP_CSM_QUANTIZATION_E</tt>. */
    vsi_s16_t coef[9];      /**< \brief The CSM coefficients.
                                 \n Valid value range: [-256, 255].
                                 \n Value range for the 8-bit fractional part: (-1, 0.996). */
} ISP_CSM_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the CSM attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pCsmAttr            A pointer to a memory place for receiving the CSM attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetCsmAttr(ISP_PORT IspPort, ISP_CSM_ATTR_S *pCsmAttr);

/*****************************************************************************/
/**
 * @brief   Sets the CSM attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pCsmAttr            A pointer to the CSM attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetCsmAttr(ISP_PORT IspPort, ISP_CSM_ATTR_S *pCsmAttr);

/* @} mpi_isp_csm */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
