#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu.h>
#include <modules/vg_lite_gpu/vg_lite.h>

#include "h264d_gpu_display_config.h"
#include "h264d_gpu_display_gpu.h"
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
#include "h264d_gpu_display_dpu.h"
#endif

#define TAG "h264d_gpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

typedef struct {
	bk_gpu_ctlr_handle_t handle;
	h264d_gpu_display_gpu_line_done_cb_t line_done_cb;
	void *line_done_args;
	h264d_gpu_display_gpu_frame_done_cb_t frame_done_cb;
	void *frame_done_args;
	beken_semaphore_t display_release_sem;
	volatile uint32_t display_pushed;
} h264d_gpu_display_gpu_ctx_t;

#define DISPLAY_RELEASE_WAIT_MS 100U
/* First 2 pushes bypass wait to fill DPU display+update slots. */
#define DISPLAY_PRIME_COUNT   2U

/* Two output frames are enough when DPU release is faster than GPU production. */
#define GPU_FRAME_POOL_COUNT  2U

typedef struct {
	void *buf;
	uint8_t in_use;
} gpu_frame_pool_entry_t;

static gpu_frame_pool_entry_t s_frame_pool[GPU_FRAME_POOL_COUNT];
static uint32_t s_frame_pool_buf_size;
static uint32_t s_frame_pool_init_count;

static h264d_gpu_display_gpu_ctx_t s_gpu_ctx = {0};

void vg_lite_bus_error_handler(void)
{
	LOGE("vg_lite bus error captured in app override\r\n");
}

static void *h264d_gpu_display_frame_malloc(uint32_t size)
{
	void *ptr = NULL;
	uint32_t flags;
	uint32_t i;

	flags = rtos_enter_critical();
	if (size == s_frame_pool_buf_size && s_frame_pool_init_count != 0U) {
		for (i = 0U; i < s_frame_pool_init_count; i++) {
			if (s_frame_pool[i].buf != NULL && s_frame_pool[i].in_use == 0U) {
				s_frame_pool[i].in_use = 1U;
				ptr = s_frame_pool[i].buf;
				break;
			}
		}
	}
	rtos_exit_critical(flags);

	if (ptr == NULL) {
		ptr = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
		if (ptr == NULL) {
			LOGE("frame malloc FAILED size=%u pool_size=%u\r\n",
			     (unsigned)size,
			     (unsigned)s_frame_pool_buf_size);
		}
	}
	return ptr;
}

static avdk_err_t h264d_gpu_display_frame_free(void *ptr)
{
	uint32_t flags;
	uint8_t from_pool = 0U;
	uint32_t i;

	if (ptr == NULL) {
		return AVDK_ERR_OK;
	}

	flags = rtos_enter_critical();
	for (i = 0U; i < s_frame_pool_init_count; i++) {
		if (s_frame_pool[i].buf == ptr) {
			s_frame_pool[i].in_use = 0U;
			from_pool = 1U;
			break;
		}
	}
	rtos_exit_critical(flags);

	if (from_pool == 0U) {
		bk_frame_buffer_free(ptr);
	}
	return AVDK_ERR_OK;
}

