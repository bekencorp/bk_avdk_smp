// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file
 *   This file implements the OpenThread platform abstraction for radio communication.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "FreeRTOS.h"
#include "task.h"

#include <openthread/logging.h>
#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/link.h>
#include <openthread/tasklet.h>

//#include <platform-config.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>
#include <openthread/random_noncrypto.h>

#include "utils/code_utils.h"
#include "utils/link_metrics.h"
#include "utils/mac_frame.h"
#include "openthread-system.h"
#include "bk_802154.h"
#include "os/os.h"
#include "cmsis_gcc.h"

#include "bk_rf_internal.h"
#include "bk_openthread_coex.h"
#include "sys_ll.h"

// clang-format off

#define SHORT_ADDRESS_SIZE    2            ///< Size of MAC short address.
#define US_PER_MS             1000ULL      ///< Microseconds in millisecond.

#define ACK_REQUEST_OFFSET       1         ///< Byte containing Ack request bit (+1 for frame length byte).
#define ACK_REQUEST_BIT          (1 << 5)  ///< Ack request bit.
#define FRAME_PENDING_OFFSET     1         ///< Byte containing pending bit (+1 for frame length byte).
#define FRAME_PENDING_BIT        (1 << 4)  ///< Frame Pending bit.
#define SECURITY_ENABLED_OFFSET  1         ///< Byte containing security enabled bit (+1 for frame length byte).
#define SECURITY_ENABLED_BIT     (1 << 3)  ///< Security enabled bit.

#define RSSI_SETTLE_TIME_US   40           ///< RSSI settle time in microseconds.
#define SAFE_DELTA            1000         ///< A safe value for the `dt` parameter of delayed operations.

#define CSL_UNCERT            100           ///< The Uncertainty of the scheduling CSL of transmission by the parent, in ±10 us units.

#define BK_802154_RX_BUFFERS  20

#define BK_TX_TIME_OFFSET     350         //unit:us
#define BK_SFD_TIME_OFFSET    450         //unit:us

#if defined(__ICCARM__)
_Pragma("diag_suppress=Pe167")
#endif

enum
{
    BK_RECEIVE_SENSITIVITY  = -100, // dBm
    BK_MIN_CCA_ED_THRESHOLD = -94,  // dBm
};

// clang-format on

static bool sDisabled;

static otError      sReceiveError = OT_ERROR_NONE;
static otRadioFrame sReceivedFrame;
static otRadioFrame sTransmitFrame;

static otRadioFrame sAckFrame;
static uint8_t      sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE + 1];

static uint8_t      sAckPsdu[OT_RADIO_FRAME_MAX_SIZE + 1];

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
static otRadioIeInfo sTransmitIeInfo;
static otInstance   *sInstance = NULL;
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
static uint8_t sAckIeData[OT_ACK_IE_MAX_SIZE];
static uint8_t sAckIeDataLength = 0;
#endif

static int8_t   sMaxTxPowerTable[OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN + 1];
static int8_t   sDefaultTxPower;
static int8_t   sLnaGain    = 0;
static uint16_t sRegionCode = 0;

static uint32_t sEnergyDetectionTime;
static uint8_t  sEnergyDetectionChannel;
static int8_t   sEnergyDetected;

static otRadioContext sRadioContext;
static otPanId  sPanid;

typedef enum
{
    kPendingEventSleep,                // Requested to enter Sleep state.
    kPendingEventFrameTransmitted,     // Transmitted frame and received ACK (if requested).
    kPendingEventChannelAccessFailure, // Failed to transmit frame (channel busy).
    kPendingEventInvalidOrNoAck,       // Failed to transmit frame (received invalid or no ACK).
    kPendingEventNoBuf,                // Failed to transmit frame (no buffer).
    kPendingEventReceiveFailed,        // Failed to receive a valid frame.
    kPendingEventEnergyDetectionStart, // Requested to start Energy Detection procedure.
    kPendingEventEnergyDetected,       // Energy Detection finished.
} RadioPendingEvents;

static uint32_t sPendingEvents;

static void bk_802154_transmit_done_no_ack(otInstance *aInstance);
static void bk_802154_transmit_done_with_ack(otInstance *aInstance, bk_802154_frame_t *data_p);
static void bk_802154_transmit_failed_ex(otInstance *aInstance);
static void bk_802154_energy_detected_ex(otInstance *aInstance);
static void bk_802154_received_done_ex(otInstance *aInstance, bk_802154_frame_t *data_p);
static void bk_802154_receive_failed_ex(otInstance *aInstance);
static void ReverseExtAddress(otExtAddress *aReversed, const otExtAddress *aOrigin)
{
    for (size_t i = 0; i < sizeof(*aReversed); i++)
    {
        aReversed->m8[i] = aOrigin->m8[sizeof(*aOrigin) - 1 - i];
    }
}

