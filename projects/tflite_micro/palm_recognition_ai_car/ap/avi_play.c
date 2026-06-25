#include <os/os.h>
#include <os/mem.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <components/bk_lcd_panel.h>
#include <components/bk_video_player/bk_video_player_engine.h>
#include <components/bk_video_player/container_parser/bk_video_player_avi_parser.h>
#include <components/bk_video_player/video_decoder/bk_video_player_hw_jpeg_decoder.h>
#if CONFIG_VFS
#include "bk_partition.h"
#include "bk_posix.h"
#elif CONFIG_FATFS
#include "ff.h"
#include "diskio.h"
#endif
#include <driver/gpio.h>
#include "gpio_driver.h"

#define TAG "avi_play"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

extern const bk_lcd_panel_t lcd_device_jd9853;

static bk_display_ctlr_handle_t lcd_display_handle = NULL;
static bk_video_player_engine_handle_t g_video_player_handle = NULL;
static bool g_video_player_opened = false;
static bool g_rgb565_byte_swap = true;

bk_display_spi_bus_config_t spi_ctlr_config = {
    .mode = BK_DISPLAY_SPI_BUS_MODE_HW,
    .lcd_panel = &lcd_device_jd9853,
    .spi_id = 0,
    .dc_pin = GPIO_50,
    .reset_pin = GPIO_52,
    .te_pin = 0,
};

#if CONFIG_VFS
static int vfs_fd = -1;
#elif CONFIG_FATFS
static FATFS *pfs = NULL;
#endif
static void bk_sdcard_mount(void)
{
#if CONFIG_VFS
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
	partition.mount_path = "fatfs";

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
	if (ret != BK_OK) {
		LOGE("mount failed, ret=%d\r\n", ret);
		return;
	}

	LOGD("mount success\r\n");

#elif CONFIG_FATFS
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
#endif
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
    if (frame != NULL) {
        bk_frame_buffer_free(frame);
    }

    return AVDK_ERR_OK;
}

static avdk_err_t video_packet_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0) {
        return AVDK_ERR_INVAL;
    }

    void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, buffer->length);
    if (frame == NULL) {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->data = frame;
    buffer->frame_buffer = frame;
    buffer->user_data = NULL;
    return AVDK_ERR_OK;
}

static void video_buffer_free_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL) {
        return;
    }

    if (buffer->frame_buffer != NULL) {
        bk_frame_buffer_free(buffer->frame_buffer);
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->pts = 0;
    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
}

static avdk_err_t video_output_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0) {
        return AVDK_ERR_INVAL;
    }

    void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, buffer->length);
    if (frame == NULL) {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->data = frame;
    buffer->frame_buffer = frame;
    buffer->user_data = NULL;
    return AVDK_ERR_OK;
}

static avdk_err_t audio_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0) {
        return AVDK_ERR_INVAL;
    }

    buffer->data = os_malloc(buffer->length);
    if (buffer->data == NULL) {
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
    return AVDK_ERR_OK;
}

static void audio_buffer_free_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL) {
        return;
    }

    if (buffer->data != NULL) {
        os_free(buffer->data);
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->pts = 0;
    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
}

static void audio_decode_complete_cb(void *user_data, const video_player_audio_packet_meta_t *meta, video_player_buffer_t *buffer)
{
    (void)user_data;
    (void)meta;

    audio_buffer_free_cb(NULL, buffer);
}

static void rgb565_byte_swap(uint8_t *data, uint32_t length)
{
    if (data == NULL) {
        return;
    }

    uint16_t *pixels = (uint16_t *)data;
    uint32_t pixel_count = length / sizeof(uint16_t);

    for (uint32_t i = 0; i < pixel_count; i++) {
        uint16_t pixel = pixels[i];
        pixels[i] = (uint16_t)((pixel >> 8) | (pixel << 8));
    }
}

static void video_decode_complete_cb(void *user_data, const video_player_video_frame_meta_t *meta, video_player_buffer_t *buffer)
{
    (void)user_data;
    (void)meta;

    if (buffer == NULL || buffer->data == NULL) {
        return;
    }

    if (g_rgb565_byte_swap) {
        rgb565_byte_swap(buffer->data, buffer->length);
    }

    if (lcd_display_handle == NULL) {
        video_buffer_free_cb(NULL, buffer);
        return;
    }

    avdk_err_t ret = bk_display_flush(lcd_display_handle, buffer->data, display_frame_free_cb);
    if (ret != AVDK_ERR_OK) {
        LOGW("%s: bk_display_flush failed, ret=%d\n", __func__, ret);
        video_buffer_free_cb(NULL, buffer);
        return;
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->pts = 0;
    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
}

static void playback_finished_cb(void *user_data, const char *file_path)
{
    (void)user_data;

    if (g_video_player_handle == NULL || file_path == NULL) {
        return;
    }

    avdk_err_t ret = bk_video_player_engine_play_file(g_video_player_handle, file_path);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s: replay failed, ret=%d, file=%s\r\n", __func__, ret, file_path);
    }
}

