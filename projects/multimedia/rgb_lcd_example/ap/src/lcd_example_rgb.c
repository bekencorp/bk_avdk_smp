// Copyright 2020-2021 Beken
// RGB LCD example: open/close + flush thread (single file, no queue).
// Flush thread uses direct malloc -> fill -> flush(..., display_frame_free) -> delay per frame.

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include <components/bk_display.h>          /* umbrella: bus + panel + display ctlr */
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <avdk_check.h>
#include <avdk_error.h>
#include "lcd_example.h"

#define TAG "rgb_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/* Per-case timings (rough hold, not strictly timed). */
#define RGB_LCD_BOOT_HOLD_MS     (20 * 1000)
#define RGB_LCD_CASE_POLL_MS     (100)
#define RGB_LCD_SWITCH_STEP_MS   (5 * 1000)

/* Flush free callback: display driver calls this when done with the frame; we just free it. */
static avdk_err_t display_frame_free(void *args)
{
    if (args != NULL)
        bk_frame_buffer_free(args);
    return AVDK_ERR_OK;
}

const uint16_t rgb565_color_map[] = {
    RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_YELLOW, RGB565_CYAN,
    RGB565_MAGENTA, RGB565_WHITE, RGB565_BLACK, RGB565_GRAY, RGB565_BROWN,
    RGB565_PINK, RGB565_ORANGE, RGB565_PURPLE,
};

/* Simple fill: one RGB565 color for full frame. */
static void fill_rgb565_frame(void *frame, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t *p = (uint16_t *)frame;
    uint32_t n = (uint32_t)width * height;
    while (n--)
        *p++ = color;
}

/* Simple fill: one ARGB8888 color for full frame. */
static void fill_argb8888_frame(void *frame, uint16_t width, uint16_t height, uint32_t color)
{
    uint32_t *p = (uint32_t *)frame;
    uint32_t n = (uint32_t)width * height;
    while (n--)
        *p++ = color;
}

static uint32_t get_frame_size(bk_pixel_format_t format, uint16_t w, uint16_t h)
{
    switch (format) {
    case BK_PIXEL_FORMAT_RGB565:
        return (uint32_t)w * h * 2;
    case BK_PIXEL_FORMAT_ARGB8888:
        return (uint32_t)w * h * 4;
    default:
        return (uint32_t)w * h * 2;
    }
}

// ----- RGB open / close -----

avdk_err_t lcd_example_rgb_open(display_ctx_t *context, const char *panel_name, bk_pixel_format_t format)
{
    int ret = AVDK_ERR_GENERIC;
    const bk_display_rgb_panel_t *panel = NULL;

    bk_display_dpu_config_t dpu_config = {
        .video.enable = true,
        .video.decompress = (format == BK_PIXEL_FORMAT_ARGB8888),
        .video.format = format,
    };

    /* RGB panels drive register-init through a private SW (GPIO bit-bang)
     * SPI channel; pixel data flows through the parallel 24-bit RGB
     * lanes muxed by the panel-common driver and pushed by the DPU.
     *
     * cmd_width is filled in below once the panel descriptor has been
     * resolved, so the SW SPI bus' panel-IO is created with the
     * correct wire format (8 = 9-bit SPI, 16 = 4-byte packed). */
    bk_display_spi_bus_config_t rgb_cfg_bus = {
        .mode    = BK_DISPLAY_SPI_BUS_MODE_SW,
        .clk_pin = GPIO_8,
        .csx_pin = GPIO_28,
        .sda_pin = GPIO_9,
    };

    bk_lcd_panel_config_t panel_config = {
        .reset_pin = GPIO_6,
    };

    const bk_display_rgb_panel_t *rgb_panels[8];
    uint32_t num_rgb_panels = bk_lcd_get_rgb_panel_list(rgb_panels, 8);
    if (panel_name != NULL && panel_name[0] != '\0')
    {
        for (uint32_t i = 0; i < num_rgb_panels; i++)
        {
            if (rgb_panels[i] != NULL && rgb_panels[i]->name != NULL &&
                os_strcmp(panel_name, rgb_panels[i]->name) == 0)
            {
                panel = rgb_panels[i];
                LOGI("Using RGB panel: %s\n", panel_name);
                break;
            }
        }
        if (panel == NULL)
        {
            LOGE("RGB panel not found: %s\n", panel_name);
            ret = AVDK_ERR_UNSUPPORTED;
            goto err;
        }
    }
    else
    {
        if (num_rgb_panels > 0 && rgb_panels[0] != NULL)
        {
            panel = rgb_panels[0];
            LOGI("No panel specified, using first RGB panel: %s\n", panel->name ? panel->name : "<noname>");
        }
        else
        {
            LOGE("No RGB panel available\n");
            goto err;
        }
    }

    /* Pin the SW SPI bus' panel-IO wire format to the chosen panel's
     * protocol. spi_cmd_16bit is the panel descriptor's own metadata
     * (single source of truth); the bus_cfg copy lets bus_new() build
     * the panel-IO with the right encoding before any panel-common
     * tx_param call. */
    rgb_cfg_bus.cmd_width = panel->spi_cmd_16bit ? 16 : 8;

    AVDK_GOTO_ON_ERROR(bk_display_spi_bus_new(&context->spi_bus_handle, &rgb_cfg_bus), err, TAG, "display rgb cfg-bus new err\n");
    AVDK_GOTO_ON_ERROR(bk_lcd_rgb_panel_new(context->spi_bus_handle, &panel_config, panel, &context->panel_handle), err, TAG, "create panel err\n");

    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&context->dpu_ctlr_handle, context->panel_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(context->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(context->dpu_ctlr_handle), err, TAG, "display open err\n");

    context->width = panel->timing.h_size;
    context->height = panel->timing.v_size;
    context->format = format;
    context->pixel_width = (format == BK_PIXEL_FORMAT_ARGB8888) ? 4 : 2;

    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
    bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_output_high(GPIO_7);

    LOGI("RGB LCD opened: format=%d, %dx%d\n", format, context->width, context->height);
    return AVDK_ERR_OK;
err:
    if (context->spi_bus_handle)
    {
        bk_display_bus_delete(context->spi_bus_handle);
        context->spi_bus_handle = NULL;
    }
    return ret;
}

