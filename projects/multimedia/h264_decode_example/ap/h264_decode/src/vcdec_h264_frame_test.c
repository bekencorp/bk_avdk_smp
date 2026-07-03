#include "vcdec_h264_test_common.h"
#include "h264_decode_h264_parser.h"

#define TAG "vcdec_h264_frame"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/*
 * Whole-frame (non-B) decode test on the bk_decoder frame controller
 * (bk_h264_decode_frame_ctlr_new). Decodes every AU of the selected stream into
 * a single NV12 output buffer and validates frame-done callbacks, info echo and
 * frame-type coverage.
 */
static bk_err_t vcdec_h264_frame_run(h264_decode_test_stream_t stream_id)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg = vcdec_h264_get_stream_cfg(stream_id);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint32_t frame_size;
	uint32_t done_aus = 0U;
	uint32_t frame_type_idr = 0U;
	uint32_t frame_type_i = 0U;
	uint32_t frame_type_p = 0U;
	const char *fail_stage = "start";
	uint8_t test_pass = 0U;
	avdk_err_t ret = AVDK_ERR_OK;
	uint32_t round;

	if (stream_cfg == NULL || stream_cfg->stream == NULL || stream_cfg->bytes == NULL ||
	    *stream_cfg->bytes == 0U) {
		LOGE("invalid stream cfg\r\n");
		return BK_FAIL;
	}

	frame_size = vcdec_h264_frame_size(stream_cfg->width, stream_cfg->height);
	os_memset(&ctx, 0, sizeof(ctx));

	LOGI("vcdec h264 frame test start, stream=%s %ux%u bytes=%u\r\n",
	     stream_cfg->name, (unsigned)stream_cfg->width, (unsigned)stream_cfg->height,
	     (unsigned)(*stream_cfg->bytes));

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, *stream_cfg->bytes);
	if (stream_buf == NULL) {
		fail_stage = "alloc_stream_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, stream_cfg->stream, *stream_cfg->bytes);

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
	if (out_buf == NULL) {
		fail_stage = "alloc_out_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memset(out_buf, 0, frame_size);

	{
		bk_h264_decode_frame_config_t cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;

		cfg.timeout_ms = 1000U;
		cfg.out_width = (uint16_t)stream_cfg->width;
		cfg.out_height = (uint16_t)stream_cfg->height;
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

	for (round = 0U; round < VCDEC_H264_TEST_ROUNDS; round++) {
		uint32_t offset = 0U;

		LOGI("decode round %u/%u start\r\n",
		     (unsigned)(round + 1U), (unsigned)VCDEC_H264_TEST_ROUNDS);

		while (offset < *stream_cfg->bytes) {
			const uint8_t *au_ptr = NULL;
			uint32_t au_size = 0U;
			bk_h264_decode_input_t in = {0};
			bk_h264_decode_info_t info = {0};

			if (h264_decode_h264_next_au(stream_buf, *stream_cfg->bytes, &offset, &au_ptr, &au_size) != AVDK_ERR_OK) {
				break;
			}
			if (au_ptr == NULL || au_size == 0U) {
				continue;
			}

			in.stream = (uint8_t *)(uintptr_t)au_ptr;
			in.stream_len = au_size;
			in.out_buffer = out_buf;
			in.out_buffer_size = frame_size;

			ret = bk_h264_decode_frame(dec, &in);
			if (ret != AVDK_ERR_OK) {
				fail_stage = "decode_frame";
				LOGE("decode au failed, round=%u au=%u ret=%d\r\n",
				     (unsigned)(round + 1U), (unsigned)(done_aus + 1U), ret);
				goto cleanup;
			}

			ret = bk_h264_decode_get_info(dec, &info);
			if (ret != AVDK_ERR_OK) {
				fail_stage = "get_info";
				LOGE("get info failed, au=%u ret=%d\r\n", (unsigned)(done_aus + 1U), ret);
				goto cleanup;
			}

			if (vcdec_h264_check_info(stream_cfg, &info, au_ptr, au_size) != BK_OK) {
				fail_stage = "check_info";
				ret = AVDK_ERR_GENERIC;
				goto cleanup;
			}

			switch (info.frame_type) {
			case VCDEC_H264_FRAME_IDR:
				frame_type_idr++;
				break;
			case VCDEC_H264_FRAME_I:
				frame_type_i++;
				break;
			case VCDEC_H264_FRAME_P:
				frame_type_p++;
				break;
			default:
				break;
			}

			LOGI("decoded au=%u size=%u type=%s ref=%u frame_done=%u\r\n",
			     (unsigned)(done_aus + 1U),
			     (unsigned)au_size,
			     vcdec_h264_frame_type_name(info.frame_type),
			     (unsigned)info.is_reference,
			     (unsigned)ctx.frame_done_count);
			done_aus++;
		}
	}

	if (done_aus == 0U) {
		fail_stage = "no_access_unit";
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}
	if (ctx.frame_done_count != done_aus) {
		fail_stage = "frame_done_count";
		ret = AVDK_ERR_GENERIC;
		LOGE("frame done count mismatch: cb=%u expect=%u\r\n",
		     (unsigned)ctx.frame_done_count, (unsigned)done_aus);
		goto cleanup;
	}
	if (ctx.last_frame_status != BK_OK) {
		fail_stage = "frame_done_status";
		ret = AVDK_ERR_GENERIC;
		LOGE("last frame status=%d\r\n", ctx.last_frame_status);
		goto cleanup;
	}
	if (frame_type_idr == 0U || frame_type_p == 0U) {
		fail_stage = "frame_type_coverage";
		ret = AVDK_ERR_GENERIC;
		LOGE("decoded frame types insufficient: idr=%u i=%u p=%u\r\n",
		     (unsigned)frame_type_idr, (unsigned)frame_type_i, (unsigned)frame_type_p);
		goto cleanup;
	}

	vcdec_h264_dump_nv12("vcdec_h264_test_out", out_buf, frame_size);
	test_pass = 1U;
	LOGI("vcdec h264 frame test done, decoded_aus=%u idr=%u i=%u p=%u\r\n",
	     (unsigned)done_aus, (unsigned)frame_type_idr,
	     (unsigned)frame_type_i, (unsigned)frame_type_p);

cleanup:
	vcdec_h264_destroy_decoder(&dec);
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}
	os_memset(&ctx, 0, sizeof(ctx));

	vcdec_h264_log_result("vcdec_h264_test", test_pass, fail_stage, ret,
			      done_aus, VCDEC_H264_TEST_ROUNDS);
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_frame_test(h264_decode_test_stream_t stream)
{
	(void)vcdec_h264_frame_run(stream);
}
