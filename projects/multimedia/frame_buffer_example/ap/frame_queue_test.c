#include <stdint.h>
#include <stdbool.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "bk_frame_queue.h"
#include "frame_queue_test.h"

#define TAG "frame_queue_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define FRAME_QUEUE_TEST_PAYLOAD_SIZE 128U
#define FRAME_QUEUE_TEST_MAX_BLOCKS   4U
#define FRAME_QUEUE_TEST_MAX_CONSUMERS 3U
#define FRAME_QUEUE_TEST_POOL_SIZE    2048U
#define FRAME_QUEUE_TEST_TIMEOUT_MS   1000U

typedef struct {
	uint32_t index;
	void *buffer;
} frame_queue_test_user_data_t;

typedef struct {
	bk_frame_queue_handle_t queue;
	beken_semaphore_t started_sem;
	beken_semaphore_t done_sem;
	void *buffer;
	void *user_data;
	bk_err_t ret;
} frame_queue_producer_wait_ctx_t;

typedef struct {
	bk_frame_queue_handle_t queue;
	uint32_t consumer_id;
	beken_semaphore_t started_sem;
	beken_semaphore_t done_sem;
	bk_err_t ret;
} frame_queue_consumer_wait_ctx_t;

static uint8_t s_frame_queue_test_pool[FRAME_QUEUE_TEST_POOL_SIZE]
	__attribute__((aligned(BK_FRAME_QUEUE_ALIGN_SIZE)));
static frame_queue_test_user_data_t s_frame_queue_test_user_data[FRAME_QUEUE_TEST_MAX_BLOCKS];

static bool frame_queue_check(bool condition, const char *message)
{
	if (!condition) {
		LOGE("[RESULT][FAIL] frame_queue_test %s\r\n", message);
		return false;
	}

	return true;
}

static void *frame_queue_user_data_init(uint32_t index, void *buffer, void *ctx)
{
	frame_queue_test_user_data_t *user_data = (frame_queue_test_user_data_t *)ctx;

	if ((index >= FRAME_QUEUE_TEST_MAX_BLOCKS) || (user_data == NULL)) {
		return NULL;
	}

	user_data[index].index = index;
	user_data[index].buffer = buffer;
	return &user_data[index];
}

static bool frame_queue_check_pattern(const uint8_t *buffer, uint32_t length, uint8_t pattern)
{
	if (!buffer) {
		return false;
	}

	for (uint32_t i = 0; i < length; i++) {
		if (buffer[i] != pattern) {
			return false;
		}
	}

	return true;
}

static bool frame_queue_create_test_queue(bk_frame_queue_handle_t *queue,
	uint32_t block_count, uint32_t consumer_count)
{
	uint32_t pool_size = bk_frame_queue_calc_pool_size(FRAME_QUEUE_TEST_PAYLOAD_SIZE,
		block_count);
	bk_frame_queue_config_t config = {
		.block_size = FRAME_QUEUE_TEST_PAYLOAD_SIZE,
		.block_count = block_count,
		.consumer_count = consumer_count,
		.pool = s_frame_queue_test_pool,
		.pool_size = pool_size,
		.user_data_cb = frame_queue_user_data_init,
		.user_data_ctx = s_frame_queue_test_user_data,
	};

	if (!frame_queue_check((queue != NULL) && (block_count <= FRAME_QUEUE_TEST_MAX_BLOCKS) &&
		(consumer_count <= FRAME_QUEUE_TEST_MAX_CONSUMERS), "create_args")) {
		return false;
	}

	if (!frame_queue_check((pool_size != 0U) && (pool_size <= sizeof(s_frame_queue_test_pool)),
		"pool_size")) {
		return false;
	}

	os_memset(s_frame_queue_test_pool, 0, sizeof(s_frame_queue_test_pool));
	os_memset(s_frame_queue_test_user_data, 0, sizeof(s_frame_queue_test_user_data));
	return frame_queue_check(bk_frame_queue_create(&config, queue) == BK_OK, "create");
}

