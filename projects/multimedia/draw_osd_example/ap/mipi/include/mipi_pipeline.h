#ifndef __MIPI_PIPELINE_H__
#define __MIPI_PIPELINE_H__

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include <components/bk_gpu_ctlr.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Local MIPI camera live video pipeline (like doorbell_lp `joint_test open mipi 1080p 30`):
 *
 *   GC2053 CSI --> ISP MP(NV12 flexa) --> bk_flexa_isp_gpu_bond
 *       --> GPU(rotate90 + NV12->ARGB8888 + HV compress) --> DPU/MIPI LCD(hx8399c 1080x1920)
 *
 * OSD cases compose icons/text into sprites and get the GPU controller via
 * mipi_pipeline_get_gpu_handle(); the controller composites the shared overlay.
 *
 * Mutually exclusive with UVC path: both own the same GPU + MIPI display; do not open both.
 *
 * Camera segment (GC2053 CSI + ISP MP, formerly mipi_camera.c) is merged into mipi_pipeline.c (static).
 * Only full-pipeline open/close is exposed (like uvc/src/uvc_pipeline.c).
 */

/* ---- Full pipeline (public API) ---- */
avdk_err_t mipi_pipeline_open(uint16_t width, uint16_t height, uint8_t fps);
avdk_err_t mipi_pipeline_close(void);
bool       mipi_pipeline_is_open(void);
bk_gpu_ctlr_handle_t mipi_pipeline_get_gpu_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* __MIPI_PIPELINE_H__ */
