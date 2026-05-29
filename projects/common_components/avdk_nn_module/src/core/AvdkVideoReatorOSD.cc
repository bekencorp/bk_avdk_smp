#include "AvdkDetectionModel.h"
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdint.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include <components/bk_flexa_bond.h>

#include "AvdkVideoReatorOSD.h"

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

AvdkVideoReatorOSD::AvdkVideoReatorOSD(AvdkDetectionModel *detection_model)
{
    this->detection_model = detection_model;
    this->infer_thread_running = 0;
    this->display_thread_running = 0;
    this->infer_thread = NULL;
    this->display_thread = NULL;
    this->thread = NULL;
    this->detect_enable = 0;
    this->mode = AVDK_VIDEO_REATOR_MODE_DISPLAY;

    this->isp_gpu_bond = NULL;

    /* Initialize semaphore for infer thread synchronization */
    rtos_init_semaphore(&infer_thread_sem, 1);
    /* Initialize semaphore for display thread synchronization */
    rtos_init_semaphore(&display_thread_sem, 1);
}

static void WorkerThreadEntry(void *arg)
{
    if (arg)
        ((AvdkVideoReatorOSD *)arg)->WorkerThread();
}

void AvdkVideoReatorOSD::WorkerThread()
{
    LOGI("AvdkVideoReatorOSD::WorkerThread\n");
    int ret = BK_OK;

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

int AvdkVideoReatorOSD::init()
{
    LOGI("AvdkVideoReatorOSD::init\n");

    detection_model->LogEnable(true);
    int ret = detection_model->init();
    if (ret != BK_OK)
    {
        LOGE("detection_model init failed, ret: %d\n", ret);
        return ret;
    }

    if (BK_PIXEL_FORMAT_RGB888 == detection_model->getFormat())
    {
        frame_size = detection_model->getWidth() * detection_model->getHeight() * 4;
    }
    else
    {
        frame_size = bk_image_size_get(detection_model->getWidth(), detection_model->getHeight(), detection_model->getFormat());
    }
    if (frame_size == 0)
    {
        LOGE("invalid model frame format: %d\n", detection_model->getFormat());
        return BK_FAIL;
    }

    soruce_frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);

    if (soruce_frame == NULL)
    {
        LOGE("soruce_frame malloc fail, size: %d\n", frame_size);
        return BK_FAIL;
    }

    return BK_OK;
}

int AvdkVideoReatorOSD::start()
{
    int ret = rtos_create_hsram_thread(&thread,
        CONFIG_AVDK_VIDEO_REATOR_OSD_WORKER_PRIORITY,
        "worker_thread",
        (beken_thread_function_t)WorkerThreadEntry,
        CONFIG_AVDK_VIDEO_REATOR_OSD_WORKER_STACK_SIZE,
        (void *)this);

    if (ret != BK_OK)
    {
        LOGE("create worker_thread fail\n");
        return ret;
    }

    return BK_OK;
}

int AvdkVideoReatorOSD::OpenISPCamera()
{
    LOGI("AvdkVideoReatorOSD::OpenISPCamera\n");
    int ret = BK_OK;

    camera_board_config_t *board_config = app_camera_board_config_get();
    board_config->isp.sp_width = detection_model->getWidth();
    board_config->isp.sp_height = detection_model->getHeight();
    board_config->isp.sp_format = detection_model->getFormat();

    ret = app_isp_mipi_camera_turn_on(app_camera_board_config_get());
    if (ret != BK_OK)
    {
        LOGE("app_isp_mipi_camera_turn_on failed, ret: %d\n", ret);
        return ret;
    }

    ret = app_isp_camera_sp_channel_turn_on(board_config);
    if (ret != BK_OK)
    {
        LOGE("app_isp_camera_sp_channel_turn_on failed, ret: %d\n", ret);
        return ret;
    }

    return 0;
}

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

int AvdkVideoReatorOSD::CloseCamera()
{
    LOGI("AvdkVideoReatorOSD::CloseCamera\n");

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

int AvdkVideoReatorOSD::ReadCameraFrame(uint8_t *frame, uint32_t size, uint32_t timeout)
{
    LOGV("AvdkVideoReatorOSD::ReadCameraFrame\n");
    int ret = BK_FAIL;
    // current support sp channel
    if (frame == NULL)
    {
        LOGE("%s, %d frame malloc failed\n", __func__, __LINE__);
        return ret;
    }

#if CONFIG_USB_CAMERA
    //s_detection_frame = frame;

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

    LOGV("AvdkVideoReatorOSD::ReadCameraFrame ret: %d, timeout: %X\n", ret, timeout);

    return ret;
}


int AvdkVideoReatorOSD::OpenDisplay()
{
    avdk_err_t ret = AVDK_ERR_OK;
#if CONFIG_VG_LITE_GPU
    bk_gpu_ctlr_handle_t gpu_handle = NULL;
    void *isp_handle = NULL;
#endif
    LOGI("AvdkVideoReatorOSD::OpenDisplay\n");
    app_mipi_lcd_turn_on(app_display_board_config_get());
#if CONFIG_VG_LITE_GPU

    ret = app_gpu_turn_on(app_gpu_board_config_get());
    if (ret != BK_OK) {
        LOGE("%s, app_gpu_turn_on failed, ret = %d\n", __func__, ret);
        goto error;
    }
    gpu_handle = app_gpu_handle_get();
    if (gpu_handle == NULL) {
        LOGE("%s, app_gpu_handle_get failed\n", __func__);
        goto error;
    }
    isp_handle = app_isp_handle_get();
    if (isp_handle == NULL) {
        LOGE("%s, app_isp_handle_get failed\n", __func__);
        goto error;
    }
    ret = bk_flexa_isp_gpu_bond_start(&isp_gpu_bond, isp_handle, gpu_handle);
    if (ret != BK_OK) {
        LOGE("%s, bk_flexa_isp_gpu_bond_start failed, ret = %d\n", __func__, ret);
        goto error;
    }

#endif
    return 0;
error:
    return ret;
}

int AvdkVideoReatorOSD::CloseDisplay()
{
    LOGI("AvdkVideoReatorOSD::CloseDisplay\n");
    return 0;
}