static avdk_err_t h264d_gpu_display_frame_pool_init(uint32_t buf_size)
{
	uint32_t i;
	uint32_t flags;

	if (buf_size == 0U) {
		return AVDK_ERR_INVAL;
	}

	flags = rtos_enter_critical();
	if (s_frame_pool_init_count == GPU_FRAME_POOL_COUNT) {
		for (i = 0U; i < s_frame_pool_init_count; i++) {
			s_frame_pool[i].in_use = 0U;
		}
		rtos_exit_critical(flags);
		return AVDK_ERR_OK;
	}
	rtos_exit_critical(flags);

	for (i = 0U; i < GPU_FRAME_POOL_COUNT; i++) {
		s_frame_pool[i].buf = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, buf_size);
		s_frame_pool[i].in_use = 0U;
		if (s_frame_pool[i].buf == NULL) {
			uint32_t j;

			LOGE("frame pool init: alloc slot %u (size=%u) failed\r\n",
			     (unsigned)i, (unsigned)buf_size);
			for (j = 0U; j < i; j++) {
				if (s_frame_pool[j].buf != NULL) {
					bk_frame_buffer_free(s_frame_pool[j].buf);
					s_frame_pool[j].buf = NULL;
					s_frame_pool[j].in_use = 0U;
				}
			}
			s_frame_pool_init_count = 0U;
			s_frame_pool_buf_size = 0U;
			return AVDK_ERR_NOMEM;
		}
	}
	s_frame_pool_init_count = GPU_FRAME_POOL_COUNT;
	s_frame_pool_buf_size = buf_size;
	LOGI("frame pool ready: %u slots x %u bytes = %u total\r\n",
	     (unsigned)GPU_FRAME_POOL_COUNT,
	     (unsigned)buf_size,
	     (unsigned)(GPU_FRAME_POOL_COUNT * buf_size));

	return AVDK_ERR_OK;
}

void h264d_gpu_display_gpu_frame_pool_deinit(void)
{
	void *bufs[GPU_FRAME_POOL_COUNT] = {0};
	uint32_t i;
	uint32_t flags;

	flags = rtos_enter_critical();
	for (i = 0U; i < GPU_FRAME_POOL_COUNT; i++) {
		bufs[i] = s_frame_pool[i].buf;
		s_frame_pool[i].buf = NULL;
		s_frame_pool[i].in_use = 0U;
	}
	s_frame_pool_init_count = 0U;
	s_frame_pool_buf_size = 0U;
	rtos_exit_critical(flags);

	for (i = 0U; i < GPU_FRAME_POOL_COUNT; i++) {
		if (bufs[i] != NULL) {
			bk_frame_buffer_free(bufs[i]);
		}
	}
}

static void h264d_gpu_display_line_done(uint32_t done_lines, void *args)
{
	(void)args;

	if (s_gpu_ctx.line_done_cb != NULL) {
		s_gpu_ctx.line_done_cb(done_lines, s_gpu_ctx.line_done_args);
	}
}

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
static void h264d_gpu_display_release_sem_deinit(void)
{
	if (s_gpu_ctx.display_release_sem != NULL) {
		beken_semaphore_t sem = s_gpu_ctx.display_release_sem;

		s_gpu_ctx.display_release_sem = NULL;
		(void)rtos_deinit_semaphore(&sem);
	}
	s_gpu_ctx.display_pushed = 0U;
}

static avdk_err_t h264d_gpu_display_dpu_release(void *ptr)
{
	(void)h264d_gpu_display_frame_free(ptr);
	if (s_gpu_ctx.display_release_sem != NULL) {
		(void)rtos_set_semaphore(&s_gpu_ctx.display_release_sem);
	}
	return AVDK_ERR_OK;
}
#endif

static void h264d_gpu_display_frame_done(void *frame, uint32_t frame_size, void *args)
{
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	avdk_err_t ret;
#endif

	(void)args;

	if (s_gpu_ctx.frame_done_cb != NULL) {
		s_gpu_ctx.frame_done_cb(frame, frame_size, s_gpu_ctx.frame_done_args);
	}

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	/* See display_release_sem comment for the back-pressure design. */
	if (s_gpu_ctx.display_pushed >= DISPLAY_PRIME_COUNT &&
	    s_gpu_ctx.display_release_sem != NULL) {
		(void)rtos_get_semaphore(&s_gpu_ctx.display_release_sem,
					 DISPLAY_RELEASE_WAIT_MS);
	}

	ret = h264d_gpu_display_dpu_flush(frame, h264d_gpu_display_dpu_release);
	if (ret != AVDK_ERR_OK) {
		LOGW("dpu flush failed, drop frame ret=%d\r\n", (int)ret);
		(void)h264d_gpu_display_frame_free(frame);
		return;
	}
	s_gpu_ctx.display_pushed++;
#else
	(void)h264d_gpu_display_frame_free(frame);
#endif
}

