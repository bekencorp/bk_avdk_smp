/*
 * Copyright 2026 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "arm_cmse.h"
#include "cmsis.h"
#include "exception_info.h"
#include "region_defs.h"
#include "tfm_spm_log.h"
#include "bk_tfm_cp_dump.h"

#define CP_SEC_DUMP_VECTOR_CALLBACK_OFFSET 0x20U
#define CP_SEC_DUMP_VECTOR_CONTEXT_OFFSET  0x24U
#define CP_SEC_DUMP_VECTOR_ABI_OFFSET      0x28U
#define CP_SEC_DUMP_CONTEXT_MAGIC          0x43505346U /* "CPSF" */
#define CP_SEC_DUMP_ABI_VERSION            1U
#define CP_SEC_DUMP_CONTEXT_WORDS          48U
#define CP_SEC_DUMP_CONTEXT_SIZE           (CP_SEC_DUMP_CONTEXT_WORDS * sizeof(uint32_t))
#define CP_SEC_DUMP_ABI_INFO               0x53440130U /* "SD", v1, 48 words */

#define CP_SEC_DUMP_FLAG_SOURCE_SECURE     (1U << 0)
#define CP_SEC_DUMP_FLAG_SOURCE_THREAD     (1U << 1)
#define CP_SEC_DUMP_FLAG_SOURCE_PSP        (1U << 2)
#define CP_SEC_DUMP_FLAG_DCRS_STACKED      (1U << 3)
#define CP_SEC_DUMP_FLAG_FP_STACKED        (1U << 4)

#define CP_EXC_RETURN_SPSEL                 (1U << 2)
#define CP_EXC_RETURN_MODE                  (1U << 3)
#define CP_EXC_RETURN_FTYPE                 (1U << 4)
#define CP_EXC_RETURN_DCRS                  (1U << 5)
#define CP_EXC_RETURN_SECURE                (1U << 6)
#define XPSR_STACK_ALIGN                    (1U << 9)

#define CP_CORE_ID_ADDR                     0xE005001CU

typedef struct cp_secure_fault_context {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t core_id;
    uint32_t flags;
    uint32_t exception_lr;
    uint32_t frame_sp;
    uint32_t frame_valid;
    uint32_t r[13];
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t msp_s;
    uint32_t psp_s;
    uint32_t msp_ns;
    uint32_t psp_ns;
    uint32_t control_s;
    uint32_t control_ns;
    uint32_t sfsr;
    uint32_t sfar;
    uint32_t cfsr_s;
    uint32_t hfsr_s;
    uint32_t bfar_s;
    uint32_t mmfar_s;
    uint32_t ipsr;
    uint32_t primask_s;
    uint32_t basepri_s;
    uint32_t faultmask_s;
    uint32_t fpscr_s;
    uint32_t primask_ns;
    uint32_t basepri_ns;
    uint32_t faultmask_ns;
    uint32_t reserved[3];
} cp_secure_fault_context_t;

_Static_assert(sizeof(cp_secure_fault_context_t) == CP_SEC_DUMP_CONTEXT_SIZE,
               "CP Secure dump context ABI size mismatch");

typedef void (*cp_ns_dump_callback_t)(void *)
    __attribute__((cmse_nonsecure_call));

static bool range_contains(uint32_t start, uint32_t limit,
                           uint32_t address, uint32_t size)
{
    if ((size == 0U) || (address < start) || (address > limit)) {
        return false;
    }

    return (size - 1U) <= (limit - address);
}

static bool ns_readable_range(uint32_t address, uint32_t size)
{
    return range_contains(NS_CODE_START, NS_CODE_LIMIT, address, size) ||
           range_contains(NS_DATA_START, NS_DATA_LIMIT, address, size);
}

static bool source_uses_psp(uint32_t exc_return, uint32_t control_ns)
{
    if ((exc_return & CP_EXC_RETURN_MODE) == 0U) {
        return false;
    }

    if ((exc_return & CP_EXC_RETURN_SECURE) != 0U) {
        return (exc_return & CP_EXC_RETURN_SPSEL) != 0U;
    }

    return (control_ns & CONTROL_SPSEL_Msk) != 0U;
}

static uint32_t get_raw_frame(const struct exception_info_t *info,
                              bool source_secure, bool source_psp)
{
    if (source_secure) {
        return source_psp ? info->PSP : info->MSP;
    }

    return source_psp ? __TZ_get_PSP_NS() : __TZ_get_MSP_NS();
}

