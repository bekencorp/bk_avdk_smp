/*
 * bk_draw_osd controller private interface.
 * Manages assets/display list (dynamic_array + mutex + add/remove);
 * pixel compositing and GPU submit are delegated to osd_engine.
 */
#ifndef __BK_DRAW_OSD_CTLR_H__
#define __BK_DRAW_OSD_CTLR_H__

#include "components/bk_draw_osd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create controller instance (called by public wrapper bk_draw_osd_new) */
avdk_err_t osd_ctlr_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DRAW_OSD_CTLR_H__ */
