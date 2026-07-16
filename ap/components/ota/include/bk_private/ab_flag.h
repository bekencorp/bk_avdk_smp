/*
 * ab_flag.h - AB (position-independent) ping-pong boot-flag record.
 *
 * SINGLE SOURCE OF TRUTH for the on-flash layout of the AB update state stored
 * in the "ota_fina_executive" partition. This file is duplicated verbatim on
 * the bootloader side (arm_bootloader/.../core/ab_flag.h) and mirrored by the
 * packager (tools/build_tools/build_process/bk_sdk/bk_sdk_project.py). ALL
 * copies -- bootloader, AP and the Python packager -- MUST agree byte-for-byte,
 * or the CRC check will reject a perfectly good record.
 *
 * Design (see plan ab_flag_pingpong_redesign):
 *   - The partition is 8K = two 4K sectors used as a ping-pong pair. A commit
 *     always writes the *other* sector; the previous copy stays valid until the
 *     new one is fully written and its CRC checks out, so a power loss during
 *     the write never produces an all-0xFF (virgin) partition.
 *   - Each 4K sector holds one 32-byte record at its start. The two copies are
 *     therefore 0x1000 apart -- NEVER lay them out as a contiguous C array.
 *   - "seq" is a monotonic ping-pong freshness selector (not a firmware version
 *     and not an anti-rollback counter): read picks the larger valid seq.
 *   - "crc32" covers bytes [0 .. AB_FLAG_CRC_LEN) and, being last, doubles as a
 *     commit marker (a half-written record fails CRC and is ignored).
 *
 * CRC32 convention: standard zlib/PKZIP (poly 0xEDB88320, init 0, final
 * inversion). Matches the bootloader ota_verify_calc_crc32(), the app-side
 * crc32_zlib() and Python zlib.crc32. Do NOT use CheckSumUtils' CRC32_* (it
 * omits the final inversion -> off by ^0xFFFFFFFF).
 */

#ifndef AB_FLAG_H
#define AB_FLAG_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AB_FLAG_MAGIC        0x31464241u  /* 'A''B''F''1' little-endian */
#define AB_FLAG_STRUCT_VER   1u
#define AB_FLAG_RECORD_SIZE  32u
#define AB_FLAG_CRC_LEN      28u          /* bytes [0..0x1B] covered by crc32 */
#define AB_FLAG_SECTOR       0x1000u      /* one 4K sector per ping-pong copy */
#define AB_FLAG_COPIES       2u
#define AB_FLAG_DEFAULT_TRY_MAX 3u        /* trial boots before rollback */

typedef enum {
    AB_SLOT_A = 0,
    AB_SLOT_B = 1,
} ab_slot_t;

typedef enum {
    AB_STATE_NORMAL    = 0x01, /* running the committed exec_slot, nothing pending */
    AB_STATE_TRIAL     = 0x02, /* new image in update_slot on trial, awaiting confirm */
    AB_STATE_CONFIRMED = 0x03, /* transient; once persisted it is equivalent to NORMAL */
} ab_boot_state_t;

typedef enum {
    AB_DL_IDLE    = 0x00,
    AB_DL_ONGOING = 0x01,
    AB_DL_DONE    = 0x02,
} ab_dl_state_t;

/* 32-byte record. Naturally aligned (every uint32_t sits on a 4-byte boundary),
 * so sizeof == 32 without packing; the static assert below enforces it. */
typedef struct {
    uint32_t magic;        /* 0x00 AB_FLAG_MAGIC */
    uint16_t struct_ver;   /* 0x04 layout version, forward compatible */
    uint16_t size;         /* 0x06 sizeof(record) == 32, basic self-check */
    uint32_t seq;          /* 0x08 monotonic ping-pong selector, larger == newer */
    uint8_t  exec_slot;    /* 0x0C committed boot slot (0=A/1=B) */
    uint8_t  update_slot;  /* 0x0D OTA target / trial slot */
    uint8_t  boot_state;   /* 0x0E ab_boot_state_t */
    uint8_t  dl_state;     /* 0x0F ab_dl_state_t (diagnostic / partial-download hint) */
    uint8_t  try_max;      /* 0x10 rollback threshold (default 3) */
    uint8_t  rsvd0[3];     /* 0x11..0x13 */
    uint32_t rsvd1[2];     /* 0x14..0x1B reserved */
    uint32_t crc32;        /* 0x1C CRC32 over bytes [0..0x1B] */
} ab_flag_record_t;

typedef char ab_flag_record_size_assert[(sizeof(ab_flag_record_t) == AB_FLAG_RECORD_SIZE) ? 1 : -1];

/* Per-target hardware/back-end operations. Each side (bootloader / AP / CP)
 * fills these with its own flash driver + zlib-CRC32 so the ping-pong algorithm
 * below stays identical everywhere.
 *   read/erase_sector/write take an absolute flash offset (partition base +
 *   sector offset). crc32 must be the zlib-style CRC32 over [buf, buf+len). */
typedef struct {
    void     (*read)(uint32_t off, void *buf, uint32_t len);
    void     (*erase_sector)(uint32_t off);
    void     (*write)(uint32_t off, const void *buf, uint32_t len);
    uint32_t (*crc32)(const void *buf, uint32_t len);
} ab_flag_ops_t;

/* Monotonic comparison that tolerates 32-bit wraparound: true if seq_a is newer
 * than seq_b. */
static inline int ab_seq_newer(uint32_t seq_a, uint32_t seq_b)
{
    return (int32_t)(seq_a - seq_b) > 0;
}

static inline int ab_record_is_valid(const ab_flag_record_t *rec, const ab_flag_ops_t *ops)
{
    if (rec->magic != AB_FLAG_MAGIC) {
        return 0;
    }
    if (rec->size != AB_FLAG_RECORD_SIZE) {
        return 0;
    }
    /* Unknown / future layout: refuse rather than misparse. */
    if (rec->struct_ver == 0u || rec->struct_ver > AB_FLAG_STRUCT_VER) {
        return 0;
    }
    if (ops->crc32(rec, AB_FLAG_CRC_LEN) != rec->crc32) {
        return 0;
    }
    return 1;
}

/* Scan all ping-pong copies and return the freshest valid one.
 * @param latest  out: receives the selected record (freshest valid copy).
 * @return the sector index [0 .. AB_FLAG_COPIES) of the selected record, or -1
 *         if no copy is valid (virgin / doubly-corrupted). On equal seq the
 *         lower-indexed copy wins (ab_seq_newer is a strict comparison). */
static inline int ab_record_read_latest(uint32_t part_base, const ab_flag_ops_t *ops,
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

/* Commit a new record to the ping-pong pair (power-loss safe).
 *
 * The caller fills the semantic fields of *new_record (exec_slot, update_slot,
 * boot_state, dl_state, try_max) -- ideally via memset(0) then field writes so
 * the reserved bytes are 0, matching the packager. This routine stamps
 * magic/struct_ver/size, assigns the next seq, computes the CRC, then erases
 * and writes the opposite sector.
 *
 * @return the sector index written (0 or 1). */
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
    ops->write(part_base + (uint32_t)write_idx * AB_FLAG_SECTOR, new_record, AB_FLAG_RECORD_SIZE);
    return write_idx;
}

#ifdef __cplusplus
}
#endif

#endif /* AB_FLAG_H */
