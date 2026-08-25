#include <stddef.h>
#include <stdbool.h>
#include "common/bk_assert.h"
#include "bk_arch.h"
#include "os/mem.h"
#include "bk_coredump.h"
#include "memory.h"
#include "sys_sw_regs.h"
#include "reg_base.h"
#include <modules/pm.h>

/*
 * AP-power gating for the CP self-coredump.
 *
 * On AP-powerdown low-power builds (CONFIG_PM_AP_POWERDOWN_WHEN_LV) the CP can
 * assert while the AP / PSRAM power domain is off. A CP RAM read into a
 * powered-down domain stalls the shared SoC bus; the dump loop then stops
 * feeding the WWDT, so the NMI watchdog fires (reset reason "nmi watchdog")
 * and resets the chip mid-dump - truncating the coredump after the CP-local
 * SRAM regions and dropping the task list, backtrace and the "user except
 * handler end" marker. Skip those windows when the AP is not powered so the
 * dump always completes. Peripheral-register reads go through the CP-local
 * AHBP path and stay safe even when an AP bank is wedged, so only RAM windows
 * are gated here.
 */
static bool bk_coredump_window_needs_ap_power(uint32_t addr, uint32_t size)
{
    uint32_t end = addr + size;

    /* AP-shared SRAM banks (SRAM3..SRAM6), above the CP-local SRAM0..SRAM2. */
    if ((addr < SOC_SRAM_DATA_END) && (end > SOC_SRAM3_DATA_BASE)) {
        return true;
    }
    /* M55 (AP) TCM alias window. */
    if ((addr < SOC_USB_TCM_BASE) && (end > SOC_M55_TCM_BASE)) {
        return true;
    }
    /* PSRAM / QSPI media memory (AP / media power domain). */
    if ((addr < ((uint32_t)SOC_QSPI1_DATA_BASE + (uint32_t)SOC_QSPI1_DATA_SIZE)) &&
        (end > (uint32_t)SOC_PSRAM0_DATA_BASE)) {
        return true;
    }
    return false;
}

static bool bk_coredump_window_readable(uint32_t addr, uint32_t size)
{
    if (!bk_coredump_window_needs_ap_power(addr, size)) {
        return true;
    }
    return bk_pm_ap_boot_success_get();
}

/*
 * Power-aware wrapper around bk_coredump_write_memory(): dump the window when
 * it is reachable, otherwise emit a skip marker (so the offline parser sees an
 * explicit gap instead of silence) and return without touching the bus.
 */
static void bk_coredump_write_memory_checked(const char *name, uint32_t start_addr, uint32_t end_addr)
{
    if (!bk_coredump_window_readable(start_addr, end_addr - start_addr)) {
        bk_coredump_write_prompt(
            ">>>>skip mem dump, region: %s, stack_top=%08x, stack end=%08x (ap powered down)\r\n",
            name, start_addr, end_addr);
        return;
    }
    bk_coredump_write_memory(name, start_addr, end_addr);
}

/*
 * SYS_AHBP debug-mux capture.
 *
 * REG_0x23.dbug_mux[3:0] selects what is routed to the REG_0x25 readout.
 * Selector map:
 *   0 dbug_config0 (programmable via REG_0x22)   1 csi    2 vid_post   3 usbhs
 *   4 cpu0pc       5 cpu0fault[31:0]   6 cpu0fault[42:32]
 *   7 cpu1pc       8 cpu1fault[31:0]   9 cpu1fault[42:32]
 *   A [17:9]cpu1_INTNUM,[8:0]cpu0_INTNUM   B trace_clk/ctrl/data
 *
 * cpu0/1 fault[42:0] indicates whether a core is stalled on an outstanding /
 * locked bus access; cpuX_INTNUM indicates whether a core is in an ISR. These
 * reads go through the CP-local AHBP bus, so they are captured even when an AP
 * SRAM bank is wedged on subsequent LDRs.
 */
#define BK_AHBP_DBUG_MUX_MASK    0xFU
#define BK_AHBP_DBUG_MUX_CPU0PC  0x4U

