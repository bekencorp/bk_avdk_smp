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

#ifndef __MPI_ISP_WDR_H__
#define __MPI_ISP_WDR_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond WDR_V30
 *
 * @defgroup mpi_isp_wdr WDR V30 Definitions
 * @{
 *
 */

#define VSI_ISP_WDR_STRENGTH_MIN 0           /**< \brief The minimum value of <tt>strength</tt> for WDR. */
#define VSI_ISP_WDR_STRENGTH_MAX 128         /**< \brief The maximum value of <tt>strength</tt> for WDR. */
#define VSI_ISP_WDR_GLOBAL_STRENGTH_MIN 0    /**< \brief The minimum value of <tt>globalStrength</tt> for WDR. */
#define VSI_ISP_WDR_GLOBAL_STRENGTH_MAX 128  /**< \brief The maximum value of <tt>globalStrength</tt> for WDR. */
#define VSI_ISP_WDR_MAX_GAIN_MIN 1           /**< \brief The minimum value of <tt>maxGain</tt> for WDR. */
#define VSI_ISP_WDR_MAX_GAIN_MAX 255         /**< \brief The maximum value of <tt>maxGain</tt> for WDR. */

/** \brief Contains the WDR attributes for use in manual mode. */
typedef struct vsiISP_WDR_MANUAL_ATTR_S {
    vsi_u8_t strength;        /**< \brief The WDR strength.
                                   \n A greater value indicates stronger WDR effects.
                                   \n Valid value range: [0, 128]. */
    vsi_u8_t globalStrength;  /**< \brief The global WDR strength.
                                   \n Valid value range: [0, 128]. */
    vsi_u8_t maxGain;         /**< \brief The maximum WDR gain.
                                   \n Valid value range: [1, 255]. */
} ISP_WDR_MANUAL_ATTR_S;

/** \brief Contains the WDR attributes for use in auto mode. */
typedef struct vsiISP_WDR_AUTO_ATTR_S {
    vsi_u8_t strength[ISP_AUTO_STRENGTH_NUM];        /**< \brief The WDR strength.
                                                          \n Valid value range: [0, 128]. */
    vsi_u8_t globalStrength[ISP_AUTO_STRENGTH_NUM];  /**< \brief The global WDR strength.
                                                          \n Valid value range: [0, 128]. */
    vsi_u8_t maxGain[ISP_AUTO_STRENGTH_NUM];         /**< \brief The maximum WDR gain.
                                                          \n Valid value range: [1, 255]. */
} ISP_WDR_AUTO_ATTR_S;

/** \brief Contains the WDR attributes. */
typedef struct vsiISP_WDR_ATTR_S {
    vsi_bool_t           enable;       /**< \brief Whether to enable WDR.
                                            \n Valid values:
                                            \n - 0: Disable.
                                            \n - 1: Enable. */
    vsi_u32_t            opType;       /**< \brief The operation mode of WDR.
                                            \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    ISP_WDR_MANUAL_ATTR_S manualAttr;  /**< \brief The WDR attributes for use in manual mode. */
    ISP_WDR_AUTO_ATTR_S  autoAttr;     /**< \brief The WDR attributes for use in auto mode. */
} ISP_WDR_ATTR_S;


/** \brief Contains the WDR metadata that needs to be written into registers. */
typedef struct vsiISP_WDR_S {
    vsi_bool_t enable;       /**< \brief Whether to enable WDR.
                                  \n Valid values:
                                  \n - 0: Disable.
                                  \n - 1: Enable. */
    vsi_u8_t strength;       /**< \brief The WDR strength.
                                  \n A greater value indicates stronger WDR effects.
                                  \n Valid value range: [0, 128]. */
    vsi_u8_t globalStrength; /**< \brief The global WDR strength.
                                  \n Valid value range: [0, 128]. */
    vsi_u8_t maxGain;        /**< \brief The maximum WDR gain.
                                  \n Valid value range: [1, 255]. */
} ISP_WDR_META_S;


/*****************************************************************************/
/**
 * @brief   Gets the WDR attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pWdrAttr            A pointer to a memory place for receiving the WDR attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetWdrAttr(ISP_PORT IspPort, ISP_WDR_ATTR_S *pWdrAttr);


/*****************************************************************************/
/**
 * @brief   Sets the WDR attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pWdrAttr            A pointer to the WDR attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetWdrAttr(ISP_PORT IspPort, ISP_WDR_ATTR_S *pWdrAttr);

/* @} mpi_isp_wdr */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
