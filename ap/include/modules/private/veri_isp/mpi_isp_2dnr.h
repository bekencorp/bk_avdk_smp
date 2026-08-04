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

#ifndef __MPI_ISP_2DNR_H__
#define __MPI_ISP_2DNR_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond DENOISE2D_V30
 *
 * @defgroup mpi_isp_2dnr 2DNR V30 Definitions
 * @{
 *
 */

#define ISP_2DNR_SIGMA_BIN     60  /**< \brief 2DNR sigma Y LUT size. */

#define VSI_ISP_2DNR_STRENGTH_MAX             127        /**< \brief The maximum value of 2DNR strength. */
#define VSI_ISP_2DNR_STRENGTH_MIN             0          /**< \brief The minimum value of 2DNR strength. */
#define VSI_ISP_2DNR_PREGAMMASTRENGTH_MAX     127        /**< \brief The maximum value of 2DNR pregamma strength. */
#define VSI_ISP_2DNR_PREGAMMASTRENGTH_MIN     0          /**< \brief The minimum value of 2DNR pregamma strength. */
#define VSI_ISP_2DNR_SIGMAY_MAX               4095       /**< \brief The maximum value of 2DNR sigma Y. */
#define VSI_ISP_2DNR_SIGMAY_MIN               0          /**< \brief The minimum value of 2DNR sigma Y. */

/** \brief   2DNR manual attributes. */
typedef struct vsiISP_2DNR_MANUAL_ATTR_S {
    vsi_u8_t   strength;                     /**< \brief 2DNR strength. Range: [0, 127] */
    vsi_u8_t   pregammaStrength;             /**< \brief Whether to enable 2DNR pregamma transform. \n 0: Disable. \n 1-127: Enable. */
    vsi_u16_t  sigmaY[ISP_2DNR_SIGMA_BIN]; /**< \brief Represent the spatial weight. Range: [0, 4095] */
} ISP_2DNR_MANUAL_ATTR_S;

/** \brief   2DNR auto attributes. */
typedef struct vsiISP_2DNR_AUTO_ATTR_S {
    vsi_u8_t   strength[ISP_AUTO_STRENGTH_NUN];           /**< \brief 2DNR strength. Range: [0, 127] */
    vsi_u8_t   pregammaStrength[ISP_AUTO_STRENGTH_NUN];   /**< \brief Whether to enable 2DNR pregamma transform. \n 0: Disable. \n 1-127: Enable. */
    vsi_u16_t  sigmaY[ISP_AUTO_STRENGTH_NUN][ISP_2DNR_SIGMA_BIN]; /**< \brief Represent the spatial weight. Range: [0, 4095] */
} ISP_2DNR_AUTO_ATTR_S;

typedef struct vsiISP_2DNR_ATTR_S {
    vsi_bool_t              enable;           /**< \brief Whether to enable 2DNR. \n 0: Disable. \n 1: Enable. */
    vsi_u32_t               opType;           /**< \brief 2DNR mode attributes, reference ISP_OP_TYPE_E */
    ISP_2DNR_MANUAL_ATTR_S  manualAttr;       /**< \brief 2DNR manual attributes */
    ISP_2DNR_AUTO_ATTR_S    autoAttr;         /**< \brief 2DNR auto attributes */
} ISP_2DNR_ATTR_S;

/** \brief   2DNR metadata that need to be written into registers. */
typedef struct vsiISP_2DNR_S {
    vsi_bool_t enable;                       /**< \brief Whether to enable 2DNR. \n 0: Disable. \n 1: Enable. */
    vsi_u8_t   strength;                     /**< \brief 2DNR strength. Range: [0, 127] */
    vsi_u16_t  sigmaY[ISP_2DNR_SIGMA_BIN];   /**< \brief Represent the spatial weight. Range: [0, 4095] */
    vsi_u8_t   pregammaStrength;             /**< \brief Whether to enable 2DNR pregamma transform. \n 0: Disable. \n 1-127: Enable. */
} ISP_2DNR_META_S;

/*****************************************************************************/
/**
 * @brief   Gets 2DNR attributes.
 *
 * @param   IspPort             Port ID
 * @param   pst2DnrAttr          Pointer to the 2DNR attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_Get2DnrAttr(ISP_PORT IspPort, ISP_2DNR_ATTR_S *pst2DnrAttr);


/*****************************************************************************/
/**
 * @brief   Sets 2DNR attributes.
 *
 * @param   IspPort             Port ID
 * @param   pst2DnrAttr          Pointer to the 2DNR attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_Set2DnrAttr(ISP_PORT IspPort, ISP_2DNR_ATTR_S *pst2DnrAttr);

/* @} mpi_isp_2dnr */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
