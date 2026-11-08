#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "components/bk_encode/bk_jpeg_encode_types.h"
#include "modules/vcenc/vcenc_jpeg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JPEG_ENCODE_OSD_SLOT_COUNT 8U

/*
 * OSD buffer lifecycle (per slot), mirroring the H.264 overlay manager:
 *   pending  - written by set_osd(), takes effect on next pre-encode sync
 *   active   - last config pushed to vcenc for the current encode_frame
 *   retire   - previous active buffer, freed only after encode_frame completes
 *
 * set_osd() must never free active/retire. Uncommitted pending (never synced)
 * may be freed immediately when replaced.
 */
typedef struct {
    bool enabled;
    void *buffer;
    bk_jpeg_encode_osd_buffer_free_cb_t buffer_free;
    void *free_arg;
    uint32_t format;
    uint8_t alpha;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint8_t bitmap_y;
    uint8_t bitmap_u;
    uint8_t bitmap_v;

    void *active_buffer;
    bk_jpeg_encode_osd_buffer_free_cb_t active_buffer_free;
    void *active_free_arg;
    bool active_enabled;

    void *retire_buffer;
    bk_jpeg_encode_osd_buffer_free_cb_t retire_buffer_free;
    void *retire_free_arg;
} jpeg_encode_osd_slot_state_t;

void jpeg_encode_osd_module_init(void);

avdk_err_t jpeg_encode_osd_validate_common(uint32_t frame_w,
                                           uint32_t frame_h,
                                           const bk_jpeg_encode_osd_t *osd);

avdk_err_t jpeg_encode_build_osd_bitmap_boxes(uint32_t frame_w,
                                              uint32_t frame_h,
                                              bk_jpeg_encode_osd_bitmap_boxes_t *boxes,
                                              bk_jpeg_encode_osd_t *osd);

avdk_err_t jpeg_encode_build_osd_argb8888_boxes(uint32_t frame_w,
                                                uint32_t frame_h,
                                                bk_jpeg_encode_osd_argb8888_boxes_t *boxes,
                                                bk_jpeg_encode_osd_t *osd);

/* Software (CPU) overlay for the packed YUYV422 input used by the tiny-jpeg path. */
void jpeg_encode_osd_draw_yuyv(uint8_t *frame,
                               uint32_t frame_w,
                               uint32_t frame_h,
                               jpeg_encode_osd_slot_state_t *slots);

avdk_err_t jpeg_encode_set_osd_common(jpeg_enc_param_t *enc_param,
                                      bool encoder_inited,
                                      jpeg_encode_osd_slot_state_t *slots,
                                      bk_jpeg_encode_osd_t *osd);

avdk_err_t jpeg_encode_set_osd_bitmap_boxes_common(jpeg_enc_param_t *enc_param,
                                                   bool encoder_inited,
                                                   jpeg_encode_osd_slot_state_t *slots,
                                                   bk_jpeg_encode_osd_bitmap_boxes_t *boxes);

avdk_err_t jpeg_encode_set_osd_argb8888_boxes_common(jpeg_enc_param_t *enc_param,
                                                     bool encoder_inited,
                                                     jpeg_encode_osd_slot_state_t *slots,
                                                     bk_jpeg_encode_osd_argb8888_boxes_t *boxes);

void jpeg_encode_osd_sync_to_vcenc(jpeg_enc_param_t *enc_param,
                                   jpeg_encode_osd_slot_state_t *slots);

void jpeg_encode_osd_finish_frame(jpeg_encode_osd_slot_state_t *slots);

void jpeg_encode_osd_release_all(jpeg_enc_param_t *enc_param,
                                 jpeg_encode_osd_slot_state_t *slots);

#ifdef __cplusplus
}
#endif