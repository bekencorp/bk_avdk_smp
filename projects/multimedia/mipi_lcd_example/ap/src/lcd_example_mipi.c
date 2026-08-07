// Copyright 2020-2021 Beken
// MIPI LCD example: DSI open/close and flush thread start/stop (single file, no queue).
// Flush thread uses direct malloc -> fill -> flush(..., display_frame_free) -> delay per frame.

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>          /* umbrella: bus + panel + display ctlr */
#include <common/avdk_pixel_types.h>
#include "lcd_example.h"
#include "lcd_image.h"
#include <modules/pm.h>

/* Per-case display duration (rough hold, not strictly timed) before auto teardown. */
#define MIPI_LCD_CASE_DURATION_MS  (20 * 1000)
#define MIPI_LCD_CASE_POLL_MS      (100)
#define MIPI_LCD_SWITCH_STEP_MS    (5 * 1000)

#define MAX_PANEL_COUNT 32
#define TAG "mipi_lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)



static avdk_err_t bk_lodoen_enable(bool enable)
{
    int ldo_en = enable ? PM_AUXLDO_ENABLE : PM_AUXLDO_DISABLE;
    LOGI("%s, vddio enable: %d\n", __func__, ldo_en);

    pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
    auxldo_cfg.ldo   = AUXLDOS_SEL_1P8V;
    auxldo_cfg.out   = PM_AUXLDO_1P8V_OUT_1P8V;
    auxldo_cfg.user  = PM_AUXLDO_USER_DISPLAY;
    auxldo_cfg.state = ldo_en;
    AVDK_RETURN_ON_ERROR(bk_pm_auxldo_ctrl_vote(&auxldo_cfg), TAG, "display 1p8v ldo vote failed");
    rtos_delay_milliseconds(1);
    return AVDK_ERR_OK;
}

/* Flush free callback: display driver calls this when done with the frame; we just free it. */
static avdk_err_t display_frame_free(void *args)
{
    if (args != NULL)
        bk_frame_buffer_free(args);
    return AVDK_ERR_OK;
}



/* Frame size in bytes for given format and resolution. */
static uint32_t get_frame_size(bk_pixel_format_t format, uint16_t width, uint16_t height, bool decompress)
{
    switch (format)
    {
    case BK_PIXEL_FORMAT_RGB565:
        return (uint32_t)width * height * 2;
    case BK_PIXEL_FORMAT_RGB888:
        return (uint32_t)width * height * 3;
    case BK_PIXEL_FORMAT_NV12:
        return (uint32_t)width * height + (uint32_t)width * ((height + 1) / 2);
    case BK_PIXEL_FORMAT_ARGB8888:
        if (decompress)
            return (uint32_t)width * height * 2;
        else
            return (uint32_t)width * height * 4;
    default:
        return (uint32_t)width * height * 2;
    }
}

const uint16_t rgb565_color_map[] = {
    RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_YELLOW, RGB565_CYAN,
    RGB565_MAGENTA, RGB565_WHITE, RGB565_BLACK, RGB565_GRAY, RGB565_BROWN,
    RGB565_PINK, RGB565_ORANGE, RGB565_PURPLE,
};

static void rgb565_to_nv12(uint16_t rgb565, uint8_t *y_val, uint8_t *u_val, uint8_t *v_val)
{
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    uint8_t r = (r5 << 3) | (r5 >> 2);
    uint8_t g = (g6 << 2) | (g6 >> 4);
    uint8_t b = (b5 << 3) | (b5 >> 2);
    int y = (77 * r + 150 * g + 29 * b) >> 8;
    int u = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
    int v = ((128 * r - 107 * g - 21 * b) >> 8) + 128;
    if (y < 0) y = 0; if (y > 255) y = 255;
    if (u < 0) u = 0; if (u > 255) u = 255;
    if (v < 0) v = 0; if (v > 255) v = 255;
    *y_val = (uint8_t)y;
    *u_val = (uint8_t)u;
    *v_val = (uint8_t)v;
}

