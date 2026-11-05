/*
 * sdload - background PSRAM/CPU/memory-bus load generator. See sdload.h.
 */
#include "sdload.h"

#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdbool.h>

/* Porting hooks: redefine these for a different platform. */
#ifndef SDLOAD_MALLOC
#define SDLOAD_MALLOC(size)   psram_malloc(size)
#endif
#ifndef SDLOAD_FREE
#define SDLOAD_FREE(ptr)      os_free(ptr)
#endif
#ifndef SDLOAD_LOG
#define SDLOAD_LOG(...)       bk_printf(__VA_ARGS__)
#endif

#define SDLOAD_TASK_PRIO      4
#define SDLOAD_TASK_STACK     4096
#define SDLOAD_TASK_NAME      "sdload"
#define SDLOAD_STOP_POLL_MS   10
#define SDLOAD_STOP_POLL_CNT  500
#define SDLOAD_SRC_PATTERN    0x5a
#define SDLOAD_DST_PATTERN    0xa5

extern void bk_task_wdt_feed(void);

static beken_thread_t s_thread = NULL;
static volatile bool  s_run = false;
static uint8_t *s_src = NULL;
static uint8_t *s_dst = NULL;
static uint32_t s_size = 0;
static uint32_t s_chunk = 0;
static uint32_t s_mode = SDLOAD_MODE_COPY;
static uint32_t s_work_kb = 0;
static uint32_t s_start_ms = 0;
static uint32_t s_elapsed_ms = 0;
static volatile uint32_t s_iterations = 0;
/* Written only by the load task, so the read-mode checksum cannot be
 * optimized away. */
static volatile uint32_t s_sink = 0;

static uint32_t sdload_checksum(const uint8_t *data, uint32_t len)
{
	const uint32_t *p = (const uint32_t *)data;
	uint32_t sum = 0;
	uint32_t i;

	for (i = 0; i < (len / sizeof(uint32_t)); i++)
		sum ^= p[i];

	return sum;
}

static void sdload_task(beken_thread_arg_t arg)
{
	uint32_t offset;

	(void)arg;

	while (s_run) {
		for (offset = 0; offset < s_size; offset += s_chunk) {
			uint32_t len = s_size - offset;

			if (len > s_chunk)
				len = s_chunk;

			switch (s_mode) {
			case SDLOAD_MODE_FILL:
				os_memset(s_dst + offset, (int)(offset >> 12), len);
				break;
			case SDLOAD_MODE_READ:
				s_sink ^= sdload_checksum(s_src + offset, len);
				break;
			case SDLOAD_MODE_COPY:
			default:
				os_memcpy(s_dst + offset, s_src + offset, len);
				break;
			}
		}

		s_iterations++;
		/* The task never blocks, so the idle task cannot feed the task
		 * watchdog on this core. Feed it here. */
		bk_task_wdt_feed();
	}

	s_elapsed_ms = rtos_get_time() - s_start_ms;

	SDLOAD_FREE(s_src);
	SDLOAD_FREE(s_dst);
	s_src = NULL;
	s_dst = NULL;
	s_size = 0;
	s_thread = NULL;
	rtos_delete_thread(NULL);
}

int sdload_start(uint32_t work_kb, uint32_t mode, uint32_t chunk_kb)
{
	uint32_t size;
	uint32_t chunk;
	bk_err_t ret;

	if (s_run || (s_thread != NULL))
		return SDLOAD_ERR_BUSY;
	if ((work_kb == 0) || (work_kb > SDLOAD_MAX_WORK_KB))
		return SDLOAD_ERR_PARAM;
	if ((mode != SDLOAD_MODE_COPY) && (mode != SDLOAD_MODE_FILL) &&
	    (mode != SDLOAD_MODE_READ))
		return SDLOAD_ERR_PARAM;

	if (chunk_kb == 0)
		chunk_kb = SDLOAD_DEFAULT_CHUNK_KB;
	if (chunk_kb > work_kb)
		chunk_kb = work_kb;

	size = work_kb * 1024u;
	chunk = chunk_kb * 1024u;

	s_src = (uint8_t *)SDLOAD_MALLOC(size);
	s_dst = (uint8_t *)SDLOAD_MALLOC(size);
	if ((s_src == NULL) || (s_dst == NULL)) {
		SDLOAD_LOG("[SDLOAD] psram alloc %u KB x2 failed\r\n",
			   (unsigned)work_kb);
		SDLOAD_FREE(s_src);
		SDLOAD_FREE(s_dst);
		s_src = NULL;
		s_dst = NULL;
		return SDLOAD_ERR_NOMEM;
	}

	os_memset(s_src, SDLOAD_SRC_PATTERN, size);
	os_memset(s_dst, SDLOAD_DST_PATTERN, size);

	s_size = size;
	s_chunk = chunk;
	s_mode = mode;
	s_work_kb = work_kb;
	s_iterations = 0;
	s_elapsed_ms = 0;
	s_start_ms = rtos_get_time();
	s_run = true;

	ret = rtos_create_thread(&s_thread, SDLOAD_TASK_PRIO, SDLOAD_TASK_NAME,
				 sdload_task, SDLOAD_TASK_STACK,
				 (beken_thread_arg_t)0);
	if (ret != 0) {
		s_run = false;
		s_thread = NULL;
		SDLOAD_FREE(s_src);
		SDLOAD_FREE(s_dst);
		s_src = NULL;
		s_dst = NULL;
		s_size = 0;
		SDLOAD_LOG("[SDLOAD] task create failed %d\r\n", ret);
		return SDLOAD_ERR_THREAD;
	}

	SDLOAD_LOG("[SDLOAD] started: work=%u KB x2 mode=%u chunk=%u KB prio=%u\r\n",
		   (unsigned)work_kb, (unsigned)mode, (unsigned)chunk_kb,
		   SDLOAD_TASK_PRIO);
	return SDLOAD_OK;
}

int sdload_stop(void)
{
	uint32_t i;

	if (!s_run && (s_thread == NULL))
		return SDLOAD_OK;

	s_run = false;
	for (i = 0; (i < SDLOAD_STOP_POLL_CNT) && (s_thread != NULL); i++)
		rtos_delay_milliseconds(SDLOAD_STOP_POLL_MS);

	if (s_thread != NULL) {
		SDLOAD_LOG("[SDLOAD] stop timeout\r\n");
		return SDLOAD_ERR_TIMEOUT;
	}

	SDLOAD_LOG("[SDLOAD] stopped\r\n");
	return SDLOAD_OK;
}

int sdload_is_running(void)
{
	return s_run ? 1 : 0;
}

int sdload_get_info(sdload_info_t *info)
{
	if (info == NULL)
		return SDLOAD_ERR_PARAM;

	info->mode = s_mode;
	info->work_kb = s_work_kb;
	info->chunk_kb = s_chunk / 1024u;
	info->iterations = s_iterations;
	info->elapsed_ms = s_run ? (rtos_get_time() - s_start_ms) : s_elapsed_ms;
	return SDLOAD_OK;
}