avdk_err_t lcd_example_rgb_close(display_ctx_t *context)
{
    if (context == NULL)
        return AVDK_ERR_INVAL;

    if (context->thread)
    {
        context->enable = 0;
        bk_display_ctlr_handle_t handle_to_close = context->dpu_ctlr_handle;
        context->dpu_ctlr_handle = NULL;
        rtos_get_semaphore(&context->sem, BEKEN_WAIT_FOREVER);
        context->thread = NULL;
        rtos_deinit_semaphore(&context->sem);
        if (handle_to_close)
        {
            bk_display_deinit(handle_to_close);
            bk_display_delete(handle_to_close);
        }
    }
    else
    {
        if (context->dpu_ctlr_handle)
        {
            bk_display_deinit(context->dpu_ctlr_handle);
            bk_display_delete(context->dpu_ctlr_handle);
            context->dpu_ctlr_handle = NULL;
        }
        if (context->sem)
            rtos_deinit_semaphore(&context->sem);
    }

    if (context->panel_handle)
    {
        bk_lcd_panel_delete(context->panel_handle);
        context->panel_handle = NULL;
    }
    if (context->spi_bus_handle)
    {
        bk_display_bus_delete(context->spi_bus_handle);
        context->spi_bus_handle = NULL;
    }
    LOGI("RGB LCD closed\n");
    return AVDK_ERR_OK;
}

// ----- Flush thread: direct malloc -> fill -> flush -> delay (no queue) -----

static void lcd_example_flush_thread(void *args)
{
    display_ctx_t *disp_ctx = (display_ctx_t *)args;
    uint32_t frame_size = get_frame_size(disp_ctx->format, disp_ctx->width, disp_ctx->height);

    rtos_set_semaphore(&disp_ctx->sem);

    while (disp_ctx->enable)
    {
        void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
        if (frame == NULL)
        {
            LOGE("Failed to allocate frame buffer, size=%u\n", (unsigned)frame_size);
            rtos_delay_milliseconds(50);
            continue;
        }

        if (disp_ctx->format == BK_PIXEL_FORMAT_RGB565)
        {
            uint16_t fill_color = rgb565_color_map[disp_ctx->pixel_index++];
            if (disp_ctx->pixel_index >= sizeof(rgb565_color_map) / sizeof(rgb565_color_map[0]))
                disp_ctx->pixel_index = 0;
            fill_rgb565_frame(frame, disp_ctx->width, disp_ctx->height, fill_color);
        }
        else if (disp_ctx->format == BK_PIXEL_FORMAT_ARGB8888)
        {
            uint16_t rgb565 = rgb565_color_map[disp_ctx->pixel_index++];
            if (disp_ctx->pixel_index >= sizeof(rgb565_color_map) / sizeof(rgb565_color_map[0]))
                disp_ctx->pixel_index = 0;
            uint32_t argb = 0xFF000000U | ((uint32_t)((rgb565 >> 11) & 0x1F) * 255 / 31) << 16
                | ((uint32_t)((rgb565 >> 5) & 0x3F) * 255 / 63) << 8
                | ((uint32_t)(rgb565 & 0x1F) * 255 / 31);
            fill_argb8888_frame(frame, disp_ctx->width, disp_ctx->height, argb);
        }
        else
        {
            LOGE("Unsupported format: %d\n", disp_ctx->format);
            bk_frame_buffer_free(frame);
            rtos_delay_milliseconds(50);
            continue;
        }

        if (disp_ctx->dpu_ctlr_handle == NULL)
        {
            bk_frame_buffer_free(frame);
            break;
        }

        if (bk_display_flush(disp_ctx->dpu_ctlr_handle, frame, display_frame_free) != AVDK_ERR_OK)
        {
            LOGE("bk_display_flush failed\n");
            bk_frame_buffer_free(frame);
        }
        rtos_delay_milliseconds(500);
    }

    disp_ctx->thread = NULL;
    rtos_set_semaphore(&disp_ctx->sem);
    rtos_delete_thread(NULL);
}

