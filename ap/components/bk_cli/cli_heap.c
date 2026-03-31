/**
 * Heap related CLI commands.
 *
 * Notes:
 * - Logs are in English (CLI/test output).
 * - Comments are in English by design.
 */

#include <stdlib.h>

#include "cli.h"

#include <string.h>
#include <common/bk_err.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>

/* =========================================================================
 * mem_mt_test implementation (moved from bk_mem_mt_test.c)
 * ========================================================================= */

#define MEM_MT_LOG_TAG "mem_mt"

typedef void *(*bk_mem_alloc_fn_t)(void *ctx, size_t size);
typedef void (*bk_mem_free_fn_t)(void *ctx, void *ptr);

typedef struct {
	const char *name;
	void *ctx;
	bk_mem_alloc_fn_t alloc;
	bk_mem_free_fn_t free;
} bk_mem_pool_ops_t;

typedef enum {
	BK_MEM_PATTERN_NONE = 0,
	BK_MEM_PATTERN_HEAD_TAIL = 1,
	BK_MEM_PATTERN_FULL = 2,
} bk_mem_pattern_mode_t;

typedef struct {
	uint32_t task_cnt;
	uint32_t iterations;
	uint32_t min_size;
	uint32_t max_size;
	uint32_t local_slots;
	uint32_t shared_slots;
	uint32_t delay_ms;
	uint32_t seed;
	bk_mem_pattern_mode_t pattern_mode;
} bk_mem_mt_test_cfg_t;

typedef struct {
	void *ptr;
	uint32_t size;
	uint32_t cookie;
	uint8_t pool_idx;
} mem_mt_block_t;

typedef struct {
	/* Control */
	volatile uint32_t abort;

	/* Sync */
	beken_semaphore_t ready_sem;
	beken_semaphore_t start_sem;
	beken_semaphore_t done_sem;

	/* Shared blocks */
	beken_mutex_t shared_mutex;
	mem_mt_block_t *shared;
	uint32_t shared_cap;
	uint32_t shared_used;

	/* Pools */
	const bk_mem_pool_ops_t *pools;
	uint32_t pool_cnt;

	/* Config */
	bk_mem_mt_test_cfg_t cfg;

	/* Stats */
	uint32_t alloc_ok;
	uint32_t alloc_fail;
	uint32_t free_ok;
	uint32_t shared_put;
	uint32_t shared_get;
	uint32_t corrupt;
} mem_mt_ctx_t;

typedef struct {
	mem_mt_ctx_t *ctx;
	uint32_t task_id;
	uint32_t seed;
} mem_mt_task_arg_t;

static inline uint32_t mem_mt_rand_u32(uint32_t *seed)
{
	/* LCG */
	*seed = (*seed * 1103515245u) + 12345u;
	return *seed;
}

static inline uint32_t mem_mt_rand_range(uint32_t *seed, uint32_t min_v, uint32_t max_v)
{
	if (min_v >= max_v) {
		return min_v;
	}
	return min_v + (mem_mt_rand_u32(seed) % (max_v - min_v + 1u));
}

static inline uint8_t mem_mt_pattern_byte(uint32_t cookie, uint32_t size, uint32_t pos)
{
	uint32_t x = cookie ^ (size * 2654435761u) ^ (pos * 33u);
	x ^= (x >> 16);
	return (uint8_t)(x & 0xFFu);
}

static void mem_mt_fill_pattern(void *ptr, uint32_t size, uint32_t cookie, bk_mem_pattern_mode_t mode)
{
	if (mode == BK_MEM_PATTERN_NONE || ptr == NULL || size == 0) {
		return;
	}

	uint8_t *p = (uint8_t *)ptr;
	if (mode == BK_MEM_PATTERN_FULL || size <= 128u) {
		for (uint32_t i = 0; i < size; i++) {
			p[i] = mem_mt_pattern_byte(cookie, size, i);
		}
		return;
	}

	/* Head/tail mode */
	uint32_t head = (size < 64u) ? size : 64u;
	for (uint32_t i = 0; i < head; i++) {
		p[i] = mem_mt_pattern_byte(cookie, size, i);
	}
	for (uint32_t i = 0; i < head; i++) {
		uint32_t pos = (size - head) + i;
		p[pos] = mem_mt_pattern_byte(cookie, size, pos);
	}
}

static bk_err_t mem_mt_check_pattern(void *ptr, uint32_t size, uint32_t cookie, bk_mem_pattern_mode_t mode)
{
	if (mode == BK_MEM_PATTERN_NONE || ptr == NULL || size == 0) {
		return kNoErr;
	}

	uint8_t *p = (uint8_t *)ptr;
	if (mode == BK_MEM_PATTERN_FULL || size <= 128u) {
		for (uint32_t i = 0; i < size; i++) {
			if (p[i] != mem_mt_pattern_byte(cookie, size, i)) {
				return kGeneralErr;
			}
		}
		return kNoErr;
	}

	uint32_t head = (size < 64u) ? size : 64u;
	for (uint32_t i = 0; i < head; i++) {
		if (p[i] != mem_mt_pattern_byte(cookie, size, i)) {
			return kGeneralErr;
		}
	}
	for (uint32_t i = 0; i < head; i++) {
		uint32_t pos = (size - head) + i;
		if (p[pos] != mem_mt_pattern_byte(cookie, size, pos)) {
			return kGeneralErr;
		}
	}
	return kNoErr;
}

static void mem_mt_stats_inc(volatile uint32_t *v)
{
	uint32_t flags = rtos_enter_critical();
	(*v)++;
	rtos_exit_critical(flags);
}

static void mem_mt_abort(mem_mt_ctx_t *ctx)
{
	uint32_t flags = rtos_enter_critical();
	ctx->abort = 1;
	rtos_exit_critical(flags);
}

static int mem_mt_shared_put(mem_mt_ctx_t *ctx, const mem_mt_block_t *b)
{
	if (ctx->shared_cap == 0 || ctx->shared == NULL) {
		return -1;
	}

	rtos_lock_mutex(&ctx->shared_mutex);
	for (uint32_t i = 0; i < ctx->shared_cap; i++) {
		if (ctx->shared[i].ptr == NULL) {
			ctx->shared[i] = *b;
			ctx->shared_used++;
			rtos_unlock_mutex(&ctx->shared_mutex);
			mem_mt_stats_inc(&ctx->shared_put);
			return 0;
		}
	}
	rtos_unlock_mutex(&ctx->shared_mutex);
	return -1;
}

static int mem_mt_shared_take_any(mem_mt_ctx_t *ctx, uint32_t *seed, mem_mt_block_t *out)
{
	if (ctx->shared_cap == 0 || ctx->shared == NULL || out == NULL) {
		return -1;
	}

	rtos_lock_mutex(&ctx->shared_mutex);
	if (ctx->shared_used == 0) {
		rtos_unlock_mutex(&ctx->shared_mutex);
		return -1;
	}

	/* Try a few random indices first */
	for (uint32_t t = 0; t < 8; t++) {
		uint32_t idx = mem_mt_rand_range(seed, 0, ctx->shared_cap - 1u);
		if (ctx->shared[idx].ptr != NULL) {
			*out = ctx->shared[idx];
			memset(&ctx->shared[idx], 0, sizeof(ctx->shared[idx]));
			ctx->shared_used--;
			rtos_unlock_mutex(&ctx->shared_mutex);
			mem_mt_stats_inc(&ctx->shared_get);
			return 0;
		}
	}

	/* Fallback linear scan */
	for (uint32_t i = 0; i < ctx->shared_cap; i++) {
		if (ctx->shared[i].ptr != NULL) {
			*out = ctx->shared[i];
			memset(&ctx->shared[i], 0, sizeof(ctx->shared[i]));
			ctx->shared_used--;
			rtos_unlock_mutex(&ctx->shared_mutex);
			mem_mt_stats_inc(&ctx->shared_get);
			return 0;
		}
	}

	rtos_unlock_mutex(&ctx->shared_mutex);
	return -1;
}

