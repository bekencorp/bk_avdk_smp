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

#ifndef __MPI_ISP_AE_H__
#define __MPI_ISP_AE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond AE_V10
 *
 * @defgroup mpi_isp_ae AE V10 Definitions
 * @{
 *
 */

#include "vsi_comm_ae.h"

/*****************************************************************************/
/**
 * @brief   Registers AE.
 *
 * @param   IspPort             Port ID
 * @param   pAeLib              Pointer to the AE library
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_AeRegCallBack(ISP_PORT IspPort, ISP_AE_FUNC_S *pAeLib);


/*****************************************************************************/
/**
 * @brief   Unregisters AE.
 *
 * @param   IspPort             Port ID
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_AeUnRegCallBack(ISP_PORT IspPort);


/*****************************************************************************/
/**
 * @brief   Sets exposure attributes.
 *
 * @param   IspPort             Port ID
 * @param   pExpAttr            Pointer to the exposure attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetExposureAttr(ISP_PORT IspPort, ISP_EXPOSURE_ATTR_S *pExpAttr);


/*****************************************************************************/
/**
 * @brief   Gets exposure attributes.
 *
 * @param   IspPort             Port ID
 * @param   pExpAttr            Pointer to the exposure attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetExposureAttr(ISP_PORT IspPort, ISP_EXPOSURE_ATTR_S *pExpAttr);


/*****************************************************************************/
/**
 * @brief   (Reserved) Sets HDR exposure attributes.
 *
 * @param   IspPort             Port ID
 * @param   pHdrExpAttr         Pointer to the HDR exposure attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetHdrExposureAttr(ISP_PORT IspPort, ISP_HDR_EXPOSURE_ATTR_S *pHdrExpAttr);


/*****************************************************************************/
/**
 * @brief   (Reserved) Gets HDR exposure attributes.
 *
 * @param   IspPort             Port ID
 * @param   pHdrExpAttr         Pointer to the HDR exposure attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetHdrExposureAttr(ISP_PORT IspPort, ISP_HDR_EXPOSURE_ATTR_S *pHdrExpAttr);


/*****************************************************************************/
/**
 * @brief   Gets exposure information.
 *
 * @param   IspPort             Port ID
 * @param   pExpInfo          Pointer to the exposure information
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_QueryExposureInfo(ISP_PORT IspPort,  ISP_EXPOSURE_INFO_S *pExpInfo);


/*****************************************************************************/
/**
 * @brief   Gets sensor confiuration status.
 *
 * @param   IspPort             Port ID
 *
 * @retval  true                Busy
 * @retval  false               Not busy
 *
 *****************************************************************************/
vsi_bool_t VSI_MPI_ISP_SnsCfgIsBusy(ISP_PORT IspPort);


/*****************************************************************************/
/**
 * @brief   Gets sensor stream status.
 *
 * @param   IspPort             Port ID
 *
 * @retval  true                Stream on
 * @retval  false               Stream off
 *
 *****************************************************************************/
vsi_bool_t VSI_MPI_ISP_SnsStreamStatus(ISP_PORT IspPort);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif