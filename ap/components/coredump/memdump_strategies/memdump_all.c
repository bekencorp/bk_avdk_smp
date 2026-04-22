#include "bk_coredump.h"
#include "reg_base.h"
#include "os/mem.h"
#include "common/bk_assert.h"

#if CONFIG_MEMDUMP_ALL
void bk_coredump_memory(void) __attribute__((alias("bk_coredump_memory_all")));
#endif

void bk_coredump_memory_all(void)
{
    bk_dump_dtcm();
    bk_dump_all_sram();
    bk_dump_extra_mem();
    bk_dump_psram_mem();
    bk_dump_peri_regs();
}