static void mem_mt_free_block(mem_mt_ctx_t *ctx, const mem_mt_block_t *b)
{
	if (b->ptr == NULL) {
		return;
	}
	if (b->pool_idx >= ctx->pool_cnt) {
		BK_LOGE(MEM_MT_LOG_TAG, "Invalid pool idx %u\r\n", b->pool_idx);
		mem_mt_abort(ctx);
		return;
	}

	if (mem_mt_check_pattern(b->ptr, b->size, b->cookie, ctx->cfg.pattern_mode) != kNoErr) {
		mem_mt_stats_inc(&ctx->corrupt);
		BK_LOGE(MEM_MT_LOG_TAG,
		        "Pattern mismatch: ptr=%p size=%u cookie=0x%08X pool=%s\r\n",
		        b->ptr, b->size, b->cookie, ctx->pools[b->pool_idx].name ? ctx->pools[b->pool_idx].name : "unknown");
		mem_mt_abort(ctx);
	}

	ctx->pools[b->pool_idx].free(ctx->pools[b->pool_idx].ctx, b->ptr);
	mem_mt_stats_inc(&ctx->free_ok);
}

static void mem_mt_task_main(void *arg)
{
	mem_mt_task_arg_t *a = (mem_mt_task_arg_t *)arg;
	mem_mt_ctx_t *ctx = a->ctx;
	uint32_t task_id = a->task_id;
	uint32_t seed = a->seed;

	uint32_t local_cap = ctx->cfg.local_slots;
	mem_mt_block_t *local = NULL;
	uint32_t local_used = 0;

	if (local_cap > 0) {
		local = (mem_mt_block_t *)os_zalloc(sizeof(mem_mt_block_t) * local_cap);
		if (local == NULL) {
			BK_LOGE(MEM_MT_LOG_TAG, "Task%u: local meta alloc failed\r\n", task_id);
			mem_mt_abort(ctx);
		}
	}

	/* Signal ready and wait start */
	rtos_set_semaphore(&ctx->ready_sem);
	rtos_get_semaphore(&ctx->start_sem, BEKEN_WAIT_FOREVER);

	for (uint32_t i = 0; i < ctx->cfg.iterations && !ctx->abort; i++) {
		uint32_t r = mem_mt_rand_u32(&seed);
		uint32_t do_alloc = (r & 1u);

		/* Bias towards freeing when local is full */
		if (local_cap > 0 && local_used == local_cap) {
			do_alloc = 0;
		}

		if (do_alloc) {
			if (ctx->pool_cnt == 0) {
				mem_mt_abort(ctx);
				break;
			}

			uint32_t pool_idx = mem_mt_rand_range(&seed, 0, ctx->pool_cnt - 1u);
			uint32_t sz = mem_mt_rand_range(&seed, ctx->cfg.min_size, ctx->cfg.max_size);

			/* Mix a few corner sizes */
			if ((r & 0x3Fu) == 0u) sz = 0;
			if ((r & 0x3Fu) == 1u) sz = 1;
			if ((r & 0x3Fu) == 2u) sz = 4;
			if ((r & 0x3Fu) == 3u) sz = 8;

			void *p = ctx->pools[pool_idx].alloc(ctx->pools[pool_idx].ctx, (size_t)sz);
			if (p == NULL) {
				mem_mt_stats_inc(&ctx->alloc_fail);
			} else {
				mem_mt_stats_inc(&ctx->alloc_ok);
				uint32_t cookie = (0xA5A50000u ^ (task_id << 8) ^ i ^ sz ^ pool_idx);
				mem_mt_fill_pattern(p, sz, cookie, ctx->cfg.pattern_mode);

				mem_mt_block_t b = {
					.ptr = p,
					.size = sz,
					.cookie = cookie,
					.pool_idx = (uint8_t)pool_idx,
				};

				/* Half chance to publish into shared table */
				if (ctx->cfg.shared_slots > 0 && (r & 2u)) {
					if (mem_mt_shared_put(ctx, &b) != 0) {
						/* Fallback to local */
						if (local && local_used < local_cap) {
							for (uint32_t s = 0; s < local_cap; s++) {
								if (local[s].ptr == NULL) {
									local[s] = b;
									local_used++;
									break;
								}
							}
						} else {
							mem_mt_free_block(ctx, &b);
						}
					}
				} else {
					if (local && local_used < local_cap) {
						for (uint32_t s = 0; s < local_cap; s++) {
							if (local[s].ptr == NULL) {
								local[s] = b;
								local_used++;
								break;
							}
						}
					} else {
						/* No local slots, free immediately */
						mem_mt_free_block(ctx, &b);
					}
				}
			}
		} else {
			/* Free path: prefer shared to stress cross-thread free */
			mem_mt_block_t b = {0};
			int got = -1;

			if (ctx->cfg.shared_slots > 0 && (r & 3u) != 0u) {
				got = mem_mt_shared_take_any(ctx, &seed, &b);
			}
			if (got != 0 && local && local_used > 0) {
				/* Take from local */
				uint32_t idx = mem_mt_rand_range(&seed, 0, local_cap - 1u);
				for (uint32_t t = 0; t < local_cap; t++) {
					uint32_t k = (idx + t) % local_cap;
					if (local[k].ptr != NULL) {
						b = local[k];
						memset(&local[k], 0, sizeof(local[k]));
						local_used--;
						got = 0;
						break;
					}
				}
			}

			if (got == 0) {
				mem_mt_free_block(ctx, &b);
			}
		}

		if (ctx->cfg.delay_ms) {
			rtos_delay_milliseconds(ctx->cfg.delay_ms);
		}
	}

	/* Cleanup: free local leftovers */
	if (local) {
		for (uint32_t i = 0; i < local_cap; i++) {
			if (local[i].ptr != NULL) {
				mem_mt_free_block(ctx, &local[i]);
			}
		}
		os_free(local);
	}

	/* Signal done */
	rtos_set_semaphore(&ctx->done_sem);

	/* Free arg and exit */
	os_free(a);
	rtos_delete_thread(NULL);
}

