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
#include "common/bk_include.h"
#include "sdkconfig.h"
#include "interrupt_controller.h"

#include "bk_arch.h"

typedef void(*VECTOR_ENTRY_TYPE)(void);
/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __StackTopCore0;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler  (void);
void _default_handler_(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Exceptions */
#if CONFIG_SOC_SMP
__NO_RETURN void Reset_Handler_Core0(void);
#else
__NO_RETURN void Reset_Handler(void);
#endif
void soc_nmi_handler(void);
void soc_hardfault_handler(void);
void soc_memmanage_handler(void);
void soc_busfault_handler(void);
void soc_usagefault_handler(void);
void soc_securefault_handler(void);
void soc_svc_handler(void);
void soc_debugmon_handler(void);
void soc_pendsv_handler(void);
void soc_systick_handler(void);
void M55sub_Handler(void) __attribute__ ((weak));

#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif



__attribute__((used, section(".vectors_iram"))) \
const VECTOR_ENTRY_TYPE __SOC_VECTOR_TABLE[VECTOR_SYS_ENTRY_CNT + __INT_NUMBER_MAX] = {
    (VECTOR_ENTRY_TYPE)(&__StackTopCore0),    /*     Initial Stack Pointer */
#if CONFIG_SOC_SMP
    Reset_Handler_Core0,                      /*     Reset Handler */
#else
    Reset_Handler,                      /*     Reset Handler */
#endif
    soc_nmi_handler,                              /* -14 NMI Handler */
    soc_hardfault_handler,                        /* -13 Hard Fault Handler */
    soc_memmanage_handler,                        /* -12 MPU Fault Handler */
    soc_busfault_handler,                         /* -11 Bus Fault Handler */
    soc_usagefault_handler,                       /* -10 Usage Fault Handler */
    soc_securefault_handler,                      /*  -9 Secure Fault Handler */
    0,                                        /*     Reserved */
    0,                                        /*     Reserved */
    0,                                        /*     Reserved */
    soc_svc_handler,                              /*  -5 SVCall Handler */
    soc_debugmon_handler,                         /*  -4 Debug Monitor Handler */
    0,                                            /*     Reserved */
    soc_pendsv_handler,                           /*  -2 PendSV Handler */
    soc_systick_handler,                          /*  -1 SysTick Handler */
    M55sub_Handler,                           /*  -0 M55sub Handler  */                      /*   AP  Zero Table End */
    /* sys_int_src[63:0] the 64 interrupt source will be registered by its device driver*/
};

#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif


uint32_t get_relocate_vector_table(void) {
    return (uint32_t) &(__SOC_VECTOR_TABLE[0]);
}


uint32_t relocate_vector_table(void)
{
    uint32_t vtor_addr;

    vtor_addr = get_relocate_vector_table();
    SCB->VTOR = vtor_addr;

    return vtor_addr;
}
// eof

