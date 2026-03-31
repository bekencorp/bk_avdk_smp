#include <os/os.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#if CONFIG_FATFS
#include "ff.h"
#include "diskio.h"
#endif
#include "avi_player.h"
#include <driver/gpio.h>
#include "gpio_driver.h"

#define TAG "avi_play"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern const bk_lcd_panel_t lcd_device_jd9853;

static bk_display_bus_handle_t lcd_display_handle = NULL;
static bk_avi_player_t *g_avi_player_handle = NULL;
static beken_thread_t g_avi_player_thread = NULL;
static beken_semaphore_t g_avi_player_sem = NULL;
static bool g_avi_player_is_running = false;
static uint16_t *g_rgb565_framebuffer = NULL;

bk_display_spi_bus_config_t spi_ctlr_config = {
    .lcd_panel = &lcd_device_jd9853,
    .spi_id = 0,
    .dc_pin = GPIO_50,
    .reset_pin = GPIO_52,
    .te_pin = 0,
};

static FATFS *pfs = NULL;
static void bk_sdcard_mount(void)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];

    if (pfs != NULL) {
        os_free(pfs);
    }

    pfs = os_malloc(sizeof(FATFS));
	if (NULL == pfs) {
		LOGE("f_mount malloc failed!\r\n");
		goto failed_mount;
	}

    sprintf(cFileName, "%d:", DISK_NUMBER_SDIO_SD);
    fr = f_mount(pfs, cFileName, 1);
    if (fr != FR_OK) {
        LOGE("f_mount failed:%d\r\n", fr);
    } else {
        LOGD("f_mount OK!\r\n");
    }

failed_mount:
    LOGD("----- bk_sdcard_mount over  -----\r\n\r\n");
}

avdk_err_t lcd_backlight_open(uint8_t bl_io)
{
    gpio_dev_unmap(bl_io);
    BK_LOG_ON_ERR(bk_gpio_enable_output(bl_io));
    BK_LOG_ON_ERR(bk_gpio_pull_up(bl_io));
    bk_gpio_set_output_high(bl_io);

    return AVDK_ERR_OK;
}

static avdk_err_t display_frame_free_cb(void *frame)
{
    return AVDK_ERR_OK;
}

