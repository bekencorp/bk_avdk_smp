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

#include <stdint.h>
#include <stdbool.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "../../../middleware/driver/spinlock/spinlock.h"
#include "../include/bk_frame_queue.h"

#define TAG "frame_queue"

#define FRAME_QUEUE_LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define FRAME_QUEUE_LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define FRAME_QUEUE_LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define FRAME_QUEUE_LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define BK_FRAME_QUEUE_MAX_CONSUMERS 32U
#define BK_FRAME_QUEUE_BLOCK_MAGIC 0x46515545U
#define BK_FRAME_QUEUE_EVENT_DATA (1U << 0)
#define BK_FRAME_QUEUE_EVENT_WAKE (1U << 1)

typedef enum
{
	BK_FRAME_QUEUE_BLOCK_FREE,
	BK_FRAME_QUEUE_BLOCK_WRITING,
	BK_FRAME_QUEUE_BLOCK_READY,
	BK_FRAME_QUEUE_BLOCK_IN_USE,
} bk_frame_queue_block_state_t;

typedef struct
{
	uint32_t magic;
	uint8_t *data;
	uint32_t capacity;
	uint32_t length;
	uint32_t sequence;
	uint32_t timestamp;
	void *user_data;
	uint32_t pending_consumer_mask;
	uint32_t acquired_consumer_mask;
	bk_frame_queue_block_state_t state;
	uint16_t index;
} bk_frame_queue_block_t;

struct bk_frame_queue
{
	bk_frame_queue_config_t config;
	uint8_t *pool;
	uint32_t pool_size;
	uint32_t head_stride;
	uint32_t payload_stride;
	uint32_t block_stride;
	beken_semaphore_t free_sem;
	beken_event_t *ready_events;
	bk_frame_queue_block_t **free_ring;
	bk_frame_queue_block_t **ready_rings;
	uint32_t free_head;
	uint32_t free_tail;
	uint32_t free_count;
	uint32_t *ready_heads;
	uint32_t *ready_tails;
	uint32_t *ready_counts;
#if CONFIG_SPINLOCK_SECTION
	volatile spinlock_t *lock;
#else
	volatile spinlock_t lock;
#endif
	uint32_t registered_consumer_mask;
	uint32_t sequence;
	uint32_t dropped_count;
	uint32_t busy_count;
	uint32_t max_pending_mask;
	uint32_t max_pending_count;
	bool free_sem_inited;
	uint32_t ready_event_inited_count;
	bool enabled;
};

