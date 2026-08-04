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

#ifndef __MPI_ISP_CPROC_H__
#define __MPI_ISP_CPROC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond CPROC_V11
 *
 * @defgroup mpi_isp_cproc CPROC V11 Definitions
 * @{
 *
 */

#define VSI_ISP_CPROC_BRIGHTNESS_MAX   127        /**< \brief The maximum value of <tt>brightness</tt> for CPROC. */
#define VSI_ISP_CPROC_BRIGHTNESS_MIN   -128       /**< \brief The minimum value of <tt>brightness</tt> for CPROC. */
#define VSI_ISP_CPROC_CONTRAST_MAX     255        /**< \brief The maximum value of <tt>contrast</tt> for CPROC. */
#define VSI_ISP_CPROC_CONTRAST_MIN     0          /**< \brief The minimum value of <tt>contrast</tt> for CPROC. */
#define VSI_ISP_CPROC_SATURATION_MAX   255        /**< \brief The maximum value of <tt>saturation</tt> for CPROC. */
#define VSI_ISP_CPROC_SATURATION_MIN   0          /**< \brief The minimum value of <tt>saturation</tt> for CPROC. */
#define VSI_ISP_CPROC_HUE_MAX          127        /**< \brief The maximum value of <tt>hue</tt> for CPROC. */
#define VSI_ISP_CPROC_HUE_MIN          -128       /**< \brief The minimum value of <tt>hue</tt> for CPROC. */


/** \brief Contains the CPROC attributes. */
typedef struct  vsiISP_CPROC_ATTR_S {
    vsi_bool_t enable;                   /**< \brief Whether to enable CPROC.
                                              \n Valid values:
                                              \n - 0: Disable.
                                              \n - 1: Enable. */
    vsi_s8_t   brightness;    /**< \brief The brightness adjustment value.
                                   \n Valid value range: [-128, 127]. */
    vsi_u8_t   contrast;      /**< \brief The contrast adjustment value.
                                   \n Valid value range: [0, 255]. */
    vsi_u8_t   saturation;    /**< \brief The saturation adjustment value.
                                   \n Valid value range: [0, 255]. */
    vsi_s8_t   hue;           /**< \brief The rotation of the hue of an element.
                                   \n Valid value range: [-90, 90]. */
} ISP_CPROC_ATTR_S;

/** \brief CPROC metadata that needs to be written into registers. */
typedef ISP_CPROC_ATTR_S ISP_CPROC_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the CPROC attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pCprocAttr          A pointer to a memory place for receiving the CPROC attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetCprocAttr(ISP_PORT IspPort, ISP_CPROC_ATTR_S *pCprocAttr);

/*****************************************************************************/
/**
 * @brief   Sets the CPROC attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pCprocAttr          A pointer to the CPROC attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetCprocAttr(ISP_PORT IspPort, ISP_CPROC_ATTR_S *pCprocAttr);

/* @} mpi_isp_cproc */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
