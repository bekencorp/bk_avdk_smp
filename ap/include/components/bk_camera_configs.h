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


#define CAM_CSI_DEFAULT_RAW10_CONFIG(w, h, f) {             \
    .port_id = ISP_MIPI_PORT_ID,                            \
    .width = w,                                             \
    .height = h,                                            \
    .fps = f,                                               \
    .isp_mode = BK_ISP_MODE_RAW,                            \
    .hdr_mode = BK_ISP_HDR_MODE_LINEAR,                     \
    .input_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,              \
    .clk = 60000000,                                        \
    .input_type = BK_ISP_INPUT_TYPE_CSI_SENSOR,             \
    .input_rect = {0, 0, w, h},                             \
    .input_crop = {0, 0, w, h},                             \
}

#define CAM_DVP_DEFAULT_RAW8_CONFIG(w, h, f) {              \
    .port_id = ISP_MIPI_PORT_ID/*there is a bug use ISP_MIPI_PORT_ID for dvp frame buffer mode, need to fix it in the future*/,                             \
    .width = w,                                             \
    .height = h,                                            \
    .fps = f,                                               \
    .isp_mode = BK_ISP_MODE_RAW,                            \
    .hdr_mode = BK_ISP_HDR_MODE_LINEAR,                     \
    .input_pixel_fmt = BK_PIXEL_FORMAT_RGGB8,               \
    .clk = 60000000,                                        \
    .input_type = BK_ISP_INPUT_TYPE_CSI_SENSOR/**there is a bug use BK_ISP_INPUT_TYPE_CSI_SENSOR for dvp frame buffer mode, need to fix it in the future*/,             \
    .input_rect = {0, 0, w, h},                             \
    .input_crop = {0, 0, w, h},                             \
}



#define CAM_MP_NV12_RB_INSTANCE_CONFIG(w, h) {              \
    .buf_cnt = 3,                                           \
    .enable_flexa = 1,                                      \
    .work_mode = 1,                                         \
    .width = w,                                             \
    .height = h,                                            \
    .format = BK_PIXEL_FORMAT_NV12, /*BK_PIXEL_FORMAT_NV12*/\
}

#define CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT() {               \
    .i2c_id = 1,                                            \
    .data_size = 1,                                         \
    .timeout_ms = 2000,                                     \
    .pin_scl = GPIO_69,                                     \
    .pin_sda = GPIO_70,                                     \
    .mipi_port_en = 1,                                      \
    .dvp_port_en = 0,                                       \
}

#define DVP_CAM_BUS_I2C1_8BIT_2000TIMEOUT() {               \
    .i2c_id = 1,                                            \
    .data_size = 1,                                         \
    .timeout_ms = 2000,                                     \
    .pin_scl = GPIO_0,                                      \
    .pin_sda = GPIO_1,                                      \
    .mipi_port_en = 0,                                      \
    .dvp_port_en = 1,                                       \
}

#define DUAL_CAM_BUS_I2C1_8BIT_2000TIMEOUT() {              \
    .i2c_id = 1,                                            \
    .data_size = 1,                                         \
    .timeout_ms = 2000,                                     \
    .pin_scl = GPIO_0,                                      \
    .pin_sda = GPIO_1,                                      \
    .mipi_port_en = 1,                                      \
    .dvp_port_en = 1,                                       \
}

#ifdef __cplusplus
}
#endif

