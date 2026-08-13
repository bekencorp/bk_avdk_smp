;/*
; * Copyright (c)     2023-2028 ARM Limited
; *
; * Licensed under the Apache License, Version 2.0 (the "License");
; * you may not use this file except in compliance with the License.
; * You may obtain a copy of the License at
; *
; *     http://www.apache.org/licenses/LICENSE-2.0
; *
; * Unless required by applicable law or agreed to in writing, software
; * distributed under the License is distributed on an "AS IS" BASIS,
; * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; * See the License for the specific language governing permissions and
; * limitations under the License.
; *
; *
; * This file is derivative of CMSIS V5.00 startup_ARMCM33.S
; */

#include "tfm_plat_config.h"
#include "region_defs.h"
#include "platform_irq.h"
#include "os/os.h"
#include "cmsis_gcc.h"
#include "system_cmsdk_bk7236.h"
#include "hal_jtag.h"
#include "hal_hw_fih.h"
#include "hal_sw_fih.h"
#if CONFIG_SLEEP_RETENTION_NSC
#include "sleep_fastboot_tfm.h"
#endif

#define ENTRY_SECTION  __attribute__((section(".fix.reset_entry")))
#define SYSTEM_BASE_ADDR                 (0x44010000)
#define OTP_APB_BASE_ADDRESSS            (0x42100000)  /* BK7259 OTP APB base (was bk7239n 0x4b100000) */
#define MEM_CHECK_BASE_ADDRESSS          (0x44890000)
#define GET_BIT_VAL(val, bit)            ((val >> bit) & 1)
#define SET_BIT_VAL(val, bit, new_val)   ((val & (~ (1 << bit)))|(new_val << bit))

extern uint32_t __StackSeal;
extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;
extern __NO_RETURN void __PROGRAM_START(void);
extern void sys_drv_early_init(void);
extern void sys_hal_early_init_tfm(void);
extern bk_err_t sys_hal_ctrl_vdddig_h_vol(uint32_t vol_value);
extern void sys_hal_switch_freq(uint32_t cksel_core, uint32_t ckdiv_core, uint32_t ckdiv_cpu0);
extern void sys_hal_flash_set_clk(uint32_t value);
extern void sys_hal_flash_set_clk_div(uint32_t value);

typedef void(*VECTOR_TABLE_Type)(void);

__NO_RETURN void Reset_Handler  (void);

/* See startup_bk7259_bl2.c: the CMSIS __PROGRAM_START / __cmsis_start does not
 * resolve in this M52 build, so replicate the C-runtime entry (copy/zero table
 * init + newlib _start) in a normal function. */
typedef struct { uint32_t const *src; uint32_t *dest; uint32_t wlen; } __copy_table_t;
typedef struct { uint32_t *dest; uint32_t wlen; } __zero_table_t;
extern const __copy_table_t __copy_table_start__;
extern const __copy_table_t __copy_table_end__;
extern const __zero_table_t __zero_table_start__;
extern const __zero_table_t __zero_table_end__;
extern __NO_RETURN void _start(void);
extern void __libc_init_array(void);
extern int main(void);
extern void *memcpy(void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);

static __NO_RETURN void Pre_Main(void)
{
    for (const __copy_table_t *t = &__copy_table_start__; t < &__copy_table_end__; ++t) {
        if (t->wlen > 0) {
            memcpy(t->dest, t->src, t->wlen * 4);
        }
    }
    for (const __zero_table_t *t = &__zero_table_start__; t < &__zero_table_end__; ++t) {
        if (t->wlen > 0) {
            memset(t->dest, 0, t->wlen * 4);
        }
    }
    /* BK7259: do NOT re-enter newlib _start()/_mainCRTStartup here. Like the
     * bk7259 BL2 startup, the C-runtime relocation (copy/zero tables above) is
     * already done; _mainCRTStartup re-inits the stack/runtime and hangs the
     * secure image (observed: stops right after TS3). Run the init-array
     * constructors and call main() directly (the tail of CMSIS __cmsis_start). */
    __libc_init_array();
    (void)main();
    while (1) { }
}

void NMI_Handler            (void) __attribute__ ((weak));
void HardFault_Handler      (void) __attribute__ ((weak));
void MemManage_Handler      (void) __attribute__ ((weak));
void BusFault_Handler       (void) __attribute__ ((weak));
void UsageFault_Handler     (void) __attribute__ ((weak));
void SecureFault_Handler    (void) __attribute__ ((weak));
void SVC_Handler            (void) __attribute__ ((weak));
void DebugMon_Handler       (void) __attribute__ ((weak));
void PendSV_Handler         (void) __attribute__ ((weak));
void SysTick_Handler        (void) __attribute__ ((weak));
void TFM_TIMER0_Handler     (void) __attribute__ ((weak));
void UART_InterruptHandler  (void) __attribute__ ((weak));
void TFM_TIMER1_Handler     (void) __attribute__ ((weak));

