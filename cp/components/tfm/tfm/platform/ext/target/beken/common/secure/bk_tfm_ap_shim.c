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

/* AP-private M55 DTCM security attribution, programmed while still Secure. */
#define AP_DTGU_LUT_ADDR        0xE001E610
#define AP_DTGU_ALL_NS          0xFFFF

#define AP_CORE_ID_ADDR         0xE005001C
#define AP_L2_CONTROL_ADDR      0xA0000100
#define AP_L2_AUX_CTRL_ADDR     0xA0000104
#define AP_L2_INT_CLEAR_ADDR    0xA0000220
#define AP_L2_INT_MASK_ADDR     0xA0000214
#define AP_L2_INVALID_WAY_ADDR  0xA000077C
#define AP_L2_CACHE_SYNC_ADDR   0xA0000730
#define AP_L2_ADDR_FILTER_ADDR  0xA0000C00
#define AP_L2_AUX_CTRL_VALUE    0x02020000
#define AP_L2_ALL_WAYS          0x0000FFFF
#define AP_L2_INT_BITS          0x000001FF
#define AP_L2_WAIT_COUNT        0x00100000

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
 *   1. initializes and enables the shared L2 cache from Secure AP core0;
 *   2. assigns all DTCM blocks Non-Secure (CP enables DTCM before reset release);
 *   3. programs the SAU so the whole Non-Secure alias window is Non-Secure;
 *   4. routes all interrupts to the Non-Secure state;
 *   5. targets BusFault/HardFault/NMI to Non-Secure (AIRCR.BFHFNMINS) and
 *      prioritizes Secure (AIRCR.PRIS); ordinary Non-Secure faults must reach
 *      the Non-Secure vector table (AP NS coredump);
 *   6. grants the Non-Secure state access to the FPU;
 *   7. installs a minimal AP Secure world: points the Secure VTOR at a
 *      resident Secure vector table and enables SecureFault (SHCSR). AP
 *      core-local security violations (SecureFault) and Secure fault
 *      escalations (Secure HardFault/MemManage/BusFault/UsageFault/NMI) are
 *      then trapped in Secure state and dumped over UART0 by the resident
 *      handler, instead of being lost. UART0 is used (not the secure UART1)
 *      because the AP is forced Non-Secure as a bus master and cannot reach
 *      the CP-owned secure UART1. This is the only Secure
 *      runtime left on the AP core after the branch;
 *   8. selects the per-core Non-Secure vector table by core id;
 *   9. loads VTOR_NS / MSP_NS and branches to the Non-Secure reset handler.
 * SYS Non-Secure attribute is applied earlier by CP PPHS config.
 *
 * The blob is fully position independent: the vector head, the two vector
 * parameter words, the code, the Secure vector table and the Secure fault
 * handler are copied verbatim to AP_SHIM_BASE. The secure world patches the
 * core1 vector word after the copy. The Secure vector table entries and the
 * boot entry encode absolute addresses using the compile-time AP_SHIM_BASE
 * (CONFIG_AP_SPE_RAM_ADDR), and the handler is self-contained (pure MMIO, no
 * external calls) so it keeps working from the copied location.
 *
 * The image is authored as two assembly modules for readability: this boot
 * shim (ap_shim_blob/ap_shim_code) and the AP Secure fault dump unit
 * (ap_sec_dump, defined just below). Both share the .rodata.ap_shim section and
 * are laid out contiguously, so they are still copied as one image and
 * ap_shim_blob_end still bounds the whole thing.
 *
 * Blob layout at AP_SHIM_BASE:
 *   +0x00  initial MSP (top of shim stack, also the Secure fault stack)
 *   +0x04  shim entry (thumb)
 *   +0x08  ns_vec0 : core0 Non-Secure vector table
 *   +0x0C  ns_vec1 : core1 Non-Secure vector table (patched by secure world)
 *   +0x10  code
 *   ...    ap_sec_vtable : Secure vector table (128-byte aligned)
 *   ...    ap_sec_fault  : resident Secure fault handler + UART0 dump helpers
 */
