#ifndef __OSD_UVC_H__
#define __OSD_UVC_H__

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include "draw_osd_test.h"   /* osd_blend_mode_t */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Overlay OSD on UVC camera video.
 *
 * Like MIPI, the UVC pipeline GPU is already in use (NV12->ARGB8888 scale/rotate/compress);
 * a second VG-Lite context is not allowed. blend_info[] elements are composited on CPU into
 * transparent sprites (PSRAM, src_format=ABGR8888 matching BGRA backdrop), then SRC_OVER via bk_gpu_blit_set.
 *
 * Pipeline must be open before use (osd_uvc_show auto-starts it).
 */

/* Show OSD on UVC video (default blend_info[]; component auto-clusters slots).
 * mode: OSD_BLEND_AT_FRAME_END (once at frame end) / OSD_BLEND_PER_FLEXA (per block).
 * First call auto-starts UVC pipeline. */
avdk_err_t osd_uvc_show(osd_blend_mode_t mode);

/* Runtime dynamic list refresh: update <name> <content>; remove <name>. Requires show first. */
avdk_err_t osd_uvc_update(const char *name, const char *content);
avdk_err_t osd_uvc_remove(const char *name);

/* Clear UVC OSD overlay (keep UVC video) */
avdk_err_t osd_uvc_clear(void);

/* Close UVC path: remove OSD, then close UVC pipeline (free GPU/memory for MIPI switch) */
avdk_err_t osd_uvc_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __OSD_UVC_H__ */