const VECTOR_TABLE_Type __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
    (VECTOR_TABLE_Type)(&__INITIAL_SP),      /*     Initial Stack Pointer */

    /* Core interrupts */
    Reset_Handler,                            /*     Reset Handler */
    NMI_Handler,                              /* -14 NMI Handler */
    HardFault_Handler,                        /* -13 Hard Fault Handler */
    MemManage_Handler,                        /* -12 MPU Fault Handler */
    BusFault_Handler,                         /* -11 Bus Fault Handler */
    UsageFault_Handler,                       /* -10 Usage Fault Handler */
    SecureFault_Handler,                      /*  -9 Secure Fault Handler */
    0,                                        /*  -8 */
    0,                                        /*  -7 */
    0,                                        /*  -6 */
    SVC_Handler,                              /*  -5 SVCall Handler */
    DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
    0,                                        /*  -3 */
    PendSV_Handler,                           /*  -2 PendSV Handler */
    SysTick_Handler,                          /*  -1 SysTick Handler */

    /* External interrupts */
    0,                                        /* Interrupt 0 */
    0,                                        /* Interrupt 1 */
    0,                                        /* Interrupt 2 */
    TFM_TIMER0_Handler,                       /* Interrupt 3  (TIMER0_IRQn, HW Timer0) */
    UART_InterruptHandler,                    /* Interrupt 4 */
    0,                                        /* Interrupt 5 */
    0,                                        /* Interrupt 6 */
    0,                                        /* Interrupt 7 */
    0,                                        /* Interrupt 8 */
    0,                                        /* Interrupt 9 */
    0,                                        /* Interrupt 10 */
    0,                                        /* Interrupt 11 */
    0,                                        /* Interrupt 12 */
    0,                                        /* Interrupt 13 (ACOMP0_IRQn on BK7259) */
    0,                                        /* Interrupt 14 */
    0,                                        /* Interrupt 15 */
    0,                                        /* Interrupt 16 */
    0,                                        /* Interrupt 17 */
    0,                                        /* Interrupt 18 */
    0,                                        /* Interrupt 19 */
    0,                                        /* Interrupt 20 */
    0,                                        /* Interrupt 21 */
    0,                                        /* Interrupt 22 */
    0,                                        /* Interrupt 23 */
    0,                                        /* Interrupt 24 */
    0,                                        /* Interrupt 25 */
    0,                                        /* Interrupt 26 */
    0,                                        /* Interrupt 27 */
    0,                                        /* Interrupt 28 */
    0,                                        /* Interrupt 29 */
    0,                                        /* Interrupt 30 */
    0,                                        /* Interrupt 31 */
    0,                                        /* Interrupt 32 */
    0,                                        /* Interrupt 33 */
    0,                                        /* Interrupt 34 */
    0,                                        /* Interrupt 35 */
    0,                                        /* Interrupt 36 */
    0,                                        /* Interrupt 37 */
    0,                                        /* Interrupt 38 */
    0,                                        /* Interrupt 39 */
    0,                                        /* Interrupt 40 */
    0,                                        /* Interrupt 41 */
    0,                                        /* Interrupt 42 */
    0,                                        /* Interrupt 43 */
    0,                                        /* Interrupt 44 */
    0,                                        /* Interrupt 45 */
    0,                                        /* Interrupt 46 */
    0,                                        /* Interrupt 47 */
    (void (*)(void))0x454B4542,               /* Interrupt 48, offset 0x100 magic: BEKEN */
    (void (*)(void))0x0000004E,               /* Interrupt 49 */

    0,                                        /* Interrupt 50 */
    0,                                        /* Interrupt 51 */
    0,                                        /* Interrupt 52 */
    0,                                        /* Interrupt 53 */
    0,                                        /* Interrupt 54 */
    0,                                        /* Interrupt 55 */
    0,                                        /* Interrupt 56 */
    0,                                        /* Interrupt 57 */
    TFM_TIMER1_Handler,                       /* Interrupt 58 (TIMER1_IRQn, HW Timer1) */
    0,                                        /* Interrupt 59 */
    0,                                        /* Interrupt 60 */
    0,                                        /* Interrupt 61 */
    0,                                        /* Interrupt 62 */
    0,                                        /* Interrupt 63 */

    /* Reserve padding to protect magic code region */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

__NO_RETURN ENTRY_SECTION __attribute__((naked)) void Reset_Handler(void)
{
    /* BK7259 bring-up: keep tfm_s reset minimal and aligned with the bk7259
     * bringup tfm_s startup (assembly: msplim -> copy/zero -> SystemInit ->
     * _start). Drop the psa_level3 additions that destabilised / hung the
     * secure image right at entry:
     *   - bk_fih_init()/FIH_ASSERT128 (FIH hardware self-test; user dropped FIH)
     *   - hal_secure_debug()
     *   - a 2nd sys_drv_early_init() (already done by BL2; re-running it on the
     *     analog/clock path from the secure side hangs)
     *   - sys_hal_switch_cpu_bus_freq() (BL2 kept the bootrom clock; re-switching
     *     here is unnecessary and risky)
     * The watchdogs are already disabled by BL2. */
    __set_MSPLIM((uint32_t)(&__STACK_LIMIT));

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
    __TZ_set_STACKSEAL_S((uint32_t *)(&__STACK_SEAL));
#endif

    /* CMSIS System Initialization */
    SystemInit();

#if CONFIG_SLEEP_RETENTION_NSC
    tfm_sleep_early_boot();
#endif

    /* Enter PreMain (C library entry point) */
    Pre_Main();
}
