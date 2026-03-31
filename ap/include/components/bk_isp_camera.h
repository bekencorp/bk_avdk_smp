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

#ifdef __cplusplus
extern "C" {
#endif


avdk_err_t bk_isp_camera_dev_init(bk_isp_camera_ctlr_handle_t handle);
avdk_err_t bk_isp_camera_port_init(bk_isp_camera_ctlr_handle_t handle, void *config);
avdk_err_t bk_isp_camera_port_change(bk_isp_camera_ctlr_handle_t handle);
avdk_err_t bk_isp_camera_deinit(bk_isp_camera_ctlr_handle_t handle);
avdk_err_t bk_isp_camera_open(bk_isp_camera_ctlr_handle_t handle, void *parameter);
avdk_err_t bk_isp_camera_close(bk_isp_camera_ctlr_handle_t handle);
avdk_err_t bk_isp_camera_read(bk_isp_camera_ctlr_handle_t handle, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout);
avdk_err_t bk_isp_camera_register_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, bk_camera_isr_t cb, void *arg);
avdk_err_t bk_isp_camera_deregister_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, void *arg);
avdk_err_t bk_isp_camera_ctlr_ioctl(bk_isp_camera_ctlr_handle_t handle, bk_cam_interface_ioctl_t ioctl, void *arg);
avdk_err_t bk_isp_camera_delete(bk_isp_camera_ctlr_handle_t handle);
avdk_err_t bk_isp_camera_channel_open(bk_isp_camera_ctlr_handle_t handle, uint8_t channel, bk_isp_camera_channel_config_t *config);
avdk_err_t bk_isp_camera_channel_close(bk_isp_camera_ctlr_handle_t handle, uint8_t channel);
bk_isp_camera_channel_state_t bk_isp_camera_channel_state_get(bk_isp_camera_ctlr_handle_t handle, uint8_t channel);
#ifdef __cplusplus
}
#endif

