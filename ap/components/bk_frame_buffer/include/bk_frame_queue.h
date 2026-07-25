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

#pragma once

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BK_FRAME_QUEUE_ALIGN_SIZE
#define BK_FRAME_QUEUE_ALIGN_SIZE 64U
#endif

#define BK_FRAME_QUEUE_ALIGN_UP(value) \
	(((value) + BK_FRAME_QUEUE_ALIGN_SIZE - 1U) & ~(BK_FRAME_QUEUE_ALIGN_SIZE - 1U))

#define BK_FRAME_QUEUE_ADDR_ALIGN(addr) \
	((void *)BK_FRAME_QUEUE_ALIGN_UP((uintptr_t)(addr)))

#define BK_FRAME_QUEUE_LEN_ALIGN(length) \
	BK_FRAME_QUEUE_ALIGN_UP(length)

typedef void *(*bk_frame_queue_user_data_cb_t)(uint32_t index, void *buffer, void *ctx);

typedef struct
{
	uint32_t block_size;
	uint32_t block_count;
	uint32_t consumer_count;
	/* External pool. Size should be from bk_frame_queue_calc_pool_size(). */
	void *pool;
	uint32_t pool_size;
	bk_frame_queue_user_data_cb_t user_data_cb;
	void *user_data_ctx;
} bk_frame_queue_config_t;

typedef struct
{
	uint32_t total_blocks;
	uint32_t free_blocks;
	uint32_t ready_blocks;
	uint32_t in_use_blocks;
	uint32_t writing_blocks;
	uint32_t dropped_count;
	uint32_t busy_count;
	uint32_t max_pending_mask;
	uint32_t max_pending_count;
} bk_frame_queue_stats_t;

typedef struct bk_frame_queue *bk_frame_queue_handle_t;

uint32_t bk_frame_queue_calc_pool_size(uint32_t block_size, uint32_t block_count);
bk_err_t bk_frame_queue_create(const bk_frame_queue_config_t *config,
	bk_frame_queue_handle_t *queue);
/* Destroy requires all producer/consumer users stopped and all blocks released. */
bk_err_t bk_frame_queue_destroy(bk_frame_queue_handle_t queue);

/*
 * Producer APIs are task/ISR safe. In ISR context timeout_ms is treated as no-wait.
 * No free block returns BK_ERR_TIMEOUT.
 */
bk_err_t bk_frame_queue_producer_acquire(bk_frame_queue_handle_t queue,
	void **buffer, void **user_data, uint32_t timeout_ms);
/*
 * On BK_OK the frame is visible to all registered consumers.
 * On BK_ERR_BUSY the input buffer has been recycled by the queue and must not be
 * passed to bk_frame_queue_producer_drop().
 */
bk_err_t bk_frame_queue_producer_commit(bk_frame_queue_handle_t queue,
	void *buffer, uint32_t length, void *user_data);
bk_err_t bk_frame_queue_producer_drop(bk_frame_queue_handle_t queue, void *buffer);
bk_err_t bk_frame_queue_consumer_register(bk_frame_queue_handle_t queue,
	uint32_t *consumer_id);
/* Unregister wakes a consumer blocked in acquire; that acquire returns BK_ERR_NOT_FOUND. */
bk_err_t bk_frame_queue_consumer_unregister(bk_frame_queue_handle_t queue,
	uint32_t consumer_id);
/* Consumer APIs are task/ISR safe. In ISR context timeout_ms is treated as no-wait. */
bk_err_t bk_frame_queue_consumer_acquire(bk_frame_queue_handle_t queue,
	uint32_t consumer_id, void **buffer, uint32_t *length, void **user_data,
	uint32_t timeout_ms);
bk_err_t bk_frame_queue_consumer_release(bk_frame_queue_handle_t queue,
	uint32_t consumer_id, void *buffer);
bk_err_t bk_frame_queue_get_stats(bk_frame_queue_handle_t queue,
	bk_frame_queue_stats_t *stats);
void bk_frame_queue_dump(bk_frame_queue_handle_t queue);

#ifdef __cplusplus
}
#endif
