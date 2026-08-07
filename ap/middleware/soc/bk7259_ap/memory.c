// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "os/os.h"
#include "os/mem.h"
#include "bk_arch.h"
#include "common/bk_assert.h"
#include "sdkconfig.h"
#include "stack_base.h"
#include "reg_base.h"
#include "memory.h"

extern unsigned char _data_ram_begin;
#define RAM_START_ADDRESS  ((uint32_t)&_data_ram_begin)

extern unsigned char __data_start__;
#define DATA_START_ADDRESS ((uint32_t)&__data_start__)

extern unsigned char _data_ram_end;
#define DATA_END_ADDRESS ((uint32_t)&_data_ram_end)

extern unsigned char _bss_start;
#define BSS_START_ADDRESS ((uint32_t)&_bss_start)

extern unsigned char _bss_end;
#define BSS_END_ADDRESS ((uint32_t)&_bss_end)

extern unsigned char _heap_start, _heap_end;
#define HEAP_START_ADDRESS    (void*)&_heap_start
#define HEAP_END_ADDRESS      (void*)&_heap_end

extern unsigned char __dtcm_start__;
#define DTCM_START_ADDRESS ((uint32_t)&__dtcm_start__)

extern unsigned char __dtcm_end__;
#define DTCM_END_ADDRESS ((uint32_t)&__dtcm_end__)

extern unsigned char __itcm_start__;
#define ITCM_START_ADDRESS ((uint32_t)&__itcm_start__)

extern unsigned char __itcm_end__;
#define ITCM_END_ADDRESS ((uint32_t)&__itcm_end__)

extern unsigned char __iram_start__;
#define IRAM_START_ADDRESS ((uint32_t)&__iram_start__)

extern unsigned char __iram_end__;
#define IRAM_END_ADDRESS ((uint32_t)&__iram_end__)

#if (CONFIG_PSRAM_AS_SYS_MEMORY)
#define PSRAM_HEAP_ADDR         CONFIG_AP_PSRAM_HEAP_ADDR
#define PSRAM_HEAP_SIZE         CONFIG_AP_PSRAM_HEAP_SIZE
#endif

#if (CONFIG_AP_PSRAM_SECTION_ADDR)
extern unsigned char __psram_data_start__;
#define PSRAM_DATA_START_ADDRESS ((uint32_t)&__psram_data_start__)

extern unsigned char __psram_data_end__;
#define PSRAM_DATA_END_ADDRESS ((uint32_t)&__psram_data_end__)

extern unsigned char __psram_bss_start__;
#define PSRAM_BSS_START_ADDRESS ((uint32_t)&__psram_bss_start__)

extern unsigned char __psram_bss_end__;
#define PSRAM_BSS_END_ADDRESS ((uint32_t)&__psram_bss_end__)
#endif //#if (CONFIG_AP_PSRAM_SECTION_ADDR)


#if CONFIG_SOC_SMP
extern unsigned char _estack_core0;
extern unsigned char _estack_core1;
#else
extern unsigned char _estack;
#endif

extern unsigned char _stext;
extern unsigned char __etext;


#if CONFIG_AP_PSRAM_TEXT_ADDR
extern unsigned char __psram_text_start__;
extern unsigned char __psram_text_end__;
#endif

#if CONFIG_PSRAM
extern unsigned char __psram_code_lma;
extern unsigned char __psram_code_start__;
extern unsigned char __psram_code_end__;
#endif

#define FLASH_CODE_REGION_START (uint32_t)&_stext
#define FLASH_CODE_REGION_END (uint32_t)&__etext


#if CONFIG_SOC_SMP
#define CORE0_MSP_BOTTOM (uint32_t)&_estack_core0
#define CORE1_MSP_BOTTOM (uint32_t)&_estack_core1
#else
#define CORE0_MSP_BOTTOM (uint32_t)&_estack
#endif

#if CONFIG_AP_PSRAM_TEXT_ADDR
#define PSRAM_CODE_REGION_START (uint32_t)&__psram_text_start__
#define PSRAM_CODE_REGION_END (uint32_t)&__psram_text_end__
#endif

