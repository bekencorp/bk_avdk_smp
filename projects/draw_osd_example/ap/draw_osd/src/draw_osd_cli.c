#include <os/os.h>
#include <driver/gpio.h>
#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include "components/bk_draw_osd.h"
#include "components/bk_display.h"
#include "gpio_driver.h"
#include "frame_buffer.h"
#include "lcd_panel_devices.h"
#include "draw_osd_test.h"
#include "blend.h"
#include "cli.h"
#include "driver/pwr_clk.h"

#define TAG "draw_osd"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_draw_osd_ctlr_handle_t draw_osd_handle = NULL;
static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static const lcd_device_t *lcd_device =  &lcd_device_st7701sn;
static frame_buffer_t *bg_frame = NULL;

#define BL_PIN  GPIO_7
#define LCD_LDO_PIN GPIO_13

static avdk_err_t display_frame_free_cb(void *frame)
{
    LOGI("display_frame_free_cb, frame = %p\n", frame);
    frame_buffer_display_free((frame_buffer_t *)frame);
    return AVDK_ERR_OK;
}

void cli_draw_osd_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;
    int i = 0;

    // check arguments
    if (argc < 2) {
        LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
        goto exit;
    }

    if (strcmp(argv[1], "init") == 0)
    {
        osd_ctlr_config_t osd_cfg = {0}; 
        osd_cfg.blend_assets =blend_assets;  /**<all assets */
        osd_cfg.blend_info = blend_info;     /**<real display */
        osd_cfg.draw_in_psram = false;
        ret = bk_draw_osd_new(&draw_osd_handle, &osd_cfg);

        bk_display_rgb_ctlr_config_t lcd_display_config = {0};
        lcd_display_config.lcd_device = lcd_device;
        lcd_display_config.clk_pin = GPIO_0;
        lcd_display_config.cs_pin = GPIO_12;
        lcd_display_config.sda_pin = GPIO_1;
        lcd_display_config.rst_pin = GPIO_6;
        ret = bk_display_rgb_new(&lcd_display_handle, &lcd_display_config);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_rgb_new failed!\n");
        LOGD("bk_display_rgb_new success!\n");


        gpio_dev_unmap(BL_PIN);
        BK_LOG_ON_ERR(bk_gpio_enable_output(BL_PIN));
        bk_gpio_set_output_high(BL_PIN);

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);

        ret = bk_display_open(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_open failed!\n");
    }
    else if (strcmp(argv[1], "deinit") == 0)
    {
        ret = bk_draw_osd_delete(draw_osd_handle);
        draw_osd_handle  = NULL;
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_draw_osd_delete failed!\n");
        ret = bk_display_close(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_close failed!\n");
        ret = bk_display_delete(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_delete failed!\n");
        lcd_display_handle = NULL;
        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
        bk_gpio_set_output_low(BL_PIN);
    }
    else if (strcmp(argv[1], "array") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(draw_osd_handle, TAG, "draw_osd_handle is NULL!");
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");

        uint16_t bg_frame_width = 480;
        uint16_t bg_frame_height = 864;
        uint32_t len = bg_frame_width * bg_frame_height * 2;

        bg_frame = frame_buffer_display_malloc(len);
        AVDK_GOTO_VOID_ON_FALSE(bg_frame, exit, TAG, "frame_buffer_display_malloc failed!\n");
        for( i = 0; i < len; i+=2)
        {
            *(uint16_t *)(bg_frame->frame + i) = 0x00;
        }
        bg_frame->fmt = PIXEL_FMT_RGB565_LE;
        bg_frame->width = bg_frame_width;
        bg_frame->height = bg_frame_height;

        if (argv[2] != NULL && strcmp(argv[2], "updata") == 0)
        {
            if (argv[3] == NULL)
            {
                LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
                goto exit;
            }
            ret = bk_draw_osd_add_or_updata(draw_osd_handle, argv[3], argv[4]);
        }
        else if (argv[2] != NULL && strcmp(argv[2], "remove") == 0)
        {
            if (argv[3] == NULL)
            {
                LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
                goto exit;
            }
            ret = bk_draw_osd_remove(draw_osd_handle, argv[3]);
        }

        osd_bg_info_t bg_info = {0};
        bg_info.frame = bg_frame;
        bg_info.width = lcd_device->width;
        bg_info.height = lcd_device->height;
        ret = bk_draw_osd_array(draw_osd_handle, &bg_info, NULL);

        if (bg_frame != NULL)
        {
            ret = bk_display_flush(lcd_display_handle, bg_frame, display_frame_free_cb);
            if (ret != BK_OK)
            {
                display_frame_free_cb(bg_frame);
                bg_frame = NULL;
                LOGE("bk_display_flush failed\n");
            }
        }
    }
    else if (strcmp(argv[1], "get_info") == 0)
    {
        uint32_t is_printf = 1;
        const blend_info_t *resources = NULL;
        uint32_t size = 0;
        if (argc > 2 && strcmp(argv[2], "no_print") == 0) {
            is_printf = 0;
        }
        LOGI("get current draw info (print mode: %s)\n", is_printf ? "open" : "close");  
        ret = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_DRAW_INFO, is_printf, (uint32_t)&resources, (uint32_t)&size);
    }
    else if (strcmp(argv[1], "get_assets") == 0)
    {
        uint32_t is_printf = 1;
        if (argc > 2 && strcmp(argv[2], "no_print") == 0) {
            is_printf = 0;
        }
        LOGI("get all available assets (print mode: %s)\n", is_printf ? "open" : "close");
        ret = bk_draw_osd_ioctl(draw_osd_handle, OSD_CTLR_CMD_GET_ALL_ASSETS, is_printf, 0, 0);
    }
    else if (strcmp(argv[1], "img") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(draw_osd_handle, TAG, "draw_osd_handle is NULL!");
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");
        uint16_t bg_frame_width = 480;
        uint16_t bg_frame_height = 864;

        uint32_t len = bg_frame_width * bg_frame_height * 2;
        bg_frame = frame_buffer_display_malloc(len);
        AVDK_GOTO_VOID_ON_FALSE(bg_frame, exit, TAG, "frame_buffer_display_malloc failed!\n");

        bg_frame->fmt = PIXEL_FMT_RGB565_LE;
        bg_frame->width = bg_frame_width;
        bg_frame->height = bg_frame_height;
        for( i = 0; i < bg_frame->width * bg_frame->height * 2; i+=2)
        {
            *(uint16_t *)(bg_frame->frame + i) = 0x001f;
        }

        osd_bg_info_t bg_info = {0};
        bg_info.frame = bg_frame;
        bg_info.width = lcd_device->width;
        bg_info.height = lcd_device->height;

        blend_info_t wifi_info = {.name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0"};
        ret = bk_draw_osd_image(draw_osd_handle, &bg_info, &wifi_info);
        if (bg_frame != NULL)
        {
            ret = bk_display_flush(lcd_display_handle, bg_frame, display_frame_free_cb);
            if (ret != BK_OK)
            {
                display_frame_free_cb(bg_frame);
                bg_frame = NULL;
                LOGE("bk_display_flush failed\n");
            }
        }
    }
    else if (strcmp(argv[1], "font") == 0)
    {    
        AVDK_RETURN_VOID_ON_FALSE(draw_osd_handle, TAG, "draw_osd_handle is NULL!");
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");  
        uint16_t bg_frame_width = 480;
        uint16_t bg_frame_height = 864;

        uint32_t len = bg_frame_width * bg_frame_height * 2;
        bg_frame = frame_buffer_display_malloc(len);
        AVDK_GOTO_VOID_ON_FALSE(bg_frame, exit, TAG, "frame_buffer_display_malloc failed!\n");

        bg_frame->fmt = PIXEL_FMT_RGB565_LE;
        bg_frame->width = bg_frame_width;
        bg_frame->height = bg_frame_height;
        for( i = 0; i < bg_frame->width * bg_frame->height * 2; i+=2)
        {
            *(uint16_t *)(bg_frame->frame + i) = 0x001f;
        }

        osd_bg_info_t bg_info = {0};
        bg_info.frame = bg_frame;
        bg_info.width = lcd_device->width;
        bg_info.height = lcd_device->height;

        blend_info_t font_info = {.name = "clock", .addr = &font_clock, .content = "12:68"};
        ret = bk_draw_osd_font(draw_osd_handle, &bg_info, &font_info);
        if (bg_frame != NULL)
        {
            ret = bk_display_flush(lcd_display_handle, bg_frame, display_frame_free_cb);
            if (ret != BK_OK)
            {
                display_frame_free_cb(bg_frame);
                bg_frame = NULL;
                LOGE("bk_display_flush failed\n");
            }
        }
    }
    else
    {
        LOGE("%s, %d: invalid arguments\n", __func__, __LINE__);
    }

exit:
    if (ret != BK_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define CMDS_COUNT  (sizeof(s_draw_osd_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_draw_osd_test_commands[] =
{
    {"osd", "init | assets | img | font | deinit", cli_draw_osd_test_cmd},
};

int cli_draw_osd_test_init(void)
{
    return cli_register_commands(s_draw_osd_test_commands, CMDS_COUNT);
}
