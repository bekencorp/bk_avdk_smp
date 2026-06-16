/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE 1:  This project provides two demo applications.  A simple blinky
 * style project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky style version.
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware are defined in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, and two tasks.  It then starts the
 * scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  prvQueueSendTask() sits in a loop that causes it to repeatedly
 * block for 200 milliseconds, before sending the value 100 to the queue that
 * was created within main_blinky().  Once the value is sent, the task loops
 * back around to block for another 200 milliseconds...and so on.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() sits in a loop where it repeatedly
 * blocks on attempts to read data from the queue that was created within
 * main_blinky().  When data is received, the task checks the value of the
 * data, and if the value equals the expected 100, toggles an LED.  The 'block
 * time' parameter passed to the queue receive function specifies that the
 * task should be held in the Blocked state indefinitely to wait for data to
 * be available on the queue.  The queue receive task will only leave the
 * Blocked state when the queue send task writes to the queue.  As the queue
 * send task writes to the queue every 200 milliseconds, the queue receive
 * task leaves the Blocked state every 200 milliseconds, and therefore toggles
 * the LED every 200 milliseconds.
 */

/* Library includes. */
#include <stdint.h>
#include "common/bk_utils.h"
#include "deep_lv.h"
#include "aon_pmu_hal.h"
#include "FreeRTOS.h"
#include "arch_interrupt.h"
#include "bk_arch.h"
#include "cache.h"

#ifndef portSVC_DEEP_LV_EXIT
#define portSVC_DEEP_LV_EXIT   (0x0E)
#endif
#ifndef portSVC_DEEP_LV_ENTER
#define portSVC_DEEP_LV_ENTER  (0x0F)
#endif

#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )

static __used DLV_SEC dlv_context_t s_dlv_context = {0};
static __used volatile dlv_stack_frame_t *s_current_stack_frame = &(s_dlv_context.stk_frame);

extern void flush_all_dcache(void);

uint32_t dlv_is_startup(void)
{
	return aon_pmu_hal_get_dlv_startup();
}

__IRAM_SEC uint32_t dlv_is_startup_iram(void)
{
	return aon_pmu_hal_get_dlv_startup_iram();
}

__IRAM_SEC void dlv_startup(void)
{
	uint32_t dlv_startup = aon_pmu_hal_get_dlv_startup_iram();
	if (dlv_startup) {
		//aon_pmu_hal_set_dlv_startup(0);
	}
	dlv_context_restore();
}

__IRAM_SEC void dlv_trigger_backup_context(void)
{
	__asm volatile
	(
		" .syntax unified           \n"
		" cpsie i                   \n" /* Globally enable interrupts. */
		" cpsie f                   \n"
		" dsb                       \n"
		" isb                       \n"
		" svc %0                    \n"
		" nop                       \n"
		"                           \n"
		" .align 4                  \n"
		::"i"(portSVC_DEEP_LV_ENTER):"r1","r2","memory"
	);

	/*PATCH:get control register*/
	dlv_context_t *dlv = &s_dlv_context;
	dlv_core_t *core_info = &(dlv->core);
	core_info->control_s_val = __get_CONTROL();
	core_info->control_ns_val = __TZ_get_CONTROL_NS();
}

/*************************************************************/
__IRAM_SEC DLV_STATIC void dlv_scb_save(dlv_context_t *dlv)
{
	dlv_scb_t *scb_info = &(dlv->sys_ctrl);

	scb_info->aircr_val = SCB->AIRCR;
	scb_info->scr_val = SCB->SCR;
	scb_info->ccr_val = SCB->CCR;
	scb_info->cpacr_val = SCB->CPACR;
	scb_info->nsacr_val = SCB->NSACR;
	scb_info->vtor_val = SCB->VTOR;
	// scb_info->shpr_val[10] = SCB->SHPR[10];
	// scb_info->shpr_val[11] = SCB->SHPR[11];
	// scb_info->shcsr_val = SCB->SHCSR;

	scb_info->shpr3_val = portNVIC_SHPR3_REG;
}

