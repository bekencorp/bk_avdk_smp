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

#ifndef __MPI_ISP_MI_H__
#define __MPI_ISP_MI_H__

/*****************************************************************************/
/**
 * @brief   Gets MI attributes.
 *
 * @param   IspChn              Channel ID
 * @param   pChnAttr            Pointer to the channel attributes
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetMiChnAttr(ISP_CHN IspChn, ISP_CHN_ATTR_S *pChnAttr);

/*****************************************************************************/
/**
 * @brief   Gets MI attributes.
 *
 * @param   IspChn              Channel ID
 * @param   stream              Enable/Disable stream
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetMiChnStream(ISP_CHN IspChn, vsi_u8_t stream);

/*****************************************************************************/
/**
 * @brief   Gets MI attributes.
 *
 * @param   IspChn              Channel ID
 * @param   miMis            Mi mis value
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_MiIrqProcess(ISP_DEV IspDev, vsi_u32_t miMis);

int VSI_MPI_ISP_SetRingBufferFmt(ISP_CHN IspChn , FORMAT_S *pFormat);
int VSI_MPI_ISP_SetMiV10LineEnable(ISP_CHN IspChn, vsi_u8_t enable);

#endif
