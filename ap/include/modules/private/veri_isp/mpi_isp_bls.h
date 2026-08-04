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

#define VSI_ISP_BLS_MIN         0        /**< \brief The minimum value of <tt>blackLevel</tt> for BLS. */
#define VSI_ISP_BLS_MAX         4095     /**< \brief The maximum value of <tt>blackLevel</tt> for BLS. */

/** \brief Contains the BLS attributes for use in manual mode. */
typedef struct vsiISP_BLS_MANUAL_ATTR_S {
    vsi_s32_t blackLevel[4]; /**< \brief The BLS values for each RAW data channel:
                                  \n - <tt>blackLevel[0]</tt> for the R channel.
                                  \n - <tt>blackLevel[1]</tt> for the Gb channel.
                                  \n - <tt>blackLevel[2]</tt> for the Gr channel.
                                  \n - <tt>blackLevel[3]</tt> for the B channel.
                                  \n Valid value range: [0, 4095] based on 12-bit raw data.
                                  \n For 10-bit raw data, the values need a left shift by 2 bits.
                                  \n Default value: 64. */
} ISP_BLS_MANUAL_ATTR_S;

/** \brief Contains the BLS attributes for use in auto mode. */
typedef struct vsiISP_BLS_AUTO_ATTR_S {
    vsi_u32_t blackLevel[ISP_AUTO_STRENGTH_NUM][4]; /**< \brief The BLS values for each RAW data channel:
                                                         \n - <tt>blackLevel[<em>n</em>][0]</tt> for the R channel.
                                                         \n - <tt>blackLevel[<em>n</em>][1]</tt> for the Gb channel.
                                                         \n - <tt>blackLevel[<em>n</em>][2]</tt> for the Gr channel.
                                                         \n - <tt>blackLevel[<em>n</em>][3]</tt> for the B channel.
                                                         \n Valid value range: [0, 4095] based on 12-bit raw data.
                                                         \n For 10-bit raw data, the values need a left shift by 2 bits.
                                                         \n Default value: 64. */
} ISP_BLS_AUTO_ATTR_S;

/** \brief Contains the BLS attributes. */
typedef struct vsiISP_BLS_ATTR_S {
    vsi_bool_t            enable;     /**< \brief Whether to enable BLS.
                                           \n Valid values:
                                           \n - 0: Disable.
                                           \n - 1: Enable. */
    vsi_u32_t             opType;     /**< \brief The operation mode of BLS.
                                           \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_BLS_MANUAL_ATTR_S manualAttr; /**< \brief The BLS attributes for use in manual mode. */
    ISP_BLS_AUTO_ATTR_S   autoAttr;   /**< \brief The BLS attributes for use in auto mode. */
} ISP_BLS_ATTR_S;

/** \brief Contains the BLS metadata that needs to be written into registers. */
typedef struct vsiISP_BLS_S {
    vsi_bool_t enable;            /**< \brief Whether to enable BLS.
                                       \n Valid values:
                                       \n - 0: Disable.
                                       \n - 1: Enable. */
    vsi_u32_t  blackLevel[4];     /**< \brief The BLS values for each RAW data channel:
                                       \n - <tt>blackLevel[0]</tt> for the R channel.
                                       \n - <tt>blackLevel[1]</tt> for the Gb channel.
                                       \n - <tt>blackLevel[2]</tt> for the Gr channel.
                                       \n - <tt>blackLevel[3]</tt> for the B channel.
                                       \n Valid value range: [0, 4095] based on 12-bit raw data.
                                       \n For 10-bit raw data, the values need a left shift by 2 bits.
                                       \n Default value: 64. */
} ISP_BLS_META_S;

/*****************************************************************************/
/**
 * @brief   Gets the BLS attributes of an ISP device port.
 *
 * \param   IspPort             The ID of the port.
 * \param   pBlsAttr            A pointer to a memory place for receiving the BLS attributes.
 *
 * \retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetBlsAttr(ISP_PORT IspPort, ISP_BLS_ATTR_S *pBlsAttr);

/*****************************************************************************/
/**
 * @brief   Sets the BLS attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pBlsAttr            A pointer to the BLS attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
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
