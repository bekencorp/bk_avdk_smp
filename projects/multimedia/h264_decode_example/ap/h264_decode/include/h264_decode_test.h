#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/**
 * @brief H.264 decode test CLI entry.
 *
 * CLI format:
 *   - h264_decode h264d
 *   - h264_decode jpegd
 *
 * Notes:
 * - This test relies on the platform H.264 decoder component (`bk_h264d`).
 * - The default test calls `h264_decoder_test()` implemented in
 *   `ap/components/bk_vpu/bk_h264d/bk_test_h264d.c`, which uses an internal
 *   H.264 demo stream.
 * - JPEG decode test calls `jpeg_decoder_test()` implemented in the same file.
 */
void cli_h264_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef __cplusplus
}
#endif