static bool bk_frame_queue_is_power_of_two(uint32_t value)
{
	return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static uint32_t bk_frame_queue_consumer_bit(uint32_t consumer_id)
{
	return (uint32_t)(1UL << consumer_id);
}

uint32_t bk_frame_queue_calc_pool_size(uint32_t block_size, uint32_t block_count)
{
	uint32_t head_stride;
	uint32_t payload_stride;
	uint32_t block_stride;
	uint32_t required_size;

	if ((block_size == 0U) || (block_count == 0U)) {
		return 0;
	}

	head_stride = BK_FRAME_QUEUE_LEN_ALIGN(sizeof(bk_frame_queue_block_t));
	payload_stride = BK_FRAME_QUEUE_LEN_ALIGN(block_size);
	block_stride = head_stride + payload_stride;
	if ((payload_stride < block_size) || (block_stride < payload_stride)) {
		return 0;
	}

	if ((UINT32_MAX / block_stride) < block_count) {
		return 0;
	}

	required_size = block_stride * block_count;
	if ((UINT32_MAX - required_size) < (BK_FRAME_QUEUE_ALIGN_SIZE - 1U)) {
		return 0;
	}

	return required_size + BK_FRAME_QUEUE_ALIGN_SIZE - 1U;
}

static void bk_frame_queue_lock(bk_frame_queue_handle_t queue, uint32_t *flags)
{
#if CONFIG_SPINLOCK_SECTION
	spin_lock_irqsave(queue->lock, *flags);
#else
	spin_lock_irqsave(&queue->lock, *flags);
#endif
}

static void bk_frame_queue_unlock(bk_frame_queue_handle_t queue, uint32_t flags)
{
#if CONFIG_SPINLOCK_SECTION
	spin_unlock_irqrestore(queue->lock, flags);
#else
	spin_unlock_irqrestore(&queue->lock, flags);
#endif
}

static void bk_frame_queue_drain_sem(beken_semaphore_t *semaphore)
{
	while (rtos_get_semaphore(semaphore, BEKEN_NO_WAIT) == BK_OK) {
	}
}

static bk_frame_queue_block_t *bk_frame_queue_block_at(bk_frame_queue_handle_t queue, uint32_t index)
{
	return (bk_frame_queue_block_t *)(queue->pool + (index * queue->block_stride));
}

static bool bk_frame_queue_ring_push(bk_frame_queue_block_t **ring, uint32_t depth,
	uint32_t *tail, uint32_t *count, bk_frame_queue_block_t *block)
{
	if (*count >= depth) {
		return false;
	}

	ring[*tail] = block;
	*tail = (*tail + 1U) % depth;
	(*count)++;
	return true;
}

static bk_frame_queue_block_t *bk_frame_queue_ring_pop(bk_frame_queue_block_t **ring,
	uint32_t depth, uint32_t *head, uint32_t *count)
{
	bk_frame_queue_block_t *block;

	if (*count == 0U) {
		return NULL;
	}

	block = ring[*head];
	*head = (*head + 1U) % depth;
	(*count)--;
	return block;
}

static bool bk_frame_queue_free_push_locked(bk_frame_queue_handle_t queue,
	bk_frame_queue_block_t *block)
{
	return bk_frame_queue_ring_push(queue->free_ring, queue->config.block_count,
		&queue->free_tail, &queue->free_count, block);
}

static bk_frame_queue_block_t *bk_frame_queue_free_pop_locked(bk_frame_queue_handle_t queue)
{
	return bk_frame_queue_ring_pop(queue->free_ring, queue->config.block_count,
		&queue->free_head, &queue->free_count);
}

static bk_frame_queue_block_t **bk_frame_queue_ready_ring(bk_frame_queue_handle_t queue,
	uint32_t consumer_id)
{
	return &queue->ready_rings[consumer_id * queue->config.block_count];
}

static bool bk_frame_queue_ready_push_locked(bk_frame_queue_handle_t queue,
	uint32_t consumer_id, bk_frame_queue_block_t *block)
{
	return bk_frame_queue_ring_push(bk_frame_queue_ready_ring(queue, consumer_id),
		queue->config.block_count, &queue->ready_tails[consumer_id],
		&queue->ready_counts[consumer_id], block);
}

static bk_frame_queue_block_t *bk_frame_queue_ready_pop_locked(bk_frame_queue_handle_t queue,
	uint32_t consumer_id)
{
	return bk_frame_queue_ring_pop(bk_frame_queue_ready_ring(queue, consumer_id),
		queue->config.block_count, &queue->ready_heads[consumer_id],
		&queue->ready_counts[consumer_id]);
}

static bk_err_t bk_frame_queue_validate_config(const bk_frame_queue_config_t *config)
{
	if (!config) {
		return BK_ERR_NULL_PARAM;
	}

	if ((config->block_size == 0U) || (config->block_count == 0U)) {
		return BK_ERR_PARAM;
	}

	if ((config->consumer_count == 0U) ||
		(config->consumer_count > BK_FRAME_QUEUE_MAX_CONSUMERS)) {
		return BK_ERR_PARAM;
	}

	if (!bk_frame_queue_is_power_of_two(BK_FRAME_QUEUE_ALIGN_SIZE)) {
		return BK_ERR_PARAM;
	}

	return BK_OK;
}

static bk_err_t bk_frame_queue_prepare_pool(bk_frame_queue_handle_t queue)
{
	uintptr_t pool_addr;
	uintptr_t aligned_pool_addr;
	uint32_t align_offset;
	uint32_t required_size;

	queue->head_stride = BK_FRAME_QUEUE_LEN_ALIGN(sizeof(bk_frame_queue_block_t));
	queue->payload_stride = BK_FRAME_QUEUE_LEN_ALIGN(queue->config.block_size);
	queue->block_stride = queue->head_stride + queue->payload_stride;

	required_size = queue->block_stride * queue->config.block_count;
	if ((required_size == 0U) || ((required_size / queue->block_stride) != queue->config.block_count)) {
		return BK_ERR_PARAM;
	}

	if ((queue->payload_stride < queue->config.block_size) ||
		(queue->block_stride < queue->payload_stride)) {
		return BK_ERR_PARAM;
	}

	if (!queue->config.pool) {
		return BK_ERR_NULL_PARAM;
	}

	pool_addr = (uintptr_t)queue->config.pool;
	aligned_pool_addr = (uintptr_t)BK_FRAME_QUEUE_ADDR_ALIGN(pool_addr);
	align_offset = (uint32_t)(aligned_pool_addr - pool_addr);

	if (queue->config.pool_size < (required_size + align_offset)) {
		return BK_ERR_PARAM;
	}

	queue->pool = (uint8_t *)aligned_pool_addr;
	queue->pool_size = required_size;
	return BK_OK;
}

static void bk_frame_queue_cleanup(bk_frame_queue_handle_t queue)
{
	if (!queue) {
		return;
	}

	for (uint32_t i = 0; i < queue->ready_event_inited_count; i++) {
		(void)rtos_deinit_event_flags(&queue->ready_events[i]);
	}

	if (queue->free_sem_inited) {
		bk_frame_queue_drain_sem(&queue->free_sem);
		(void)rtos_deinit_semaphore(&queue->free_sem);
	}

	if (queue->ready_counts) {
		os_free(queue->ready_counts);
	}

	if (queue->ready_tails) {
		os_free(queue->ready_tails);
	}

	if (queue->ready_heads) {
		os_free(queue->ready_heads);
	}

	if (queue->ready_rings) {
		os_free(queue->ready_rings);
	}

	if (queue->ready_events) {
		os_free(queue->ready_events);
	}

	if (queue->free_ring) {
		os_free(queue->free_ring);
	}

#if CONFIG_SPINLOCK_SECTION
	if (queue->lock) {
		(void)spinlock_mem_dynamic_free((spinlock_t *)queue->lock);
	}
#endif

	os_free(queue);
}

static bk_err_t bk_frame_queue_init_queues(bk_frame_queue_handle_t queue)
{
	bk_err_t ret;
	uint32_t ring_size = sizeof(bk_frame_queue_block_t *) * queue->config.block_count;
	uint32_t ready_ring_size = ring_size * queue->config.consumer_count;

	queue->free_ring = (bk_frame_queue_block_t **)os_zalloc(ring_size);
	if (!queue->free_ring) {
		return BK_ERR_NO_MEM;
	}

	queue->ready_rings = (bk_frame_queue_block_t **)os_zalloc(ready_ring_size);
	if (!queue->ready_rings) {
		return BK_ERR_NO_MEM;
	}

	queue->ready_events = (beken_event_t *)os_zalloc(sizeof(beken_event_t) *
		queue->config.consumer_count);
	if (!queue->ready_events) {
		return BK_ERR_NO_MEM;
	}

	queue->ready_heads = (uint32_t *)os_zalloc(sizeof(uint32_t) * queue->config.consumer_count);
	queue->ready_tails = (uint32_t *)os_zalloc(sizeof(uint32_t) * queue->config.consumer_count);
	queue->ready_counts = (uint32_t *)os_zalloc(sizeof(uint32_t) * queue->config.consumer_count);
	if (!queue->ready_heads || !queue->ready_tails || !queue->ready_counts) {
		return BK_ERR_NO_MEM;
	}

	ret = rtos_init_semaphore_ex(&queue->free_sem, queue->config.block_count, 0);
	if (ret != BK_OK) {
		return ret;
	}
	queue->free_sem_inited = true;

	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		ret = rtos_init_event_flags(&queue->ready_events[i]);
		if (ret != BK_OK) {
			return ret;
		}
		queue->ready_event_inited_count++;
	}
	return BK_OK;
}

