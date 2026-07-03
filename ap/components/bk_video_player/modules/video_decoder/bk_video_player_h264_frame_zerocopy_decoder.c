// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * Zero-copy / B-frame capable whole-frame H.264 video-player decoder.
 *
 * This is a parallel decoder to bk_video_player_h264_frame_decoder.c (which is
 * kept unchanged). Instead of bk_h264_decode_frame_ctlr (legacy, non-B), it
 * drives bk_h264_decode_frame_zerocopy_ctlr: the controller owns an internal
 * h264d_fbpool, decodes directly into pool buffers (no reference-frame backup
 * copies) and supports B-frame decoding with POC-based display reordering.
 *
 * Bridging the controller's "feed AU in decode order / dequeue frames in
 * display order" model onto the video_player engine's synchronous, one-AU-in /
 * one-frame-out decode() contract:
 *   - On each decode() the input AU is converted to Annex-B (with SPS/PPS
 *     injection) and fed to the controller.
 *   - All currently displayable frames are drained (DEQUEUE), copy-compacted
 *     to visible NV12 into decoder-owned buffers, and the pool frames are
 *     released immediately (so the pool never starves).
 *   - Drained frames are queued in display order in an internal FIFO; one is
 *     handed to the engine per decode() call. When no frame is ready yet
 *     (reorder fill at GOP start) decode() returns AVDK_ERR_TIMEOUT and the
 *     engine simply skips output for that AU.
 *   - PTS is recovered by mapping the sorted set of fed AU PTS values onto the
 *     display-ordered output (display order == POC order == ascending PTS).
 *   - At each IDR (GOP boundary) and at deinit the controller is flushed so
 *     trailing reordered pictures of the previous GOP are emitted.
 *
 * The single remaining copy (pool -> decoder-owned visible NV12 buffer) is
 * required by the engine's output-buffer ownership model; it is handed to the
 * engine using the same release-prealloc + pointer-swap pattern as the GPU
 * decoder (bk_video_player_h264_flexa_decoder.c).
 */

#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_decode/bk_h264_decode_ctlr.h"
#include "components/bk_decode/bk_h264_decode_types.h"
#include "components/bk_frame_buffer.h"
#include "components/media_types.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h"

#define TAG "vp_h264_fzc_dec"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define H264_NALU_TYPE_IDR       5
#define H264_NALU_TYPE_SPS       7
#define H264_NALU_TYPE_PPS       8
#define H264_NALU_HDR_TYPE(b)    ((uint8_t)((b) & 0x1FU))

#define H264_PARAM_SET_MAX_SIZE  1024U
#define H264_FRAME_PAD_BYTES     128U
#define H264_ALIGN_UP(v, a)      (((v) + ((a) - 1U)) & ~((a) - 1U))

/* Display bridging parameters.
 *
 * disp_depth is the controller's extra display-pool depth on top of the codec
 * DPB. Each pool slot is a full coded-size frame plus its MV buffer
 * (~3.6 MB at 1088x1920) drawn from the finite MEM_SLAB_HEAP_UNCODED, and the
 * pool reserves (dpb_count + disp_depth) such slots. At 1080x1920 the engine's
 * pre-allocated NV12 output buffer (~3.1 MB) is also live in the same heap, so
 * disp_depth is kept at 1: with disp_depth=1 the controller publishes exactly
 * one display-order frame at a time, which this 1-in/1-out decoder copies
 * straight into the engine's output buffer and releases immediately - no extra
 * decoder-owned frame copies, keeping peak heap usage within budget while still
 * preserving B-frame display reordering (handled by the DPB). */
#define VP_H264_FZC_DISP_DEPTH       1U
#define VP_H264_FZC_PENDING_PTS_CAP  16U
#define VP_H264_FZC_DEQUEUE_TMO_MS   30U
#define VP_H264_FZC_FLUSH_TMO_MS     200U

static bool hw_h264_frame_is_valid_nal_type(uint8_t type)
{
    /* NAL type 0 is unspecified and 24..31 are not valid AVC stream NALs. */
    return (type >= 1U && type <= 23U);
}

