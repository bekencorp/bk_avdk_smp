#include "bk_coredump.h"
#include "reg_base.h"
#include "os/mem.h"
#include "common/bk_assert.h"

/* Full memory dump is selected when MEMDUMP_ALL is on OR this is a Debug
 * build. CONFIG_DEBUG_VERSION is set at compile time by the build system for
 * the developer environment (and by an explicit BUILD_VERSION=Debug), so a
 * Debug build always dumps full memory while a Release build follows
 * CONFIG_MEMDUMP_ALL (minimal by default). Keep this condition the exact
 * complement of memdump_minimal.c so the aliases are defined exactly once. */
#if CONFIG_MEMDUMP_ALL || CONFIG_DEBUG_VERSION
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
    bk_dump_peri_probes();
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
