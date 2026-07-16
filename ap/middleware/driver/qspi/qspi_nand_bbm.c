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

#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include <driver/qspi_nand_bbm.h>
#include <os/mem.h>
#include <common/bk_crc.h>
#include "qspi_driver.h"
#include "qspi_nand_flash.h"

#if CONFIG_QSPI_NAND_FLASH

#define NAND_BBM_MAGIC          0x4D42424BU  /* 'KBBM' little-endian */
#define NAND_BBM_VERSION        1U
#define NAND_BBM_META_BLOCKS    2U
#define NAND_BBM_RESERVE_PCT    2U
#define NAND_BBM_RESERVE_MIN    4U
#define NAND_BBM_INVALID_PHYS   0xFFFFU
#define NAND_BBM_CRC_INIT       0xFFU

#define NAND_BBM_BLOCK_BYTES    NAND_BLOCK_SIZE_BYTES

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t seq;
	uint32_t total_blocks;
	uint32_t data_blocks;
	uint32_t reserve_start;
	uint32_t reserve_count;
	uint32_t next_spare;
	uint32_t l2p_bytes;
	uint32_t bad_bytes;
	uint8_t  crc;          /* crc8 over l2p[] followed by bad[] */
	uint8_t  rsvd[3];
} nand_bbm_hdr_t;

typedef struct {
	bool     inited;
	uint32_t total_blocks;
	uint32_t data_blocks;
	uint32_t reserve_start;
	uint32_t reserve_count;
	uint32_t next_spare;
	uint32_t meta_block[NAND_BBM_META_BLOCKS];
	uint16_t *l2p;         /* logical block -> physical block */
	uint8_t  *bad;         /* physical bad-block bitmap, 1 bit/block */
	uint32_t seq;
	uint8_t  meta_idx;     /* meta copy to overwrite on next persist */
} nand_bbm_t;

static nand_bbm_t s_bbm[QSPI_ID_MAX];

static inline nand_bbm_t *bbm_get(qspi_id_t id)
{
	return (id < QSPI_ID_MAX) ? &s_bbm[id] : NULL;
}

static inline bool bbm_bad_get(const nand_bbm_t *b, uint32_t phys)
{
	return (b->bad[phys >> 3] & (1U << (phys & 0x7))) != 0;
}

static inline void bbm_bad_set(nand_bbm_t *b, uint32_t phys)
{
	b->bad[phys >> 3] |= (1U << (phys & 0x7));
}

static inline uint32_t bbm_l2p_bytes(const nand_bbm_t *b)
{
	return b->data_blocks * (uint32_t)sizeof(uint16_t);
}

static inline uint32_t bbm_bad_bytes(const nand_bbm_t *b)
{
	return (b->total_blocks + 7U) / 8U;
}

static void bbm_compute_layout(nand_bbm_t *b)
{
	uint32_t total = NAND_DEVICE_TOTAL_SIZE / NAND_BBM_BLOCK_BYTES;
	uint32_t reserve = (total * NAND_BBM_RESERVE_PCT) / 100U;

	if (reserve < NAND_BBM_RESERVE_MIN) {
		reserve = NAND_BBM_RESERVE_MIN;
	}

	b->total_blocks = total;
	b->meta_block[0] = total - 1U;
	b->meta_block[1] = total - 2U;
	b->reserve_count = reserve;
	b->data_blocks = total - NAND_BBM_META_BLOCKS - reserve;
	b->reserve_start = b->data_blocks;
	b->next_spare = 0;
	b->seq = 0;
	b->meta_idx = 0;
}

/* Return the next unused good block from the reserved spare pool. */
static uint16_t bbm_alloc_spare(nand_bbm_t *b)
{
	while (b->next_spare < b->reserve_count) {
		uint32_t phys = b->reserve_start + b->next_spare;
		b->next_spare++;
		if (!bbm_bad_get(b, phys)) {
			return (uint16_t)phys;
		}
	}
	QSPI_LOGE("bbm: spare pool exhausted\r\n");
	return NAND_BBM_INVALID_PHYS;
}

static bk_err_t bbm_phys_block(const nand_bbm_t *b, uint32_t log_block, uint32_t *phys)
{
	if (log_block >= b->data_blocks) {
		return BK_ERR_PARAM;
	}
	uint16_t p = b->l2p[log_block];
	if (p == NAND_BBM_INVALID_PHYS) {
		return BK_ERR_QSPI_NAND_NO_SPARE;
	}
	*phys = p;
	return BK_OK;
}

