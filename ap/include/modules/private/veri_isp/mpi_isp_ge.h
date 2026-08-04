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

#ifndef __MPI_ISP_GE_H__
#define __MPI_ISP_GE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond GE_V20
 *
 * @defgroup mpi_isp_ge GE V20 Definitions
 * @{
 *
 */


#define VSI_ISP_GE_THRESHOLD_MAX 65535         /**< \brief The maximum value of <tt>threshold</tt> for GE. */
#define VSI_ISP_GE_THRESHOLD_MIN 0             /**< \brief The minimum value of <tt>threshold</tt> for GE. */


/** \brief Contains the GE attributes. */
typedef struct vsiISP_GE_ATTR_S {
    vsi_bool_t            enable;      /**< \brief Whether to enable GE.
                                            \n Valid values:
                                            \n - 0: Disable.
                                            \n - 1: Enable. */
    vsi_u16_t             threshold;   /**< \brief GE threshold value.
                                            \n Valid value range: [0, 65535]. */
} ISP_GE_ATTR_S;

/** \brief GE metadata that needs to be written into registers. */
typedef ISP_GE_ATTR_S ISP_GE_META_S;


/*****************************************************************************/
/**
 * @brief   Gets the GE attributes of an ISP device port.
 *
 * \param   IspPort             The ID of the port.
 * \param   pGeAttr             A pointer to a memory place for receiving the GE attributes.
 *
 * \retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetGeAttr(ISP_PORT IspPort, ISP_GE_ATTR_S *pGeAttr);

/*****************************************************************************/
/**
 * @brief   Sets the GE attributes of an ISP device port.
 *
 * @param   IspPort                  The ID of the port.
 * @param   pGeAttr                  A pointer to the GE attributes.

 *
 * @retval  VSI_SUCCESS              The operation succeeds.
 * @retval  VSI_ERR_ILLEGAL_PARAM    Parameters are invalid.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetGeAttr(ISP_PORT IspPort, ISP_GE_ATTR_S *pGeAttr);

/* @} mpi_isp_ge */
/* @endcond */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
