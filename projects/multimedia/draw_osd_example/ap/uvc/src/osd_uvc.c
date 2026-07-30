/*
 * UVC path OSD overlay (thin layer).
 *
 * Owns a separate bk_draw_osd instance bound to the UVC display pipeline GPU
 * (src_format=ABGR8888, same as MIPI, compensates BGRA byte order of NV12->ARGB compressed backdrop).
 * Fully independent from osd_mipi.c with no shared static state, so MIPI + UVC dual-camera
 * OSD can run concurrently without interference.
 *
 * Content comes from assets blend_info[], positioned by each entry's xpos/ypos.
 */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/avdk_pixel_types.h>

#include "components/bk_draw_osd.h"
#include "components/bk_gpu.h"            /* bk_gpu_ioctl + BK_GPU_IOCTL_SET_OSD_BY_FLEXA */
#include "blend.h"
#include "uvc_pipeline.h"                /* uvc_pipeline_open/close/is_open */
#include "display.h"                     /* display_get_gpu_handle */
#include "osd_uvc.h"

#define TAG "osd_uvc"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* After GPU rotate90, display buffer is portrait 1080(w) x 1920(h) */
#define OSD_UVC_PANEL_W   1080
#define OSD_UVC_PANEL_H   1920

/* Module-local OSD instance (bound to UVC display GPU) */
static bk_draw_osd_ctlr_handle_t s_uvc_osd = NULL;

static avdk_err_t ensure_pipeline(void)
{
    if (uvc_pipeline_is_open()) {
        return AVDK_ERR_OK;
    }
    LOGI("pipeline not open, starting UVC 1920x1080@30 on port 1...\n");
    avdk_err_t ret = uvc_pipeline_open(1, 1920, 1080, 30);
    if (ret != AVDK_ERR_OK) {
        LOGE("pipeline_open failed %d (camera connected on port 1?)\n", ret);
    }
    return ret;
}

/* (Re)create UVC OSD instance from current pipeline GPU handle.
 * Destroy any existing instance first (clears old blits) to avoid stale GPU handle after pipeline reopen. */
static bk_draw_osd_ctlr_handle_t uvc_osd_create(void)
{
    if (s_uvc_osd) {
        bk_draw_osd_delete(s_uvc_osd);
        s_uvc_osd = NULL;
    }
    bk_gpu_ctlr_handle_t gpu = display_get_gpu_handle();
    if (gpu == NULL) {
        LOGE("uvc gpu handle NULL (pipeline not open?)\n");
        return NULL;
    }
    osd_ctlr_config_t cfg = {0};
    cfg.gpu          = gpu;
    cfg.panel_w      = OSD_UVC_PANEL_W;
    cfg.panel_h      = OSD_UVC_PANEL_H;
    cfg.src_format   = BK_PIXEL_FORMAT_ABGR8888;   /* same as MIPI; otherwise icons/text R/B swap */
    cfg.blend_assets = blend_assets;               /* asset table: add_or_update lookup by name */
    cfg.blend_info   = blend_info;                 /* default dynamic list: array(NULL) renders it */
    avdk_err_t ret = bk_draw_osd_new(&s_uvc_osd, &cfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_draw_osd_new failed %d\n", ret);
        s_uvc_osd = NULL;
    }
    return s_uvc_osd;
}

/* Show OSD on UVC video: render default blend_info[] (component auto-clusters slots; upper layer skips slot/begin/commit).
 * mode selects blend timing (once at frame end vs per flexa block). */
avdk_err_t osd_uvc_show(osd_blend_mode_t mode)
{
    avdk_err_t ret = ensure_pipeline();
    if (ret != AVDK_ERR_OK) return ret;

    /* Blend timing is per bound GPU (per-instance), not a global switch */
    bk_gpu_ctlr_handle_t gpu = display_get_gpu_handle();
    if (gpu == NULL) {
        LOGE("uvc gpu handle NULL (pipeline not open?)\n");
        return AVDK_ERR_GENERIC;
    }
    bool per_flexa = (mode == OSD_BLEND_PER_FLEXA);
    bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &per_flexa);

    bk_draw_osd_ctlr_handle_t osd = uvc_osd_create();
    if (osd == NULL) return AVDK_ERR_GENERIC;

    LOGI("UVC OSD: blend=%s, list=blend_info\n", per_flexa ? "per-flexa" : "frame-end");
    return bk_draw_osd_array(osd, NULL);   /* render default dynamic list, auto-cluster slots */
}

/* Runtime dynamic list refresh (no instance rebuild; re-render with array(NULL) after edits):
 *   ap_cmd osd uvc update <name> <content>
 *   ap_cmd osd uvc remove <name>  — e.g. remove wifi_group / remove text1
 * Requires osd_uvc_show first. */
avdk_err_t osd_uvc_update(const char *name, const char *content)
{
    if (s_uvc_osd == NULL) {
        LOGE("no osd instance, run 'osd uvc frame|flexa' first\n");
        return AVDK_ERR_GENERIC;
    }
    if (name == NULL || name[0] == '\0' || content == NULL || content[0] == '\0') {
        LOGE("update needs <name> <content>, e.g. 'update wifi_group wifi_rssi_2' or 'update text1 12:53'\n");
        return AVDK_ERR_INVAL;
    }

    bk_draw_osd_add_or_update(s_uvc_osd, name, content);
    LOGI("UVC OSD update: '%s' -> '%s'\n", name, content);
    return bk_draw_osd_array(s_uvc_osd, NULL);
}

avdk_err_t osd_uvc_remove(const char *name)
{
    if (s_uvc_osd == NULL) {
        LOGE("no osd instance, run 'osd uvc frame|flexa' first\n");
        return AVDK_ERR_GENERIC;
    }
    if (name == NULL || name[0] == '\0') {
        LOGE("remove needs <name>, e.g. 'remove wifi_group' or 'remove text1'\n");
        return AVDK_ERR_INVAL;
    }

    bk_draw_osd_remove(s_uvc_osd, name);
    LOGI("UVC OSD remove: '%s'\n", name);
    return bk_draw_osd_array(s_uvc_osd, NULL);
}

avdk_err_t osd_uvc_clear(void)
{
    if (s_uvc_osd) {
        bk_draw_osd_delete(s_uvc_osd);
        s_uvc_osd = NULL;
        LOGI("UVC OSD cleared\n");
    }
    return AVDK_ERR_OK;
}

avdk_err_t osd_uvc_close(void)
{
    osd_uvc_clear();                      /* remove OSD overlay first (delete instance, clear blits) */
    LOGI("UVC pipeline closing...\n");
    return uvc_pipeline_close();          /* then close UVC pipeline, free GPU/memory for MIPI switch */
}
