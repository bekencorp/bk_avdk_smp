#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

// H264 encode test type
typedef enum {
    H264_ENCODE_MODE_NORMAL = 0,
} h264_encode_test_type_t;

// CLI command functions
void cli_h264_encode_error_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_h264_encode_regular_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_h264_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_h264e_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

// Buffer callback functions
void *h264_encode_buffer_request_cb(uint32_t size);
uint32_t h264_encode_buffer_complete_cb(void *buffer, uint32_t result);

// Helper functions
bk_err_t create_and_open_encoder(void **h264_encode_handle, void *h264_encode_config);
bk_err_t close_and_delete_encoder(void **h264_encode_handle);
bk_err_t perform_h264_encode_test(void *h264_encode_handle, const char *test_name);
bk_err_t perform_h264_encode_async_test(void *h264_encode_handle, const char *test_name);

#ifdef __cplusplus
}
#endif

