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

//#include "doorbell_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "app_camera_types.h"
#include <common/avdk_pixel_types.h>

#define APP_ISP_MP_CHN_ID 0
#define APP_ISP_SP_CHN_ID 1

int app_isp_camera_turn_off(void);
bool app_isp_camera_state_get(void);
int app_isp_camera_soft_reset(void);
void *app_isp_handle_get(void);
int app_isp_mipi_camera_turn_on(const camera_board_config_t *config);
int app_isp_dvp_camera_turn_on(camera_parameters_ext_t *paramters);
int app_isp_camera_sp_channel_turn_on(const camera_board_config_t *config);
int app_isp_camera_channel_read(uint8_t channel ,uint8_t *frame, uint32_t size, uint32_t timeout);

int app_isp_dual_camera_turn_on(camera_parameters_ext_t *paramters);
int app_isp_dual_camera_port_change();
int app_isp_dual_dvp_on(camera_parameters_ext_t *paramters);

int app_uvc_turn_on(camera_parameters_ext_t *paramters);
int app_uvc_turn_off(uint8_t port_id);



int app_camera_board_config_set(camera_board_config_t *config);
camera_board_config_t *app_camera_board_config_get(void);

#ifdef __cplusplus
}
#endif
