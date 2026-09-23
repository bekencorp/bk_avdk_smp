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

/* Arm the AON WDT alone (val in ms) and stop the CPU WWDT. The WWDT period is
 * 16-bit at 32kHz, so it caps at ~2s, too short for a 64KB block erase at its
 * datasheet maximum - flash_ll_wait_op_done() spins with no feed point inside.
 * The OTA install runs under this mode; update_wdt() restores both.
 *
 * Measured 4% long (0xFFFF -> 67.2s AON, 2.10s WWDT), so the ms unit comes off
 * an internal RC near 31.5kHz and drifts with temperature.
 *
 * val is capped at 16 bits: the key sits at bits[23:16] of the same register. */
void update_wdt_aon_only(uint32_t val)
{
        val &= 0xFFFFu;

        REG_WRITE(SOC_AON_PMU_REG_BASE + 0x2 * 4,
                  (REG_READ(SOC_AON_PMU_REG_BASE + 0x2 * 4) & ~0x7u) | 0x7u);
        REG_WRITE(SOC_AON_WDT_REG_BASE + 0x0, 0x5A0000 | val);
        REG_WRITE(SOC_AON_WDT_REG_BASE + 0x0, 0xA50000 | val);
        REG_WRITE(0xE0050010, 0x5A0000);
        REG_WRITE(0xE0050010, 0xA50000);
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

