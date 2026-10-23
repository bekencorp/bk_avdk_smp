#pragma once

#include <stdint.h>
#include <os/os.h>
#include <soc/soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Logic-analyzer debug IO
 * ============================================================================
 *
 * Every probe below is driven by one project-level macro DEBUG_IO_<SIGNAL>_PIN.
 * Define it to a GPIO id (0 ~ SOC_GPIO_NUM-1) in the project's
 *
 *     projects/<proj>/ap/config/<target>/usr_debug_io_cfg.h
 *
 * to compile that probe in on that pad; leave it undefined to compile the probe
 * out completely (zero code). The file is optional - same convention as
 * usr_gpio_cfg.h / usr_dbg_probe_cfg.h. No runtime init is needed: the
 * register value written by GPIO_UP/GPIO_DOWN already selects the GPIO output
 * function (gpio_fun_sel = FUNC_CODE_OUTPUT) and disables the pull on that pad.
 *
 * The probes are built on GPIO_UP/GPIO_DOWN from soc/bk7259/soc_debug.h, so
 * the master switch CONFIG_GPIO_DEBUG_ENABLE there must also be set to 1 for
 * the pads to physically toggle; with it at 0 every probe stays a no-op.
 */

#if defined(__has_include)
#  if __has_include("usr_debug_io_cfg.h")
#    include "usr_debug_io_cfg.h"
#  endif
#endif

#define DEBUG_IO_PULSE(id) do { GPIO_UP(id); GPIO_DOWN(id); } while (0)

/* ---- ISP MI ------------------------------------------------------------- */

#ifdef DEBUG_IO_ISP_MP_FRAME_PIN
#define ISP_MP_FRAME_START()  GPIO_UP(DEBUG_IO_ISP_MP_FRAME_PIN)
#define ISP_MP_FRAME_END()    GPIO_DOWN(DEBUG_IO_ISP_MP_FRAME_PIN)
#else
#define ISP_MP_FRAME_START()  do {} while (0)
#define ISP_MP_FRAME_END()    do {} while (0)
#endif

#ifdef DEBUG_IO_ISP_MP_LINE_PIN
#define ISP_MP_LINE_START()   GPIO_UP(DEBUG_IO_ISP_MP_LINE_PIN)
#define ISP_MP_LINE_END()     GPIO_DOWN(DEBUG_IO_ISP_MP_LINE_PIN)
#else
#define ISP_MP_LINE_START()   do {} while (0)
#define ISP_MP_LINE_END()     do {} while (0)
#endif

#ifdef DEBUG_IO_ISP_SP_FRAME_PIN
#define ISP_SP_FRAME_START()  GPIO_UP(DEBUG_IO_ISP_SP_FRAME_PIN)
#define ISP_SP_FRAME_END()    GPIO_DOWN(DEBUG_IO_ISP_SP_FRAME_PIN)
#else
#define ISP_SP_FRAME_START()  do {} while (0)
#define ISP_SP_FRAME_END()    do {} while (0)
#endif

#ifdef DEBUG_IO_ISP_SP_LINE_PIN
#define ISP_SP_LINE_START()   GPIO_UP(DEBUG_IO_ISP_SP_LINE_PIN)
#define ISP_SP_LINE_END()     GPIO_DOWN(DEBUG_IO_ISP_SP_LINE_PIN)
#else
#define ISP_SP_LINE_START()   do {} while (0)
#define ISP_SP_LINE_END()     do {} while (0)
#endif

/* ---- GPU / HPDMA / OSD -------------------------------------------------- */

#ifdef DEBUG_IO_GPU_FRAME_PIN
#define GPU_FRAME_START()     GPIO_UP(DEBUG_IO_GPU_FRAME_PIN)
#define GPU_FRAME_END()       GPIO_DOWN(DEBUG_IO_GPU_FRAME_PIN)
#else
#define GPU_FRAME_START()     do {} while (0)
#define GPU_FRAME_END()       do {} while (0)
#endif

#ifdef DEBUG_IO_GPU_LINE_PIN
#define GPU_LINE_START()      GPIO_UP(DEBUG_IO_GPU_LINE_PIN)
#define GPU_LINE_END()        GPIO_DOWN(DEBUG_IO_GPU_LINE_PIN)
#else
#define GPU_LINE_START()      do {} while (0)
#define GPU_LINE_END()        do {} while (0)
#endif

#ifdef DEBUG_IO_HPDMA_LINE_PIN
#define HPDMA_LINE_START()    GPIO_UP(DEBUG_IO_HPDMA_LINE_PIN)
#define HPDMA_LINE_END()      GPIO_DOWN(DEBUG_IO_HPDMA_LINE_PIN)
#else
#define HPDMA_LINE_START()    do {} while (0)
#define HPDMA_LINE_END()      do {} while (0)
#endif

/* OSD_BLIT = one complete composition, OSD_SLOT = one logical layer blit.
 * Layers include PIP and product OSD (time/WiFi). */
