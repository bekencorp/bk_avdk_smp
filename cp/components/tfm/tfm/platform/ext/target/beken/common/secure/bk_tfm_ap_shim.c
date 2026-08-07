// Copyright 2023-2028 Beken
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

#include <stdint.h>
#include "bk_tfm_ap_shim.h"
#include "partitions_gen.h"
#include "ram_regions.h"

#define STRINGIFY_VALUE_(value) #value
#define STRINGIFY_VALUE(value) STRINGIFY_VALUE_(value)
#define AP_SHIM_BASE CONFIG_AP_SPE_RAM_ADDR
#define AP_SHIM_STACK_TOP \
    (CONFIG_AP_SPE_RAM_ADDR + CONFIG_AP_SPE_RAM_SIZE - 0x200)

/* AP flash XIP NS alias: SOC_FLASH_DATA_BASE (0x04000000 + 0x10000000). */
#define AP_FLASH_XIP_BASE  0x14000000u

#if defined(CONFIG_PRIMARY_AP_APP_VIRTUAL_CODE_START)
#define AP_CORE0_NS_VECTOR  (AP_FLASH_XIP_BASE + CONFIG_PRIMARY_AP_APP_VIRTUAL_CODE_START)
#elif defined(CONFIG_APPLICATION1_PARTITION_OFFSET)
#define AP_CORE0_NS_VECTOR  (AP_FLASH_XIP_BASE + CONFIG_APPLICATION1_PARTITION_OFFSET)
#else
#define AP_CORE0_NS_VECTOR  0x14212000u
#endif

/*
 * AP boot shim.
 *
 * The AP application is linked and runs Non-Secure, but the AP core resets in
 * the Secure state. This shim is a small Secure stub that the secure world
 * copies into the reserved Secure RAM block (AP_SHIM_BASE, first 4K of smem3)
 * and points the AP boot vector at. On each AP core the shim:
 *   1. programs the SAU so the whole Non-Secure alias window is Non-Secure;
 *   2. routes all interrupts to the Non-Secure state;
 *   3. targets BusFault/HardFault/NMI to Non-Secure (AIRCR.BFHFNMINS) and
 *      prioritizes Secure (AIRCR.PRIS); there is no Secure runtime after the
 *      branch, so these faults must reach the Non-Secure vector table;
 *   4. grants the Non-Secure state access to the FPU;
 *   5. selects the per-core Non-Secure vector table by core id;
 *   6. loads VTOR_NS / MSP_NS and branches to the Non-Secure reset handler.
 * SYS Non-Secure attribute is applied earlier by CP PPHS config.
 *
 * The blob is fully position independent: the vector head, the two vector
 * parameter words and the code are copied verbatim to AP_SHIM_BASE. The secure
 * world patches the core1 vector word after the copy.
 *
 * Blob layout at AP_SHIM_BASE:
 *   +0x00  initial MSP (top of shim stack)
 *   +0x04  shim entry (thumb)
 *   +0x08  ns_vec0 : core0 Non-Secure vector table
 *   +0x0C  ns_vec1 : core1 Non-Secure vector table (patched by secure world)
 *   +0x10  code
 */
