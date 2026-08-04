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

#ifndef __MPI_ISP_DPCC_H__
#define __MPI_ISP_DPCC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond DPCC_V10
 *
 * @defgroup mpi_isp_dpcc DPCC V10 Definitions
 * @{
 *
 */

#define VSI_ISP_DPCC_STRENGTH_MAX 5             /**< \brief The maximum value of DPCC effect. */
#define VSI_ISP_DPCC_STRENGTH_MIN 0             /**< \brief The minimum value of DPCC effect. */


/** \brief   DPCC attributes. */
typedef struct vsiISP_DPCC_ATTR_S {
    vsi_bool_t enable;                     /**< \brief Whether to enable DPCC. \n 0: Disable. \n 1: Enable. */
    vsi_u8_t   strength;                  /**< \brief A larger value indicates stronger DPCC effects. Range:[0, 5]. */
} ISP_DPCC_ATTR_S;

/** \brief DPCC metadata structure that need to be written into registers. */
typedef ISP_DPCC_ATTR_S ISP_DPCC_META_S;


/*****************************************************************************/
/**
 * @brief   Gets DPCC attributes.
 *
 * @param   IspPort             Port ID
 * @param   pDpccAttr          Pointer to the DPCC attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetDpccAttr(ISP_PORT IspPort, ISP_DPCC_ATTR_S *pDpccAttr);


/*****************************************************************************/
/**
 * @brief   Sets DPCC attributes.
 *
 * @param   IspPort             Port ID
 * @param   pDpccAttr          Pointer to the DPCC attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetDpccAttr(ISP_PORT IspPort, ISP_DPCC_ATTR_S *pDpccAttr);

/* @} mpi_isp_dpcc */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
