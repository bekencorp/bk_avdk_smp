#include <stdint.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_encode/bk_jpeg_encode_ctlr.h>
#include <components/avdk_utils/avdk_error.h>

#include "jpeg_encode_test.h"
#include "../common/jpeg_encode_nv12_256x128.h"

#define TAG "bk_jpeg_enc_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define JPEG_ENC_TEST_WIDTH       (1920U)
#define JPEG_ENC_TEST_HEIGHT      (1080U)
#define JPEG_ENC_TEST_SRC_WIDTH   (256U)
#define JPEG_ENC_TEST_SRC_HEIGHT  (128U)
#define JPEG_ENC_TEST_INPUT_SIZE  (JPEG_ENC_TEST_WIDTH * JPEG_ENC_TEST_HEIGHT * 3U / 2U)
#define JPEG_ENC_TEST_SRC_SIZE    (JPEG_ENC_TEST_SRC_WIDTH * JPEG_ENC_TEST_SRC_HEIGHT * 3U / 2U)
#define JPEG_ENC_TEST_FRAME_CNT   (10U)
#define JPEG_ENC_TEST_QUALITY     (5U)
#define JPEG_ENC_TEST_TIMEOUT_MS  (3000U)
#define JPEG_ENC_TEST_FLEXA_BLOCK_LINES (16U)
#define JPEG_ENC_TEST_FLEXA_BLOCKS \
	((JPEG_ENC_TEST_HEIGHT + JPEG_ENC_TEST_FLEXA_BLOCK_LINES - 1U) / JPEG_ENC_TEST_FLEXA_BLOCK_LINES)

/* 1: 用 stack_mem_dump 打印编码后的 JPEG 比特流（与 vcdec_h264_test 中 DUMP 用法一致） */
#define JPEG_ENCODE_TEST_DUMP_ENABLE         0

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

typedef struct {
	beken_semaphore_t done_sem;
	bk_jpeg_encode_ctlr_handle_t handle;
	volatile uint32_t result;
	volatile uint32_t jpeg_size;
	uint32_t flexa_wr_blocks;
} bk_jpeg_encode_test_ctx_t;

static void bk_jpeg_encode_test_dump_jpeg(const char *tag, uint8_t *buf, uint32_t size)
{
#if JPEG_ENCODE_TEST_DUMP_ENABLE
	LOGI("dump %s encoded jpeg buf=%p size=%u\r\n", tag, buf, (unsigned)size);
	stack_mem_dump((uint32_t)(uintptr_t)buf, (uint32_t)(uintptr_t)(buf + size));
#else
	(void)tag;
	(void)buf;
	(void)size;
#endif
}

static void *bk_jpeg_test_outbuf_malloc(uint32_t size, void *args)
{
	(void)args;

	uint32_t frame_size = ((sizeof(frame_buffer_t) + size + 63) >> 6) << 6;
	frame_buffer_t *frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);

	if (frame == NULL) {
		LOGE("output frame malloc failed, size=%u\r\n", size);
		return NULL;
	}

	frame->frame = ((uint8_t *)frame) + (((sizeof(frame_buffer_t) + 63) >> 6) << 6);
	frame->size = size;
	frame->length = 0;
	frame->width = JPEG_ENC_TEST_WIDTH;
	frame->height = JPEG_ENC_TEST_HEIGHT;
	return frame->frame;
}

static uint32_t bk_jpeg_test_outbuf_complete(bk_jpeg_encode_outbuf_info_t *info)
{
	if (info == NULL) {
		return BK_FAIL;
	}

	bk_jpeg_encode_test_ctx_t *ctx = (bk_jpeg_encode_test_ctx_t *)info->args;

	if (ctx != NULL) {
		ctx->result = info->status;
		ctx->jpeg_size = info->length;
		if (ctx->done_sem != NULL) {
			rtos_set_semaphore(&ctx->done_sem);
		}
	}

	if (info->status == BK_OK)
		LOGI("jpeg callback ok, size=%u\r\n", info->length);
	else
		LOGE("jpeg callback fail, status=%u len=%u\r\n", info->status, info->length);

	if (info->status == BK_OK && info->length > 0U)
		bk_jpeg_encode_test_dump_jpeg("jpeg_encode_out", (uint8_t *)info->outbuf, info->length);

	if (info->outbuf == NULL) {
		LOGE("output buffer is NULL\r\n");
		return BK_FAIL;
	}

	uint32_t frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
	frame_buffer_t *frame = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
	bk_frame_buffer_free(frame);
	return BK_OK;
}

