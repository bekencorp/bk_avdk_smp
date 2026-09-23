#include <stdbool.h>
#include "common/bk_assert.h"
#include "bk_arch.h"
#include "os/mem.h"
#include "bk_coredump.h"
#include "bk_dump_manifest.h"
#include "memory.h"
#include "reg_base.h"
#include "sys_ahbp_ll.h"

/*
 * A peripheral bank whose clock is gated or whose power domain is off does not
 * answer on the bus: the read stalls until a watchdog resets the chip, taking
 * the rest of the dump with it.
 *
 * GPU is the known case: bk_gpu_driver_deinit() calls
 * bk_pm_clock_ctrl(PM_CLK_ID_GPU, PM_CLK_CTRL_PWR_DOWN), which clears
 * sys_ahbp rega.gpu_cken, while the shared VIDEO_POST power domain can stay up
 * for DPU. pwd_video_post is active-high (1 = domain powered down).
 */
static bool peri_reg_bank_is_readable(uint32_t start_addr)
{
    if (start_addr == (uint32_t)SOC_GPU_REG_BASE) {
        return (sys_ahbp_ll_get_rege_pwd_video_post() == 0U) &&
               (sys_ahbp_ll_get_rega_gpu_cken() != 0U);
    }

    return true;
}

void bk_dump_peri_regs(void)
{
    uint32_t peri_reg_info_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri_reg_info_list = bk_get_peri_reg_info_list();
    for (int i = 0; i < peri_reg_info_count; i++) {
        if (!peri_reg_bank_is_readable(peri_reg_info_list[i].start_addr)) {
            bk_coredump_write_prompt(
                "skip region: %s, addr=%08x, reason=clock_or_power_off\r\n",
                peri_reg_info_list[i].name,
                peri_reg_info_list[i].start_addr);
            continue;
        }
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
#if CONFIG_SPE
    bk_coredump_write_memory("MEM_CHECK", (uint32_t)SOC_MEM_CHECK_REG_BASE,
        (uint32_t)(SOC_MEM_CHECK_REG_BASE + 0x81 * 4));
#endif
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

/*
 * P0-1 path 4: AP-local RAM dump used when the CP handoff fails.
 *
 * Composed from the same safe primitives that back manifest(AP): current-context
 * stacks first, then DTCM, SRAM (with the SRAM3 secure carve-out skip applied in
 * bk_dump_all_sram), registered extra memory and PSRAM, keeping the secure-skip /
 * validity guards that a raw manifest address walk would drop (a raw
 * SRAM3-from-base read would SecureFault / stall the bus). Intentionally omits
 * the destructive peri probes, which stay Debug-only (P2-2).
 *
 * Peripheral register banks are deliberately NOT dumped here: a bank read can
 * stall the bus until the watchdog fires, so the caller emits them only after
 * the RAM image and the end marker are out.
 */
void bk_coredump_self_ram_memory(void)
{
    bk_dump_mstack();
    bk_dump_pstack();
    bk_dump_dtcm();
    bk_dump_all_sram();
    bk_dump_extra_mem();
    bk_dump_psram_mem();
}
