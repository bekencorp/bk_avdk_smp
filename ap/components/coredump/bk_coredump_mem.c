#include "common/bk_assert.h"
#include "bk_arch.h"
#include "os/mem.h"
#include "bk_coredump.h"
#include "memory.h"

void bk_dump_peri_regs(void)
{
    uint32_t peri_reg_info_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri_reg_info_list = bk_get_peri_reg_info_list();
    for (int i = 0; i < peri_reg_info_count; i++) {
        bk_coredump_write_memory(
            peri_reg_info_list[i].name,
            peri_reg_info_list[i].start_addr,
            peri_reg_info_list[i].start_addr + peri_reg_info_list[i].size
        );
    }
}

extern void bk_get_dtcm_info(bk_mem_addr_t *info);
void bk_dump_dtcm(void)
{
    bk_mem_addr_t dtcm_info;
    bk_get_dtcm_info(&dtcm_info);
    if (dtcm_info.start_addr != 0 && dtcm_info.size != 0) {
        bk_coredump_write_memory("DTCM", dtcm_info.start_addr, dtcm_info.start_addr + dtcm_info.size);
    }
}

/*
 * The low part of SRAM3 is the secure carve-out and is unreachable from a
 * Non-Secure alias (the read SecureFaults / stalls the bus), so the dump must
 * start past it. On this core the carve-out is 0x1000 (4K) at the SRAM3 base
 * (0x28100000).
 */
#define COREDUMP_SRAM3_SECURE_SKIP   (0x1000U)

void bk_dump_all_sram(void)
{
    uint32_t sram_info_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram_info_list = bk_get_sram_info_list();
    for (int i = 0; i < sram_info_count; i++) {
        uint32_t start = sram_info_list[i].start_addr;
        uint32_t size = sram_info_list[i].size;

        if ((start == (uint32_t)SOC_SRAM3_DATA_BASE) && (size > COREDUMP_SRAM3_SECURE_SKIP)) {
            start += COREDUMP_SRAM3_SECURE_SKIP;
            size -= COREDUMP_SRAM3_SECURE_SKIP;
        }

        bk_coredump_write_memory(
            sram_info_list[i].name,
            start,
            start + size
        );
    }
    bk_coredump_write_memory("MEM_CHECK", (uint32_t)SOC_MEM_CHECK_REG_BASE, (uint32_t)(SOC_MEM_CHECK_REG_BASE + 0x81 * 4));
}

void bk_dump_extra_mem(void)
{
    uint32_t dump_sys_mem_count = bk_get_dump_sys_mem_count();
    bk_mem_addr_t *dump_sys_mem_info = bk_get_dump_sys_mem_info();
    for (int i = 0; i < dump_sys_mem_count; i++) {
        bk_coredump_write_memory("EXTRA_MEM", dump_sys_mem_info[i].start_addr, dump_sys_mem_info[i].start_addr + dump_sys_mem_info[i].size);
    }
}


extern uint32_t bk_get_msp_bottom(void);
extern uint32_t bk_get_current_stack_bottom(void);

void bk_dump_mstack(void)
{
    uint32_t msp = __get_MSP();
    uint32_t msp_bottom = bk_get_msp_bottom();
    if (msp >= msp_bottom) {
        bk_coredump_write_prompt("msp stack invalid, msp: 0x%lx, msp_bottom: 0x%lx\r\n", msp, msp_bottom);
        return;
    }
    bk_coredump_write_memory("mstack", msp, msp_bottom);
}

void bk_dump_pstack(void)
{
    uint32_t psp = __get_PSP();
    uint32_t psp_bottom = bk_get_current_stack_bottom();
    if (psp_bottom - psp > 4096) {
        psp_bottom = psp + 4096;
    }
    if (psp >= psp_bottom) {
        bk_coredump_write_prompt("psp stack invalid, psp: 0x%lx, psp_bottom: 0x%lx\r\n", psp, psp_bottom);
        return;
    }
    bk_coredump_write_memory("pstack", psp, psp_bottom);
}

void bk_dump_psram_mem(void)
{
    // dump psram heap
    bk_dump_mem_info_t mem_info;
    bk_get_psram_heap_info(&mem_info);
    if (mem_info.start_addr != 0 && mem_info.size != 0) {
        bk_coredump_write_memory(mem_info.name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
    // dump psram bss
    bk_get_psram_bss_info(&mem_info);
    if (mem_info.start_addr != 0 && mem_info.size != 0) {
        bk_coredump_write_memory(mem_info.name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
    // dump psram data
    bk_get_psram_data_info(&mem_info);
    if (mem_info.start_addr != 0 && mem_info.size != 0) {
        bk_coredump_write_memory(mem_info.name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
}
