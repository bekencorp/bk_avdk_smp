#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "components/bk_frame_buffer.h"
#include "components/media_types.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"

#define TAG "vp_jpeg_dec"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Hardware JPEG decoder private context
typedef struct hw_jpeg_decoder_ctx_s
{
    bk_jpeg_decode_ctlr_handle_t hw_decoder_handle; // bk_decoder JPEG controller handle
    video_player_video_params_t video_params;       // Video parameters (width, height, fps, format)
    bool is_initialized;                            // Initialization flag
    uint8_t *tmp_nv12;                              // Temporary NV12 buffer for bk_decoder output
    uint32_t tmp_nv12_size;                         // Temporary NV12 buffer size
} hw_jpeg_decoder_ctx_t;

static video_player_video_decoder_ops_t s_ops_template;

// Allocate ops + context in a single block to avoid double os_malloc.
// Keep ops as the first member so we can pass ops as the instance pointer.
typedef struct
{
    video_player_video_decoder_ops_t ops;
    hw_jpeg_decoder_ctx_t ctx;
} hw_jpeg_decoder_instance_t;

static avdk_err_t hw_jpeg_decoder_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
                                                        const video_player_video_format_t **formats,
                                                        uint32_t *format_count)
{
    (void)ops;

    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_video_format_t s_formats[] = {
        VIDEO_PLAYER_VIDEO_FORMAT_MJPEG,
    };

    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

// Forward declaration
static avdk_err_t hw_jpeg_decoder_deinit(struct video_player_video_decoder_ops_s *ops);

static bool hw_jpeg_pixel_bytes(pixel_format_t fmt, uint32_t *out_bpp)
{
    if (out_bpp == NULL)
    {
        return false;
    }

    switch (fmt)
    {
        case PIXEL_FMT_NV12:
        case PIXEL_FMT_YUV420SP:
            *out_bpp = 0;
            return true;
        case PIXEL_FMT_RGB888:
            *out_bpp = 3;
            return true;
        case PIXEL_FMT_RGB565:
        case PIXEL_FMT_YUYV:
            *out_bpp = 2;
            return true;
        default:
            return false;
    }
}

static inline uint8_t hw_jpeg_clip_u8(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 255)
    {
        return 255;
    }

    return (uint8_t)value;
}

static uint32_t hw_jpeg_calc_output_size(uint32_t width, uint32_t height, pixel_format_t fmt)
{
    switch (fmt)
    {
        case PIXEL_FMT_NV12:
        case PIXEL_FMT_YUV420SP:
            return (width * height * 3U) / 2U;
        case PIXEL_FMT_RGB888:
            return width * height * 3U;
        case PIXEL_FMT_RGB565:
        case PIXEL_FMT_YUYV:
            return width * height * 2U;
        default:
            return 0;
    }
}

static bool hw_jpeg_needs_direct_nv12(pixel_format_t fmt)
{
    return (fmt == PIXEL_FMT_NV12 || fmt == PIXEL_FMT_YUV420SP);
}

static void hw_jpeg_decode_frame_done_cb(int status, void *args)
{
    (void)args;

    if (status != BK_OK)
    {
        LOGW("%s: bk_decoder frame callback status=%d\n", __func__, status);
    }
}

