// Copyright 2026 Beken
//
// dbg_probe RAM ring-buffer backend (Ver6). See dbg_sink_ram.h for the model.

#include "dbg_sink_ram.h"

#if CONFIG_DBG_PROBE && CONFIG_DBG_PROBE_RAMRING

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "dbg_probe_cfg.h"
#include "cmsis_compiler.h"

/* Per-core ring storage in .noinit: survives .bss zero in b_prep_entry_main so
 * boot-early frames (pre-prep) are not wiped. Cold boot => garbage hdr => init. */
typedef struct {
	dbg_ram_ring_hdr_t hdr;
	uint8_t            buf[DBG_PROBE_RING_MAIN_SIZE];
} dbg_ram_main_t;

typedef struct {
	dbg_ram_ring_hdr_t hdr;
	uint8_t            buf[DBG_PROBE_RING_CRIT_SIZE];
} dbg_ram_crit_t;

static dbg_ram_main_t s_ring_main[DBG_PROBE_NUM_CORES] __NO_INIT;
static dbg_ram_crit_t s_ring_crit[DBG_PROBE_NUM_CORES] __NO_INIT;

static dbg_ram_ring_hdr_t *dbg_ram_select(uint8_t core, uint8_t bucket,
	uint8_t **buf, uint32_t *size, uint32_t *buf_off)
{
	if (core >= (uint32_t)DBG_PROBE_NUM_CORES) {
		return NULL;
	}
	if (bucket == DBG_RAM_BUCKET_CRIT) {
		*buf     = s_ring_crit[core].buf;
		*size    = (uint32_t)sizeof(s_ring_crit[core].buf);
		*buf_off = (uint32_t)offsetof(dbg_ram_crit_t, buf);
		return &s_ring_crit[core].hdr;
	}
	*buf     = s_ring_main[core].buf;
	*size    = (uint32_t)sizeof(s_ring_main[core].buf);
	*buf_off = (uint32_t)offsetof(dbg_ram_main_t, buf);
	return &s_ring_main[core].hdr;
}

/* Table-less CRC-32 (poly 0xEDB88320) over the ring's immutable identity prefix
 * [magic .. buf_size] so a memory-dump scanner can trust a located header. The
 * mutable counters (wr_off/wrap_cnt/seq) are intentionally NOT covered. */
static uint32_t dbg_ram_crc32(const uint8_t *d, uint32_t n)
{
	uint32_t c = 0xFFFFFFFFu;

	for (uint32_t i = 0; i < n; i++) {
		c ^= d[i];
		for (uint32_t k = 0; k < 8u; k++) {
			c = (c >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(c & 1u)));
		}
	}
	return ~c;
}

static bool dbg_ram_magic_ok(const dbg_ram_ring_hdr_t *hdr)
{
	return hdr->magic[0] == DBG_RAM_MAGIC_B0 && hdr->magic[1] == DBG_RAM_MAGIC_B1
		&& hdr->magic[2] == DBG_RAM_MAGIC_B2 && hdr->magic[3] == DBG_RAM_MAGIC_B3
		&& hdr->magic[4] == DBG_RAM_MAGIC_B4 && hdr->magic[5] == DBG_RAM_MAGIC_B5
		&& hdr->magic[6] == DBG_RAM_MAGIC_B6 && hdr->magic[7] == DBG_RAM_MAGIC_B7;
}

static bool dbg_ram_hdr_valid(const dbg_ram_ring_hdr_t *hdr)
{
	uint32_t expect;

	if (hdr == NULL || !dbg_ram_magic_ok(hdr)) {
		return false;
	}
	if (hdr->hdr_ver != (uint16_t)DBG_RAM_HDR_VER
		|| hdr->frame_fmt != (uint16_t)DBG_RAM_FRAME_FMT_V5
		|| hdr->buf_size == 0u) {
		return false;
	}
	expect = dbg_ram_crc32((const uint8_t *)hdr, offsetof(dbg_ram_ring_hdr_t, wr_off));
	return hdr->hdr_crc == expect;
}