static void fill_frame_rgb565(void *frame_base, uint16_t width, uint16_t height,
                             uint16_t fill_color, uint16_t *sram_buffer, uint32_t sram_buffer_size)
{
    if (!frame_base || !sram_buffer)
        return;
    uint32_t row_size = width * sizeof(uint16_t);
    uint32_t pixels_per_8_rows = width * 8;
    uint16_t *sram_ptr = sram_buffer;
    uint16_t *sram_end = sram_ptr + pixels_per_8_rows;
    while (sram_ptr < sram_end)
        *sram_ptr++ = fill_color;

    uint32_t total_rows = height;
    uint32_t copied_rows = 0;
    uint8_t *frame_ptr = (uint8_t *)frame_base;
    while (copied_rows < total_rows)
    {
        uint32_t rows_to_copy = (total_rows - copied_rows > 8) ? 8 : (total_rows - copied_rows);
        uint32_t copy_size = row_size * rows_to_copy;
        os_memcpy(frame_ptr, sram_buffer, copy_size);
        frame_ptr += copy_size;
        copied_rows += rows_to_copy;
    }
}

static void fill_frame_rgb888(void *frame_base, uint16_t width, uint16_t height,
                              uint16_t rgb565_color, uint8_t *sram_buffer, uint32_t sram_buffer_size)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint32_t row_size;
    uint32_t copied_rows;
    uint8_t *frame_ptr;

    if (!frame_base || !sram_buffer)
        return;

    r = ((rgb565_color >> 11) & 0x1F) << 3;
    g = ((rgb565_color >> 5) & 0x3F) << 2;
    b = (rgb565_color & 0x1F) << 3;
    row_size = width * 3;

    for (uint32_t i = 0; i < row_size * 8; i += 3)
    {
        sram_buffer[i] = r;
        sram_buffer[i + 1] = g;
        sram_buffer[i + 2] = b;
    }

    copied_rows = 0;
    frame_ptr = (uint8_t *)frame_base;
    while (copied_rows < height)
    {
        uint32_t rows_to_copy = (height - copied_rows > 8) ? 8 : (height - copied_rows);
        uint32_t copy_size = row_size * rows_to_copy;
        os_memcpy(frame_ptr, sram_buffer, copy_size);
        frame_ptr += copy_size;
        copied_rows += rows_to_copy;
    }
}

static void fill_frame_nv12(void *frame_base, uint16_t width, uint16_t height,
                            uint16_t rgb565_color, uint8_t *sram_buffer, uint32_t sram_buffer_size)
{
    if (!frame_base || !sram_buffer)
        return;
    uint8_t y_val, u_val, v_val;
    rgb565_to_nv12(rgb565_color, &y_val, &u_val, &v_val);
    uint32_t y_row_size = width;
    uint32_t y_8rows_size = y_row_size * 8;
    uint32_t uv_4rows_size = y_row_size * 4;
    os_memset(sram_buffer, y_val, y_8rows_size);
    uint8_t *uv_buffer = sram_buffer + y_8rows_size;
    uint16_t uv_pattern = (u_val << 8) | v_val;
    uint16_t *uv_ptr_16 = (uint16_t *)uv_buffer;
    uint16_t *uv_end_16 = uv_ptr_16 + (uv_4rows_size / 2);
    while (uv_ptr_16 < uv_end_16)
        *uv_ptr_16++ = uv_pattern;

    uint8_t *frame_y = (uint8_t *)frame_base;
    uint8_t *frame_uv = frame_y + (width * height);
    uint32_t total_rows = height;
    uint32_t copied_rows = 0;
    while (copied_rows < total_rows)
    {
        uint32_t rows_to_copy = (total_rows - copied_rows > 8) ? 8 : (total_rows - copied_rows);
        uint32_t y_copy_size = y_row_size * rows_to_copy;
        os_memcpy(frame_y, sram_buffer, y_copy_size);
        uint32_t uv_rows_to_copy = (rows_to_copy + 1) / 2;
        uint32_t uv_copy_size = y_row_size * uv_rows_to_copy;
        os_memcpy(frame_uv, uv_buffer, uv_copy_size);
        frame_y += y_copy_size;
        frame_uv += uv_copy_size;
        copied_rows += rows_to_copy;
    }
}

