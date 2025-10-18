#include <os/os.h>
#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <os/str.h>
#include "dma2d_test.h"
#include "components/bk_display.h"
#include "components/bk_draw_osd_types.h"
#include "blend_assets/blend.h"
#include "components/bk_dma2d.h"
#include "frame_buffer.h"

#define TAG "blend_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void bk_mem_dump_ex(const char * title, unsigned char * data, uint32_t data_len);
extern bk_display_ctlr_handle_t lcd_display_handle;

static avdk_err_t display_frame_free_cb(void *frame)
{
    LOGI("display_frame_free_cb, frame = %p\n", frame);
    frame_buffer_display_free((frame_buffer_t *)frame);
    return AVDK_ERR_OK;
}

void bk_dma2d_blend_complete_cb(dma2d_trans_status_t status, void *user_data)
{
    GPIO_UP(GPIO_5);
    GPIO_DOWN(GPIO_5);
    LOGD("bk_dma2d_blend_complete_cb, status = %d \n", status);
}

/**
 * @brief 直接使用dma2d_blend_config_t进行图像混合
 * 
 * @param handle DMA2D控制器句柄
 * @param frame 帧缓冲区
 * @param lcd_width LCD宽度
 * @param lcd_height LCD高度
 * @param img_info 图像信息
 * @return bk_err_t 
 */
bk_err_t bk_dma2d_image_blend(bk_dma2d_ctlr_handle_t handle, frame_buffer_t *frame, uint16_t lcd_width, uint16_t lcd_height, const bk_blend_t *img_info, bool is_sync)
{
    avdk_err_t ret = AVDK_ERR_OK;
    if ((frame == NULL) || (img_info == NULL))
    {
        return BK_FAIL;
    }
    
    const bk_blend_t *img_dsc = (bk_blend_t *)img_info;
    if ((img_info->width + img_info->xpos > lcd_width) || (img_info->height + img_info->ypos > lcd_height))
    {
        LOGW("%s %d img size is beyond the boundaries of lcd\n", __func__, __LINE__);
        if (img_dsc->width + img_dsc->xpos > lcd_width)
            LOGI("content: %s, xpos %d + width %d > lcd_width %d\n", __func__, img_dsc->xpos, img_dsc->width, lcd_width);
        if (img_dsc->height + img_dsc->ypos > lcd_height)
            LOGI("content: %s, ypos %d + height %d > lcd_width %d\n", __func__, img_dsc->ypos, img_dsc->height, lcd_height);

        return BK_FAIL;
    }
    
    uint16_t lcd_start_x = 0;
    uint16_t lcd_start_y = 0;
    if ((lcd_width < frame->width) || (lcd_height < frame->height)) //for lcd size is small then frame image size
    {
        if (lcd_width < frame->width)
        {
            lcd_start_x = (frame->width - lcd_width) / 2;
        }
        if (lcd_height < frame->height)
        {
            lcd_start_y = (frame->height - lcd_height) / 2;
        }
    }
    
    LOGI("start: bg_width %d, bg_height %d, pos(%d, %d), xsize %d, ysize %d\n", 
            frame->width, frame->height, img_dsc->xpos, img_dsc->ypos, img_dsc->width, img_dsc->height);
            
    dma2d_blend_config_t blend_config = {0};
    
    blend_config.blend.pfg_addr = (char *)img_dsc->image.data;
    blend_config.blend.pbg_addr = (char *)(frame->frame);
    blend_config.blend.pdst_addr = (char *)(frame->frame);
    blend_config.blend.fg_color_mode = DMA2D_INPUT_ARGB8888;
    
    switch (frame->fmt)
    {
        case PIXEL_FMT_YUYV:
            blend_config.blend.bg_color_mode = DMA2D_INPUT_YUYV;
            blend_config.blend.dst_color_mode = DMA2D_OUTPUT_YUYV;
            break;
        case PIXEL_FMT_VUYY:
            blend_config.blend.bg_color_mode = DMA2D_INPUT_VUYY;
            blend_config.blend.dst_color_mode = DMA2D_OUTPUT_YUYV;  
            break;
        case PIXEL_FMT_RGB888:
            blend_config.blend.bg_color_mode = DMA2D_INPUT_RGB888;
            blend_config.blend.dst_color_mode = DMA2D_OUTPUT_RGB888;
            break;
        case PIXEL_FMT_RGB565:
        default:
            blend_config.blend.bg_color_mode = DMA2D_INPUT_RGB565;
            blend_config.blend.dst_color_mode = DMA2D_OUTPUT_RGB565;
            break;
    }
    
    blend_config.blend.fg_red_blue_swap = DMA2D_RB_SWAP;   
    blend_config.blend.bg_red_blue_swap = DMA2D_RB_REGULAR;
    blend_config.blend.dst_red_blue_swap = DMA2D_RB_REGULAR;

    blend_config.blend.fg_frame_width = img_dsc->width;
    blend_config.blend.fg_frame_height = img_dsc->height;
    blend_config.blend.bg_frame_width = frame->width;
    blend_config.blend.bg_frame_height = frame->height;
    blend_config.blend.dst_frame_width = frame->width;
    blend_config.blend.dst_frame_height = frame->height;

    blend_config.blend.fg_frame_xpos = 0;
    blend_config.blend.fg_frame_ypos = 0;
    blend_config.blend.bg_frame_xpos = lcd_start_x + img_dsc->xpos;
    blend_config.blend.bg_frame_ypos = lcd_start_y + img_dsc->ypos;
    blend_config.blend.dst_frame_xpos = lcd_start_x + img_dsc->xpos;
    blend_config.blend.dst_frame_ypos = lcd_start_y + img_dsc->ypos;

    blend_config.blend.fg_pixel_byte = FOUR_BYTES;
    
    switch (frame->fmt)
    {
        case PIXEL_FMT_ARGB8888:
            blend_config.blend.bg_pixel_byte = FOUR_BYTES;
            blend_config.blend.dst_pixel_byte = FOUR_BYTES;
            break;
        case PIXEL_FMT_RGB888:
            blend_config.blend.bg_pixel_byte = THREE_BYTES;
            blend_config.blend.dst_pixel_byte = THREE_BYTES;
            break;
        case PIXEL_FMT_RGB565:
        default:
            blend_config.blend.bg_pixel_byte = TWO_BYTES;
            blend_config.blend.dst_pixel_byte = TWO_BYTES;
            break;
    }

    blend_config.blend.dma2d_width = img_dsc->width;
    blend_config.blend.dma2d_height = img_dsc->height;
    blend_config.blend.fg_alpha_mode = DMA2D_NO_MODIF_ALPHA;
    blend_config.blend.bg_alpha_mode = DMA2D_NO_MODIF_ALPHA;

    blend_config.transfer_complete_cb = bk_dma2d_blend_complete_cb;
    blend_config.is_sync = is_sync;
    ret = bk_dma2d_blend(handle, &blend_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_blend failed! \n");
        return BK_FAIL;
    }

    return BK_OK;
}

