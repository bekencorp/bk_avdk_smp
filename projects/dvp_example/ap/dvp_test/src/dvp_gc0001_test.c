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

#define GC0001_WRITE_ADDRESS (0x42)
#define GC0001_READ_ADDRESS (0x43)
#define GC0001_CHIP_ID (0x9D)

#define TAG "gc0001"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SENSOR_I2C_READ(reg, value) \
    do {\
        dvp_camera_i2c_read_uint8((GC0001_WRITE_ADDRESS >> 1), reg, value);\
    }while (0)

#define SENSOR_I2C_WRITE(reg, value) \
    do {\
        dvp_camera_i2c_write_uint8((GC0001_WRITE_ADDRESS >> 1), reg, value);\
    }while (0)

// gc0001_DEV
const uint8_t sensor_gc0001_init_talbe[][2] =
{
    // sensor init table, need to be filled
    // {0xFE, 0x80},
    // {0xFE, 0x80},

};

bool gc0001_detect(void)
{
    uint8_t data = 0;

    SENSOR_I2C_READ(0xF0, &data);

    LOGD("%s, id: 0x%02X\n", __func__, data);

    if (data == GC0001_CHIP_ID)
    {
        LOGD("%s success\n", __func__);
        return true;
    }

    return false;
}

int gc0001_init(void)

{
    uint32_t size = sizeof(sensor_gc0001_init_talbe) / 2, i;

    LOGD("%s\n", __func__);

    for (i = 0; i < size; i++)
    {
        SENSOR_I2C_WRITE(sensor_gc0001_init_talbe[i][0], sensor_gc0001_init_talbe[i][1]);
    }

    return 0;
}

int gc0001_set_ppi(media_ppi_t ppi)
{
    return 0;
}

int gc0001_set_fps(frame_fps_t fps)
{
    return 0;
}

int gc0001_reset(void)
{
    SENSOR_I2C_WRITE(0xFE, 0x80);
    return 0;
}

int gc0001_read_register(uint32_t reg, uint32_t *data)
{
    uint8_t val = 0;
    SENSOR_I2C_READ(reg, &val);
    *data = val;
    return 0;
}

int gc0001_write_register(uint32_t reg, uint32_t data)
{
    SENSOR_I2C_WRITE(reg, data);
    return 0;
}


const dvp_sensor_config_t dvp_sensor_gc0001 =
{
    .name = "gc0001",
    .clk = MCLK_24M,
    .fmt = PIXEL_FMT_YUYV,
    .vsync = SYNC_HIGH_LEVEL,
    .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .def_ppi = PPI_640X480,
    .def_fps = FPS25,
    /* capability config */
    .fps_cap = FPS30,
    .ppi_cap = PPI_CAP_640X480,
    .id = GC0001_CHIP_ID,
    .address = (GC0001_WRITE_ADDRESS >> 1),
    .init = gc0001_init,
    .detect = gc0001_detect,
    .set_ppi = gc0001_set_ppi,
    .set_fps = gc0001_set_fps,
    .power_down = gc0001_reset,
    .read_register = gc0001_read_register,
    .write_register = gc0001_write_register,
};

const dvp_sensor_config_t *gc0001_detect_sensor(void)
{
    LOGI("%s\n", __func__);
    if (gc0001_detect())
    {
        return &dvp_sensor_gc0001;
    }

    return NULL;
}

BK_CAMERA_SENSOR_DETECT_SECTION(gc0001_detect_sensor);