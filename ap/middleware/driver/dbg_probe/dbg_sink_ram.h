// Copyright 2026 Beken
//
// dbg_probe RAM ring-buffer backend (Ver6). Always-on tee: every runtime frame
// is also copied into a per-core circular RAM buffer (overwrite policy) so that,
// with no UART attached, a memory dump (or the `dbgp dump` CLI) can recover the
// most recent probe timeline. Per-core rings keep the hot path spinlock-free
// (the writer only disables IRQs on its own core, like the exclusive UART path).
//
// Each (core, bucket) owns one ring = a locatable header immediately followed by
// its data buffer. A raw memory dump can be scanned for the 8-byte magic to find
// every ring; the self-describing Ver5 frames let the parser resync after the
// wrap point. Bucket routing isolates rare critical frames (exceptions) from the
// high-volume default stream so a flood cannot evict them.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "dbg_probe_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bucket ids (routing target). */
#define DBG_RAM_BUCKET_MAIN   0u   /* default: all non-critical frames */
#define DBG_RAM_BUCKET_CRIT   1u   /* exceptions / faults (never flooded out) */
#define DBG_RAM_NUM_BUCKETS   2u

#if CONFIG_DBG_PROBE && CONFIG_DBG_PROBE_RAMRING

#include "dbg_probe.h"   /* DBG_PROBE_KIND_* */

#define DBG_RAM_HDR_VER       6u
#define DBG_RAM_FRAME_FMT_V5  5u            /* on-wire frame format = Ver5 4B/8B */
/* 8-byte magic as bytes "DBGPRING" — what a memory-dump scanner greps for. */
#define DBG_RAM_MAGIC_B0 'D'
#define DBG_RAM_MAGIC_B1 'B'
#define DBG_RAM_MAGIC_B2 'G'
#define DBG_RAM_MAGIC_B3 'P'
#define DBG_RAM_MAGIC_B4 'R'
#define DBG_RAM_MAGIC_B5 'I'
#define DBG_RAM_MAGIC_B6 'N'
#define DBG_RAM_MAGIC_B7 'G'

/* Locatable ring header (fixed layout; data buffer follows at +buf_off). */
typedef struct {
	uint8_t  magic[8];     /* "DBGPRING" */
	uint16_t hdr_ver;      /* = DBG_RAM_HDR_VER */
	uint16_t frame_fmt;    /* on-wire frame format (= DBG_RAM_FRAME_FMT_V5) */
	uint32_t buf_off;      /* byte offset from &hdr to the data buffer */
	uint32_t buf_size;     /* data buffer size in bytes */
	uint32_t wr_off;       /* next write position in the data buffer (wraps) */
	uint32_t wrap_cnt;     /* number of times wr_off wrapped (overwrite count) */
	uint32_t seq;          /* total frames written (monotonic) */
	uint8_t  core_id;      /* which core owns this ring */
	uint8_t  bucket_id;    /* DBG_RAM_BUCKET_* */
	uint16_t rsv;
	uint32_t hdr_crc;      /* crc32 over this header with hdr_crc treated as 0 */
} dbg_ram_ring_hdr_t;

/** Initialize all rings owned by `core`.
 *  force=false: skip buckets whose header is already valid (preserves early boot
 *               frames written before runtime_init).
 *  force=true:  always reset (used once at dbg_probe_init for a clean boot stream). */
void dbg_sink_ram_init_core(uint8_t core, bool force);

/** Append a frame to (core, bucket)'s ring. Caller MUST hold the per-core
 *  critical section (IRQs disabled); per-core rings need no cross-core lock. */
void dbg_sink_ram_write(uint8_t core, uint8_t bucket, const uint8_t *frame, uint32_t len);

/** Route a frame kind to a bucket (EXC -> critical, else main). */
static inline uint8_t dbg_sink_ram_route(uint8_t kind)
{
	return (kind == DBG_PROBE_KIND_EXC) ? DBG_RAM_BUCKET_CRIT : DBG_RAM_BUCKET_MAIN;
}

/** Accessor for the dump path (CLI / offline). Returns the ring header for
 *  (core, bucket) and, via out-params, its data buffer and size. Returns NULL
 *  if the indices are out of range or the ring was never initialized.
 *
 *  DESIGN CONSTRAINT — lock-free dump: the writer runs on the ring's OWNER
 *  core with only that core's IRQs disabled; there is NO cross-core lock, by
 *  design (keeps the probe hot path zero-overhead). A reader on another core
 *  therefore observes a LIVE ring. Readers MUST:
 *   1) snapshot (wr_off, wrap_cnt, seq) with a stable-read loop (re-read until
 *      two passes agree) — never trust a single pass while probes may run;
 *   2) re-read seq after copying the data and compare with the snapshot: equal
 *      means the dump is clean; a delta means frames churned mid-dump and the
 *      bytes near the write head are untrusted (frames self-describe, so an
 *      offline parser can resync past any garbled region).
 *  For a guaranteed-clean dump, quiesce the probes first. */
const dbg_ram_ring_hdr_t *dbg_sink_ram_get(uint8_t core, uint8_t bucket,
	const uint8_t **buf, uint32_t *size);

#else /* disabled: zero-overhead no-ops */

static inline void dbg_sink_ram_init_core(uint8_t core, bool force) { (void)core; (void)force; }
static inline void dbg_sink_ram_write(uint8_t core, uint8_t bucket,
	const uint8_t *frame, uint32_t len) { (void)core; (void)bucket; (void)frame; (void)len; }
static inline uint8_t dbg_sink_ram_route(uint8_t kind) { (void)kind; return DBG_RAM_BUCKET_MAIN; }

#endif /* CONFIG_DBG_PROBE && CONFIG_DBG_PROBE_RAMRING */

#ifdef __cplusplus
}
#endif
