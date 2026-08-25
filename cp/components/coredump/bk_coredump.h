#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "os/mem.h"

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

void bk_coredump_write_prompt(const char *format, ...);
void bk_coredump_write_prompt_data(uint8_t *data, uint32_t size);

/* Print the AON-RTC microsecond timestamp of the dump moment, so the dump can
 * be aligned with the interrupt recorder / task timeline (same time base). */
void bk_coredump_dump_time(uint64_t time_us);

const char *bk_coredump_get_fault_type(void);

void bk_coredump_memory(void);
void bk_coredump_memory_essential(void);
void bk_coredump_memory_extended(void);
void bk_coredump_memory_peripherals(void);
void bk_coredump_ap_memory(void);
/* P1-3/P2-2 split: RAM image + safe peripheral registers, and the (Debug-only)
 * destructive probes, so the trap path can emit the end marker between them. */
void bk_coredump_ap_dump_ram_and_regs(void);
void bk_coredump_ap_dump_regs(void);
void bk_coredump_ap_dump_probes(void);

/* P1-2: true when a stopped AP core could not be confirmed in reset, so the CP
 * must downgrade AP cross-reads to safe register-only reads. */
bool bk_coredump_ap_stop_unconfirmed(void);
void bk_coredump_dump_ap_memory_for_trap(void);

bk_mem_addr_t *bk_get_dump_sys_mem_info(void);
uint32_t bk_get_dump_sys_mem_count(void);

void bk_coredump_lock(void);
void bk_coredump_feed_watchdogs(void);

uint32_t *bk_find_next_valid_lr_pos(uint32_t *start, uint32_t *end);

void bk_dump_peri_regs(void);
void bk_dump_peri_probes(void);
void bk_dump_dtcm(void);
void bk_dump_all_sram(void);
void bk_dump_extra_mem(void);
void bk_dump_mstack(void);
void bk_dump_pstack(void);
void bk_dump_psram_mem(void);
