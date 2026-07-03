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

#define TAG "vp_h264_frame_dec"

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

static bool hw_h264_frame_is_valid_nal_type(uint8_t type)
{
    /*
     * H.264 NAL type 0 is unspecified and 24..31 are not valid AVC stream NALs
     * (they are used by RTP packetization modes). Feeding these malformed AUs
     * to the hardware decoder corrupts the coded-stream slab on this platform.
     */
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
} hw_h264_decoder_frame_ctx_t;

typedef struct
{
    video_player_video_decoder_ops_t ops;
    hw_h264_decoder_frame_ctx_t      ctx;
} hw_h264_decoder_frame_instance_t;

static video_player_video_decoder_ops_t s_ops_template;

static avdk_err_t hw_h264_decoder_frame_deinit(struct video_player_video_decoder_ops_s *ops);

static void hw_h264_frame_release_param_sets(hw_h264_decoder_frame_ctx_t *ctx)
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

static avdk_err_t hw_h264_frame_parse_avcc(hw_h264_decoder_frame_ctx_t *ctx,
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

static bool hw_h264_frame_buffer_is_annex_b(const uint8_t *buf, uint32_t len)
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

/* Validate AVCC by walking the length-prefixed NALU chain. This must be
 * preferred over Annex-B start-code scanning because a 4-byte AVCC length in
 * 0x00000100..0x000001FF has the same bytes as a 3-byte Annex-B start code. */
static bool hw_h264_frame_buffer_is_avcc(const uint8_t *buf, uint32_t len, uint8_t length_size)
{
    if (buf == NULL || (length_size != 1U && length_size != 2U && length_size != 4U))
    {
        return false;
    }

    uint32_t i = 0U;
    uint32_t nalu_count = 0U;
    while (i + length_size <= len)
    {
        uint32_t nalu_len = 0U;
        for (uint8_t k = 0U; k < length_size; k++)
        {
            nalu_len = (nalu_len << 8) | (uint32_t)buf[i + k];
        }
        i += length_size;
        if (nalu_len == 0U || nalu_len > (len - i))
        {
            return false;
        }
        i += nalu_len;
        nalu_count++;
    }

    return (i == len) && (nalu_count > 0U);
}

static avdk_err_t hw_h264_frame_ensure_annexb_buf(hw_h264_decoder_frame_ctx_t *ctx, uint32_t need)
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

static avdk_err_t hw_h264_frame_append_nalu(hw_h264_decoder_frame_ctx_t *ctx,
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

static avdk_err_t hw_h264_frame_avcc_to_annexb(hw_h264_decoder_frame_ctx_t *ctx,
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
    avdk_err_t ret = hw_h264_frame_ensure_annexb_buf(ctx, reserve);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    uint32_t out = 0;
    if (do_inject)
    {
        ret = hw_h264_frame_append_nalu(ctx, &out, ctx->sps_data, ctx->sps_size);
        if (ret == AVDK_ERR_OK)
        {
            ret = hw_h264_frame_append_nalu(ctx, &out, ctx->pps_data, ctx->pps_size);
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

        ret = hw_h264_frame_append_nalu(ctx, &out, &src[in], nalu_len);
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

static void hw_h264_frame_done_cb(int status, void *args)
{
    (void)args;
    if (status != BK_OK)
    {
        LOGW("%s: frame done status=%d\n", __func__, status);
    }
}

static void hw_h264_frame_pack_nv12_visible(uint8_t *buf,
                                            uint32_t visible_w,
                                            uint32_t visible_h,
                                            uint32_t coded_w,
                                            uint32_t coded_h)
{
    if (buf == NULL || visible_w == 0U || visible_h == 0U ||
        coded_w < visible_w || coded_h < visible_h)
    {
        return;
    }

    if (coded_w == visible_w && coded_h == visible_h)
    {
        return;
    }

    for (uint32_t y = 0; y < visible_h; y++)
    {
        os_memmove(buf + y * visible_w,
                   buf + y * coded_w,
                   visible_w);
    }

    uint8_t *dst_uv = buf + visible_w * visible_h;
    uint8_t *src_uv = buf + coded_w * coded_h;
    for (uint32_t y = 0; y < (visible_h / 2U); y++)
    {
        os_memmove(dst_uv + y * visible_w,
                   src_uv + y * coded_w,
                   visible_w);
    }
}

static avdk_err_t hw_h264_frame_reset_controller(hw_h264_decoder_frame_ctx_t *ctx,
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

    bk_h264_decode_frame_config_t cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;
    cfg.timeout_ms = 1000U;
    cfg.out_width = (uint16_t)H264_ALIGN_UP(width, 16U);
    cfg.out_height = (uint16_t)H264_ALIGN_UP(height, 16U);
    cfg.out_format = BK_PIXEL_FORMAT_NV12;
    cfg.frame_done_cb = hw_h264_frame_done_cb;
    cfg.frame_done_args = NULL;

    avdk_err_t ret = bk_h264_decode_frame_ctlr_new(&ctx->hw_decoder_handle, &cfg);
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
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_decoder_frame_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
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

static avdk_err_t hw_h264_decoder_frame_init(struct video_player_video_decoder_ops_s *ops,
                                             video_player_video_params_t *params)
{
    hw_h264_decoder_frame_instance_t *self = __containerof(ops, hw_h264_decoder_frame_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");

    hw_h264_decoder_frame_ctx_t *ctx = &self->ctx;
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
        hw_h264_decoder_frame_deinit(ops);
    }

    os_memcpy(&ctx->video_params, params, sizeof(video_player_video_params_t));
    ctx->video_params.codec_config = NULL;
    ctx->video_params.codec_config_size = 0;
    ctx->nalu_length_size = 4U;
    ctx->need_inject_params = true;

    if (params->codec_config != NULL && params->codec_config_size > 0U)
    {
        avdk_err_t pret = hw_h264_frame_parse_avcc(ctx, params->codec_config, params->codec_config_size);
        if (pret != AVDK_ERR_OK)
        {
            LOGW("%s: parse avcC failed, ret=%d, continue without cached SPS/PPS\n", __func__, pret);
            hw_h264_frame_release_param_sets(ctx);
            ctx->nalu_length_size = 4U;
        }
    }

    avdk_err_t ret = hw_h264_frame_reset_controller(ctx,
                                                    (uint16_t)params->width,
                                                    (uint16_t)params->height);
    if (ret != AVDK_ERR_OK)
    {
        hw_h264_frame_release_param_sets(ctx);
        return ret;
    }

    ctx->is_initialized = true;
    LOGI("%s: frame decoder initialized %ux%u length_size=%u\n",
         __func__,
         params->width,
         params->height,
         ctx->nalu_length_size);
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_decoder_frame_deinit(struct video_player_video_decoder_ops_s *ops)
{
    hw_h264_decoder_frame_instance_t *self = __containerof(ops, hw_h264_decoder_frame_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");

    hw_h264_decoder_frame_ctx_t *ctx = &self->ctx;
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

    hw_h264_frame_release_param_sets(ctx);
    ctx->is_initialized = false;
    ctx->need_inject_params = true;
    return AVDK_ERR_OK;
}

static avdk_err_t hw_h264_decoder_frame_decode(struct video_player_video_decoder_ops_s *ops,
                                               video_player_buffer_t *in_buffer,
                                               video_player_buffer_t *out_buffer,
                                               pixel_format_t out_fmt)
{
    hw_h264_decoder_frame_instance_t *self = __containerof(ops, hw_h264_decoder_frame_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer && in_buffer->data, AVDK_ERR_INVAL, TAG, "invalid input buffer");
    AVDK_RETURN_ON_FALSE(out_buffer && out_buffer->data, AVDK_ERR_INVAL, TAG, "invalid output buffer");

    hw_h264_decoder_frame_ctx_t *ctx = &self->ctx;
    AVDK_RETURN_ON_FALSE(ctx->hw_decoder_handle, AVDK_ERR_GENERIC, TAG, "decoder not initialized");
    AVDK_RETURN_ON_FALSE(ctx->is_initialized, AVDK_ERR_GENERIC, TAG, "decoder not initialized");

    pixel_format_t requested_fmt = out_fmt;
    if (requested_fmt == PIXEL_FMT_UNKNOW || requested_fmt == 0)
    {
        requested_fmt = PIXEL_FMT_NV12;
    }
    if (requested_fmt != PIXEL_FMT_NV12 && requested_fmt != PIXEL_FMT_YUV420SP)
    {
        LOGE("%s: frame decoder only supports NV12 output, fmt=%d\n", __func__, requested_fmt);
        out_buffer->length = 0;
        return AVDK_ERR_UNSUPPORTED;
    }

    const uint32_t width = ctx->video_params.width;
    const uint32_t height = ctx->video_params.height;
    const uint32_t coded_width = H264_ALIGN_UP(width, 16U);
    const uint32_t coded_height = H264_ALIGN_UP(height, 16U);
    const uint32_t visible_out_size = (width * height * 3U) / 2U;
    const uint32_t coded_out_size = (coded_width * coded_height * 3U) / 2U;
    if (coded_out_size > out_buffer->length)
    {
        LOGE("%s: output too small, need=%u got=%u\n",
             __func__, coded_out_size, out_buffer->length);
        out_buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    uint8_t *bs_data = in_buffer->data;
    uint32_t bs_len = in_buffer->length;
    const bool is_avcc_input = hw_h264_frame_buffer_is_avcc(in_buffer->data,
                                                            in_buffer->length,
                                                            ctx->nalu_length_size);
    const bool is_annexb_input = !is_avcc_input &&
                                 hw_h264_frame_buffer_is_annex_b(in_buffer->data,
                                                                 in_buffer->length);
    const bool need_inject_before = ctx->need_inject_params;
    avdk_err_t ret = AVDK_ERR_OK;

    if (!is_annexb_input)
    {
        uint32_t cvt_len = 0;
        ret = hw_h264_frame_avcc_to_annexb(ctx,
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
        ret = hw_h264_frame_ensure_annexb_buf(ctx, need);
        if (ret != AVDK_ERR_OK)
        {
            out_buffer->length = 0;
            return ret;
        }

        uint32_t out = 0;
        ret = hw_h264_frame_append_nalu(ctx, &out, ctx->sps_data, ctx->sps_size);
        if (ret == AVDK_ERR_OK)
        {
            ret = hw_h264_frame_append_nalu(ctx, &out, ctx->pps_data, ctx->pps_size);
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

    bk_h264_decode_input_t in = {0};
    in.stream = bs_data;
    in.stream_len = bs_len;
    in.out_buffer = out_buffer->data;
    in.out_buffer_size = coded_out_size;

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

    hw_h264_frame_pack_nv12_visible(out_buffer->data,
                                    width,
                                    height,
                                    coded_width,
                                    coded_height);

    out_buffer->length = visible_out_size;
    out_buffer->pts = in_buffer->pts;
    return AVDK_ERR_OK;
}

static video_player_video_decoder_ops_t *hw_h264_decoder_frame_create(void)
{
    hw_h264_decoder_frame_instance_t *inst = os_malloc(sizeof(hw_h264_decoder_frame_instance_t));
    if (inst == NULL)
    {
        return NULL;
    }

    os_memset(inst, 0, sizeof(*inst));
    os_memcpy(&inst->ops, &s_ops_template, sizeof(video_player_video_decoder_ops_t));
    return &inst->ops;
}

static void hw_h264_decoder_frame_destroy(video_player_video_decoder_ops_t *ops)
{
    if (ops == NULL || ops == &s_ops_template)
    {
        return;
    }

    hw_h264_decoder_frame_instance_t *inst = __containerof(ops, hw_h264_decoder_frame_instance_t, ops);
    os_free(inst);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create = hw_h264_decoder_frame_create,
    .destroy = hw_h264_decoder_frame_destroy,
    .get_supported_formats = hw_h264_decoder_frame_get_supported_formats,
    .init = hw_h264_decoder_frame_init,
    .deinit = hw_h264_decoder_frame_deinit,
    .decode = hw_h264_decoder_frame_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_hw_h264_decoder_frame_ops(void)
{
    return &s_ops_template;
}
