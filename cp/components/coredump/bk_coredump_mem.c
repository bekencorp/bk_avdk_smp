#include <stddef.h>
#include "common/bk_assert.h"
#include "bk_arch.h"
#include "os/mem.h"
#include "bk_coredump.h"
#include "memory.h"
#include "sys_sw_regs.h"
#include "sys_ahbp_ll.h"

typedef void (*bk_dump_mem_getter_t)(bk_dump_mem_info_t *info);

#define COREDUMP_PSRAM0_PROBE_ADDR        (0x60000000U)
#define COREDUMP_PSRAM0_PROBE_PRE_SIZE    (0x100U)
#define COREDUMP_PSRAM0_PROBE_POST_SIZE   (0x10U)
#define COREDUMP_PSRAM0_PROBE_PATTERN     (0x12345678U)
#define COREDUMP_AP_SRAM_PROBE_SIZE       (0x20U)
#define COREDUMP_AP_DTCM_PROBE_SIZE       (0x20U)
#define COREDUMP_AP_SRAM_PROBE_PATTERN    (0x5a5a1234U)
#define COREDUMP_AP_DTCM_PROBE_PATTERN    (0xa5a54321U)
#define COREDUMP_AP_SRAM0_PROBE_ADDR      (0x28180000U)
#define COREDUMP_AP_SRAM1_PROBE_ADDR      (0x28100000U)
#define COREDUMP_AP_DTCM_PROBE_ADDR       (0x28200000U)

static void bk_dump_sys_ahbp_debug_mux_readouts(void)
{
    uint32_t saved_reg23 = sys_ahbp_ll_get_reg23_value();
    uint32_t cpu0pc = 0;
    uint32_t mosi_state = 0;
    uint32_t slave_busy = 0;

    /*
     * H1 AHB-stall forensics: keep the existing cpu0pc mux sample, then
     * briefly switch to raw bus debug muxes so a later heartbeat dump can
     * identify the stuck master/slave instead of inferring it from PC only.
     * Restore REG_0x23 before returning so normal GPIO debug selection is
     * not permanently changed after coredump.
     */
    sys_ahbp_ll_set_reg23_dbug_mux(0x4);
    __DSB();
    cpu0pc = sys_ahbp_ll_get_reg25_value();

    sys_ahbp_ll_set_reg23_dbug_mux(0xc);
    __DSB();
    mosi_state = sys_ahbp_ll_get_reg25_value();

    sys_ahbp_ll_set_reg23_dbug_mux(0xd);
    __DSB();
    slave_busy = sys_ahbp_ll_get_reg25_value();

    sys_ahbp_ll_set_reg23_value(saved_reg23);
    __DSB();

    bk_coredump_write_prompt(
        "SYS_AHBP_DBG saved_reg23=0x%08lx cpu0pc(mux4)=0x%08lx mosi_state(muxC)=0x%08lx slave_busy(muxD)=0x%08lx\r\n",
        saved_reg23, cpu0pc, mosi_state, slave_busy);
}

static void bk_dump_psram0_base_write_probe(void)
{
    volatile uint32_t *probe = (volatile uint32_t *)COREDUMP_PSRAM0_PROBE_ADDR;
    uint32_t before = probe[0];

    bk_coredump_write_memory("PSRAM0_BASE_PRE", COREDUMP_PSRAM0_PROBE_ADDR,
        COREDUMP_PSRAM0_PROBE_ADDR + COREDUMP_PSRAM0_PROBE_PRE_SIZE);

    probe[0] = COREDUMP_PSRAM0_PROBE_PATTERN;
    __DSB();

    bk_coredump_write_prompt(
        "PSRAM0_BASE_WRITE_PROBE addr=0x%08lx before=0x%08lx write=0x%08lx after=0x%08lx\r\n",
        COREDUMP_PSRAM0_PROBE_ADDR, before, COREDUMP_PSRAM0_PROBE_PATTERN, probe[0]);

    bk_coredump_write_memory("PSRAM0_BASE_POST", COREDUMP_PSRAM0_PROBE_ADDR,
        COREDUMP_PSRAM0_PROBE_ADDR + COREDUMP_PSRAM0_PROBE_POST_SIZE);
}

static void bk_dump_fixed_write_probe(const char *write_name, const char *pre_name, const char *post_name,
    uint32_t addr, uint32_t size, uint32_t pattern)
{
    volatile uint32_t *probe = (volatile uint32_t *)addr;
    uint32_t before = probe[0];

    bk_coredump_write_memory(pre_name, addr, addr + size);

    probe[0] = pattern;
    __DSB();

    bk_coredump_write_prompt(
        "%s addr=0x%08lx before=0x%08lx write=0x%08lx after=0x%08lx\r\n",
        write_name, addr, before, pattern, probe[0]);

    bk_coredump_write_memory(post_name, addr, addr + size);
}

static void bk_dump_ap_sram_dtcm_write_probes(void)
{
    bk_dump_fixed_write_probe("AP_SRAM_28180000_WRITE_PROBE",
        "AP_SRAM_28180000_PRE", "AP_SRAM_28180000_POST",
        COREDUMP_AP_SRAM0_PROBE_ADDR,
        COREDUMP_AP_SRAM_PROBE_SIZE, COREDUMP_AP_SRAM_PROBE_PATTERN);
    bk_dump_fixed_write_probe("AP_SRAM_28100000_WRITE_PROBE",
        "AP_SRAM_28100000_PRE", "AP_SRAM_28100000_POST",
        COREDUMP_AP_SRAM1_PROBE_ADDR,
        COREDUMP_AP_SRAM_PROBE_SIZE, COREDUMP_AP_SRAM_PROBE_PATTERN);
    bk_dump_fixed_write_probe("AP_DTCM_28200000_WRITE_PROBE",
        "AP_DTCM_28200000_PRE", "AP_DTCM_28200000_POST",
        COREDUMP_AP_DTCM_PROBE_ADDR,
        COREDUMP_AP_DTCM_PROBE_SIZE, COREDUMP_AP_DTCM_PROBE_PATTERN);
}

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

    /*
     * Keep raw peripheral byte-dumps ahead of live debug probes. If an H1
     * failure makes a SYS_AHBP/PSRAM MMIO access stall, the forensic register
     * regions above have already been emitted.
     */
    bk_dump_psram0_base_write_probe();
    bk_dump_ap_sram_dtcm_write_probes();
    bk_dump_sys_ahbp_debug_mux_readouts();
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

    /*
     * These probes perform live MMIO writes/reads. Run them only after the raw
     * peripheral windows are serialized so a probe stall does not hide them.
     */
    bk_dump_psram0_base_write_probe();
    bk_dump_ap_sram_dtcm_write_probes();
    bk_dump_sys_ahbp_debug_mux_readouts();
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