int bk_mem_mt_test_run(const bk_mem_pool_ops_t *pools, uint32_t pool_cnt, const bk_mem_mt_test_cfg_t *cfg)
{
	if (pools == NULL || cfg == NULL || pool_cnt == 0) {
		BK_LOGE(MEM_MT_LOG_TAG, "Invalid args\r\n");
		return -1;
	}
	if (cfg->task_cnt == 0 || cfg->iterations == 0) {
		BK_LOGE(MEM_MT_LOG_TAG, "Invalid cfg: task_cnt/iterations\r\n");
		return -2;
	}
	if (cfg->min_size > cfg->max_size) {
		BK_LOGE(MEM_MT_LOG_TAG, "Invalid cfg: min_size > max_size\r\n");
		return -3;
	}

	mem_mt_ctx_t *ctx = (mem_mt_ctx_t *)os_zalloc(sizeof(mem_mt_ctx_t));
	if (ctx == NULL) {
		BK_LOGE(MEM_MT_LOG_TAG, "Ctx alloc failed\r\n");
		return -4;
	}

	ctx->pools = pools;
	ctx->pool_cnt = pool_cnt;
	ctx->cfg = *cfg;

	/* Shared table */
	if (cfg->shared_slots > 0) {
		ctx->shared = (mem_mt_block_t *)os_zalloc(sizeof(mem_mt_block_t) * cfg->shared_slots);
		if (ctx->shared == NULL) {
			BK_LOGE(MEM_MT_LOG_TAG, "Shared table alloc failed\r\n");
			os_free(ctx);
			return -5;
		}
		ctx->shared_cap = cfg->shared_slots;
		rtos_init_mutex(&ctx->shared_mutex);
	}

	/* Semaphores */
	rtos_init_semaphore_ex(&ctx->ready_sem, (int)cfg->task_cnt, 0);
	rtos_init_semaphore_ex(&ctx->start_sem, (int)cfg->task_cnt, 0);
	rtos_init_semaphore_ex(&ctx->done_sem, (int)cfg->task_cnt, 0);

	BK_LOGI(MEM_MT_LOG_TAG,
	        "Start: tasks=%u iter=%u size=[%u,%u] local_slots=%u shared_slots=%u delay_ms=%u seed=0x%08X pools=%u pattern=%u\r\n",
	        cfg->task_cnt, cfg->iterations, cfg->min_size, cfg->max_size,
	        cfg->local_slots, cfg->shared_slots, cfg->delay_ms, cfg->seed, pool_cnt, (uint32_t)cfg->pattern_mode);

	/* Spawn tasks */
	for (uint32_t t = 0; t < cfg->task_cnt; t++) {
		mem_mt_task_arg_t *a = (mem_mt_task_arg_t *)os_zalloc(sizeof(mem_mt_task_arg_t));
		if (a == NULL) {
			BK_LOGE(MEM_MT_LOG_TAG, "Arg alloc failed for task%u\r\n", t);
			mem_mt_abort(ctx);
			break;
		}
		a->ctx = ctx;
		a->task_id = t;
		a->seed = (cfg->seed ^ (t * 0x9E3779B9u) ^ 0x1234u);

		beken_thread_t th = NULL;
		bk_err_t ret = rtos_create_thread(&th,
		                                 BEKEN_DEFAULT_WORKER_PRIORITY,
		                                 "mem_mt",
		                                 (beken_thread_function_t)mem_mt_task_main,
		                                 4096,
		                                 a);
		if (ret != kNoErr) {
			BK_LOGE(MEM_MT_LOG_TAG, "Create task%u failed: %d\r\n", t, ret);
			os_free(a);
			mem_mt_abort(ctx);
			break;
		}
	}

	/* Wait all ready (or abort) */
	{
		uint32_t ready_cnt = 0;
		beken_time_t last_log_ms = 0;
		(void)beken_time_get_time(&last_log_ms);

		while (ready_cnt < cfg->task_cnt && !ctx->abort) {
			bk_err_t r = rtos_get_semaphore(&ctx->ready_sem, 1000);
			if (r == kNoErr) {
				ready_cnt++;
				continue;
			}

			beken_time_t now_ms = 0;
			(void)beken_time_get_time(&now_ms);
			if (now_ms - last_log_ms >= 1000) {
				BK_LOGI(MEM_MT_LOG_TAG,
				        "Progress: ready=%u/%u alloc_ok=%u alloc_fail=%u free_ok=%u corrupt=%u abort=%u\r\n",
				        ready_cnt, cfg->task_cnt, ctx->alloc_ok, ctx->alloc_fail, ctx->free_ok, ctx->corrupt, ctx->abort);
				last_log_ms = now_ms;
			}
		}
	}

	/* Release start barrier */
	for (uint32_t t = 0; t < cfg->task_cnt; t++) {
		rtos_set_semaphore(&ctx->start_sem);
	}

	/* Wait all done */
	{
		uint32_t done_cnt = 0;
		beken_time_t last_log_ms = 0;
		(void)beken_time_get_time(&last_log_ms);

		while (done_cnt < cfg->task_cnt) {
			bk_err_t r = rtos_get_semaphore(&ctx->done_sem, 1000);
			if (r == kNoErr) {
				done_cnt++;
				continue;
			}

			beken_time_t now_ms = 0;
			(void)beken_time_get_time(&now_ms);
			if (now_ms - last_log_ms >= 1000) {
				BK_LOGI(MEM_MT_LOG_TAG,
				        "Progress: done=%u/%u alloc_ok=%u alloc_fail=%u free_ok=%u shared_put=%u shared_get=%u corrupt=%u abort=%u\r\n",
				        done_cnt, cfg->task_cnt, ctx->alloc_ok, ctx->alloc_fail, ctx->free_ok,
				        ctx->shared_put, ctx->shared_get, ctx->corrupt, ctx->abort);
				last_log_ms = now_ms;
			}
		}
	}

	/* Drain shared leftovers (if any) */
	if (ctx->shared) {
		for (uint32_t i = 0; i < ctx->shared_cap; i++) {
			if (ctx->shared[i].ptr != NULL) {
				mem_mt_free_block(ctx, &ctx->shared[i]);
			}
		}
	}

	BK_LOGI(MEM_MT_LOG_TAG,
	        "Result: abort=%u alloc_ok=%u alloc_fail=%u free_ok=%u shared_put=%u shared_get=%u corrupt=%u\r\n",
	        ctx->abort, ctx->alloc_ok, ctx->alloc_fail, ctx->free_ok, ctx->shared_put, ctx->shared_get, ctx->corrupt);

	/* Cleanup */
	rtos_deinit_semaphore(&ctx->ready_sem);
	rtos_deinit_semaphore(&ctx->start_sem);
	rtos_deinit_semaphore(&ctx->done_sem);

	if (ctx->shared) {
		rtos_deinit_mutex(&ctx->shared_mutex);
		os_free(ctx->shared);
	}
	int fail = (ctx->abort || ctx->corrupt) ? -10 : 0;
	os_free(ctx);
	return fail;
}

