#include <stdint.h>
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_decode/bk_h264_decode_ctlr.h>
#include <components/bk_flexa_bond.h>

#include "cli.h"
#include "bk_private/bk_cli.h"
#include "h264d_gpu_display_config.h"
#include "h264d_gpu_display_demo.h"
#include "h264d_gpu_display_h264_parser.h"
#include "h264d_gpu_display_gpu.h"
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
#include "h264d_gpu_display_dpu.h"
#endif
#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP
#include "h264d_gpu_display_isp.h"
#endif
#ifdef CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_1280X720
#include "h264_decode_stream_1280x720.h"
#else
#include "h264_decode_stream_720x1280.h"
#endif

#ifdef CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_1280X720
#define H264D_GPU_DISPLAY_STREAM_DATA  h264_decode_stream_1280x720
#define H264D_GPU_DISPLAY_STREAM_BYTES h264_decode_stream_1280x720_bytes
#else
#define H264D_GPU_DISPLAY_STREAM_DATA  h264_decode_stream_720x1280
#define H264D_GPU_DISPLAY_STREAM_BYTES h264_decode_stream_720x1280_bytes
#endif

#define TAG "h264d_gpu_demo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#define H264D_GPU_DISPLAY_TASK_PRIORITY   3
#define H264D_GPU_DISPLAY_TASK_STACK_SIZE (1024 * 16)
#define H264D_GPU_DISPLAY_TIMEOUT_MS      1000U
#define H264D_GPU_DISPLAY_SEG_HEIGHT_MB   1U
#define H264D_GPU_DISPLAY_SEG_NUM         4U
#define H264D_GPU_DISPLAY_FPS_TIMER_MS    4000U

typedef enum {
	H264D_GPU_DISPLAY_RUN_FLEXA_NV12 = 0,
	H264D_GPU_DISPLAY_RUN_FRAME_RGB565,
	H264D_GPU_DISPLAY_RUN_FRAME_RGB888,
} h264d_gpu_display_run_mode_t;

typedef struct {
	volatile uint32_t decoded_frames;
	volatile uint32_t fps_last_decoded_frames;
	uint8_t fps_timer_started;
	beken_timer_t fps_timer;
} h264d_gpu_display_fps_t;

typedef struct {
	volatile uint32_t frame_done_count;
	volatile int last_frame_status;
} h264d_gpu_display_frame_done_t;

static beken_thread_t s_h264d_gpu_display_thread = NULL;
static volatile uint8_t s_h264d_gpu_display_running = 0U;

/* Co-operative stop flag, set by CLI 'stop', polled between frames. */
static volatile uint8_t s_h264d_gpu_display_stop_request = 0U;
/* Outer-loop budget. 0 = infinite (until stop), N = exactly N passes. */
static volatile uint32_t s_h264d_gpu_display_max_loops = 0U;
static volatile h264d_gpu_display_run_mode_t s_h264d_gpu_display_run_mode = H264D_GPU_DISPLAY_RUN_FLEXA_NV12;

static void *h264d_gpu_display_stream_buffer_malloc(uint32_t size)
{
	void *ptr;

	LOGI("copy stream to frame buffer coded heap, size=%u\r\n", (unsigned)size);

	ptr = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size);
	if (ptr == NULL) {
		LOGE("alloc stream frame buffer failed, size=%u\r\n", (unsigned)size);
		return NULL;
	}

	LOGI("stream frame buffer=%p\r\n", ptr);
	return ptr;
}

static void h264d_gpu_display_stream_buffer_free(void *ptr)
{
	if (ptr != NULL) {
		bk_frame_buffer_free(ptr);
	}
}

static avdk_err_t h264d_gpu_display_display_frame_free(void *ptr)
{
	if (ptr != NULL) {
		bk_frame_buffer_free(ptr);
	}

	return AVDK_ERR_OK;
}

/* Flexa ring buffer is consumed by GPU/DMA directly -> 64-byte alignment. */
static void *h264d_gpu_display_hsram_aligned_malloc(uint32_t alignment, uint32_t size)
{
	void *raw;
	uint32_t total;
	uintptr_t start;
	uintptr_t aligned;

	if (alignment < (uint32_t)sizeof(void *)) {
		alignment = (uint32_t)sizeof(void *);
	}
	if ((alignment & (alignment - 1U)) != 0U) {
		return NULL;
	}

	total = size + alignment - 1U + (uint32_t)sizeof(void *);
	raw = hsram_malloc(total);
	if (raw == NULL) {
		return NULL;
	}

	start = (uintptr_t)raw + sizeof(void *);
	aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
	((void **)aligned)[-1] = raw;
	return (void *)aligned;
}

