#include <stdint.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/system.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_encode/bk_jpeg_encode_ctlr.h>
#include <components/avdk_utils/avdk_error.h>
#include <driver/aon_rtc.h>

#include "jpeg_encode_test.h"
#include "jpeg_encode_osd.h"
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
#define JPEG_ENC_TEST_SW_FRAME_CNT      (3U)
#define JPEG_ENC_TEST_SW_FRAME_OUT_SIZE (512U * 1024U)
#define JPEG_ENC_TEST_QUALITY     (5U)
#define JPEG_ENC_TEST_SW_QUALITY (75U)
#define JPEG_ENC_TEST_TIMEOUT_MS  (3000U)
#define JPEG_ENC_TEST_FLEXA_BLOCK_LINES (16U)
#define JPEG_ENC_TEST_FLEXA_BLOCKS \
	((JPEG_ENC_TEST_HEIGHT + JPEG_ENC_TEST_FLEXA_BLOCK_LINES - 1U) / JPEG_ENC_TEST_FLEXA_BLOCK_LINES)
/*
 * SW/HW slice (flexa) mode derives the chroma address from
 * width * JPEG_ENC_TEST_FLEXA_BLOCK_LINES * input_flexa_cnt, i.e. the luma
 * plane is addressed as whole 16-line MB rows. A 1080-line frame therefore
 * has to live in a 1088-line tall buffer, otherwise the last MB row reads
 * past the Y plane and the last colour block is corrupted.
 */
#define JPEG_ENC_TEST_FLEXA_ALIGNED_HEIGHT \
	(JPEG_ENC_TEST_FLEXA_BLOCKS * JPEG_ENC_TEST_FLEXA_BLOCK_LINES)
#define JPEG_ENC_TEST_FLEXA_INPUT_SIZE \
	(JPEG_ENC_TEST_WIDTH * JPEG_ENC_TEST_FLEXA_ALIGNED_HEIGHT * 3U / 2U)

/* 1: 用 stack_mem_dump 打印编码后的 JPEG 比特流（与 vcdec_h264_test 中 DUMP 用法一致） */
#define JPEG_ENCODE_TEST_DUMP_ENABLE         0

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
extern void bk_task_wdt_feed(void);

typedef struct {
	beken_semaphore_t done_sem;
	bk_jpeg_encode_ctlr_handle_t handle;
	volatile uint32_t result;
	volatile uint32_t jpeg_size;
	uint32_t flexa_wr_blocks;
	uint8_t *copy_dst;
	uint32_t copy_size;
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
					const uint8_t *src, uint32_t src_size,
					uint32_t dst_height)
{
	uint32_t src_y_size = JPEG_ENC_TEST_SRC_WIDTH * JPEG_ENC_TEST_SRC_HEIGHT;
	uint32_t dst_y_size = JPEG_ENC_TEST_WIDTH * dst_height;
	uint8_t *dst_uv = dst + dst_y_size;
	const uint8_t *src_uv = src + src_y_size;

	if (dst == NULL || src == NULL ||
	    dst_size != JPEG_ENC_TEST_WIDTH * dst_height * 3U / 2U ||
	    src_size != JPEG_ENC_TEST_SRC_SIZE ||
	    (JPEG_ENC_TEST_WIDTH % 2U) != 0U || (dst_height % 2U) != 0U) {
		return BK_FAIL;
	}

	bk_jpeg_test_copy_plane_tiled(dst, JPEG_ENC_TEST_WIDTH, dst_height,
				      src, JPEG_ENC_TEST_SRC_WIDTH, JPEG_ENC_TEST_SRC_HEIGHT);
	bk_jpeg_test_copy_plane_tiled(dst_uv, JPEG_ENC_TEST_WIDTH, dst_height / 2U,
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

	if (bk_jpeg_test_copy_nv12_tiled(input, input_size, src, src_size,
					 JPEG_ENC_TEST_HEIGHT) != BK_OK) {
		LOGE("input NV12 tile copy failed, src_size=%u dst_size=%u\r\n", src_size, input_size);
		bk_frame_buffer_free(input);
		return NULL;
	}

	LOGI("input NV12 ready, addr=%p src_size=%u dst_size=%u %ux%u\r\n",
	     input, src_size, input_size, JPEG_ENC_TEST_WIDTH, JPEG_ENC_TEST_HEIGHT);
	return input;
}

/*
 * Flexa/slice 模式的输入必须是 16 行对齐的 NV12（1080 行帧需要按 1088 行存放），
 * 否则编码器按 width * 16 * blocks 计算 chroma 基址时会越过 Y 平面，
 * 导致画面最后一块颜色异常。
 */
static uint8_t *bk_jpeg_test_alloc_input_flexa(const uint8_t *src, uint32_t src_size)
{
	uint32_t input_size = JPEG_ENC_TEST_FLEXA_INPUT_SIZE;
	uint8_t *input = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, input_size + 32U);

	if (input == NULL) {
		LOGE("flexa input malloc failed, size=%u\r\n", input_size);
		return NULL;
	}

	if (bk_jpeg_test_copy_nv12_tiled(input, input_size, src, src_size,
					 JPEG_ENC_TEST_FLEXA_ALIGNED_HEIGHT) != BK_OK) {
		LOGE("flexa input NV12 tile copy failed, src_size=%u dst_size=%u\r\n",
		     src_size, input_size);
		bk_frame_buffer_free(input);
		return NULL;
	}

	LOGI("flexa input NV12 ready, addr=%p src_size=%u dst_size=%u %ux%u(aligned)\r\n",
	     input, src_size, input_size, JPEG_ENC_TEST_WIDTH,
	     JPEG_ENC_TEST_FLEXA_ALIGNED_HEIGHT);
	return input;
}