static uint32_t get_basic_frame(uint32_t raw_frame, uint32_t exc_return)
{
    uint32_t frame = raw_frame;

    if ((exc_return & CP_EXC_RETURN_FTYPE) == 0U) {
        frame += 18U * sizeof(uint32_t);
    }
    if ((exc_return & CP_EXC_RETURN_DCRS) == 0U) {
        frame += 10U * sizeof(uint32_t);
    }

    return frame;
}

static bool frame_is_readable(uint32_t raw_frame, uint32_t basic_frame,
                              bool source_secure)
{
    uint32_t size;

    if (((raw_frame | basic_frame) & (sizeof(uint32_t) - 1U)) != 0U ||
        basic_frame < raw_frame ||
        basic_frame > UINT32_MAX - (8U * sizeof(uint32_t))) {
        return false;
    }

    size = basic_frame - raw_frame + 8U * sizeof(uint32_t);

    if (source_secure) {
        return range_contains(S_DATA_START, S_DATA_LIMIT, raw_frame, size);
    }

    return range_contains(NS_DATA_START, NS_DATA_LIMIT, raw_frame, size);
}

static void fill_context(volatile cp_secure_fault_context_t *context,
                         const struct exception_info_t *info,
                         const uint32_t *raw_frame, const uint32_t *frame,
                         uint32_t core_id,
                         bool source_secure, bool source_psp)
{
    uint32_t flags = 0U;
    const uint32_t *callee_saved = info->CALLEE_SAVED_COPY;

    context->magic = 0U;
    context->version = CP_SEC_DUMP_ABI_VERSION;
    context->size = sizeof(*context);
    context->core_id = core_id;

    if (source_secure) {
        flags |= CP_SEC_DUMP_FLAG_SOURCE_SECURE;
    }
    if ((info->EXC_RETURN & CP_EXC_RETURN_MODE) != 0U) {
        flags |= CP_SEC_DUMP_FLAG_SOURCE_THREAD;
    }
    if (source_psp) {
        flags |= CP_SEC_DUMP_FLAG_SOURCE_PSP;
    }
    if ((info->EXC_RETURN & CP_EXC_RETURN_DCRS) == 0U) {
        flags |= CP_SEC_DUMP_FLAG_DCRS_STACKED;
    }
    if ((info->EXC_RETURN & CP_EXC_RETURN_FTYPE) == 0U) {
        flags |= CP_SEC_DUMP_FLAG_FP_STACKED;
    }

    context->flags = flags;
    context->exception_lr = info->EXC_RETURN;
    context->frame_sp = (uint32_t)frame;
    context->frame_valid = 1U;

    context->r[0] = frame[0];
    context->r[1] = frame[1];
    context->r[2] = frame[2];
    context->r[3] = frame[3];
    if ((info->EXC_RETURN & CP_EXC_RETURN_DCRS) == 0U) {
        callee_saved = frame - 8;
    }
    for (uint32_t i = 0; i < 8U; i++) {
        context->r[4U + i] = callee_saved[i];
    }
    context->r[12] = frame[4];
    context->lr = frame[5];
    context->pc = frame[6];
    context->xpsr = frame[7];
    context->sp = (uint32_t)(frame + 8);
    if ((context->xpsr & XPSR_STACK_ALIGN) != 0U) {
        context->sp += sizeof(uint32_t);
    }

    context->msp_s = info->MSP;
    context->psp_s = info->PSP;
    context->msp_ns = __TZ_get_MSP_NS();
    context->psp_ns = __TZ_get_PSP_NS();
    context->control_s = __get_CONTROL();
    context->control_ns = __TZ_get_CONTROL_NS();
    context->sfsr = info->SFSR;
    context->sfar = info->SFAR;
    context->cfsr_s = info->CFSR;
    context->hfsr_s = info->HFSR;
    context->bfar_s = info->BFAR;
    context->mmfar_s = info->MMFAR;
    context->ipsr = info->VECTACTIVE;
    context->primask_s = __get_PRIMASK();
    context->basepri_s = __get_BASEPRI();
    context->faultmask_s = __get_FAULTMASK();
    context->fpscr_s =
        ((info->EXC_RETURN & CP_EXC_RETURN_FTYPE) == 0U)
            ? raw_frame[16]
            : 0U;
    context->primask_ns = __TZ_get_PRIMASK_NS();
    context->basepri_ns = __TZ_get_BASEPRI_NS();
    context->faultmask_ns = __TZ_get_FAULTMASK_NS();
    context->reserved[0] = 0U;
    context->reserved[1] = 0U;
    context->reserved[2] = 0U;

    __DSB();
    context->magic = CP_SEC_DUMP_CONTEXT_MAGIC;
    __DSB();
    __ISB();
}

