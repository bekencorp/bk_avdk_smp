#pragma once

#include <components/bk_gpu.h>
#include "h264d_gpu_display_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*h264d_gpu_display_gpu_line_done_cb_t)(uint32_t done_lines, void *args);
typedef void (*h264d_gpu_display_gpu_frame_done_cb_t)(void *frame, uint32_t frame_size, void *args);

avdk_err_t h264d_gpu_display_gpu_open(uint8_t *src_buffer,
				      uint8_t flexa_buffer_count,
				      uint16_t src_width,
				      uint16_t src_height,
				      h264d_gpu_display_gpu_line_done_cb_t line_done_cb,
				      void *line_done_args,
				      h264d_gpu_display_gpu_frame_done_cb_t frame_done_cb,
				      void *frame_done_args);
avdk_err_t h264d_gpu_display_gpu_blit_rgb_frame(const uint8_t *src_buffer,
						uint16_t src_width,
						uint16_t src_height,
						bk_pixel_format_t src_format,
						void **out_frame,
						uint32_t *out_frame_size);
void h264d_gpu_display_gpu_blit_deinit(void);
void h264d_gpu_display_gpu_close(void);
void h264d_gpu_display_gpu_frame_pool_deinit(void);
bk_gpu_ctlr_handle_t h264d_gpu_display_gpu_handle_get(void);

#ifdef __cplusplus
}
#endif
