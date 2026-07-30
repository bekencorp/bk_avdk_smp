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

#include "runtime_defs.h"
#include "sprt_partition_metadata_indicator.h"
#include "tfm_sp_log.h"

#include "psa/error.h"
#include "psa/service.h"

/* BK7259 bring-up diag: raw UART1 (secure) marker. PSA-RoT partitions run
 * PRIVILEGED under isolation L2, so they may access the secure UART1. 'C<c>'. */
#define CSFN_PUTC(ch) do { volatile unsigned int *u1=(volatile unsigned int*)0x45830000; \
    volatile unsigned char *u1b=(volatile unsigned char*)0x45830000; \
    while (u1[0x18/4]&(1u<<16)){} u1b[0x1C]=(ch); } while(0)
#define CSFN_MARK(c) do { CSFN_PUTC('C'); CSFN_PUTC(c); CSFN_PUTC('\r'); CSFN_PUTC('\n'); } while(0)
#define CSFN_HEX(v) do { unsigned int _v=(v); const char *_h="0123456789abcdef"; \
    CSFN_PUTC('@'); for(int _i=28;_i>=0;_i-=4){ CSFN_PUTC(_h[(_v>>_i)&0xf]); } \
    CSFN_PUTC('\r'); CSFN_PUTC('\n'); } while(0)

void common_sfn_thread(void *param)
{
    psa_signal_t sig_asserted, signal_mask, sig;
    psa_msg_t msg;
    struct runtime_metadata_t *meta;
    service_fn_t *p_sfn_table;
    sfn_init_fn_t sfn_init;

    CSFN_MARK('0');
    meta = PART_METADATA();
    CSFN_MARK('1');
    sfn_init = (sfn_init_fn_t)meta->entry;
    p_sfn_table = (service_fn_t *)meta->sfn_table;
    signal_mask = (1UL << meta->n_sfn) - 1;
    /* Dump the init entry address so we know WHICH partition is running. */
    CSFN_HEX((unsigned int)(uintptr_t)sfn_init);
    CSFN_MARK('2');

    if (sfn_init && sfn_init(param) != PSA_SUCCESS) {
        LOG_ERRFMT("Partition initialization FAILED in 0x%x\r\n", sfn_init);
        psa_panic();
    }
    CSFN_MARK('3');
    /* About to enter the SFN message loop; dump the signal mask so we can tell
     * which partition parked here (e.g. the NS-agent has no SFN services). */
    CSFN_HEX(signal_mask);

    while (1) {
        sig_asserted = psa_wait(signal_mask, PSA_BLOCK);
        /* Handle signals */
        for (int i = 0; sig_asserted != 0 && i < meta->n_sfn; i++) {
            sig = 1UL << i;
            if (sig_asserted & sig) {
                /* The i bit signal asserted, index of SFN is i as well */
                if (!p_sfn_table[i]) {
                    /* No corresponding SFN */
                    psa_panic();
                }

                psa_get(sig, &msg);
                psa_reply(msg.handle, ((service_fn_t)p_sfn_table[i])(&msg));
                sig_asserted &= ~sig;
            }
        }

        if (sig_asserted != 0) {
            /* Wrong signal asserted */
            psa_panic();
        }
    }
}
