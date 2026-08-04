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

#ifndef __MPI_ISP_RDMA_H__
#define __MPI_ISP_RDMA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond RDMA_V10
 *
 * @defgroup mpi_isp_rdma RDMA V10 Definitions
 * @{
 *
 */

/*****************************************************************************/
/**
 * @brief   Sets the RDMA attributes of an ISP device port.
 *
 * \param   IspChn              The ID of the port.
 * \param   pRdmaAttr           A pointer to the RDMA attributes.
 *
 * \retval  VSI_SUCCESS         The operation succeeds.
 * \retval  VSI_FAILURE         The operation fails.
 *
 *****************************************************************************/

int VSI_MPI_ISP_SetRdmaAttr(ISP_CHN IspChn, FORMAT_S *pRdmaAttr);

/*****************************************************************************/
/**
 * @brief   Gets the RDMA attributes of an ISP device port.
 *
 * \param   IspChn              The ID of the port.
 * \param   pRdmaAttr           A pointer to a memory place for receiving the RDMA attributes.
 *
 * \retval  VSI_SUCCESS         The operation succeeds.
 * \retval  VSI_FAILURE         The operation fails.
 *
 *****************************************************************************/

int VSI_MPI_ISP_GetRdmaAttr(ISP_CHN IspChn, FORMAT_S *pRdmaAttr);

/*****************************************************************************/
/**
 * @brief   Enables the RDMA attributes of an ISP device port.
 *
 * \param   IspChn              The ID of the port.
 * \param   enable              Whether to enable the RDMA attributes of the port.
 *
 * \retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnableRdmaAttr(ISP_CHN IspChn, vsi_bool_t enable);

/* @} mpi_isp_rdma */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
