/*
 * JPEG decode test: uses bk_jpeg_decode_* (bk_decoder component) which wraps vcdec HAL.
 * Image dimensions come from bk_jpeg_decode_get_img_info() before decode.
 */

#include <stdint.h>
#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include "components/log.h"
#include "components/bk_frame_buffer.h"
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "bk_flexa_bond_types.h"
#include "jpeg_176_144.h"

#define TAG "vcdec_jpeg_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define VCDEC_JPEG_DECODE_CNT 5U

#define OUT_DUMP_ENABLE 0

#define VCDEC_BOOT_DEMO_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define VCDEC_BOOT_DEMO_TASK_STACK_SIZE  (1024 * 16)

#ifdef OUT_DUMP_ENABLE
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
#endif

/*
 * Frame-done callback is kept to show the complete controller configuration path.
 * This demo uses the synchronous decode API, so the callback only prints errors.
 */
static void vcdec_jpeg_frame_done_cb(int status, void *args)
{
	(void)args;

	if (status != 0) {
		LOGE("frame done callback reported status=%d\r\n", status);
	}
}

typedef struct {
	/* Final full-frame NV12 output buffer. */
	uint8_t *frame_y;
	uint8_t *frame_c;
	uint32_t frame_width;
	uint32_t frame_height;
	/* FLEXA ping-pong output buffer exported by hardware. */
	uint8_t *pp_y;
	uint8_t *pp_c;
	uint32_t pp_seg_num;
	uint32_t pp_seg_rows;
	bk_jpeg_decode_ctlr_handle_t dec;
} vcdec_jpeg_flexa_copy_ctx_t;

static vcdec_jpeg_flexa_copy_ctx_t s_flexa_copy_ctx;
static beken_thread_t s_vcdec_boot_demo_thread = NULL;
static volatile uint8_t s_vcdec_boot_demo_running = 0;

// if flexa_done_cb is NULL, flexa_done_cb will not be called
// then shold send BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR in flexa_done_cb
// even the flexa_done_cb is NULL, the bond should be registered, and unregistered in the end
static bk_flexa_bond_t s_jpeg_flexa_bond = {0};

/* Release controller resources safely so cleanup paths stay readable. */
static void vcdec_jpeg_destroy_decoder(bk_jpeg_decode_ctlr_handle_t *dec)
{
	if (dec == NULL || *dec == NULL) {
		return;
	}

	(void)bk_jpeg_decode_close(*dec);
	(void)bk_jpeg_decode_deinit(*dec);
	(void)bk_jpeg_decode_delete(*dec);
	*dec = NULL;
}

