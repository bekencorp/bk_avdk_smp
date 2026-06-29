#ifndef __OSD_DISP_COMPAT_H__
#define __OSD_DISP_COMPAT_H__

/*
 * bk7259 显示/帧缓冲兼容层
 *
 * bk7258 的 draw_osd_example 依赖 multimedia 组件的 frame_buffer.h
 * （frame_buffer_display_malloc/free + frame_buffer_t）以及 RGB LCD
 * （bk_display_rgb_new）。bk7259 没有这些 API：帧缓冲走
 * bk_frame_buffer_malloc()，显示走 DSI(bus->panel->dpu) + bk_display_flush()。
 *
 * 本兼容层在 example 内重新实现 7258 用到的几个符号，使 OSD 组件与测试
 * 代码无需大改即可在 7259 上编译运行：
 *   - frame_buffer_t（直接复用 7259 的 common/avdk_pixel_types.h 定义）
 *   - frame_buffer_display_malloc/free（包一层 bk_frame_buffer_malloc/free）
 *   - osd_display_open/close（DSI st7701sn 480x854, RGB565）
 *   - osd_display_flush（把 fb->frame 交给 bk_display_flush，并释放包装结构）
 */

#include <common/avdk_pixel_types.h>          /* frame_buffer_t / pixel_format_t */
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_display.h>             /* bk_display_ctlr_handle_t / bk_display_flush */

#ifdef __cplusplus
extern "C" {
#endif

/* OSD 绘制使用的背景分辨率，对齐所选 DSI 面板 st7701sn 480x854 */
#define OSD_BG_W   480
#define OSD_BG_H   854

/**
 * @brief 分配一个显示用帧缓冲（结构体 + 像素 buffer）
 * @param size 像素数据字节数
 * @return frame_buffer_t*；失败返回 NULL。frame 字段指向可被 DPU flush 的内存
 */
frame_buffer_t *frame_buffer_display_malloc(uint32_t size);

/**
 * @brief 释放 frame_buffer_display_malloc 返回的帧缓冲（释放像素 buffer + 结构体）
 */
void frame_buffer_display_free(frame_buffer_t *fb);

/**
 * @brief 打开 DSI 显示（st7701sn 480x854, RGB565），返回 DPU 控制器句柄
 */
avdk_err_t osd_display_open(bk_display_ctlr_handle_t *out_handle);

/**
 * @brief 关闭显示，释放 DPU/panel/bus
 */
avdk_err_t osd_display_close(bk_display_ctlr_handle_t handle);

/**
 * @brief 把 fb 的像素数据送显，并释放 fb 包装结构。
 *        成功时像素 buffer 的释放交给显示驱动回调；失败时本函数释放全部内存。
 */
avdk_err_t osd_display_flush(bk_display_ctlr_handle_t handle, frame_buffer_t *fb);

#ifdef __cplusplus
}
#endif

#endif /* __OSD_DISP_COMPAT_H__ */
