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

#define TFM_SLEEP_AON_FAST_BOOT       (1u << 1)

extern uint32_t sys_is_enable_fast_boot(void);
extern uint32_t sys_is_running_from_deep_sleep(void);
extern void tcm_spe(void);

static bool tfm_sleep_warm_boot_applicable(void)
{
	if (sys_is_running_from_deep_sleep() == 0u) {
		return false;
	}

	// if (sys_is_enable_fast_boot() == 0u) {
	// 	return false;
	// }

	if ((REG_READ(SOC_AON_PMU_REG_BASE + (0x7bu << 2)) &
	     TFM_SLEEP_AON_FAST_BOOT) == 0u) {
		return false;
	}

	if (!tfm_sleep_context_is_valid()) {
		return false;
	}

	return true;
}

static void tfm_sleep_secure_hw_init(void)
{
	sau_and_idau_cfg();
	(void)tfm_hal_secure_static_mpu_init();
	tfm_hal_dma_init();

	/* Restore interrupt routing: deep sleep resets NVIC->ITNS to secure and
	 * ITNS is inaccessible from the non-secure side, so it must be redone
	 * here before returning to NSPE. */
	(void)nvic_interrupt_target_state_cfg();
}

__attribute__((naked)) __attribute__((noreturn))
static void tfm_sleep_bxns_entry(uint32_t ns_ep)
{
	__asm volatile(
		"clrm {r1-r12, r14, apsr}\n"
		"bic r0, r0, #1\n"
		"bxns r0\n"
	);
}

__attribute__((noreturn))
static void tfm_sleep_jump_to_ns(void)
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
			      SCB_AIRCR_PRIS_Msk);
	SCB->AIRCR = reg_val;

	SCB->NSACR |= SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk;

	tcm_spe();

	__disable_irq();
	__DSB();
	__ISB();
	tfm_sleep_bxns_entry(ns_ep);
}

void tfm_sleep_early_boot(void)
{
	if (!tfm_sleep_warm_boot_applicable()) {
		return;
	}

	if (tfm_sleep_context_restore() != 0) {
		return;
	}

	tfm_sleep_secure_hw_init();
	tfm_sleep_jump_to_ns();
}
