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
#include "os/os.h"
#include "cmsis_gcc.h"
#include "hal_sw_fih.h"
#include "hal_hw_fih.h"
#include "driver/wdt.h"
#include "bl2_board_clock.h"
#include "deepsleep_fastboot_bl2.h"
#include <soc/soc.h>

#define ENTRY_SECTION  __attribute__((section(".fix.reset_entry")))
#define BL2_STRINGIFY_(x) #x
#define BL2_STRINGIFY(x)  BL2_STRINGIFY_(x)

extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;
extern __NO_RETURN void __PROGRAM_START(void);
extern void SystemInit (void);

typedef void(*VECTOR_TABLE_Type)(void);

__NO_RETURN void Reset_Handler  (void);

/* Self-contained C-runtime entry.
 *
 * The CMSIS __PROGRAM_START / __cmsis_start (cmsis_gcc.h) is a
 * __STATIC_FORCEINLINE helper; in this M52 build it neither inlines into the
 * naked Reset_Handler nor resolves as an out-of-line symbol (the macro target
 * stays unresolved at link). Instead replicate exactly what __cmsis_start does
 * - run the linker .copy.table / .zero.table, then jump to the newlib entry
 * _start() (which performs __libc_init_array and calls main) - in a normal
 * (non-naked) function called from Reset_Handler. */
typedef struct { 
    uint32_t const *src; 
    int32_t *dest; 
    uint32_t wlen; 
} __copy_table_t;
typedef struct { 
    uint32_t *dest; 
    uint32_t wlen; 
} __zero_table_t;
extern const __copy_table_t __copy_table_start__;
extern const __copy_table_t __copy_table_end__;
extern const __zero_table_t __zero_table_start__;
extern const __zero_table_t __zero_table_end__;
extern __NO_RETURN void _start(void);
extern void *memcpy(void *, const void *, unsigned int);
extern void *memset(void *, int, unsigned int);

/* Run the linker copy/zero tables (relocates .data, .iram, .vectors_iram from
 * flash LOAD addr to their RAM run addr, and zeroes .bss). Runs after BL2
 * clock boost so memcpy/memset execute at CPU/flash target frequency. */
static void Run_Copy_Zero_Tables(void)
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
}

extern void __libc_init_array(void);
extern int main(void);

static __NO_RETURN void Pre_Main(void)
{
    /* C-runtime copy/zero tables already ran in Reset_Handler
     * (Run_Copy_Zero_Tables). Do NOT re-enter newlib _mainCRTStartup/_start here:
     * on this BL2 it re-initialises the C runtime / stack in a way that triggers
     * a reset loop. Run the init-array constructors and call main() directly,
     * mirroring the tail of CMSIS __cmsis_start that the bk7259 bringup uses. */
    __libc_init_array();
    (void)main();
    for (;;) { }
}

void NMI_Handler            (void);
void HardFault_Handler      (void);
void MemManage_Handler      (void);
void BusFault_Handler       (void);
void UsageFault_Handler     (void);
void SecureFault_Handler    (void);
void SVC_Handler            (void);
void DebugMon_Handler       (void);
void PendSV_Handler         (void);
void SysTick_Handler        (void);
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
    UART_InterruptHandler,                    /* Interrupt 4 */
    0,                                        /* Interrupt 5 */
    0,                                        /* Interrupt 6 */
    0,                                        /* Interrupt 7 */
    0,                                        /* Interrupt 8 */
    0,                                        /* Interrupt 9 */
    0,                                        /* Interrupt 10 */
    0,                                        /* Interrupt 11 */
    0,                                        /* Interrupt 12 */
    0,                                        /* Interrupt 13 */
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
    (void (*)(void))0x140,                    /* Default Jump BIN offset  */
    (void (*)(void))0x100,                    /* Default Jump BIN length */
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
    (void (*)(void))0x454B4542,               /* offset 0x100, magic code: BEKEN */
    (void (*)(void))0x0000004E,
    /* Reserve 32bytes to protect magic code */
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