avdk_err_t h264d_gpu_display_gpu_open(uint8_t *src_buffer,
				      uint8_t flexa_buffer_count,
				      uint16_t src_width,
				      uint16_t src_height,
				      h264d_gpu_display_gpu_line_done_cb_t line_done_cb,
				      void *line_done_args,
				      h264d_gpu_display_gpu_frame_done_cb_t frame_done_cb,
				      void *frame_done_args)
{
	avdk_err_t ret;
	bk_gpu_ctlr_config_t gpu_cfg;

	if (src_buffer == NULL || src_width == 0U || src_height == 0U || flexa_buffer_count == 0U) {
		return AVDK_ERR_INVAL;
	}
	if (src_width != H264D_GPU_DISPLAY_TEST_STREAM_WIDTH ||
	    src_height != H264D_GPU_DISPLAY_TEST_STREAM_HEIGHT) {
		LOGE("unsupported gpu input size %ux%u, expected %ux%u\r\n",
		     (unsigned)src_width,
		     (unsigned)src_height,
		     H264D_GPU_DISPLAY_TEST_STREAM_WIDTH,
		     H264D_GPU_DISPLAY_TEST_STREAM_HEIGHT);
		return AVDK_ERR_INVAL;
	}
	if (s_gpu_ctx.handle != NULL) {
		return AVDK_ERR_BUSY;
	}

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	if (s_gpu_ctx.display_release_sem == NULL) {
		bk_err_t bk_ret = rtos_init_semaphore(&s_gpu_ctx.display_release_sem, 1);

		if (bk_ret != BK_OK) {
			LOGE("init display_release_sem failed=%d\r\n", (int)bk_ret);
			return AVDK_ERR_GENERIC;
		}
	} else {
		/* Drain stale release signal from previous gpu_open/close cycle. */
		(void)rtos_get_semaphore(&s_gpu_ctx.display_release_sem, 0U);
	}
	s_gpu_ctx.display_pushed = 0U;
#endif

	{
		uint32_t pool_buf_size = (uint32_t)bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888) *
					 ((uint32_t)H264D_GPU_DISPLAY_GPU_DST_WIDTH / 4U) *
					 (uint32_t)H264D_GPU_DISPLAY_GPU_DST_HEIGHT;
		avdk_err_t pool_ret = h264d_gpu_display_frame_pool_init(pool_buf_size);

		if (pool_ret != AVDK_ERR_OK) {
			LOGE("frame pool init failed=%d (size=%u)\r\n",
			     (int)pool_ret, (unsigned)pool_buf_size);
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
			h264d_gpu_display_release_sem_deinit();
#endif
			return pool_ret;
		}
	}

	os_memset(&gpu_cfg, 0, sizeof(gpu_cfg));
	gpu_cfg.rotate_degree = H264D_GPU_DISPLAY_GPU_ROTATE_DEGREE;
	gpu_cfg.src_width = src_width;
	gpu_cfg.src_height = src_height;
	gpu_cfg.dst_width = H264D_GPU_DISPLAY_GPU_DST_WIDTH;
	gpu_cfg.dst_height = H264D_GPU_DISPLAY_GPU_DST_HEIGHT;
	gpu_cfg.src_format = BK_PIXEL_FORMAT_NV12;
	gpu_cfg.dst_format = BK_PIXEL_FORMAT_ARGB8888;
	gpu_cfg.scale = true;
	gpu_cfg.compress = true;
	gpu_cfg.src_buffer = src_buffer;
	gpu_cfg.flexa = true;
	gpu_cfg.flexa_lines = H264D_GPU_DISPLAY_GPU_FLEXA_LINES;
	gpu_cfg.flexa_buff_cnt = flexa_buffer_count;
	gpu_cfg.frame_malloc = h264d_gpu_display_frame_malloc;
	gpu_cfg.frame_free = h264d_gpu_display_frame_free;
	gpu_cfg.flexa_line_done = h264d_gpu_display_line_done;
	gpu_cfg.flexa_line_done_args = NULL;
	gpu_cfg.frame_done = h264d_gpu_display_frame_done;
	gpu_cfg.frame_done_args = NULL;

	s_gpu_ctx.line_done_cb = line_done_cb;
	s_gpu_ctx.line_done_args = line_done_args;
	s_gpu_ctx.frame_done_cb = frame_done_cb;
	s_gpu_ctx.frame_done_args = frame_done_args;

	LOGI("gpu cfg: src=%ux%u dst=%ux%u rotate=%u compress=%u scale=%u flexa_lines=%u flexa_buf_cnt=%u src_buf=%p\r\n",
	     (unsigned)gpu_cfg.src_width,
	     (unsigned)gpu_cfg.src_height,
	     (unsigned)gpu_cfg.dst_width,
	     (unsigned)gpu_cfg.dst_height,
	     (unsigned)gpu_cfg.rotate_degree,
	     (unsigned)gpu_cfg.compress,
	     (unsigned)gpu_cfg.scale,
	     (unsigned)gpu_cfg.flexa_lines,
	     (unsigned)gpu_cfg.flexa_buff_cnt,
	     gpu_cfg.src_buffer);

	ret = bk_gpu_ctlr_new(&s_gpu_ctx.handle, &gpu_cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("gpu step failed: bk_gpu_ctlr_new ret=%d\r\n", (int)ret);
		goto error;
	}

	ret = bk_gpu_init(s_gpu_ctx.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("gpu step failed: bk_gpu_init ret=%d\r\n", (int)ret);
		goto error;
	}

	ret = bk_gpu_open(s_gpu_ctx.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("gpu step failed: bk_gpu_open ret=%d\r\n", (int)ret);
		goto error;
	}

	LOGI("gpu open success, src=%ux%u dst=%ux%u rotate=%u compress=%u scale=%u\r\n",
	     (unsigned)src_width,
	     (unsigned)src_height,
	     (unsigned)gpu_cfg.dst_width,
	     (unsigned)gpu_cfg.dst_height,
	     (unsigned)gpu_cfg.rotate_degree,
	     (unsigned)gpu_cfg.compress,
	     (unsigned)gpu_cfg.scale);

	return AVDK_ERR_OK;