__asm__(
"    .section .rodata.ap_shim,\"a\"\n"
"    .align 3\n"
"    .global ap_shim_blob\n"
"    .global ap_shim_blob_end\n"
"ap_shim_blob:\n"
"    .word " STRINGIFY_VALUE(AP_SHIM_STACK_TOP) "\n"                   /* initial MSP */
"    .word (ap_shim_code - ap_shim_blob) + "
               STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"      /* entry|thumb */
"    .word 0x14212000\n"                                      /* ns_vec0 core0 */
"    .word 0\n"                                               /* ns_vec1 core1 */
"    .align 3\n"
"    .thumb\n"
"    .thumb_func\n"
"ap_shim_code:\n"
     /* SYS is already Non-Secure from CP PPHS apply; shim only switches the
      * core into the Non-Secure state. */
     /* SAU region0: 0x10000000..0xDFFFFFFF Non-Secure, then enable SAU */
"    ldr  r0, =0xE000EDD8\n"       /* SAU_RNR  */
"    movs r1, #0\n"
"    str  r1, [r0]\n"
"    ldr  r0, =0xE000EDDC\n"       /* SAU_RBAR */
"    ldr  r1, =0x10000000\n"
"    str  r1, [r0]\n"
"    ldr  r0, =0xE000EDE0\n"       /* SAU_RLAR */
"    ldr  r1, =0xDFFFFFE1\n"       /* limit 0xDFFFFFE0 | ENABLE */
"    str  r1, [r0]\n"
"    ldr  r0, =0xE000EDD0\n"       /* SAU_CTRL */
"    movs r1, #1\n"                /* ENABLE=1, ALLNS=0 */
"    str  r1, [r0]\n"
     /* all interrupts target Non-Secure (NVIC->ITNS[0..15] = 0xFFFFFFFF) */
"    ldr  r0, =0xE000E380\n"
"    ldr  r1, =0xFFFFFFFF\n"
"    movs r2, #16\n"
"1:  str  r1, [r0]\n"
"    adds r0, r0, #4\n"
"    subs r2, r2, #1\n"
"    bne  1b\n"
     /* AIRCR: target BusFault/HardFault/NMI to Non-Secure (BFHFNMINS) and
      * prioritize Secure (PRIS). read-modify-write with the VECTKEY, keeping
      * the low config/status half and never asserting SYSRESETREQ. */
"    ldr  r0, =0xE000ED0C\n"       /* SCB->AIRCR */
"    ldr  r1, [r0]\n"
"    lsls r1, r1, #16\n"           /* drop the read-back VECTKEYSTAT */
"    lsrs r1, r1, #16\n"           /* r1 = AIRCR & 0x0000FFFF */
"    ldr  r2, =0x05FA6000\n"       /* VECTKEY | PRIS(b14) | BFHFNMINS(b13) */
"    orrs r1, r1, r2\n"
"    str  r1, [r0]\n"
     /* grant Non-Secure access to the FPU (NSACR CP10/CP11) */
"    ldr  r0, =0xE000ED8C\n"
"    ldr  r1, [r0]\n"
"    orr  r1, r1, #0xC00\n"
"    str  r1, [r0]\n"
"    dsb\n"
"    isb\n"
     /* select per-core Non-Secure vector by AP core id: the per-core cpuid reg
      * packs id in bits[3:0] (AP core0 = 2, AP core1 = 3) and a 0xC magic in
      * bits[7:4]. Match id==3 to pick the core1 vector, everything else core0. */
"    ldr  r0, =0xE005001C\n"       /* AP per-core cpuid (PPB) */
"    ldr  r1, [r0]\n"
"    and  r1, r1, #0xF\n"          /* keep cpu_id: core0->2, core1->3 */
     /* dispatch by cpu_id */
"    cmp  r1, #3\n"
"    ldr  r2, =0x28100008\n"       /* &ns_vec0 (core0) */
"    bne  2f\n"
"    adds r2, r2, #4\n"            /* &ns_vec1 (core1) */
"2:  ldr  r3, [r2]\n"
     /* VTOR_NS = vector base */
"    ldr  r0, =0xE002ED08\n"       /* SCB_NS->VTOR */
"    str  r3, [r0]\n"
     /* MSP_NS + reset handler from the Non-Secure vector table */
"    ldr  r4, [r3]\n"
"    ldr  r5, [r3, #4]\n"
"    msr  MSP_NS, r4\n"
"    bic  r5, r5, #1\n"            /* BXNS target LSB=0 -> Non-Secure */
"    dsb\n"
"    isb\n"
"    bxns r5\n"
"    .ltorg\n"
"ap_shim_blob_end:\n"
);

extern const uint32_t ap_shim_blob[];
extern const uint32_t ap_shim_blob_end[];

/* Copy the shim into the reserved Secure RAM block and patch the core1 vector.
 * Must run after the AP power domain is up and the smem3 MPC has kept the first
 * block Secure. Returns the AP boot address (shim entry vector). */
uint32_t bk_ap_shim_install(uint32_t core1_ns_vector)
{
	volatile uint32_t *dst = (volatile uint32_t *)AP_SHIM_BASE;
	const uint32_t *src = ap_shim_blob;
	uint32_t words = (uint32_t)(ap_shim_blob_end - ap_shim_blob);
	uint32_t i;

	for (i = 0; i < words; i++) {
		dst[i] = src[i];
	}

	/* Patch core0/core1 Non-Secure vectors (blob word index 2/3, +0x08/+0x0C). */
	dst[2] = AP_CORE0_NS_VECTOR;
	dst[3] = core1_ns_vector;

	__asm__ volatile("dsb; isb");

	return AP_SHIM_BASE;
}
