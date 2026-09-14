#pragma once

#include <avdk_error.h>
#include <stdint.h>
#include <components/bk_gpu_ctlr.h>   /* bk_gpu_ctlr_handle_t */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Shared LCD/DPU bring-up (hx8399c MIPI 1080x1920 + DPU decompress). Used by MIPI and UVC paths,
 * so kept as a separate module (not embedded in either pipeline):
 *   - display_open / display_close manage panel VDDIO 1.8V (PM_AUXLDO_USER_DISPLAY):
 *     enable before DSI, disable after DPU/panel teardown; UVC-only open won't miss VDDIO.
 *   - MIPI: display_open() only; GPU built by mipi_pipeline;
 *   - UVC:  display_open_gpu_flexa() also creates display GPU (NV12->ARGB8888 rotate90 + compress);
 *           OSD binds via display_get_gpu_handle() for SRC_OVER.
 */

/* Open LCD + DPU only (no GPU) */
avdk_err_t display_open(void);

/* Open LCD + DPU and create flexa GPU (UVC path; src=NV12 line-by-line) */
avdk_err_t display_open_gpu_flexa(uint16_t width, uint16_t height,
                                  uint8_t *src_buffer, uint8_t flexa_buff_cnt);

/* Display GPU handle (non-NULL only after display_open_gpu_flexa; NULL on MIPI path) */
bk_gpu_ctlr_handle_t display_get_gpu_handle(void);

/* DPU handle (for frame_done flush) */
void *display_get_dpu_handle(void);

/* Close display */
avdk_err_t display_close(void);

#ifdef __cplusplus
}
#endif
