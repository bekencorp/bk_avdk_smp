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

#include <os/mem.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>

#include <avdk_error.h>


#define TAG "code_img"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define MAX_QUE_LEN (10)//5 is too small, 10 is enough
#define FRAME_SIZE  (1024 * 200)//500K is too large, 200K is enough
typedef struct
{
    uint32_t param;
} img_msg_t;

typedef struct
{
    uint8_t input_enable : 1;
    uint8_t output_enable : 1;
    beken_queue_t free_queue;
    beken_queue_t ready_queue;
} img_service_t;

img_service_t s_img_service = {0};

bk_err_t bk_encoded_data_manager_init(void)
{
    bk_err_t ret = BK_OK;

    img_service_t *img_service = &s_img_service;

    if (img_service->input_enable || img_service->output_enable)
    {
        img_service->input_enable = true;
        img_service->output_enable = true;
        return ret;
    }

    if (img_service->free_queue == NULL)
    {
        ret = rtos_init_queue(&img_service->free_queue,
                      "enc_free_que",
                      sizeof(img_msg_t),
                      MAX_QUE_LEN);
        if (ret != BK_OK) {
            LOGE("%s, %d, encode free_que init fail \n", __func__, __LINE__);
            goto error;
        }
    }

    if (img_service->ready_queue == NULL)
    {
        ret = rtos_init_queue(&img_service->ready_queue,
                          "enc_ready_que",
                          sizeof(img_msg_t),
                          MAX_QUE_LEN);
        if (ret != BK_OK) {
            LOGE("%s, %d, enc_ready_que init fail \n", __func__, __LINE__);
            goto error;
        }
        for (int i = 0 ; i < MAX_QUE_LEN; i ++)
        {
            img_msg_t msg;
            uint32_t frame_size = ((sizeof(frame_buffer_t) + FRAME_SIZE + 63) >> 6) << 6;
            frame_buffer_t *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, frame_size);
            if (frame == NULL)
            {
                LOGE("%s, %d, frame_buffer_coded_data_mallocs fail \n", __func__, __LINE__);
                goto error;
            }

            os_memset(frame, 0, frame_size);
            frame->frame = (uint8_t *)frame + frame_size - FRAME_SIZE;
            frame->size = FRAME_SIZE;
            msg.param = (uint32_t)frame;
            if (img_service->free_queue)
            {
                ret = rtos_push_to_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT);
                if (ret != BK_OK) {
                    LOGE("%s, %d, queue send fail \n", __func__, __LINE__);
                    goto error;
                }
            }
        }
    }

    img_service->input_enable = true;
    img_service->output_enable = true;

    return ret;

error:

    if (img_service->free_queue)
    {
        img_msg_t msg = {0};
        while (rtos_pop_from_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
        {
            if (msg.param)
            {
                frame_buffer_t *frame = (frame_buffer_t *)msg.param;
                bk_frame_buffer_free((void *)frame->frame);
                os_free(frame);
            }
        }

        rtos_deinit_queue(&img_service->free_queue);
    }

    if (img_service->ready_queue)
    {
        rtos_deinit_queue(&img_service->ready_queue);
    }

    os_memset(img_service, 0, sizeof(img_service_t));

    return ret;
}

