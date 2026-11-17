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
 *   - jpeg_decode vcdec_jpegd_frame_scale  - frame mode PP scale
 *   - jpeg_decode vcdec_jpegd_frame_crop   - frame mode PP crop
 *   - jpeg_decode vcdec_jpegd_flexa_crop   - FLEXA PP crop
 *   - jpeg_decode vcdec_jpegd_frame_scale_crop  - frame mode PP crop + scale
 *   - jpeg_decode jpegd_uvc_gpu_display [crop] - UVC MJPEG -> PP crop -> GPU rotate -> LCD
 *   - jpeg_decode jpegd_uvc_gpu_display close - stop the UVC display pipeline
 */
void cli_jpeg_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

void jpeg_decode_uvc_gpu_display_test(void);
void jpeg_decode_uvc_gpu_display_stop(void);
void jpeg_decode_uvc_gpu_display_set_param(uint8_t crop);

#ifdef CONFIG_BK_DECODER
void vcdec_jpeg_frame_test(void);
void vcdec_jpeg_flexa_test(void);
void vcdec_jpeg_frame_rgb_test(void);
void vcdec_jpeg_frame_scale_test(void);
void vcdec_jpeg_frame_crop_test(void);
#if 0 /* Temporarily disabled: Flexa decode scaling is offline. */
void vcdec_jpeg_flexa_scale_test(void);
#endif
void vcdec_jpeg_flexa_crop_test(void);
void vcdec_jpeg_frame_scale_crop_test(void);
#if 0 /* Temporarily disabled: Flexa decode scaling is offline. */
void vcdec_jpeg_flexa_scale_crop_test(void);
#endif
void vcdec_jpeg_run_boot_demo(void);
#endif

#ifdef __cplusplus
}
#endif
