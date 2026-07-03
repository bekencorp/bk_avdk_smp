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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os/os.h"


#include <openthread-core-bk7239n-config.h>
#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include "openthread-system.h"

#include "bk_ot_alarm.h"

extern void bkRadioInit(void); 
extern void bkRadioDeinit(void); 

extern void bkRadioProcess(otInstance *aInstance);

bool gPlatformPseudoResetWasRequested = false;

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    if(gPlatformPseudoResetWasRequested)
    {
        otSysDeinit();
    }

    bkRadioInit();

    gPlatformPseudoResetWasRequested=false;
}

void otSysDeinit(void)
{
    bkRadioDeinit();
}

bool otSysPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void otSysProcessDrivers(otInstance *aInstance)
{

    bk_ot_alarm_process(aInstance);
    bkRadioProcess(aInstance);
    bk_ot_alarm_process(aInstance);
}

void otSysEventSignalPending()
{}
