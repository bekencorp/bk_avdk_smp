// Copyright 2023-2028 Beken
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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* boot_param: manages the dedicated 8K `boot_param` partition (two 4K
 * ping-pong sectors) that stores the A/B boot record.
 *
 * This header is the SINGLE SOURCE OF TRUTH for the on-flash record layout AND
 * the ping-pong algorithm (ab_record_is_valid / read_latest / commit below are
 * static-inline so BL2, the AP side and any other consumer share one identical
 * implementation and only inject their own flash driver + zlib-CRC32 through
 * ab_flag_ops_t). The layout MUST stay byte-for-byte identical to the Python
 * packer (beken_utils/scripts/partition.py: process_boot_param). Any layout
 * change here requires updating the packer and bumping AB_FLAG_STRUCT_VER.
 *
 * The public boot_param_* API (BL2-facing, in boot_param.c) is a thin wrapper
 * that binds the BL2 flash back-end to these shared primitives and adds the A/B
 * slot-selection state machine. The record keeps its A/B semantics
 * (ab_flag_record_t, ab_slot_t, exec/update_slot, AB_FLAG_* constants). */

#define AB_FLAG_MAGIC       0x31464241u   /* 'A''B''F''1' little-endian */
#define AB_FLAG_STRUCT_VER  1u            /* layout version, forward compat */
#define AB_FLAG_SECTOR      0x1000u       /* 4K, one ping-pong copy per sector */
#define AB_FLAG_COPIES      2u            /* ping-pong sector count */
#define AB_FLAG_RECORD_SIZE 32u           /* sizeof(ab_flag_record_t) */
#define AB_FLAG_CRC_LEN     28u           /* CRC covers head bytes [0..0x1B] */
#define AB_TRY_MAX_DEFAULT  5u            /* default rollback threshold */
#define AB_PMU_TRY_MAX      7u            /* AON_PMU counter is 3-bit; try_max caps here */

typedef enum { AB_SLOT_A = 0, AB_SLOT_B = 1 } ab_slot_t;

typedef enum {
	AB_STATE_NORMAL    = 0x01,   /* running committed exec_slot, nothing pending */
	AB_STATE_TRIAL     = 0x02,   /* new image in update_slot, on trial, awaiting confirm */
	AB_STATE_CONFIRMED = 0x03,   /* app-confirmed transient (settles to NORMAL) */
} ab_boot_state_t;               /* 0x00 reserved as invalid/uninitialized */

typedef enum {
	AB_DL_IDLE    = 0,
	AB_DL_ONGOING = 1,           /* OTA writing in progress (torn-write detect) */
	AB_DL_DONE    = 2,
} ab_dl_state_t;

/* Persisted 32-byte record. Little-endian, packed. Reserved bytes are zeroed
 * (NOT 0xFF) and participate in the CRC. crc32 covers bytes [0x00..0x1B]. */
typedef struct {
	uint32_t magic;         /* 0x00  AB_FLAG_MAGIC */
	uint16_t struct_ver;    /* 0x04  layout version */
	uint16_t size;          /* 0x06  sizeof(record)=32, sanity */
	uint32_t seq;           /* 0x08  monotonic, larger = newer (ping-pong selector) */
	uint8_t  exec_slot;     /* 0x0C  committed boot slot 0=A/1=B */
	uint8_t  update_slot;   /* 0x0D  slot under trial / OTA target */
	uint8_t  boot_state;    /* 0x0E  ab_boot_state_t */
	uint8_t  dl_state;      /* 0x0F  ab_dl_state_t */
	uint8_t  try_max;       /* 0x10  rollback threshold (default 5) */
	uint8_t  rsvd0[3];      /* 0x11..0x13 reserved, must be zero */
	uint32_t rsvd1[2];      /* 0x14..0x1B reserved */
	uint32_t crc32;         /* 0x1C  CRC32 over bytes[0..0x1B] */
} ab_flag_record_t;

_Static_assert(sizeof(ab_flag_record_t) == 32, "ab_flag_record_t must be 32 bytes");

