#include "os/os.h"
#include "os/mem.h"
#include "common/bk_err.h"
#include "lv_jpeg_hw_decode.h"
#if CONFIG_BK_DECODER && CONFIG_FRAME_BUFFER
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "components/bk_frame_buffer.h"
#include "components/media_types.h"
#include "driver/psram.h"
#endif

#define TAG "lv_hw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#if CONFIG_BK_DECODER && CONFIG_FRAME_BUFFER

static uint8_t lv_jpeg_clip_u8(int value)
{
    if (value < 0) {
        return 0;
    }

    if (value > 255) {
        return 255;
    }

    return (uint8_t)value;
}

static inline uint16_t bswap16_self(uint16_t x)
{
    uint32_t result;
    __asm__ volatile (
        "eor   %1, %1, %1, ror #16 \n"
        "mov   %1, %1, ror #8      \n"
        : "=r" (result)
        : "0" ((uint32_t)x << 16)
    );

    return (uint16_t)(result >> 16);
}

static bk_err_t lv_jpeg_nv12_to_rgb565(const uint8_t *src_nv12,
                                       uint32_t width,
                                       uint32_t height,
                                       uint8_t *dst_rgb565,
                                       bool byte_swap)
{
    if (src_nv12 == NULL || dst_rgb565 == NULL || (width & 1U) || (height & 1U)) {
        return BK_FAIL;
    }

    const uint8_t *y_plane = src_nv12;
    const uint8_t *uv_plane = src_nv12 + width * height;
    uint16_t *dst = (uint16_t *)dst_rgb565;

    for (uint32_t y = 0; y < height; y++) {
        const uint8_t *y_row = y_plane + y * width;
        const uint8_t *uv_row = uv_plane + (y >> 1) * width;

        for (uint32_t x = 0; x < width; x++) {
            const uint32_t uv_idx = x & ~1U;
            const int u = (int)uv_row[uv_idx] - 128;
            const int v = (int)uv_row[uv_idx + 1U] - 128;
            int c = (int)y_row[x] - 16;
            c = (c < 0) ? 0 : c;

            const uint8_t r = lv_jpeg_clip_u8((298 * c + 409 * v + 128) >> 8);
            const uint8_t g = lv_jpeg_clip_u8((298 * c - 100 * u - 208 * v + 128) >> 8);
            const uint8_t b = lv_jpeg_clip_u8((298 * c + 516 * u + 128) >> 8);
            uint16_t rgb565 = (uint16_t)(((uint16_t)(r >> 3) << 11) |
                                         ((uint16_t)(g >> 2) << 5) |
                                         (uint16_t)(b >> 3));

            dst[y * width + x] = byte_swap ? bswap16_self(rgb565) : rgb565;
        }
    }

    return BK_OK;
}

static void lv_jpeg_set_rgb565_header(lv_img_dsc_t *img_dst, uint32_t width, uint32_t height)
{
#if CONFIG_LVGL_V8
    img_dst->header.always_zero = 0;
    img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
    img_dst->header.w = width;
    img_dst->header.h = height;
#else
    img_dst->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dst->header.cf = LV_COLOR_FORMAT_RGB565;
    img_dst->header.flags = 0;
    img_dst->header.w = width;
    img_dst->header.h = height;
    img_dst->header.stride = width * 2;
#endif
}

static void lv_jpeg_hw_destroy_decoder(bk_jpeg_decode_ctlr_handle_t *decoder)
{
    if (decoder == NULL || *decoder == NULL) {
        return;
    }

    bk_jpeg_decode_close(*decoder);
    bk_jpeg_decode_deinit(*decoder);
    bk_jpeg_decode_delete(*decoder);
    *decoder = NULL;
}

static bk_err_t lv_jpeg_hw_get_info(uint8_t *jpeg_data,
                                    uint32_t jpeg_size,
                                    bk_jpeg_decode_img_info_t *img_info)
{
    os_memset(img_info, 0, sizeof(*img_info));
    img_info->input_stream = jpeg_data;
    img_info->input_stream_length = jpeg_size;

    bk_err_t ret = bk_jpeg_decode_get_img_info(img_info);
    if (ret != BK_OK) {
        LOGE("[%s][%d] get img info failed, ret: %d\r\n", __func__, __LINE__, ret);
    }

    return ret;
}

