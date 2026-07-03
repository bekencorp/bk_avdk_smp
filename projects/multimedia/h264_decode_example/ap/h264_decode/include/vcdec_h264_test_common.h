#pragma once

#include <stdint.h>
#include "os/os.h"
#include "os/mem.h"
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_decode/bk_h264_decode_ctlr.h>
#include "h264_decode_test.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VCDEC_H264_TEST_ROUNDS       1U
#define VCDEC_H264_TEST_DUMP_ENABLE  0

/* Static description of one embedded test stream. */
typedef struct {
	h264_decode_test_stream_t id;
	const char *name;
	const uint8_t *stream;
	const uint32_t *bytes;
	uint32_t width;
	uint32_t height;
} vcdec_h264_test_stream_cfg_t;

/* Per-run state shared between the decode loop and the controller callbacks. */
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

/* Look up the static config for an embedded test stream, or NULL if unknown. */
const vcdec_h264_test_stream_cfg_t *vcdec_h264_get_stream_cfg(h264_decode_test_stream_t stream);

/* Human-readable name of a decoded frame type. */
const char *vcdec_h264_frame_type_name(bk_h264_decode_frame_type_t type);

/* NV12 frame buffer byte size for the given geometry. */
uint32_t vcdec_h264_frame_size(uint32_t width, uint32_t height);

/* Close + deinit + delete a controller handle and clear the pointer. */
void vcdec_h264_destroy_decoder(bk_h264_decode_ctlr_handle_t *dec);

/* Optional NV12 dump helper (compiled out unless VCDEC_H264_TEST_DUMP_ENABLE). */
void vcdec_h264_dump_nv12(const char *tag, uint8_t *buf, uint32_t size);

/* Emit the standard [RESULT][PASS]/[FAIL] line used by the IT harness. */
void vcdec_h264_log_result(const char *case_name, uint8_t pass,
			   const char *stage, avdk_err_t ret,
			   uint32_t done_aus, uint32_t total_rounds);

/* Validate that bk_h264_decode_get_info() echoes the expected geometry/input. */
bk_err_t vcdec_h264_check_info(const vcdec_h264_test_stream_cfg_t *stream_cfg,
			       const bk_h264_decode_info_t *info,
			       const uint8_t *au_ptr, uint32_t au_size);

/* Shared frame-done callback: bumps ctx->frame_done_count and records status. */
void vcdec_h264_frame_done_cb(int status, void *args);

#ifdef __cplusplus
}
#endif
