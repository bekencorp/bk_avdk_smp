// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include <stdint.h>

#include "deepsleep_fastboot_bl2.h"
#include "flash_layout.h"
#include "region_defs.h"
#include <soc/soc.h>

/*
 * This path runs before BL2 installs its stack and before SystemInit().
 * It must remain one naked assembly function: no C call, local variable,
 * writable static state, libc access or initialized driver is allowed.
 *
 * CONFIG_DIRECT_XIP:
 *   Validate the TF-M retention record, restore A/B flash remap, then jump.
 * CONFIG_OTA_OVERWRITE:
 *   Single execute slot — on deepsleep fast_boot just jump to TF-M. No
 *   retention record, no remap programming, no error logging.
 */
#define BL2_DS_ENTRY       __attribute__((section(".fix.reset_entry")))
#define BL2_DS_STRINGIFY_(x) #x
#define BL2_DS_STRINGIFY(x)  BL2_DS_STRINGIFY_(x)

#define BL2_DS_ANA_REG14_ADDR      (SOC_SYS_REG_BASE + (0x4E * 4))
#define BL2_DS_PMU_R0_ADDR         (SOC_AON_PMU_REG_BASE + (0x0 * 4))
#define BL2_DS_PMU_R25_ADDR        (SOC_AON_PMU_REG_BASE + (0x25 * 4))
#define BL2_DS_PMU_SHADOW_ADDR     (SOC_AON_PMU_REG_BASE + (0x7B * 4))
#define BL2_DS_FAST_BOOT_BIT       (0x2) /* AON PMU R0.fast_boot */
/* Same unlock sequence as aon_pmu_hal_r0_latch_to_r7b(). */
#define BL2_DS_PMU_LATCH_KEY1      (0x424B55AA)
#define BL2_DS_PMU_LATCH_KEY2      (0xBDB4AA55)

#if CONFIG_DIRECT_XIP
#define BL2_DS_RETENTION_MAGIC     (0x46584252) /* Little-endian "RBXF": Retention Boot XIP Flash record. */
#define BL2_DS_RETENTION_MAGIC_INV (0xB9A7BDAD)

#define BL2_DS_FLASH_PS_CTRL_ADDR  (SOC_FLASH_REG_BASE + (0x0B * 4))
#define BL2_DS_FLASH_OFFSET_ADDR   (SOC_FLASH_REG_BASE + (0x18 * 4))
#define BL2_DS_FLASH_CTRL_ADDR     (SOC_FLASH_REG_BASE + (0x19 * 4))

#define BL2_DS_EXPECTED_BEGIN      CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET
#define BL2_DS_EXPECTED_END        (CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET + CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE)
#define BL2_DS_EXPECTED_REMAP      (CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET - CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET)
#endif

/*
 * Returns r0=0 to the caller when fastboot is not applicable. On success it
 * installs the TF-M secure vector/MSP and jumps to its Reset_Handler.
 */
