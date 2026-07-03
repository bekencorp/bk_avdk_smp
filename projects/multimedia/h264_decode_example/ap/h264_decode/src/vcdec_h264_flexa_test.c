#include "vcdec_h264_test_common.h"
#include "bk_flexa_bond_types.h"
#include "h264_decode_h264_parser.h"

#define TAG "vcdec_h264_flexa"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB  1U
#define VCDEC_H264_TEST_FLEXA_SEG_NUM        2U

static bk_flexa_bond_t s_h264_flexa_bond = {0};

static uint32_t vcdec_h264_flexa_size(uint32_t width)
{
	return bk_image_size_get((uint16_t)width,
	                         16U * VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB * VCDEC_H264_TEST_FLEXA_SEG_NUM,
	                         BK_PIXEL_FORMAT_NV12);
}

static void vcdec_h264_release_flexa_rd_ptr(vcdec_h264_test_ctx_t *ctx, uint32_t wr_ptr)
{
	uint32_t frame_seg_cnt;
	bk_h264_decode_port_rd_t rd_cmd;

	if (ctx == NULL || ctx->dec == NULL || ctx->pp_seg_lines == 0U || ctx->frame_height == 0U) {
		return;
	}

	if (wr_ptr == 0U) {
		wr_ptr = (ctx->frame_height + ctx->pp_seg_lines - 1U) / ctx->pp_seg_lines;
	}

	frame_seg_cnt = (ctx->frame_height + ctx->pp_seg_lines - 1U) / ctx->pp_seg_lines;
	s_h264_flexa_bond.last_lines = wr_ptr;
	rd_cmd.port_ptr = &s_h264_flexa_bond;
	rd_cmd.rd_blocks = wr_ptr;
	(void)bk_h264_decode_ioctl(ctx->dec, BK_H264_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);

	if (wr_ptr == frame_seg_cnt) {
		(void)bk_h264_decode_ioctl(ctx->dec, BK_H264_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE, &s_h264_flexa_bond);
	}
}

static void vcdec_h264_flexa_done_cb(uint32_t wr_ptr, void *args)
{
	vcdec_h264_test_ctx_t *ctx = (vcdec_h264_test_ctx_t *)args;

	if (ctx == NULL) {
		return;
	}

	ctx->flexa_done_count++;
	ctx->last_wr_ptr = wr_ptr;
	vcdec_h264_release_flexa_rd_ptr(ctx, wr_ptr);
}

static void bond_h264d_flexa_done_cb(uint32_t wr_cnt, void *args)
{
	(void)wr_cnt;
	(void)args;
}

static void bond_h264d_frame_done_cb(uint32_t status, void *args)
{
	(void)args;

	if (status != BK_OK) {
		LOGE("bond frame done callback reported status=%u\r\n", (unsigned)status);
	}
}

/*
 * Segmented (FLEXA) decode test on the bk_decoder flexa controller
 * (bk_h264_decode_flexa_ctlr_new). Drives the consumer read-pointer advance via
 * a bk_flexa_bond and validates frame-done / flexa-done callbacks and frame-type
 * coverage.
 */
