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

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Unified per-domain dump manifest (Topic A).
 *
 * Every dump path (CP self-dump, CP-dumps-AP, AP-observes-CP-hang, AP self
 * fallback) sources the set of memory/peripheral windows it touches from this
 * single manifest instead of hand-maintained magic sizes, so coverage cannot
 * drift between paths. The manifest is composed at call time from the existing
 * region getters (SRAM/peripheral/PSRAM tables in soc/<chip>/memory.c), so it never
 * duplicates address data - it only annotates it with dump attributes.
 *
 * The offline parser negotiates field naming via BK_DUMP_FORMAT_VERSION; bump
 * it whenever a region name or the manifest layout changes.
 */
#define BK_DUMP_FORMAT_VERSION 2U

typedef enum {
    DUMP_DOMAIN_CP = 0,
    DUMP_DOMAIN_AP = 1,
} dump_domain_t;

typedef enum {
    REGION_RAM = 0,   /* on-chip SRAM / TCM / system RAM */
    REGION_PSRAM,     /* PSRAM / external media memory */
    REGION_PERI,      /* peripheral register banks */
} region_class_t;

typedef struct {
    const char    *name;
    uint32_t       start_addr;
    uint32_t       size;
    region_class_t cls;
    uint8_t        tier;             /* T0..T4 output priority (0 = highest) */
    uint8_t        cross_read_safe;  /* peer may safely cross-read when owner is wedged */
    uint8_t        engineering_only; /* only dumped on CONFIG_DEBUG_VERSION builds */
    uint8_t        sensitive;        /* reserved: Release redaction (not implemented) */
} dump_region_t;

/*
 * Return the manifest for a domain and its entry count. The returned pointer is
 * owned by the manifest module (a static scratch buffer filled in on each call)
 * and is valid until the next bk_dump_manifest_get() call. Intended for the
 * single-threaded exception/dump context only.
 *
 * Entries with start_addr==0 or size==0 (runtime-unresolved windows, e.g. an
 * unused PSRAM heap) are omitted, so callers can iterate [0, *count) directly.
 */
const dump_region_t *bk_dump_manifest_get(dump_domain_t domain, uint32_t *count);

#ifdef __cplusplus
}
#endif