typedef struct
{
    bk_h264_decode_ctlr_handle_t hw_decoder_handle;
    video_player_video_params_t  video_params;
    bool                         is_initialized;

    uint8_t  *annexb_buf;
    uint32_t  annexb_buf_size;

    uint8_t  *sps_data;
    uint16_t  sps_size;
    uint8_t  *pps_data;
    uint16_t  pps_size;
    uint8_t   nalu_length_size;
    bool      need_inject_params;

    /* True when the container supplied an avcC config: every sample is then a
     * length-prefixed AVCC access unit and must always go through the
     * AVCC->Annex-B converter. Auto-detecting Annex-B per packet is unsafe for
     * AVCC because a NAL length prefix such as 0x000001xx aliases a start code,
     * which would feed the raw (read-only) packet buffer to the hardware as the
     * decode stream and let it scribble past the AU into the buffer guard. */
    bool      input_is_avcc;

    bool      seen_first_frame;   /* track GOP boundaries for flush-on-IDR */

    /* Sorted-ascending pending PTS values of fed-but-not-displayed AUs. */
    uint64_t pending_pts[VP_H264_FZC_PENDING_PTS_CAP];
    uint32_t pending_pts_count;
} hw_h264_fzc_ctx_t;

typedef struct
{
    video_player_video_decoder_ops_t ops;
    hw_h264_fzc_ctx_t                ctx;
} hw_h264_fzc_instance_t;

static video_player_video_decoder_ops_t s_ops_template;

static avdk_err_t hw_h264_fzc_deinit(struct video_player_video_decoder_ops_s *ops);

/* ----------------------------------------------------------------------------
 * SPS/PPS + AVCC->Annex-B conversion (mirrors bk_video_player_h264_frame_decoder.c).
 * -------------------------------------------------------------------------- */

static void hw_h264_fzc_release_param_sets(hw_h264_fzc_ctx_t *ctx)
{
    if (ctx->sps_data != NULL)
    {
        os_free(ctx->sps_data);
        ctx->sps_data = NULL;
    }
    ctx->sps_size = 0;

    if (ctx->pps_data != NULL)
    {
        os_free(ctx->pps_data);
        ctx->pps_data = NULL;
    }
    ctx->pps_size = 0;
}

static avdk_err_t hw_h264_fzc_parse_avcc(hw_h264_fzc_ctx_t *ctx,
                                         const uint8_t *cfg,
                                         uint32_t cfg_size)
{
    AVDK_RETURN_ON_FALSE(ctx && cfg, AVDK_ERR_INVAL, TAG, "invalid avcC args");
    AVDK_RETURN_ON_FALSE(cfg_size >= 7U, AVDK_ERR_INVAL, TAG, "avcC too short: %u", cfg_size);
    AVDK_RETURN_ON_FALSE(cfg[0] == 0x01, AVDK_ERR_INVAL, TAG, "avcC bad version: 0x%02x", cfg[0]);

    uint32_t off = 4U;
    const uint8_t length_size = (cfg[off++] & 0x03U) + 1U;
    if (length_size != 1U && length_size != 2U && length_size != 4U)
    {
        return AVDK_ERR_UNSUPPORTED;
    }
    ctx->nalu_length_size = length_size;

    const uint8_t num_sps = cfg[off++] & 0x1FU;
    if (num_sps == 0U)
    {
        return AVDK_ERR_INVAL;
    }

    for (uint32_t i = 0; i < num_sps; i++)
    {
        if (off + 2U > cfg_size)
        {
            return AVDK_ERR_INVAL;
        }
        const uint16_t len = (uint16_t)(((uint16_t)cfg[off] << 8) | cfg[off + 1U]);
        off += 2U;
        if (len == 0U || len > H264_PARAM_SET_MAX_SIZE || off + len > cfg_size)
        {
            return AVDK_ERR_INVAL;
        }
        if (i == 0U)
        {
            ctx->sps_data = (uint8_t *)os_malloc(len);
            if (ctx->sps_data == NULL)
            {
                return AVDK_ERR_NOMEM;
            }
            os_memcpy(ctx->sps_data, &cfg[off], len);
            ctx->sps_size = len;
        }
        off += len;
    }

    if (off >= cfg_size)
    {
        return AVDK_ERR_INVAL;
    }
    const uint8_t num_pps = cfg[off++];
    if (num_pps == 0U)
    {
        return AVDK_ERR_INVAL;
    }

    for (uint32_t i = 0; i < num_pps; i++)
    {
        if (off + 2U > cfg_size)
        {
            return AVDK_ERR_INVAL;
        }
        const uint16_t len = (uint16_t)(((uint16_t)cfg[off] << 8) | cfg[off + 1U]);
        off += 2U;
        if (len == 0U || len > H264_PARAM_SET_MAX_SIZE || off + len > cfg_size)
        {
            return AVDK_ERR_INVAL;
        }
        if (i == 0U)
        {
            ctx->pps_data = (uint8_t *)os_malloc(len);
            if (ctx->pps_data == NULL)
            {
                return AVDK_ERR_NOMEM;
            }
            os_memcpy(ctx->pps_data, &cfg[off], len);
            ctx->pps_size = len;
        }
        off += len;
    }

    LOGI("%s: avcC parsed, length_size=%u sps=%u pps=%u\n",
         __func__, ctx->nalu_length_size, ctx->sps_size, ctx->pps_size);
    return AVDK_ERR_OK;
}

