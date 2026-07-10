#include "vcdec_h264_test_common.h"
#include "h264_decode_h264_parser.h"

#define TAG "vcdec_h264_scale"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/*
 * Frame-mode H.264 decode with PP down-scale (1280x720 -> 640x360 NV12).
 * Exercises the vcdec PP scaling path added in Gerrit 94584.
 */
static bk_err_t vcdec_h264_frame_scale_run(h264_decode_test_stream_t stream_id)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg = vcdec_h264_get_stream_cfg(stream_id);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint16_t out_w;
	uint16_t out_h;
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

	out_w = (uint16_t)(stream_cfg->width / 2U);
	out_h = (uint16_t)(stream_cfg->height / 2U);
	out_size = vcdec_h264_frame_size(out_w, (uint16_t)(((uint32_t)out_h + 15U) & ~15U));
	os_memset(&ctx, 0, sizeof(ctx));

	LOGI("vcdec h264 scale test start, stream=%s %ux%u -> %ux%u bytes=%u\r\n",
	     stream_cfg->name, (unsigned)stream_cfg->width, (unsigned)stream_cfg->height,
	     (unsigned)out_w, (unsigned)out_h, (unsigned)(*stream_cfg->bytes));

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
		cfg.out_width = out_w;
		cfg.out_height = out_h;
		cfg.out_format = BK_PIXEL_FORMAT_NV12;
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
				LOGE("scale decode au failed, au=%u ret=%d\r\n",
				     (unsigned)(done_aus + 1U), ret);
				goto cleanup;
			}

			ret = bk_h264_decode_get_info(dec, &info);
			if (ret != AVDK_ERR_OK) {
				fail_stage = "get_info";
				goto cleanup;
			}

			if (info.width != out_w || info.height != out_h) {
				fail_stage = "scaled_size";
				LOGE("scaled size mismatch: got %ux%u expect %ux%u\r\n",
				     (unsigned)info.width, (unsigned)info.height,
				     (unsigned)out_w, (unsigned)out_h);
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
				LOGI("scale decode au=%u %ux%u->%ux%u cost=%u ms frame_done=%u\r\n",
				     (unsigned)done_aus,
				     (unsigned)stream_cfg->width, (unsigned)stream_cfg->height,
				     (unsigned)out_w, (unsigned)out_h,
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

	LOGI("scale decode summary: frames=%u total=%u ms avg=%u ms min=%u ms max=%u ms\r\n",
	     (unsigned)done_aus, (unsigned)total_decode_ms,
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

	if (test_pass) {
		LOGI("[RESULT][PASS] vcdec_h264_frame_scale %s decoded_aus=%u\r\n",
		     stream_cfg != NULL ? stream_cfg->name : "unknown", (unsigned)done_aus);
	} else {
		LOGE("[RESULT][FAIL] vcdec_h264_frame_scale failed at %s, ret=%d, decoded_aus=%u\r\n",
		     fail_stage, ret, (unsigned)done_aus);
	}
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_frame_scale_test(h264_decode_test_stream_t stream)
{
	(void)vcdec_h264_frame_scale_run(stream);
}