/*
 * Pack one line as Y-Cr-Y-Cb pairs (byte order Y V Y U).
 *
 * The software JPEG encoder (tjpg_enc, which must not be modified) emits the
 * chroma block sampled from byte +1 of every pixel pair before the one sampled
 * from byte +3, while its scan header declares the component order Y, Cb, Cr.
 * Putting Cr in the first chroma byte of the pair makes the coded component
 * order match the header, so the decoded picture gets the right colours. The
 * conversion is therefore done here, outside the encoder module.
 * U and V come from the same NV12 chroma pair as before, only their position
 * inside the packed pair changes.
 */
static void bk_jpeg_test_copy_yuyv_line(uint8_t *dst_line, const uint8_t *src_y_line,
					const uint8_t *src_uv_line, uint32_t dst_width,
					uint32_t src_width)
{
	for (uint32_t x = 0U; x < dst_width; x += 2U) {
		uint32_t col = x % src_width;
		const uint8_t *uv = src_uv_line + (col / 2U) * 2U;

		dst_line[x * 2U] = src_y_line[col];
		dst_line[x * 2U + 1U] = uv[1];
		dst_line[x * 2U + 2U] = src_y_line[(col + 1U) % src_width];
		dst_line[x * 2U + 3U] = uv[0];
	}
}

static int bk_jpeg_test_copy_nv12_to_yuyv_tiled(uint8_t *dst, uint32_t dst_size,
						const uint8_t *src, uint32_t src_size,
						uint32_t dst_width, uint32_t dst_height)
{
	uint32_t src_y_size = JPEG_ENC_TEST_SRC_WIDTH * JPEG_ENC_TEST_SRC_HEIGHT;
	uint32_t dst_size_expected = dst_width * dst_height * 2U;
	const uint8_t *src_uv = src + src_y_size;

	if (dst == NULL || src == NULL || dst_size != dst_size_expected ||
	    src_size != JPEG_ENC_TEST_SRC_SIZE ||
	    (dst_width % 2U) != 0U || (JPEG_ENC_TEST_SRC_WIDTH % 2U) != 0U ||
	    (JPEG_ENC_TEST_SRC_HEIGHT % 2U) != 0U) {
		return BK_FAIL;
	}

	for (uint32_t row = 0U; row < dst_height; row++) {
		uint32_t src_row = row % JPEG_ENC_TEST_SRC_HEIGHT;
		const uint8_t *src_y_line = src + src_row * JPEG_ENC_TEST_SRC_WIDTH;
		const uint8_t *src_uv_line = src_uv + (src_row / 2U) * JPEG_ENC_TEST_SRC_WIDTH;
		uint8_t *dst_line = dst + row * dst_width * 2U;

		bk_jpeg_test_copy_yuyv_line(dst_line, src_y_line, src_uv_line,
					    dst_width, JPEG_ENC_TEST_SRC_WIDTH);
	}

	return BK_OK;
}

static uint8_t *bk_jpeg_test_alloc_input_yuyv(const uint8_t *src, uint32_t src_size,
					      uint32_t width, uint32_t height)
{
	uint32_t input_size = width * height * 2U;
	uint8_t *input = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, input_size + 32U);

	if (input == NULL) {
		LOGE("yuyv input malloc failed, size=%u\r\n", input_size);
		return NULL;
	}

	if (bk_jpeg_test_copy_nv12_to_yuyv_tiled(input, input_size, src, src_size,
						 width, height) != BK_OK) {
		LOGE("input YUYV tile copy failed, src_size=%u dst_size=%u\r\n", src_size, input_size);
		bk_frame_buffer_free(input);
		return NULL;
	}

	LOGI("input YUYV ready, addr=%p src_size=%u dst_size=%u %ux%u\r\n",
	     input, src_size, input_size, width, height);
	return input;
}

/*
 * The display test keeps the decode ring, GPU frames and OSD buffers in the
 * UNCODED heap, so a fresh 3 MB input per round would eventually fail to
 * allocate. Allocate the test input once and let every round reuse it.
 */
static uint8_t *s_osd_input_nv12;
static uint8_t *s_osd_input_nv12_flexa;
static uint8_t *s_osd_input_yuyv;

static uint8_t *bk_jpeg_test_osd_input_nv12(void)
{
	if (s_osd_input_nv12 == NULL) {
		s_osd_input_nv12 = bk_jpeg_test_alloc_input(jpeg_encode_nv12_256x128,
							    jpeg_encode_nv12_256x128_bytes);
	}

	return s_osd_input_nv12;
}

static uint8_t *bk_jpeg_test_osd_input_nv12_flexa(void)
{
	if (s_osd_input_nv12_flexa == NULL) {
		s_osd_input_nv12_flexa = bk_jpeg_test_alloc_input_flexa(jpeg_encode_nv12_256x128,
									jpeg_encode_nv12_256x128_bytes);
	}

	return s_osd_input_nv12_flexa;
}

static uint8_t *bk_jpeg_test_osd_input_yuyv(void)
{
	if (s_osd_input_yuyv == NULL) {
		s_osd_input_yuyv = bk_jpeg_test_alloc_input_yuyv(jpeg_encode_nv12_256x128,
								 jpeg_encode_nv12_256x128_bytes,
								 JPEG_ENC_TEST_WIDTH, JPEG_ENC_TEST_HEIGHT);
	}

	return s_osd_input_yuyv;
}