static bool hw_h264_fzc_buffer_is_annex_b(const uint8_t *buf, uint32_t len)
{
    const uint32_t scan = (len > 64U) ? 64U : len;
    for (uint32_t i = 0; i + 3U < scan; i++)
    {
        if (buf[i] == 0x00 && buf[i + 1U] == 0x00)
        {
            if (buf[i + 2U] == 0x01)
            {
                return true;
            }
            if (buf[i + 2U] == 0x00 && buf[i + 3U] == 0x01)
            {
                return true;
            }
        }
    }
    return false;
}

static avdk_err_t hw_h264_fzc_ensure_annexb_buf(hw_h264_fzc_ctx_t *ctx, uint32_t need)
{
    if (ctx->annexb_buf != NULL && ctx->annexb_buf_size >= need)
    {
        return AVDK_ERR_OK;
    }

    if (ctx->annexb_buf != NULL)
    {
        bk_frame_buffer_free(ctx->annexb_buf);
        ctx->annexb_buf = NULL;
        ctx->annexb_buf_size = 0;
    }

    ctx->annexb_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
                                                        need + H264_FRAME_PAD_BYTES);
    if (ctx->annexb_buf == NULL)
    {
        return AVDK_ERR_NOMEM;
    }

    ctx->annexb_buf_size = need;
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_fzc_append_nalu(hw_h264_fzc_ctx_t *ctx,
                                          uint32_t *out,
                                          const uint8_t *nalu,
                                          uint32_t nalu_len)
{
    if (*out + 4U + nalu_len > ctx->annexb_buf_size)
    {
        return AVDK_ERR_NOMEM;
    }

    ctx->annexb_buf[(*out)++] = 0x00;
    ctx->annexb_buf[(*out)++] = 0x00;
    ctx->annexb_buf[(*out)++] = 0x00;
    ctx->annexb_buf[(*out)++] = 0x01;
    os_memcpy(&ctx->annexb_buf[*out], nalu, nalu_len);
    *out += nalu_len;
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_fzc_avcc_to_annexb(hw_h264_fzc_ctx_t *ctx,
                                             const uint8_t *src,
                                             uint32_t src_len,
                                             bool inject_param_sets,
                                             uint32_t *out_len)
{
    AVDK_RETURN_ON_FALSE(ctx && src && out_len, AVDK_ERR_INVAL, TAG, "invalid args");

    const uint8_t length_size = ctx->nalu_length_size;
    if (length_size != 1U && length_size != 2U && length_size != 4U)
    {
        return AVDK_ERR_INVAL;
    }

    bool has_idr = false;
    bool has_inband_sps = false;
    bool has_inband_pps = false;
    bool has_invalid_nal = false;
    uint32_t scan = 0;
    while (scan + length_size <= src_len)
    {
        uint32_t nalu_len = 0;
        for (uint8_t k = 0; k < length_size; k++)
        {
            nalu_len = (nalu_len << 8) | src[scan + k];
        }
        scan += length_size;
        if (nalu_len == 0U || nalu_len > (src_len - scan))
        {
            break;
        }

        const uint8_t type = H264_NALU_HDR_TYPE(src[scan]);
        if (!hw_h264_frame_is_valid_nal_type(type))
        {
            has_invalid_nal = true;
            break;
        }
        if (type == H264_NALU_TYPE_IDR)
        {
            has_idr = true;
        }
        else if (type == H264_NALU_TYPE_SPS)
        {
            has_inband_sps = true;
        }
        else if (type == H264_NALU_TYPE_PPS)
        {
            has_inband_pps = true;
        }
        scan += nalu_len;
    }

    const bool sample_self_contained = (has_inband_sps && has_inband_pps);
    const bool do_inject = (inject_param_sets || (has_idr && ctx->need_inject_params)) &&
                           ctx->sps_size > 0U &&
                           ctx->pps_size > 0U &&
                           !sample_self_contained;

    if (has_invalid_nal)
    {
        return AVDK_ERR_INVAL;
    }

    const uint32_t reserve = src_len + (src_len / 4U) + ctx->sps_size + ctx->pps_size + 32U;
    avdk_err_t ret = hw_h264_fzc_ensure_annexb_buf(ctx, reserve);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    uint32_t out = 0;
    if (do_inject)
    {
        ret = hw_h264_fzc_append_nalu(ctx, &out, ctx->sps_data, ctx->sps_size);
        if (ret == AVDK_ERR_OK)
        {
            ret = hw_h264_fzc_append_nalu(ctx, &out, ctx->pps_data, ctx->pps_size);
        }
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }

    uint32_t in = 0;
    while (in + length_size <= src_len)
    {
        uint32_t nalu_len = 0;
        for (uint8_t k = 0; k < length_size; k++)
        {
            nalu_len = (nalu_len << 8) | src[in + k];
        }
        in += length_size;

        if (nalu_len == 0U || nalu_len > (src_len - in))
        {
            return AVDK_ERR_INVAL;
        }
        if (!hw_h264_frame_is_valid_nal_type(H264_NALU_HDR_TYPE(src[in])))
        {
            return AVDK_ERR_INVAL;
        }

        ret = hw_h264_fzc_append_nalu(ctx, &out, &src[in], nalu_len);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
        in += nalu_len;
    }

    *out_len = out;
    if (has_idr && (do_inject || sample_self_contained))
    {
        ctx->need_inject_params = false;
    }
    return AVDK_ERR_OK;
}

/* Detect whether an Annex-B access unit contains an IDR slice NAL. */
static bool hw_h264_fzc_annexb_contains_idr(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len < 5U)
    {
        return false;
    }
    for (uint32_t i = 0; i + 3U < len; i++)
    {
        if (data[i] == 0x00U && data[i + 1U] == 0x00U)
        {
            uint32_t prefix = 0U;
            if (data[i + 2U] == 0x01U)
            {
                prefix = 3U;
            }
            else if (i + 3U < len && data[i + 2U] == 0x00U && data[i + 3U] == 0x01U)
            {
                prefix = 4U;
            }
            if (prefix > 0U && i + prefix < len &&
                H264_NALU_HDR_TYPE(data[i + prefix]) == H264_NALU_TYPE_IDR)
            {
                return true;
            }
        }
    }
    return false;
}
/* ----------------------------------------------------------------------------
 * Pending-PTS map: display order == POC order == ascending PTS, so we keep the
 * fed-but-not-displayed PTS values sorted ascending and pop the minimum for
 * each dequeued (display-order) frame.
 * -------------------------------------------------------------------------- */