__asm__(
"    .section .rodata.ap_shim,\"a\"\n"
"    .balign 128\n"                  /* blob base 128-aligned so ap_sec_vtable
                                        lands 128-aligned after copy (VTOR) */
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
     /* Initialize the shared PL310 only on AP core0 while still Secure. If
      * invalidate does not complete before the timeout, leave L2 disabled and
      * continue booting so a controller failure cannot trap the AP in reset. */
"    ldr  r0, =" STRINGIFY_VALUE(AP_CORE_ID_ADDR) "\n"
"    ldr  r2, [r0]\n"
"    and  r2, r2, #0xF\n"
"    cmp  r2, #2\n"
"    bne  9f\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_CONTROL_ADDR) "\n"
"    ldr  r1, [r0]\n"
"    tst  r1, #1\n"
"    bne  9f\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_AUX_CTRL_ADDR) "\n"
"    ldr  r1, =" STRINGIFY_VALUE(AP_L2_AUX_CTRL_VALUE) "\n"
"    str  r1, [r0]\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_ADDR_FILTER_ADDR) "\n"
"    movs r1, #0\n"
"    str  r1, [r0]\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_INT_CLEAR_ADDR) "\n"
"    ldr  r1, =" STRINGIFY_VALUE(AP_L2_INT_BITS) "\n"
"    str  r1, [r0]\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_INT_MASK_ADDR) "\n"
"    str  r1, [r0]\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_INVALID_WAY_ADDR) "\n"
"    ldr  r1, =" STRINGIFY_VALUE(AP_L2_ALL_WAYS) "\n"
"    str  r1, [r0]\n"
"    ldr  r2, =" STRINGIFY_VALUE(AP_L2_WAIT_COUNT) "\n"
"8:  ldr  r1, [r0]\n"
"    cbz  r1, 7f\n"
"    subs r2, r2, #1\n"
"    bne  8b\n"
"    b    9f\n"
"7:  ldr  r0, =" STRINGIFY_VALUE(AP_L2_CACHE_SYNC_ADDR) "\n"
"    movs r1, #0\n"
"    str  r1, [r0]\n"
"    dsb\n"
"    isb\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_L2_CONTROL_ADDR) "\n"
"    movs r1, #1\n"
"    str  r1, [r0]\n"
"    dsb\n"
"    isb\n"
"9:\n"
     /* CP enables this core's DTCM through cpu0_init_dtcm_en before releasing
      * reset. Grant every DTGU block to Non-Secure before the NS reset handler
      * copies .dtcm to the 0x30000000 alias. */
"    ldr  r0, =" STRINGIFY_VALUE(AP_DTGU_LUT_ADDR) "\n"
"    ldr  r1, =" STRINGIFY_VALUE(AP_DTGU_ALL_NS) "\n"
"    str  r1, [r0]\n"
"    dsb\n"
"    isb\n"
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
     /* install the minimal AP Secure world: point the Secure VTOR at the
      * resident ap_sec_vtable (absolute addr = offset + AP_SHIM_BASE) and
      * enable SecureFault so AP core-local security violations trap Secure and
      * are dumped over UART0 (Secure HardFault also vectors here). BFHFNMINS
      * stays 1 so ordinary NS faults still reach the AP NS coredump. */
"    ldr  r0, =0xE000ED08\n"       /* SCB_S->VTOR (Secure) */
"    adr  r1, ap_sec_vtable\n"     /* PC-relative -> absolute addr in copy */
"    str  r1, [r0]\n"
"    ldr  r0, =0xE000ED24\n"       /* SCB_S->SHCSR (Secure) */
"    ldr  r1, [r0]\n"
"    orr  r1, r1, #0x80000\n"      /* SECUREFAULTENA (bit19) */
"    str  r1, [r0]\n"
"    dsb\n"
"    isb\n"
     /* select per-core Non-Secure vector by AP core id: the per-core cpuid reg
      * packs id in bits[3:0] (AP core0 = 2, AP core1 = 3) and a 0xC magic in
      * bits[7:4]. Match id==3 to pick the core1 vector, everything else core0. */
"    ldr  r0, =" STRINGIFY_VALUE(AP_CORE_ID_ADDR) "\n"
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
);

/* ---------------------------------------------------------------------------
 * AP Secure fault dump unit.
 *
 * Encapsulated as its own assembly module so it is not interleaved with the
 * boot shim above. It is emitted into the SAME section (.rodata.ap_shim), right
 * after the shim, so bk_ap_shim_install() still copies shim + dump as a single
 * contiguous image into AP_SHIM_BASE. Keeping one copied image is exactly what
 * lets the shim reach ap_sec_vtable with a short PC-relative `adr`, and lets the
 * vector table encode absolute targets as (label - ap_shim_blob) + AP_SHIM_BASE.
 *
 * Contents: the resident Secure vector table, the self-contained fault handler
 * (dumps the basic registers over UART0 then reboots via AON_WDT) and the UART0
 * print helpers + string table. Pure MMIO, no external calls, so it keeps
 * working from the copied location. Vectored from SecureFault (AP core-local
 * security violation) and Secure fault escalations (Secure HardFault/MemManage/
 * BusFault/UsageFault/NMI). Runs on the Secure MSP set at the blob head.
 * ------------------------------------------------------------------------- */
