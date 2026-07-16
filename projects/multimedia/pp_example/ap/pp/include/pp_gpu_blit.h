#pragma once

#include <common/bk_include.h>
#include <common/avdk_pixel_types.h>

#ifdef __cplusplus
extern "C" {
#endif

avdk_err_t pp_gpu_blit_rgb_frame(const uint8_t *src_buffer,
				 uint16_t src_width,
				 uint16_t src_height,
				 bk_pixel_format_t src_format,
				 void **out_frame,
				 uint32_t *out_frame_size);
void pp_gpu_blit_deinit(void);

#ifdef __cplusplus
}
#endif
