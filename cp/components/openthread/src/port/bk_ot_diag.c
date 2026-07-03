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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/link.h>
//#include <platform-config.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
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

#include "driver/gpio.h"
#include "bk_private/interrupt_base.h"
// #include "bk_sensor_internal.h"
#include "temp_detect_pub.h"

typedef enum
{
    kDiagTransmitModeIdle,
    kDiagTransmitModePackets,
    kDiagTransmitModeCarrier,
}DiagTrasmitMode;

struct PlatformDiagCommand
{
    const char *mName;
    otError (*mCommand)(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
};

struct PlatformDiagMessage
{
    const char mMessageDescriptor[11];
    uint8_t mChannel;
    int16_t mID;
    uint32_t mCnt;
};

extern void bkRadioClearPendingEvents(void);

static bool                     sDiagMode               = false;
static uint8_t                  sChannel                = 20;
static DiagTrasmitMode          sTransmitMode           = kDiagTransmitModeIdle;
static uint8_t                  sTxPower                = 0;
static bool                     sListen                 = false;
static int16_t                  sID                     = -1;
static otPlatDiagOutputCallback sDiagOutputCallback     = NULL;
static void                     *sDiagCallbackContext   = NULL;
static int32_t                  sTxCount                = 0;
static struct PlatformDiagMessage sDiagMessage          = {.mMessageDescriptor = "DiagMessage",
                                                            .mChannel           = 0,
                                                            .mID                = 0,
                                                            .mCnt               = 0};
static uint32_t                 sTxPeriod               = 1;
static int32_t                  sTxRequestedCount       = 1;

static void diagOutput(const char *aFormat, ...)
{
    va_list args;
    
    va_start(args, aFormat);

    if(sDiagOutputCallback != NULL)
    {
        sDiagOutputCallback(aFormat, args, sDiagCallbackContext);
    }

    va_end(args);
}

static bool startCarrierTransmission(void)
{
    bk_ieee802154_channel_set(sChannel);
    bk_ieee802154_set_txpower(sTxPower);

    //ToDo 
    return false;//nrf_802154_continuous_carrier();//???????????????????????????????????????????????
}
bool otPlatDiagModeGet(void)
{
    return sDiagMode;
}

void otPlatDiagModeSet(bool aMode)
{
    sDiagMode = aMode;

    if(!sDiagMode)
    {
        otPlatRadioReceive(NULL, sChannel);
        otPlatRadioSleep(NULL);

        // Clear all remaining events before switching to MAC callbacks.
        bkRadioClearPendingEvents();
    }
    else
    {
        //Reinit
        sTransmitMode = kDiagTransmitModeIdle;
    }
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    sChannel = aChannel;
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
    sTxPower = aTxPower;
}

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);

    if(sListen && (aError == OT_ERROR_NONE))
    {
        if(aFrame->mLength == sizeof(struct PlatformDiagMessage))
        {
            struct PlatformDiagMessage *message = (struct PlatformDiagMessage *)aFrame->mPsdu;
            
            if(strncmp(message->mMessageDescriptor, "DiagMessage", 11)==0)
            {
                otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_PLATFORM,
                        "{\"Frame\":{"
                        "\"LocalChannel\":%u,"
                        "\"RemoteChannel\":%u,"
                        "\"CNT\":%" PRIu32 ","
                        "\"LocalID\":%" PRId16","
                        "\"RemoteID\":%" PRId16 ","
                        "\"RSSI\":%d"
                        "}}\r\n",
                        aFrame->mChannel, message->mChannel, message->mCnt, sID, message->mID,
                        aFrame->mInfo.mRxInfo.mRssi);
            }
        }
    }
}

void otPlatDiagSetOutputCallback(otInstance *aInstance, otPlatDiagOutputCallback aCallback, void *aContext)
{
    OT_UNUSED_VARIABLE(aInstance);
    sDiagOutputCallback = aCallback;
    sDiagCallbackContext = aContext;
}

otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    if(aEnable)
    {
        otEXPECT_ACTION(startCarrierTransmission(), error = OT_ERROR_FAILED);
    }
    else
    {
        otPlatRadioReceive(aInstance, sChannel);
    }
