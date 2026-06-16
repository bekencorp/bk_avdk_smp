#pragma once

#include <stdint.h>
#include <common/avdk_pixel_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pixel format codes aligned with isp_dump_tool.py / PIXEL_FORMAT_* */
#define ISP_FRAME_FORMAT_RAW10   21
#define ISP_FRAME_FORMAT_NV12    23

#define ISP_FRAME_RX_TIMEOUT_MS_MIN  1U
#define ISP_FRAME_RX_TIMEOUT_MS_MAX  1000000U

typedef struct
{
    uint32_t rx_timeout;
    uint16_t sensor_w;
    uint16_t sensor_h;
    uint16_t isp_w;
    uint16_t isp_h;
    uint16_t format;
    char pattern[8];
} isp_frame_capture_config_t;

void isp_frame_capture_apply_resolution(uint16_t sensor_w, uint16_t sensor_h,
                                      uint16_t isp_w, uint16_t isp_h);

static inline int isp_frame_format_is_valid(uint16_t format)
{
    return (format == ISP_FRAME_FORMAT_RAW10 || format == ISP_FRAME_FORMAT_NV12);
}

static inline bk_pixel_format_t isp_frame_format_to_bk_pixel(uint16_t format)
{
    if (format == ISP_FRAME_FORMAT_RAW10)
    {
        return BK_PIXEL_FORMAT_RAW10;
    }
    return BK_PIXEL_FORMAT_NV12;
}

static inline uint32_t isp_frame_rx_timeout_ms_for_read(uint32_t configured_ms, uint32_t fallback_ms)
{
    uint32_t t = configured_ms ? configured_ms : fallback_ms;

    if (t < ISP_FRAME_RX_TIMEOUT_MS_MIN)
    {
        t = ISP_FRAME_RX_TIMEOUT_MS_MIN;
    }
    if (t > ISP_FRAME_RX_TIMEOUT_MS_MAX)
    {
        t = ISP_FRAME_RX_TIMEOUT_MS_MAX;
    }
    return t;
}

#ifdef __cplusplus
}
#endif