__IRAM_SEC DLV_STATIC void dlv_nvic_save(dlv_context_t *dlv)
{
	dlv_nvic_t *nvic_info = &(dlv->nvic);
	nvic_info->iser_val[0] = NVIC->ISER[0];
	nvic_info->iser_val[1] = NVIC->ISER[1];
	nvic_info->iser_val[2] = NVIC->ISER[2];
	nvic_info->itns_val[0] = NVIC->ITNS[0];
	nvic_info->itns_val[1] = NVIC->ITNS[1];
	for (uint32_t i = 0; i < 64; i++) {
		nvic_info->ipr_val[i] = NVIC->IPR[i];
	}
}

__IRAM_SEC DLV_STATIC void dlv_systick_save(dlv_context_t *dlv)
{
	dlv_systick_t *systick_info = &(dlv->systick);

	systick_info->ctrl = SysTick->CTRL;
	systick_info->load = SysTick->LOAD;
	systick_info->val = SysTick->VAL;
	systick_info->calib = SysTick->CALIB;
}

__IRAM_SEC DLV_STATIC void dlv_sau_save(dlv_context_t *dlv)
{
	uint32_t i;
	dlv_sau_t *sau_info = &(dlv->sau);

	sau_info->is_enable = (SAU->CTRL) & SAU_CTRL_ENABLE_Msk;
	if(sau_info->is_enable){
		sau_info->ctrl_val = SAU->CTRL;
		sau_info->rnr_val = SAU->RNR;

		for(i = 0; i < 8; i ++){
			SAU->RNR = i;
			sau_info->rbar_val[i] = SAU->RBAR;
			sau_info->rlar_val[i] = SAU->RLAR;
		}
	}
}

__IRAM_SEC DLV_STATIC void dlv_mpu_save(dlv_context_t *dlv)
{
	uint32_t i;
	dlv_mpu_t *mpu_info = &(dlv->mpu);

	mpu_info->is_enable = (MPU->CTRL) & MPU_CTRL_ENABLE_Msk;
	if(mpu_info->is_enable){
		mpu_info->ctrl_val = MPU->CTRL;
		mpu_info->rnr_val = MPU->RNR;

		for(i = 0; i < 16; i ++){
			MPU->RNR = i;
			mpu_info->rbar_val[i] = MPU->RBAR;
			mpu_info->rlar_val[i] = MPU->RLAR;
		}
	}
}

__IRAM_SEC DLV_STATIC void dlv_fpu_save(dlv_context_t *dlv)
{
#if CONFIG_SUPPORT_FPU
	dlv_fpu_t *fpu_info = &(dlv->fpu);

	fpu_info->fpcar_val = FPU->FPCAR;
	fpu_info->fpccr_val = FPU->FPCCR;
	fpu_info->fpdscr_val = FPU->FPDSCR;
#else
	(void)dlv;
#endif
}

__IRAM_SEC DLV_STATIC void dlv_itcm_save(dlv_context_t *dlv)
{
#if CONFIG_SUPPORT_ITCM
	dlv_itcm_t *itcm_info = &(dlv->itcm);

	itcm_info->tcmcr_val = TCM->ITCMCR;
	itcm_info->tgu_ctrl_val = ITGU->TGU_CTRL;
	itcm_info->tgu_lut_val = ITGU->TGU_LUT;
#else
	(void)dlv;
#endif
}

__IRAM_SEC DLV_STATIC void dlv_dtcm_save(dlv_context_t *dlv)
{
#if CONFIG_SUPPORT_DTCM
	dlv_dtcm_t *dtcm_info = &(dlv->dtcm);

	dtcm_info->tcmcr_val = TCM->DTCMCR;
	dtcm_info->tgu_ctrl_val = DTGU->TGU_CTRL;
	dtcm_info->tgu_lut_val = DTGU->TGU_LUT;
#else
	(void)dlv;
#endif
}