static void hw_h264_fzc_pts_clear(hw_h264_fzc_ctx_t *ctx)
{
    ctx->pending_pts_count = 0;
}

static void hw_h264_fzc_pts_push(hw_h264_fzc_ctx_t *ctx, uint64_t pts)
{
    uint32_t i;

    if (ctx->pending_pts_count >= VP_H264_FZC_PENDING_PTS_CAP)
    {
        /* Should not happen for sane reorder depths; drop the oldest (front). */
        for (i = 1; i < ctx->pending_pts_count; i++)
        {
            ctx->pending_pts[i - 1] = ctx->pending_pts[i];
        }
        ctx->pending_pts_count--;
    }

    i = ctx->pending_pts_count;
    while (i > 0U && ctx->pending_pts[i - 1U] > pts)
    {
        ctx->pending_pts[i] = ctx->pending_pts[i - 1U];
        i--;
    }
    ctx->pending_pts[i] = pts;
    ctx->pending_pts_count++;
}

static bool hw_h264_fzc_pts_pop_min(hw_h264_fzc_ctx_t *ctx, uint64_t *out_pts)
{
    uint32_t i;

    if (ctx->pending_pts_count == 0U)
    {
        return false;
    }
    *out_pts = ctx->pending_pts[0];
    for (i = 1; i < ctx->pending_pts_count; i++)
    {
        ctx->pending_pts[i - 1] = ctx->pending_pts[i];
    }
    ctx->pending_pts_count--;
    return true;
}