static bk_err_t lv_jpeg_hw_create_decoder(bk_jpeg_decode_ctlr_handle_t *decoder,
                                          const bk_jpeg_decode_img_info_t *img_info)
{
    bk_jpeg_decode_frame_config_t config = DEFAULT_JPEG_DECODE_FRAME_CONFIG;
    config.out_width = img_info->width;
    config.out_height = img_info->height;
    config.out_format = BK_PIXEL_FORMAT_NV12;

    bk_err_t ret = bk_jpeg_decode_frame_ctlr_new(decoder, &config);
    if (ret != BK_OK) {
        return ret;
    }

    ret = bk_jpeg_decode_init(*decoder);
    if (ret != BK_OK) {
        lv_jpeg_hw_destroy_decoder(decoder);
        return ret;
    }

    ret = bk_jpeg_decode_open(*decoder);
    if (ret != BK_OK) {
        lv_jpeg_hw_destroy_decoder(decoder);
    }

    return ret;
}

bk_err_t lv_jpeg_hw_decode_start(uint8_t *jpeg_data, uint32_t jpeg_size, lv_img_dsc_t *img_dst, bool byte_swap)
{
    bk_err_t ret = BK_FAIL;
    uint8_t *nv12_data = NULL;
    bk_jpeg_decode_ctlr_handle_t decoder = NULL;
    bk_jpeg_decode_img_info_t img_info = {0};

    if (jpeg_data == NULL || jpeg_size == 0 || img_dst == NULL) {
        LOGE("[%s][%d] invalid params\r\n", __func__, __LINE__);
        return ret;
    }

    ret = lv_jpeg_hw_get_info(jpeg_data, jpeg_size, &img_info);
    if (ret != BK_OK) {
        return ret;
    }

    lv_jpeg_set_rgb565_header(img_dst, img_info.width, img_info.height);
    img_dst->data_size = img_info.width * img_info.height * 2;
    img_dst->data = psram_malloc(img_dst->data_size);
    if (!img_dst->data) {
        LOGE("[%s][%d] malloc psram size %d fail\r\n", __func__, __LINE__, img_dst->data_size);
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    do {
        const uint32_t nv12_size = bk_image_size_get(img_info.width, img_info.height, BK_PIXEL_FORMAT_NV12);
        nv12_data = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, nv12_size);
        if (nv12_data == NULL) {
            LOGE("[%s][%d] malloc nv12 size %d fail\r\n", __func__, __LINE__, nv12_size);
            ret = BK_ERR_NO_MEM;
            break;
        }

        ret = lv_jpeg_hw_create_decoder(&decoder, &img_info);
        if (ret != BK_OK) {
            LOGE("%s create decoder fail %d\n", __func__, ret);
            break;
        }

        bk_jpeg_decode_input_t input = {0};
        input.stream = jpeg_data;
        input.stream_len = jpeg_size;
        input.out_buffer = nv12_data;
        input.out_buffer_size = nv12_size;

        ret = bk_jpeg_decode_frame(decoder, &input);
        if (ret != BK_OK) {
            LOGE("%s hw decode start fail %d\n", __func__, ret);
            break;
        }

        ret = lv_jpeg_nv12_to_rgb565(nv12_data, img_info.width, img_info.height, (uint8_t *)img_dst->data, byte_swap);
    } while(0);

    lv_jpeg_hw_destroy_decoder(&decoder);
    if (nv12_data != NULL) {
        bk_frame_buffer_free(nv12_data);
    }
    if (ret != BK_OK && img_dst->data != NULL) {
        psram_free((void *)img_dst->data);
        img_dst->data = NULL;
        img_dst->data_size = 0;
    }

    return ret;
}

#else

bk_err_t lv_jpeg_hw_decode_start(uint8_t *jpeg_data, uint32_t jpeg_size, lv_img_dsc_t *img_dst, bool byte_swap)
{
    (void)jpeg_data;
    (void)jpeg_size;
    (void)img_dst;
    (void)byte_swap;

    LOGE("%s requires CONFIG_BK_DECODER and CONFIG_FRAME_BUFFER\r\n", __func__);
    return BK_ERR_NOT_SUPPORT;
}

#endif
