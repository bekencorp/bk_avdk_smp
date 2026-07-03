#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

typedef enum {
	/* Baseline 1 IDR + 30 P stream, no B-frames. */
	H264_DECODE_TEST_STREAM_1280X720_1I30P = 0,
	/* Main profile stream with B-frames (IBBP). */
	H264_DECODE_TEST_STREAM_1280X720_IBBP  = 1,
} h264_decode_test_stream_t;

/**
 * @brief H.264 decode test CLI entry.
 *
 * CLI format:
 *   - h264_decode vcdec_h264d [1280x720_1i30p|1280x720_ibbp]
 *   - h264_decode vcdec_h264d_flexa [1280x720_1i30p|1280x720_ibbp]
 *   - h264_decode vcdec_h264d_frame_zerocopy [1280x720_1i30p|1280x720_ibbp]
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
void vcdec_h264_frame_test(h264_decode_test_stream_t stream);
void vcdec_h264_flexa_test(h264_decode_test_stream_t stream);
void vcdec_h264_frame_zerocopy_test(h264_decode_test_stream_t stream);
void vcdec_h264_run_boot_demo(void);

#ifdef __cplusplus
}
#endif