static void h264d_gpu_display_hsram_aligned_free(void *ptr)
{
	void *raw;

	if (ptr == NULL) {
		return;
	}

	raw = ((void **)ptr)[-1];
	os_free(raw);
}

static uint32_t h264d_gpu_display_flexa_buffer_size(uint32_t width)
{
	return bk_image_size_get((uint16_t)width,
	                         16U * H264D_GPU_DISPLAY_SEG_HEIGHT_MB * H264D_GPU_DISPLAY_SEG_NUM,
	                         BK_PIXEL_FORMAT_NV12);
}

static uint32_t h264d_gpu_display_rgb_output_size(uint32_t width, uint32_t height, bk_pixel_format_t fmt)
{
	uint32_t h_aligned = (height + 15U) & ~15U;

	if (fmt == BK_PIXEL_FORMAT_RGB888) {
		/* VCDec PP RGB888 path writes one pixel per 32-bit word. */
		return width * h_aligned * 4U;
	}

	return bk_image_size_get((uint16_t)width, (uint16_t)h_aligned, fmt);
}

static uint32_t h264d_gpu_display_buffer_hash(const uint8_t *buf, uint32_t size)
{
	uint32_t hash = 2166136261U;
	uint32_t step;
	uint32_t i;

	if (buf == NULL || size == 0U) {
		return 0U;
	}

	step = (size > 8192U) ? (size / 8192U) : 1U;
	for (i = 0U; i < size; i += step) {
		hash ^= buf[i];
		hash *= 16777619U;
	}

	return hash;
}

static const char *h264d_gpu_display_run_mode_name(h264d_gpu_display_run_mode_t mode)
{
	switch (mode) {
	case H264D_GPU_DISPLAY_RUN_FRAME_RGB565:
		return "frame_rgb565";
	case H264D_GPU_DISPLAY_RUN_FRAME_RGB888:
		return "frame_rgb888";
	default:
		return "flexa_nv12";
	}
}

static bk_pixel_format_t h264d_gpu_display_run_mode_format(h264d_gpu_display_run_mode_t mode)
{
	return (mode == H264D_GPU_DISPLAY_RUN_FRAME_RGB565) ?
		BK_PIXEL_FORMAT_RGB565 : BK_PIXEL_FORMAT_RGB888;
}

static void h264d_gpu_display_frame_done_cb(int status, void *args)
{
	h264d_gpu_display_frame_done_t *done = (h264d_gpu_display_frame_done_t *)args;

	if (done == NULL) {
		return;
	}

	done->last_frame_status = status;
	done->frame_done_count++;
}

static void h264d_gpu_display_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
	size_t msg_len;

	if (pcWriteBuffer == NULL || xWriteBufferLen <= 0 || msg == NULL) {
		return;
	}

	msg_len = os_strlen(msg);
	if (msg_len >= (size_t)xWriteBufferLen) {
		msg_len = (size_t)xWriteBufferLen - 1U;
	}

	os_memcpy(pcWriteBuffer, msg, msg_len);
	pcWriteBuffer[msg_len] = '\0';
}

static void h264d_gpu_display_fps_timer_cb(void *args)
{
	h264d_gpu_display_fps_t *fps = (h264d_gpu_display_fps_t *)args;
	uint32_t curr;
	uint32_t delta;
	uint32_t fps_x10;

	if (fps == NULL) {
		return;
	}

	curr = fps->decoded_frames;
	delta = curr - fps->fps_last_decoded_frames;
	fps->fps_last_decoded_frames = curr;

	fps_x10 = (delta * 10000U) / H264D_GPU_DISPLAY_FPS_TIMER_MS;
	LOGI("h264 decode: [%ufps]\r\n", (unsigned)(fps_x10 / 10U));
}

static avdk_err_t h264d_gpu_display_fps_timer_start(h264d_gpu_display_fps_t *fps)
{
	bk_err_t bk_ret;

	if (fps == NULL) {
		return AVDK_ERR_INVAL;
	}

	fps->fps_last_decoded_frames = fps->decoded_frames;
	bk_ret = rtos_init_timer(&fps->fps_timer,
				 H264D_GPU_DISPLAY_FPS_TIMER_MS,
				 h264d_gpu_display_fps_timer_cb,
				 fps);
	if (bk_ret != BK_OK) {
		LOGE("init h264 decode fps timer failed=%d\r\n", (int)bk_ret);
		return AVDK_ERR_GENERIC;
	}

	bk_ret = rtos_start_timer(&fps->fps_timer);
	if (bk_ret != BK_OK) {
		LOGE("start h264 decode fps timer failed=%d\r\n", (int)bk_ret);
		(void)rtos_deinit_timer(&fps->fps_timer);
		return AVDK_ERR_GENERIC;
	}

	fps->fps_timer_started = 1U;
	return AVDK_ERR_OK;
}