void bk_jpeg_encode_osd_input_release(void)
{
	if (s_osd_input_nv12 != NULL) {
		bk_frame_buffer_free(s_osd_input_nv12);
		s_osd_input_nv12 = NULL;
	}

	if (s_osd_input_nv12_flexa != NULL) {
		bk_frame_buffer_free(s_osd_input_nv12_flexa);
		s_osd_input_nv12_flexa = NULL;
	}

	if (s_osd_input_yuyv != NULL) {
		bk_frame_buffer_free(s_osd_input_yuyv);
		s_osd_input_yuyv = NULL;
	}
}

static void bk_jpeg_log_result(const char *name, uint8_t pass, const char *stage, int ret,
			       uint32_t done_frames, uint32_t total_frames, uint32_t last_size)
{
	if (pass)
		LOGI("[RESULT][PASS] %s success, frames=%u/%u encoded_size=%u\r\n",
		     name, done_frames, total_frames, last_size);
	else
		LOGE("[RESULT][FAIL] %s failed at %s ret=%d frames=%u/%u encoded_size=%u\r\n",
		     name, stage, ret, done_frames, total_frames, last_size);
}

static int bk_jpeg_test_frame_run(const char *name, uint8_t enable_osd, const char *osd_mode_text)
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

	LOGI("%s start, frames=%u quality=%u osd=%u\r\n", name,
	     (unsigned)JPEG_ENC_TEST_FRAME_CNT, (unsigned)JPEG_ENC_TEST_QUALITY,
	     (unsigned)enable_osd);

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

	if (enable_osd) {
		avdk_ret = jpeg_encode_osd_submit_all_channels(handle, osd_mode_text);
		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "set_osd";
			ret = (int)avdk_ret;
			LOGE("jpeg osd submit failed, ret=%d\r\n", avdk_ret);
			goto exit_destroy;
		}
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

	bk_jpeg_log_result(name, pass, fail_stage, ret, done_frames,
			   JPEG_ENC_TEST_FRAME_CNT, ctx.jpeg_size);
	return ret;
}

int bk_jpeg_encode_frame_test(void)
{
	return bk_jpeg_test_frame_run("bk_jpeg_encode_frame_test", 0U, NULL);
}

/* Records the encoded JPEG size only; the caller owns the output buffer. */
static uint32_t bk_jpeg_test_frame_once_complete(bk_jpeg_encode_outbuf_info_t *info)
{
	bk_jpeg_encode_test_ctx_t *ctx;

	if (info == NULL)
		return BK_FAIL;

	ctx = (bk_jpeg_encode_test_ctx_t *)info->args;
	if (ctx != NULL) {
		ctx->result = info->status;
		ctx->jpeg_size = info->length;
		if (ctx->done_sem != NULL)
			rtos_set_semaphore(&ctx->done_sem);
	}

	return BK_OK;
}

int bk_jpeg_encode_osd_frame_once(uint8_t *out_buf, uint32_t out_size, uint32_t *out_len,
				  const char *mode_text)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *input = NULL;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_frame_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	bk_jpeg_encode_input_t enc_in;
	const char *fail_stage = "start";
	uint8_t sem_ready = 0U;

	if (out_buf == NULL || out_len == NULL || out_size == 0U)
		return BK_FAIL;

	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));
	os_memset(&enc_in, 0, sizeof(enc_in));

	if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
		LOGE("osd frame once sem init failed\r\n");
		return BK_FAIL;
	}
	sem_ready = 1U;

	input = bk_jpeg_test_osd_input_nv12();
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
	config.outbuf_malloc = NULL;
	config.outbuf_complete = bk_jpeg_test_frame_once_complete;
	config.outbuf_complete_args = &ctx;

	avdk_ret = bk_jpeg_encode_frame_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "frame_new";
		ret = (int)avdk_ret;
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

	avdk_ret = jpeg_encode_osd_submit_all_channels(handle, mode_text);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "set_osd";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	ctx.result = (uint32_t)~0U;
	ctx.jpeg_size = 0U;
	enc_in.pic_buf = (uint32_t)(uintptr_t)input;
	enc_in.pic_lines = 0U;
	enc_in.out_buf = (uint32_t)(uintptr_t)out_buf;
	enc_in.out_size = out_size;

	avdk_ret = bk_jpeg_encode_frame(handle, &enc_in);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "encode_frame";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	if (bk_jpeg_wait_done(&ctx, "osd_frame_once") != BK_OK) {
		fail_stage = "wait_done";
		ret = BK_FAIL;
		goto exit_destroy;
	}

	if (ctx.jpeg_size == 0U || ctx.jpeg_size > out_size) {
		fail_stage = "jpeg_size";
		ret = BK_FAIL;
		goto exit_destroy;
	}

	*out_len = ctx.jpeg_size;
	ret = BK_OK;

exit_destroy:
	if (handle != NULL) {
		bk_jpeg_encode_close(handle);
		bk_jpeg_encode_deinit(handle);
		bk_jpeg_encode_delete(handle);
	}

exit:
	if (sem_ready)
		(void)rtos_deinit_semaphore(&ctx.done_sem);

	if (ret != BK_OK)
		LOGE("osd frame once failed at %s ret=%d\r\n", fail_stage, ret);

	return ret;
}

