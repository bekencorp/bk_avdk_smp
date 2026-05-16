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

#include "gpu_vn_ctlr_v2.h"

#define TAG "display_test"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t display_test_open_with_gpu(void);
avdk_err_t display_test_close(void);

typedef struct {
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    bk_gpu_ctlr_handle_t gpu_handle;
} app_display_config_t;

static app_display_config_t *s_display_config = NULL;

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
    const bk_lcd_panel_dev_config_t panel_dev_config = 
    {
        .reset_pin = GPIO_60,
        .reset_active_level = false,
    };

    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&display_config->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");

    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(display_config->dis_bus_handle, &panel_dev_config, panel, &display_config->panel_handle),
                       err, TAG, "create panel err\n");


    bk_lcd_panel_reset(display_config->panel_handle);
    bk_lcd_panel_init(display_config->panel_handle);
    dpu_config.timing = panel->timing;

    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&display_config->dpu_ctlr_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
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

static avdk_err_t display_test_turn_on(void)
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

    ret = lcd_example_dsi_open(display_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, open display failed\n", __func__, __LINE__);
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

static avdk_err_t display_test_turn_off(void)
{
    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_OK;
    }

    /* TODO: display_test_turn_off must be invoked from the display
     * thread before tearing the pipeline down (close/delete order
     * matters). The previous in-line teardown referenced API names
     * (bk_display_dpu_ctlr_destroy / bk_display_bus_destroy /
     * bk_lcd_panel_destroy / bk_display_bus_disable) that no longer
     * exist after the lifecycle cleanup; intentionally left empty
     * pending the threading rework. */

    os_free(display_config);
    s_display_config = NULL;

    LOGD("%s, %d, display turn off complete\n", __func__, __LINE__);

    return AVDK_ERR_OK;
}

static void *display_test_frame_malloc(uint32_t size)
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

static avdk_err_t display_test_frame_free(void *ptr)
{
    bk_frame_buffer_free(ptr);
    return AVDK_ERR_OK;
}

static void display_test_frame_display(void *frame, uint32_t frame_size, void *args)
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
        display_test_frame_free(frame);
        return;
    }

    avdk_err_t ret = bk_display_flush(display_config->dpu_ctlr_handle, frame, display_test_frame_free);
    if (ret != AVDK_ERR_OK)
    {
        LOGD("%s, %d, GPU failed to complete frame %d\r\n", __func__, __LINE__, ret);
        display_test_frame_free(frame);
    }
}

static bk_err_t display_test_enable_gpu(void)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_gpu_ctlr_config_t gpu_config;

    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_GENERIC;
    }

    os_memset(&gpu_config, 0, sizeof(bk_gpu_ctlr_config_t));
    /* Rotate to match 1080x1920 portrait panel. */
    gpu_config.rotate_degree = 90;
    gpu_config.src_width = 1920;
    gpu_config.src_height = 1088;
    gpu_config.dst_width = 1920;
    gpu_config.dst_height = 1080;
    gpu_config.src_format = BK_PIXEL_FORMAT_NV12;
    gpu_config.dst_format = BK_PIXEL_FORMAT_ARGB8888;
    gpu_config.compress = true;
    gpu_config.scale = true;
    gpu_config.malloc = display_test_frame_malloc;
    gpu_config.free = display_test_frame_free;
    gpu_config.frame_display = display_test_frame_display;
    gpu_config.frame_display_args = NULL;

    ret = bk_gpu_test_ctlr_new(&display_config->gpu_handle, &gpu_config);

    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_gpu_test_init(display_config->gpu_handle);

    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_gpu_test_open(display_config->gpu_handle);

    if (ret != AVDK_ERR_OK) {
        LOGW("%s, %d\n", __func__, __LINE__);
    }

    return ret;
}

void cli_display_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_OK;
    if (argc < 2)
    {
        LOGE("Usage: display <command>\n");
        return;
    }

    if (os_strcmp(argv[1], "open") == 0)
    {
        ret = display_test_open_with_gpu();
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        ret = display_test_close();
    }
    else
    {
        LOGE("Usage: display <command>\n");
        return;
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to %s display, ret=%d\n", __func__, ret);
    } else {
        LOGI("Successfully %s display\n", __func__);
    }
}

avdk_err_t display_test_open_with_gpu(void)
{
    avdk_err_t ret = display_test_turn_on();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, turn on display failed, ret=%d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = display_test_enable_gpu();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, enable gpu failed, ret=%d\n", __func__, __LINE__, ret);
        return ret;
    }

    return AVDK_ERR_OK;
}

avdk_err_t display_test_close(void)
{
    return display_test_turn_off();
}

void *display_test_get_dpu_handle(void)
{
    app_display_config_t *display_config = s_display_config;
    if (display_config == NULL) {
        LOGE("%s, %d, display config not initialized\n", __func__, __LINE__);
        return NULL;
    }

    return display_config->dpu_ctlr_handle;
}