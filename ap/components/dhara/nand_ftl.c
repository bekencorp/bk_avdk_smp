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

/*
 * 512B logical-sector block device over the Dhara FTL (see nand_ftl.h).
 *
 * Dhara maps in whole NAND pages (2048B). To expose 512B sectors for host / FAT
 * compatibility, sub-page writes read-modify-write the surrounding page and let
 * Dhara allocate a fresh physical page (its log-structured allocator performs
 * wear levelling and garbage collection). Every page program commits atomically,
 * so an interrupted write leaves the previous version intact after remount.
 */

#include <common/bk_include.h>

#if CONFIG_DHARA_FTL

#include <os/os.h>
#include <os/mem.h>
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include <driver/nand_ftl.h>
#include "nand_ftl_priv.h"

#define FTL_TAG "nand_ftl"
#define FTL_LOGI(...) BK_LOGI(FTL_TAG, ##__VA_ARGS__)
#define FTL_LOGW(...) BK_LOGW(FTL_TAG, ##__VA_ARGS__)
#define FTL_LOGE(...) BK_LOGE(FTL_TAG, ##__VA_ARGS__)

#ifndef CONFIG_NAND_FTL_START_BLOCK
#define CONFIG_NAND_FTL_START_BLOCK 0
#endif
#ifndef CONFIG_NAND_FTL_BLOCK_COUNT
#define CONFIG_NAND_FTL_BLOCK_COUNT 0   /* 0 => from start block to end of device */
#endif
#ifndef CONFIG_NAND_FTL_GC_RATIO
#define CONFIG_NAND_FTL_GC_RATIO 8
#endif

#ifdef CONFIG_QSPI_NAND_FLASH_SIZE
#define FTL_DEVICE_SIZE  ((uint64_t)CONFIG_QSPI_NAND_FLASH_SIZE)
#else
#define FTL_DEVICE_SIZE  ((uint64_t)128U * 1024U * 1024U)
#endif

static struct nand_ftl_ctx s_ctx[QSPI_ID_MAX];

struct nand_ftl_ctx *nand_ftl_get(qspi_id_t id)
{
	return (id < QSPI_ID_MAX) ? &s_ctx[id] : NULL;
}

static void nand_ftl_free_bufs(struct nand_ftl_ctx *c)
{
	if (c->page_buf) { os_free(c->page_buf); c->page_buf = NULL; }
	if (c->rmw_buf) { os_free(c->rmw_buf); c->rmw_buf = NULL; }
	if (c->copy_buf) { os_free(c->copy_buf); c->copy_buf = NULL; }
	if (c->bad_bitmap) { os_free(c->bad_bitmap); c->bad_bitmap = NULL; }
}

bk_err_t bk_nand_ftl_init(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c) {
		return BK_ERR_PARAM;
	}
	if (c->inited) {
		return BK_OK;
	}

	uint32_t device_blocks = (uint32_t)(FTL_DEVICE_SIZE / NAND_FTL_BLOCK_SIZE);
	uint32_t start = (uint32_t)CONFIG_NAND_FTL_START_BLOCK;
	uint32_t count = (uint32_t)CONFIG_NAND_FTL_BLOCK_COUNT;

	if (start >= device_blocks) {
		FTL_LOGE("start block %u >= device blocks %u\r\n", start, device_blocks);
		return BK_ERR_PARAM;
	}
	if (count == 0 || (start + count) > device_blocks) {
		count = device_blocks - start;
	}
	if (count < 4) {
		FTL_LOGE("partition too small: %u blocks\r\n", count);
		return BK_ERR_PARAM;
	}

	BK_RETURN_ON_ERR(bk_qspi_driver_init());
	BK_RETURN_ON_ERR(bk_qspi_flash_init(id));

	c->id = id;
	c->start_block = start;
	c->nand.log2_page_size = NAND_FTL_LOG2_PAGE_SIZE;
	c->nand.log2_ppb = NAND_FTL_LOG2_PPB;
	c->nand.num_blocks = count;
	c->bad_bitmap_bytes = (count + 7U) / 8U;

	c->page_buf = (uint8_t *)os_malloc(NAND_FTL_PAGE_SIZE);
	c->rmw_buf = (uint8_t *)os_malloc(NAND_FTL_PAGE_SIZE);
	c->copy_buf = (uint8_t *)os_malloc(NAND_FTL_PAGE_SIZE);
	c->bad_bitmap = (uint8_t *)os_zalloc(c->bad_bitmap_bytes);
	if (!c->page_buf || !c->rmw_buf || !c->copy_buf || !c->bad_bitmap) {
		nand_ftl_free_bufs(c);
		return BK_ERR_NO_MEM;
	}

	if (rtos_init_recursive_mutex(&c->lock) != BK_OK) {
		nand_ftl_free_bufs(c);
		return BK_FAIL;
	}

	dhara_map_init(&c->map, &c->nand, c->page_buf, (uint8_t)CONFIG_NAND_FTL_GC_RATIO);

	dhara_error_t err = DHARA_E_NONE;
	if (dhara_map_resume(&c->map, &err) < 0) {
		/* No valid stored state: an empty map is ready. First write (or a
		 * host/FatFS mkfs) will lay down a fresh filesystem. On a never-used
		 * device dhara_map_resume() reports DHARA_E_TOO_BAD simply because it
		 * found no valid checkpoint (not a real bad-block problem), so word the
		 * log so it isn't mistaken for a defective part. */
		FTL_LOGI("no valid journal found (first use / unformatted), starting empty [dhara:%s]\r\n",
			 dhara_strerror(err));
	} else {
		FTL_LOGI("FTL resumed: %u/%u sectors used\r\n",
			 (unsigned)dhara_map_size(&c->map),
			 (unsigned)dhara_map_capacity(&c->map));
	}

	c->inited = true;
	FTL_LOGI("init id=%u start_blk=%u blocks=%u capacity=%u sectors(512B)=%u\r\n",
		 id, start, count, (unsigned)dhara_map_capacity(&c->map),
		 (unsigned)(dhara_map_capacity(&c->map) * NAND_FTL_SECTORS_PER_PAGE));
	return BK_OK;
}

