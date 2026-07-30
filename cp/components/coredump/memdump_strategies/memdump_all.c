#include "bk_coredump.h"
#include "reg_base.h"
#include "os/mem.h"
#include "common/bk_assert.h"

#if CONFIG_MEMDUMP_ALL
void bk_coredump_memory(void) __attribute__((alias("bk_coredump_memory_all")));
void bk_coredump_memory_essential(void) __attribute__((alias("bk_coredump_memory_cpu_essential")));
void bk_coredump_memory_extended(void) __attribute__((alias("bk_coredump_memory_cpu_extended")));
void bk_coredump_memory_peripherals(void) __attribute__((alias("bk_coredump_memory_peri_regs")));
#endif

void bk_coredump_memory_all(void)
{
    bk_dump_dtcm();
    bk_dump_all_sram();
    bk_dump_extra_mem();
    bk_dump_psram_mem();
    bk_dump_peri_regs();
}

void bk_coredump_memory_cpu_essential(void)
{
    bk_dump_dtcm();
    bk_dump_all_sram();
}

void bk_coredump_memory_cpu_extended(void)
{
    bk_dump_extra_mem();
    bk_dump_psram_mem();
}

void bk_coredump_memory_peri_regs(void)
{
    bk_dump_peri_regs();
}
