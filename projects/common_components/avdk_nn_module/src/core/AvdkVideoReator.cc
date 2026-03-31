#include "AvdkDetectionModel.h"
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include "AvdkVideoReator.h"

#include "app_camera.h"
#include "app_display.h"
#if CONFIG_VG_LITE_GPU
#include "app_gpu.h"
#endif

#include "driver/drv_tp.h"
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#include "beken_ui.h"
#endif

#if CONFIG_COMMON_LVGL_UI
#include "beken_ui.h"
#endif

const char* TAG = "vi-reator";

#define LOGI(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE((char*)TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD((char*)TAG, ##__VA_ARGS__)
#define LOGV(...)

AvdkVideoReator::AvdkVideoReator(AvdkDetectionModel *detection_model)
{
    this->detection_model = detection_model;
    this->infer_thread_running = 0;
    this->display_thread_running = 0;
    this->infer_thread = NULL;
    this->display_thread = NULL;
    this->thread = NULL;
    this->detect_enable = 0;

    /* Initialize semaphore for infer thread synchronization */
    rtos_init_semaphore(&infer_thread_sem, 1);
    /* Initialize semaphore for display thread synchronization */
    rtos_init_semaphore(&display_thread_sem, 1);
}

static void WorkerThreadEntry(void *arg)
{
    if (arg)
        ((AvdkVideoReator *)arg)->WorkerThread();
}

static void InferThreadEntry(void *arg)
{
    if (arg)
        ((AvdkVideoReator *)arg)->InferThread();
}

static void DisplayThreadEntry(void *arg)
{
    if (arg)
        ((AvdkVideoReator *)arg)->DisplayThread();
}

static void aov_frame_buffer_free(void *frame, void *args)
{
    int (*cb)(void *args) = (int (*)(void *args))args;
    cb(frame);
}

#define WIDTH (1088)
#define HEIGHT (832)

#if CONFIG_LVGL
static lv_image_dsc_t s_display_image_dsc;
static bool s_display_image_dsc_inited = false;
#endif

static void aoo_detection_flush_cb(void *args, void *frame_buffer, int (*cb)(void *args))
{
    bk_gpu_blit_config_t blit_config = {
        .src_x = 0,
        .src_y = 0,
        .src_width = WIDTH,
        .src_height = HEIGHT,
        .src_format = BK_PIXEL_FORMAT_RGB565,
        .dst_x = 0,
        .dst_y = WIDTH,
        .args = (void*)cb,
        .free = aov_frame_buffer_free,
    };

    LOGI("%s %d\n", __func__, __LINE__);

    bk_gpu_blit_set(app_gpu_handle_get(), frame_buffer, &blit_config);
}

void AvdkVideoReator::WorkerThread()
{
    LOGI("AvdkVideoReator::WorkerThread\n");
    int ret = BK_OK;

    detection_model->LogEnable(true);
    detection_model->init();

    if (BK_PIXEL_FORMAT_RGB888 == detection_model->getFormat())
    {
        frame_size = detection_model->getWidth() * detection_model->getHeight() * 4;
    }
    else
    {
        frame_size = bk_image_size_get(detection_model->getWidth(), detection_model->getHeight(), detection_model->getFormat());
    }


    soruce_frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);

    if (soruce_frame == NULL)
    {
        LOGE("soruce_frame malloc fail, size: %d\n", frame_size);
        return;
    }

    if (mode == AVDK_VIDEO_REATOR_MODE_DISPLAY)
    {
        OpenCameraWithDisplay();
        OpenDisplay();
        OpenGui();
    }
    else
    {
        OpenCameraWithoutDisplay();
        OpenDisplayWithoutGPU();
    }


    while(1)
    {
        ret = ReadCameraFrame(soruce_frame, frame_size, -1);

        if (ret != BK_OK)
        {
            LOGE("###########%s, %d read frame failed##########\n", __func__, __LINE__);
            rtos_delay_milliseconds(1000 * 2);
            continue;
        }

        LOGV("read frame: %p, size: %d, %d\n", soruce_frame, frame_size, ret);

        ret = detection_model->run(soruce_frame, frame_size);
    }

    LOGI("############### Thread Exit ################\n");

    rtos_delete_thread(NULL);
}

int AvdkVideoReator::init_model()
{
    int ret = BK_OK;

    if (detection_model == NULL)
    {
        LOGE("detection_model is NULL\n");
        return BK_FAIL;
    }

    detection_model->LogEnable(true);

    ret = detection_model->init();
    if (ret != BK_OK)
    {
        LOGE("detection_model init failed, ret: %d\n", ret);
        return ret;
    }

    return BK_OK;
}

