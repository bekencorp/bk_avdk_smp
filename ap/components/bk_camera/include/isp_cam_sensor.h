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

#include <driver/isp_types.h>
#include "vsios_i2c.h"
#include <components/bk_camera_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct isp_sensor_t
{
    RECT_S ppi;
    RECT_S acq;
    uint16_t fps;
} isp_sensor_t;

ISP_PUB_ATTR_S * isp_sensor_get_main_attr();

ISP_PUB_ATTR_S * isp_sensor_get_csi_attr();

ISP_PUB_ATTR_S * isp_sensor_get_dvp_attr();

bk_err_t isp_csi_camera_detect(isp_sensor_t *isp_sns, bk_camera_bus_t *bus);

bk_err_t isp_dvp_camera_detect(isp_sensor_t *isp_sns, bk_camera_bus_t *bus);

bk_err_t isp_csi_camera_init(void);

bk_err_t isp_dvp_camera_init(void);

void isp_csi_sensor_reg_ctrl(uint8_t cmd, uint16_t addr, uint8_t val);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

