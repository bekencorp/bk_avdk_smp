/*
 * H264 encode timing / oscilloscope helper:
 * - Toggles GPIO during encode (same idea as mjpeg_encode_stress.c debug macros).
 * - Reports DWT cycle counts per frame (optional reference; scope on GPIO is primary).
 *
 * CLI: h264_encode_time_statisticsi <frame|sw_flexa> [num_frames]
 *
 * GPIO 映射（与 MJPEG 调试宏一致，可按需改下面 H264_STAT_GPIO_*）:
 *   GPIO_32: 整次 h264_encoder_encode() 高电平窗口（整帧或 flexa 一轮送入）
 *   GPIO_33: 仅 sw_flexa — 每个 16 行块回调周期内一次短脉冲（便于看行块节奏）
 */

#include <stdint.h>
#include <string.h>
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/bk_frame_buffer.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "h264_encoder_api.h"
#include "dwt.h"

#ifndef MEM_CACHABLE_MASK
#define MEM_CACHABLE_MASK ((uint32_t)0)
#endif

#ifndef VCENC_INTRA_FRAME
#define VCENC_INTRA_FRAME 0u
#endif

#ifndef VCENC_PREDICTED_FRAME
#define VCENC_PREDICTED_FRAME 1u
#endif

#define TAG "h264_time_stat"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* 默认分辨率，与 h264_encode_stress 一致；编译前可改宏做不同分辨率 */
#ifndef H264_STAT_WIDTH
#define H264_STAT_WIDTH 1920u
#endif

#ifndef H264_STAT_HEIGHT
#define H264_STAT_HEIGHT 1080u
#endif

#define H264_STAT_YUV_SIZE   ((uint32_t)(H264_STAT_WIDTH) * (uint32_t)(H264_STAT_HEIGHT) * 3U / 2U)
#define H264_STAT_OUT_SIZE   ((uint32_t)(H264_STAT_WIDTH) * (uint32_t)(H264_STAT_HEIGHT))

#define H264_STAT_FLEXA_BLOCK_LINES  (16U)
#define H264_STAT_FLEXA_RING_BLOCKS  (3U)

/* 示波器探头接脚 — 若与板级复用冲突请改为空闲 GPIO */
#ifndef H264_STAT_GPIO_FRAME
#define H264_STAT_GPIO_FRAME GPIO_32
#endif

#ifndef H264_STAT_GPIO_LINE
#define H264_STAT_GPIO_LINE GPIO_33
#endif

/* DWT 换算：与 mjpeg_encode_stress 中 CONFIG_MJPEG_ENCODE_FRAME_ENCODE_TIME_AUTO 一致假设 CPU 480MHz */
#ifndef H264_STAT_CPU_MHZ
#define H264_STAT_CPU_MHZ 480u
#endif

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

#define H264_STAT_GPIO_OUT_INIT(id) \
	do { \
		gpio_dev_unmap(id); \
		bk_gpio_enable_output(id); \
		bk_gpio_set_output_low(id); \
	} while (0)

#define H264_STAT_FRAME_PULSE_BEGIN() \
	do { \
		bk_gpio_set_output_low(H264_STAT_GPIO_FRAME); \
		bk_gpio_set_output_high(H264_STAT_GPIO_FRAME); \
	} while (0)

#define H264_STAT_FRAME_PULSE_END() \
	do { \
		bk_gpio_set_output_low(H264_STAT_GPIO_FRAME); \
	} while (0)

#define H264_STAT_LINE_PULSE_BEGIN() \
	do { \
		bk_gpio_set_output_low(H264_STAT_GPIO_LINE); \
		bk_gpio_set_output_high(H264_STAT_GPIO_LINE); \
	} while (0)

#define H264_STAT_LINE_PULSE_END() \
	do { \
		bk_gpio_set_output_low(H264_STAT_GPIO_LINE); \
	} while (0)

#ifndef H264_STRESS_RED_Y
#define H264_STRESS_RED_Y 76u
#endif
#ifndef H264_STRESS_RED_U
#define H264_STRESS_RED_U 91u
#endif
#ifndef H264_STRESS_RED_V
#define H264_STRESS_RED_V 255u
#endif