#ifndef __VECTOR_IRAM_ATTRIBUTE
#define __VECTOR_IRAM_ATTRIBUTE   __attribute__((used, section(".vectors_iram")))
#endif
/* Keep vectors in 512-aligned IRAM so interrupts still work while XIP flash is
 * erased/programmed. Alignment and flash->IRAM copy are handled by bk7259_bl2.ld. */
const VECTOR_TABLE_Type __VECTOR_IRAM[] __VECTOR_IRAM_ATTRIBUTE = {
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
    UART_InterruptHandler,                    /* Interrupt 4 */
};
__NO_RETURN ENTRY_SECTION __attribute__((naked)) void Reset_Handler(void)
{
    __asm volatile(
#if CONFIG_DIRECT_XIP || CONFIG_OTA_OVERWRITE
        /*
         * This call and the fastboot probe are stackless. If the probe
         * returns, install the BL2 stack and continue with a normal boot.
         * XIP restores A/B remap from the retention record; OVERWRITE just
         * jumps to TF-M (single execute slot).
         */
        "bl bl2_deepsleep_fastboot\n"
#endif
        "ldr r0, =" BL2_STRINGIFY(__INITIAL_SP) "\n"
        "msr msp, r0\n"
        "ldr r0, =" BL2_STRINGIFY(__STACK_LIMIT) "\n"
        "msr msplim, r0\n"
        "dsb\n"
        "isb\n"
        "b Reset_Handler_C\n"
    );
}

__NO_RETURN ENTRY_SECTION void Reset_Handler_C(void)
{
    /* CMSIS System Initialization */
    SystemInit();

    /* Raise clocks before copy/zero :
     * analog early init -> PLL sources -> high freq (CPU 240MHz, flash 80MHz).
     */
    bl2_clock_analog_early_init();
    bl2_clock_enable_pll();
    bl2_clock_enable_high_freq();

    /* Relocate .data/.iram/.vectors_iram and zero .bss at target frequency. */
    Run_Copy_Zero_Tables();

    /* Enter PreMain (C library entry point). */
    Pre_Main();
}

static void flt_putc(char c)
{
	volatile unsigned int  *u1  = (volatile unsigned int  *)0x45830000;
	volatile unsigned char *u1b = (volatile unsigned char *)0x45830000;
	while (u1[0x18 / 4] & (1u << 16)) { }
	u1b[0x1C] = (unsigned char)c;
}
static void flt_puts(const char *s)
{
	for (; *s; ++s) { flt_putc(*s); }
}
static void flt_puthex(unsigned int v)
{
	flt_putc('0'); flt_putc('x');
	for (int i = 28; i >= 0; i -= 4) {
		unsigned int nib = (v >> i) & 0xF;
		flt_putc(nib < 10 ? ('0' + nib) : ('a' + nib - 10));
	}
}
static void flt_kv(const char *k, unsigned int v)
{
	flt_puts(k); flt_putc('='); flt_puthex(v); flt_putc('\r'); flt_putc('\n');
}
/* Dump fault state. sp points at the exception stack frame
 * (r0,r1,r2,r3,r12,lr,pc,xpsr). exc_return is the EXC_RETURN value (LR on entry)
 * so we can tell which stack was active and whether the frame is trustworthy.
 * Non-static: called from naked asm handlers. */