bool bk_nand_ftl_is_inited(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	return c && c->inited;
}

uint32_t bk_nand_ftl_sector_size(qspi_id_t id)
{
	(void)id;
	return NAND_FTL_SECTOR_SIZE;
}

uint32_t bk_nand_ftl_sector_count(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		return 0;
	}
	return (uint32_t)dhara_map_capacity(&c->map) * NAND_FTL_SECTORS_PER_PAGE;
}

bk_err_t bk_nand_ftl_read(qspi_id_t id, uint32_t sector, uint8_t *buf, uint32_t count)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		return BK_ERR_STATE;
	}
	BK_RETURN_ON_NULL(buf);

	rtos_lock_recursive_mutex(&c->lock);
	bk_err_t status = BK_OK;

	while (count) {
		uint32_t dpage = sector / NAND_FTL_SECTORS_PER_PAGE;
		uint32_t sub = sector % NAND_FTL_SECTORS_PER_PAGE;
		uint32_t nsub = NAND_FTL_SECTORS_PER_PAGE - sub;
		if (nsub > count) {
			nsub = count;
		}

		dhara_error_t err = DHARA_E_NONE;
		if (dhara_map_read(&c->map, dpage, c->rmw_buf, &err) < 0) {
			FTL_LOGE("read dpage %u failed (%s)\r\n", dpage, dhara_strerror(err));
			status = BK_FAIL;
			break;
		}

		os_memcpy(buf, c->rmw_buf + sub * NAND_FTL_SECTOR_SIZE, nsub * NAND_FTL_SECTOR_SIZE);

		buf += nsub * NAND_FTL_SECTOR_SIZE;
		sector += nsub;
		count -= nsub;
	}

	rtos_unlock_recursive_mutex(&c->lock);
	return status;
}

bk_err_t bk_nand_ftl_write(qspi_id_t id, uint32_t sector, const uint8_t *buf, uint32_t count)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		return BK_ERR_STATE;
	}
	BK_RETURN_ON_NULL(buf);

	rtos_lock_recursive_mutex(&c->lock);
	bk_err_t status = BK_OK;

	while (count) {
		uint32_t dpage = sector / NAND_FTL_SECTORS_PER_PAGE;
		uint32_t sub = sector % NAND_FTL_SECTORS_PER_PAGE;
		uint32_t nsub = NAND_FTL_SECTORS_PER_PAGE - sub;
		if (nsub > count) {
			nsub = count;
		}

		dhara_error_t err = DHARA_E_NONE;

		/* Only read-modify-write when the write does not cover the whole
		 * page; a full-page write overwrites every byte anyway. */
		if (nsub != NAND_FTL_SECTORS_PER_PAGE) {
			if (dhara_map_read(&c->map, dpage, c->rmw_buf, &err) < 0) {
				FTL_LOGE("rmw read dpage %u failed (%s)\r\n", dpage, dhara_strerror(err));
				status = BK_FAIL;
				break;
			}
		}

		os_memcpy(c->rmw_buf + sub * NAND_FTL_SECTOR_SIZE, buf, nsub * NAND_FTL_SECTOR_SIZE);

		if (dhara_map_write(&c->map, dpage, c->rmw_buf, &err) < 0) {
			FTL_LOGE("write dpage %u failed (%s)\r\n", dpage, dhara_strerror(err));
			status = BK_FAIL;
			break;
		}

		buf += nsub * NAND_FTL_SECTOR_SIZE;
		sector += nsub;
		count -= nsub;
	}

	rtos_unlock_recursive_mutex(&c->lock);
	return status;
}

