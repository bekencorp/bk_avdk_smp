#include <os/os.h>
#include <os/str.h>
#include <components/avdk_utils/avdk_error.h>
#include "dma2d_test.h"
#include "components/bk_dma2d.h"
#include "frame_buffer.h"
#define TAG "dma2d_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

beken_semaphore_t dma2d_sem;
extern void bk_mem_dump_ex(const char * title, unsigned char * data, uint32_t data_len);

avdk_err_t memcopy_src_data_pre(void *src, uint32_t color, uint8_t pixel_byte, uint32_t width, uint32_t height)
{
    if (src == NULL) {
        LOGE("memcopy_src_data_pre: src is NULL");
        return AVDK_ERR_INVAL;
    }
    if (pixel_byte == 2) { // RGB565
        uint16_t *src_data = (uint16_t *)src;
        for (int i = 0; i < width * height; i++) {
            src_data[i] = (uint16_t)color;
        }
    } else if (pixel_byte == 3) { // RGB888
        uint8_t *src_data = (uint8_t *)src;
        for (int i = 0; i < width * height; i++) {
            src_data[i * 3] = (uint8_t)(color >> 16);     // R
            src_data[i * 3 + 1] = (uint8_t)(color >> 8);  // G
            src_data[i * 3 + 2] = (uint8_t)color;         // B
        }
    } else if (pixel_byte == 4) { // ARGB8888
        uint32_t *src_data = (uint32_t *)src;
        for (int i = 0; i < width * height; i++) {
            src_data[i] = color;
        }
    } else {
        LOGE("memcopy_src_data_pre: pixel_byte %d not support", pixel_byte);
        return AVDK_ERR_UNSUPPORTED;
    }
    return AVDK_ERR_OK;
}

avdk_err_t memcopy_test_check(uint16_t *dst, uint16_t *src, uint8_t pixel_byte, uint32_t width, uint32_t height)
{
    uint32_t compare_result = 0;
    uint32_t pixel_count = width * height;
    
    if (pixel_byte == 2) { // RGB565 格式
        uint16_t *src_data = (uint16_t *)src;
        uint16_t *dst_data = (uint16_t *)dst;
        for (uint32_t i = 0; i < pixel_count; i++) {
            if (src_data[i] != dst_data[i]) {
                compare_result++;
                if (compare_result <= 10) {
                    LOGD("Pixel mismatch at index %d: src=0x%x, dst=0x%x \n", 
                            i, src_data[i], dst_data[i]);
                }
            }
        }
    } else if (pixel_byte == 3) { // RGB888 格式
        uint8_t *src_data = (uint8_t *)src;
        uint8_t *dst_data = (uint8_t *)dst;

        for (uint32_t i = 0; i < pixel_count; i++) {
            if (src_data[i * 3] != dst_data[i * 3] || 
                src_data[i * 3 + 1] != dst_data[i * 3 + 1] || 
                src_data[i * 3 + 2] != dst_data[i * 3 + 2]) {
                compare_result++;
                if (compare_result <= 10) {
                    LOGD("Pixel mismatch at index %d: src=0x%02x%02x%02x, dst=0x%02x%02x%02x \n", 
                            i, src_data[i * 3], src_data[i * 3 + 1], src_data[i * 3 + 2],
                            dst_data[i * 3], dst_data[i * 3 + 1], dst_data[i * 3 + 2]);
                }
            }
        }
    } else if (pixel_byte == 4) { // ARGB8888
        uint32_t *src_data = (uint32_t *)src;
        uint32_t *dst_data = (uint32_t *)dst;
        
        for (uint32_t i = 0; i < pixel_count; i++) {
            if (src_data[i] != dst_data[i]) {
                compare_result++;
                if (compare_result <= 10) {
                    LOGD("Pixel mismatch at index %d: src=0x%x, dst=0x%x \n", 
                            i, src_data[i], dst_data[i]);
                }
            }
        }
    }
    
    if (compare_result == 0) {
        LOGI("DMA2D memcpy test PASSED \n");
        return AVDK_ERR_OK;
    } else {
        LOGE("DMA2D memcpy test FAILED: found %d mismatched pixel(s)\n", compare_result);
        return AVDK_ERR_GENERIC;
    }
}

