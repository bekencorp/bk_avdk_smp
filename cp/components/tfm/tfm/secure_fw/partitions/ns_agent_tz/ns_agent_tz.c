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

#define TAG "ns_agent_tz"

/* Apply the plaintext CP PPRO config before BXNS. The AP-side PPHS is deferred to
 * psa_ap_secure_prepare(): the AP power domain is off here (CP NS powers it on the
 * AP start request), and once CP PPRO marks AON/sys Non-Secure the secure world
 * must not probe that region through the secure alias. */
__used static void ns_init_hook(void)
{
    BK_LOGI(TAG, "config ppc and NSPE is coming\r\n");
    /* Cold boot: cache both CP/AP configs while flash is still Secure, so the
     * later AP-side PPHS apply (from the secure-prepare NSC) reads from RAM and
     * never touches the flash controller after CP marks flash Non-secure. */
    if (bk_ppc_cache_load_from_flash() != 0 ||
        bk_ppc_apply_cp_config_from_flash() != 0) {
        /* Fail closed if the image is missing, erased, or unreadable. */
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
#if (CONFIG_TFM_FLOAT_ABI > 0)
/* IAR throws an error if the S0-S31 syntax is used.
 * Splitting the command into two parts solved the issue.
 */
#if defined(__ICCARM__)
        "   vscclrm  {S0-S30, VPR}                  \n"
        "   vscclrm  {S31, VPR}                     \n"
#else
        "   vscclrm  {S0-S31, VPR}                  \n"
#endif
        "   mov      r1, #0                         \n"
        "   vmsr     fpscr_nzcvqc, r1               \n"
        "   mrs      r1, control                    \n"
        "   bic      r1, r1, #4                     \n"
        "   msr      control, r1                    \n"
        "   isb                                     \n"
#endif
        "   clrm     {r1-r12, r14, apsr}            \n"
        "   bic      r0, r0, #1                     \n"
        "   bxns     r0                             \n"
        "ns_agent_nspe_jump_panic:                  \n"
        "   b        .                              \n"
    );
}
