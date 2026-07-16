#pragma once

#include <stdint.h>

#define VP_H264_PARAM_SET_MAX_SIZE        1024U
#define VP_H264_FRAME_PAD_BYTES           2048U
#define VP_H264_FRAME_BUF_ALIGN_BYTES     64U

static inline uint32_t vp_h264_align_up(uint32_t value, uint32_t align)
{
    return (value + align - 1U) & ~(align - 1U);
}

static inline uint32_t vp_h264_annexb_alloc_size(uint32_t payload_plus_pad)
{
    return vp_h264_align_up(payload_plus_pad, VP_H264_FRAME_BUF_ALIGN_BYTES);
}
