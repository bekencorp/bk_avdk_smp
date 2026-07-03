#include "vcdec_h264_test_common.h"
#include "h264_decode_h264_parser.h"

#define TAG "vcdec_h264_fzc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VCDEC_H264_FZC_DISP_DEPTH      2U
#define VCDEC_H264_FZC_DEQUEUE_TMO_MS  200U

static uint32_t vcdec_h264_fzc_signature(const uint8_t *buf, uint32_t size)
{
	uint32_t sig = 0U;
	uint32_t i;
	uint32_t step;

	if (buf == NULL || size == 0U) {
		return 0U;
	}
	step = (size > 256U) ? (size / 256U) : 1U;
	for (i = 0; i < size; i += step) {
		sig = (sig * 131U) + buf[i];
	}
	return sig;
}

/*
 * Drain every currently-ready display frame from the frame-zerocopy controller,
 * validate display (POC) ordering and pixel content, then release each frame.
 * Returns the number of frames drained, accumulating stats via the in/out ptrs.
 */
static uint32_t vcdec_h264_fzc_drain(bk_h264_decode_ctlr_handle_t dec, uint32_t timeout_ms,
				      int32_t *last_poc, uint32_t *poc_violations,
				      uint32_t *nonzero_sig, uint32_t *disp_b,
				      uint32_t width, uint32_t height)
{
	uint32_t drained = 0U;

	for (;;) {
		bk_h264_decode_dequeue_t dq = {0};
		uint32_t sig;

		dq.timeout_ms = timeout_ms;
		if (bk_h264_decode_ioctl(dec, BK_H264_DECODE_IOCTL_DEQUEUE, &dq) != AVDK_ERR_OK) {
			break; /* timeout / empty */
		}

		if (dq.frame.frame_type == VCDEC_H264_FRAME_IDR) {
			*last_poc = INT32_MIN;
		}
		if (dq.frame.poc <= *last_poc) {
			(*poc_violations)++;
			LOGE("POC order violation: poc=%ld last=%ld type=%s\r\n",
			     (long)dq.frame.poc, (long)*last_poc,
			     vcdec_h264_frame_type_name((bk_h264_decode_frame_type_t)dq.frame.frame_type));
		}
		*last_poc = dq.frame.poc;

		if (dq.frame.width != width || dq.frame.height != height) {
			LOGE("frame geometry %ux%u != expected %ux%u\r\n",
			     (unsigned)dq.frame.width, (unsigned)dq.frame.height,
			     (unsigned)width, (unsigned)height);
			(*poc_violations)++;
		}

		sig = vcdec_h264_fzc_signature(dq.frame.data, dq.frame.data_len);
		if (sig != 0U) {
			(*nonzero_sig)++;
		}
		if (dq.frame.frame_type == VCDEC_H264_FRAME_B) {
			(*disp_b)++;
		}

		LOGI("disp poc=%ld type=%s %ux%u len=%u sig=0x%08x\r\n",
		     (long)dq.frame.poc,
		     vcdec_h264_frame_type_name((bk_h264_decode_frame_type_t)dq.frame.frame_type),
		     (unsigned)dq.frame.width, (unsigned)dq.frame.height,
		     (unsigned)dq.frame.data_len, (unsigned)sig);

		(void)bk_h264_decode_ioctl(dec, BK_H264_DECODE_IOCTL_RELEASE, &dq.frame);
		drained++;
		timeout_ms = 0U; /* only the first dequeue of a drain may block */
	}
	return drained;
}

/*
 * Zero-copy / B-frame decode test on the bk_decoder frame-zerocopy controller
 * (bk_h264_decode_frame_zerocopy_ctlr_new). Frames are retrieved via
 * DEQUEUE/RELEASE ioctls, then validated for POC display ordering, non-zero
 * pixel content and frame accounting (display count == decoded count).
 */
