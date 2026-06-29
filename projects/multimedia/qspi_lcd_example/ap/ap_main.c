#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "media_service.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <components/bk_lcd_panel.h>

#define TAG "qspi_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define QSPI_LCD_BACKLIGHT_PIN GPIO_7
#define QSPI_LCD_RESET_PIN     GPIO_40
#define QSPI_LCD_TE_PIN        GPIO_41

#define RED_COLOR              0xF800
#define GREEN_COLOR            0x07E0
#define BLUE_COLOR             0x001F

#define QSPI_LCD_AUTO_REFRESH_INTERVAL_MS    3000
#define QSPI_LCD_AUTO_REFRESH_TASK_PRIORITY  BEKEN_DEFAULT_WORKER_PRIORITY
#define QSPI_LCD_AUTO_REFRESH_TASK_STACK     (1024 * 4)
#define QSPI_LCD_AUTO_REFRESH_COLOR_COUNT    (sizeof(s_qspi_lcd_auto_colors) / sizeof(s_qspi_lcd_auto_colors[0]))

extern const bk_display_qspi_panel_t lcd_device_spd2010;

static const uint16_t s_qspi_lcd_auto_colors[] = {
    RED_COLOR,
    GREEN_COLOR,
    BLUE_COLOR,
};

static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static bk_display_qspi_ctlr_config_t qspi_ctlr_config = {
    .lcd_panel = &lcd_device_spd2010,
    .qspi_id = 0,
    .reset_pin = QSPI_LCD_RESET_PIN,
    .te_pin = QSPI_LCD_TE_PIN,
};

static bool is_display_init = false;
static beken_thread_t s_qspi_lcd_auto_refresh_thread = NULL;
static uint8_t *disp_frame = NULL;
static uint32_t disp_frame_size = 0;

static avdk_err_t display_frame_free_cb(void *frame)
{
    return AVDK_ERR_OK;
}

static void lcd_backlight_open(gpio_id_t gpio_id)
{
    bk_gpio_set_capacity(gpio_id, GPIO_DRIVER_CAPACITY_3);
    BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
    BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));
}

static void qspi_lcd_display_fill_pure_color(uint8_t *frame, uint32_t frame_size, uint16_t color)
{
    uint8_t data[2] = {
        color >> 8,
        color & 0xff,
    };

    for (uint32_t i = 0; i < frame_size; i += 2) {
        frame[i] = data[0];
        frame[i + 1] = data[1];
    }
}

static avdk_err_t qspi_lcd_display_init(void)
{
    avdk_err_t ret;

    if (is_display_init) {
        return AVDK_ERR_OK;
    }

    ret = bk_display_qspi_ctlr_new(&lcd_display_handle, &qspi_ctlr_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_qspi_ctlr_new failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_display_init(lcd_display_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_init failed, ret=%d\n", ret);
        goto fail;
    }

    ret = bk_display_open(lcd_display_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_open failed, ret=%d\n", ret);
        goto fail;
    }

    disp_frame_size = qspi_ctlr_config.lcd_panel->qspi->frame_len;
    disp_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, disp_frame_size);
    if (disp_frame == NULL) {
        LOGE("frame malloc failed\n");
        ret = AVDK_ERR_NOMEM;
        goto fail;
    }

    lcd_backlight_open(QSPI_LCD_BACKLIGHT_PIN);
    is_display_init = true;
    return AVDK_ERR_OK;

fail:
    if (disp_frame != NULL) {
        bk_frame_buffer_free(disp_frame);
        disp_frame = NULL;
        disp_frame_size = 0;
    }
    if (lcd_display_handle != NULL) {
        bk_display_delete(lcd_display_handle);
        lcd_display_handle = NULL;
    }
    return ret;
}

static void cli_qspi_lcd_display_cmd(uint16_t color)
{
    avdk_err_t ret;

    LOGI("cli_qspi_lcd_display_cmd color: 0x%04x\n", color);

    ret = qspi_lcd_display_init();
    if (ret != AVDK_ERR_OK) {
        return;
    }

    qspi_lcd_display_fill_pure_color(disp_frame, disp_frame_size, color);

    ret = bk_display_flush(lcd_display_handle, disp_frame, display_frame_free_cb);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_flush failed, ret=%d\n", ret);
        return;
    }

    LOGD("bk_display_flush frame success\n");
}

static void qspi_lcd_auto_refresh_task(void *arg)
{
    (void)arg;

    while (1) {
        for (uint32_t i = 0; i < QSPI_LCD_AUTO_REFRESH_COLOR_COUNT; i++) {
            cli_qspi_lcd_display_cmd(s_qspi_lcd_auto_colors[i]);
            rtos_delay_milliseconds(QSPI_LCD_AUTO_REFRESH_INTERVAL_MS);
        }
    }
}

static void qspi_lcd_auto_refresh_start(void)
{
    bk_err_t ret;

    if (s_qspi_lcd_auto_refresh_thread != NULL) {
        LOGW("qspi lcd auto refresh task already running\n");
        return;
    }

    ret = rtos_create_thread(&s_qspi_lcd_auto_refresh_thread,
                             QSPI_LCD_AUTO_REFRESH_TASK_PRIORITY,
                             "qspi_lcd_auto",
                             (beken_thread_function_t)qspi_lcd_auto_refresh_task,
                             QSPI_LCD_AUTO_REFRESH_TASK_STACK,
                             NULL);
    if (ret != BK_OK) {
        LOGE("create qspi lcd auto refresh task failed, ret=%d\n", ret);
        s_qspi_lcd_auto_refresh_thread = NULL;
        return;
    }

    LOGI("qspi lcd auto refresh task started\n");
}

static void cli_qspi_lcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc != 2) {
        cli_qspi_lcd_display_cmd(RED_COLOR);
        return;
    }

    uint16_t color = os_strtoul(argv[1], NULL, 16) & 0xffff;
    cli_qspi_lcd_display_cmd(color);
}

static const struct cli_command s_qspi_lcd_commands[] =
{
    {"qspi_lcd", "qspi_lcd [rgb565_hex]", cli_qspi_lcd_cmd},
};

static int cli_qspi_lcd_init(void)
{
    return cli_register_commands(s_qspi_lcd_commands,
                                 sizeof(s_qspi_lcd_commands) / sizeof(s_qspi_lcd_commands[0]));
}

int main(void)
{
    bk_init();

    media_service_init();
    bk_frame_buffer_init();
    cli_qspi_lcd_init();
    qspi_lcd_auto_refresh_start();

    return 0;
}
