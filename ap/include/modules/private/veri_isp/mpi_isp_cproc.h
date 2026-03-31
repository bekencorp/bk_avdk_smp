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

#define VSI_ISP_CPROC_BRIGHTNESS_MAX   127        /**< \brief The maximum value of CPROC brightness. */
#define VSI_ISP_CPROC_BRIGHTNESS_MIN   -128       /**< \brief The minimum value of CPROC brightness. */
#define VSI_ISP_CPROC_CONTRAST_MAX     255        /**< \brief The maximum value of CPROC contrast. */
#define VSI_ISP_CPROC_CONTRAST_MIN     0          /**< \brief The minimum value of CPROC contrast. */
#define VSI_ISP_CPROC_SATURATION_MAX   255        /**< \brief The maximum value of CPROC saturation. */
#define VSI_ISP_CPROC_SATURATION_MIN   0          /**< \brief The minimum value of CPROC saturation. */
#define VSI_ISP_CPROC_HUE_MAX          90         /**< \brief The maximum value of CPROC hue. */
#define VSI_ISP_CPROC_HUE_MIN          -90        /**< \brief The minimum value of CPROC hue. */

/** \brief   CPROC manual attributes. */
typedef struct vsiISP_CPROC_MANUAL_ATTR_S {
    vsi_s8_t   brightness;    /**< \brief CPROC brightness adjustment value. Range: [-127, 127]*/
    vsi_u8_t   contrast;      /**< \brief CPROC contrast adjustment value. Range: [0, 255]*/
    vsi_u8_t   saturation;    /**< \brief CPROC saturation adjustment value. Range: [0, 255]*/
    vsi_s8_t   hue;           /**< \brief CPROC rotation in HSV domain. Range: [-90, 90]*/
} ISP_CPROC_MANUAL_ATTR_S;

/** \brief   CPROC auto attributes. */
typedef struct vsiISP_CPROC_AUTO_ATTR_S {
    vsi_s8_t   brightness[ISP_AUTO_STRENGTH_NUN];    /**< \brief CPROC brightness adjustment value. Range: [-127, 127]*/
    vsi_u8_t   contrast[ISP_AUTO_STRENGTH_NUN];      /**< \brief CPROC contrast adjustment value. Range: [0, 255]*/
    vsi_u8_t   saturation[ISP_AUTO_STRENGTH_NUN];    /**< \brief CPROC saturation adjustment value. Range: [0, 255]*/
    vsi_s8_t   hue[ISP_AUTO_STRENGTH_NUN];           /**< \brief CPROC rotation in HSV domain. Range: [-90, 90]*/
} ISP_CPROC_AUTO_ATTR_S;

/** \brief   CPROC attribute. */
typedef struct  vsiISP_CPROC_ATTR_S {
    vsi_bool_t enable;         /**< \brief Whether to enable CPROC. \n 0: Disable CPROC. \n 1: Enable CPROC. */
    vsi_u32_t  opType;          /**< \brief The running mode, reference ISP_OP_TYPE_E. \n 0: Automatic. \n 1: Manual. */
    ISP_CPROC_MANUAL_ATTR_S manualAttr;  /**< \brief CPROC manual attributes. */
    ISP_CPROC_AUTO_ATTR_S autoAttr;      /**< \brief CPROC auto attributes. */
} ISP_CPROC_ATTR_S;

/** \brief   CPROC metadata structure that need to be written into registers. */
typedef struct vsiISP_CPROC_S {
    vsi_bool_t enable;         /**< \brief Whether to enable CPROC. \n 0: Disable CPROC. \n 1: Enable CPROC. */
    vsi_s8_t   brightness;     /**< \brief CPROC brightness adjustment value. Range: [-127, 127]*/
    vsi_u8_t   contrast;       /**< \brief CPROC contrast adjustment value. Range: [0, 255]*/
    vsi_u8_t   saturation;     /**< \brief CPROC saturation adjustment value. Range: [0, 255]*/
    vsi_s8_t   hue;            /**< \brief CPROC rotation in HSV domain. Range: [-90, 90]*/
} ISP_CPROC_META_S;

/*****************************************************************************/
/**
 * @brief   Gets CPROC attributes.
 *
 * @param   IspPort             Port ID.
 * @param   pCprocAttr        Pointer to CPROC attributes.
 *
 * @retval  VSI_SUCCESS         Operation succeeded.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetCprocAttr(ISP_PORT IspPort, ISP_CPROC_ATTR_S *pCprocAttr);

/*****************************************************************************/
/**
 * @brief   Sets CPROC attributes.
 *
 * @param   IspPort             Port ID.
 * @param   pCprocAttr        Pointer to CPROC attributes.
 *
 * @retval  VSI_SUCCESS         Operation succeeded.
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
