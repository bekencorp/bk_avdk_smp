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

#ifndef __VSI_ISP_DMSC_H__
#define __VSI_ISP_DMSC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond DMSC_V10
 *
 * @defgroup mpi_isp_dmsc DMSC V10 Definitions
 * @{
 *
 */

#define VSI_ISP_DMSC_THRESHOLD_MIN         0        /**< \brief The minimum value of <tt>threshold</tt> for DMSC. */
#define VSI_ISP_DMSC_THRESHOLD_MAX         255      /**< \brief The maximum value of <tt>threshold</tt> for DMSC. */
#define VSI_ISP_CAC_MIN                    -256     /**< \brief The minimum value of <tt>aBlue</tt>, <tt>aRed</tt>, <tt>bBlue</tt>, <tt>bRed</tt>, <tt>cBlue</tt>, and <tt>cRed</tt> for DMSC. */
#define VSI_ISP_CAC_MAX                    255      /**< \brief The maximum value of <tt>aBlue</tt>, <tt>aRed</tt>, <tt>bBlue</tt>, <tt>bRed</tt>, <tt>cBlue</tt>, and <tt>cRed</tt> for DMSC. */
#define VSI_ISP_CAC_X_NORMSHIFT_MIN        0        /**< \brief The minimum value of <tt>xNormShift</tt> for DMSC. */
#define VSI_ISP_CAC_X_NORMSHIFT_MAX        15       /**< \brief The maximum value of <tt>xNormShift</tt> for DMSC. */
#define VSI_ISP_CAC_X_NORMFACTOR_MIN       0        /**< \brief The minimum value of <tt>xNormFactor</tt> for DMSC. */
#define VSI_ISP_CAC_X_NORMFACTOR_MAX       31       /**< \brief The maximum value of <tt>xNormFactor</tt> for DMSC. */
#define VSI_ISP_CAC_Y_NORMSHIFT_MIN        0        /**< \brief The minimum value of <tt>yNormShift</tt> for DMSC. */
#define VSI_ISP_CAC_Y_NORMSHIFT_MAX        15       /**< \brief The maximum value of <tt>yNormShift</tt> for DMSC. */
#define VSI_ISP_CAC_Y_NORMFACTOR_MIN       0        /**< \brief The minimum value of <tt>yNormFactor</tt> for DMSC. */
#define VSI_ISP_CAC_Y_NORMFACTOR_MAX       31       /**< \brief The maximum value of <tt>yNormFactor</tt> for DMSC. */

/** \brief Contains the color aberration correction (CAC) attributes of DMSC. */
typedef struct vsiISP_CAC_ATTR_S {
    vsi_bool_t enable;   /**< \brief Whether to enable CAC.
                              \n Valid values:
                              \n - 0: Disable.
                              \n - 1: Enable. */
    vsi_u16_t hOffset;   /**< \brief The horizontal distance between the image center and optical center.
                              \n Valid value range: [0, h_size/2] where h_size refers to the output image width. */
    vsi_u16_t vOffset;   /**< \brief The vertical distance between the image center and optical center.
                              \n Valid value range: [0, v_size/2] where v_size refers to the output image height. */
    vsi_s16_t aBlue;     /**< \brief The parameter A_Blue for radial shift calculation in the blue channel.
                              \n Valid value range: [-256, 255].
                              \n Value range for the 4-bit fractional part: (-16, 15.9375). */
    vsi_s16_t aRed;      /**< \brief The parameter A_Red for radial shift calculation in the red channel.
                              \n Valid value range: [-256, 255].
                              \n Value range for the 4-bit fractional part: (-16, 15.9375). */
    vsi_s16_t bBlue;     /**< \brief The parameter B_Blue for radial shift calculation in the blue channel.
                              \n Valid value range: [-256, 255].
                              \n Value range for the 4-bit fractional part: (-16, 15.9375). */
    vsi_s16_t bRed;      /**< \brief The parameter B_Red for radial shift calculation in the red channel.
                              \n Valid value range: [-256, 255].
                              \n Value range for the 4-bit fractional part: (-16, 15.9375). */
    vsi_s16_t cBlue;     /**< \brief The parameter C_Blue for radial shift calculation in the blue channel.
                              \n Valid value range: [-256, 255].
                              \n Value range for the 4-bit fractional part: (-16, 15.9375). */
    vsi_s16_t cRed;      /**< \brief The parameter C_Red for radial shift calculation in the red channel.
                              \n Valid value range: [-256 255].
                              \n Value range for the 4-bit fractional part: (-16, 15.9375). */
    vsi_u8_t xNormShift; /**< \brief The horizontal normalization shift parameter x_ns (3-bit unsigned integer) used in the following formula:
                              \n x_d[7:0] = (((h_count << 4) >> x_ns) * x_normfactor) >> 5.
                              \n Valid value range: [0, 15]. */
    vsi_u8_t xNormFactor;/**< \brief The horizontal scaling factor x_normfactor (5-bit unsigned integer) used in the following formula:
                              \n x_d[7:0] = (((h_count << 4) >> x_ns) * x_normfactor) >> 5.
                              \n Valid value range: [0, 31]. */
    vsi_u8_t yNormShift; /**< \brief The vertical normalization shift parameter y_ns (3-bit unsigned integer) used in the following formula:
                              \n y_d[7:0] = (((v_count << 4) >> y_ns) * y_normfactor) >> 5.
                              \n Valid value range: [0, 15]. */
    vsi_u8_t yNormFactor; /**< \brief The vertical scaling factor y_normfactor (5-bit unsigned integer) used in the following formula:
				               \n y_d[7:0] = (((v_count << 4) >> y_ns) * y_normfactor) >> 5.
                               \n Valid value range: [0, 31]. */
} ISP_CAC_ATTR_S;

/** \brief Contains the DMSC attributes. */
typedef struct vsiISP_DMSC_ATTR_S {
    vsi_bool_t enable;                /**< \brief Whether to enable DMSC.
                                           \n Valid values:
                                           \n - 0: Disable.
                                           \n - 1: Enable. */
    vsi_u8_t threshold;               /**< \brief The threshold for Bayer demosaicing texture detection.
                                           \n The value that is 4-bit left shifted is compared with the difference of the vertical and horizontal 12-bit wide texture indicators, to decide whether to set the vertical or horizontal texture flag.
                                           \n Valid value range: [0, 255], where:
                                           \n - 0x00: Maximum edge sensitivity.
                                           \n - 0xFF: No texture detection. */
    ISP_CAC_ATTR_S cacAttr;           /**< \brief The CAC attributes. */
} ISP_DMSC_ATTR_S;

/** \brief DMSC metadata that needs to be written into registers. */
typedef ISP_DMSC_ATTR_S ISP_DMSC_META_S;


/*****************************************************************************/
/**
 * @brief   Gets the DMSC attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pDmscAttr           A pointer to a memory place for receiving the DMSC attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetDmscAttr(ISP_PORT IspPort, ISP_DMSC_ATTR_S *pDmscAttr);

/*****************************************************************************/
/**
 * @brief   Sets the DMSC attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pDmscAttr           A pointer to the DMSC attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetDmscAttr(ISP_PORT IspPort, ISP_DMSC_ATTR_S *pDmscAttr);

/* @} mpi_isp_dmsc */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
