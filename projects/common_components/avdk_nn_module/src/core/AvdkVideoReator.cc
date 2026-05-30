#include "AvdkDetectionModel.h"
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdint.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include <avdk_pixel_convert.h>

#include "AvdkVideoReator.h"

#include "app_camera.h"
#include "app_camera_types.h"
#include "app_display.h"

#if CONFIG_USB_CAMERA
#include "app_jpeg_decode.h"
#include "components/bk_flexa_bond.h"
#endif

#if (CONFIG_PSRAM_WRITE_THROUGH)
#include <driver/psram_types.h>
#include <driver/psram.h>
#endif

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

static const char* TAG = "vi-reator";

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
        .rotate_degree = 0,
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

        ret = detection_model->run(soruce_frame, frame_size, BK_PIXEL_FORMAT_BGRA8888);
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

        ret = detection_model->run(soruce_frame, frame_size, BK_PIXEL_FORMAT_BGRA8888);
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

void AvdkVideoReator::DisplayThread()
{
    LOGI("AvdkVideoReator::DisplayThread\n");
    int ret = BK_OK;

    if (BK_PIXEL_FORMAT_RGB888 == detection_model->getFormat())
    {
#if CONFIG_USB_CAMERA
        frame_size = detection_model->getWidth() * detection_model->getHeight() * 3;
#else
        frame_size = detection_model->getWidth() * detection_model->getHeight() * 4;
#endif
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

        if (detect_enable)
        {
        #if (CONFIG_USB_CAMERA)
            ret = detection_model->run(display_frame, frame_size, BK_PIXEL_FORMAT_RGB888);
        #else
            ret = detection_model->run(display_frame, frame_size, BK_PIXEL_FORMAT_BGRA8888);
        #endif
        }

#if (CONFIG_USB_CAMERA)
        bk_pixel_rgb888_to_rgb565(display_frame, rgb565_frame, detection_model->getWidth(), detection_model->getHeight());
#else
        bk_pixel_bgra8888_to_rgb565((uint32_t *)display_frame, rgb565_frame, detection_model->getWidth(), detection_model->getHeight());
#endif
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

#if CONFIG_USB_CAMERA
static camera_parameters_ext_t s_ext_parameters = {
    .port = 1,
    .camera_width = 1280,
    .camera_height = 720,
    .camera_out_format = BK_IMAGE_FORMAT_MJPEG,
};

static void *s_mjpegd_gpu_bond = NULL;
static bk_gpu_ctlr_handle_t s_gpu_handle = NULL;
static uint8_t *s_detection_frame = NULL;

#if (CONFIG_PSRAM_WRITE_THROUGH)
static psram_write_through_area_t s_psram_cover_area = PSRAM_WRITE_THROUGH_AREA_COUNT;
#endif

static void *gpu_frame_malloc(uint32_t size)
{
    void *disp_frame = NULL;

    disp_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
    if (disp_frame == NULL)
    {
        LOGE("GPU failed to malloc frame\r\n");
        return NULL;
    }

#if (CONFIG_PSRAM_WRITE_THROUGH)
    if (bk_psram_enable_write_through(s_psram_cover_area, (uint32_t)disp_frame, (uint32_t)((uint32_t)disp_frame + size)) != BK_OK)
    {
        LOGE("Failed to enable write through\r\n");
        return NULL;
    }
#endif

    return disp_frame;
}

static avdk_err_t doorbell_frame_free(void *ptr)
{
    bk_frame_buffer_free(ptr);

    return AVDK_ERR_OK;
}

static void gpu_frame_display(void *frame, uint32_t frame_size, void *args)
{
    if (s_detection_frame) {
        os_memcpy(s_detection_frame, frame, frame_size);
    }

    doorbell_frame_free(frame);
}

static avdk_err_t gpu_turn_on(uint16_t width, uint16_t height, uint16_t dst_width, uint16_t dst_height)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_gpu_ctlr_config_t gpu_config;

    LOGI("%s, %d, width: %d, height: %d\n", __func__, __LINE__, width, height);

    os_memset(&gpu_config, 0, sizeof(bk_gpu_ctlr_config_t));

    gpu_config.src_width = width;
    gpu_config.src_height = height;
    gpu_config.dst_width = dst_width;
    gpu_config.dst_height = dst_height;
    gpu_config.rotate_degree = 0;
    gpu_config.src_format = BK_PIXEL_FORMAT_NV12;
    gpu_config.dst_format = BK_PIXEL_FORMAT_RGB888;
    gpu_config.compress = false;
    gpu_config.scale = true;
    gpu_config.frame_malloc = gpu_frame_malloc;
    gpu_config.frame_free = NULL;

    uint8_t *decode_buffer = NULL;
    uint8_t decode_buf_cnt = 0;
    doorbell_decode_get_flexa_context(&decode_buffer, &decode_buf_cnt);
    if (decode_buffer == NULL || decode_buf_cnt == 0) {
        LOGW("%s, %d\n", __func__, __LINE__);
        return AVDK_ERR_GENERIC;
    }

    gpu_config.src_buffer = decode_buffer;
    gpu_config.flexa = true;
    gpu_config.flexa_lines = 16;
    gpu_config.flexa_buff_cnt = decode_buf_cnt;
    gpu_config.flexa_line_done = NULL;
    gpu_config.flexa_line_done_args = NULL;
    gpu_config.frame_done = gpu_frame_display;
    gpu_config.frame_done_args = NULL;

    ret = bk_gpu_ctlr_new(&s_gpu_handle, &gpu_config);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_gpu_init(s_gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_gpu_open(s_gpu_handle);
    if (ret != AVDK_ERR_OK) {
        LOGW("%s, %d\n", __func__, __LINE__);
    }

    return ret;
}

avdk_err_t gpu_turn_off(bk_gpu_ctlr_handle_t gpu_handle)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (gpu_handle == NULL)
    {
        LOGW("%s, %d, gpu handle is NULL\n", __func__, __LINE__);
        return AVDK_ERR_GENERIC;
    }

    ret = bk_gpu_close(gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d, close gpu failed: %d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = bk_gpu_deinit(gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d, deinit gpu failed: %d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = bk_gpu_delete(gpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s, %d, delete gpu failed: %d\n", __func__, __LINE__, ret);
        return ret;
    }

#if (CONFIG_PSRAM_WRITE_THROUGH)
    bk_err_t wt_ret = bk_psram_free_write_through_channel(s_psram_cover_area);
    if (wt_ret != BK_OK)
    {
        LOGW("%s, %d, free write-through channel failed: %d, area=%d\n", __func__, __LINE__, wt_ret, s_psram_cover_area);
        return AVDK_ERR_GENERIC;
    }
#endif

    return AVDK_ERR_OK;
}

int AvdkVideoReator::OpenUVCCameraWithDisplay()
{
    LOGI("AvdkVideoReator::OpenUVCCameraWithDisplay\n");
    bk_err_t ret = BK_FAIL;
    bk_jpeg_decode_ctlr_handle_t decode_handle = NULL;

    ret = app_uvc_turn_on(&s_ext_parameters);
    if (ret != BK_OK)
    {
        LOGE("%s, app_uvc_turn_on failed, ret = %d\n", __func__, ret);
        return ret;
    }

    ret = doorbell_jpeg_decode_open(s_ext_parameters.camera_width, s_ext_parameters.camera_height, BK_IMAGE_FORMAT_MJPEG, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, decode_test_open failed, ret = %d\n", __func__, ret);
        app_uvc_turn_off(s_ext_parameters.port);
        return ret;
    }

    LOGI("%s, %d, camera_width: %d, camera_height: %d, dst_width: %d, dst_height: %d\n", __func__, __LINE__, s_ext_parameters.camera_width, s_ext_parameters.camera_height, detection_model->getWidth(), detection_model->getHeight());
    ret = gpu_turn_on(s_ext_parameters.camera_width, s_ext_parameters.camera_height, detection_model->getWidth(), detection_model->getHeight());
    if (ret != BK_OK)
    {
        LOGE("%s, gpu_turn_on failed, ret = %d\n", __func__, ret);
        app_uvc_turn_off(s_ext_parameters.port);
        doorbell_jpeg_decode_close();
        return ret;
    }

    ret = doorbell_decode_get_handle(&decode_handle);
    if (ret != BK_OK)
    {
        LOGE("%s, doorbell_decode_get_handle failed, ret = %d\n", __func__, ret);
        app_uvc_turn_off(s_ext_parameters.port);
        doorbell_jpeg_decode_close();
        gpu_turn_off(s_gpu_handle);
        return ret;
    }

    ret = bk_flexa_mjpegd_gpu_bond_start(&s_mjpegd_gpu_bond, decode_handle, s_gpu_handle);
    if (ret != BK_OK)
    {
        LOGE("%s, bk_flexa_mjpegd_gpu_bond_start failed, ret = %d\n", __func__, ret);
        app_uvc_turn_off(s_ext_parameters.port);
        doorbell_jpeg_decode_close();
        gpu_turn_off(s_gpu_handle);
        return ret;
    }

    ret = app_mipi_lcd_turn_on(app_display_board_config_get());
    if (ret != BK_OK)
    {
        LOGE("%s, app_mipi_lcd_turn_on failed, ret = %d\n", __func__, ret);
        app_uvc_turn_off(s_ext_parameters.port);
        doorbell_jpeg_decode_close();
        gpu_turn_off(s_gpu_handle);
        return ret;
    }

    return BK_OK;
}

int AvdkVideoReator::CloseUVCCameraWithDisplay()
{
    LOGI("AvdkVideoReator::CloseUVCCameraWithDisplay\n");
    bk_err_t ret = BK_FAIL;

    ret = app_mipi_lcd_turn_off();
    if (ret != BK_OK)
    {
        LOGE("%s, app_mipi_lcd_turn_off failed, ret = %d\n", __func__, ret);
        return ret;
    }

    bk_flexa_mjpegd_gpu_bond_stop(&s_mjpegd_gpu_bond);

    ret = gpu_turn_off(s_gpu_handle);
    if (ret != BK_OK)
    {
        LOGE("%s, gpu_turn_off failed, ret = %d\n", __func__, ret);
        return ret;
    }

    ret = doorbell_jpeg_decode_close();
    if (ret != BK_OK)
    {
        LOGE("%s, doorbell_jpeg_decode_close failed, ret = %d\n", __func__, ret);
        return ret;
    }

    ret = app_uvc_turn_off(s_ext_parameters.port);
    if (ret != BK_OK)
    {
        LOGE("%s, app_uvc_turn_off failed, ret = %d\n", __func__, ret);
        return ret;
    }

    return 0;
}
#endif

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

#if CONFIG_USB_CAMERA
    s_detection_frame = frame;

    return BK_OK;
#else
    if (mode == AVDK_VIDEO_REATOR_MODE_NODISPLAY)
    {
        ret = app_isp_camera_channel_read(APP_ISP_MP_CHN_ID, frame, size, -1);
    }
    else
    {
        ret = app_isp_camera_channel_read(APP_ISP_SP_CHN_ID, frame, size, -1);
    }
#endif

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