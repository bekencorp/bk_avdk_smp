/*
 * Shared display layer: hx8399c MIPI 1080x1920 LCD + DPU (video decompress).
 * MIPI path uses it for panel only; UVC path also creates a flexa GPU exposed to OSD.
 * Trimmed from uvc/src/display_test.c (removed decode-attach dead path and debug CLI).
 */
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <avdk_error.h>
#include <avdk_check.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include <components/bk_gpu_types.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>          /* umbrella: bus + panel + display ctlr */
#include <common/avdk_pixel_types.h>
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>
#include <modules/pm.h>

#include <components/bk_gpu.h>
#include <components/bk_gpu_ctlr.h>
#include "display.h"

#define TAG "display"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define DISPLAY_GPU_FLEXA_LINES     16

typedef struct {
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    bk_gpu_ctlr_handle_t gpu_handle;
} app_display_config_t;

static app_display_config_t *s_display_config = NULL;

/* Panel MIPI IO domain 1.8V VDDIO (PM_AUXLDO_USER_DISPLAY).
 * Must enable before DSI/panel init; disable after DPU/panel teardown on close.
 * MIPI/UVC share display_open/close; VDDIO managed here so UVC-only open won't miss it. */
static avdk_err_t display_vddio_enable(bool enable)
{
    pm_auxldo_ctrl_cfg_t cfg = {0};
    cfg.ldo   = AUXLDOS_SEL_1P8V;
    cfg.out   = PM_AUXLDO_1P8V_OUT_1P8V;
    cfg.user  = PM_AUXLDO_USER_DISPLAY;
    cfg.state = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&cfg), TAG, "display 1p8v vddio vote failed");
    rtos_delay_milliseconds(1);
    LOGI("display vddio %s\n", enable ? "on" : "off");
    return AVDK_ERR_OK;
}

static avdk_err_t lcd_example_dsi_open(app_display_config_t *display_config)
{
    int ret = AVDK_ERR_GENERIC;
    const bk_display_dsi_panel_t *panel = &lcd_device_hx8399c_mipi_1080x1920;

    // Configure DPU
    bk_display_dpu_config_t dpu_config = {0};
    dpu_config.video.enable = true;
    dpu_config.video.decompress = true;
    dpu_config.video.format = BK_PIXEL_FORMAT_ARGB8888;

    // Configure panel device
    const bk_lcd_panel_config_t panel_config =
    {
        .reset_pin = GPIO_60,
    };

    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&display_config->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");

    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(display_config->dis_bus_handle, &panel_config, panel, &display_config->panel_handle),
                       err, TAG, "create panel err\n");

    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&display_config->dpu_ctlr_handle, display_config->panel_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(display_config->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(display_config->dpu_ctlr_handle), err, TAG, "display open err\n");

    // Enable backlight
    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
    bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_output_high(GPIO_7);

    LOGI("LCD opened successfully: panel=%s, format=%d, decompress=%d\n",
         panel->name, dpu_config.video.format, dpu_config.video.decompress);

    return AVDK_ERR_OK;

err:
    return ret;
}

static avdk_err_t display_turn_on(void)
{
    avdk_err_t ret = AVDK_ERR_OK;
    app_display_config_t *display_config = s_display_config;
    if (display_config != NULL) {
        LOGE("%s, %d, display config already initialized\n", __func__, __LINE__);
        return AVDK_ERR_OK;
    }

    display_config = (app_display_config_t *)os_malloc(sizeof(app_display_config_t));
    if (display_config == NULL) {
        LOGE("%s, %d, malloc display config failed\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }

    os_memset(display_config, 0, sizeof(app_display_config_t));

    ret = display_vddio_enable(true);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, enable vddio failed\n", __func__, __LINE__);
        goto out;
    }

    ret = lcd_example_dsi_open(display_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, open display failed\n", __func__, __LINE__);
        (void)display_vddio_enable(false);
        goto out;
    }

    s_display_config = display_config;

    LOGD("%s, %d, display turn on complete\n", __func__, __LINE__);

    return ret;

out:
    if (display_config) {
        os_free(display_config);
        s_display_config = NULL;
    }

    return ret;
}

static avdk_err_t display_turn_off(void)
{
    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_OK;
    }

    /* Teardown reverse of open; GPU frames feed DPU — stop GPU before display:
     *   GPU : bk_gpu_close -> bk_gpu_deinit -> bk_gpu_delete
     *   DPU : bk_display_close -> bk_display_deinit -> bk_display_delete
     *   panel/bus : bk_lcd_panel_delete -> bk_display_bus_delete
     * See mipi_lcd_example/lcd_example_mipi.c lcd_example_dsi_close(). */
    if (display_config->gpu_handle != NULL) {
        bk_gpu_close(display_config->gpu_handle);
        bk_gpu_deinit(display_config->gpu_handle);
        bk_gpu_delete(display_config->gpu_handle);
        display_config->gpu_handle = NULL;
    }

    if (display_config->dpu_ctlr_handle != NULL) {
        bk_display_close(display_config->dpu_ctlr_handle);
        bk_display_deinit(display_config->dpu_ctlr_handle);
        bk_display_delete(display_config->dpu_ctlr_handle);
        display_config->dpu_ctlr_handle = NULL;
    }

    if (display_config->panel_handle != NULL) {
        bk_lcd_panel_delete(display_config->panel_handle);
        display_config->panel_handle = NULL;
    }

    if (display_config->dis_bus_handle != NULL) {
        bk_display_bus_delete(display_config->dis_bus_handle);
        display_config->dis_bus_handle = NULL;
    }

    /* Backlight off: open drives GPIO_7 high; pull low here */
    bk_gpio_set_output_low(GPIO_7);

    /* Disable VDDIO after panel/DPU removed; avoid powering floating IO (matches doorbell app_mipi_lcd_turn_off) */
    (void)display_vddio_enable(false);

    os_free(display_config);
    s_display_config = NULL;

    LOGD("%s, %d, display turn off complete\n", __func__, __LINE__);

    return AVDK_ERR_OK;
}