static int bk_jpeg_test_sw_flexa_run(const char *name, uint8_t enable_osd,
				     const char *osd_mode_text)
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

	LOGI("%s start, blocks=%u quality=%u frames=%u osd=%u\r\n", name,
	     (unsigned)JPEG_ENC_TEST_FLEXA_BLOCKS, (unsigned)JPEG_ENC_TEST_QUALITY,
	     (unsigned)JPEG_ENC_TEST_FRAME_CNT, (unsigned)enable_osd);
	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));

	if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
		fail_stage = "init_done_sem";
		LOGE("init flexa done sem failed\r\n");
		bk_jpeg_log_result(name, 0U, fail_stage, BK_FAIL, 0U, JPEG_ENC_TEST_FRAME_CNT, 0U);
		return BK_FAIL;
	}

	input = bk_jpeg_test_alloc_input_flexa(jpeg_encode_nv12_256x128, jpeg_encode_nv12_256x128_bytes);
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	config.width = JPEG_ENC_TEST_WIDTH;
	config.height = JPEG_ENC_TEST_HEIGHT;
	config.input_format = BK_PIXEL_FORMAT_NV12;
	config.input_flexa_cnt = JPEG_ENC_TEST_FLEXA_BLOCKS;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = JPEG_ENC_TEST_FLEXA_INPUT_SIZE;
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

	if (enable_osd) {
		avdk_ret = jpeg_encode_osd_submit_all_channels(handle, osd_mode_text);
		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "set_osd";
			ret = (int)avdk_ret;
			LOGE("jpeg osd submit failed, ret=%d\r\n", avdk_ret);
			goto exit;
		}
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

	bk_jpeg_log_result(name, pass, fail_stage, ret, done_frames,
			   JPEG_ENC_TEST_FRAME_CNT, ctx.jpeg_size);
	return ret;
}

int bk_jpeg_encode_sw_flexa_test(void)
{
	return bk_jpeg_test_sw_flexa_run("bk_jpeg_encode_sw_flexa_test", 0U, NULL);
}

static int bk_jpeg_test_sw_frame_run(const char *name, uint8_t enable_osd,
				     const char *osd_mode_text)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *input = NULL;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_sw_frame_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	const char *fail_stage = "start";
	uint8_t pass = 0U;
	uint32_t done_frames = 0U;
	bk_jpeg_encode_input_t enc_in;

	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));
	os_memset(&enc_in, 0, sizeof(enc_in));

	LOGI("%s start, frames=%u quality=%u osd=%u\r\n", name,
	     (unsigned)JPEG_ENC_TEST_SW_FRAME_CNT, (unsigned)JPEG_ENC_TEST_SW_QUALITY,
	     (unsigned)enable_osd);

	input = bk_jpeg_test_alloc_input_yuyv(jpeg_encode_nv12_256x128,
					      jpeg_encode_nv12_256x128_bytes,
					      JPEG_ENC_TEST_WIDTH, JPEG_ENC_TEST_HEIGHT);
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	config.width = JPEG_ENC_TEST_WIDTH;
	config.height = JPEG_ENC_TEST_HEIGHT;
	config.input_format = BK_PIXEL_FORMAT_YUYV;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = JPEG_ENC_TEST_WIDTH * JPEG_ENC_TEST_HEIGHT * 2U;
	config.quality = JPEG_ENC_TEST_SW_QUALITY;
	config.outbuf_malloc = bk_jpeg_test_outbuf_malloc;
	config.outbuf_complete = bk_jpeg_test_outbuf_complete;
	config.outbuf_complete_args = &ctx;
	config.osd_in_place = JPEG_SW_OSD_IN_PLACE;

	avdk_ret = bk_jpeg_encode_sw_frame_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "sw_frame_new";
		ret = (int)avdk_ret;
		LOGE("bk_jpeg_encode_sw_frame_new failed, ret=%d\r\n", avdk_ret);
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

	if (enable_osd) {
		avdk_ret = jpeg_encode_osd_submit_all_channels(handle, osd_mode_text);
		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "set_osd";
			ret = (int)avdk_ret;
			LOGE("jpeg osd submit failed, ret=%d\r\n", avdk_ret);
			goto exit_destroy;
		}
	}

	for (uint32_t i = 0U; i < JPEG_ENC_TEST_SW_FRAME_CNT; i++) {
		ctx.result = (uint32_t)~0U;
		ctx.jpeg_size = 0U;

		enc_in.pic_buf = (uint32_t)(uintptr_t)input;
		enc_in.pic_lines = 0U;
		enc_in.out_buf = 0U;
		enc_in.out_size = JPEG_ENC_TEST_SW_FRAME_OUT_SIZE;

		avdk_ret = bk_jpeg_encode_frame(handle, &enc_in);
		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "encode_frame";
			ret = (int)avdk_ret;
			LOGE("sw frame round %u failed, avdk_ret=%d\r\n",
			     (unsigned)(i + 1U), avdk_ret);
			goto exit_destroy;
		}

		if (ctx.result != BK_OK) {
			fail_stage = "callback_status";
			ret = (int)ctx.result;
			LOGE("sw frame round %u bad status %u\r\n",
			     (unsigned)(i + 1U), ctx.result);
			goto exit_destroy;
		}

		done_frames++;
		LOGI("sw frame round %u/%u done, size=%u\r\n",
		     (unsigned)(i + 1U), (unsigned)JPEG_ENC_TEST_SW_FRAME_CNT, ctx.jpeg_size);
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

	bk_jpeg_log_result(name, pass, fail_stage, ret, done_frames,
			   JPEG_ENC_TEST_SW_FRAME_CNT, ctx.jpeg_size);
	return ret;
}

