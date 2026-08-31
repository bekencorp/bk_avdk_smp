#include "os/os.h"
#include "os/mem.h"
#include <common/avdk_pixel_types.h>
#include "components/media_types.h"
#if CONFIG_FRAME_BUFFER
#include "components/bk_frame_buffer.h"
#endif
#include "driver/psram.h"
#include "lv_jpeg_hw_decode.h"
#include "lv_jpeg_sw_decode.h"
#include "lvgl.h"
#include "lv_vendor.h"
#if !CONFIG_LVGL_V8
#include "src/draw/lv_image_decoder_private.h"
#endif
#include "bk_posix.h"

#define TAG "lv_img_utility"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LV_IMG_FILE_ALIGN(size) (((size) + 3U) & ~3U)

typedef struct {
    uint8_t *data;
    uint32_t size;
    bool use_frame_buffer;
} lv_img_file_data_t;

static bk_err_t lv_img_read_file_to_mem(char *filename, uint8_t *data)
{
    uint8 *sram_addr = NULL;
    uint32 once_read_len = 1024 * 4;
    uint8_t *dst = data;
    int fd = -1;
    int read_len = 0;
    bk_err_t ret = BK_FAIL;

    do {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            LOGE("[%s][%d] open fail:%s\r\n", __FUNCTION__, __LINE__, filename);
            ret = BK_FAIL;
            break;
        }

        sram_addr = lv_vendor_malloc(once_read_len);
        if (sram_addr == NULL) {
            LOGE("[%s][%d] malloc fail\r\n", __FUNCTION__, __LINE__);
            ret = BK_FAIL;
            break;
        }

        while(1)
        {
            read_len = read(fd, sram_addr, once_read_len);
            if (read_len < 0) {
                LOGD("[%s][%d] read file fail.\r\n", __FUNCTION__, __LINE__);
                ret= BK_FAIL;
                break;
            }

            if (read_len == 0) {
                ret = BK_OK;
                break;
            }

            uint32 copy_len = read_len;
            if (copy_len % 4) {
                copy_len = LV_IMG_FILE_ALIGN(copy_len);
            }
            bk_psram_word_memcpy((uint32 *)dst, sram_addr, copy_len);
            dst += read_len;
        }
    } while(0);

    if (sram_addr) {
        lv_vendor_free(sram_addr);
        sram_addr = NULL;
    }

    if (fd >= 0) {
        close(fd);
    }

    return ret;
}

