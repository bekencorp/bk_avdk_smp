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

#include "platform_irq.h"
#include "tfm_plat_config.h"
#include "region_defs.h"
#include "os/os.h"
#include "STAR_SE.h"
#include "cmsis.h"
#include "cmsis_gcc.h"

#define ENTRY_SECTION  __attribute__((section(".fix.reset_entry")))

extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;
extern __NO_RETURN void __PROGRAM_START(void);
extern void SystemInit (void);

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
    _start();
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
void UART_InterruptHandler  (void) __attribute__ ((weak));

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
	0,                                        /* Interrupt 3 */
};

__NO_RETURN ENTRY_SECTION __attribute__((naked)) void Reset_Handler(void)
{
    __set_MSPLIM((uint32_t)(&__STACK_LIMIT));
    SCB->VTOR = (uint32_t) &__VECTOR_TABLE[0];
    Pre_Main();
}
