// Copyright 2025-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "video_engine.h"
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include "network_transfer.h"
#include <common/avdk_pixel_types.h>
#if CONFIG_USB_CAMERA
#include <components/bk_uvc_camera_types.h>
#include <components/bk_flexa_bond.h>
#include "app_camera.h"
#include "doorbell_img_manager.h"
#include "encode_frame_que.h"
#include "app_jpeg_decode.h"
#include "app_codec.h"
#endif

#define TAG "video_engine"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/* Video frame transfer task configuration */
#define VIDEO_TRANSFER_TASK_NAME        "video_xfer"
#define VIDEO_TRANSFER_TASK_PRIORITY    5
#define VIDEO_TRANSFER_TASK_STACK_SIZE  (4 * 1024)
#define VIDEO_ENCODED_FRAME_TIMEOUT_MS  (50)
#define VIDEO_UVC_DEFAULT_PORT          (1)
#define VIDEO_UVC_DEFAULT_FPS           (20)
#define VIDEO_UVC_CAMERA_ID             (1)
#define VIDEO_TRANSFER_LOG_INTERVAL     (30)

typedef enum {
    VIDEO_FRAME_SOURCE_NONE = 0,
    VIDEO_FRAME_SOURCE_CODED_POOL,
} video_frame_source_t;

/**
 * @brief Video engine internal context structure
 */
typedef struct {
    image_format_t transfer_format;             /**< Transfer format (IMAGE_MJPEG/IMAGE_H264/IMAGE_H265) */
    beken_thread_t transfer_task_handle;        /**< Transfer task handle */
    bool transfer_task_running;                 /**< Transfer task running flag */
    uint16_t camera_id;                         /**< Current camera source id */
    bool camera_opened;                         /**< Camera opened flag */
    video_frame_source_t frame_source;          /**< Current frame source */
    uint16_t stream_width;                      /**< Resolution for coded-pool tx metadata (agora_demo) */
    uint16_t stream_height;
#if CONFIG_USB_CAMERA
    void *h264e_bond;                           /**< MJPEG decode <-> H264 encode flexa bond (UVC H264 path) */
#endif
    /* Engine state */
    bool is_started;                            /**< Video engine started flag */
} video_engine_ctx_t;

/* Global video engine context */
static video_engine_ctx_t *g_video_engine_ctx = NULL;

#if CONFIG_USB_CAMERA
__attribute__((weak)) void devices_cli_init(void)
{
}
#endif

static const char *video_engine_frame_source_name(video_frame_source_t source)
{
    switch (source)
    {
        case VIDEO_FRAME_SOURCE_CODED_POOL:
            return "coded_pool";
        case VIDEO_FRAME_SOURCE_NONE:
        default:
            return "none";
    }
}

static const char *video_engine_image_format_name(image_format_t format)
{
    switch (format)
    {
        case IMAGE_YUV:
            return "YUV";
        case IMAGE_MJPEG:
            return "MJPEG";
        case IMAGE_H264:
            return "H264";
        default:
            return "UNKNOWN";
    }
}

static bool video_engine_camera_is_open(void)
{
    if (g_video_engine_ctx == NULL) {
        return false;
    }

    return g_video_engine_ctx->camera_opened;
}

/**
 * Fill frame_buffer_t fmt/width/height for network send (agora_demo).
 * Keeps doorbell H264 encode free of transport-layer pixel format tagging.
 */
static void video_engine_tag_frame_for_tx(frame_buffer_t *frame)
{
    if (frame == NULL || g_video_engine_ctx == NULL) {
        return;
    }
    if (g_video_engine_ctx->frame_source != VIDEO_FRAME_SOURCE_CODED_POOL) {
        return;
    }

    frame->width = g_video_engine_ctx->stream_width;
    frame->height = g_video_engine_ctx->stream_height;

    switch (g_video_engine_ctx->transfer_format) {
    case IMAGE_H264:
        frame->fmt = PIXEL_FMT_H264;
        break;
    case IMAGE_MJPEG:
        frame->fmt = PIXEL_FMT_JPEG;
        break;
    default:
        break;
    }
}

/**
 * Transfer task: encoded frames from doorbell_img_manager (coded pool) only.
 */