static void fill_frame_argb8888(void *frame_base, uint16_t width, uint16_t height)
{
    if (!frame_base)
        return;
    if (width == 1080 && height == 1920)
    {
        uint32_t i, n = (1088U * 1920U) / sizeof(compressed_1088x1920_argb8888);
        for (i = 0; i < n; i++)
            os_memcpy((uint8_t *)frame_base + i * sizeof(compressed_1088x1920_argb8888),
                      (void *)compressed_1088x1920_argb8888, sizeof(compressed_1088x1920_argb8888));
    }
    else if (width == 720 && height == 1280)
    {
        uint32_t i, n = (720U * 1280U) / sizeof(compressed_720x1280_argb8888);
        for (i = 0; i < n; i++)
            os_memcpy((uint8_t *)frame_base + i * sizeof(compressed_720x1280_argb8888),
                      (void *)compressed_720x1280_argb8888, sizeof(compressed_720x1280_argb8888));
    }
}

// ----- DSI open / close -----

avdk_err_t lcd_example_dsi_open(display_ctx_t *context, const char *panel_name, bk_pixel_format_t format)
{
    int ret = AVDK_ERR_GENERIC;
    const bk_display_dsi_panel_t *panel = NULL;

    bk_display_dpu_config_t dpu_config = {0};
    dpu_config.video.enable = true;
    dpu_config.video.decompress = (format == BK_PIXEL_FORMAT_ARGB8888);
    dpu_config.video.format = format;

    const bk_lcd_panel_config_t panel_config = {
        .reset_pin = GPIO_60,
    };

    bk_lodoen_enable(true);
    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&context->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");

    const bk_display_dsi_panel_t *panel_list[MAX_PANEL_COUNT];
    uint32_t panel_count = bk_lcd_get_mipi_panel_list(panel_list, MAX_PANEL_COUNT);
    panel = NULL;
    for (uint32_t i = 0; i < panel_count; i++)
    {
        if (panel_list[i] != NULL && panel_list[i]->name != NULL &&
            os_strcmp(panel_name, panel_list[i]->name) == 0)
        {
            panel = panel_list[i];
            LOGI("Found panel: %s\n", panel_name);
            break;
        }
    }
    if (panel == NULL)
    {
        LOGE("Unsupported panel name: %s\n", panel_name);
        ret = AVDK_ERR_UNSUPPORTED;
        goto err;
    }

    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(context->dis_bus_handle, &panel_config, panel, &context->panel_handle),
                       err, TAG, "create panel err\n");
    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&context->dpu_ctlr_handle, context->panel_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(context->dpu_ctlr_handle), err, TAG, "display init err\n");
    AVDK_GOTO_ON_ERROR(bk_display_open(context->dpu_ctlr_handle), err, TAG, "display open err\n");

    context->width = panel->timing.h_size;
    context->height = panel->timing.v_size;
    context->format = format;
    context->decompress = dpu_config.video.decompress;

    if (os_strcmp(panel_name, "lt8912b_mipi_1280x720") != 0)
    {
        gpio_dev_unmap(GPIO_7);
        BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
        BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
        bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_high(GPIO_7);
    }

    LOGI("LCD opened: panel=%s, format=%d, %dx%d\n", panel_name, format, context->width, context->height);
    return AVDK_ERR_OK;
err:
    return ret;
}

avdk_err_t lcd_example_dsi_close(display_ctx_t *context)
{
    if (context == NULL)
        return AVDK_ERR_INVAL;
    bk_gpio_set_output_low(GPIO_7);
    if (context->dpu_ctlr_handle)
    {
        bk_display_deinit(context->dpu_ctlr_handle);
        bk_display_delete(context->dpu_ctlr_handle);
        context->dpu_ctlr_handle = NULL;
    }

    if (context->panel_handle)
    {
        bk_lcd_panel_delete(context->panel_handle);
        context->panel_handle = NULL;
    }
    if (context->dis_bus_handle)
    {
        bk_display_bus_delete(context->dis_bus_handle);
        context->dis_bus_handle = NULL;
    }
    bk_lodoen_enable(false);

    LOGI("%s complete\n", __func__);
    return AVDK_ERR_OK;
}

