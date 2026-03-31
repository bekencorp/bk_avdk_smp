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
 * @brief   Register AWB.
 *
 * @param   IspPort             Port ID
 * @param   pAwbLib             Pointer to the AWB library
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_AwbRegCallBack(ISP_PORT IspPort, ISP_AWB_FUNC_S *pAwbLib);

/*****************************************************************************/
/**
 * @brief   Unregister AWB.
 *
 * @param   IspPort             Port ID
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_AwbUnRegCallBack(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Gets WB attributes.
 *
 * @param   IspPort             Port ID
 * @param   pWbAttr             Pointer to the WB attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetWbAttr(ISP_PORT IspPort, ISP_WB_ATTR_S *pWbAttr);

/*****************************************************************************/
/**
 * @brief   Sets WB attribute.
 *
 * @param   IspPort             Port ID
 * @param   pWbAttr           Pointer to the WB attribute

 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetWbAttr(ISP_PORT IspPort, ISP_WB_ATTR_S *pWbAttr);

/*****************************************************************************/
/**
 * @brief   Gets AWB information.
 *
 * @param   IspPort             Port ID
 * @param   pAwbInfo            Pointer to the AWB information
 *
 * @retval  VSI_SUCCESS         Operation succeeded
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
