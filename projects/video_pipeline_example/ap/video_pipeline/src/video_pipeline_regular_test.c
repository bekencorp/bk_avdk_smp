// Copyright 2020-2021 Beken
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

#include "video_pipeline_test.h"
#include <os/os.h>
#include <frame_buffer.h>
#include <components/bk_video_pipeline/bk_video_pipeline.h>

#define TAG "video_pipeline_regular_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// 测试用的管道编解码器句柄
static bk_video_pipeline_handle_t g_video_pipeline_handle = NULL;

// JPEG解码回调函数
static bk_err_t jpeg_complete(bk_err_t result, frame_buffer_t *out_frame)
{
    bk_err_t ret = BK_OK;
    frame_buffer_encode_free(out_frame);
    return ret;
}

static frame_buffer_t *jpeg_read(uint32_t timeout_ms)
{
    frame_buffer_t *frame = frame_buffer_encode_malloc(jpeg_length_422_864_480);
    if (frame == NULL) {
        LOGE("jpeg_read malloc failed\r\n");
        return NULL;
    }
    os_memcpy(frame->frame, jpeg_data_422_864_480, jpeg_length_422_864_480);
    frame->width = 864;
    frame->height = 480;
    frame->fmt = PIXEL_FMT_JPEG;
    frame->length = jpeg_length_422_864_480;
    return frame;
}

static const jpeg_callback_t jpeg_cbs = {
    .read = jpeg_read,
    .complete = jpeg_complete,
};

// 解码回调函数
static bk_err_t display_frame_free_cb(void *frame)
{
    frame_buffer_display_free((frame_buffer_t *)frame);
    return BK_OK;
}

static bk_err_t decode_complete(dec_end_type_t format_type, bk_err_t result, frame_buffer_t *out_frame)
{
    bk_err_t ret = BK_FAIL;
    if (format_type == HW_DEC_END || format_type == SW_DEC_END)
    {
        LOGI("Decode complete with result: %d\n", result);
        display_frame_free_cb(out_frame);
        return BK_OK;
    }
    return ret;
}

static frame_buffer_t *decode_malloc(uint32_t size)
{
    return frame_buffer_encode_malloc(size);
}

static bk_err_t decode_free(frame_buffer_t *frame)
{
    frame_buffer_encode_free(frame);
    return BK_OK;
}

static const decode_callback_t decode_cbs = {
    .malloc = decode_malloc,
    .free = decode_free,
    .complete = decode_complete,
};

// H.264编码回调函数
static frame_buffer_t *h264e_frame_malloc(uint32_t size)
{
    return frame_buffer_encode_malloc(size);
}

static void h264e_frame_complete(frame_buffer_t *frame, int result)
{
    if (result != AVDK_ERR_OK) {
        LOGI("H264 encode failed with result: %d\n", result);
        frame_buffer_encode_free(frame);
    } else {
        LOGI("H264 encode success, frame size: %d\n", frame->length);
        frame_buffer_encode_free(frame);
    }
}

static const bk_h264e_callback_t h264e_cbs = {
    .malloc = h264e_frame_malloc,
    .complete = h264e_frame_complete,
};

/**
 * @brief 初始化video pipeline
 *
 * @return 0 on success, negative on failure
 */
static int video_pipeline_init(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_config_t video_pipeline_config = {0};

    if (g_video_pipeline_handle == NULL) {
        video_pipeline_config.jpeg_cbs = &jpeg_cbs;
        video_pipeline_config.decode_cbs = &decode_cbs;
        ret = bk_video_pipeline_new(&g_video_pipeline_handle, &video_pipeline_config);
        if (ret != BK_OK) {
            LOGE("bk_video_pipeline_new failed, ret: %d\n", ret);
            return ret;
        }
    }
    
    return BK_OK;
}

/**
 * @brief 测试硬件旋转功能
 *
 * @return 0 on success, negative on failure
 */