extern beken_queue_t g_802154_msg_queue;

static void bk_802154_process_msg_queue(otInstance *aInstance)
{
    uint32_t timeout_ms = 10;
#if 0
    if (otTaskletsArePending(aInstance)) {
        timeout_ms = 0;
    } else {
        timeout_ms = 10;
    }

    uint32_t ulNotifiedValue;
    if (xTaskNotifyWait(0x00,
                    ULONG_MAX,
                    &ulNotifiedValue,
                    timeout_ms) == pdTRUE)
    {
        for (int i = 0; i < BK_802154_MSG_MAX; i++)
        {
            if ((ulNotifiedValue >> i) & 0x01)
            {
                switch (i)
                {
                case BK_802154_MSG_TX_DONE_NO_ACK:
                    otLogDebgPlat("tx done no ack\n");
                    bk_802154_transmit_done_without_ack(aInstance);
                    break;
                case BK_802154_MSG_TX_DONE_WITH_ACK:
                    otLogDebgPlat("tx done with ack\n");
                    bk_802154_transmit_done_with_ack(aInstance, bk_ieee802154_get_ack_buffer());
                    break;
                case BK_802154_MSG_RX_DONE:
                    otLogDebgPlat("rx done\n");
                    bk_802154_frame_t *rx_frame = NULL;
                    rx_frame = bk_ieee802154_get_rx_read_buffer();
                    if (rx_frame)
                    {
                        bk_802154_received_done_ex(aInstance, rx_frame);
                    }
                    break;
                case BK_802154_MSG_TX_FAIL:
                    otLogDebgPlat("tx fail\n");
                    bk_802154_transmit_failed_ex(aInstance);
                    break;
                case BK_802154_MSG_RX_FAIL:
                    otLogDebgPlat("rx fail, err\n");
                    bk_802154_receive_failed_ex(aInstance);
                    break;
                case BK_802154_MSG_ED_DONE:
                    otLogDebgPlat("energy detect done\n");
                    bk_802154_energy_detected_ex(aInstance);
                    break;
                default:
                    otLogCritPlat("undefined message:%d\n",i);
                    break;
                }
            }
        }
    }
    else
    {
        //otLogCritPlat("openThread xTaskNotifyWait failed\r\n");
    }
#else
    bk_err_t ret;
    bk_802154_msg_t msg = {0};
    if (otTaskletsArePending(aInstance)) {
        timeout_ms = 0;
    } else {
        timeout_ms = 1;
    }
    ret = rtos_pop_from_queue(&g_802154_msg_queue, &msg, timeout_ms);
    if (BK_OK == ret) {
        switch (msg.msg_type) {
            case BK_802154_MSG_TX_DONE_NO_ACK:
                otLogDebgPlat("tx done no ack\n");
                bk_802154_transmit_done_no_ack(aInstance);
                break;
            case BK_802154_MSG_TX_DONE_WITH_ACK:
                otLogDebgPlat("tx done with ack\n");
                bk_802154_transmit_done_with_ack(aInstance, msg.msg.frame);
                break;
            case BK_802154_MSG_RX_DONE:
                otLogDebgPlat("rx done\n");
#if 0
                bk_802154_frame_t *rx_frame = NULL;
                rx_frame = bk_ieee802154_get_rx_read_buffer();
                if (rx_frame)
                {
                    bk_802154_received_done_ex(aInstance, rx_frame);
                }
#endif
                bk_802154_received_done_ex(aInstance, msg.msg.frame);
                break;
            case BK_802154_MSG_TX_FAIL:
                otLogDebgPlat("tx fail\n");
                bk_802154_transmit_failed_ex(aInstance);
                break;
            case BK_802154_MSG_RX_FAIL:
                otLogDebgPlat("rx fail, err\n");
                bk_802154_receive_failed_ex(aInstance);
                break;
            case BK_802154_MSG_ED_DONE:
                otLogDebgPlat("energy detect done\n");
                bk_802154_energy_detected_ex(aInstance);
                break;
            case BK_802154_MSG_FRAME_PROTECT:
                bk_ieee802154_frame_protect_handle_msg(msg.msg.frame_protect.action,
                                                       msg.msg.frame_protect.dur_ms);
                break;
            default:
                otLogCritPlat("undefined message:%d\n", msg.msg_type);
        }
    }
#endif
}

static int8_t GetTransmitPowerForChannel(uint8_t aChannel)
{
    return 0;
}