void AvdkVideoReator::InferThread()
{
    LOGI("AvdkVideoReator::InferThread\n");
    int ret = BK_OK;

    if (BK_PIXEL_FORMAT_RGB888 == detection_model->getFormat())
    {
        frame_size = detection_model->getWidth() * detection_model->getHeight() * 4;
    }
    else
    {
        frame_size = bk_image_size_get(detection_model->getWidth(), detection_model->getHeight(), detection_model->getFormat());
    }

    soruce_frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);

    if (soruce_frame == NULL)
    {
        LOGE("soruce_frame malloc fail, size: %d\n", frame_size);
        infer_thread_running = 0;
        /* Signal thread start failure */
        rtos_set_semaphore(&infer_thread_sem);
        rtos_delete_thread(NULL);
    }

    /* Signal thread started successfully */
    rtos_set_semaphore(&infer_thread_sem);

    /* Inference loop: read frames and run inference without opening peripherals */
    while (infer_thread_running)
    {
        ret = ReadCameraFrame(soruce_frame, frame_size, -1);

        if (ret != BK_OK)
        {
            LOGE("###########%s, %d read frame failed##########\n", __func__, __LINE__);
            rtos_delay_milliseconds(1000 * 2);
            continue;
        }

        LOGV("read frame: %p, size: %d, %d\n", soruce_frame, frame_size, ret);

        ret = detection_model->run(soruce_frame, frame_size);
    }

    /* Free allocated frame buffer */
    if (soruce_frame != NULL)
    {
        bk_frame_buffer_free(soruce_frame);
        soruce_frame = NULL;
    }

    LOGI("############### Infer Thread Exit ################\n");

    /* Signal thread exit before deleting */
    rtos_set_semaphore(&infer_thread_sem);

    rtos_delete_thread(NULL);
}

int argb8888_frame_to_rgb565(uint32_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
    if (src == NULL || dst == NULL || width == 0 || height == 0) {
        LOGE("%s, %d src or dst is NULL or width or height is 0\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    for (uint32_t i = 0; i < width * height; ++i) {
        uint32_t argb = src[i];

        uint8_t r = (argb >> 16) & 0xFF;
        uint8_t g = (argb >> 8)  & 0xFF;
        uint8_t b = (argb >> 0)  & 0xFF;

        uint16_t r5 = (uint16_t)(r >> 3);
        uint16_t g6 = (uint16_t)(g >> 2);
        uint16_t b5 = (uint16_t)(b >> 3);

        dst[i] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    }

    return BK_OK;
}

void AvdkVideoReator::DisplayThread()
{
    LOGI("AvdkVideoReator::DisplayThread\n");
    int ret = BK_OK;

    if (BK_PIXEL_FORMAT_RGB888 == detection_model->getFormat())
    {
        frame_size = detection_model->getWidth() * detection_model->getHeight() * 4;
    }
    else
    {
        frame_size = bk_image_size_get(detection_model->getWidth(), detection_model->getHeight(), detection_model->getFormat());
    }

    display_frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);

    if (display_frame == NULL)
    {
        LOGE("display_frame malloc fail, size: %d\n", frame_size);
        display_thread_running = 0;
        /* Signal thread start failure */
        rtos_set_semaphore(&display_thread_sem);
        rtos_delete_thread(NULL);
    }

    uint16_t *rgb565_frame = (uint16_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);
    if (rgb565_frame == NULL)
    {
        bk_frame_buffer_free(display_frame);
        display_frame = NULL;
        LOGE("rgb565_frame malloc fail, size: %d\n", frame_size);
        display_thread_running = 0;
        /* Signal thread start failure */
        rtos_set_semaphore(&display_thread_sem);
        rtos_delete_thread(NULL);
    }

    /* Signal thread started successfully */
    rtos_set_semaphore(&display_thread_sem);

    /* Display loop: read frames from camera without changing peripherals state */
    while (display_thread_running)
    {
        ret = ReadCameraFrame(display_frame, frame_size, -1);

        if (ret != BK_OK)
        {
            LOGE("###########%s, %d read frame failed##########\n", __func__, __LINE__);
            rtos_delay_milliseconds(10);
            continue;
        }
        LOGI("display read frame: %p, size: %d, %d\n", display_frame, frame_size, ret);

        if (detect_enable)
        {
            ret = detection_model->run(display_frame, frame_size);
        }

        argb8888_frame_to_rgb565((uint32_t *)display_frame, rgb565_frame, detection_model->getWidth(), detection_model->getHeight());

        if (!s_display_image_dsc_inited)
        {
            uint32_t width = detection_model->getWidth();
            uint32_t height = detection_model->getHeight();

            s_display_image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
            s_display_image_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
            s_display_image_dsc.header.flags = 0;
            s_display_image_dsc.header.w = (uint16_t)width;
            s_display_image_dsc.header.h = (uint16_t)height;
            s_display_image_dsc.header.stride = width * 2;
            s_display_image_dsc.data_size = width * height * 2;
            s_display_image_dsc.data = (const uint8_t *)rgb565_frame;

            s_display_image_dsc_inited = true;
        }

        lv_vendor_disp_lock();
        lv_image_set_src(bk_lv_tool_ui.page_1_image_3, &s_display_image_dsc);
        lv_vendor_disp_unlock();
    }

    /* Free allocated frame buffer */
    if (display_frame != NULL)
    {
        bk_frame_buffer_free(display_frame);
        display_frame = NULL;
    }

    if (rgb565_frame != NULL)
    {
        bk_frame_buffer_free(rgb565_frame);
        rgb565_frame = NULL;
    }

    LOGI("############### Display Thread Exit ################\n");

    /* Signal thread exit before deleting */
    rtos_set_semaphore(&display_thread_sem);

    rtos_delete_thread(NULL);
}


