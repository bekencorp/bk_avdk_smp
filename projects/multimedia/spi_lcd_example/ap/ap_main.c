#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "sys_driver.h"
#include "media_service.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <components/bk_lcd_panel.h>

#define TAG "spi_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


extern const bk_lcd_panel_t lcd_device_jd9853;

static bk_display_ctlr_handle_t lcd_display_handle = NULL;
bk_display_spi_bus_config_t spi_ctlr_config = {
    .mode = BK_DISPLAY_SPI_BUS_MODE_HW,
    .lcd_panel = &lcd_device_jd9853,
    .spi_id = 0,
    .dc_pin = GPIO_25,
    .reset_pin = GPIO_26,
    .te_pin = 0,
};
static bool is_display_init = false;

#define RED_COLOR       0xF800
#define GREEN_COLOR     0x07E0
#define BLUE_COLOR      0x001F

static avdk_err_t display_frame_free_cb(void *frame)
{
    // bk_frame_buffer_free((frame);
    return AVDK_ERR_OK;
}

void lcd_spi_display_fill_pure_color(void *frame_buffer, uint32_t frame_len, uint16_t color)
{
    uint8_t data[2] = {0};

    data[0] = color >> 8;
    data[1] = color;

    for (int i = 0; i < frame_len; i+=2)
    {
        ((uint8_t *)frame_buffer)[i] = data[0];
        ((uint8_t *)frame_buffer)[i + 1] = data[1];
    }
}

uint32_t frame_len = 0;
void *disp_frame = NULL;

void cli_spi_lcd_display_cmd(uint16_t color)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
 
    LOGI("cli_spi_lcd_display_cmd color: %d\n", color);

    if (!is_display_init) {
        ret = bk_display_spi_ctlr_new(&lcd_display_handle, &spi_ctlr_config);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_display_spi_ctlr_new failed!\n");
            return;
        }
        LOGD("bk_display_spi_ctlr_new success!\n");

        ret = bk_display_init(lcd_display_handle);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_display_init failed!\n");
            bk_display_delete(lcd_display_handle);
            lcd_display_handle = NULL;
            return;
        }

        ret = bk_display_open(lcd_display_handle);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_display_open failed!\n");
            bk_display_delete(lcd_display_handle);
            lcd_display_handle = NULL;
            return;
        }

        frame_len = spi_ctlr_config.lcd_panel->width * spi_ctlr_config.lcd_panel->height * 2;
        disp_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_len);
        if (disp_frame == NULL) {
            LOGE("frame malloc failed!\n");
            bk_display_delete(lcd_display_handle);
            lcd_display_handle = NULL;
            return;
        }
        is_display_init = true;
    }

    lcd_spi_display_fill_pure_color(disp_frame, frame_len, color);

    ret = bk_display_flush(lcd_display_handle, disp_frame, display_frame_free_cb);
    if (ret != AVDK_ERR_OK) {
        display_frame_free_cb(disp_frame);
        LOGE("bk_display_flush failed!\n");
        return;
    }

    LOGD("bk_display_flush frame success!\n");
}

#define CMDS_COUNT  (sizeof(s_spi_lcd_commands) / sizeof(struct cli_command))

void cli_spi_lcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
    if (argc != 2) {
        cli_spi_lcd_display_cmd(RED_COLOR);
        return;
    }
    uint16_t color = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    cli_spi_lcd_display_cmd(color);
}

static const struct cli_command s_spi_lcd_commands[] =
{
    {"spi_lcd", "spi_lcd", cli_spi_lcd_cmd},
};

int cli_spi_lcd_init(void)
{
    return cli_register_commands(s_spi_lcd_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_frame_buffer_init();

    cli_spi_lcd_init();

    return 0;
}