static void dataInit(void)
{
    sDisabled = true;

    sDefaultTxPower      = OT_RADIO_POWER_INVALID;
    sTransmitFrame.mPsdu = sTransmitPsdu + 1;
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = &sTransmitIeInfo;
#endif

    sReceiveError = OT_ERROR_NONE;

    sReceivedFrame.mPsdu = NULL;

    sAckFrame.mInfo.mTxInfo.mIeInfo = NULL;
    for (size_t i = 0; i < otARRAY_LENGTH(sMaxTxPowerTable); i++)
    {
        sMaxTxPowerTable[i] = OT_RADIO_POWER_INVALID;
    }
    sAckFrame.mPsdu = sAckPsdu + 1;
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    otLinkMetricsInit(BK_RECEIVE_SENSITIVITY);
#endif
    memset(&sRadioContext, 0x0, sizeof(otRadioContext));
}

static void convertShortAddress(uint8_t *aTo, uint16_t aFrom)
{
    aTo[0] = (uint8_t)aFrom;
    aTo[1] = (uint8_t)(aFrom >> 8);
}

static inline bool isPendingEventSet(RadioPendingEvents aEvent)
{
    return sPendingEvents & (1UL << aEvent);
}

static void setPendingEvent(RadioPendingEvents aEvent)
{
    volatile uint32_t pendingEvents;
    uint32_t          bitToSet = 1UL << aEvent;

    do
    {
        pendingEvents = __LDREXW((uint32_t *)&sPendingEvents);
        pendingEvents |= bitToSet;
    } while (__STREXW(pendingEvents, (uint32_t *)&sPendingEvents));

    otSysEventSignalPending();
}

static void resetPendingEvent(RadioPendingEvents aEvent)
{
    volatile uint32_t pendingEvents;
    uint32_t          bitsToRemain = ~(1UL << aEvent);

    do
    {
        pendingEvents = __LDREXW((uint32_t *)&sPendingEvents);
        pendingEvents &= bitsToRemain;
    } while (__STREXW(pendingEvents, (uint32_t *)&sPendingEvents));
}

static inline void clearPendingEvents(void)
{
    // Clear pending events that could cause race in the MAC layer.
    volatile uint32_t pendingEvents;
    uint32_t          bitsToRemain = ~(0UL);

    bitsToRemain &= ~(1UL << kPendingEventSleep);

    do
    {
        pendingEvents = __LDREXW((uint32_t *)&sPendingEvents);
        pendingEvents &= bitsToRemain;
    } while (__STREXW(pendingEvents, (uint32_t *)&sPendingEvents));
}

#if !OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);
    uint8_t eui64[8] = {0};
    memcpy(aIeeeEui64, eui64, 8);
}
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(aInstance);
    sPanid = aPanId;
    bk_ieee802154_set_panid(aPanId);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    ReverseExtAddress(&sRadioContext.mExtAddress, aExtAddress);
#endif
    bk_ieee802154_set_extended_address(aExtAddress->m8);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    sRadioContext.mShortAddress = aShortAddress;
    bk_ieee802154_set_short_address(aShortAddress);
}

void otPlatRadioSetAlternateShortAddress(otInstance *aInstance, otShortAddress aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sRadioContext.mAlternateShortAddress = aShortAddress;
}

void bkRadioInit(void) {
    dataInit();
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    otLinkMetricsInit(BK_RECEIVE_SENSITIVITY);
#endif
    bk_ieee802154_enable();
    bk_ieee802154_set_promiscuous(false);
    bk_ieee802154_set_rx_when_idle(false);

}

void bkRadioDeinit(void)
{
    //bk_802154_sleep();
    bk_ieee802154_disable();
    sPendingEvents = 0;
}

void bkRadioClearPendingEvents(void)
{
    sPendingEvents = 0;
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sDisabled)
    {
        return OT_RADIO_STATE_DISABLED;
    }

    switch (bk_ieee802154_state_get())
    {
        case BK_802154_STATE_SLEEP:
            return OT_RADIO_STATE_SLEEP;

        case BK_802154_STATE_RECEIVE:
        case BK_802154_STATE_RECEIVE_BUSY:
        case BK_802154_STATE_TRANSMIT_IMM_ACK:
        case BK_802154_STATE_TRANSMIT_ENH_ACK:
        case BK_802154_STATE_ED:
            return OT_RADIO_STATE_RECEIVE;

        case BK_802154_STATE_TRANSMIT:
        case BK_802154_STATE_TRANSMIT_CCA:
            return OT_RADIO_STATE_TRANSMIT;

        default:
            return OT_RADIO_STATE_INVALID; // Make sure driver returned valid state.
    }

    return OT_RADIO_STATE_RECEIVE; // It is the default state. Return it in case of unknown.
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return !sDisabled;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

