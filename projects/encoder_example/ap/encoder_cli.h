#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define YUV_TEST_BUF_WIDTH (640)
#define YUV_TEST_BUF_HEIGHT (480)

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

void cli_jpeg_encoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
uint8_t *get_yuv_test_buf(void);

#ifdef __cplusplus
}
#endif