bk_err_t bk_avi_player_start(const char *file_path)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (file_path == NULL) {
        return BK_FAIL;
    }

    bk_sdcard_mount();

    if (g_video_player_handle != NULL && g_video_player_opened) {
        ret = bk_video_player_engine_play_file(g_video_player_handle, file_path);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_video_player_engine_play_file failed, ret=%d\r\n", ret);
            return BK_FAIL;
        }

        LOGI("%s replay complete\n", __func__);
        return BK_OK;
    }

    if (lcd_display_handle == NULL) {
        ret = bk_display_spi_ctlr_new(&lcd_display_handle, &spi_ctlr_config);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_display_spi_ctlr_new failed, ret=%d!\n", ret);
            goto fail;
        }

        LOGD("bk_display_spi_ctlr_new success!\n");

        ret = bk_display_init(lcd_display_handle);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_display_init failed, ret=%d!\n", ret);
            goto fail;
        }

        ret = bk_display_open(lcd_display_handle);
        if (ret != AVDK_ERR_OK) {
            LOGE("bk_display_open failed, ret=%d!\n", ret);
            goto fail;
        }

        lcd_backlight_open(GPIO_29);
    }

    bk_video_player_config_t cfg;
    os_memset(&cfg, 0, sizeof(cfg));

    cfg.video.parser_to_decode_buffer_count = 2;
    cfg.video.decode_to_output_buffer_count = 2;
    cfg.video.packet_buffer_alloc_cb = video_packet_buffer_alloc_cb;
    cfg.video.packet_buffer_free_cb = video_buffer_free_cb;
    cfg.video.buffer_alloc_cb = video_output_buffer_alloc_cb;
    cfg.video.buffer_free_cb = video_buffer_free_cb;
    cfg.video.decode_complete_cb = video_decode_complete_cb;
    cfg.video.output_format = PIXEL_FMT_RGB565;
    cfg.audio.parser_to_decode_buffer_count = 2;
    cfg.audio.decode_to_output_buffer_count = 2;
    cfg.audio.buffer_alloc_cb = audio_buffer_alloc_cb;
    cfg.audio.buffer_free_cb = audio_buffer_free_cb;
    cfg.audio.decode_complete_cb = audio_decode_complete_cb;
    cfg.playback_finished_cb = playback_finished_cb;

    ret = bk_video_player_engine_new(&g_video_player_handle, &cfg);
    if (ret != AVDK_ERR_OK || g_video_player_handle == NULL) {
        LOGE("bk_video_player_engine_new failed, ret=%d\r\n", ret);
        g_video_player_handle = NULL;
        goto fail;
    }

    ret = bk_video_player_engine_register_container_parser(g_video_player_handle, bk_video_player_get_avi_parser_ops());
    if (ret != AVDK_ERR_OK) {
        LOGE("register avi parser failed, ret=%d\r\n", ret);
        goto fail;
    }

    ret = bk_video_player_engine_register_video_decoder(g_video_player_handle, bk_video_player_get_hw_jpeg_decoder_ops());
    if (ret != AVDK_ERR_OK) {
        LOGE("register hw jpeg decoder failed, ret=%d\r\n", ret);
        goto fail;
    }

    ret = bk_video_player_engine_open(g_video_player_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_video_player_engine_open failed, ret=%d\r\n", ret);
        goto fail;
    }
    g_video_player_opened = true;

    ret = bk_video_player_engine_set_file_path(g_video_player_handle, file_path);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_video_player_engine_set_file_path failed, ret=%d\r\n", ret);
        goto fail;
    }

    ret = bk_video_player_engine_play(g_video_player_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_video_player_engine_play failed, ret=%d\r\n", ret);
        goto fail;
    }

    LOGI("%s complete\n", __func__);

    return BK_OK;

fail:
    if (g_video_player_handle != NULL) {
        if (g_video_player_opened) {
            bk_video_player_engine_close(g_video_player_handle);
            g_video_player_opened = false;
        }
        bk_video_player_engine_delete(g_video_player_handle);
        g_video_player_handle = NULL;
    }

    if (lcd_display_handle != NULL) {
        bk_display_delete(lcd_display_handle);
        lcd_display_handle = NULL;
    }

    return BK_FAIL;
}