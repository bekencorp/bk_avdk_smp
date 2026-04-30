#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include <components/log.h>
#include <components/bk_display.h>
#include <components/bk_display_bus.h>
#include <components/bk_display_dpu_ctlr.h>
#include <components/bk_lcd_panel.h>
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>
#include <avdk_check.h>

#include "h264d_gpu_display_display.h"

#if !CONFIG_LCD_HX8399C_MIPI_1080x1920
extern const bk_display_dsi_panel_t lcd_device_hx8399c_mipi_1080x1920;
#endif

#define TAG "h264d_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264D_GPU_DISPLAY_PANEL_RESET_PIN      GPIO_60
#define H264D_GPU_DISPLAY_PANEL_BACKLIGHT_PIN  GPIO_7

typedef enum {
	H264D_GPU_DISPLAY_STATE_OFF = 0,
	H264D_GPU_DISPLAY_STATE_TURNING_ON,
	H264D_GPU_DISPLAY_STATE_ON,
	H264D_GPU_DISPLAY_STATE_TURNING_OFF,
} h264d_gpu_display_state_t;

typedef struct {
	h264d_gpu_display_state_t state;
	bk_display_ctlr_handle_t dpu_ctlr_handle;
	bk_display_bus_handle_t dsi_bus_handle;
	bk_avdk_lcd_panel_handle_t panel_handle;
	bk_display_bus_handle_t cfg_bus_handle;
	const bk_display_dsi_panel_t *panel_desc;
} h264d_gpu_display_ctx_t;

typedef struct {
	struct {
		uint8_t enable;
		int8_t pin_reset;
		int8_t pin_backlight;
		int8_t pin_scl;
		int8_t pin_sda;
		const bk_display_dsi_panel_t *panel;
	} mipi;

	struct {
		bool enable;
		bool decompress;
		bk_pixel_format_t format;
	} dpu_video;
} h264d_gpu_display_board_config_t;

static h264d_gpu_display_ctx_t *s_display_ctx = NULL;
static beken_mutex_t s_display_mutex = NULL;

static h264d_gpu_display_board_config_t s_display_board_config = {
	.mipi = {
		.enable = true,
		.pin_reset = H264D_GPU_DISPLAY_PANEL_RESET_PIN,
		.pin_backlight = H264D_GPU_DISPLAY_PANEL_BACKLIGHT_PIN,
		.pin_scl = -1,
		.pin_sda = -1,
		.panel = &lcd_device_hx8399c_mipi_1080x1920,
	},
	.dpu_video = {
		.enable = true,
		.decompress = true,
		.format = BK_PIXEL_FORMAT_ARGB8888,
	},
};

static bool h264d_gpu_display_state_is_on(const h264d_gpu_display_ctx_t *ctx)
{
	return (ctx != NULL) &&
	       (ctx->state == H264D_GPU_DISPLAY_STATE_ON) &&
	       (ctx->dpu_ctlr_handle != NULL);
}

static const bk_display_dsi_panel_t *h264d_gpu_display_panel_get(void)
{
	if (s_display_ctx != NULL && s_display_ctx->panel_desc != NULL) {
		return s_display_ctx->panel_desc;
	}

	if (s_display_board_config.mipi.panel != NULL) {
		return s_display_board_config.mipi.panel;
	}

	return &lcd_device_hx8399c_mipi_1080x1920;
}