static void vcdec_jpeg_log_test_result(const char *case_name, uint8_t pass,
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

void vcdec_jpeg_frame_test(void)
{
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint32_t out_width = 0U;
	uint32_t out_height = 0U;
	uint32_t out_size;
	avdk_err_t ret;
	uint32_t i;
	uint32_t done_rounds = 0U;
	uint8_t test_pass = 0U;
	const char *fail_stage = "start";
	bk_jpeg_decode_ctlr_handle_t dec = NULL;

	bk_jpeg_decode_img_info_t img_info = {0};
	LOGI("normal jpeg decode demo start\r\n");

	img_info.input_stream = (uint8_t *)(uintptr_t)(const void *)JPEGData_176_144;
	img_info.input_stream_length = JPEG_176_144_SIZE;
	ret = bk_jpeg_decode_get_img_info(&img_info);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "get_img_info";
		LOGE("get image info failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	out_width = img_info.width;
	out_height = img_info.height;
	out_size = out_width * out_height * 3U / 2U;
	LOGI("image info: width=%u height=%u out_size=%u format=NV12\r\n",
		(unsigned)out_width, (unsigned)out_height, (unsigned)out_size);

	/*
	 * VCDEC HW accesses buffers via bus master.
	 * Allocate stream/table/output buffers from video mem-slab heaps (PSRAM) to
	 * avoid DEC_BUS_INT caused by non-accessible SRAM heap addresses.
	 */
	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, JPEG_176_144_SIZE);
	if (stream_buf == NULL) {
		fail_stage = "alloc_stream_buf";
		ret = AVDK_ERR_NOMEM;
		LOGE("alloc stream buffer failed, size=%u\r\n", (unsigned)JPEG_176_144_SIZE);
		goto cleanup;
	}
	os_memcpy(stream_buf, (const void *)JPEGData_176_144, JPEG_176_144_SIZE);
	LOGI("stream buffer ready, size=%u\r\n", (unsigned)JPEG_176_144_SIZE);

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		fail_stage = "alloc_out_buf";
		ret = AVDK_ERR_NOMEM;
		LOGE("alloc output buffer failed, size=%u\r\n", (unsigned)out_size);
		goto cleanup;
	}
	os_memset(out_buf, 0, out_size);
	LOGI("output buffer ready, size=%u\r\n", (unsigned)out_size);

	/* Create a frame-controller path: full-frame output is written directly to out_buf. */
	bk_jpeg_decode_frame_config_t cfg = DEFAULT_JPEG_DECODE_FRAME_CONFIG;
	cfg.frame_done_cb = vcdec_jpeg_frame_done_cb;
	cfg.frame_done_args = NULL;
	cfg.timeout_ms = 1000;
	cfg.out_width = out_width;
	cfg.out_height = out_height;
	cfg.out_format = BK_PIXEL_FORMAT_NV12;
	ret = bk_jpeg_decode_frame_ctlr_new(&dec, &cfg);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "frame_ctlr_new";
		LOGE("create frame controller failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	ret = bk_jpeg_decode_init(dec);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decoder_init";
		LOGE("decoder init failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	ret = bk_jpeg_decode_open(dec);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decoder_open";
		LOGE("decoder open failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	LOGI("decoder open success, repeat=%u\r\n", (unsigned)VCDEC_JPEG_DECODE_CNT);

	for (i = 0; i < VCDEC_JPEG_DECODE_CNT; i++) {
		LOGI("decode round %u/%u start\r\n",
			(unsigned)(i + 1U), (unsigned)VCDEC_JPEG_DECODE_CNT);
		bk_jpeg_decode_input_t in = {0};
		in.stream = stream_buf;
		in.stream_len = JPEG_176_144_SIZE;
		in.out_buffer = out_buf;
		in.out_buffer_size = out_size;
		ret = bk_jpeg_decode_frame(dec, &in);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "decode_frame";
			LOGE("decode round %u failed, ret=%d\r\n", (unsigned)(i + 1U), ret);
			goto cleanup;
		}
		#if OUT_DUMP_ENABLE
		LOGI("decode round %u/%u dump\r\n",
			(unsigned)(i + 1U), (unsigned)VCDEC_JPEG_DECODE_CNT);
		stack_mem_dump((uint32_t)out_buf, (uint32_t)(out_buf+out_size));
		#endif
		done_rounds++;
		LOGI("decode round %u/%u done\r\n",
			(unsigned)(i + 1U), (unsigned)VCDEC_JPEG_DECODE_CNT);
	}

#if OUT_DUMP_ENABLE
	LOGI("dump output buffer for inspection: addr=%p size=%u\r\n", out_buf, (unsigned)out_size);
	stack_mem_dump((uint32_t)out_buf, (uint32_t)(out_buf+out_size));
#endif
	test_pass = 1U;
	vcdec_jpeg_destroy_decoder(&dec);
	LOGI("normal jpeg decode demo complete\r\n");

cleanup:
	vcdec_jpeg_destroy_decoder(&dec);
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
		LOGI("free output buffer\r\n");
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
		LOGI("free stream buffer\r\n");
	}
	vcdec_jpeg_log_test_result("vcdec_jpeg_test", test_pass, fail_stage, ret,
		done_rounds, VCDEC_JPEG_DECODE_CNT);

}

static void bond_jpegd_flexa_done_cb(uint32_t wrCnt, void *args)
{
	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
	if (in_stream == NULL)
	{
		return;
	}
	bk_jpeg_decode_ctlr_handle_t dec = (bk_jpeg_decode_ctlr_handle_t)in_stream->handle;
	if (dec == NULL)
	{
		return;
	}
	// this shoule be send to out_stream, but out_stream is NULL
	// so we set BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR directly
	if (wrCnt == 0)
	{
		wrCnt = in_stream->max_lines_per_frame;
	}
	in_stream->last_lines = wrCnt;
	bk_jpeg_decode_port_rd_t rd_cmd = {
		.port_ptr = &s_jpeg_flexa_bond,
		.rd_blocks = wrCnt,
	};
	bk_jpeg_decode_ioctl(dec, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
	if(wrCnt == in_stream->max_lines_per_frame)
	{
		//this should set by out_stream frame_done_cb, but out_stream is NULL
		// so we set BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE directly
		bk_jpeg_decode_ioctl(dec, BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE, in_stream);
	}
}

static void bond_jpegd_frame_done_cb(uint32_t status, void *args)
{
	bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
	if (in_stream == NULL)
	{
		return;
	}
	bk_jpeg_decode_ctlr_handle_t dec = (bk_jpeg_decode_ctlr_handle_t)in_stream->handle;
	if (dec == NULL)
	{
		return;
	}
}

/*
 * FLEXA output callback:
		return;
	}
	bk_jpeg_decode_ctlr_handle_t dec = (bk_jpeg_decode_ctlr_handle_t)out_stream->handle;
	if (dec == NULL) {
		return;
	}
 * 1. The decoder writes one segment into the ping-pong buffer.
 * 2. CPU copies that segment into the final full-frame NV12 buffer.
 * 3. CPU advances rd_ptr so hardware can reuse the segment slot.
 */
static void vcdec_jpeg_flexa_done_cb(uint32_t wrCnt, void *args)
{
	LOGE("vcdec_jpeg_flexa_done_cb, wrCnt=%u\r\n", (unsigned)wrCnt);
	vcdec_jpeg_flexa_copy_ctx_t *ctx = (vcdec_jpeg_flexa_copy_ctx_t *)args;
	if (ctx == NULL)
	{
		ctx = &s_flexa_copy_ctx;
	}
	if (wrCnt == 0U)
	{
		wrCnt = (ctx->frame_height + (ctx->pp_seg_rows / ctx->pp_seg_num) - 1) / (ctx->pp_seg_rows / ctx->pp_seg_num);
	}

	if (ctx->frame_y == NULL || ctx->frame_c == NULL ||
		ctx->pp_y == NULL || ctx->pp_c == NULL ||
		ctx->frame_width == 0U || ctx->frame_height == 0U ||
		ctx->pp_seg_num == 0U || ctx->pp_seg_rows == 0U) {
		LOGE("invalid flexa ctx\r\n");
		return;
	}

#if OUT_DUMP_ENABLE
	/* rd is the segment index counter already completed (rd_ptr before increment). */
	const uint32_t rd = wrCnt - 1U;
	const uint32_t seg_idx = rd % ctx->pp_seg_num;
	const uint32_t start_row = rd * 16U;
	if (start_row >= ctx->frame_height) {
		LOGD("skip flexa segment rd=%u start_row=%u >= frame_height=%u\r\n",
			(unsigned)rd, (unsigned)start_row, (unsigned)ctx->frame_height);
		return;
	}

	uint32_t copy_rows = 16U;
	if (start_row + copy_rows > ctx->frame_height)
		copy_rows = ctx->frame_height - start_row;

	const uint32_t w = ctx->frame_width;
	const uint32_t seg_y_stride = w * 16U;
	const uint32_t seg_c_stride = seg_y_stride / 2U;

	uint8_t *src_y = ctx->pp_y + seg_idx * seg_y_stride;
	uint8_t *src_c = ctx->pp_c + seg_idx * seg_c_stride;
	uint8_t *dst_y = ctx->frame_y + start_row * w;
	uint8_t *dst_c = ctx->frame_c + (start_row * w) / 2U;

	os_memcpy(dst_y, src_y, w * copy_rows);
	os_memcpy(dst_c, src_c, (w * copy_rows) / 2U);

	LOGD("flexa copy done: wrCnt=%u seg_idx=%u start_row=%u rows=%u\r\n",
		(unsigned)wrCnt, (unsigned)seg_idx, (unsigned)start_row, (unsigned)copy_rows);
#endif
}

void vcdec_jpeg_flexa_test(void)
{
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint8_t *pp_buf = NULL;
	uint32_t out_width = 0U;
	uint32_t out_height = 0U;
	uint32_t out_size;
	uint32_t pp_size;
	avdk_err_t ret;
	uint32_t i;
	uint32_t done_rounds = 0U;
	uint8_t test_pass = 0U;
	const char *fail_stage = "start";

	/* PP output ring-buffer: 2 segments, 1 MB (16 lines) per segment. */
	const uint32_t seg_ht_mb = 1U;
	const uint32_t seg_num = 2U;
	const uint32_t seg_rows = 16U * seg_ht_mb * seg_num;
	bk_jpeg_decode_img_info_t img_info = {0};

	LOGI("flexa jpeg decode demo start\r\n");

	img_info.input_stream = (uint8_t *)(uintptr_t)(const void *)JPEGData_176_144;
	img_info.input_stream_length = JPEG_176_144_SIZE;
	ret = bk_jpeg_decode_get_img_info(&img_info);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "get_img_info";
		LOGE("get image info failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	out_width = img_info.width;
	out_height = img_info.height;
	/* Full-frame output buffer (Y + C). */
	out_size = out_width * out_height * 3U / 2U;
	/* Pingpong buffer for HW Flexa output (2 x 16 rows). */
	pp_size = out_width * seg_rows * 3U / 2U;
	LOGI("image info: width=%u height=%u out_size=%u pp_size=%u seg_num=%u seg_rows=%u\r\n",
		(unsigned)out_width, (unsigned)out_height, (unsigned)out_size,
		(unsigned)pp_size, (unsigned)seg_num, (unsigned)seg_rows);

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, JPEG_176_144_SIZE);
	if (stream_buf == NULL) {
		fail_stage = "alloc_stream_buf";
		ret = AVDK_ERR_NOMEM;
		LOGE("alloc stream buffer failed, size=%u\r\n", (unsigned)JPEG_176_144_SIZE);
		goto cleanup;
	}
	os_memcpy(stream_buf, (const void *)JPEGData_176_144, JPEG_176_144_SIZE);
	LOGI("stream buffer ready, size=%u\r\n", (unsigned)JPEG_176_144_SIZE);

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		fail_stage = "alloc_out_buf";
		ret = AVDK_ERR_NOMEM;
		LOGE("alloc full-frame buffer failed, size=%u\r\n", (unsigned)out_size);
		goto cleanup;
	}
	os_memset(out_buf, 0, out_size);
	LOGI("full-frame buffer ready, size=%u\r\n", (unsigned)out_size);

	/* Pingpong buffer (HW output ring-buffer): layout is [Y for seg_num][C for seg_num]. */
	pp_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, pp_size);
	if (pp_buf == NULL) {
		fail_stage = "alloc_pp_buf";
		ret = AVDK_ERR_NOMEM;
		LOGE("alloc ping-pong buffer failed, size=%u\r\n", (unsigned)pp_size);
		goto cleanup;
	}
	os_memset(pp_buf, 0, pp_size);
	LOGI("ping-pong buffer ready, size=%u\r\n", (unsigned)pp_size);

	s_flexa_copy_ctx.frame_y = out_buf;
	s_flexa_copy_ctx.frame_c = out_buf + out_width * out_height;
	s_flexa_copy_ctx.frame_width = out_width;
	s_flexa_copy_ctx.frame_height = out_height;
	s_flexa_copy_ctx.pp_y = pp_buf;
	s_flexa_copy_ctx.pp_c = pp_buf + out_width * seg_rows;
	s_flexa_copy_ctx.pp_seg_num = seg_num;
	s_flexa_copy_ctx.pp_seg_rows = seg_rows;
	LOGI("flexa copy ctx ready: frame_y=%p frame_c=%p pp_y=%p pp_c=%p\r\n",
		s_flexa_copy_ctx.frame_y, s_flexa_copy_ctx.frame_c,
		s_flexa_copy_ctx.pp_y, s_flexa_copy_ctx.pp_c);

	/*
	 * Create a FLEXA controller path:
	 * hardware writes segment data into pp_buf, and flexa_done_cb stitches it
	 * into the final full-frame NV12 buffer.
	 */
	bk_jpeg_decode_flexa_config_t cfg = DEFAULT_JPEG_DECODE_FLEXA_CONFIG;
	cfg.frame_done_cb = vcdec_jpeg_frame_done_cb;
	cfg.frame_done_args = NULL;
	cfg.flexa_done_cb = vcdec_jpeg_flexa_done_cb;
	cfg.flexa_done_args = &s_flexa_copy_ctx;
	cfg.timeout_ms = 1000;
	cfg.segment_height = (uint16_t)seg_ht_mb;
	cfg.segment_number = (uint8_t)seg_num;
	cfg.out_width = out_width;
	cfg.out_height = out_height;
	cfg.out_format = BK_PIXEL_FORMAT_NV12;
	ret = bk_jpeg_decode_flexa_ctlr_new(&s_flexa_copy_ctx.dec, &cfg);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "flexa_ctlr_new";
		LOGE("create flexa controller failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	ret = bk_jpeg_decode_init(s_flexa_copy_ctx.dec);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decoder_init";
		LOGE("decoder init failed, ret=%d\r\n", ret);
		goto cleanup;
	}

	ret = bk_jpeg_decode_open(s_flexa_copy_ctx.dec);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decoder_open";
		LOGE("decoder open failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	LOGI("flexa decoder open success\r\n");

	s_jpeg_flexa_bond.flexa_done = bond_jpegd_flexa_done_cb;
	s_jpeg_flexa_bond.frame_done = bond_jpegd_frame_done_cb;
	s_jpeg_flexa_bond.bond_config = NULL;
	s_jpeg_flexa_bond.handle = (void *)s_flexa_copy_ctx.dec;
	s_jpeg_flexa_bond.max_lines_per_frame = (out_height + (16 * seg_ht_mb - 1)) / (16 * seg_ht_mb);

	ret = bk_jpeg_decode_ioctl(s_flexa_copy_ctx.dec, BK_JPEG_DECODE_IOCTL_REGISTER_BOND, &s_jpeg_flexa_bond);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "register_bond";
		LOGE("register flexa bond failed, ret=%d\r\n", ret);
		goto cleanup;
	}
	LOGI("flexa bond registered\r\n");

	for (i = 0; i < VCDEC_JPEG_DECODE_CNT; i++) {
		LOGI("flexa decode round %u/%u start\r\n",
			(unsigned)(i + 1U), (unsigned)VCDEC_JPEG_DECODE_CNT);
		os_memset(out_buf, 0, out_size);
		os_memset(pp_buf, 0, pp_size);

		bk_jpeg_decode_input_t in = {0};
		in.stream = stream_buf;
		in.stream_len = JPEG_176_144_SIZE;
		in.out_buffer = pp_buf;
		in.out_buffer_size = pp_size;
		ret = bk_jpeg_decode_frame(s_flexa_copy_ctx.dec, &in);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "decode_frame";
			LOGE("flexa decode round %u failed, ret=%d\r\n", (unsigned)(i + 1U), ret);
			goto cleanup;
		}
		done_rounds++;

		#if OUT_DUMP_ENABLE
		LOGI("flexa decode round %u/%u dump\r\n",
			(unsigned)(i + 1U), (unsigned)VCDEC_JPEG_DECODE_CNT);
		stack_mem_dump((uint32_t)out_buf, (uint32_t)(out_buf+out_size));
		#endif
		LOGI("flexa decode round %u/%u done\r\n",
			(unsigned)(i + 1U), (unsigned)VCDEC_JPEG_DECODE_CNT);
	}

	(void)bk_jpeg_decode_ioctl(s_flexa_copy_ctx.dec, BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND, &s_jpeg_flexa_bond);
	LOGI("flexa bond unregistered\r\n");
	test_pass = 1U;
	vcdec_jpeg_destroy_decoder(&s_flexa_copy_ctx.dec);
	LOGI("flexa jpeg decode demo complete\r\n");