static bk_err_t bbm_persist(qspi_id_t id, nand_bbm_t *b)
{
	uint32_t l2p_bytes = bbm_l2p_bytes(b);
	uint32_t bad_bytes = bbm_bad_bytes(b);
	uint32_t rec = sizeof(nand_bbm_hdr_t) + l2p_bytes + bad_bytes;

	uint8_t *buf = (uint8_t *)os_zalloc(rec);
	if (!buf) {
		return BK_ERR_NO_MEM;
	}

	uint8_t *payload = buf + sizeof(nand_bbm_hdr_t);
	os_memcpy(payload, b->l2p, l2p_bytes);
	os_memcpy(payload + l2p_bytes, b->bad, bad_bytes);

	nand_bbm_hdr_t *h = (nand_bbm_hdr_t *)buf;
	h->magic = NAND_BBM_MAGIC;
	h->version = NAND_BBM_VERSION;
	h->seq = ++b->seq;
	h->total_blocks = b->total_blocks;
	h->data_blocks = b->data_blocks;
	h->reserve_start = b->reserve_start;
	h->reserve_count = b->reserve_count;
	h->next_spare = b->next_spare;
	h->l2p_bytes = l2p_bytes;
	h->bad_bytes = bad_bytes;
	h->crc = hnd_crc8(payload, l2p_bytes + bad_bytes, NAND_BBM_CRC_INIT);

	uint32_t meta = b->meta_block[b->meta_idx];
	bk_err_t ret = bk_qspi_flash_nand_block_erase(id, meta);
	if (ret == BK_OK) {
		ret = bk_qspi_flash_write(id, meta * NAND_BBM_BLOCK_BYTES, buf, rec);
	}
	if (ret != BK_OK) {
		QSPI_LOGE("bbm: persist copy %u failed(%d)\r\n", b->meta_idx, ret);
		b->seq--;
	} else {
		b->meta_idx ^= 1U;
	}
	os_free(buf);
	return ret;
}

static bool bbm_load_copy(qspi_id_t id, nand_bbm_t *b, uint32_t meta_block,
                          uint8_t *tmp, uint32_t rec, uint32_t *out_seq)
{
	if (bk_qspi_flash_read(id, meta_block * NAND_BBM_BLOCK_BYTES, tmp, rec) != BK_OK) {
		return false;
	}

	nand_bbm_hdr_t *h = (nand_bbm_hdr_t *)tmp;
	uint32_t l2p_bytes = bbm_l2p_bytes(b);
	uint32_t bad_bytes = bbm_bad_bytes(b);

	if (h->magic != NAND_BBM_MAGIC || h->version != NAND_BBM_VERSION) {
		return false;
	}
	if (h->total_blocks != b->total_blocks || h->data_blocks != b->data_blocks ||
	    h->l2p_bytes != l2p_bytes || h->bad_bytes != bad_bytes) {
		return false;
	}

	uint8_t *payload = tmp + sizeof(nand_bbm_hdr_t);
	if (hnd_crc8(payload, l2p_bytes + bad_bytes, NAND_BBM_CRC_INIT) != h->crc) {
		return false;
	}

	*out_seq = h->seq;
	return true;
}

static void bbm_load_apply(nand_bbm_t *b, const uint8_t *tmp)
{
	nand_bbm_hdr_t *h = (nand_bbm_hdr_t *)tmp;
	uint32_t l2p_bytes = bbm_l2p_bytes(b);
	uint32_t bad_bytes = bbm_bad_bytes(b);
	const uint8_t *payload = tmp + sizeof(nand_bbm_hdr_t);

	os_memcpy(b->l2p, payload, l2p_bytes);
	os_memcpy(b->bad, payload + l2p_bytes, bad_bytes);
	b->next_spare = h->next_spare;
	b->seq = h->seq;
}

/* Load the freshest valid BBT copy; returns true if one was restored. */
static bool bbm_load(qspi_id_t id, nand_bbm_t *b)
{
	uint32_t rec = sizeof(nand_bbm_hdr_t) + bbm_l2p_bytes(b) + bbm_bad_bytes(b);
	uint8_t *tmp = (uint8_t *)os_malloc(rec);
	if (!tmp) {
		return false;
	}

	bool found = false;
	uint32_t best_seq = 0;

	for (uint8_t i = 0; i < NAND_BBM_META_BLOCKS; i++) {
		uint32_t seq = 0;
		if (!bbm_load_copy(id, b, b->meta_block[i], tmp, rec, &seq)) {
			continue;
		}
		if (!found || seq >= best_seq) {
			found = true;
			best_seq = seq;
			bbm_load_apply(b, tmp);
			/* Next persist overwrites the older copy. */
			b->meta_idx = (i == 0) ? 1U : 0U;
		}
	}

	os_free(tmp);
	return found;
}