avdk_err_t lcd_example_flush_thread_start(display_ctx_t *context)
{
    avdk_err_t ret = rtos_init_semaphore_ex(&context->sem, 1, 0);
    if (ret != BK_OK)
    {
        LOGE("Failed to init semaphore for flush thread\n");
        return ret;
    }
    context->enable = 1;

    ret = rtos_create_thread(&context->thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "rgb_flush",
                             (beken_thread_function_t)lcd_example_flush_thread,
                             1024 * 8,
                             context);
    if (ret != BK_OK)
    {
        LOGE("Failed to create flush thread\n");
        rtos_deinit_semaphore(&context->sem);
        context->enable = 0;
        return ret;
    }

    ret = rtos_get_semaphore(&context->sem, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK)
    {
        context->enable = 0;
        rtos_delete_thread(&context->thread);
        rtos_deinit_semaphore(&context->sem);
        return ret;
    }
    LOGI("RGB flush thread started\n");
    return AVDK_ERR_OK;
}

/* Stop only the flush thread (keep DPU/panel/bus alive) for runtime format switch. */
avdk_err_t lcd_example_rgb_flush_thread_stop(display_ctx_t *context)
{
    if (context == NULL)
        return AVDK_ERR_INVAL;

    if (context->thread == NULL)
        return AVDK_ERR_OK;

    context->enable = 0;
    rtos_get_semaphore(&context->sem, BEKEN_WAIT_FOREVER);
    rtos_deinit_semaphore(&context->sem);
    return AVDK_ERR_OK;
}

// ----- Runtime pixel-format switch primitive (returns status) -----