static void h264_stat_fill_nv12_pure_red(uint8_t *yuv)
{
	uint32_t i;
	uint32_t y_plane_size = (uint32_t)H264_STAT_WIDTH * (uint32_t)H264_STAT_HEIGHT;

	if (yuv == NULL) {
		return;
	}
	for (i = 0; i < y_plane_size; i++) {
		yuv[i] = (uint8_t)H264_STRESS_RED_Y;
	}
	for (i = y_plane_size; i + 1U < H264_STAT_YUV_SIZE; i += 2U) {
		yuv[i] = (uint8_t)H264_STRESS_RED_U;
		yuv[i + 1U] = (uint8_t)H264_STRESS_RED_V;
	}
}

static void h264_stat_gpio_init_for_mode(int flexa)
{
	H264_STAT_GPIO_OUT_INIT(H264_STAT_GPIO_FRAME);
	if (flexa) {
		H264_STAT_GPIO_OUT_INIT(H264_STAT_GPIO_LINE);
	}
}

/* flexa 行缓冲回调：逻辑对齐 bk_test_h264e.c / NV12（与 MJPEG flexa 布局一致） */
static uint8_t *s_stat_flexa_src = NULL;
static uint32_t s_stat_flexa_src_h = 0;
static volatile int32_t s_stat_flexa_linebuf_count = 0;

static uint32_t h264_stat_flexa_linebuf_done(uint8_t *y_dst, uint8_t *u_dst, uint8_t *v_dst)
{
	(void)v_dst;

	H264_STAT_LINE_PULSE_END();

	const uint32_t width = (uint32_t)H264_STAT_WIDTH;
	const uint32_t height = s_stat_flexa_src_h;
	const uint32_t y_plane_size = width * height;
	const uint32_t block_y = width * H264_STAT_FLEXA_BLOCK_LINES;
	const uint32_t block_uv = width * (H264_STAT_FLEXA_BLOCK_LINES / 2U);

	if (s_stat_flexa_src == NULL || y_dst == NULL || u_dst == NULL) {
		return 0;
	}

	const uint32_t dst_slot = (uint32_t)(s_stat_flexa_linebuf_count % (int32_t)H264_STAT_FLEXA_RING_BLOCKS);
	y_dst += dst_slot * block_y;
	u_dst += dst_slot * block_uv;

	const uint32_t start_y_line = (uint32_t)s_stat_flexa_linebuf_count * H264_STAT_FLEXA_BLOCK_LINES;
	if (start_y_line >= height) {
		os_memset(y_dst, 0, block_y);
		os_memset(u_dst, 0x80, block_uv);
		s_stat_flexa_linebuf_count++;
		H264_STAT_LINE_PULSE_BEGIN();
		return 1;
	}

	const uint32_t src_y_off = start_y_line * width;
	const uint32_t src_uv_off = (start_y_line / 2U) * width;
	const uint8_t *src_y = s_stat_flexa_src + src_y_off;
	const uint8_t *src_uv = s_stat_flexa_src + y_plane_size + src_uv_off;

	os_memcpy(y_dst, src_y, block_y);
	os_memcpy(u_dst, src_uv, block_uv);

	s_stat_flexa_linebuf_count++;

	H264_STAT_LINE_PULSE_BEGIN();
	return 1;
}

static volatile uint32_t s_stat_out_nal = 0;

static void h264_stat_out_callback(uint8_t *buffer, uint32_t size, uint32_t type)
{
	(void)buffer;
	(void)size;
	if (type == VCENC_OUT_IFRAME || type == VCENC_OUT_PFRAME) {
		s_stat_out_nal++;
	}
}