int AvdkVideoReator::start()
{
    int ret = rtos_create_hsram_thread(&thread,
        BEKEN_DEFAULT_WORKER_PRIORITY,
        "worker_thread",
        (beken_thread_function_t)WorkerThreadEntry,
        1024 * 20,
        (void *)this);

    if (ret != BK_OK)
    {
    LOGE("create worker_thread fail\n");
    }

    return 0;
}

int AvdkVideoReator::start(avdk_video_reator_mode_t mode)
{
    LOGI("AvdkVideoReator::start mode: %d\n", mode);
    this->mode = mode;
    return start();
}

int AvdkVideoReator::start_detect()
{
    LOGI("AvdkVideoReator::start_detect\n");
    this->mode = AVDK_VIDEO_REATOR_MODE_NODISPLAY;
    detect_enable = 1;
    return BK_OK;
}

int AvdkVideoReator::stop_detect()
{
    LOGI("AvdkVideoReator::stop_detect\n");
    detect_enable = 0;
    return BK_OK;
}

int AvdkVideoReator::start_infer()
{
    LOGI("AvdkVideoReator::start_infer\n");

    if (infer_thread_running)
    {
        LOGW("Infer thread already running\n");
        return BK_OK;
    }

    /* Use NODISPLAY mode by default for inference only */
    this->mode = AVDK_VIDEO_REATOR_MODE_NODISPLAY;
    infer_thread_running = 1;

    int ret = rtos_create_hsram_thread(&infer_thread,
        BEKEN_DEFAULT_WORKER_PRIORITY,
        "infer_thread",
        (beken_thread_function_t)InferThreadEntry,
        1024 * 20,
        (void *)this);

    if (ret != BK_OK)
    {
        LOGE("create infer_thread fail\n");
        infer_thread_running = 0;
        return ret;
    }

    /* Wait for thread to start (wait forever) */
    rtos_get_semaphore(&infer_thread_sem, BEKEN_WAIT_FOREVER);

    LOGI("Infer thread started successfully\n");
    return BK_OK;
}

int AvdkVideoReator::stop_infer()
{
    LOGI("AvdkVideoReator::stop_infer\n");

    if (!infer_thread_running)
    {
        LOGW("Infer thread not running\n");
        return BK_OK;
    }

    /* Signal thread to stop */
    infer_thread_running = 0;

    /* Wait for thread to exit (wait forever) */
    rtos_get_semaphore(&infer_thread_sem, BEKEN_WAIT_FOREVER);

    infer_thread = NULL;

    return BK_OK;
}

int AvdkVideoReator::start_display()
{
    LOGI("AvdkVideoReator::start_display\n");

    if (display_thread_running)
    {
        LOGW("Display thread already running\n");
        return BK_OK;
    }

    display_thread_running = 1;

    int ret = rtos_create_hsram_thread(&display_thread,
                                        BEKEN_DEFAULT_WORKER_PRIORITY,
                                        "display_thread",
                                        (beken_thread_function_t)DisplayThreadEntry,
                                        1024 * 4,
                                        (void *)this);

    if (ret != BK_OK)
    {
        LOGE("create display_thread fail\n");
        display_thread_running = 0;
        return ret;
    }

    /* Wait for thread to start (wait forever) */
    rtos_get_semaphore(&display_thread_sem, BEKEN_WAIT_FOREVER);

    LOGI("Display thread started successfully\n");
    return BK_OK;
}

int AvdkVideoReator::stop_display()
{
    LOGI("AvdkVideoReator::stop_display\n");

    if (!display_thread_running)
    {
        LOGW("Display thread not running\n");
        return BK_OK;
    }

    /* Signal thread to stop */
    display_thread_running = 0;

    /* Wait for thread to exit (wait forever) */
    rtos_get_semaphore(&display_thread_sem, BEKEN_WAIT_FOREVER);

    display_thread = NULL;

    return BK_OK;
}

