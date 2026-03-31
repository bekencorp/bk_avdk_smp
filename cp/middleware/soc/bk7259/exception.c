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
#include "cmsis_compiler.h"
#include "components/log.h"
#include "components/system.h"
#include "arch_interrupt.h"
#include "aon_pmu_driver.h"
#include "bk_pm_internal_api.h"
#include <modules/pm.h>
#include <reset_reason.h>
#include "bk_aon_wdt.h"

#if CONFIG_WDT_EN
#include "wdt_driver.h"
#include <driver/wdt.h>
#endif

void bk_exception_handler(uint32_t reset_reason, uint32_t lr, uint32_t sp);

__STATIC_FORCEINLINE void dump_system_info(uint32_t rr, uint32_t lr, uint32_t sp) {
#if (CONFIG_SWD_DEBUG_MODE)
	volatile uint32_t g_test18 = 1;
	while (g_test18);
#endif
	bk_exception_handler(rr, lr, sp);
}

#define dump_fault_info(rr) \
{\
	uint32_t lr = __get_LR();\
	uint32_t sp = __get_MSP();\
\
    __asm volatile \
    (\
        "	push {r4-r11}								\n"\
    );\
\
	dump_system_info(rr, lr, sp);\
\
	while(1);\
}

void user_nmi_handler(uint32_t lr, uint32_t sp)
{
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
	if(arch_is_enter_exception())
	{
		//For nmi wdt reset
		aon_pmu_drv_wdt_change_not_rosc_clk();
		aon_pmu_drv_wdt_rst_dev_enable();
		while(1);
	}

	if(reboot_tag_is_reboot()) {
		while(1);
	}

#if CONFIG_WDT_EN
	bk_wdt_feed();
#endif

	dump_system_info(RESET_SOURCE_NMI_WDT, lr, sp);
#else // nmi wdt without system info dump
	if(!(reboot_tag_is_reboot())) {
		bk_misc_set_reset_reason(RESET_SOURCE_NMI_WDT);
	}

	aon_pmu_drv_wdt_change_not_rosc_clk();
	aon_pmu_drv_wdt_rst_dev_enable();
	while(1){
		;
	}
#endif // CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
}

__NAKED __attribute__((section(".itcm"))) void soc_hardfault_handler(void)
{
	dump_fault_info(RESET_SOURCE_HARD_FAULT);
}

__NAKED void soc_nmi_handler(void)
{
	uint32_t lr = __get_LR();
	uint32_t sp = __get_MSP();

    __asm volatile
    (
        "	push {r4-r11}								\n"
    );

	user_nmi_handler(lr, sp);

	while(1);}

__NAKED void soc_memmanage_handler(void)
{
	dump_fault_info(RESET_SOURCE_MPU_FAULT);
}

__NAKED void soc_busfault_handler(void)
{
	dump_fault_info(RESET_SOURCE_BUS_FAULT);
}

__NAKED void soc_usagefault_handler(void)
{
	dump_fault_info(RESET_SOURCE_USAGE_FAULT);
}

__NAKED void soc_securefault_handler(void)
{
	dump_fault_info(RESET_SOURCE_SECURE_FAULT);
}

#if CONFIG_SOC_CORTEX_M_UART_DEBUG
/* debug_monitor_exception.c*/
#include "debug_monitor_exception.h"

void debug_monitor_handler_c(CONTEXT_FRAME_T *frame);

__NAKED void soc_debugmon_handler(void)
{
  __asm volatile(
      "tst lr, #4 \n"
      "ite eq \n"
      "mrseq r0, msp \n"
      "mrsne r0, psp \n"
      "b debug_monitor_handler_c \n");
}
#else
__NAKED void soc_debugmon_handler(void)
{
	dump_fault_info(RESET_SOURCE_DEBUG_MONITOR_FAULT);
}
#endif // CONFIG_SOC_CORTEX_M_UART_DEBUG

__NAKED void soc_default_handler(void)
{
	dump_fault_info(RESET_SOURCE_DEFAULT_EXCEPTION);
}