/**
 * @brief 使用新的bk_dma2d_image_blend函数的测试API
 * 
 * @param handle DMA2D控制器句柄
 * @param bg_format 背景格式
 * @param bg_color 背景颜色
 * @param bg_width 背景宽度
 * @param bg_height 背景高度
 * @return int 
 */
int dma2d_blend_test(bk_dma2d_ctlr_handle_t handle, const char *bg_format, uint32_t bg_color,
                    uint16_t bg_width, uint16_t bg_height, bool is_sync)
{
    LOGE("%s \n", __func__);
    avdk_err_t ret = AVDK_ERR_OK;
    pixel_format_t fmt;
    uint8_t bg_pixel_byte = 0;
    if (os_strcmp(bg_format, "ARGB8888") == 0) {
        bg_pixel_byte = 4;
        fmt = PIXEL_FMT_ARGB8888;
    } else if (os_strcmp(bg_format, "RGB888") == 0) {
        bg_pixel_byte = 3;
        fmt = PIXEL_FMT_RGB888;
    } else {
        bg_pixel_byte = 2;
        fmt = PIXEL_FMT_RGB565; 
    }
    GPIO_DOWN(GPIO_4);
    GPIO_DOWN(GPIO_5);
    GPIO_DOWN(GPIO_8);
    GPIO_DOWN(GPIO_3);

    frame_buffer_t *bg_frame = frame_buffer_display_malloc(bg_width * bg_height * bg_pixel_byte);
    AVDK_RETURN_ON_FALSE(bg_frame, AVDK_ERR_NOMEM, TAG, "frame_buffer_display_malloc failed! \n");

    // Fill background frame based on input_bg_mode
    for (int i = 0; i < bg_width * bg_height; i++) {
        if (bg_pixel_byte == 4) {
            ((uint32_t *)bg_frame->frame)[i] = bg_color;
        } else if (bg_pixel_byte == 3) {
            uint8_t *pixel = (uint8_t *)bg_frame->frame + i * 3;
            pixel[0] = (bg_color & 0xFF);         // Blue
            pixel[1] = (bg_color & 0xFF00) >> 8;  // Green
            pixel[2] = (bg_color & 0xFF0000) >> 16; // Red
        } else { // RGB565
            ((uint16_t *)bg_frame->frame)[i] = bg_color;
        }
    }

    bg_frame->width = bg_width;
    bg_frame->height = bg_height;
    bg_frame->fmt = fmt;

    GPIO_UP(GPIO_4);
    // 使用新的bk_dma2d_image_blend2函数
    ret = bk_dma2d_image_blend(handle, bg_frame, 480, 854, &img_cloudy_to_sunny, is_sync);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_image_blend2 failed! \n");
        return ret;
    }
    GPIO_DOWN(GPIO_4);
    GPIO_UP(GPIO_4);
    ret = bk_dma2d_image_blend(handle, bg_frame, 480, 854, &img_wifi_rssi0, is_sync);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_image_blend2 failed! \n");
        return ret;
    }
    GPIO_DOWN(GPIO_4);
    GPIO_UP(GPIO_4);
    ret = bk_dma2d_image_blend(handle, bg_frame, 480, 854, &img_battery1, is_sync);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_dma2d_image_blend2 failed! \n");
        return ret;
    }
    GPIO_DOWN(GPIO_4);
    ret = bk_display_flush(lcd_display_handle, bg_frame, display_frame_free_cb);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_flush failed!\n");
        frame_buffer_display_free(bg_frame);
        return ret;
    }
    return ret;
}