static bk_err_t h264_stat_run_frame_mode(uint32_t num_frames)
{
	void *enc = NULL;
	int32_t ret;
	uint32_t f;
	uint64_t cyc_sum = 0;
	frame_buffer_t *yuv_fb = NULL;
	frame_buffer_t *out_fb = NULL;
	uint8_t *yuv = NULL;
	uint8_t *out = NULL;

	uint32_t fb_yuv = H264_STAT_YUV_SIZE + sizeof(frame_buffer_t) + 32U;
	uint32_t fb_out = H264_STAT_OUT_SIZE + sizeof(frame_buffer_t) + 32U;

	yuv_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_yuv);
	if (!yuv_fb) {
		LOGE("frame: yuv bk_frame_buffer_malloc failed\n");
		return BK_FAIL;
	}
	os_memset(yuv_fb, 0, sizeof(frame_buffer_t));
	yuv_fb->frame = (uint8_t *)((((uint32_t)(yuv_fb + 1U) >> 5U) + 1U) << 5U);
	yuv_fb->size = H264_STAT_YUV_SIZE;
	yuv = yuv_fb->frame;

	out_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_out);
	if (!out_fb) {
		LOGE("frame: out bk_frame_buffer_malloc failed\n");
		bk_frame_buffer_free(yuv_fb);
		return BK_FAIL;
	}
	os_memset(out_fb, 0, sizeof(frame_buffer_t));
	out_fb->frame = (uint8_t *)((((uint32_t)(out_fb + 1U) >> 5U) + 1U) << 5U);
	out_fb->size = H264_STAT_OUT_SIZE;
	out = out_fb->frame;

	h264_stat_fill_nv12_pure_red(yuv);

	h264_stat_gpio_init_for_mode(0);
	dwt_init_cycle_counter();

	ret = h264_encoder_init(&enc,
				H264_STAT_WIDTH,
				H264_STAT_HEIGHT,
				VCENC_FLEXA_MODE_NONE,
				NULL,
				h264_stat_out_callback);
	if (ret != 0 || enc == NULL) {
		LOGE("h264_encoder_init (frame) failed ret=%d\n", (int)ret);
		bk_frame_buffer_free(out_fb);
		bk_frame_buffer_free(yuv_fb);
		return BK_FAIL;
	}

	s_stat_out_nal = 0;
	for (f = 0; f < num_frames; f++) {
		uint32_t coding = (f == 0U) ? VCENC_INTRA_FRAME : VCENC_PREDICTED_FRAME;
		uint32_t c0 = dwt_get_cycle_counter_val();

		H264_STAT_FRAME_PULSE_BEGIN();
		ret = h264_encoder_encode(enc,
					  (uint32_t)(uintptr_t)yuv,
					  0,
					  coding,
					  (uint32_t)(uintptr_t)out,
					  H264_STAT_OUT_SIZE);
		H264_STAT_FRAME_PULSE_END();

		uint32_t c1 = dwt_get_cycle_counter_val();
		uint32_t delta = c1 - c0;
		cyc_sum += (uint64_t)delta;

		if (ret < 0) {
			LOGE("frame: h264_encoder_encode failed ret=%d frame=%u\n", (int)ret, (unsigned)f);
			break;
		}
	}

	LOGI("frame mode: frames=%u out_if_p=%u sum_cycles=%llu avg_cycles=%llu (~%llu us @ %uMHz)\n",
	     (unsigned)num_frames,
	     (unsigned)s_stat_out_nal,
	     (unsigned long long)cyc_sum,
	     (unsigned long long)(num_frames ? (cyc_sum / num_frames) : 0ULL),
	     (unsigned long long)(num_frames ? (cyc_sum / num_frames / H264_STAT_CPU_MHZ) : 0ULL),
	     (unsigned)H264_STAT_CPU_MHZ);

	h264_encoder_deinit(enc);
	bk_frame_buffer_free(out_fb);
	bk_frame_buffer_free(yuv_fb);
	return (ret < 0) ? BK_FAIL : BK_OK;
}