// ----- Flush thread: direct malloc -> fill -> flush -> delay (no queue) -----

static void lcd_example_flush_thread(void *args)
{
    display_ctx_t *disp_ctx = (display_ctx_t *)args;
    avdk_err_t ret;
    uint32_t frame_size = get_frame_size(disp_ctx->format, disp_ctx->width, disp_ctx->height, disp_ctx->decompress);

    if (frame_size == 0)
    {
        LOGE("Unsupported frame config: format=%d decompress=%d %ux%u\n",
             disp_ctx->format, disp_ctx->decompress, disp_ctx->width, disp_ctx->height);
        rtos_set_semaphore(&disp_ctx->sem);
        rtos_delete_thread(NULL);
        return;
    }

    rtos_set_semaphore(&disp_ctx->sem);

    uint32_t sram_buffer_size = 0;
    void *sram_buffer = NULL;
    if (disp_ctx->format == BK_PIXEL_FORMAT_RGB565)
    {
        sram_buffer_size = disp_ctx->width * sizeof(uint16_t) * 8;
        sram_buffer = os_malloc(sram_buffer_size);
        if (!sram_buffer)
        {
            LOGE("Failed to allocate SRAM buffer for RGB565\n");
            rtos_set_semaphore(&disp_ctx->sem);
            rtos_delete_thread(NULL);
            return;
        }
    }
    else if (disp_ctx->format == BK_PIXEL_FORMAT_RGB888)
    {
        sram_buffer_size = disp_ctx->width * 3 * 8;
        sram_buffer = os_malloc(sram_buffer_size);
        if (!sram_buffer)
        {
            LOGE("Failed to allocate SRAM buffer for RGB888\n");
            rtos_set_semaphore(&disp_ctx->sem);
            rtos_delete_thread(NULL);
            return;
        }
    }
    else if (disp_ctx->format == BK_PIXEL_FORMAT_NV12)
    {
        sram_buffer_size = disp_ctx->width * 12;
        sram_buffer = os_malloc(sram_buffer_size);
        if (!sram_buffer)
        {
            LOGE("Failed to allocate SRAM buffer for NV12\n");
            rtos_set_semaphore(&disp_ctx->sem);
            rtos_delete_thread(NULL);
            return;
        }
    }
    void *frame = NULL;
    while (disp_ctx->enable)
    {
        frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
        if (frame == NULL)
        {
            LOGE("Failed to allocate frame buffer, size=%u\n", (unsigned)frame_size);
            rtos_delay_milliseconds(50);
            continue;
        }

        uint16_t fill_color = rgb565_color_map[disp_ctx->pixel_index++];
        if (disp_ctx->pixel_index >= sizeof(rgb565_color_map) / sizeof(rgb565_color_map[0]))
            disp_ctx->pixel_index = 0;

        switch (disp_ctx->format)
        {
        case BK_PIXEL_FORMAT_RGB565:
            fill_frame_rgb565(frame, disp_ctx->width, disp_ctx->height, fill_color, (uint16_t *)sram_buffer, sram_buffer_size);
            break;
        case BK_PIXEL_FORMAT_RGB888:
            fill_frame_rgb888(frame, disp_ctx->width, disp_ctx->height, fill_color, (uint8_t *)sram_buffer, sram_buffer_size);
            break;
        case BK_PIXEL_FORMAT_NV12:
            fill_frame_nv12(frame, disp_ctx->width, disp_ctx->height, fill_color, (uint8_t *)sram_buffer, sram_buffer_size);
            break;
        case BK_PIXEL_FORMAT_ARGB8888:
            fill_frame_argb8888(frame, disp_ctx->width, disp_ctx->height);
            break;
        default:
            LOGE("Unsupported pixel format: %d\n", disp_ctx->format);
            bk_frame_buffer_free(frame);
            rtos_delay_milliseconds(50);
            continue;
        }

        if (disp_ctx->dpu_ctlr_handle == NULL)
        {
            bk_frame_buffer_free(frame);
            break;
        }

        ret = bk_display_flush(disp_ctx->dpu_ctlr_handle, frame, display_frame_free);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("bk_display_flush failed: %d\n", ret);
            bk_frame_buffer_free(frame);
        }
        frame = NULL;
        rtos_delay_milliseconds(500);
    }
    if (sram_buffer != NULL)
        os_free(sram_buffer);
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
                             "mipi_flush",
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
    LOGI("Flush thread started\n");
    return AVDK_ERR_OK;
}