static void bk_jpeg_test_flexa_done(uint32_t rd_blocks, void *args)
{
	bk_jpeg_encode_test_ctx_t *ctx = (bk_jpeg_encode_test_ctx_t *)args;
	uint32_t next_wr_blocks;
	avdk_err_t ret;

	// LOGI("jpeg sw flexa done, rd=%u\r\n", (unsigned)rd_blocks);

	if (ctx == NULL || ctx->handle == NULL) {
		LOGE("jpeg sw flexa done invalid ctx, rd=%u\r\n", (unsigned)rd_blocks);
		return;
	}

	next_wr_blocks = rd_blocks + 1U;
	if (next_wr_blocks > JPEG_ENC_TEST_FLEXA_BLOCKS)
		next_wr_blocks = JPEG_ENC_TEST_FLEXA_BLOCKS;

	if (next_wr_blocks <= ctx->flexa_wr_blocks)
		return;

	ret = bk_jpeg_encode_ioctl(ctx->handle,
				   BK_JPEG_ENCODE_IOCTL_SET_FLEXA_LINES_READY,
				   (void *)(uintptr_t)next_wr_blocks);
	if (ret != AVDK_ERR_OK) {
		LOGE("advance jpeg sw flexa wr failed, rd=%u wr=%u ret=%d\r\n",
		     (unsigned)rd_blocks, (unsigned)next_wr_blocks, ret);
		return;
	}

	ctx->flexa_wr_blocks = next_wr_blocks;
	// LOGI("jpeg sw flexa advance wr, rd=%u wr=%u/%u\r\n",
	//      (unsigned)rd_blocks, (unsigned)next_wr_blocks,
	//      (unsigned)JPEG_ENC_TEST_FLEXA_BLOCKS);
}

static int bk_jpeg_wait_done(bk_jpeg_encode_test_ctx_t *ctx, const char *name)
{
	bk_err_t sem_ret;

	if (ctx == NULL || ctx->done_sem == NULL) {
		return BK_FAIL;
	}

	sem_ret = rtos_get_semaphore(&ctx->done_sem, JPEG_ENC_TEST_TIMEOUT_MS);
	if (sem_ret != BK_OK) {
		LOGE("%s wait encode done timeout, ret=%d\r\n", name, sem_ret);
		return BK_FAIL;
	}
	if (ctx->result != BK_OK) {
		LOGE("%s encode failed, result=%u\r\n", name, (unsigned)ctx->result);
		return BK_FAIL;
	}

	LOGI("%s encode done, size=%u\r\n", name, (unsigned)ctx->jpeg_size);
	return BK_OK;
}

static void bk_jpeg_test_copy_plane_tiled(uint8_t *dst, uint32_t dst_width,
					  uint32_t dst_height, const uint8_t *src,
					  uint32_t src_width, uint32_t src_height)
{
	for (uint32_t row = 0U; row < dst_height; row++) {
		uint8_t *dst_line = dst + row * dst_width;
		const uint8_t *src_line = src + (row % src_height) * src_width;

		for (uint32_t col = 0U; col < dst_width; col += src_width) {
			uint32_t copy_width = src_width;

			if (copy_width > (dst_width - col))
				copy_width = dst_width - col;

			os_memcpy(dst_line + col, src_line, copy_width);
		}
	}
}

