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

#include <stdio.h>
#include <stdarg.h>
#include "sdkconfig.h"
#include "components/log.h"
#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/platform/logging.h>
#include "components/system.h"
#include <openthread/logging.h>


static otLogLevel sUserSetLogLevel;
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) ||\
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)

#define OT_LOG_PLAT_TAG   "OT_PLT"
extern void bk_printf_port_ext(int level, char *tag, const char *fmt, va_list args);
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list args;
    va_start(args,aFormat);
    sUserSetLogLevel=otLoggingGetLevel();

    switch(aLogLevel)
    {
        case OT_LOG_LEVEL_CRIT:
            if(sUserSetLogLevel >= OT_LOG_LEVEL_CRIT)
            {
                bk_printf_port_ext(BK_LOG_ERROR,OT_LOG_PLAT_TAG,aFormat,args);
                bk_printf_raw(BK_LOG_ERROR,OT_LOG_PLAT_TAG,"%s","\n");
            }
            break;
        case OT_LOG_LEVEL_WARN:
            if(sUserSetLogLevel >= OT_LOG_LEVEL_WARN)
            {
                bk_printf_port_ext(BK_LOG_WARN,OT_LOG_PLAT_TAG,aFormat,args);
                bk_printf_raw(BK_LOG_WARN,OT_LOG_PLAT_TAG,"%s","\n");
            }
            break;
        case OT_LOG_LEVEL_NOTE:
        case OT_LOG_LEVEL_INFO:
            if(sUserSetLogLevel >= OT_LOG_LEVEL_INFO)
            {
                bk_printf_port_ext(BK_LOG_INFO,OT_LOG_PLAT_TAG,aFormat,args);
                bk_printf_raw(BK_LOG_INFO,OT_LOG_PLAT_TAG,"%s","\n");
            }
            break;
        default:
            if(sUserSetLogLevel >= OT_LOG_LEVEL_DEBG)
            {
                bk_printf_port_ext(BK_LOG_DEBUG,OT_LOG_PLAT_TAG,aFormat,args);
                bk_printf_raw(BK_LOG_DEBUG,OT_LOG_PLAT_TAG,"%s","\n");
            }
            break;
    }
    va_end(args);
}
#endif
#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
void otPlatLogHandleLevelChanged(otLogLevel aLogLevel)
{
    sUserSetLogLevel=aLogLevel;
}
#endif