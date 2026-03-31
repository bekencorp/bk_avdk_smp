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

#define LSC_SEC_CNT 16          /**< \brief LSC section count */
#define LSC_SEC_POINT_CNT 17    /**< \brief LSC section point count */

#define LSC_DATA_TAB_SIZE 289   /**< \brief LSC data table size */
#define LSC_SEC_TBL_SIZE 8      /**< \brief LSC xSize and ySize size */

#define VSI_ISP_LSC_MATRIX_MIN    1024    /**< \brief The minimum value of LSC matrix. */
#define VSI_ISP_LSC_MATRIX_MAX    4095    /**< \brief The maximum value of LSC matrix. */
#define VSI_ISP_LSC_X_SIZE_MIN    1       /**< \brief The minimum value of LSC x size. */
#define VSI_ISP_LSC_Y_SIZE_MIN    1       /**< \brief The minimum value of LSC y size. */


/** \brief   Manual LSC attributes. */
typedef struct vsiISP_LSC_MANUAL_ATTR_S {
    vsi_u16_t rData[LSC_DATA_TAB_SIZE];     /**< \brief LSC red gains. Range: [1024, 4095]. */
    vsi_u16_t grData[LSC_DATA_TAB_SIZE];    /**< \brief LSC green (red) gains. Range: [1024, 4095]. */
    vsi_u16_t gbData[LSC_DATA_TAB_SIZE];    /**< \brief LSC green (blue) gains. Range: [1024, 4095]. */
    vsi_u16_t bData[LSC_DATA_TAB_SIZE];     /**< \brief LSC blue gains. Range: [1024, 4095]. */
} ISP_LSC_MANUAL_ATTR_S;

/** \brief   LSC illuminant attributes. */
typedef struct vsiISP_ILLUMINANT_LSC_S {
    vsi_u32_t colorTemp;   /**< \brief Color temperature value.
                                \n range [2000, 10000]*/
    vsi_u16_t rData[LSC_DATA_TAB_SIZE];    /**< \brief LSC red gains. Range: [1024, 4095]. */
    vsi_u16_t grData[LSC_DATA_TAB_SIZE];   /**< \brief LSC gr gains. Range: [1024, 4095]. */
    vsi_u16_t gbData[LSC_DATA_TAB_SIZE];   /**< \brief LSC gb gains. Range: [1024, 4095]. */
    vsi_u16_t bData[LSC_DATA_TAB_SIZE];    /**< \brief LSC blue gains. Range: [1024, 4095]. */
} ISP_ILLUMINANT_LSC_S;

/** \brief   Auto LSC attributes. */
typedef struct vsiISP_LSC_AUTO_ATTR_S {
    ISP_ILLUMINANT_LSC_S illuminantLSC[ILLUMINANT_TYPE_CNT]; /**< \brief LSC illuminant attributes */
} ISP_LSC_AUTO_ATTR_S;

/** \brief   LSC attributes. */
typedef struct vsiISP_LSC_ATTR_S {
    vsi_bool_t enable;                     /**< \brief Whether to enable LSC. \n 0: Disable. \n 1: Enable. */
    vsi_u32_t  opType;                 /**< \brief LSC configurations, reference ISP_OP_TYPE_E */
    vsi_u16_t xSize[LSC_SEC_TBL_SIZE];  /**< \brief LSC int array for x axis size.
                                             \n Range: [1 ispCoreSize.width/2] */
    vsi_u16_t ySize[LSC_SEC_TBL_SIZE];  /**< \brief LSC int array for y axis size.
                                             \n Range: [1 ispCoreSize.height/2] */
    ISP_LSC_MANUAL_ATTR_S manualAttr;         /**< \brief LSC manual configuration */
    ISP_LSC_AUTO_ATTR_S autoAttr;             /**< \brief LSC auto configurations */
} ISP_LSC_ATTR_S;

/** \brief   LSC metadata structure that need to be written into registers. */
typedef struct vsiISP_LSC_S {
    vsi_bool_t enable;                   /**< \brief Whether to enable LSC. \n 0: Disable. \n 1: Enable. */
    vsi_u16_t xSize[LSC_SEC_TBL_SIZE];   /**< \brief LSC int array for x axis size. */
    vsi_u16_t ySize[LSC_SEC_TBL_SIZE];   /**< \brief LSC int array for y axis size. */
    vsi_u16_t xGrad[LSC_SEC_TBL_SIZE];   /**< \brief LSC gradient table X Sectors. */
    vsi_u16_t yGrad[LSC_SEC_TBL_SIZE];   /**< \brief LSC gradient table Y Sectors. */
    vsi_u16_t rData[LSC_DATA_TAB_SIZE];  /**< \brief LSC red gains. */
    vsi_u16_t grData[LSC_DATA_TAB_SIZE]; /**< \brief LSC gb gains. */
    vsi_u16_t gbData[LSC_DATA_TAB_SIZE]; /**< \brief LSC gb gains. */
    vsi_u16_t bData[LSC_DATA_TAB_SIZE];  /**< \brief LSC blue gains. */
} ISP_LSC_META_S;


/*****************************************************************************/
/**
 * @brief   Gets LSC attributes.
 *
 * @param   IspPort             Port ID
 * @param   pLscAttr            Pointer to the LSC attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetLscAttr(ISP_PORT IspPort, ISP_LSC_ATTR_S *pLscAttr);

/*****************************************************************************/
/**
 * @brief   Sets LSC attributes.
 *
 * @param   IspPort             Port ID
 * @param   pLscAttr            Pointer to the LSC attributes

 *
 * @retval  VSI_SUCCESS         Operation succeeded
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
