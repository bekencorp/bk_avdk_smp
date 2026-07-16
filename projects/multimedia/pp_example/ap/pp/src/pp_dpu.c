#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include <components/log.h>
#include <components/bk_display.h>
#include <avdk_check.h>
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>
#include "pp_dpu.h"

#define TAG "pp_dpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define PP_EXAMPLE_PANEL_RESET_PIN      GPIO_60
#define PP_EXAMPLE_PANEL_BACKLIGHT_PIN  GPIO_7

typedef enum {
	PP_DPU_STATE_OFF = 0,
	PP_DPU_STATE_ON,
} pp_dpu_state_t;

typedef struct {
	pp_dpu_state_t state;
	bk_display_ctlr_handle_t dpu_ctlr_handle;
	bk_display_bus_handle_t dsi_bus_handle;
	bk_avdk_lcd_panel_handle_t panel_handle;
} pp_dpu_ctx_t;

static pp_dpu_ctx_t *s_dpu_ctx = NULL;
static beken_mutex_t s_dpu_mutex = NULL;

static const bk_display_dsi_panel_t *s_dpu_panel = &lcd_device_hx8399c_mipi_1080x1920;
static const int8_t s_dpu_panel_reset_pin = PP_EXAMPLE_PANEL_RESET_PIN;
static const int8_t s_dpu_panel_backlight_pin = PP_EXAMPLE_PANEL_BACKLIGHT_PIN;
static const bool s_dpu_video_enable = true;
static const bool s_dpu_video_decompress = true;
static const bk_pixel_format_t s_dpu_video_format = BK_PIXEL_FORMAT_ARGB8888;

static bool pp_dpu_state_is_on(const pp_dpu_ctx_t *ctx)
{
	return (ctx != NULL) && (ctx->state == PP_DPU_STATE_ON) && (ctx->dpu_ctlr_handle != NULL);
}

static const bk_display_dsi_panel_t *pp_dpu_panel_get(void)
{
	return s_dpu_panel;
}

