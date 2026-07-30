#pragma once

#include <avdk_error.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * UVC full pipeline one-shot open/close: UVC host camera -> MJPEG hw decode -> flexa GPU (display) -> LCD.
 * Internally wires encode frame queue / MJPEG decode thread / UVC camera / flexa bond; three public entry points only.
 * GPU is created by display module (display_open_gpu_flexa); OSD binds via display_get_gpu_handle().
 */

avdk_err_t uvc_pipeline_open(uint8_t port, uint16_t width, uint16_t height, uint8_t fps);
avdk_err_t uvc_pipeline_close(void);
bool       uvc_pipeline_is_open(void);

#ifdef __cplusplus
}
#endif
