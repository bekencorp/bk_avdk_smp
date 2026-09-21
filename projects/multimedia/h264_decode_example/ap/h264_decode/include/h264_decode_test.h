#pragma once

#include <os/os.h>
#include <components/bk_decode/bk_h264_decode_ctlr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/*
 * Whether the decoder DPB gets a PSRAM write-through window.
 *
 * POOL is a narrow window sized to the DPB slots, so only the buffers the
 * decoder writes are covered and neighbouring allocations keep their normal
 * read-modify-write behaviour. OFF leaves the DPB cached, for measuring what
 * cover is actually worth.
 */
typedef enum {
	H264_DECODE_COVER_OFF = 0,
	H264_DECODE_COVER_POOL,
} h264_decode_cover_mode_t;

/* FLEXA PP ring is allocated from HSRAM (SMEM3/4). */
typedef enum {
	VCDEC_H264_RING_HEAP_HSRAM = 0,
} vcdec_h264_ring_heap_t;

typedef enum {
	/* Baseline 1 IDR + 30 P stream, no B-frames. */
	H264_DECODE_TEST_STREAM_1280X720_1I30P = 0,
	/* Main profile stream with B-frames (IBBP). */
	H264_DECODE_TEST_STREAM_1280X720_IBBP  = 1,
	/* 1080p streams: require CONFIG_H264_DECODE_ENABLE_1080P at build time. */
	H264_DECODE_TEST_STREAM_1920X1080      = 2,
	H264_DECODE_TEST_STREAM_1920X1080_GOP30 = 3,
} h264_decode_test_stream_t;

/**
 * @brief H.264 decode test CLI entry.
 *
 * CLI format:
 *   - h264_decode vcdec_h264d [1280x720_1i30p|1280x720_ibbp]
 *   - h264_decode vcdec_h264d_flexa [1280x720_1i30p|1280x720_ibbp]
 *   - h264_decode vcdec_h264d_frame_zerocopy [1280x720_1i30p|1280x720_ibbp]
 *   - h264_decode vcdec_h264d_frame_rgb             (PP RGB565/RGB888 frame output)
 *   - h264_decode vcdec_h264d_osd                   (PP OSD alpha-blend)
 *   - h264_decode vcdec_h264d_scale [stream]          (PP down-scale frame output)
 *
 * Notes:
 * - All subcommands exercise the `bk_decoder/h264d` controller abstraction
 *   (`bk_h264_decode_ctlr`) on top of the vcdec driver:
 *   `vcdec_h264d` (whole-frame, non-B), `vcdec_h264d_flexa` (segmented), and
 *   `vcdec_h264d_frame_zerocopy` (zero-copy / B-frame).
 * - Both 1280x720 streams (`1i30p` and `ibbp`) are embedded simultaneously; the
 *   stream argument selects which one to decode. `1280x720` is accepted as an
 *   alias for the IBBP (B-frame) stream.
 */
void cli_h264_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
/* Select the mode applied to subsequently initialized decoder instances. */
void h264_decode_cover_set_mode(h264_decode_cover_mode_t mode);
avdk_err_t h264_decode_cover_apply(bk_h264_decode_ctlr_handle_t decoder);
/* Applies to the next FLEXA run, since the ring is allocated per run. */
void vcdec_h264_ring_heap_set(vcdec_h264_ring_heap_t heap);
vcdec_h264_ring_heap_t vcdec_h264_ring_heap_get(void);
const char *vcdec_h264_ring_heap_name(vcdec_h264_ring_heap_t heap);
void vcdec_h264_frame_test(h264_decode_test_stream_t stream);
void vcdec_h264_flexa_test(h264_decode_test_stream_t stream);
void vcdec_h264_frame_zerocopy_test(h264_decode_test_stream_t stream);
void vcdec_h264_frame_rgb_test(void);
void vcdec_h264_osd_test(void);
void vcdec_h264_frame_scale_test(h264_decode_test_stream_t stream);
void vcdec_h264_run_boot_demo(void);

#ifdef __cplusplus
}
#endif