__asm__(
"    .section .rodata.ap_shim,\"a\"\n"
"    .balign 128\n"                  /* vtable 128-aligned (Secure VTOR) */
"    .global ap_sec_dump\n"
"ap_sec_dump:\n"
"ap_sec_vtable:\n"
"    .word 0\n"                                                                    /* 0  initial SP (unused at runtime) */
"    .word 0\n"                                                                    /* 1  Reset       (unused) */
"    .word (ap_sec_fault - ap_shim_blob) + " STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"  /* 2  NMI */
"    .word (ap_sec_fault - ap_shim_blob) + " STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"  /* 3  HardFault */
"    .word (ap_sec_fault - ap_shim_blob) + " STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"  /* 4  MemManage */
"    .word (ap_sec_fault - ap_shim_blob) + " STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"  /* 5  BusFault */
"    .word (ap_sec_fault - ap_shim_blob) + " STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"  /* 6  UsageFault */
"    .word (ap_sec_fault - ap_shim_blob) + " STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) " + 1\n"  /* 7  SecureFault */
"    .thumb\n"
"    .thumb_func\n"
"ap_sec_fault:\n"
"    mov  r7, lr\n"                /* preserve EXC_RETURN before any BL */
"    push {r4-r11}\n"              /* safe fallback snapshot; DCRS=0 uses stacked values */
"    mov  r8, sp\n"                /* r8 -> saved {r4..r11}: [r8,#0]=r4 ... [#28]=r11 */
     /* pick the faulting exception frame SP (banked S/NS, MSP/PSP) */
"    tst  r7, #0x40\n"             /* EXC_RETURN.S (bit6): 1 = came from Secure */
"    beq  30f\n"
     /* From Secure, select the S banked SP via EXC_RETURN.SPSEL. For MSP the
      * handler's push above has moved SP by 0x20, so recover the hardware-frame
      * base from the saved snapshot pointer instead of reading current MSP. */
"    tst  r7, #0x04\n"             /* SPSEL (bit2) */
"    ite  eq\n"
"    addeq r6, r8, #0x20\n"
"    mrsne r6, psp\n"
"    b    31f\n"
     /* from Non-Secure: EXC_RETURN.SPSEL is unreliable across the NS->S
      * transition (it can report MSP while the NS thread actually ran on PSP),
      * so use CONTROL_NS.SPSEL, which is authoritative for the NS thread stack
      * and is not altered by taking the exception. NS handler mode uses MSP_NS. */
"30: tst  r7, #0x08\n"             /* EXC_RETURN.Mode (bit3): 0=handler,1=thread */
"    beq  32f\n"
"    mrs  r0, control_ns\n"
"    tst  r0, #0x02\n"             /* CONTROL_NS.SPSEL (bit1): 1 = PSP */
"    bne  33f\n"
"32: mrs  r6, msp_ns\n"
"    b    31f\n"
"33: mrs  r6, psp_ns\n"
"31:\n"
     /* EXC_RETURN.FType (bit4): 0 means an extended FP frame precedes the
      * basic R0-R3/R12/LR/PC/xPSR frame. Point r6 at that basic frame. */
"    tst  r7, #0x10\n"
"    bne  34f\n"
"    adds r6, r6, #0x48\n"
"34:\n"
     /* EXC_RETURN.DCRS=0 means the hardware stacked an Additional State
      * Context before the basic frame:
      *   signature, reserved, R4-R11, R0-R3, R12, LR, PC, xPSR.
      * Advance r6 by 10 words so the fixed offsets below address R0..xPSR.
      * Keep r8 on the handler's safe software snapshot until r6 is validated. */
"    tst  r7, #0x20\n"             /* DCRS (bit5): 0 = additional context present */
"    bne  35f\n"
"    adds r6, r6, #0x28\n"
"35:\n"
     /* header + fault status registers */