error:
	if (s_gpu_ctx.handle != NULL) {
		avdk_err_t close_ret;

		LOGI("gpu cleanup after open failure\r\n");
		close_ret = bk_gpu_close(s_gpu_ctx.handle);
		if (close_ret != AVDK_ERR_OK) {
			LOGE("skip gpu deinit/delete because bk_gpu_close failed: %d\r\n", (int)close_ret);
		} else {
			(void)bk_gpu_deinit(s_gpu_ctx.handle);
			(void)bk_gpu_delete(s_gpu_ctx.handle);
			s_gpu_ctx.handle = NULL;
		}
	}
	s_gpu_ctx.line_done_cb = NULL;
	s_gpu_ctx.line_done_args = NULL;
	s_gpu_ctx.frame_done_cb = NULL;
	s_gpu_ctx.frame_done_args = NULL;
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	h264d_gpu_display_release_sem_deinit();
#endif
	LOGE("gpu open failed: %d\r\n", (int)ret);
	return ret;
}

void h264d_gpu_display_gpu_close(void)
{
	if (s_gpu_ctx.handle != NULL) {
		(void)bk_gpu_close(s_gpu_ctx.handle);
		(void)bk_gpu_deinit(s_gpu_ctx.handle);
		(void)bk_gpu_delete(s_gpu_ctx.handle);
		s_gpu_ctx.handle = NULL;
	}

	s_gpu_ctx.line_done_cb = NULL;
	s_gpu_ctx.line_done_args = NULL;
	s_gpu_ctx.frame_done_cb = NULL;
	s_gpu_ctx.frame_done_args = NULL;
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	h264d_gpu_display_release_sem_deinit();
#endif
	LOGI("gpu closed\r\n");
}

bk_gpu_ctlr_handle_t h264d_gpu_display_gpu_handle_get(void)
{
	return s_gpu_ctx.handle;
}