/* =========================================================================
 * CLI glue
 * ========================================================================= */

static void *mem_mt_os_alloc(void *ctx, size_t size)
{
	(void)ctx;
	return os_malloc(size);
}

static void mem_mt_os_free(void *ctx, void *ptr)
{
	(void)ctx;
	os_free(ptr);
}

#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
static void *mem_mt_psram_alloc(void *ctx, size_t size)
{
	(void)ctx;
	return psram_malloc(size);
}

static void mem_mt_psram_free(void *ctx, void *ptr)
{
	(void)ctx;
	/* Keep consistent with existing heap_test behavior */
	os_free(ptr);
}
#endif

#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
static void *mem_mt_hsram_alloc(void *ctx, size_t size)
{
	(void)ctx;
	return hsram_malloc(size);
}

static void mem_mt_hsram_free(void *ctx, void *ptr)
{
	(void)ctx;
	os_free(ptr);
}
#endif

static void cli_mem_mt_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	/* Args with defaults (only iterations is required):
	 * mem_mt_test <iterations> [task_cnt] [max_size] [pool_mask] [delay_ms]
	 *
	 * Defaults:
	 * - task_cnt=8 min_size=0 max_size=4096
	 * - local_slots=64 shared_slots=256 pool_mask=0x1(os)
	 * - pattern=1(head/tail) delay_ms=1 seed=now(ms)
	 *
	 * pool_mask bit0: os_malloc/os_free
	 * pool_mask bit1: psram_malloc/os_free (if enabled)
	 * pool_mask bit2: hsram_malloc/os_free (if enabled)
	 */
	if (argc > 1 && (!os_strcmp(argv[1], "-h") || !os_strcmp(argv[1], "--help"))) {
		BK_LOGI(NULL, "Usage: mem_mt_test <iterations> [task_cnt] [max_size] [pool_mask] [delay_ms]\r\n");
		BK_LOGI(NULL, "  Defaults: task_cnt=8 max_size=4096 pool_mask=0x1 delay_ms=1 pattern=1 seed=now(ms)\r\n");
		BK_LOGI(NULL, "  pool_mask: bit0=os bit1=psram bit2=hsram\r\n");
		BK_LOGI(NULL, "  Example: mem_mt_test 100000\r\n");
		BK_LOGI(NULL, "  Example: mem_mt_test 100000 8 4096 1 1\r\n");
		return;
	}

	if (argc < 2) {
		BK_LOGI(NULL, "Usage: mem_mt_test <iterations> [task_cnt] [max_size] [pool_mask] [delay_ms]\r\n");
		return;
	}

	uint32_t iterations = (uint32_t)os_strtoul(argv[1], NULL, 0);
	uint32_t task_cnt = (argc > 2) ? (uint32_t)os_strtoul(argv[2], NULL, 0) : 8;
	uint32_t min_size = 0;
	uint32_t max_size = (argc > 3) ? (uint32_t)os_strtoul(argv[3], NULL, 0) : 4096;
	uint32_t pool_mask = (argc > 4) ? (uint32_t)os_strtoul(argv[4], NULL, 0) : 0x1;
	uint32_t delay_ms = (argc > 5) ? (uint32_t)os_strtoul(argv[5], NULL, 0) : 1;

	/* Recommended default: head/tail pattern check */
	uint32_t pattern = 1;

	/* Default seed: current time in ms */
	beken_time_t now = 0;
	(void)beken_time_get_time(&now);
	uint32_t seed = now ? (uint32_t)now : 1;

	/* Keep slots fixed by default to reduce CLI args */
	uint32_t local_slots = 64;
	uint32_t shared_slots = 256;

	bk_mem_pool_ops_t pools[3];
	uint32_t pool_cnt = 0;
	os_memset(pools, 0, sizeof(pools));

	if (pool_mask & 0x1u) {
		pools[pool_cnt++] = (bk_mem_pool_ops_t){
			.name = "os",
			.ctx = NULL,
			.alloc = mem_mt_os_alloc,
			.free = mem_mt_os_free,
		};
	}