static inline uint8_t clip_u8(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

/**
 * Convert one NV12 frame (YUV420) to RGB565.
 * Optional byte swap (high/low byte of each 16-bit pixel) for display endianness.
 *
 * NV12 layout:
 *  - Y plane:  width * height bytes
 *  - UV plane: width * height / 2 bytes, interleaved U V U V...
 *
 * @param src_nv12   Pointer to NV12 frame buffer.
 * @param width      Image width in pixels (must be > 0 and even).
 * @param height     Image height in pixels (must be > 0 and even).
 * @param dst_rgb565 Output buffer for RGB565 pixels, size >= width * height.
 * @param byte_swap  If true, swap high/low byte of each RGB565 word (e.g. for display bus).
 *
 * @return 0 on success, negative value on error.
 */
int nv12_to_rgb565(const uint8_t *src_nv12,
                   int width,
                   int height,
                   uint16_t *dst_rgb565,
                   bool byte_swap)
{
    if (!src_nv12 || !dst_rgb565) {
        return -1; /* invalid pointer */
    }

    if (width <= 0 || height <= 0) {
        return -2; /* invalid size */
    }

    /* NV12 requires even width and height for 4:2:0 chroma subsampling */
    if ((width & 1) != 0 || (height & 1) != 0) {
        return -3; /* width/height must be even */
    }

    /* avoid overflow in size computation */
    size_t wh = (size_t)width * (size_t)height;
    if (wh == 0) {
        return -4;
    }

    const uint8_t *y_plane  = src_nv12;
    const uint8_t *uv_plane = src_nv12 + wh;

    /* Process two horizontal pixels per iteration (same UV pair) to reduce UV fetches and loop overhead. */
    for (int y = 0; y < height; ++y) {
        int uv_row = (y >> 1) * width;
        for (int x = 0; x < width; x += 2) {
            int uv_index = uv_row + x;
            uint8_t U = uv_plane[uv_index + 0];
            int E = (int)U - 128;
            uint8_t V = uv_plane[uv_index + 1];
            int F = (int)V - 128;

            /* Precompute chroma terms shared by both pixels (ITU-R BT.601). */
            int term_ug = -100 * E;
            int term_vr = 409 * F;
            int term_vg = -208 * F;
            int term_ub = 516 * E;

            for (int k = 0; k < 2; ++k) {
                int xi = x + k;
                int y_index = y * width + xi;
                int C = (int)y_plane[y_index] - 16;
                if (C < 0) {
                    C = 0;
                }

                int R = (298 * C + term_vr + 128) >> 8;
                int G = (298 * C + term_ug + term_vg + 128) >> 8;
                int B = (298 * C + term_ub + 128) >> 8;

                uint16_t r5 = (uint16_t)(clip_u8(R) >> 3);
                uint16_t g6 = (uint16_t)(clip_u8(G) >> 2);
                uint16_t b5 = (uint16_t)(clip_u8(B) >> 3);
                uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

                if (byte_swap) {
                    rgb565 = (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
                }
                dst_rgb565[y_index] = rgb565;
            }
        }
    }

    return 0;
}

static void avi_player_thread(beken_thread_arg_t data)
{
    bk_err_t ret;
    uint32_t delay_time = 0;
    uint32_t start_time, end_time;

    g_avi_player_is_running = true;
    rtos_set_semaphore(&g_avi_player_sem);

    g_avi_player_handle->pos = 0;
    delay_time = 1000 / (uint32_t)g_avi_player_handle->avi->fps;

    while (g_avi_player_is_running)
    {
        if (g_avi_player_handle->pos == g_avi_player_handle->video_num) {
            g_avi_player_handle->pos = 0;
        }

        start_time = rtos_get_time();
        ret = bk_avi_player_video_parse();
        if (ret < 0) {
            LOGE("%s %d bk_avi_player_video_parse failed\r\n", __func__, __LINE__);
            g_avi_player_handle->pos++;
            continue;
        }
        g_avi_player_handle->pos++;

        nv12_to_rgb565(g_avi_player_handle->framebuffer, g_avi_player_handle->avi->width, g_avi_player_handle->avi->height, g_rgb565_framebuffer, g_avi_player_handle->swap_flag);

        bk_display_bus_flush(lcd_display_handle, (uint8_t *)g_rgb565_framebuffer, display_frame_free_cb);

        end_time = rtos_get_time();
        LOGV("bk_avi_player_video_parse time: %d ms\n", end_time - start_time);

        if (end_time - start_time > delay_time) {
            LOGV("bk_avi_player_video_parse time is too long, just delay 2ms, time: %d ms\n",  end_time - start_time);
            rtos_delay_milliseconds(2);
        } else {
            rtos_delay_milliseconds(delay_time - (end_time - start_time));
        }
    }

    g_avi_player_thread = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t bk_avi_player_start(const char *file_path)
{
    bk_err_t ret = BK_OK;
    bk_avi_player_config_t avi_player_config = {0};

    bk_sdcard_mount();

    avi_player_config.file_path = file_path;
    avi_player_config.output_format = AVI_PLAYER_OUTPUT_FORMAT_YUYV;
    avi_player_config.segment_flag = false;
    avi_player_config.rgb565_byte_swap_flag = true;

    ret = bk_avi_player_open(&avi_player_config);
    if (ret != BK_OK) {
        LOGE("bk_avi_player_open failed!\r\n");
        return ret;
    }

    g_avi_player_handle = bk_avi_player_get_handle();
    if (g_avi_player_handle == NULL) {
        LOGE("bk_avi_player_get_g_avi_player_handle failed!\r\n");
        return ret;
    }

    g_rgb565_framebuffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, 240 * 304 * 2);
    if (g_rgb565_framebuffer == NULL) {
        LOGE("g_rgb565_framebuffer malloc failed!\r\n");
        return ret;
    }

    ret = bk_display_spi_bus_new(&lcd_display_handle, &spi_ctlr_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_spi_new failed!\n");
        return ret;
    }

    LOGD("bk_display_spi_new success!\n");
    ret = bk_display_bus_enable(lcd_display_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_display_open failed!\n");
        return ret;
    }

    lcd_backlight_open(GPIO_29);

    ret = rtos_init_semaphore_ex(&g_avi_player_sem, 1, 0);
    if (ret != BK_OK) {
        LOGE("rtos_init_semaphore_ex failed!\r\n");
        return ret;
    }

    ret = rtos_create_thread(&g_avi_player_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                             "avi_player_thread",
                             (beken_thread_function_t)avi_player_thread,
                             1024 * 4,
                             NULL);

    if (ret != BK_OK) {
        LOGE("rtos_create_thread failed!\r\n");
        return ret;
    }
    
    rtos_get_semaphore(&g_avi_player_sem, BEKEN_WAIT_FOREVER);

    LOGI("%s complete\n", __func__);

    return BK_OK;
}