static bk_err_t bk_frame_queue_init_blocks(bk_frame_queue_handle_t queue)
{
	for (uint32_t i = 0; i < queue->config.block_count; i++) {
		bk_frame_queue_block_t *block =
			(bk_frame_queue_block_t *)(queue->pool + (i * queue->block_stride));

		os_memset(block, 0, sizeof(*block));
		block->magic = BK_FRAME_QUEUE_BLOCK_MAGIC;
		block->data = (uint8_t *)block + queue->head_stride;
		block->capacity = queue->payload_stride;
		block->index = (uint16_t)i;
		if (queue->config.user_data_cb) {
			block->user_data = queue->config.user_data_cb(i, block->data,
				queue->config.user_data_ctx);
		}
		block->state = BK_FRAME_QUEUE_BLOCK_FREE;
		if (!bk_frame_queue_free_push_locked(queue, block)) {
			return BK_ERR_BUSY;
		}
		if (rtos_set_semaphore(&queue->free_sem) != BK_OK) {
			return BK_FAIL;
		}
	}

	return BK_OK;
}

static bk_err_t bk_frame_queue_init_resources(bk_frame_queue_handle_t queue)
{
	bk_err_t ret;

	ret = bk_frame_queue_prepare_pool(queue);
	if (ret != BK_OK) {
		return ret;
	}

	ret = bk_frame_queue_init_queues(queue);
	if (ret != BK_OK) {
		return ret;
	}

	return bk_frame_queue_init_blocks(queue);
}

static bk_frame_queue_block_t *bk_frame_queue_buffer_to_block(bk_frame_queue_handle_t queue, void *buffer)
{
	uintptr_t addr;
	uintptr_t base;
	uintptr_t offset;
	uint32_t index;
	bk_frame_queue_block_t *block;

	if (!queue || !buffer) {
		return NULL;
	}

	addr = (uintptr_t)buffer;
	base = (uintptr_t)queue->pool;
	if ((addr < base) || (addr >= (base + queue->pool_size))) {
		return NULL;
	}

	offset = addr - base;
	if ((offset % queue->block_stride) != queue->head_stride) {
		return NULL;
	}

	index = offset / queue->block_stride;
	if (index >= queue->config.block_count) {
		return NULL;
	}

	block = (bk_frame_queue_block_t *)(queue->pool + (index * queue->block_stride));
	if (block->magic != BK_FRAME_QUEUE_BLOCK_MAGIC) {
		return NULL;
	}

	if (block->data != (uint8_t *)buffer) {
		return NULL;
	}

	return block;
}

static bk_err_t bk_frame_queue_push_free_block(bk_frame_queue_handle_t queue, bk_frame_queue_block_t *block)
{
	block->length = 0;
	block->pending_consumer_mask = 0;
	block->acquired_consumer_mask = 0;
	block->state = BK_FRAME_QUEUE_BLOCK_FREE;
	return bk_frame_queue_free_push_locked(queue, block) ? BK_OK : BK_ERR_BUSY;
}