void tfm_hal_secure_fault_handoff(void)
{
#ifdef TFM_EXCEPTION_INFO_DUMP
    struct exception_info_t info;
    uint32_t vtor = SCB_NS->VTOR;
    uint32_t callback_word;
    uint32_t carrier;
    uint32_t context_base;
    uint32_t context_address;
    uint32_t callback_address;
    uint32_t core_id;
    uint32_t control_ns;
    uint32_t raw_frame_address;
    uint32_t frame_address;
    bool source_secure;
    bool source_psp;
    cp_ns_dump_callback_t callback;
    volatile cp_secure_fault_context_t *context;

    if (((vtor & 0x7FU) != 0U) ||
        !ns_readable_range(vtor, CP_SEC_DUMP_VECTOR_ABI_OFFSET + sizeof(uint32_t))) {
        SPMLOG_ERRMSG("CP_SEC_DUMP:F0 invalid NS VTOR\r\n");
        return;
    }

    callback_word = *(volatile const uint32_t *)
        (vtor + CP_SEC_DUMP_VECTOR_CALLBACK_OFFSET);
    carrier = *(volatile const uint32_t *)
        (vtor + CP_SEC_DUMP_VECTOR_CONTEXT_OFFSET);
    if (*(volatile const uint32_t *)(vtor + CP_SEC_DUMP_VECTOR_ABI_OFFSET) !=
        CP_SEC_DUMP_ABI_INFO) {
        SPMLOG_ERRMSG("CP_SEC_DUMP:F1 ABI mismatch\r\n");
        return;
    }

    if (((callback_word & 1U) == 0U) ||
        ((carrier & (sizeof(uint32_t) - 1U)) != 0U) ||
        !range_contains(NS_CODE_START, NS_CODE_LIMIT,
                        callback_word & ~1U, sizeof(uint16_t)) ||
        !ns_readable_range(carrier, sizeof(uint32_t))) {
        SPMLOG_ERRMSG("CP_SEC_DUMP:F2 invalid callback/carrier\r\n");
        return;
    }

    context_base = *(volatile const uint32_t *)carrier;
    core_id = *(volatile const uint32_t *)CP_CORE_ID_ADDR & 0xFU;
    if (core_id > 1U) {
        SPMLOG_ERRMSG("CP_SEC_DUMP:F3 invalid core ID\r\n");
        return;
    }

    context_address = context_base + core_id * CP_SEC_DUMP_CONTEXT_SIZE;
    if (((context_base | context_address) & 0x1FU) != 0U ||
        (context_address < context_base) ||
        !range_contains(NS_DATA_START, NS_DATA_LIMIT, context_address,
                        CP_SEC_DUMP_CONTEXT_SIZE)) {
        SPMLOG_ERRMSG("CP_SEC_DUMP:F4 invalid context address\r\n");
        return;
    }

    tfm_exception_info_get_context(&info);
    source_secure = (info.EXC_RETURN & CP_EXC_RETURN_SECURE) != 0U;
    control_ns = __TZ_get_CONTROL_NS();
    source_psp = source_uses_psp(info.EXC_RETURN, control_ns);
    raw_frame_address = get_raw_frame(&info, source_secure, source_psp);
    frame_address = get_basic_frame(raw_frame_address, info.EXC_RETURN);
    if (!frame_is_readable(raw_frame_address, frame_address, source_secure)) {
        return;
    }

    context = (volatile cp_secure_fault_context_t *)context_address;
    fill_context(context, &info, (const uint32_t *)raw_frame_address,
                 (const uint32_t *)frame_address, core_id, source_secure,
                 source_psp);

    callback_address = callback_word | 1U;
    callback = (cp_ns_dump_callback_t)cmse_nsfptr_create(
        (void *)callback_address);
    callback((void *)context);
#endif
}
