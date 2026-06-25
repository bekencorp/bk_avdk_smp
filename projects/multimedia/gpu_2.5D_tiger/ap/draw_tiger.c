#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
//#include <driver/gicv2.h>
#include <components/media_types.h>
#include <components/bk_frame_buffer.h>
#include "cli.h"
#include "sys_driver.h"

#include <components/bk_display.h>          /* umbrella: bus + panel + display ctlr */
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include "gpu_core.h"

#include "tiger_paths.h"
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>


#define TAG "app"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void bk_psram_init();

typedef struct
{
    uint8_t enable;
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    uint16_t lcd_width;
    uint16_t lcd_height;
    void *frame_buffer[2];
    uint8_t frame_buffer_index;
    vg_lite_buffer_t draw_buffer;
    beken_thread_t draw_thd;
} display_ctx_t;

static display_ctx_t *g_disp_ctx = NULL;
static beken_semaphore_t s_buf_ready_sem = NULL;

static int on_frame_released(void *frame)
{
    rtos_set_semaphore(&s_buf_ready_sem);
    return BK_OK;
}

static int zoomOut    = 0;
static int scaleCount = 0;

void animateTiger(vg_lite_matrix_t *matrix)
{
    if (zoomOut) {
        vg_lite_scale(1.25, 1.25, matrix);
        if (0 == --scaleCount) {
            zoomOut = 0;
        }
    } else {
        vg_lite_scale(0.8, 0.8, matrix);
        if (20 == ++scaleCount) {
            zoomOut = 1;
        }
    }
    
    vg_lite_rotate(1.0, matrix);
}

static void animateTiger_scale(vg_lite_matrix_t* matrix)
{
    static float scale = 0.35f;
    static uint32_t isZoomOut = 1;

    if (isZoomOut)
    {
        scale = scale * 1.1f;
        if (scale > 6.0f)
        {
            scale = 6.0f;
            isZoomOut = 0;
        }
    }
    else
    {
        scale = scale / 1.1f;
        if (scale < 0.35f)
        {
            scale = 0.35f;
            isZoomOut = 1;
        }
    }

    vg_lite_identity(matrix);
    vg_lite_translate((float)400.0f, (float)800.0f, matrix);
    vg_lite_scale(scale, scale, matrix);
}

static void animateTiger_rotate(vg_lite_matrix_t* matrix)
{
    static float rotate = 0.0f;
    static uint32_t isClockwise = 1;
    static float rotateStep = 4.0f;

    if (isClockwise)
    {
        rotate = rotate + rotateStep;
        if (rotate >= 360.0f)
        {
            rotate = 360.0f;
            isClockwise = 0;
        }
    }
    else
    {
        rotate = rotate - rotateStep;
        if (rotate <= 0.0f)
        {
            rotate = 0.0f;
            isClockwise = 1; 
        }
    }

    vg_lite_identity(matrix);
    vg_lite_translate((float)480.0f, (float)960.0f, matrix);
    vg_lite_scale(4.0f, 4.0f, matrix);
    vg_lite_rotate(rotate, matrix);
}

static void animateTiger_translate(vg_lite_matrix_t* matrix)
{
    static const vg_lite_float_t SCREEN_WIDTH = 1080.0f;
    static const vg_lite_float_t SCREEN_HEIGHT = 1920.0f;
    static const vg_lite_float_t MOVE_STEP = 8.0f;

    static vg_lite_float_t x = 96.0f;
    static vg_lite_float_t y = 96.0f;

    static int x_direction = 1;
    static int y_direction = 1;

    x += MOVE_STEP * x_direction;
    if (x <= 96.0f) {
        x = 96.0f;
        x_direction = 1;
    } else if (x >= SCREEN_WIDTH - 224) {
        x = SCREEN_WIDTH - 224;
        x_direction = -1;
    }

    y += MOVE_STEP * y_direction;
    if (y <= 96.0f) {
        y = 96.0f;
        y_direction = 1;
    } else if (y >= SCREEN_HEIGHT - 256) {
        y = SCREEN_HEIGHT - 256;
        y_direction = -1;
    }

    vg_lite_identity(matrix);
    vg_lite_translate(x, y, matrix);
    vg_lite_scale(3.0f, 3.0f, matrix);
}