static bool bk_frame_queue_block_recyclable(bk_frame_queue_block_t *block)
{
	return (block->pending_consumer_mask == 0U) &&
		(block->acquired_consumer_mask == 0U) &&
		(block->state != BK_FRAME_QUEUE_BLOCK_FREE) &&
		(block->state != BK_FRAME_QUEUE_BLOCK_WRITING);
}

static uint32_t bk_frame_queue_count_bits(uint32_t value)
{
	uint32_t count = 0;

	while (value != 0U) {
		value &= (value - 1U);
		count++;
	}

	return count;
}

static void bk_frame_queue_update_max_pending(bk_frame_queue_handle_t queue, uint32_t pending_mask)
{
	uint32_t pending_count = bk_frame_queue_count_bits(pending_mask);

	if (pending_count > queue->max_pending_count) {
		queue->max_pending_count = pending_count;
		queue->max_pending_mask = pending_mask;
	}
}

static uint32_t bk_frame_queue_drop_oldest_ready_locked(bk_frame_queue_handle_t queue);

static uint32_t bk_frame_queue_remaining_timeout(uint32_t start_ms, uint32_t timeout_ms)
{
	uint32_t now_ms;
	uint32_t elapsed_ms;

	if ((timeout_ms == BEKEN_WAIT_FOREVER) || (timeout_ms == BEKEN_NO_WAIT)) {
		return timeout_ms;
	}

	now_ms = rtos_get_time();
	elapsed_ms = (now_ms >= start_ms) ? (now_ms - start_ms) : timeout_ms;
	if (elapsed_ms >= timeout_ms) {
		return BEKEN_NO_WAIT;
	}

	return timeout_ms - elapsed_ms;
}

static bk_frame_queue_block_t *bk_frame_queue_pop_free_block(bk_frame_queue_handle_t queue)
{
	bk_frame_queue_block_t *block;
	uint32_t flags;

	bk_frame_queue_lock(queue, &flags);
	if (!queue->enabled) {
		bk_frame_queue_unlock(queue, flags);
		return NULL;
	}

	if (queue->free_count == 0U) {
		(void)bk_frame_queue_drop_oldest_ready_locked(queue);
	}

	block = bk_frame_queue_free_pop_locked(queue);
	if (block && (block->state == BK_FRAME_QUEUE_BLOCK_FREE)) {
		block->state = BK_FRAME_QUEUE_BLOCK_WRITING;
		bk_frame_queue_unlock(queue, flags);
		return block;
	}

	if (block) {
		(void)bk_frame_queue_push_free_block(queue, block);
	}
	bk_frame_queue_unlock(queue, flags);
	return NULL;
}

static bk_frame_queue_block_t *bk_frame_queue_wait_free_block(bk_frame_queue_handle_t queue,
	uint32_t timeout_ms)
{
	uint32_t start_ms = rtos_get_time();
	uint32_t wait_ms = timeout_ms;

	while (1) {
		bk_frame_queue_block_t *block = bk_frame_queue_pop_free_block(queue);

		if (block) {
			return block;
		}

		if (wait_ms == BEKEN_NO_WAIT) {
			return NULL;
		}

		if (rtos_get_semaphore(&queue->free_sem, wait_ms) != BK_OK) {
			return NULL;
		}

		wait_ms = bk_frame_queue_remaining_timeout(start_ms, timeout_ms);
	}

}

static bk_frame_queue_block_t *bk_frame_queue_pop_ready_block(bk_frame_queue_handle_t queue,
	uint32_t consumer_id)
{
	bk_frame_queue_block_t *block;
	uint32_t flags;

	bk_frame_queue_lock(queue, &flags);
	block = bk_frame_queue_ready_pop_locked(queue, consumer_id);
	if (queue->ready_counts[consumer_id] == 0U) {
		(void)rtos_clear_event_flags(&queue->ready_events[consumer_id],
			BK_FRAME_QUEUE_EVENT_DATA);
	}
	bk_frame_queue_unlock(queue, flags);
	return block;
}

static bk_frame_queue_block_t *bk_frame_queue_wait_ready_block(bk_frame_queue_handle_t queue,
	uint32_t consumer_id, uint32_t timeout_ms)
{
	uint32_t start_ms = rtos_get_time();
	uint32_t wait_ms = timeout_ms;

	while (1) {
		beken_event_flags_t events = rtos_wait_for_event_flags(&queue->ready_events[consumer_id],
			BK_FRAME_QUEUE_EVENT_DATA | BK_FRAME_QUEUE_EVENT_WAKE, false,
			WAIT_FOR_ANY_EVENT, wait_ms);

		if ((events & BK_FRAME_QUEUE_EVENT_WAKE) != 0U) {
			(void)rtos_clear_event_flags(&queue->ready_events[consumer_id],
				BK_FRAME_QUEUE_EVENT_WAKE);
			return NULL;
		}

		if ((events & BK_FRAME_QUEUE_EVENT_DATA) == 0U) {
			return NULL;
		}

		bk_frame_queue_block_t *block = bk_frame_queue_pop_ready_block(queue, consumer_id);

		if (block) {
			return block;
		}

		wait_ms = bk_frame_queue_remaining_timeout(start_ms, timeout_ms);
		if (wait_ms == BEKEN_NO_WAIT) {
			return NULL;
		}
	}

	return NULL;
}