#if !CONFIG_SPE && defined(CONFIG_CP_SPE_RAM_ADDR) && \
    defined(CONFIG_CP_SPE_RAM_SIZE) && defined(CONFIG_AP_SPE_RAM_ADDR) && \
    defined(CONFIG_AP_SPE_RAM_SIZE)
#if CONFIG_SRAM_DIRECT_ADDR
#define NS_SRAM_CPU_ADDR(addr) \
    ((uint32_t)(addr) + SOC_S_NS_ADDR_DIFF + SOC_SRAM_DIRECT_ADDR_BIT)
#else
#define NS_SRAM_CPU_ADDR(addr) ((uint32_t)(addr) + SOC_S_NS_ADDR_DIFF)
#endif
_Static_assert(NS_SRAM_CPU_ADDR(CONFIG_CP_SPE_RAM_ADDR) == SOC_SRAM0_DATA_BASE,
    "CP secure RAM must start at SRAM0");
_Static_assert(CONFIG_CP_SPE_RAM_SIZE < SOC_SRAM0_DATA_SIZE,
    "CP secure RAM exceeds SRAM0");
_Static_assert(NS_SRAM_CPU_ADDR(CONFIG_AP_SPE_RAM_ADDR) == SOC_SRAM3_DATA_BASE,
    "AP secure RAM must start at SRAM3");
_Static_assert(CONFIG_AP_SPE_RAM_SIZE < SOC_SRAM3_DATA_SIZE,
    "AP secure RAM exceeds SRAM3");
#define SRAM0_DUMP_BASE \
    (NS_SRAM_CPU_ADDR(CONFIG_CP_SPE_RAM_ADDR) + CONFIG_CP_SPE_RAM_SIZE)
#define SRAM0_DUMP_SIZE (SOC_SRAM0_DATA_SIZE - CONFIG_CP_SPE_RAM_SIZE)
#define SRAM3_DUMP_BASE \
    (NS_SRAM_CPU_ADDR(CONFIG_AP_SPE_RAM_ADDR) + CONFIG_AP_SPE_RAM_SIZE)
#define SRAM3_DUMP_SIZE (SOC_SRAM3_DATA_SIZE - CONFIG_AP_SPE_RAM_SIZE)
#else
#define SRAM0_DUMP_BASE SOC_SRAM0_DATA_BASE
#define SRAM0_DUMP_SIZE SOC_SRAM0_DATA_SIZE
#define SRAM3_DUMP_BASE SOC_SRAM3_DATA_BASE
#define SRAM3_DUMP_SIZE SOC_SRAM3_DATA_SIZE
#endif

const bk_dump_mem_info_t bk7259_sram_info[] = {
    {"SRAM0", SRAM0_DUMP_BASE, SRAM0_DUMP_SIZE},
    {"SRAM1", SOC_SRAM1_DATA_BASE, SOC_SRAM1_DATA_SIZE},
    {"SRAM2", SOC_SRAM2_DATA_BASE, SOC_SRAM2_DATA_SIZE},
    {"SRAM3", SRAM3_DUMP_BASE, SRAM3_DUMP_SIZE},
    {"SRAM4", SOC_SRAM4_DATA_BASE, SOC_SRAM4_DATA_SIZE},
    {"SRAM5", SOC_SRAM5_DATA_BASE, SOC_SRAM5_DATA_SIZE},
    {"SRAM6", SOC_SRAM6_DATA_BASE, SOC_SRAM6_DATA_SIZE},
};

const bk_dump_mem_info_t* bk_get_sram_info_list(void)
{
    return bk7259_sram_info;
}

uint32_t bk_get_sram_info_count(void)
{
    return sizeof(bk7259_sram_info) / sizeof(bk_dump_mem_info_t);
}

