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

#ifndef __MPI_ISP_LSC_H__
#define __MPI_ISP_LSC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond LSC_V10
 *
 * @defgroup mpi_isp_lsc LSC V10 Definitions
 * @{
 *
 */

#define LSC_SEC_CNT 16          /**< \brief The number of LSC sections. */
#define LSC_SEC_POINT_CNT 17    /**< \brief The number of LSC section points. */

#define LSC_DATA_TAB_SIZE 289   /**< \brief The size of the LSC data table. */
#define LSC_SEC_TBL_SIZE 8      /**< \brief The size of LSC xSize and ySize. */

#define VSI_ISP_LSC_MATRIX_MIN    1024    /**< \brief The minimum value of each matrix parameter for LSC. */
#define VSI_ISP_LSC_MATRIX_MAX    4095    /**< \brief The maximum value of each matrix parameter for LSC. */
#define VSI_ISP_LSC_X_SIZE_MIN    10      /**< \brief The minimum value of <tt>xSize</tt> for LSC. */
#define VSI_ISP_LSC_Y_SIZE_MIN    8       /**< \brief The minimum value of <tt>ySize</tt> for LSC. */


/** \brief Contains the LSC attributes for use in manual mode. */
typedef struct vsiISP_LSC_MANUAL_ATTR_S {
    vsi_u16_t rData[LSC_DATA_TAB_SIZE];     /**< \brief The R gains.
                                                 \n Valid value range: [1024, 4095]. */
    vsi_u16_t grData[LSC_DATA_TAB_SIZE];    /**< \brief The Gr gains.
                                                 \n Valid value range: [1024, 4095]. */
    vsi_u16_t gbData[LSC_DATA_TAB_SIZE];    /**< \brief The Gb gains.
                                                 \n Valid value range: [1024, 4095]. */
    vsi_u16_t bData[LSC_DATA_TAB_SIZE];     /**< \brief The B gains.
                                                 \n Valid value range: [1024, 4095]. */
} ISP_LSC_MANUAL_ATTR_S;

/** \brief Contains the LSC attributes of an illuminant. */
typedef struct vsiISP_ILLUMINANT_LSC_S {
    vsi_u32_t colorTemp;                   /**< \brief The color temperature.
                                                \n Valid value range: [2000, 10000]. */
    vsi_u16_t rData[LSC_DATA_TAB_SIZE];    /**< \brief The R gains.
                                                \n Valid value range: [1024, 4095]. */
    vsi_u16_t grData[LSC_DATA_TAB_SIZE];   /**< \brief The Gr gains.
                                                \n Valid value range: [1024, 4095]. */
    vsi_u16_t gbData[LSC_DATA_TAB_SIZE];   /**< \brief The Gb gains.
                                                \n Valid value range: [1024, 4095]. */
    vsi_u16_t bData[LSC_DATA_TAB_SIZE];    /**< \brief The Blue gains.
                                                \n Valid value range: [1024, 4095]. */
} ISP_ILLUMINANT_LSC_S;

/** \brief Contains the LSC attributes for use in auto mode. */
typedef struct vsiISP_LSC_AUTO_ATTR_S {
    ISP_ILLUMINANT_LSC_S illuminantLSC[ILLUMINANT_TYPE_CNT]; /**< \brief The LSC attributes of each illuminant. */
} ISP_LSC_AUTO_ATTR_S;

/** \brief Contains the LSC attributes. */
typedef struct vsiISP_LSC_ATTR_S {
    vsi_bool_t enable;                     /**< \brief Whether to enable LSC.
                                                \n Valid values:
                                                \n - 0: Disable.
                                                \n - 1: Enable. */
    vsi_u32_t  opType;                     /**< \brief The operation mode of LSC.
                                                \n Valid values: See <tt> \ref ISP_OP_TYPE_E</tt>. */
    vsi_u16_t xSize[LSC_SEC_TBL_SIZE];     /**< \brief The LSC int array for x axis size.
                                                \n Valid value range: [1, ispCoreSize.width/2]. */
    vsi_u16_t ySize[LSC_SEC_TBL_SIZE];     /**< \brief The LSC int array for y axis size.
                                                \n Valid value range: [1, ispCoreSize.height/2]. */
    ISP_LSC_MANUAL_ATTR_S manualAttr;      /**< \brief The LSC attributes for use in manual mode. */
    ISP_LSC_AUTO_ATTR_S autoAttr;          /**< \brief The LSC attributes for use in auto mode. */
} ISP_LSC_ATTR_S;

/** \brief Contains the LSC metadata that needs to be written into registers. */
typedef struct vsiISP_LSC_S {
    vsi_bool_t enable;                   /**< \brief Whether to enable LSC.
                                              \n Valid values:
                                              \n - 0: Disable.
                                              \n - 1: Enable. */
    vsi_u16_t xSize[LSC_SEC_TBL_SIZE];   /**< \brief The LSC int array for x axis size.
                                              \n Valid value range: [1, ispCoreSize.width/2]. */
    vsi_u16_t ySize[LSC_SEC_TBL_SIZE];   /**< \brief The LSC int array for y axis size.
                                              \n Valid value range: [1, ispCoreSize.height/2]. */
    vsi_u16_t xGrad[LSC_SEC_TBL_SIZE];   /**< \brief The gradient table X sectors. */
    vsi_u16_t yGrad[LSC_SEC_TBL_SIZE];   /**< \brief The gradient table Y sectors. */
    vsi_u16_t rData[LSC_DATA_TAB_SIZE];  /**< \brief The R gains. */
    vsi_u16_t grData[LSC_DATA_TAB_SIZE]; /**< \brief The Gr gains. */
    vsi_u16_t gbData[LSC_DATA_TAB_SIZE]; /**< \brief The Gb gains. */
    vsi_u16_t bData[LSC_DATA_TAB_SIZE];  /**< \brief The B gains. */
} ISP_LSC_META_S;


/*****************************************************************************/
/**
 * @brief   Gets the LSC attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pLscAttr            A pointer to a memory place for receiving the LSC attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetLscAttr(ISP_PORT IspPort, ISP_LSC_ATTR_S *pLscAttr);

/*****************************************************************************/
/**
 * @brief   Sets the LSC attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pLscAttr            A pointer to the LSC attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetLscAttr(ISP_PORT IspPort, ISP_LSC_ATTR_S *pLscAttr);

/* @} mpi_isp_lsc */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
