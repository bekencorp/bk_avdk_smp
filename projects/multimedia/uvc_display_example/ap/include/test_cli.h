#ifndef __UVC_CLI_H__
#define __UVC_CLI_H__

#include <avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#include <components/bk_gpu_ctlr.h>

void cli_uvc_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_display_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_decode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_pipeline_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

void *display_test_get_dpu_handle(void);
bk_gpu_ctlr_handle_t display_test_get_gpu_handle(void);
avdk_err_t display_test_open_with_gpu(void);
avdk_err_t display_test_open_with_gpu_flexa(uint16_t width, uint16_t height,
                                            uint8_t *src_buffer, uint8_t flexa_buff_cnt);
avdk_err_t display_test_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __UVC_CLI_H__ */
