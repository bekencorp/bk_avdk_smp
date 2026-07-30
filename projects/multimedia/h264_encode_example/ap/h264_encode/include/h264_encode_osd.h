#pragma once

#include <stdint.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_encode/bk_h264_encode_ctlr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t scale;
    uint32_t argb;
} h264_encode_osd_layout_t;

avdk_err_t h264_encode_osd_submit_slot(bk_h264_encode_ctlr_handle_t handle,
                                       uint32_t slot_index,
                                       const char *text);

avdk_err_t h264_encode_osd_submit_all(bk_h264_encode_ctlr_handle_t handle,
                                      uint32_t elapsed_ms,
                                      uint32_t frame_index);

avdk_err_t h264_encode_osd_clear_slot(bk_h264_encode_ctlr_handle_t handle,
                                      uint32_t slot_index);

const h264_encode_osd_layout_t *h264_encode_osd_get_layout(uint32_t slot_index);

int h264_encode_osd_test(void);

#ifdef __cplusplus
}
#endif
