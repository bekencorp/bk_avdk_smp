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

#include "sdkconfig.h"
#include <stdint.h>

#if CONFIG_ARMV8_M_MAINLINE
#include "armv8m_reg.h"
#endif
#include "cache.h"
#if CONFIG_L2_CACHE_ENABLE
#include "l2_cache.h"
#endif
#include <os/os.h>

#include "bk_arch.h"

__STATIC_INLINE void SCB_EnableOnlyICache (void)
{
  #if defined (__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
    __DSB();
    __ISB();
    SCB->CCR |=  (uint32_t)SCB_CCR_IC_Msk;  /* enable I-Cache */
    __DSB();
    __ISB();
  #endif
}

__STATIC_FORCEINLINE void SCB_EnableOnlyDCache (void)
{
  #if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (SCB->CCR & SCB_CCR_DC_Msk) return;  /* return if DCache is already enabled */

    SCB->CSSELR = 0U;                       /* select Level 1 data cache */
    __DSB();

    SCB->CCR |=  (uint32_t)SCB_CCR_DC_Msk;  /* enable D-Cache */

    __DSB();
    __ISB();
  #endif
}







/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
__FLASH_BOOT_CODE void b_system_base_init (void)
{
#if (!(CONFIG_SUPPORT_ARCH_UNALIGNED))
	SYS_CTRL_BLK->CCR |= SCB_CCR_UNALIGN_TRP_MSK;
#endif

#if CONFIG_SPE
    /* secureFault enable*/
	SYS_CTRL_BLK->SHCSR |= SCB_SHCSR_SECUREFAULTENA_MSK;
#endif

	/* enable system fault exception*/
	SYS_CTRL_BLK->SHCSR |= SCB_SHCSR_MEMFAULTENA_MSK;
	SYS_CTRL_BLK->SHCSR |= SCB_SHCSR_USGFAULTENA_MSK;
	SYS_CTRL_BLK->SHCSR |= SCB_SHCSR_BUSFAULTENA_MSK;

	/* enable div_0_trp*/
	SYS_CTRL_BLK->CCR |= SCB_CCR_DIV_0_TRP_MSK;

	/* disable unalign_trp */
	SYS_CTRL_BLK->CCR &= (~SCB_CCR_UNALIGN_TRP_MSK);
	

}
// eof

