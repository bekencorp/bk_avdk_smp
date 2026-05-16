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
#include <sys_types.h>
#include <modules/pm.h>
#define TAG "app-disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


typedef enum
{
    APP_DISPLAY_STATE_OFF = 0,
    APP_DISPLAY_STATE_TURNING_ON,
    APP_DISPLAY_STATE_ON,
    APP_DISPLAY_STATE_TURNING_OFF,
} app_display_state_t;

typedef struct
{
    app_display_state_t state;
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
static beken_mutex_t s_disp_mutex = NULL;


display_board_config_t *display_board_config = NULL;
/**
 * @brief Vote display-domain AuxLDO (1.8V vddio) on/off.
 *
 * Single owner of PM_AUXLDO_USER_DISPLAY. Called by app_mipi_lcd_turn_on/off;
 * higher layers MUST NOT vote PM_AUXLDO_USER_DISPLAY themselves.
 */
avdk_err_t app_display_power_enable(bool enable)
{
    int ldo_en = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    LOGI("%s, vddio enable: %d\n", __func__, ldo_en);

    pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
    auxldo_cfg.ldo = AUXLDOS_SEL_1P8V;
    auxldo_cfg.out = PM_AUXLDO_1P8V_OUT_1P8V;
    auxldo_cfg.user = PM_AUXLDO_USER_DISPLAY;
    auxldo_cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "display 1p8v ldo vote failed");
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}
static bool app_display_state_is_on(const display_ctx_t *ctx)
{
    return (ctx != NULL) && (ctx->state == APP_DISPLAY_STATE_ON) && (ctx->dpu_ctlr_handle != NULL);
}

static void app_display_ctx_destroy(display_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    if (ctx->dpu_ctlr_handle)
    {
        (void)bk_display_close(ctx->dpu_ctlr_handle);
        (void)bk_display_deinit(ctx->dpu_ctlr_handle);
        (void)bk_display_delete(ctx->dpu_ctlr_handle);
        ctx->dpu_ctlr_handle = NULL;
    }

    if (ctx->panel_handle)
    {
        bk_lcd_panel_reset(ctx->panel_handle);
        bk_lcd_panel_del(ctx->panel_handle);
        ctx->panel_handle = NULL;
    }

    if (ctx->dis_bus_handle)
    {
        bk_display_bus_disable(ctx->dis_bus_handle);
        bk_display_bus_delete(ctx->dis_bus_handle);
        ctx->dis_bus_handle = NULL;
    }

    if (ctx->cfg_bus_handle)
    {
        bk_display_bus_delete(ctx->cfg_bus_handle);
        ctx->cfg_bus_handle = NULL;
    }

    ctx->state = APP_DISPLAY_STATE_OFF;
    os_memset(ctx, 0, sizeof(display_ctx_t));
    os_free(ctx);
}

static bk_err_t app_display_lock(void)
{
    if (s_disp_mutex == NULL)
    {
        bk_err_t ret = rtos_init_mutex(&s_disp_mutex);
        if (ret != BK_OK)
        {
            LOGE("%s, init mutex failed: %d\n", __func__, ret);
            return ret;
        }
    }

    bk_err_t ret = rtos_lock_mutex(&s_disp_mutex);
    if (ret != BK_OK)
    {
        LOGE("%s, lock mutex failed: %d\n", __func__, ret);
    }
    return ret;
}

static void app_display_unlock(void)
{
    if (s_disp_mutex != NULL)
    {
        (void)rtos_unlock_mutex(&s_disp_mutex);
    }
}

void *app_mipi_lcd_handle_get(void)
{
    void *handle = NULL;

    if (app_display_lock() != BK_OK)
    {
        return NULL;
    }

    if (s_disp_ctx != NULL)
    {
        if (s_disp_ctx->state == APP_DISPLAY_STATE_ON)
        {
            handle = s_disp_ctx->dpu_ctlr_handle;
        }
    }

    app_display_unlock();
    return handle;
}


