/*
 * Standalone PP test.
 *
 * Produces an NV12 picture by decoding an embedded 1280x720 H.264 stream
 * (bk_h264_decode, used only to prepare test input), then feeds that NV12
 * buffer into the PP module (bk_pp_process) for scaling and/or format conversion.
 */

#include <stdint.h>
#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include "components/log.h"
#include "components/bk_frame_buffer.h"
#include <common/avdk_pixel_types.h>
#include "components/bk_decode/bk_h264_decode_ctlr.h"
#include "components/bk_decode/bk_pp_ctlr.h"
#include "h264_decode_stream_1280x720.h"
#include "h264_decode_h264_parser.h"
#include "pp_test.h"
#include "pp_config.h"
#if PP_EXAMPLE_ENABLE_MIPI_DISPLAY
#include "pp_dpu.h"
#include "pp_gpu_blit.h"
#endif

#define TAG "pp_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

#define PP_EXAMPLE_ROUNDS                 3U

#define PP_EXAMPLE_OUT_DOWN_W             640U
#define PP_EXAMPLE_OUT_DOWN_H             360U
#define PP_EXAMPLE_OUT_UP_W               1920U
#define PP_EXAMPLE_OUT_UP_H               1080U

#define PP_BOOT_DEMO_TASK_PRIORITY        3
#define PP_BOOT_DEMO_TASK_STACK_SIZE      (1024 * 16)
#define PP_DISPLAY_FRAME_INTERVAL_MS      33U
#define PP_DISPLAY_CLI_FRAME_LOOPS        90U
#define PP_DISPLAY_PANEL_SETTLE_MS        200U

static beken_thread_t s_pp_boot_demo_thread = NULL;
static volatile uint8_t s_pp_boot_demo_running = 0;
static volatile uint8_t s_pp_display_keep_panel_on = 0U;

static void pp_log_result(const char *case_name, uint8_t pass,
	const char *stage, avdk_err_t ret, uint32_t done_rounds, uint32_t total_rounds)
{
	if (pass) {
		LOGI("[RESULT][PASS] %s success, rounds=%u/%u\r\n",
			case_name, (unsigned)done_rounds, (unsigned)total_rounds);
	} else {
		LOGE("[RESULT][FAIL] %s failed at %s, ret=%d, rounds=%u/%u\r\n",
			case_name, stage, ret, (unsigned)done_rounds, (unsigned)total_rounds);
	}
}

static uint16_t pp_out_h_storage(uint32_t out_format, uint16_t out_h)
{
	if (out_format == BK_PIXEL_FORMAT_RGB565 ||
	    out_format == BK_PIXEL_FORMAT_RGB888) {
		return (uint16_t)((out_h + 15U) & ~15U);
	}

	return (uint16_t)((out_h + 1U) & ~1U);
}

static uint32_t pp_output_size(uint32_t out_format, uint16_t out_w, uint16_t out_h)
{
	uint16_t h_storage = pp_out_h_storage(out_format, out_h);

	switch (out_format) {
	case BK_PIXEL_FORMAT_RGB565:
		return (uint32_t)out_w * (uint32_t)h_storage * 2U;
	case BK_PIXEL_FORMAT_RGB888:
		return (uint32_t)out_w * (uint32_t)h_storage * 4U;
	case BK_PIXEL_FORMAT_NV12:
	default:
		return (uint32_t)out_w * (uint32_t)h_storage * 3U / 2U;
	}
}

static void pp_destroy_decoder(bk_h264_decode_ctlr_handle_t *dec)
{
	if (dec == NULL || *dec == NULL) {
		return;
	}
	(void)bk_h264_decode_close(*dec);
	(void)bk_h264_decode_deinit(*dec);
	(void)bk_h264_decode_delete(*dec);
	*dec = NULL;
}

