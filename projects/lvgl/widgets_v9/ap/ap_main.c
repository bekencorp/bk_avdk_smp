#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include "components/media_types.h"
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#include "lv_demo_widgets.h"
#endif
#include "driver/drv_tp.h"
#include "media_service.h"
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_display.h>
#include <components/bk_display_dpu_ctlr.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>
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
    uint8_t enable;
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
	bk_avdk_lcd_panel_handle_t panel_handle;
} display_ctx_t;

static display_ctx_t *g_disp_ctx = NULL;

static void bk_widgets_flush_cb(void *args, void *frame_buffer, int (*cb)(void *args))
{
    bk_display_flush(args, frame_buffer, cb);
}

bk_err_t lvgl_app_widgets_init(void)
{
    bk_err_t ret = BK_OK;
    lv_vnd_config_t lv_vnd_config = {0};

    g_disp_ctx = os_malloc(sizeof(display_ctx_t));
    if (g_disp_ctx == NULL) {
        LOGE("Failed to allocate display context\n");
        return BK_FAIL;
    }
    os_memset(g_disp_ctx, 0, sizeof(display_ctx_t));

    bk_display_dpu_config_t dpu_config =
    {
        .video.enable = true,
        .video.decompress = false,
        .video.format = BK_PIXEL_FORMAT_RGB565,
        .graphic.enable = false,
    };

    const bk_lcd_panel_dev_config_t panel_dev_config =
    {
        .reset_pin = GPIO_60,
        .rgb_ele_order = COLOR_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
        .flags.reset_active_level = 0,
    };

    #define WIDTH (1080)
    #define HEIGHT (1920)

    // Create DSI bus
    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&g_disp_ctx->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_bus_enable(g_disp_ctx->dis_bus_handle), err, TAG, "display bus enable err\n");

    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(g_disp_ctx->dis_bus_handle, &panel_dev_config, &lcd_device_hx8399c_mipi_1080x1920, &g_disp_ctx->panel_handle),
                       err, TAG, "create panel err\n");

    bk_lcd_panel_reset(g_disp_ctx->panel_handle);
    bk_lcd_panel_init(g_disp_ctx->panel_handle);
    bk_lcd_panel_get_disp_timing(g_disp_ctx->panel_handle, &dpu_config.timing);

    dpu_config.video.disp_x = 0;
    dpu_config.video.disp_y = 0;
    dpu_config.video.disp_w = dpu_config.timing.h_size;
    dpu_config.video.disp_h = dpu_config.timing.v_size;
    
    dpu_config.graphic.disp_x = 0;
    dpu_config.graphic.disp_y = 0;
    dpu_config.graphic.disp_w = dpu_config.timing.h_size;
    dpu_config.graphic.disp_h = dpu_config.timing.v_size;
	
    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&g_disp_ctx->dpu_ctlr_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display open err\n");

    /* enable backlight */
    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
    bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);  // Enhance GPIO Driver Capacity
    bk_gpio_set_output_high(GPIO_7);

#if (CONFIG_LV_USE_DRAW_VG_LITE || LV_USE_GPU_ROTATE)
    extern int gpu_bsp_init(uint32_t tess_width, uint32_t tess_heigth);
    gpu_bsp_init(WIDTH / 4, HEIGHT / 4);
#endif

    lv_vnd_config.width = WIDTH;
    lv_vnd_config.height = HEIGHT;
    lv_vnd_config.render_mode = RENDER_DIRECT_MODE;
    if (lv_vnd_config.render_mode == RENDER_PARTIAL_MODE) {
        lv_vnd_config.draw_pixel_size = 120 * 1024;
    }
    lv_vnd_config.rotation = ROTATE_NONE;
    lv_vnd_config.frame_buffer[0] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, WIDTH * HEIGHT * sizeof(lv_color_t));
    lv_vnd_config.frame_buffer[1] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, WIDTH * HEIGHT * sizeof(lv_color_t));
    lv_vnd_config.args = g_disp_ctx->dpu_ctlr_handle;
    lv_vnd_config.flush_cb = bk_widgets_flush_cb;

    lv_vendor_init(&lv_vnd_config);

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vendor_disp_lock();
    lv_demo_widgets();
    lv_vendor_disp_unlock();

    lv_vendor_start();

    return BK_OK;

err:
    // Handle error and release allocated resources
    if (g_disp_ctx) {
        if (g_disp_ctx->dpu_ctlr_handle) {
            bk_display_deinit(g_disp_ctx->dpu_ctlr_handle);
            bk_display_delete(g_disp_ctx->dpu_ctlr_handle);
            g_disp_ctx->dpu_ctlr_handle = NULL;
        }

        if (g_disp_ctx->panel_handle) {
            bk_lcd_panel_reset(g_disp_ctx->panel_handle);
            bk_lcd_panel_del(g_disp_ctx->panel_handle);
            g_disp_ctx->panel_handle = NULL;
        }

        if (g_disp_ctx->dis_bus_handle) {
            bk_display_bus_delete(g_disp_ctx->dis_bus_handle);
            g_disp_ctx->dis_bus_handle = NULL;
        }

        os_free(g_disp_ctx);
        g_disp_ctx = NULL;
    }

    return ret;
}

#define CMDS_COUNT  (sizeof(s_widgets_commands) / sizeof(struct cli_command))

void cli_widgets_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    LOGD("%s %d\r\n", __func__, __LINE__);
}

static const struct cli_command s_widgets_commands[] =
{
    {"widgets", "widgets", cli_widgets_cmd},
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

    lvgl_app_widgets_init();

    return 0;
}