__attribute__((naked)) int BL2_DS_ENTRY bl2_deepsleep_fastboot(void)
{
#if CONFIG_DIRECT_XIP
	__asm volatile(
		/* Restore the ALO-to-core power switch before reading AON state. */
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_ANA_REG14_ADDR) "\n"
		"ldr r1, [r0]\n"
		"orr r1, r1, #1\n"
		"str r1, [r0]\n"
		"dsb\n"
		"isb\n"

		/* R7B is the retained shadow of AON PMU R0. */
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_PMU_SHADOW_ADDR) "\n"
		"ldr r1, [r0]\n"
		"tst r1, #" BL2_DS_STRINGIFY(BL2_DS_FAST_BOOT_BIT) "\n" /* fast_boot */
		"beq 9f\n"

		/*
		 * Validate the 32-byte TF-M retention record:
		 * magic, inverse magic, XOR, sum and fixed A/B geometry.
		 */
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_RETENTION_ADDR) "\n"
		"ldr r1, [r0, #0]\n"
		"ldr r8, =" BL2_DS_STRINGIFY(BL2_DS_RETENTION_MAGIC) "\n"
		"cmp r1, r8\n"
		"bne 9f\n"
		"ldr r1, [r0, #4]\n"
		"ldr r9, =" BL2_DS_STRINGIFY(BL2_DS_RETENTION_MAGIC_INV) "\n"
		"cmp r1, r9\n"
		"bne 9f\n"
		"ldr r2, [r0, #8]\n"         /* saved REG16 */
		"ldr r3, [r0, #12]\n"        /* saved REG17 */
		"ldr r4, [r0, #16]\n"        /* saved REG18 */
		"ldrb r5, [r0, #20]\n"       /* saved REG19[0] */
		"ldrb r6, [r0, #21]\n"       /* saved REG0B[25] */
		"ldrb r11, [r0, #22]\n"      /* reserved[0] */
		"ldrb r12, [r0, #23]\n"      /* reserved[1] */
		"ldr r7, [r0, #24]\n"        /* saved XOR */
		"ldr r10, [r0, #28]\n"       /* saved sum */
		"eor r9, r8, r2\n"
		"eor r9, r9, r3\n"
		"eor r9, r9, r4\n"
		"eor r9, r9, r5\n"
		"eor r9, r9, r6\n"
		"eor r9, r9, r11\n"
		"eor r9, r9, r12\n"
		"cmp r9, r7\n"
		"bne 9f\n"
		"add r9, r8, r2\n"
		"add r9, r9, r3\n"
		"add r9, r9, r4\n"
		"add r9, r9, r5\n"
		"add r9, r9, r6\n"
		"add r9, r9, r11\n"
		"add r9, r9, r12\n"
		"cmp r9, r10\n"
		"bne 9f\n"
		"cmp r6, #1\n"
		"bhi 9f\n"
		"cmp r5, #1\n"
		"bhi 9f\n"
		"ldr r8, =" BL2_DS_STRINGIFY(BL2_DS_EXPECTED_BEGIN) "\n"
		"cmp r2, r8\n"
		"bne 9f\n"
		"ldr r8, =" BL2_DS_STRINGIFY(BL2_DS_EXPECTED_END) "\n"
		"cmp r3, r8\n"
		"bne 9f\n"
		"ldr r8, =" BL2_DS_STRINGIFY(BL2_DS_EXPECTED_REMAP) "\n"
		"cmp r4, r8\n"
		"bne 9f\n"

		/* Program REG16/17/18 and REG0B[25] first; enable REG19 last. */
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_FLASH_OFFSET_ADDR - 8) "\n"
		"str r2, [r0, #0]\n"
		"str r3, [r0, #4]\n"
		"str r4, [r0, #8]\n"
		"dsb\n"
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_FLASH_PS_CTRL_ADDR) "\n"
		"ldr r1, [r0]\n"
		"bic r1, r1, #0x02000000\n"
		"orr r1, r1, r6, lsl #25\n"
		"str r1, [r0]\n"
		"dsb\n"
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_FLASH_CTRL_ADDR) "\n"
		"ldr r1, [r0]\n"
		"bic r1, r1, #1\n"
		"orr r1, r1, r5\n"
		"str r1, [r0]\n"
		"dsb\n"
		"isb\n"

		/* Validate the TF-M secure vector without touching RAM. */
		"ldr r0, =" BL2_DS_STRINGIFY(S_CODE_START) "\n"
		"ldr r1, [r0]\n"             /* target MSP */
		"ldr r2, [r0, #4]\n"         /* target Reset_Handler */
		"cmp r1, #0\n"
		"beq 9f\n"
		"tst r1, #7\n"
		"bne 9f\n"
		"ldr r3, =" BL2_DS_STRINGIFY(S_DATA_START) "\n"
		"cmp r1, r3\n"
		"bls 9f\n"
		"ldr r3, =" BL2_DS_STRINGIFY(S_DATA_LIMIT + 1) "\n"
		"cmp r1, r3\n"
		"bhi 9f\n"
		"tst r2, #1\n"
		"beq 9f\n"
		"bic r3, r2, #1\n"
		"ldr r4, =" BL2_DS_STRINGIFY(S_CODE_START) "\n"
		"cmp r3, r4\n"
		"blo 9f\n"
		"ldr r4, =" BL2_DS_STRINGIFY(S_CODE_LIMIT) "\n"
		"cmp r3, r4\n"
		"bhi 9f\n"

		/* Switch stacks only at the final branch; no instruction pushes. */
		"cpsid i\n"
		"ldr r3, =0xE000ED08\n"      /* SCB->VTOR */
		"str r0, [r3]\n"
		"mov r7, r2\n"
		"movs r2, #0\n"
		"msr msplim, r2\n"
		"msr psplim, r2\n"
		"msr msp, r1\n"
		"mov r0, r2\n"
		"mov r1, r2\n"
		"mov r3, r2\n"
		"mov r4, r2\n"
		"mov r5, r2\n"
		"mov r6, r2\n"
		"mov r8, r2\n"
		"mov r9, r2\n"
		"mov r10, r2\n"
		"mov r11, r2\n"
		"mov r12, r2\n"
		"mov lr, r2\n"
		"dsb\n"
		"isb\n"
		"cpsie i\n"
		"bx r7\n"

		/*
		 * Fastboot aborted: clear R0.fast_boot, then R25-latch into R7B
		 * (aon_pmu_hal_r0_latch_to_r7b). Still no stack use.
		 */
		"9:\n"
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_PMU_R0_ADDR) "\n"
		"ldr r1, [r0]\n"
		"bic r1, r1, #" BL2_DS_STRINGIFY(BL2_DS_FAST_BOOT_BIT) "\n"
		"str r1, [r0]\n"
		"dsb\n"
		"ldr r2, =" BL2_DS_STRINGIFY(BL2_DS_PMU_R25_ADDR) "\n"
		"ldr r3, =" BL2_DS_STRINGIFY(BL2_DS_PMU_LATCH_KEY1) "\n"
		"str r3, [r2]\n"
		"ldr r3, =" BL2_DS_STRINGIFY(BL2_DS_PMU_LATCH_KEY2) "\n"
		"str r3, [r2]\n"
		"dsb\n"
		"movs r0, #0\n"
		"bx lr\n"
	);
