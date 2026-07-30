/*
 * MIPI path OSD overlay (thin layer).
 *
 * Owns a separate bk_draw_osd instance bound to the MIPI pipeline GPU
 * (src_format=ABGR8888 compensates NV12->ARGB channel order). Sprite
 * composition, font rasterization, and GPU submit are handled by osd_engine
 * in the component; this file only feeds the controller blend_info[] from
 * assets using each entry's xpos/ypos.
 *
 * Independent from osd_uvc.c; no shared static state; can run concurrently.
 */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/avdk_pixel_types.h>

#include "components/bk_draw_osd.h"
#include "components/bk_gpu.h"            /* bk_gpu_ioctl + BK_GPU_IOCTL_SET_OSD_BY_FLEXA */
#include "blend.h"
#include "mipi_pipeline.h"
#include "osd_mipi.h"

#define TAG "osd_mipi"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* After GPU rotate90, display buffer is portrait 1080(w) x 1920(h) */
#define OSD_MIPI_PANEL_W  1080
#define OSD_MIPI_PANEL_H  1920

/* Module-local OSD instance (bound to MIPI pipeline GPU) */
static bk_draw_osd_ctlr_handle_t s_mipi_osd = NULL;

/* Start MIPI video pipeline (GC2053 1920x1080@30) */
static avdk_err_t ensure_pipeline(void)
{
    if (mipi_pipeline_is_open()) {
        return AVDK_ERR_OK;
    }
    LOGI("mipi pipeline not open, starting GC2053 1920x1080@30...\n");
    avdk_err_t ret = mipi_pipeline_open(1920, 1080, 25);
    if (ret != AVDK_ERR_OK) {
        LOGE("mipi_pipeline_open failed %d (camera connected?)\n", ret);
    }
    return ret;
}

/* (Re)create MIPI OSD instance from current pipeline GPU handle.
 * Destroy any existing instance first (clears old blits) to avoid stale GPU handle after pipeline reopen. */
static bk_draw_osd_ctlr_handle_t mipi_osd_create(void)
{
    if (s_mipi_osd) {
        bk_draw_osd_delete(s_mipi_osd);
        s_mipi_osd = NULL;
    }
    bk_gpu_ctlr_handle_t gpu = mipi_pipeline_get_gpu_handle();
    if (gpu == NULL) {
        LOGE("mipi gpu handle NULL (pipeline not open?)\n");
        return NULL;
    }
    osd_ctlr_config_t cfg = {0};
    cfg.gpu          = gpu;
    cfg.panel_w      = OSD_MIPI_PANEL_W;
    cfg.panel_h      = OSD_MIPI_PANEL_H;
    cfg.src_format   = BK_PIXEL_FORMAT_ABGR8888;   /* NV12->ARGB channel order compensation */
    cfg.blend_assets = blend_assets;               /* asset table: add_or_update lookup by name */
    cfg.blend_info   = blend_info;                 /* default dynamic list: array(NULL) renders it */
    avdk_err_t ret = bk_draw_osd_new(&s_mipi_osd, &cfg);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_draw_osd_new failed %d\n", ret);
        s_mipi_osd = NULL;
    }
    return s_mipi_osd;
}

/* Show OSD on MIPI video: render default blend_info[] (component auto-clusters slots; upper layer skips slot/begin/commit).
 * mode selects blend timing (once at frame end vs per flexa block). */
avdk_err_t osd_mipi_show(osd_blend_mode_t mode)
{
    avdk_err_t ret = ensure_pipeline();
    if (ret != AVDK_ERR_OK) return ret;

    /* Blend timing is per bound GPU (per-instance), not a global switch */
    bk_gpu_ctlr_handle_t gpu = mipi_pipeline_get_gpu_handle();
    if (gpu == NULL) {
        LOGE("mipi gpu handle NULL (pipeline not open?)\n");
        return AVDK_ERR_GENERIC;
    }
    bool per_flexa = (mode == OSD_BLEND_PER_FLEXA);
    bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &per_flexa);

    bk_draw_osd_ctlr_handle_t osd = mipi_osd_create();
    if (osd == NULL) return AVDK_ERR_GENERIC;

    LOGI("MIPI OSD: blend=%s, list=blend_info\n", per_flexa ? "per-flexa" : "frame-end");
    return bk_draw_osd_array(osd, NULL);   /* render default dynamic list, auto-cluster slots */
}

/* Runtime dynamic list refresh (no instance rebuild; re-render with array(NULL) after edits):
 *   ap_cmd osd mipi update <name> <content>
 *     - image: update wifi_group wifi_rssi_2 / wifi_group wifi_rssi_full
 *     - text:  update text1 12:53
 *   ap_cmd osd mipi remove <name>  — e.g. remove wifi_group / remove text1
 * Requires osd_mipi_show first. */
avdk_err_t osd_mipi_update(const char *name, const char *content)
{
    if (s_mipi_osd == NULL) {
        LOGE("no osd instance, run 'osd mipi frame|flexa' first\n");
        return AVDK_ERR_GENERIC;
    }
    if (name == NULL || name[0] == '\0' || content == NULL || content[0] == '\0') {
        LOGE("update needs <name> <content>, e.g. 'update wifi_group wifi_rssi_2' or 'update text1 12:53'\n");
        return AVDK_ERR_INVAL;
    }

    bk_draw_osd_add_or_update(s_mipi_osd, name, content);
    LOGI("MIPI OSD update: '%s' -> '%s'\n", name, content);
    return bk_draw_osd_array(s_mipi_osd, NULL);
}

avdk_err_t osd_mipi_remove(const char *name)
{
    if (s_mipi_osd == NULL) {
        LOGE("no osd instance, run 'osd mipi frame|flexa' first\n");
        return AVDK_ERR_GENERIC;
    }
    if (name == NULL || name[0] == '\0') {
        LOGE("remove needs <name>, e.g. 'remove wifi_group' or 'remove text1'\n");
        return AVDK_ERR_INVAL;
    }

    bk_draw_osd_remove(s_mipi_osd, name);
    LOGI("MIPI OSD remove: '%s'\n", name);
    return bk_draw_osd_array(s_mipi_osd, NULL);
}

avdk_err_t osd_mipi_clear(void)
{
    if (s_mipi_osd) {
        bk_draw_osd_delete(s_mipi_osd);   /* delete clears registered blits and frees sprites */
        s_mipi_osd = NULL;
        LOGI("MIPI OSD cleared\n");
    }
    return AVDK_ERR_OK;
}

avdk_err_t osd_mipi_close(void)
{
    osd_mipi_clear();                     /* remove OSD overlay first (delete instance, clear blits) */
    LOGI("MIPI pipeline closing...\n");
    return mipi_pipeline_close();         /* then close camera pipeline, free GPU/memory for UVC switch */
}