static void bk_frame_queue_signal_free_blocks(bk_frame_queue_handle_t queue, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		(void)rtos_set_semaphore(&queue->free_sem);
	}
}

static uint32_t bk_frame_queue_drop_ready_for_consumer_locked(bk_frame_queue_handle_t queue,
	uint32_t consumer_id)
{
	uint32_t bit = bk_frame_queue_consumer_bit(consumer_id);
	bk_frame_queue_block_t *block;

	block = bk_frame_queue_ready_pop_locked(queue, consumer_id);
	if (!block) {
		return 0;
	}

	if (queue->ready_counts[consumer_id] == 0U) {
		(void)rtos_clear_event_flags(&queue->ready_events[consumer_id],
			BK_FRAME_QUEUE_EVENT_DATA);
	}

	block->pending_consumer_mask &= ~bit;
	block->acquired_consumer_mask &= ~bit;
	queue->dropped_count++;

	if (bk_frame_queue_block_recyclable(block)) {
		return (bk_frame_queue_push_free_block(queue, block) == BK_OK) ? 1U : 0U;
	}

	return 0;
}

static uint32_t bk_frame_queue_drop_oldest_ready_locked(bk_frame_queue_handle_t queue)
{
	uint32_t victim = BK_FRAME_QUEUE_MAX_CONSUMERS;
	uint32_t max_count = 0;

	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		if (((queue->registered_consumer_mask & bk_frame_queue_consumer_bit(i)) != 0U) &&
			(queue->ready_counts[i] > max_count)) {
			max_count = queue->ready_counts[i];
			victim = i;
		}
	}

	if (victim >= queue->config.consumer_count) {
		return 0;
	}

	return bk_frame_queue_drop_ready_for_consumer_locked(queue, victim);
}

static void bk_frame_queue_signal_ready_consumers(bk_frame_queue_handle_t queue, uint32_t mask)
{
	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		if ((mask & bk_frame_queue_consumer_bit(i)) != 0U) {
			rtos_set_event_flags(&queue->ready_events[i], BK_FRAME_QUEUE_EVENT_DATA);
		}
	}
}

static bool bk_frame_queue_ready_rings_have_room(bk_frame_queue_handle_t queue, uint32_t mask)
{
	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		if (((mask & bk_frame_queue_consumer_bit(i)) != 0U) &&
			(queue->ready_counts[i] >= queue->config.block_count)) {
			return false;
		}
	}

	return true;
}

static uint32_t bk_frame_queue_prepare_ready_rings_locked(bk_frame_queue_handle_t queue,
	uint32_t mask)
{
	uint32_t freed_count = 0;

	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		if ((mask & bk_frame_queue_consumer_bit(i)) == 0U) {
			continue;
		}

		if (queue->ready_counts[i] >= queue->config.block_count) {
			freed_count += bk_frame_queue_drop_ready_for_consumer_locked(queue, i);
		}
	}

	return freed_count;
}

static bk_err_t bk_frame_queue_push_ready_consumers(bk_frame_queue_handle_t queue,
	bk_frame_queue_block_t *block, uint32_t mask)
{
	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		if ((mask & bk_frame_queue_consumer_bit(i)) == 0U) {
			continue;
		}

		if (!bk_frame_queue_ready_push_locked(queue, i, block)) {
			return BK_ERR_BUSY;
		}
	}

	return BK_OK;
}

bk_err_t bk_frame_queue_create(const bk_frame_queue_config_t *config,
	bk_frame_queue_handle_t *queue)
{
	bk_frame_queue_handle_t new_queue;
	bk_err_t ret;

	if (!queue) {
		return BK_ERR_NULL_PARAM;
	}
	*queue = NULL;

	ret = bk_frame_queue_validate_config(config);
	if (ret != BK_OK) {
		return ret;
	}

	new_queue = (bk_frame_queue_handle_t)os_zalloc(sizeof(struct bk_frame_queue));
	if (!new_queue) {
		return BK_ERR_NO_MEM;
	}

#if CONFIG_SPINLOCK_SECTION
	new_queue->lock = spinlock_mem_dynamic_alloc();
	if (!new_queue->lock) {
		os_free(new_queue);
		return BK_ERR_NO_MEM;
	}
#else
	spinlock_init((spinlock_t *)&new_queue->lock);
#endif
	new_queue->config = *config;
	ret = bk_frame_queue_init_resources(new_queue);
	if (ret != BK_OK) {
		bk_frame_queue_cleanup(new_queue);
		return ret;
	}

	new_queue->enabled = true;
	*queue = new_queue;
	return BK_OK;
}