int lv_img_get_filelen(char *filename)
{
    int ret = BK_FAIL;
    struct stat statbuf;

    do {
        if (!filename) {
            LOGE("[%s][%d]param is null.\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_PARAM;
            break;
        }

        ret = stat(filename, &statbuf);
        if (BK_OK != ret) {
            LOGE("[%s][%d] sta fail:%s\r\n", __FUNCTION__, __LINE__, filename);
            break;
        }

        ret = statbuf.st_size;
        LOGD("[%s][%d] %s size:%d\r\n", __FUNCTION__, __LINE__, filename, ret);
    } while(0);

    return ret;
}

static void lv_img_file_data_free(lv_img_file_data_t *file_data)
{
    if (file_data == NULL || file_data->data == NULL) {
        return;
    }

#if CONFIG_FRAME_BUFFER
    if (file_data->use_frame_buffer) {
        bk_frame_buffer_free(file_data->data);
    } else {
        psram_free(file_data->data);
    }
#else
    psram_free(file_data->data);
#endif

    file_data->data = NULL;
    file_data->size = 0;
}

static bk_err_t lv_img_read_file(char *file_name, bool use_frame_buffer, lv_img_file_data_t *file_data)
{
    int file_len;
    bk_err_t ret = BK_FAIL;

    if (file_data == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    os_memset(file_data, 0, sizeof(*file_data));

    do {
        file_len = lv_img_get_filelen(file_name);
        if (file_len <= 0) {
            LOGE("[%s][%d] %s don't exit in fatfs\r\n", __FUNCTION__, __LINE__, file_name);
            break;
        }

        const uint32_t alloc_size = LV_IMG_FILE_ALIGN((uint32_t)file_len);
#if CONFIG_FRAME_BUFFER
        if (use_frame_buffer) {
            file_data->data = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, alloc_size);
        } else {
            file_data->data = psram_malloc(alloc_size);
        }
#else
        file_data->data = psram_malloc(alloc_size);
#endif
        if (!file_data->data) {
            LOGE("[%s][%d] file data malloc fail, size:%d\r\n", __FUNCTION__, __LINE__, alloc_size);
            break;
        }
        os_memset(file_data->data, 0, alloc_size);

        file_data->size = file_len;
#if CONFIG_FRAME_BUFFER
        file_data->use_frame_buffer = use_frame_buffer;
#else
        file_data->use_frame_buffer = false;
#endif
        ret = lv_img_read_file_to_mem((char *)file_name, file_data->data);
        if (BK_OK != ret) {
            lv_img_file_data_free(file_data);
        }
    } while(0);

    return ret;
}

static bk_err_t lv_img_file_jpeg_sw_dec(char *file_name, lv_img_dsc_t *img_dst, bool byte_swap)
{
    int ret = BK_FAIL;
    lv_img_file_data_t jpeg_file;

    do {
        ret = lv_img_read_file(file_name, false, &jpeg_file);
        if (ret != BK_OK) {
            break;
        }

        ret = lv_jpeg_sw_decode_start(jpeg_file.data, jpeg_file.size, img_dst, byte_swap);
        if (BK_OK == ret) {
            LOGD("[%s][%d] decode success, width:%d, height:%d, size:%d\r\n", __FUNCTION__, __LINE__,
                                            img_dst->header.w, img_dst->header.h, img_dst->data_size);
        }
    } while(0);

    lv_img_file_data_free(&jpeg_file);

    return ret;
}

static bk_err_t lv_img_file_jpeg_hw_dec(char *file_name, lv_img_dsc_t *img_dst, bool byte_swap)
{
    int ret = BK_FAIL;
    lv_img_file_data_t jpeg_file;

    do {
        ret = lv_img_read_file(file_name, true, &jpeg_file);
        if (ret != BK_OK) {
            break;
        }

        ret = lv_jpeg_hw_decode_start(jpeg_file.data, jpeg_file.size, img_dst, byte_swap);
        if (BK_OK == ret) {
            LOGD("[%s][%d] hw decode success, width:%d, height:%d, size:%d\r\n", __FUNCTION__, __LINE__,
                                                img_dst->header.w, img_dst->header.h, img_dst->data_size);
        }
    } while(0);

    lv_img_file_data_free(&jpeg_file);

    return ret;
}

bk_err_t lv_jpeg_img_load_with_sw_dec(char *filename, lv_img_dsc_t *img_dst, bool byte_swap)
{
    int ret = BK_FAIL;

    do {
        if (!filename || !img_dst) {
            LOGE("[%s][%d]filename or img_dst is null\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        ret = lv_jpeg_sw_decode_init();
        if (ret != BK_OK) {
            LOGE("[%s][%d] lv_jpeg_sw_decode_init fail\r\n", __FUNCTION__, __LINE__);
            break;
        }

        ret = lv_img_file_jpeg_sw_dec(filename, img_dst, byte_swap);
        if (ret != BK_OK) {
            LOGE("%s jpeg sw decode fail\r\n", __func__);
            break;
        }

        ret = lv_jpeg_sw_decode_deinit();
        if (ret != BK_OK) {
            LOGE("[%s][%d] lv_jpeg_sw_decode_deinit fail\r\n", __FUNCTION__, __LINE__);
            break;
        }
    } while(0);

    return ret;
}

bk_err_t lv_jpeg_img_load_with_hw_dec(char *filename, lv_img_dsc_t *img_dst, bool byte_swap)
{
    int ret = BK_FAIL;

    do {
        if (!filename || !img_dst) {
            LOGE("[%s][%d]filename or img_dst is null\r\n", __FUNCTION__, __LINE__);
            ret = BK_ERR_NULL_PARAM;
            break;
        }

        ret = lv_img_file_jpeg_hw_dec(filename, img_dst, byte_swap);
        if (ret != BK_OK) {
            LOGE("%s jpeg hw decode fail\r\n", __func__);
            break;
        }
    } while(0);

    return ret;
}

bk_err_t lv_png_img_load(char *filename, lv_img_dsc_t *img_dst)
{
    int ret = BK_FAIL;
#if CONFIG_LVGL_V8
    lv_img_decoder_dsc_t img_decoder_dsc;
#else
    lv_image_decoder_dsc_t img_decoder_dsc;
    lv_image_decoder_args_t args = {0};
#endif
    uint32_t data_size = 0;

    if (!filename || !img_dst) {
        ret = BK_ERR_NULL_PARAM;
        LOGE("[%s][%d]param invalid\r\n", __FUNCTION__, __LINE__);
        return ret;
    }

    memset((char *)&img_decoder_dsc, 0, sizeof(img_decoder_dsc));
#if CONFIG_LVGL_V8
    img_decoder_dsc.src_type = LV_IMG_SRC_FILE;
    ret = lv_img_decoder_open(&img_decoder_dsc, filename, img_decoder_dsc.color, img_decoder_dsc.frame_id);
    if (ret != LV_RES_OK) {
        LOGE("[%s][%d] decoder open fail:%d\r\n", __FUNCTION__, __LINE__, ret);
        ret = BK_FAIL;
        return ret;
    }

    memcpy(&img_dst->header, &img_decoder_dsc.header, sizeof(lv_img_header_t));
    if (img_dst->header.cf == LV_IMG_CF_TRUE_COLOR_ALPHA) {
        data_size = LV_IMG_BUF_SIZE_TRUE_COLOR_ALPHA(img_dst->header.w, img_dst->header.h);
    } else {
        data_size = LV_IMG_BUF_SIZE_TRUE_COLOR(img_dst->header.w, img_dst->header.h);
    }
#else
    args.no_cache = true;
    ret = lv_image_decoder_open(&img_decoder_dsc, filename, &args);
    if (ret != LV_RESULT_OK || img_decoder_dsc.decoded == NULL || img_decoder_dsc.decoded->data == NULL) {
        LOGE("[%s][%d] decoder open fail:%d\r\n", __FUNCTION__, __LINE__, ret);
        ret = BK_FAIL;
        return ret;
    }

    memcpy(&img_dst->header, &img_decoder_dsc.decoded->header, sizeof(lv_image_header_t));
    img_dst->header.flags = 0;
    data_size = img_decoder_dsc.decoded->data_size;
#endif

    img_dst->data = psram_malloc(data_size);
    if (img_dst->data == NULL) {
        LOGE("[%s][%d] psram malloc fail\r\n", __FUNCTION__, __LINE__);
#if CONFIG_LVGL_V8
        lv_img_decoder_close(&img_decoder_dsc);
#else
        lv_image_decoder_close(&img_decoder_dsc);
#endif
        return BK_ERR_NO_MEM;
    }

    img_dst->data_size = data_size;
#if CONFIG_LVGL_V8
    os_memcpy((void *)img_dst->data, img_decoder_dsc.img_data, data_size);
    lv_img_decoder_close(&img_decoder_dsc);
#else
    os_memcpy((void *)img_dst->data, img_decoder_dsc.decoded->data, data_size);
    lv_image_decoder_close(&img_decoder_dsc);
#endif

    return ret;
}

void lv_img_decode_unload(lv_img_dsc_t *img_dst)
{
    if (img_dst) {
        if (img_dst->data) {
            psram_free((void *)img_dst->data);
            img_dst->data = NULL;
        }
        img_dst->data_size = 0;
        os_memset(&img_dst->header, 0, sizeof(img_dst->header));
    }
}

