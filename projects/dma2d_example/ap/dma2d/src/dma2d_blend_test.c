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

void bk_dma2d_blend_complete_cb(dma2d_trans_status_t status, void *user_data)
{
    LOGD("bk_dma2d_blend_complete_cb, status = %d \n", status);
}

int dma2d_blend_test(bk_dma2d_ctlr_handle_t handle, const char *fg_format, const char *bg_format,
                    const char *output_format, uint32_t bg_color, uint32_t fg_color,
                    uint16_t bg_width, uint16_t bg_height,
                    uint16_t fg_width, uint16_t fg_height,
                    uint16_t dst_width, uint16_t dst_height,
                    uint16_t bg_frame_xpos, uint16_t bg_frame_ypos,
                    uint16_t fg_frame_xpos, uint16_t fg_frame_ypos,
                    uint16_t dst_frame_xpos, uint16_t dst_frame_ypos,
                    uint16_t dma2d_width, uint16_t dma2d_height,
                    uint8_t fg_alpha_value)
{
    avdk_err_t ret = AVDK_ERR_OK;
    input_color_mode_t input_fg_mode, input_bg_mode;
    out_color_mode_t output_mode;
    uint8_t fg_pixel_byte, bg_pixel_byte, dst_pixel_byte;

    if (os_strcmp(fg_format, "ARGB8888") == 0) {
        input_fg_mode = DMA2D_INPUT_ARGB8888;
        fg_pixel_byte = 4;
    } else if (os_strcmp(fg_format, "RGB888") == 0) {
        input_fg_mode = DMA2D_INPUT_RGB888;
        fg_pixel_byte = 3;
    } else {
        input_fg_mode = DMA2D_INPUT_RGB565;
        fg_pixel_byte = 2;
    }
    
    if (os_strcmp(bg_format, "ARGB8888") == 0) {
        input_bg_mode = DMA2D_INPUT_ARGB8888;
        bg_pixel_byte = 4;
    } else if (os_strcmp(bg_format, "RGB888") == 0) {
        input_bg_mode = DMA2D_INPUT_RGB888;
        bg_pixel_byte = 3;
    } else {
        input_bg_mode = DMA2D_INPUT_RGB565;
        bg_pixel_byte = 2;
    }
    
    if (os_strcmp(output_format, "ARGB8888") == 0) {
        output_mode = DMA2D_OUTPUT_ARGB8888;
        dst_pixel_byte = 4;
    } else if (os_strcmp(output_format, "RGB888") == 0) {
        output_mode = DMA2D_OUTPUT_RGB888;
        dst_pixel_byte = 3;
    } else {
        output_mode = DMA2D_OUTPUT_RGB565;
        dst_pixel_byte = 2;
    }

    frame_buffer_t *bg_frame = frame_buffer_display_malloc(bg_width * bg_height * bg_pixel_byte);
    AVDK_RETURN_ON_FALSE(bg_frame, AVDK_ERR_NOMEM, TAG, "frame_buffer_display_malloc failed! \n");
    
    frame_buffer_t *fg_frame = frame_buffer_display_malloc(fg_width * fg_height * fg_pixel_byte);
    if (!fg_frame) {
        LOGE("frame_buffer_display_malloc failed! \n");
        frame_buffer_display_free(bg_frame);
        return AVDK_ERR_NOMEM;
    }
    
    frame_buffer_t *dst_frame = frame_buffer_display_malloc(dst_width * dst_height * dst_pixel_byte);
    if (!dst_frame) {
        LOGE("frame_buffer_display_malloc failed! \n");
        frame_buffer_display_free(bg_frame);
        frame_buffer_display_free(fg_frame);
        return AVDK_ERR_NOMEM;
    }

    LOGI("%s blend information \n", __func__);
    LOGI("bg_color %x, fg_color %x, bg_width: %d, bg_height: %d, fg_width: %d, fg_height: %d, dst_width: %d, dst_height: %d, bg_frame_xpos: %d, bg_frame_ypos: %d, fg_frame_xpos: %d, fg_frame_ypos: %d, dma2d_width: %d, dma2d_height: %d \n",
        bg_color, fg_color, bg_width, bg_height, fg_width, fg_height, dst_width, dst_height,
        bg_frame_xpos, bg_frame_ypos, fg_frame_xpos, fg_frame_ypos, dma2d_width, dma2d_height);
    
    for (int i = 0; i < bg_width * bg_height; i++) {
        ((uint16_t *)bg_frame->frame)[i] = bg_color;
    }
    
    for (int i = 0; i < fg_width * fg_height; i++) {
        ((uint16_t *)fg_frame->frame)[i] = fg_color;
    }
    
    dma2d_blend_config_t blend_config = {0};
    //fg config
    blend_config.blend.pfg_addr = (char *)fg_frame->frame;
    blend_config.blend.fg_frame_width = fg_width;
    blend_config.blend.fg_frame_height = fg_height;
    blend_config.blend.fg_frame_xpos = fg_frame_xpos;
    blend_config.blend.fg_frame_ypos = fg_frame_ypos;
    blend_config.blend.fg_color_mode = input_fg_mode;
    blend_config.blend.fg_pixel_byte = fg_pixel_byte;

    blend_config.blend.pbg_addr = (char *)bg_frame->frame;
    blend_config.blend.bg_frame_width = bg_width;
    blend_config.blend.bg_frame_height = bg_height;
    blend_config.blend.bg_frame_xpos = bg_frame_xpos;
    blend_config.blend.bg_frame_ypos = bg_frame_ypos;
    blend_config.blend.bg_color_mode = input_bg_mode;
    blend_config.blend.bg_pixel_byte = bg_pixel_byte;

    blend_config.blend.pdst_addr = (char *)dst_frame->frame;
    blend_config.blend.dst_frame_width = dst_width;
    blend_config.blend.dst_frame_height = dst_height;
    blend_config.blend.dst_frame_xpos = dst_frame_xpos;
    blend_config.blend.dst_frame_ypos = dst_frame_ypos;
    blend_config.blend.dst_color_mode = output_mode;
    blend_config.blend.dst_pixel_byte = dst_pixel_byte;

    blend_config.blend.dma2d_width = dma2d_width;
    blend_config.blend.dma2d_height = dma2d_height;
    
    blend_config.blend.fg_alpha_value = fg_alpha_value;
    blend_config.blend.fg_alpha_mode = DMA2D_NO_MODIF_ALPHA;
    blend_config.blend.bg_alpha_mode = DMA2D_REPLACE_ALPHA;
    blend_config.blend.fg_red_blue_swap = DMA2D_RB_REGULAR;
    blend_config.blend.bg_red_blue_swap = DMA2D_RB_REGULAR;
    blend_config.blend.dst_red_blue_swap = DMA2D_RB_REGULAR;

    blend_config.transfer_complete_cb = bk_dma2d_blend_complete_cb;
    blend_config.is_sync = true;
    
    ret = bk_dma2d_blend(handle, &blend_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_blend failed! \n");
        return ret;
    }

    frame_buffer_display_free(bg_frame);
    frame_buffer_display_free(fg_frame);
    frame_buffer_display_free(dst_frame);
    return ret;
}