bk_err_t bk_frame_queue_destroy(bk_frame_queue_handle_t queue)
{
	uint32_t flags;

	if (!queue) {
		return BK_ERR_NULL_PARAM;
	}

	bk_frame_queue_lock(queue, &flags);
	for (uint32_t i = 0; i < queue->config.block_count; i++) {
		if (bk_frame_queue_block_at(queue, i)->state != BK_FRAME_QUEUE_BLOCK_FREE) {
			bk_frame_queue_unlock(queue, flags);
			return BK_ERR_BUSY;
		}
	}
	queue->enabled = false;
	bk_frame_queue_unlock(queue, flags);

	bk_frame_queue_cleanup(queue);
	return BK_OK;
}

bk_err_t bk_frame_queue_producer_acquire(bk_frame_queue_handle_t queue,
	void **buffer, void **user_data, uint32_t timeout_ms)
{
	bk_frame_queue_block_t *block = NULL;

	if (!queue || !buffer) {
		return BK_ERR_NULL_PARAM;
	}
	*buffer = NULL;
	if (user_data) {
		*user_data = NULL;
	}

	if (rtos_is_in_interrupt_context()) {
		block = bk_frame_queue_pop_free_block(queue);
	} else {
		block = bk_frame_queue_wait_free_block(queue, timeout_ms);
	}

	if (!block) {
		return BK_ERR_TIMEOUT;
	}

	*buffer = block->data;
	if (user_data) {
		*user_data = block->user_data;
	}
	return BK_OK;
}

bk_err_t bk_frame_queue_producer_commit(bk_frame_queue_handle_t queue,
	void *buffer, uint32_t length, void *user_data)
{
	bk_frame_queue_block_t *block;
	bk_err_t ret;
	bool signal_free = false;
	uint32_t signal_ready_mask = 0;
	uint32_t signal_free_count = 0;
	uint32_t pending_mask;
	uint32_t flags;

	block = bk_frame_queue_buffer_to_block(queue, buffer);
	if (!block) {
		return BK_ERR_PARAM;
	}

	bk_frame_queue_lock(queue, &flags);
	if (!queue->enabled || (block->state != BK_FRAME_QUEUE_BLOCK_WRITING)) {
		bk_frame_queue_unlock(queue, flags);
		return BK_ERR_STATE;
	}

	if (length > block->capacity) {
		bk_frame_queue_unlock(queue, flags);
		return BK_ERR_PARAM;
	}

	pending_mask = queue->registered_consumer_mask;
	if (pending_mask == 0U) {
		queue->dropped_count++;
		ret = bk_frame_queue_push_free_block(queue, block);
		signal_free = (ret == BK_OK);
		bk_frame_queue_unlock(queue, flags);
		if (signal_free) {
			(void)rtos_set_semaphore(&queue->free_sem);
		}
		return ret;
	}

	signal_free_count = bk_frame_queue_prepare_ready_rings_locked(queue, pending_mask);
	if (!bk_frame_queue_ready_rings_have_room(queue, pending_mask)) {
		queue->busy_count++;
		queue->dropped_count++;
		ret = bk_frame_queue_push_free_block(queue, block);
		if (ret == BK_OK) {
			signal_free_count++;
		}
		bk_frame_queue_unlock(queue, flags);
		bk_frame_queue_signal_free_blocks(queue, signal_free_count);
		return BK_ERR_BUSY;
	}

	block->length = length;
	block->timestamp = rtos_get_time();
	block->sequence = queue->sequence++;
	block->user_data = user_data;
	block->pending_consumer_mask = pending_mask;
	block->acquired_consumer_mask = 0;
	block->state = BK_FRAME_QUEUE_BLOCK_READY;
	ret = bk_frame_queue_push_ready_consumers(queue, block, pending_mask);
	if (ret == BK_OK) {
		signal_ready_mask = pending_mask;
		bk_frame_queue_update_max_pending(queue, pending_mask);
	}
	bk_frame_queue_unlock(queue, flags);
	bk_frame_queue_signal_free_blocks(queue, signal_free_count);
	if (signal_ready_mask != 0U) {
		bk_frame_queue_signal_ready_consumers(queue, signal_ready_mask);
	}
	return ret;
}

bk_err_t bk_frame_queue_producer_drop(bk_frame_queue_handle_t queue, void *buffer)
{
	bk_frame_queue_block_t *block = bk_frame_queue_buffer_to_block(queue, buffer);
	bk_err_t ret;
	bool signal_free;
	uint32_t flags;

	if (!block) {
		return BK_ERR_PARAM;
	}

	bk_frame_queue_lock(queue, &flags);
	if (block->state != BK_FRAME_QUEUE_BLOCK_WRITING) {
		bk_frame_queue_unlock(queue, flags);
		return BK_ERR_STATE;
	}

	queue->dropped_count++;
	ret = bk_frame_queue_push_free_block(queue, block);
	signal_free = (ret == BK_OK);
	bk_frame_queue_unlock(queue, flags);
	if (signal_free) {
		(void)rtos_set_semaphore(&queue->free_sem);
	}
	return ret;
}

