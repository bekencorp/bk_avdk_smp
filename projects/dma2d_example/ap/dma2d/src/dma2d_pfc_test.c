#include <os/os.h>
#include <components/avdk_utils/avdk_error.h>
#include <os/str.h>
#include "dma2d_test.h"
#include "frame_buffer.h"
#define TAG "dma2d_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void bk_mem_dump_ex(const char * title, unsigned char * data, uint32_t data_len);

void bk_dma2d_pfc_complete_cb(dma2d_trans_status_t status, void *user_data)
{
    LOGD("bk_dma2d_pfc_complete_cb, status = %d \n", status);
}

int dma2d_pfc_test(bk_dma2d_ctlr_handle_t handle, const char *input_format, const char *output_format,
                  uint32_t color, uint16_t src_width, uint16_t src_height,
                  uint16_t dst_width, uint16_t dst_height,
                  uint16_t src_frame_xpos, uint16_t src_frame_ypos,
                  uint16_t dst_frame_xpos, uint16_t dst_frame_ypos,
                  uint16_t dma2d_width, uint16_t dma2d_height)
{
    avdk_err_t ret = AVDK_ERR_OK;
    input_color_mode_t input_color_mode;
    out_color_mode_t output_color_mode;
    uint8_t src_pixel_byte, dst_pixel_byte;
    
    if (os_strcmp(input_format, "ARGB8888") == 0) {
        input_color_mode = DMA2D_INPUT_ARGB8888;
        src_pixel_byte = 4;
    } else if (os_strcmp(input_format, "RGB888") == 0) {
        input_color_mode = DMA2D_INPUT_RGB888;
        src_pixel_byte = 3;
    } else {
        input_color_mode = DMA2D_INPUT_RGB565;
        src_pixel_byte = 2;
    }
    
    if (os_strcmp(output_format, "ARGB8888") == 0) {
        output_color_mode = DMA2D_OUTPUT_ARGB8888;
        dst_pixel_byte = 4;
    } else if (os_strcmp(output_format, "RGB888") == 0) {
        output_color_mode = DMA2D_OUTPUT_RGB888;
        dst_pixel_byte = 3;
    } else {
        output_color_mode = DMA2D_OUTPUT_RGB565;
        dst_pixel_byte = 2;
    }
    
    LOGI("%s pfc information \n", __func__);
    LOGI("input_color_mode %d, output_color_mode %d, color %x, src_width: %d, src_height: %d, dst_width: %d, dst_height: %d, src_frame_xpos: %d, src_frame_ypos: %d, dst_frame_xpos: %d, dst_frame_ypos: %d, dma2d_width: %d, dma2d_height: %d \n",
        input_color_mode, output_color_mode, color, src_width, src_height, dst_width, dst_height,
        src_frame_xpos, src_frame_ypos, dst_frame_xpos, dst_frame_ypos, dma2d_width, dma2d_height);

    dma2d_pfc_memcpy_config_t pfc_config = {0};
    frame_buffer_t *src_frame = frame_buffer_display_malloc(src_width * src_height * src_pixel_byte);
    AVDK_RETURN_ON_FALSE(src_frame, AVDK_ERR_NOMEM, TAG, "frame_buffer_display_malloc failed! \n");
    
    frame_buffer_t *dst_frame = frame_buffer_display_malloc(dst_width * dst_height * dst_pixel_byte);
    if (!dst_frame) {
        LOGE("frame_buffer_display_malloc failed! \n");
        frame_buffer_display_free(src_frame);
        return AVDK_ERR_NOMEM;
    }

    os_memset((uint16_t *)src_frame->frame, color, src_width * src_height * src_pixel_byte);
    for (int i = 0; i < src_width * src_height; i++) {
        ((uint16_t *)src_frame->frame)[i] = color;
    }
    os_memset((void *)dst_frame->frame, 0, dst_width * dst_height * dst_pixel_byte);

    //src frame config
    pfc_config.pfc.input_addr = (char *)src_frame->frame;
    pfc_config.pfc.src_frame_width = src_width;
    pfc_config.pfc.src_frame_height = src_height;
    pfc_config.pfc.src_frame_xpos = src_frame_xpos;
    pfc_config.pfc.src_frame_ypos = src_frame_ypos;
    pfc_config.pfc.input_color_mode = input_color_mode;
    pfc_config.pfc.src_pixel_byte = src_pixel_byte;
    
    //dst frame config
    pfc_config.pfc.output_addr = (char *)dst_frame->frame;
    pfc_config.pfc.dst_frame_width = dst_width;
    pfc_config.pfc.dst_frame_height = dst_height;
    pfc_config.pfc.dst_frame_xpos = dst_frame_xpos;
    pfc_config.pfc.dst_frame_ypos = dst_frame_ypos;
    pfc_config.pfc.output_color_mode = output_color_mode;
    pfc_config.pfc.dst_pixel_byte = dst_pixel_byte;
    
    //dma2d memcpy config
    pfc_config.pfc.dma2d_width = dma2d_width;
    pfc_config.pfc.dma2d_height = dma2d_height;

    //memcpy complete cb
    pfc_config.transfer_complete_cb = bk_dma2d_pfc_complete_cb;
    pfc_config.is_sync = true;
    
    ret = bk_dma2d_pixel_conversion(handle, &pfc_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_pixel_conversion failed! \n");
        return ret;
    }

    frame_buffer_display_free(src_frame);
    frame_buffer_display_free(dst_frame);
    return ret;
}