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
 * @brief   Registers the AE library for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAeLib              A pointer to the AE library.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_AeRegCallBack(ISP_PORT IspPort, ISP_AE_FUNC_S *pAeLib);


/*****************************************************************************/
/**
 * @brief   Unregisters the AE library for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_AeUnRegCallBack(ISP_PORT IspPort);


/*****************************************************************************/
/**
 * @brief   Sets the exposure attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pExpAttr            A pointer to the exposure attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetExposureAttr(ISP_PORT IspPort, ISP_EXPOSURE_ATTR_S *pExpAttr);


/*****************************************************************************/
/**
 * @brief   Gets the exposure attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pExpAttr            A pointer to a memory place for receiving the exposure attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetExposureAttr(ISP_PORT IspPort, ISP_EXPOSURE_ATTR_S *pExpAttr);


/*****************************************************************************/
/**
 * @brief   (Reserved) Sets the HDR exposure attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pHdrExpAttr         A pointer to the HDR exposure attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetHdrExposureAttr(ISP_PORT IspPort, ISP_HDR_EXPOSURE_ATTR_S *pHdrExpAttr);


/*****************************************************************************/
/**
 * @brief   (Reserved) Gets the HDR exposure attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pHdrExpAttr         A pointer to a memory place for receiving the HDR exposure attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetHdrExposureAttr(ISP_PORT IspPort, ISP_HDR_EXPOSURE_ATTR_S *pHdrExpAttr);


/*****************************************************************************/
/**
 * @brief   Gets the exposure information of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pExpInfo            A pointer to a memory place for receiving the exposure information.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_QueryExposureInfo(ISP_PORT IspPort,  ISP_EXPOSURE_INFO_S *pExpInfo);


/*****************************************************************************/
/**
 * @brief   Gets the sensor confiuration status of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  true                Busy
 * @retval  false               Not busy
 *
 *****************************************************************************/
vsi_bool_t VSI_MPI_ISP_SnsCfgIsBusy(ISP_PORT IspPort);


/*****************************************************************************/
/**
 * @brief   Gets the streaming status of the sensor of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  true                Streaming on
 * @retval  false               Streaming off
 *
 *****************************************************************************/
vsi_bool_t VSI_MPI_ISP_SnsStreamStatus(ISP_PORT IspPort);

/* @} mpi_isp_ae */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
