#include "vcdec_h264_test_common.h"
#include "h264_decode_h264_parser.h"

#define TAG "vcdec_h264_osd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VCDEC_H264_OSD_WIDTH   320U
#define VCDEC_H264_OSD_HEIGHT  320U

extern const unsigned int h264d_xrgb888_320x320_len;
extern const unsigned char h264d_xrgb888_320x320[];

static uint32_t vcdec_h264_osd_output_size(uint32_t width, uint32_t height, bk_pixel_format_t fmt)
{
	uint32_t h_aligned = (height + 15U) & ~15U;

	if (fmt == BK_PIXEL_FORMAT_RGB888) {
		/* VCDec PP RGB888 path writes one pixel per 32-bit word. */
		return width * h_aligned * 4U;
	}

	return bk_image_size_get((uint16_t)width, (uint16_t)h_aligned, fmt);
}

static const char *vcdec_h264_osd_format_name(bk_pixel_format_t fmt)
{
	return (fmt == BK_PIXEL_FORMAT_RGB565) ? "rgb565" : "rgb888";
}

/*
 * Frame-mode H.264 decode with PP OSD alpha-blend.
 * The decoder converts 1280x720 H.264 to RGB565/RGB888 and blends a 320x320
 * ARGB OSD image at the top-left corner.
 */
static bk_err_t vcdec_h264_osd_run(bk_pixel_format_t fmt)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg =
		vcdec_h264_get_stream_cfg(H264_DECODE_TEST_STREAM_1280X720_1I30P);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint8_t *osd_buf = NULL;
	uint32_t osd_size = 0U;
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

	out_size = vcdec_h264_osd_output_size(stream_cfg->width, stream_cfg->height, fmt);
	os_memset(&ctx, 0, sizeof(ctx));

	LOGI("vcdec h264 osd test start, stream=%s %ux%u fmt=%s bytes=%u\r\n",
	     stream_cfg->name, (unsigned)stream_cfg->width, (unsigned)stream_cfg->height,
	     vcdec_h264_osd_format_name(fmt), (unsigned)(*stream_cfg->bytes));

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

	osd_size = h264d_xrgb888_320x320_len;
	osd_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, osd_size);
	if (osd_buf == NULL) {
		LOGE("alloc osd buffer failed, size=%u\r\n", (unsigned)osd_size);
		fail_stage = "alloc_osd_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(osd_buf, h264d_xrgb888_320x320, osd_size);

	{
		bk_h264_decode_frame_config_t cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;

		cfg.timeout_ms = 1000U;
		cfg.out_width = (uint16_t)stream_cfg->width;
		cfg.out_height = (uint16_t)stream_cfg->height;
		cfg.out_format = fmt;
		cfg.frame_done_cb = vcdec_h264_frame_done_cb;
		cfg.frame_done_args = &ctx;
		cfg.osd[0].enable = 1U;
		cfg.osd[0].originX = 0;
		cfg.osd[0].originY = 0;
		cfg.osd[0].height = VCDEC_H264_OSD_HEIGHT;
		cfg.osd[0].width = VCDEC_H264_OSD_WIDTH;
		cfg.osd[0].alphaBlendEna = 1U;
		cfg.osd[0].blendComponentBase = osd_buf;
		cfg.osd[0].blendWidth = VCDEC_H264_OSD_WIDTH;
		cfg.osd[0].blendHeight = VCDEC_H264_OSD_HEIGHT;
		LOGI("osd enabled: pos=%ux%u size=%ux%u fmt=%u buf=%p bytes=%u\r\n",
		     (unsigned)cfg.osd[0].originX,
		     (unsigned)cfg.osd[0].originY,
		     (unsigned)cfg.osd[0].width,
		     (unsigned)cfg.osd[0].height,
		     (unsigned)fmt,
		     cfg.osd[0].blendComponentBase,
		     (unsigned)osd_size);

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
			uint32_t decode_start_ms;
			uint32_t decode_cost_ms;

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

			decode_start_ms = rtos_get_time();
			ret = bk_h264_decode_frame(dec, &in);
			decode_cost_ms = rtos_get_time() - decode_start_ms;
			if (ret != AVDK_ERR_OK) {
				fail_stage = "decode_frame";
				LOGE("osd decode au failed, au=%u ret=%d\r\n",
				     (unsigned)(done_aus + 1U), ret);
				goto cleanup;
			}

			ret = bk_h264_decode_get_info(dec, &info);
			if (ret != AVDK_ERR_OK || vcdec_h264_check_info(stream_cfg, &info, au_ptr, au_size) != BK_OK) {
				fail_stage = "check_info";
				ret = AVDK_ERR_GENERIC;
				goto cleanup;
			}

			total_decode_ms += decode_cost_ms;
			if (decode_cost_ms < min_decode_ms) {
				min_decode_ms = decode_cost_ms;
			}
			if (decode_cost_ms > max_decode_ms) {
				max_decode_ms = decode_cost_ms;
			}

			done_aus++;
			if (done_aus <= 4U) {
				LOGI("osd %s au=%u size=%u decode=%u ms frame_done=%u\r\n",
				     vcdec_h264_osd_format_name(fmt),
				     (unsigned)done_aus,
				     (unsigned)au_size,
				     (unsigned)decode_cost_ms,
				     (unsigned)ctx.frame_done_count);
			}
		}
	}

	if (done_aus == 0U || ctx.frame_done_count != done_aus || ctx.last_frame_status != BK_OK) {
		fail_stage = "done_count";
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}

	LOGI("osd %s decode summary: frames=%u total=%u ms avg=%u ms min=%u ms max=%u ms\r\n",
	     vcdec_h264_osd_format_name(fmt), (unsigned)done_aus, (unsigned)total_decode_ms,
	     (unsigned)(total_decode_ms / done_aus), (unsigned)min_decode_ms, (unsigned)max_decode_ms);
	test_pass = 1U;

cleanup:
	vcdec_h264_destroy_decoder(&dec);
	if (osd_buf != NULL) {
		bk_frame_buffer_free(osd_buf);
	}
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}

	if (test_pass) {
		LOGI("[RESULT][PASS] vcdec_h264_frame_%s_osd decoded_aus=%u\r\n",
		     vcdec_h264_osd_format_name(fmt), (unsigned)done_aus);
	} else {
		LOGE("[RESULT][FAIL] vcdec_h264_frame_%s_osd failed at %s, ret=%d, decoded_aus=%u\r\n",
		     vcdec_h264_osd_format_name(fmt), fail_stage, ret, (unsigned)done_aus);
	}
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_osd_test(void)
{
	LOGI("vcdec h264 OSD test start\r\n");
	if (vcdec_h264_osd_run(BK_PIXEL_FORMAT_RGB565) != BK_OK) {
		return;
	}
	if (vcdec_h264_osd_run(BK_PIXEL_FORMAT_RGB888) != BK_OK) {
		return;
	}
	LOGI("[RESULT][PASS] vcdec_h264_osd_test success\r\n");
}
