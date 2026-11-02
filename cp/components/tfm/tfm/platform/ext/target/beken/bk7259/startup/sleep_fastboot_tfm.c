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
#include "cp_lv_reserve.h"
#include "aon_pmu_hal.h"
#include "aon_pmu_ll.h"

extern uint32_t sys_is_running_from_deep_sleep(void);
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

static TFM_RAMFUNC void tfm_sleep_secure_hw_init(void)
{
	sau_and_idau_cfg();
	(void)tfm_hal_secure_static_mpu_init();
	tfm_hal_dma_init();

	/* Restore interrupt routing: deep sleep resets NVIC->ITNS to secure and
	 * ITNS is inaccessible from the non-secure side, so it must be redone
	 * here before returning to NSPE. */
	(void)nvic_interrupt_target_state_cfg();
}

__attribute__((section(".tfm_fastboot_code"), naked, noreturn))
static void tfm_sleep_bxns_entry(uint32_t ns_ep)
{
	/*
	 * Two SECURE-banked processor states must be sane before BXNS, or the
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
	 * clrm below clears r1-r12,r14,apsr (not r0/PRIMASK), so it is safe to use
	 * r1 before it and to clear the masks after it.
	 */
	__asm volatile(
		"mrs r1, control\n"
		"bic r1, r1, #4\n"      /* clear CONTROL.FPCA (secure) */
		"msr control, r1\n"
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

	if (tfm_sleep_context_apply_ppc() != 0) {
		while (1) {
		}
	}

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

	__disable_irq();
	__DSB();
	__ISB();

	tfm_sleep_bxns_entry(ns_ep);
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
	tfm_fastboot_core_init();

	if (sys_is_running_from_deep_sleep() != 0u) {
		cp_lv_reserve_restore();
	}
	else {
		cp_lv_reserve_discard();
	}

	if (tfm_sleep_context_restore() != 0) {
		/* No path back to BL2 cold boot: clear the marker and restart. */
		tfm_sleep_clear_aon_fastboot_flags();
		NVIC_SystemReset();
	}

	tfm_sleep_secure_hw_init();
	tfm_sleep_jump_to_ns();
}
