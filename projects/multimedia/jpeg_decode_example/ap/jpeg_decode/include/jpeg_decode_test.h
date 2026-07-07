#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/**
 * @brief JPEG decode test CLI entry.
 *
 * CLI format:
 *   - jpeg_decode vcdec_jpegd              - frame mode NV12 decode
 *   - jpeg_decode vcdec_jpegd_flexa        - FLEXA NV12 decode
 *   - jpeg_decode vcdec_jpegd_frame_rgb    - frame mode PP RGB565 + RGB888
 */
void cli_jpeg_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef CONFIG_BK_DECODER
void vcdec_jpeg_frame_test(void);
void vcdec_jpeg_flexa_test(void);
void vcdec_jpeg_frame_rgb_test(void);
void vcdec_jpeg_run_boot_demo(void);
#endif

#ifdef __cplusplus
}
#endif