#if !OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    OT_UNUSED_VARIABLE(aInstance);
#else
    sInstance = aInstance;
#endif

    if (sDisabled)
    {
        sDisabled = false;
        error     = OT_ERROR_NONE;
    }

    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT(otPlatRadioIsEnabled(aInstance));
    otEXPECT_ACTION(otPlatRadioGetState(aInstance) == OT_RADIO_STATE_SLEEP || isPendingEventSet(kPendingEventSleep),
            error = OT_ERROR_INVALID_STATE);

    sDisabled = true;

exit:
    return error;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_channel_set(aChannel);
    bk_ieee802154_receive();

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
otError otPlatRadioReceiveAt(otInstance *aInstance, uint8_t aChannel, uint32_t aStart, uint32_t aDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_receive_at((aStart + aDuration));
    clearPendingEvents();
    return OT_ERROR_NONE;
}
#endif

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError err;
    aFrame->mPsdu[-1] = aFrame->mLength;
    // This block should be called in SFD ISR
    {
        //uint64_t sfdTxTime = otPlatTimeGet() - 32 * (aFrame->mLength);
        uint64_t sfdTxTime = otPlatTimeGet() + BK_SFD_TIME_OFFSET;

        err = otMacFrameProcessTxSfd(&sTransmitFrame, sfdTxTime, &sRadioContext);
        if (err != OT_ERROR_NONE)
        {
            otLogCritPlat("otMacFrameProcessTxSfd faied[%s][%d]", __func__, __LINE__);
            return err;
        }
    }

    {
        bk_ieee802154_channel_set(aFrame->mChannel);
#if 1
        if (aFrame->mInfo.mTxInfo.mTxDelay)
        {
            uint64_t off_time = (uint64_t)aFrame->mInfo.mTxInfo.mTxDelay + (uint64_t)aFrame->mInfo.mTxInfo.mTxDelayBaseTime;
            bk_err_t rtn =  bk_ieee802154_transmit_at(&aFrame->mPsdu[-1], aFrame->mInfo.mTxInfo.mCsmaCaEnabled, off_time - BK_TX_TIME_OFFSET);
            if (rtn != BK_OK)
            {
                return OT_ERROR_INVALID_STATE;
            }
        }
        else
#endif
        {
            bk_err_t rtn = bk_ieee802154_transmit(&aFrame->mPsdu[-1], aFrame->mInfo.mTxInfo.mCsmaCaEnabled);
            if (rtn != BK_OK)
            {
                return OT_ERROR_INVALID_STATE;
            }
        }
    }

    otPlatRadioTxStarted(aInstance, aFrame);
    return OT_ERROR_NONE; 
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return bk_ieee802154_get_recent_rssi();
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (otRadioCaps)(OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF |
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
            OT_RADIO_CAPS_TRANSMIT_SEC | OT_RADIO_CAPS_TRANSMIT_TIMING | OT_RADIO_CAPS_RECEIVE_TIMING |
#endif
            OT_RADIO_CAPS_SLEEP_TO_TX);
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return bk_ieee802154_get_promiscuous();
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_set_promiscuous(aEnable);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_set_pending_mode(aEnable);
}

//TODO
otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_add_pending_addr((uint8_t *)&aShortAddress, true);
    return OT_ERROR_NONE;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_add_pending_addr(aExtAddress->m8, false);
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_clear_pending_addr((uint8_t *)&aShortAddress, true);
    return OT_ERROR_NONE;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_clear_pending_addr(aExtAddress->m8, false);
    return OT_ERROR_NONE;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_reset_pending_table(true);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_reset_pending_table(false);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    sEnergyDetectionChannel = aScanChannel;
    sEnergyDetectionTime = aScanDuration * US_PER_MS / US_PER_SYMBLE;
    clearPendingEvents();
    bk_ieee802154_thread_ed_scan_info(sEnergyDetectionTime, aScanChannel);
    bk_ieee802154_channel_set(aScanChannel);
    bk_ieee802154_energy_detect(sEnergyDetectionTime);

    return OT_ERROR_NONE;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    *aPower = bk_ieee802154_get_txpower();
    return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_set_txpower(aPower);

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    *aThreshold = bk_ieee802154_get_cca_threshold();

    return OT_ERROR_NONE;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    bk_ieee802154_set_cca_threshold(aThreshold);

    return OT_ERROR_NONE;
}

otError otPlatRadioGetFemLnaGain(otInstance *aInstance, int8_t *aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    if (aGain == NULL)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        *aGain = sLnaGain;
    }

    return error;
}

