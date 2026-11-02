// Copyright 2023-2024 Beken
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

#include "reg_base.h"

#if CONFIG_SPE

#define CP_MEM_CHECK_POINT_PER_BLOCK   4u
#define CP_MEM_CHECK_BLOCK_NUM         3u
#define CP_MEM_CHECK_POINT_NUM         (CP_MEM_CHECK_BLOCK_NUM * CP_MEM_CHECK_POINT_PER_BLOCK)
#define CP_MEM_CHECK_REG_IDX_FIRST     8u
#define CP_MEM_CHECK_BST_VALID_MASK    0x8000u
#define CP_MEM_CHECK_BST_ADDR_MASK     0x7FFFu
#define CP_MEM_CHECK_ROW_BYTES         4u

#include "ram_regions.h"

#if defined(CONFIG_CP_SPE_RAM_SIZE) && (CONFIG_CP_SPE_RAM_SIZE > 0) /* secure project */
#define CP_MEM_CHECK_S_PREFIX      CONFIG_CP_SPE_RAM_SIZE   /* SRAM0 Secure prefix length */
#define CP_MEM_CHECK_NS_ALIAS(b)   ((b) + SOC_S_NS_ADDR_DIFF)
#else                                                              /* plain project */
#define CP_MEM_CHECK_S_PREFIX      0u
#define CP_MEM_CHECK_NS_ALIAS(b)   (b)
#endif

/* Returns the address of the captured word, or 0 when the entry is invalid or
 * decodes outside its block. Whether that address is a legal target is up to
 * the caller. */
static inline __attribute__((always_inline)) uint32_t cp_mem_check_bad_point_addr_get(uint32_t idx)
{
	volatile uint32_t *reg_base = (volatile uint32_t *)SOC_MEM_CHECK_REG_BASE;
	/* The repair point sits in the upper half of the register. */
	uint16_t bst_high = (uint16_t)(reg_base[CP_MEM_CHECK_REG_IDX_FIRST + idx] >> 16);
	uint32_t mem_base;
	uint32_t mem_size;

	if ((bst_high & CP_MEM_CHECK_BST_VALID_MASK) == 0) {
		return 0;
	}

	switch (idx / CP_MEM_CHECK_POINT_PER_BLOCK) {
	case 0:
		mem_base = SOC_SRAM0_DATA_BASE;
		mem_size = SOC_SRAM0_DATA_SIZE;
		break;
	case 1:
		mem_base = SOC_SRAM1_DATA_BASE;
		mem_size = SOC_SRAM1_DATA_SIZE;
		break;
	default:
		mem_base = SOC_SRAM2_DATA_BASE;
		mem_size = SOC_SRAM2_DATA_SIZE;
		break;
	}

	uint32_t off = (uint32_t)(bst_high & CP_MEM_CHECK_BST_ADDR_MASK) * CP_MEM_CHECK_ROW_BYTES;

	if (off >= mem_size) {
		return 0;
	}

	uint32_t addr = mem_base + off;

	if (addr >= (SOC_SRAM0_DATA_BASE + CP_MEM_CHECK_S_PREFIX)) {
		addr = CP_MEM_CHECK_NS_ALIAS(mem_base) + off;
	}

	return addr;
}

/* ---------------------------------------------------------------------------
 * AP side of the same hardware.
 *
 * The AP SRAM bad points live in the AP subsystem's MEM_CHECK window (Secure
 * alias 0x48010000, SOC_AP_MEM_CHECK_REG_BASE), one controller-wide set of 16
 * repair points over SRAM3..SRAM6.
 * The repair record is the LOW half word here (the CP half reads the high half)
 * and each block carries an instance index whose width depends on how the
 * 128-bit beat is split across repair circuits: SMEM3..SMEM5 are 2 x 64-bit,
 * SMEM6 is 4 x 32-bit.
 *
 * Shared by the CP-driven implementations of both project shapes: the plain
 * project runs this on the Secure CP directly, the secure project runs it in
 * TF-M.
 *
 * The alias split works exactly like the CP half above, only the Secure prefix
 * is much smaller: SMEM3's first 4K is the AP boot shim, kept Secure because the
 * AP core resets in the Secure state and runs it before dropping to Non-Secure
 * (see bk_tfm_ap_shim.c, CONFIG_AP_SPE_RAM_SIZE). Everything else - the rest of
 * SMEM3 and all of SMEM4..6, i.e. the AP heap and RAM - is Non-Secure, so the
 * Secure world reaches it through the NS alias. Outside the secure project there
 * is no shim and the whole window is plain memory. */

#if defined(CONFIG_AP_SPE_RAM_SIZE) && (CONFIG_AP_SPE_RAM_SIZE > 0)  /* secure project */
#define AP_MEM_CHECK_S_PREFIX   ((uint32_t)CONFIG_AP_SPE_RAM_SIZE)
#define AP_MEM_CHECK_NS_OFFSET  SOC_S_NS_ADDR_DIFF
#else                                                               /* plain project */
#define AP_MEM_CHECK_S_PREFIX   0u
#define AP_MEM_CHECK_NS_OFFSET  0u
#endif

