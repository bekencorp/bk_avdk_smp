#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <common/bk_err.h>
#include <components/log.h>
#include <avdk_error.h>

#include "components/bk_frame_buffer.h"
#include <components/bk_lcd_types.h>
#include "components/bk_display.h"
#include "app_display.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include <avdk_check.h>
#include <components/bk_display_dpu_ctlr.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>

#define TAG "app-disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


typedef struct
{
    uint8_t enable;
#if (DISP_DEBUG_TIMER_ENABLE)
    beken_timer_t timer;
    float rps;             //hw refreash per second
    float fps;             //frame per second
    uint32_t refreash_count;
    uint32_t frame_count;
    uint32_t last_refreash_count;
    uint32_t last_frame_count;
#endif
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    bk_display_bus_handle_t cfg_bus_handle;
} display_ctx_t;

display_ctx_t *s_disp_ctx = NULL;


display_board_config_t *display_board_config = NULL;

void *app_mipi_lcd_handle_get(void)
{
    AVDK_RETURN_ON_FALSE(s_disp_ctx, NULL, TAG, "s_disp_ctx NULL \n");
    return s_disp_ctx->dpu_ctlr_handle;
}


int app_display_flush_complete(uint32_t frame)
{
#if (DISP_DEBUG_TIMER_ENABLE)
    display_ctx_t *config = s_disp_ctx;
    AVDK_RETURN_ON_FALSE(config, BK_FAIL, TAG, "s_disp_ctx NULL \n");

    config->refreash_count++;
#endif
    return BK_OK;
}

int app_mipi_lcd_turn_off(void)
{
    bk_err_t ret = BK_OK;

    display_ctx_t *config = s_disp_ctx;
    display_board_config_t *board_config = app_display_board_config_get();
    if (config == NULL)
    {
        LOGI("%s, already turn off %d \n", __func__, __LINE__);
        return ret;
    }

    if (config->enable == false)
    {
        LOGE("%s, display state is already disable, may be turning off now\n", __func__);
        return BK_FAIL;
    }

    config->enable = false;
    if (board_config && board_config->mipi.enable) {
        bk_gpio_set_output_low(board_config->mipi.pin_backlight);
    }
    if (config->dpu_ctlr_handle)
    {
        ret = bk_display_deinit(config->dpu_ctlr_handle);
        bk_display_delete(config->dpu_ctlr_handle);
        config->dpu_ctlr_handle = NULL;
    }
    if (config->panel_handle)
    {
        bk_lcd_panel_reset(config->panel_handle);
        bk_lcd_panel_del(config->panel_handle);
        config->panel_handle = NULL;
    }

    if (config->dis_bus_handle)
    {
        bk_display_bus_delete(config->dis_bus_handle);
        config->dis_bus_handle = NULL;
    }

    if (config->cfg_bus_handle) {
        bk_display_bus_delete(config->cfg_bus_handle);
        config->cfg_bus_handle = NULL;
    }
//    display_frame_manager_deinit();
    os_memset(config, 0, sizeof(display_ctx_t));
    os_free(config);
    s_disp_ctx = NULL;
    LOGI("%s complete\n", __func__);
    return BK_OK;
}


