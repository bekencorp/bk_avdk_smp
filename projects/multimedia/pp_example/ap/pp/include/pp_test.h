#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#define PP_EXAMPLE_SRC_WIDTH              1280U
#define PP_EXAMPLE_SRC_HEIGHT             720U

/**
 * PP module test CLI entry.
 *
 * Source: embedded 1280x720 H.264 (1I30P) decoded to NV12, then fed to PP.
 *
 * CLI format:
 *   - pp nv12_rgb565        - same resolution NV12 -> RGB565
 *   - pp nv12_rgb888        - same resolution NV12 -> RGB888
 *   - pp nv12_scale_down    - NV12 1280x720 -> 640x360
 *   - pp nv12_scale_up      - NV12 1280x720 -> 1920x1080
 *   - pp nv12_rgb565_down   - NV12 -> RGB565 640x360
 *   - pp nv12_rgb888_down   - NV12 -> RGB888 640x360
 *   - pp nv12_rgb565_up     - NV12 -> RGB565 1920x1080
 *   - pp nv12_rgb888_up     - NV12 -> RGB888 1920x1080
 *   - pp help | -h          - show usage
 */
void cli_pp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#ifdef CONFIG_BK_DECODER
void pp_nv12_rgb565_test(void);
void pp_nv12_rgb888_test(void);
void pp_nv12_scale_down_test(void);
void pp_nv12_scale_up_test(void);
void pp_nv12_rgb565_down_test(void);
void pp_nv12_rgb888_down_test(void);
void pp_nv12_rgb565_up_test(void);
void pp_nv12_rgb888_up_test(void);
void pp_run_boot_demo(void);
#endif

#ifdef __cplusplus
}
#endif