bk_err_t bk_encoded_data_manager_deinit(uint8_t input)
{
    // input:opuput/1:0
    bk_err_t ret = BK_OK;

    img_service_t *img_service = &s_img_service;
    img_msg_t msg = {0};

    if (input)
    {
        if (img_service->input_enable == false)
        {
            return ret;
        }

        img_service->input_enable = false;
    }
    else
    {
        if (img_service->output_enable == false)
        {
            return ret;
        }

        img_service->output_enable = false;
    }

    if (img_service->input_enable || img_service->output_enable)
    {
        return ret;
    }

#if 0
    frame_buffer_t *frame = NULL;
    if (img_service->free_queue)
    {
        while (rtos_pop_from_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
        {
            if (msg.param)
            {
                frame = (frame_buffer_t *)msg.param;
                bk_frame_buffer_free((void *)frame->frame);
                os_free(frame);
                frame = NULL;
            }
        }

        rtos_deinit_queue(&img_service->free_queue);
    }

    if (img_service->ready_queue)
    {
        while (rtos_pop_from_queue(&img_service->ready_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
        {
            if (msg.param)
            {
                frame = (frame_buffer_t *)msg.param;
                bk_frame_buffer_free((void *)frame->frame);
                os_free(frame);
                frame = NULL;
            }
        }
        rtos_deinit_queue(&img_service->ready_queue);
    }

    os_memset(img_service, 0, sizeof(img_service_t));
#else
    if (img_service->ready_queue)
    {
        while (rtos_pop_from_queue(&img_service->ready_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
        {
            if (msg.param)
            {
                rtos_push_to_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT);
            }
        }
    }

    int msg_cnt = 0;
    if (img_service->free_queue)
    {
        while (rtos_pop_from_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT) == BK_OK)
        {
            msg_cnt ++;
            if (msg.param)
            {
                rtos_push_to_queue(&img_service->ready_queue, &msg, BEKEN_NO_WAIT);
            }
        }
    }

    LOGW("%s, %d ###current not free frame buffer, msg_cnt:%d#####\n", __func__, __LINE__, msg_cnt);
#endif
    return ret;
}

void *bk_encoded_data_request(void)
{
    bk_err_t ret = BK_FAIL;

    img_service_t *img_service = &s_img_service;
    img_msg_t msg = {0};
    frame_buffer_t *frame = NULL;

    if (img_service && img_service->free_queue)
    {
        ret = rtos_pop_from_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT);
        if (ret == BK_OK)
        {
            frame = (frame_buffer_t *)msg.param;
            frame->h264_type = 1;
            frame->length = 0;
        }
#if 0//def CONFIG_USB_CAMERA
        else
        {
            ret = rtos_pop_from_queue(&img_service->ready_queue, &msg, BEKEN_NO_WAIT);
            if (ret == BK_OK)
            {
                frame = (frame_buffer_t *)msg.param;
                frame->h264_type = 1;
                frame->length = 0;
            }
        }
#endif
    }

    return frame;
}

bk_err_t bk_encoded_data_complete_request(uint8_t *frame)
{
    bk_err_t ret = BK_FAIL;

    img_service_t *img_service = &s_img_service;
    img_msg_t msg = {0};

    if (img_service && img_service->ready_queue)
    {
        msg.param = (uint32_t)frame;
        ret = rtos_push_to_queue(&img_service->ready_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGW("%s, %d ready queue overflow, please check!\n", __func__, __LINE__);
        }
    }
    else
    {
        LOGW("%s, %d there is mem leak, please check!\n", __func__, __LINE__);
    }

    return ret;
}

bk_err_t bk_encoded_data_free_request(uint8_t *frame)
{
    bk_err_t ret = BK_FAIL;

    img_service_t *img_service = &s_img_service;
    img_msg_t msg = {0};

    if (img_service && img_service->free_queue)
    {
        msg.param = (uint32_t)frame;
        ret = rtos_push_to_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGW("%s, %d ready queue overflow, please check!\n", __func__, __LINE__);
        }
    }
    else
    {
        LOGW("%s, %d there is mem leak, please check!\n", __func__, __LINE__);
    }

    return ret;
}

void *bk_encoded_complete_data_request(uint32_t timeout_ms)
{
    bk_err_t ret = BK_FAIL;

    img_service_t *img_service = &s_img_service;
    img_msg_t msg = {0};
    frame_buffer_t *frame = NULL;

    if (img_service && img_service->ready_queue)
    {
        ret = rtos_pop_from_queue(&img_service->ready_queue, &msg, timeout_ms);
        if (ret == BK_OK)
        {
            frame = (frame_buffer_t *)msg.param;
        }
    }

    return frame;
}

bk_err_t bk_encoded_complete_data_free_request(uint8_t *frame)
{
    bk_err_t ret = BK_FAIL;

    img_service_t *img_service = &s_img_service;
    img_msg_t msg = {0};

    if (img_service && img_service->free_queue)
    {
        msg.param = (uint32_t)frame;
        ret = rtos_push_to_queue(&img_service->free_queue, &msg, BEKEN_NO_WAIT);
        if (ret != BK_OK)
        {
            LOGW("%s, %d ready queue overflow, please check!\n", __func__, __LINE__);
        }
    }
    else
    {
        LOGW("%s, %d there is mem leak, please check!\n", __func__, __LINE__);
    }

    return ret;
}

