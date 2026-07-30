#include "bk_coredump.h"

#ifndef CONFIG_MEMDUMP_ALL
void bk_coredump_memory(void) __attribute__((alias("bk_coredump_memory_minimal")));
void bk_coredump_memory_essential(void) __attribute__((alias("bk_coredump_memory_stack_essential")));
void bk_coredump_memory_extended(void) __attribute__((alias("bk_coredump_memory_stack_extended")));
void bk_coredump_memory_peripherals(void) __attribute__((alias("bk_coredump_memory_minimal_peripherals")));
#endif

void bk_coredump_memory_minimal(void)
{
    bk_dump_mstack();
    bk_dump_pstack();
    bk_dump_peri_regs();
}

void bk_coredump_memory_stack_essential(void)
{
    bk_dump_mstack();
}

void bk_coredump_memory_stack_extended(void)
{
    bk_dump_pstack();
}

void bk_coredump_memory_minimal_peripherals(void)
{
    bk_dump_peri_regs();
}