__IRAM_SEC DLV_STATIC void dlv_core_save(dlv_context_t *dlv)
{
	dlv_core_t *core_info = &(dlv->core);

	core_info->msplim_val = __get_MSPLIM();
	core_info->msp_val = __get_MSP();
	core_info->psplim_val = __get_PSPLIM();
	core_info->psp_val = __get_PSP();

	core_info->basepri_s_val = __get_BASEPRI();
	core_info->basepri_ns_val = __TZ_get_BASEPRI_NS();
#if CONFIG_GET_AT_SVC_CONTEXT
	/*FIXME:control register is not the contex of deep lv*/
	core_info->control_s_val = __get_CONTROL();
	core_info->control_ns_val = __TZ_get_CONTROL_NS();
#endif
	core_info->primask_s_val  = __get_PRIMASK();
	core_info->primask_ns_val = __TZ_get_PRIMASK_NS();
	core_info->fault_mask_s_val = __get_FAULTMASK();
	core_info->fault_mask_ns_val = __TZ_get_FAULTMASK_NS();
}

__IRAM_SEC void mini_dlv_stack_frame_save_and_dlv(void)
{
	uint32_t *psp_addr = (uint32_t *)__get_PSP();
	s_current_stack_frame->r0 = psp_addr[0];
	s_current_stack_frame->r1 = psp_addr[1];
	s_current_stack_frame->r2 = psp_addr[2];
	s_current_stack_frame->r3 = psp_addr[3];
	s_current_stack_frame->r12 = psp_addr[4];
	s_current_stack_frame->lr = psp_addr[5];
	s_current_stack_frame->ret_pc = psp_addr[6];
	s_current_stack_frame->xpsr = psp_addr[7];

	aon_pmu_ll_set_r0_memchk_bps(1);
	aon_pmu_ll_set_r0_fast_boot(1);
	aon_pmu_hal_set_dlv_startup(1);

#ifdef CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif
}

__attribute__((naked)) __IRAM_SEC void dlv_stack_frame_save_and_dlv(uint32_t exc_return)
{
	__asm volatile
	(
		"    .syntax unified                                \n"
		"                                                   \n"
		"    mov r3, r0                                     \n"/* r3 = EXC_RETURN from naked SVC entry. */
		"    ldr r0, stack_frame_const1                     \n"
		"    ldr r0, [r0]                                   \n"/* Read s_current_stack_frame. */
		"    mrs r1, psp                                    \n"/* Read PSP in r1. */
		"    mrs r2, psplim                                 \n"/* r2 = PSPLIM. */
		"    stmia r0!, {r1-r11}                            \n"/* Save PSP, PSPLIM, EXC_RETURN and the task's r4-r11 before any C prologue runs. */
		"    bl mini_dlv_stack_frame_save_and_dlv           \n"
		"    bl deep_lv_enter                               \n"
		"    dsb                                            \n"
		"    isb                                            \n"
		"    ldr r0, stack_frame_const1                     \n"
		"    ldr r0, [r0]                                   \n"/* Read s_current_stack_frame. */
		"    ldr lr, [r0, #8]                               \n"/* Reload EXC_RETURN from stk_frame.exc_return. */
		"    bx lr                                          \n"
		"                                                   \n"
		"    .align 4                                       \n"
		"stack_frame_const1: .word s_current_stack_frame    \n"
	);
}

/*************************************************************/
__IRAM_SEC DLV_STATIC void dlv_scb_restore(dlv_context_t *dlv)
{
	dlv_scb_t *scb_info = &(dlv->sys_ctrl);
	uint32_t ccr_val = scb_info->ccr_val;

	SCB->VTOR = scb_info->vtor_val;
	SCB->CPACR = scb_info->cpacr_val;
	SCB->SCR = scb_info->scr_val;
	SCB->CCR = ccr_val;
	SCB->NSACR = scb_info->nsacr_val;
	SCB->AIRCR = scb_info->aircr_val;
	// SCB->SHPR[10] = scb_info->shpr_val[10];
	// SCB->SHPR[11] = scb_info->shpr_val[11];
	// SCB->SHCSR = scb_info->shcsr_val;

	if(ccr_val & SCB_CCR_IC_Msk){
		SCB_EnableICache();
	}

	if(ccr_val & SCB_CCR_DC_Msk){
		SCB_EnableDCache();
		SCB_CleanInvalidateDCache();
	}
}