void bk_dma2d_memcpy_complete_cb(dma2d_trans_status_t status, void *user_data)
{
    rtos_set_semaphore(&dma2d_sem);
    LOGD("bk_dma2d_memcpy_complete_cb, status = %d \n", status);
}
int dma2d_memcpy_test(bk_dma2d_ctlr_handle_t handle, const char *format, uint32_t color,
                     uint16_t src_width, uint16_t src_height,
                     uint16_t dst_width, uint16_t dst_height,
                     uint16_t src_frame_xpos, uint16_t src_frame_ypos,
                     uint16_t dst_frame_xpos, uint16_t dst_frame_ypos,
                     uint16_t dma2d_width, uint16_t dma2d_height)
{
    avdk_err_t ret = AVDK_ERR_OK;
    input_color_mode_t color_format, output_color_mode;
    uint8_t pixel_byte;
    //bk_dma2d_ioctl(handle, DMA2D_IOCTL_SET_SWRESRT, 0, 0, 0); // 软件复位

    if (os_strcmp(format, "ARGB8888") == 0) {
        color_format = DMA2D_INPUT_ARGB8888;
        output_color_mode = DMA2D_OUTPUT_ARGB8888;
        pixel_byte = 4;
    } else if (os_strcmp(format, "RGB888") == 0) {
        color_format = DMA2D_INPUT_RGB888;
        output_color_mode = DMA2D_OUTPUT_RGB888;

        pixel_byte = 3;
    } else {
        color_format = DMA2D_INPUT_RGB565;
        output_color_mode = DMA2D_OUTPUT_RGB565;
        pixel_byte = 2;
    }
    
    LOGI("%s memcopy information \n", __func__);
    LOGI("color_format %d, color %x, src_width: %d, src_height: %d, dst_width: %d, dst_height: %d, src_frame_xpos: %d, src_frame_ypos: %d, dst_frame_xpos: %d, dst_frame_ypos: %d, dma2d_width: %d, dma2d_height: %d \n",
        color_format, color, src_width, src_height, dst_width, dst_height,
        src_frame_xpos, src_frame_ypos, dst_frame_xpos, dst_frame_ypos, dma2d_width, dma2d_height);

    ret = rtos_init_semaphore(&dma2d_sem, 1);
    AVDK_RETURN_ON_ERROR(ret, TAG, "rtos_init_semaphore failed! \n");

    dma2d_memcpy_config_t memcpy_config = {0};
    frame_buffer_t *src_frame = frame_buffer_display_malloc(src_width * src_height * pixel_byte);
    AVDK_RETURN_ON_FALSE(src_frame, AVDK_ERR_NOMEM, TAG, "frame_buffer_display_malloc failed! \n");
    
    frame_buffer_t *dst_frame = frame_buffer_display_malloc(dst_width * dst_height * pixel_byte);
    if (!dst_frame) {
        LOGE("frame_buffer_display_malloc failed! \n");
        frame_buffer_display_free(src_frame);
        return AVDK_ERR_NOMEM;
    }

    memcopy_src_data_pre((uint8_t *)src_frame->frame,color, pixel_byte, src_width, src_height);
    os_memset((void *)dst_frame->frame, 0, dst_width * dst_height * pixel_byte);
    //bk_mem_dump_ex("src_frame", src_frame->frame, src_width *src_height * pixel_byte + 4);

    //src frame config
    memcpy_config.memcpy.input_addr = (char *)src_frame->frame;
    memcpy_config.memcpy.src_frame_width = src_width;
    memcpy_config.memcpy.src_frame_height = src_height;
    memcpy_config.memcpy.src_frame_xpos = src_frame_xpos;
    memcpy_config.memcpy.src_frame_ypos = src_frame_ypos;
    memcpy_config.memcpy.input_color_mode = color_format;
    memcpy_config.memcpy.src_pixel_byte = pixel_byte;
    
    //dst frame config
    memcpy_config.memcpy.output_addr = (char *)dst_frame->frame;
    memcpy_config.memcpy.dst_frame_width = dst_width;
    memcpy_config.memcpy.dst_frame_height = dst_height;
    memcpy_config.memcpy.dst_frame_xpos = dst_frame_xpos;
    memcpy_config.memcpy.dst_frame_ypos = dst_frame_ypos;
    memcpy_config.memcpy.output_color_mode = output_color_mode;
    memcpy_config.memcpy.dst_pixel_byte = pixel_byte;
    
    //dma2d memcpy config
    memcpy_config.memcpy.dma2d_width = dma2d_width;
    memcpy_config.memcpy.dma2d_height = dma2d_height;
    
    //memcpy complete cb
    memcpy_config.transfer_complete_cb = bk_dma2d_memcpy_complete_cb;
    memcpy_config.is_sync = false;
    
    ret = bk_dma2d_memcpy(handle, &memcpy_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_memcpy failed! \n");
        goto out;
    }

    ret = rtos_get_semaphore(&dma2d_sem, BEKEN_NEVER_TIMEOUT);
    if (ret != AVDK_ERR_OK) {
        LOGE("rtos_get_semaphore failed! \n");
        goto out;
    }

    //memcopy_test_check((uint16_t *)dst_frame->frame, (uint16_t *)src_frame->frame, pixel_byte, dst_width, dst_height);


out:
    if (src_frame) {
        frame_buffer_display_free(src_frame);
    }
    if (dst_frame) {
        frame_buffer_display_free(dst_frame);
    }
    rtos_deinit_semaphore(&dma2d_sem);
    return ret;
}
