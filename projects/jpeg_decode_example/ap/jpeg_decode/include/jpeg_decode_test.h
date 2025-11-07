#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

typedef enum {
    JPEG_DECODE_MODE_HARDWARE = 0,
    JPEG_DECODE_MODE_SOFTWARE,
    JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1,
    JPEG_DECODE_MODE_SOFTWARE_DTCM_CP2,
    JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1_CP2,
} jpeg_decode_test_type_t;

void cli_jpeg_decode_error_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_jpeg_decode_regular_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_jpeg_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

bk_err_t jpeg_decode_in_complete(frame_buffer_t *in_frame);
frame_buffer_t *jpeg_decode_out_malloc(uint32_t size);
bk_err_t jpeg_decode_out_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame);

bk_err_t create_and_open_decoder(void **jpeg_decode_handle, void *jpeg_decode_config, jpeg_decode_test_type_t jpeg_decode_test_type);
bk_err_t close_and_delete_decoder(void **jpeg_decode_handle, jpeg_decode_test_type_t jpeg_decode_test_type);
bk_err_t perform_jpeg_decode_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name, jpeg_decode_test_type_t jpeg_decode_test_type);
bk_err_t perform_jpeg_decode_async_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name, jpeg_decode_test_type_t jpeg_decode_test_type);
bk_err_t perform_jpeg_decode_async_burst_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, 
                                           const char *test_name, uint32_t burst_count);
bk_err_t perform_jpeg_decode_sw_async_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name, jpeg_decode_test_type_t jpeg_decode_test_type);
bk_err_t perform_jpeg_decode_sw_async_burst_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, 
                                           const char *test_name, jpeg_decode_test_type_t jpeg_decode_test_type, uint32_t burst_count);

bk_err_t perform_jpeg_decode_hw_opt_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name);
bk_err_t perform_jpeg_decode_hw_opt_async_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name);
bk_err_t perform_jpeg_decode_hw_opt_async_burst_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name, uint32_t burst_count);
bk_err_t create_and_open_hw_opt_decoder(void **jpeg_decode_handle, void *jpeg_decode_config);
bk_err_t close_and_delete_hw_opt_decoder(void **jpeg_decode_handle);

#ifdef __cplusplus
}
#endif