exit:
    return error;    
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
    if(sTransmitMode == kDiagTransmitModePackets)
    {
        if((sTxCount > 0) || (sTxCount == -1))
        {
            otRadioFrame *sTxPacket = otPlatRadioGetTransmitBuffer(aInstance);

            sTxPacket->mLength = sizeof(struct PlatformDiagMessage);
            sTxPacket->mChannel= sChannel;

            sDiagMessage.mChannel = sTxPacket->mChannel;
            sDiagMessage.mID = sID;

            memcpy(sTxPacket->mPsdu, &sDiagMessage, sizeof(struct PlatformDiagMessage));
            otPlatRadioTransmit(aInstance, sTxPacket);

            sDiagMessage.mCnt++;

            if(sTxCount != -1)
            {
                sTxCount--;
            }

            uint32_t now = otPlatAlarmMilliGetNow();
            otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);

        }
        else
        {
            sTransmitMode = kDiagTransmitModeIdle;
            otPlatAlarmMilliStop(aInstance);
            otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_PLATFORM, "Transmit done");
        }
    }
}

otError otPlatDiagGpioSet(uint32_t aGpio, bool aValue)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(aGpio<SOC_GPIO_NUM, error= OT_ERROR_INVALID_ARGS);
    bk_gpio_enable_output(aGpio);
    if(aValue)
        bk_gpio_set_output_high(aGpio);
    else
        bk_gpio_set_output_low(aGpio);
exit:
    return error;    
}

otError otPlatDiagGpioGet(uint32_t aGpio, bool *aValue)
{
    otError error = OT_ERROR_NONE;
    uint32_t uConfig=0;
    
    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION((aValue !=NULL) &&(aGpio<SOC_GPIO_NUM), error = OT_ERROR_INVALID_ARGS);
    uConfig = bk_gpio_get_value(aGpio);
    if(uConfig & 0x4)
        *aValue = (bool)bk_gpio_get_input(aGpio);
    else
        *aValue = (bool)bk_gpio_get_output(aGpio);
exit:
    return error;        
}

otError otPlatDiagGpioSetMode(uint32_t aGpio, otGpioMode aMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(aGpio<SOC_GPIO_NUM, error = OT_ERROR_INVALID_ARGS);

    switch(aMode)
    {
        case OT_GPIO_MODE_INPUT:
            bk_gpio_disable_output(aGpio);
            bk_gpio_enable_input(aGpio);
            break;
        case OT_GPIO_MODE_OUTPUT:
            bk_gpio_disable_input(aGpio);
            bk_gpio_enable_output(aGpio);
            break;
        default:
            error = OT_ERROR_INVALID_ARGS;
    }
exit:
    return error;    
}

