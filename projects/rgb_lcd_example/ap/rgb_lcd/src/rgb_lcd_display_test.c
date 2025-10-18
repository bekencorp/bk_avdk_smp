#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <os/str.h>
#include "rgb_lcd_test.h"
#include "driver/pwr_clk.h"
#define TAG "rgb_lcd_display"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#define LCD_LDO_PIN          (GPIO_13)
static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static const lcd_device_t *lcd_device =  &lcd_device_st7701sn;
static avdk_err_t display_frame_free_cb(void *frame);
static avdk_err_t display_frame_free_cb(void *frame)
{
    frame_buffer_display_free((frame_buffer_t *)frame);
    return AVDK_ERR_OK;
}

void cli_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    char *msg = NULL;
    if (os_strcmp(argv[1], "open") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(!lcd_display_handle, TAG, "lcd_display_handle not NULL,may not delete last time!");

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);

        bk_display_rgb_ctlr_config_t lcd_display_config = {0};

        lcd_display_config.lcd_device = lcd_device;
        lcd_display_config.clk_pin = GPIO_0;
        lcd_display_config.cs_pin = GPIO_12;
        lcd_display_config.sda_pin = GPIO_1;
        lcd_display_config.rst_pin = GPIO_6;
        ret = bk_display_rgb_new(&lcd_display_handle, &lcd_display_config);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_rgb_new failed!\n");
        LOGD("bk_display_rgb_new success!\n");

        lcd_backlight_open(GPIO_7);
        ret = bk_display_open(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_open failed!\n");

         uint32_t frame_cnt = 20;
        if (argc > 2)
        {
            frame_cnt = os_strtoul(argv[2], NULL, 10);
        }
        LOGD("display flush will test %d frame! \n", frame_cnt);
        while (frame_cnt--)
        {
            frame_buffer_t *disp_frame = NULL;
            uint32_t jpeg_len = lcd_device->width * lcd_device->height * 2;
            disp_frame = frame_buffer_display_malloc(jpeg_len);
            AVDK_GOTO_VOID_ON_FALSE(disp_frame, exit, TAG, "frame malloc failed,flush failed!\n");

            disp_frame->fmt = PIXEL_FMT_RGB565;
            disp_frame->width = lcd_device->width;
            disp_frame->height = lcd_device->height;
            lcd_fill_rand_color(jpeg_len, disp_frame->frame);
            ret = bk_display_flush(lcd_display_handle, disp_frame, display_frame_free_cb);
            if (ret != AVDK_ERR_OK)
            {
                display_frame_free_cb(disp_frame);
                LOGE("bk_display_flush failed!\n");
                goto exit;
            }
            rtos_delay_milliseconds(300);
        }
        LOGD("bk_display_flush frame success!\n");
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        ret = bk_display_close(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_close failed!\n");
        lcd_backlight_close(GPIO_7);
        LOGD("bk_display_close success!\n");

        ret = bk_display_delete(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_delete failed!\n");
        lcd_display_handle = NULL;
        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
        LOGD("bk_display_delete success!\n");
    }
    else
    {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

    exit:
    if (ret != AVDK_ERR_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}