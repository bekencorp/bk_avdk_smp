#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <os/str.h>
#include <components/system.h>
#include <os/os.h>
#include "cli.h"
#include "gpio_driver.h"
#include "rgb_lcd_test.h"
#include <driver/gpio.h>
#include <components/avdk_utils/avdk_error.h>
#include "driver/pwr_clk.h"
#define TAG "rgb_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LCD_LDO_PIN          (GPIO_13)

avdk_err_t lcd_fill_rand_color(uint32_t len, uint8_t *addr)
{
    uint32_t color_rand = 0;
    uint8_t *p_addr = addr;
    int i = 0;

    color_rand = (uint32_t)rand();

    for( i = 0; i < len; i+=2)
    {
        *(uint16_t *)(p_addr + i) = color_rand;
    }
    return AVDK_ERR_OK;
}

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

avdk_err_t lcd_ldo_open(uint8_t lcd_ldo_io)
{
    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, lcd_ldo_io, GPIO_OUTPUT_STATE_HIGH);
    return AVDK_ERR_OK;
}
avdk_err_t lcd_ldo_close(uint8_t lcd_ldo_io)
{
    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, lcd_ldo_io, GPIO_OUTPUT_STATE_LOW);
    return AVDK_ERR_OK;
}

#define CMDS_COUNT  (sizeof(s_lcd_display_commands) / sizeof(struct cli_command))

static const struct cli_command s_lcd_display_commands[] =
{
    {"lcd_display_api", "init/open/close/flush cnt /delete", cli_lcd_display_api_cmd},
    {"lcd_display", "open/close", cli_lcd_display_cmd},
};

int cli_lcd_display_init(void)
{
    return cli_register_commands(s_lcd_display_commands, CMDS_COUNT);
}