otError otPlatDiagGpioGetMode(uint32_t aGpio, otGpioMode *aMode)
{
    otError error = OT_ERROR_NONE;
    uint32_t uReg=0;

    otEXPECT_ACTION(otPlatDiagModeGet(), error= OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(((aMode !=NULL)&&(aGpio<SOC_GPIO_NUM)), error = OT_ERROR_INVALID_ARGS);

    uReg = bk_gpio_get_value(aGpio);

    if(uReg & 0x4)
    {
        *aMode = OT_GPIO_MODE_INPUT;
    }
    else
    {
        *aMode = OT_GPIO_MODE_OUTPUT;
    }
exit:
    return error;    
}

static otError parseLong(char *aArgs, long *aValue)
{
    char *endptr;
    *aValue = strtol(aArgs, &endptr, 0);
    return (*endptr == '\0')? OT_ERROR_NONE:OT_ERROR_PARSE;
}

static void appendErrorResult(otError aError)
{
    if(aError != OT_ERROR_NONE)
    {
        diagOutput("failed\r\nstatus %#x\r\n", aError);
    }
}
static otError processListen(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if(aArgsLength == 0)
    {
        diagOutput("listen: %s\r\n", sListen == true? "yes":"no");
    }
    else
    {
        long value;
        error = parseLong(aArgs[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        sListen = (bool)(value);
        diagOutput("set listen to %s\r\nstatus 0x%02x\r\n", sListen==true? "yes":"no", error);
    }
exit:
    appendErrorResult(error);
    return error;
}

static otError processID(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if(aArgsLength == 0)
    {
        diagOutput("ID: %" PRId16 "\r\n", sID);
    }
    else
    {
        long value;

        error = parseLong(aArgs[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value>=0, error=OT_ERROR_INVALID_ARGS);
        sID=(uint16_t)(value);
        diagOutput("set ID to %" PRId16 "\r\nstatus 0x%02x\r\n",sID, error);
    }

exit:
    appendErrorResult(error);
    return error;
}

static otError processTransmit(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if(aArgsLength == 0)
    {
        diagOutput("transmit will send %" PRId32 " diagnostic messages with %" PRIu32
                    " ms interval\r\nstatus 0x%02x\r\n",
                    sTxRequestedCount, sTxPeriod, error);
    }
    else if(strcmp(aArgs[0], "stop")==0)
    {
        otEXPECT_ACTION(sTransmitMode != kDiagTransmitModeIdle, error = OT_ERROR_INVALID_STATE);

        otPlatAlarmMilliStop(aInstance);
        diagOutput("diagnostic message transmission is stopped\r\nstatus 0x%02x\r\n", error);
        sTransmitMode = kDiagTransmitModeIdle;
        otPlatRadioReceive(aInstance, sChannel);
    }
    else if(strcmp(aArgs[0], "start")==0)
    {
        otEXPECT_ACTION(sTransmitMode == kDiagTransmitModeIdle, error = OT_ERROR_INVALID_STATE);

        otPlatAlarmMilliStop(aInstance);
        sTransmitMode = kDiagTransmitModePackets;
        sTxCount = sTxRequestedCount;
        uint32_t now = otPlatAlarmMilliGetNow();
        otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);
        diagOutput("sending %" PRId32 " diagnostic messages with %" PRIu32 " ms interval\r\nstatus 0x%02x\r\n",sTxRequestedCount, sTxPeriod, error);
    }
    else if(strcmp(aArgs[0], "carrier")==0)
    {
        otEXPECT_ACTION(sTransmitMode==kDiagTransmitModeIdle, error = OT_ERROR_INVALID_STATE);
        otEXPECT_ACTION(startCarrierTransmission(), error = OT_ERROR_FAILED);

        sTransmitMode = kDiagTransmitModeCarrier;
        diagOutput("Sending carrier on channel %d with tx power %d\r\nstatus 0x%02x\r\n",sChannel, sTxPower, error);
    }
    else if(strcmp(aArgs[0], "interval")==0)
    {
        long value;

        otEXPECT_ACTION(aArgsLength ==2, error = OT_ERROR_INVALID_ARGS);
        error=parseLong(aArgs[1], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value > 0, error=OT_ERROR_INVALID_ARGS);
        sTxPeriod = (uint32_t)(value);
        diagOutput("set diagnostic messages interval to %" PRIu32 " ms\r\nstatus 0x%02x\r\n", sTxPeriod, error);
    }
    else if(strcmp(aArgs[0], "count")==0)
    {
        long value;
        otEXPECT_ACTION(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
        error = parseLong(aArgs[1], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION((value>0)||(value== -1), error = OT_ERROR_INVALID_ARGS);
        sTxRequestedCount=(uint32_t)(value);
        diagOutput("set diagnostic messages count to %" PRId32 "\r\nstatus 0x%02x\r\n", sTxRequestedCount, error);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    appendErrorResult(error);
    return error;

}

static otError processTemp(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;
    uint32_t temperature;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(aArgsLength== 0, error = OT_ERROR_INVALID_ARGS);

    temp_detect_get_temperature(&temperature);
    diagOutput("%" PRId32 "\r\n",temperature);

exit:
    appendErrorResult(error);
    return error;
}

static otError processCcaThreshold(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error= OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if(aArgsLength ==0)
    {
        diagOutput("cca threshold: %u\r\n", bk_ieee802154_get_cca_threshold());
    }
    else
    {
        long value;
        error = parseLong(aArgs[0],&value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value >= 0 && value<= 0xFF, error = OT_ERROR_INVALID_ARGS);
        bk_ieee802154_set_cca_threshold(value);
        diagOutput("set cca threshold to %u\r\nstatus 0x%02x\r\n",bk_ieee802154_get_cca_threshold(),error);
    }

exit:
    appendErrorResult(error);
    return error;
}
const struct PlatformDiagCommand sCommands[]={{"ccathreshold", &processCcaThreshold},
                                            {"id", &processID},
                                            {"listen", &processListen},
                                            {"temp", &processTemp},
                                            {"transmit", &processTransmit}};

otError otPlatDiagProcess(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;
    size_t i;

    for(i=0; i< otARRAY_LENGTH(sCommands);i++)
    {
        if(strcmp(aArgs[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(aInstance, aArgsLength-1, aArgsLength>1? &aArgs[1]:NULL);
            break;
        }
    }
    return error;
}