__IRAM_SEC DLV_STATIC void dlv_nvic_restore(dlv_context_t *dlv)
{
	dlv_nvic_t *nvic_info = &(dlv->nvic);

	NVIC->ISER[0] = nvic_info->iser_val[0];
	NVIC->ISER[1] = nvic_info->iser_val[1];
	NVIC->ISER[2] = nvic_info->iser_val[2];
	NVIC->ITNS[0] = nvic_info->itns_val[0];
	NVIC->ITNS[1] = nvic_info->itns_val[1];
	for (uint32_t i = 0; i < 64; i++) {
		NVIC->IPR[i] = nvic_info->ipr_val[i];
	}
}

__IRAM_SEC DLV_STATIC void dlv_systick_restore(dlv_context_t *dlv)
{
	dlv_systick_t *systick_info = &(dlv->systick);

	SysTick->LOAD = systick_info->load;
	SysTick->CTRL = systick_info->ctrl;
}

__IRAM_SEC DLV_STATIC void dlv_sau_restore(dlv_context_t *dlv)
{
	#if (0 == CONFIG_SAU_RECONFIG)
	uint32_t i;
	#endif
	dlv_sau_t *sau_info = &(dlv->sau);

	if(sau_info->is_enable){
	#if CONFIG_SAU_RECONFIG
		TZ_SAU_Setup();
	#else
		for(i = 0; i < 8; i ++){
			SAU->RNR = i;
			SAU->RBAR = sau_info->rbar_val[i];
			SAU->RLAR = sau_info->rlar_val[i];
		}
		SAU->RNR = sau_info->rnr_val;
		SAU->CTRL = sau_info->ctrl_val;
	#endif
	}
}

__IRAM_SEC DLV_STATIC void dlv_mpu_restore(dlv_context_t *dlv)
{
	uint32_t i;
	dlv_mpu_t *mpu_info = &(dlv->mpu);

	if(mpu_info->is_enable){
		for(i = 0; i < 16; i ++){
			MPU->RNR = i;
			MPU->RBAR = mpu_info->rbar_val[i];
			MPU->RLAR = mpu_info->rlar_val[i];
		}

		MPU->RNR = mpu_info->rnr_val;
		MPU->CTRL = mpu_info->ctrl_val;
	}
}

__IRAM_SEC DLV_STATIC void dlv_fpu_restore(dlv_context_t *dlv)
{
#if CONFIG_SUPPORT_FPU
	dlv_fpu_t *fpu_info = &(dlv->fpu);

	FPU->FPCAR = fpu_info->fpcar_val;
	FPU->FPCCR = fpu_info->fpccr_val;
	FPU->FPDSCR = fpu_info->fpdscr_val;
#else
	(void)dlv;
#endif
}

__IRAM_SEC DLV_STATIC void dlv_itcm_restore(dlv_context_t *dlv)
{
#if CONFIG_SUPPORT_ITCM
	dlv_itcm_t *itcm_info = &(dlv->itcm);

	TCM->ITCMCR = itcm_info->tcmcr_val;
	ITGU->TGU_LUT = itcm_info->tgu_lut_val;
	ITGU->TGU_CTRL = itcm_info->tgu_ctrl_val;
#else
	(void)dlv;
#endif
}

__IRAM_SEC DLV_STATIC void dlv_dtcm_restore(dlv_context_t *dlv)
{
#if CONFIG_SUPPORT_DTCM
	dlv_dtcm_t *dtcm_info = &(dlv->dtcm);

	TCM->DTCMCR = dtcm_info->tcmcr_val;
	DTGU->TGU_LUT = dtcm_info->tgu_lut_val;
	DTGU->TGU_CTRL = dtcm_info->tgu_ctrl_val;
#else
	(void)dlv;
#endif
}