static uint32_t bk_coredump_ahbp_read_mux(volatile uint32_t *ahbp,
                                          uint32_t config1, uint32_t mux)
{
    ahbp[0x23U] = config1 | (mux & BK_AHBP_DBUG_MUX_MASK);
    for (volatile uint32_t s = 0; s < 64U; s++) {
        (void)ahbp[0x23U];   /* let the debug-mux routing settle */
    }
    return ahbp[0x25U];
}

static void bk_coredump_log_ahb_arb_mux_sweep(const char *region_name)
{
    volatile uint32_t *ahbp = (volatile uint32_t *)((uintptr_t)SOC_SYS_AHBP_REG_BASE);
    uint32_t config1 = ahbp[0x23U] & ~BK_AHBP_DBUG_MUX_MASK;

    /* Full selector sweep (raw, for completeness incl. csi/vid_post/trace). */
    for (uint32_t mux = 0; mux < 16U; mux++) {
        bk_coredump_write_prompt(
            "agent_debug runId=V2 hypothesisId=H_MUX region=%s mux=%lu readout=0x%08lx\r\n",
            region_name, (unsigned long)mux,
            (unsigned long)bk_coredump_ahbp_read_mux(ahbp, config1, mux));
    }

    /* Decoded high-value signals: cpu0/1 PC, 43-bit fault vectors, INTNUM. */
    uint32_t cpu0pc   = bk_coredump_ahbp_read_mux(ahbp, config1, 0x4U);
    uint32_t cpu0f_lo = bk_coredump_ahbp_read_mux(ahbp, config1, 0x5U);
    uint32_t cpu0f_hi = bk_coredump_ahbp_read_mux(ahbp, config1, 0x6U);
    uint32_t cpu1pc   = bk_coredump_ahbp_read_mux(ahbp, config1, 0x7U);
    uint32_t cpu1f_lo = bk_coredump_ahbp_read_mux(ahbp, config1, 0x8U);
    uint32_t cpu1f_hi = bk_coredump_ahbp_read_mux(ahbp, config1, 0x9U);
    uint32_t intnum   = bk_coredump_ahbp_read_mux(ahbp, config1, 0xAU);
    uint32_t cfg0     = ahbp[0x22U];   /* dbug_config0 (selects mux=0 source) */

    /* Split into 2 lines so the cpu1 fault/INTNUM fields fit the 128B prompt buffer. */
    bk_coredump_write_prompt(
        "agent_debug runId=V3 hypothesisId=H_CPUFAULT region=%s "
        "cpu0pc=0x%08lx cpu0fault=0x%03lx%08lx cpu0_intnum=0x%03lx\r\n",
        region_name,
        (unsigned long)cpu0pc, (unsigned long)(cpu0f_hi & 0x7ffU), (unsigned long)cpu0f_lo,
        (unsigned long)(intnum & 0x1ffU));
    bk_coredump_write_prompt(
        "agent_debug runId=V3 hypothesisId=H_CPUFAULT region=%s "
        "cpu1pc=0x%08lx cpu1fault=0x%03lx%08lx cpu1_intnum=0x%03lx dbug_config0=0x%08lx\r\n",
        region_name,
        (unsigned long)cpu1pc, (unsigned long)(cpu1f_hi & 0x7ffU), (unsigned long)cpu1f_lo,
        (unsigned long)((intnum >> 9) & 0x1ffU), (unsigned long)cfg0);

    /* restore the cpu0pc selector for any later raw SYS_AHBP dump */
    ahbp[0x23U] = config1 | BK_AHBP_DBUG_MUX_CPU0PC;
    (void)ahbp[0x23U];
}

/*
 * The coredump prompt buffer is char[128] (bk_coredump_uart.c), so a single
 * prompt longer than ~118 visible chars is silently truncated by vsnprintf.
 * Emit at most 4 hex fields per line so every word survives.
 */
static void bk_coredump_dump_reg_block(const char *tag, const char *region_name,
                                       volatile uint32_t *base, uint32_t nwords)
{
    for (uint32_t i = 0; i < nwords; i += 4U) {
        bk_coredump_write_prompt(
            "agent_debug runId=V3 hypothesisId=%s region=%s "
            "w%lu=0x%08lx w%lu=0x%08lx w%lu=0x%08lx w%lu=0x%08lx\r\n",
            tag, region_name,
            (unsigned long)(i + 0U), (unsigned long)base[i + 0U],
            (unsigned long)(i + 1U), (unsigned long)base[i + 1U],
            (unsigned long)(i + 2U), (unsigned long)base[i + 2U],
            (unsigned long)(i + 3U), (unsigned long)base[i + 3U]);
    }
}