#ifdef DEBUG_IO_OSD_BLIT_PIN
#define OSD_BLIT_START()      GPIO_UP(DEBUG_IO_OSD_BLIT_PIN)
#define OSD_BLIT_END()        GPIO_DOWN(DEBUG_IO_OSD_BLIT_PIN)
#else
#define OSD_BLIT_START()      do {} while (0)
#define OSD_BLIT_END()        do {} while (0)
#endif

#ifdef DEBUG_IO_OSD_SLOT_PIN
#define OSD_SLOT_START()      GPIO_UP(DEBUG_IO_OSD_SLOT_PIN)
#define OSD_SLOT_END()        GPIO_DOWN(DEBUG_IO_OSD_SLOT_PIN)
#else
#define OSD_SLOT_START()      do {} while (0)
#define OSD_SLOT_END()        do {} while (0)
#endif

/* ---- DPU ---------------------------------------------------------------- */

#ifdef DEBUG_IO_DPU_VIDEO_FRAME_PIN
#define DPU_VIDEO_FRAME_START()   GPIO_UP(DEBUG_IO_DPU_VIDEO_FRAME_PIN)
#define DPU_VIDEO_FRAME_END()     GPIO_DOWN(DEBUG_IO_DPU_VIDEO_FRAME_PIN)
#else
#define DPU_VIDEO_FRAME_START()   do {} while (0)
#define DPU_VIDEO_FRAME_END()     do {} while (0)
#endif

#ifdef DEBUG_IO_DPU_VIDEO_ISR_PIN
#define DPU_VIDEO_ISR_START()     GPIO_UP(DEBUG_IO_DPU_VIDEO_ISR_PIN)
#define DPU_VIDEO_ISR_END()       GPIO_DOWN(DEBUG_IO_DPU_VIDEO_ISR_PIN)
#else
#define DPU_VIDEO_ISR_START()     do {} while (0)
#define DPU_VIDEO_ISR_END()       do {} while (0)
#endif

#ifdef DEBUG_IO_DPU_GRAPHIC_FRAME_PIN
#define DPU_GRAPHIC_FRAME_START() GPIO_UP(DEBUG_IO_DPU_GRAPHIC_FRAME_PIN)
#define DPU_GRAPHIC_FRAME_END()   GPIO_DOWN(DEBUG_IO_DPU_GRAPHIC_FRAME_PIN)
#else
#define DPU_GRAPHIC_FRAME_START() do {} while (0)
#define DPU_GRAPHIC_FRAME_END()   do {} while (0)
#endif

#ifdef DEBUG_IO_DPU_GRAPHIC_ISR_PIN
#define DPU_GRAPHIC_ISR_START()   GPIO_UP(DEBUG_IO_DPU_GRAPHIC_ISR_PIN)
#define DPU_GRAPHIC_ISR_END()     GPIO_DOWN(DEBUG_IO_DPU_GRAPHIC_ISR_PIN)
#else
#define DPU_GRAPHIC_ISR_START()   do {} while (0)
#define DPU_GRAPHIC_ISR_END()     do {} while (0)
#endif

/* ---- DECODE / ENCODE ----------------------------------------------------
 * Object-like macros (call sites use them without parentheses). *_DONE is a
 * single pulse. */

#ifdef DEBUG_IO_DECODE_FRAME_PIN
#define DECODE_FRAME_START    GPIO_UP(DEBUG_IO_DECODE_FRAME_PIN)
#define DECODE_FRAME_END      GPIO_DOWN(DEBUG_IO_DECODE_FRAME_PIN)
#else
#define DECODE_FRAME_START    do {} while (0)
#define DECODE_FRAME_END      do {} while (0)
#endif

#ifdef DEBUG_IO_DECODE_LINE_PIN
#define DECODE_LINE_START     GPIO_UP(DEBUG_IO_DECODE_LINE_PIN)
#define DECODE_LINE_END       GPIO_DOWN(DEBUG_IO_DECODE_LINE_PIN)
#else
#define DECODE_LINE_START     do {} while (0)
#define DECODE_LINE_END       do {} while (0)
#endif

#ifdef DEBUG_IO_DECODE_DONE_PIN
#define DECODE_FRAME_DONE     DEBUG_IO_PULSE(DEBUG_IO_DECODE_DONE_PIN)
#else
#define DECODE_FRAME_DONE     do {} while (0)
#endif

#ifdef DEBUG_IO_ENCODE_FRAME_PIN
#define ENCODE_FRAME_START    GPIO_UP(DEBUG_IO_ENCODE_FRAME_PIN)
#define ENCODE_FRAME_END      GPIO_DOWN(DEBUG_IO_ENCODE_FRAME_PIN)
#else
#define ENCODE_FRAME_START    do {} while (0)
#define ENCODE_FRAME_END      do {} while (0)
#endif

#ifdef DEBUG_IO_ENCODE_LINE_PIN
#define ENCODE_LINE_START     GPIO_UP(DEBUG_IO_ENCODE_LINE_PIN)
#define ENCODE_LINE_END       GPIO_DOWN(DEBUG_IO_ENCODE_LINE_PIN)
#else
#define ENCODE_LINE_START     do {} while (0)
#define ENCODE_LINE_END       do {} while (0)
#endif

