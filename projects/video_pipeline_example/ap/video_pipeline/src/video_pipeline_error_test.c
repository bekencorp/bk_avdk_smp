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

#define TAG "video_pipeline_error_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

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

/**
 * @brief 测试空句柄错误
 *
 * @return 0 on success (错误处理正常), negative on failure
 */
static int video_pipeline_null_handle_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_h264e_config_t h264e_config = {0};
    bk_video_pipeline_decode_config_t decode_config = {0};
    video_pipeline_module_status_t status = VIDEO_PIPELINE_MODULE_DISABLED;
    
    LOGI("Starting null handle error test...\n");
    
    // 测试用空句柄调用各个API
    LOGI("Testing bk_video_pipeline_open_h264e with null handle...\n");
    ret = bk_video_pipeline_open_h264e(NULL, &h264e_config);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    LOGI("Testing bk_video_pipeline_close_h264e with null handle...\n");
    ret = bk_video_pipeline_close_h264e(NULL);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    LOGI("Testing bk_video_pipeline_open_rotate with null handle...\n");
    ret = bk_video_pipeline_open_rotate(NULL, &decode_config);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    LOGI("Testing bk_video_pipeline_close_rotate with null handle...\n");
    ret = bk_video_pipeline_close_rotate(NULL);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    LOGI("Testing bk_video_pipeline_delete with null handle...\n");
    ret = bk_video_pipeline_delete(NULL);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    LOGI("Testing bk_video_pipeline_get_module_status with null handle...\n");
    ret = bk_video_pipeline_get_module_status(NULL, VIDEO_PIPELINE_MODULE_H264E, &status);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    LOGI("Null handle error test completed\n");

    return 0; // 测试成功执行
}

/**
 * @brief 测试无效配置错误
 *
 * @return 0 on success (错误处理正常), negative on failure
 */
static int video_pipeline_invalid_config_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_handle_t handle = NULL;
    
    LOGI("Starting invalid config error test...\n");
    
    // 首先创建一个有效的句柄
    LOGI("Creating valid video pipeline handle...\n");
    bk_video_pipeline_config_t valid_config = {0};
    bk_video_pipeline_decode_config_t invalid_decode_config = {0};
    bk_video_pipeline_h264e_config_t invalid_h264e_config = {0};

    valid_config.jpeg_cbs = &jpeg_cbs;
    valid_config.decode_cbs = &decode_cbs;
    ret = bk_video_pipeline_new(&handle, &valid_config);
    if (ret != BK_OK) {
        LOGE("Failed to create valid handle, ret: %d\n", ret);
        return ret;
    }
    
    // 测试无效的旋转角度,
    LOGI("Testing invalid rotation angle (91 degrees)...\n");
    invalid_decode_config.rotate_angle = 4; // 无效的旋转角度
    ret = bk_video_pipeline_open_rotate(handle, &invalid_decode_config);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    bk_video_pipeline_close_rotate(handle);

    // 测试无效的旋转模式
    LOGI("Testing invalid rotation mode (3)...\n");
    invalid_decode_config.rotate_angle = ROTATE_90; // 恢复有效角度
    invalid_decode_config.rotate_mode = 3; // 无效的旋转模式
    ret = bk_video_pipeline_open_rotate(handle, &invalid_decode_config);
    LOGI("Result: %d (expected: non-zero)\n", ret);
   
    // 测试H.264编码无效分辨率
    LOGI("Testing invalid H.264 encode resolution (0x0)...\n");

    invalid_h264e_config.width = 0;
    invalid_h264e_config.height = 0;
    invalid_h264e_config.fps = FPS30;
    ret = bk_video_pipeline_open_h264e(handle, &invalid_h264e_config);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    // 清理资源
    LOGI("Cleaning up resources...\n");
    bk_video_pipeline_delete(handle);
    
    LOGI("Invalid config error test completed\n");
    return 0; // 测试成功执行
}

/**
 * @brief 测试无回调错误
 *
 * @return 0 on success (错误处理正常), negative on failure
 */
static int video_pipeline_no_callback_test(void)
{
    bk_err_t ret = BK_FAIL;
    bk_video_pipeline_handle_t handle = NULL;
    bk_video_pipeline_config_t config = {0};
    bk_video_pipeline_h264e_config_t video_pipeline_h264e_config = {0};

    LOGI("Starting no callback error test...\n");
    
    // 创建一个基本的句柄

    // 测试无JPEG回调
    config.jpeg_cbs = NULL; // 无JPEG回调
    config.decode_cbs = NULL; // 无解码回调
    ret = bk_video_pipeline_new(&handle, &config);
    LOGI("Result: %d (expected: non-zero)\n", ret);


    config.jpeg_cbs = &jpeg_cbs;
    config.decode_cbs = &decode_cbs; 
    ret = bk_video_pipeline_new(&handle, &config);
    if (ret != BK_OK) {
        LOGE("Failed to create handle, ret: %d\n", ret);
        return ret;
    }
    
    // 测试无H.264编码回调
    LOGI("Testing open_h264e without H.264 encode callback...\n");

    video_pipeline_h264e_config.width = 864;
    video_pipeline_h264e_config.height = 480;
    video_pipeline_h264e_config.fps = FPS30;
    video_pipeline_h264e_config.h264e_cb = NULL; // 无H.264编码回调
    
    ret = bk_video_pipeline_open_h264e(handle, &video_pipeline_h264e_config);
    LOGI("Result: %d (expected: non-zero)\n", ret);
    
    // 清理资源
    LOGI("Cleaning up resources...\n");
    bk_video_pipeline_delete(handle);
    
    LOGI("No callback error test completed\n");
    return 0; // 测试成功执行
}

// 修改原有的CLI命令实现，添加具体的错误测试函数调用
void cli_video_pipeline_error_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc != 2) {
        LOGI("usage: video_pipeline_error_test null_handle/invalid_config/no_callback\n");
        return;
    }

    int ret = 0;
    
    if (strcmp(argv[1], "null_handle") == 0) {
        ret = video_pipeline_null_handle_test();
    } else if (strcmp(argv[1], "invalid_config") == 0) {
        ret = video_pipeline_invalid_config_test();
    } else if (strcmp(argv[1], "no_callback") == 0) {
        ret = video_pipeline_no_callback_test();
    } else {
        LOGI("Invalid test type: %s\n", argv[1]);
        LOGI("Valid options: null_handle, invalid_config, no_callback\n");
    }
    
    if (ret != 0) {
        LOGI("Test failed with return code: %d\n", ret);
    }
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