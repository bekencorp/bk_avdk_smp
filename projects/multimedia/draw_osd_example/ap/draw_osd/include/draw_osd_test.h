#ifndef __DRAW_OSD_TEST_H__
#define __DRAW_OSD_TEST_H__

#include <stdbool.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/*
 * OSD blend timing (when OSD is composited onto video). Maps to CLI frame / flexa.
 * Applied via bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, ...) on bound GPU.
 * (Former osd_blend_mode.h merged here; shared by overlay and CLI.)
 */
typedef enum {
    OSD_BLEND_AT_FRAME_END = 0,  /* SRC_OVER once after full frame (default; latency at frame end) */
    OSD_BLEND_PER_FLEXA    = 1,  /* SRC_OVER per flexa block (spread cost across frame, reduce drops) */
} osd_blend_mode_t;

/* IT result log (strings must match .it.csv expectations); defined in draw_osd_cli.c */
void draw_osd_log_result(const char *name, bool pass, const char *stage);

/* CLI command handler */
void cli_draw_osd_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/* CLI init */
int cli_draw_osd_test_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRAW_OSD_TEST_H__ */