int bk_jpeg_encode_osd_sw_frame_once(uint8_t *out_buf, uint32_t out_size, uint32_t *out_len,
				     const char *mode_text)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *input = NULL;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_sw_frame_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	bk_jpeg_encode_input_t enc_in;
	const char *fail_stage = "start";

	LOGE("%s %d %d\n", __func__, __LINE__, JPEG_ENC_TEST_SW_QUALITY);

	if (out_buf == NULL || out_len == NULL || out_size == 0U)
		return BK_FAIL;

	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));
	os_memset(&enc_in, 0, sizeof(enc_in));

	input = bk_jpeg_test_osd_input_yuyv();
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	config.width = JPEG_ENC_TEST_WIDTH;
	config.height = JPEG_ENC_TEST_HEIGHT;
	config.input_format = BK_PIXEL_FORMAT_YUYV;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = JPEG_ENC_TEST_WIDTH * JPEG_ENC_TEST_HEIGHT * 2U;
	config.quality = JPEG_ENC_TEST_SW_QUALITY;
	config.outbuf_malloc = NULL;
	config.outbuf_complete = bk_jpeg_test_frame_once_complete;
	config.outbuf_complete_args = &ctx;
	config.osd_in_place = JPEG_SW_OSD_IN_PLACE;

	avdk_ret = bk_jpeg_encode_sw_frame_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "sw_frame_new";
		ret = (int)avdk_ret;
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

	avdk_ret = jpeg_encode_osd_submit_all_channels(handle, mode_text);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "set_osd";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	ctx.result = (uint32_t)~0U;
	ctx.jpeg_size = 0U;
	enc_in.pic_buf = (uint32_t)(uintptr_t)input;
	enc_in.pic_lines = 0U;
	enc_in.out_buf = (uint32_t)(uintptr_t)out_buf;
	enc_in.out_size = out_size;

	avdk_ret = bk_jpeg_encode_frame(handle, &enc_in);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "encode_frame";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	if (ctx.result != BK_OK) {
		fail_stage = "callback_status";
		ret = (int)ctx.result;
		goto exit_destroy;
	}

	if (ctx.jpeg_size == 0U || ctx.jpeg_size > out_size) {
		fail_stage = "jpeg_size";
		goto exit_destroy;
	}

	*out_len = ctx.jpeg_size;
	ret = BK_OK;

exit_destroy:
	if (handle != NULL) {
		bk_jpeg_encode_close(handle);
		bk_jpeg_encode_deinit(handle);
		bk_jpeg_encode_delete(handle);
	}

exit:
	if (ret != BK_OK)
		LOGE("osd sw frame once failed at %s ret=%d\r\n", fail_stage, ret);

	return ret;
}

/* Copies the encoded JPEG into the caller buffer, then releases the encoder frame. */
static uint32_t bk_jpeg_test_flexa_once_complete(bk_jpeg_encode_outbuf_info_t *info)
{
	bk_jpeg_encode_test_ctx_t *ctx;
	uint32_t frame_size;
	frame_buffer_t *frame;

	if (info == NULL)
		return BK_FAIL;

	ctx = (bk_jpeg_encode_test_ctx_t *)info->args;
	if (ctx != NULL) {
		ctx->result = info->status;
		ctx->jpeg_size = info->length;

		if (info->status == BK_OK && info->outbuf != NULL && info->length > 0U &&
		    ctx->copy_dst != NULL && info->length <= ctx->copy_size) {
			os_memcpy(ctx->copy_dst, (const uint8_t *)info->outbuf, info->length);
		} else if (info->status == BK_OK) {
			ctx->result = BK_FAIL;
		}

		if (ctx->done_sem != NULL)
			rtos_set_semaphore(&ctx->done_sem);
	}

	if (info->outbuf != NULL) {
		frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
		frame = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
		bk_frame_buffer_free(frame);
	}

	return BK_OK;
}

int bk_jpeg_encode_osd_flexa_once(uint8_t *out_buf, uint32_t out_size, uint32_t *out_len,
				  const char *mode_text)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *input = NULL;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_sw_flexa_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	const char *fail_stage = "start";

	if (out_buf == NULL || out_len == NULL || out_size == 0U)
		return BK_FAIL;

	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));

	if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
		LOGE("init osd flexa once sem failed\r\n");
		return BK_FAIL;
	}

	input = bk_jpeg_test_osd_input_nv12_flexa();
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	config.width = JPEG_ENC_TEST_WIDTH;
	config.height = JPEG_ENC_TEST_HEIGHT;
	config.input_format = BK_PIXEL_FORMAT_NV12;
	config.input_flexa_cnt = JPEG_ENC_TEST_FLEXA_BLOCKS;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = JPEG_ENC_TEST_FLEXA_INPUT_SIZE;
	config.quality = JPEG_ENC_TEST_QUALITY;
	config.outbuf_malloc = bk_jpeg_test_outbuf_malloc;
	config.outbuf_complete = bk_jpeg_test_flexa_once_complete;
	config.outbuf_complete_args = &ctx;
	config.flexa_done = bk_jpeg_test_flexa_done;
	config.flexa_done_arg = &ctx;

	avdk_ret = bk_jpeg_encode_sw_flexa_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "sw_flexa_new";
		ret = (int)avdk_ret;
		goto exit;
	}

	ctx.handle = handle;
	ctx.copy_dst = out_buf;
	ctx.copy_size = out_size;

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

	avdk_ret = jpeg_encode_osd_submit_all_channels(handle, mode_text);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "set_osd";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	ctx.result = (uint32_t)~0U;
	ctx.jpeg_size = 0U;
	ctx.flexa_wr_blocks = 2U;

	avdk_ret = bk_jpeg_encode_ioctl(handle, BK_JPEG_ENCODE_IOCTL_SET_FRAME_READY, NULL);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "set_frame_ready";
		ret = (int)avdk_ret;
		goto exit_destroy;
	}

	ret = bk_jpeg_wait_done(&ctx, "osd flexa once");
	if (ret != BK_OK) {
		fail_stage = "wait_done";
		goto exit_destroy;
	}

	if (ctx.jpeg_size == 0U || ctx.jpeg_size > out_size) {
		fail_stage = "jpeg_size";
		ret = BK_FAIL;
		goto exit_destroy;
	}

	*out_len = ctx.jpeg_size;
	ret = BK_OK;