otError otPlatRadioSetFemLnaGain(otInstance *aInstance, int8_t aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    int8_t  threshold;
    int8_t  oldLnaGain = sLnaGain;
    otError error      = OT_ERROR_NONE;

    error = otPlatRadioGetCcaEnergyDetectThreshold(aInstance, &threshold);
    otEXPECT(error == OT_ERROR_NONE);

    sLnaGain = aGain;
    error    = otPlatRadioSetCcaEnergyDetectThreshold(aInstance, threshold);
    otEXPECT_ACTION(error == OT_ERROR_NONE, sLnaGain = oldLnaGain);

exit:
    return error;
}

void bkRadioProcess(otInstance *aInstance)
{
    bool isEventPending = false;

    bk_802154_process_msg_queue(aInstance);
    if (isPendingEventSet(kPendingEventSleep))
    {
        isEventPending = true;
    }
#if 1
    if (isPendingEventSet(kPendingEventFrameTransmitted))
    {
        resetPendingEvent(kPendingEventFrameTransmitted);
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, &sAckFrame, OT_ERROR_NONE);
        }
#if 0
        bk_802154_frame_t *bufferAddress = (bk_802154_frame_t *) &sAckFrame.mPsdu[-1];
        bufferAddress->used = false;
#endif
    }
#endif

    if (isEventPending)
    {
        otSysEventSignalPending();
    }
    if (bk_ieee802154_state_get() == BK_802154_STATE_IDLE &&
        bk_ieee802154_check_delayed_send())
    {
        otLogCritPlat("sending delayed tx");
        otPlatRadioTransmit(aInstance, otPlatRadioGetTransmitBuffer(aInstance));
    }
}

void bk_802154_received_done_ex(otInstance *aInstance, bk_802154_frame_t *data_p)
{
    otError      error = OT_ERROR_NONE;
    otMacAddress macAddress;
    OT_UNUSED_VARIABLE(macAddress);

    memset(&sReceivedFrame, 0x0, sizeof(otRadioFrame));
    sReceivedFrame.mPsdu               = &data_p->frame[1];
    sReceivedFrame.mLength             = data_p->frame[0];
    sReceivedFrame.mInfo.mRxInfo.mRssi = data_p->rssi;
    sReceivedFrame.mInfo.mRxInfo.mLqi  = data_p->lqi;
    sReceivedFrame.mChannel            = bk_ieee802154_channel_get();

    sReceivedFrame.mInfo.mRxInfo.mAckedWithFramePending = data_p->pending;
    sReceivedFrame.mInfo.mRxInfo.mAckedWithSecEnhAck    = data_p->enh_ack;

    // Get the timestamp when the SFD was received.
    //sReceivedFrame->mInfo.mRxInfo.mTimestamp = bk_802154_timestamp_end_to_phr_convert(time, p_data[0]);
    sReceivedFrame.mInfo.mRxInfo.mTimestamp = data_p->time;
    otEXPECT_ACTION(otMacFrameDoesAddrMatchAny(&sReceivedFrame, sPanid, sRadioContext.mShortAddress,
                                                   sRadioContext.mAlternateShortAddress, &sRadioContext.mExtAddress),
                        error = OT_ERROR_ABORT);
    
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    otEXPECT_ACTION(otMacFrameGetSrcAddr(&sReceivedFrame, &macAddress) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
#endif

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    if (sReceivedFrame.mInfo.mRxInfo.mAckedWithSecEnhAck)
    {
        sReceivedFrame.mInfo.mRxInfo.mAckFrameCounter    = otMacFrameGetFrameCounter(&sAckFrame);
        sReceivedFrame.mInfo.mRxInfo.mAckKeyId    = otMacFrameGetKeyId(&sAckFrame);
    }
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

exit:
    if (error != OT_ERROR_ABORT)
    {
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, &sReceivedFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            otPlatRadioReceiveDone(aInstance, &sReceivedFrame, OT_ERROR_NONE);
        }
    }
    else
    {
        otPlatRadioReceiveDone(aInstance, NULL, OT_ERROR_ABORT);
        otLogCritPlat("Abort Error");
    }
    data_p->used = false;
}

