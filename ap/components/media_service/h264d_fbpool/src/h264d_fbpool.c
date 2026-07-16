// Copyright 2026-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "h264d_fbpool.h"

#define TAG "h264d_fbpool"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define FBPOOL_MAX_DPB_SLOTS   16U
#define FBPOOL_DISP_DEPTH_MAX  4U
/* 16 DPB references + 1 decode target + display margin */
#define FBPOOL_MAX_SLOTS       (FBPOOL_MAX_DPB_SLOTS + 1U + FBPOOL_DISP_DEPTH_MAX)
#define FBPOOL_DEFAULT_ALIGN   64U
/* HW writes a trailing completion-sync region after the MV data. */
#define FBPOOL_SYNC_BYTES      32U

struct h264d_fbpool {
	uint32_t              align;
	uint16_t              disp_depth;
	h264d_fbpool_alloc_cb alloc_cb;
	h264d_fbpool_free_cb  free_cb;

	uint16_t              count;        /* active slots = dpb_count + disp_depth */
	uint32_t              yuv_size;     /* per-slot YUV bytes (pre-align) */
	uint32_t              mv_size;      /* per-slot MV bytes, 0 = none */

	vcdec_fb_t            slots[FBPOOL_MAX_SLOTS];
	void                 *raw[FBPOOL_MAX_SLOTS];   /* original alloc ptr for free */

	uint8_t               free_idx[FBPOOL_MAX_SLOTS];
	uint16_t              free_top;     /* number of entries on the free stack */

	uint8_t               disp_q[FBPOOL_MAX_SLOTS];
	uint16_t              disp_head;
	uint16_t              disp_tail;
	uint16_t              disp_cnt;

	uint32_t              acquire_timeout_ms; /* 0 = non-blocking acquire */

	beken_mutex_t         lock;
	beken_semaphore_t     disp_sem;
	beken_semaphore_t     free_sem;   /* posted whenever a slot returns to FREE */

	vcdec_fb_if_t         ifc;
};

static void *fbpool_default_alloc(uint32_t size, uint32_t align)
{
	(void)align;
	return os_malloc(size);
}

static void fbpool_default_free(void *ptr)
{
	os_free(ptr);
}

static void fbpool_release_buffers(h264d_fbpool_t *pool)
{
	uint32_t i;

	for (i = 0; i < FBPOOL_MAX_SLOTS; i++) {
		if (pool->raw[i] != NULL) {
			pool->free_cb(pool->raw[i]);
			pool->raw[i] = NULL;
		}
		os_memset(&pool->slots[i], 0, sizeof(pool->slots[i]));
	}
	pool->count = 0U;
	pool->free_top = 0U;
	pool->disp_head = 0U;
	pool->disp_tail = 0U;
	pool->disp_cnt = 0U;
}

/* ---- decode-side vtable implementation ------------------------------------ */

static vcdec_ret_e fbpool_reconfigure(void *ctx, uint16_t dpb_count,
                                      uint32_t yuv_size, uint32_t mv_size)
{
	h264d_fbpool_t *pool = (h264d_fbpool_t *)ctx;
	uint32_t i;
	uint32_t count;
	uint32_t aligned_yuv;
	uint32_t slot_bytes;

	if (pool == NULL || yuv_size == 0U) {
		return VCDEC_INVALID_ARGUMENT;
	}

	count = (uint32_t)dpb_count + (uint32_t)pool->disp_depth;
	if (count == 0U || count > FBPOOL_MAX_SLOTS) {
		LOGE("reconfigure bad count=%u (dpb=%u disp=%u)\r\n",
		     (unsigned)count, (unsigned)dpb_count, (unsigned)pool->disp_depth);
		return VCDEC_INVALID_ARGUMENT;
	}

	rtos_lock_mutex(&pool->lock);

	/* Geometry unchanged -> idempotent. */
	if (pool->count == count && pool->yuv_size == yuv_size && pool->mv_size == mv_size) {
		rtos_unlock_mutex(&pool->lock);
		return VCDEC_OK;
	}

	fbpool_release_buffers(pool);

	aligned_yuv = (yuv_size + pool->align - 1U) & ~(pool->align - 1U);
	/* dirMvOffset == yuv_size (MV region immediately follows YUV, matching the
	 * native driver). Use the unaligned yuv_size as the offset so y_bus+yuv_size
	 * lands exactly where the HW expects colocated MVs. */
	slot_bytes = aligned_yuv + mv_size + FBPOOL_SYNC_BYTES;

	for (i = 0; i < count; i++) {
		uint8_t *raw;
		uintptr_t base;

		raw = (uint8_t *)pool->alloc_cb(slot_bytes + pool->align, pool->align);
		if (raw == NULL) {
			LOGE("alloc slot %u failed (size=%u)\r\n", (unsigned)i, (unsigned)slot_bytes);
			fbpool_release_buffers(pool);
			rtos_unlock_mutex(&pool->lock);
			return VCDEC_MEMORY_ERROR;
		}
		pool->raw[i] = raw;
		base = ((uintptr_t)raw + (pool->align - 1U)) & ~(uintptr_t)(pool->align - 1U);

		os_memset(&pool->slots[i], 0, sizeof(pool->slots[i]));
		pool->slots[i].y_ptr = (uint8_t *)base;
		pool->slots[i].y_bus = (uint32_t)base;
		pool->slots[i].capacity = aligned_yuv;
		if (mv_size != 0U) {
			pool->slots[i].mv_ptr = (uint8_t *)(base + yuv_size);
			pool->slots[i].mv_bus = (uint32_t)(base + yuv_size);
			pool->slots[i].mv_size = mv_size;
			os_memset(pool->slots[i].mv_ptr, 0, mv_size + FBPOOL_SYNC_BYTES);
		}
		pool->slots[i].index = (uint8_t)i;
		pool->slots[i].state = VCDEC_FB_FREE;
		pool->free_idx[pool->free_top++] = (uint8_t)i;
	}

	pool->count = (uint16_t)count;
	pool->yuv_size = yuv_size;
	pool->mv_size = mv_size;
	rtos_unlock_mutex(&pool->lock);

	LOGI("reconfigure ok: slots=%u yuv=%u mv=%u align=%u\r\n",
	     (unsigned)count, (unsigned)yuv_size, (unsigned)mv_size, (unsigned)pool->align);
	return VCDEC_OK;
}

