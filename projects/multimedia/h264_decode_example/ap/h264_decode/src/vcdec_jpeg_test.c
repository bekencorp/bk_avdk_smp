/*
 * JPEG decode test: uses bk_jpeg_decode_* (bk_decoder component) which wraps vcdec HAL.
 * Output dimensions are read from JPEG SOF0 (Start of Frame) before decode.
 */

#include <stdint.h>
#include <string.h>
#include "os/os.h"
#include "os/mem.h"
#include "components/log.h"
#include "components/bk_frame_buffer.h"
#include "cache.h"
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "jpeg_176_144.h"

#define TAG "vcdec_jpeg_test"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VCDEC_JPEG_DECODE_CNT 5U

extern void ppRbReadPointerSet(uint32_t value);

/* JPEG SOF0 marker (Start of Frame, baseline DCT); after 0xFF 0xC0: 2B length, 1B precision, 2B height, 2B width (big-endian) */
#define JPEG_MARKER_SOF0  0xC0
#define JPEG_SOF0_MIN_LEN 9U

/**
 * Parse JPEG stream for SOF0 (0xFF 0xC0) and fill width/height. Returns 0 on success, -1 on parse error.
 */
static int jpeg_parse_sof0_dimensions(const uint8_t *buf, uint32_t size, uint32_t *out_width, uint32_t *out_height)
{
	uint32_t i;

	if (buf == NULL || out_width == NULL || out_height == NULL || size < 2U + JPEG_SOF0_MIN_LEN)
		return -1;

	for (i = 0; i + 2U + JPEG_SOF0_MIN_LEN <= size; i++) {
		if (buf[i] != 0xFF)
			continue;
		if (buf[i + 1] != JPEG_MARKER_SOF0)
			continue;
		*out_height = (uint32_t)((buf[i + 5] << 8) | buf[i + 6]);
		*out_width  = (uint32_t)((buf[i + 7] << 8) | buf[i + 8]);
		if (*out_width == 0U || *out_height == 0U)
			return -1;
		return 0;
	}
	return -1;
}

static void vcdec_jpeg_frame_done_cb(int status, void *args)
{
	LOGI("frame done status=%d\r\n", status);
}

typedef struct {
	uint8_t *frame_y;
	uint8_t *frame_c;
	uint32_t frame_width;
	uint32_t frame_height;
	uint8_t *pp_y;
	uint8_t *pp_c;
	uint32_t pp_seg_num;
	uint32_t pp_seg_rows;
	bk_jpeg_decode_ctlr_handle_t dec;
} vcdec_jpeg_flexa_copy_ctx_t;

static vcdec_jpeg_flexa_copy_ctx_t s_flexa_copy_ctx;

/*
 * PORT_SET_RD_PTR matches ctrl->port[i].bond (see bk_jpeg_decode_ctlr_ioctl).
 * It must be the same pointer passed to REGISTER_BOND — not the decoder handle.
 * Zero-filled blob: flexa_done is NULL so flexa_done_cb skips bond dispatch.
 */
#define VCDEC_JPEG_FLEXA_BOND_STUB_BYTES 128U
static uint8_t s_jpeg_flexa_bond_stub[VCDEC_JPEG_FLEXA_BOND_STUB_BYTES];