static bool frame_queue_produce_frame(bk_frame_queue_handle_t queue, uint8_t pattern,
	uint32_t length, void **buffer)
{
	void *producer_buffer = NULL;
	void *user_data = NULL;
	bk_err_t ret;

	ret = bk_frame_queue_producer_acquire(queue, &producer_buffer, &user_data, BEKEN_NO_WAIT);
	if (!frame_queue_check(ret == BK_OK, "producer_acquire")) {
		return false;
	}

	os_memset(producer_buffer, pattern, length);
	ret = bk_frame_queue_producer_commit(queue, producer_buffer, length, user_data);
	if (!frame_queue_check(ret == BK_OK, "producer_commit")) {
		return false;
	}

	if (buffer) {
		*buffer = producer_buffer;
	}
	return true;
}

static bool frame_queue_consume_frame(bk_frame_queue_handle_t queue, uint32_t consumer,
	uint8_t pattern, uint32_t length, bool release, void **buffer)
{
	void *consumer_buffer = NULL;
	void *user_data = NULL;
	uint32_t acquired_length = 0;
	bk_err_t ret;

	ret = bk_frame_queue_consumer_acquire(queue, consumer, &consumer_buffer,
		&acquired_length, &user_data, BEKEN_NO_WAIT);
	if (!frame_queue_check(ret == BK_OK, "consumer_acquire")) {
		return false;
	}

	if (!frame_queue_check(acquired_length == length, "consumer_length") ||
		!frame_queue_check(frame_queue_check_pattern((uint8_t *)consumer_buffer, length, pattern),
		"consumer_pattern") ||
		!frame_queue_check(user_data != NULL, "consumer_user_data")) {
		return false;
	}

	if (release) {
		ret = bk_frame_queue_consumer_release(queue, consumer, consumer_buffer);
		if (!frame_queue_check(ret == BK_OK, "consumer_release")) {
			return false;
		}
	}

	if (buffer) {
		*buffer = consumer_buffer;
	}
	return true;
}

static bool frame_queue_test_multi_consumer_basic(void)
{
	bk_frame_queue_handle_t queue = NULL;
	uint32_t consumer0 = 0;
	uint32_t consumer1 = 0;
	void *producer_buffer = NULL;
	void *consumer0_buffer = NULL;
	void *consumer1_buffer = NULL;
	void *recycled_buffer = NULL;
	bool ok = true;

	LOGI("frame_queue_test multi_consumer_basic\r\n");
	ok = ok && frame_queue_create_test_queue(&queue, 1, 2);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer0) == BK_OK,
		"register_c0");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer1) == BK_OK,
		"register_c1");

	ok = ok && frame_queue_produce_frame(queue, 0x5a, FRAME_QUEUE_TEST_PAYLOAD_SIZE,
		&producer_buffer);
	ok = ok && frame_queue_consume_frame(queue, consumer0, 0x5a,
		FRAME_QUEUE_TEST_PAYLOAD_SIZE, false, &consumer0_buffer);
	ok = ok && frame_queue_consume_frame(queue, consumer1, 0x5a,
		FRAME_QUEUE_TEST_PAYLOAD_SIZE, false, &consumer1_buffer);
	ok = ok && frame_queue_check(consumer0_buffer == producer_buffer, "consumer0_buffer");
	ok = ok && frame_queue_check(consumer1_buffer == producer_buffer, "consumer1_buffer");

	ok = ok && frame_queue_check(bk_frame_queue_consumer_release(queue, consumer0,
		consumer0_buffer) == BK_OK, "release_c0");
	ok = ok && frame_queue_check(bk_frame_queue_producer_acquire(queue, &recycled_buffer,
		NULL, BEKEN_NO_WAIT) == BK_ERR_TIMEOUT,
		"not_recycled_before_all_release");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_release(queue, consumer1,
		consumer1_buffer) == BK_OK, "release_c1");

	recycled_buffer = NULL;
	ok = ok && frame_queue_check(bk_frame_queue_producer_acquire(queue, &recycled_buffer,
		NULL, BEKEN_NO_WAIT) == BK_OK, "recycled_acquire");
	ok = ok && frame_queue_check(recycled_buffer == producer_buffer, "recycled_buffer");
	if (recycled_buffer) {
		ok = ok && frame_queue_check(bk_frame_queue_producer_drop(queue, recycled_buffer) == BK_OK,
			"producer_drop");
	}

	ok = ok && frame_queue_check(bk_frame_queue_destroy(queue) == BK_OK, "destroy");
	return ok;
}