void fault_report(const char *tag, unsigned int *sp, unsigned int exc_return)
{
	volatile unsigned int *SCB_CFSR  = (volatile unsigned int *)0xE000ED28;
	volatile unsigned int *SCB_HFSR  = (volatile unsigned int *)0xE000ED2C;
	volatile unsigned int *SCB_MMFAR = (volatile unsigned int *)0xE000ED34;
	volatile unsigned int *SCB_BFAR  = (volatile unsigned int *)0xE000ED38;
	volatile unsigned int *SCB_SFSR  = (volatile unsigned int *)0xE000EDE4;
	volatile unsigned int *SCB_SFAR  = (volatile unsigned int *)0xE000EDE8;
	unsigned int msp, psp, msplim;
	__asm volatile("mrs %0, msp"    : "=r"(msp));
	__asm volatile("mrs %0, psp"    : "=r"(psp));
	__asm volatile("mrs %0, msplim" : "=r"(msplim));

	flt_puts("\r\n!!FAULT "); flt_puts(tag); flt_puts("\r\n");
	flt_kv("CFSR ", *SCB_CFSR);
	flt_kv("HFSR ", *SCB_HFSR);
	flt_kv("MMFAR", *SCB_MMFAR);
	flt_kv("BFAR ", *SCB_BFAR);
	flt_kv("SFSR ", *SCB_SFSR);
	flt_kv("SFAR ", *SCB_SFAR);
	/* System state: tells us which stack was live + if MSP overflowed MSPLIM. */
	flt_kv("EXC_R", exc_return);
	flt_kv("MSP  ", msp);
	flt_kv("PSP  ", psp);
	flt_kv("MSPLM", msplim);
	flt_kv("frmSP", (unsigned int)sp);
	/* Only trust the stacked frame if it lies in on-chip SRAM (0x28000000..) or
	 * DTCM (0x20000000..). If frmSP is anywhere else the frame is bogus and the
	 * st_* below are stale garbage (do NOT symbolize st_PC in that case). */
	if (sp &&
	    (((unsigned int)sp >= 0x20000000u && (unsigned int)sp < 0x20080000u) ||
	     ((unsigned int)sp >= 0x28000000u && (unsigned int)sp < 0x28200000u))) {
		flt_kv("st_R0 ", sp[0]);
		flt_kv("st_R1 ", sp[1]);
		flt_kv("st_R2 ", sp[2]);
		flt_kv("st_R3 ", sp[3]);
		flt_kv("st_R12", sp[4]);
		flt_kv("st_LR ", sp[5]);
		flt_kv("st_PC ", sp[6]);   /* real faulting PC (frame validated) */
		flt_kv("st_xPS", sp[7]);
	} else {
		flt_puts("frame INVALID (stack corrupt/overflow) - st_* NOT captured\r\n");
	}
	while (1) { }
}
/* Naked entry: capture the active SP (MSP/PSP) and EXC_RETURN for the reporter.
 * r0=tag, r1=frame SP, r2=EXC_RETURN(lr). */
#define DEFINE_FAULT_HANDLER(name, tag)                                       \
__attribute__((naked)) void name(void)                                        \
{                                                                             \
	__asm volatile(                                                       \
		"mov r2, lr            \n"                                     \
		"tst lr, #4            \n"                                     \
		"ite eq                \n"                                     \
		"mrseq r1, msp         \n"                                     \
		"mrsne r1, psp         \n"                                     \
		"mov r0, %0            \n"                                     \
		"b fault_report        \n"                                    \
		: : "r" (tag) : "r0", "r1", "r2");                            \
}

void default_handler(void)
{
	while (1)
	{
	}
}

void NMI_Handler(void)
{
	default_handler();
}

DEFINE_FAULT_HANDLER(HardFault_Handler,  "HardFault")
DEFINE_FAULT_HANDLER(MemManage_Handler,  "MemManage")
DEFINE_FAULT_HANDLER(BusFault_Handler,   "BusFault")
DEFINE_FAULT_HANDLER(UsageFault_Handler, "UsageFault")
DEFINE_FAULT_HANDLER(SecureFault_Handler,"SecureFault")

void SVC_Handler(void)
{
	default_handler();
}

void DebugMon_Handler(void)
{
	default_handler();
}

void PendSV_Handler(void)
{
	default_handler();
}
