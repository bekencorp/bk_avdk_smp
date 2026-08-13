/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 * Copyright (c)     2023-2028 Cypress Semiconductor Corporation (an Infineon
 * company) or an affiliate of Cypress Semiconductor Corporation. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stdint.h>

#include "compiler_ext_defs.h"
#include "security_defs.h"
#include "tfm_arch.h"
#include "tfm_hal_platform.h"
#include "bk_tfm_ppc.h"
#if CONFIG_SLEEP_RETENTION_NSC
#include "tfm_sleep_context.h"
#endif

__used static void ns_init_hook(void)
{
    /* Cold boot: cache both CP/AP configs while flash is still Secure, so the
     * later AP-side PPHS apply reads from RAM (no flash access after CP marks
     * flash Non-secure). */
    if (bk_ppc_cache_load_from_flash() != 0 ||
        bk_ppc_apply_cp_config_from_flash() != 0) {
        while (1) {
        }
    }
    bk_ppc_set_ap_master_nsec();
#if CONFIG_SLEEP_RETENTION_NSC
    (void)tfm_sleep_context_refresh_ppro();
#endif
}

__naked void ns_agent_tz_main(uint32_t c_entry)
{
    __ASM volatile(
        SYNTAX_UNIFIED
        "   push     {r0}                           \n"
        "   bl       ns_init_hook                   \n"
        "   pop      {r0}                           \n"
        "   ldr      r2, [sp]                       \n"
        "   ldr      r3, ="M2S(STACK_SEAL_PATTERN)" \n" /* SEAL double-check */
        "   cmp      r2, r3                         \n"
        "   bne      ns_agent_nspe_jump_panic       \n"
        "   movs     r2, #1                         \n" /* For NS execution */
        "   bics     r0, r2                         \n"
        "   mov      r1, r0                         \n"
#if (CONFIG_TFM_FLOAT_ABI >= 1)
        "   vmov     d0, r0, r1                     \n"
        "   vmov     d1, r0, r1                     \n"
        "   vmov     d2, r0, r1                     \n"
        "   vmov     d3, r0, r1                     \n"
        "   vmov     d4, r0, r1                     \n"
        "   vmov     d5, r0, r1                     \n"
        "   vmov     d6, r0, r1                     \n"
        "   vmov     d7, r0, r1                     \n"
        "   mrs      r2, control                    \n"
        "   bic      r2, r2, #4                     \n"
        "   msr      control, r2                    \n"
        "   isb                                     \n"
#endif
        "   mov      r2, r0                         \n"
        "   mov      r3, r0                         \n"
        "   mov      r4, r0                         \n"
        "   mov      r5, r0                         \n"
        "   mov      r6, r0                         \n"
        "   mov      r7, r0                         \n"
        "   mov      r8, r0                         \n"
        "   mov      r9, r0                         \n"
        "   mov      r10, r0                        \n"
        "   mov      r11, r0                        \n"
        "   mov      r12, r0                        \n"
        "   mov      r14, r0                        \n"
        "   bxns     r0                             \n"
        "ns_agent_nspe_jump_panic:                  \n"
        "   b        .                              \n"
    );
}