static void frame_queue_producer_wait_thread(void *arg)
{
	frame_queue_producer_wait_ctx_t *ctx = (frame_queue_producer_wait_ctx_t *)arg;

	(void)rtos_set_semaphore(&ctx->started_sem);
	ctx->ret = bk_frame_queue_producer_acquire(ctx->queue, &ctx->buffer,
		&ctx->user_data, FRAME_QUEUE_TEST_TIMEOUT_MS);
	if (ctx->ret == BK_OK) {
		(void)bk_frame_queue_producer_drop(ctx->queue, ctx->buffer);
	}
	(void)rtos_set_semaphore(&ctx->done_sem);
	rtos_delete_thread(NULL);
}

static void frame_queue_consumer_wait_thread(void *arg)
{
	frame_queue_consumer_wait_ctx_t *ctx = (frame_queue_consumer_wait_ctx_t *)arg;
	void *buffer = NULL;
	uint32_t length = 0;
	void *user_data = NULL;

	(void)rtos_set_semaphore(&ctx->started_sem);
	ctx->ret = bk_frame_queue_consumer_acquire(ctx->queue, ctx->consumer_id,
		&buffer, &length, &user_data, BEKEN_WAIT_FOREVER);
	(void)rtos_set_semaphore(&ctx->done_sem);
	rtos_delete_thread(NULL);
}

static bool frame_queue_test_blocking_release(void)
{
	bk_frame_queue_handle_t queue = NULL;
	uint32_t consumer0 = 0;
	uint32_t consumer1 = 0;
	void *held_buffer = NULL;
	frame_queue_producer_wait_ctx_t wait_ctx = {0};
	beken_thread_t thread = NULL;
	bool ok = true;

	LOGI("frame_queue_test blocking_release\r\n");
	ok = ok && frame_queue_create_test_queue(&queue, 1, 2);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer0) == BK_OK,
		"blocking_register_c0");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer1) == BK_OK,
		"blocking_register_c1");
	ok = ok && frame_queue_produce_frame(queue, 0xa5, 64U, NULL);
	ok = ok && frame_queue_consume_frame(queue, consumer0, 0xa5, 64U, true, NULL);
	ok = ok && frame_queue_consume_frame(queue, consumer1, 0xa5, 64U, false, &held_buffer);

	ok = ok && frame_queue_check(rtos_init_semaphore(&wait_ctx.started_sem, 1) == BK_OK,
		"started_sem");
	ok = ok && frame_queue_check(rtos_init_semaphore(&wait_ctx.done_sem, 1) == BK_OK,
		"done_sem");
	wait_ctx.queue = queue;
	wait_ctx.ret = BK_FAIL;
	ok = ok && frame_queue_check(rtos_create_thread(&thread, BEKEN_DEFAULT_WORKER_PRIORITY,
		"fq_prod_wait", frame_queue_producer_wait_thread, 2048, &wait_ctx) == BK_OK,
		"create_wait_thread");
	ok = ok && frame_queue_check(rtos_get_semaphore(&wait_ctx.started_sem,
		FRAME_QUEUE_TEST_TIMEOUT_MS) == BK_OK, "wait_thread_started");

	rtos_delay_milliseconds(100);
	ok = ok && frame_queue_check(wait_ctx.buffer == NULL, "producer_is_blocked");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_release(queue, consumer1,
		held_buffer) == BK_OK, "slow_release");
	ok = ok && frame_queue_check(rtos_get_semaphore(&wait_ctx.done_sem,
		FRAME_QUEUE_TEST_TIMEOUT_MS) == BK_OK, "wait_thread_done");
	ok = ok && frame_queue_check(wait_ctx.ret == BK_OK, "producer_unblocked");

	(void)rtos_deinit_semaphore(&wait_ctx.started_sem);
	(void)rtos_deinit_semaphore(&wait_ctx.done_sem);
	ok = ok && frame_queue_check(bk_frame_queue_destroy(queue) == BK_OK, "destroy_blocking");
	return ok;
}

