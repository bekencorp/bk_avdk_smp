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

#define AP_SEC_DUMP_ABI_INFO 0x53440130U
extern void bk_coredump_secure_fault_callback(void *context);
extern const uintptr_t g_ap_secure_fault_context_address;

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler(void);
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

void SVC_Handler(void)
{
  soc_svc_handler();
}

void DebugMon_Handler(void)
{
  soc_debugmon_handler();
}

void PendSV_Handler(void)
{
  soc_pendsv_handler();
}

void SysTick_Handler(void)
{
  soc_systick_handler();
}

void M55sub_Handler(void)
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

const VECTOR_ENTRY_TYPE __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
  (VECTOR_ENTRY_TYPE)(&__INITIAL_SP),       /*     Initial Stack Pointer */
  Reset_Handler,                            /*     Reset Handler */
  NMI_Handler,                              /* -14 NMI Handler */
  HardFault_Handler,                        /* -13 Hard Fault Handler */
  MemManage_Handler,                        /* -12 MPU Fault Handler */
  BusFault_Handler,                         /* -11 Bus Fault Handler */
  UsageFault_Handler,                       /* -10 Usage Fault Handler */
  SecureFault_Handler,                      /*  -9 Secure Fault Handler */
  (VECTOR_ENTRY_TYPE)bk_coredump_secure_fault_callback,       /* Reserved: Secure dump callback */
  (VECTOR_ENTRY_TYPE)&g_ap_secure_fault_context_address,       /* Reserved: Secure dump context pointer */
  (VECTOR_ENTRY_TYPE)AP_SEC_DUMP_ABI_INFO,                     /* Reserved: Secure dump ABI */
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

static uint32_t bk_sys_get_uart_base(uint32_t uart_id)
{
    switch (uart_id) {
        case 0: return SOC_UART0_REG_BASE;
        case 1: return SOC_UART1_REG_BASE;
        case 2: return SOC_UART2_REG_BASE;
        default: return SOC_UART0_REG_BASE;
    }
}

/**
 * @brief Direct UART character output (independent of any system functions)
 */
 static void bk_sys_uart_putc(uint32_t uart_id, char c)
 {
    // FIFO status bits
#define SOC_UART_TX_FIFO_EMPTY   (1 << 17)
#define SOC_UART_TX_FIFO_FULL    (1 << 16)
#define SOC_UART_FIFO_STATUS     0x18
#define SOC_UART_FIFO_PORT       0x1C

    uint32_t uart_base = bk_sys_get_uart_base(uart_id);

    volatile uint32_t *uart_fifo_status = (volatile uint32_t *)((uintptr_t)(uart_base + SOC_UART_FIFO_STATUS));
    volatile uint32_t *uart_fifo_port = (volatile uint32_t *)((uintptr_t)(uart_base + SOC_UART_FIFO_PORT));
     
     // Wait for TX FIFO to have space
    while ((*uart_fifo_status & SOC_UART_TX_FIFO_FULL) != 0) {
        // Simple delay to avoid infinite loop
        for (volatile int i = 0; i < 100; i++);
    }
     
    // Send character
    *uart_fifo_port = (uint32_t)c;
 }
 

 int32_t bk_sys_uart_write_string(uint32_t uart_id, const char *string)
{
	const char *p = string;

	while (*string) {
		if (*string == '\n') {
			if (p == string || *(string - 1) != '\r')
				bk_sys_uart_putc(uart_id, '\r'); /* append '\r' */
		}
		bk_sys_uart_putc(uart_id, *string++);
	}

	return 0;
}



#define ENTRY_SECTION  __attribute__((naked, section(".fix.reset_entry")))

/* Records that the reset handler was entered, for postmortem/hang debugging
 * (e.g. when a reboot stalls before the system is re-initialized). */
volatile uint32_t g_reset_entry_state = 0;

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN ENTRY_SECTION void Reset_Handler(void)
{
  g_reset_entry_state = 1;
  // dlv_hook();

  __set_MSPLIM((uint32_t)(&__STACK_LIMIT));

  __disable_irq();

  b_system_base_init();

  b_prep_entry_main();

  b_program_start();
}
// eof