static bk_err_t vcdec_h264_flexa_run(h264_decode_test_stream_t stream_id)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg = vcdec_h264_get_stream_cfg(stream_id);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint8_t *pp_buf = NULL;
	uint32_t pp_size;
	uint32_t done_aus = 0U;
	uint32_t frame_type_idr = 0U;
	uint32_t frame_type_i = 0U;
	uint32_t frame_type_p = 0U;
	uint32_t expected_frame_done_count = 0U;
	const char *fail_stage = "start";
	uint8_t test_pass = 0U;
	avdk_err_t ret = AVDK_ERR_OK;
	uint32_t round;
	uint8_t clear_out_done = 0U;

	if (stream_cfg == NULL || stream_cfg->stream == NULL || stream_cfg->bytes == NULL ||
	    *stream_cfg->bytes == 0U) {
		LOGE("invalid stream cfg\r\n");
		return BK_FAIL;
	}

	pp_size = vcdec_h264_flexa_size(stream_cfg->width);
	os_memset(&ctx, 0, sizeof(ctx));

	LOGI("vcdec h264 flexa test start, stream=%s %ux%u bytes=%u\r\n",
	     stream_cfg->name, (unsigned)stream_cfg->width, (unsigned)stream_cfg->height,
	     (unsigned)(*stream_cfg->bytes));

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, *stream_cfg->bytes);
	if (stream_buf == NULL) {
		fail_stage = "alloc_stream_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, stream_cfg->stream, *stream_cfg->bytes);

	pp_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, pp_size);
	if (pp_buf == NULL) {
		fail_stage = "alloc_pp_buf";
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memset(pp_buf, 0, pp_size);

	ctx.frame_height = stream_cfg->height;
	ctx.pp_seg_lines = 16U * VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB;

	{
		bk_h264_decode_flexa_config_t cfg = DEFAULT_H264_DECODE_FLEXA_CONFIG;

		cfg.timeout_ms = 1000U;
		cfg.out_width = (uint16_t)stream_cfg->width;
		cfg.out_height = (uint16_t)stream_cfg->height;
		cfg.out_format = BK_PIXEL_FORMAT_NV12;
		cfg.segment_height = VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB;
		cfg.segment_number = VCDEC_H264_TEST_FLEXA_SEG_NUM;
		cfg.frame_done_cb = vcdec_h264_frame_done_cb;
		cfg.frame_done_args = &ctx;
		cfg.flexa_done_cb = vcdec_h264_flexa_done_cb;
		cfg.flexa_done_args = &ctx;
		ret = bk_h264_decode_flexa_ctlr_new(&dec, &cfg);
		if (ret != AVDK_ERR_OK) {
			fail_stage = "flexa_ctlr_new";
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

	s_h264_flexa_bond.flexa_done = bond_h264d_flexa_done_cb;
	s_h264_flexa_bond.frame_done = bond_h264d_frame_done_cb;
	s_h264_flexa_bond.bond_config = NULL;
	s_h264_flexa_bond.handle = dec;
	s_h264_flexa_bond.max_lines_per_frame =
		(stream_cfg->height + ctx.pp_seg_lines - 1U) / ctx.pp_seg_lines;
	ret = bk_h264_decode_ioctl(dec, BK_H264_DECODE_IOCTL_REGISTER_BOND, &s_h264_flexa_bond);
	if (ret != AVDK_ERR_OK) {
		fail_stage = "register_bond";
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

			if (!clear_out_done) {
				os_memset(pp_buf, 0, pp_size);
				clear_out_done = 1U;
			}

			in.stream = (uint8_t *)(uintptr_t)au_ptr;
			in.stream_len = au_size;
			in.out_buffer = pp_buf;
			in.out_buffer_size = pp_size;

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

			LOGI("decoded au=%u size=%u type=%s ref=%u frame_done=%u flexa_done=%u\r\n",
			     (unsigned)(done_aus + 1U),
			     (unsigned)au_size,
			     vcdec_h264_frame_type_name(info.frame_type),
			     (unsigned)info.is_reference,
			     (unsigned)ctx.frame_done_count,
			     (unsigned)ctx.flexa_done_count);
			done_aus++;
		}
	}

	if (done_aus == 0U) {
		fail_stage = "no_access_unit";
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}
	/*
	 * FLEXA controller currently forwards two frame-done notifications:
	 * 1. the underlying vcdec frame-done callback
	 * 2. the controller-level completion callback after registered ports finish
	 */
	expected_frame_done_count = done_aus * 2U;
	if (ctx.frame_done_count != expected_frame_done_count) {
		fail_stage = "frame_done_count";
		ret = AVDK_ERR_GENERIC;
		LOGE("frame done count mismatch: cb=%u expect=%u aus=%u\r\n",
		     (unsigned)ctx.frame_done_count,
		     (unsigned)expected_frame_done_count,
		     (unsigned)done_aus);
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

	test_pass = 1U;
	LOGI("vcdec h264 flexa test done, decoded_aus=%u idr=%u i=%u p=%u\r\n",
	     (unsigned)done_aus, (unsigned)frame_type_idr,
	     (unsigned)frame_type_i, (unsigned)frame_type_p);

cleanup:
	if (dec != NULL) {
		(void)bk_h264_decode_ioctl(dec, BK_H264_DECODE_IOCTL_UNREGISTER_BOND, &s_h264_flexa_bond);
	}
	vcdec_h264_destroy_decoder(&dec);
	if (pp_buf != NULL) {
		bk_frame_buffer_free(pp_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}
	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&s_h264_flexa_bond, 0, sizeof(s_h264_flexa_bond));

	vcdec_h264_log_result("vcdec_h264_flexa_test", test_pass, fail_stage, ret,
			      done_aus, VCDEC_H264_TEST_ROUNDS);
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_flexa_test(h264_decode_test_stream_t stream)
{
	(void)vcdec_h264_flexa_run(stream);
}