"    adr  r0, 90f\n"
"    bl   ap_sec_puts\n"
"    adr  r0, 91f\n"               /* ER   = EXC_RETURN */
"    mov  r1, r7\n"
"    bl   ap_sec_kv\n"
"    mrs  r1, control_ns\n"        /* CONTROL_NS: bit1=SPSEL used to pick the frame */
"    adr  r0, 122f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, =0xE000EDE4\n"       /* SFSR */
"    ldr  r1, [r1]\n"
"    adr  r0, 92f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, =0xE000EDE8\n"       /* SFAR */
"    ldr  r1, [r1]\n"
"    adr  r0, 93f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, =0xE000ED28\n"       /* CFSR */
"    ldr  r1, [r1]\n"
"    adr  r0, 94f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, =0xE000ED2C\n"       /* HFSR */
"    ldr  r1, [r1]\n"
"    adr  r0, 95f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, =0xE000ED38\n"       /* BFAR */
"    ldr  r1, [r1]\n"
"    adr  r0, 96f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, =0xE000ED34\n"       /* MMFAR */
"    ldr  r1, [r1]\n"
"    adr  r0, 97f\n"
"    bl   ap_sec_kv\n"
"    adr  r0, 98f\n"               /* FSP  = frame SP */
"    mov  r1, r6\n"
"    bl   ap_sec_kv\n"
     /* banked stack pointers (S and NS, MSP and PSP) */
"    mrs  r1, msp\n"
"    adr  r0, 110f\n"
"    bl   ap_sec_kv\n"
"    mrs  r1, psp\n"
"    adr  r0, 111f\n"
"    bl   ap_sec_kv\n"
"    mrs  r1, msp_ns\n"
"    adr  r0, 112f\n"
"    bl   ap_sec_kv\n"
"    mrs  r1, psp_ns\n"
"    adr  r0, 113f\n"
"    bl   ap_sec_kv\n"
     /* (R4-R11 are printed further down, between the stacked R3 and R12, via
      * ap_sec_pr_r4r11, so the general-purpose registers read R0..R11 in order.) */
     /* Only dereference an aligned complete 32-byte basic frame in memory this
      * handler can read. The AP is a forced-NS bus master, so its stacks live
      * in the SMEM Non-Secure aliases, DTCM, or its Secure shim block. */
"    tst  r6, #3\n"
"    bne  41f\n"
"    ldr  r0, =0x38000000\n"       /* SMEM Non-Secure alias, cacheable */
"    subs r1, r6, r0\n"
"    ldr  r0, =0x001FFFE0\n"       /* region size - complete frame size */
"    cmp  r1, r0\n"
"    bls  40f\n"
"    ldr  r0, =0x3c000000\n"       /* SMEM Non-Secure alias, non-cacheable */
"    subs r1, r6, r0\n"
"    ldr  r0, =0x001FFFE0\n"
"    cmp  r1, r0\n"
"    bls  40f\n"
"    ldr  r0, =0x20000000\n"       /* DTCM */
"    subs r1, r6, r0\n"
"    ldr  r0, =0x0007FFE0\n"
"    cmp  r1, r0\n"
"    bls  40f\n"
"    ldr  r0, =" STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_ADDR) "\n"   /* AP Secure shim block */
"    subs r1, r6, r0\n"
"    ldr  r0, =(" STRINGIFY_VALUE(CONFIG_AP_SPE_RAM_SIZE) " - 0x20)\n"
"    cmp  r1, r0\n"
"    bls  40f\n"
     /* frame invalid: still emit the live callee-saved snapshot, then flag it */
"41:\n"
"    bl   ap_sec_pr_r4r11\n"
"    adr  r0, 99f\n"               /* frame invalid */
"    bl   ap_sec_puts\n"
"    b    ap_sec_spin\n"
"40:\n"
     /* With an Additional State Context, print the hardware-stacked faulting
      * R4-R11 instead of the handler-entry values. Their base is 0x20 bytes
      * before the adjusted basic-frame pointer in r6. */
"    tst  r7, #0x20\n"
"    it   eq\n"
"    subeq r8, r6, #0x20\n"
"    ldr  r1, [r6, #0x00]\n"       /* stacked R0 */
"    adr  r0, 103f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r6, #0x04]\n"       /* stacked R1 */
"    adr  r0, 104f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r6, #0x08]\n"       /* stacked R2 */
"    adr  r0, 105f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r6, #0x0C]\n"       /* stacked R3 */
"    adr  r0, 106f\n"
"    bl   ap_sec_kv\n"
     /* faulting callee-saved R4-R11 here so the dump reads R0..R11 ascending */
