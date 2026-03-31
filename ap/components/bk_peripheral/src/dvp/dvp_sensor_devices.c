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
#include <components/bk_dvp_camera_types.h>
#include <driver/i2c.h>

#define DVP_I2C_TIMEOUT (2000)

const dvp_sensor_config_t *dvp_sensor_configs[] =
{
    &dvp_sensor_gc0328c,
    &dvp_sensor_hm1055,
    &dvp_sensor_gc2145,
    &dvp_sensor_ov2640,
    &dvp_sensor_gc0308,
    &dvp_sensor_SC101,
};


static const dvp_sensor_config_t **devices_list = NULL;
static uint16_t devices_size = 0;

void bk_dvp_set_devices_list(const dvp_sensor_config_t **list, uint16_t size)
{
    devices_list = list;
    devices_size = size;
}

void dvp_sensor_devices_init(void)
{
    bk_dvp_set_devices_list(&dvp_sensor_configs[0], sizeof(dvp_sensor_configs) / sizeof(dvp_sensor_config_t *));
}

const dvp_sensor_config_t **get_sensor_config_devices_list(void)
{
    return devices_list;
}

int get_sensor_config_devices_num(void)
{
    return devices_size;
}

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id)
{
    uint32_t i;

    for (i = 0; i < devices_size; i++)
    {
        if (devices_list[i]->id == id)
        {
            return devices_list[i];
        }
    }

    return NULL;
}

bk_camera_sensor_handle_t bk_dvp_get_sensor_auto_detect(bk_camera_bus_t *bus)
{
    uint32_t i;
    uint8_t count = 3;
    bk_camera_sensor_config_t config = {
        .pin_reset = 0xFF,
        .pin_pwdn = 0xFF,
        .pin_xclk = 0,
        .bus = bus,
    };

    bk_camera_sensor_handle_t handle = NULL;

    do
    {
        for (i = 0; i < devices_size; i++)
        {
            if (AVDK_ERR_OK == devices_list[i]->detect(&handle, &config))
            {
                return handle;
            }
        }

        count--;

        rtos_delay_milliseconds(5);

    }
    while (count > 0);

    return NULL;
}

int dvp_camera_i2c_read_uint8(uint8_t addr, uint8_t reg, uint8_t *value)
{
#if 0
    i2c_mem_param_t mem_param = {0};

    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = value;

    return bk_i2c_memory_read(CONFIG_DVP_CAMERA_I2C_ID, &mem_param);
#else
    return 0;
#endif
}

int dvp_camera_i2c_read_uint16(uint8_t addr, uint16_t reg, uint8_t *value)
{
#if 0
    i2c_mem_param_t mem_param = {0};

    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = value;

    return bk_i2c_memory_read(CONFIG_DVP_CAMERA_I2C_ID, &mem_param);
#else
    return 0;
#endif
}

int dvp_camera_i2c_write_uint8(uint8_t addr, uint8_t reg, uint8_t value)
{
#if 0
    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = (uint8_t *)(&value);

    return bk_i2c_memory_write(CONFIG_DVP_CAMERA_I2C_ID, &mem_param);
#else
    return 0;
#endif
}

int dvp_camera_i2c_write_uint16(uint8_t addr, uint16_t reg, uint8_t value)
{
#if 0
    i2c_mem_param_t mem_param = {0};
    mem_param.dev_addr = addr;
    mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
    mem_param.data_size = 1;
    mem_param.timeout_ms = DVP_I2C_TIMEOUT;
    mem_param.mem_addr = reg;
    mem_param.data = (uint8_t *)(&value);

    return bk_i2c_memory_write(CONFIG_DVP_CAMERA_I2C_ID, &mem_param);
#else
    return 0;
#endif
}


