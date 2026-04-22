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

#include "AvdkVideoReator.h"
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


class AvdkVideoReator {

private:
    AvdkDetectionModel *detection_model;


    isp_handle_t isp_handle;
    mipi_csi_handle_t csi_handle;
    bk_camera_sensor_handle_t sensor_handle;
    bk_isp_camera_ctlr_handle_t camera_ctlr_handle;

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

    AvdkVideoReator(AvdkDetectionModel *detection_model);

    /**
     * @brief Initialize detection model.
     *
     * This function enables TFLM log callback and calls detection_model->init().
     *
     * @return BK_OK on success, negative or non-zero error code on failure.
     */
    int init_model();

    int start(avdk_video_reator_mode_t mode);
    int stop();

    int start_detect();

    int stop_detect();
    /**
     * @brief Start inference thread without opening peripherals.
     *
     * This will create a thread that reads camera frames and runs inference.
     * Camera and display should be opened separately before calling this.
     * Uses AVDK_VIDEO_REATOR_MODE_NODISPLAY mode by default.
     *
     * @return 0 on success, negative on error.
     */
    int start_infer();

    /**
     * @brief Stop inference thread.
     *
     * This will stop and destroy the inference thread created by start_infer().
     *
     * @return 0 on success, negative on error.
     */
    int stop_infer();

    /**
     * @brief Start display thread to fetch camera frames.
     *
     * This will create a thread that continuously reads camera frames by calling
     * ReadCameraFrame(). Camera and display should be opened separately before
     * calling this API. The display thread does not change camera/display state.
     *
     * @return 0 on success, negative on error.
     */
    int start_display();

    /**
     * @brief Stop display thread.
     *
     * This will stop and destroy the display thread created by start_display().
     *
     * @return 0 on success, negative on error.
     */
    int stop_display();

    int OpenCameraWithDisplay();
    int OpenCameraWithoutDisplay();

#if CONFIG_USB_CAMERA
    int OpenUVCCameraWithDisplay();
    int CloseUVCCameraWithDisplay();
#endif

    int CloseCamera();
    int ReadCameraFrame(uint8_t *frame, uint32_t size, uint32_t timeout);

    int OpenDisplay();
    int CloseDisplay();

    int OpenDisplayWithoutGPU();
    int CloseDisplayWithoutGPU();

    int OpenGui();
    int CloseGui();

    void WorkerThread();
    void InferThread();
    void DisplayThread();

protected:
    int start();
};