static void *display_frame_malloc(uint32_t size)
{
    void *disp_frame = NULL;

    disp_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    if (disp_frame == NULL)
    {
        LOGE("GPU failed to malloc frame\r\n");
        return NULL;
    }
    return disp_frame;
}

static avdk_err_t display_frame_free(void *ptr)
{
    bk_frame_buffer_free(ptr);
    return AVDK_ERR_OK;
}

static void display_frame_display(void *frame, uint32_t frame_size, void *args)
{
    (void)frame_size;
    (void)args;

    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return;
    }

    if (display_config->dpu_ctlr_handle == NULL) {
        LOGE("%s, %d, display dpu ctlr not initialized\n", __func__, __LINE__);
        display_frame_free(frame);
        return;
    }

    avdk_err_t ret = bk_display_flush(display_config->dpu_ctlr_handle, frame, display_frame_free);
    if (ret != AVDK_ERR_OK)
    {
        LOGD("%s, %d, GPU failed to complete frame %d\r\n", __func__, __LINE__, ret);
        display_frame_free(frame);
    }
}

static bk_err_t display_enable_gpu(bool use_flexa, uint8_t *src_buffer, uint8_t flexa_buff_cnt,
                                   uint16_t src_width, uint16_t src_height)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_gpu_ctlr_config_t gpu_config;

    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_GENERIC;
    }

    if (display_config->gpu_handle != NULL) {
        LOGE("%s, %d, gpu already enabled\n", __func__, __LINE__);
        return AVDK_ERR_BUSY;
    }

    os_memset(&gpu_config, 0, sizeof(bk_gpu_ctlr_config_t));
    gpu_config.rotate_degree = 90;
    gpu_config.src_width = src_width;
    gpu_config.src_height = src_height;   /* match doorbell app_gpu_v2_turn_on: use raw height */
    gpu_config.dst_width = 1920;
    gpu_config.dst_height = 1080;
    gpu_config.src_format = BK_PIXEL_FORMAT_NV12;
    gpu_config.dst_format = BK_PIXEL_FORMAT_ARGB8888;
    gpu_config.compress = true;
    gpu_config.scale = true;
    gpu_config.flexa = use_flexa;
    gpu_config.flexa_lines = DISPLAY_GPU_FLEXA_LINES;
    gpu_config.flexa_buff_cnt = flexa_buff_cnt;
    gpu_config.src_buffer = src_buffer;
    gpu_config.flexa_line_done = NULL;    /* doorbell uses callback; NULL is fine here */
    gpu_config.flexa_line_done_args = NULL;
    gpu_config.frame_malloc = display_frame_malloc;
    gpu_config.frame_free = display_frame_free;
    gpu_config.frame_done = display_frame_display;
    gpu_config.frame_done_args = NULL;

    ret = bk_gpu_ctlr_new(&display_config->gpu_handle, &gpu_config);

    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_gpu_init(display_config->gpu_handle);

    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_gpu_open(display_config->gpu_handle);

    if (ret != AVDK_ERR_OK) {
        LOGW("%s, %d\n", __func__, __LINE__);
    }

    return ret;
}

avdk_err_t display_open(void)
{
    avdk_err_t ret = display_turn_on();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, turn on display failed, ret=%d\n", __func__, __LINE__, ret);
    }
    return ret;
}

avdk_err_t display_open_gpu_flexa(uint16_t width, uint16_t height,
                                  uint8_t *src_buffer, uint8_t flexa_buff_cnt)
{
    avdk_err_t ret = display_turn_on();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s turn on display failed, ret=%d\n", __func__, ret);
        return ret;
    }

    ret = display_enable_gpu(true, src_buffer, flexa_buff_cnt, width, height);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s enable gpu failed, ret=%d\n", __func__, ret);
        return ret;
    }

    return AVDK_ERR_OK;
}

bk_gpu_ctlr_handle_t display_get_gpu_handle(void)
{
    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        return NULL;
    }
    return display_config->gpu_handle;
}

void *display_get_dpu_handle(void)
{
    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return NULL;
    }

    return display_config->dpu_ctlr_handle;
}

avdk_err_t display_close(void)
{
    return display_turn_off();
}
