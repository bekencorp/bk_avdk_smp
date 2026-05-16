#include <stddef.h>
#include "common/bk_assert.h"
#include "bk_arch.h"
#include "os/mem.h"
#include "bk_coredump.h"
#include "memory.h"
#include "sys_sw_regs.h"

typedef void (*bk_dump_mem_getter_t)(bk_dump_mem_info_t *info);

static void bk_dump_ap_window(const char *name, bk_dump_mem_getter_t getter)
{
    bk_dump_mem_info_t mem_info = {0};

    getter(&mem_info);
    if ((mem_info.start_addr == 0U) || (mem_info.size == 0U)) {
        bk_coredump_write_prompt("%s empty window, skip\r\n", name);
        return;
    }
    if (!bk_check_addr_in_ap_dump_range(mem_info.start_addr, mem_info.size)) {
        bk_coredump_write_prompt("%s invalid window, start=0x%lx, size=0x%lx, skip\r\n",
            name, mem_info.start_addr, mem_info.size);
        return;
    }

    bk_coredump_write_memory(name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
}

static void bk_dump_ap_heap_mem(void)
{
    bk_dump_ap_window("AP_PSRAM_HEAP", bk_get_ap_psram_heap_info);
}

static void bk_dump_ap_dtcm(void)
{
    bk_dump_ap_window("AP_DTCM", bk_get_ap_dtcm_info);
}

static void bk_dump_ap_all_sram(void)
{
    uint32_t sram_info_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram_info_list = bk_get_sram_info_list();

    for (uint32_t i = 0; i < sram_info_count; i++) {
        if (!bk_check_addr_in_ap_dump_range(sram_info_list[i].start_addr, sram_info_list[i].size)) {
            bk_coredump_write_prompt("AP_%s invalid window, start=0x%lx, size=0x%lx, skip\r\n",
                sram_info_list[i].name, sram_info_list[i].start_addr, sram_info_list[i].size);
            continue;
        }

        bk_coredump_write_memory(sram_info_list[i].name, sram_info_list[i].start_addr,
            sram_info_list[i].start_addr + sram_info_list[i].size);
    }

    bk_coredump_write_memory("MEM_CHECK", (uint32_t)SOC_MEM_CHECK_REG_BASE,
        (uint32_t)(SOC_MEM_CHECK_REG_BASE + 0x81 * 4));
}

static void bk_dump_ap_extra_mem(void)
{
    for (uint32_t i = 0; i < BK_SYS_SW_REGS_AP_EXTRA_DUMP_MAX; i++) {
        ap_extra_dump_info_t info = {0};

        if (!bk_sys_sw_regs_get_ap_extra_dump(i, &info)) {
            continue;
        }
        if (!bk_check_addr_in_ap_dump_range(info.start_addr, info.size)) {
            bk_coredump_write_prompt("AP_EXTRA_MEM invalid window, start=0x%lx, size=0x%lx, skip\r\n",
                info.start_addr, info.size);
            continue;
        }

        bk_coredump_write_memory("AP_EXTRA_MEM", info.start_addr, info.start_addr + info.size);
    }
}

static void bk_dump_ap_psram_mem(void)
{
    bk_dump_ap_window("AP_PSRAM_HEAP", bk_get_ap_psram_heap_info);
    bk_dump_ap_window("AP_PSRAM_DATA", bk_get_ap_psram_data_info);
    bk_dump_ap_window("AP_PSRAM_BSS", bk_get_ap_psram_bss_info);
}

static void bk_dump_ap_peri_regs(void)
{
    uint32_t peri_reg_info_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri_reg_info_list = bk_get_peri_reg_info_list();

    for (uint32_t i = 0; i < peri_reg_info_count; i++) {
        if (!bk_check_addr_in_ap_dump_range(peri_reg_info_list[i].start_addr, peri_reg_info_list[i].size)) {
            bk_coredump_write_prompt("AP_%s invalid window, start=0x%lx, size=0x%lx, skip\r\n",
                peri_reg_info_list[i].name, peri_reg_info_list[i].start_addr, peri_reg_info_list[i].size);
            continue;
        }

        bk_coredump_write_memory(peri_reg_info_list[i].name, peri_reg_info_list[i].start_addr,
            peri_reg_info_list[i].start_addr + peri_reg_info_list[i].size);
    }
}

void bk_coredump_ap_memory(void)
{
    bk_dump_ap_dtcm();
    bk_dump_ap_all_sram();
    bk_dump_ap_extra_mem();
    bk_dump_ap_psram_mem();
    bk_dump_ap_peri_regs();
}

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

void bk_dump_all_sram(void)
{
    uint32_t sram_info_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram_info_list = bk_get_sram_info_list();
    for (int i = 0; i < sram_info_count; i++) {
        bk_coredump_write_memory(
            sram_info_list[i].name,
            sram_info_list[i].start_addr,
            sram_info_list[i].start_addr + sram_info_list[i].size
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

    bk_dump_ap_heap_mem();
}