static vcdec_fb_t *fbpool_take_free_locked(h264d_fbpool_t *pool)
{
	uint8_t idx;
	vcdec_fb_t *fb;

	if (pool->free_top == 0U) {
		return NULL;
	}
	idx = pool->free_idx[--pool->free_top];
	fb = &pool->slots[idx];
	fb->refcount = 1;
	fb->state = VCDEC_FB_DECODING;
	fb->in_disp_q = 0U;
	fb->is_reference = 0U;
	fb->is_long_term = 0U;
	return fb;
}

/*
 * Acquire a free slot as the decode target. When no slot is free, wait up to
 * acquire_timeout_ms for the application to release a display frame (release
 * runs on another thread and posts free_sem) and retry; return NULL on timeout
 * so the caller can drop the frame. The deadline loop always re-checks the free
 * list under the lock, so it is robust to stale/extra free_sem posts. When a
 * slot is already free the fast path returns immediately (behavior-neutral
 * under no contention).
 */
static vcdec_fb_t *fbpool_acquire(void *ctx)
{
	h264d_fbpool_t *pool = (h264d_fbpool_t *)ctx;
	vcdec_fb_t *fb;
	uint32_t start;
	uint32_t timeout;

	if (pool == NULL) {
		return NULL;
	}

	timeout = pool->acquire_timeout_ms;
	start = rtos_get_time();

	for (;;) {
		rtos_lock_mutex(&pool->lock);
		fb = fbpool_take_free_locked(pool);
		rtos_unlock_mutex(&pool->lock);
		if (fb != NULL) {
			return fb;
		}

		if (timeout == 0U) {
			LOGE("acquire failed: no free slot (non-blocking)\r\n");
			return NULL;
		}

		uint32_t elapsed = rtos_get_time() - start; /* unsigned: wrap-safe */
		if (elapsed >= timeout) {
			LOGE("acquire timeout (%ums): no free slot, drop frame\r\n",
			     (unsigned)timeout);
			return NULL;
		}

		/* Wait for a release to free a slot; result ignored, loop rechecks. */
		(void)rtos_get_semaphore(&pool->free_sem, timeout - elapsed);
	}
}

static void fbpool_ref(void *ctx, vcdec_fb_t *fb)
{
	h264d_fbpool_t *pool = (h264d_fbpool_t *)ctx;

	if (pool == NULL || fb == NULL) {
		return;
	}
	rtos_lock_mutex(&pool->lock);
	fb->refcount++;
	rtos_unlock_mutex(&pool->lock);
}

