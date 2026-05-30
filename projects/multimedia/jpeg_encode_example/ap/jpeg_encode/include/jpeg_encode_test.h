#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

void cli_jpeg_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int cli_jpeg_encode_init(void);

/** VCENC JPEG frame-mode test using bk_jpeg_encode_* (256x128 NV12). Logs [RESULT][PASS]/[FAIL]. */
int bk_jpeg_encode_frame_test(void);

/** VCENC JPEG software-Flexa test using 256x128 NV12 input. Logs [RESULT][PASS]/[FAIL]. */
int bk_jpeg_encode_sw_flexa_test(void);

/** Optional boot demo: delayed run of frame and software-Flexa JPEG tests. */
void vcenc_jpeg_run_boot_demo(void);

#ifdef __cplusplus
}
#endif
