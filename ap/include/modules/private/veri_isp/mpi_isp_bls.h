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

#ifndef __MPI_ISP_BLS_H__
#define __MPI_ISP_BLS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond BLS_V10
 *
 * @defgroup mpi_isp_bls BLS V10 Definitions
 * @{
 *
 */

#define VSI_ISP_BLS_MIN         0        /**< \brief The minimum value of BLS. */
#define VSI_ISP_BLS_MAX         4095     /**< \brief The maximum value of BLS. */

/** \brief BLS manual structure. */
typedef struct vsiISP_BLS_MANUAL_ATTR_S {
    vsi_s32_t blackLevel[4]; /**< \brief The RAW channel. The value range is [0, 4095] and the default value is 64.
    \n Based on 12-bit RAW, if it is 10-bit RAW, the black level needs a left shift by 2 bits.
                                                            \n BLS[0]: R. \n BLS[1]: Gb.
                                                            \n BLS[2]: Gr. \n BLS[3]: B. */

} ISP_BLS_MANUAL_ATTR_S;

/** \brief BLS auto structure. */
typedef struct vsiISP_BLS_AUTO_ATTR_S {
    vsi_u32_t blackLevel[ISP_AUTO_STRENGTH_NUN][4]; /**< \brief The RAW channel. The value range is [0, 4095] and the default value is 64.
     \n Based on 12-bit RAW, if it is 10-bit RAW, the black level needs a left shift by 2 bits.
                                                            \n BLS[0]: R. \n BLS[1]: Gb.
                                                            \n BLS[2]: Gr. \n BLS[3]: B. */
} ISP_BLS_AUTO_ATTR_S;

/** \brief   BLS attributes. */
typedef struct vsiISP_BLS_ATTR_S {
    vsi_bool_t            enable;  /**< \brief Whether to enable BLS. \n 0: Disable BLS. \n 1: Enable BLS. */
    vsi_u32_t             opType; /**< \brief BLS mode attributes, reference ISP_OP_TYPE_E */
    ISP_BLS_MANUAL_ATTR_S manualAttr; /**< \brief BLS manual attributes */
    ISP_BLS_AUTO_ATTR_S   autoAttr;   /**< \brief BLS auto attributes */
} ISP_BLS_ATTR_S;

/** \brief   BLS metadata structure that need to be written into registers. */
typedef struct vsiISP_BLS_S {
    vsi_bool_t enable;            /**< \brief Whether to enable BLS. \n 0: Disable BLS. \n 1: Enable BLS. */
    vsi_u32_t  blackLevel[4];     /**< \brief The RAW channel values. */
} ISP_BLS_META_S;

/*****************************************************************************/
/**
 * @brief   Gets BLS attributes.
 *
 * \param   IspPort             Port ID
 * \param   pBlsAttr            Pointer to the BLS attributes
 *
 * \retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetBlsAttr(ISP_PORT IspPort, ISP_BLS_ATTR_S *pBlsAttr);

/*****************************************************************************/
/**
 * @brief   Sets BLS attributes.
 *
 * @param   IspPort             Port ID
 * @param   pBlsAttr            Pointer to the BLS attributes

 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetBlsAttr(ISP_PORT IspPort, ISP_BLS_ATTR_S *pBlsAttr);

/* @} mpi_isp_bls */
/* @endcond */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
