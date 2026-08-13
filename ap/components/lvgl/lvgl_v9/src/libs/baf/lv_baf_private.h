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
};

#endif /* LV_USE_BAF */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_BAF_PRIVATE_H */