const bk_dump_mem_info_t bk7259_peri_reg_info[] = {
    {"SYS", (uint32_t)SOC_SYS_REG_BASE, (0x5c*4)},
    // flash regs warning!!!
    {"FLASH", (uint32_t)SOC_FLASH_REG_BASE, (0x20*4)},
    {"HSPL0_CFG", (uint32_t)SOC_HSPL0_REG_BASE, (0x10*4)},
    {"HSPL0_STA", (uint32_t)SOC_HSPL0_REG_BASE + (0x20*4), (0x10*4)},
    {"HSPL1_CFG", (uint32_t)SOC_HSPL1_REG_BASE, (0x10*4)},
    {"HSPL1_STA", (uint32_t)SOC_HSPL1_REG_BASE + (0x20*4), (0x10*4)},
    {"AON_PMU", (uint32_t)SOC_AON_PMU_REG_BASE, (0x7f*4)},
#if (CONFIG_SUPPORT_IO_MATRIX)
    {"IOMX", (uint32_t)SOC_IOMX_REG_BASE + (0x40*4), (0x52*4)},
#else
    {"AON_GPIO", (uint32_t)SOC_AON_GPIO_REG_BASE+ (0x2*4), (0x30*4)},
#endif
#if CONFIG_GENERAL_DMA
    {"GENER_DMA", (uint32_t)SOC_GENER_DMA_REG_BASE, (0x44*4)},
#if (SOC_DMA_UNIT_NUM > 1)
    {"GENER_DMA1", (uint32_t)SOC_GENER_DMA1_REG_BASE, (0x44*4)},
#endif
#endif
#if CONFIG_MAILBOX
    {"MBOX0", (uint32_t)SOC_MBOX0_REG_BASE, (0x38*4)},
#endif
#if CONFIG_AON_RTC
    {"AON_RTC", (uint32_t)SOC_AON_RTC_REG_BASE, (0x0a*4)},
#endif
#if CONFIG_PSRAM
    {"PSRAM0", (uint32_t)SOC_PSRAM0_REG_BASE, (0x18*4)},
    {"PSRAM1", (uint32_t)SOC_PSRAM1_REG_BASE, (0x18*4)},
#endif
    /* Bus-stall forensics (mirror of CP-side peri_reg_info[]).
     * Sizes/offsets must stay in sync with cp/middleware/soc/bk7259/memory.c
     * so AP self-dumps and CP-initiated AP dumps yield identical regions.
     *
     *   HPDMA  : 0x100*4 covers ctrl + status of all 16 channels.
     *   ISP    : 0x80*4  covers ID + global control block.
     *   ISP_MI : separate 0x10*4 window at offset 0x1070*4 captures
     *            ISP_TIMEOUT_CFG_STREAMxx / ISP_STREAM_STATUS_STREAMxx.
     *   H26E   : 0x80*4  covers encode engine status.
     *   DPU    : 0x100*4 covers viv_dc chip-ID + interrupt regs.
     *   GPU    : 0x100*4 covers AQHIIDLEREG / AQINTACK / AQINTREN.
     *   PPHS   : 0x10*4  AHB access controller (M55 side), regs 0..7.
     *            Reg0x4[31] ahbp_ahb_sresp gates bus-error response.
     *   PPRO   : 0x24*4  AHB access controller (M52 side), regs 0..0x23.
     *            Reg0x7/0x8/0x9 carry the AON/BAK sresp control bits.
     *   SYS_AHBP: 0x60*4  M55 SYSTEM block @ 0x48000000 — PLL/clock/reset/
     *            power-domain + per-master QoS + DPU sub-gates.  Without
     *            this we can't tell whether a victim peripheral was
     *            clock-gated or in reset at hang time. */
    {"HPDMA",  (uint32_t)SOC_HPDMA_REG_BASE,            (0x100*4)},
    {"ISP",    (uint32_t)SOC_ISP_REG_BASE,              (0x80*4)},
    {"ISP_MI", (uint32_t)SOC_ISP_REG_BASE + (0x1070*4), (0x10*4)},
    {"H26E",   (uint32_t)SOC_H26E_REG_BASE,             (0x80*4)},
    {"DPU",    (uint32_t)SOC_DPU_REG_BASE,              (0x100*4)},
    {"GPU",    (uint32_t)SOC_GPU_REG_BASE,              (0x100*4)},
    {"PPHS",   (uint32_t)SOC_PPHS_REG_BASE,             (0x10*4)},
    {"PPRO",   (uint32_t)SOC_PPRO_REG_BASE,             (0x24*4)},
    {"SYS_AHBP", (uint32_t)SOC_SYS_AHBP_REG_BASE,       (0x60*4)},
};

const bk_dump_mem_info_t* bk_get_peri_reg_info_list(void)
{
    return bk7259_peri_reg_info;
}

uint32_t bk_get_peri_reg_info_count(void)
{
    return sizeof(bk7259_peri_reg_info) / sizeof(bk_dump_mem_info_t);
}