/* Copy + compact one pool frame (coded layout, 16-aligned pitch) into a visible
 * NV12 buffer (stride == visible width), matching the legacy frame decoder's
 * output layout. */
static void hw_h264_fzc_copy_visible_nv12(uint8_t *dst,
                                          const uint8_t *src,
                                          uint32_t visible_w,
                                          uint32_t visible_h,
                                          uint32_t coded_w,
                                          uint32_t coded_h)
{
    if (coded_w == visible_w && coded_h == visible_h)
    {
        os_memcpy(dst, src, (visible_w * visible_h * 3U) / 2U);
        return;
    }

    for (uint32_t y = 0; y < visible_h; y++)
    {
        os_memcpy(dst + y * visible_w, src + y * coded_w, visible_w);
    }

    uint8_t *dst_uv = dst + visible_w * visible_h;
    const uint8_t *src_uv = src + coded_w * coded_h;
    for (uint32_t y = 0; y < (visible_h / 2U); y++)
    {
        os_memcpy(dst_uv + y * visible_w, src_uv + y * coded_w, visible_w);
    }
}

/* Discard every currently displayable frame from the controller, releasing the
 * pool slot and keeping the pending-PTS map aligned. Used at GOP boundaries and
 * teardown so the zero-copy pool is fully returned; no heap copy is made. */
static void hw_h264_fzc_discard_pending(hw_h264_fzc_ctx_t *ctx, uint32_t first_timeout_ms)
{
    uint32_t timeout_ms = first_timeout_ms;

    if (ctx->hw_decoder_handle == NULL)
    {
        return;
    }

    for (;;)
    {
        bk_h264_decode_dequeue_t dq;
        os_memset(&dq, 0, sizeof(dq));
        dq.timeout_ms = timeout_ms;
        timeout_ms = 0U; /* only the first dequeue may block */

        if (bk_h264_decode_ioctl(ctx->hw_decoder_handle, BK_H264_DECODE_IOCTL_DEQUEUE, &dq) != AVDK_ERR_OK)
        {
            break; /* timeout / empty */
        }

        (void)bk_h264_decode_ioctl(ctx->hw_decoder_handle, BK_H264_DECODE_IOCTL_RELEASE, &dq.frame);

        uint64_t drop_pts = 0;
        (void)hw_h264_fzc_pts_pop_min(ctx, &drop_pts);
    }
}

static void hw_h264_fzc_flush_controller(hw_h264_fzc_ctx_t *ctx, uint32_t drain_timeout_ms)
{
    if (ctx->hw_decoder_handle == NULL)
    {
        return;
    }
    if (bk_h264_decode_ioctl(ctx->hw_decoder_handle, BK_H264_DECODE_IOCTL_FLUSH, NULL) != AVDK_ERR_OK)
    {
        LOGW("%s: flush ioctl failed\n", __func__);
    }
    hw_h264_fzc_discard_pending(ctx, drain_timeout_ms);
}

static void hw_h264_frame_done_cb(int status, void *args)
{
    (void)args;
    if (status != BK_OK)
    {
        LOGW("%s: frame done status=%d\n", __func__, status);
    }
}

static avdk_err_t hw_h264_fzc_reset_controller(hw_h264_fzc_ctx_t *ctx,
                                               uint16_t width,
                                               uint16_t height)
{
    if (ctx->hw_decoder_handle != NULL)
    {
        (void)bk_h264_decode_close(ctx->hw_decoder_handle);
        (void)bk_h264_decode_deinit(ctx->hw_decoder_handle);
        (void)bk_h264_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
    }

    bk_h264_decode_frame_zerocopy_config_t cfg = DEFAULT_H264_DECODE_FRAME_ZEROCOPY_CONFIG;
    cfg.timeout_ms = 1000U;
    cfg.out_width = (uint16_t)H264_ALIGN_UP(width, 16U);
    cfg.out_height = (uint16_t)H264_ALIGN_UP(height, 16U);
    cfg.out_format = BK_PIXEL_FORMAT_NV12;
    cfg.disp_depth = (uint16_t)VP_H264_FZC_DISP_DEPTH;
    cfg.frame_done_cb = hw_h264_frame_done_cb;
    cfg.frame_done_args = NULL;

    avdk_err_t ret = bk_h264_decode_frame_zerocopy_ctlr_new(&ctx->hw_decoder_handle, &cfg);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    ret = bk_h264_decode_init(ctx->hw_decoder_handle);
    if (ret != AVDK_ERR_OK)
    {
        (void)bk_h264_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
        return ret;
    }

    ret = bk_h264_decode_open(ctx->hw_decoder_handle);
    if (ret != AVDK_ERR_OK)
    {
        (void)bk_h264_decode_deinit(ctx->hw_decoder_handle);
        (void)bk_h264_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
        return ret;
    }

    ctx->need_inject_params = true;
    ctx->seen_first_frame = false;
    hw_h264_fzc_pts_clear(ctx);
    LOGI("%s: controller ready, out=%ux%u disp_depth=%u\n",
         __func__, (unsigned)cfg.out_width, (unsigned)cfg.out_height,
         (unsigned)cfg.disp_depth);
    return AVDK_ERR_OK;
}