static int bk_jpeg_test_copy_nv12_tiled(uint8_t *dst, uint32_t dst_size,
					const uint8_t *src, uint32_t src_size)
{
	uint32_t src_y_size = JPEG_ENC_TEST_SRC_WIDTH * JPEG_ENC_TEST_SRC_HEIGHT;
	uint32_t dst_y_size = JPEG_ENC_TEST_WIDTH * JPEG_ENC_TEST_HEIGHT;
	uint8_t *dst_uv = dst + dst_y_size;
	const uint8_t *src_uv = src + src_y_size;

	if (dst == NULL || src == NULL || dst_size != JPEG_ENC_TEST_INPUT_SIZE ||
	    src_size != JPEG_ENC_TEST_SRC_SIZE ||
	    (JPEG_ENC_TEST_WIDTH % 2U) != 0U || (JPEG_ENC_TEST_HEIGHT % 2U) != 0U) {
		return BK_FAIL;
	}

	bk_jpeg_test_copy_plane_tiled(dst, JPEG_ENC_TEST_WIDTH, JPEG_ENC_TEST_HEIGHT,
				      src, JPEG_ENC_TEST_SRC_WIDTH, JPEG_ENC_TEST_SRC_HEIGHT);
	bk_jpeg_test_copy_plane_tiled(dst_uv, JPEG_ENC_TEST_WIDTH, JPEG_ENC_TEST_HEIGHT / 2U,
				      src_uv, JPEG_ENC_TEST_SRC_WIDTH, JPEG_ENC_TEST_SRC_HEIGHT / 2U);

	return BK_OK;
}

static uint8_t *bk_jpeg_test_alloc_input(const uint8_t *src, uint32_t src_size)
{
	uint32_t input_size = JPEG_ENC_TEST_INPUT_SIZE;
	uint8_t *input = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, input_size + 32U);

	if (input == NULL) {
		LOGE("input malloc failed, size=%u\r\n", input_size);
		return NULL;
	}

	if (bk_jpeg_test_copy_nv12_tiled(input, input_size, src, src_size) != BK_OK) {
		LOGE("input NV12 tile copy failed, src_size=%u dst_size=%u\r\n", src_size, input_size);
		bk_frame_buffer_free(input);
		return NULL;
	}

	LOGI("input NV12 ready, addr=%p src_size=%u dst_size=%u %ux%u\r\n",
	     input, src_size, input_size, JPEG_ENC_TEST_WIDTH, JPEG_ENC_TEST_HEIGHT);
	return input;
}

static void bk_jpeg_log_result(const char *name, uint8_t pass, const char *stage, int ret,
			       uint32_t done_frames, uint32_t last_size)
{
	if (pass)
		LOGI("[RESULT][PASS] %s success, frames=%u/%u encoded_size=%u\r\n",
		     name, done_frames, JPEG_ENC_TEST_FRAME_CNT, last_size);
	else
		LOGE("[RESULT][FAIL] %s failed at %s ret=%d frames=%u/%u encoded_size=%u\r\n",
		     name, stage, ret, done_frames, JPEG_ENC_TEST_FRAME_CNT, last_size);
}

int bk_jpeg_encode_frame_test(void)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *input = NULL;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_frame_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	const char *fail_stage = "start";
	uint8_t pass = 0U;
	uint32_t done_frames = 0U;
	bk_jpeg_encode_input_t enc_in;

	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));
	os_memset(&enc_in, 0, sizeof(enc_in));

	LOGI("JPEG encode test start, frames=%u quality=%u\r\n",
	     (unsigned)JPEG_ENC_TEST_FRAME_CNT, (unsigned)JPEG_ENC_TEST_QUALITY);

	input = bk_jpeg_test_alloc_input(jpeg_encode_nv12_256x128, jpeg_encode_nv12_256x128_bytes);
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	config.width = JPEG_ENC_TEST_WIDTH;
	config.height = JPEG_ENC_TEST_HEIGHT;
	config.input_format = BK_PIXEL_FORMAT_NV12;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = JPEG_ENC_TEST_INPUT_SIZE;
	config.quality = JPEG_ENC_TEST_QUALITY;
	config.outbuf_malloc = bk_jpeg_test_outbuf_malloc;
	config.outbuf_complete = bk_jpeg_test_outbuf_complete;
	config.outbuf_complete_args = &ctx;

	avdk_ret = bk_jpeg_encode_frame_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "frame_new";
		ret = (int)avdk_ret;
		LOGE("bk_jpeg_encode_frame_new failed, ret=%d\r\n", avdk_ret);
		goto exit;
	}

	avdk_ret = bk_jpeg_encode_init(handle);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "init";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	avdk_ret = bk_jpeg_encode_open(handle);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "open";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	for (uint32_t i = 0U; i < JPEG_ENC_TEST_FRAME_CNT; i++) {
		ctx.result = (uint32_t)~0U;
		ctx.jpeg_size = 0U;

		enc_in.pic_buf = 0;
		enc_in.pic_lines = 0;
		enc_in.out_buf = 0;
		enc_in.out_size = 0;

		avdk_ret = bk_jpeg_encode_frame(handle, &enc_in);
		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "encode_frame";
			ret = (int)avdk_ret;
			LOGE("encode round %u failed, avdk_ret=%d\r\n", (unsigned)(i + 1U), avdk_ret);
			goto exit_destroy;
		}

		if (ctx.result != BK_OK) {
			fail_stage = "callback_status";
			ret = (int)ctx.result;
			LOGE("encode round %u bad status %u\r\n", (unsigned)(i + 1U), ctx.result);
			goto exit_destroy;
		}

		done_frames++;
		LOGI("encode round %u/%u done, size=%u\r\n",
		     (unsigned)(i + 1U), (unsigned)JPEG_ENC_TEST_FRAME_CNT, ctx.jpeg_size);
	}

	pass = 1U;
	ret = BK_OK;