static void h264d_gpu_display_backlight_on(void)
{
	if (!s_display_board_config.mipi.enable ||
	    s_display_board_config.mipi.pin_backlight < 0) {
		return;
	}

	gpio_dev_unmap(s_display_board_config.mipi.pin_backlight);
	BK_LOG_ON_ERR(bk_gpio_enable_output(s_display_board_config.mipi.pin_backlight));
	BK_LOG_ON_ERR(bk_gpio_pull_up(s_display_board_config.mipi.pin_backlight));
	bk_gpio_set_capacity(s_display_board_config.mipi.pin_backlight,
			     GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_output_high(s_display_board_config.mipi.pin_backlight);
}

static void h264d_gpu_display_backlight_off(void)
{
	if (!s_display_board_config.mipi.enable ||
	    s_display_board_config.mipi.pin_backlight < 0) {
		return;
	}

	bk_gpio_set_output_low(s_display_board_config.mipi.pin_backlight);
}

static bk_err_t h264d_gpu_display_lock(void)
{
	bk_err_t ret;

	if (s_display_mutex == NULL) {
		ret = rtos_init_mutex(&s_display_mutex);
		if (ret != BK_OK) {
			return ret;
		}
	}

	return rtos_lock_mutex(&s_display_mutex);
}

static void h264d_gpu_display_unlock(void)
{
	if (s_display_mutex != NULL) {
		(void)rtos_unlock_mutex(&s_display_mutex);
	}
}

static void h264d_gpu_display_destroy_ctx(h264d_gpu_display_ctx_t *ctx)
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

	if (ctx->cfg_bus_handle != NULL) {
		(void)bk_display_bus_delete(ctx->cfg_bus_handle);
		ctx->cfg_bus_handle = NULL;
	}

	if (ctx->dsi_bus_handle != NULL) {
		(void)bk_display_bus_delete(ctx->dsi_bus_handle);
		ctx->dsi_bus_handle = NULL;
	}

	ctx->state = H264D_GPU_DISPLAY_STATE_OFF;
	os_memset(ctx, 0, sizeof(*ctx));
	os_free(ctx);
}

avdk_err_t h264d_gpu_display_display_open(void)
{
	bk_err_t ret;
	h264d_gpu_display_ctx_t *ctx;
	bk_display_dpu_config_t dpu_cfg;
	bk_lcd_panel_dev_config_t panel_cfg;
	const bk_display_dsi_panel_t *panel;

	ret = h264d_gpu_display_lock();
	if (ret != BK_OK) {
		return AVDK_ERR_GENERIC;
	}

	if (s_display_ctx != NULL) {
		if (s_display_ctx->state != H264D_GPU_DISPLAY_STATE_ON) {
			h264d_gpu_display_unlock();
			return AVDK_ERR_GENERIC;
		}
		h264d_gpu_display_unlock();
		return AVDK_ERR_OK;
	}

	panel = h264d_gpu_display_panel_get();
	if (panel == NULL) {
		h264d_gpu_display_unlock();
		return AVDK_ERR_INVAL;
	}

	ctx = (h264d_gpu_display_ctx_t *)os_malloc(sizeof(*ctx));
	if (ctx == NULL) {
		h264d_gpu_display_unlock();
		return AVDK_ERR_NOMEM;
	}
	os_memset(ctx, 0, sizeof(*ctx));
	ctx->state = H264D_GPU_DISPLAY_STATE_TURNING_ON;
	ctx->panel_desc = panel;

	os_memset(&dpu_cfg, 0, sizeof(dpu_cfg));
	dpu_cfg.video.enable = s_display_board_config.dpu_video.enable;
	dpu_cfg.video.decompress = s_display_board_config.dpu_video.decompress;
	dpu_cfg.video.format = s_display_board_config.dpu_video.format;

	LOGI("display step: dsi_bus_new\r\n");
	ret = bk_display_dsi_bus_new(&ctx->dsi_bus_handle, NULL);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("display step: bus_enable\r\n");
	ret = bk_display_bus_enable(ctx->dsi_bus_handle);
	if (ret != BK_OK) {
		goto error;
	}

	os_memset(&panel_cfg, 0, sizeof(panel_cfg));
	if (s_display_board_config.mipi.pin_scl >= 0 &&
	    s_display_board_config.mipi.pin_sda >= 0 &&
	    (s_display_board_config.mipi.pin_scl != 0 ||
	     s_display_board_config.mipi.pin_sda != 0)) {
		bk_display_i2c_bus_config_t i2c_cfg = {
			.scl_pin = (uint8_t)s_display_board_config.mipi.pin_scl,
			.sda_pin = (uint8_t)s_display_board_config.mipi.pin_sda,
		};

		ret = bk_display_i2c_bus_new(&ctx->cfg_bus_handle, &i2c_cfg);
		if (ret != BK_OK) {
			goto error;
		}
	}

	panel_cfg.reset_pin = s_display_board_config.mipi.pin_reset;
	panel_cfg.rgb_ele_order = COLOR_RGB_ELEMENT_ORDER_RGB;
	panel_cfg.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
	panel_cfg.bits_per_pixel = 16;
	panel_cfg.vendor_config = (ctx->cfg_bus_handle != NULL) ?
		&ctx->cfg_bus_handle : NULL;

	LOGI("display step: panel_new\r\n");
	ret = bk_lcd_mipi_panel_new(ctx->dsi_bus_handle,
				    &panel_cfg,
				    panel,
				    &ctx->panel_handle);
	if (ret != BK_OK) {
		goto error;
	}

	LOGI("display step: panel_reset\r\n");
	ret = bk_lcd_panel_reset(ctx->panel_handle);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("display step: panel_init\r\n");
	ret = bk_lcd_panel_init(ctx->panel_handle);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("display step: get_timing\r\n");
	ret = bk_lcd_panel_get_disp_timing(ctx->panel_handle, &dpu_cfg.timing);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("display step: dpu_new\r\n");
	ret = bk_display_dpu_ctlr_new(&ctx->dpu_ctlr_handle, &dpu_cfg);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("display step: display_init\r\n");
	ret = bk_display_init(ctx->dpu_ctlr_handle);
	if (ret != BK_OK) {
		goto error;
	}
	LOGI("display step: display_open\r\n");
	ret = bk_display_open(ctx->dpu_ctlr_handle);
	if (ret != BK_OK) {
		goto error;
	}

	LOGI("display step: backlight_on\r\n");
	h264d_gpu_display_backlight_on();

	ctx->state = H264D_GPU_DISPLAY_STATE_ON;
	s_display_ctx = ctx;
	h264d_gpu_display_unlock();
	LOGI("display open success\r\n");
	return AVDK_ERR_OK;

error:
	h264d_gpu_display_destroy_ctx(ctx);
	h264d_gpu_display_unlock();
	LOGE("display open failed, ret=%d\r\n", (int)ret);
	return AVDK_ERR_GENERIC;
}