static void video_engine_transfer_task(void *arg)
{
    (void)arg;
    frame_buffer_t *frame = NULL;
    bk_err_t ret;
    uint32_t frame_count = 0;
    uint32_t fail_count = 0;

    LOGI("%s: video transfer task started\n", __func__);

    while (g_video_engine_ctx && g_video_engine_ctx->transfer_task_running) {
        frame = (frame_buffer_t *)bk_encoded_complete_data_request(VIDEO_ENCODED_FRAME_TIMEOUT_MS);
        if (frame == NULL) {
            continue;
        }

        if (g_video_engine_ctx == NULL) {
            bk_encoded_data_free_request((uint8_t *)frame);
            break;
        }

        video_engine_tag_frame_for_tx(frame);

        ret = ntwk_trans_send_video(frame);
        if (ret < 0) {
            fail_count++;
            LOGW("%s: send failed ret=%d fmt=%s len=%lu fails=%lu\n",
                 __func__,
                 ret,
                 video_engine_image_format_name(g_video_engine_ctx->transfer_format),
                 (unsigned long)frame->length,
                 (unsigned long)fail_count);
        } else {
            frame_count++;
            if (frame_count <= 5 || (frame_count % VIDEO_TRANSFER_LOG_INTERVAL) == 0) {
                LOGI("%s: frame[%lu] fmt=%s len=%lu seq=%lu\n",
                     __func__,
                     (unsigned long)frame_count,
                     video_engine_image_format_name(g_video_engine_ctx->transfer_format),
                     (unsigned long)frame->length,
                     (unsigned long)frame->sequence);
            }
        }

        bk_encoded_data_free_request((uint8_t *)frame);
        frame = NULL;
    }

    LOGI("%s: video transfer task exit\n", __func__);

    if (g_video_engine_ctx != NULL) {
        g_video_engine_ctx->transfer_task_handle = NULL;
    }

    rtos_delete_thread(NULL);
}


/* ============================= Public APIs ============================= */

int video_engine_init(void)
{
    bk_err_t ret = BK_OK;

    if (g_video_engine_ctx != NULL) {
        if (g_video_engine_ctx->is_started) {
            LOGD("%s: Video engine already initialized and started\n", __func__);
            return BK_OK;
        } else {
            LOGW("%s: Video engine context exists but not started, restarting\n", __func__);
            return video_engine_start();
        }
    }
    
    g_video_engine_ctx = (video_engine_ctx_t *)os_malloc(sizeof(video_engine_ctx_t));
    if (g_video_engine_ctx == NULL) {
        LOGE("%s: Failed to allocate memory for video_engine_ctx\n", __func__);
        return BK_FAIL;
    }

    os_memset(g_video_engine_ctx, 0, sizeof(video_engine_ctx_t));

    /* Start video engine with default configuration */
    ret = video_engine_start();
    if (ret != BK_OK) {
        LOGE("%s: video_engine_start failed, ret=%d\n", __func__, ret);
        os_free(g_video_engine_ctx);
        g_video_engine_ctx = NULL;
        
        return ret;
    }
    
    LOGI("%s: Video engine initialized successfully\n", __func__);
    return BK_OK;
}

int video_engine_deinit(void)
{
    bk_err_t ret = BK_OK;
    
    LOGI("%s: Deinitializing video engine\n", __func__);
    
    if (g_video_engine_ctx == NULL)
    {
        LOGD("%s: g_video_engine_ctx is NULL, already deinitialized\n", __func__);
        return BK_OK;
    }
    
    ret = video_engine_stop();
    if (ret != BK_OK) {
        LOGE("%s: video_engine_stop failed, ret=%d\n", __func__, ret);
    }
    
    os_free(g_video_engine_ctx);
    g_video_engine_ctx = NULL;
    
    LOGI("%s: Video engine deinitialized successfully\n", __func__);
    
    return BK_OK;
}

