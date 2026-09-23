// Copyright     2023-2028 Beken
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

#include <soc/soc.h>
#include "wdt.h"
#include "bk_wdt.h"

void update_wdt(uint32_t val)
{
        /* Mirror the bk7259 bringup wdt_driver.c wdt_time_set(): program the WDT
         * reset-config, then stop BOTH the AON WDT (0x44000600 + 0x0) and the
         * CPU window watchdog (0xE0050010) with the 0x5A0000/0xA50000 unlock
         * pair. The previous code used the wrong AON offset (+0x8) and never
         * touched the CPU WWDT, so the ~1s WWDT kept resetting BL2. */
        REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2 * 4,
                  (REG_READ(SOC_AON_PMU_REG_BASE + 0x2 * 4) & ~0x7u) | 0x7u);
        REG_WRITE(SOC_AON_WDT_REG_BASE + 0x0, 0x5A0000 | val);
        REG_WRITE(SOC_AON_WDT_REG_BASE + 0x0, 0xA50000 | val);
        REG_WRITE(0xE0050010, 0x5A0000 | val);
        REG_WRITE(0xE0050010, 0xA50000 | val);
}

void close_wdt(void)
{
        update_wdt(0);
}

void update_aon_wdt(uint32_t val)
{
#if CONFIG_SUPPORT_SWD_DEBUG
        if(val){
                return;
        }
#endif
}

void close_aon_wdt(void)
{
        update_aon_wdt(0);
}

