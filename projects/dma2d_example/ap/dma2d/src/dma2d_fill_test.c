#include <os/os.h>
#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <os/str.h>
#include "dma2d_test.h"

#define TAG "dma2d_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void bk_mem_dump_ex(const char * title, unsigned char * data, uint32_t data_len);

void bk_dma2d_fill_complete_cb(dma2d_trans_status_t status, void *user_data)
{
    LOGD("bk_dma2d_fill_complete_cb, status = %d \n", status);
}

int dma2d_fill_test(bk_dma2d_ctlr_handle_t handle, const char *format, uint32_t color,
                   uint16_t frame_width, uint16_t frame_height,
                   uint16_t xpos, uint16_t ypos,
                   uint16_t fill_width, uint16_t fill_height)
{
    avdk_err_t ret = AVDK_ERR_OK;
    out_color_mode_t color_format;
    uint8_t pixel_byte;
    
    if (os_strcmp(format, "ARGB8888") == 0) {
        color_format = DMA2D_OUTPUT_ARGB8888;
        pixel_byte = 4;
    } else if (os_strcmp(format, "RGB888") == 0) {
        color_format = DMA2D_OUTPUT_RGB888;
        pixel_byte = 3;
    } else {
        color_format = DMA2D_OUTPUT_RGB565;
        pixel_byte = 2;
    }
    
    LOGI("%s fill information \n", __func__);
    LOGI("color_format %d, color %x, frame_width %d, frame_height %d, xpos %d, ypos %d, fill_width %d, fill_height %d,\n", 
        color_format, color, frame_width, frame_height, xpos, ypos, fill_width, fill_height);

    frame_buffer_t *dma2d_frame = frame_buffer_display_malloc(frame_width * frame_height * pixel_byte);
    AVDK_RETURN_ON_FALSE(dma2d_frame, AVDK_ERR_NOMEM, TAG, "frame_buffer_display_malloc failed! \n");
    os_memset((void *)dma2d_frame->frame, 0, frame_width * frame_height * pixel_byte);

    dma2d_fill_config_t fill_config = {0};
    fill_config.fill.frameaddr = dma2d_frame->frame;
    fill_config.fill.color = color;
    fill_config.fill.color_format = color_format;
    fill_config.fill.pixel_byte = pixel_byte;
    fill_config.fill.frame_xsize = frame_width;
    fill_config.fill.frame_ysize = frame_height;
    fill_config.fill.xpos = xpos;
    fill_config.fill.ypos = ypos;
    fill_config.fill.width = fill_width;
    fill_config.fill.height = fill_height;
    fill_config.transfer_complete_cb = bk_dma2d_fill_complete_cb;
    fill_config.is_sync = true;
    fill_config.user_data = dma2d_frame;

    ret = bk_dma2d_fill(handle, &fill_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_fill failed!\n");
        frame_buffer_display_free(dma2d_frame);
        return ret;
    }
    frame_buffer_display_free(dma2d_frame);
    return ret;
}