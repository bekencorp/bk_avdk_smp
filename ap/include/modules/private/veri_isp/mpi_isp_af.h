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

#ifndef __MPI_ISP_AF_H__
#define __MPI_ISP_AF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond AF_V10
 *
 * @defgroup mpi_isp_af AF V10 Definitions
 * @{
 *
 */

#include "vsi_comm_af.h"

/*****************************************************************************/
/**
 * @brief   Registers the AF library for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAfLib              A pointer to the AF library.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_AfRegCallBack(ISP_PORT IspPort, ISP_AF_FUNC_S *pAfLib);


/*****************************************************************************/
/**
 * @brief   Unregisters the AF library for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_AfUnRegCallBack(ISP_PORT IspPort);


/*****************************************************************************/
/**
 * @brief   Sets the focus attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pFocusAttr          A pointer to the focus attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetFocusAttr(ISP_PORT IspPort, ISP_FOCUS_ATTR_S *pFocusAttr);


/*****************************************************************************/
/**
 * @brief   Gets the focus attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pFocusAttr          A pointer to the focus attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetFocusAttr(ISP_PORT IspPort, ISP_FOCUS_ATTR_S *pFocusAttr);


/*****************************************************************************/
/**
 * @brief   Gets the focus information of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pFocusInfo          A pointer to the focus information.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_QueryFocusInfo(ISP_PORT IspPort,  ISP_FOCUS_INFO_S *pFocusInfo);


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
vsi_bool_t VSI_MPI_ISP_AfGetSnsStreamStat(ISP_PORT IspPort);

/* @} mpi_isp_af */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
