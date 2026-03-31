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


#ifdef CONFIG_FACE_DETECTION_V2

#elif CONFIG_TFLM_PERSON_DETECTION_V1
#elif CONFIG_TFLM_FACE_DETECTION_V1
#define AOV_WIDTH                           (192)
#define AOV_HEIGHT                          (192)
#define AOV_FORMAT                          (PIXEL_FORMAT_RGB888)
#define AVO_FRAME_SIZE                      (AOV_WIDTH * AOV_HEIGHT * 4)
#elif CONFIG_TFLM_GESTURE_DETECTION_V1
#define AOV_WIDTH                           (192)
#define AOV_HEIGHT                          (192)
#define AOV_FORMAT                          (PIXEL_FORMAT_RGB888)
#define AVO_FRAME_SIZE                      (AOV_WIDTH * AOV_HEIGHT * 4)
#else
//TODO FIX ME #error "must define WIDTH/HEIGHT/FRAME_SIZE"
#endif

int aov_isp_camera_turn_off(void);
int aov_isp_camera_turn_on(uint16_t width, uint16_t height, uint16_t pixel_format);
bool aov_isp_camera_state_get(void);
void *aov_isp_handle_get(void);
int aov_isp_camera_frame_get(uint8_t *frame, uint32_t size);

#ifdef __cplusplus
}
#endif