static int video_pipeline_hardware_rotate_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_decode_config_t video_pipeline_decode_config = {0};
    
    LOGI("Starting hardware rotation test...\n");
    
    // 初始化video pipeline
    ret = video_pipeline_init();
    if (ret != BK_OK) {
        return ret;
    }
    
    // 配置硬件旋转
    video_pipeline_decode_config.rotate_mode = HW_ROTATE;
    video_pipeline_decode_config.rotate_angle = ROTATE_90; // 90度旋转
    
    // 打开旋转模块
    ret = bk_video_pipeline_open_rotate(g_video_pipeline_handle, &video_pipeline_decode_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_rotate failed, ret: %d\n", ret);
        return ret;
    }

    LOGI("Hardware rotation module opened successfully\n");

    //等待旋转
    rtos_delay_milliseconds(5000);

    // 检查旋转模块状态
    video_pipeline_module_status_t rotate_state = VIDEO_PIPELINE_MODULE_DISABLED;
    ret = bk_video_pipeline_get_module_status(g_video_pipeline_handle, VIDEO_PIPELINE_MODULE_ROTATE, &rotate_state);
    if (ret != BK_OK) {
        LOGE("Failed to get rotate status, ret: %d\n", ret);
    } else {
        LOGI("Rotate module status: %s\n", rotate_state == VIDEO_PIPELINE_MODULE_ENABLED ? "ENABLED" : "DISABLED");
    }
    
    // 关闭旋转模块
    ret = bk_video_pipeline_close_rotate(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_rotate failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("Hardware rotation test completed successfully\n");
    return BK_OK;
}

/**
 * @brief 测试软件旋转功能
 *
 * @return 0 on success, negative on failure
 */
