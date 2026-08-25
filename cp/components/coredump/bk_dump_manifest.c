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
 * CP-side unified dump manifest.
 *
 *  - DUMP_DOMAIN_CP : the CP's own dump set (CP self-dump path).
 *  - DUMP_DOMAIN_AP : the AP windows the CP pulls on an AP trap dump.
 *
 * The CP self-dump and CP-dumps-AP paths keep their mature power-gated dump
 * routines; this manifest is composed from the exact same region getters they
 * iterate, so it is the single documented source of truth used for offline
 * coverage verification (FI-9) and carries BK_DUMP_FORMAT_VERSION.
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

static void manifest_add_getter(uint32_t *n, void (*getter)(bk_dump_mem_info_t *),
                                region_class_t cls, uint8_t tier, uint8_t cross_read_safe)
{
    bk_dump_mem_info_t info = {0};
    getter(&info);
    manifest_add_info(n, &info, cls, tier, cross_read_safe);
}

static uint32_t manifest_build_cp(void)
{
    uint32_t n = 0;
    bk_mem_addr_t addr = {0};

    uint32_t peri_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri = bk_get_peri_reg_info_list();
    for (uint32_t i = 0; i < peri_count; i++) {
        manifest_add_info(&n, &peri[i], REGION_PERI, 1U, 1U);
    }

    bk_get_dtcm_info(&addr);
    manifest_add(&n, "DTCM", addr.start_addr, addr.size, REGION_RAM, 2U, 1U, 0U);

    uint32_t sram_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram = bk_get_sram_info_list();
    for (uint32_t i = 0; i < sram_count; i++) {
        manifest_add_info(&n, &sram[i], REGION_RAM, 2U, 1U);
    }

    manifest_add_getter(&n, bk_get_psram_heap_info, REGION_PSRAM, 3U, 1U);
    manifest_add_getter(&n, bk_get_psram_bss_info, REGION_PSRAM, 3U, 1U);
    manifest_add_getter(&n, bk_get_psram_data_info, REGION_PSRAM, 3U, 1U);

    return n;
}

static uint32_t manifest_build_ap(void)
{
    uint32_t n = 0;

    /* Peripheral banks the CP reads on behalf of the AP (safe AHBP path). */
    uint32_t peri_count = bk_get_peri_reg_info_count();
    const bk_dump_mem_info_t *peri = bk_get_peri_reg_info_list();
    for (uint32_t i = 0; i < peri_count; i++) {
        manifest_add_info(&n, &peri[i], REGION_PERI, 1U, 1U);
    }

    manifest_add_getter(&n, bk_get_ap_dtcm_info, REGION_RAM, 2U, 1U);
    manifest_add_getter(&n, bk_get_ap_ram_info, REGION_RAM, 2U, 1U);

    uint32_t sram_count = bk_get_sram_info_count();
    const bk_dump_mem_info_t *sram = bk_get_sram_info_list();
    for (uint32_t i = 0; i < sram_count; i++) {
        manifest_add_info(&n, &sram[i], REGION_RAM, 2U, 1U);
    }

    manifest_add_getter(&n, bk_get_ap_psram_heap_info, REGION_PSRAM, 3U, 1U);
    manifest_add_getter(&n, bk_get_ap_psram_data_info, REGION_PSRAM, 3U, 1U);
    manifest_add_getter(&n, bk_get_ap_psram_bss_info, REGION_PSRAM, 3U, 1U);

    return n;
}

const dump_region_t *bk_dump_manifest_get(dump_domain_t domain, uint32_t *count)
{
    uint32_t n;

    if (domain == DUMP_DOMAIN_AP) {
        n = manifest_build_ap();
    } else {
        n = manifest_build_cp();
    }

    if (count != NULL) {
        *count = n;
    }
    return s_manifest;
}