/* Decode the first access unit of the embedded 1280x720 H.264 stream to NV12. */
static avdk_err_t pp_decode_nv12(uint8_t **nv12_buf, uint32_t *nv12_size,
					 uint16_t *width, uint16_t *height)
{
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint32_t out_size;
	avdk_err_t ret;
	uint32_t offset = 0U;
	const uint8_t *au_ptr = NULL;
	uint32_t au_size = 0U;
	bk_h264_decode_input_t in = {0};
	bk_h264_decode_info_t info = {0};

	out_size = pp_output_size(BK_PIXEL_FORMAT_NV12,
					  PP_EXAMPLE_SRC_WIDTH, PP_EXAMPLE_SRC_HEIGHT);

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
						       h264_decode_stream_1280x720_1i30p_bytes);
	if (stream_buf == NULL) {
		LOGE("alloc stream buffer failed\r\n");
		return AVDK_ERR_NOMEM;
	}
	os_memcpy(stream_buf, h264_decode_stream_1280x720_1i30p,
		  h264_decode_stream_1280x720_1i30p_bytes);

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		LOGE("alloc nv12 buffer failed, size=%u\r\n", (unsigned)out_size);
		ret = AVDK_ERR_NOMEM;
		goto fail;
	}
	os_memset(out_buf, 0, out_size);

	{
		bk_h264_decode_frame_config_t cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;

		cfg.timeout_ms = 2000U;
		cfg.out_width = PP_EXAMPLE_SRC_WIDTH;
		cfg.out_height = PP_EXAMPLE_SRC_HEIGHT;
		cfg.out_format = BK_PIXEL_FORMAT_NV12;
		ret = bk_h264_decode_frame_ctlr_new(&dec, &cfg);
		if (ret != AVDK_ERR_OK) {
			LOGE("frame ctlr new failed, ret=%d\r\n", ret);
			goto fail;
		}
	}
	ret = bk_h264_decode_init(dec);
	if (ret != AVDK_ERR_OK) {
		LOGE("decoder init failed, ret=%d\r\n", ret);
		goto fail;
	}
	ret = bk_h264_decode_open(dec);
	if (ret != AVDK_ERR_OK) {
		LOGE("decoder open failed, ret=%d\r\n", ret);
		goto fail;
	}

	ret = h264_decode_h264_next_au(stream_buf, h264_decode_stream_1280x720_1i30p_bytes,
				       &offset, &au_ptr, &au_size);
	if (ret != AVDK_ERR_OK || au_ptr == NULL || au_size == 0U) {
		LOGE("parse first au failed, ret=%d\r\n", ret);
		ret = AVDK_ERR_GENERIC;
		goto fail;
	}

	in.stream = (uint8_t *)(uintptr_t)au_ptr;
	in.stream_len = au_size;
	in.out_buffer = out_buf;
	in.out_buffer_size = out_size;
	ret = bk_h264_decode_frame(dec, &in);
	if (ret != AVDK_ERR_OK) {
		LOGE("h264 decode frame failed, ret=%d\r\n", ret);
		goto fail;
	}

	ret = bk_h264_decode_get_info(dec, &info);
	if (ret != AVDK_ERR_OK) {
		LOGE("get decode info failed, ret=%d\r\n", ret);
		goto fail;
	}
	if (info.width != PP_EXAMPLE_SRC_WIDTH || info.height != PP_EXAMPLE_SRC_HEIGHT) {
		LOGE("decoded size mismatch: got %ux%u expect %ux%u\r\n",
		     (unsigned)info.width, (unsigned)info.height,
		     (unsigned)PP_EXAMPLE_SRC_WIDTH, (unsigned)PP_EXAMPLE_SRC_HEIGHT);
		ret = AVDK_ERR_GENERIC;
		goto fail;
	}

	pp_destroy_decoder(&dec);
	bk_frame_buffer_free(stream_buf);

	*nv12_buf = out_buf;
	*nv12_size = out_size;
	*width = PP_EXAMPLE_SRC_WIDTH;
	*height = PP_EXAMPLE_SRC_HEIGHT;
	LOGI("decode nv12 ok: %ux%u size=%u\r\n",
		(unsigned)PP_EXAMPLE_SRC_WIDTH, (unsigned)PP_EXAMPLE_SRC_HEIGHT,
		(unsigned)out_size);
	return AVDK_ERR_OK;

fail:
	pp_destroy_decoder(&dec);
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}
	return ret;
}

#if PP_EXAMPLE_ENABLE_MIPI_DISPLAY
static avdk_err_t pp_display_frame_free(void *ptr)
{
	if (ptr != NULL) {
		bk_frame_buffer_free(ptr);
	}

	return AVDK_ERR_OK;
}

static uint32_t pp_buffer_hash(const uint8_t *buf, uint32_t size)
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

/*
 * Continuously blit + flush to MIPI, mirroring h264d_gpu_display_run_dec_scale().
 * max_loops == 0 means run forever (boot demo keeps the panel lit).
 */
