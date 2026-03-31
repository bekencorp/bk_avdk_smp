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

#ifndef __MPI_ISP_DG_H__
#define __MPI_ISP_DG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond DG_V11
 *
 * @defgroup mpi_isp_dg DG V11 Definitions
 * @{
 *
 */

#define DG_SIZE 4              /**< \brief The size of DG. */

#define VSI_ISP_DG_MIN 256     /**< \brief The minimum value of digital gain. */
#define VSI_ISP_DG_MAX 65535   /**< \brief The maximum value of digital gain. */

/** \brief   DG attributes. */
typedef struct vsiISP_DG_ATTR_S {
    vsi_bool_t           enable;           /**< \brief Whether to enable DG. \n 0: Disable. \n 1: Enable. */
    vsi_u16_t            gains[DG_SIZE];   /**< \brief DG configurations. \n gains[0]: R. \n gains[1]: Gb.
                                                                          \n gains[2]: Gr. \n gains[3]: B.
                                                                          \n Range: [256 65535].
                                                                          \n 8-bit fractional part[1 255.99].
                                                                          \n (256 / 256) = 1.
                                                                          \n  (65535 / 256) = 255.99. */
} ISP_DG_ATTR_S;

/** \brief DG metadata structure that need to be written into registers. */
typedef ISP_DG_ATTR_S ISP_DG_META_S;


/*****************************************************************************/
/**
 * @brief   Gets DG attributes.
 *
 * @param   IspPort             Port ID
 * @param   pDgAttr           Pointer to the DG attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetDgAttr(ISP_PORT IspPort, ISP_DG_ATTR_S *pDgAttr);


/*****************************************************************************/
/**
 * @brief   Sets DG attributes.
 *
 * @param   IspPort             Port ID
 * @param   pDgAttr           Pointer to the DG attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetDgAttr(ISP_PORT IspPort, ISP_DG_ATTR_S *pDgAttr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