static int video_engine_uvc_camera_open(camera_parameters_t *parameters)
{
    avdk_err_t ret = AVDK_ERR_OK;
    camera_parameters_ext_t ext_parameters = {0};
    bool use_h264_pipeline = false;

    if (parameters == NULL)
    {
        LOGE("video_engine_uvc_camera_open: parameters is NULL");
        return BK_FAIL;
    }

    if (g_video_engine_ctx == NULL)
    {
        LOGE("%s: g_video_engine_ctx is NULL, call video_engine_info_init() first\n", __func__);
        return BK_FAIL;
    }

    if (g_video_engine_ctx->camera_opened)
    {
        LOGE("%s, uvc camera have been already opened!\n", __func__);
        return ret;
    }

    use_h264_pipeline = (parameters->format == 1);

    if (!use_h264_pipeline)
    {
        g_video_engine_ctx->transfer_format = IMAGE_MJPEG;
        g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_CODED_POOL;
    }
    else
    {
        g_video_engine_ctx->transfer_format = IMAGE_H264;
        /*
         * UVC H264 mode on this project uses the doorbell pipeline:
         * MJPEG input -> JPEG decode -> H264 encode -> coded frame pool.
         * The temporary encode queue is internal to the pipeline, while
         * video_engine itself always transfers from the coded pool.
         */
        g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
    }

    LOGI("%s: request open port=%u %ux%u@%u out=%s path=%s src=%s\n",
         __func__,
         VIDEO_UVC_DEFAULT_PORT,
         parameters->width,
         parameters->height,
         VIDEO_UVC_DEFAULT_FPS,
         use_h264_pipeline ? "H264" : "MJPEG",
         use_h264_pipeline ? "MJPEG->decode->H264" : "MJPEG-direct",
         video_engine_frame_source_name(g_video_engine_ctx->frame_source));

    ext_parameters.fps = VIDEO_UVC_DEFAULT_FPS;
    ext_parameters.port = VIDEO_UVC_DEFAULT_PORT;
    ext_parameters.camera_width = parameters->width;
    ext_parameters.camera_height = parameters->height;
    ext_parameters.camera_out_format = parameters->format;

    ret = app_uvc_turn_on(&ext_parameters);
    if (ret != BK_OK)
    {
        LOGE("%s: app_uvc_turn_on failed, ret=%d\n", __func__, ret);
        g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
        LOGW("%s: UVC open failed, cleanup done\n", __func__);
        return ret;
    }

    if (use_h264_pipeline)
    {
        ret = doorbell_jpeg_decode_open(parameters->width, parameters->height, BK_IMAGE_FORMAT_MJPEG, 1);
        LOGI("%s: doorbell_jpeg_decode_open ret=%d\n", __func__, ret);
        if (ret != BK_OK)
        {
            LOGE("%s: doorbell_jpeg_decode_open failed, ret=%d\n", __func__, ret);
            (void)app_uvc_turn_off(VIDEO_UVC_DEFAULT_PORT);
            g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
            return ret;
        }

        ret = doorbell_h264_encode_open(parameters->width, parameters->height);
        LOGI("%s: doorbell_h264_encode_open ret=%d\n", __func__, ret);
        if (ret != BK_OK)
        {
            LOGE("%s: doorbell_h264_encode_open failed, ret=%d\n", __func__, ret);
            (void)doorbell_jpeg_decode_close();
            (void)app_uvc_turn_off(VIDEO_UVC_DEFAULT_PORT);
            g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
            return ret;
        }

#if CONFIG_USB_CAMERA
        {
            bk_jpeg_decode_ctlr_handle_t decode_handle = NULL;
            bk_h264_encode_ctlr_handle_t encode_handle = NULL;
            avdk_err_t dec_gh_ret;
            bk_err_t enc_gh_ret;
            avdk_err_t bond_ret;

            g_video_engine_ctx->h264e_bond = NULL;

            dec_gh_ret = doorbell_decode_get_handle(&decode_handle);
            LOGI("%s: doorbell_decode_get_handle ret=%d\n", __func__, dec_gh_ret);
            if (dec_gh_ret != AVDK_ERR_OK || decode_handle == NULL)
            {
                LOGE("%s: doorbell_decode_get_handle failed, ret=%d\n", __func__, dec_gh_ret);
                (void)doorbell_h264_encode_close();
                (void)doorbell_jpeg_decode_close();
                (void)app_uvc_turn_off(VIDEO_UVC_DEFAULT_PORT);
                g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
                return BK_FAIL;
            }

            enc_gh_ret = doorbell_h264_encode_get_handle(&encode_handle);
            LOGI("%s: doorbell_h264_encode_get_handle ret=%d\n", __func__, enc_gh_ret);
            if (enc_gh_ret != BK_OK || encode_handle == NULL)
            {
                LOGE("%s: doorbell_h264_encode_get_handle failed, ret=%d\n", __func__, enc_gh_ret);
                (void)doorbell_h264_encode_close();
                (void)doorbell_jpeg_decode_close();
                (void)app_uvc_turn_off(VIDEO_UVC_DEFAULT_PORT);
                g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
                return BK_FAIL;
            }

            bond_ret = bk_flexa_mjpegd_h264e_bond_start(&g_video_engine_ctx->h264e_bond,
                                                       decode_handle, encode_handle);
            LOGI("%s: bk_flexa_mjpegd_h264e_bond_start ret=%d\n", __func__, bond_ret);
            if (bond_ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_flexa_mjpegd_h264e_bond_start failed, ret=%d\n", __func__, bond_ret);
                g_video_engine_ctx->h264e_bond = NULL;
                (void)doorbell_h264_encode_close();
                (void)doorbell_jpeg_decode_close();
                (void)app_uvc_turn_off(VIDEO_UVC_DEFAULT_PORT);
                g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
                return BK_FAIL;
            }

            LOGI("%s: MJPEG->H264 flexa bond started decode=%p encode=%p\n",
                 __func__, decode_handle, encode_handle);
        }
#endif

        g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_CODED_POOL;
        LOGI("%s: enabled UVC pipeline MJPEG->decode->H264\n", __func__);
    }

    g_video_engine_ctx->stream_width = parameters->width;
    g_video_engine_ctx->stream_height = parameters->height;
    g_video_engine_ctx->camera_id = VIDEO_UVC_CAMERA_ID;
    g_video_engine_ctx->camera_opened = true;
    LOGI("%s: UVC camera opened successfully via app_uvc_turn_on\n", __func__);

    return ret;
}


