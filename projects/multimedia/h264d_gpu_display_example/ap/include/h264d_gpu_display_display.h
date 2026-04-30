#pragma once

#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

avdk_err_t h264d_gpu_display_display_open(void);
void h264d_gpu_display_display_close(void);
void *h264d_gpu_display_display_handle_get(void);
avdk_err_t h264d_gpu_display_display_flush(void *frame, avdk_err_t (*free_cb)(void *args));
uint16_t h264d_gpu_display_display_width(void);
uint16_t h264d_gpu_display_display_height(void);

#ifdef __cplusplus
}
#endif
