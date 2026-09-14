#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include <components/bk_isp_camera.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ISP MP frame mode + vc_mux peek -> GPU -> LCD (single camera `isp open mp`, align doorbell) */
avdk_err_t isp_display_pipeline_start(bk_isp_camera_ctlr_handle_t camera_handle,
                                      uint16_t src_w, uint16_t src_h);
avdk_err_t isp_display_pipeline_stop(void);
bool isp_display_pipeline_is_running(void);

/* Frame-mode GPU for dual-VC route (vc_mux peek -> GPU -> LCD) */
avdk_err_t isp_display_gpu_frame_turn_on(uint16_t width, uint16_t height);
avdk_err_t isp_display_gpu_frame_turn_on_with_flexa_src(uint16_t width, uint16_t height,
                                                        void *src_buffer, uint8_t flexa_buff_cnt,
                                                        void (*flexa_line_done)(uint32_t done_lines, void *args),
                                                        void *flexa_line_done_args);
avdk_err_t isp_display_gpu_frame_turn_off(void);
avdk_err_t isp_display_gpu_frame_process(void *frame, uint32_t sequence,
                                         void (*input_free)(void *frame, void *args), void *args);
avdk_err_t isp_display_gpu_frame_prepare(void *frame,
                                         void (*input_free)(void *frame, void *args), void *args);
avdk_err_t isp_display_gpu_frame_lines_ready(uint32_t sequence, uint32_t line_cnt);
void isp_display_gpu_frame_abort(void);
bool isp_display_gpu_frame_busy(void);
bool isp_display_lcd_is_open(void);

#ifdef __cplusplus
}
#endif