static bool frame_queue_test_fast_slow_consumer_drop(void)
{
	bk_frame_queue_handle_t queue = NULL;
	uint32_t fast_consumer = 0;
	uint32_t slow_consumer = 0;
	void *slow_buffer = NULL;
	uint32_t length = 0;
	bk_frame_queue_stats_t stats = {0};
	bool ok = true;

	LOGI("frame_queue_test fast_slow_consumer_drop\r\n");
	ok = ok && frame_queue_create_test_queue(&queue, 2, 2);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue,
		&fast_consumer) == BK_OK, "drop_register_fast");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue,
		&slow_consumer) == BK_OK, "drop_register_slow");

	ok = ok && frame_queue_produce_frame(queue, 0x11, 32U, NULL);
	ok = ok && frame_queue_consume_frame(queue, fast_consumer, 0x11, 32U, true, NULL);
	ok = ok && frame_queue_produce_frame(queue, 0x22, 32U, NULL);
	ok = ok && frame_queue_consume_frame(queue, fast_consumer, 0x22, 32U, true, NULL);
	ok = ok && frame_queue_produce_frame(queue, 0x33, 32U, NULL);
	ok = ok && frame_queue_consume_frame(queue, fast_consumer, 0x33, 32U, true, NULL);

	ok = ok && frame_queue_consume_frame(queue, slow_consumer, 0x22, 32U, true,
		&slow_buffer);
	ok = ok && frame_queue_consume_frame(queue, slow_consumer, 0x33, 32U, true,
		&slow_buffer);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_acquire(queue, slow_consumer,
		&slow_buffer, &length, NULL, BEKEN_NO_WAIT) == BK_ERR_TIMEOUT,
		"slow_no_stale_frame");
	ok = ok && frame_queue_check(bk_frame_queue_get_stats(queue, &stats) == BK_OK,
		"drop_stats");
	ok = ok && frame_queue_check(stats.dropped_count >= 1U, "drop_count");
	ok = ok && frame_queue_check(stats.max_pending_count == 2U, "max_pending_count");

	ok = ok && frame_queue_check(bk_frame_queue_destroy(queue) == BK_OK, "destroy_drop");
	return ok;
}

static bool frame_queue_test_unregister_wakeup(void)
{
	bk_frame_queue_handle_t queue = NULL;
	uint32_t consumer = 0;
	frame_queue_consumer_wait_ctx_t wait_ctx = {0};
	beken_thread_t thread = NULL;
	bool ok = true;

	LOGI("frame_queue_test unregister_wakeup\r\n");
	ok = ok && frame_queue_create_test_queue(&queue, 1, 1);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer) == BK_OK,
		"wakeup_register");
	ok = ok && frame_queue_check(rtos_init_semaphore(&wait_ctx.started_sem, 1) == BK_OK,
		"wakeup_started_sem");
	ok = ok && frame_queue_check(rtos_init_semaphore(&wait_ctx.done_sem, 1) == BK_OK,
		"wakeup_done_sem");
	wait_ctx.queue = queue;
	wait_ctx.consumer_id = consumer;
	wait_ctx.ret = BK_FAIL;
	ok = ok && frame_queue_check(rtos_create_thread(&thread, BEKEN_DEFAULT_WORKER_PRIORITY,
		"fq_cons_wait", frame_queue_consumer_wait_thread, 2048, &wait_ctx) == BK_OK,
		"create_consumer_wait_thread");
	ok = ok && frame_queue_check(rtos_get_semaphore(&wait_ctx.started_sem,
		FRAME_QUEUE_TEST_TIMEOUT_MS) == BK_OK, "consumer_wait_started");

	rtos_delay_milliseconds(100);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_unregister(queue, consumer) == BK_OK,
		"wakeup_unregister");
	ok = ok && frame_queue_check(rtos_get_semaphore(&wait_ctx.done_sem,
		FRAME_QUEUE_TEST_TIMEOUT_MS) == BK_OK, "consumer_wait_done");
	ok = ok && frame_queue_check(wait_ctx.ret == BK_ERR_NOT_FOUND, "consumer_wait_not_found");

	(void)rtos_deinit_semaphore(&wait_ctx.started_sem);
	(void)rtos_deinit_semaphore(&wait_ctx.done_sem);
	ok = ok && frame_queue_check(bk_frame_queue_destroy(queue) == BK_OK, "destroy_wakeup");
	return ok;
}