__IRAM_SEC DLV_STATIC void dlv_core_restore(dlv_context_t *dlv)
{
	dlv_core_t *core_info = &(dlv->core);

#if 1//CONFIG_SUPPORT_ITCM || CONFIG_SUPPORT_DTCM
	__set_MSPLIM(core_info->msplim_val);
#endif
	/* Restore MSP after MSPLIM so stack limit is valid before adjusting main stack pointer. */
	__set_MSP(core_info->msp_val);
#if 1//CONFIG_SUPPORT_ITCM || CONFIG_SUPPORT_DTCM
	__set_PSPLIM(core_info->psplim_val);
	__set_PSP(core_info->psp_val);
#endif

	__TZ_set_BASEPRI_NS(core_info->basepri_ns_val);
	__TZ_set_PRIMASK_NS(core_info->primask_ns_val);
	__TZ_set_FAULTMASK_NS(core_info->fault_mask_ns_val);
	__set_PRIMASK(core_info->primask_s_val);
	__set_FAULTMASK(core_info->fault_mask_s_val);
	__set_BASEPRI(core_info->basepri_s_val);
#if CONFIG_DUMMY_
	/*FIXME: it effects system stack:msp or psp*/
	__TZ_set_CONTROL_NS(core_info->control_ns_val);
	__set_CONTROL(core_info->control_s_val);
#endif
}

__IRAM_SEC DLV_STATIC void dlv_stack_frame_restore(void)
{
	volatile uint32_t *psp_addr = (volatile uint32_t *)s_current_stack_frame->psp;

	psp_addr[0] = s_current_stack_frame->r0;
	psp_addr[1] = s_current_stack_frame->r1;
	psp_addr[2] = s_current_stack_frame->r2;
	psp_addr[3] = s_current_stack_frame->r3;
	psp_addr[4] = s_current_stack_frame->r12;
	psp_addr[5] = s_current_stack_frame->lr;
	psp_addr[6] = s_current_stack_frame->ret_pc;
	psp_addr[7] = s_current_stack_frame->xpsr;

	__asm volatile
	(
		"    .syntax unified                                \n"
		"    @ Above C code wrote to [PSP]: psp_addr[0..7] = r0,r1,r2,r3,r12,lr,ret_pc,xpsr \n"
		"    @ bx r3 (r3=EXC_RETURN): CPU pops that frame -> PC = psp_addr[6] = ret_pc     \n"
		"                                                   \n"
		"    cpsid i                                        \n" /* Globally disable interrupts. */
		"    cpsie f                                        \n"
		"    ldr r1, stack_frame_const2                     \n"
		"    ldr r0, [r1]                                   \n"/* Read s_dlv_context.stk_frame */
		"    ldmia r0!, {r1-r11}                            \n"/* r1=PSP, r2=PSPLIM, r3=exc_return; r4-r11 from stk_frame */
		"    msr psp, r1                                    \n"/* Restore the PSP register value. */
		"    msr psplim, r2                                 \n"/* Restore the PSPLIM register value. */
		"    movs r0, #2                                    \n"/* r0 = 2. */
		"    msr  CONTROL, r0                               \n"/* Switch to use PSP in the thread mode. */
		"    bx r3                                          \n"/* r3=EXC_RETURN: pop [PSP] -> R0-R3,R12,LR,PC,xPSR; next PC = psp_addr[6]=ret_pc */
		"                                                   \n"
		"    .align 4                                       \n"
		"stack_frame_const2: .word s_current_stack_frame    \n"
	);
}
/*************************************************************/

__IRAM_SEC void dlv_context_save(void)
{
	dlv_context_t *dlv = &s_dlv_context;

	dlv_scb_save(dlv);
	dlv_nvic_save(dlv);
	dlv_systick_save(dlv);
	dlv_sau_save(dlv);
	dlv_mpu_save(dlv);
	dlv_fpu_save(dlv);
	// dlv_itcm_save(dlv);
	// dlv_dtcm_save(dlv);
	dlv_core_save(dlv);
}

__IRAM_SEC DLV_STATIC void dlv_icache_flush_before_sleep(void)
{
#if CONFIG_ICACHE
	cache_instr_invd_all();
	__asm volatile
	(
		"dsb          \n"
		"isb          \n"
		:::"memory"
	);
#endif
}