bk_err_t bk_frame_queue_consumer_register(bk_frame_queue_handle_t queue,
	uint32_t *consumer_id)
{
	uint32_t flags;

	if (!queue || !consumer_id) {
		return BK_ERR_NULL_PARAM;
	}

	bk_frame_queue_lock(queue, &flags);
	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		uint32_t bit = bk_frame_queue_consumer_bit(i);

		if ((queue->registered_consumer_mask & bit) == 0U) {
			queue->registered_consumer_mask |= bit;
			*consumer_id = i;
			bk_frame_queue_unlock(queue, flags);
			return BK_OK;
		}
	}
	bk_frame_queue_unlock(queue, flags);
	return BK_ERR_NO_MEM;
}

static uint32_t bk_frame_queue_clear_consumer_from_blocks(bk_frame_queue_handle_t queue, uint32_t bit)
{
	uint32_t freed_count = 0;

	for (uint32_t i = 0; i < queue->config.block_count; i++) {
		bk_frame_queue_block_t *block = bk_frame_queue_block_at(queue, i);

		if ((block->pending_consumer_mask & bit) == 0U) {
			continue;
		}

		if ((block->acquired_consumer_mask & bit) != 0U) {
			continue;
		}

		block->pending_consumer_mask &= ~bit;
		block->acquired_consumer_mask &= ~bit;
		if (bk_frame_queue_block_recyclable(block)) {
			if (bk_frame_queue_push_free_block(queue, block) == BK_OK) {
				freed_count++;
			}
		}
	}

	return freed_count;
}

static void bk_frame_queue_clear_ready_ring_locked(bk_frame_queue_handle_t queue, uint32_t consumer_id)
{
	queue->ready_heads[consumer_id] = 0;
	queue->ready_tails[consumer_id] = 0;
	queue->ready_counts[consumer_id] = 0;
}

static uint32_t bk_frame_queue_recycle_ready_blocks(bk_frame_queue_handle_t queue)
{
	uint32_t freed_count = 0;

	for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
		bk_frame_queue_clear_ready_ring_locked(queue, i);
	}

	for (uint32_t i = 0; i < queue->config.block_count; i++) {
		bk_frame_queue_block_t *block = bk_frame_queue_block_at(queue, i);

		if (bk_frame_queue_block_recyclable(block)) {
			if (bk_frame_queue_push_free_block(queue, block) == BK_OK) {
				freed_count++;
			}
		}
	}

	return freed_count;
}

bk_err_t bk_frame_queue_consumer_unregister(bk_frame_queue_handle_t queue,
	uint32_t consumer_id)
{
	uint32_t bit;
	uint32_t flags;
	uint32_t freed_count;
	bool no_consumers;

	if (!queue || (consumer_id >= queue->config.consumer_count)) {
		return BK_ERR_PARAM;
	}

	bit = bk_frame_queue_consumer_bit(consumer_id);
	bk_frame_queue_lock(queue, &flags);
	if ((queue->registered_consumer_mask & bit) == 0U) {
		bk_frame_queue_unlock(queue, flags);
		return BK_ERR_NOT_FOUND;
	}

	queue->registered_consumer_mask &= ~bit;
	bk_frame_queue_clear_ready_ring_locked(queue, consumer_id);
	freed_count = bk_frame_queue_clear_consumer_from_blocks(queue, bit);
	if (queue->registered_consumer_mask == 0U) {
		freed_count += bk_frame_queue_recycle_ready_blocks(queue);
	}
	no_consumers = (queue->registered_consumer_mask == 0U);
	bk_frame_queue_unlock(queue, flags);
	rtos_set_event_flags(&queue->ready_events[consumer_id], BK_FRAME_QUEUE_EVENT_WAKE);
	if (no_consumers) {
		for (uint32_t i = 0; i < queue->config.consumer_count; i++) {
			(void)rtos_clear_event_flags(&queue->ready_events[i],
				BK_FRAME_QUEUE_EVENT_DATA);
		}
	}
	bk_frame_queue_signal_free_blocks(queue, freed_count);
	return BK_OK;
}