exit_destroy:
	if (handle != NULL) {
		bk_jpeg_encode_close(handle);
		bk_jpeg_encode_deinit(handle);
		bk_jpeg_encode_delete(handle);
	}

exit:
	if (ctx.done_sem != NULL)
		rtos_deinit_semaphore(&ctx.done_sem);

	if (ret != BK_OK)
		LOGE("osd flexa once failed at %s ret=%d\r\n", fail_stage, ret);

	return ret;
}

/*
 * Encodes per case: the first frame is a warm-up (it allocates the OSD staging
 * buffer in copy mode), the remaining four are timed. The input size comes from
 * the CLI (default 640x360), so the same binary covers both a quick 640x360 run
 * and the slower 1920x1080 one without a rebuild. A software encode is slow,
 * keep the run short enough for the automation timeout.
 */
#define JPEG_OSD_BENCH_FRAMES          (5U)
#define JPEG_OSD_BENCH_CASE_CNT        (8U)
#define JPEG_OSD_BENCH_NAME_LEN        (24U)

typedef struct {
	char name[JPEG_OSD_BENCH_NAME_LEN];
	uint32_t format;
	uint32_t width;
	uint32_t height;
} bk_jpeg_bench_case_t;

static const char *bk_jpeg_bench_format_name(uint32_t format)
{
	return (format == BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP) ?
		"bitmap" : "argb8888";
}

static void bk_jpeg_bench_fill_case(bk_jpeg_bench_case_t *bench, const char *prefix,
				    uint32_t format, uint32_t width, uint32_t height)
{
	bench->format = format;
	bench->width = width;
	bench->height = height;
	os_snprintf(bench->name, sizeof(bench->name), "%s_%ux%u",
		    prefix, (unsigned)width, (unsigned)height);
}

/*
 * Size ladder derived from the requested frame size: 1/16 -> 1/4 -> 9/16 -> the
 * whole frame. Every dimension is rounded down to 8 pixels because BITMAP is
 * 1 bpp and the encoder requires 8-pixel alignment; the ARGB8888 and the BITMAP
 * case of a pair therefore cover exactly the same area and their timings are
 * directly comparable. The two formats alternate so a pair shows up next to
 * each other in the log.
 */
static void bk_jpeg_bench_build_cases(uint32_t width, uint32_t height,
				      bk_jpeg_bench_case_t *cases)
{
	static const uint32_t num[JPEG_OSD_BENCH_CASE_CNT / 2U] = { 1U, 1U, 9U, 16U };
	static const uint32_t den[JPEG_OSD_BENCH_CASE_CNT / 2U] = { 16U, 4U, 16U, 16U };
	uint32_t frame_w = width & ~7U;
	uint32_t frame_h = height & ~7U;

	for (uint32_t i = 0U; i < (JPEG_OSD_BENCH_CASE_CNT / 2U); i++) {
		uint32_t w = (uint32_t)(((uint64_t)width * num[i]) / den[i]) & ~7U;
		uint32_t h = (uint32_t)(((uint64_t)height * num[i]) / den[i]) & ~7U;

		if ((w == 0U) || (h == 0U)) {
			w = frame_w;
			h = frame_h;
		}

		bk_jpeg_bench_fill_case(&cases[i * 2U], "argb",
					BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888, w, h);
		bk_jpeg_bench_fill_case(&cases[i * 2U + 1U], "bitmap",
					BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP, w, h);
	}
}

static const bk_jpeg_bench_case_t s_bench_base_case = {
	"no_osd", BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888, 0U, 0U,
};

/*
 * Software OSD timing: one sw_frame controller per case, the first frame is a
 * warm-up (it also allocates the OSD staging buffer), the following frames are
 * timed with the platform microsecond timestamp bk_aon_rtc_get_us().
 */
