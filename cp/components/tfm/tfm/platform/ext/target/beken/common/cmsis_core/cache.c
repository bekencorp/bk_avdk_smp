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

#include "platform_irq.h"
#include "STAR_SE.h"
#include "core_star.h"
#include "sdkconfig.h"

#define L2C_MAINT_CTRL_ALL_REG  (*(volatile uint32_t *)0xE0060020U)
#define L2C_SECIRQSTAT_REG      (*(volatile uint32_t *)0xE0060100U)
#define L2C_SECIRQCLR_REG       (*(volatile uint32_t *)0xE0060104U)
#define L2C_REQ_DONE            (1U << 2)
#define L2C_REQ_ERROR           (1U << 3)
#define L2C_INVALID_ALL         2U
#define L2C_CLEAN_INVALID_ALL   3U

static int maintain_all_l2cache(uint32_t operation)
{
    uint32_t timeout = 1000000U;
    uint32_t status;

    L2C_SECIRQCLR_REG = L2C_REQ_DONE;
    L2C_MAINT_CTRL_ALL_REG = operation;
    __DSB();
    __ISB();
    while (timeout--) {
        status = L2C_SECIRQSTAT_REG;
        if (status & L2C_REQ_DONE) {
            L2C_SECIRQCLR_REG = L2C_REQ_DONE;
            __DSB();
            __ISB();
            return (status & L2C_REQ_ERROR) ? -1 : 0;
        }
    }
    return -1;
}


void sram_dcache_map(void)
{
}

int show_cache_config_info(void)
{
	return 0;
}

void flush_dcache(void *va, long size)
{
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk) {
        SCB_CleanInvalidateDCache_by_Addr(va, size);
    }
}

int is_scb_dcache_enabled(void)
{
    return (SCB->CCR & SCB_CCR_DC_Msk) != 0;
}

void disable_scb_dcache(void)
{
    SCB_DisableDCache();
}

void enable_scb_dcache(void)
{
    SCB_EnableDCache();
}

void flush_invalidate_dcache(void)
{
    SCB_InvalidateDCache();
}

void flush_all_dcache(void)
{
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk) {
        SCB_CleanInvalidateDCache();
    }
    (void)maintain_all_l2cache(L2C_CLEAN_INVALID_ALL);
}

int arch_dcache_invd_all(void)
{
    if (SCB->CLIDR & SCB_CLIDR_DC_Msk) {
        SCB_InvalidateDCache();
    }
    return maintain_all_l2cache(L2C_INVALID_ALL);
}

void enable_dcache(int enable)
{
    if (enable == 0) {
        SCB_DisableDCache();
#if CONFIG_MPU
        mpu_disable();
#endif
    } else {
#if CONFIG_MPU
        mpu_enable();
#endif
        SCB_EnableDCache();
        SCB_CleanInvalidateDCache();
    }
}

void invalidate_icache(void)
{
    SCB_InvalidateICache();
}
// eof