static avdk_err_t pp_display_play(uint8_t *buf, uint16_t width, uint16_t height,
				  uint32_t out_format, uint32_t max_loops, uint8_t keep_panel_on)
{
	avdk_err_t ret;
	uint32_t loop = 0U;

	ret = pp_dpu_open();
	if (ret != AVDK_ERR_OK) {
		LOGE("dpu open failed, ret=%d\r\n", ret);
		return ret;
	}

	rtos_delay_milliseconds(PP_DISPLAY_PANEL_SETTLE_MS);
	LOGI("display play start: src=%ux%u fmt=%u panel=%ux%u loops=%u keep=%u hash=0x%08x\r\n",
	     (unsigned)width, (unsigned)height, (unsigned)out_format,
	     (unsigned)pp_dpu_width(), (unsigned)pp_dpu_height(),
	     (unsigned)max_loops, (unsigned)keep_panel_on,
	     (unsigned)pp_buffer_hash(buf, 4096U));

	while (max_loops == 0U || loop < max_loops) {
		void *display_frame = NULL;
		uint32_t display_frame_size = 0U;

		ret = pp_gpu_blit_rgb_frame(buf, width, height, (bk_pixel_format_t)out_format,
					    &display_frame, &display_frame_size);
		if (ret != AVDK_ERR_OK) {
			LOGE("gpu blit failed at loop=%u, ret=%d\r\n", (unsigned)loop, ret);
			break;
		}

		ret = pp_dpu_flush(display_frame, pp_display_frame_free);
		if (ret != AVDK_ERR_OK) {
			LOGE("dpu flush failed at loop=%u, ret=%d\r\n", (unsigned)loop, ret);
			(void)pp_display_frame_free(display_frame);
			break;
		}

		loop++;
		if ((loop == 1U) || ((loop % 30U) == 0U)) {
			LOGI("display loop %u flushed\r\n", (unsigned)loop);
		}
		rtos_delay_milliseconds(PP_DISPLAY_FRAME_INTERVAL_MS);
	}

	if (keep_panel_on == 0U) {
		pp_dpu_close();
		pp_gpu_blit_deinit();
	} else {
		LOGI("display keep panel on, loop=%u\r\n", (unsigned)loop);
	}

	return ret;
}
#endif /* PP_EXAMPLE_ENABLE_MIPI_DISPLAY */

static void pp_run_case(uint32_t out_format, uint16_t out_w, uint16_t out_h,
				const char *case_name)
{
	uint8_t *nv12_buf = NULL;
	uint8_t *out_buf = NULL;
	uint32_t nv12_size = 0U;
	uint16_t width = 0U;
	uint16_t height = 0U;
	uint32_t out_size;
	uint32_t done_rounds = 0U;
	uint8_t test_pass = 0U;
	const char *fail_stage = "start";
	avdk_err_t ret;
	uint32_t i;
	bk_pp_ctlr_handle_t pp = NULL;
	bk_pp_config_t pp_cfg = DEFAULT_PP_CONFIG;
	uint8_t pp_opened = 0U;

	LOGI("%s start: %ux%u -> %ux%u format=%u\r\n",
		case_name, (unsigned)PP_EXAMPLE_SRC_WIDTH, (unsigned)PP_EXAMPLE_SRC_HEIGHT,
		(unsigned)out_w, (unsigned)out_h, (unsigned)out_format);

	ret = pp_decode_nv12(&nv12_buf, &nv12_size, &width, &height);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decode_nv12";
		goto cleanup;
	}

	out_size = pp_output_size(out_format, out_w, out_h);
	if (out_size == 0U) {
		fail_stage = "out_size";
		ret = AVDK_ERR_INVAL;
		goto cleanup;
	}

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		fail_stage = "alloc_out_buf";
		ret = AVDK_ERR_NOMEM;
		LOGE("alloc output buffer failed, size=%u\r\n", (unsigned)out_size);
		goto cleanup;
	}

	pp_cfg.timeout_ms = 2000U;
	ret = bk_pp_ctlr_new(&pp, &pp_cfg);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "pp_new";
		goto cleanup;
	}
	ret = bk_pp_init(pp);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "pp_init";
		goto cleanup;
	}
	ret = bk_pp_open(pp);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "pp_open";
		goto cleanup;
	}
	pp_opened = 1U;

	for (i = 0U; i < PP_EXAMPLE_ROUNDS; i++) {
		os_memset(out_buf, 0, out_size);

		bk_pp_process_req_t req = {0};
		req.in_y = nv12_buf;
		req.in_c = nv12_buf + (uint32_t)width * (uint32_t)height;
		req.in_width = width;
		req.in_height = height;
		req.in_format = BK_PIXEL_FORMAT_NV12;
		req.out_width = out_w;
		req.out_height = out_h;
		req.out_buffer = out_buf;
		req.out_size = out_size;
		req.out_format = out_format;

		ret = bk_pp_process(pp, &req);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "pp_process";
			LOGE("pp round %u failed, ret=%d\r\n", (unsigned)(i + 1U), ret);
			goto cleanup;
		}
		done_rounds++;
		LOGI("%s round %u/%u done\r\n", case_name,
			(unsigned)(i + 1U), (unsigned)PP_EXAMPLE_ROUNDS);
	}

	test_pass = 1U;

