#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include "draw_osd_test.h"
#include "osd_uvc.h"
#include "osd_mipi.h"
#include "cli.h"

#define TAG "draw_osd"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* IT result log: strings must match .it.csv exactly ([RESULT][PASS] <name> success).
 * Shared by boot auto-start (ap_main.c) and CLI subcommands on success/failure. */
void draw_osd_log_result(const char *name, bool pass, const char *stage)
{
    if (name == NULL) {
        return;
    }
    if (pass) {
        LOGI("[RESULT][PASS] %s success\r\n", name);
    } else {
        LOGE("[RESULT][FAIL] %s failed at %s\r\n", name, (stage != NULL) ? stage : "unknown");
    }
}

/*
 * Pipeline-only OSD example commands (MIPI / UVC real UI only). Single command family:
 *
 *   ap_cmd osd <mipi|uvc> <frame|flexa>   — show OSD on that path (auto-starts pipeline on first use)
 *       mipi / uvc — select path; each owns a separate bk_draw_osd instance
 *       frame      — one SRC_OVER at frame end (OSD_BLEND_AT_FRAME_END)
 *       flexa      — SRC_OVER per flexa block (OSD_BLEND_PER_FLEXA); cost spread across frame
 *   ap_cmd osd <mipi|uvc> update <name> <content>
 *                                         — runtime dynamic list edit (requires prior show):
 *                                            update wifi_group wifi_rssi_2 / update text1 12:53
 *   ap_cmd osd <mipi|uvc> remove <name>   — remove element, e.g. remove wifi_group / remove text1
 *   ap_cmd osd <mipi|uvc> clear           — remove OSD overlay only; keep live video
 *   ap_cmd osd <mipi|uvc> close           — remove OSD and close pipeline (free GPU/memory before switching camera)
 *
 * Content from assets blend_info[] (see blend_dsc.c); positioned by xpos/ypos.
 * Component auto-clusters array into slots (multi-blit); internal, no CLI control needed.
 * Blend timing (frame/flexa) set by osd_mipi_show/osd_uvc_show via bk_gpu_ioctl(BK_GPU_IOCTL_SET_OSD_BY_FLEXA) on bound GPU.
 *
 * Note: mipi and uvc cannot run together (shared GPU/display memory); run `osd <current> close` before switching.
 * mipi / uvc subcommands log [RESULT][PASS|FAIL] (case osd_<pipe>_<suffix>, matches .it.csv).
 */

/* Build IT case name osd_<pipe>_<suffix> into buf. Returns buf or NULL (action/arg not IT-scored).
 *   update text1        -> osd_<pipe>_update_text
 *   update wifi_group   -> osd_<pipe>_update_wifi
 *   remove wifi_group   -> osd_<pipe>_remove_wifi
 *   clear/close/frame/flexa -> osd_<pipe>_clear/close/frame/flexa */
static const char *draw_osd_it_case(char *buf, uint32_t buflen, const char *pipe,
                                    const char *action, const char *name)
{
    const char *suffix = NULL;

    if (os_strcmp(action, "update") == 0) {
        if (name != NULL && os_strcmp(name, "text1") == 0) {
            suffix = "update_text";
        } else if (name != NULL && os_strcmp(name, "wifi_group") == 0) {
            suffix = "update_wifi";
        }
    } else if (os_strcmp(action, "remove") == 0) {
        if (name != NULL && os_strcmp(name, "wifi_group") == 0) {
            suffix = "remove_wifi";
        }
    } else if (os_strcmp(action, "clear") == 0 || os_strcmp(action, "close") == 0 ||
               os_strcmp(action, "frame") == 0 || os_strcmp(action, "flexa") == 0) {
        suffix = action;
    }

    if (suffix == NULL) {
        return NULL;
    }
    os_snprintf(buf, buflen, "osd_%s_%s", pipe, suffix);
    return buf;
}

void cli_draw_osd_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;
    const char *result_case = NULL;
    const char *stage = "exec";

    if (argc < 3) {
        LOGI("usage: osd <mipi|uvc> <frame|flexa|update|remove|clear|close>\n");
        goto exit;
    }

    bool is_mipi = (os_strcmp(argv[1], "mipi") == 0);
    bool is_uvc  = (os_strcmp(argv[1], "uvc") == 0);
    if (!is_mipi && !is_uvc) {
        LOGE("pipe must be mipi|uvc (got '%s')\n", argv[1]);
        goto exit;
    }

    /* Compute IT case name (symmetric for mipi/uvc); log [RESULT] on success/failure at exit */
    char case_buf[40];
    result_case = draw_osd_it_case(case_buf, sizeof(case_buf), argv[1], argv[2],
                                   (argc > 3) ? argv[3] : NULL);

    if (os_strcmp(argv[2], "update") == 0) {
        const char *name    = (argc > 3) ? argv[3] : NULL;
        const char *content = (argc > 4) ? argv[4] : NULL;
        ret = is_mipi ? osd_mipi_update(name, content) : osd_uvc_update(name, content);
        goto exit;
    }

    if (os_strcmp(argv[2], "remove") == 0) {
        const char *name = (argc > 3) ? argv[3] : NULL;
        ret = is_mipi ? osd_mipi_remove(name) : osd_uvc_remove(name);
        goto exit;
    }

    /* clear: remove OSD overlay only; keep video */
    if (os_strcmp(argv[2], "clear") == 0) {
        ret = is_mipi ? osd_mipi_clear() : osd_uvc_clear();
        goto exit;
    }

    /* close: remove OSD and close pipeline (free GPU/memory before switching camera) */
    if (os_strcmp(argv[2], "close") == 0) {
        ret = is_mipi ? osd_mipi_close() : osd_uvc_close();
        goto exit;
    }

    /* frame / flexa: select blend timing (osd_*_show sets via ioctl on bound GPU) */
    osd_blend_mode_t mode;
    if (os_strcmp(argv[2], "frame") == 0) {
        mode = OSD_BLEND_AT_FRAME_END;
    } else if (os_strcmp(argv[2], "flexa") == 0) {
        mode = OSD_BLEND_PER_FLEXA;
    } else {
        LOGE("action must be frame|flexa|update|remove|clear|close (got '%s')\n", argv[2]);
        result_case = NULL;
        goto exit;
    }

    LOGI("osd %s: blend=%s\n", argv[1], (mode == OSD_BLEND_PER_FLEXA) ? "flexa(per-block)" : "frame(frame-end)");
    ret = is_mipi ? osd_mipi_show(mode) : osd_uvc_show(mode);

exit:
    if (result_case != NULL) {
        draw_osd_log_result(result_case, (ret == AVDK_ERR_OK), stage);
    }
    msg = (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR;
    LOGI("%s ---complete (ret=%d)\n", __func__, ret);
    if (pcWriteBuffer != NULL && msg != NULL) {
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    }
}

#define CMDS_COUNT  (sizeof(s_draw_osd_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_draw_osd_test_commands[] =
{
    {"osd", "osd <mipi|uvc> <frame|flexa|update|remove|clear|close>", cli_draw_osd_test_cmd},
};

int cli_draw_osd_test_init(void)
{
    return cli_register_commands(s_draw_osd_test_commands, CMDS_COUNT);
}