static void bk_coredump_log_ahb_state_before_sram(const char *region_name)
{
    volatile uint32_t *ahbp  = (volatile uint32_t *)((uintptr_t)SOC_SYS_AHBP_REG_BASE);
    volatile uint32_t *hpdma = (volatile uint32_t *)((uintptr_t)SOC_HPDMA_REG_BASE);
    volatile uint32_t *h26e  = (volatile uint32_t *)((uintptr_t)SOC_H26E_REG_BASE);
    volatile uint32_t *h26d  = (volatile uint32_t *)((uintptr_t)SOC_H26D_REG_BASE);
    volatile uint32_t *gpu   = (volatile uint32_t *)((uintptr_t)SOC_GPU_REG_BASE);
    volatile uint32_t *dpu   = (volatile uint32_t *)((uintptr_t)SOC_DPU_REG_BASE);

    /* SYS_AHBP control snapshot, split so reg21/reg23 are not truncated.
     * regA = per-engine clock-enable (confirms H26E/H26D/GPU/DPU are clocked);
     * note 0x08..0x0b are clock cksel/ckdiv, NOT bus-busy bits. */
    bk_coredump_write_prompt(
        "agent_debug runId=V3 hypothesisId=H_AHBP region=%s "
        "regc=0x%08lx rege=0x%08lx regf=0x%08lx regA=0x%08lx\r\n",
        region_name,
        (unsigned long)ahbp[0xcU], (unsigned long)ahbp[0xeU],
        (unsigned long)ahbp[0xfU], (unsigned long)ahbp[0xaU]);
    bk_coredump_write_prompt(
        "agent_debug runId=V3 hypothesisId=H_AHBP region=%s "
        "reg20=0x%08lx reg21=0x%08lx reg23=0x%08lx\r\n",
        region_name,
        (unsigned long)ahbp[0x20U], (unsigned long)ahbp[0x21U],
        (unsigned long)ahbp[0x23U]);

    /* capture cpu0/1 PC, fault vectors and INTNUM via dbug_mux */
    bk_coredump_log_ahb_arb_mux_sweep(region_name);

    for (uint32_t ch = 0; ch < 4U; ch++) {
        uint32_t off = (0x40U + ch * 0x100U) / 4U;
        /* split into two lines so src_rd/dst_wr/status survive the 128B buffer */
        bk_coredump_write_prompt(
            "agent_debug runId=V3 hypothesisId=H_HPDMA region=%s ch=%lu "
            "ctrl=0x%08lx src_start=0x%08lx dst_start=0x%08lx req_mux=0x%08lx\r\n",
            region_name, (unsigned long)ch,
            (unsigned long)hpdma[off + 0U], (unsigned long)hpdma[off + 2U],
            (unsigned long)hpdma[off + 1U], (unsigned long)hpdma[off + 7U]);
        bk_coredump_write_prompt(
            "agent_debug runId=V3 hypothesisId=H_HPDMA region=%s ch=%lu "
            "src_rd=0x%08lx dst_wr=0x%08lx status=0x%08lx\r\n",
            region_name, (unsigned long)ch,
            (unsigned long)hpdma[off + 0xAU], (unsigned long)hpdma[off + 0xBU],
            (unsigned long)hpdma[off + 0xCU]);
    }

    /*
     * Full register banks, 4 words/line so nothing truncates.
     * H_H26E: H264 encoder status. H_H26D: VC8000 decoder shared by MJPEG
     * decode. H_GPU / H_DPU: other AHB masters that can hold an SRAM slave.
     */
    bk_coredump_dump_reg_block("H_H26E", region_name, h26e, 16U);
    bk_coredump_dump_reg_block("H_H26D", region_name, h26d, 16U);
    bk_coredump_dump_reg_block("H_GPU",  region_name, gpu,  8U);
    bk_coredump_dump_reg_block("H_DPU",  region_name, dpu,  8U);
}

/*
 * Returns true for the wedge-prone AP SRAM banks (SRAM3 / SRAM4), used to gate
 * the AHB snapshot + sub-bank sweep to just those banks.
 */