#if PP_EXAMPLE_ENABLE_MIPI_DISPLAY
	if (test_pass != 0U) {
		uint32_t display_loops = s_pp_display_keep_panel_on ?
			0U : PP_DISPLAY_CLI_FRAME_LOOPS;
		uint8_t keep_panel_on = s_pp_display_keep_panel_on;
		avdk_err_t disp_ret = pp_display_play(out_buf, out_w, out_h, out_format,
						      display_loops, keep_panel_on);

		if (disp_ret != AVDK_ERR_OK) {
			test_pass = 0U;
			fail_stage = "display";
			ret = disp_ret;
		}
	}
#endif

cleanup:
	if (pp != NULL) {
		if (pp_opened) {
			(void)bk_pp_close(pp);
		}
		(void)bk_pp_deinit(pp);
		(void)bk_pp_delete(pp);
		pp = NULL;
	}
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
	}
	if (nv12_buf != NULL) {
		bk_frame_buffer_free(nv12_buf);
	}
	pp_log_result(case_name, test_pass, fail_stage, ret,
			      done_rounds, PP_EXAMPLE_ROUNDS);
}

void pp_nv12_rgb565_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_RGB565,
			    PP_EXAMPLE_SRC_WIDTH, PP_EXAMPLE_SRC_HEIGHT,
			    "pp_nv12_rgb565");
}

void pp_nv12_rgb888_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_RGB888,
			    PP_EXAMPLE_SRC_WIDTH, PP_EXAMPLE_SRC_HEIGHT,
			    "pp_nv12_rgb888");
}

void pp_nv12_scale_down_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_NV12,
			    PP_EXAMPLE_OUT_DOWN_W, PP_EXAMPLE_OUT_DOWN_H,
			    "pp_nv12_scale_down");
}

void pp_nv12_scale_up_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_NV12,
			    PP_EXAMPLE_OUT_UP_W, PP_EXAMPLE_OUT_UP_H,
			    "pp_nv12_scale_up");
}

void pp_nv12_rgb565_down_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_RGB565,
			    PP_EXAMPLE_OUT_DOWN_W, PP_EXAMPLE_OUT_DOWN_H,
			    "pp_nv12_rgb565_down");
}

void pp_nv12_rgb888_down_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_RGB888,
			    PP_EXAMPLE_OUT_DOWN_W, PP_EXAMPLE_OUT_DOWN_H,
			    "pp_nv12_rgb888_down");
}

void pp_nv12_rgb565_up_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_RGB565,
			    PP_EXAMPLE_OUT_UP_W, PP_EXAMPLE_OUT_UP_H,
			    "pp_nv12_rgb565_up");
}

void pp_nv12_rgb888_up_test(void)
{
	pp_run_case(BK_PIXEL_FORMAT_RGB888,
			    PP_EXAMPLE_OUT_UP_W, PP_EXAMPLE_OUT_UP_H,
			    "pp_nv12_rgb888_up");
}

static void pp_boot_demo_task_entry(void *arg)
{
	(void)arg;

	rtos_delay_milliseconds(1000);
	pp_nv12_rgb565_down_test();

	s_pp_boot_demo_running = 0;
	s_pp_boot_demo_thread = NULL;
	rtos_delete_thread(NULL);
}

void pp_run_boot_demo(void)
{
	bk_err_t ret;

	if (s_pp_boot_demo_running) {
		LOGE("pp boot demo already running\r\n");
		return;
	}

	s_pp_boot_demo_running = 1;
	s_pp_display_keep_panel_on = 0U;
	ret = rtos_core0_create_thread(&s_pp_boot_demo_thread,
		PP_BOOT_DEMO_TASK_PRIORITY,
		"pp_boot_demo",
		(beken_thread_function_t)pp_boot_demo_task_entry,
		PP_BOOT_DEMO_TASK_STACK_SIZE,
		NULL);
	if (ret != BK_OK) {
		LOGE("create pp boot demo task failed, ret=%d\r\n", (int)ret);
		s_pp_boot_demo_running = 0;
		s_pp_display_keep_panel_on = 0U;
		s_pp_boot_demo_thread = NULL;
	}
}
