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

#ifndef __MPI_ISP_WB_H__
#define __MPI_ISP_WB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "vsi_comm_awb.h"

/**
 * @cond WB_V10
 *
 * @defgroup mpi_isp_wb WB V10 Definitions
 * @{
 *
 */

/*****************************************************************************/
/**
 * @brief   Registers the AWB library for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAwbLib             A pointer to the AWB library.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_AwbRegCallBack(ISP_PORT IspPort, ISP_AWB_FUNC_S *pAwbLib);

/*****************************************************************************/
/**
 * @brief   Unregisters the AWB library for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_AwbUnRegCallBack(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Gets the WB attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pWbAttr             A pointer to a memory place for receiving the WB attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetWbAttr(ISP_PORT IspPort, ISP_WB_ATTR_S *pWbAttr);

/*****************************************************************************/
/**
 * @brief   Sets the WB attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pWbAttr             A pointer to the WB attributes.

 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetWbAttr(ISP_PORT IspPort, ISP_WB_ATTR_S *pWbAttr);

/*****************************************************************************/
/**
 * @brief   Gets the AWB information of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAwbInfo            A pointer to a memory place for receiving the AWB information.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_QueryAwbInfo(ISP_PORT IspPort,  ISP_AWB_INFO_S *pAwbInfo);

/* @} mpi_isp_wb */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
