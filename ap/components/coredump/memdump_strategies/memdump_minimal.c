#include "bk_coredump.h"

/* Minimal dump is the complement of memdump_all.c: used only for a Release
 * build with CONFIG_MEMDUMP_ALL off. */
#if !(CONFIG_MEMDUMP_ALL || CONFIG_DEBUG_VERSION)
void bk_coredump_memory(void) __attribute__((alias("bk_coredump_memory_minimal")));
#endif

void bk_coredump_memory_minimal(void)
{
    bk_dump_mstack();
    bk_dump_pstack();
    bk_dump_peri_regs();
}
