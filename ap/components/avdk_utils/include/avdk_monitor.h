#pragma once


//#define ISP_LOGIC_DEBUG
//#define GPU_LOGIC_DEBUG
//#define DPU_LOGIC_DEBUG
//#define DECODE_LOGIC_DEBUG
//#define ENCODE_LOGIC_DEBUG

#ifdef ISP_LOGIC_DEBUG
#define ISP_MP_FRAME_START() do { GPIO_DOWN(0); GPIO_UP(0); } while (0);
#define ISP_MP_FRAME_END() do { GPIO_DOWN(0); } while (0);
#define ISP_MP_LINE_START() do { GPIO_DOWN(1); GPIO_UP(1); } while (0);
#define ISP_MP_LINE_END() do { GPIO_DOWN(1); } while (0);
#define ISP_SP_FRAME_START() //do { GPIO_DOWN(3); GPIO_UP(3); } while (0);
#define ISP_SP_FRAME_END() //do { GPIO_DOWN(3); } while (0);
#define ISP_SP_LINE_START() //do { GPIO_DOWN(2); GPIO_UP(2); } while (0);
#define ISP_SP_LINE_END() //do { GPIO_DOWN(2); } while (0);
#else
#define ISP_MP_FRAME_START()
#define ISP_MP_FRAME_END()
#define ISP_MP_LINE_START()
#define ISP_MP_LINE_END()
#define ISP_SP_FRAME_START()
#define ISP_SP_FRAME_END()
#define ISP_SP_LINE_START()
#define ISP_SP_LINE_END()
#endif

#ifdef GPU_LOGIC_DEBUG
#define GPU_FRAME_START() do { GPIO_DOWN(8); GPIO_UP(8); } while (0);
#define GPU_FRAME_END() do { GPIO_DOWN(8); } while (0);
#define GPU_LINE_START() do { GPIO_DOWN(9); GPIO_UP(9); } while (0);
#define GPU_LINE_END() do { GPIO_DOWN(9); } while (0);
#define HPDMA_LINE_START() do { GPIO_DOWN(10); GPIO_UP(10); } while (0);
#define HPDMA_LINE_END() do { GPIO_DOWN(10); } while (0);

#else
#define GPU_FRAME_START()
#define GPU_FRAME_END()
#define GPU_LINE_START()
#define GPU_LINE_END()

#define HPDMA_LINE_START()
#define HPDMA_LINE_END()
#endif

#ifdef DPU_LOGIC_DEBUG
#define DPU_VIDEO_FRAME_START() do { GPIO_DOWN(11); GPIO_UP(11); } while (0);
#define DPU_VIDEO_FRAME_END() do { GPIO_DOWN(11); } while (0);
#define DPU_VIDEO_ISR_START() do { GPIO_DOWN(20); GPIO_UP(20); } while (0);
#define DPU_VIDEO_ISR_END() do { GPIO_DOWN(20); } while (0);
#define DPU_GRAPHIC_FRAME_START() //do { GPIO_DOWN(11); GPIO_UP(11); } while (0);
#define DPU_GRAPHIC_FRAME_END() //do { GPIO_DOWN(11); } while (0);
#define DPU_GRAPHIC_ISR_START() //do { GPIO_DOWN(20); GPIO_UP(20); } while (0);
#define DPU_GRAPHIC_ISR_END() //do { GPIO_DOWN(20); } while (0);
#else
#define DPU_VIDEO_FRAME_START()
#define DPU_VIDEO_FRAME_END()
#define DPU_VIDEO_ISR_START()
#define DPU_VIDEO_ISR_END()
#define DPU_GRAPHIC_FRAME_START()
#define DPU_GRAPHIC_FRAME_END()
#define DPU_GRAPHIC_ISR_START()
#define DPU_GRAPHIC_ISR_END()
#endif

#ifdef DECODE_LOGIC_DEBUG
#define DECODE_FRAME_START      do { GPIO_DOWN(34); GPIO_UP(34); } while (0);
#define DECODE_FRAME_END        do { GPIO_DOWN(34); } while (0);
#define DECODE_LINE_START       do { GPIO_DOWN(35); GPIO_UP(35); } while (0);
#define DECODE_LINE_END         do { GPIO_DOWN(35); } while (0);
#define DECODE_FRAME_DONE       do { GPIO_UP(36); GPIO_DOWN(36); } while (0);
#else
#define DECODE_FRAME_START
#define DECODE_FRAME_END
#define DECODE_LINE_START
#define DECODE_LINE_END
#define DECODE_FRAME_DONE
#endif

#ifdef ENCODE_LOGIC_DEBUG
#define ENCODE_FRAME_START      do { GPIO_DOWN(37); GPIO_UP(37); } while (0);
#define ENCODE_FRAME_END        do { GPIO_DOWN(37); } while (0);
#define ENCODE_LINE_START       do { GPIO_DOWN(38); GPIO_UP(38); } while (0);
#define ENCODE_LINE_END         do { GPIO_DOWN(38); } while (0);
#define ENCODE_FRAME_DONE       do { GPIO_UP(39); GPIO_DOWN(39); } while (0);
#else
#define ENCODE_FRAME_START
#define ENCODE_FRAME_END
#define ENCODE_LINE_START
#define ENCODE_LINE_END
#define ENCODE_FRAME_DONE
#endif


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