static bool bk_coredump_entry_is_ap_sram(const bk_dump_mem_info_t *info)
{
    if ((info == NULL) || (info->name == NULL)) {
        return false;
    }
    /* The SRAM list uses bare names "SRAM3" / "SRAM4". */
    return (info->name[0] == 'S') && (info->name[1] == 'R') &&
           (info->name[2] == 'A') && (info->name[3] == 'M') &&
           ((info->name[4] == '3') || (info->name[4] == '4'));
}

/*
 * SRAM sub-bank wedge probe.
 *
 * SRAM3/4 can wedge on read at a 64KB-aligned address that varies per fault.
 * This walks the bank in 64KB strides, emitting one "probe addr=..." line
 * BEFORE each read; the last such line that reaches the UART identifies the
 * wedged sub-bank (the following LDR never returned). The probe uses a single
 * uncached LDR per stride and will hang on a read-side wedge, so it is only
 * run after the raw memory image has been secured.
 */
#define BK_COREDUMP_SRAM_SUBBANK_STRIDE 0x10000U

static void bk_coredump_probe_sweep_sram(const char *region_name,
                                         uint32_t base, uint32_t size)
{
    uint32_t end = base + size;
    for (uint32_t addr = base; addr < end; addr += BK_COREDUMP_SRAM_SUBBANK_STRIDE) {
        bk_coredump_write_prompt(
            "agent_debug runId=V17 hypothesisId=H_SUBBANK_WEDGE region=%s probe addr=0x%08lx\r\n",
            region_name, (unsigned long)addr);
        volatile uint32_t sink = *(volatile uint32_t *)(uintptr_t)addr;
        (void)sink;
        bk_coredump_write_prompt(
            "agent_debug runId=V17 hypothesisId=H_SUBBANK_WEDGE region=%s ok addr=0x%08lx\r\n",
            region_name, (unsigned long)addr);
    }
}

typedef void (*bk_dump_mem_getter_t)(bk_dump_mem_info_t *info);

#define COREDUMP_PSRAM0_PROBE_PRE_SIZE    (0x100U)
#define COREDUMP_PSRAM0_PROBE_POST_SIZE   (0x10U)
#define COREDUMP_PSRAM0_PROBE_PATTERN     (0x12345678U)
#define COREDUMP_AP_SRAM_PROBE_SIZE       (0x20U)
#define COREDUMP_AP_DTCM_PROBE_SIZE       (0x20U)
#define COREDUMP_AP_SRAM_PROBE_PATTERN    (0x5a5a1234U)
#define COREDUMP_AP_DTCM_PROBE_PATTERN    (0xa5a54321U)

static void bk_dump_psram0_base_write_probe(void)
{
    uint32_t probe_addr = (uint32_t)SOC_GET_NS_ADDR(SOC_PSRAM0_DATA_BASE);
    volatile uint32_t *probe = (volatile uint32_t *)(uintptr_t)probe_addr;
    uint32_t before = probe[0];

    bk_coredump_write_memory("PSRAM0_BASE_PRE", probe_addr,
        probe_addr + COREDUMP_PSRAM0_PROBE_PRE_SIZE);

    probe[0] = COREDUMP_PSRAM0_PROBE_PATTERN;
    __DSB();

    bk_coredump_write_prompt(
        "PSRAM0_BASE_WRITE_PROBE addr=0x%08lx before=0x%08lx write=0x%08lx after=0x%08lx\r\n",
        probe_addr, before, COREDUMP_PSRAM0_PROBE_PATTERN, probe[0]);

    bk_coredump_write_memory("PSRAM0_BASE_POST", probe_addr,
        probe_addr + COREDUMP_PSRAM0_PROBE_POST_SIZE);

    probe[0] = before;
    __DSB();
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

    /* Preserve the captured fault scene after the diagnostic write test. */
    probe[0] = before;
    __DSB();
}

static const bk_dump_mem_info_t *bk_coredump_find_sram_info(char bank)
{
    uint32_t count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *list = bk_get_sram_info_list();

    for (uint32_t i = 0; i < count; i++) {
        const char *name = list[i].name;
        if (name != NULL &&
            name[0] == 'S' && name[1] == 'R' && name[2] == 'A' &&
            name[3] == 'M' && name[4] == bank && name[5] == '\0') {
            return &list[i];
        }
    }
    return NULL;
}

