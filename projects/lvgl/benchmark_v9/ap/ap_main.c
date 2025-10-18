#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "cli.h"
#include "components/media_types.h"
#include "driver/drv_tp.h"
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#include "lv_demo_benchmark.h"
#endif
#include "media_service.h"
#include "components/bk_display.h"
#include "driver/gpio.h"
#include "gpio_driver.h"
#include "driver/pwr_clk.h"
#define TAG "benchmark"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LCD_LDO_PIN         (GPIO_13)
extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);
extern const lcd_device_t lcd_device_st7701s;

bk_display_rgb_ctlr_config_t rgb_ctlr_config = {
    .lcd_device = &lcd_device_st7701s,
    .clk_pin = GPIO_0,
    .cs_pin = GPIO_12,
    .sda_pin = GPIO_1,
    .rst_pin = GPIO_6,
};

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

bk_err_t lvgl_app_benchmark_init(void)
{
    bk_err_t ret = BK_OK;
    lv_vnd_config_t lv_vnd_config = {0};

    lv_vnd_config.width = rgb_ctlr_config.lcd_device->width;
    lv_vnd_config.height = rgb_ctlr_config.lcd_device->height;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    lv_vnd_config.rotation = ROTATE_NONE;
    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) {
        lv_vnd_config.frame_buffer[i] = frame_buffer_display_malloc(lv_vnd_config.width * lv_vnd_config.height * sizeof(bk_color_t));
        if (lv_vnd_config.frame_buffer[i] == NULL) {
            LOGE("lv_frame_buffer[%d] malloc failed\r\n", i);
            return BK_FAIL;
        }
    }
    bk_display_rgb_new(&lv_vnd_config.handle, &rgb_ctlr_config);
    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
    ret = lv_vendor_init(&lv_vnd_config);
    if (ret != BK_OK) {
        LOGE("%s lv_vendor_init fail\n", __func__);
        return ret;
    }

    bk_display_open(lv_vnd_config.handle);
    lcd_backlight_open(GPIO_7);

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vendor_disp_lock();
    lv_demo_benchmark();
    lv_vendor_disp_unlock();

    lv_vendor_start();

    return BK_OK;
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_LVGL_CODE_RUN, PM_POWER_MODULE_STATE_ON);

    lvgl_app_benchmark_init();

    return 0;
}