void bk_get_psram_heap_info(bk_dump_mem_info_t *info)
{
    info->name = "PSRAM_HEAP";
#if CONFIG_PSRAM_AS_SYS_MEMORY
    if (bk_psram_heap_get_used_count() == 0) {
        info->start_addr = 0;
        info->size = 0;
    } else {
        info->start_addr = PSRAM_HEAP_ADDR;
        info->size = PSRAM_HEAP_SIZE;
    }
#else
    info->start_addr = 0;
    info->size = 0;
#endif
}

void bk_get_psram_bss_info(bk_dump_mem_info_t *info)
{
    info->name = "PSRAM_BSS";
#if CONFIG_AP_PSRAM_SECTION_ADDR
    info->start_addr = PSRAM_DATA_START_ADDRESS;
    info->size = PSRAM_DATA_END_ADDRESS - PSRAM_DATA_START_ADDRESS;
#else
    info->start_addr = 0;
    info->size = 0;
#endif
}

void bk_get_psram_data_info(bk_dump_mem_info_t *info)
{
    info->name = "PSRAM_DATA";
#if CONFIG_AP_PSRAM_SECTION_ADDR
    info->start_addr = PSRAM_BSS_START_ADDRESS;
    info->size = PSRAM_BSS_END_ADDRESS - PSRAM_BSS_START_ADDRESS;
#else
    info->start_addr = 0;
    info->size = 0;
#endif
}

__attribute__((section(".iram"), noinline)) void bk_get_psram_code_info(bk_psram_code_info_t *info)
{
    if (info == NULL) {
        return;
    }

    info->run_addr = 0;
    info->load_addr = 0;
    info->size = 0;

#if CONFIG_PSRAM
    uint32_t start = (uint32_t)&__psram_code_start__;
    uint32_t end = (uint32_t)&__psram_code_end__;

    if (end > start) {
        info->run_addr = start;
        info->load_addr = (uint32_t)&__psram_code_lma;
        info->size = end - start;
    }
#endif
}

uint32_t mem_is_including_tcm(void)
{
#if (CONFIG_SUPPORT_ITCM) || (CONFIG_SUPPORT_DTCM)
    return 1;
#else
    return 0;
#endif
}

uint32_t mem_dump_dtcm(void)
{
    return 0;
}

uint32_t mem_dump_itcm(void)
{
    return 0;
}

void bk_get_dtcm_info(bk_mem_addr_t *info)
{
    info->start_addr = DTCM_START_ADDRESS;
    info->size = DTCM_END_ADDRESS - DTCM_START_ADDRESS;
}

void bk_get_itcm_info(bk_mem_addr_t *info)
{
    info->start_addr = ITCM_START_ADDRESS;
    info->size = ITCM_END_ADDRESS - ITCM_START_ADDRESS;
}

static inline bool addr_is_in_flash_txt(uint32_t addr)
{
    return ((addr >= FLASH_CODE_REGION_START) && (addr < FLASH_CODE_REGION_END));
}

static inline bool addr_is_in_itcm_txt(uint32_t addr)
{
    return ((addr >= ITCM_START_ADDRESS) && (addr < ITCM_END_ADDRESS));
}

static inline bool addr_is_in_dtcm(uint32_t addr)
{
    return ((addr >= DTCM_START_ADDRESS) && (addr < DTCM_END_ADDRESS));
}

static inline bool addr_is_in_sram(uint32_t addr)
{
    addr = SOC_SRAM_PERI_ADDR(addr);
    return ((addr >= SOC_RAM_BASE) && (addr < SOC_RAM_BASE + SOC_RAM_SIZE));
}

static inline bool addr_is_in_iram_txt(uint32_t addr)
{
    return ((addr >= IRAM_START_ADDRESS) && (addr < IRAM_END_ADDRESS));
}

static inline bool addr_is_in_psram(uint32_t addr)
{
#if CONFIG_CP_PSRAM_HEAP_ADDR
    return ((addr >= CONFIG_CP_PSRAM_HEAP_ADDR) && (addr < CONFIG_CP_PSRAM_HEAP_ADDR + CONFIG_CP_PSRAM_HEAP_SIZE));
#else
    return false;
#endif
}