static void h264d_gpu_display_fps_timer_stop(h264d_gpu_display_fps_t *fps)
{
	if (fps == NULL || fps->fps_timer_started == 0U) {
		return;
	}

	if (rtos_is_timer_running(&fps->fps_timer)) {
		(void)rtos_stop_timer(&fps->fps_timer);
	}
	(void)rtos_deinit_timer(&fps->fps_timer);
	fps->fps_timer_started = 0U;
}

static avdk_err_t h264d_gpu_display_run(void)
{
	avdk_err_t ret = AVDK_ERR_OK;
	uint8_t *stream_buf = NULL;
	uint8_t *pp_buf = NULL;
	h264d_gpu_display_h264_frame_t *frame_table = NULL;
	uint32_t frame_count = 0U;
	uint32_t stream_size = H264D_GPU_DISPLAY_STREAM_BYTES;
	const uint32_t width = H264D_GPU_DISPLAY_TEST_STREAM_WIDTH;
	const uint32_t height = H264D_GPU_DISPLAY_TEST_STREAM_HEIGHT;
	const uint32_t pp_size = h264d_gpu_display_flexa_buffer_size(width);
	uint32_t frame_index;
	bk_h264_decode_ctlr_handle_t decoder = NULL;
	void *bond = NULL;
	h264d_gpu_display_fps_t fps;
	bk_h264_decode_flexa_config_t dec_cfg = DEFAULT_H264_DECODE_FLEXA_CONFIG;

	os_memset(&fps, 0, sizeof(fps));

	stream_buf = (uint8_t *)h264d_gpu_display_stream_buffer_malloc(stream_size);
	if (stream_buf == NULL) {
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, H264D_GPU_DISPLAY_STREAM_DATA, stream_size);
	LOGI("stream copied to frame buffer, stream=%s src=%p dst=%p bytes=%u\r\n",
	     H264D_GPU_DISPLAY_TEST_STREAM_NAME,
	     H264D_GPU_DISPLAY_STREAM_DATA,
	     stream_buf,
	     (unsigned)stream_size);

	LOGI("stage: parse h264 frames\r\n");
	ret = h264d_gpu_display_h264_build_frame_table(stream_buf, stream_size, &frame_table, &frame_count);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	pp_buf = (uint8_t *)h264d_gpu_display_hsram_aligned_malloc(64U, pp_size);
	if (pp_buf == NULL) {
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memset(pp_buf, 0, pp_size);
	LOGI("flexa pp buffer=%p size=%u aligned64=%u\r\n",
	     pp_buf,
	     (unsigned)pp_size,
	     ((uintptr_t)pp_buf & 0x3FU) == 0U ? 1U : 0U);

	dec_cfg.timeout_ms = H264D_GPU_DISPLAY_TIMEOUT_MS;
	dec_cfg.out_width = (uint16_t)width;
	dec_cfg.out_height = (uint16_t)height;
	dec_cfg.out_format = BK_PIXEL_FORMAT_NV12;
	dec_cfg.segment_height = H264D_GPU_DISPLAY_SEG_HEIGHT_MB;
	dec_cfg.segment_number = H264D_GPU_DISPLAY_SEG_NUM;

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	LOGI("stage: open dpu\r\n");
	ret = h264d_gpu_display_dpu_open();
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
#else
	LOGI("stage: display disabled by H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY\r\n");
#endif

	LOGI("stage: open gpu\r\n");
	ret = h264d_gpu_display_gpu_open(pp_buf,
					 H264D_GPU_DISPLAY_SEG_NUM,
					 (uint16_t)width,
					 (uint16_t)height,
					 NULL,
					 NULL,
					 NULL,
					 NULL);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	LOGI("stage: create decoder\r\n");
	ret = bk_h264_decode_flexa_ctlr_new(&decoder, &dec_cfg);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
	LOGI("stage: init decoder\r\n");
	ret = bk_h264_decode_init(decoder);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
	LOGI("stage: open decoder\r\n");
	ret = bk_h264_decode_open(decoder);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	LOGI("stage: start h264d->gpu bond\r\n");
	ret = bk_flexa_h264d_gpu_bond_start(&bond, decoder, h264d_gpu_display_gpu_handle_get());
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP
	if (h264d_gpu_display_isp_is_open()) {
		LOGI("stage: ISP overlay active (PIP enabled)\r\n");
	} else {
		LOGI("stage: ISP overlay inactive (run 'isp_open' to enable PIP)\r\n");
	}
#endif

	ret = h264d_gpu_display_fps_timer_start(&fps);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	LOGI("demo start, stream=%s decode=%ux%u, display=%ux%u gpu dst=%ux%u rotate=%u compress=1 scale=1\r\n",
	     H264D_GPU_DISPLAY_TEST_STREAM_NAME,
	     (unsigned)width,
	     (unsigned)height,
	     (unsigned)h264d_gpu_display_dpu_width(),
	     (unsigned)h264d_gpu_display_dpu_height(),
	     (unsigned)H264D_GPU_DISPLAY_GPU_DST_WIDTH,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DST_HEIGHT,
	     (unsigned)H264D_GPU_DISPLAY_GPU_ROTATE_DEGREE);
#else
	LOGI("demo start, stream=%s decode=%ux%u, display=disabled gpu dst=%ux%u rotate=%u compress=1 scale=1\r\n",
	     H264D_GPU_DISPLAY_TEST_STREAM_NAME,
	     (unsigned)width,
	     (unsigned)height,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DST_WIDTH,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DST_HEIGHT,
	     (unsigned)H264D_GPU_DISPLAY_GPU_ROTATE_DEGREE);
#endif

	{
		uint32_t loop_index = 0U;
		const uint32_t max_loops = s_h264d_gpu_display_max_loops;
		uint8_t aborted = 0U;

		/*
		 * Outer loop: replay from byte 0 each pass. First frame is
		 * always an IDR (SPS+PPS+IDR slice) which resets decoder
		 * reference state, so rewinding is just frame_index=0 -- no
		 * need to recreate decoder/GPU/display/bond.
		 * Exits on: stop_request, max_loops reached, or decode error.
		 */
		while (s_h264d_gpu_display_stop_request == 0U) {
			uint32_t frame_done_this_loop = 0U;

			LOGI(">>> loop %u start\r\n", (unsigned)(loop_index + 1U));

			for (frame_index = 0U; frame_index < frame_count; frame_index++) {
				const h264d_gpu_display_h264_frame_t *frame = &frame_table[frame_index];
				bk_h264_decode_input_t input = {0};
				bk_h264_decode_info_t info = {0};

				if (s_h264d_gpu_display_stop_request != 0U) {
					aborted = 1U;
					break;
				}

				input.stream = stream_buf + frame->offset;
				input.stream_len = frame->size;
				input.out_buffer = pp_buf;
				input.out_buffer_size = pp_size;

				ret = bk_h264_decode_frame(decoder, &input);
				if (ret != AVDK_ERR_OK) {
					LOGE("decode failed on loop=%u frame=%u ret=%d\r\n",
					     (unsigned)(loop_index + 1U),
					     (unsigned)(fps.decoded_frames + 1U),
					     (int)ret);
					goto cleanup;
				}

				ret = bk_h264_decode_get_info(decoder, &info);
				if (ret != AVDK_ERR_OK) {
					LOGE("get info failed on loop=%u frame=%u ret=%d\r\n",
					     (unsigned)(loop_index + 1U),
					     (unsigned)(fps.decoded_frames + 1U),
					     (int)ret);
					goto cleanup;
				}

				fps.decoded_frames++;
				frame_done_this_loop++;
			}

			if (frame_done_this_loop == 0U) {
				/* Aborted before first frame, or malformed stream. */
				break;
			}

			loop_index++;
			LOGI("<<< loop %u done (frame_done=%u total_frames=%u)\r\n",
			     (unsigned)loop_index,
			     (unsigned)frame_done_this_loop,
			     (unsigned)fps.decoded_frames);

			if (aborted != 0U) {
				break;
			}
			if (max_loops != 0U && loop_index >= max_loops) {
				LOGI("loop budget %u reached, exiting\r\n",
				     (unsigned)max_loops);
				break;
			}
		}
	}

	if (fps.decoded_frames == 0U) {
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}

	LOGI("demo done, decoded_frames=%u\r\n", (unsigned)fps.decoded_frames);

	ret = AVDK_ERR_OK;

cleanup:
	h264d_gpu_display_fps_timer_stop(&fps);
	if (bond != NULL) {
		bk_flexa_h264d_gpu_bond_stop(bond);
	}
	h264d_gpu_display_gpu_close();
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	h264d_gpu_display_dpu_close();
#endif
	h264d_gpu_display_gpu_frame_pool_deinit();
	if (decoder != NULL) {
		(void)bk_h264_decode_close(decoder);
		(void)bk_h264_decode_deinit(decoder);
		(void)bk_h264_decode_delete(decoder);
	}
	if (pp_buf != NULL) {
		h264d_gpu_display_hsram_aligned_free(pp_buf);
	}
	h264d_gpu_display_h264_frame_table_free(frame_table);
	if (stream_buf != NULL) {
		h264d_gpu_display_stream_buffer_free(stream_buf);
	}

	LOGI("[RESULT][%s] decoded_frames=%u\r\n",
	     (ret == AVDK_ERR_OK) ? "PASS" : "FAIL",
	     (unsigned)fps.decoded_frames);
	return ret;
}

static avdk_err_t h264d_gpu_display_run_rgb(h264d_gpu_display_run_mode_t mode)
{
	avdk_err_t ret = AVDK_ERR_OK;
	uint8_t *stream_buf = NULL;
	uint8_t *rgb_buf = NULL;
	h264d_gpu_display_h264_frame_t *frame_table = NULL;
	uint32_t frame_count = 0U;
	uint32_t stream_size = H264D_GPU_DISPLAY_STREAM_BYTES;
	const uint32_t width = H264D_GPU_DISPLAY_TEST_STREAM_WIDTH;
	const uint32_t height = H264D_GPU_DISPLAY_TEST_STREAM_HEIGHT;
	bk_pixel_format_t rgb_format = h264d_gpu_display_run_mode_format(mode);
	uint32_t rgb_size = h264d_gpu_display_rgb_output_size(width, height, rgb_format);
	uint32_t frame_index;
	uint32_t loop_index = 0U;
	uint8_t aborted = 0U;
	bk_h264_decode_ctlr_handle_t decoder = NULL;
	h264d_gpu_display_fps_t fps;
	h264d_gpu_display_frame_done_t frame_done;
	bk_h264_decode_frame_config_t dec_cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;

	os_memset(&fps, 0, sizeof(fps));
	os_memset(&frame_done, 0, sizeof(frame_done));

	stream_buf = (uint8_t *)h264d_gpu_display_stream_buffer_malloc(stream_size);
	if (stream_buf == NULL) {
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, H264D_GPU_DISPLAY_STREAM_DATA, stream_size);
	LOGI("rgb test stream copied, mode=%s stream=%s bytes=%u\r\n",
	     h264d_gpu_display_run_mode_name(mode),
	     H264D_GPU_DISPLAY_TEST_STREAM_NAME,
	     (unsigned)stream_size);

	ret = h264d_gpu_display_h264_build_frame_table(stream_buf, stream_size, &frame_table, &frame_count);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	rgb_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, rgb_size);
	if (rgb_buf == NULL) {
		LOGE("alloc rgb decode buffer failed, size=%u\r\n", (unsigned)rgb_size);
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memset(rgb_buf, 0, rgb_size);
	LOGI("rgb decode buffer=%p size=%u format=%u\r\n",
	     rgb_buf, (unsigned)rgb_size, (unsigned)rgb_format);

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	LOGI("stage: open dpu for rgb test\r\n");
	ret = h264d_gpu_display_dpu_open();
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
#else
	LOGI("stage: display disabled, rgb test will only run decode+gpu blit\r\n");
#endif

	dec_cfg.timeout_ms = H264D_GPU_DISPLAY_TIMEOUT_MS;
	dec_cfg.out_width = (uint16_t)width;
	dec_cfg.out_height = (uint16_t)height;
	dec_cfg.out_format = rgb_format;
	dec_cfg.frame_done_cb = h264d_gpu_display_frame_done_cb;
	dec_cfg.frame_done_args = &frame_done;

	ret = bk_h264_decode_frame_ctlr_new(&decoder, &dec_cfg);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
	ret = bk_h264_decode_init(decoder);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
	ret = bk_h264_decode_open(decoder);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	ret = h264d_gpu_display_fps_timer_start(&fps);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	LOGI("rgb demo start, mode=%s decode=%ux%u display=%ux%u rotate=%u\r\n",
	     h264d_gpu_display_run_mode_name(mode),
	     (unsigned)width,
	     (unsigned)height,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DISPLAY_WIDTH,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DISPLAY_HEIGHT,
	     (unsigned)H264D_GPU_DISPLAY_GPU_ROTATE_DEGREE);

	while (s_h264d_gpu_display_stop_request == 0U) {
		uint32_t frame_done_this_loop = 0U;

		LOGI(">>> rgb loop %u start\r\n", (unsigned)(loop_index + 1U));
		for (frame_index = 0U; frame_index < frame_count; frame_index++) {
			const h264d_gpu_display_h264_frame_t *frame = &frame_table[frame_index];
			bk_h264_decode_input_t input = {0};
			void *display_frame = NULL;
			uint32_t display_frame_size = 0U;
			uint32_t rgb_hash;
			uint32_t expected_frame_done = frame_done.frame_done_count + 1U;

			if (s_h264d_gpu_display_stop_request != 0U) {
				aborted = 1U;
				break;
			}

			input.stream = stream_buf + frame->offset;
			input.stream_len = frame->size;
			input.out_buffer = rgb_buf;
			input.out_buffer_size = rgb_size;

			ret = bk_h264_decode_frame(decoder, &input);
			if (ret != AVDK_ERR_OK ||
			    frame_done.frame_done_count != expected_frame_done ||
			    frame_done.last_frame_status != BK_OK) {
				LOGE("rgb decode failed loop=%u frame=%u ret=%d done=%u expected=%u status=%d\r\n",
				     (unsigned)(loop_index + 1U),
				     (unsigned)(fps.decoded_frames + 1U),
				     (int)ret,
				     (unsigned)frame_done.frame_done_count,
				     (unsigned)expected_frame_done,
				     (int)frame_done.last_frame_status);
				ret = AVDK_ERR_GENERIC;
				goto cleanup;
			}

			rgb_hash = h264d_gpu_display_buffer_hash(rgb_buf, rgb_size);
			if (fps.decoded_frames < 4U) {
				LOGI("rgb frame %u hash=0x%08x first_word=0x%08x\r\n",
				     (unsigned)(fps.decoded_frames + 1U),
				     (unsigned)rgb_hash,
				     (unsigned)((const uint32_t *)rgb_buf)[0]);
			}

			ret = h264d_gpu_display_gpu_blit_rgb_frame(rgb_buf,
								  (uint16_t)width,
								  (uint16_t)height,
								  rgb_format,
								  &display_frame,
								  &display_frame_size);
			if (ret != AVDK_ERR_OK) {
				goto cleanup;
			}

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
			ret = h264d_gpu_display_dpu_flush(display_frame, h264d_gpu_display_display_frame_free);
			if (ret != AVDK_ERR_OK) {
				LOGE("rgb dpu flush failed ret=%d\r\n", (int)ret);
				(void)h264d_gpu_display_display_frame_free(display_frame);
				goto cleanup;
			}
#else
			LOGI("rgb display disabled, drop gpu frame=%p size=%u\r\n",
			     display_frame, (unsigned)display_frame_size);
			(void)h264d_gpu_display_display_frame_free(display_frame);
#endif

			fps.decoded_frames++;
			frame_done_this_loop++;
			rtos_delay_milliseconds(33U);
		}

		if (frame_done_this_loop == 0U) {
			break;
		}

		loop_index++;
		LOGI("<<< rgb loop %u done (frame_done=%u total_frames=%u)\r\n",
		     (unsigned)loop_index,
		     (unsigned)frame_done_this_loop,
		     (unsigned)fps.decoded_frames);

		if (aborted != 0U) {
			break;
		}
		if (s_h264d_gpu_display_max_loops != 0U &&
		    loop_index >= s_h264d_gpu_display_max_loops) {
			LOGI("rgb loop budget %u reached, exiting\r\n",
			     (unsigned)s_h264d_gpu_display_max_loops);
			break;
		}
	}

	if (fps.decoded_frames == 0U) {
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}

	ret = AVDK_ERR_OK;

cleanup:
	h264d_gpu_display_fps_timer_stop(&fps);
	h264d_gpu_display_gpu_blit_deinit();
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	h264d_gpu_display_dpu_close();
#endif
	if (decoder != NULL) {
		(void)bk_h264_decode_close(decoder);
		(void)bk_h264_decode_deinit(decoder);
		(void)bk_h264_decode_delete(decoder);
	}
	if (rgb_buf != NULL) {
		bk_frame_buffer_free(rgb_buf);
	}
	h264d_gpu_display_h264_frame_table_free(frame_table);
	if (stream_buf != NULL) {
		h264d_gpu_display_stream_buffer_free(stream_buf);
	}

	LOGI("[RESULT][%s] %s decoded_frames=%u\r\n",
	     (ret == AVDK_ERR_OK) ? "PASS" : "FAIL",
	     h264d_gpu_display_run_mode_name(mode),
	     (unsigned)fps.decoded_frames);
	return ret;
}

static void h264d_gpu_display_task_entry(void *arg)
{
	(void)arg;
	h264d_gpu_display_run_mode_t mode = s_h264d_gpu_display_run_mode;

	LOGI("task entry, mode=%s max_loops=%u (0=infinite)\r\n",
	     h264d_gpu_display_run_mode_name(mode),
	     (unsigned)s_h264d_gpu_display_max_loops);
	if (mode == H264D_GPU_DISPLAY_RUN_FLEXA_NV12) {
		(void)h264d_gpu_display_run();
	} else {
		(void)h264d_gpu_display_run_rgb(mode);
	}

	s_h264d_gpu_display_running = 0U;
	s_h264d_gpu_display_stop_request = 0U;
	s_h264d_gpu_display_thread = NULL;
	rtos_delete_thread(NULL);
}

static void h264d_gpu_display_print_usage(void)
{
	bk_printf("Usage:\r\n");
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	bk_printf("  h264d_gpu_display start [loops]                              - loop H264D -> GPU -> MIPI display test\r\n");
#else
	bk_printf("  h264d_gpu_display start [loops]                              - loop H264D -> GPU test without display\r\n");
#endif
	bk_printf("  h264d_gpu_display start_rgb565 [loops]                       - decode H264 to RGB565, then GPU -> display\r\n");
	bk_printf("  h264d_gpu_display start_rgb888 [loops]                       - decode H264 to RGB888(32bpp), then GPU -> display\r\n");
	bk_printf("                                                               - omit loops or 0: run forever until 'stop'\r\n");
	bk_printf("                                                               - loops > 0: stop after that many full passes\r\n");
	bk_printf("  h264d_gpu_display stop                                       - request the running demo to stop after current frame\r\n");
	bk_printf("  h264d_gpu_display help                                       - show this help\r\n");
#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP
	bk_printf("  h264d_gpu_display isp_open [sensor_w sensor_h fps isp_w isp_h [dst_x dst_y]]\r\n");
	bk_printf("                                                               - bring up CSI sensor + ISP for PIP overlay\r\n");
	bk_printf("                                                               - no-arg uses defaults (%u %u %u %u %u %u %u)\r\n",
	          (unsigned)H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_WIDTH,
	          (unsigned)H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_HEIGHT,
	          (unsigned)H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_FPS,
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_WIDTH,
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_HEIGHT,
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_X,
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_Y);
	bk_printf("                                                               - default PIP rotate=%u, visible=%ux%u\r\n",
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE,
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_VISIBLE_WIDTH,
	          (unsigned)H264D_GPU_DISPLAY_ISP_PIP_VISIBLE_HEIGHT);
	bk_printf("  h264d_gpu_display isp_close                                   - tear down ISP + sensor + bus\r\n");
#endif
}

static avdk_err_t h264d_gpu_display_start_mode(uint32_t max_loops, h264d_gpu_display_run_mode_t mode)
{
	avdk_err_t ret;

	if (s_h264d_gpu_display_running != 0U) {
		return AVDK_ERR_BUSY;
	}

	s_h264d_gpu_display_running = 1U;
	s_h264d_gpu_display_stop_request = 0U;
	s_h264d_gpu_display_max_loops = max_loops;
	s_h264d_gpu_display_run_mode = mode;

	ret = rtos_core0_create_thread(&s_h264d_gpu_display_thread,
				       H264D_GPU_DISPLAY_TASK_PRIORITY,
				       "h264d_gpu_disp",
				       (beken_thread_function_t)h264d_gpu_display_task_entry,
				       H264D_GPU_DISPLAY_TASK_STACK_SIZE,
				       NULL);
	if (ret != BK_OK) {
		s_h264d_gpu_display_running = 0U;
		s_h264d_gpu_display_thread = NULL;
		return AVDK_ERR_GENERIC;
	}

	return AVDK_ERR_OK;
}

avdk_err_t h264d_gpu_display_start(uint32_t max_loops)
{
	return h264d_gpu_display_start_mode(max_loops, H264D_GPU_DISPLAY_RUN_FLEXA_NV12);
}

static avdk_err_t h264d_gpu_display_stop_task(void)
{
	if (s_h264d_gpu_display_running == 0U) {
		LOGI("stop: demo not running\r\n");
		return AVDK_ERR_OK;
	}
	s_h264d_gpu_display_stop_request = 1U;
	LOGI("stop requested, demo will exit after current frame\r\n");
	return AVDK_ERR_OK;
}

#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP
static avdk_err_t cli_h264d_gpu_display_isp_open(int argc, char **argv)
{
	h264d_gpu_display_isp_params_t params = {
		.sensor_w = H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_WIDTH,
		.sensor_h = H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_HEIGHT,
		.fps      = H264D_GPU_DISPLAY_ISP_SENSOR_DEFAULT_FPS,
		.isp_w     = H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_WIDTH,
		.isp_h     = H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_HEIGHT,
		.dst_x    = H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_X,
		.dst_y    = H264D_GPU_DISPLAY_ISP_PIP_DEFAULT_DST_Y,
	};

	if (argc != 2 && argc != 7 && argc != 9) {
		LOGE("isp_open: bad arg count (%d), expect 0 or 5 or 7 numeric args\r\n", argc - 2);
		return AVDK_ERR_INVAL;
	}
	if (argc >= 7) {
		params.sensor_w = (uint16_t)os_strtoul(argv[2], NULL, 10);
		params.sensor_h = (uint16_t)os_strtoul(argv[3], NULL, 10);
		params.fps      = (uint16_t)os_strtoul(argv[4], NULL, 10);
		params.isp_w     = (uint16_t)os_strtoul(argv[5], NULL, 10);
		params.isp_h     = (uint16_t)os_strtoul(argv[6], NULL, 10);
	}
	if (argc == 9) {
		params.dst_x = (uint16_t)os_strtoul(argv[7], NULL, 10);
		params.dst_y = (uint16_t)os_strtoul(argv[8], NULL, 10);
	}

	return h264d_gpu_display_isp_open(&params);
}
#endif /* H264D_GPU_DISPLAY_ENABLE_ISP_PIP */

static void cli_h264d_gpu_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	avdk_err_t ret = AVDK_ERR_OK;

	if (argc < 2) {
		h264d_gpu_display_print_usage();
		h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
		h264d_gpu_display_print_usage();
		h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		uint32_t loops = 0U;

		if (argc >= 3) {
			loops = (uint32_t)os_strtoul(argv[2], NULL, 10);
		}
		ret = h264d_gpu_display_start_mode(loops, H264D_GPU_DISPLAY_RUN_FLEXA_NV12);
		h264d_gpu_display_write_rsp(pcWriteBuffer,
					       xWriteBufferLen,
					       (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
		return;
	}

	if (os_strcmp(argv[1], "start_rgb565") == 0 ||
	    os_strcmp(argv[1], "start_rgb888") == 0) {
		uint32_t loops = 0U;
		h264d_gpu_display_run_mode_t mode = (os_strcmp(argv[1], "start_rgb565") == 0) ?
			H264D_GPU_DISPLAY_RUN_FRAME_RGB565 : H264D_GPU_DISPLAY_RUN_FRAME_RGB888;

		if (argc >= 3) {
			loops = (uint32_t)os_strtoul(argv[2], NULL, 10);
		}
		ret = h264d_gpu_display_start_mode(loops, mode);
		h264d_gpu_display_write_rsp(pcWriteBuffer,
					       xWriteBufferLen,
					       (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
		return;
	}

	if (os_strcmp(argv[1], "stop") == 0) {
		ret = h264d_gpu_display_stop_task();
		h264d_gpu_display_write_rsp(pcWriteBuffer,
					       xWriteBufferLen,
					       (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
		return;
	}

#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP
	if (os_strcmp(argv[1], "isp_open") == 0) {
		ret = cli_h264d_gpu_display_isp_open(argc, argv);
		h264d_gpu_display_write_rsp(pcWriteBuffer,
					       xWriteBufferLen,
					       (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
		return;
	}
	if (os_strcmp(argv[1], "isp_close") == 0) {
		h264d_gpu_display_isp_close();
		h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
		return;
	}
#endif

	h264d_gpu_display_print_usage();
	h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
}

int cli_h264d_gpu_display_init(void)
{
	static const struct cli_command s_h264d_gpu_display_cmds[] = {
		{"h264d_gpu_display", "h264d_gpu_display help|start [loops]|stop|isp_open|isp_close", cli_h264d_gpu_display_cmd},
	};

	return cli_register_commands(s_h264d_gpu_display_cmds, sizeof(s_h264d_gpu_display_cmds) / sizeof(s_h264d_gpu_display_cmds[0]));
}