/* Back-end injected into the ping-pong algorithm below: flash driver +
 * zlib-CRC32. read/erase_sector/write take an ABSOLUTE flash offset (partition
 * base + sector offset); crc32 is the zlib-style CRC32 over [buf, buf+len). */
typedef struct {
	void     (*read)(uint32_t off, void *buf, uint32_t len);
	void     (*erase_sector)(uint32_t off);
	void     (*write)(uint32_t off, const void *buf, uint32_t len);
	uint32_t (*crc32)(const void *buf, uint32_t len);
} ab_flag_ops_t;

/* Shared back-end instance (boot_param_ops.c), compiled into both platform_bl2
 * and platform_s so BL2 and the SPE confirm path use identical flash access. */
extern const ab_flag_ops_t boot_param_ops;

/* Absolute phys base of the boot_param partition (partition table, macro
 * fallback CONFIG_BOOT_PARAM_PHY_PARTITION_OFFSET). */
uint32_t boot_param_partition_base(void);

/* AON_PMU trial-boot counter (boot_param_ops.c): runtime count in a 3-bit
 * AON_PMU field, not flash. Survives warm reset, cleared by cold power-on. Only
 * TRIAL boots touch it (decide_slot inc/clear, SPE confirm clear); NORMAL does
 * not. */
uint8_t boot_param_pmu_try_get(void);
void    boot_param_pmu_try_inc(void);
void    boot_param_pmu_try_clear(void);

/* Monotonic comparison tolerant of 32-bit wraparound: true if a is newer. */
static inline int ab_seq_newer(uint32_t a, uint32_t b)
{
	return (int32_t)(a - b) > 0;
}

/* Validate one record: magic/size/struct_ver plus zlib-CRC32 over [0..CRC_LEN).
 * Rejects struct_ver 0 or newer-than-known rather than misparsing. */
static inline int ab_record_is_valid(const ab_flag_record_t *rec,
				     const ab_flag_ops_t *ops)
{
	if (rec->magic != AB_FLAG_MAGIC) {
		return 0;
	}
	if (rec->size != AB_FLAG_RECORD_SIZE) {
		return 0;
	}
	if (rec->struct_ver == 0u || rec->struct_ver > AB_FLAG_STRUCT_VER) {
		return 0;
	}
	if (ops->crc32(rec, AB_FLAG_CRC_LEN) != rec->crc32) {
		return 0;
	}
	return 1;
}

/* Scan all ping-pong copies, return the freshest valid one in *latest and its
 * sector index [0..AB_FLAG_COPIES), or -1 if none valid (virgin/corrupt). */
static inline int ab_record_read_latest(uint32_t part_base,
					const ab_flag_ops_t *ops,
					ab_flag_record_t *latest)
{
	ab_flag_record_t candidate;
	int latest_idx = -1;
	uint32_t sector_idx;

	for (sector_idx = 0; sector_idx < AB_FLAG_COPIES; sector_idx++) {
		ops->read(part_base + sector_idx * AB_FLAG_SECTOR, &candidate, AB_FLAG_RECORD_SIZE);
		if (!ab_record_is_valid(&candidate, ops)) {
			continue;
		}
		if (latest_idx < 0 || ab_seq_newer(candidate.seq, latest->seq)) {
			*latest = candidate;
			latest_idx = (int)sector_idx;
		}
	}
	return latest_idx;
}

/* Power-loss-safe commit: caller fills the semantic fields of *new_record
 * (ideally memset(0) then field writes so reserved bytes stay 0). This stamps
 * magic/struct_ver/size, assigns the next seq, computes CRC, then erases and
 * writes the OPPOSITE sector. Returns the sector index written (0/1). */