// Animation state enumeration
typedef enum {
    ANIM_STATE_SCALE = 0,
    ANIM_STATE_ROTATE,
    ANIM_STATE_TRANSLATE,
    ANIM_STATE_MAX
} anim_state_t;

#define ANIM_DURATION_SCALE     600
#define ANIM_DURATION_ROTATE    600
#define ANIM_DURATION_TRANSLATE 1000

static void animateTiger_sequence(vg_lite_matrix_t* matrix, uint32_t width, uint32_t height)
{
    static anim_state_t current_state = ANIM_STATE_SCALE;
    static uint32_t frame_counter = 0;
    static uint32_t state_durations[ANIM_STATE_MAX] = {
        ANIM_DURATION_SCALE,
        ANIM_DURATION_ROTATE,
        ANIM_DURATION_TRANSLATE
    };

    switch (current_state)
    {
        case ANIM_STATE_SCALE:
            animateTiger_scale(matrix);
            break;
        case ANIM_STATE_ROTATE:
            animateTiger_rotate(matrix);
            break;
        case ANIM_STATE_TRANSLATE:
            animateTiger_translate(matrix);
            break;
        default:
            animateTiger_scale(matrix);
            break;
    }

    frame_counter++;
    if (frame_counter >= state_durations[current_state])
    {
        frame_counter = 0;
        anim_state_t prev_state = current_state;
        current_state = (anim_state_t)((current_state + 1) % ANIM_STATE_MAX);

        /* One full round (scale -> rotate -> translate) just finished */
        if (prev_state == ANIM_STATE_TRANSLATE)
        {
            LOGI("tiger animation round complete\r\n");
        }
    }
}

static void redraw(vg_lite_buffer_t* rt, vg_lite_matrix_t* matrix)
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    uint8_t count;

    if (rt == NULL)
    {
        LOGI("vg_lite_get_renderTarget error\r\n");
        while (1);
    }

    vg_lite_clear(rt, NULL, 0xFF782277);
    animateTiger_sequence(matrix, g_disp_ctx->lcd_width, g_disp_ctx->lcd_height);

    for (count = 0; count < pathCount; count++)
    {
        error = vg_lite_draw(rt, &path[count], VG_LITE_FILL_EVEN_ODD, matrix, VG_LITE_BLEND_NONE, color_data[count]);
        if (error)
        {
            LOGI("vg_lite_draw() returned error %d\r\n", error);
            return;
        }
        vg_lite_finish();
    }

    return;
}

static void render_tiger_task(void *arg)
{
    vg_lite_matrix_t matrix;

    LOGI("render_tiger_task\r\n");

    // GPU rendering: redraw the tiger animation into the current frame buffer
    redraw(&g_disp_ctx->draw_buffer, &matrix);

    // Submit the drawn buffer to DPU for display
    bk_display_flush(g_disp_ctx->dpu_ctlr_handle,
                     g_disp_ctx->frame_buffer[g_disp_ctx->frame_buffer_index],
                     on_frame_released);

    while (1)
    {
        // Swap frame buffer.
        g_disp_ctx->frame_buffer_index = (g_disp_ctx->frame_buffer_index + 1) % 2;
        vg_lite_allocate_with_data(&g_disp_ctx->draw_buffer,
                                   g_disp_ctx->frame_buffer[g_disp_ctx->frame_buffer_index],
                                   NULL, NULL, NULL);

        // Wait for DPU to release this buffer
        rtos_get_semaphore(&s_buf_ready_sem, BEKEN_WAIT_FOREVER);

        // GPU rendering: redraw the tiger animation into the current frame buffer
        redraw(&g_disp_ctx->draw_buffer, &matrix);

        // Submit the drawn buffer to DPU for display
        bk_display_flush(g_disp_ctx->dpu_ctlr_handle,
                         g_disp_ctx->frame_buffer[g_disp_ctx->frame_buffer_index],
                         on_frame_released);
    }
}

