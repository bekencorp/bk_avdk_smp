#include <stdint.h>
#include "os/os.h"
#include "os/mem.h"
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_decode/bk_h264_decode_ctlr.h>
#include "bk_flexa_bond_types.h"
#include "h264_decode_test.h"
#include "h264_decode_h264_parser.h"
#include "h264_decode_stream_1280x720.h"
#include "h264_decode_stream_256x128.h"

#define TAG "vcdec_h264_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB  1U
#define VCDEC_H264_TEST_FLEXA_SEG_NUM        2U
#define VCDEC_H264_TEST_ROUNDS               1U
#define VCDEC_H264_TEST_DUMP_ENABLE          0
#define VCDEC_BOOT_DEMO_TASK_PRIORITY        (BEKEN_DEFAULT_WORKER_PRIORITY)
#define VCDEC_BOOT_DEMO_TASK_STACK_SIZE      (1024 * 16)

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

typedef enum {
	VCDEC_H264_TEST_MODE_FRAME = 0,
	VCDEC_H264_TEST_MODE_FLEXA = 1,
} vcdec_h264_test_mode_t;

typedef struct {
	h264_decode_test_stream_t id;
	const char *name;
	const uint8_t *stream;
	const uint32_t *bytes;
	uint32_t width;
	uint32_t height;
} vcdec_h264_test_stream_cfg_t;

typedef struct {
	volatile uint32_t frame_done_count;
	volatile int last_frame_status;
	volatile uint32_t flexa_done_count;
	volatile uint32_t last_wr_ptr;
	/* FLEXA rd-ptr advance needs only the segment geometry below. */
	uint32_t frame_height;
	uint32_t pp_seg_lines;
	bk_h264_decode_ctlr_handle_t dec;
} vcdec_h264_test_ctx_t;

static const vcdec_h264_test_stream_cfg_t s_vcdec_h264_test_stream_1280x720 = {
	.id = H264_DECODE_TEST_STREAM_1280X720,
	.name = "1280x720",
	.stream = h264_decode_stream_1280x720,
	.bytes = &h264_decode_stream_1280x720_bytes,
	.width = 1280U,
	.height = 720U,
};

static const vcdec_h264_test_stream_cfg_t s_vcdec_h264_test_stream_256x128 = {
	.id = H264_DECODE_TEST_STREAM_256X128,
	.name = "256x128",
	.stream = h264_decode_stream_256x128,
	.bytes = &h264_decode_stream_256x128_bytes,
	.width = 256U,
	.height = 128U,
};

static bk_flexa_bond_t s_h264_flexa_bond = {0};
static beken_thread_t s_vcdec_boot_demo_thread = NULL;
static volatile uint8_t s_vcdec_boot_demo_running = 0;

static uint32_t vcdec_h264_frame_size(uint32_t width, uint32_t height)
{
	return bk_image_size_get((uint16_t)width, (uint16_t)height, BK_PIXEL_FORMAT_NV12);
}

static uint32_t vcdec_h264_flexa_size(uint32_t width)
{
	return bk_image_size_get((uint16_t)width,
	                         16U * VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB * VCDEC_H264_TEST_FLEXA_SEG_NUM,
	                         BK_PIXEL_FORMAT_NV12);
}

static const vcdec_h264_test_stream_cfg_t *vcdec_h264_get_stream_cfg(h264_decode_test_stream_t stream)
{
	switch (stream) {
	case H264_DECODE_TEST_STREAM_1280X720:
		return &s_vcdec_h264_test_stream_1280x720;
	case H264_DECODE_TEST_STREAM_256X128:
		return &s_vcdec_h264_test_stream_256x128;
	default:
		return NULL;
	}
}

static const char *vcdec_h264_frame_type_name(bk_h264_decode_frame_type_t type)
{
	switch (type) {
	case VCDEC_H264_FRAME_IDR:
		return "IDR";
	case VCDEC_H264_FRAME_I:
		return "I";
	case VCDEC_H264_FRAME_P:
		return "P";
	default:
		return "UNKNOWN";
	}
}

