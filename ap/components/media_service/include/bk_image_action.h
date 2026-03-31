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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

typedef enum
{
    IMG_SER_DECODER_REQUEST,
    IMG_SER_DECODER_RESPONSE,
    IMG_SER_ROTATE_SET,
    IMG_SER_ROTATE_REQUEST,
    IMG_SER_ROTATE_RESPONSE,
    IMG_SER_SCALE_SET,
    IMG_SER_SCALE_REQUEST,
    IMG_SER_SCALE_RESPONSE,
    IMG_SER_RESET,
    IMG_SER_EXIT,
} image_msg_type_t;

typedef enum
{
    RD_MODE_FRAME,
    RD_MODE_PIPELINE,
} read_mode_t;


typedef struct
{
    uint32_t event;
    union
    {
        uint32_t param;
        void *ptr;
    };
    void *args;
} img_msg_t;


typedef struct
{
#define LCD_READ_MODULE     "lcd"
    //camera_context_t context;
    beken_thread_t thread;
    beken_semaphore_t sem;
    // bk_multiplex_module_t module;
    // bk_multiplex_queue_t queue;
    // bk_multiplex_queue_t h264_queue;
    void *param;
    uint8_t running : 1;
    EventGroupHandle_t waitting_handle;
} camera_read_task_t;

void camera_pipeline_read_entry(beken_thread_arg_t data);
void camera_flexa_read_entry(beken_thread_arg_t data);

bk_err_t bk_img_msg_send(img_msg_t *msg);
void image_decode_complete(void *cfg, uint8_t state);

void bk_h264_action_enable(bool enable);
void bk_display_action_enable(bool enable);

bool bk_pipeline_peripheral_all_closed(void);

void bk_pipeline_mjpeg_frame_complete(frame_buffer_t *frame, uint8_t state, void *args);
void bk_pipeline_display_frame_complete(frame_buffer_t *frame, uint8_t state, void *args);

// TODO: Temporary
uint32_t gpu_get_write_lines(void);
void gpu_set_read_lines(uint32_t done);
void gpu_set_enable(bool en);
beken_semaphore_t* gpu_get_process_sem(void);
void gpu_done_callback(void);
void* gpu_get_rb(void);

#ifdef __cplusplus
}
#endif
