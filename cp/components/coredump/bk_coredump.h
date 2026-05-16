#pragma once
#include <stdint.h>
#include <stddef.h>
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

const char *bk_coredump_get_fault_type(void);

void bk_coredump_memory(void);
void bk_coredump_ap_memory(void);
void bk_coredump_dump_ap_memory_for_trap(void);

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