bk_err_t bk_frame_queue_consumer_acquire(bk_frame_queue_handle_t queue,
	uint32_t consumer_id, void **buffer, uint32_t *length, void **user_data,
	uint32_t timeout_ms)
{
	bk_frame_queue_block_t *block = NULL;
	uint32_t bit;
	uint32_t flags;

	if (!queue || !buffer || !length || (consumer_id >= queue->config.consumer_count)) {
		return BK_ERR_PARAM;
	}
	*buffer = NULL;
	*length = 0;
	if (user_data) {
		*user_data = NULL;
	}

	bit = bk_frame_queue_consumer_bit(consumer_id);
	if ((queue->registered_consumer_mask & bit) == 0U) {
		return BK_ERR_NOT_FOUND;
	}

	if (rtos_is_in_interrupt_context()) {
		block = bk_frame_queue_pop_ready_block(queue, consumer_id);
	} else {
		block = bk_frame_queue_wait_ready_block(queue, consumer_id, timeout_ms);
	}
	if (!block) {
		bool registered;

		bk_frame_queue_lock(queue, &flags);
		registered = ((queue->registered_consumer_mask & bit) != 0U);
		bk_frame_queue_unlock(queue, flags);
		return registered ? BK_ERR_TIMEOUT : BK_ERR_NOT_FOUND;
	}

	bk_frame_queue_lock(queue, &flags);
	if (((block->state != BK_FRAME_QUEUE_BLOCK_READY) &&
		(block->state != BK_FRAME_QUEUE_BLOCK_IN_USE)) ||
		((block->pending_consumer_mask & bit) == 0U) ||
		((block->acquired_consumer_mask & bit) != 0U)) {
		bk_frame_queue_unlock(queue, flags);
		return BK_ERR_STATE;
	}

	block->acquired_consumer_mask |= bit;
	block->pending_consumer_mask |= bit;
	block->state = BK_FRAME_QUEUE_BLOCK_IN_USE;
	bk_frame_queue_update_max_pending(queue, block->pending_consumer_mask);
	*buffer = block->data;
	*length = block->length;
	if (user_data) {
		*user_data = block->user_data;
	}
	bk_frame_queue_unlock(queue, flags);
	return BK_OK;
}

bk_err_t bk_frame_queue_consumer_release(bk_frame_queue_handle_t queue,
	uint32_t consumer_id, void *buffer)
{
	bk_frame_queue_block_t *block;
	uint32_t bit;
	bool recycle;
	bk_err_t ret = BK_OK;
	uint32_t flags;

	block = bk_frame_queue_buffer_to_block(queue, buffer);
	if (!block || (consumer_id >= queue->config.consumer_count)) {
		return BK_ERR_PARAM;
	}

	bit = bk_frame_queue_consumer_bit(consumer_id);
	bk_frame_queue_lock(queue, &flags);
	if (((block->acquired_consumer_mask & bit) == 0U) ||
		((block->pending_consumer_mask & bit) == 0U)) {
		bk_frame_queue_unlock(queue, flags);
		return BK_ERR_STATE;
	}

	block->acquired_consumer_mask &= ~bit;
	block->pending_consumer_mask &= ~bit;
	recycle = (block->pending_consumer_mask == 0U);
	if (recycle) {
		ret = bk_frame_queue_push_free_block(queue, block);
	}
	bk_frame_queue_unlock(queue, flags);
	if (recycle && (ret == BK_OK)) {
		(void)rtos_set_semaphore(&queue->free_sem);
	}
	return ret;
}

bk_err_t bk_frame_queue_get_stats(bk_frame_queue_handle_t queue,
	bk_frame_queue_stats_t *stats)
{
	uint32_t flags;

	if (!queue || !stats) {
		return BK_ERR_NULL_PARAM;
	}

	os_memset(stats, 0, sizeof(*stats));
	bk_frame_queue_lock(queue, &flags);
	stats->total_blocks = queue->config.block_count;
	stats->dropped_count = queue->dropped_count;
	stats->busy_count = queue->busy_count;
	stats->max_pending_mask = queue->max_pending_mask;
	stats->max_pending_count = queue->max_pending_count;
	for (uint32_t i = 0; i < queue->config.block_count; i++) {
		switch (bk_frame_queue_block_at(queue, i)->state) {
		case BK_FRAME_QUEUE_BLOCK_FREE:
			stats->free_blocks++;
			break;
		case BK_FRAME_QUEUE_BLOCK_READY:
			stats->ready_blocks++;
			break;
		case BK_FRAME_QUEUE_BLOCK_IN_USE:
			stats->in_use_blocks++;
			break;
		case BK_FRAME_QUEUE_BLOCK_WRITING:
			stats->writing_blocks++;
			break;
		default:
			break;
		}
	}
	bk_frame_queue_unlock(queue, flags);
	return BK_OK;
}

void bk_frame_queue_dump(bk_frame_queue_handle_t queue)
{
	if (!queue) {
		FRAME_QUEUE_LOGW("queue is NULL\n");
		return;
	}

	FRAME_QUEUE_LOGI("frame_queue: blocks=%u stride=%u pool=%p size=%u registered=0x%08x\n",
		queue->config.block_count, queue->block_stride, queue->pool, queue->pool_size,
		queue->registered_consumer_mask);
	for (uint32_t i = 0; i < queue->config.block_count; i++) {
		bk_frame_queue_block_t *block = bk_frame_queue_block_at(queue, i);

		FRAME_QUEUE_LOGI("block[%u]: state=%u len=%u seq=%u pending=0x%08x acquired=0x%08x\n",
			i, block->state, block->length, block->sequence,
			block->pending_consumer_mask, block->acquired_consumer_mask);
	}
	FRAME_QUEUE_LOGI("stats: dropped=%u busy=%u max_pending=0x%08x\n",
		queue->dropped_count, queue->busy_count, queue->max_pending_mask);
	FRAME_QUEUE_LOGI("stats: max_pending_count=%u\n", queue->max_pending_count);
}
