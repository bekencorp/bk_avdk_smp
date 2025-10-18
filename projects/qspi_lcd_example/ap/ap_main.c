#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include "cli.h"
#include "components/bk_display.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "frame_buffer.h"
#include "lcd_panel_devices.h"
#include "driver/pwr_clk.h"
#include <components/avdk_utils/avdk_error.h>

#define TAG "qspi_lcd_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LCD_LDO_PIN          (GPIO_13)

static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static frame_buffer_t *disp_frame = NULL;
bk_display_qspi_ctlr_config_t qspi_ctlr_config = {
    .lcd_device = &lcd_device_st77903_h0165y008t,
    .qspi_id = 0,
    .reset_pin = GPIO_40,
    .te_pin = 0,
};

avdk_err_t lcd_backlight_open(uint8_t bl_io)
{
    gpio_dev_unmap(bl_io);
    BK_LOG_ON_ERR(bk_gpio_enable_output(bl_io));
    BK_LOG_ON_ERR(bk_gpio_pull_up(bl_io));
    bk_gpio_set_output_high(bl_io);

    return AVDK_ERR_OK;
}

avdk_err_t lcd_backlight_close(uint8_t bl_io)
{
    BK_LOG_ON_ERR(bk_gpio_pull_down(bl_io));
    bk_gpio_set_output_low(bl_io);

    return AVDK_ERR_OK;
}

static avdk_err_t display_frame_free_cb(void *frame)
{
    // frame_buffer_display_free((frame_buffer_t *)frame);

    return AVDK_ERR_OK;
}

#define RED_COLOR       0xF800
#define GREEN_COLOR     0x07E0
#define BLUE_COLOR      0x001F

void lcd_qspi_display_fill_pure_color(frame_buffer_t *frame_buffer, uint16_t color)
{
    uint8_t data[2] = {0};

    data[0] = color >> 8;
    data[1] = color;

    for (int i = 0; i < frame_buffer->size; i+=2)
    {
        frame_buffer->frame[i] = data[0];
        frame_buffer->frame[i + 1] = data[1];
    }
}

void cli_qspi_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    char *msg = NULL;

    if (os_strcmp(argv[1], "open") == 0) {
        AVDK_RETURN_VOID_ON_FALSE(!lcd_display_handle, TAG, "lcd_display_handle not NULL, may not delete last time!");

        ret = bk_display_qspi_new(&lcd_display_handle, &qspi_ctlr_config);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_rgb_new failed!\n");
        LOGD("bk_display_qspi_new success!\n");

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
        ret = bk_display_open(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_open failed!\n");
        lcd_backlight_open(GPIO_7);

        uint32_t frame_len = qspi_ctlr_config.lcd_device->width * qspi_ctlr_config.lcd_device->height * 2;
        disp_frame = frame_buffer_display_malloc(frame_len);
        AVDK_GOTO_VOID_ON_FALSE(disp_frame, exit, TAG, "frame malloc failed, flush failed!\n");

        disp_frame->fmt = PIXEL_FMT_RGB565;
        disp_frame->width = qspi_ctlr_config.lcd_device->width;
        disp_frame->height = qspi_ctlr_config.lcd_device->height;

        uint32_t frame_cnt = 20;
        if (argc > 2) {
            frame_cnt = os_strtoul(argv[2], NULL, 10);
        }
        LOGD("display flush will test %d frame! \n", frame_cnt);
        lcd_qspi_display_fill_pure_color(disp_frame, RED_COLOR);

        while (frame_cnt--) {
            ret = bk_display_flush(lcd_display_handle, disp_frame, display_frame_free_cb);
            if (ret != AVDK_ERR_OK) {
                display_frame_free_cb(disp_frame);
                LOGE("bk_display_flush failed!\n");
                goto exit;
            }
            rtos_delay_milliseconds(100);
        }
        LOGD("bk_display_flush frame success!\n");
    } else if (os_strcmp(argv[1], "close") == 0) {
        ret = bk_display_close(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_close failed!\n");
        lcd_backlight_close(GPIO_7);
        LOGD("bk_display_close success!\n");

        ret = bk_display_delete(lcd_display_handle);
        AVDK_GOTO_VOID_ON_FALSE(ret == AVDK_ERR_OK, exit, TAG, "bk_display_delete failed!\n");
        lcd_display_handle = NULL;
        LOGD("bk_display_delete success!\n");

        if (disp_frame) {
            frame_buffer_display_free(disp_frame);
            disp_frame = NULL;
        }

        bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
    } else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

exit:
    if (ret != AVDK_ERR_OK) {
        msg = CLI_CMD_RSP_ERROR;
    }
    else {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}


#define CMDS_COUNT  (sizeof(s_qspi_lcd_display_commands) / sizeof(struct cli_command))

static const struct cli_command s_qspi_lcd_display_commands[] =
{
    {"qspi_lcd_display", "open/close", cli_qspi_lcd_display_cmd},
};

int cli_qspi_lcd_display_init(void)
{
    return cli_register_commands(s_qspi_lcd_display_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    cli_qspi_lcd_display_init();

    return 0;
}
