#pragma once
#include <stdint.h>
#include <stddef.h>
#include "os/mem.h"

#define AP_SEC_DUMP_VECTOR_CALLBACK_INDEX 8U
#define AP_SEC_DUMP_VECTOR_CONTEXT_INDEX  9U
#define AP_SEC_DUMP_VECTOR_ABI_INDEX      10U
#define AP_SEC_DUMP_CONTEXT_MAGIC         0x41505346U /* "APSF" */
#define AP_SEC_DUMP_ABI_VERSION           1U
#define AP_SEC_DUMP_CONTEXT_WORDS         48U
#define AP_SEC_DUMP_ABI_INFO              0x53440130U /* "SD", v1, 48 words */

#define AP_SEC_DUMP_FLAG_SOURCE_SECURE    (1U << 0)
#define AP_SEC_DUMP_FLAG_SOURCE_THREAD    (1U << 1)
#define AP_SEC_DUMP_FLAG_SOURCE_PSP       (1U << 2)
#define AP_SEC_DUMP_FLAG_DCRS_STACKED     (1U << 3)
#define AP_SEC_DUMP_FLAG_FP_STACKED       (1U << 4)

typedef struct ap_secure_fault_context {
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
} ap_secure_fault_context_t;

_Static_assert(sizeof(ap_secure_fault_context_t) ==
               AP_SEC_DUMP_CONTEXT_WORDS * sizeof(uint32_t),
               "AP Secure dump context ABI size mismatch");

#if CONFIG_SOC_SMP
#define AP_SEC_DUMP_CONTEXT_COUNT 2U
#else
#define AP_SEC_DUMP_CONTEXT_COUNT 1U
#endif

extern volatile ap_secure_fault_context_t
    g_ap_secure_fault_context[AP_SEC_DUMP_CONTEXT_COUNT];
extern const uintptr_t g_ap_secure_fault_context_address;
void bk_coredump_secure_fault_callback(void *context_arg);

typedef struct bk_assert_info
{
    uint32_t magic;
    const char *func;
    int line;
} bk_assert_info_t;

typedef struct
{
    uint32_t lr;
    uint32_t sp;
    uint32_t reset_reason;
    /* Captured at exception entry, BEFORE interrupts are disabled, so they
     * reflect the real pre-exception state (these are not auto-stacked by HW). */
    uint32_t primask;
    uint32_t basepri;
    uint32_t faultmask;
    uint32_t control;
    const ap_secure_fault_context_t *secure_context;
    /* AON-RTC microsecond timestamp captured at exception entry, on the same
     * bk_aon_rtc_get_us() time base as the interrupt recorder, so the dump can
     * be aligned with the interrupt/task records and the exception timeline. */
    uint64_t exception_time_us;
} bk_exception_t;

/* coredump writer api */

void bk_coredump_writer_init(void);
void bk_coredump_write(const char *format, ...);
void bk_coredump_writer_deinit(void);

typedef enum {
    COREDUMP_REGISTERS_INFO = 0,
    COREDUMP_BUILD_INFO,
    COREDUMP_BOARD_INFO,
    COREDUMP_ARCH_INFO,
    COREDUMP_CORE_INFO,
    COREDUMP_EXCEPTION_INFO,
    COREDUMP_ASSERT_INFO,
    COREDUMP_TRACEBACK_INFO,
} COREDUMP_META_INFO;

void bk_coredump_write_meta_info(COREDUMP_META_INFO info, void *data);
void bk_coredump_write_registers(const char *name, uint32_t value);
void bk_coredump_write_memory(const char *name, uint32_t stack_top, uint32_t stack_bottom);

void bk_coredump_registers(bk_exception_t *self);
void bk_coredump_secure_registers(const ap_secure_fault_context_t *context);
void bk_exception_handler_from_secure(const ap_secure_fault_context_t *context);

void bk_coredump_write_prompt(const char *format, ...);
void bk_coredump_write_prompt_data(uint8_t *data, uint32_t size);

/* Print the AON-RTC microsecond timestamp of the dump moment, so the dump can
 * be aligned with the interrupt recorder / task timeline (same time base). */
void bk_coredump_dump_time(uint64_t time_us);

const char *bk_coredump_get_fault_type(void);

void bk_coredump_memory(void);

bk_mem_addr_t *bk_get_dump_sys_mem_info(void);
uint32_t bk_get_dump_sys_mem_count(void);

void bk_coredump_lock(void);

uint32_t *bk_find_next_valid_lr_pos(uint32_t *start, uint32_t *end);

void bk_dump_peri_regs(void);
void bk_dump_dtcm(void);
void bk_dump_all_sram(void);
void bk_dump_extra_mem(void);
void bk_dump_mstack(void);
void bk_dump_pstack(void);
void bk_dump_psram_mem(void);

/* P0-1 path 4: AP-local full-memory dump used as the CP-handoff-failure
 * fallback (registers/stacks + manifest(AP) memory), then the caller resets. */
void bk_coredump_self_full_memory(void);
