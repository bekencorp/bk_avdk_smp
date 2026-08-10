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
#include "soc/reg_base.h"
#include "soc/soc.h"
#include "bk_arch.h"
#include <stdint.h>
#include "soc_debug.h"
#include "dbg_probe.h"
/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __StackTopCore0;
extern uint32_t __StackLimitCore0;
extern uint32_t __vector_core0_table;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler_Core0(void);
void _default_handler_(void);
void dlv_hook(void);
__FLASH_BOOT_CODE void b_system_base_init (void);
__FLASH_BOOT_CODE void b_prep_entry_main(void);
void b_program_start(void);
typedef void(*VECTOR_ENTRY_TYPE)(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
void soc_nmi_handler(void) __attribute__ ((weak));
void soc_hardfault_handler(void) __attribute__ ((weak));
void soc_memmanage_handler(void) __attribute__ ((weak));
void soc_busfault_handler(void) __attribute__ ((weak));
void soc_usagefault_handler(void) __attribute__ ((weak));
void soc_securefault_handler(void) __attribute__ ((weak));
void soc_svc_handler(void) __attribute__ ((weak));
void soc_debugmon_handler(void) __attribute__ ((weak));
void soc_pendsv_handler(void) __attribute__ ((weak));
void soc_systick_handler(void) __attribute__ ((weak));
void soc_m55sub_handler(void) __attribute__ ((weak));

void NMI_Handler(void)
{
  soc_nmi_handler();
}

void HardFault_Handler(void)
{
  soc_hardfault_handler();
}

void MemManage_Handler(void)
{
  soc_memmanage_handler();
}

void BusFault_Handler(void)
{
  soc_busfault_handler();
}

void UsageFault_Handler(void)
{
  soc_usagefault_handler();
}

void SecureFault_Handler(void)
{
  soc_securefault_handler();
}

__attribute__((weak)) void SVC_Handler(void) 
{
  soc_svc_handler();
}

__attribute__((weak)) void DebugMon_Handler(void) 
{
  soc_debugmon_handler();
}

__attribute__((weak)) void PendSV_Handler(void) 
{
  soc_pendsv_handler();
}

__attribute__((weak)) void SysTick_Handler(void) 
{
  soc_systick_handler();
}

__attribute__((weak)) void M55sub_Handler(void) 
{
  soc_m55sub_handler();
}

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

extern const long __copy_table_start__;
extern const long __copy_table_end__;
extern const long __zero_table_start__;
extern const long __zero_table_end__;

__attribute__((used, section(".vectors_core0"))) \
const VECTOR_ENTRY_TYPE __VECTOR_TABLE_CORE0[] = {
  (VECTOR_ENTRY_TYPE)(&__StackTopCore0),    /*     Initial Stack Pointer */
  Reset_Handler_Core0,                      /*     Reset Handler */
  NMI_Handler,                              /* -14 NMI Handler */
  HardFault_Handler,                        /* -13 Hard Fault Handler */
  MemManage_Handler,                        /* -12 MPU Fault Handler */
  BusFault_Handler,                         /* -11 Bus Fault Handler */
  UsageFault_Handler,                       /* -10 Usage Fault Handler */
  SecureFault_Handler,                      /*  -9 Secure Fault Handler */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  SVC_Handler,                              /*  -5 SVCall Handler */
  DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
  0,                                        /*     Reserved */
  PendSV_Handler,                           /*  -2 PendSV Handler */
  SysTick_Handler,                          /*  -1 SysTick Handler */
  M55sub_Handler,                           /*  -0 M55sub Handler  */
  (VECTOR_ENTRY_TYPE)&__copy_table_start__,                     /*   AP  Copy Table Start */
  (VECTOR_ENTRY_TYPE)&__copy_table_end__,                       /*   AP  Copy Table End */
  (VECTOR_ENTRY_TYPE)&__zero_table_start__,                     /*   AP  Zero Table Start */
  (VECTOR_ENTRY_TYPE)&__zero_table_end__,                       /*   AP  Zero Table End */
};

#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
 #ifndef __STACK_LIMIT_CORE0
 #define __STACK_LIMIT_CORE0             __StackLimitCore0
 #endif

#define ENTRY_SECTION  __attribute__((naked, section(".fix.reset_entry")))

/* Records that the core0 reset handler was entered, for postmortem/hang
 * debugging (e.g. when a reboot stalls before the system is re-initialized). */
volatile uint32_t g_reset_entry_state_core0 = 0;

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN ENTRY_SECTION void Reset_Handler_Core0(void)
{
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
  /*
   * Resume before touching .data/.bss: CP has already restored AP SRAM/DTCM,
   * and normal C runtime initialization would destroy the suspended RTOS.
   */
  dlv_hook();
#endif

  g_reset_entry_state_core0 = 1;

  __set_MSPLIM((uint32_t)(&__STACK_LIMIT_CORE0));

  __disable_irq();

#if !CONFIG_SPE
  /* The CP does not pre-copy the AP image, so initialize .bss/.data/.dtcm here,
   * before b_system_base_init() which may rely on initialized data. */
  {
    extern void b_bss_zero(void);
    extern void b_data_copy(void);
    extern void b_data_copy_dtcm(void);
    b_bss_zero();
    b_data_copy();
    b_data_copy_dtcm();
  }
#endif

  dbg_probe_init();
  dbg_probe_early_stage(0u, 1);

  b_system_base_init();
  dbg_probe_early_stage(0u, 3);

  b_prep_entry_main();
  dbg_probe_early_stage(0u, 4);
  /* RAM (.data/.bss) is ready after b_prep_entry_main(); safe for the runtime
   * (re)init path. NULL => compile-time defaults. */
  dbg_probe_runtime_init(NULL);
  b_program_start();
}
// eof