"    bl   ap_sec_pr_r4r11\n"
"    ldr  r1, [r6, #0x10]\n"       /* stacked R12 */
"    adr  r0, 107f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r6, #0x18]\n"       /* stacked PC */
"    adr  r0, 100f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r6, #0x14]\n"       /* stacked LR */
"    adr  r0, 101f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r6, #0x1C]\n"       /* stacked xPSR */
"    adr  r0, 102f\n"
"    bl   ap_sec_kv\n"
"    b    ap_sec_handoff\n"
     /* Publish the captured Secure-fault context to the callback/context
      * addresses carried in NS vector reserved slots 8..10, then call the NS
      * callback with BLXNS. The callback enters the existing AP coredump path. */
"ap_sec_handoff:\n"
"    bl   ap_sec_set_reboot_reason\n"
"    ldr  r0, =0xE002ED08\n"       /* SCB_NS->VTOR */
"    ldr  r0, [r0]\n"
"    ldr  r4, [r0, #" STRINGIFY_VALUE(AP_SEC_DUMP_VECTOR_CALLBACK_OFFSET) "]\n"
"    ldr  r5, [r0, #" STRINGIFY_VALUE(AP_SEC_DUMP_VECTOR_CONTEXT_OFFSET) "]\n"
"    ldr  r1, [r0, #" STRINGIFY_VALUE(AP_SEC_DUMP_VECTOR_ABI_OFFSET) "]\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_SEC_DUMP_ABI_INFO) "\n"
"    cmp  r1, r0\n"
"    bne  ap_sec_spin\n"
     /* Slot 9 points at a const NS word containing the context-array address.
      * Validate the carrier before dereferencing an NS-controlled vector word. */
"    ldr  r0, =0x10000000\n"
"    cmp  r5, r0\n"
"    blo  ap_sec_spin\n"
"    ldr  r0, =0xE0000000\n"
"    cmp  r5, r0\n"
"    bhs  ap_sec_spin\n"
"    ldr  r5, [r5]\n"
     /* callback must be in the SAU Non-Secure window */
"    bic  r4, r4, #1\n"
"    ldr  r0, =0x10000000\n"
"    cmp  r4, r0\n"
"    blo  ap_sec_spin\n"
"    ldr  r0, =0xE0000000\n"
"    cmp  r4, r0\n"
"    bhs  ap_sec_spin\n"
     /* Select per-core context (AP core0 id=2, core1 id=3). */
"    ldr  r0, =" STRINGIFY_VALUE(AP_CORE_ID_ADDR) "\n"
"    ldr  r3, [r0]\n"
"    and  r3, r3, #0xF\n"
"    cmp  r3, #3\n"
"    it   eq\n"
"    addeq r5, r5, #" STRINGIFY_VALUE(AP_SEC_DUMP_CONTEXT_SIZE) "\n"
     /* context must be writable NS SMEM or DTCM */
"    ldr  r0, =0x38000000\n"
"    subs r1, r5, r0\n"
"    ldr  r0, =0x00200000\n"
"    cmp  r1, r0\n"
"    blo  70f\n"
"    ldr  r0, =0x3C000000\n"
"    subs r1, r5, r0\n"
"    ldr  r0, =0x00200000\n"
"    cmp  r1, r0\n"
"    blo  70f\n"
"    ldr  r0, =0x30000000\n"
"    subs r1, r5, r0\n"
"    ldr  r0, =0x00080000\n"
"    cmp  r1, r0\n"
"    bhs  ap_sec_spin\n"
"70:\n"
     /* Clear commit first, then fill the fixed 48-word ABI. */
"    movs r0, #0\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_MAGIC) "]\n"
"    movs r0, #" STRINGIFY_VALUE(AP_SEC_DUMP_CONTEXT_VERSION) "\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_VERSION) "]\n"
"    movs r0, #" STRINGIFY_VALUE(AP_SEC_DUMP_CONTEXT_SIZE) "\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_SIZE) "]\n"
"    str  r3, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_CORE_ID) "]\n"
     /* source flags from EXC_RETURN */