void bk_802154_receive_failed_ex(otInstance *aInstance)
{
    bk_802154_rx_err_t error = bk_ieee802154_get_tx_err();
    switch (error)
    {
        case BK_802154_RX_ERR_MALFORMED:
            sReceiveError = OT_ERROR_FCS;
            break;

        case BK_802154_RX_ERR_ABORT:
        case BK_802154_RX_ERR_NO_BUFFER:
            sReceiveError = OT_ERROR_FAILED;
            break;

        default:
            otLogCritPlat("[Warning]received failed type:%d", error);
            sReceiveError = OT_ERROR_FAILED;
            break;
    }

#if 0
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    if ((error == BK_802154_RX_ERROR_DELAYED_TIMEOUT) || (error == BK_802154_RX_ERROR_TIMESLOT_ENDED))
    {
        sReceiveError = OT_ERROR_NONE;
        setPendingEvent(kPendingEventSleep);
    }
    else
#endif
#endif
    {
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, NULL, sReceiveError);
        }
        else
#endif
        {
            otPlatRadioReceiveDone(aInstance, NULL, sReceiveError);
        }
    }
}

#if 1
void bk_802154_transmit_done_no_ack(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NONE);
    }
    else
#endif
    {
        otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_NONE);
    }
}

void bk_802154_transmit_done_with_ack_rapid(bk_802154_frame_t *data_p)
{
#if 0
    if (data_p->used == false)
        otLogCritPlat("SHOULD NOT HAPPEN!!!");
    if (data_p == NULL)
        otLogCritPlat("SHOULD NOT HAPPEN!!!");
#endif

    bool isTxDone = false;

    uint8_t rxSeq;
    uint8_t txSeq;

    sAckFrame.mPsdu               = &data_p->frame[1];
    sAckFrame.mLength             = data_p->frame[0];
    sAckFrame.mInfo.mRxInfo.mRssi = data_p->rssi;
    sAckFrame.mInfo.mRxInfo.mLqi  = data_p->lqi;
    sAckFrame.mChannel            = bk_ieee802154_channel_get();

    // Unable to simulate SFD, so use the rx done timestamp instead.
    sAckFrame.mInfo.mRxInfo.mTimestamp = data_p->time;

    isTxDone = (otMacFrameGetSequence(&sAckFrame, &rxSeq) == OT_ERROR_NONE &&
               otMacFrameGetSequence(&sTransmitFrame, &txSeq) == OT_ERROR_NONE && rxSeq == txSeq);

    if (!isTxDone)
    {
        otLogWarnPlat("ACK is NOT expected!,rxSeq:%d,txSeq:%d,data_p addr:%p\n", rxSeq, txSeq, data_p);
        //Mightbe the sTransmitFrame is loaded by the next frame, we allow this
    }
    setPendingEvent(kPendingEventFrameTransmitted);
#if CONFIG_OT_TRIP_COEX_EN
    bk_ieee802154_thread_rf_free(bk_ot_get_rf_priority(), bk_ot_get_thread_task_type(), false);

#endif

}

void bk_802154_transmit_done_with_ack(otInstance *aInstance, bk_802154_frame_t *data_p)
{
    if (data_p->used == false)
        otLogCritPlat("SHOULD NOT HAPPEN!!!");
    if (data_p == NULL)
        otLogCritPlat("SHOULD NOT HAPPEN!!!");

    bool isTxDone = false;
    otRadioFrame ackFrame = {0};

    uint8_t rxSeq;
    uint8_t txSeq;
    
    ackFrame.mPsdu               = &data_p->frame[1];
    ackFrame.mLength             = data_p->frame[0];
    ackFrame.mInfo.mRxInfo.mRssi = data_p->rssi;
    ackFrame.mInfo.mRxInfo.mLqi  = data_p->lqi;
    ackFrame.mChannel            = bk_ieee802154_channel_get();

    // Unable to simulate SFD, so use the rx done timestamp instead.
    ackFrame.mInfo.mRxInfo.mTimestamp = data_p->time;

    isTxDone = (otMacFrameGetSequence(&ackFrame, &rxSeq) == OT_ERROR_NONE &&
               otMacFrameGetSequence(&sTransmitFrame, &txSeq) == OT_ERROR_NONE && rxSeq == txSeq);

    if (!isTxDone)
    {
        otLogWarnPlat("ACK is NOT expected!,rxSeq:%d,txSeq:%d\n", rxSeq, txSeq);
        //Mightbe the sTransmitFrame is loaded by the next frame, we allow this
    }
    
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NONE);
    }
    else
#endif
    {
        otPlatRadioTxDone(aInstance, &sTransmitFrame, &ackFrame, OT_ERROR_NONE);
    }
    data_p->used = false;
}

void bk_802154_transmit_failed_ex(otInstance *aInstance)
{
    bk_802154_tx_err_t error = bk_ieee802154_get_tx_err();
    switch (error)
    {
        case BK_802154_TX_ERR_CCA_BUSY:
        case BK_802154_TX_ERR_ABORT:
#if OPENTHREAD_CONFIG_DIAG_ENABLE

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_CHANNEL_ACCESS_FAILURE);
            }
            else