#elif CONFIG_OTA_OVERWRITE
	/*
	 * Compressed-overwrite: one execute slot (primary_all). On deepsleep
	 * fast_boot, skip BL2 image selection / decompress and jump straight
	 * to TF-M. Failures fall through silently to the normal BL2 path.
	 */
	__asm volatile(
		/* Restore the ALO-to-core power switch before reading AON state. */
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_ANA_REG14_ADDR) "\n"
		"ldr r1, [r0]\n"
		"orr r1, r1, #1\n"
		"str r1, [r0]\n"
		"dsb\n"
		"isb\n"

		/* R7B is the retained shadow of AON PMU R0. */
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_PMU_SHADOW_ADDR) "\n"
		"ldr r1, [r0]\n"
		"tst r1, #" BL2_DS_STRINGIFY(BL2_DS_FAST_BOOT_BIT) "\n"
		"beq 9f\n"

		/* Validate the TF-M secure vector without touching RAM. */
		"ldr r0, =" BL2_DS_STRINGIFY(S_CODE_START) "\n"
		"ldr r1, [r0]\n"             /* target MSP */
		"ldr r2, [r0, #4]\n"         /* target Reset_Handler */
		"cmp r1, #0\n"
		"beq 9f\n"
		"tst r1, #7\n"
		"bne 9f\n"
		"ldr r3, =" BL2_DS_STRINGIFY(S_DATA_START) "\n"
		"cmp r1, r3\n"
		"bls 9f\n"
		"ldr r3, =" BL2_DS_STRINGIFY(S_DATA_LIMIT + 1) "\n"
		"cmp r1, r3\n"
		"bhi 9f\n"
		"tst r2, #1\n"
		"beq 9f\n"
		"bic r3, r2, #1\n"
		"ldr r4, =" BL2_DS_STRINGIFY(S_CODE_START) "\n"
		"cmp r3, r4\n"
		"blo 9f\n"
		"ldr r4, =" BL2_DS_STRINGIFY(S_CODE_LIMIT) "\n"
		"cmp r3, r4\n"
		"bhi 9f\n"

		/* Switch stacks only at the final branch; no instruction pushes. */
		"cpsid i\n"
		"ldr r3, =0xE000ED08\n"      /* SCB->VTOR */
		"str r0, [r3]\n"
		"mov r7, r2\n"
		"movs r2, #0\n"
		"msr msplim, r2\n"
		"msr psplim, r2\n"
		"msr msp, r1\n"
		"mov r0, r2\n"
		"mov r1, r2\n"
		"mov r3, r2\n"
		"mov r4, r2\n"
		"mov r5, r2\n"
		"mov r6, r2\n"
		"mov r8, r2\n"
		"mov r9, r2\n"
		"mov r10, r2\n"
		"mov r11, r2\n"
		"mov r12, r2\n"
		"mov lr, r2\n"
		"dsb\n"
		"isb\n"
		"cpsie i\n"
		"bx r7\n"

		/* Same as DIRECT_XIP: clear R0.fast_boot then R25 latch. */
		"9:\n"
		"ldr r0, =" BL2_DS_STRINGIFY(BL2_DS_PMU_R0_ADDR) "\n"
		"ldr r1, [r0]\n"
		"bic r1, r1, #" BL2_DS_STRINGIFY(BL2_DS_FAST_BOOT_BIT) "\n"
		"str r1, [r0]\n"
		"dsb\n"
		"ldr r2, =" BL2_DS_STRINGIFY(BL2_DS_PMU_R25_ADDR) "\n"
		"ldr r3, =" BL2_DS_STRINGIFY(BL2_DS_PMU_LATCH_KEY1) "\n"
		"str r3, [r2]\n"
		"ldr r3, =" BL2_DS_STRINGIFY(BL2_DS_PMU_LATCH_KEY2) "\n"
		"str r3, [r2]\n"
		"dsb\n"
		"movs r0, #0\n"
		"bx lr\n"
	);
#else
	__asm volatile(
		"movs r0, #0\n"
		"bx lr\n"
	);
#endif
}
