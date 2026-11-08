#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "components/media_types.h"

/* UI backend selection: 1 = JavaScript through bk_jerryscript, 0 = native C.
 * Both implementations are compiled; this switch only decides which one runs.
 *
 * The JavaScript source itself is selected by CONFIG_LAUNCHER_JS_FROM_FS:
 *   0 (default) = run the bundle converted to C (ap/js_bundle.c)
 *   1           = load the application from the littlefs image in usr_config */
#define LAUNCHER_UI_USE_JS 1

#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#if (LAUNCHER_UI_USE_JS)
#include "bk_jerryscript.h"
#if (CONFIG_LAUNCHER_JS_FROM_FS)
#include "bk_js_app.h"
#else
#include "js_bundle.h"
#endif
#else
#include "launcher.h"
#include "game_2048.h"
#include "juice_rush.h"
#endif
#endif
#include "driver/drv_tp.h"
#include "media_service.h"
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_display.h>          /* umbrella: bus + panel + display ctlr */
#include <driver/gpio.h>
#include "gpio_driver.h"


#define TAG "launcher"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

typedef struct
{
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    void *frame_buffer[CONFIG_LVGL_FRAME_BUFFER_NUM];
} display_ctx_t;

static display_ctx_t *g_disp_ctx = NULL;

static void launcher_flush_cb(void *args, void *frame_buffer, int (*cb)(void *args))
{
    bk_display_flush(args, frame_buffer, cb);
}

static void lvgl_app_launcher_free_frame_buffers(display_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) {
        if (ctx->frame_buffer[i] != NULL) {
            bk_frame_buffer_free(ctx->frame_buffer[i]);
            ctx->frame_buffer[i] = NULL;
        }
    }
}

static void lvgl_app_launcher_display_deinit(display_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->dpu_ctlr_handle != NULL) {
        BK_LOG_ON_ERR(bk_display_close(ctx->dpu_ctlr_handle));
        BK_LOG_ON_ERR(bk_display_deinit(ctx->dpu_ctlr_handle));
        BK_LOG_ON_ERR(bk_display_delete(ctx->dpu_ctlr_handle));
        ctx->dpu_ctlr_handle = NULL;
    }

    if (ctx->panel_handle != NULL) {
        BK_LOG_ON_ERR(bk_lcd_panel_delete(ctx->panel_handle));
        ctx->panel_handle = NULL;
    }

    if (ctx->dis_bus_handle != NULL) {
        BK_LOG_ON_ERR(bk_display_bus_delete(ctx->dis_bus_handle));
        ctx->dis_bus_handle = NULL;
    }
}

static void lvgl_app_launcher_resource_deinit(void)
{
#if (LAUNCHER_UI_USE_JS)
    /* Stop the JavaScript engine first: it owns timers and event callbacks
     * which reference LVGL objects. */
    if (bk_js_is_ready()) {
        bk_js_deinit();
    }
#endif

    if (lv_vendor_is_initialized()) {
        lv_vendor_stop();
        lv_vendor_deinit();
    }

#if (!LAUNCHER_UI_USE_JS)
    /* LVGL objects are already released, so only clear the cached pointers. */
    launcher_reset();
    game_2048_reset();
    juice_rush_reset();
#endif

#if (CONFIG_TP)
    drv_tp_close();
#endif

    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    bk_gpio_set_output_low(GPIO_7);

    lvgl_app_launcher_display_deinit(g_disp_ctx);
    lvgl_app_launcher_free_frame_buffers(g_disp_ctx);

    if (g_disp_ctx != NULL) {
        os_free(g_disp_ctx);
        g_disp_ctx = NULL;
    }
}

bk_err_t lvgl_app_launcher_deinit(void)
{
    if (g_disp_ctx == NULL) {
        LOGW("%s already deinit\n", __func__);
        return BK_OK;
    }

    lvgl_app_launcher_resource_deinit();
    LOGI("%s complete\n", __func__);

    return BK_OK;
}

#if (LAUNCHER_UI_USE_JS)
/* Start the JavaScript engine and run the UI.  The source comes either from the
 * bundle compiled into the firmware (default) or from the littlefs image in the
 * usr_config partition, depending on CONFIG_LAUNCHER_JS_FROM_FS. */
