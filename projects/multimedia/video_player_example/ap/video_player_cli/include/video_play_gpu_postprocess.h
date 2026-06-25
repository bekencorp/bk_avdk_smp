#pragma once

#include <stdint.h>

#include "components/avdk_utils/avdk_types.h"
#include "video_play_callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    void    *data;
    uint32_t size;
    uint16_t visible_width;
    uint16_t visible_height;
    uint16_t render_width;
    uint16_t render_height;
} video_play_gpu_postprocess_frame_t;

avdk_err_t video_play_gpu_postprocess_nv12_rotate(const uint8_t *nv12,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  uint32_t stride,
                                                  uint32_t y_plane_height,
                                                  video_play_rotate_mode_t rotate,
                                                  video_play_gpu_postprocess_frame_t *out_frame);

avdk_err_t video_play_gpu_postprocess_free_frame(void *frame);

void video_play_gpu_postprocess_deinit(void);

#ifdef __cplusplus
}
#endif