static void fbpool_unref(void *ctx, vcdec_fb_t *fb)
{
	h264d_fbpool_t *pool = (h264d_fbpool_t *)ctx;
	uint8_t freed = 0U;

	if (pool == NULL || fb == NULL) {
		return;
	}
	rtos_lock_mutex(&pool->lock);
	if (fb->refcount > 0) {
		fb->refcount--;
	}
	if (fb->refcount <= 0) {
		fb->refcount = 0;
		fb->state = VCDEC_FB_FREE;
		fb->is_reference = 0U;
		fb->in_disp_q = 0U;
		pool->free_idx[pool->free_top++] = fb->index;
		freed = 1U;
	}
	rtos_unlock_mutex(&pool->lock);
	if (freed) {
		rtos_set_semaphore(&pool->free_sem);
	}
}

static void fbpool_publish(void *ctx, vcdec_fb_t *fb)
{
	h264d_fbpool_t *pool = (h264d_fbpool_t *)ctx;

	if (pool == NULL || fb == NULL) {
		return;
	}
	rtos_lock_mutex(&pool->lock);
	if (!fb->in_disp_q) {
		fb->in_disp_q = 1U;
		fb->refcount++;            /* display/app hold */
		fb->state = VCDEC_FB_READY;
		pool->disp_q[pool->disp_tail] = fb->index;
		pool->disp_tail = (uint16_t)((pool->disp_tail + 1U) % FBPOOL_MAX_SLOTS);
		pool->disp_cnt++;
		rtos_unlock_mutex(&pool->lock);
		rtos_set_semaphore(&pool->disp_sem);
		return;
	}
	rtos_unlock_mutex(&pool->lock);
}

static void fbpool_flush(void *ctx)
{
	h264d_fbpool_t *pool = (h264d_fbpool_t *)ctx;

	if (pool == NULL) {
		return;
	}
	/* Drop pending (not-yet-dequeued) display holds. Frames already handed to
	 * the application keep their hold until released. */
	uint32_t freed = 0U;
	rtos_lock_mutex(&pool->lock);
	while (pool->disp_cnt > 0U) {
		uint8_t idx = pool->disp_q[pool->disp_head];
		vcdec_fb_t *fb = &pool->slots[idx];
		pool->disp_head = (uint16_t)((pool->disp_head + 1U) % FBPOOL_MAX_SLOTS);
		pool->disp_cnt--;
		fb->in_disp_q = 0U;
		if (fb->refcount > 0) {
			fb->refcount--;
		}
		if (fb->refcount <= 0) {
			fb->refcount = 0;
			fb->state = VCDEC_FB_FREE;
			fb->is_reference = 0U;
			pool->free_idx[pool->free_top++] = fb->index;
			freed++;
		}
	}
	/* Drain any stale semaphore counts. */
	rtos_unlock_mutex(&pool->lock);
	while (rtos_get_semaphore(&pool->disp_sem, 0) == BK_OK) {
		;
	}
	while (freed-- > 0U) {
		rtos_set_semaphore(&pool->free_sem);
	}
}

/* ---- application-side API -------------------------------------------------- */

avdk_err_t h264d_fbpool_create(h264d_fbpool_t **pool_p, uint32_t align,
                               uint16_t disp_depth,
                               h264d_fbpool_alloc_cb alloc_cb,
                               h264d_fbpool_free_cb free_cb)
{
	h264d_fbpool_t *pool;

	if (pool_p == NULL) {
		return AVDK_ERR_INVAL;
	}
	if (disp_depth == 0U || disp_depth > FBPOOL_DISP_DEPTH_MAX) {
		LOGE("invalid disp_depth=%u (max=%u)\r\n",
		     (unsigned)disp_depth, (unsigned)FBPOOL_DISP_DEPTH_MAX);
		return AVDK_ERR_INVAL;
	}
	if ((alloc_cb == NULL) != (free_cb == NULL)) {
		return AVDK_ERR_INVAL;
	}

	pool = (h264d_fbpool_t *)os_malloc(sizeof(*pool));
	if (pool == NULL) {
		return AVDK_ERR_NOMEM;
	}
	os_memset(pool, 0, sizeof(*pool));
	pool->align = (align != 0U) ? align : FBPOOL_DEFAULT_ALIGN;
	/* align must be a power of two for the masking above */
	if ((pool->align & (pool->align - 1U)) != 0U) {
		pool->align = FBPOOL_DEFAULT_ALIGN;
	}
	pool->disp_depth = disp_depth;
	pool->alloc_cb = (alloc_cb != NULL) ? alloc_cb : fbpool_default_alloc;
	pool->free_cb = (free_cb != NULL) ? free_cb : fbpool_default_free;

	if (rtos_init_mutex(&pool->lock) != BK_OK) {
		os_free(pool);
		return AVDK_ERR_NOMEM;
	}
	if (rtos_init_semaphore(&pool->disp_sem, FBPOOL_MAX_SLOTS) != BK_OK) {
		rtos_deinit_mutex(&pool->lock);
		os_free(pool);
		return AVDK_ERR_NOMEM;
	}
	if (rtos_init_semaphore(&pool->free_sem, FBPOOL_MAX_SLOTS) != BK_OK) {
		rtos_deinit_semaphore(&pool->disp_sem);
		rtos_deinit_mutex(&pool->lock);
		os_free(pool);
		return AVDK_ERR_NOMEM;
	}
	pool->acquire_timeout_ms = 0U; /* non-blocking until configured */

	pool->ifc.ctx = pool;
	pool->ifc.reconfigure = fbpool_reconfigure;
	pool->ifc.acquire = fbpool_acquire;
	pool->ifc.ref = fbpool_ref;
	pool->ifc.unref = fbpool_unref;
	pool->ifc.publish = fbpool_publish;
	pool->ifc.flush = fbpool_flush;

	*pool_p = pool;
	return AVDK_ERR_OK;
}

