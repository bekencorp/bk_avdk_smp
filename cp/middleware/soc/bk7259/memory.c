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

#include <stddef.h>
#include "os/os.h"
#include "os/mem.h"
#include "bk_arch.h"
#include "common/bk_assert.h"
#include "sdkconfig.h"
#include "stack_base.h"
#include "sys_sw_regs.h"
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
#define PSRAM_HEAP_ADDR         CONFIG_CP_PSRAM_HEAP_ADDR
#define PSRAM_HEAP_SIZE         CONFIG_CP_PSRAM_HEAP_SIZE
#endif


#if (CONFIG_CP_PSRAM_SECTION_ADDR)
extern unsigned char __psram_data_start__;
#define PSRAM_DATA_START_ADDRESS ((uint32_t)&__psram_data_start__)

extern unsigned char __psram_data_end__;
#define PSRAM_DATA_END_ADDRESS ((uint32_t)&__psram_data_end__)

extern unsigned char __psram_bss_start__;
#define PSRAM_BSS_START_ADDRESS ((uint32_t)&__psram_bss_start__)

extern unsigned char __psram_bss_end__;
#define PSRAM_BSS_END_ADDRESS ((uint32_t)&__psram_bss_end__)
#endif //#if (CONFIG_CP_PSRAM_SECTION_ADDR)

#if CONFIG_SOC_SMP
extern unsigned char _estack_core0;
extern unsigned char _estack_core1;
#else
extern unsigned char _estack;
#endif

extern unsigned char _stext;
extern unsigned char __etext;

#define FLASH_CODE_REGION_START (uint32_t)&_stext
#define FLASH_CODE_REGION_END (uint32_t)&__etext

#if CONFIG_SOC_SMP
#define CORE0_MSP_BOTTOM (uint32_t)&_estack_core0
#define CORE1_MSP_BOTTOM (uint32_t)&_estack_core1
#else
#define CORE0_MSP_BOTTOM (uint32_t)&_estack
#endif

static void bk_get_ap_heap_info_common(bk_sys_sw_regs_ap_heap_id_t id, const char *name, bk_dump_mem_info_t *info)
{
    ap_heap_dump_info_t heap_info = {0};

    info->name = name;

    if (!bk_sys_sw_regs_get_ap_heap_dump(id, &heap_info) ||
        (heap_info.max_alloc_end <= heap_info.pool_base)) {
        info->start_addr = 0U;
        info->size = 0U;
        return;
    }

    info->start_addr = heap_info.pool_base;
    info->size = heap_info.max_alloc_end - heap_info.pool_base;
}

const bk_dump_mem_info_t bk7259_sram_info[] = {
    {"SRAM0", SOC_SRAM0_DATA_BASE, SOC_SRAM0_DATA_SIZE},
    {"SRAM1", SOC_SRAM1_DATA_BASE, SOC_SRAM1_DATA_SIZE},
    {"SRAM2", SOC_SRAM2_DATA_BASE, SOC_SRAM2_DATA_SIZE},
    {"SRAM3", SOC_SRAM3_DATA_BASE, SOC_SRAM3_DATA_SIZE},
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
    {"PSRAM", (uint32_t)SOC_PSRAM_REG_BASE, (0x17*4)},
#endif
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

void bk_get_ap_psram_heap_info(bk_dump_mem_info_t *info)
{
    bk_get_ap_heap_info_common(BK_SYS_SW_REGS_AP_HEAP_PSRAM, "AP_PSRAM_HEAP", info);
}

void bk_get_psram_bss_info(bk_dump_mem_info_t *info)
{
	info->name = "PSRAM_BSS";
#if CONFIG_CP_PSRAM_SECTION_ADDR
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
#if CONFIG_CP_PSRAM_SECTION_ADDR
	info->start_addr = PSRAM_BSS_START_ADDRESS;
	info->size = PSRAM_BSS_END_ADDRESS - PSRAM_BSS_START_ADDRESS;
#else
	info->start_addr = 0;
	info->size = 0;
#endif
}

static inline bool addr_is_in_flash_txt(uint32_t addr)
{
    return ((addr >= FLASH_CODE_REGION_START) && (addr < FLASH_CODE_REGION_END));
}

static inline bool addr_is_in_itcm_txt(uint32_t addr)
{
    return false;
}

static inline bool addr_is_in_dtcm(uint32_t addr)
{
    return false;
}

static inline bool addr_is_in_sram(uint32_t addr)
{
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
#if CONFIG_CP_PSRAM_TEXT_ADDR
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
    return addr_is_in_dtcm(addr) || addr_is_in_sram(addr) || addr_is_in_psram(addr);
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
    info->start_addr = 0;
    info->size = 0;
}

void bk_get_itcm_info(bk_mem_addr_t *info)
{
    info->start_addr = 0;
    info->size = 0;
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