static void pp_dpu_backlight_on(void)
{
	if (s_dpu_panel_backlight_pin < 0) {
		return;
	}

	gpio_dev_unmap(s_dpu_panel_backlight_pin);
	BK_LOG_ON_ERR(bk_gpio_enable_output(s_dpu_panel_backlight_pin));
	BK_LOG_ON_ERR(bk_gpio_pull_up(s_dpu_panel_backlight_pin));
	bk_gpio_set_capacity(s_dpu_panel_backlight_pin, GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_output_high(s_dpu_panel_backlight_pin);
}

static void pp_dpu_backlight_off(void)
{
	if (s_dpu_panel_backlight_pin < 0) {
		return;
	}

	bk_gpio_set_output_low(s_dpu_panel_backlight_pin);
}

static bk_err_t pp_dpu_lock(void)
{
	bk_err_t ret;

	if (s_dpu_mutex == NULL) {
		ret = rtos_init_mutex(&s_dpu_mutex);
		if (ret != BK_OK) {
			return ret;
		}
	}

	return rtos_lock_mutex(&s_dpu_mutex);
}

static void pp_dpu_unlock(void)
{
	if (s_dpu_mutex != NULL) {
		(void)rtos_unlock_mutex(&s_dpu_mutex);
	}
}

static void pp_dpu_mutex_deinit(void)
{
	if (s_dpu_mutex != NULL) {
		(void)rtos_deinit_mutex(&s_dpu_mutex);
		s_dpu_mutex = NULL;
	}
}

static void pp_dpu_destroy_ctx(pp_dpu_ctx_t *ctx)
{
	if (ctx == NULL) {
		return;
	}

	if (ctx->dpu_ctlr_handle != NULL) {
		(void)bk_display_close(ctx->dpu_ctlr_handle);
		(void)bk_display_deinit(ctx->dpu_ctlr_handle);
		(void)bk_display_delete(ctx->dpu_ctlr_handle);
		ctx->dpu_ctlr_handle = NULL;
	}

	if (ctx->panel_handle != NULL) {
		(void)bk_lcd_panel_delete(ctx->panel_handle);
		ctx->panel_handle = NULL;
	}

	if (ctx->dsi_bus_handle != NULL) {
		(void)bk_display_bus_delete(ctx->dsi_bus_handle);
		ctx->dsi_bus_handle = NULL;
	}

	ctx->state = PP_DPU_STATE_OFF;
	os_memset(ctx, 0, sizeof(*ctx));
	os_free(ctx);
}

avdk_err_t pp_dpu_open(void)
{
	bk_err_t ret;
	pp_dpu_ctx_t *ctx;
	bk_display_dpu_config_t dpu_cfg;
	bk_lcd_panel_config_t panel_cfg;
	const bk_display_dsi_panel_t *panel;

	ret = pp_dpu_lock();
	if (ret != BK_OK) {
		return AVDK_ERR_GENERIC;
	}

	if (s_dpu_ctx != NULL) {
		pp_dpu_unlock();
		return AVDK_ERR_OK;
	}

	panel = pp_dpu_panel_get();
	if (panel == NULL) {
		pp_dpu_unlock();
		pp_dpu_mutex_deinit();
		return AVDK_ERR_INVAL;
	}

	ctx = (pp_dpu_ctx_t *)os_malloc(sizeof(*ctx));
	if (ctx == NULL) {
		pp_dpu_unlock();
		pp_dpu_mutex_deinit();
		return AVDK_ERR_NOMEM;
	}
	os_memset(ctx, 0, sizeof(*ctx));

	os_memset(&dpu_cfg, 0, sizeof(dpu_cfg));
	dpu_cfg.video.enable = s_dpu_video_enable;
	dpu_cfg.video.decompress = s_dpu_video_decompress;
	dpu_cfg.video.format = s_dpu_video_format;

	LOGI("dpu step: dsi_bus_new\r\n");
	ret = bk_display_dsi_bus_new(&ctx->dsi_bus_handle, NULL);
	if (ret != BK_OK) {
		goto error;
	}

	os_memset(&panel_cfg, 0, sizeof(panel_cfg));
	panel_cfg.reset_pin = s_dpu_panel_reset_pin;

	LOGI("dpu step: panel_new\r\n");
	ret = bk_lcd_mipi_panel_new(ctx->dsi_bus_handle,
				    &panel_cfg,
				    panel,
				    &ctx->panel_handle);
	if (ret != BK_OK) {
		goto error;
	}

	LOGI("dpu step: dpu_new\r\n");
	ret = bk_display_dpu_ctlr_new(&ctx->dpu_ctlr_handle, ctx->panel_handle, &dpu_cfg);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("dpu step: display_init\r\n");
	ret = bk_display_init(ctx->dpu_ctlr_handle);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("dpu step: display_open\r\n");
	ret = bk_display_open(ctx->dpu_ctlr_handle);
	if (ret != BK_OK) {
		goto error;
	}

	LOGI("dpu step: backlight_on\r\n");
	pp_dpu_backlight_on();

	ctx->state = PP_DPU_STATE_ON;
	s_dpu_ctx = ctx;
	pp_dpu_unlock();
	LOGI("dpu open success, panel=%ux%u\r\n",
	     (unsigned)pp_dpu_width(), (unsigned)pp_dpu_height());
	return AVDK_ERR_OK;

error:
	pp_dpu_destroy_ctx(ctx);
	pp_dpu_unlock();
	pp_dpu_mutex_deinit();
	LOGE("dpu open failed, ret=%d\r\n", (int)ret);
	return AVDK_ERR_GENERIC;
}

void pp_dpu_close(void)
{
	pp_dpu_ctx_t *ctx;

	if (pp_dpu_lock() != BK_OK) {
		return;
	}

	ctx = s_dpu_ctx;
	s_dpu_ctx = NULL;
	pp_dpu_unlock();

	if (ctx != NULL) {
		pp_dpu_backlight_off();
		pp_dpu_destroy_ctx(ctx);
		LOGI("dpu closed\r\n");
	}
	pp_dpu_mutex_deinit();
}

avdk_err_t pp_dpu_flush(void *frame, avdk_err_t (*free_cb)(void *args))
{
	avdk_err_t ret = AVDK_ERR_GENERIC;

	if (pp_dpu_lock() != BK_OK) {
		return ret;
	}

	if (pp_dpu_state_is_on(s_dpu_ctx)) {
		ret = bk_display_flush(s_dpu_ctx->dpu_ctlr_handle, frame, free_cb);
	}

	pp_dpu_unlock();
	return ret;
}

uint16_t pp_dpu_width(void)
{
	return pp_dpu_panel_get()->timing.h_size;
}

uint16_t pp_dpu_height(void)
{
	return pp_dpu_panel_get()->timing.v_size;
}