int video_engine_camera_close(void)
{
    bk_err_t ret = BK_OK;

    if (g_video_engine_ctx == NULL)
    {
        LOGE("%s: g_video_engine_ctx is NULL\n", __func__);
        return BK_FAIL;
    }

    if (!g_video_engine_ctx->camera_opened)
    {
        LOGE("%s: UVC camera is not opened\n", __func__);
        return ret;
    }

    ret = app_uvc_turn_off(VIDEO_UVC_DEFAULT_PORT);
    if (ret != BK_OK)
    {
        LOGE("%s: app_uvc_turn_off failed, ret=%d\n", __func__, ret);
        return ret;
    }

    if (g_video_engine_ctx->transfer_format == IMAGE_H264)
    {
        ret = doorbell_h264_encode_close();
        if (ret != BK_OK)
        {
            LOGE("%s: doorbell_h264_encode_close failed, ret=%d\n", __func__, ret);
            return ret;
        }

        ret = doorbell_jpeg_decode_close();
        if (ret != BK_OK)
        {
            LOGE("%s: doorbell_jpeg_decode_close failed, ret=%d\n", __func__, ret);
            return ret;
        }

        #if CONFIG_USB_CAMERA
        if (g_video_engine_ctx->h264e_bond != NULL)
        {
            bk_flexa_mjpegd_h264e_bond_stop(g_video_engine_ctx->h264e_bond);
            g_video_engine_ctx->h264e_bond = NULL;
        }
        #endif
        ret = encode_frame_que_deinit();
        if (ret != BK_OK)
        {
            LOGE("%s: encode_frame_que_deinit failed, ret=%d\n", __func__, ret);
            return ret;
        }
    }

    g_video_engine_ctx->camera_id = 0;
    g_video_engine_ctx->camera_opened = false;
    if (g_video_engine_ctx->frame_source == VIDEO_FRAME_SOURCE_CODED_POOL)
    {
        bk_encoded_data_manager_deinit(1);
        bk_encoded_data_manager_deinit(0);
    }
    g_video_engine_ctx->frame_source = VIDEO_FRAME_SOURCE_NONE;
    g_video_engine_ctx->stream_width = 0;
    g_video_engine_ctx->stream_height = 0;
    LOGI("%s: UVC camera closed\n", __func__);

    return BK_OK;
}