static int bk_jpeg_bench_run_case(const bk_jpeg_bench_case_t *bench, uint8_t osd_in_place,
				  uint32_t width, uint32_t height,
				  uint8_t *input, uint8_t *out_buf, uint32_t out_size,
				  uint32_t *avg_us, uint32_t *fps_x10)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	bk_jpeg_encode_ctlr_handle_t handle = NULL;
	bk_jpeg_encode_sw_frame_config_t config;
	bk_jpeg_encode_test_ctx_t ctx;
	bk_jpeg_encode_input_t enc_in;
	const char *fail_stage = "start";
	uint64_t total_us = 0U;
	uint32_t frames = 0U;

	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&config, 0, sizeof(config));
	os_memset(&enc_in, 0, sizeof(enc_in));

	config.width = width;
	config.height = height;
	config.input_format = BK_PIXEL_FORMAT_YUYV;
	config.input_buf = (uint32_t)(uintptr_t)input;
	config.input_size = width * height * 2U;
	config.quality = JPEG_ENC_TEST_SW_QUALITY;
	config.outbuf_malloc = NULL;
	config.outbuf_complete = bk_jpeg_test_frame_once_complete;
	config.outbuf_complete_args = &ctx;
	config.osd_in_place = osd_in_place;

	avdk_ret = bk_jpeg_encode_sw_frame_new(&handle, &config);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "sw_frame_new";
		ret = (int)avdk_ret;
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

	avdk_ret = jpeg_encode_osd_submit_bench_osd(handle, bench->format,
						    bench->width, bench->height);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "set_osd";
		ret = (int)avdk_ret;
		LOGE("osd bench %s set osd failed, ret=%d\r\n", bench->name, avdk_ret);
		goto exit_destroy;
	}

	enc_in.pic_buf = (uint32_t)(uintptr_t)input;
	enc_in.pic_lines = 0U;
	enc_in.out_buf = (uint32_t)(uintptr_t)out_buf;
	enc_in.out_size = out_size;

	for (uint32_t i = 0U; i < JPEG_OSD_BENCH_FRAMES; i++) {
		uint64_t t0;
		uint64_t t1;

		ctx.result = (uint32_t)~0U;
		ctx.jpeg_size = 0U;

		t0 = bk_aon_rtc_get_us();
		avdk_ret = bk_jpeg_encode_frame(handle, &enc_in);
		t1 = bk_aon_rtc_get_us();

		/*
		 * One 1080p software frame plus a full-frame OSD keeps this task on the
		 * CPU for seconds, so the idle task never runs and the task watchdog
		 * (60 s) fires during a full bench run. Feed it and let the lower
		 * priority tasks run between frames; neither is inside the timed window.
		 */
		bk_task_wdt_feed();
		rtos_delay_milliseconds(10);

		if (avdk_ret != AVDK_ERR_OK) {
			fail_stage = "encode_frame";
			ret = (int)avdk_ret;
			LOGE("osd bench %s round %u failed, avdk_ret=%d\r\n",
			     bench->name, (unsigned)(i + 1U), avdk_ret);
			goto exit_destroy;
		}

		if (ctx.result != BK_OK) {
			fail_stage = "callback_status";
			ret = (int)ctx.result;
			LOGE("osd bench %s round %u bad status %u\r\n",
			     bench->name, (unsigned)(i + 1U), ctx.result);
			goto exit_destroy;
		}

		if (i == 0U)
			continue;	/* warm-up frame, not counted */

		total_us += (t1 - t0);
		frames++;
	}

	if (frames == 0U) {
		fail_stage = "timing";
		goto exit_destroy;
	}

	*avg_us = (uint32_t)(total_us / frames);
	*fps_x10 = (*avg_us > 0U) ? (uint32_t)(10000000ULL / *avg_us) : 0U;

	LOGI("[OSD-BENCH] mode=%s %s fmt=%s osd=%ux%u px=%u frames=%u avg=%u.%ums fps=%u.%u\r\n",
	     osd_in_place ? "inplace" : "copy",
	     bench->name, bk_jpeg_bench_format_name(bench->format),
	     (unsigned)bench->width, (unsigned)bench->height,
	     (unsigned)(bench->width * bench->height),
	     (unsigned)frames, *avg_us / 1000U, (*avg_us / 100U) % 10U,
	     *fps_x10 / 10U, *fps_x10 % 10U);

	ret = BK_OK;

exit_destroy:
	if (handle != NULL) {
		bk_jpeg_encode_close(handle);
		bk_jpeg_encode_deinit(handle);
		bk_jpeg_encode_delete(handle);
	}

exit:
	if (ret != BK_OK)
		LOGE("osd bench %s failed at %s ret=%d\r\n", bench->name, fail_stage, ret);

	return ret;
}