static void bk_dump_ap_sram_dtcm_write_probes(void)
{
    const bk_dump_mem_info_t *sram5 = bk_coredump_find_sram_info('5');
    const bk_dump_mem_info_t *sram3 = bk_coredump_find_sram_info('3');
    bk_dump_mem_info_t dtcm = {0};

    if (sram5 != NULL && sram5->size >= COREDUMP_AP_SRAM_PROBE_SIZE) {
        bk_dump_fixed_write_probe("AP_SRAM5_WRITE_PROBE",
            "AP_SRAM5_PRE", "AP_SRAM5_POST", sram5->start_addr,
            COREDUMP_AP_SRAM_PROBE_SIZE, COREDUMP_AP_SRAM_PROBE_PATTERN);
    }

    if (sram3 != NULL && sram3->size >= COREDUMP_AP_SRAM_PROBE_SIZE) {
        /* start_addr is SRAM3_DUMP_BASE, after the AP Secure shim window. */
        bk_dump_fixed_write_probe("AP_SRAM3_WRITE_PROBE",
            "AP_SRAM3_PRE", "AP_SRAM3_POST", sram3->start_addr,
            COREDUMP_AP_SRAM_PROBE_SIZE, COREDUMP_AP_SRAM_PROBE_PATTERN);
    }

    bk_get_ap_dtcm_info(&dtcm);
    if (dtcm.start_addr != 0U && dtcm.size >= COREDUMP_AP_DTCM_PROBE_SIZE) {
        bk_dump_fixed_write_probe("AP_DTCM_WRITE_PROBE",
            "AP_DTCM_PRE", "AP_DTCM_POST", dtcm.start_addr,
            COREDUMP_AP_DTCM_PROBE_SIZE, COREDUMP_AP_DTCM_PROBE_PATTERN);
    }
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

#if CONFIG_SPE
    bk_coredump_write_memory("MEM_CHECK", (uint32_t)SOC_MEM_CHECK_REG_BASE,
        (uint32_t)(SOC_MEM_CHECK_REG_BASE + 0x81 * 4));
#endif
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

/*
 * Diagnostic probes for the wedge-prone SRAM banks (SRAM3 / SRAM4): emit the
 * AHB / HPDMA / H26E snapshot, then walk the bank in 64KB strides. The sweep
 * deliberately performs raw LDRs and will hang on a wedged sub-bank, so it is
 * only ever run AFTER the raw memory image has been secured.
 */
static void bk_coredump_probe_wedge_sram_banks(void)
{
    uint32_t sram_info_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram_info_list = bk_get_sram_info_list();

    for (uint32_t i = 0; i < sram_info_count; i++) {
        if (!bk_coredump_entry_is_ap_sram(&sram_info_list[i])) {
            continue;
        }
        bk_coredump_log_ahb_state_before_sram(sram_info_list[i].name);
        bk_coredump_probe_sweep_sram(sram_info_list[i].name,
                                     sram_info_list[i].start_addr,
                                     sram_info_list[i].size);
    }
}

/* Dump the peripheral register banks only (RAM banks are handled in the RAM
 * phase). Register reads go through the CP-local AHBP path and are normally
 * safe even when an AP SRAM bank is wedged. */
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

/* Phase 1 - RAM image. Captured first so that a stall in the phase-2 probes
 * cannot cost us the RAM contents. */
static void bk_coredump_ap_dump_ram(void)
{
    bk_dump_ap_dtcm();
    bk_dump_ap_all_sram();
    bk_dump_ap_extra_mem();
    bk_dump_ap_psram_mem();
}

/* AP peripheral register banks only (safe AHBP reads). Used as the P1-2
 * downgrade target when the AP cores' stop could not be confirmed and cross-
 * reading AP RAM would risk a bus stall. */
void bk_coredump_ap_dump_regs(void)
{
    bk_dump_ap_peri_regs();
}

/* Phase 1 RAM image + Phase 2a peripheral register banks (both safe: the RAM
 * reads are power-gated and the register reads go through the CP-local AHBP
 * path). No destructive probes - callers emit the end marker after this and run
 * the hang-prone probes afterwards (P1-3). */
void bk_coredump_ap_dump_ram_and_regs(void)
{
    bk_coredump_ap_dump_ram();
    bk_dump_ap_peri_regs();
}

/* Phase 2b - destructive AP diagnostic probes (live write probes + hang-prone
 * SRAM sub-bank read sweep). Engineering-only: compiled out of Release builds
 * (P2-2), and callers run this only AFTER the end marker (P1-3). */
void bk_coredump_ap_dump_probes(void)
{
#if CONFIG_DEBUG_VERSION
    bk_dump_psram0_base_write_probe();
    bk_dump_ap_sram_dtcm_write_probes();
    bk_coredump_probe_wedge_sram_banks();
#else
    bk_coredump_write_prompt(">>>>skip ap diagnostic probes (release build)\r\n");
#endif
}

void bk_coredump_ap_memory(void)
{
    /*
     * Dump-order policy (integrity first):
     *   Phase 1  RAM image  : DTCM / SRAM / PSRAM / extra system memory.
     *   Phase 2a register banks (safe AHBP reads).
     *   Phase 2b destructive probes (AHB debug snapshot, live write probes,
     *            SRAM sub-bank read sweep) that may stall on a wedged bus -
     *            Debug-only and run last.
     *
     * Phase 1 runs first so the RAM image is never lost to a phase-2 stall.
     */
    bk_coredump_ap_dump_ram_and_regs();
    bk_coredump_ap_dump_probes();
}

void bk_dump_peri_regs(void)
{
    uint32_t peri_reg_info_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri_reg_info_list = bk_get_peri_reg_info_list();

    /* Register banks only. These reads go through the CP-local AHBP path and
     * stay safe even when an AP bank is wedged, so they are not power-gated.
     * The hang-prone live-write/sub-bank probes are split into
     * bk_dump_peri_probes() so the exception path can run them AFTER the
     * task-list/backtrace/epilogue. */
    for (uint32_t i = 0; i < peri_reg_info_count; i++) {
        bk_coredump_write_memory(
            peri_reg_info_list[i].name,
            peri_reg_info_list[i].start_addr,
            peri_reg_info_list[i].start_addr + peri_reg_info_list[i].size
        );
    }
}

/* Diagnostic probes (live write probes + hang-prone sub-bank sweep). They
 * write/read PSRAM and AP SRAM/DTCM, which stall the bus when the AP power
 * domain is down - skip them entirely in that case. Because AON WDT is never
 * stopped, the wedge sweep can trip the watchdog reset; run this LAST (after
 * the epilogue) so a stall here cannot cost us the essential dump. */
void bk_dump_peri_probes(void)
{
#if CONFIG_DEBUG_VERSION
    /* P2-2: destructive AP/PSRAM probes are engineering-only. */
    if (bk_pm_ap_boot_success_get()) {
        bk_dump_psram0_base_write_probe();
        bk_dump_ap_sram_dtcm_write_probes();
        bk_coredump_probe_wedge_sram_banks();
    } else {
        bk_coredump_write_prompt(">>>>skip ap/psram diagnostic probes (ap powered down)\r\n");
    }
#else
    bk_coredump_write_prompt(">>>>skip ap/psram diagnostic probes (release build)\r\n");
#endif
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
        bk_coredump_write_memory_checked(
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
        bk_coredump_write_memory_checked("EXTRA_MEM", dump_sys_mem_info[i].start_addr, dump_sys_mem_info[i].start_addr + dump_sys_mem_info[i].size);
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
        bk_coredump_write_memory_checked(mem_info.name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
    // dump psram bss
    bk_get_psram_bss_info(&mem_info);
    if (mem_info.start_addr != 0 && mem_info.size != 0) {
        bk_coredump_write_memory_checked(mem_info.name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
    // dump psram data
    bk_get_psram_data_info(&mem_info);
    if (mem_info.start_addr != 0 && mem_info.size != 0) {
        bk_coredump_write_memory_checked(mem_info.name, mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }

    /* AP PSRAM heap lives in the AP/media power domain; skip it when AP is off. */
    if (bk_pm_ap_boot_success_get()) {
        bk_dump_ap_heap_mem();
    } else {
        bk_coredump_write_prompt(">>>>skip mem dump, region: AP_PSRAM_HEAP (ap powered down)\r\n");
    }
}