#ifdef DEBUG_IO_ENCODE_DONE_PIN
#define ENCODE_FRAME_DONE     DEBUG_IO_PULSE(DEBUG_IO_ENCODE_DONE_PIN)
#else
#define ENCODE_FRAME_DONE     do {} while (0)
#endif

/* ---- Generic slots for solution-level probes (NPU, 3A, camera id, ...) ---- */

#ifdef DEBUG_IO_USER0_PIN
#define DEBUG_IO_USER0_UP()   GPIO_UP(DEBUG_IO_USER0_PIN)
#define DEBUG_IO_USER0_DOWN() GPIO_DOWN(DEBUG_IO_USER0_PIN)
#else
#define DEBUG_IO_USER0_UP()   do {} while (0)
#define DEBUG_IO_USER0_DOWN() do {} while (0)
#endif

#ifdef DEBUG_IO_USER1_PIN
#define DEBUG_IO_USER1_UP()   GPIO_UP(DEBUG_IO_USER1_PIN)
#define DEBUG_IO_USER1_DOWN() GPIO_DOWN(DEBUG_IO_USER1_PIN)
#else
#define DEBUG_IO_USER1_UP()   do {} while (0)
#define DEBUG_IO_USER1_DOWN() do {} while (0)
#endif

#ifdef DEBUG_IO_USER2_PIN
#define DEBUG_IO_USER2_UP()   GPIO_UP(DEBUG_IO_USER2_PIN)
#define DEBUG_IO_USER2_DOWN() GPIO_DOWN(DEBUG_IO_USER2_PIN)
#else
#define DEBUG_IO_USER2_UP()   do {} while (0)
#define DEBUG_IO_USER2_DOWN() do {} while (0)
#endif

#ifdef DEBUG_IO_USER3_PIN
#define DEBUG_IO_USER3_UP()   GPIO_UP(DEBUG_IO_USER3_PIN)
#define DEBUG_IO_USER3_DOWN() GPIO_DOWN(DEBUG_IO_USER3_PIN)
#else
#define DEBUG_IO_USER3_UP()   do {} while (0)
#define DEBUG_IO_USER3_DOWN() do {} while (0)
#endif

/* ============================================================================
 * Runtime fps / irq monitor
 * ============================================================================ */

typedef struct {
    uint8_t enable;
    //uint32_t isp : 1;
    uint32_t mp : 1;
    uint32_t sp : 1;
    uint32_t gpu : 1;
    uint32_t dpu : 1;

    uint16_t isp_mp_frame_count;
    uint16_t isp_mp_line_count;

    uint16_t isp_sp_frame_count;
    uint16_t isp_sp_line_count;

    uint16_t gpu_frame_count;
    uint16_t gpu_line_count;

    uint16_t dpu_fps_count;
    uint16_t dpu_isr_count;

    beken_thread_t thread;
} avdk_monitor_info_t;

extern avdk_monitor_info_t *avdk_monitor_info;

#define AVDK_MONITOR_MP_ENABLE() if (avdk_monitor_info) avdk_monitor_info->mp = 1;
#define AVDK_MONITOR_MP_LINE_PLUS() if (avdk_monitor_info) avdk_monitor_info->isp_mp_line_count++;
#define AVDK_MONITOR_MP_FRAME_PLUS() if (avdk_monitor_info) avdk_monitor_info->isp_mp_frame_count++;
#define AVDK_MONITOR_SP_ENABLE() if (avdk_monitor_info) avdk_monitor_info->sp = 1;
#define AVDK_MONITOR_SP_LINE_PLUS() if (avdk_monitor_info) avdk_monitor_info->isp_sp_line_count++;
#define AVDK_MONITOR_SP_FRAME_PLUS() if (avdk_monitor_info) avdk_monitor_info->isp_sp_frame_count++;
#define AVDK_MONITOR_GPU_ENABLE() if (avdk_monitor_info) avdk_monitor_info->gpu = 1;
#define AVDK_MONITOR_GPU_LINE_PLUS() if (avdk_monitor_info) avdk_monitor_info->gpu_line_count++;
#define AVDK_MONITOR_GPU_FRAME_PLUS() if (avdk_monitor_info) avdk_monitor_info->gpu_frame_count++;
#define AVDK_MONITOR_DPU_ENABLE() if (avdk_monitor_info) avdk_monitor_info->dpu = 1;
#define AVDK_MONITOR_DPU_FPS_PLUS() if (avdk_monitor_info) avdk_monitor_info->dpu_fps_count++;
#define AVDK_MONITOR_DPU_ISR_PLUS() if (avdk_monitor_info) avdk_monitor_info->dpu_isr_count++;

void avdk_monitor_init(void);
void avdk_monitor_start(void);
void avdk_monitor_stop(void);
void avdk_monitor_deinit(void);

#ifdef __cplusplus
}
#endif