#endif
            {
                otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_CHANNEL_ACCESS_FAILURE);
            }
            break;

        case BK_802154_TX_ERR_INVALID_ACK:
        case BK_802154_TX_ERR_NO_ACK:
#if OPENTHREAD_CONFIG_DIAG_ENABLE

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NO_ACK);
            }
            else
#endif
            {
                otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_NO_ACK);
            }
            break;
        case BK_802154_TX_ERR_NO_BUFFER:
#if OPENTHREAD_CONFIG_DIAG_ENABLE

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NO_BUFS);
            }
            else
#endif
            {
                otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_NO_BUFS);
            }
            break;

        default:
            {
                otLogCritPlat("Unsupported Error type\r\n");
            }
            assert(false);
    }
}

void bk_802154_energy_detected_ex(otInstance *aInstance)
{
    //sEnergyDetected = result;
    otPlatRadioEnergyScanDone(aInstance, sEnergyDetected);
}
#endif

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return BK_RECEIVE_SENSITIVITY;
}

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return otPlatTimeGet();
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
void otPlatRadioSetMacKey(otInstance             *aInstance,
                          uint8_t                 aKeyIdMode,
                          uint8_t                 aKeyId,
                          const otMacKeyMaterial *aPrevKey,
                          const otMacKeyMaterial *aCurrKey,
                          const otMacKeyMaterial *aNextKey,
                          otRadioKeyType          aKeyType)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyIdMode);

    otEXPECT(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

    sRadioContext.mKeyId               = aKeyId;
    sRadioContext.mKeyType             = aKeyType;
    sRadioContext.mPrevMacFrameCounter = sRadioContext.mMacFrameCounter;
    sRadioContext.mMacFrameCounter     = 0;

    memcpy(&sRadioContext.mPrevKey, aPrevKey, sizeof(otMacKeyMaterial));
    memcpy(&sRadioContext.mCurrKey, aCurrKey, sizeof(otMacKeyMaterial));
    memcpy(&sRadioContext.mNextKey, aNextKey, sizeof(otMacKeyMaterial));

exit:
    return;
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    OT_UNUSED_VARIABLE(aInstance);

    sRadioContext.mMacFrameCounter = aMacFrameCounter;
}

#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError otPlatRadioEnableCsl(otInstance         *aInstance,
                             uint32_t            aCslPeriod,
                             otShortAddress      aShortAddr,
                             const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddr);
    OT_UNUSED_VARIABLE(aExtAddr);

    assert(aCslPeriod < UINT16_MAX);
    sRadioContext.mCslPeriod = (uint16_t)aCslPeriod;

    return OT_ERROR_NONE;
}

otError otPlatRadioResetCsl(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sRadioContext.mCslPeriod = 0;

    return OT_ERROR_NONE;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
    OT_UNUSED_VARIABLE(aInstance);

    sRadioContext.mCslSampleTime = aCslSampleTime;
}

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return 80;
    //return otPlatTimeGetXtalAccuracy() / 2;
}

uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return CSL_UNCERT;
}

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
otError otPlatRadioConfigureEnhAckProbing(otInstance          *aInstance,
                                          otLinkMetrics        aLinkMetrics,
                                          const otShortAddress aShortAddress,
                                          const otExtAddress  *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return otLinkMetricsConfigureEnhAckProbing(aShortAddress, aExtAddress, aLinkMetrics);
}
#endif

otError otPlatRadioSetChannelMaxTransmitPower(otInstance *aInstance, uint8_t aChannel, int8_t aMaxPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aChannel >= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN && aChannel <= OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
            error = OT_ERROR_INVALID_ARGS);

    sMaxTxPowerTable[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN] = aMaxPower;
#if 0
    if (aChannel == bk_802154_channel_get())
    {
        bk_802154_tx_power_set(GetTransmitPowerForChannel(aChannel));
    }
#endif

exit:
    return error;
}

int8_t bkGetChannelMaxTransmitPower(uint8_t aChannel)
{
    int8_t power;

    if (aChannel < OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN || aChannel > OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX)
    {
        power = OT_RADIO_POWER_INVALID;
    }
    else
    {
        power = sMaxTxPowerTable[aChannel - OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN];
    }

    return power;
}

otError otPlatRadioSetRegion(otInstance *aInstance, uint16_t aRegionCode)
{
    OT_UNUSED_VARIABLE(aInstance);

    sRegionCode = aRegionCode;
    return OT_ERROR_NONE;
}

