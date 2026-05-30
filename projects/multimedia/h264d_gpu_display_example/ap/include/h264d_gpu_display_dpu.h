#pragma once

#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

avdk_err_t h264d_gpu_display_dpu_open(void);
void h264d_gpu_display_dpu_close(void);
avdk_err_t h264d_gpu_display_dpu_flush(void *frame, avdk_err_t (*free_cb)(void *args));
uint16_t h264d_gpu_display_dpu_width(void);
uint16_t h264d_gpu_display_dpu_height(void);

#ifdef __cplusplus
}
#endif
