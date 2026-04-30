#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu.h>
#include <modules/vg_lite_gpu/vg_lite.h>

#include "h264d_gpu_display_gpu.h"
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
#include "h264d_gpu_display_display.h"
#endif

#define TAG "h264d_gpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264D_GPU_DISPLAY_GPU_ROTATE_DEGREE  90
#define H264D_GPU_DISPLAY_GPU_FLEXA_LINES    16
#define H264D_GPU_DISPLAY_GPU_SRC_WIDTH      1280
#define H264D_GPU_DISPLAY_GPU_SRC_HEIGHT     720
#define H264D_GPU_DISPLAY_GPU_DISPLAY_WIDTH  1080
#define H264D_GPU_DISPLAY_GPU_DISPLAY_HEIGHT 1920
/* For 90-degree rotation, the VG-Lite destination is configured pre-rotation. */
#define H264D_GPU_DISPLAY_GPU_DST_WIDTH      H264D_GPU_DISPLAY_GPU_DISPLAY_HEIGHT
#define H264D_GPU_DISPLAY_GPU_DST_HEIGHT     H264D_GPU_DISPLAY_GPU_DISPLAY_WIDTH

typedef struct {
	bk_gpu_ctlr_handle_t handle;
	h264d_gpu_display_gpu_line_done_cb_t line_done_cb;
	void *line_done_args;
	h264d_gpu_display_gpu_frame_done_cb_t frame_done_cb;
	void *frame_done_args;
} h264d_gpu_display_gpu_ctx_t;

static h264d_gpu_display_gpu_ctx_t s_gpu_ctx = {0};

void vg_lite_bus_error_handler(void)
{
	LOGE("vg_lite bus error captured in app override\r\n");
}

static void *h264d_gpu_display_frame_malloc(uint32_t size)
{
	void *ptr = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);

	LOGI("frame malloc size=%u ptr=%p\r\n", (unsigned)size, ptr);
	return ptr;
}

static avdk_err_t h264d_gpu_display_frame_free(void *ptr)
{
	if (ptr != NULL) {
		bk_frame_buffer_free(ptr);
	}
	return AVDK_ERR_OK;
}

static void h264d_gpu_display_frame_display(void *frame, uint32_t frame_size, void *args)
{
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	avdk_err_t ret;
#endif

	(void)frame_size;
	(void)args;

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	ret = h264d_gpu_display_display_flush(frame, h264d_gpu_display_frame_free);
	if (ret != AVDK_ERR_OK) {
		LOGW("display flush failed, drop frame ret=%d\r\n", (int)ret);
		(void)h264d_gpu_display_frame_free(frame);
	}
#else
	(void)h264d_gpu_display_frame_free(frame);
#endif
}

static void h264d_gpu_display_line_done(uint32_t done_lines, void *args)
{
	(void)args;

	if (s_gpu_ctx.line_done_cb != NULL) {
		s_gpu_ctx.line_done_cb(done_lines, s_gpu_ctx.line_done_args);
	}
}

static void h264d_gpu_display_frame_done(void *frame, uint32_t frame_size, void *args)
{
	(void)args;

	if (s_gpu_ctx.frame_done_cb != NULL) {
		s_gpu_ctx.frame_done_cb(frame, frame_size, s_gpu_ctx.frame_done_args);
	}
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
	if (src_width != H264D_GPU_DISPLAY_GPU_SRC_WIDTH || src_height != H264D_GPU_DISPLAY_GPU_SRC_HEIGHT) {
		LOGE("unsupported gpu input size %ux%u, expected %ux%u\r\n",
		     (unsigned)src_width,
		     (unsigned)src_height,
		     H264D_GPU_DISPLAY_GPU_SRC_WIDTH,
		     H264D_GPU_DISPLAY_GPU_SRC_HEIGHT);
		return AVDK_ERR_INVAL;
	}
	if (s_gpu_ctx.handle != NULL) {
		return AVDK_ERR_BUSY;
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
	gpu_cfg.malloc = h264d_gpu_display_frame_malloc;
	gpu_cfg.free = h264d_gpu_display_frame_free;
	gpu_cfg.frame_display = h264d_gpu_display_frame_display;
	gpu_cfg.frame_display_args = NULL;
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
	LOGI("gpu closed\r\n");
}

bk_gpu_ctlr_handle_t h264d_gpu_display_gpu_handle_get(void)
{
	return s_gpu_ctx.handle;
}