/* First-time bring-up: scan factory bad blocks and build the remap. */
static void bbm_factory_scan(qspi_id_t id, nand_bbm_t *b)
{
	for (uint32_t blk = 0; blk < b->total_blocks; blk++) {
		bool bad = false;
		if (bk_qspi_flash_nand_is_factory_bad(id, blk, &bad) == BK_OK && bad) {
			bbm_bad_set(b, blk);
			QSPI_LOGW("bbm: factory bad block %u\r\n", blk);
		}
	}

	for (uint32_t lb = 0; lb < b->data_blocks; lb++) {
		if (bbm_bad_get(b, lb)) {
			b->l2p[lb] = bbm_alloc_spare(b);
		} else {
			b->l2p[lb] = (uint16_t)lb;
		}
	}
}

/* Retire the physical block backing a logical block and remap it to a spare. */
static bk_err_t bbm_retire_and_remap(qspi_id_t id, nand_bbm_t *b, uint32_t log_block)
{
	uint16_t old = b->l2p[log_block];
	if (old != NAND_BBM_INVALID_PHYS) {
		bbm_bad_set(b, old);
	}

	uint16_t sp = bbm_alloc_spare(b);
	if (sp == NAND_BBM_INVALID_PHYS) {
		return BK_ERR_QSPI_NAND_NO_SPARE;
	}
	b->l2p[log_block] = sp;
	bk_err_t persist_ret = bbm_persist(id, b);
	QSPI_LOGW("bbm: logical %u retired phys %u -> spare %u\r\n", log_block, old, sp);
	return persist_ret;
}

bk_err_t bk_qspi_nand_bbm_init(qspi_id_t id)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b) {
		return BK_ERR_PARAM;
	}
	if (b->inited) {
		return BK_OK;
	}

	bbm_compute_layout(b);

	b->l2p = (uint16_t *)os_malloc(bbm_l2p_bytes(b));
	b->bad = (uint8_t *)os_zalloc(bbm_bad_bytes(b));
	if (!b->l2p || !b->bad) {
		if (b->l2p) { os_free(b->l2p); b->l2p = NULL; }
		if (b->bad) { os_free(b->bad); b->bad = NULL; }
		return BK_ERR_NO_MEM;
	}

	for (uint32_t i = 0; i < b->data_blocks; i++) {
		b->l2p[i] = (uint16_t)i;
	}

	if (bbm_load(id, b)) {
		QSPI_LOGI("bbm: BBT restored (seq=%u, data_blocks=%u)\r\n", b->seq, b->data_blocks);
	} else {
		QSPI_LOGI("bbm: no valid BBT, scanning factory bad blocks\r\n");
		bbm_factory_scan(id, b);
		(void)bbm_persist(id, b);
	}

	b->inited = true;
	return BK_OK;
}

uint32_t bk_qspi_nand_bbm_logical_size(qspi_id_t id)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b || !b->inited) {
		return 0;
	}
	return b->data_blocks * NAND_BBM_BLOCK_BYTES;
}

bk_err_t bk_qspi_nand_bbm_read(qspi_id_t id, uint32_t log_addr, void *buf, uint32_t len)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b || !b->inited) {
		return BK_ERR_STATE;
	}
	BK_RETURN_ON_NULL(buf);

	uint8_t *p = (uint8_t *)buf;
	while (len) {
		uint32_t lb = log_addr / NAND_BBM_BLOCK_BYTES;
		uint32_t off = log_addr % NAND_BBM_BLOCK_BYTES;
		uint32_t chunk = NAND_BBM_BLOCK_BYTES - off;
		if (chunk > len) {
			chunk = len;
		}

		uint32_t phys = 0;
		BK_RETURN_ON_ERR(bbm_phys_block(b, lb, &phys));
		BK_RETURN_ON_ERR(bk_qspi_flash_read(id, phys * NAND_BBM_BLOCK_BYTES + off, p, chunk));

		log_addr += chunk;
		p += chunk;
		len -= chunk;
	}
	return BK_OK;
}