static int video_pipeline_software_rotate_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_decode_config_t video_pipeline_decode_config = {0};
    
    LOGI("Starting software rotation test...\n");
    
    // 初始化video pipeline
    ret = video_pipeline_init();
    if (ret != BK_OK) {
        return ret;
    }
    
    // 配置软件旋转
    video_pipeline_decode_config.rotate_mode = SW_ROTATE;
    video_pipeline_decode_config.rotate_angle = ROTATE_180; // 180度旋转
    
    // 打开旋转模块
    ret = bk_video_pipeline_open_rotate(g_video_pipeline_handle, &video_pipeline_decode_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_rotate failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("Software rotation module opened successfully\n");
    
    //等待旋转
    rtos_delay_milliseconds(5000);

    // 检查旋转模块状态
    video_pipeline_module_status_t rotate_state = VIDEO_PIPELINE_MODULE_DISABLED;
    ret = bk_video_pipeline_get_module_status(g_video_pipeline_handle, VIDEO_PIPELINE_MODULE_ROTATE, &rotate_state);
    if (ret != BK_OK) {
        LOGE("Failed to get rotate status, ret: %d\n", ret);
    } else {
        LOGI("Rotate module status: %s\n", rotate_state == VIDEO_PIPELINE_MODULE_ENABLED ? "ENABLED" : "DISABLED");
    }
    
    // 关闭旋转模块
    ret = bk_video_pipeline_close_rotate(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_rotate failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("Software rotation test completed successfully\n");
    return BK_OK;
}

/**
 * @brief 测试H.264编码功能
 *
 * @return 0 on success, negative on failure
 */
static int video_pipeline_h264_encode_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_h264e_config_t video_pipeline_h264e_config = {0};
    
    LOGI("Starting H.264 encoding test...\n");
    
    // 初始化video pipeline
    ret = video_pipeline_init();
    if (ret != BK_OK) {
        return ret;
    }
    
    // 配置H.264编码器    
    video_pipeline_h264e_config.width = 864;
    video_pipeline_h264e_config.height = 480;
    video_pipeline_h264e_config.fps = FPS30;
    video_pipeline_h264e_config.sw_rotate_angle = ROTATE_NONE;
    video_pipeline_h264e_config.h264e_cb = &h264e_cbs;
    
    // 打开H.264编码器
    ret = bk_video_pipeline_open_h264e(g_video_pipeline_handle, &video_pipeline_h264e_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_h264e failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("H.264 encoder opened successfully\n");
    
    rtos_delay_milliseconds(5000);

    // 检查编码器状态
    video_pipeline_module_status_t h264e_state = VIDEO_PIPELINE_MODULE_DISABLED;
    ret = bk_video_pipeline_get_module_status(g_video_pipeline_handle, VIDEO_PIPELINE_MODULE_H264E, &h264e_state);
    if (ret != BK_OK) {
        LOGE("Failed to get H.264 encoder status, ret: %d\n", ret);
    } else {
        LOGI("H.264 encoder status: %s\n", h264e_state == VIDEO_PIPELINE_MODULE_ENABLED ? "ENABLED" : "DISABLED");
    }
    
    // 关闭H.264编码器
    ret = bk_video_pipeline_close_h264e(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_h264e failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("H.264 encoding test completed successfully\n");
    return BK_OK;
}

/**
 * @brief 测试H.264编码功能和硬件旋转功能
 *
 * @return 0 on success, negative on failure
 */
static int video_pipeline_h264_encode_and_hw_rotate_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_h264e_config_t video_pipeline_h264e_config = {0};
    bk_video_pipeline_decode_config_t video_pipeline_decode_config = {0};
    LOGI("Starting H.264 encoding test...\n");
    
    // 初始化video pipeline
    ret = video_pipeline_init();
    if (ret != BK_OK) {
        return ret;
    }
    
    // 配置H.264编码器    
    video_pipeline_h264e_config.width = 864;
    video_pipeline_h264e_config.height = 480;
    video_pipeline_h264e_config.fps = FPS30;
    video_pipeline_h264e_config.sw_rotate_angle = ROTATE_90;
    video_pipeline_h264e_config.h264e_cb = &h264e_cbs;
    
    // 打开H.264编码器
    ret = bk_video_pipeline_open_h264e(g_video_pipeline_handle, &video_pipeline_h264e_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_h264e failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("H.264 encoder opened successfully\n");
    
    // 配置软件旋转
    video_pipeline_decode_config.rotate_mode = HW_ROTATE;
    video_pipeline_decode_config.rotate_angle = ROTATE_90; // 90度旋转
    ret = bk_video_pipeline_open_rotate(g_video_pipeline_handle, &video_pipeline_decode_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_rotate failed, ret: %d\n", ret);
        return ret;
    }

    rtos_delay_milliseconds(5000);


    // 检查编码器状态
    video_pipeline_module_status_t h264e_state = VIDEO_PIPELINE_MODULE_DISABLED;
    ret = bk_video_pipeline_get_module_status(g_video_pipeline_handle, VIDEO_PIPELINE_MODULE_H264E, &h264e_state);
    if (ret != BK_OK) {
        LOGE("Failed to get H.264 encoder status, ret: %d\n", ret);
    } else {
        LOGI("H.264 encoder status: %s\n", h264e_state == VIDEO_PIPELINE_MODULE_ENABLED ? "ENABLED" : "DISABLED");
    }
    
    // 关闭解码
    ret = bk_video_pipeline_close_rotate(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_decode failed, ret: %d\n", ret);
        return ret;
    }
    
    // 关闭H.264编码器
    ret = bk_video_pipeline_close_h264e(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_h264e failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("H.264 encoding test completed successfully\n");
    return BK_OK;
}

/**
 * @brief 测试H.264编码功能和软件旋转功能
 *
 * @return 0 on success, negative on failure
 */
static int video_pipeline_h264_encode_and_sw_rotate_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_h264e_config_t video_pipeline_h264e_config = {0};
    bk_video_pipeline_decode_config_t video_pipeline_decode_config = {0};
    LOGI("Starting H.264 encoding test...\n");
    
    // 初始化video pipeline
    ret = video_pipeline_init();
    if (ret != BK_OK) {
        return ret;
    }
    
    // 配置H.264编码器    
    video_pipeline_h264e_config.width = 864;
    video_pipeline_h264e_config.height = 480;
    video_pipeline_h264e_config.fps = FPS30;
    video_pipeline_h264e_config.h264e_cb = &h264e_cbs;
    // 打开H.264编码器
    ret = bk_video_pipeline_open_h264e(g_video_pipeline_handle, &video_pipeline_h264e_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_h264e failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("H.264 encoder opened successfully\n");
    
    // 配置软件旋转
    video_pipeline_decode_config.rotate_mode = SW_ROTATE;
    video_pipeline_decode_config.rotate_angle = ROTATE_180; // 180度旋转

    ret = bk_video_pipeline_open_rotate(g_video_pipeline_handle, &video_pipeline_decode_config);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_open_rotate failed, ret: %d\n", ret);
        return ret;
    }

    rtos_delay_milliseconds(5000);

    // 检查编码器状态
    video_pipeline_module_status_t h264e_state = VIDEO_PIPELINE_MODULE_DISABLED;
    ret = bk_video_pipeline_get_module_status(g_video_pipeline_handle, VIDEO_PIPELINE_MODULE_H264E, &h264e_state);
    if (ret != BK_OK) {
        LOGE("Failed to get H.264 encoder status, ret: %d\n", ret);
    } else {
        LOGI("H.264 encoder status: %s\n", h264e_state == VIDEO_PIPELINE_MODULE_ENABLED ? "ENABLED" : "DISABLED");
    }
    
    // 关闭解码
    ret = bk_video_pipeline_close_rotate(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_rotate failed, ret: %d\n", ret);
        return ret;
    }
    
    // 关闭H.264编码器
    ret = bk_video_pipeline_close_h264e(g_video_pipeline_handle);
    if (ret != BK_OK) {
        LOGE("bk_video_pipeline_close_h264e failed, ret: %d\n", ret);
        return ret;
    }
    
    LOGI("H.264 encoding test completed successfully\n");
    return BK_OK;
}

/**
 * @brief 清理video pipeline资源
 */
static void video_pipeline_deinit(void)
{
    if (g_video_pipeline_handle != NULL) {
        bk_video_pipeline_delete(g_video_pipeline_handle);
        g_video_pipeline_handle = NULL;
    }
}

// 修改原有的CLI命令实现，添加具体的测试函数调用
void cli_video_pipeline_regular_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc != 2) {
        LOGI("usage: video_pipeline_regular_test hardware_rotate/software_rotate/h264_encode\n");
        return;
    }

    int ret = 0;
    
    if (strcmp(argv[1], "hardware_rotate") == 0) {
        ret = video_pipeline_hardware_rotate_test();
    } else if (strcmp(argv[1], "software_rotate") == 0) {
        ret = video_pipeline_software_rotate_test();
    } else if (strcmp(argv[1], "h264_encode") == 0) {
        ret = video_pipeline_h264_encode_test();
    }
    else if (strcmp(argv[1], "h264_encode_and_hw_rotate") == 0) {
        ret = video_pipeline_h264_encode_and_hw_rotate_test();
    }
    else if (strcmp(argv[1], "h264_encode_and_sw_rotate") == 0) {
        ret = video_pipeline_h264_encode_and_sw_rotate_test();
    }
    else {
        LOGI("Invalid test type: %s\n", argv[1]);
        LOGI("Valid options: hardware_rotate, software_rotate, h264_encode\n");
        ret = -1;
    }
    
    if(ret == BK_OK) {
        LOGI("Test %s passed\n", argv[1]);
    } else {
        LOGE("Test %s failed, ret: %d\n", argv[1], ret);
    }
    // 测试完成后清理资源
    video_pipeline_deinit();
    
    char *msg = NULL;
    if (ret != BK_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }
    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;
}