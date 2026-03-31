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
#ifndef __FLEXA_SYNC_H__
#define __FLEXA_SYNC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/**
 * @cond FLEXA_SYNC_V10
 *
 * @defgroup flexa_sync FLEXA_SYNCHRONIZER V10 Definitions
 * @{
 *
 */

#define VSI_FLEXA_SYNC_STREAM_MAX  12        /**< \brief The maximum FLEXA Synchronizer stream configuration array size pre-allocated. */

/** \brief   FLEXA Synchronizer stream configuration structure. */
typedef struct vsiFLEXA_SYNC_STREAM_CFG_S {
    vsi_u8_t  streamId;                  /**< \brief Stream ID. */
    vsi_u16_t entryCnt;                  /**< \brief The amount of entries in the segment buffer of the stream. */
    vsi_u16_t producerTimeOut;           /**< \brief Producer time out threshold. */
    vsi_u16_t consumerTimeOut;           /**< \brief Consumer time out threshold. */
} VSI_FLEXA_STREAM_CFG_S;

/** \brief   This stucture defines the FLEXA Synchronizer attributes. */
typedef struct vsiFLEXA_SYNC_ATTR_S {
    vsi_u8_t  streamNum;                                             /**< \brief Valid FLEXA Synchronizer stream number, must less than VSI_FLEXA_SYNC_STREAM_MAX. */
    VSI_FLEXA_STREAM_CFG_S streamAttr[VSI_FLEXA_SYNC_STREAM_MAX];    /**< \brief SBI stream configuration, reference ISP_SBI_Stream_Configure. */
} VSI_FLEXA_SYNC_ATTR_S;

/** \brief   This enumeration specifies the FLEXA Synchronizer streaming status. */
typedef enum vsiFLEXA_STREAM_STATUS_E {
    FLEXA_STREAM_STATE_UNINITIALIZED = 0,   /**< \brief Streaming is uninitialized. */
    FLEXA_STREAM_STATE_OUT_SYNC      = 1,   /**< \brief Streaming is out of sync. */
    FLEXA_STREAM_STATE_RUNNING       = 2,   /**< \brief Streaming is running. */
} VSI_FLEXA_STREAM_STATUS_E;

/** \brief   FLEXA Synchronizer stream status structure. */
typedef struct vsiFLEXA_SYNC_STREAM_STATUS_S {
    vsi_u8_t  streamId;                  /**< \brief Stream ID. */
    vsi_u16_t validEntryCnt;             /**< \brief The amount of valid entries in the segment buffer. */
    vsi_u32_t streamState;               /**< \brief Streaming state, reference VSI_FLEXA_STREAM_STATUS_E. */
    vsi_u32_t reserved;                  /**< \brief Reserved parameter for the extension. */
} VSI_FLEXA_STREAM_STATUS_S;

/*****************************************************************************/
/**
 * @brief   Gets FLEXA Synchronizer attributes.
 *
 * \param   IspChn           ISP information, include device ID, port ID, channel ID.
 * \param   pSyncAttr        Pointer to the FLEXA Synchronizer attributes.
 *
 * @retval  VSI_SUCCESS      The operation is successful.
 *
 *****************************************************************************/
int VSI_FLEXA_GetSyncAttr(ISP_CHN IspChn, VSI_FLEXA_SYNC_ATTR_S *pSyncAttr);

/*****************************************************************************/
/**
 * @brief   Sets FLEXA Synchronizer attributes.
 *
 * \param   IspChn           ISP information, include device ID, port ID, channel ID.
 * \param   pSyncAttr        Pointer to the FLEXA Synchronizer attributes.
 *
 * @retval  VSI_SUCCESS      The operation is successful.
 *
 *****************************************************************************/
int VSI_FLEXA_SetSyncAttr(ISP_CHN IspChn, VSI_FLEXA_SYNC_ATTR_S *pSyncAttr);

/*****************************************************************************/
/**
 * @brief   Gets FLEXA Synchronizer stream status.
 *
 * \param   IspChn           ISP information, include device ID, port ID, channel ID.
 * \param   pSyncStatus      Pointer to the FLEXA Synchronizer stream status structure.
 *
 * @retval  VSI_SUCCESS      The operation is successful.
 *
 *****************************************************************************/
int VSI_FLEXA_GetSyncStreamStatus(ISP_CHN IspChn, VSI_FLEXA_STREAM_STATUS_S *pSyncStatus);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
