#ifndef __OSD_MIPI_H__
#define __OSD_MIPI_H__

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include "draw_osd_test.h"   /* osd_blend_mode_t */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Overlay OSD on live MIPI camera video (local GC2053 CSI + ISP + GPU).
 *
 * Content comes from assets blend_info[] (see coordinate macros in blend_dsc.c / bk_font.c / bk_img.c),
 * positioned by each entry's xpos/ypos. Overlay attaches to pipeline GPU (SRC_OVER each frame)
 * over live camera, not a static background.
 */

/* Show OSD on MIPI video (default blend_info[]; component auto-clusters slots).
 * mode: OSD_BLEND_AT_FRAME_END (once at frame end) / OSD_BLEND_PER_FLEXA (per block).
 * First call auto-starts MIPI camera pipeline. */
avdk_err_t osd_mipi_show(osd_blend_mode_t mode);

/* Runtime dynamic list refresh: update <name> <content>; remove <name>. Requires show first. */
avdk_err_t osd_mipi_update(const char *name, const char *content);
avdk_err_t osd_mipi_remove(const char *name);

/* Clear MIPI OSD overlay (keep live video) */
avdk_err_t osd_mipi_clear(void);

/* Close MIPI path: remove OSD, then close camera pipeline (free GPU/memory for UVC switch) */
avdk_err_t osd_mipi_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __OSD_MIPI_H__ */
