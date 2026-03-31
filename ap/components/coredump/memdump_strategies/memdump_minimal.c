#include "bk_coredump.h"

#ifndef CONFIG_MEMDUMP_ALL
void bk_coredump_memory(void) __attribute__((alias("bk_coredump_memory_minimal")));
#endif

void bk_coredump_memory_minimal(void)
{
    bk_dump_peri_regs();
    bk_dump_mstack();
    bk_dump_pstack();
}