int AvdkVideoReator::OpenCameraWithDisplay()
{
    LOGI("AvdkVideoReator::OpenCameraWithDisplay\n");

    camera_board_config_t *board_config = app_camera_board_config_get();
    board_config->isp.sp_width = detection_model->getWidth();
    board_config->isp.sp_height = detection_model->getHeight();
    board_config->isp.sp_format = detection_model->getFormat();

    app_isp_mipi_camera_turn_on(app_camera_board_config_get());
    app_isp_camera_sp_channel_turn_on(board_config);

    return 0;
}

int AvdkVideoReator::OpenCameraWithoutDisplay()
{
    LOGI("AvdkVideoReator::OpenCameraWithoutDisplay\n");


    camera_board_config_t *board_config = app_camera_board_config_get();
    board_config->mipi.sensor_max_width = 1088;
    board_config->mipi.sensor_max_height = 1088;
    board_config->mipi.sensor_fps = 15;
    board_config->isp.mp_enable = false;
    board_config->isp.mp_flexa = false;
    board_config->isp.mp_width = detection_model->getWidth();
    board_config->isp.mp_height = detection_model->getHeight();
    board_config->isp.mp_format = detection_model->getFormat();
    board_config->isp.sp_enable = false;
    board_config->isp.sp_flexa = false;

    app_isp_mipi_camera_turn_on(board_config);
    return 0;
}


int AvdkVideoReator::CloseCamera()
{
    LOGI("AvdkVideoReator::CloseCamera\n");

    /* Camera is opened via app_isp_mipi_camera_turn_on() in app_camera.c.
     * Use app_isp_camera_turn_off() to stop MP/SP instances and cleanup safely.
     */
    bk_err_t ret = app_isp_camera_turn_off();
    if (ret != BK_OK)
    {
        LOGE("app_isp_camera_turn_off failed, ret: %d\n", ret);
        return ret;
    }

    return BK_OK;
}

int AvdkVideoReator::ReadCameraFrame(uint8_t *frame, uint32_t size, uint32_t timeout)
{
    LOGV("AvdkVideoReator::ReadCameraFrame\n");
    int ret = BK_FAIL;
    // current support sp channel
    if (frame == NULL)
    {
        LOGE("%s, %d frame malloc failed\n", __func__, __LINE__);
        return ret;
    }

    if (mode == AVDK_VIDEO_REATOR_MODE_NODISPLAY)
    {
        ret = app_isp_camera_channel_read(APP_ISP_MP_CHN_ID, frame, size, -1);
    }
    else
    {
        ret = app_isp_camera_channel_read(APP_ISP_SP_CHN_ID, frame, size, -1);
    }

    LOGV("AvdkVideoReator::ReadCameraFrame ret: %d, timeout: %X\n", ret, timeout);

    return ret;
}


int AvdkVideoReator::OpenDisplay()
{
    LOGI("AvdkVideoReator::OpenDisplay\n");
    app_mipi_lcd_turn_on(app_display_board_config_get());
#if CONFIG_VG_LITE_GPU
    app_gpu_turn_on(app_gpu_board_config_get());
#endif
    return 0;
}

int AvdkVideoReator::OpenDisplayWithoutGPU()
{
    LOGI("AvdkVideoReator::OpenDisplayWithoutGPU\n");
    app_mipi_lcd_turn_on(app_display_board_config_get());
    return 0;
}

int AvdkVideoReator::CloseDisplayWithoutGPU()
{
    LOGI("AvdkVideoReator::CloseDisplayWithoutGPU\n");
    return 0;
}

int AvdkVideoReator::CloseDisplay()
{
    LOGI("AvdkVideoReator::CloseDisplay\n");
    return 0;
}

int AvdkVideoReator::OpenGui()
{
    LOGI("AvdkVideoReator::OpenGui\n");
    lv_vnd_config_t lv_vnd_config = {0};

    lv_vnd_config.width = WIDTH;
    lv_vnd_config.height = HEIGHT;
    lv_vnd_config.render_mode = RENDER_DIRECT_MODE;

    lv_vnd_config.rotation = ROTATE_NONE;
	lv_vnd_config.frame_buffer[0] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, WIDTH * HEIGHT * sizeof(lv_color_t));
	lv_vnd_config.frame_buffer[1] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, WIDTH * HEIGHT * sizeof(lv_color_t));
    lv_vnd_config.flush_cb = aoo_detection_flush_cb;

    lv_vendor_init(&lv_vnd_config);

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vendor_disp_lock();
#if CONFIG_COMMON_LVGL_UI
    beken_ui_init();
#endif
    lv_vendor_disp_unlock();

    lv_vendor_start();
    return 0;
}

int AvdkVideoReator::CloseGui()
{
    LOGI("AvdkVideoReator::CloseGui\n");
    return 0;
}