static void vcdec_jpeg_flexa_done_cb(uint32_t wrCnt, void *args)
{
	vcdec_jpeg_flexa_copy_ctx_t *ctx = (vcdec_jpeg_flexa_copy_ctx_t *)args;
	if (ctx == NULL)
	{
		ctx = &s_flexa_copy_ctx;
	}
	/*
	 * Flexa mode uses PP output ring-buffer. When one segment is done, PP raises
	 * INT_SRC_H264D_PP and upper layer should advance RD_PTR to release segments.
	 *
	 * In this test, HW outputs to a pingpong ring-buffer (2 x 16 rows). We copy the
	 * finished 16-row segment into the full-frame output buffer, then advance RD_PTR.
	 */
	if (wrCnt == 0U)
		return;

	if (ctx->frame_y == NULL || ctx->frame_c == NULL ||
		ctx->pp_y == NULL || ctx->pp_c == NULL ||
		ctx->frame_width == 0U || ctx->frame_height == 0U ||
		ctx->pp_seg_num == 0U || ctx->pp_seg_rows == 0U) {
		{
			bk_jpeg_decode_port_rd_t rd_cmd = {
				.port_ptr = s_jpeg_flexa_bond_stub,
				.rd_blocks = wrCnt,
			};
			bk_jpeg_decode_ioctl(ctx->dec, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
		}
		return;
	}

	/* rd is the segment index counter already completed (rd_ptr before increment). */
	const uint32_t rd = wrCnt - 1U;
	const uint32_t seg_idx = rd % ctx->pp_seg_num;
	const uint32_t start_row = rd * 16U;
	if (start_row >= ctx->frame_height) {
		bk_jpeg_decode_port_rd_t rd_cmd = {
			.port_ptr = s_jpeg_flexa_bond_stub,
			.rd_blocks = wrCnt,
		};
		bk_jpeg_decode_ioctl(ctx->dec, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
		return;
	}

	uint32_t copy_rows = 16U;
	if (start_row + copy_rows > ctx->frame_height)
		copy_rows = ctx->frame_height - start_row;

	const uint32_t w = ctx->frame_width;
	const uint32_t seg_y_stride = w * 16U;
	const uint32_t seg_c_stride = seg_y_stride / 2U;

	uint8_t *src_y = ctx->pp_y + seg_idx * seg_y_stride;
	uint8_t *src_c = ctx->pp_c + seg_idx * seg_c_stride;
	uint8_t *dst_y = ctx->frame_y + start_row * w;
	uint8_t *dst_c = ctx->frame_c + (start_row * w) / 2U;

	/* Ensure CPU sees HW-written pingpong data. */
	cache_data_invd_range(src_y, w * copy_rows);
	cache_data_invd_range(src_c, (w * copy_rows) / 2U);

	os_memcpy(dst_y, src_y, w * copy_rows);
	os_memcpy(dst_c, src_c, (w * copy_rows) / 2U);

	// extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
	// stack_mem_dump((uint32_t)dst_y, (uint32_t)(dst_y+w*copy_rows));
	// stack_mem_dump((uint32_t)dst_c, (uint32_t)(dst_c+(w*copy_rows)/2U));
	/* Release the segment after copying. */
	{
		bk_jpeg_decode_port_rd_t rd_cmd = {
			.port_ptr = s_jpeg_flexa_bond_stub,
			.rd_blocks = wrCnt,
		};
		bk_jpeg_decode_ioctl(ctx->dec, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
	}

}

void vcdec_jpeg_test(void)
{
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint32_t out_width = 0U;
	uint32_t out_height = 0U;
	uint32_t out_size;
	avdk_err_t ret;
	uint32_t i;
	bk_jpeg_decode_ctlr_handle_t dec = NULL;

	LOGI("enter vcdec_jpeg_test\r\n");

	ret = jpeg_parse_sof0_dimensions((const uint8_t *)JPEGData_176_144, JPEG_176_144_SIZE, &out_width, &out_height);
	if (ret != 0) {
		LOGE("jpeg_parse_sof0_dimensions failed, cannot get image size\r\n");
		return;
	}
	out_size = out_width * out_height * 3U / 2U;
	LOGI("image %ux%u, out_size %u\r\n", (unsigned)out_width, (unsigned)out_height, (unsigned)out_size);

	/*
	 * VCDEC HW accesses buffers via bus master.
	 * Allocate stream/table/output buffers from video mem-slab heaps (PSRAM) to
	 * avoid DEC_BUS_INT caused by non-accessible SRAM heap addresses.
	 */
	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, JPEG_176_144_SIZE);
	if (stream_buf == NULL) {
		LOGE("alloc stream_buf failed\r\n");
		return;
	}
	os_memcpy(stream_buf, (const void *)JPEGData_176_144, JPEG_176_144_SIZE);
	cache_data_flush_range(stream_buf, JPEG_176_144_SIZE);

	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		LOGE("alloc out_buf failed\r\n");
		goto cleanup;
	}
	os_memset(out_buf, 0, out_size);
	cache_data_flush_range(out_buf, out_size);

	bk_jpeg_decode_config_t cfg = {0};
	cfg.decode_mode = BK_JPEG_DECODE_FLEXA_MODE_NONE;
	cfg.frame_done_cb = vcdec_jpeg_frame_done_cb;
	cfg.args = &s_flexa_copy_ctx;
	cfg.timeout_ms = 1000;
	cfg.width = (uint16_t)out_width;
	cfg.height = (uint16_t)out_height;
	cfg.segment_height = (uint16_t)1;
	cfg.segment_number = (uint8_t)2;
	ret = bk_jpeg_decode_new(&dec, &cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_jpeg_decode_new failed %d\r\n", (int)ret);
		goto cleanup;
	}
	ret = bk_jpeg_decode_init(dec);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_jpeg_decode_init failed %d\r\n", (int)ret);
		goto cleanup;
	}
	ret = bk_jpeg_decode_open(dec);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_jpeg_decode_open failed %d\r\n", (int)ret);
		goto cleanup;
	}

	for (i = 0; i < VCDEC_JPEG_DECODE_CNT; i++) {
		LOGI("decode %u/%u\r\n", (unsigned)(i + 1), (unsigned)VCDEC_JPEG_DECODE_CNT);

		bk_jpeg_decode_input_t in = {0};
		in.stream = stream_buf;
		in.stream_len = JPEG_176_144_SIZE;
		in.out_buffer = out_buf;
		in.out_buffer_size = out_size;
		ret = bk_jpeg_decode_frame(dec, &in);
		cache_data_invd_range(out_buf, out_size);
		if (ret != AVDK_ERR_OK) {
			LOGE("bk_jpeg_decode_frame failed %d\r\n", (int)ret);
		}
	}

	LOGI("decode done\r\n");

	extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
	stack_mem_dump((uint32_t)out_buf, (uint32_t)(out_buf+out_size));

	(void)bk_jpeg_decode_close(dec);
	(void)bk_jpeg_decode_deinit(dec);
	(void)bk_jpeg_decode_delete(dec);
	dec = NULL;

