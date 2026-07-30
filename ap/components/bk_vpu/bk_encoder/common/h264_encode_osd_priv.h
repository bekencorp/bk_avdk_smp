#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "components/bk_encode/bk_h264_encode_types.h"
#include "modules/vcenc/vcenc_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define H264_ENCODE_OSD_SLOT_COUNT 8U

/*
 * OSD buffer lifecycle (per slot):
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
    bk_h264_encode_osd_buffer_free_cb_t buffer_free;
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
    bk_h264_encode_osd_buffer_free_cb_t active_buffer_free;
    void *active_free_arg;
    bool active_enabled;

    void *retire_buffer;
    bk_h264_encode_osd_buffer_free_cb_t retire_buffer_free;
    void *retire_free_arg;
} h264_encode_osd_slot_state_t;

void h264_encode_osd_module_init(void);

avdk_err_t h264_encode_set_osd_common(h264_enc_param_t *enc_param,
                                      bool encoder_inited,
                                      h264_encode_osd_slot_state_t *slots,
                                      bk_h264_encode_osd_t *osd);

void h264_encode_osd_sync_to_vcenc(h264_enc_param_t *enc_param,
                                   h264_encode_osd_slot_state_t *slots);

void h264_encode_osd_finish_frame(h264_encode_osd_slot_state_t *slots);

void h264_encode_osd_release_all(h264_enc_param_t *enc_param,
                                 h264_encode_osd_slot_state_t *slots);

#ifdef __cplusplus
}
#endif