bk_err_t bk_nand_ftl_sync(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		return BK_ERR_STATE;
	}

	rtos_lock_recursive_mutex(&c->lock);
	dhara_error_t err = DHARA_E_NONE;
	int ret = dhara_map_sync(&c->map, &err);
	rtos_unlock_recursive_mutex(&c->lock);

	if (ret < 0) {
		FTL_LOGE("sync failed (%s)\r\n", dhara_strerror(err));
		return BK_FAIL;
	}
	return BK_OK;
}

bk_err_t bk_nand_ftl_format(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		return BK_ERR_STATE;
	}

	rtos_lock_recursive_mutex(&c->lock);
	dhara_map_clear(&c->map);
	dhara_error_t err = DHARA_E_NONE;
	int ret = dhara_map_sync(&c->map, &err);
	rtos_unlock_recursive_mutex(&c->lock);

	if (ret < 0) {
		FTL_LOGE("format sync failed (%s)\r\n", dhara_strerror(err));
		return BK_FAIL;
	}
	FTL_LOGI("formatted (map cleared)\r\n");
	return BK_OK;
}

bk_err_t bk_nand_ftl_inject_bad(qspi_id_t id, uint32_t block)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		return BK_ERR_STATE;
	}
	if (block >= c->nand.num_blocks) {
		return BK_ERR_PARAM;
	}

	rtos_lock_recursive_mutex(&c->lock);
	/* Destructive test hook: retire a physical block so Dhara must relocate
	 * around it on the next erase/prog. */
	nand_ftl_bad_set(c, block);
	(void)bk_qspi_flash_nand_mark_bad(c->id, nand_ftl_phys_block(c, block));
	rtos_unlock_recursive_mutex(&c->lock);

	FTL_LOGW("injected bad block %u (phys %u)\r\n", block, nand_ftl_phys_block(c, block));
	return BK_OK;
}

void bk_nand_ftl_dump(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		FTL_LOGI("FTL id=%u not initialized\r\n", id);
		return;
	}

	rtos_lock_recursive_mutex(&c->lock);
	uint32_t bad = 0;
	for (uint32_t b = 0; b < c->nand.num_blocks; b++) {
		if (nand_ftl_bad_get(c, b)) {
			bad++;
		}
	}

	FTL_LOGI("=== NAND FTL (id=%u) ===\r\n", id);
	FTL_LOGI("partition: start_blk=%u blocks=%u (%u KB/blk)\r\n",
		 c->start_block, c->nand.num_blocks, NAND_FTL_BLOCK_SIZE / 1024U);
	FTL_LOGI("capacity=%u pages, used=%u pages\r\n",
		 (unsigned)dhara_map_capacity(&c->map), (unsigned)dhara_map_size(&c->map));
	FTL_LOGI("logical: %u sectors x %uB\r\n",
		 (unsigned)(dhara_map_capacity(&c->map) * NAND_FTL_SECTORS_PER_PAGE), NAND_FTL_SECTOR_SIZE);
	FTL_LOGI("known bad blocks (cached): %u\r\n", bad);
	rtos_unlock_recursive_mutex(&c->lock);
}

void bk_nand_ftl_scan_factory_bad(qspi_id_t id)
{
	struct nand_ftl_ctx *c = nand_ftl_get(id);
	if (!c || !c->inited) {
		FTL_LOGI("FTL id=%u not initialized\r\n", id);
		return;
	}

	rtos_lock_recursive_mutex(&c->lock);

	uint32_t factory = 0;
	uint32_t cached = 0;
	FTL_LOGI("=== factory bad-block scan (id=%u) ===\r\n", id);
	FTL_LOGI("partition: start_blk=%u blocks=%u\r\n", c->start_block, c->nand.num_blocks);

	for (uint32_t b = 0; b < c->nand.num_blocks; b++) {
		uint32_t phys = nand_ftl_phys_block(c, b);
		bool is_bad = false;
		bk_err_t r = bk_qspi_flash_nand_is_factory_bad(c->id, phys, &is_bad);
		if (r != BK_OK) {
			FTL_LOGW("  blk %u (phys %u): read err %d\r\n", b, phys, r);
			continue;
		}
		if (is_bad) {
			factory++;
			FTL_LOGI("  FACTORY BAD: blk %u (phys %u)%s\r\n", b, phys,
				 nand_ftl_bad_get(c, b) ? " [also cached]" : "");
		}
		if (nand_ftl_bad_get(c, b)) {
			cached++;
		}
	}

	FTL_LOGI("factory bad total=%u, runtime cached total=%u (cached-minus-factory ~= runtime retired)\r\n",
		 factory, cached);
	rtos_unlock_recursive_mutex(&c->lock);
}

#endif /* CONFIG_DHARA_FTL */
