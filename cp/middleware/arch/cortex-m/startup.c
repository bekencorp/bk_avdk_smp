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
#include "wdt_driver.h"
#include "sys_hal.h"

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
void b_system_base_init (void);
void b_prep_entry_main(void);
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

__attribute__((naked)) void Default_Handler(void)
{
  volatile uint32_t i = 1;
  while (i) {
    __WFI();
  }
}

void bk_enable_swd(void)
{
	sys_hal_enable_swd();
}



/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

const VECTOR_ENTRY_TYPE __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
  (VECTOR_ENTRY_TYPE)(&__INITIAL_SP),       /*     Initial Stack Pointer */
  Reset_Handler,                            /*     Reset Handler */
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

  /* Interrupts */
  Default_Handler,                        /* 0 DMA0_NSEC_Handler */
  Default_Handler,                         /* 1 ENCP_SEC_Handler */
  Default_Handler,                        /* 2 ENCP_NSEC_Handler */
  Default_Handler,                           /* 3 TIMER0_Handler */
  Default_Handler,                            /* 4 UART0_Handler */
  Default_Handler,                             /* 5 PWM0_Handler */
  Default_Handler,                             /* 6 I2C0_Handler */
  Default_Handler,                             /* 7 SPI0_Handler */
  Default_Handler,                           /* 8 SARADC_Handler */
  0,                                        /* 9 Reserved */
  Default_Handler,                             /* 10 SDIO_Handler */
  Default_Handler,                             /* 11 GDMA_Handler */
  Default_Handler,                               /* 12 LA_Handler */
  Default_Handler,                           /* 13 TIMER1_Handler */
  Default_Handler,                             /* 14 I2C1_Handler */
  Default_Handler,                            /* 15 UART1_Handler */
  Default_Handler,                            /* 16 UART2_Handler */
  0,                                        /* 17 Reserved */
  Default_Handler,                              /* 18 LED_Handler */
  0,                                        /* 19 Reserved*/
  0,                                        /* 20 Reserved */
  Default_Handler,                             /* 21 CKMN_Handler */
  0,                                        /* 22 Reserved */
  0,                                        /* 23 Reserved */
  Default_Handler,                             /* 24 I2S0_Handler */
  0,                                        /* 25 Reserved */
  0,                                        /* 26 Reserved */
  0,                                        /* 27 Reserved */
  0,                                        /* 28 Reserved */
  Default_Handler,                          /* 29 PHY_MBP_Handler */
  Default_Handler,                          /* 30 PHY_RIU_Handler */
  Default_Handler,              /* 31 MAC_INT_TX_RX_TIMER_Handler */
  Default_Handler,               /* 32 MAC_INT_TX_RX_MISC_Handler */
  Default_Handler,               /* 33 MAC_INT_RX_TRIGGER_Handler */
  Default_Handler,               /* 34 MAC_INT_TX_TRIGGER_Handler */
  Default_Handler,             /* 35 MAC_INT_PORT_TRIGGER_Handler */
  Default_Handler,                      /* 36 MAC_INT_GEN_Handler */
  Default_Handler,                              /* 37 HSU_Handler */
  Default_Handler,                   /* 38 INT_MAC_WAKEUP_Handler */
  Default_Handler,                             /* 39 BTDM_Handler */
  Default_Handler,                              /* 40 BLE_Handler */
  Default_Handler,                               /* 41 BT_Handler */
  Default_Handler,                            /* 42 QSPI0_Handler */
  0,                                        /* 43 Reserved */
  0,                                        /* 44 Reserved */
  0,                                        /* 45 Reserved */
  0,                                        /* 46 Reserved */
  0,                                        /* 47 Reserved */
  /* BK7259 legacy download mode requires that the flash offset 0x100 is 'BEKEN'.*/
  (void (*)(void))0x454B4542,
  (void (*)(void))0x0000204E,
  Default_Handler,                           /* 48 THREAD_Handler */
  0,                                        /* 49 Reserved */
  Default_Handler,                              /* 50 OTP_Handler */
  Default_Handler,                      /* 51 DPLL_UNLOCK_Handler */
  0,                                        /* 52 Reserved */
  0,                                        /* 53 Reserved */
  0,                                        /* 54 Reserved */
  Default_Handler,                             /* 55 GPIO_S_Handler */
  Default_Handler,                          /* 56 GPIO_NS_Handler */
  0,                                        /* 57 Reserved */
  Default_Handler,                         /* 58 ANA_GPIO_Handler */
  Default_Handler,                          /* 59 ANA_RTC_Handler */
  Default_Handler,                    /* 60 ABNORMAL_GPIO_Handler */
  Default_Handler,                     /* 61 ABNORMAL_RTC_Handler */
  Default_Handler,                          /* 62 DIG_RTC_Handler */
  0,                                        /* 63 Reserved */
  /* Interrupts 64 .. 480 are left out */
};

#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

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
  __disable_irq();
  dlv_hook();

  __set_MSPLIM((uint32_t)(&__STACK_LIMIT));

  //__disable_irq();

  bk_wdt_force_feed();
  bk_enable_swd();

  b_system_base_init();
  b_prep_entry_main();
  b_program_start();
}
// eof