static avdk_err_t hw_jpeg_decoder_reset_controller(hw_jpeg_decoder_ctx_t *ctx, uint16_t width, uint16_t height)
{
    AVDK_RETURN_ON_FALSE(ctx, AVDK_ERR_INVAL, TAG, "ctx is NULL");

    if (ctx->hw_decoder_handle != NULL)
    {
        (void)bk_jpeg_decode_close(ctx->hw_decoder_handle);
        (void)bk_jpeg_decode_deinit(ctx->hw_decoder_handle);
        (void)bk_jpeg_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
    }

    bk_jpeg_decode_frame_config_t config = DEFAULT_JPEG_DECODE_FRAME_CONFIG;
    config.timeout_ms = 1000;
    config.out_width = width;
    config.out_height = height;
    config.out_format = BK_PIXEL_FORMAT_NV12;
    config.frame_done_cb = hw_jpeg_decode_frame_done_cb;
    config.frame_done_args = NULL;

    avdk_err_t ret = bk_jpeg_decode_frame_ctlr_new(&ctx->hw_decoder_handle, &config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_jpeg_decode_frame_ctlr_new failed, ret=%d\n", __func__, ret);
        return ret;
    }

    ret = bk_jpeg_decode_init(ctx->hw_decoder_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_jpeg_decode_init failed, ret=%d\n", __func__, ret);
        (void)bk_jpeg_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
        return ret;
    }

    ret = bk_jpeg_decode_open(ctx->hw_decoder_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_jpeg_decode_open failed, ret=%d\n", __func__, ret);
        (void)bk_jpeg_decode_deinit(ctx->hw_decoder_handle);
        (void)bk_jpeg_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
        return ret;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t hw_jpeg_decoder_ensure_tmp_nv12(hw_jpeg_decoder_ctx_t *ctx, uint32_t size)
{
    AVDK_RETURN_ON_FALSE(ctx, AVDK_ERR_INVAL, TAG, "ctx is NULL");

    if (ctx->tmp_nv12 != NULL && ctx->tmp_nv12_size >= size)
    {
        return AVDK_ERR_OK;
    }

    if (ctx->tmp_nv12 != NULL)
    {
        bk_frame_buffer_free(ctx->tmp_nv12);
        ctx->tmp_nv12 = NULL;
        ctx->tmp_nv12_size = 0;
    }

    ctx->tmp_nv12 = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    if (ctx->tmp_nv12 == NULL)
    {
        LOGE("%s: allocate tmp nv12 buffer failed, size=%u\n", __func__, size);
        return AVDK_ERR_NOMEM;
    }

    ctx->tmp_nv12_size = size;
    return AVDK_ERR_OK;
}

static avdk_err_t hw_jpeg_nv12_to_yuyv(const uint8_t *src_nv12,
                                       uint32_t width,
                                       uint32_t height,
                                       uint8_t *dst_yuyv)
{
    AVDK_RETURN_ON_FALSE(src_nv12 && dst_yuyv, AVDK_ERR_INVAL, TAG, "invalid nv12/yuyv buffer");

    if (((width & 1U) != 0U) || ((height & 1U) != 0U))
    {
        LOGW("%s: NV12 to YUYV requires even width/height, got %ux%u\n", __func__, width, height);
        return AVDK_ERR_UNSUPPORTED;
    }

    const uint8_t *y_plane = src_nv12;
    const uint8_t *uv_plane = src_nv12 + width * height;

    for (uint32_t y = 0; y < height; y++)
    {
        const uint8_t *y_row = y_plane + y * width;
        const uint8_t *uv_row = uv_plane + (y >> 1) * width;
        uint8_t *dst_row = dst_yuyv + y * width * 2U;

        for (uint32_t x = 0; x < width; x += 2U)
        {
            const uint8_t u = uv_row[x];
            const uint8_t v = uv_row[x + 1U];

            dst_row[(x << 1)] = y_row[x];
            dst_row[(x << 1) + 1U] = u;
            dst_row[(x << 1) + 2U] = y_row[x + 1U];
            dst_row[(x << 1) + 3U] = v;
        }
    }

    return AVDK_ERR_OK;
}

static avdk_err_t hw_jpeg_nv12_to_rgb565(const uint8_t *src_nv12,
                                         uint32_t width,
                                         uint32_t height,
                                         uint8_t *dst_rgb565)
{
    AVDK_RETURN_ON_FALSE(src_nv12 && dst_rgb565, AVDK_ERR_INVAL, TAG, "invalid nv12/rgb565 buffer");

    if (((width & 1U) != 0U) || ((height & 1U) != 0U))
    {
        LOGW("%s: NV12 to RGB565 requires even width/height, got %ux%u\n", __func__, width, height);
        return AVDK_ERR_UNSUPPORTED;
    }

    const uint8_t *y_plane = src_nv12;
    const uint8_t *uv_plane = src_nv12 + width * height;
    uint16_t *dst = (uint16_t *)dst_rgb565;

    for (uint32_t y = 0; y < height; y++)
    {
        const uint8_t *y_row = y_plane + y * width;
        const uint8_t *uv_row = uv_plane + (y >> 1) * width;

        for (uint32_t x = 0; x < width; x++)
        {
            const uint32_t uv_idx = x & ~1U;
            const int u = (int)uv_row[uv_idx] - 128;
            const int v = (int)uv_row[uv_idx + 1U] - 128;
            int c = (int)y_row[x] - 16;
            if (c < 0)
            {
                c = 0;
            }

            const uint8_t r = hw_jpeg_clip_u8((298 * c + 409 * v + 128) >> 8);
            const uint8_t g = hw_jpeg_clip_u8((298 * c - 100 * u - 208 * v + 128) >> 8);
            const uint8_t b = hw_jpeg_clip_u8((298 * c + 516 * u + 128) >> 8);

            dst[y * width + x] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
                                            ((uint16_t)(g >> 2) << 5) |
                                            (uint16_t)(b >> 3));
        }
    }

    return AVDK_ERR_OK;
}

static avdk_err_t hw_jpeg_nv12_to_rgb888(const uint8_t *src_nv12,
                                         uint32_t width,
                                         uint32_t height,
                                         uint8_t *dst_rgb888)
{
    AVDK_RETURN_ON_FALSE(src_nv12 && dst_rgb888, AVDK_ERR_INVAL, TAG, "invalid nv12/rgb888 buffer");

    if (((width & 1U) != 0U) || ((height & 1U) != 0U))
    {
        LOGW("%s: NV12 to RGB888 requires even width/height, got %ux%u\n", __func__, width, height);
        return AVDK_ERR_UNSUPPORTED;
    }

    const uint8_t *y_plane = src_nv12;
    const uint8_t *uv_plane = src_nv12 + width * height;

    for (uint32_t y = 0; y < height; y++)
    {
        const uint8_t *y_row = y_plane + y * width;
        const uint8_t *uv_row = uv_plane + (y >> 1) * width;
        uint8_t *dst_row = dst_rgb888 + y * width * 3U;

        for (uint32_t x = 0; x < width; x++)
        {
            const uint32_t uv_idx = x & ~1U;
            const int u = (int)uv_row[uv_idx] - 128;
            const int v = (int)uv_row[uv_idx + 1U] - 128;
            int c = (int)y_row[x] - 16;
            if (c < 0)
            {
                c = 0;
            }

            dst_row[x * 3U] = hw_jpeg_clip_u8((298 * c + 409 * v + 128) >> 8);
            dst_row[x * 3U + 1U] = hw_jpeg_clip_u8((298 * c - 100 * u - 208 * v + 128) >> 8);
            dst_row[x * 3U + 2U] = hw_jpeg_clip_u8((298 * c + 516 * u + 128) >> 8);
        }
    }

    return AVDK_ERR_OK;
}

// Initialize hardware JPEG decoder
static avdk_err_t hw_jpeg_decoder_init(struct video_player_video_decoder_ops_s *ops, video_player_video_params_t *params)
{
    hw_jpeg_decoder_instance_t *hw_instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(hw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_jpeg_decoder_ctx_t *ctx = &hw_instance->ctx;
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");

    LOGI("%s: Initializing hardware JPEG decoder, width=%d, height=%d, format=%d, jpeg_subsampling=%u\n",
         __func__, params->width, params->height, params->format, params->jpeg_subsampling);

    // Hardware JPEG decoder only supports MJPEG format
    if (params->format != VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
    {
        LOGW("%s: HW JPEG decoder only supports MJPEG format (got format=%u), will fallback to SW decoder\n",
             __func__, params->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (params->width == 0 || params->height == 0)
    {
        LOGE("%s: invalid jpeg size %ux%u\n", __func__, params->width, params->height);
        return AVDK_ERR_INVAL;
    }

    // NV12 output requires even width/height. Return unsupported so upper layer can fallback.
    if (((params->width & 1U) != 0U) || ((params->height & 1U) != 0U))
    {
        LOGW("%s: bk_decoder path requires even width/height for NV12 output, got %ux%u\n",
             __func__, params->width, params->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    // Check if already initialized
    if (ctx->is_initialized)
    {
        LOGW("%s: Hardware JPEG decoder already initialized, deinitializing first\n", __func__);
        hw_jpeg_decoder_deinit(ops);
    }

    // Save video parameters
    os_memcpy(&ctx->video_params, params, sizeof(video_player_video_params_t));

    ctx->tmp_nv12 = NULL;
    ctx->tmp_nv12_size = 0;

    avdk_err_t ret = hw_jpeg_decoder_reset_controller(ctx, params->width, params->height);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to initialize bk_decoder controller, ret=%d\n", __func__, ret);
        return ret;
    }

    ctx->is_initialized = true;

    LOGI("%s: bk_decoder JPEG controller initialized successfully\n", __func__);

    return AVDK_ERR_OK;
}

// Deinitialize hardware JPEG decoder
static avdk_err_t hw_jpeg_decoder_deinit(struct video_player_video_decoder_ops_s *ops)
{
    hw_jpeg_decoder_instance_t *hw_instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(hw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_jpeg_decoder_ctx_t *ctx = &hw_instance->ctx;

    if (ctx->hw_decoder_handle != NULL)
    {
        LOGI("%s: Deinitializing bk_decoder JPEG controller\n", __func__);
        (void)bk_jpeg_decode_close(ctx->hw_decoder_handle);
        (void)bk_jpeg_decode_deinit(ctx->hw_decoder_handle);
        (void)bk_jpeg_decode_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
    }

    if (ctx->tmp_nv12 != NULL)
    {
        bk_frame_buffer_free(ctx->tmp_nv12);
        ctx->tmp_nv12 = NULL;
        ctx->tmp_nv12_size = 0;
    }

    ctx->is_initialized = false;

    return AVDK_ERR_OK;
}

// Decode video data using hardware JPEG decoder
static avdk_err_t hw_jpeg_decoder_decode(struct video_player_video_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer,
                                        pixel_format_t out_fmt)
{
    hw_jpeg_decoder_instance_t *hw_instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(hw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_jpeg_decoder_ctx_t *ctx = &hw_instance->ctx;
    AVDK_RETURN_ON_FALSE(in_buffer, AVDK_ERR_INVAL, TAG, "in_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data, AVDK_ERR_INVAL, TAG, "in_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(ctx->hw_decoder_handle, AVDK_ERR_GENERIC, TAG, "Hardware decoder not initialized");
    AVDK_RETURN_ON_FALSE(ctx->is_initialized, AVDK_ERR_GENERIC, TAG, "Hardware decoder not initialized");
    AVDK_RETURN_ON_FALSE(out_buffer->frame_buffer, AVDK_ERR_INVAL, TAG, "out_buffer->frame_buffer is NULL");

    // Output format is provided by caller via out_buffer->frame_buffer->fmt.
    // bk_decoder outputs NV12, so non-NV12 requests are converted before returning.
    frame_buffer_t *out_frame = (frame_buffer_t *)out_buffer->frame_buffer;

    // out_fmt is the expected output pixel format for this decode call.
    // Keep backward compatibility: if out_fmt is not set, fall back to out_frame->fmt.
    pixel_format_t requested_fmt = out_fmt;
    if (requested_fmt == PIXEL_FMT_UNKNOW || requested_fmt == 0)
    {
        requested_fmt = out_frame->fmt;
    }
    if (requested_fmt == PIXEL_FMT_UNKNOW || requested_fmt == 0)
    {
        requested_fmt = PIXEL_FMT_YUYV;
    }

    uint32_t pixel_bytes = 0;
    if (!hw_jpeg_pixel_bytes(requested_fmt, &pixel_bytes))
    {
        LOGE("%s: Unsupported requested output fmt=%d\n", __func__, requested_fmt);
        return AVDK_ERR_UNSUPPORTED;
    }
    (void)pixel_bytes;

    bk_jpeg_decode_img_info_t img_info = {0};
    img_info.input_stream = (uint8_t *)in_buffer->data;
    img_info.input_stream_length = in_buffer->length;
    avdk_err_t ret = bk_jpeg_decode_get_img_info(&img_info);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s: Failed to get JPEG image info via bk_decoder, fallback to SW, ret=%d\n", __func__, ret);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (((img_info.width & 1U) != 0U) || ((img_info.height & 1U) != 0U))
    {
        LOGW("%s: bk_decoder NV12 path requires even width/height, got %ux%u\n",
             __func__, img_info.width, img_info.height);
        return AVDK_ERR_UNSUPPORTED;
    }

    if ((ctx->video_params.width != img_info.width) || (ctx->video_params.height != img_info.height))
    {
        LOGI("%s: Reconfiguring bk_decoder from %ux%u to %ux%u\n",
             __func__,
             ctx->video_params.width,
             ctx->video_params.height,
             img_info.width,
             img_info.height);

        ret = hw_jpeg_decoder_reset_controller(ctx, (uint16_t)img_info.width, (uint16_t)img_info.height);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: Failed to reconfigure bk_decoder, ret=%d\n", __func__, ret);
            return ret;
        }

        ctx->video_params.width = (uint16_t)img_info.width;
        ctx->video_params.height = (uint16_t)img_info.height;
    }

    const uint32_t required_out_size = hw_jpeg_calc_output_size(img_info.width, img_info.height, requested_fmt);
    if (required_out_size == 0U)
    {
        LOGE("%s: Unsupported requested output fmt=%d\n", __func__, requested_fmt);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (required_out_size > out_buffer->length)
    {
        LOGE("%s: Output buffer too small, need=%u, got=%u, fmt=%d\n",
             __func__, required_out_size, out_buffer->length, requested_fmt);
        return AVDK_ERR_NOMEM;
    }

    uint8_t *decode_dst = out_buffer->data;
    uint32_t decode_dst_size = required_out_size;
    if (!hw_jpeg_needs_direct_nv12(requested_fmt))
    {
        const uint32_t nv12_size = hw_jpeg_calc_output_size(img_info.width, img_info.height, PIXEL_FMT_NV12);
        ret = hw_jpeg_decoder_ensure_tmp_nv12(ctx, nv12_size);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }

        decode_dst = ctx->tmp_nv12;
        decode_dst_size = nv12_size;
    }

    bk_jpeg_decode_input_t decode_input = {0};
    decode_input.stream = (uint8_t *)in_buffer->data;
    decode_input.stream_len = in_buffer->length;
    decode_input.out_buffer = decode_dst;
    decode_input.out_buffer_size = decode_dst_size;

    ret = bk_jpeg_decode_frame(ctx->hw_decoder_handle, &decode_input);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_decoder JPEG decode failed, ret=%d\n", __func__, ret);
        return ret;
    }

    if (hw_jpeg_needs_direct_nv12(requested_fmt))
    {
        out_frame->frame = out_buffer->data;
        out_frame->size = required_out_size;
        out_frame->length = required_out_size;
        out_frame->width = img_info.width;
        out_frame->height = img_info.height;
        out_frame->fmt = requested_fmt;
        out_frame->timestamp = (uint32_t)in_buffer->pts;
        out_buffer->length = required_out_size;
        out_buffer->pts = in_buffer->pts;
        return AVDK_ERR_OK;
    }

    switch (requested_fmt)
    {
        case PIXEL_FMT_YUYV:
            ret = hw_jpeg_nv12_to_yuyv(ctx->tmp_nv12, img_info.width, img_info.height, out_buffer->data);
            break;
        case PIXEL_FMT_RGB565:
            ret = hw_jpeg_nv12_to_rgb565(ctx->tmp_nv12, img_info.width, img_info.height, out_buffer->data);
            break;
        case PIXEL_FMT_RGB888:
            ret = hw_jpeg_nv12_to_rgb888(ctx->tmp_nv12, img_info.width, img_info.height, out_buffer->data);
            break;
        default:
            ret = AVDK_ERR_UNSUPPORTED;
            break;
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: format conversion failed, fmt=%d ret=%d\n", __func__, requested_fmt, ret);
        return ret;
    }

    out_frame->width = img_info.width;
    out_frame->height = img_info.height;
    out_frame->fmt = requested_fmt;
    out_frame->frame = out_buffer->data;
    out_frame->size = required_out_size;
    out_frame->length = required_out_size;
    out_frame->timestamp = (uint32_t)in_buffer->pts;
    out_buffer->length = required_out_size;
    out_buffer->pts = in_buffer->pts;

    LOGV("%s: bk_decoder JPEG decode successful, output size=%u, fmt=%d\n",
         __func__, out_buffer->length, requested_fmt);

    return AVDK_ERR_OK;
}

static video_player_video_decoder_ops_t *hw_jpeg_decoder_create(void)
{
    hw_jpeg_decoder_instance_t *instance = os_malloc(sizeof(hw_jpeg_decoder_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate hardware JPEG decoder instance\n", __func__);
        return NULL;
    }
    os_memset(instance, 0, sizeof(hw_jpeg_decoder_instance_t));

    os_memcpy(&instance->ops, &s_ops_template, sizeof(video_player_video_decoder_ops_t));
    return &instance->ops;
}

static void hw_jpeg_decoder_destroy(video_player_video_decoder_ops_t *ops)
{
    if (ops == NULL)
    {
        return;
    }
    if (ops == &s_ops_template)
    {
        return;
    }
    hw_jpeg_decoder_instance_t *instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    os_free(instance);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create = hw_jpeg_decoder_create,
    .destroy = hw_jpeg_decoder_destroy,
    .get_supported_formats = hw_jpeg_decoder_get_supported_formats,
    .init = hw_jpeg_decoder_init,
    .deinit = hw_jpeg_decoder_deinit,
    .decode = hw_jpeg_decoder_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_hw_jpeg_decoder_ops(void)
{
    return &s_ops_template;
}