"    movs r0, #0\n"
"    tst  r7, #0x40\n"
"    it   ne\n"
"    orrne r0, r0, #" STRINGIFY_VALUE(AP_SEC_CTX_FLAG_SOURCE_SECURE) "\n"
"    tst  r7, #0x08\n"
"    it   ne\n"
"    orrne r0, r0, #" STRINGIFY_VALUE(AP_SEC_CTX_FLAG_SOURCE_THREAD) "\n"
"    tst  r7, #0x40\n"             /* Secure source: EXC_RETURN.SPSEL is authoritative */
"    beq  71f\n"
"    tst  r7, #0x04\n"
"    bne  72f\n"
"    b    73f\n"
"71: tst  r7, #0x08\n"             /* NS handler source always used MSP_NS */
"    beq  73f\n"
"    mrs  r1, control_ns\n"        /* NS thread source follows CONTROL_NS.SPSEL */
"    tst  r1, #0x02\n"
"    beq  73f\n"
"72: orr  r0, r0, #" STRINGIFY_VALUE(AP_SEC_CTX_FLAG_SOURCE_PSP) "\n"
"73:\n"
"    tst  r7, #0x20\n"
"    it   eq\n"
"    orreq r0, r0, #" STRINGIFY_VALUE(AP_SEC_CTX_FLAG_DCRS_STACKED) "\n"
"    tst  r7, #0x10\n"
"    it   eq\n"
"    orreq r0, r0, #" STRINGIFY_VALUE(AP_SEC_CTX_FLAG_FP_STACKED) "\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_FLAGS) "]\n"
"    str  r7, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_EXC_RETURN) "]\n"
"    str  r6, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_FRAME_SP) "]\n"
"    movs r0, #1\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_FRAME_VALID) "]\n"
     /* R0-R3 and R12 from the hardware basic frame. */
"    ldr  r0, [r6, #0x00]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x00]\n"
"    ldr  r0, [r6, #0x04]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x04]\n"
"    ldr  r0, [r6, #0x08]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x08]\n"
"    ldr  r0, [r6, #0x0C]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x0C]\n"
     /* R4-R11 from the selected live/additional-context snapshot. */
"    ldr  r0, [r8, #0x00]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x10]\n"
"    ldr  r0, [r8, #0x04]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x14]\n"
"    ldr  r0, [r8, #0x08]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x18]\n"
"    ldr  r0, [r8, #0x0C]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x1C]\n"
"    ldr  r0, [r8, #0x10]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x20]\n"
"    ldr  r0, [r8, #0x14]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x24]\n"
"    ldr  r0, [r8, #0x18]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x28]\n"
"    ldr  r0, [r8, #0x1C]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x2C]\n"
"    ldr  r0, [r6, #0x10]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_R0) " + 0x30]\n"
     /* LR/PC/xPSR and the restored source SP. */
"    ldr  r0, [r6, #0x14]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_LR) "]\n"
"    ldr  r0, [r6, #0x18]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_PC) "]\n"
"    ldr  r0, [r6, #0x1C]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_XPSR) "]\n"
"    add  r1, r6, #0x20\n"
"    tst  r0, #0x200\n"
"    it   ne\n"
"    addne r1, r1, #4\n"
"    str  r1, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_SP) "]\n"
     /* Banked stack/control state and Secure fault status. */
"    mrs  r0, msp\n"
"    add  r0, r0, #0x20\n"         /* undo handler's saved {r4..r11} */
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_MSP_S) "]\n"
"    mrs  r0, psp\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_PSP_S) "]\n"
"    mrs  r0, msp_ns\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_MSP_NS) "]\n"
"    mrs  r0, psp_ns\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_PSP_NS) "]\n"
"    mrs  r0, control\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_CONTROL_S) "]\n"
"    mrs  r0, control_ns\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_CONTROL_NS) "]\n"
"    ldr  r0, =0xE000EDE4\n"
"    ldr  r0, [r0]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_SFSR) "]\n"
"    ldr  r0, =0xE000EDE8\n"
"    ldr  r0, [r0]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_SFAR) "]\n"
"    ldr  r0, =0xE000ED28\n"
"    ldr  r0, [r0]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_CFSR_S) "]\n"
"    ldr  r0, =0xE000ED2C\n"
"    ldr  r0, [r0]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_HFSR_S) "]\n"
"    ldr  r0, =0xE000ED38\n"
"    ldr  r0, [r0]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_BFAR_S) "]\n"
"    ldr  r0, =0xE000ED34\n"
"    ldr  r0, [r0]\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_MMFAR_S) "]\n"
"    mrs  r0, ipsr\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_IPSR) "]\n"
"    mrs  r0, primask\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_PRIMASK_S) "]\n"
"    mrs  r0, basepri\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_BASEPRI_S) "]\n"
"    mrs  r0, faultmask\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_FAULTMASK_S) "]\n"
"    movs r0, #0\n"                /* FPSCR capture can itself fault if FP is unavailable */
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_FPSCR_S) "]\n"
"    mrs  r0, primask_ns\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_PRIMASK_NS) "]\n"
"    mrs  r0, basepri_ns\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_BASEPRI_NS) "]\n"
"    mrs  r0, faultmask_ns\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_FAULTMASK_NS) "]\n"
"    dsb\n"
"    ldr  r0, =" STRINGIFY_VALUE(AP_SEC_DUMP_CONTEXT_MAGIC) "\n"
"    str  r0, [r5, #" STRINGIFY_VALUE(AP_SEC_CTX_MAGIC) "]\n"
"    dsb\n"
"    isb\n"
     /* r0 carries the sole intentional cross-domain argument. */
