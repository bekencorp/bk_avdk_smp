#pragma once

#include <common/bk_include.h>
#include <common/avdk_pixel_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Whole-frame GPU blit path (used by the start_rgb565 / start_rgb888 and the
 * start_dec_scale / start_dec_scale_cvt demo modes). Takes a full source frame
 * in PSRAM (src_format: RGB565, RGB888 or NV12) and scales/rotates it into a
 * compressed display frame using a strip-based GPU->SRAM + HPDMA->PSRAM
 * pipeline (see h264d_gpu_display_gpu_blit.c). This is separate from the
 * flexa NV12 streaming path in h264d_gpu_display_gpu.h.
 */
avdk_err_t h264d_gpu_display_gpu_blit_rgb_frame(const uint8_t *src_buffer,
						uint16_t src_width,
						uint16_t src_height,
						bk_pixel_format_t src_format,
						void **out_frame,
						uint32_t *out_frame_size);
void h264d_gpu_display_gpu_blit_deinit(void);

#ifdef __cplusplus
}
#endif
