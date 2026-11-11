// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include <stdbool.h>
#include <stdint.h>

#include "cmsis.h"
#include "region_defs.h"
#include "sleep_fastboot_tfm.h"
#include "soc/soc.h"
#include "target_cfg.h"
#include "tfm_hal_isolation.h"
#include "tfm_hal_platform.h"
#include "tfm_sleep_context.h"
#include "aon_pmu_hal.h"
#include "aon_pmu_ll.h"

extern void tcm_spe(void);
extern uint32_t __STACK_LIMIT;
extern uint32_t __StackSeal;

/* Place a function in the SRAM-resident fastboot resume section. These run at
 * the reset-default low clock on the wakeup path, so build them optimized
 * instead of the project-wide -O0 to cut the instruction count. */
#define TFM_RAMFUNC \
	__attribute__((section(".tfm_fastboot_code"), noinline, optimize("O2")))

#define AON_PMU_R0_ADDR    (SOC_AON_PMU_REG_BASE + 0x00u)
#define AON_PMU_R25_ADDR   (SOC_AON_PMU_REG_BASE + (0x25u << 2))
#define AON_PMU_R7B_ADDR   (SOC_AON_PMU_REG_BASE + (0x7Bu << 2))
#define AON_PMU_LATCH_KEY1 (0x424B55AAu)
#define AON_PMU_LATCH_KEY2 (0xBDB4AA55u)

/* Non-secure alias of UART0, written by uart_min_switch_uart0_to_nsec(). */
#define UART0_NON_SECURE_BASE (0x54820000u)

/* NVIC_SetPriority() is emitted out of line at the project -O0 settings, so the
 * SHPR byte write is done directly here to keep the resume path in SRAM. */
#define TFM_SLEEP_SHPR(irqn) \
	SCB->SHPR[(((uint32_t)(irqn)) & 0xFu) - 4u]
#define TFM_SLEEP_PRIO(prio) \
	((uint32_t)(prio) << (8u - __NVIC_PRIO_BITS))
#define TFM_SLEEP_PENDSV_PRIO ((1u << (__NVIC_PRIO_BITS - 1u)) - 1u)

/* Clear the deep-sleep markers in the retained R0 image. Register-only, so it
 * is safe to call from the SRAM resume path. */
static TFM_RAMFUNC void tfm_sleep_clear_aon_fastboot_flags(void)
{
	aon_pmu_r0_t retained;

	/* R7B is the retained source of truth after reset. Restore all retained
	 * fields to R0 before clearing the fastboot markers so the whole-word
	 * latch cannot erase reset reason, owner, or other retained state. */
	retained.v = REG_READ(AON_PMU_R7B_ADDR);
	retained.fast_boot = 0;
	retained.dlv_startup = 0;
	REG_WRITE(AON_PMU_R0_ADDR, retained.v);
	__DSB();
	REG_WRITE(AON_PMU_R25_ADDR, AON_PMU_LATCH_KEY1);
	REG_WRITE(AON_PMU_R25_ADDR, AON_PMU_LATCH_KEY2);
	__DSB();
}

/* TF-M programs the secure exception priorities and the AIRCR.PRIS bit only once,
 * inside tfm_core_init(). The fastboot resume path never runs that code, so the
 * values have to be re-applied here: the SPM scheduler requires secure PendSV to
 * be the lowest secure priority.
 *
 * Kept in SRAM instead of calling tfm_arch_set_secure_exception_priorities():
 * that function lives in flash and NVIC_SetPriority is emitted out of line at
 * the project -O0, while the resume path must not execute from flash. Level
 * values mirror tfm_arch.h (all zero, PENDSV_PRIO_FOR_SCHED for PendSV). */
static TFM_RAMFUNC void tfm_sleep_set_secure_exception_priorities(void)
{
	uint32_t aircr = SCB->AIRCR;
	uint32_t vectkey = (~aircr & SCB_AIRCR_VECTKEYSTAT_Msk);

	SCB->AIRCR = SCB_AIRCR_PRIS_Msk | vectkey |
		     (aircr & ~SCB_AIRCR_VECTKEY_Msk);

	TFM_SLEEP_SHPR(MemoryManagement_IRQn) = TFM_SLEEP_PRIO(0);
	TFM_SLEEP_SHPR(BusFault_IRQn) = TFM_SLEEP_PRIO(0);
	TFM_SLEEP_SHPR(SecureFault_IRQn) = TFM_SLEEP_PRIO(0);
	TFM_SLEEP_SHPR(SVCall_IRQn) = TFM_SLEEP_PRIO(0);
	TFM_SLEEP_SHPR(PendSV_IRQn) = TFM_SLEEP_PRIO(TFM_SLEEP_PENDSV_PRIO);
}

