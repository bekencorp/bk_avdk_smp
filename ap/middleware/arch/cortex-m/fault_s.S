/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 *
 * @brief Fault handler installed in the fault vectors
 *
 * Entry point for the HardFault, MemManageFault, BusFault, UsageFault,
 * SecureFault and Debug Monitor exceptions.
 *
 * The function supplies the values of
 * - the MSP
 * - the PSP
 * - the EXC_RETURN value
 * - callee saved register state (r4-r11, psp)
 * as parameters to the b_arm_fault() C function that will perform the
 * rest of the fault handling:
 *    (i.e. b_arm_fault(MSP, PSP, EXC_RETURN, CALLEE_REGS)).
 * Provides these symbols:
 *
 *   b_arm_hard_fault
 *   b_arm_mpu_fault
 *   b_arm_bus_fault
 *   b_arm_usage_fault
 *   b_arm_secure_fault
 *   b_arm_debug_monitor
 *   b_arm_exc_spurious
 */

	.text
	.syntax unified
	.thumb

	.section .vector_table,"a",%progbits
	.align 2
	.globl _default_handler_

_default_handler_:
	mrs r0, MSP
	mrs r1, PSP
	push {r0, lr}
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Build _callee_saved_t. To match the struct
	 * definition we push the psp & then r11-r4
	 */
	push { r1, r2 }
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	mov  r3, r11
	mov  r2, r10
	push {r2, r3}
	mov  r3, r9
	mov  r2, r8
	push {r2, r3}
	push {r4-r7}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	push {r4-r11}
#endif
	mov  r3, sp /* pointer to _callee_saved_t */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	mov r2, lr /* EXC_RETURN */
	bl b_arm_fault
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* We do not need to restore any register state here
	 * because we did not use any callee-saved registers
	 * in this routine. Therefore, we can just reset
	 * the MSP to its value prior to entering the function
	 */
	add sp, #40
#endif
	pop {r0, pc}

	.end
