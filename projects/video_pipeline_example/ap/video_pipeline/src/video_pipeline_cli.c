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

#include "frame_buffer.h"
#include "video_pipeline_test.h"
#include <os/os.h>

#define TAG "video_pipeline_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define CMD_USAGE "usage: video_pipeline\n"

static bk_video_pipeline_handle_t handle;

static frame_buffer_t *h264e_frame_malloc(uint32_t size)
{
    return frame_buffer_encode_malloc(size);
}

static void h264e_frame_complete(frame_buffer_t *frame, int result)
{
    frame_buffer_encode_free(frame);
}

static const bk_h264e_callback_t h264e_cb = 
{
    .malloc = h264e_frame_malloc,
    .complete = h264e_frame_complete,
};

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

static bk_err_t decode_complete(dec_end_type_t format_type, bk_err_t result, frame_buffer_t *out_frame)
{
    frame_buffer_display_free(out_frame);
    return BK_OK;
}

static frame_buffer_t *decode_malloc(uint32_t size)
{
    return frame_buffer_display_malloc(size);
}

static bk_err_t decode_free(frame_buffer_t *frame)
{
    frame_buffer_display_free(frame);
    return BK_OK;
}

static const decode_callback_t decode_cbs = {
    .malloc = decode_malloc,
    .free = decode_free,
    .complete = decode_complete,
};

void cli_video_pipeline_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_FAIL;

    if (argc < 2)
    {
        LOGI("Usage: %s <video_pipeline>\n", argv[0]);
        return;
    }
    if (os_strcmp(argv[1], "init") == 0) {
        bk_video_pipeline_config_t bk_video_pipeline_config = {0};

        bk_video_pipeline_config.jpeg_cbs = &jpeg_cbs;
        bk_video_pipeline_config.decode_cbs = &decode_cbs;

        ret = bk_video_pipeline_new(&handle, &bk_video_pipeline_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_hardware_jpeg_decode_new failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_hardware_jpeg_decode_new success!\n", __func__, __LINE__);
        }
    } else if (os_strcmp(argv[1], "deinit") == 0) {
        ret = bk_video_pipeline_delete(handle);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_video_pipeline_delete failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_video_pipeline_delete success!\n", __func__, __LINE__);
        }
    } else if (os_strcmp(argv[1], "open_h264e") == 0) {
        bk_video_pipeline_h264e_config_t bk_video_pipeline_h264e_config = {0};
        bk_video_pipeline_h264e_config.h264e_cb = &h264e_cb;
        bk_video_pipeline_h264e_config.width = 480;
        bk_video_pipeline_h264e_config.height = 864;
        bk_video_pipeline_h264e_config.fps = FPS30;
        bk_video_pipeline_h264e_config.sw_rotate_angle = ROTATE_90;
        ret = bk_video_pipeline_open_h264e(handle, &bk_video_pipeline_h264e_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_video_pipeline_open_h264e failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_video_pipeline_open_h264e success!\n", __func__, __LINE__);
        }
    } else if (os_strcmp(argv[1], "close_h264e") == 0) {
        ret = bk_video_pipeline_close_h264e(handle);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_video_pipeline_close_h264e failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_video_pipeline_close_h264e success!\n", __func__, __LINE__);
        }
    } else if (os_strcmp(argv[1], "open_rotate") == 0) {
        bk_video_pipeline_decode_config_t bk_video_pipeline_decode_config = {0};
        bk_video_pipeline_decode_config.rotate_angle = ROTATE_90;
        bk_video_pipeline_decode_config.rotate_mode = HW_ROTATE;
        ret = bk_video_pipeline_open_rotate(handle, &bk_video_pipeline_decode_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_video_pipeline_open_rotate failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_video_pipeline_open_rotate success!\n", __func__, __LINE__);
        }
    } else if (os_strcmp(argv[1], "close_rotate") == 0) {
        ret = bk_video_pipeline_close_rotate(handle);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_video_pipeline_close_rotate failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_video_pipeline_close_rotate success!\n", __func__, __LINE__);
        }
    }
    else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }
    char *msg = NULL;
    if (ret != BK_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }
    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}