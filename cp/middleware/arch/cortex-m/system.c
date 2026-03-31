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

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void b_system_base_init (void)
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

#if CONFIG_SUPPORT_L1_CACHE
	unified_cache_enable_icache();
#elif (CONFIG_ICACHE)
	arch_icache_enable();
#endif
}
// eof