static bk_err_t lvgl_app_launcher_js_start(void)
{
    bk_js_err_t err = bk_js_init(NULL);

    if (err != BK_JS_OK) {
        LOGE("js init failed: %s\n", bk_js_err_to_str(err));
        return BK_FAIL;
    }

#if (CONFIG_LAUNCHER_JS_FROM_FS)
    /* The littlefs image is mounted by bk_js_init(); load the "launcher"
     * application from it so the UI can be updated without rebuilding. */
    err = bk_js_app_start("launcher");
    if (err != BK_JS_OK) {
        LOGE("js app failed: %s\n", bk_js_err_to_str(err));
        return BK_FAIL;
    }

    LOGI("js app '%s' running from fs\n", bk_js_app_current());
#else
    /* Run the bundle converted to C, no file system access involved. */
    err = bk_js_eval("launcher.js", js_bundle_source(), js_bundle_len(), 0);
    if (err != BK_JS_OK) {
        LOGE("js bundle failed: %s\n", bk_js_err_to_str(err));
        return BK_FAIL;
    }

    LOGI("js bundle running from firmware\n");
#endif

    return BK_OK;
}
#endif

bk_err_t lvgl_app_launcher_init(void)
{
    bk_err_t ret = BK_OK;
    lv_vnd_config_t lv_vnd_config = {0};
    uint32_t frame_buffer_size = 0;

    if (g_disp_ctx != NULL) {
        LOGW("%s already init\n", __func__);
        return BK_OK;
    }

    g_disp_ctx = os_malloc(sizeof(display_ctx_t));
    if (g_disp_ctx == NULL) {
        LOGE("Failed to allocate display context\n");
        return BK_FAIL;
    }
    os_memset(g_disp_ctx, 0, sizeof(display_ctx_t));

    bk_display_dpu_config_t dpu_config =
    {
        .video.enable = true,
        .video.decompress = true,
        .video.format = BK_PIXEL_FORMAT_ARGB8888,
    };

    /* ST7701S 480x480 RGB panel: pixels over the 24-bit parallel bus, registers over SW SPI. */
    const bk_display_rgb_panel_t *panel = bk_lcd_find_rgb_panel_by_name("st7701s_rgb_480x480");
    if (panel == NULL) {
        LOGE("RGB panel st7701s_rgb_480x480 not found, please enable CONFIG_LCD_ST7701S\n");
        ret = BK_FAIL;
        goto err;
    }

    /* SW-SPI control channel, must match the board wiring. */
    bk_display_spi_bus_config_t rgb_cfg_bus =
    {
        .clk_pin = GPIO_8,
        .csx_pin = GPIO_28,
        .sda_pin = GPIO_9,
        .cmd_width = panel->spi_cmd_16bit ? 16 : 8,   /* Must follow the panel's command width. */
    };

    const bk_lcd_panel_config_t panel_config =
    {
        .reset_pin = GPIO_6,
    };

    #define WIDTH (480)
    #define HEIGHT (480)

    AVDK_GOTO_ON_ERROR(bk_display_spi_bus_new(&g_disp_ctx->dis_bus_handle, &rgb_cfg_bus), err, TAG, "display rgb cfg-bus new err\n");
    AVDK_GOTO_ON_ERROR(bk_lcd_rgb_panel_new(g_disp_ctx->dis_bus_handle, &panel_config, panel, &g_disp_ctx->panel_handle),
                       err, TAG, "create panel err\n");

    dpu_config.video.disp_x = 0;
    dpu_config.video.disp_y = 0;
    dpu_config.video.disp_w = panel->timing.h_size;
    dpu_config.video.disp_h = panel->timing.v_size;

    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&g_disp_ctx->dpu_ctlr_handle, g_disp_ctx->panel_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display open err\n");

    /* Backlight pin. */
    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
    bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_output_high(GPIO_7);

    lv_vnd_config.width = WIDTH;
    lv_vnd_config.height = HEIGHT;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    if (lv_vnd_config.render_mode == RENDER_PARTIAL_MODE) {
        lv_vnd_config.draw_pixel_size = WIDTH * 48 * sizeof(bk_color_t);
    }
    lv_vnd_config.rotation = ROTATE_NONE;
    lv_vnd_config.disp_width = WIDTH;
    lv_vnd_config.disp_height = HEIGHT;
    lv_vnd_config.output_compress = true;
    if (lv_vnd_config.output_compress && lv_vnd_config.render_mode == RENDER_PARTIAL_MODE) {
        if (WIDTH % 16 || HEIGHT % 4) {
            lv_vnd_config.disp_width = (WIDTH + 15) & ~15;
            lv_vnd_config.disp_height = (HEIGHT + 3) & ~3;
        }
        LOGI("lv_vnd_config.disp_width:%d, lv_vnd_config.disp_height:%d\r\n", lv_vnd_config.disp_width, lv_vnd_config.disp_height);
    }

    if (lv_vnd_config.output_compress) {
        frame_buffer_size = lv_vnd_config.disp_width * lv_vnd_config.disp_height;
    } else {
        frame_buffer_size = lv_vnd_config.disp_width * lv_vnd_config.disp_height * sizeof(bk_color_t);
    }

    for (int i = 0; i < CONFIG_LVGL_FRAME_BUFFER_NUM; i++) {
        if (i % 2) {
            g_disp_ctx->frame_buffer[i] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_buffer_size);
        } else {
            g_disp_ctx->frame_buffer[i] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_buffer_size);
        }
        if (g_disp_ctx->frame_buffer[i] == NULL) {
            LOGE("frame buffer %d malloc failed, size=%u\n", i, frame_buffer_size);
            ret = BK_FAIL;
            goto err;
        }
        lv_vnd_config.frame_buffer[i] = g_disp_ctx->frame_buffer[i];
    }
    lv_vnd_config.args = g_disp_ctx->dpu_ctlr_handle;
    lv_vnd_config.flush_cb = launcher_flush_cb;

    ret = lv_vendor_init(&lv_vnd_config);
    if (ret != BK_OK) {
        LOGE("lv_vendor_init failed, ret=%d\n", ret);
        goto err;
    }

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vendor_start();