#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
	if (pool_mask & 0x2u) {
		pools[pool_cnt++] = (bk_mem_pool_ops_t){
			.name = "psram",
			.ctx = NULL,
			.alloc = mem_mt_psram_alloc,
			.free = mem_mt_psram_free,
		};
	}
#endif

#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
	if (pool_mask & 0x4u) {
		pools[pool_cnt++] = (bk_mem_pool_ops_t){
			.name = "hsram",
			.ctx = NULL,
			.alloc = mem_mt_hsram_alloc,
			.free = mem_mt_hsram_free,
		};
	}
#endif

	if (pool_cnt == 0) {
		BK_LOGE(NULL, "No valid pool selected/enabled (pool_mask=0x%X)\r\n", pool_mask);
		return;
	}

	bk_mem_mt_test_cfg_t cfg = {
		.task_cnt = task_cnt,
		.iterations = iterations,
		.min_size = min_size,
		.max_size = max_size,
		.local_slots = local_slots,
		.shared_slots = shared_slots,
		.delay_ms = delay_ms,
		.seed = seed,
		.pattern_mode = (bk_mem_pattern_mode_t)pattern,
	};

	int ret = bk_mem_mt_test_run(pools, pool_cnt, &cfg);
	if (ret == 0) {
		BK_LOGI(NULL, "mem_mt_test PASS\r\n");
	} else {
		BK_LOGE(NULL, "mem_mt_test FAIL: %d\r\n", ret);
	}
}


static void cli_bk_heap_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    void *p;
    p = os_malloc(1024);
    BK_LOGI(NULL, "os_malloc: %p\r\n", p);
    os_free(p);
#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    p = psram_malloc(1024);
    BK_LOGI(NULL, "psram_malloc: %p\r\n", p);
    os_free(p);
#endif
#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    p = hsram_malloc(1024);
    BK_LOGI(NULL, "hsram_malloc: %p\r\n", p);
    os_free(p);
#endif
}

ARCH_CLI_CMD_EXPORT static const struct cli_command s_heap_commands[] = {
	{"mem_mt_test", "mem_mt_test <iterations> [task_cnt] [max_size] [pool_mask] [delay_ms]", cli_mem_mt_test_cmd},
	{"heap_test", "bk_heap_test", cli_bk_heap_test_cmd},
};




