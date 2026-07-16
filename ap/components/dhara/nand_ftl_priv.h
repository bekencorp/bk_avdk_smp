// Copyright 2024-2025 Beken
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <os/os.h>
#include <driver/qspi_types.h>
#include <driver/qspi_flash.h>
#include "dhara/map.h"
#include "dhara/nand.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ZB35Q01CYIG geometry the FTL is built on (matches QSPI_NAND_* in
 * driver/qspi_flash.h). Kept local so the FTL does not pull the driver's
 * private qspi_nand_flash.h. */
#define NAND_FTL_PAGE_SIZE        QSPI_NAND_PAGE_SIZE          /* 2048 */
#define NAND_FTL_PPB              64U                          /* pages/block */
#define NAND_FTL_BLOCK_SIZE       QSPI_NAND_BLOCK_SIZE         /* 128 KB */
#define NAND_FTL_LOG2_PAGE_SIZE   11U                          /* log2(2048) */
#define NAND_FTL_LOG2_PPB         6U                           /* log2(64) */

/* Logical sector exposed to FatFS / USB MSC. 512B for host compatibility. */
#define NAND_FTL_SECTOR_SIZE      512U
#define NAND_FTL_SECTORS_PER_PAGE (NAND_FTL_PAGE_SIZE / NAND_FTL_SECTOR_SIZE) /* 4 */

/* Per-QSPI-id FTL instance. dhara_nand is embedded so the global
 * dhara_nand_* callbacks can recover the context via container_of(). */
struct nand_ftl_ctx {
	struct dhara_nand nand;    /* dhara geometry + identity for container_of */
	struct dhara_map  map;     /* dhara journalled sector map */
	qspi_id_t         id;      /* backing QSPI controller id */
	uint32_t          start_block; /* physical block where the FTL partition starts */
	uint8_t          *page_buf;    /* dhara_map scratch page (NAND_FTL_PAGE_SIZE) */
	uint8_t          *rmw_buf;     /* read-modify-write page for 512B sectors */
	uint8_t          *copy_buf;    /* dhara_nand_copy scratch page */
	uint8_t          *bad_bitmap;  /* runtime bad-block cache, 1 bit/block */
	uint32_t          bad_bitmap_bytes;
	beken_mutex_t     lock;
	bool              inited;
};

#ifndef container_of
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

static inline struct nand_ftl_ctx *nand_ftl_ctx_of(const struct dhara_nand *n)
{
	return container_of((void *)n, struct nand_ftl_ctx, nand);
}

static inline uint32_t nand_ftl_phys_page(const struct nand_ftl_ctx *c, dhara_page_t p)
{
	return p + c->start_block * NAND_FTL_PPB;
}

static inline uint32_t nand_ftl_phys_block(const struct nand_ftl_ctx *c, dhara_block_t b)
{
	return b + c->start_block;
}

static inline bool nand_ftl_bad_get(const struct nand_ftl_ctx *c, uint32_t b)
{
	return (c->bad_bitmap[b >> 3] & (1U << (b & 0x7U))) != 0;
}

static inline void nand_ftl_bad_set(struct nand_ftl_ctx *c, uint32_t b)
{
	c->bad_bitmap[b >> 3] |= (1U << (b & 0x7U));
}

/* Defined in nand_ftl.c, used by the dhara glue for context lookup by id. */
struct nand_ftl_ctx *nand_ftl_get(qspi_id_t id);

#ifdef __cplusplus
}
#endif