bk_err_t bk_qspi_nand_bbm_prog(qspi_id_t id, uint32_t log_addr, const void *buf, uint32_t len)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b || !b->inited) {
		return BK_ERR_STATE;
	}
	BK_RETURN_ON_NULL(buf);

	const uint8_t *p = (const uint8_t *)buf;
	while (len) {
		uint32_t lb = log_addr / NAND_BBM_BLOCK_BYTES;
		uint32_t off = log_addr % NAND_BBM_BLOCK_BYTES;
		uint32_t chunk = NAND_BBM_BLOCK_BYTES - off;
		if (chunk > len) {
			chunk = len;
		}

		uint32_t phys = 0;
		BK_RETURN_ON_ERR(bbm_phys_block(b, lb, &phys));

		bk_err_t ret = bk_qspi_flash_write(id, phys * NAND_BBM_BLOCK_BYTES + off, p, chunk);
		if (ret != BK_OK) {
			/* Retire the block so future use avoids it, but surface the error
			 * so the filesystem relocates this write elsewhere. */
			(void)bbm_retire_and_remap(id, b, lb);
			return ret;
		}

		log_addr += chunk;
		p += chunk;
		len -= chunk;
	}
	return BK_OK;
}

bk_err_t bk_qspi_nand_bbm_erase(qspi_id_t id, uint32_t log_addr, uint32_t size)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b || !b->inited) {
		return BK_ERR_STATE;
	}
	if ((log_addr % NAND_BBM_BLOCK_BYTES) || (size % NAND_BBM_BLOCK_BYTES) || size == 0) {
		return BK_ERR_PARAM;
	}

	while (size) {
		uint32_t lb = log_addr / NAND_BBM_BLOCK_BYTES;
		uint32_t phys = 0;
		BK_RETURN_ON_ERR(bbm_phys_block(b, lb, &phys));

		bk_err_t ret = bk_qspi_flash_nand_block_erase(id, phys);
		if (ret != BK_OK) {
			BK_RETURN_ON_ERR(bbm_retire_and_remap(id, b, lb));
			BK_RETURN_ON_ERR(bbm_phys_block(b, lb, &phys));
			BK_RETURN_ON_ERR(bk_qspi_flash_nand_block_erase(id, phys));
		}

		log_addr += NAND_BBM_BLOCK_BYTES;
		size -= NAND_BBM_BLOCK_BYTES;
	}
	return BK_OK;
}

void bk_qspi_nand_bbm_dump(qspi_id_t id)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b || !b->inited) {
		QSPI_LOGI("bbm: not initialized\r\n");
		return;
	}

	QSPI_LOGI("=== QSPI NAND BBM (id=%u) ===\r\n", id);
	QSPI_LOGI("total_blocks=%u data_blocks=%u\r\n", b->total_blocks, b->data_blocks);
	QSPI_LOGI("reserve pool: start=%u count=%u used=%u\r\n",
	          b->reserve_start, b->reserve_count, b->next_spare);
	QSPI_LOGI("meta blocks: %u / %u, seq=%u\r\n",
	          b->meta_block[0], b->meta_block[1], b->seq);

	uint32_t bad_cnt = 0;
	for (uint32_t blk = 0; blk < b->total_blocks; blk++) {
		if (bbm_bad_get(b, blk)) {
			QSPI_LOGI("  bad phys block %u\r\n", blk);
			bad_cnt++;
		}
	}
	QSPI_LOGI("bad block count=%u\r\n", bad_cnt);

	uint32_t remap_cnt = 0;
	for (uint32_t lb = 0; lb < b->data_blocks; lb++) {
		if (b->l2p[lb] != (uint16_t)lb) {
			QSPI_LOGI("  remap logical %u -> phys %u\r\n", lb, b->l2p[lb]);
			remap_cnt++;
		}
	}
	QSPI_LOGI("active remap count=%u\r\n", remap_cnt);
}

bk_err_t bk_qspi_nand_bbm_inject_bad(qspi_id_t id, uint32_t log_block)
{
	nand_bbm_t *b = bbm_get(id);
	if (!b || !b->inited) {
		return BK_ERR_STATE;
	}
	if (log_block >= b->data_blocks) {
		return BK_ERR_PARAM;
	}
	return bbm_retire_and_remap(id, b, log_block);
}

#endif /* CONFIG_QSPI_NAND_FLASH */
