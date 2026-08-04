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


#include "vsi_comm_sns.h"
#include "vsi_comm_isp.h"


/**
 * @defgroup mpi_isp ISP Definitions
 * @{
 *
 *
 */

/*****************************************************************************/
/**
 * @brief   Creates and initializes an ISP instance for an ISP device and initializes the ISP pipeline.
 *
 * @param   IspDev              The ID of the ISP device.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_Init(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Destroys an ISP instance for an ISP device.
 *
 * @param   IspDev              The ID of the ISP device.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_Exit(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Registers the sensor callback function for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pSnsObj             A pointer to the configurations of the sensor.
 * @param   snsDev              The ID of the sensor device.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsRegCallBack(ISP_PORT IspPort, ISP_SNS_OBJ_S *pSnsObj, vsi_u8_t snsDev);

/*****************************************************************************/
/**
 * @brief   Unregisters the sensor callback function for an ISP device port.
 *
 * @param   IspPort              The ID of the port.
 *
 * @retval  VSI_SUCCESS          The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsUnRegCallBack(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Sets the attribute of an ISP device.
 *
 * @param   IspDev              The ID of the ISP device.
 * @param   pDevAttr            A pointer to the device attribute.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetDevAttr(ISP_DEV IspDev, ISP_DEV_ATTR_S *pDevAttr);

/*****************************************************************************/
/**
 * @brief   Gets the attribute of an ISP device.
 *
 * @param   IspDev              The ID of the ISP device.
 * @param   pDevAttr            A pointer to a memory place for receiving the device attribute.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetDevAttr(ISP_DEV IspDev, ISP_DEV_ATTR_S *pDevAttr);

/*****************************************************************************/
/**
 * @brief   Enables an ISP device.
 *
 * @param   IspDev              The ID of the ISP device.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnableDev(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Disables an ISP device.
 *
 * @param   IspDev              The ID of the ISP device.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DisableDev(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Sets the attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pPortAttr           A pointer to the port attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetPortAttr(ISP_PORT IspPort, ISP_PORT_ATTR_S *pPortAttr);

/*****************************************************************************/
/**
 * @brief   Gets the attributes of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pPortAttr           A pointer to a memory place for receiving the port attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetPortAttr(ISP_PORT IspPort, ISP_PORT_ATTR_S *pPortAttr);

/*****************************************************************************/
/**
 * @brief   Enables an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnablePort(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Disables an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DisablePort(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Writes data to a register through an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   reg                 The address of the register.
 * @param   val                 The data to be written.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_WriteReg(ISP_PORT IspPort, vsi_u32_t reg, vsi_u32_t val);

/*****************************************************************************/
/**
 * @brief   Reads data from a register through an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   reg                 The address of the register.
 * @param   pVal                A pointer to a memory place for receiving the register data.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_ReadReg(ISP_PORT IspPort, vsi_u32_t reg, vsi_u32_t *pVal);

/*****************************************************************************/
/**
 * @brief   Performs a safety reset for an ISP device.
 *
 * @param   IspDev              The ID of the ISP device.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SafetyReset(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   Sets the attributes of an ISP channel.
 *
 * @param   IspChn              The ID of the channel.
 * @param   pChnAttr            A pointer to the channel attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetChnAttr(ISP_CHN IspChn, ISP_CHN_ATTR_S *pChnAttr);

/*****************************************************************************/
/**
 * @brief   Gets the attributes of an ISP channel.
 *
 * @param   IspChn              The ID of the channel.
 * @param   pChnAttr            A pointer to a memory place for receiving the channel attributes.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetChnAttr(ISP_CHN IspChn, ISP_CHN_ATTR_S *pChnAttr);

/*****************************************************************************/
/**
 * @brief   Enables an ISP channel.
 *
 * @param   IspChn              The ID of the channel.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_EnableChn(ISP_CHN IspChn);

/*****************************************************************************/
/**
 * @brief   Disables an ISP channel.
 *
 * @param   IspChn              The ID of the channel.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DisableChn(ISP_CHN IspChn);

/*****************************************************************************/
/**
 * @brief   Gets the streaming status of an ISP channel.
 *
 * @param   IspChn              The ID of the channel.
 * @param   pState              A pointer to a memory space for receiving the status.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetStreamStatus(ISP_CHN IspChn, vsi_u32_t *pState);

/*****************************************************************************/
/**
 * @brief   Sets auto routes for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAutoRoute          A pointer to the auto route configurations.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetAutoRoute(ISP_PORT IspPort, ISP_AUTO_ROUTE_S *pAutoRoute);

/*****************************************************************************/
/**
 * @brief   Gets the auto route configurations of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pAutoRoute          A pointer to a memory place for receiving the auto route configurations.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetAutoRoute(ISP_PORT IspPort, ISP_AUTO_ROUTE_S *pAutoRoute);

/*****************************************************************************/
/**
 * @brief   Adds a video buffer to the buffer queue of an ISP channel.
 *
 * @param   IspChn              The ID of the ISP channel.
 * @param   pBuf                A pointer to the buffer.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_QBUF(ISP_CHN IspChn, VIDEO_BUF_S *pBuf);

/*****************************************************************************/
/**
 * @brief   Dequeues a video buffer from the buffer queue of an ISP channel.
 *
 * @param   IspChn              The ID of the channel.
 * @param   pBuf                A pointer to the buffer.
 * @param   timeMs              The time it takes to dequeue the buffer.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_DQBUF(ISP_CHN IspChn, VIDEO_BUF_S *pBuf,  vsi_u32_t timeMs);

/* @} mpi_isp */

/**
 * @defgroup mpi_isp_metadata Metadata Definitions
 * @{
 *  
 */

/*****************************************************************************/
/**
 * @brief   Gets the metadata of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pMetaData           A pointer to a memory place for receiving the metadata.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_GetMetaData(ISP_PORT IspPort, void *pMetaData);

/*****************************************************************************/
/**
 * @brief   Sets the metadata of an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 * @param   pMetaData           A pointer to the metadata.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SetMetaData(ISP_PORT IspPort, void *pMetaData);

/* @} mpi_isp_metadata */

/**
 * @defgroup mpi_isp ISP Definitions
 * @{
 *
 *
 */

/*****************************************************************************/
/**
 * @brief   Turns on sensor streaming for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsStreamOn(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   Turns off sensor streaming for an ISP device port.
 *
 * @param   IspPort             The ID of the port.
 *
 * @retval  VSI_SUCCESS         The operation succeeds.
 *
 *****************************************************************************/
int VSI_MPI_ISP_SnsStreamOff(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   register ISP/MI ISR callbacks for the driver layer.
 */
int VSI_MPI_ISP_RegIsrCallBack(ISP_DEV IspDev, ISP_ISR_CBS_S cbs);

/*****************************************************************************/
/**
 * @brief   unregister ISP/MI ISR callbacks.
 */
int VSI_MPI_ISP_DeRegIsrCallBack(ISP_DEV IspDev);

/*****************************************************************************/
/**
 * @brief   pipeline is created.
 */
void VSI_MPI_ISP_PipeLineSet(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   port input configuration.
 */
int VSI_MPI_ISP_SetInput(ISP_PORT IspPort);

/*****************************************************************************/
/**
 * @brief   sub-module soft reset via VI_IRCL.
 */
void VSI_MPI_RESET(ISP_DEV IspDev, vsi_u32_t module);

/*****************************************************************************/
/**
 * @brief   release all sub-module soft resets via VI_IRCL.
 */
void VSI_MPI_RESET_CLEAR(ISP_DEV IspDev);

/* @} mpi_isp */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