/* ----------------------------------------------------------------------------
 * video_player_video_decoder_ops_t implementation.
 * -------------------------------------------------------------------------- */

static avdk_err_t hw_h264_fzc_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
                                                    const video_player_video_format_t **formats,
                                                    uint32_t *format_count)
{
    (void)ops;
    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_video_format_t s_formats[] = {
        VIDEO_PLAYER_VIDEO_FORMAT_H264,
    };
    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_fzc_init(struct video_player_video_decoder_ops_s *ops,
                                   video_player_video_params_t *params)
{
    hw_h264_fzc_instance_t *self = __containerof(ops, hw_h264_fzc_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");

    hw_h264_fzc_ctx_t *ctx = &self->ctx;
    if (params->format != VIDEO_PLAYER_VIDEO_FORMAT_H264)
    {
        return AVDK_ERR_UNSUPPORTED;
    }
    if (params->width == 0U || params->height == 0U ||
        (params->width & 1U) || (params->height & 1U))
    {
        return AVDK_ERR_INVAL;
    }

    if (ctx->is_initialized)
    {
        hw_h264_fzc_deinit(ops);
    }

    os_memcpy(&ctx->video_params, params, sizeof(video_player_video_params_t));
    ctx->video_params.codec_config = NULL;
    ctx->video_params.codec_config_size = 0;
    ctx->nalu_length_size = 4U;
    ctx->need_inject_params = true;
    ctx->seen_first_frame = false;
    ctx->input_is_avcc = false;
    hw_h264_fzc_pts_clear(ctx);

    if (params->codec_config != NULL && params->codec_config_size > 0U)
    {
        avdk_err_t pret = hw_h264_fzc_parse_avcc(ctx, params->codec_config, params->codec_config_size);
        if (pret != AVDK_ERR_OK)
        {
            LOGW("%s: parse avcC failed, ret=%d, continue without cached SPS/PPS\n", __func__, pret);
            hw_h264_fzc_release_param_sets(ctx);
            ctx->nalu_length_size = 4U;
        }
        else
        {
            ctx->input_is_avcc = true;
        }
    }

    avdk_err_t ret = hw_h264_fzc_reset_controller(ctx,
                                                  (uint16_t)params->width,
                                                  (uint16_t)params->height);
    if (ret != AVDK_ERR_OK)
    {
        hw_h264_fzc_release_param_sets(ctx);
        return ret;
    }

    ctx->is_initialized = true;
    LOGI("%s: frame-zerocopy decoder initialized %ux%u length_size=%u disp_depth=%u\n",
         __func__, params->width, params->height, ctx->nalu_length_size,
         (unsigned)VP_H264_FZC_DISP_DEPTH);
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_fzc_deinit(struct video_player_video_decoder_ops_s *ops)
{
    hw_h264_fzc_instance_t *self = __containerof(ops, hw_h264_fzc_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");

    hw_h264_fzc_ctx_t *ctx = &self->ctx;

    /* Drop trailing reordered pictures so the pool is fully released. */
    if (ctx->hw_decoder_handle != NULL)
    {
        hw_h264_fzc_flush_controller(ctx, VP_H264_FZC_FLUSH_TMO_MS);
    }
    hw_h264_fzc_pts_clear(ctx);

    if (ctx->hw_decoder_handle != NULL)
    {
        (void)bk_h264_decode_close(ctx->hw_decoder_handle);
        (void)bk_h264_decode_deinit(ctx->hw_decoder_handle);
        (void)bk_h264_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
    }

    if (ctx->annexb_buf != NULL)
    {
        bk_frame_buffer_free(ctx->annexb_buf);
        ctx->annexb_buf = NULL;
        ctx->annexb_buf_size = 0;
    }

    hw_h264_fzc_release_param_sets(ctx);
    ctx->is_initialized = false;
    ctx->need_inject_params = true;
    ctx->seen_first_frame = false;
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_fzc_decode(struct video_player_video_decoder_ops_s *ops,
                                     video_player_buffer_t *in_buffer,
                                     video_player_buffer_t *out_buffer,
                                     pixel_format_t out_fmt)
{
    hw_h264_fzc_instance_t *self = __containerof(ops, hw_h264_fzc_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer && in_buffer->data, AVDK_ERR_INVAL, TAG, "invalid input buffer");
    AVDK_RETURN_ON_FALSE(out_buffer && out_buffer->data, AVDK_ERR_INVAL, TAG, "invalid output buffer");

    hw_h264_fzc_ctx_t *ctx = &self->ctx;
    AVDK_RETURN_ON_FALSE(ctx->hw_decoder_handle, AVDK_ERR_GENERIC, TAG, "decoder not initialized");
    AVDK_RETURN_ON_FALSE(ctx->is_initialized, AVDK_ERR_GENERIC, TAG, "decoder not initialized");

    pixel_format_t requested_fmt = out_fmt;
    if (requested_fmt == PIXEL_FMT_UNKNOW || requested_fmt == 0)
    {
        requested_fmt = PIXEL_FMT_NV12;
    }
    if (requested_fmt != PIXEL_FMT_NV12 && requested_fmt != PIXEL_FMT_YUV420SP)
    {
        LOGE("%s: frame-zerocopy decoder only supports NV12 output, fmt=%d\n", __func__, requested_fmt);
        out_buffer->length = 0;
        return AVDK_ERR_UNSUPPORTED;
    }

    uint8_t *bs_data = in_buffer->data;
    uint32_t bs_len = in_buffer->length;
    /* AVCC containers carry length-prefixed access units; never auto-detect
     * Annex-B for them (a 0x000001xx length prefix aliases a start code and
     * would pass the raw packet buffer to the hardware). Only probe when the
     * container gave no avcC config and the stream is genuinely elementary. */
    const bool is_annexb_input = !ctx->input_is_avcc &&
                                 hw_h264_fzc_buffer_is_annex_b(in_buffer->data, in_buffer->length);
    const bool need_inject_before = ctx->need_inject_params;
    avdk_err_t ret = AVDK_ERR_OK;

    if (!is_annexb_input)
    {
        uint32_t cvt_len = 0;
        ret = hw_h264_fzc_avcc_to_annexb(ctx,
                                         in_buffer->data,
                                         in_buffer->length,
                                         ctx->need_inject_params,
                                         &cvt_len);
        if (ret != AVDK_ERR_OK)
        {
            out_buffer->length = 0;
            return ret;
        }
        bs_data = ctx->annexb_buf;
        bs_len = cvt_len;
    }
    else if (ctx->need_inject_params && ctx->sps_size > 0U && ctx->pps_size > 0U)
    {
        const uint32_t need = in_buffer->length + ctx->sps_size + ctx->pps_size + 16U;
        ret = hw_h264_fzc_ensure_annexb_buf(ctx, need);
        if (ret != AVDK_ERR_OK)
        {
            out_buffer->length = 0;
            return ret;
        }

        uint32_t out = 0;
        ret = hw_h264_fzc_append_nalu(ctx, &out, ctx->sps_data, ctx->sps_size);
        if (ret == AVDK_ERR_OK)
        {
            ret = hw_h264_fzc_append_nalu(ctx, &out, ctx->pps_data, ctx->pps_size);
        }
        if (ret != AVDK_ERR_OK || out + in_buffer->length > ctx->annexb_buf_size)
        {
            out_buffer->length = 0;
            return (ret != AVDK_ERR_OK) ? ret : AVDK_ERR_NOMEM;
        }
        os_memcpy(&ctx->annexb_buf[out], in_buffer->data, in_buffer->length);
        bs_data = ctx->annexb_buf;
        bs_len = out + in_buffer->length;
        ctx->need_inject_params = false;
    }

    /* GOP boundary: flush the previous GOP's trailing reordered pictures so the
     * pending-PTS map and display ordering restart cleanly at each IDR. */
    if (ctx->seen_first_frame && hw_h264_fzc_annexb_contains_idr(bs_data, bs_len))
    {
        hw_h264_fzc_flush_controller(ctx, VP_H264_FZC_FLUSH_TMO_MS);
        hw_h264_fzc_pts_clear(ctx);
    }

    bk_h264_decode_input_t in = {0};
    in.stream = bs_data;
    in.stream_len = bs_len;
    in.out_buffer = NULL;       /* zero-copy pool path supplies the decode target */
    in.out_buffer_size = 0;

    ret = bk_h264_decode_frame(ctx->hw_decoder_handle, &in);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_h264_decode_frame failed, ret=%d, bs_len=%u, pts=%llu, need_inj_before=%u\n",
             __func__, ret, bs_len, (unsigned long long)in_buffer->pts,
             (unsigned)(need_inject_before ? 1U : 0U));
        out_buffer->length = 0;
        out_buffer->pts = in_buffer->pts;
        ctx->need_inject_params = true;
        return ret;
    }

    ctx->seen_first_frame = true;
    hw_h264_fzc_pts_push(ctx, in_buffer->pts);

    /* Pull the next display-order frame. With disp_depth=1 the controller
     * publishes one frame at a time, so a single dequeue yields the frame to
     * show now (B-frame reordering is resolved inside the DPB). The decoded
     * NV12 is copy-compacted straight into the engine's pre-allocated output
     * buffer - no decoder-owned frame allocation - then the pool slot is
     * released immediately so the next decode has a free slot. */
    const uint32_t visible_w = ctx->video_params.width;
    const uint32_t visible_h = ctx->video_params.height;
    const uint32_t visible_size = (visible_w * visible_h * 3U) / 2U;

    bk_h264_decode_dequeue_t dq;
    os_memset(&dq, 0, sizeof(dq));
    dq.timeout_ms = VP_H264_FZC_DEQUEUE_TMO_MS;

    if (bk_h264_decode_ioctl(ctx->hw_decoder_handle, BK_H264_DECODE_IOCTL_DEQUEUE, &dq) != AVDK_ERR_OK)
    {
        /* No displayable frame yet (reorder fill). Engine skips this AU. */
        out_buffer->length = 0;
        return AVDK_ERR_TIMEOUT;
    }

    /* The engine pre-allocates out_buffer->data sized for the configured NV12
     * frame (width*height*3/2 == visible_size), so the compacted copy fits. */
    hw_h264_fzc_copy_visible_nv12(out_buffer->data, dq.frame.data, visible_w, visible_h,
                                  dq.frame.width, dq.frame.height);

    /* Return the pool slot right away so the pool never starves. */
    (void)bk_h264_decode_ioctl(ctx->hw_decoder_handle, BK_H264_DECODE_IOCTL_RELEASE, &dq.frame);

    out_buffer->length = visible_size;
    if (!hw_h264_fzc_pts_pop_min(ctx, &out_buffer->pts))
    {
        out_buffer->pts = in_buffer->pts;
    }
    return AVDK_ERR_OK;
}

static video_player_video_decoder_ops_t *hw_h264_fzc_create(void)
{
    hw_h264_fzc_instance_t *inst = os_malloc(sizeof(hw_h264_fzc_instance_t));
    if (inst == NULL)
    {
        return NULL;
    }

    os_memset(inst, 0, sizeof(*inst));
    os_memcpy(&inst->ops, &s_ops_template, sizeof(video_player_video_decoder_ops_t));
    return &inst->ops;
}

static void hw_h264_fzc_destroy(video_player_video_decoder_ops_t *ops)
{
    if (ops == NULL || ops == &s_ops_template)
    {
        return;
    }

    hw_h264_fzc_instance_t *inst = __containerof(ops, hw_h264_fzc_instance_t, ops);
    os_free(inst);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create = hw_h264_fzc_create,
    .destroy = hw_h264_fzc_destroy,
    .get_supported_formats = hw_h264_fzc_get_supported_formats,
    .init = hw_h264_fzc_init,
    .deinit = hw_h264_fzc_deinit,
    .decode = hw_h264_fzc_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_hw_h264_decoder_frame_zerocopy_ops(void)
{
    return &s_ops_template;
}