static bk_err_t h264_stat_run_sw_flexa_mode(uint32_t num_frames)
{
	void *enc = NULL;
	int32_t ret = 0;
	uint32_t f;
	uint64_t cyc_sum = 0;
	uint8_t *src_raw = NULL;
	uint8_t *ring_raw = NULL;
	uint8_t *ring_aligned = NULL;
	frame_buffer_t *out_fb = NULL;
	uint8_t *out = NULL;

	const uint32_t width = (uint32_t)H264_STAT_WIDTH;
	const uint32_t orig_h = (uint32_t)H264_STAT_HEIGHT;
	const uint32_t aligned_h = (orig_h + (H264_STAT_FLEXA_BLOCK_LINES - 1U)) & ~(H264_STAT_FLEXA_BLOCK_LINES - 1U);
	const uint32_t y_size_al = width * aligned_h;
	const uint32_t yuv_size_al = y_size_al * 3U / 2U;
	const uint32_t ring_bytes = width * H264_STAT_FLEXA_BLOCK_LINES * H264_STAT_FLEXA_RING_BLOCKS * 3U / 2U;

	uint32_t fb_out = H264_STAT_OUT_SIZE + sizeof(frame_buffer_t) + 32U;

	out_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_out);
	if (!out_fb) {
		LOGE("flexa: out bk_frame_buffer_malloc failed\n");
		return BK_FAIL;
	}
	os_memset(out_fb, 0, sizeof(frame_buffer_t));
	out_fb->frame = (uint8_t *)((((uint32_t)(out_fb + 1U) >> 5U) + 1U) << 5U);
	out_fb->size = H264_STAT_OUT_SIZE;
	out = out_fb->frame;

	src_raw = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, yuv_size_al);
	if (src_raw == NULL) {
		LOGE("flexa: src malloc failed size=%u\n", (unsigned)yuv_size_al);
		bk_frame_buffer_free(out_fb);
		return BK_FAIL;
	}

	/*
	 * NV12 按对齐后的高存放：Y 连续 y_size_al，UV 从 y_size_al 起，长度 width*(aligned_h/2)。
	 * （若把 UV 紧接在 orig_h 的 Y 后面再 pad，会与 flexa/bk_test 里按 y_size_al 取 UV 的约定不一致。）
	 */
	os_memset(src_raw, 0, yuv_size_al);
	{
		uint32_t i;
		const uint32_t y_plane = width * orig_h;
		for (i = 0; i < y_plane; i++) {
			src_raw[i] = (uint8_t)H264_STRESS_RED_Y;
		}
		if (aligned_h > orig_h) {
			const uint8_t *last_y = src_raw + (orig_h - 1U) * width;
			uint8_t *pad_y = src_raw + y_plane;
			uint32_t pl;
			for (pl = 0; pl < aligned_h - orig_h; pl++) {
				os_memcpy(pad_y + pl * width, last_y, width);
			}
		}
	}
	{
		uint8_t *uv = src_raw + y_size_al;
		const uint32_t uv_rows = aligned_h / 2U;
		uint32_t r;
		for (r = 0; r < uv_rows; r++) {
			uint32_t x;
			for (x = 0; x < width; x += 2U) {
				uv[r * width + x] = (uint8_t)H264_STRESS_RED_U;
				uv[r * width + x + 1U] = (uint8_t)H264_STRESS_RED_V;
			}
		}
	}

	ring_raw = (uint8_t *)os_malloc(ring_bytes + 64U);
	if (ring_raw == NULL) {
		LOGE("flexa: ring malloc failed\n");
		bk_frame_buffer_free(src_raw);
		bk_frame_buffer_free(out_fb);
		return BK_FAIL;
	}
	ring_aligned = (uint8_t *)((((uint32_t)(uintptr_t)ring_raw + 63U) & ~(uint32_t)63) | MEM_CACHABLE_MASK);

	s_stat_flexa_src = src_raw;
	s_stat_flexa_src_h = aligned_h;

	h264_stat_gpio_init_for_mode(1);
	dwt_init_cycle_counter();

	ret = h264_encoder_init(&enc,
				H264_STAT_WIDTH,
				H264_STAT_HEIGHT,
				VCENC_FLEXA_MODE_SOFTWARE,
				h264_stat_flexa_linebuf_done,
				h264_stat_out_callback);
	if (ret != 0 || enc == NULL) {
		LOGE("h264_encoder_init (flexa) failed ret=%d\n", (int)ret);
		os_free(ring_raw);
		bk_frame_buffer_free(src_raw);
		bk_frame_buffer_free(out_fb);
		s_stat_flexa_src = NULL;
		return BK_FAIL;
	}

	s_stat_out_nal = 0;
	for (f = 0; f < num_frames; f++) {
		uint32_t coding = (f == 0U) ? VCENC_INTRA_FRAME : VCENC_PREDICTED_FRAME;
		const uint32_t y_prefill = width * H264_STAT_FLEXA_BLOCK_LINES * H264_STAT_FLEXA_RING_BLOCKS;
		const uint32_t uv_prefill = width * (H264_STAT_FLEXA_BLOCK_LINES / 2U) * H264_STAT_FLEXA_RING_BLOCKS;
		uint32_t c0;
		uint32_t c1;
		uint32_t delta;

		os_memcpy(ring_aligned, src_raw, y_prefill);
		os_memcpy(ring_aligned + y_prefill, src_raw + y_size_al, uv_prefill);

		/* 与 bk_h264e_flx_test 一致：预填后从 count=3 开始，picLines = 16*3 */
		s_stat_flexa_linebuf_count = 3;

		c0 = dwt_get_cycle_counter_val();
		H264_STAT_FRAME_PULSE_BEGIN();
		H264_STAT_LINE_PULSE_BEGIN();
		ret = h264_encoder_encode(enc,
					  (uint32_t)(uintptr_t)ring_aligned,
					  H264_STAT_FLEXA_BLOCK_LINES * H264_STAT_FLEXA_RING_BLOCKS,
					  coding,
					  (uint32_t)(uintptr_t)out,
					  H264_STAT_OUT_SIZE);
		H264_STAT_LINE_PULSE_END();
		H264_STAT_FRAME_PULSE_END();
		c1 = dwt_get_cycle_counter_val();

		delta = c1 - c0;
		cyc_sum += (uint64_t)delta;

		if (ret < 0) {
			LOGE("flexa: h264_encoder_encode failed ret=%d frame=%u\n", (int)ret, (unsigned)f);
			break;
		}
	}

	LOGI("sw_flexa: frames=%u out_if_p=%u sum_cycles=%llu avg_cycles=%llu (~%llu us @ %uMHz)\n",
	     (unsigned)num_frames,
	     (unsigned)s_stat_out_nal,
	     (unsigned long long)cyc_sum,
	     (unsigned long long)(num_frames ? (cyc_sum / num_frames) : 0ULL),
	     (unsigned long long)(num_frames ? (cyc_sum / num_frames / H264_STAT_CPU_MHZ) : 0ULL),
	     (unsigned)H264_STAT_CPU_MHZ);

	h264_encoder_deinit(enc);
	s_stat_flexa_src = NULL;

	os_free(ring_raw);
	bk_frame_buffer_free(src_raw);
	bk_frame_buffer_free(out_fb);

	return (ret < 0) ? BK_FAIL : BK_OK;
}