void h264d_fbpool_destroy(h264d_fbpool_t *pool)
{
	if (pool == NULL) {
		return;
	}
	rtos_lock_mutex(&pool->lock);
	fbpool_release_buffers(pool);
	rtos_unlock_mutex(&pool->lock);
	rtos_deinit_semaphore(&pool->free_sem);
	rtos_deinit_semaphore(&pool->disp_sem);
	rtos_deinit_mutex(&pool->lock);
	os_free(pool);
}

void h264d_fbpool_set_acquire_timeout(h264d_fbpool_t *pool, uint32_t timeout_ms)
{
	if (pool == NULL) {
		return;
	}
	rtos_lock_mutex(&pool->lock);
	pool->acquire_timeout_ms = timeout_ms;
	rtos_unlock_mutex(&pool->lock);
}

avdk_err_t h264d_fbpool_get_if(h264d_fbpool_t *pool, vcdec_fb_if_t *out)
{
	if (pool == NULL || out == NULL) {
		return AVDK_ERR_INVAL;
	}
	*out = pool->ifc;
	return AVDK_ERR_OK;
}

avdk_err_t h264d_fbpool_dequeue(h264d_fbpool_t *pool, vcdec_frame_t *out,
                                uint32_t timeout_ms)
{
	vcdec_fb_t *fb;
	uint8_t idx;

	if (pool == NULL || out == NULL) {
		return AVDK_ERR_INVAL;
	}
	if (rtos_get_semaphore(&pool->disp_sem, timeout_ms) != BK_OK) {
		return AVDK_ERR_TIMEOUT;
	}

	rtos_lock_mutex(&pool->lock);
	if (pool->disp_cnt == 0U) {
		rtos_unlock_mutex(&pool->lock);
		return AVDK_ERR_TIMEOUT;
	}
	idx = pool->disp_q[pool->disp_head];
	pool->disp_head = (uint16_t)((pool->disp_head + 1U) % FBPOOL_MAX_SLOTS);
	pool->disp_cnt--;
	fb = &pool->slots[idx];
	fb->in_disp_q = 0U;
	fb->state = VCDEC_FB_INUSE;

	out->data = fb->y_ptr;
	out->data_len = fb->data_len;
	out->capacity = fb->capacity;
	out->width = fb->width;
	out->height = fb->height;
	out->format = fb->format;
	out->frame_type = fb->frame_type;
	out->poc = fb->poc;
	out->token = fb;
	rtos_unlock_mutex(&pool->lock);
	return AVDK_ERR_OK;
}

avdk_err_t h264d_fbpool_release(h264d_fbpool_t *pool, const vcdec_frame_t *frame)
{
	vcdec_fb_t *fb;

	if (pool == NULL || frame == NULL || frame->token == NULL) {
		return AVDK_ERR_INVAL;
	}
	fb = (vcdec_fb_t *)frame->token;
	uint8_t freed = 0U;
	rtos_lock_mutex(&pool->lock);
	if (fb->refcount > 0) {
		fb->refcount--;
	}
	if (fb->refcount <= 0) {
		fb->refcount = 0;
		fb->state = VCDEC_FB_FREE;
		fb->is_reference = 0U;
		fb->in_disp_q = 0U;
		pool->free_idx[pool->free_top++] = fb->index;
		freed = 1U;
	}
	rtos_unlock_mutex(&pool->lock);
	if (freed) {
		rtos_set_semaphore(&pool->free_sem);
	}
	return AVDK_ERR_OK;
}

uint16_t h264d_fbpool_free_count(h264d_fbpool_t *pool)
{
	uint16_t n;

	if (pool == NULL) {
		return 0U;
	}
	rtos_lock_mutex(&pool->lock);
	n = pool->free_top;
	rtos_unlock_mutex(&pool->lock);
	return n;
}
