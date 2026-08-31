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

static void lv_jpeg_rgb565_byte_swap(uint8_t *data, uint32_t data_size)
{
    if (data == NULL) {
        return;
    }

    for (uint32_t i = 0; i + 1U < data_size; i += 2) {
        uint16_t *p = (uint16_t *)&data[i];
        *p = bswap16_self(*p);
    }
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
    config.out_format = BK_PIXEL_FORMAT_RGB565;

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
    const uint32_t hw_output_size = img_info.width * ((img_info.height + 1U) & ~1U) * 2U;
    img_dst->data = psram_malloc(hw_output_size);
    if (!img_dst->data) {
        LOGE("[%s][%d] malloc psram size %d fail\r\n", __func__, __LINE__, hw_output_size);
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    do {
        ret = lv_jpeg_hw_create_decoder(&decoder, &img_info);
        if (ret != BK_OK) {
            LOGE("%s create decoder fail %d\n", __func__, ret);
            break;
        }

        bk_jpeg_decode_input_t input = {0};
        input.stream = jpeg_data;
        input.stream_len = jpeg_size;
        input.out_buffer = (uint8_t *)img_dst->data;
        input.out_buffer_size = hw_output_size;

        ret = bk_jpeg_decode_frame(decoder, &input);
        if (ret != BK_OK) {
            LOGE("%s hw decode start fail %d\n", __func__, ret);
            break;
        }

        if (byte_swap) {
            lv_jpeg_rgb565_byte_swap((uint8_t *)img_dst->data, img_dst->data_size);
        }
    } while(0);

    lv_jpeg_hw_destroy_decoder(&decoder);
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
