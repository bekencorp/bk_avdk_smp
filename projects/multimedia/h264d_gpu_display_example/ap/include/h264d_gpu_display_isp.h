#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <avdk_error.h>

#include "h264d_gpu_display_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP

typedef struct {
	uint16_t sensor_w;   /* CSI sensor input width  (e.g. 1280) */
	uint16_t sensor_h;   /* CSI sensor input height (e.g. 720)  */
	uint16_t fps;        /* sensor frame rate (e.g. 20)         */
	uint16_t isp_w;       /* ISP output width  (<= sensor_w)  */
	uint16_t isp_h;       /* ISP output height (<= sensor_h)  */
	uint16_t dst_x;      /* overlay x on GPU output frame       */
	uint16_t dst_y;      /* overlay y on GPU output frame       */
} h264d_gpu_display_isp_params_t;

/*
 * Bring up CSI sensor + ISP channel and start the overlay task.
 * ISP frames will be blitted onto the GPU output frame via bk_gpu_blit_set
 * each time the GPU controller is open. Safe to call before or after the
 * H.264 pipeline is started.
 */
avdk_err_t h264d_gpu_display_isp_open(const h264d_gpu_display_isp_params_t *params);

bool h264d_gpu_display_isp_is_open(void);

/*
 * Stop overlay task, clear GPU blit, close ISP channel, tear down ISP +
 * sensor + bus. Safe to call multiple times.
 */
void h264d_gpu_display_isp_close(void);

#endif /* H264D_GPU_DISPLAY_ENABLE_ISP_PIP */

#ifdef __cplusplus
}
#endif