avdk_err_t draw_tiger(void)
{
    uint32_t id = 0;
    avdk_err_t ret = AVDK_ERR_GENERIC;

    LOGI("%s\n", __func__);

    bk_frame_buffer_init();

    g_disp_ctx = os_malloc(sizeof(display_ctx_t));

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

    AVDK_GOTO_ON_ERROR(bk_display_dsi_bus_new(&g_disp_ctx->dis_bus_handle, NULL), err, TAG, "display dsi bus new err\n");

#if CONFIG_LCD_HX8399C_MIPI_1080x1920
    AVDK_GOTO_ON_ERROR(bk_lcd_mipi_panel_new(g_disp_ctx->dis_bus_handle, &panel_config, &lcd_device_hx8399c_mipi_1080x1920, &g_disp_ctx->panel_handle),
                       err, TAG, "create panel err\n");
#endif

    const bk_display_timing_t *panel_timing = &lcd_device_hx8399c_mipi_1080x1920.timing;

    dpu_config.video.disp_x = 0;
    dpu_config.video.disp_y = 0;
    dpu_config.video.disp_w = panel_timing->h_size;
    dpu_config.video.disp_h = panel_timing->v_size;

    AVDK_GOTO_ON_ERROR(bk_display_dpu_ctlr_new(&g_disp_ctx->dpu_ctlr_handle, g_disp_ctx->panel_handle, &dpu_config), err, TAG, "display dpu ctlr new err\n");
    AVDK_GOTO_ON_ERROR(bk_display_init(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display init err\n");
    bk_lcd_panel_read_id(g_disp_ctx->panel_handle, &id);
    LOGI("read lcd id: 0x%x\n", id);
    AVDK_GOTO_ON_ERROR(bk_display_open(g_disp_ctx->dpu_ctlr_handle), err, TAG, "display open err\n");

    /* enable backlight */
    gpio_dev_unmap(GPIO_7);
    BK_LOG_ON_ERR(bk_gpio_enable_output(GPIO_7));
    BK_LOG_ON_ERR(bk_gpio_pull_up(GPIO_7));
    bk_gpio_set_capacity(GPIO_7, GPIO_DRIVER_CAPACITY_3);  // Enhance GPIO Driver Capacity
    bk_gpio_set_output_high(GPIO_7);

    g_disp_ctx->lcd_width = panel_timing->h_size + 8;
    g_disp_ctx->lcd_height = panel_timing->v_size;
    g_disp_ctx->frame_buffer_index = 0;
    g_disp_ctx->frame_buffer[0] = (void*)((uint32_t)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, (panel_timing->h_size + 64) * panel_timing->v_size) & 0xFFFFFFC0);
    g_disp_ctx->frame_buffer[1] = (void*)((uint32_t)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, (panel_timing->h_size + 64) * panel_timing->v_size) & 0xFFFFFFC0);

    bk_gpu_driver_init();
    vg_lite_init(panel_timing->h_size / 2, panel_timing->v_size / 2);

    memset(&g_disp_ctx->draw_buffer,0,sizeof(vg_lite_buffer_t));
    vg_lite_buffer_t *pdraw_buffer = &g_disp_ctx->draw_buffer;

    pdraw_buffer->width  = g_disp_ctx->lcd_width;
    pdraw_buffer->height = g_disp_ctx->lcd_height;
    pdraw_buffer->format = VG_LITE_BGRA8888;
    pdraw_buffer->tiled = 1;
    pdraw_buffer->compress_mode = VG_LITE_DEC_HV_SAMPLE;
    vg_lite_allocate_with_data(pdraw_buffer,
                               g_disp_ctx->frame_buffer[g_disp_ctx->frame_buffer_index],
                               NULL,
                               NULL, NULL);

    rtos_init_semaphore_ex(&s_buf_ready_sem, 1, 1);

    AVDK_RETURN_ON_ERROR(rtos_create_thread(&g_disp_ctx->draw_thd,
                       BEKEN_DEFAULT_WORKER_PRIORITY,
                       "gpu",
                       (beken_thread_function_t)render_tiger_task,
                       1024 * 5,
                       NULL), TAG, "create render_tiger_task thread failed");

    return BK_OK;

err:
	//TODO FIXME: free resource

	return ret;
}