static bool frame_queue_test_unregister_recycle_in_use(void)
{
	bk_frame_queue_handle_t queue = NULL;
	uint32_t consumer0 = 0;
	uint32_t consumer1 = 0;
	void *recycled_buffer = NULL;
	bool ok = true;

	LOGI("frame_queue_test unregister_recycle_in_use\r\n");
	ok = ok && frame_queue_create_test_queue(&queue, 1, 2);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer0) == BK_OK,
		"unregister_register_c0");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer1) == BK_OK,
		"unregister_register_c1");
	ok = ok && frame_queue_produce_frame(queue, 0x66, 32U, NULL);
	ok = ok && frame_queue_consume_frame(queue, consumer0, 0x66, 32U, true, NULL);

	ok = ok && frame_queue_check(bk_frame_queue_consumer_unregister(queue, consumer1) == BK_OK,
		"unregister_c1");
	ok = ok && frame_queue_check(bk_frame_queue_producer_acquire(queue, &recycled_buffer,
		NULL, BEKEN_NO_WAIT) == BK_OK, "unregister_recycled_acquire");
	ok = ok && frame_queue_check(bk_frame_queue_producer_drop(queue, recycled_buffer) == BK_OK,
		"unregister_recycled_drop");
	ok = ok && frame_queue_check(bk_frame_queue_destroy(queue) == BK_OK,
		"destroy_unregister_recycle");
	return ok;
}

static bool frame_queue_test_repeat_register_unregister(void)
{
	bk_frame_queue_handle_t queue = NULL;
	uint32_t consumer0 = 0;
	uint32_t consumer1 = 0;
	uint32_t consumer_reused = 0;
	void *buffer = NULL;
	bool ok = true;

	LOGI("frame_queue_test repeat_register_unregister\r\n");
	ok = ok && frame_queue_create_test_queue(&queue, 2, 2);
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer0) == BK_OK,
		"repeat_register_c0");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue, &consumer1) == BK_OK,
		"repeat_register_c1");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue,
		&consumer_reused) == BK_ERR_NO_MEM, "repeat_register_full");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_unregister(queue,
		consumer0) == BK_OK, "repeat_unregister_c0");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_register(queue,
		&consumer_reused) == BK_OK, "repeat_register_reused");
	ok = ok && frame_queue_check(consumer_reused == consumer0, "repeat_reused_id");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_unregister(queue,
		consumer1) == BK_OK, "repeat_unregister_c1");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_unregister(queue,
		consumer_reused) == BK_OK, "repeat_unregister_reused");
	ok = ok && frame_queue_check(bk_frame_queue_consumer_unregister(queue,
		consumer_reused) == BK_ERR_NOT_FOUND, "repeat_unregister_not_found");

	ok = ok && frame_queue_check(bk_frame_queue_producer_acquire(queue, &buffer,
		NULL, BEKEN_NO_WAIT) == BK_OK, "no_consumer_acquire");
	os_memset(buffer, 0x44, 16U);
	ok = ok && frame_queue_check(bk_frame_queue_producer_commit(queue, buffer,
		16U, NULL) == BK_OK, "no_consumer_commit");
	buffer = NULL;
	ok = ok && frame_queue_check(bk_frame_queue_producer_acquire(queue, &buffer,
		NULL, BEKEN_NO_WAIT) == BK_OK, "no_consumer_recycle");
	ok = ok && frame_queue_check(bk_frame_queue_producer_drop(queue, buffer) == BK_OK,
		"no_consumer_drop");

	ok = ok && frame_queue_check(bk_frame_queue_destroy(queue) == BK_OK, "destroy_repeat");
	return ok;
}

void frame_queue_test(void)
{
	LOGI("frame_queue_test start\r\n");

	if (frame_queue_test_multi_consumer_basic() &&
		frame_queue_test_blocking_release() &&
		frame_queue_test_fast_slow_consumer_drop() &&
		frame_queue_test_unregister_wakeup() &&
		frame_queue_test_unregister_recycle_in_use() &&
		frame_queue_test_repeat_register_unregister()) {
		LOGI("[RESULT][PASS] frame_queue_test success\r\n");
	} else {
		LOGE("[RESULT][FAIL] frame_queue_test failed\r\n");
	}
}