#if (LAUNCHER_UI_USE_JS)
    /* The LVGL task is running, build the UI from JavaScript. */
    ret = lvgl_app_launcher_js_start();
    if (ret != BK_OK) {
        goto err;
    }
#else
    /* Native C UI: build it from this task while holding the display lock. */
    lv_vendor_disp_lock();
    launcher_start();
    lv_vendor_disp_unlock();
#endif

    return BK_OK;

err:
    lvgl_app_launcher_resource_deinit();

    return ret;
}

#define CMDS_COUNT  (sizeof(s_launcher_commands) / sizeof(struct cli_command))

static bk_err_t launcher_rotation_from_degrees(uint16_t degrees, rott_angle_t *rotation)
{
    if (rotation == NULL) {
        return BK_FAIL;
    }

    switch (degrees) {
    case 0:
        *rotation = ROTATE_NONE;
        return BK_OK;
    case 90:
        *rotation = ROTATE_90;
        return BK_OK;
    case 180:
        *rotation = ROTATE_180;
        return BK_OK;
    case 270:
        *rotation = ROTATE_270;
        return BK_OK;
    default:
        return BK_FAIL;
    }
}

void cli_launcher_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc == 3 && os_strcmp(argv[1], "rot") == 0) {
        uint16_t degrees = (uint16_t)os_strtoul(argv[2], NULL, 10);
        rott_angle_t rotation = ROTATE_NONE;
        bk_err_t ret = launcher_rotation_from_degrees(degrees, &rotation);
        if (ret == BK_OK) {
            ret = lv_vendor_set_dynamic_rotation(rotation);
        }
        LOGI("launcher rot %u ret=%d\r\n", degrees, ret);
        return;
    }

    if (argc == 2 && os_strcmp(argv[1], "close") == 0) {
        bk_err_t ret = lvgl_app_launcher_deinit();
        LOGI("launcher close ret=%d\r\n", ret);
        return;
    }

    if (argc == 2 && os_strcmp(argv[1], "open") == 0) {
        bk_err_t ret = lvgl_app_launcher_init();
        LOGI("launcher open ret=%d\r\n", ret);
        return;
    }

    LOGI("usage: launcher rot <0|90|180|270> | launcher close | launcher open\r\n");
}

static const struct cli_command s_launcher_commands[] =
{
    {"launcher", "launcher rot <0|90|180|270> | launcher close | launcher open", cli_launcher_cmd},
};

int cli_launcher_init(void)
{
    return cli_register_commands(s_launcher_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_frame_buffer_init();

    cli_launcher_init();

    lvgl_app_launcher_init();

    return 0;
}
