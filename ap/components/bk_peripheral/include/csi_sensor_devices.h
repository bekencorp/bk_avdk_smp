// Copyright 2025-2026 Beken
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
#include <components/bk_csi_camera_types.h>
#include <components/bk_camera_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const csi_sensor_config_t csi_sensor_gc4653;
//extern const csi_sensor_config_t csi_sensor_gc2053;
extern const csi_sensor_config_t csi_sensor_ov2775;

typedef enum {
	CSI_SNS_READ = 0,
	CSI_SNS_WRITE,
	CSI_SNS_STANDBY,
	CSI_SNS_RESUME,
	CSI_SNS_STREAMON,
	CSI_SNS_UNKNOWN,
} csi_sns_cmd_t;

void csi_sensor_devices_init(void);

bk_camera_sensor_handle_t csi_sensor_detect(bk_camera_bus_t *bus);



#ifdef __cplusplus
}
#endif