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

#include <components/bk_isp_camera_types.h>
#include <driver/isp_types.h>
#include <common/avdk_pixel_types.h>
#ifdef __cplusplus
extern "C" {
#endif

/* map interval value start */
typedef uint16_t bk_isp_input_type_t;
#define BK_ISP_INPUT_TYPE_CSI_SENSOR  (0)
#define BK_ISP_INPUT_TYPE_DVP_SENSOR  (3)

typedef uint16_t bk_isp_mode_t;
#define BK_ISP_MODE_BT656  (1)
#define BK_ISP_MODE_BT601  (2)
#define BK_ISP_MODE_RAW    (3)

typedef uint16_t bk_isp_hdr_mode_t;
#define BK_ISP_HDR_MODE_LINEAR        (0)
#define BK_ISP_HDR_MODE_ISP_STICH     (1)
#define BK_ISP_HDR_MODE_SENSOR_STICH  (2)


/* map interval value end */

typedef struct
{
    uint8_t port_id;
    uint16_t width;
    uint16_t height;
    bk_isp_input_type_t input_type;
    bk_isp_mode_t isp_mode;
    bk_isp_hdr_mode_t hdr_mode;
    bk_pixel_format_t input_pixel_fmt;
    uint32_t clk;
    bk_rect_t input_rect;
    bk_rect_t input_crop;
    uint8_t fps;
    const void *sensor_object;
} bk_isp_camera_ctlr_config_t;




//TODO Need remove from public interface

#define ISP_CHANNEL_INSTANCE_MAX (2)

typedef struct
{
    uint8_t read_enable;
    uint8_t thread_enable;
    uint32_t read_timeout;
    uint8_t channel;
    beken_semaphore_t sem;
    beken_thread_t thread;
    void *controller;
    uint8_t *frame;
    uint32_t size;
} isp_channel_read_ctx_t;

typedef struct
{
    uint8_t read_register;
    uint8_t chnl;
    uint8_t state;
    uint8_t sensor_ctlr;
    void *isp_handle;
    isp_channel_read_ctx_t read_ctx[ISP_CHANNEL_INSTANCE_MAX];
    bk_isp_camera_channel_state_t channel_state[ISP_CHANNEL_INSTANCE_MAX];
    uint8_t skip_frames[ISP_CHANNEL_INSTANCE_MAX];
    ISP_PUB_ATTR_S attr[ISP_PORT_CNT];
    // bk_isp_camera_ctlr_config_t config;
    bk_camera_ctlr_t ops;
} bk_camera_isp_ctlr_t;

/**
 * @brief Create a new ISP camera controller instance
 * @param handle Output pointer that receives the controller handle
 * @return AVDK error code
 */
avdk_err_t bk_camera_isp_ctlr_new(bk_isp_camera_ctlr_handle_t *handle);


#ifdef __cplusplus
}
#endif