static inline bool addr_is_in_psram_txt(uint32_t addr)
{
#if CONFIG_AP_PSRAM_TEXT_ADDR
    return ((addr >= PSRAM_CODE_REGION_START) && (addr < PSRAM_CODE_REGION_END));
#else
    return false;
#endif
}

bool bk_check_addr_in_code_section(uint32_t addr)
{
    return addr_is_in_itcm_txt(addr) || addr_is_in_flash_txt(addr) ||
           addr_is_in_iram_txt(addr) || addr_is_in_psram_txt(addr);
}

bool bk_check_addr_in_ram(uint32_t addr)
{
    return addr_is_in_dtcm(addr) || addr_is_in_sram(addr) ||
           addr_is_in_iram_txt(addr) || addr_is_in_psram(addr);
}

uint32_t bk_get_msp_bottom(void)
{
#if CONFIG_SOC_SMP
    if (portGET_CORE_ID() == 0) {
        return CORE0_MSP_BOTTOM;
    } else if (portGET_CORE_ID() == 1) {
        return CORE1_MSP_BOTTOM;
    } else {
        BK_RAW_LOGW(NULL, "unknown core id\r\n");
        return 0;
    }
#else
    return CORE0_MSP_BOTTOM;
#endif
}

extern uint32_t * vTaskStackAddr(void);
extern uint32_t vTaskStackSize(void);
extern uint32_t vTaskTcbAddr(void);
uint32_t bk_get_current_stack_bottom(void)
{
    uint32_t tcb_addr = vTaskTcbAddr();
    if (!bk_check_addr_in_ram(tcb_addr)) {
        return __get_PSP() + 1024;
    }
    if (!vTaskStackAddr()) {
        return 0;
    }
    return (uint32_t)vTaskStackAddr() + (vTaskStackSize() * sizeof(uint32_t));
}

uint32_t mem_addr_is_in_itcm_range(uint32_t addr)
{
    return 0;
}

uint32_t mem_get_heap_start_addr(void)
{
    return (uint32_t)HEAP_START_ADDRESS;
}

uint32_t mem_get_heap_size(void)
{
    return (uint32_t)HEAP_END_ADDRESS - (uint32_t)HEAP_START_ADDRESS;
}

void mem_show_info(void)
{
    BK_LOGD(RTOS_TAG, "\r\n");
    BK_LOGD(RTOS_TAG, "%-8s %-8s %-8s %-8s\r\n", "mem_type", "start", "end", "size");
    BK_LOGD(RTOS_TAG, "%-8s %-8s %-8s %-8s\r\n", "--------", "--------", "--------", "--------");

    if(mem_is_including_tcm()){
        BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "itcm", ITCM_START_ADDRESS, ITCM_END_ADDRESS, (ITCM_END_ADDRESS - ITCM_START_ADDRESS));
        BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "dtcm", DTCM_START_ADDRESS, DTCM_END_ADDRESS, (DTCM_END_ADDRESS - DTCM_START_ADDRESS));
    }

    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "ram", RAM_START_ADDRESS, HEAP_END_ADDRESS, (HEAP_END_ADDRESS - RAM_START_ADDRESS));
    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "non_heap", RAM_START_ADDRESS, HEAP_START_ADDRESS, (HEAP_START_ADDRESS - RAM_START_ADDRESS));
    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "iram", IRAM_START_ADDRESS, IRAM_END_ADDRESS, (IRAM_END_ADDRESS - IRAM_START_ADDRESS));
    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "data", DATA_START_ADDRESS, DATA_END_ADDRESS, (DATA_END_ADDRESS - DATA_START_ADDRESS));
    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "bss", BSS_START_ADDRESS, BSS_END_ADDRESS, (BSS_END_ADDRESS - BSS_START_ADDRESS));
    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "heap", HEAP_START_ADDRESS, HEAP_END_ADDRESS, (HEAP_END_ADDRESS - HEAP_START_ADDRESS));
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
    BK_LOGD(RTOS_TAG, "%-8s 0x%-6x 0x%-6x %-8d\r\n", "psram", PSRAM_HEAP_ADDR, (PSRAM_HEAP_ADDR + PSRAM_HEAP_SIZE), PSRAM_HEAP_SIZE);
#endif
}
// eof