exit_destroy:
	if (handle != NULL) {
		bk_jpeg_encode_close(handle);
		bk_jpeg_encode_deinit(handle);
		bk_jpeg_encode_delete(handle);
	}

exit:
	if (input != NULL)
		bk_frame_buffer_free(input);

	bk_jpeg_log_result("bk_jpeg_encode_frame_test", pass, fail_stage, ret, done_frames, ctx.jpeg_size);
	return ret;
}

int bk_jpeg_encode_sw_flexa_test(void)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *input = NULL;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_sw_flexa_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	const char *fail_stage = "start";
	uint8_t pass = 0U;
	uint32_t done_frames = 0U;

	LOGI("JPEG sw flexa encode test start, blocks=%u quality=%u frames=%u\r\n",
	     (unsigned)JPEG_ENC_TEST_FLEXA_BLOCKS, (unsigned)JPEG_ENC_TEST_QUALITY,
	     (unsigned)JPEG_ENC_TEST_FRAME_CNT);
	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));

	if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
		fail_stage = "init_done_sem";
		LOGE("init flexa done sem failed\r\n");
		bk_jpeg_log_result("bk_jpeg_encode_sw_flexa_test", 0U, fail_stage, BK_FAIL, 0U, 0U);
		return BK_FAIL;
	}

	input = bk_jpeg_test_alloc_input(jpeg_encode_nv12_256x128, jpeg_encode_nv12_256x128_bytes);
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	config.width = JPEG_ENC_TEST_WIDTH;
	config.height = JPEG_ENC_TEST_HEIGHT;
	config.input_format = BK_PIXEL_FORMAT_NV12;
	config.input_flexa_cnt = JPEG_ENC_TEST_FLEXA_BLOCKS;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = JPEG_ENC_TEST_INPUT_SIZE;
	config.quality = JPEG_ENC_TEST_QUALITY;
	config.outbuf_malloc = bk_jpeg_test_outbuf_malloc;
	config.outbuf_complete = bk_jpeg_test_outbuf_complete;
	config.outbuf_complete_args = &ctx;
	config.flexa_done = bk_jpeg_test_flexa_done;
	config.flexa_done_arg = &ctx;

	avdk_ret = bk_jpeg_encode_sw_flexa_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "sw_flexa_new";
		ret = (int)avdk_ret;
		LOGE("bk_jpeg_encode_sw_flexa_new failed, ret=%d\r\n", avdk_ret);
		goto exit;
	}
	ctx.handle = handle;
	LOGI("JPEG sw flexa controller created, blocks=%u\r\n",
	     (unsigned)JPEG_ENC_TEST_FLEXA_BLOCKS);

	avdk_ret = bk_jpeg_encode_init(handle);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "init";
		ret = (int)avdk_ret;
		LOGE("bk_jpeg_encode_init failed, ret=%d\r\n", avdk_ret);
		goto exit;
	}

	avdk_ret = bk_jpeg_encode_open(handle);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "open";
		ret = (int)avdk_ret;
		LOGE("bk_jpeg_encode_open failed, ret=%d\r\n", avdk_ret);
		goto exit;
	}

	for (uint32_t i = 0U; i < JPEG_ENC_TEST_FRAME_CNT; i++) {
		ctx.result = BK_FAIL;
		ctx.jpeg_size = 0U;
		ctx.flexa_wr_blocks = 2U;
		LOGI("JPEG sw flexa round %u/%u start, initial_ready_blocks=%u total_blocks=%u\r\n",
		     (unsigned)(i + 1U), (unsigned)JPEG_ENC_TEST_FRAME_CNT,
		     (unsigned)ctx.flexa_wr_blocks, (unsigned)JPEG_ENC_TEST_FLEXA_BLOCKS);

		avdk_ret = bk_jpeg_encode_ioctl(handle, BK_JPEG_ENCODE_IOCTL_SET_FRAME_READY, NULL);
		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "set_frame_ready";
			ret = (int)avdk_ret;
			LOGE("flexa round %u set frame ready failed, ret=%d\r\n",
			     (unsigned)(i + 1U), avdk_ret);
			goto exit;
		}

		fail_stage = "wait_done";
		ret = bk_jpeg_wait_done(&ctx, "jpeg flexa");
		if (ret != BK_OK) {
			LOGE("flexa round %u failed, ret=%d\r\n", (unsigned)(i + 1U), ret);
			goto exit;
		}

		done_frames++;
		LOGI("JPEG sw flexa round %u/%u done\r\n",
		     (unsigned)(i + 1U), (unsigned)JPEG_ENC_TEST_FRAME_CNT);
	}

	pass = 1U;
	ret = BK_OK;
	LOGI("JPEG sw flexa encode test complete\r\n");