static void dbg_ram_init_one(dbg_ram_ring_hdr_t *hdr, uint8_t core, uint8_t bucket,
	uint32_t buf_size, uint32_t buf_off)
{
	hdr->magic[0] = DBG_RAM_MAGIC_B0; hdr->magic[1] = DBG_RAM_MAGIC_B1;
	hdr->magic[2] = DBG_RAM_MAGIC_B2; hdr->magic[3] = DBG_RAM_MAGIC_B3;
	hdr->magic[4] = DBG_RAM_MAGIC_B4; hdr->magic[5] = DBG_RAM_MAGIC_B5;
	hdr->magic[6] = DBG_RAM_MAGIC_B6; hdr->magic[7] = DBG_RAM_MAGIC_B7;
	hdr->hdr_ver   = (uint16_t)DBG_RAM_HDR_VER;
	hdr->frame_fmt = DBG_RAM_FRAME_FMT_V5;
	hdr->buf_off   = buf_off;
	hdr->buf_size  = buf_size;
	hdr->wr_off    = 0u;
	hdr->wrap_cnt  = 0u;
	hdr->seq       = 0u;
	hdr->core_id   = core;
	hdr->bucket_id = bucket;
	hdr->rsv       = 0u;
	/* CRC over the identity prefix: magic(8)+hdr_ver(2)+frame_fmt(2)+buf_off(4)+buf_size(4). */
	hdr->hdr_crc   = dbg_ram_crc32((const uint8_t *)hdr,
		offsetof(dbg_ram_ring_hdr_t, wr_off));
}

static void dbg_ram_init_bucket(uint8_t core, uint8_t bucket, bool force)
{
	uint8_t *buf;
	uint32_t size, off;
	dbg_ram_ring_hdr_t *h = dbg_ram_select(core, bucket, &buf, &size, &off);

	if (h == NULL) {
		return;
	}
	if (!force && dbg_ram_hdr_valid(h)) {
		return;
	}
	dbg_ram_init_one(h, core, bucket, size, off);
}

void dbg_sink_ram_init_core(uint8_t core, bool force)
{
	dbg_ram_init_bucket(core, DBG_RAM_BUCKET_MAIN, force);
	dbg_ram_init_bucket(core, DBG_RAM_BUCKET_CRIT, force);
}

void dbg_sink_ram_write(uint8_t core, uint8_t bucket, const uint8_t *frame, uint32_t len)
{
	uint8_t *buf;
	uint32_t size, off, wr;
	dbg_ram_ring_hdr_t *h = dbg_ram_select(core, bucket, &buf, &size, &off);

	if (h == NULL || frame == NULL || size == 0u || len == 0u) {
		return;
	}
	if (h->buf_size == 0u) {
		return;   /* ring not initialized yet */
	}
	if (len > size) {
		len = size;   /* never overrun (a single frame is <= 8 B << ring size) */
	}
	wr = h->wr_off;
	for (uint32_t i = 0; i < len; i++) {
		buf[wr] = frame[i];
		if (++wr >= size) {
			wr = 0u;
			h->wrap_cnt++;
		}
	}
	h->wr_off = wr;
	h->seq++;
}

const dbg_ram_ring_hdr_t *dbg_sink_ram_get(uint8_t core, uint8_t bucket,
	const uint8_t **buf, uint32_t *size)
{
	uint8_t *b;
	uint32_t sz, off;
	dbg_ram_ring_hdr_t *h = dbg_ram_select(core, bucket, &b, &sz, &off);

	if (h == NULL || h->buf_size == 0u) {
		return NULL;   /* uninitialized -> nothing to dump */
	}
	if (buf != NULL) {
		*buf = b;
	}
	if (size != NULL) {
		*size = sz;
	}
	return h;
}

#endif /* CONFIG_DBG_PROBE && CONFIG_DBG_PROBE_RAMRING */
