#include "vcdec_h264_test_common.h"
#include "h264_decode_h264_parser.h"

#define TAG "vcdec_h264_rgb"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static uint32_t vcdec_h264_rgb_output_size(uint32_t width, uint32_t height, bk_pixel_format_t fmt)
{
	uint32_t h_aligned = (height + 15U) & ~15U;

	if (fmt == BK_PIXEL_FORMAT_RGB888) {
		/* VCDec PP RGB888 path writes one pixel per 32-bit word. */
		return width * h_aligned * 4U;
	}

	return bk_image_size_get((uint16_t)width, (uint16_t)h_aligned, fmt);
}

static const char *vcdec_h264_rgb_format_name(bk_pixel_format_t fmt)
{
	return (fmt == BK_PIXEL_FORMAT_RGB565) ? "rgb565" : "rgb888";
}

static void vcdec_h264_rgb_log_format_result(const char *case_name, uint8_t pass,
					     const char *stage, avdk_err_t ret,
					     uint32_t done_aus)
{
	if (pass) {
		LOGI("%s format success, decoded_aus=%u\r\n",
		     case_name, (unsigned)done_aus);
	} else {
		LOGE("[RESULT][FAIL] %s failed at %s, ret=%d, decoded_aus=%u\r\n",
		     case_name, stage, ret, (unsigned)done_aus);
	}
}

static bk_err_t vcdec_h264_frame_rgb_run(bk_pixel_format_t fmt)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg =
		vcdec_h264_get_stream_cfg(H264_DECODE_TEST_STREAM_1280X720_1I30P);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint32_t out_size;
	uint32_t done_aus = 0U;
	uint32_t total_decode_ms = 0U;
	uint32_t min_decode_ms = 0xFFFFFFFFU;
	uint32_t max_decode_ms = 0U;
	const char *fail_stage = "start";
	uint8_t test_pass = 0U;
	avdk_err_t ret = AVDK_ERR_OK;

	if (stream_cfg == NULL || stream_cfg->stream == NULL || stream_cfg->bytes == NULL ||
	    *stream_cfg->bytes == 0U) {
		LOGE("invalid stream cfg\r\n");
		return BK_FAIL;
	}

	out_size = vcdec_h264_rgb_output_size(stream_cfg->width, stream_cfg->height, fmt);
	os_memset(&ctx, 0, sizeof(ctx));

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, *stream_cfg->bytes);
	if (stream_buf == NULL) {
		fail_stage = "alloc_stream_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, stream_cfg->stream, *stream_cfg->bytes);

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		fail_stage = "alloc_out_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memset(out_buf, 0, out_size);

	{
		bk_h264_decode_frame_config_t cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;

		cfg.timeout_ms = 1000U;
		cfg.out_width = (uint16_t)stream_cfg->width;
		cfg.out_height = (uint16_t)stream_cfg->height;
		cfg.out_format = fmt;
		cfg.frame_done_cb = vcdec_h264_frame_done_cb;
		cfg.frame_done_args = &ctx;
		ret = bk_h264_decode_frame_ctlr_new(&dec, &cfg);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "frame_ctlr_new";
			goto cleanup;
		}
	}

	ctx.dec = dec;
	ret = bk_h264_decode_init(dec);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decoder_init";
		goto cleanup;
	}
	ret = bk_h264_decode_open(dec);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "decoder_open";
		goto cleanup;
	}

	{
		uint32_t offset = 0U;

		while (offset < *stream_cfg->bytes) {
			const uint8_t *au_ptr = NULL;
			uint32_t au_size = 0U;
			bk_h264_decode_input_t in = {0};
			bk_h264_decode_info_t info = {0};
			uint32_t start_ms;
			uint32_t cost_ms;

			if (h264_decode_h264_next_au(stream_buf, *stream_cfg->bytes, &offset, &au_ptr, &au_size) != AVDK_ERR_OK) {
				break;
			}
			if (au_ptr == NULL || au_size == 0U) {
				continue;
			}

			in.stream = (uint8_t *)(uintptr_t)au_ptr;
			in.stream_len = au_size;
			in.out_buffer = out_buf;
			in.out_buffer_size = out_size;

			start_ms = rtos_get_time();
			ret = bk_h264_decode_frame(dec, &in);
			cost_ms = rtos_get_time() - start_ms;
			if (ret != AVDK_ERR_OK) {
				fail_stage = "decode_frame";
				goto cleanup;
			}
			total_decode_ms += cost_ms;
			if (cost_ms < min_decode_ms) {
				min_decode_ms = cost_ms;
			}
			if (cost_ms > max_decode_ms) {
				max_decode_ms = cost_ms;
			}
			LOGI("vcdec_h264_frame_%s au=%u size=%u decode=%u ms frame_done=%u\r\n",
			     vcdec_h264_rgb_format_name(fmt), (unsigned)(done_aus + 1U),
			     (unsigned)au_size, (unsigned)cost_ms, (unsigned)ctx.frame_done_count);

			ret = bk_h264_decode_get_info(dec, &info);
			if (ret != AVDK_ERR_OK || vcdec_h264_check_info(stream_cfg, &info, au_ptr, au_size) != BK_OK) {
				fail_stage = "check_info";
				ret = AVDK_ERR_GENERIC;
				goto cleanup;
			}
			done_aus++;
		}
	}

	if (done_aus == 0U || ctx.frame_done_count != done_aus || ctx.last_frame_status != BK_OK) {
		fail_stage = "done_count";
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}

	LOGI("vcdec_h264_frame_%s decode summary: frames=%u total=%u ms avg=%u ms min=%u ms max=%u ms\r\n",
	     vcdec_h264_rgb_format_name(fmt), (unsigned)done_aus, (unsigned)total_decode_ms,
	     (unsigned)(total_decode_ms / done_aus), (unsigned)min_decode_ms, (unsigned)max_decode_ms);
	test_pass = 1U;

cleanup:
	vcdec_h264_destroy_decoder(&dec);
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}
	vcdec_h264_rgb_log_format_result(fmt == BK_PIXEL_FORMAT_RGB565 ?
					 "vcdec_h264_frame_rgb565" : "vcdec_h264_frame_rgb888",
					 test_pass, fail_stage, ret, done_aus);
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_frame_rgb_test(void)
{
	LOGI("vcdec h264 frame RGB format test start\r\n");
	if (vcdec_h264_frame_rgb_run(BK_PIXEL_FORMAT_RGB565) != BK_OK) {
		return;
	}
	if (vcdec_h264_frame_rgb_run(BK_PIXEL_FORMAT_RGB888) != BK_OK) {
		return;
	}
	LOGI("[RESULT][PASS] vcdec_h264_frame_rgb_test success\r\n");
}