/* Same register writes as enable_fault_handlers() in target_cfg.c, inlined here
 * because the flash-resident implementation calls NVIC_SetPriority out of line. */
static TFM_RAMFUNC void tfm_sleep_enable_fault_handlers(void)
{
	TFM_SLEEP_SHPR(SecureFault_IRQn) = TFM_SLEEP_PRIO(0);

	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk |
		      SCB_SHCSR_BUSFAULTENA_Msk |
		      SCB_SHCSR_MEMFAULTENA_Msk |
		      SCB_SHCSR_SECUREFAULTENA_Msk;
}

static TFM_RAMFUNC void tfm_sleep_scheduler_state_reset(void)
{
	/* SPM scheduler lock, retained in SRAM across the sleep. arch_attempt_schedule()
	 * only pends PendSV while it reads SCHEDULER_UNLOCKED (0); a switch interrupted
	 * by the sleep could otherwise leave the secure scheduler permanently locked. */
	extern uint32_t scheduler_lock;

	scheduler_lock = 0u;
	/* Drop stale system exception requests left over from before the sleep. */
	SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk | SCB_ICSR_PENDSTCLR_Msk;
	__DSB();
	__ISB();
}

static TFM_RAMFUNC void tfm_sleep_secure_hw_init(void)
{
	sau_and_idau_cfg();
	(void)tfm_hal_secure_static_mpu_init();
	tfm_hal_dma_init();

	/* Restore interrupt routing: deep sleep resets NVIC->ITNS to secure and
	 * ITNS is inaccessible from the non-secure side, so it must be redone
	 * here before returning to NSPE. */
	(void)nvic_interrupt_target_state_cfg();

	tfm_sleep_set_secure_exception_priorities();
}

__attribute__((section(".tfm_fastboot_code"), naked, noreturn))
static void tfm_sleep_bxns_entry(uint32_t ns_ep, uint32_t secure_psp,
				 uint32_t secure_psplim, uint32_t secure_control)
{
	/*
	 * Three SECURE-banked processor states must be sane before BXNS, or the
	 * first NS exception (the deep-LV EXIT 'svc') never runs and the core
	 * appears hung. The NS side can only clear its own banked copies, so we
	 * must fix the SECURE ones here, in the last instructions before BXNS:
	 *
	 * 1) CONTROL.FPCA (secure): if left set, NS exception entry lazily stacks
	 *    SECURE FP context -> SecureFault routed to secure -> silent hang.
	 *
	 * 2) PRIMASK/FAULTMASK/BASEPRI (secure): tfm_sleep_jump_to_ns() runs
	 *    __disable_irq() (PRIMASK_S=1) right before this. PRIMASK is banked,
	 *    so the NS 'cpsie i' cannot clear PRIMASK_S. With PRIMASK_S=1 the
	 *    execution priority is boosted to 0, so the NS SVCall (a configurable
	 *    priority exception) cannot be taken and ESCALATES TO HARDFAULT
	 *    (observed: CFSR=0, HFSR.FORCED=1). Clear the secure masks so the NS
	 *    SVCall is takeable. This is the actual deep-LV warm-boot hang fix.
	 *
	 * 3) PSP/PSPLIM/CONTROL.SPSEL (secure): a Deep-LV wakeup returns from the
	 *    reset vector with the banked secure stack registers cleared. TF-M
	 *    derives the SPM connection slot from PSPLIM on every NS call, so
	 *    PSPLIM=0 makes handle_to_connection() underflow and the SPM panics on
	 *    the first PSA call. Restore the values captured right before the
	 *    cold-boot BXNS; PSP must be programmed before SPSEL selects it.
	 *
	 * clrm below clears r1-r12,r14,apsr (not r0/PRIMASK), so it is safe to use
	 * r1 before it and to clear the masks after it.
	 */
	__asm volatile(
		"msr psplim, r2\n"
		"msr psp, r1\n"
		"isb\n"
		"bic r3, r3, #5\n"      /* keep SPSEL, clear nPRIV and FPCA */
		"msr control, r3\n"
		"isb\n"
		"clrm {r1-r12, r14, apsr}\n"
		"msr basepri, r1\n"     /* r1==0 after clrm -> BASEPRI_S = 0 */
		"cpsie i\n"             /* clear PRIMASK_S: NS SVCall must be takeable */
		"cpsie f\n"             /* clear FAULTMASK_S */
		"isb\n"
		"bic r0, r0, #1\n"
		"bxns r0\n"
	);
}