cleanup:
	if (dec != NULL) {
		(void)bk_jpeg_decode_close(dec);
		(void)bk_jpeg_decode_deinit(dec);
		(void)bk_jpeg_decode_delete(dec);
	}
	if (out_buf != NULL)
		bk_frame_buffer_free(out_buf);
	if (stream_buf != NULL)
		bk_frame_buffer_free(stream_buf);

	LOGI("exit vcdec_jpeg_test\r\n");
}

void vcdec_jpeg_flexa_test(void)
{
	uint8_t *stream_buf = NULL;
	uint8_t *out_buf = NULL;
	uint8_t *pp_buf = NULL;
	uint32_t out_width = 0U;
	uint32_t out_height = 0U;
	uint32_t out_size;
	uint32_t pp_size;
	avdk_err_t ret;
	uint32_t i;

	/* PP output ring-buffer: 2 segments, 1 MB (16 lines) per segment. */
	const uint32_t seg_ht_mb = 1U;
	const uint32_t seg_num = 2U;
	const uint32_t seg_rows = 16U * seg_ht_mb * seg_num;

	LOGI("enter vcdec_jpeg_flexa_test\r\n");

	ret = jpeg_parse_sof0_dimensions((const uint8_t *)JPEGData_176_144, JPEG_176_144_SIZE, &out_width, &out_height);
	if (ret != 0) {
		LOGE("jpeg_parse_sof0_dimensions failed, cannot get image size\r\n");
		return;
	}

	/* Full-frame output buffer (Y + C). */
	out_size = out_width * out_height * 3U / 2U;
	/* Pingpong buffer for HW Flexa output (2 x 16 rows). */
	pp_size = out_width * seg_rows * 3U / 2U;
	LOGI("image %ux%u, frame_size %u, pingpong_size %u (seg_rows=%u)\r\n",
		(unsigned)out_width, (unsigned)out_height,
		(unsigned)out_size, (unsigned)pp_size, (unsigned)seg_rows);

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, JPEG_176_144_SIZE);
	if (stream_buf == NULL) {
		LOGE("alloc stream_buf failed\r\n");
		return;
	}
	os_memcpy(stream_buf, (const void *)JPEGData_176_144, JPEG_176_144_SIZE);
	cache_data_flush_range(stream_buf, JPEG_176_144_SIZE);

	/* Full-frame output buffer. */
	out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, out_size);
	if (out_buf == NULL) {
		LOGE("alloc out_buf failed\r\n");
		goto cleanup;
	}
	os_memset(out_buf, 0, out_size);

	/* Pingpong buffer (HW output ring-buffer): layout is [Y for seg_num][C for seg_num]. */
	pp_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, pp_size);
	if (pp_buf == NULL) {
		LOGE("alloc pp_buf failed\r\n");
		goto cleanup;
	}
	os_memset(pp_buf, 0, pp_size);

	s_flexa_copy_ctx.frame_y = out_buf;
	s_flexa_copy_ctx.frame_c = out_buf + out_width * out_height;
	s_flexa_copy_ctx.frame_width = out_width;
	s_flexa_copy_ctx.frame_height = out_height;
	s_flexa_copy_ctx.pp_y = pp_buf;
	s_flexa_copy_ctx.pp_c = pp_buf + out_width * seg_rows;
	s_flexa_copy_ctx.pp_seg_num = seg_num;
	s_flexa_copy_ctx.pp_seg_rows = seg_rows;

	bk_jpeg_decode_config_t cfg = {0};
	cfg.decode_mode = BK_JPEG_DECODE_FLEXA_MODE_FLEXA;
	cfg.frame_done_cb = vcdec_jpeg_frame_done_cb;
	cfg.flexa_done_cb = vcdec_jpeg_flexa_done_cb;
	cfg.args = &s_flexa_copy_ctx;
	cfg.timeout_ms = 1000;
	cfg.segment_height = (uint16_t)seg_ht_mb;
	cfg.segment_number = (uint8_t)seg_num;
	cfg.width = (uint16_t)out_width;
	cfg.height = (uint16_t)out_height;
	ret = bk_jpeg_decode_new(&s_flexa_copy_ctx.dec, &cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_jpeg_decode_new failed %d\r\n", (int)ret);
		goto cleanup;
	}
	ret = bk_jpeg_decode_init(s_flexa_copy_ctx.dec);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_jpeg_decode_init failed %d\r\n", (int)ret);
		goto cleanup;
	}

	ret = bk_jpeg_decode_open(s_flexa_copy_ctx.dec);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_jpeg_decode_open failed %d\r\n", (int)ret);
		goto cleanup;
	}

	os_memset(s_jpeg_flexa_bond_stub, 0, sizeof(s_jpeg_flexa_bond_stub));
	ret = bk_jpeg_decode_ioctl(s_flexa_copy_ctx.dec, BK_JPEG_DECODE_IOCTL_REGISTER_BOND,
				   s_jpeg_flexa_bond_stub);
	if (ret != AVDK_ERR_OK) {
		LOGE("REGISTER_BOND failed %d\r\n", (int)ret);
		goto cleanup;
	}

	for (i = 0; i < VCDEC_JPEG_DECODE_CNT; i++) {
		LOGI("flexa decode %u/%u\r\n", (unsigned)(i + 1), (unsigned)VCDEC_JPEG_DECODE_CNT);
		/* Clear frame and pingpong buffers for visibility. */
		os_memset(out_buf, 0, out_size);
		os_memset(pp_buf, 0, pp_size);

		bk_jpeg_decode_input_t in = {0};
		in.stream = stream_buf;
		in.stream_len = JPEG_176_144_SIZE;
		in.out_buffer = pp_buf;
		in.out_buffer_size = pp_size;
		ret = bk_jpeg_decode_frame(s_flexa_copy_ctx.dec, &in);

		if (ret == AVDK_ERR_OK) {
			static int flag = 0;
			if (flag == 0) {
				flag = 1;
				LOGI("dump out_buf\r\n");
				// extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
				// stack_mem_dump((uint32_t)out_buf, (uint32_t)(out_buf+out_size));
			}
		}
		if (ret != AVDK_ERR_OK) {
			LOGE("bk_jpeg_decode_frame failed %d\r\n", (int)ret);
		}
	}

	LOGI("flexa decode done\r\n");
	(void)bk_jpeg_decode_ioctl(s_flexa_copy_ctx.dec, BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND,
				   s_jpeg_flexa_bond_stub);
	(void)bk_jpeg_decode_close(s_flexa_copy_ctx.dec);
	(void)bk_jpeg_decode_deinit(s_flexa_copy_ctx.dec);
	(void)bk_jpeg_decode_delete(s_flexa_copy_ctx.dec);
	s_flexa_copy_ctx.dec = NULL;

cleanup:
	if (s_flexa_copy_ctx.dec != NULL) {
		(void)bk_jpeg_decode_ioctl(s_flexa_copy_ctx.dec, BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND,
					   s_jpeg_flexa_bond_stub);
		(void)bk_jpeg_decode_close(s_flexa_copy_ctx.dec);
		(void)bk_jpeg_decode_deinit(s_flexa_copy_ctx.dec);
		(void)bk_jpeg_decode_delete(s_flexa_copy_ctx.dec);
	}
	if (out_buf != NULL)
		bk_frame_buffer_free(out_buf);
	if (pp_buf != NULL)
		bk_frame_buffer_free(pp_buf);
	if (stream_buf != NULL)
		bk_frame_buffer_free(stream_buf);

	os_memset(&s_flexa_copy_ctx, 0, sizeof(s_flexa_copy_ctx));

	LOGI("exit vcdec_jpeg_flexa_test\r\n");
}