/**
 * @brief Start video transfer task
 * 
 * This function creates a task that continuously pops frames from the frame queue
 * and sends them using ntwk_trans_send_video().
 * 
 * @return BK_OK on success, BK_FAIL otherwise
 */
int video_engine_transfer_start(void)
{
    bk_err_t ret = BK_FAIL;

    if (g_video_engine_ctx == NULL)
    {
        LOGE("%s: g_video_engine_ctx is NULL\n", __func__);
        return BK_FAIL;
    }

    if (!video_engine_camera_is_open())
    {
        LOGE("%s: camera not open!\n", __func__);
        return BK_FAIL;
    }

    /* Check if task is already running */
    if (g_video_engine_ctx->transfer_task_running)
    {
        LOGW("%s: transfer task already running\n", __func__);
        return BK_OK;
    }

    g_video_engine_ctx->transfer_task_running = true;

    ret = rtos_create_thread(&g_video_engine_ctx->transfer_task_handle,
                            VIDEO_TRANSFER_TASK_PRIORITY,
                            VIDEO_TRANSFER_TASK_NAME,
                            video_engine_transfer_task,
                            VIDEO_TRANSFER_TASK_STACK_SIZE,
                            NULL);
    
    if (ret != BK_OK)
    {
        LOGE("%s: create transfer task failed, ret=%d\n", __func__, ret);
        g_video_engine_ctx->transfer_task_running = false;
        return BK_FAIL;
    }

    LOGI("%s: video transfer task started successfully\n", __func__);
    return BK_OK;
}

/**
 * @brief Stop video transfer task
 * 
 * This function stops the video transfer task gracefully.
 * 
 * @return BK_OK on success, BK_FAIL otherwise
 */
int video_engine_transfer_stop(void)
{
    bk_err_t ret = BK_OK;
    
    if (g_video_engine_ctx == NULL)
    {
        LOGE("%s: g_video_engine_ctx is NULL\n", __func__);
        return BK_FAIL;
    }

    if (!g_video_engine_ctx->transfer_task_running)
    {
        LOGW("%s: transfer task not running\n", __func__);
        return BK_OK;
    }

    g_video_engine_ctx->transfer_task_running = false;


    if (g_video_engine_ctx->transfer_task_handle != NULL)
    {
        LOGW("%s: transfer task did not exit gracefully, force delete\n", __func__);
        ret = rtos_delete_thread(&g_video_engine_ctx->transfer_task_handle);
        g_video_engine_ctx->transfer_task_handle = NULL;
    }

    LOGI("%s: video transfer task stopped\n", __func__);
    return ret;
}


int video_engine_camera_turn_on(camera_parameters_t *parameters)
{
    bk_err_t ret = BK_FAIL;

    if (parameters == NULL)
    {
        LOGE("%s: parameters is NULL\n", __func__);
        return BK_FAIL;
    }

    LOGI("%s: Camera params - id:%d, %dx%d, format:%d\n", __func__,
        parameters->id, parameters->width, parameters->height, parameters->format);
    
    if (parameters->width == 0 || parameters->height == 0)
    {
        LOGW("%s: invalid resolution, using default %dx%d\n", __func__,
             CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH,
             CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT);
        parameters->width = CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH;
        parameters->height = CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT;
    }
    
    if (parameters->format > 2)
    {
        parameters->format = 0;
    }

    if (parameters->id == VIDEO_UVC_CAMERA_ID)
    {
        ret = video_engine_uvc_camera_open(parameters);
    }
    else
    {
        LOGE("%s: unsupported camera id %d (only UVC id=%d)\n",
             __func__, parameters->id, VIDEO_UVC_CAMERA_ID);
        ret = BK_FAIL;
    }

    if (ret != BK_OK)
    {
        LOGE("%s: camera open failed, ret=%d\n", __func__, ret);
        return ret;
    }

    LOGI("%s: Camera opened successfully\n", __func__);
    return ret;    
}