avdk_err_t rgb_lcd_do_switch(display_ctx_t *context, const bk_display_pixel_format_config_t *config)
{
    bk_pixel_format_t old_format;
    uint8_t old_pixel_width;

    if (context == NULL || config == NULL || context->spi_bus_handle == NULL)
    {
        LOGE("rgb_lcd_do_switch: LCD is off or invalid args\r\n");
        return AVDK_ERR_INVAL;
    }

    old_format = context->format;
    old_pixel_width = context->pixel_width;

    if (lcd_example_rgb_flush_thread_stop(context) != AVDK_ERR_OK)
    {
        LOGE("lcd_example_rgb_flush_thread_stop failed\r\n");
        return AVDK_ERR_GENERIC;
    }

    if (bk_display_ioctl(context->dpu_ctlr_handle, BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT, (void *)config) != AVDK_ERR_OK)
    {
        LOGE("bk_display_ioctl runtime switch failed\r\n");
        context->format = old_format;
        context->pixel_width = old_pixel_width;
        lcd_example_flush_thread_start(context);
        return AVDK_ERR_GENERIC;
    }

    context->format = config->format;
    context->pixel_width = (config->format == BK_PIXEL_FORMAT_ARGB8888) ? 4 : 2;

    if (lcd_example_flush_thread_start(context) != AVDK_ERR_OK)
    {
        LOGE("lcd_example_flush_thread_start failed after switch\r\n");
        context->format = old_format;
        context->pixel_width = old_pixel_width;
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

// ----- IT cases: run in a worker thread, display then auto-close, print result at exit -----

static volatile uint8_t s_rgb_case_running = 0;
static volatile uint8_t s_rgb_case_abort = 0;
static beken_thread_t s_rgb_case_thread = NULL;
static const char *s_rgb_case_panel = NULL;

/* Result string spacing must match .it.csv exactly: caller controls leading space in name. */
static void rgb_lcd_log_result(const char *name, bool pass, const char *stage)
{
    if (pass)
        LOGI("[RESULT][PASS]%s success\r\n", name);
    else
        LOGI("[RESULT][FAIL]%s failed at %s\r\n", name, stage);
}

static void rgb_lcd_case_hold(uint32_t duration_ms)
{
    uint32_t waited = 0;

    while (waited < duration_ms && s_rgb_case_abort == 0)
    {
        rtos_delay_milliseconds(RGB_LCD_CASE_POLL_MS);
        waited += RGB_LCD_CASE_POLL_MS;
    }
}

static void rgb_lcd_case_teardown(display_ctx_t *context)
{
    if (context == NULL)
        return;
    if (context->spi_bus_handle != NULL || context->thread != NULL)
        lcd_example_rgb_close(context);
}

static bool rgb_lcd_case_open_rgb565(display_ctx_t *context, const char **stage)
{
    os_memset(context, 0, sizeof(*context));
    if (lcd_example_rgb_open(context, s_rgb_case_panel, BK_PIXEL_FORMAT_RGB565) != AVDK_ERR_OK)
    {
        *stage = "rgb_open";
        return false;
    }
    if (lcd_example_flush_thread_start(context) != AVDK_ERR_OK)
    {
        *stage = "flush_start";
        return false;
    }
    return true;
}

static void rgb_lcd_rgb565_task(void *arg)
{
    display_ctx_t *context = (display_ctx_t *)arg;
    const char *stage = "open";
    bool pass;

    pass = rgb_lcd_case_open_rgb565(context, &stage);

    if (pass)
        rgb_lcd_case_hold(RGB_LCD_BOOT_HOLD_MS);

    rgb_lcd_case_teardown(context);

    /* Print result at the very end, after the LCD has displayed and the case exits. */
    rgb_lcd_log_result(" rgb_lcd_display_rgb565_test", pass, stage);

    s_rgb_case_thread = NULL;
    s_rgb_case_running = 0;
    rtos_delete_thread(NULL);
}

static void rgb_lcd_switch_format_task(void *arg)
{
    /* RGB example supports RGB565 and ARGB8888; cycle each 5s then return to RGB565. */
    static const bk_display_pixel_format_config_t s_switch_cfgs[] = {
        { .format = BK_PIXEL_FORMAT_RGB565,   .decompress = false },
        { .format = BK_PIXEL_FORMAT_ARGB8888, .decompress = true  },
        { .format = BK_PIXEL_FORMAT_RGB565,   .decompress = false },
    };
    static const char *const s_switch_names[] = { "RGB565", "ARGB8888", "RGB565" };

    display_ctx_t *context = (display_ctx_t *)arg;
    const char *stage = "start";
    bool pass = true;
    uint32_t i;

    /* Ensure LCD is on (previous case may have auto-closed -> proves independence). */
    if (context->spi_bus_handle == NULL)
        pass = rgb_lcd_case_open_rgb565(context, &stage);

    if (pass)
    {
        for (i = 0; i < sizeof(s_switch_cfgs) / sizeof(s_switch_cfgs[0]); i++)
        {
            if (rgb_lcd_do_switch(context, &s_switch_cfgs[i]) != AVDK_ERR_OK)
            {
                stage = s_switch_names[i];
                pass = false;
                break;
            }
            LOGI("switched to %s\r\n", s_switch_names[i]);
            rtos_delay_milliseconds(RGB_LCD_SWITCH_STEP_MS);
        }
    }

    /* Switching sequence itself is the display window (3 x 5s); no extra hold. */
    rgb_lcd_case_teardown(context);

    /* Print result at the very end, after the LCD has displayed and the case exits. */
    rgb_lcd_log_result("rgb_lcd_switch_format", pass, stage);

    s_rgb_case_thread = NULL;
    s_rgb_case_running = 0;
    rtos_delete_thread(NULL);
}

static void rgb_lcd_case_start(display_ctx_t *context, const char *panel_name,
                               beken_thread_function_t entry, const char *thread_name)
{
    /* Preempt any in-flight case so each case starts from a clean state. */
    if (s_rgb_case_running)
    {
        s_rgb_case_abort = 1;
        while (s_rgb_case_running)
            rtos_delay_milliseconds(RGB_LCD_CASE_POLL_MS);
    }

    s_rgb_case_abort = 0;
    s_rgb_case_running = 1;
    s_rgb_case_panel = panel_name;

    if (rtos_create_thread(&s_rgb_case_thread,
                           BEKEN_DEFAULT_WORKER_PRIORITY,
                           thread_name,
                           entry,
                           1024 * 4,
                           context) != BK_OK)
    {
        LOGE("create %s thread failed\r\n", thread_name);
        s_rgb_case_thread = NULL;
        s_rgb_case_running = 0;
    }
}

void rgb_lcd_rgb565_test(display_ctx_t *context, const char *panel_name)
{
    rgb_lcd_case_start(context, panel_name,
                       (beken_thread_function_t)rgb_lcd_rgb565_task,
                       "rgb565_case");
}

void rgb_lcd_switch_format_test(display_ctx_t *context, const char *panel_name)
{
    rgb_lcd_case_start(context, panel_name,
                       (beken_thread_function_t)rgb_lcd_switch_format_task,
                       "rgb_switch_case");
}