"    mov  r0, r5\n"
"    movs r1, #0\n"
"    mov  r2, r1\n"
"    mov  r3, r1\n"
"    mov  r5, r1\n"
"    mov  r6, r1\n"
"    mov  r7, r1\n"
"    mov  r8, r1\n"
"    mov  r9, r1\n"
"    mov  r10, r1\n"
"    mov  r11, r1\n"
"    mov  r12, r4\n"
"    mov  r4, r1\n"
"    blxns r12\n"
     /* A correctly functioning coredump callback never returns. */
"    b    ap_sec_spin\n"
     /* Persist AP SecureFault reason before either NS handoff or watchdog. */
"    .thumb_func\n"
"ap_sec_set_reboot_reason:\n"
"    ldr  r0, =0x54000000\n"       /* AON PMU R0, AP NS alias */
"    ldr  r1, [r0]\n"
"    ldr  r2, =0x7F000000\n"
"    bics r1, r1, r2\n"
"    ldr  r2, =0x15000000\n"       /* RESET_SOURCE_SECURE_FAULT */
"    orrs r1, r1, r2\n"
"    str  r1, [r0]\n"
"    ldr  r0, =0x54000094\n"       /* AON PMU R25 latch */
"    ldr  r1, =0x424B55AA\n"
"    str  r1, [r0]\n"
"    ldr  r1, =0xBDB4AA55\n"
"    str  r1, [r0]\n"
"    dsb\n"
"    bx   lr\n"
     /* Fall back to an immediate AON watchdog reset if the NS callback cannot
      * be validated or unexpectedly returns. */
"ap_sec_spin:\n"
"    bl   ap_sec_set_reboot_reason\n"
"    dsb\n"
"    ldr  r0, =0x54000600\n"       /* AON_WDT config (NS alias of 0x44000600) */
"    ldr  r1, =0x005A000A\n"       /* KEY_1ST(0x5A)<<16 | PERIOD_MIN(0xA) */
"    str  r1, [r0]\n"
"    ldr  r1, =0x00A5000A\n"       /* KEY_2ND(0xA5)<<16 | PERIOD_MIN(0xA) */
"    str  r1, [r0]\n"
"    dsb\n"
"60: b    60b\n"                   /* wait for the watchdog reset to fire */
     /* Dump R4-R11 through r8. The valid DCRS=0 path redirects r8 to the
      * hardware Additional State Context; other paths use the safe snapshot
      * taken at handler entry. Kept as a subroutine so R4-R11 can be placed
      * between stacked R3 and R12 for a natural R0..R11 ascending dump.
      * ap_sec_kv leaves r6/r8 untouched. */
"    .thumb_func\n"
"ap_sec_pr_r4r11:\n"
"    push {lr}\n"
"    ldr  r1, [r8, #0]\n"          /* R4 */
"    adr  r0, 114f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #4]\n"          /* R5 */
"    adr  r0, 115f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #8]\n"          /* R6 */
"    adr  r0, 116f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #12]\n"         /* R7 */
"    adr  r0, 117f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #16]\n"         /* R8 */
"    adr  r0, 118f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #20]\n"         /* R9 */
"    adr  r0, 119f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #24]\n"         /* R10 */
"    adr  r0, 120f\n"
"    bl   ap_sec_kv\n"
"    ldr  r1, [r8, #28]\n"         /* R11 */
"    adr  r0, 121f\n"
"    bl   ap_sec_kv\n"
"    pop  {pc}\n"
     /* --- helpers (self-contained, MMIO only) --- */
"    .thumb_func\n"
"ap_sec_putc:\n"                    /* r0 = char; clobbers r1,r2 */
"    ldr  r1, =0x44820000\n"       /* UART0 base: AP master is forced NS on the
                                    * bus, so it cannot reach the CP-owned secure
                                    * UART1 (0x45830000) -- that access bus-faults.
                                    * UART0 is the AP-reachable console, same regs. */