/**
 * @brief Start video engine with default configuration
 * 
 * This function initializes and starts the video engine using the
 * configuration defined in camera_parameters (based on CONFIG macros).
 * It will open the camera and start the transfer task.
 * 
 * @return bk_err_t 
 *         - BK_OK: Success
 *         - BK_FAIL: Failed
 */
int video_engine_start(void)
{
    camera_parameters_t camera_parameters= {
        .id = VIDEO_UVC_CAMERA_ID,
        .width = CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH,
        .height = CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT,
#if CONFIG_VIDEO_ENGINE_USE_UVC_CAMERA
        /*
         * UVC cameras expose MJPEG, but the Agora demo expects H264 upload.
         * Keep video_engine on the MJPEG -> decode -> H264 encode path by default.
         */
        .format = 1,
#elif (CONFIG_VIDEO_ENGINE_JPEG_FORMAT)
        .format = 0,
#elif (CONFIG_VIDEO_ENGINE_H264_FORMAT)
        .format = 1,
#endif
    };

    bk_err_t ret = BK_OK;
    
    if (g_video_engine_ctx == NULL) {
        LOGE("%s: g_video_engine_ctx is NULL, call video_engine_init() first\n", __func__);
        return BK_FAIL;
    }
    
    if (g_video_engine_ctx->is_started) {
        LOGD("%s: Video engine already started\n", __func__);
        return BK_OK;
    }
    
    LOGI("%s: Starting video engine (id:%d, %dx%d, format:%d)\n", __func__,
         camera_parameters.id, camera_parameters.width, 
         camera_parameters.height, camera_parameters.format);
    
    ret = video_engine_camera_turn_on(&camera_parameters);
    if (ret != BK_OK) {
        LOGE("%s: video_engine_camera_turn_on failed, ret=%d\n", __func__, ret);
        return ret;
    }
    
    /* Start video transfer task */
    ret = video_engine_transfer_start();
    if (ret != BK_OK) {
        LOGE("%s: video_engine_transfer_start failed, ret=%d\n", __func__, ret);
        video_engine_camera_close();
        return ret;
    }
    
    g_video_engine_ctx->is_started = true;
    
    LOGI("%s: Video engine started successfully\n", __func__);
    return BK_OK;
}

/**
 * @brief Stop video engine and close all related resources
 * 
 * This function stops the video transfer task and closes the camera.
 * It does not free memory or deinitialize frame queues.
 * 
 * @return bk_err_t 
 *         - BK_OK: Success
 *         - BK_FAIL: Failed
 */
int video_engine_stop(void)
{
    bk_err_t ret = BK_OK;
    bk_err_t final_ret = BK_OK;  
    
    if (g_video_engine_ctx == NULL) {
        LOGD("%s: g_video_engine_ctx is NULL, already stopped\n", __func__);
        return BK_OK;
    }
    
    if (!g_video_engine_ctx->is_started) {
        LOGD("%s: Video engine not started\n", __func__);
        return BK_OK;
    }
    
    LOGI("%s: Stopping video engine\n", __func__);
    
    /* Stop video transfer task */
    if (g_video_engine_ctx->transfer_task_running) {
        ret = video_engine_transfer_stop();
        if (ret != BK_OK) {
            LOGE("%s: video_engine_transfer_stop failed, ret=%d\n", __func__, ret);
            final_ret = ret;  
        }
    }
    
    if (g_video_engine_ctx->camera_opened) {
        ret = video_engine_camera_close();
        if (ret != BK_OK) {
            LOGE("%s: video_engine_camera_close failed, ret=%d\n", __func__, ret);
            final_ret = ret;  // Record error but continue
        }
    }
    
    g_video_engine_ctx->is_started = false;
    
    if (final_ret == BK_OK) {
        LOGI("%s: Video engine stopped successfully\n", __func__);
    } else {
        LOGW("%s: Video engine stopped with errors, ret=%d\n", __func__, final_ret);
    }
    
    return final_ret;
}

/**
 * @brief Check if video engine is currently running
 * 
 * @return bool 
 *         - true: Video engine is running
 *         - false: Video engine is not running
 */
bool video_engine_is_running(void)
{
    if (g_video_engine_ctx == NULL) {
        return false;
    }
    return g_video_engine_ctx->is_started;
}