otError otPlatRadioGetRegion(otInstance *aInstance, uint16_t *aRegionCode)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aRegionCode != NULL, error = OT_ERROR_INVALID_ARGS);

    *aRegionCode = sRegionCode;
exit:
    return error;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
static uint8_t generateAckIeData(uint8_t *aLinkMetricsIeData, uint8_t aLinkMetricsIeDataLen)
{
    OT_UNUSED_VARIABLE(aLinkMetricsIeData);
    OT_UNUSED_VARIABLE(aLinkMetricsIeDataLen);

    uint8_t offset = 0;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (sRadioContext.mCslPeriod > 0)
    {
        offset += otMacFrameGenerateCslIeTemplate(sAckIeData);
    }
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    if (aLinkMetricsIeData != NULL && aLinkMetricsIeDataLen > 0)
    {
        offset += otMacFrameGenerateEnhAckProbingIe(sAckIeData, aLinkMetricsIeData, aLinkMetricsIeDataLen);
    }
#endif

    return offset;
}
#endif

static bool hasFramePending(const otRadioFrame *aFrame)
{
#if 0
    bool         rval = false;
    otMacAddress src;

    otEXPECT_ACTION(sSrcMatchEnabled, rval = true);
    otEXPECT(otMacFrameGetSrcAddr(aFrame, &src) == OT_ERROR_NONE);

    switch (src.mType)
    {
    case OT_MAC_ADDRESS_TYPE_SHORT:
        rval = utilsSoftSrcMatchShortFindEntry(src.mAddress.mShortAddress) >= 0;
        break;
    case OT_MAC_ADDRESS_TYPE_EXTENDED:
    {
        otExtAddress extAddr;

        ReverseExtAddress(&extAddr, &src.mAddress.mExtAddress);
        rval = utilsSoftSrcMatchExtFindEntry(&extAddr) >= 0;
        break;
    }
    default:
        break;
    }

exit:
    return rval;
#endif
    return false;

}

static void convert_frame_to_ot(otRadioFrame* pReceivedFrame, bk_802154_frame_t *data_p)
{
    pReceivedFrame->mPsdu               = &data_p->frame[1];
    pReceivedFrame->mLength             = data_p->frame[0];
    pReceivedFrame->mInfo.mRxInfo.mRssi = data_p->rssi;
    pReceivedFrame->mInfo.mRxInfo.mLqi  = data_p->lqi;
    pReceivedFrame->mChannel            = bk_ieee802154_channel_get();

    pReceivedFrame->mInfo.mRxInfo.mAckedWithFramePending = data_p->pending;
    pReceivedFrame->mInfo.mRxInfo.mAckedWithSecEnhAck    = false;

    pReceivedFrame->mInfo.mRxInfo.mTimestamp = data_p->time;
}

uint8_t* bk_802154_send_enh_ack(bk_802154_frame_t *data_p)
{
    static otRadioFrame ReceivedFrame;
    convert_frame_to_ot(&ReceivedFrame, data_p);
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    // Use enh-ack for 802.15.4-2015 frames
    if (otMacFrameIsVersion2015(&ReceivedFrame))
    {
        uint8_t  linkMetricsDataLen = 0;
        uint8_t *dataPtr            = NULL;

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        uint8_t      linkMetricsData[OT_ENH_PROBING_IE_DATA_MAX_SIZE];
        otMacAddress macAddress;

        otEXPECT(otMacFrameGetSrcAddr(&ReceivedFrame, &macAddress) == OT_ERROR_NONE);

        linkMetricsDataLen = otLinkMetricsEnhAckGenData(&macAddress, ReceivedFrame.mInfo.mRxInfo.mLqi,
                                                        ReceivedFrame.mInfo.mRxInfo.mRssi, linkMetricsData);

        if (linkMetricsDataLen > 0)
        {
            dataPtr = linkMetricsData;
        }
#endif

        sAckIeDataLength = generateAckIeData(dataPtr, linkMetricsDataLen);

        otEXPECT(otMacFrameGenerateEnhAck(&ReceivedFrame, ReceivedFrame.mInfo.mRxInfo.mAckedWithFramePending,
                                          sAckIeData, sAckIeDataLength, &sAckFrame) == OT_ERROR_NONE);
        otEXPECT(otMacFrameProcessTxSfd(&sAckFrame, otPlatTimeGet() + BK_SFD_TIME_OFFSET, &sRadioContext) == OT_ERROR_NONE);

        sAckFrame.mPsdu[-1] = sAckFrame.mLength;
        sAckFrame.mChannel = ReceivedFrame.mChannel;
        return &sAckFrame.mPsdu[-1];
    }
#endif
exit:
    otLogCritPlat("Error, generate Enh_ACK");
    return NULL;
}

