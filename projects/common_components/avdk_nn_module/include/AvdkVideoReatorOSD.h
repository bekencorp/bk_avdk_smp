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

#include "AvdkDetectionModel.h"

#include <driver/mipi_csi.h>
#include <components/bk_isp_camera.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu.h>
#include <components/bk_camera_isp_ctlr.h>
#include <components/bk_camera_configs.h>
#include <components/bk_frame_buffer.h>
#include <avdk_check.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include <driver/i2c.h>
#include <components/bk_camera_sensor.h>
#include <os/os.h>

typedef enum {
    AVDK_VIDEO_REATOR_MODE_NODISPLAY,
    AVDK_VIDEO_REATOR_MODE_DISPLAY,
} avdk_video_reator_mode_t;


class AvdkVideoReatorOSD {

protected:
    AvdkDetectionModel *detection_model;

    void *isp_gpu_bond;

    beken_thread_t thread;
    beken_thread_t infer_thread;
    beken_thread_t display_thread;
    uint8_t *soruce_frame;
    uint8_t *display_frame;
    uint32_t frame_size;

    avdk_video_reator_mode_t mode;
    uint8_t infer_thread_running;
    uint8_t display_thread_running;
    uint8_t detect_enable;

    /* Semaphore for infer thread synchronization (start and exit) */
    beken_semaphore_t infer_thread_sem;
    /* Semaphore for display thread synchronization (start and exit) */
    beken_semaphore_t display_thread_sem;

public:

    AvdkVideoReatorOSD(AvdkDetectionModel *detection_model);

    /**
     * @brief Initialize detection model.
     *
     * This function enables TFLM log callback and calls detection_model->init().
     *
     * @return BK_OK on success, negative or non-zero error code on failure.
     */
    int init();

    int start();
    int stop();

    int OpenISPCamera();

    int CloseCamera();
    int ReadCameraFrame(uint8_t *frame, uint32_t size, uint32_t timeout);

    int OpenDisplay();
    int CloseDisplay();

    void WorkerThread();
};




