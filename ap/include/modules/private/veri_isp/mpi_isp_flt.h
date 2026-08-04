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

#ifndef __VSI_ISP_FLT_H__
#define __VSI_ISP_FLT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond FLT_V10
 *
 * @defgroup mpi_isp_flt FLT V10 Definitions
 * @{
 *
 */

#define VSI_ISP_FLT_LEVEL_MIN    0    /**< \brief The minimum value of each level parameter for FLT. */
#define VSI_ISP_FLT_LEVEL_MAX    10   /**< \brief The maximum value of each level parameter for FLT. */



/** \brief Contains the FLT attributes for use in manual mode. */
typedef struct vsiISP_FLT_MANUAL_ATTR_S {
    vsi_u8_t denoiseLevel;    /**< \brief The denosing level.
                                   \n Valid value range: [0, 10]. */
    vsi_u8_t sharpenLevel;    /**< \brief The sharpening level.
                                   \n Valid value range: [0, 10]. */
} ISP_FLT_MANUAL_ATTR_S;

/** \brief Contains the FLT attributes for use in auto mode. */
typedef struct vsiISP_FLT_AUTO_ATTR_S {
    vsi_u8_t denoiseLevel[ISP_AUTO_STRENGTH_NUM];    /**< \brief The denosing level.
                                                          \n Valid value range: [0, 10]. */
    vsi_u8_t sharpenLevel[ISP_AUTO_STRENGTH_NUM];    /**< \brief The sharpening level.
                                                          \n Valid value range: [0, 10]. */
} ISP_FLT_AUTO_ATTR_S;

/** \brief Contains the FLT attributes. */
typedef struct vsiISP_FLT_ATTR_S {
    vsi_bool_t            enable;         /**< \brief Whether to enable FLT.
                                               \n Valid values:
                                               \n - 0: Disable.
                                               \n - 1: Enable. */
    vsi_u32_t             opType;         /**< \brief The operation mode of FLT.
                                               \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt> */
    ISP_FLT_MANUAL_ATTR_S manualAttr;     /**< \brief The FLT attributes for use in manual mode. */
    ISP_FLT_AUTO_ATTR_S   autoAttr;       /**< \brief The FLT attributes for use in auto mode. */
} ISP_FLT_ATTR_S;

/** \brief Contains the FLT metadata that needs to be written into registers. */
typedef struct vsiISP_FLT_S {
    vsi_bool_t enable;     /**< \brief Whether to enable FLT.
                                \n Valid values:
                                \n - 0: Disable.
                                \n - 1: Enable. */
    vsi_u8_t denoiseLevel; /**< \brief The denosing level.
                                \n Valid value range: [0, 10]. */
    vsi_u8_t sharpenLevel; /**< \brief The sharpening level.
                                \n Valid value range: [0, 10]. */
} ISP_FLT_META_S;


/*****************************************************************************/
/**
 * @brief   Gets the FLT attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pFltAttr            A pointer to a memory place for receiving the FLT attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetFltAttr(ISP_PORT IspPort, ISP_FLT_ATTR_S *pFltAttr);

/*****************************************************************************/
/**
 * @brief   Sets the FLT attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pFltAttr            A pointer to the FLT attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetFltAttr(ISP_PORT IspPort, ISP_FLT_ATTR_S *pFltAttr);

/* @} mpi_isp_flt */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
