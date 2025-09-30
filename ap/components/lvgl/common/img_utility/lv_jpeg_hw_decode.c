#include "os/os.h"
#include "os/mem.h"
#include "driver/dma2d.h"
#include "lv_jpeg_hw_decode.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/media_types.h"

#define TAG "lv_hw_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static beken_semaphore_t lv_dma2d_sem = NULL;
static frame_buffer_t *g_dec_out_frame = NULL;
static bk_jpeg_decode_hw_handle_t lv_jpeg_decode_handle = NULL;

static bk_err_t lv_jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame);

static bk_jpeg_decode_hw_config_t lv_jpeg_decode_config = {
    .decode_cbs = {.out_complete = lv_jpeg_decode_complete,}
};

static void lv_dma2d_config_error(void *arg)
{
    LOGD("%s \n", __func__);
}

static void lv_dma2d_transfer_error(void *arg)
{
    LOGE("%s \n", __func__);
}

static void lv_dma2d_transfer_complete(void *arg)
{
    rtos_set_semaphore(&lv_dma2d_sem);
}

static bk_err_t lv_dma2d_yuyv2rgb565_init(void)
{
    bk_err_t ret;

    ret = rtos_init_semaphore_ex(&lv_dma2d_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s %d lv_dma2d_sem init failed\n", __func__, __LINE__);
        return ret;
    }

    bk_dma2d_driver_init();
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, lv_dma2d_config_error, NULL);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, lv_dma2d_transfer_error, NULL);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, lv_dma2d_transfer_complete, NULL);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);

    return ret;
}

static bk_err_t lv_dma2d_yuyv2rgb565_deinit(void)
{
    bk_err_t ret;

    bk_dma2d_stop_transfer();
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 0);
    bk_dma2d_driver_deinit();
    ret = rtos_deinit_semaphore(&lv_dma2d_sem);
    if (BK_OK != ret) {
        LOGE("%s %d lv_dma2d_sem deinit failed\n", __func__, __LINE__);
    }

    return ret;
}

static void lv_dma2d_yuyv2rgb565(void *src, const void *dst, uint16_t width, uint16_t height, bool byte_swap)
{
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};

    dma2d_memcpy_pfc.input_addr = (char *)src;
    dma2d_memcpy_pfc.output_addr = (char *)dst;
    dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    dma2d_memcpy_pfc.input_color_mode = DMA2D_INPUT_YUYV;
    dma2d_memcpy_pfc.output_color_mode = DMA2D_OUTPUT_RGB565;
    dma2d_memcpy_pfc.src_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dst_pixel_byte = TWO_BYTES;
    dma2d_memcpy_pfc.dma2d_width = width;
    dma2d_memcpy_pfc.dma2d_height = height;
    dma2d_memcpy_pfc.src_frame_width = width;
    dma2d_memcpy_pfc.src_frame_height = height;
    dma2d_memcpy_pfc.dst_frame_width = width;
    dma2d_memcpy_pfc.dst_frame_height = height;
    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = 0;
    dma2d_memcpy_pfc.dst_frame_ypos = 0;
    dma2d_memcpy_pfc.input_red_blue_swap = 0;
    dma2d_memcpy_pfc.output_red_blue_swap = 0;

    if (byte_swap) {
        dma2d_memcpy_pfc.out_byte_by_byte_reverse = 1;
    } else {
        dma2d_memcpy_pfc.out_byte_by_byte_reverse = 0;
    }

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();

    rtos_get_semaphore(&lv_dma2d_sem, BEKEN_NEVER_TIMEOUT);
}

static bk_err_t lv_jpeg_decode_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK) {
        LOGD("%s, %d, jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    } else {
        LOGE("%s, %d, jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }

    return BK_OK;
}

bk_err_t lv_jpeg_hw_decode_init(void)
{
    bk_err_t ret = BK_FAIL;

    lv_dma2d_yuyv2rgb565_init();

    bk_hardware_jpeg_decode_new(&lv_jpeg_decode_handle, &lv_jpeg_decode_config);
    bk_jpeg_decode_hw_open(lv_jpeg_decode_handle);

    g_dec_out_frame = os_malloc(sizeof(frame_buffer_t));
    if (!g_dec_out_frame) {
        LOGD("[%s][%d] g_dec_out_frame malloc fail\n", __FUNCTION__, __LINE__);
        return ret;
    }
    os_memset(g_dec_out_frame, 0, sizeof(frame_buffer_t));

    return ret;
}

bk_err_t lv_jpeg_hw_decode_deinit(void)
{
    bk_err_t ret = BK_FAIL;

    lv_dma2d_yuyv2rgb565_deinit();
    bk_jpeg_decode_hw_close(lv_jpeg_decode_handle);

    bk_jpeg_decode_hw_delete(lv_jpeg_decode_handle);
    lv_jpeg_decode_handle = NULL;

    if (g_dec_out_frame) {
        os_free(g_dec_out_frame);
        g_dec_out_frame = NULL;
    }

    return ret;
}

bk_err_t lv_jpeg_hw_decode_start(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst, bool byte_swap)
{
    bk_err_t ret = BK_FAIL;
    bk_jpeg_decode_img_info_t img_info = {0}; 

    if (jpeg_frame == NULL) {
        LOGE("[%s][%d] jpeg_frame is null\r\n", __func__, __LINE__);
        return ret;
    }

    if (img_dst == NULL) {
        LOGE("[%s][%d] img_dst is null\r\n", __func__, __LINE__);
        return ret;
    }

    img_info.frame = jpeg_frame;
    ret = bk_jpeg_decode_hw_get_img_info(lv_jpeg_decode_handle, &img_info);
    if (ret != BK_OK) {
        LOGE("[%s][%d] get img info failed, ret: %d\r\n", __func__, __LINE__, ret);
        return ret;
    }

    img_dst->header.always_zero = 0;
    img_dst->header.cf = LV_IMG_CF_TRUE_COLOR;
    img_dst->header.w = img_info.width;
    img_dst->header.h = img_info.height;
    img_dst->data_size = img_info.width * img_info.height * 2;
    img_dst->data = psram_malloc(img_dst->data_size);
    if (!img_dst->data) {
        LOGE("[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, img_dst->data_size);
        ret = BK_ERR_NO_MEM;
        return ret;
    }

    do {
        g_dec_out_frame->frame = psram_malloc(img_dst->data_size);
        if (!g_dec_out_frame->frame) {
            LOGE("[%s][%d] malloc psram size %d fail\r\n", __FUNCTION__, __LINE__, img_dst->data_size);
            ret = BK_ERR_NO_MEM;
            break;
        }
        g_dec_out_frame->size = img_dst->data_size;

        ret = bk_jpeg_decode_hw_decode(lv_jpeg_decode_handle, jpeg_frame, g_dec_out_frame);
        if (ret != BK_OK) {
            LOGE("%s hw decode start fail %d\n", __func__, ret);
            break;
        }

        lv_dma2d_yuyv2rgb565(g_dec_out_frame->frame, img_dst->data, img_dst->header.w, img_dst->header.h, byte_swap);
    } while(0);

    if (g_dec_out_frame->frame) {
        psram_free(g_dec_out_frame->frame);
        g_dec_out_frame->frame = NULL;
    }

    return ret;
}
