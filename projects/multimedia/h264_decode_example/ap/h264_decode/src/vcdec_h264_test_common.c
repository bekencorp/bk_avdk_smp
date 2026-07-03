#include "vcdec_h264_test_common.h"
#include "h264_decode_stream_1280x720.h"

#define TAG "vcdec_h264_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

static const vcdec_h264_test_stream_cfg_t s_vcdec_h264_stream_1i30p = {
	.id = H264_DECODE_TEST_STREAM_1280X720_1I30P,
	.name = "1280x720_1i30p",
	.stream = h264_decode_stream_1280x720_1i30p,
	.bytes = &h264_decode_stream_1280x720_1i30p_bytes,
	.width = 1280U,
	.height = 720U,
};

static const vcdec_h264_test_stream_cfg_t s_vcdec_h264_stream_ibbp = {
	.id = H264_DECODE_TEST_STREAM_1280X720_IBBP,
	.name = "1280x720_ibbp",
	.stream = h264_decode_stream_1280x720_ibbp,
	.bytes = &h264_decode_stream_1280x720_ibbp_bytes,
	.width = 1280U,
	.height = 720U,
};

const vcdec_h264_test_stream_cfg_t *vcdec_h264_get_stream_cfg(h264_decode_test_stream_t stream)
{
	switch (stream) {
	case H264_DECODE_TEST_STREAM_1280X720_1I30P:
		return &s_vcdec_h264_stream_1i30p;
	case H264_DECODE_TEST_STREAM_1280X720_IBBP:
		return &s_vcdec_h264_stream_ibbp;
	default:
		return NULL;
	}
}

const char *vcdec_h264_frame_type_name(bk_h264_decode_frame_type_t type)
{
	switch (type) {
	case VCDEC_H264_FRAME_IDR:
		return "IDR";
	case VCDEC_H264_FRAME_I:
		return "I";
	case VCDEC_H264_FRAME_P:
		return "P";
	case VCDEC_H264_FRAME_B:
		return "B";
	default:
		return "UNKNOWN";
	}
}

uint32_t vcdec_h264_frame_size(uint32_t width, uint32_t height)
{
	return bk_image_size_get((uint16_t)width, (uint16_t)height, BK_PIXEL_FORMAT_NV12);
}

void vcdec_h264_destroy_decoder(bk_h264_decode_ctlr_handle_t *dec)
{
	if (dec == NULL || *dec == NULL) {
		return;
	}

	(void)bk_h264_decode_close(*dec);
	(void)bk_h264_decode_deinit(*dec);
	(void)bk_h264_decode_delete(*dec);
	*dec = NULL;
}

void vcdec_h264_dump_nv12(const char *tag, uint8_t *buf, uint32_t size)
{
#if VCDEC_H264_TEST_DUMP_ENABLE
	uint32_t dump_size = (size + 3U) & ~3U;

	if (tag == NULL || buf == NULL || size == 0U) {
		return;
	}

	LOGI("dump %s buf=%p size=%u\r\n", tag, buf, (unsigned)size);
	stack_mem_dump((uint32_t)(uintptr_t)buf, (uint32_t)(uintptr_t)(buf + dump_size));
#else
	(void)tag;
	(void)buf;
	(void)size;
#endif
}

void vcdec_h264_log_result(const char *case_name, uint8_t pass,
			   const char *stage, avdk_err_t ret,
			   uint32_t done_aus, uint32_t total_rounds)
{
	if (pass) {
		LOGI("[RESULT][PASS] %s success, decoded_aus=%u, rounds=%u\r\n",
		     case_name, (unsigned)done_aus, (unsigned)total_rounds);
	} else {
		LOGE("[RESULT][FAIL] %s failed at %s, ret=%d, decoded_aus=%u, rounds=%u\r\n",
		     case_name, stage, ret, (unsigned)done_aus, (unsigned)total_rounds);
	}
}

bk_err_t vcdec_h264_check_info(const vcdec_h264_test_stream_cfg_t *stream_cfg,
			       const bk_h264_decode_info_t *info,
			       const uint8_t *au_ptr, uint32_t au_size)
{
	if (stream_cfg == NULL || info == NULL) {
		return BK_FAIL;
	}

	if (info->width != stream_cfg->width || info->height != stream_cfg->height) {
		LOGE("unexpected frame size: got=%ux%u expect=%ux%u\r\n",
		     (unsigned)info->width, (unsigned)info->height,
		     (unsigned)stream_cfg->width, (unsigned)stream_cfg->height);
		return BK_FAIL;
	}

	if (info->input_stream != (uint8_t *)(uintptr_t)au_ptr || info->input_stream_len != au_size) {
		LOGE("last_info stream mismatch: got=%p/%u expect=%p/%u\r\n",
		     info->input_stream, (unsigned)info->input_stream_len,
		     au_ptr, (unsigned)au_size);
		return BK_FAIL;
	}

	return BK_OK;
}

void vcdec_h264_frame_done_cb(int status, void *args)
{
	vcdec_h264_test_ctx_t *ctx = (vcdec_h264_test_ctx_t *)args;

	if (ctx == NULL) {
		return;
	}

	ctx->frame_done_count++;
	ctx->last_frame_status = status;
	if (status != BK_OK) {
		LOGE("frame done callback reported status=%d\r\n", status);
	}
}
