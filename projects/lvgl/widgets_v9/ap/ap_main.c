#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "components/media_types.h"
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#include "demos/widgets/lv_demo_widgets.h"
#endif
#include "driver/drv_tp.h"
#include "media_service.h"
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_display.h>          /* umbrella: bus + panel + display ctlr */
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>
#include <driver/gpio.h>
#include "gpio_driver.h"


#define TAG "widgets"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

static void bk_lodoen_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

typedef struct
{
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    void *frame_buffer[CONFIG_LVGL_FRAME_BUFFER_NUM];
} display_ctx_t;

static display_ctx_t *g_disp_ctx = NULL;

static void bk_widgets_flush_cb(void *args, void *frame_buffer, int (*cb)(void *args))
{
    bk_display_flush(args, frame_buffer, cb);
}

static void lvgl_app_widgets_free_frame_buffers(display_ctx_t *ctx)
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

static void lvgl_app_widgets_display_deinit(display_ctx_t *ctx)
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

static void lvgl_app_widgets_resource_deinit(void)
{
    if (lv_vendor_is_initialized()) {
        lv_vendor_stop();
        lv_vendor_deinit();
    }

#if (CONFIG_TP)
    drv_tp_close();
#endif

    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    bk_gpio_set_output_low(GPIO_7);

    lvgl_app_widgets_display_deinit(g_disp_ctx);
    lvgl_app_widgets_free_frame_buffers(g_disp_ctx);

    if (g_disp_ctx != NULL) {
        os_free(g_disp_ctx);
        g_disp_ctx = NULL;
    }
}

bk_err_t lvgl_app_widgets_deinit(void)
{
    if (g_disp_ctx == NULL) {
        LOGW("%s already deinit\n", __func__);
        return BK_OK;
    }

    lvgl_app_widgets_resource_deinit();
    LOGI("%s complete\n", __func__);

    return BK_OK;
}

bk_err_t lvgl_app_widgets_init(void)
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

    const bk_lcd_panel_config_t panel_config =
    {
        .reset_pin = GPIO_60,
    };

    #define WIDTH (1080)
    #define HEIGHT (1920)

    // Create DSI bus
    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&g_disp_ctx->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");
    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(g_disp_ctx->dis_bus_handle, &panel_config, &lcd_device_hx8399c_mipi_1080x1920, &g_disp_ctx->panel_handle),
                       err, TAG, "create panel err\n");

    dpu_config.video.disp_x = 0;
    dpu_config.video.disp_y = 0;
    dpu_config.video.disp_w = lcd_device_hx8399c_mipi_1080x1920.timing.h_size;
    dpu_config.video.disp_h = lcd_device_hx8399c_mipi_1080x1920.timing.v_size;

    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&g_disp_ctx->dpu_ctlr_handle, g_disp_ctx->panel_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display open err\n");

    /* enable backlight */
    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
    bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);  // Enhance GPIO Driver Capacity
    bk_gpio_set_output_high(GPIO_7);

    lv_vnd_config.width = WIDTH;
    lv_vnd_config.height = HEIGHT;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    if (lv_vnd_config.render_mode == RENDER_PARTIAL_MODE) {
        lv_vnd_config.draw_pixel_size = WIDTH * 64 * sizeof(bk_color_t);
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
    lv_vnd_config.flush_cb = bk_widgets_flush_cb;

    ret = lv_vendor_init(&lv_vnd_config);
    if (ret != BK_OK) {
        LOGE("lv_vendor_init failed, ret=%d\n", ret);
        goto err;
    }

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vendor_disp_lock();
    lv_demo_widgets();
    lv_vendor_disp_unlock();

    lv_vendor_start();

    return BK_OK;

err:
    lvgl_app_widgets_resource_deinit();

    return ret;
}

#define CMDS_COUNT  (sizeof(s_widgets_commands) / sizeof(struct cli_command))

static bk_err_t widgets_rotation_from_degrees(uint16_t degrees, rott_angle_t *rotation)
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

void cli_widgets_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc == 3 && os_strcmp(argv[1], "rot") == 0) {
        uint16_t degrees = (uint16_t)os_strtoul(argv[2], NULL, 10);
        rott_angle_t rotation = ROTATE_NONE;
        bk_err_t ret = widgets_rotation_from_degrees(degrees, &rotation);
        if (ret == BK_OK) {
            ret = lv_vendor_set_dynamic_rotation(rotation);
        }
        LOGI("widgets rot %u ret=%d\r\n", degrees, ret);
        return;
    }

    if (argc == 2 && os_strcmp(argv[1], "close") == 0) {
        bk_err_t ret = lvgl_app_widgets_deinit();
        LOGI("widgets close ret=%d\r\n", ret);
        return;
    }

    if (argc == 2 && os_strcmp(argv[1], "open") == 0) {
        bk_err_t ret = lvgl_app_widgets_init();
        LOGI("widgets open ret=%d\r\n", ret);
        return;
    }

    LOGI("usage: widgets rot <0|90|180|270> | widgets close | widgets open\r\n");
}

static const struct cli_command s_widgets_commands[] =
{
    {"widgets", "widgets rot <0|90|180|270> | widgets close | widgets open", cli_widgets_cmd},
};

int cli_widgets_init(void)
{
    return cli_register_commands(s_widgets_commands, CMDS_COUNT);
}

int main(void)
{
    bk_init();

    media_service_init();

    bk_lodoen_enable();

    bk_frame_buffer_init();

    cli_widgets_init();

    lvgl_app_widgets_init();

    return 0;
}
