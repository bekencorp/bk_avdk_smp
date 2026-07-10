#include "os/os.h"
#include "os/mem.h"
#include "lv_jpeg_sw_decode.h"
#include "modules/jpeg_decode_sw.h"
#include "components/media_types.h"
#include "driver/psram.h"

#define TAG "lv_sw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

bk_err_t lv_jpeg_sw_decode_init(void)
{
    return BK_OK;
}

bk_err_t lv_jpeg_sw_decode_deinit(void)
{
    return BK_OK;
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

bk_err_t lv_jpeg_sw_decode_start(uint8_t *jpeg_data, uint32_t jpeg_size, lv_img_dsc_t *img_dst, bool byte_swap)
{
    bk_err_t ret = BK_FAIL;
    sw_jpeg_dec_res_t img_info = {0};

    if (jpeg_data == NULL || jpeg_size == 0) {
        LOGE("[%s][%d] jpeg data is invalid\r\n", __func__, __LINE__);
        return ret;
    }

    if (img_dst == NULL) {
        LOGE("[%s][%d] img_dst is null\r\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_jpeg_get_img_info(jpeg_size, jpeg_data, &img_info, NULL);
    if (ret != BK_OK) {
        LOGE("[%s][%d] get img info failed, ret: %d\r\n", __func__, __LINE__, ret);
        return ret;
    }

    lv_jpeg_set_rgb565_header(img_dst, img_info.pixel_x, img_info.pixel_y);
    img_dst->data_size = img_info.pixel_x * img_info.pixel_y * 2;
    img_dst->data = psram_malloc(img_dst->data_size);
    if (!img_dst->data) {
        LOGE("[%s][%d] psram malloc fail\r\n", __FUNCTION__, __LINE__);
        ret = BK_FAIL;
        return ret;
    }

    ret = bk_jpeg_dec_sw_start_one_time(JPEGDEC_BY_FRAME,
                                        jpeg_data,
                                        (uint8_t *)img_dst->data,
                                        jpeg_size,
                                        img_dst->data_size,
                                        &img_info,
                                        0,
                                        JD_FORMAT_RGB565,
                                        ROTATE_NONE,
                                        NULL,
                                        NULL);
    if (ret != BK_OK) {
        LOGE("[%s][%d] sw decoder error\r\n", __FUNCTION__, __LINE__);
        psram_free((void *)img_dst->data);
        img_dst->data = NULL;
        img_dst->data_size = 0;
        return ret;
    }

    if (byte_swap) {
        for (int i = 0; i < img_dst->data_size; i += 2) {
            uint16_t *p = (uint16_t *)&img_dst->data[i];
            *p = bswap16_self(*p);
        }
    }

    return ret;
}