exit:
	if (handle != NULL) {
		bk_jpeg_encode_close(handle);
		bk_jpeg_encode_deinit(handle);
		bk_jpeg_encode_delete(handle);
	}
	if (input != NULL)
		bk_frame_buffer_free(input);
	if (ctx.done_sem != NULL)
		rtos_deinit_semaphore(&ctx.done_sem);

	bk_jpeg_log_result("bk_jpeg_encode_sw_flexa_test", pass, fail_stage, ret,
			   done_frames, ctx.jpeg_size);
	return ret;
}

static beken_thread_t s_jpeg_boot_thread;
static volatile uint8_t s_jpeg_boot_running;

#define JPEG_BOOT_TASK_PRIO     (BEKEN_DEFAULT_WORKER_PRIORITY)
#define JPEG_BOOT_TASK_STACK    (1024 * 16)

static void vcenc_jpeg_boot_task_entry(void *arg)
{
	(void)arg;

	rtos_delay_milliseconds(1000);
	LOGI("boot demo: bk_jpeg_encode_frame_test\r\n");
	(void)bk_jpeg_encode_frame_test();
	LOGI("boot demo: bk_jpeg_encode_sw_flexa_test\r\n");
	(void)bk_jpeg_encode_sw_flexa_test();
	s_jpeg_boot_running = 0;
	s_jpeg_boot_thread = NULL;
	rtos_delete_thread(NULL);
}

void vcenc_jpeg_run_boot_demo(void)
{
	bk_err_t err;

	if (s_jpeg_boot_running)
		return;

	s_jpeg_boot_running = 1;
	err = rtos_create_thread(&s_jpeg_boot_thread,
				 JPEG_BOOT_TASK_PRIO,
				 "jpeg_enc_boot",
				 (beken_thread_function_t)vcenc_jpeg_boot_task_entry,
				 JPEG_BOOT_TASK_STACK,
				 NULL);
	if (err != BK_OK) {
		LOGE("create jpeg boot demo thread failed, ret=%d\r\n", err);
		s_jpeg_boot_running = 0;
		s_jpeg_boot_thread = NULL;
	}
}