static bk_err_t vcdec_h264_fzc_run(h264_decode_test_stream_t stream_id)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg = vcdec_h264_get_stream_cfg(stream_id);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint32_t decoded = 0U;
	uint32_t dec_i = 0U, dec_p = 0U, dec_b = 0U;
	uint32_t disp_total = 0U;
	uint32_t disp_b = 0U;
	uint32_t nonzero_sig = 0U;
	uint32_t poc_violations = 0U;
	int32_t last_poc = INT32_MIN;
	const char *fail_stage = "start";
	uint8_t test_pass = 0U;
	avdk_err_t ret = AVDK_ERR_OK;
	uint32_t offset = 0U;

	if (stream_cfg == NULL || stream_cfg->stream == NULL || stream_cfg->bytes == NULL ||
	    *stream_cfg->bytes == 0U) {
		LOGE("invalid stream cfg\r\n");
		return BK_FAIL;
	}

	os_memset(&ctx, 0, sizeof(ctx));
	LOGI("vcdec h264 frame-zerocopy test start, stream=%s %ux%u bytes=%u\r\n",
	     stream_cfg->name, (unsigned)stream_cfg->width, (unsigned)stream_cfg->height,
	     (unsigned)(*stream_cfg->bytes));

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, *stream_cfg->bytes);
	if (stream_buf == NULL) {
		fail_stage = "alloc_stream_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, stream_cfg->stream, *stream_cfg->bytes);

	{
		bk_h264_decode_frame_zerocopy_config_t cfg = DEFAULT_H264_DECODE_FRAME_ZEROCOPY_CONFIG;

		cfg.timeout_ms = 1000U;
		cfg.out_width = (uint16_t)stream_cfg->width;
		cfg.out_height = (uint16_t)stream_cfg->height;
		cfg.out_format = BK_PIXEL_FORMAT_NV12;
		cfg.disp_depth = VCDEC_H264_FZC_DISP_DEPTH;
		cfg.frame_done_cb = vcdec_h264_frame_done_cb;
		cfg.frame_done_args = &ctx;
		ret = bk_h264_decode_frame_zerocopy_ctlr_new(&dec, &cfg);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "frame_zerocopy_ctlr_new";
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
		/* out_buffer is intentionally unused in frame-zerocopy mode. */

		ret = bk_h264_decode_frame(dec, &in);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "decode_frame";
			LOGE("decode au failed, au=%u ret=%d\r\n", (unsigned)(decoded + 1U), ret);
			goto cleanup;
		}

		if (bk_h264_decode_get_info(dec, &info) == AVDK_ERR_OK) {
			decoded++;
			if (info.frame_type == VCDEC_H264_FRAME_IDR || info.frame_type == VCDEC_H264_FRAME_I) {
				dec_i++;
			} else if (info.frame_type == VCDEC_H264_FRAME_P) {
				dec_p++;
			} else if (info.frame_type == VCDEC_H264_FRAME_B) {
				dec_b++;
			}
			LOGI("dec au=%u type=%s ref=%u\r\n",
			     (unsigned)decoded,
			     vcdec_h264_frame_type_name(info.frame_type),
			     (unsigned)info.is_reference);
		}

		disp_total += vcdec_h264_fzc_drain(dec, 0U, &last_poc, &poc_violations,
						    &nonzero_sig, &disp_b,
						    stream_cfg->width, stream_cfg->height);
	}

	/* Flush trailing reordered pictures and drain the rest. */
	if (bk_h264_decode_ioctl(dec, BK_H264_DECODE_IOCTL_FLUSH, NULL) != AVDK_ERR_OK) {
		fail_stage = "flush";
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}
	disp_total += vcdec_h264_fzc_drain(dec, VCDEC_H264_FZC_DEQUEUE_TMO_MS, &last_poc,
					    &poc_violations, &nonzero_sig, &disp_b,
					    stream_cfg->width, stream_cfg->height);

	if (decoded == 0U) {
		fail_stage = "no_access_unit";
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}
	if (disp_total != decoded) {
		fail_stage = "disp_count";
		ret = AVDK_ERR_GENERIC;
		LOGE("display count %u != decoded %u (frame leak/loss)\r\n",
		     (unsigned)disp_total, (unsigned)decoded);
		goto cleanup;
	}
	if (poc_violations != 0U) {
		fail_stage = "poc_order";
		ret = AVDK_ERR_GENERIC;
		LOGE("POC display-order violations=%u\r\n", (unsigned)poc_violations);
		goto cleanup;
	}
	if (nonzero_sig == 0U) {
		fail_stage = "all_zero_output";
		ret = AVDK_ERR_GENERIC;
		LOGE("all decoded outputs sampled as zero\r\n");
		goto cleanup;
	}

	test_pass = 1U;
	LOGI("FZC_RESULT: stream=%s frames=%u I=%u P=%u B=%u disp=%u dispB=%u\r\n",
	     stream_cfg->name, (unsigned)decoded, (unsigned)dec_i, (unsigned)dec_p,
	     (unsigned)dec_b, (unsigned)disp_total, (unsigned)disp_b);

cleanup:
	vcdec_h264_destroy_decoder(&dec);
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}
	os_memset(&ctx, 0, sizeof(ctx));

	vcdec_h264_log_result("vcdec_h264_frame_zerocopy_test", test_pass, fail_stage, ret,
			      decoded, VCDEC_H264_TEST_ROUNDS);
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_frame_zerocopy_test(h264_decode_test_stream_t stream)
{
	(void)vcdec_h264_fzc_run(stream);
}
