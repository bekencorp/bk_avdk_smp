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
#include "h264d_gpu_display_dpu.h"

#define TAG "h264d_dpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264D_GPU_DISPLAY_PANEL_RESET_PIN      GPIO_60
#define H264D_GPU_DISPLAY_PANEL_BACKLIGHT_PIN  GPIO_7

typedef enum {
	H264D_GPU_DPU_STATE_OFF = 0,
	H264D_GPU_DPU_STATE_ON,
} h264d_gpu_dpu_state_t;

typedef struct {
	h264d_gpu_dpu_state_t state;
	bk_display_ctlr_handle_t dpu_ctlr_handle;
	bk_display_bus_handle_t dsi_bus_handle;
	bk_avdk_lcd_panel_handle_t panel_handle;
} h264d_gpu_dpu_ctx_t;

static h264d_gpu_dpu_ctx_t *s_dpu_ctx = NULL;
static beken_mutex_t s_dpu_mutex = NULL;

static const bk_display_dsi_panel_t *s_dpu_panel = &lcd_device_hx8399c_mipi_1080x1920;
static const int8_t s_dpu_panel_reset_pin = H264D_GPU_DISPLAY_PANEL_RESET_PIN;
static const int8_t s_dpu_panel_backlight_pin = H264D_GPU_DISPLAY_PANEL_BACKLIGHT_PIN;
static const bool s_dpu_video_enable = true;
static const bool s_dpu_video_decompress = true;
static const bk_pixel_format_t s_dpu_video_format = BK_PIXEL_FORMAT_ARGB8888;

static bool h264d_gpu_dpu_state_is_on(const h264d_gpu_dpu_ctx_t *ctx)
{
	return (ctx != NULL) && (ctx->state == H264D_GPU_DPU_STATE_ON) && (ctx->dpu_ctlr_handle != NULL);
}

static const bk_display_dsi_panel_t *h264d_gpu_dpu_panel_get(void)
{
	return s_dpu_panel;
}

static void h264d_gpu_dpu_backlight_on(void)
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

static void h264d_gpu_dpu_backlight_off(void)
{
	if (s_dpu_panel_backlight_pin < 0) {
		return;
	}

	bk_gpio_set_output_low(s_dpu_panel_backlight_pin);
}

static bk_err_t h264d_gpu_dpu_lock(void)
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

static void h264d_gpu_dpu_unlock(void)
{
	if (s_dpu_mutex != NULL) {
		(void)rtos_unlock_mutex(&s_dpu_mutex);
	}
}

static void h264d_gpu_dpu_destroy_ctx(h264d_gpu_dpu_ctx_t *ctx)
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
		(void)bk_lcd_panel_reset(ctx->panel_handle);
		(void)bk_lcd_panel_del(ctx->panel_handle);
		ctx->panel_handle = NULL;
	}

	if (ctx->dsi_bus_handle != NULL) {
		(void)bk_display_bus_delete(ctx->dsi_bus_handle);
		ctx->dsi_bus_handle = NULL;
	}

	ctx->state = H264D_GPU_DPU_STATE_OFF;
	os_memset(ctx, 0, sizeof(*ctx));
	os_free(ctx);
}

avdk_err_t h264d_gpu_display_dpu_open(void)
{
	bk_err_t ret;
	h264d_gpu_dpu_ctx_t *ctx;
	bk_display_dpu_config_t dpu_cfg;
	bk_lcd_panel_config_t panel_cfg;
	const bk_display_dsi_panel_t *panel;

	ret = h264d_gpu_dpu_lock();
	if (ret != BK_OK) {
		return AVDK_ERR_GENERIC;
	}

	if (s_dpu_ctx != NULL) {
		h264d_gpu_dpu_unlock();
		return AVDK_ERR_OK;
	}

	panel = h264d_gpu_dpu_panel_get();
	if (panel == NULL) {
		h264d_gpu_dpu_unlock();
		return AVDK_ERR_INVAL;
	}

	ctx = (h264d_gpu_dpu_ctx_t *)os_malloc(sizeof(*ctx));
	if (ctx == NULL) {
		h264d_gpu_dpu_unlock();
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
	panel_cfg.reset_active_level = false;

	LOGI("dpu step: panel_new\r\n");
	ret = bk_lcd_mipi_panel_new(ctx->dsi_bus_handle,
				    &panel_cfg,
				    panel,
				    &ctx->panel_handle);
	if (ret != BK_OK) {
		goto error;
	}

	LOGI("dpu step: panel_reset\r\n");
	ret = bk_lcd_panel_reset(ctx->panel_handle);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("dpu step: panel_init\r\n");
	ret = bk_lcd_panel_init(ctx->panel_handle);
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
	h264d_gpu_dpu_backlight_on();

	ctx->state = H264D_GPU_DPU_STATE_ON;
	s_dpu_ctx = ctx;
	h264d_gpu_dpu_unlock();
	LOGI("dpu open success\r\n");
	return AVDK_ERR_OK;

error:
	h264d_gpu_dpu_destroy_ctx(ctx);
	h264d_gpu_dpu_unlock();
	LOGE("dpu open failed, ret=%d\r\n", (int)ret);
	return AVDK_ERR_GENERIC;
}

void h264d_gpu_display_dpu_close(void)
{
	h264d_gpu_dpu_ctx_t *ctx;

	if (h264d_gpu_dpu_lock() != BK_OK) {
		return;
	}

	ctx = s_dpu_ctx;
	s_dpu_ctx = NULL;
	h264d_gpu_dpu_unlock();

	if (ctx != NULL) {
		h264d_gpu_dpu_backlight_off();
		h264d_gpu_dpu_destroy_ctx(ctx);
		LOGI("dpu closed\r\n");
	}
}

avdk_err_t h264d_gpu_display_dpu_flush(void *frame, avdk_err_t (*free_cb)(void *args))
{
	avdk_err_t ret = AVDK_ERR_GENERIC;

	if (h264d_gpu_dpu_lock() != BK_OK) {
		return ret;
	}

	if (h264d_gpu_dpu_state_is_on(s_dpu_ctx)) {
		ret = bk_display_flush(s_dpu_ctx->dpu_ctlr_handle, frame, free_cb);
	}

	h264d_gpu_dpu_unlock();
	return ret;
}

uint16_t h264d_gpu_display_dpu_width(void)
{
	return h264d_gpu_dpu_panel_get()->timing.h_size;
}

uint16_t h264d_gpu_display_dpu_height(void)
{
	return h264d_gpu_dpu_panel_get()->timing.v_size;
}