"50: ldr  r2, [r1, #0x18]\n"       /* FIFO_STATUS */
"    tst  r2, #0x10000\n"          /* TX FIFO full (bit16) */
"    bne  50b\n"
"    strb r0, [r1, #0x1C]\n"       /* DATA */
"    bx   lr\n"
"    .thumb_func\n"
"ap_sec_puts:\n"                    /* r0 = asciz ptr */
"    push {r4, lr}\n"
"    mov  r4, r0\n"
"51: ldrb r0, [r4]\n"
"    cmp  r0, #0\n"
"    beq  52f\n"
"    bl   ap_sec_putc\n"
"    adds r4, r4, #1\n"
"    b    51b\n"
"52: pop  {r4, pc}\n"
"    .thumb_func\n"
"ap_sec_puthex:\n"                  /* r0 = value -> \"0xXXXXXXXX\" */
"    push {r4, r5, lr}\n"
"    mov  r4, r0\n"
"    movs r0, #48\n"               /* '0' */
"    bl   ap_sec_putc\n"
"    movs r0, #120\n"              /* 'x' */
"    bl   ap_sec_putc\n"
"    movs r5, #28\n"
"53: mov  r0, r4\n"
"    lsr  r0, r0, r5\n"
"    and  r0, r0, #0xF\n"
"    cmp  r0, #10\n"
"    ite  lt\n"
"    addlt r0, r0, #48\n"          /* '0' */
"    addge r0, r0, #87\n"          /* 'a' - 10 */
"    bl   ap_sec_putc\n"
"    subs r5, r5, #4\n"
"    bpl  53b\n"
"    pop  {r4, r5, pc}\n"
"    .thumb_func\n"
"ap_sec_kv:\n"                      /* r0 = label ptr, r1 = value */
"    push {r4, r5, lr}\n"
"    mov  r5, r1\n"                /* save value (putc clobbers r1) */
"    bl   ap_sec_puts\n"
"    movs r0, #61\n"               /* '=' */
"    bl   ap_sec_putc\n"
"    mov  r0, r5\n"
"    bl   ap_sec_puthex\n"
"    movs r0, #13\n"               /* CR */
"    bl   ap_sec_putc\n"
"    movs r0, #10\n"               /* LF */
"    bl   ap_sec_putc\n"
"    pop  {r4, r5, pc}\n"
     /* --- strings (data, never executed) --- */
"    .balign 2\n"
"90: .asciz \"\\r\\n!!AP_SEC_FLT\\r\\n\"\n"
"91: .asciz \"ER\"\n"
"92: .asciz \"SFSR\"\n"
"93: .asciz \"SFAR\"\n"
"94: .asciz \"CFSR\"\n"
"95: .asciz \"HFSR\"\n"
"96: .asciz \"BFAR\"\n"
"97: .asciz \"MMFAR\"\n"
"98: .asciz \"FSP\"\n"
"99: .asciz \"\\r\\nframe INVALID\\r\\n\"\n"
"100: .asciz \"PC\"\n"
"101: .asciz \"LR\"\n"
"102: .asciz \"xPSR\"\n"
"103: .asciz \"R0\"\n"
"104: .asciz \"R1\"\n"
"105: .asciz \"R2\"\n"
"106: .asciz \"R3\"\n"
"107: .asciz \"R12\"\n"
"110: .asciz \"MSP\"\n"
"111: .asciz \"PSP\"\n"
"112: .asciz \"MSP_NS\"\n"
"113: .asciz \"PSP_NS\"\n"
"114: .asciz \"R4\"\n"
"115: .asciz \"R5\"\n"
"116: .asciz \"R6\"\n"
"117: .asciz \"R7\"\n"
"118: .asciz \"R8\"\n"
"119: .asciz \"R9\"\n"
"120: .asciz \"R10\"\n"
"121: .asciz \"R11\"\n"
"122: .asciz \"CTRLNS\"\n"
"    .ltorg\n"
"    .balign 4\n"
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

	/* Keep the top 0x200 bytes reserved for the resident Secure fault stack. */
	if (words > ((CONFIG_AP_SPE_RAM_SIZE - 0x200u) / sizeof(uint32_t))) {
		return 0;
	}

	for (i = 0; i < words; i++) {
		dst[i] = src[i];
	}

	/* Patch core0/core1 Non-Secure vectors (blob word index 2/3, +0x08/+0x0C). */
	dst[2] = AP_CORE0_NS_VECTOR;
	dst[3] = core1_ns_vector;

	__asm__ volatile("dsb; isb");

	return AP_SHIM_BASE;
}