void h264d_gpu_display_display_close(void)
{
	h264d_gpu_display_ctx_t *ctx;

	if (h264d_gpu_display_lock() != BK_OK) {
		return;
	}

	ctx = s_display_ctx;
	if (ctx != NULL && ctx->state == H264D_GPU_DISPLAY_STATE_ON) {
		ctx->state = H264D_GPU_DISPLAY_STATE_TURNING_OFF;
	}
	s_display_ctx = NULL;
	h264d_gpu_display_unlock();

	if (ctx != NULL) {
		h264d_gpu_display_backlight_off();
		h264d_gpu_display_destroy_ctx(ctx);
		LOGI("display closed\r\n");
	}
}

void *h264d_gpu_display_display_handle_get(void)
{
	void *handle = NULL;

	if (h264d_gpu_display_lock() != BK_OK) {
		return NULL;
	}

	if (h264d_gpu_display_state_is_on(s_display_ctx)) {
		handle = s_display_ctx->dpu_ctlr_handle;
	}

	h264d_gpu_display_unlock();
	return handle;
}

avdk_err_t h264d_gpu_display_display_flush(void *frame, avdk_err_t (*free_cb)(void *args))
{
	avdk_err_t ret = AVDK_ERR_GENERIC;

	if (h264d_gpu_display_lock() != BK_OK) {
		return ret;
	}

	if (h264d_gpu_display_state_is_on(s_display_ctx)) {
		ret = bk_display_flush(s_display_ctx->dpu_ctlr_handle, frame, free_cb);
	}

	h264d_gpu_display_unlock();
	return ret;
}

uint16_t h264d_gpu_display_display_width(void)
{
	return h264d_gpu_display_panel_get()->timing.h_size;
}

uint16_t h264d_gpu_display_display_height(void)
{
	return h264d_gpu_display_panel_get()->timing.v_size;
}
