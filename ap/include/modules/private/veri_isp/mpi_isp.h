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

#ifndef __MPI_ISP_H__
#define __MPI_ISP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "vsi_comm_metadata.h"

/**
 * @defgroup mpi_isp Mpp ISP Definitions
 * @{
 *
 *
 */

/*****************************************************************************/
/**
 * @brief   Creates and initializes an instance, and initializes the pipeline.
 *
 * @param   IspDev              The ISP device ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_Init(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Destroys the instance.
 *
 * @param   IspDev              The ISP device ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_Exit(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Register the sensor callback function.
 *
 * @param   IspPort             The port ID.
 * @param   pSnsObj             The pointer to the sensor configuration.
 * @param   snsDev              The pointer to the sensor device.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsRegCallBack(ISP_PORT IspPort, ISP_SNS_OBJ_S *pSnsObj, vsi_u8_t snsDev);

/*****************************************************************************/
/**
 * @brief   Unregisters the sensor callback function.
 *
 * @param   IspPort              The port ID.
 *
 * @retval  VSI_SUCCESS          The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsUnRegCallBack(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Sets the ISP device working mode.
 *
 * @param   IspDev              The ISP device ID.
 * @param   pDevAttr            The pointer to the ISP device configuration.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetDevAttr(ISP_DEV IspDev, ISP_DEV_ATTR_S *pDevAttr);

/*****************************************************************************/
/**
 * @brief   Gets the ISP device working mode.
 *
 * @param   IspDev              The ISP device ID.
 * @param   pDevAttr            The pointer to the ISP device configuration.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetDevAttr(ISP_DEV IspDev, ISP_DEV_ATTR_S *pDevAttr);

/*****************************************************************************/
/**
 * @brief   Enables the ISP device.
 *
 * @param   IspDev              The ISP device ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnableDev(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Disables the ISP device.
 *
 * @param   IspDev              The ISP device ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DisableDev(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Sets the ISP port input(resolution, crop)
 *
 * @param   IspPort             Port ID.
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetInput(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Sets the ISP device port.
 *
 * @param   IspPort             The port ID.
 * @param   pPortAttr           The pointer to the port configuration.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetPortAttr(ISP_PORT IspPort, ISP_PORT_ATTR_S *pPortAttr);

/*****************************************************************************/
/**
 * @brief   Gets the ISP device port.
 *
 * @param   IspPort             The port ID.
 * @param   pPortAttr           The pointer to the port configuration.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetPortAttr(ISP_PORT IspPort, ISP_PORT_ATTR_S *pPortAttr);

/*****************************************************************************/
/**
 * @brief   Enables the ISP device port.
 *
 * @param   IspPort             The port ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnablePort(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Disables the ISP device port.
 *
 * @param   IspPort             The port ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DisablePort(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Writes the data to registers.
 *
 * @param   IspPort             The port ID.
 * @param   reg                 The data address.
 * @param   val                 The data value.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_WriteReg(ISP_PORT IspPort, vsi_u32_t reg, vsi_u32_t val);

/*****************************************************************************/
/**
 * @brief   Reads data from registers.
 *
 * @param   IspPort             The port ID.
 * @param   reg                 The data address.
 * @param   pVal                The pointer to the data value.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_ReadReg(ISP_PORT IspPort, vsi_u32_t reg, vsi_u32_t *pVal);

/*****************************************************************************/
/**
 * @brief   Safety Reset.
 *
 * @param   IspDev             The ISP device ID.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SafetyReset(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Sets the output channel.
 *
 * @param   IspChn              The ID of the output channel.
 * @param   pChnAttr            The pointer to the channel configuration.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetChnAttr(ISP_CHN IspChn, ISP_CHN_ATTR_S *pChnAttr);

/*****************************************************************************/
/**
 * @brief   Gets the output channel.
 *
 * @param   IspChn              The ID of the output channel.
 * @param   pChnAttr            The pointer to the channel configuration.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetChnAttr(ISP_CHN IspChn, ISP_CHN_ATTR_S *pChnAttr);

/*****************************************************************************/
/**
 * @brief   Enables the output channel.
 *
 * @param   IspChn              The ID of the output channel.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnableChn(ISP_CHN IspChn);

/*****************************************************************************/
/**
 * @brief   Disables the output channel.
 *
 * @param   IspChn              The ID of the output channel.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DisableChn(ISP_CHN IspChn);

/*****************************************************************************/
/**
 * @brief   Abort a pending DQBUF on the output channel.
 *
 * Puts the channel's buffer queue into StreamOff so a reader blocked in
 * VSI_MPI_ISP_DQBUF returns immediately (VSI_ERR_NOT_READY). Does NOT stop the
 * MI DMA or free buffers; a subsequent VSI_MPI_ISP_DisableChn still performs the
 * full teardown. Intended to let a channel close join its reader thread promptly.
 *
 * @param   IspChn              The ID of the output channel.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_StreamOffChn(ISP_CHN IspChn);

/*****************************************************************************/
/**
 * @brief   Get straming status.
 *
 * @param   IspChn              The ID of the output channel.
 * @param   pState              The pointer to the state.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetStreamStatus(ISP_CHN IspChn, vsi_u32_t *pState);

/*****************************************************************************/
/**
 * @brief   Sets the ISP auto route.
 *
 * @param   IspPort             The port ID.
 * @param   pAutoRoute          The pointer to the auto route.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetAutoRoute(ISP_PORT IspPort, ISP_AUTO_ROUTE_S *pAutoRoute);

/*****************************************************************************/
/**
 * @brief   Gets the ISP auto route.
 *
 * @param   IspPort             The port ID.
 * @param   pAutoRoute          The pointer to the auto route.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetAutoRoute(ISP_PORT IspPort, ISP_AUTO_ROUTE_S *pAutoRoute);

/*****************************************************************************/
/**
 * @brief   Adds a buffer to an empty buffer queue.
 *
 * @param   IspChn              The ID of the output channel.
 * @param   pBuf                The pointer to the video buffer structure.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_QBUF(ISP_CHN IspChn, VIDEO_BUF_S *pBuf);

/*****************************************************************************/
/**
 * @brief   Dequeues a buffer from a full buffer queue.
 *
 * @param   IspChn              The ID of the output channel.
 * @param   pBuf                The pointer to the video buffer structure.
 * @param   timeMs              The time used to dequeue the buffer.
 *
 * @retval  VSI_SUCCESS         The operation is successful.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DQBUF(ISP_CHN IspChn, VIDEO_BUF_S *pBuf,  vsi_u32_t timeMs);

/*****************************************************************************/
/**
 * @brief   Gets MetaData.
 *
 * @param   IspPort             Port ID
 * @param   pMetaData           Pointer to the metadata
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetMetaData(ISP_PORT IspPort, ISP_METADATA_S *pMetaData);

/*****************************************************************************/
/**
 * @brief   Sets MetaData.
 *
 * @param   IspPort             Port ID
 * @param   pMetaData           Pointer to the metadata
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetMetaData(ISP_PORT IspPort, ISP_METADATA_S *pMetaData);

/*****************************************************************************/
/**
 * @brief   Sets sensor stream on.
 *
 * @param   IspPort             Port ID
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_RegIsrCallBack(ISP_DEV IspDev, ISP_ISR_CBS_S cb);

/*****************************************************************************/
/**
 * @brief   DeRegister ISP ISR callback to uplayer.
 *
 * @param   IspDev              ISP device ID.
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_DeRegIsrCallBack(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Sets sensor stream on.
 *
 * @param   IspPort             Port ID
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsStreamOn(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Sets sensor stream off.
 *
 * @param   IspPort             Port ID
 *
 * @retval  VSI_SUCCESS         Operation succeeded
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsStreamOff(ISP_PORT IspPort);

void VSI_MPI_RESET(ISP_DEV dev_id, vsi_u32_t module_id);

void VSI_MPI_RESET_CLEAR(ISP_DEV dev_id);

void VSI_MPI_ISP_PipeLineSet(ISP_PORT IspPort);


/* @} mpi_isp */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
