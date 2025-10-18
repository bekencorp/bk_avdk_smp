#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "components/bk_display.h"
#include "gpio_driver.h"
#include "frame_buffer.h"
#include "lcd_panel_devices.h"
#include <components/avdk_utils/avdk_error.h>
#include "driver/pwr_clk.h"
#define TAG "rgb_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LCD_LDO_PIN         (GPIO_13)

static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static const lcd_device_t *lcd_device =  &lcd_device_st7796s;
static void lcd_fill_rand_color(uint32_t len, uint8_t *addr)
{
    uint32_t color_rand = 0;
    uint8_t *p_addr = addr;
    int i = 0;

    color_rand = (uint32_t)rand();

    for( i = 0; i < len; i+=2)
    {
        *(uint16_t *)(p_addr + i) = color_rand;
    }
}

static avdk_err_t lcd_backlight_open(uint8_t bl_io)
{
    gpio_dev_unmap(bl_io);
    BK_LOG_ON_ERR(bk_gpio_enable_output(bl_io));
    BK_LOG_ON_ERR(bk_gpio_pull_up(bl_io));
    bk_gpio_set_output_high(bl_io);
    return AVDK_ERR_OK;
}


static avdk_err_t lcd_backlight_close(uint8_t bl_io)
{
    BK_LOG_ON_ERR(bk_gpio_pull_down(bl_io));
    bk_gpio_set_output_low(bl_io);
    return AVDK_ERR_OK;
}

static avdk_err_t display_frame_free_cb(void *frame)
{
    frame_buffer_display_free((frame_buffer_t *)frame);
    return AVDK_ERR_OK;
}

static void cli_lcd_display_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_OK;
    if (os_strcmp(argv[1], "init") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(!lcd_display_handle, TAG, "lcd_display_handle not NULL,may not delete last time!");

        bk_display_mcu_ctlr_config_t lcd_display_config = {0};
        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
        lcd_backlight_open(GPIO_7);
        lcd_display_config.lcd_device = lcd_device;
        ret = bk_display_mcu_new(&lcd_display_handle, &lcd_display_config);
        AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_display_mcu_new failed!");
        LOGD("bk_display_mcu_new success!");
    }
    else if (os_strcmp(argv[1], "delete") == 0)
    {
        ret = bk_display_delete(lcd_display_handle);
        AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_display_delete failed!");
        lcd_display_handle = NULL;
        LOGD("bk_display_delete success!");
        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
    }
    else if (os_strcmp(argv[1], "open") == 0)
    {
        ret = bk_display_open(lcd_display_handle);
        AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_display_open failed!");
        LOGD("bk_display_open success!");
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        ret = bk_display_close(lcd_display_handle);
        AVDK_RETURN_VOID_ON_ERROR(ret, TAG, "bk_display_close failed!");
        lcd_backlight_close(GPIO_7);
        LOGD("bk_display_close success!");
    }
    else if (os_strcmp(argv[1], "flush") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(lcd_display_handle, TAG, "lcd_display_handle is NULL!");
        uint32_t frame_cnt = 10;
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
            AVDK_RETURN_VOID_ON_FALSE(disp_frame, TAG, "frame malloc failed,flush failed!");

            disp_frame->fmt = PIXEL_FMT_RGB565;
            disp_frame->width = lcd_device->width;
            disp_frame->height = lcd_device->height;
            lcd_fill_rand_color(jpeg_len, disp_frame->frame);
            ret = bk_display_flush(lcd_display_handle, disp_frame, display_frame_free_cb);
           if (ret != AVDK_ERR_OK)
            {
                display_frame_free_cb(disp_frame);
                LOGE("bk_display_flush failed!\n");
                return;
            }
            rtos_delay_milliseconds(300);
        }
        LOGD("bk_display_flush frame success!");
    }
    else
    {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }
}

static void cli_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    char *msg = NULL;
    if (os_strcmp(argv[1], "open") == 0)
    {
        AVDK_RETURN_VOID_ON_FALSE(!lcd_display_handle, TAG, "lcd_display_handle not NULL,may not delete last time!");

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
        lcd_backlight_open(GPIO_7);
        bk_display_mcu_ctlr_config_t lcd_display_config = {0};
        lcd_display_config.lcd_device = lcd_device;
        ret = bk_display_mcu_new(&lcd_display_handle, &lcd_display_config);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_mcu_new failed!\n");
        LOGD("bk_display_mcu_new success!\n");

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
        LOGD("bk_display_delete success!\n");
        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
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

#define CMDS_COUNT  (sizeof(s_lcd_display_commands) / sizeof(struct cli_command))

static const struct cli_command s_lcd_display_commands[] =
{
    {"lcd_display_api", "init/ open/close/flush cnt /delete", cli_lcd_display_api_cmd},
    {"lcd_display", " open/close", cli_lcd_display_cmd},
};

int cli_lcd_display_init(void)
{
    return cli_register_commands(s_lcd_display_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();
    media_service_init();
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
    cli_lcd_display_init();
    return 0;
}