cleanup:
	if (s_flexa_copy_ctx.dec != NULL) {
		(void)bk_jpeg_decode_ioctl(s_flexa_copy_ctx.dec, BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND, &s_jpeg_flexa_bond);
		LOGI("cleanup: flexa bond unregistered\r\n");
	}
	vcdec_jpeg_destroy_decoder(&s_flexa_copy_ctx.dec);
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
		LOGI("free full-frame buffer\r\n");
	}
	if (pp_buf != NULL) {
		bk_frame_buffer_free(pp_buf);
		LOGI("free ping-pong buffer\r\n");
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
		LOGI("free stream buffer\r\n");
	}

	os_memset(&s_flexa_copy_ctx, 0, sizeof(s_flexa_copy_ctx));
	LOGI("flexa copy ctx reset\r\n");
	vcdec_jpeg_log_test_result("vcdec_jpeg_flexa_test", test_pass, fail_stage, ret,
		done_rounds, VCDEC_JPEG_DECODE_CNT);
}


static void vcdec_jpeg_boot_demo_task_entry(void *arg)
{
	(void)arg;

	rtos_delay_milliseconds(1000);
	LOGI("vcdec_jpeg_flexa_test start\r\n");
	vcdec_jpeg_flexa_test();
	LOGI("vcdec_jpeg_flexa_test done\r\n");

	rtos_delay_milliseconds(1000);
	LOGI("vcdec_jpeg_frame_test start\r\n");
	vcdec_jpeg_frame_test();
	LOGI("vcdec_jpeg_frame_test done\r\n");

	s_vcdec_boot_demo_running = 0;
	s_vcdec_boot_demo_thread = NULL;
	rtos_delete_thread(NULL);
}

void vcdec_jpeg_run_boot_demo(void)
{
	bk_err_t ret;

	if (s_vcdec_boot_demo_running) {
		LOGE("vcdec boot demo task is already running\r\n");
		return;
	}

	s_vcdec_boot_demo_running = 1;
	ret = rtos_create_thread(&s_vcdec_boot_demo_thread,
		VCDEC_BOOT_DEMO_TASK_PRIORITY,
		"vcdec_boot_demo",
		(beken_thread_function_t)vcdec_jpeg_boot_demo_task_entry,
		VCDEC_BOOT_DEMO_TASK_STACK_SIZE,
		NULL);
	if (ret != BK_OK) {
		LOGE("create vcdec boot demo task failed, ret=%d\r\n", (int)ret);
		s_vcdec_boot_demo_running = 0;
		s_vcdec_boot_demo_thread = NULL;
		return;
	}

	LOGI("vcdec boot demo task created\r\n");
}