static TFM_RAMFUNC __attribute__((noreturn)) void tfm_sleep_jump_to_ns(void)
{
	uint32_t ns_vtor = NS_CODE_START;
	uint32_t reg_val;
	uint32_t ns_msp = *((uint32_t *)ns_vtor);
	uint32_t ns_ep = *((uint32_t *)(ns_vtor + 4u));
	uint32_t secure_psp = 0u;
	uint32_t secure_psplim = 0u;
	uint32_t secure_control = 0x2u;   /* SPSEL=1, privileged, no FPCA */

	if (tfm_sleep_context_apply_ppc() != 0) {
		while (1) {
		}
	}

	/* PPRO has now marked UART0 non-secure, so the shared console driver must
	 * be retargeted to the Non-secure alias (same as ns_init_hook on cold boot).
	 * Otherwise secure-side fault and panic output faults again on the first
	 * character and hides the original error. Written here, instead of calling
	 * uart_min_switch_uart0_to_nsec(), to keep the resume path out of flash. */
	extern volatile uint32_t g_uart0_base;
	g_uart0_base = UART0_NON_SECURE_BASE;

	SCB_NS->VTOR = ns_vtor;
	__TZ_set_MSP_NS(ns_msp);

	reg_val = SCB->AIRCR;
	reg_val &= (~(uint32_t)SCB_AIRCR_VECTKEYSTAT_Msk);
	reg_val |= (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
			      SCB_AIRCR_PRIS_Msk |
			      SCB_AIRCR_BFHFNMINS_Msk);
	SCB->AIRCR = reg_val;

	SCB->NSACR |= SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk;

	tcm_spe();

	/* A Deep-LV wakeup returns from the reset vector with the banked secure
	 * stack registers cleared. The SPM scheduler validates PSP against the
	 * stack of the thread it is about to switch to, so restore the values it
	 * recorded for the current secure thread instead of a cold-boot capture. */
	{
		extern uint32_t tfm_spm_get_ns_agent_sp(uint32_t *sp_limit);
		uint32_t spm_psp;
		uint32_t spm_psplim = 0u;

		spm_psp = tfm_spm_get_ns_agent_sp(&spm_psplim);
		if ((spm_psp != 0u) && (spm_psplim != 0u) &&
		    (spm_psp > (spm_psplim + 32u)) && ((spm_psp & 0x7u) == 0u)) {
			secure_psp = spm_psp;
			secure_psplim = spm_psplim;
		}
	}

	__disable_irq();
	__DSB();
	__ISB();

	tfm_sleep_bxns_entry(ns_ep, secure_psp, secure_psplim, secure_control);
}

/* Minimal core init after the CPU power-on reset of a deep-sleep wakeup. */
static TFM_RAMFUNC void tfm_fastboot_core_init(void)
{
	SCB->VTOR = S_CODE_START;

#if defined(__ITCM_PRESENT) && (__ITCM_PRESENT == 1U)
	TCM->ITCMCR |= SCB_ITCMCR_EN_Msk;
	ITGU->TGU_CTRL |= TGU_DBFEN_Msk | TGU_DEREN_Msk;
#endif
#if defined(__DTCM_PRESENT) && (__DTCM_PRESENT == 1U)
	TCM->DTCMCR |= SCB_DTCMCR_EN_Msk;
#endif

	__set_MSPLIM((uint32_t)(&__STACK_LIMIT));
	__TZ_set_STACKSEAL_S((uint32_t *)&__STACK_SEAL);
	__DSB();
	__ISB();
}

/* SRAM-resident fastboot resume entry, branched to from Reset_Handler. */
TFM_RAMFUNC __attribute__((noreturn)) void tfm_fastboot_resume(void)
{
	/* Keep every interrupt masked until ITNS/SAU and the secure priorities are
	 * back in place: the restored ISER bits must not be taken while the reset
	 * routing still points them to the secure state. */
	__disable_irq();
	tfm_fastboot_core_init();

	/* The cold boot enables the fault handlers in tfm_hal_platform_init(), which
	 * the resume path skips. Without them every BusFault/SecureFault escalates to
	 * HardFault and the failing service call looks like a silent hang. */
	tfm_sleep_enable_fault_handlers();

	if (tfm_sleep_context_restore() != 0) {
		/* No path back to BL2 cold boot: clear the marker and restart. */
		tfm_sleep_clear_aon_fastboot_flags();
		NVIC_SystemReset();
	}

	tfm_sleep_scheduler_state_reset();
	tfm_sleep_secure_hw_init();
	tfm_sleep_jump_to_ns();
}