#define AP_MEM_CHECK_POINT_PER_BLOCK   4u
#define AP_MEM_CHECK_BLOCK_NUM         4u
#define AP_MEM_CHECK_POINT_NUM         (AP_MEM_CHECK_BLOCK_NUM * AP_MEM_CHECK_POINT_PER_BLOCK)
#define AP_MEM_CHECK_BST_REG_FIRST     0x40u
#define AP_MEM_CHECK_BST_ENTRY_MASK    0xFFFFu
#define AP_MEM_CHECK_BST_VALID_MASK    0x8000u
#define AP_MEM_CHECK_BST_ADDR_MASK     0x7FFFu
#define AP_MEM_CHECK_BST_ROW_LOW_BITS  12u
#define AP_MEM_CHECK_BST_ROW_LOW_MASK  0x0FFFu
#define AP_MEM_CHECK_BEAT_BYTES        16u
#define AP_MEM_CHECK_MAX_INST_BYTES    8u
#define AP_MEM_CHECK_POINT_MAX_WORDS   (AP_MEM_CHECK_MAX_INST_BYTES / sizeof(uint32_t))

/* Returns the address of the captured word, or 0 when the entry is invalid or
 * decodes outside its block. Whether that address is a legal target is up to
 * the caller. */
static inline __attribute__((always_inline)) uint32_t ap_mem_check_bad_point_addr_get(uint32_t idx)
{
	volatile uint32_t *reg_base = (volatile uint32_t *)SOC_AP_MEM_CHECK_REG_BASE;
	uint32_t entry = reg_base[AP_MEM_CHECK_BST_REG_FIRST + idx] & AP_MEM_CHECK_BST_ENTRY_MASK;
	uint32_t mem_base;
	uint32_t mem_size;
	uint32_t inst_idx_bits;
	uint32_t inst_bytes;
	uint32_t inst;
	uint32_t row;
	uint32_t addr;

	if ((entry & AP_MEM_CHECK_BST_VALID_MASK) == 0) {
		return 0;
	}

	switch (idx / AP_MEM_CHECK_POINT_PER_BLOCK) {
	case 0:
		mem_base = SOC_SRAM3_DATA_BASE;
		mem_size = SOC_SRAM3_DATA_SIZE;
		inst_idx_bits = 1u;
		break;
	case 1:
		mem_base = SOC_SRAM4_DATA_BASE;
		mem_size = SOC_SRAM4_DATA_SIZE;
		inst_idx_bits = 1u;
		break;
	case 2:
		mem_base = SOC_SRAM5_DATA_BASE;
		mem_size = SOC_SRAM5_DATA_SIZE;
		inst_idx_bits = 1u;
		break;
	default:
		mem_base = SOC_SRAM6_DATA_BASE;
		mem_size = SOC_SRAM6_DATA_SIZE;
		inst_idx_bits = 2u;
		break;
	}

	inst_bytes = AP_MEM_CHECK_BEAT_BYTES >> inst_idx_bits;

	entry &= AP_MEM_CHECK_BST_ADDR_MASK;
	inst = (entry >> AP_MEM_CHECK_BST_ROW_LOW_BITS) & ((1u << inst_idx_bits) - 1u);
	row = ((entry >> (AP_MEM_CHECK_BST_ROW_LOW_BITS + inst_idx_bits)) << AP_MEM_CHECK_BST_ROW_LOW_BITS) |
		(entry & AP_MEM_CHECK_BST_ROW_LOW_MASK);
	addr = mem_base + row * AP_MEM_CHECK_BEAT_BYTES + inst * inst_bytes;

	/* The Secure prefix (the 4K boot shim) keeps the block's own alias; the
	 * Non-Secure rest is only reachable through the NS alias. */
	if (addr >= (SOC_SRAM3_DATA_BASE + AP_MEM_CHECK_S_PREFIX)) {
		addr += AP_MEM_CHECK_NS_OFFSET;
	}

	/* The row field spans the block exactly, so this cannot trip while the
	 * descriptors match the hardware. Kept as a guard in case they drift. */
	if ((row * AP_MEM_CHECK_BEAT_BYTES + inst * inst_bytes + inst_bytes) > mem_size) {
		return 0;
	}

	return addr;
}

/* Words a valid AP repair point of this index spans (2 for SMEM3..SMEM5, 1 for
 * SMEM6). Callers use it to size the capture/write-back of one entry. */
static inline __attribute__((always_inline)) uint32_t ap_mem_check_bad_point_words_get(uint32_t idx)
{
	uint32_t inst_idx_bits = ((idx / AP_MEM_CHECK_POINT_PER_BLOCK) == 3u) ? 2u : 1u;

	return (AP_MEM_CHECK_BEAT_BYTES >> inst_idx_bits) / sizeof(uint32_t);
}

/* True when addr is inside the AP retained SRAM window (SRAM3..SRAM6). Used to
 * bound a write-back whose address came from retained RAM. */
static inline __attribute__((always_inline)) uint32_t ap_mem_check_addr_in_ap_ram(uint32_t addr)
{
	/* The staging table holds whichever alias was used to reach the word, so
	 * fold the NS alias back before comparing against the block range. */
	if (addr >= (SOC_SRAM3_DATA_BASE + AP_MEM_CHECK_NS_OFFSET)) {
		addr -= AP_MEM_CHECK_NS_OFFSET;
	}

	return (addr >= SOC_SRAM3_DATA_BASE) &&
		(addr < (SOC_SRAM6_DATA_BASE + SOC_SRAM6_DATA_SIZE));
}

#endif /* CONFIG_SPE */