__IRAM_SEC void deep_lv_enter(void)
{
	dlv_icache_flush_before_sleep();
	dlv_context_save();
}

__IRAM_SEC void dlv_trigger_restore_context(void)
{
	__asm volatile
	(
		" .syntax unified           \n"
		" mrs r0, control          \n"
		" orr r0, r0, #2           \n" /* Force Thread mode to use PSP so SVC stacks on PSP, not MSP. */
		" msr control, r0          \n"
		" isb                      \n"
		" cpsie i                   \n" /* Globally enable interrupts. */
		" cpsie f                   \n"
		" dsb                       \n"
		" isb                       \n"
		" svc %0                    \n"
		" nop                       \n"
		"                           \n"
		" .align 4                  \n"
		::"i"(portSVC_DEEP_LV_EXIT):"r0", "memory"
	);
}

__IRAM_SEC __attribute__((noinline)) void dlv_restore_post_core_prepare(void)
{
	dlv_context_t *dlv = &s_dlv_context;
	dlv_scb_t *scb_info = &(dlv->sys_ctrl);

	arch_int_set_default_priority();
	dlv_nvic_restore(dlv);
	portNVIC_SHPR3_REG = scb_info->shpr3_val;
	#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(37);//3
	GPIO_DOWN(37);
	#endif
}

__IRAM_SEC __attribute__((naked, noreturn)) static void dlv_restore_post_core_finish(void)
{
	__asm volatile
	(
		"    .syntax unified                               \n"
		"    ldr r3, restore_post_core_prepare_const       \n"
		"    blx r3                                        \n"
#if CONFIG_RESTORE_VIA_EXC_RETURN
		"    ldr r3, dlv_trigger_restore_context_const     \n"
		"    bx r3                                         \n"
#else
		"    ldr r3, dlv_stack_frame_restore_const         \n"
		"    bx r3                                         \n"
#endif
		"                                                  \n"
		"    .align 4                                      \n"
		"restore_post_core_prepare_const: .word dlv_restore_post_core_prepare + 1 \n"
#if CONFIG_RESTORE_VIA_EXC_RETURN
		"dlv_trigger_restore_context_const: .word dlv_trigger_restore_context + 1 \n"
#else
		"dlv_stack_frame_restore_const: .word dlv_stack_frame_restore + 1 \n"
#endif
	);
}

__IRAM_SEC void dlv_context_restore(void)
{
	dlv_context_t *dlv = &s_dlv_context;
#if CONFIG_DEEP_LV_DEBUG
	GPIO_UP(37);//2
	GPIO_DOWN(37);
#endif
	/* Match demo restore order; skip ITCM/DTCM (not present on M52). */
	dlv_scb_restore(dlv);
	dlv_sau_restore(dlv);
	dlv_mpu_restore(dlv);
	dlv_fpu_restore(dlv);
	dlv_core_restore(dlv);
	dlv_restore_post_core_finish();
}

__IRAM_SEC __attribute__((noinline)) void dlv_deep_lv_exit_prepare(void)
{
	if (aon_pmu_ll_get_r7b_dlv_startup()) {
		uint32_t dlv_startup = aon_pmu_hal_get_dlv_startup_iram();
		if (dlv_startup) {
			aon_pmu_hal_set_dlv_startup(0);
		}
#if CONFIG_DEEP_LV_DEBUG
		GPIO_UP(37);//4
		GPIO_DOWN(37);
#endif
	}
}

__IRAM_SEC __attribute__((naked)) void deep_lv_exit(void)
{
	__asm volatile
	(
		"    .syntax unified                               \n"
		"    ldr r3, deep_lv_exit_prepare_const            \n"
		"    blx r3                                        \n"
		"    b dlv_stack_frame_restore                     \n"
		"                                                  \n"
		"    .align 4                                      \n"
		"deep_lv_exit_prepare_const: .word dlv_deep_lv_exit_prepare + 1 \n"
	);
}
// eof