static void vcdec_h264_destroy_decoder(bk_h264_decode_ctlr_handle_t *dec)
{
	if (dec == NULL || *dec == NULL) {
		return;
	}

	(void)bk_h264_decode_close(*dec);
	(void)bk_h264_decode_deinit(*dec);
	(void)bk_h264_decode_delete(*dec);
	*dec = NULL;
}

static void vcdec_h264_dump_nv12(const char *tag, uint8_t *buf, uint32_t size)
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

static void vcdec_h264_log_result(const char *case_name, uint8_t pass,
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

static void vcdec_h264_frame_done_cb(int status, void *args)
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

static bk_err_t vcdec_h264_check_info(const vcdec_h264_test_stream_cfg_t *stream_cfg,
				      const bk_h264_decode_info_t *info,
				      const uint8_t *au_ptr,
				      uint32_t au_size)
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

static bk_err_t vcdec_h264_run(vcdec_h264_test_mode_t mode,
			       h264_decode_test_stream_t stream_id)
{
	const vcdec_h264_test_stream_cfg_t *stream_cfg = vcdec_h264_get_stream_cfg(stream_id);
	vcdec_h264_test_ctx_t ctx;
	bk_h264_decode_ctlr_handle_t dec = NULL;
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint8_t *pp_buf = NULL;
	uint32_t frame_size;
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

	if (stream_cfg == NULL || stream_cfg->stream == NULL || stream_cfg->bytes == NULL ||
	    *stream_cfg->bytes == 0U) {
		LOGE("invalid stream cfg\r\n");
		return BK_FAIL;
	}

	frame_size = vcdec_h264_frame_size(stream_cfg->width, stream_cfg->height);
	pp_size = vcdec_h264_flexa_size(stream_cfg->width);
	os_memset(&ctx, 0, sizeof(ctx));

	LOGI("vcdec h264 test start, mode=%s stream=%s %ux%u bytes=%u\r\n",
	     (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame",
	     stream_cfg->name,
	     (unsigned)stream_cfg->width,
	     (unsigned)stream_cfg->height,
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

	if (mode == VCDEC_H264_TEST_MODE_FRAME) {
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
	} else {
		bk_h264_decode_flexa_config_t cfg = DEFAULT_H264_DECODE_FLEXA_CONFIG;

		pp_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, pp_size);
		if (pp_buf == NULL) {
			fail_stage = "alloc_pp_buf";
			ret = AVDK_ERR_NOMEM;
			goto cleanup;
		}
		os_memset(pp_buf, 0, pp_size);

		ctx.frame_height = stream_cfg->height;
		ctx.pp_seg_lines = 16U * VCDEC_H264_TEST_FLEXA_SEG_HEIGHT_MB;

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

	if (mode == VCDEC_H264_TEST_MODE_FLEXA) {
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
	}

	uint8_t clear_out_done = 0U;

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
				os_memset(out_buf, 0, frame_size);
				if (pp_buf != NULL) {
					os_memset(pp_buf, 0, pp_size);
				}
				clear_out_done = 1U;
			}

			in.stream = (uint8_t *)(uintptr_t)au_ptr;
			in.stream_len = au_size;
			in.out_buffer = (mode == VCDEC_H264_TEST_MODE_FLEXA) ? pp_buf : out_buf;
			in.out_buffer_size = (mode == VCDEC_H264_TEST_MODE_FLEXA) ? pp_size : frame_size;

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
	expected_frame_done_count = (mode == VCDEC_H264_TEST_MODE_FLEXA) ? (done_aus * 2U) : done_aus;
	if (ctx.frame_done_count != expected_frame_done_count) {
		fail_stage = "frame_done_count";
		ret = AVDK_ERR_GENERIC;
		LOGE("frame done count mismatch: cb=%u expect=%u aus=%u mode=%s\r\n",
		     (unsigned)ctx.frame_done_count,
		     (unsigned)expected_frame_done_count,
		     (unsigned)done_aus,
		     (mode == VCDEC_H264_TEST_MODE_FLEXA) ? "flexa" : "frame");
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

	if (mode == VCDEC_H264_TEST_MODE_FRAME) {
		vcdec_h264_dump_nv12("vcdec_h264_test_out", out_buf, frame_size);
	}
	test_pass = 1U;
	LOGI("vcdec h264 test done, decoded_aus=%u idr=%u i=%u p=%u\r\n",
	     (unsigned)done_aus,
	     (unsigned)frame_type_idr,
	     (unsigned)frame_type_i,
	     (unsigned)frame_type_p);

cleanup:
	if (dec != NULL && mode == VCDEC_H264_TEST_MODE_FLEXA) {
		(void)bk_h264_decode_ioctl(dec, BK_H264_DECODE_IOCTL_UNREGISTER_BOND, &s_h264_flexa_bond);
	}
	vcdec_h264_destroy_decoder(&dec);
	if (pp_buf != NULL) {
		bk_frame_buffer_free(pp_buf);
	}
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}
	os_memset(&ctx, 0, sizeof(ctx));
	os_memset(&s_h264_flexa_bond, 0, sizeof(s_h264_flexa_bond));

	vcdec_h264_log_result(
		(mode == VCDEC_H264_TEST_MODE_FLEXA) ? "vcdec_h264_flexa_test"
							   : "vcdec_h264_test",
		test_pass, fail_stage, ret, done_aus, VCDEC_H264_TEST_ROUNDS);
	return test_pass ? BK_OK : BK_FAIL;
}

void vcdec_h264_frame_test(h264_decode_test_stream_t stream)
{
	(void)vcdec_h264_run(VCDEC_H264_TEST_MODE_FRAME, stream);
}

void vcdec_h264_flexa_test(h264_decode_test_stream_t stream)
{
	(void)vcdec_h264_run(VCDEC_H264_TEST_MODE_FLEXA, stream);
}

static void vcdec_h264_boot_demo_task_entry(void *arg)
{
	(void)arg;

	rtos_delay_milliseconds(1000);
	LOGI("vcdec_h264_flexa_test start\r\n");
	vcdec_h264_flexa_test(H264_DECODE_TEST_STREAM_1280X720);
	LOGI("vcdec_h264_flexa_test done\r\n");

	rtos_delay_milliseconds(1000);
	LOGI("vcdec_h264_test start\r\n");
	vcdec_h264_frame_test(H264_DECODE_TEST_STREAM_1280X720);
	LOGI("vcdec_h264_test done\r\n");

	s_vcdec_boot_demo_running = 0U;
	s_vcdec_boot_demo_thread = NULL;
	rtos_delete_thread(NULL);
}

void vcdec_h264_run_boot_demo(void)
{
	bk_err_t ret;

	if (s_vcdec_boot_demo_running != 0U) {
		LOGE("vcdec h264 boot demo task is already running\r\n");
		return;
	}

	s_vcdec_boot_demo_running = 1U;
	ret = rtos_create_thread(&s_vcdec_boot_demo_thread,
				 VCDEC_BOOT_DEMO_TASK_PRIORITY,
				 "vcdec_boot_demo",
				 (beken_thread_function_t)vcdec_h264_boot_demo_task_entry,
				 VCDEC_BOOT_DEMO_TASK_STACK_SIZE,
				 NULL);
	if (ret != BK_OK) {
		LOGE("create vcdec h264 boot demo task failed, ret=%d\r\n", (int)ret);
		s_vcdec_boot_demo_running = 0U;
		s_vcdec_boot_demo_thread = NULL;
		return;
	}

	LOGI("vcdec h264 boot demo task created\r\n");
}
