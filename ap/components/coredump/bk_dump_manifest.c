// Copyright 2020-2026 Beken
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <soc/soc.h>
#include "reg_base.h"
#include "bk_dump_manifest.h"
#include "bk_coredump.h"
#include "memory.h"

/*
 * AP-side unified dump manifest.
 *
 *  - DUMP_DOMAIN_AP : the AP's own full local dump set (used by the P0-1 AP
 *    self-dump fallback path, and equal to what the CP pulls for a trap dump).
 *  - DUMP_DOMAIN_CP : the CP context the AP can observe/cross-read when the CP
 *    hangs (used by the AP-observes-CP-hang path). Peripheral windows go
 *    through the CP-local AHBP path and are marked cross_read_safe; the CP
 *    RAM/PSRAM windows are only cross-read-safe once verified on the target
 *    board (gated by CONFIG_CP_HANG_DUMP_BY_AP_MEMDUMP), matching the previous
 *    hand-coded behaviour.
 */

#define BK_DUMP_MANIFEST_MAX 64U

extern void bk_get_dtcm_info(bk_mem_addr_t *info);

static dump_region_t s_manifest[BK_DUMP_MANIFEST_MAX];

static void manifest_add(uint32_t *n, const char *name, uint32_t start, uint32_t size,
                         region_class_t cls, uint8_t tier, uint8_t cross_read_safe,
                         uint8_t engineering_only)
{
    if (*n >= BK_DUMP_MANIFEST_MAX) {
        return;
    }
    if ((name == NULL) || (start == 0U) || (size == 0U)) {
        return;   /* skip runtime-unresolved / empty windows */
    }

    dump_region_t *r = &s_manifest[*n];
    r->name = name;
    r->start_addr = start;
    r->size = size;
    r->cls = cls;
    r->tier = tier;
    r->cross_read_safe = cross_read_safe;
    r->engineering_only = engineering_only;
    r->sensitive = 0U;
    (*n)++;
}

static void manifest_add_info(uint32_t *n, const bk_dump_mem_info_t *info,
                              region_class_t cls, uint8_t tier, uint8_t cross_read_safe)
{
    manifest_add(n, info->name, info->start_addr, info->size, cls, tier, cross_read_safe, 0U);
}

static uint32_t manifest_build_ap(void)
{
    uint32_t n = 0;
    bk_dump_mem_info_t info = {0};
    bk_mem_addr_t addr = {0};

    /* Peripheral register banks (tier 1: high value, safe reads). */
    uint32_t peri_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri = bk_get_peri_reg_info_list();
    for (uint32_t i = 0; i < peri_count; i++) {
        manifest_add_info(&n, &peri[i], REGION_PERI, 1U, 1U);
    }

    /* DTCM (tier 2). */
    bk_get_dtcm_info(&addr);
    manifest_add(&n, "DTCM", addr.start_addr, addr.size, REGION_RAM, 2U, 1U, 0U);

    /* On-chip SRAM banks (tier 2). */
    uint32_t sram_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram = bk_get_sram_info_list();
    for (uint32_t i = 0; i < sram_count; i++) {
        manifest_add_info(&n, &sram[i], REGION_RAM, 2U, 1U);
    }

    /* Registered extra system memory windows (tier 3). */
    uint32_t extra_count = bk_get_dump_sys_mem_count();
    bk_mem_addr_t *extra = bk_get_dump_sys_mem_info();
    for (uint32_t i = 0; i < extra_count; i++) {
        manifest_add(&n, "EXTRA_MEM", extra[i].start_addr, extra[i].size,
                     REGION_RAM, 3U, 1U, 0U);
    }

    /* PSRAM heap / sections (tier 3). */
    bk_get_psram_heap_info(&info);
    manifest_add_info(&n, &info, REGION_PSRAM, 3U, 1U);
    bk_get_psram_bss_info(&info);
    manifest_add_info(&n, &info, REGION_PSRAM, 3U, 1U);
    bk_get_psram_data_info(&info);
    manifest_add_info(&n, &info, REGION_PSRAM, 3U, 1U);

    return n;
}

static uint32_t manifest_build_cp(void)
{
    uint32_t n = 0;

    /* CP peripheral windows readable from the AP over the CP-local AHBP path.
     * These replace the hand-coded magic-size windows previously duplicated in
     * bk_coredump_cp_hang.c. */
#if defined(SOC_SYS_REG_BASE)
    manifest_add(&n, "CP_HANG_SYS", (uint32_t)SOC_SYS_REG_BASE, 0x5cU * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_SYS_AHBP_REG_BASE)
    manifest_add(&n, "CP_HANG_SYS_AHBP", (uint32_t)SOC_SYS_AHBP_REG_BASE, 0x60U * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_AON_PMU_REG_BASE)
    manifest_add(&n, "CP_HANG_AON_PMU", (uint32_t)SOC_AON_PMU_REG_BASE, 0x7fU * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_AON_RTC_REG_BASE)
    manifest_add(&n, "CP_HANG_AON_RTC", (uint32_t)SOC_AON_RTC_REG_BASE, 0x0aU * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_MBOX0_REG_BASE)
    manifest_add(&n, "CP_HANG_MBOX0", (uint32_t)SOC_MBOX0_REG_BASE, 0x38U * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_WDT_REG_BASE)
    manifest_add(&n, "CP_HANG_WDT", (uint32_t)SOC_WDT_REG_BASE, 0x20U * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_PPHS_REG_BASE)
    manifest_add(&n, "CP_HANG_PPHS", (uint32_t)SOC_PPHS_REG_BASE, 0x10U * 4U, REGION_PERI, 1U, 1U, 0U);
#endif
#if defined(SOC_PPRO_REG_BASE)
    manifest_add(&n, "CP_HANG_PPRO", (uint32_t)SOC_PPRO_REG_BASE, 0x24U * 4U, REGION_PERI, 1U, 1U, 0U);
#endif

    /* CP RAM / PSRAM windows: cross-read safety must be verified on the target
     * board first, so they are only marked cross_read_safe when the board-gated
     * CONFIG_CP_HANG_DUMP_BY_AP_MEMDUMP is enabled. */
#if defined(CONFIG_CP_HANG_DUMP_BY_AP_MEMDUMP)
    const uint8_t cp_mem_safe = 1U;
#else
    const uint8_t cp_mem_safe = 0U;
#endif
#if defined(CONFIG_CP_RAM_ADDR) && defined(CONFIG_CP_RAM_SIZE) && CONFIG_CP_RAM_SIZE
    manifest_add(&n, "CP_RAM", (uint32_t)CONFIG_CP_RAM_ADDR, (uint32_t)CONFIG_CP_RAM_SIZE,
                 REGION_RAM, 2U, cp_mem_safe, 0U);
#endif
#if defined(CONFIG_CP_PSRAM_HEAP_ADDR) && defined(CONFIG_CP_PSRAM_HEAP_SIZE) && CONFIG_CP_PSRAM_HEAP_SIZE
    manifest_add(&n, "CP_PSRAM_HEAP", (uint32_t)CONFIG_CP_PSRAM_HEAP_ADDR, (uint32_t)CONFIG_CP_PSRAM_HEAP_SIZE,
                 REGION_PSRAM, 3U, cp_mem_safe, 0U);
#endif

    return n;
}

const dump_region_t *bk_dump_manifest_get(dump_domain_t domain, uint32_t *count)
{
    uint32_t n;

    if (domain == DUMP_DOMAIN_CP) {
        n = manifest_build_cp();
    } else {
        n = manifest_build_ap();
    }

    if (count != NULL) {
        *count = n;
    }
    return s_manifest;
}
