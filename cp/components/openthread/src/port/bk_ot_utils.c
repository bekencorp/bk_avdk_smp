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

#include <stdint.h>
#include "sys_rtos.h"
#include "os/os.h"
#include "os/mem.h"

#include <openthread/instance.h>
#include <openthread/error.h>
#include <openthread/logging.h>
#include <platform/toolchain.h>
#include <platform/radio.h>
#include "components/system.h"
#include "wdt/wdt_driver.h"
#include "driver/wdt.h"

OT_TOOL_WEAK void otPlatReset(otInstance *aInstance)
{
#if !CONFIG_SUPPORT_MATTER
    bk_reboot();
#endif
}

void otPlatAssertFail(const char *aFilename, int aLineNumber)
{
    otLogCritPlat("assert failed at %s:%d", aFilename, aLineNumber);
}

// OT_TOOL_WEAK otError otPlatMultipanGetActiveInstance(otInstance *aInstance*) { return OT_ERROR_NOT_IMPLEMENTED; }

// OT_TOOL_WEAK otError otPlatMultipanSetActiveInstance(otInstance *aInstance, bool) { return OT_ERROR_NOT_IMPLEMENTED; }
#if 0
OT_TOOL_WEAK void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64) {}

OT_TOOL_WEAK void otPlatRadioSetPanId(otInstance *aInstance, otPanId aPanId) {}

OT_TOOL_WEAK void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress) {}

OT_TOOL_WEAK void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aShortAddress) {}

OT_TOOL_WEAK void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnabled) {}

// OT_TOOL_WEAK void otPlatRadioSetRxOnWhenIdle(otInstance *aInstance, bool aEnable) {}

// OT_TOOL_WEAK bool otPlatRadioIsEnabled(otInstance *aInstance) { return true; }

OT_TOOL_WEAK otError otPlatRadioEnable(otInstance *aInstance) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioDisable(otInstance *aInstance) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioSleep(otInstance *aInstance) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance) { return NULL; }

OT_TOOL_WEAK int8_t otPlatRadioGetRssi(otInstance *aInstance) { return 0; }

OT_TOOL_WEAK otRadioCaps otPlatRadioGetCaps(otInstance *aInstance) { return OT_RADIO_CAPS_NONE; }

OT_TOOL_WEAK bool otPlatRadioGetPromiscuous(otInstance *aInstance) { return false; }

OT_TOOL_WEAK void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable) {}

OT_TOOL_WEAK otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress) { return OT_ERROR_NONE; }

OT_TOOL_WEAK otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress) { return OT_ERROR_NONE; }

OT_TOOL_WEAK void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance) {}

OT_TOOL_WEAK void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance) {}

OT_TOOL_WEAK otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aU8, uint16_t aU16) { return OT_ERROR_NOT_IMPLEMENTED; }

OT_TOOL_WEAK otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower) { return OT_ERROR_NOT_IMPLEMENTED; }

OT_TOOL_WEAK int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance) { return -100; }
OT_TOOL_WEAK otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)  { return OT_ERROR_NONE; }
OT_TOOL_WEAK otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower) { return OT_ERROR_NONE; }
OT_TOOL_WEAK otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);

    return OT_ERROR_NONE;
}
//OT_TOOL_WEAK void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...) {}
#endif


void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    return psram_zalloc(aNum * aSize);
}
void otPlatFree(void *aPtr)
{
    psram_free(aPtr);
}