void cli_h264_encode_time_statisticsi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	const char *msg = CLI_CMD_RSP_SUCCEED;
	uint32_t num_frames = 1;

	if (argc < 2) {
		LOGE("usage: h264_encode_time_statisticsi <frame|sw_flexa> [num_frames]\n");
		LOGE("  GPIO%u: encode window high; GPIO%u: flexa 16-line callback pulses\n",
		     (unsigned)H264_STAT_GPIO_FRAME, (unsigned)H264_STAT_GPIO_LINE);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	if (argc >= 3) {
		uint32_t n = (uint32_t)os_strtoul(argv[2], NULL, 0);
		if (n == 0U || n > 5000U) {
			LOGE("num_frames must be 1..5000\n");
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}
		num_frames = n;
	}

	if (os_strcmp(argv[1], "frame") == 0) {
		ret = h264_stat_run_frame_mode(num_frames);
	} else if (os_strcmp(argv[1], "sw_flexa") == 0) {
		ret = h264_stat_run_sw_flexa_mode(num_frames);
	} else {
		LOGE("usage: h264_encode_time_statisticsi <frame|sw_flexa> [num_frames]\n");
		ret = BK_FAIL;
	}

	if (ret != BK_OK) {
		msg = CLI_CMD_RSP_ERROR;
	}

exit:
	if (pcWriteBuffer && xWriteBufferLen > 0) {
		int len = os_strlen(msg);
		if (len >= xWriteBufferLen) {
			len = xWriteBufferLen - 1;
		}
		os_memcpy(pcWriteBuffer, msg, len);
		pcWriteBuffer[len] = '\0';
	}
}
