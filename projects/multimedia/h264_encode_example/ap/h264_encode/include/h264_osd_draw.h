#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define H264_OSD_SLOT_COUNT          8U

#define H264_OSD_TIME_WIDTH          112U
#define H264_OSD_TIME_HEIGHT         16U
#define H264_OSD_TIME_FONT_SCALE     2U

#define H264_OSD_LABEL_WIDTH         36U
#define H264_OSD_LABEL_HEIGHT        14U
#define H264_OSD_LABEL_FONT_SCALE    2U

void h264_osd_draw_text_argb8888(uint8_t *buffer, uint32_t width, uint32_t height,
                                 const char *text, uint32_t scale, uint32_t argb);

void h264_osd_draw_time_argb8888(uint8_t *buffer, uint32_t width, uint32_t height,
                                 const char *text);

void h264_osd_draw_text_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
                             const char *text, uint32_t scale,
                             uint8_t fg_y, uint8_t fg_u, uint8_t fg_v);

void h264_osd_draw_text_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                               const char *text, uint32_t scale);

#ifdef __cplusplus
}
#endif