static inline int ab_record_commit(uint32_t part_base, const ab_flag_ops_t *ops,
				   ab_flag_record_t *new_record)
{
	ab_flag_record_t latest;
	int latest_idx = ab_record_read_latest(part_base, ops, &latest);
	int write_idx = (latest_idx < 0) ? 0 : (latest_idx ^ 1);

	new_record->magic = AB_FLAG_MAGIC;
	new_record->struct_ver = (uint16_t)AB_FLAG_STRUCT_VER;
	new_record->size = (uint16_t)AB_FLAG_RECORD_SIZE;
	new_record->seq = (latest_idx < 0) ? 1u : (latest.seq + 1u);
	new_record->crc32 = ops->crc32(new_record, AB_FLAG_CRC_LEN);

	ops->erase_sector(part_base + (uint32_t)write_idx * AB_FLAG_SECTOR);
	ops->write(part_base + (uint32_t)write_idx * AB_FLAG_SECTOR,
		   new_record, AB_FLAG_RECORD_SIZE);
	return write_idx;
}

/* Load the authoritative record into the module's cache: read both ping-pong
 * SECTORS, validate each (magic/ver/size/crc32) and keep the valid copy with
 * the largest seq. Call once early in boot. Returns 0 if a valid record was
 * found, -1 if virgin (both copies invalid). */
int boot_param_load(void);

/* Decide which A/B image SLOT to boot this time from the loaded record, per the
 * state machine: NORMAL -> exec_slot, TRIAL(not exhausted) -> update_slot,
 * TRIAL(exhausted)/virgin -> exec_slot (fallback AB_SLOT_A). Caches the result;
 * returns AB_SLOT_A / AB_SLOT_B. On TRIAL it bumps the AON_PMU counter
 * (register-only, no flash) and boots update_slot; at try_max it rolls back to
 * exec_slot, settles to NORMAL (one commit) and clears the counter. NORMAL
 * writes neither flash nor PMU. Call once per boot. */
uint8_t boot_param_decide_slot(void);

/* Return the cached A/B slot from the last boot_param_decide_slot() without
 * recomputing. Intended for the stage-2 slot hook, which MCUboot may call
 * several times per boot. */
uint8_t boot_param_preferred_slot(void);

/* Copy the loaded authoritative record out (e.g. for logging). Returns 0 if a
 * valid record is cached, -1 if virgin (out left untouched). */
int boot_param_get_latest_record(ab_flag_record_t *out);

/* Ping-pong commit: caller fills the semantic fields of `rec`; this stamps
 * magic/ver/size, bumps seq, computes CRC and erases+writes the OPPOSITE
 * sector (never the one currently valid), so a torn write cannot destroy the
 * live copy. CRC at 0x1C doubles as the commit marker. Returns 0 on success.
 * Not called on the stage-1 boot path yet. */
int boot_param_commit(const ab_flag_record_t *rec);

/* Absolute flash offset of ping-pong SECTOR idx (0/1) in the boot_param
 * partition (partition base + idx * AB_FLAG_SECTOR). */
uint32_t boot_param_get_sector_addr(int idx);

/* zlib/PKZIP-compatible CRC32 (poly 0xEDB88320, init 0xFFFFFFFF, final xor).
 * MUST match Python zlib.crc32 used by the packer. */
uint32_t boot_param_crc32(const uint8_t *data, uint32_t len);

/* MCUboot slot-selection hook (bootutil/boot_hooks.h): supplies the A/B
 * preferred slot to find_slot_with_highest_version(). Implemented here in
 * boot_param.c (always built) rather than the optional platform hooks_bl2.c,
 * so slot steering is present regardless of the hooks build config. */
int boot_get_active_slot_hook(int img_index, uint32_t *slot);

/* Reconcile the record with the slot MCUboot ACTUALLY booted. Call once right
 * after boot_go() succeeds, passing rsp.br_image_off. If MCUboot fell back to a
 * slot other than the preferred one (preferred failed validation at runtime),
 * this persists the booted good slot as exec_slot and settles the record to
 * NORMAL, so subsequent resets boot the good slot directly instead of retrying
 * the bad one. No-op / no flash write when the booted slot matches the
 * preference (a TRIAL image that merely booted is left for the app to confirm). */
void boot_param_reconcile_booted(uint32_t image_off);

#ifdef __cplusplus
}
#endif
