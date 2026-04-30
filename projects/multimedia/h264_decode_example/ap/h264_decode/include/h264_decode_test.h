#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

typedef enum {
	H264_DECODE_TEST_STREAM_1280X720 = 0,
	H264_DECODE_TEST_STREAM_256X128 = 1,
} h264_decode_test_stream_t;

/**
 * @brief H.264 decode test CLI entry.
 *
 * CLI format:
 *   - h264_decode h264d
 *   - h264_decode h264d_flexa
 *   - h264_decode vcdec_h264d [1280x720|256x128]
 *   - h264_decode vcdec_h264d_flexa [1280x720|256x128]
 *
 * Notes:
 * - `h264d` and `h264d_flexa` call the legacy `bk_h264d` demo entry directly.
 * - `vcdec_h264d` and `vcdec_h264d_flexa` exercise the `bk_decoder/h264d`
 *   controller wrapper and validate `bk_h264_decode_get_info()` output.
 */
void cli_h264_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void vcdec_h264_test(h264_decode_test_stream_t stream);
void vcdec_h264_flexa_test(h264_decode_test_stream_t stream);
void vcdec_h264_run_boot_demo(void);

#ifdef __cplusplus
}
#endif

