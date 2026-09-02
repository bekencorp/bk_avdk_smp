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
#include "dbg_probe.h"
#include <stdint.h>

#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
#define CP_SEC_DUMP_ABI_INFO 0x53440130U
extern void bk_coredump_secure_fault_callback(void *context);
extern const uintptr_t g_cp_secure_fault_context_address;
#endif


/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __StackTopCore1;
extern uint32_t __StackLimitCore1;
extern uint32_t __vector_core1_table;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler_Core1(void);
void _default_handler_(void);
__FLASH_BOOT_CODE void b_system_base_init(void);
__FLASH_BOOT_CODE void b_prep_entry_main(void);
void b_program_start(void);
typedef void (*VECTOR_ENTRY_TYPE)(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
void soc_nmi_handler(void) __attribute__((weak));
void soc_hardfault_handler(void) __attribute__((weak));
void soc_memmanage_handler(void) __attribute__((weak));
void soc_busfault_handler(void) __attribute__((weak));
void soc_usagefault_handler(void) __attribute__((weak));
void soc_securefault_handler(void) __attribute__((weak));
void soc_svc_handler(void) __attribute__((weak));
void soc_debugmon_handler(void) __attribute__((weak));
void soc_pendsv_handler(void) __attribute__((weak));
void soc_systick_handler(void) __attribute__((weak));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#ifndef __VECTOR_SYS_ENTRY_CNT
#define __VECTOR_SYS_ENTRY_CNT (16)
#endif

__attribute__((used, section(".vectors_core1"))) const VECTOR_ENTRY_TYPE __VECTOR_TABLE_CORE1[__VECTOR_SYS_ENTRY_CNT + __INT_NUMBER_MAX] = {
    (VECTOR_ENTRY_TYPE)(&__StackTopCore1),   /*     Initial Stack Pointer */
    Reset_Handler_Core1,                     /*     Reset Handler */
    soc_nmi_handler,                        /* -14 NMI Handler */
    soc_hardfault_handler,                  /* -13 Hard Fault Handler */
    soc_memmanage_handler,                  /* -12 MPU Fault Handler */
    soc_busfault_handler,                   /* -11 Bus Fault Handler */
    soc_usagefault_handler,                 /* -10 Usage Fault Handler */
    soc_securefault_handler,                /*  -9 Secure Fault Handler */
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
    (VECTOR_ENTRY_TYPE)bk_coredump_secure_fault_callback,       /* Reserved: Secure dump callback */
    (VECTOR_ENTRY_TYPE)&g_cp_secure_fault_context_address,       /* Reserved: Secure dump context pointer */
    (VECTOR_ENTRY_TYPE)CP_SEC_DUMP_ABI_INFO,                     /* Reserved: Secure dump ABI */
#else
    0,                                      /*     Reserved */
    0,                                      /*     Reserved */
    0,                                      /*     Reserved */
#endif
    soc_svc_handler,                        /*  -5 SVCall Handler */
    soc_debugmon_handler,                   /*  -4 Debug Monitor Handler */
    0,                                      /*     Reserved */
    soc_pendsv_handler,                     /*  -2 PendSV Handler */
    soc_systick_handler,                    /*  -1 SysTick Handler */
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

uint32_t get_core1_vtor_addr(void)
{
    return (uint32_t)&(__VECTOR_TABLE_CORE1[0]);
}

#ifndef __STACK_LIMIT_CORE1
#define __STACK_LIMIT_CORE1 __StackLimitCore1
#endif

#define ENTRY_SECTION __attribute__((naked, section(".fix.reset_entry")))

/* Records that the core1 reset handler was entered, for postmortem/hang
 * debugging (e.g. when a reboot stalls before the system is re-initialized). */
volatile uint32_t g_reset_entry_state_core1 = 0;

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN ENTRY_SECTION void Reset_Handler_Core1(void)
{
    g_reset_entry_state_core1 = 1;
    __set_MSPLIM((uint32_t)(&__STACK_LIMIT_CORE1));

    __disable_irq();

    b_system_base_init();
    b_prep_entry_main();
    /* Ver4 SMP: core1 runs after core0 already initialized RAM, so the runtime
     * path is safe here. Brings up core1's own debug port (UART2/GPIO22) and
     * emits a per-core marker (0x52A6). */
    dbg_probe_runtime_init_core1();
    b_program_start();
}
// eof
