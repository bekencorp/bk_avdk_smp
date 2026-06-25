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

#if CONFIG_SUPPORT_WWDT
#include "wwdt_driver.h"
#endif
void bk_exception_handler(uint32_t reset_reason, uint32_t lr, uint32_t sp);

/* NMI / exception flow-state tracker for postmortem debugging. It is updated at
 * key milestones of the fault/NMI flow so that a debugger (or a later dump) can
 * tell how far the flow progressed - this is especially useful when the reboot
 * itself hangs and triggers a second watchdog event. */
typedef enum {
	BK_NMI_FLOW_NONE = 0,
	BK_NMI_FLOW_NMI_ENTER,
	BK_NMI_FLOW_SECONDARY,
	BK_NMI_FLOW_FEED_WDT,
	BK_NMI_FLOW_DUMP_ENTER,
} bk_nmi_flow_state_t;
volatile uint32_t g_nmi_flow_state = BK_NMI_FLOW_NONE;

extern void bk_set_swd_mode(void);

__STATIC_FORCEINLINE void dump_system_info(uint32_t rr, uint32_t lr, uint32_t sp) {
#if (CONFIG_SWD_DEBUG_MODE)
	volatile uint32_t g_test18 = 1;
	bk_set_swd_mode();
	while (g_test18);
#endif
	g_nmi_flow_state = BK_NMI_FLOW_DUMP_ENTER;
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
	g_nmi_flow_state = BK_NMI_FLOW_NMI_ENTER;
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
	if(arch_is_enter_exception())
	{
		g_nmi_flow_state = BK_NMI_FLOW_SECONDARY;
		//For nmi wdt reset
		aon_pmu_drv_wdt_change_not_rosc_clk();
		aon_pmu_drv_wdt_rst_dev_enable();
		while(1);
	}

#if CONFIG_WDT_EN
	bk_wdt_force_feed();
#endif
#if CONFIG_SUPPORT_WWDT
	bk_wwdt_force_feed();
#endif
	g_nmi_flow_state = BK_NMI_FLOW_FEED_WDT;

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