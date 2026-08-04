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

#ifndef __MPI_ISP_SBI_H__
#define __MPI_ISP_SBI_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include <driver/isp_base.h>

/**
 * @cond SBI_V10
 *
 * @defgroup mpi_isp_sbi SBI V10 Definitions
 * @{
 *
 */

#define VSI_ISP_SBI_STREAM_MAX  3        /**< \brief The maximum SBI stream configuration array size pre-allocated. */

/** \brief   This enumeration specifies the FLEXA SBI exception. */
typedef enum VsiISP_SBI_EXCEPTION_ID_E {
    ISP_SBI_NO_EXCEPTION          = 0,    /**< \brief Stream running normally. */
    ISP_SBI_DISABLED              = 1,    /**< \brief Stream disabled. */
    ISP_SBI_EXCEPTION_TIME_OUT    = 2,    /**< \brief Stream Time out. */
    ISP_SBI_EXCEPTION_OUT_OF_SYNC = 3     /**< \brief Stream Out of synchronization. */
} ISP_SBI_EXCEPTION_ID_E;

/** \brief SBI stream configuration structure. */
typedef struct vsiISP_SBI_STREAM_CFG_S {
    vsi_u8_t   streamId;              /**< \brief The SBI stream ID. */
    vsi_bool_t streamEnable;          /**< \brief Indicate SBI stream enabled. */
    vsi_u16_t  entrySize;             /**< \brief Entry size value. */
    vsi_u16_t  timeOut;               /**< \brief FLEXA HW timeout configuration. */
    vsi_u32_t  exceptionID;           /**< \brief FLEXA exception ID. */
} ISP_SBI_STREAM_CFG_S;

/** \brief   SBI parameters attributes. */
typedef struct vsiISP_SBI_ATTR_S {
    vsi_u8_t   streamNum;                                       /**< \brief Valid SBI stream number, must less than VSI_ISP_SBI_STREAM_MAX. */
    vsi_u16_t  entryCnt;                                        /**< \brief Slice entry number. */
    vsi_bool_t onLine;                                          /**< \brief Online status, used for hardware start/end operations.
                                                                        \n 0: Disable SBI stream. \n 1: Enable SBI stream. */
    ISP_SBI_STREAM_CFG_S streamAttr[VSI_ISP_SBI_STREAM_MAX];    /**< \brief SBI stream configuration, reference ISP_SBI_Stream_Configure.
                                                                    \n When the ISP outputs in yuv format,
                                                                    \n the array corresponds to the Y/Cb/Cr plane. */
} ISP_SBI_ATTR_S;


/*****************************************************************************/
/**
 * @brief   Gets SBI attributes.
 *
 * \param   IspChn              ISP information, include device ID, port ID, channel ID.
 * \param   pSbiAttr            Pointer to the SBI attributes
 *
 * \retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetSbiProducer(ISP_CHN IspChn, ISP_SBI_ATTR_S *pSbiAttr);

/*****************************************************************************/
/**
 * @brief   Sets SBI attributes.
 *
 * \param   IspChn              ISP information, include device ID, port ID, channel ID.
 * \param   pSbiAttr            Pointer to the SBI attributes
 *
 * \retval  VSI_SUCCESS         Operation succeeded
 * \retval  VSI_FAILURE         Operation failure
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetSbiProducer(ISP_CHN IspChn, ISP_SBI_ATTR_S *pSbiAttr);

/* @} mpi_isp_sbi */
/* @endcond */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