avdk_err_t lcd_example_flush_thread_stop(display_ctx_t *context)
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

avdk_err_t mipi_lcd_do_switch(display_ctx_t *context, const bk_display_pixel_format_config_t *config)
{
    bk_pixel_format_t old_format;
    uint8_t old_decompress;

    if (context == NULL || config == NULL || context->dis_bus_handle == NULL)
    {
        LOGE("mipi_lcd_do_switch: LCD is off or invalid args\r\n");
        return AVDK_ERR_INVAL;
    }

    old_format = context->format;
    old_decompress = context->decompress;

    if (lcd_example_flush_thread_stop(context) != AVDK_ERR_OK)
    {
        LOGE("lcd_example_flush_thread_stop failed\r\n");
        return AVDK_ERR_GENERIC;
    }

    if (bk_display_ioctl(context->dpu_ctlr_handle, BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT, (void *)config) != AVDK_ERR_OK)
    {
        LOGE("bk_display_ioctl runtime switch failed\r\n");
        context->format = old_format;
        context->decompress = old_decompress;
        lcd_example_flush_thread_start(context);
        return AVDK_ERR_GENERIC;
    }

    context->format = config->format;
    context->decompress = config->decompress;

    if (lcd_example_flush_thread_start(context) != AVDK_ERR_OK)
    {
        LOGE("lcd_example_flush_thread_start failed after switch\r\n");
        context->format = old_format;
        context->decompress = old_decompress;
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

// ----- IT cases: run in a worker thread, print result early, hold then auto-close -----

static volatile uint8_t s_mipi_case_running = 0;
static volatile uint8_t s_mipi_case_abort = 0;
static beken_thread_t s_mipi_case_thread = NULL;
static const char *s_mipi_case_panel = NULL;

/* Result string spacing must match .it.csv exactly: caller controls leading space in name. */
static void mipi_lcd_log_result(const char *name, bool pass, const char *stage)
{
    if (pass)
        LOGI("[RESULT][PASS]%s success\r\n", name);
    else
        LOGI("[RESULT][FAIL]%s failed at %s\r\n", name, stage);
}

static void mipi_lcd_case_hold(void)
{
    uint32_t waited = 0;

    while (waited < MIPI_LCD_CASE_DURATION_MS && s_mipi_case_abort == 0)
    {
        rtos_delay_milliseconds(MIPI_LCD_CASE_POLL_MS);
        waited += MIPI_LCD_CASE_POLL_MS;
    }
}

static void mipi_lcd_case_teardown(display_ctx_t *context)
{
    if (context == NULL)
        return;
    if (context->thread != NULL)
        lcd_example_flush_thread_stop(context);
    if (context->dis_bus_handle != NULL)
        lcd_example_dsi_close(context);
}

static bool mipi_lcd_case_open_argb8888(display_ctx_t *context, const char **stage)
{
    os_memset(context, 0, sizeof(*context));
    if (lcd_example_dsi_open(context, s_mipi_case_panel, BK_PIXEL_FORMAT_ARGB8888) != AVDK_ERR_OK)
    {
        *stage = "dsi_open";
        return false;
    }
    if (lcd_example_flush_thread_start(context) != AVDK_ERR_OK)
    {
        *stage = "flush_start";
        return false;
    }
    return true;
}

static void mipi_lcd_argb8888_task(void *arg)
{
    display_ctx_t *context = (display_ctx_t *)arg;
    const char *stage = "open";
    bool pass;

    pass = mipi_lcd_case_open_argb8888(context, &stage);

    if (pass)
        mipi_lcd_case_hold();

    mipi_lcd_case_teardown(context);

    /* Print result at the very end, after the LCD has displayed and the case exits. */
    mipi_lcd_log_result(" mipi_lcd_display_argb8888_test", pass, stage);

    s_mipi_case_thread = NULL;
    s_mipi_case_running = 0;
    rtos_delete_thread(NULL);
}

static void mipi_lcd_switch_format_task(void *arg)
{
    /* Cycle each format 5s, then return to the first (RGB565) for 5s. */
    static const bk_display_pixel_format_config_t s_switch_cfgs[] = {
        { .format = BK_PIXEL_FORMAT_RGB565,   .decompress = false },
        { .format = BK_PIXEL_FORMAT_RGB888,   .decompress = false },
        { .format = BK_PIXEL_FORMAT_ARGB8888, .decompress = true  },
        { .format = BK_PIXEL_FORMAT_RGB565,   .decompress = false },
    };
    static const char *const s_switch_names[] = { "RGB565", "RGB888", "ARGB8888", "RGB565" };

    display_ctx_t *context = (display_ctx_t *)arg;
    const char *stage = "start";
    bool pass = true;
    uint32_t i;

    /* Ensure LCD is on (previous case may have auto-closed -> proves independence). */
    if (context->dis_bus_handle == NULL)
        pass = mipi_lcd_case_open_argb8888(context, &stage);

    if (pass)
    {
        for (i = 0; i < sizeof(s_switch_cfgs) / sizeof(s_switch_cfgs[0]); i++)
        {
            if (mipi_lcd_do_switch(context, &s_switch_cfgs[i]) != AVDK_ERR_OK)
            {
                stage = s_switch_names[i];
                pass = false;
                break;
            }
            LOGI("switched to %s\r\n", s_switch_names[i]);
            rtos_delay_milliseconds(MIPI_LCD_SWITCH_STEP_MS);
        }
    }

    /* Switching sequence itself is the display window (4 x 5s); no extra hold. */
    mipi_lcd_case_teardown(context);

    /* Print result at the very end, after the LCD has displayed and the case exits. */
    mipi_lcd_log_result("mipi_lcd_switch_format", pass, stage);

    s_mipi_case_thread = NULL;
    s_mipi_case_running = 0;
    rtos_delete_thread(NULL);
}

static void mipi_lcd_case_start(display_ctx_t *context, const char *panel_name,
                                beken_thread_function_t entry, const char *thread_name)
{
    /* Preempt any in-flight case so each case starts from a clean state. */
    if (s_mipi_case_running)
    {
        s_mipi_case_abort = 1;
        while (s_mipi_case_running)
            rtos_delay_milliseconds(MIPI_LCD_CASE_POLL_MS);
    }

    s_mipi_case_abort = 0;
    s_mipi_case_running = 1;
    s_mipi_case_panel = panel_name;

    if (rtos_create_thread(&s_mipi_case_thread,
                           BEKEN_DEFAULT_WORKER_PRIORITY,
                           thread_name,
                           entry,
                           1024 * 4,
                           context) != BK_OK)
    {
        LOGE("create %s thread failed\r\n", thread_name);
        s_mipi_case_thread = NULL;
        s_mipi_case_running = 0;
    }
}

void mipi_lcd_argb8888_test(display_ctx_t *context, const char *panel_name)
{
    mipi_lcd_case_start(context, panel_name,
                        (beken_thread_function_t)mipi_lcd_argb8888_task,
                        "mipi_argb_case");
}

void mipi_lcd_switch_format_test(display_ctx_t *context, const char *panel_name)
{
    mipi_lcd_case_start(context, panel_name,
                        (beken_thread_function_t)mipi_lcd_switch_format_task,
                        "mipi_switch_case");
}