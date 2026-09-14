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

#include "dvp_sensor_devices.h"
#include <components/dvp_camera.h>
#include <driver/i2c.h>

#define DVP_I2C_TIMEOUT (50)

/* Runtime DVP sensor I2C bus id.
 * bk_dvp_detect() sets this from bk_dvp_config_t.i2c_config.id (via
 * dvp_camera_i2c_set_id) before any sensor register access, so sensor read/write
 * uses the very same bus that bk_i2c_init() was configured with. This makes
 * i2c_config.id the single source of truth and removes the old requirement to
 * also define CONFIG_DVP_CAMERA_I2C_ID. */
static uint8_t s_dvp_i2c_id;   /* always set by dvp_camera_i2c_set_id() before use */

void dvp_camera_i2c_set_id(uint8_t id)
{
    s_dvp_i2c_id = id;
}

int dvp_camera_i2c_read_uint8(uint8_t addr, uint8_t reg, uint8_t *value)
{
    i2c_mem_param_t mem_param = {0};

    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = value;

    return bk_i2c_memory_read(s_dvp_i2c_id, &mem_param);
}

int dvp_camera_i2c_read_uint16(uint8_t addr, uint16_t reg, uint8_t *value)
{
    i2c_mem_param_t mem_param = {0};

    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = value;

    return bk_i2c_memory_read(s_dvp_i2c_id, &mem_param);
}

int dvp_camera_i2c_write_uint8(uint8_t addr, uint8_t reg, uint8_t value)
{
    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = (uint8_t *)(&value);

    return bk_i2c_memory_write(s_dvp_i2c_id, &mem_param);
}

int dvp_camera_i2c_write_uint16(uint8_t addr, uint16_t reg, uint8_t value)
{
    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = (uint8_t *)(&value);

    return bk_i2c_memory_write(s_dvp_i2c_id, &mem_param);
}