int app_mipi_lcd_turn_off(void)
{
    bk_err_t ret = BK_OK;
    display_ctx_t *config = NULL;
    display_board_config_t *board_config = app_display_board_config_get();

    if (app_display_lock() != BK_OK)
    {
        return BK_FAIL;
    }

    config = s_disp_ctx;
    if (config == NULL)
    {
        LOGI("%s, already turn off %d \n", __func__, __LINE__);
        app_display_unlock();
        return ret;
    }

    if (config->state != APP_DISPLAY_STATE_ON)
    {
        LOGE("%s, invalid display state: %d\n", __func__, config->state);
        app_display_unlock();
        return BK_FAIL;
    }

    config->state = APP_DISPLAY_STATE_TURNING_OFF;
    if (board_config && board_config->mipi.enable) {
        bk_gpio_set_output_low(board_config->mipi.pin_backlight);
    }

    s_disp_ctx = NULL;
    app_display_ctx_destroy(config);
    app_display_unlock();
    app_display_power_enable(false);
    LOGI("%s complete\n", __func__);
    return BK_OK;
}


int app_mipi_lcd_turn_on(display_board_config_t *config)
{
    int ret = BK_OK;
    display_ctx_t *content = NULL;

    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    // Use default panel from board config if panel is NULL
    if (config->mipi.panel == NULL)
    {
        LOGE("No panel specified and no default panel config\n");
        return BK_FAIL;
    }

    if (app_display_lock() != BK_OK)
    {
        return BK_FAIL;
    }

    content = s_disp_ctx;
    if (content)
    {
        LOGW("%s already turned on\n", __func__);
        if (content->state != APP_DISPLAY_STATE_ON)
        {
            LOGE("%s, display state is invalid: %d\n", __func__, content->state);
            ret = BK_FAIL;
        }
        app_display_unlock();
        return ret;
    }

    content = (display_ctx_t *)os_malloc(sizeof(display_ctx_t));
    if (content == NULL)
    {
        LOGE("malloc s_disp_ctx NULL \n");
        app_display_unlock();
        return BK_ERR_NO_MEM;
    }

    os_memset(content, 0, sizeof(display_ctx_t));
    content->state = APP_DISPLAY_STATE_TURNING_ON;

    bk_display_dpu_config_t lcd_cfg =
    {
        .video.enable = config->dpu_video.enable,
        .video.decompress = config->dpu_video.decompress,
        .video.format = config->dpu_video.format,
    };

    app_display_power_enable(true);
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

    AVDK_GOTO_ON_ERROR(bk_lcd_panel_reset(content->panel_handle), err, TAG, "panel reset err\n");
    AVDK_GOTO_ON_ERROR(bk_lcd_panel_init(content->panel_handle), err, TAG, "panel init err\n");

    AVDK_GOTO_ON_ERROR(bk_lcd_panel_get_disp_timing(content->panel_handle, &lcd_cfg.timing), err, TAG, "get panel timing err\n");
    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&content->dpu_ctlr_handle, &lcd_cfg), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(content->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(content->dpu_ctlr_handle), err, TAG, "display open err\n");

    if (config->mipi.enable)
    {
        gpio_dev_unmap(config->mipi.pin_backlight);
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->mipi.pin_backlight));
        BK_LOG_ON_ERR(bk_gpio_pull_up(config->mipi.pin_backlight));
        bk_gpio_set_capacity(config->mipi.pin_backlight, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_high(config->mipi.pin_backlight);
    }
    content->state = APP_DISPLAY_STATE_ON;

    s_disp_ctx = content;
    app_display_unlock();
    return BK_OK;

err:
    if (content != NULL) {
        content->state = APP_DISPLAY_STATE_OFF;
        app_display_ctx_destroy(content);
        s_disp_ctx = NULL;
    }
    app_display_unlock();
    app_display_power_enable(false);
    LOGE("%s fail\n", __func__);
    return ret;
}

int app_mipi_lcd_flush(void *frame, avdk_err_t (*free_t)(void *args))
{
    bk_err_t ret = AVDK_ERR_GENERIC;

    if (app_display_lock() != BK_OK)
    {
        return ret;
    }

    if (app_display_state_is_on(s_disp_ctx))
    {
        ret = bk_display_flush(s_disp_ctx->dpu_ctlr_handle, frame, free_t);
    }

    app_display_unlock();
    return ret;
}

bool app_mipi_lcd_state_get(void)
{
    bool enabled = false;

    if (app_display_lock() != BK_OK)
    {
        return false;
    }

    if (app_display_state_is_on(s_disp_ctx))
    {
        enabled = true;
    }

    app_display_unlock();
    return enabled;
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

