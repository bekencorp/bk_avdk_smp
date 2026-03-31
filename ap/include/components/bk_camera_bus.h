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

#include <os/os.h>
#include <avdk_types.h>
#include <avdk_error.h>
#include <avdk_check.h>

typedef enum
{
    DVP_CAMERA_PORT = 1,     /**< dvp port */
    CSI_CAMERA_PORT,         /**< csi port */
} bk_camera_port_t;

typedef struct bk_camera_bus_ops_t bk_camera_bus_ops_t;
typedef struct bk_camera_bus_config_t bk_camera_bus_config_t;

struct bk_camera_bus_ops_t
{
    avdk_err_t (*read8)(bk_camera_bus_config_t *config, uint8_t reg, uint8_t *value);
    avdk_err_t (*read16)(bk_camera_bus_config_t *config, uint16_t reg, uint8_t *value);
    avdk_err_t (*write8)(bk_camera_bus_config_t *config, uint8_t reg, uint8_t value);
    avdk_err_t (*write16)(bk_camera_bus_config_t *config, uint16_t reg, uint8_t value);
};

struct bk_camera_bus_config_t
{
    uint8_t pin_scl;
    uint8_t pin_sda;
    uint8_t i2c_id;
    uint16_t write_address;
    uint32_t data_size;
    uint32_t timeout_ms;
    uint8_t mipi_port_en;
    uint8_t dvp_port_en;
};

typedef struct bk_camera_bus_t bk_camera_bus_t;
struct bk_camera_bus_t
{
    uint8_t pin_scl;
    uint8_t pin_sda;
    uint8_t i2c_id;
    uint16_t write_address;
    uint32_t data_size;
    uint32_t timeout_ms;
    void *i2c_handle;
    uint8_t mipi_port_en;
    uint8_t dvp_port_en;
    avdk_err_t (*read8)(bk_camera_bus_t *bus, uint8_t reg, uint8_t *value);
    avdk_err_t (*read16)(bk_camera_bus_t *bus, uint16_t reg, uint8_t *value);
    avdk_err_t (*write8)(bk_camera_bus_t *bus, uint8_t reg, uint8_t value);
    avdk_err_t (*write16)(bk_camera_bus_t *bus, uint16_t reg, uint8_t value);
};


bk_camera_bus_t *bk_camera_bus_new(bk_camera_bus_config_t *config);
avdk_err_t bk_camera_bus_enable(bk_camera_bus_t *bus);
avdk_err_t bk_camera_bus_disable(bk_camera_bus_t *bus);
avdk_err_t bk_camera_bus_delete(bk_camera_bus_t *bus);
bk_camera_bus_t *bk_camera_bus_get(void);

#ifdef __cplusplus
}
#endif