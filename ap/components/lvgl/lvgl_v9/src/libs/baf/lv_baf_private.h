/**
 * @file lv_baf_private.h
 */

#ifndef LV_BAF_PRIVATE_H
#define LV_BAF_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../widgets/image/lv_image_private.h"
#include <bk_baf.h>
#include "lv_baf.h"

#if LV_USE_BAF

struct _lv_baf_t {
    lv_image_t img;
    bk_baf_decoder_t * decoder;
    lv_timer_t * timer;
    lv_image_dsc_t imgdsc;
    /* GPU-composed ARGB8888 frame (non-cacheable frame-buffer heap) that LVGL
     * draws via lv_image_set_src; imgdsc.data points here. */
    uint8_t * frame_buf;
    /* Set only when lv_baf_set_src() was given a filesystem path: the whole .baf
     * container buffer we loaded. bk_baf parses & aliases it (the decoder owns the
     * parsed view), so we only keep the buffer here and free it in close_decoder()
     * after bk_baf_close(). NULL when the source was an in-memory container. */
    uint8_t * file_buf;
};

#endif /* LV_USE_BAF */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_BAF_PRIVATE_H */