int app_mipi_lcd_turn_on(display_board_config_t *config)
{
    int ret = BK_OK;
    display_ctx_t *content = s_disp_ctx;

    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    // Use default panel from board config if panel is NULL
    if (config->mipi.panel == NULL)
    {
        LOGE("No panel specified and no default panel config\n");
        return BK_FAIL;
    }

    if (content)
    {
        LOGW("%s already turned on\n", __func__);
        if (!content->enable)
        {
            LOGE("%s, display state is error, may be turning off now\n", __func__);
            ret = BK_FAIL;
        }
        return ret;
    }

    content = (display_ctx_t *)os_malloc(sizeof(display_ctx_t));
    AVDK_RETURN_ON_FALSE(content, BK_ERR_NO_MEM, TAG, "malloc s_disp_ctx NUL \n");

    os_memset(content, 0, sizeof(display_ctx_t));

    bk_display_dpu_config_t lcd_cfg =
    {
        .video.enable = config->dpu_video.enable,
        .video.decompress = config->dpu_video.decompress,
        .video.format = config->dpu_video.format,
    };

    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&content->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_bus_enable(content->dis_bus_handle), err, TAG, "display bus enable err\n");

    /* Create config bus for MIPI bridge when pin_scl/pin_sda are configured (I2C). */
    if (config->mipi.pin_scl >= 0 && config->mipi.pin_sda >= 0 &&  (config->mipi.pin_scl != 0 || config->mipi.pin_sda != 0)) {
        bk_display_i2c_bus_config_t i2c_cfg = {
            .scl_pin = (uint8_t)config->mipi.pin_scl,
            .sda_pin = (uint8_t)config->mipi.pin_sda,
        };
        AVDK_GOTO_ON_ERROR(bk_display_i2c_bus_new(&content->cfg_bus_handle, &i2c_cfg),
                           err, TAG, "display i2c bus new err\n");
    } else {
        content->cfg_bus_handle = NULL;
    }

    bk_lcd_panel_dev_config_t panel_dev_config = {
        .reset_pin = config->mipi.pin_reset,
        .rgb_ele_order = COLOR_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
        .vendor_config = (content->cfg_bus_handle != NULL) ? &content->cfg_bus_handle : NULL,
    };

    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(content->dis_bus_handle, &panel_dev_config, config->mipi.panel, &content->panel_handle),
                       err, TAG, "create panel err\n");

    bk_lcd_panel_reset(content->panel_handle);
    bk_lcd_panel_init(content->panel_handle);

    bk_lcd_panel_get_disp_timing(content->panel_handle, &lcd_cfg.timing);
    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&content->dpu_ctlr_handle, &lcd_cfg), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(content->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(content->dpu_ctlr_handle), err, TAG, "display open err\n");

    /* enable backlight */
    gpio_dev_unmap(config->mipi.pin_backlight);
    BK_LOG_ON_ERR(bk_gpio_enable_output(config->mipi.pin_backlight));
    BK_LOG_ON_ERR(bk_gpio_pull_up(config->mipi.pin_backlight));
    bk_gpio_set_capacity(config->mipi.pin_backlight, GPIO_DRIVER_CAPACITY_3);  // Enhance GPIO Driver Capacity
    bk_gpio_set_output_high(config->mipi.pin_backlight);
    content->enable = true;

    s_disp_ctx = content;
    return BK_OK;

err:
    if (content != NULL) {
        if (content->cfg_bus_handle) {
            bk_display_bus_delete(content->cfg_bus_handle);
            content->cfg_bus_handle = NULL;
        }
        if (content->dis_bus_handle) {
            bk_display_bus_delete(content->dis_bus_handle);
            content->dis_bus_handle = NULL;
        }
        if (content->dpu_ctlr_handle) {
            bk_display_deinit(content->dpu_ctlr_handle);
            bk_display_delete(content->dpu_ctlr_handle);
            content->dpu_ctlr_handle = NULL;
        }
        os_memset(content, 0, sizeof(display_ctx_t));
        os_free(content);
        s_disp_ctx = NULL;
    }
    LOGE("%s fail\n", __func__);
    return ret;
}

int app_mipi_lcd_flush(void *frame, avdk_err_t (*free_t)(void *args))
{
    bk_err_t ret = AVDK_ERR_GENERIC;

    if (s_disp_ctx && s_disp_ctx->dpu_ctlr_handle)
    {
         ret = bk_display_flush(s_disp_ctx->dpu_ctlr_handle, frame, free_t);
    }
    return ret;
}

bool app_mipi_lcd_state_get(void)
{
    if (s_disp_ctx && s_disp_ctx->dpu_ctlr_handle)
    {
        return true;
    }
    return false;
}


int app_display_board_config_set(display_board_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    if (display_board_config == NULL)
    {
        display_board_config = os_malloc(sizeof(display_board_config_t));
        AVDK_RETURN_ON_FALSE(display_board_config, AVDK_ERR_GENERIC, TAG, "display_board_config malloc failed");
    }

    os_memcpy(display_board_config, config, sizeof(display_board_config_t));

    return AVDK_ERR_OK;
}


display_board_config_t *app_display_board_config_get(void)
{
    return display_board_config;
}