int bk_jpeg_encode_osd_bench_test(uint32_t width, uint32_t height)
{
	int ret = BK_FAIL;
	uint8_t *input = NULL;
	uint8_t *out_buf = NULL;
	uint32_t avg_us = 0U;
	uint32_t fps_x10 = 0U;
	uint32_t base_us = 0U;
	uint32_t copy_us[JPEG_OSD_BENCH_CASE_CNT];
	uint32_t inplace_us[JPEG_OSD_BENCH_CASE_CNT];
	bk_jpeg_bench_case_t cases[JPEG_OSD_BENCH_CASE_CNT];
	uint32_t done = 0U;
	uint32_t total = JPEG_OSD_BENCH_CASE_CNT * 2U + 1U;
	const char *fail_stage = "start";

	if (((width | height) & 7U) != 0U || width < 16U || height < 16U) {
		LOGE("osd bench invalid frame %ux%u, both dimensions must be 8-aligned and >= 16\r\n",
		     (unsigned)width, (unsigned)height);
		return BK_FAIL;
	}

	os_memset(copy_us, 0, sizeof(copy_us));
	os_memset(inplace_us, 0, sizeof(inplace_us));
	os_memset(cases, 0, sizeof(cases));
	bk_jpeg_bench_build_cases(width, height, cases);

	LOGI("osd bench start, sw_frame %ux%u frames=%u(1 warm-up + %u timed) cases=%u(formats=argb8888+bitmap) modes=copy+inplace\r\n",
	     (unsigned)width, (unsigned)height,
	     (unsigned)JPEG_OSD_BENCH_FRAMES, (unsigned)(JPEG_OSD_BENCH_FRAMES - 1U),
	     (unsigned)JPEG_OSD_BENCH_CASE_CNT);

	LOGI("osd bench timing: timed from the bk_jpeg_encode_frame() call to its return, so the dcache flush, the OSD composite (+ the full-frame copy in copy mode) and the whole software encode are included\r\n");

	input = bk_jpeg_test_alloc_input_yuyv(jpeg_encode_nv12_256x128,
					      jpeg_encode_nv12_256x128_bytes,
					      width, height);
	if (input == NULL) {
		fail_stage = "alloc_input";
		goto exit;
	}

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
						    JPEG_ENC_TEST_SW_FRAME_OUT_SIZE);
	if (out_buf == NULL) {
		fail_stage = "alloc_outbuf";
		goto exit;
	}

	/* The no-OSD baseline never copies, so copy/in-place timings are identical. */
	if (bk_jpeg_bench_run_case(&s_bench_base_case, 0U, width, height, input, out_buf,
				   JPEG_ENC_TEST_SW_FRAME_OUT_SIZE,
				   &base_us, &fps_x10) != BK_OK) {
		fail_stage = s_bench_base_case.name;
		goto exit;
	}
	done++;

	for (uint32_t m = 0U; m < 2U; m++) {
		const char *mode_name = (m == 0U) ? "copy" : "inplace";
		uint32_t *result_us = (m == 0U) ? copy_us : inplace_us;

		for (uint32_t i = 0U; i < JPEG_OSD_BENCH_CASE_CNT; i++) {
			uint32_t diff_us;
			uint32_t pct_x10;

			if (bk_jpeg_bench_run_case(&cases[i], (uint8_t)m, width, height,
						   input, out_buf,
						   JPEG_ENC_TEST_SW_FRAME_OUT_SIZE,
						   &avg_us, &fps_x10) != BK_OK) {
				fail_stage = cases[i].name;
				goto exit;
			}
			done++;
			result_us[i] = avg_us;

			diff_us = (avg_us > base_us) ? (avg_us - base_us) : 0U;
			pct_x10 = (base_us > 0U) ?
				  (uint32_t)(((uint64_t)diff_us * 1000U) / base_us) : 0U;

			LOGI("[OSD-BENCH] overhead mode=%s %s fmt=%s vs no_osd: +%u.%ums (+%u.%u%%)\r\n",
			     mode_name, cases[i].name,
			     bk_jpeg_bench_format_name(cases[i].format),
			     diff_us / 1000U,
			     (diff_us / 100U) % 10U, pct_x10 / 10U, pct_x10 % 10U);
		}
	}

	LOGI("[OSD-BENCH] ===== summary: sw_frame %ux%u, average of one bk_jpeg_encode_frame() call =====\r\n",
	     (unsigned)width, (unsigned)height);
	LOGI("[OSD-BENCH] baseline no_osd: %u.%ums\r\n",
	     base_us / 1000U, (base_us / 100U) % 10U);

	for (uint32_t i = 0U; i < JPEG_OSD_BENCH_CASE_CNT; i++) {
		uint32_t copy_pct = (base_us > 0U) ?
			(uint32_t)(((uint64_t)copy_us[i] * 1000U) / base_us) : 0U;
		uint32_t inplace_pct = (base_us > 0U) ?
			(uint32_t)(((uint64_t)inplace_us[i] * 1000U) / base_us) : 0U;
		uint32_t delta_us = (copy_us[i] > inplace_us[i]) ?
			(copy_us[i] - inplace_us[i]) : (inplace_us[i] - copy_us[i]);

		LOGI("[OSD-BENCH] %s fmt=%s px=%u copy=%u.%ums(%u.%u%% of base) inplace=%u.%ums(%u.%u%% of base) copy-inplace=%s%u.%ums\r\n",
		     cases[i].name,
		     bk_jpeg_bench_format_name(cases[i].format),
		     (unsigned)(cases[i].width * cases[i].height),
		     copy_us[i] / 1000U, (copy_us[i] / 100U) % 10U,
		     copy_pct / 10U, copy_pct % 10U,
		     inplace_us[i] / 1000U, (inplace_us[i] / 100U) % 10U,
		     inplace_pct / 10U, inplace_pct % 10U,
		     (copy_us[i] >= inplace_us[i]) ? "+" : "-",
		     delta_us / 1000U, (delta_us / 100U) % 10U);
	}

	LOGI("[RESULT][PASS] bk_jpeg_encode_osd_bench_test success, baseline=%u.%ums\r\n",
	     base_us / 1000U, (base_us / 100U) % 10U);
	ret = BK_OK;

exit:
	if (out_buf != NULL)
		bk_frame_buffer_free(out_buf);

	if (input != NULL)
		bk_frame_buffer_free(input);

	if (ret != BK_OK)
		LOGE("[RESULT][FAIL] bk_jpeg_encode_osd_bench_test failed at %s, cases=%u/%u\r\n",
		     fail_stage, (unsigned)done, (unsigned)total);

	return ret;
}

int bk_jpeg_encode_osd_test(void)
{
	int frame_ret = bk_jpeg_test_frame_run("bk_jpeg_encode_osd_frame_test", 1U, "HW FRAME");
	int flexa_ret = bk_jpeg_test_sw_flexa_run("bk_jpeg_encode_osd_flexa_test", 1U, "SW FLEXA");
	int sw_frame_ret = bk_jpeg_test_sw_frame_run("bk_jpeg_encode_osd_sw_frame_test", 1U, "SW FRAME");
	uint8_t pass = (frame_ret == BK_OK && flexa_ret == BK_OK && sw_frame_ret == BK_OK) ? 1U : 0U;

	if (pass)
		LOGI("[RESULT][PASS] bk_jpeg_encode_osd_test success, frame=1 flexa=1 sw_frame=1\r\n");
	else
		LOGE("[RESULT][FAIL] bk_jpeg_encode_osd_test failed, frame=%d flexa=%d sw_frame=%d\r\n",
		     frame_ret, flexa_ret, sw_frame_ret);

	return pass ? BK_OK : BK_FAIL;
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
