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

#define GC0328C_WRITE_ADDRESS (0x42)
#define GC0328C_READ_ADDRESS (0x43)
#define GC0328C_CHIP_ID (0x9D)
#define GC_QVGA_USE_SUBSAMPLE (1)

#define TAG "gc0328c"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

#define SENSOR_I2C_READ(reg, value) \
    do {\
        dvp_camera_i2c_read_uint8((GC0328C_WRITE_ADDRESS >> 1), reg, value);\
    }while (0)

#define SENSOR_I2C_WRITE(reg, value) \
    do {\
        dvp_camera_i2c_write_uint8((GC0328C_WRITE_ADDRESS >> 1), reg, value);\
    }while (0)

// gc0328c_DEV
const uint8_t sensor_gc0328c_init_talbe[][2] =
{
    {0xfe,0x80},
    {0xfe,0x80},
    {0xfc,0x16},
    {0xfc,0x16},
    {0xfc,0x16},
    {0xfc,0x16},
    {0xf1,0x00},
    {0xf2,0x00},
    {0xfe,0x00},
    {0x4f,0x00},
    {0x03,0x00},
    {0x04,0xc0},
    {0x42,0x00},
    {0x77,0x5a},
    {0x78,0x40},
    {0x79,0x56},
    {0xfe,0x00},
    {0x09,0x00},
    {0x0a,0x00},
    {0x0b,0x00},
    {0x0c,0x00},
    {0x0d,0x01},
    {0x0e,0xe8},
    {0x0f,0x02},
    {0x10,0x88},
    {0x16,0x00},
    {0x17,0x14},
    {0x18,0x0e},
    {0x19,0x06},
    {0x1b,0x48},
    {0x1f,0xC8},
    {0x20,0x01},
    {0x21,0x78},
    {0x22,0xb0},
    {0x23,0x04},//0x06  20140519 GC0328C
    {0x24,0x11},
    {0x26,0x00},
    {0x50,0x01}, //crop mode
    //global gain for range 
    {0x70,0x45},
    {0x71,0x40},
    {0x72,0x40},
    /////////////banding/////////////
    {0x05,0x01},//hb  225
    {0x06,0x32},//
    {0x07,0x00},//vb
    {0x08,0x0c},//
    {0xfe,0x00},
    //////////// BLK//////////////////////
    {0xfe , 0x00},
    {0x27 , 0xb7},
    {0x28 , 0x7F},
    {0x29 , 0x20},
    {0x33 , 0x20},
    {0x34 , 0x20},
    {0x35 , 0x20},
    {0x36 , 0x20},
    {0x32 , 0x08},
    {0x3b , 0x00}, 
    {0x3c , 0x00},
    {0x3d , 0x00},
    {0x3e , 0x00},
    {0x47 , 0x00},
    {0x48 , 0x00},
    //////////// block enable/////////////
    {0x40,0x00}, 
    {0x41,0x00}, 
    {0x42,0x00},
    {0x44,0xb8}, //yuv//0x44=0xb8 0x70/0x71
    {0x45,0x00},
    {0x46,0x02},
    {0x49,0x03},//0x44=0xb9 0x70/0x71/0x72
    {0x4f,0x00},
    {0x4b,0x01},
    {0x50,0x01}, 
    /////////output////////
    {0xfe,0x00},
    {0xf1,0x07},
    {0xf2,0x01},
};

const uint8_t sensor_gc0328c_5pfs_talbe[][2] =
{
    // all AEC_EXP_LEVEL_X set to 0xa3c = 2620
    {0xFE, 0x10},
    {0xFE, 0x01}, // page p1

    {0x2B, 0x0a}, // AEC_EXP_LEVEL_0 [11:8]
    {0x2C, 0x3c}, // AEC_EXP_LEVEL_0 [7:0]

    {0x2D, 0x0a}, // AEC_EXP_LEVEL_1
    {0x2E, 0x3c},

    {0x2F, 0x0a}, // AEC_EXP_LEVEL_2
    {0x30, 0x3c},

    {0x31, 0x0a}, // AEC_EXP_LEVEL_3
    {0x32, 0x3c},
    //{0x33,0x2a},  // AEC_EXP_MIN
    //{0x34,0x3c},
};

const uint8_t sensor_gc0328c_10pfs_talbe[][2] =
{
    // all AEC_EXP_LEVEL_X set to 0x51e = 1310
    {0xFE, 0x10},
    {0xFE, 0x01}, // page p1

    {0x2B, 0x05}, // AEC_EXP_LEVEL_0 [11:8]
    {0x2C, 0x1e}, // AEC_EXP_LEVEL_0 [7:0]

    {0x2D, 0x05}, // AEC_EXP_LEVEL_1
    {0x2E, 0x1e},

    {0x2F, 0x05}, // AEC_EXP_LEVEL_2
    {0x30, 0x1e},

    {0x31, 0x05}, // AEC_EXP_LEVEL_3
    {0x32, 0x1e},

    //{0x33,0x25},  // // AEC_EXP_MIN
    //{0x34,0x1e},
    {0xFE, 0x00},
    {0xF1, 0x07},
    {0xF2, 0x01},
};

const uint8_t sensor_gc0328c_20pfs_talbe[][2] =
{
    // all AEC_EXP_LEVEL_X set to 0x28f = 655
    {0xFE, 0x10},
    {0xFE, 0x01}, // page p1

    {0x2B, 0x02}, // AEC_EXP_LEVEL_0 [11:8]
    {0x2C, 0x8f}, // AEC_EXP_LEVEL_0 [7:0]

    {0x2D, 0x02}, // AEC_EXP_LEVEL_1
    {0x2E, 0x8f},

    {0x2F, 0x02}, // AEC_EXP_LEVEL_2
    {0x30, 0x8f},

    {0x31, 0x02}, // AEC_EXP_LEVEL_3
    {0x32, 0x8f},
    //{0x33,0x22},  // // AEC_EXP_MIN
    //{0x34,0x8f},
};

const uint8_t sensor_gc0328c_25pfs_talbe[][2] =
{
    {0xFE, 0x10},
    {0x05, 0x01},
    {0x06, 0x0a},
    {0x07, 0x00},
    {0x08, 0x0c},
    {0xFE, 0x01},
    {0x29, 0x00},
    {0x2A, 0x7d},
    {0x2B, 0x01},
    {0x2C, 0xf4},
    {0x2D, 0x01},
    {0x2E, 0xf4},
    {0x2F, 0x01},
    {0x30, 0xf4},
    {0x31, 0x01},
    {0x32, 0xf4},
};

const uint8_t sensor_gc0328c_30pfs_talbe[][2] =
{
    {0xFE, 0x10},
    {0x05, 0x00},
    {0x06, 0x6a},
    {0x07, 0x00},
    {0x08, 0x0c},
    {0xFE, 0x01},
    {0x29, 0x00},
    {0x2A, 0x96},
    {0x2B, 0x01},
    {0x2C, 0xc2},
    {0x2D, 0x01},
    {0x2E, 0xc2},
    {0x2F, 0x01},
    {0x30, 0xc2},
    {0x31, 0x01},
    {0x32, 0xc2},
};

const uint8_t sensor_gc0328c_WQVGA_480_272_talbe[][2] =
{
#if (GC_QVGA_USE_SUBSAMPLE == 1)
    {0xFE, 0x00},
    {0x59, 0x11},
#endif

    {0xFE, 0x00},
    {0x50, 0x01},
    {0x51, 0x00},
    {0x52, 0x00},
    {0x53, 0x00},
    {0x54, 0x00},

    {0x55, 0x01},
    {0x56, 0x10},
    {0x57, 0x01},
    {0x58, 0xe0},
    {0xfe, 0x00},
};

const uint8_t sensor_gc0328c_QVGA_320_240_talbe[][2] =
{
#if (GC_QVGA_USE_SUBSAMPLE == 0) // crop window mode
    {0xFE, 0x00},
    {0x50, 0x01},
    {0x51, 0x00},
    {0x52, 0x78},
    {0x53, 0x00},
    {0x54, 0xa0},

    {0x55, 0x00},
    {0x56, 0xf0},
    {0x57, 0x01},
    {0x58, 0x40},
#else
    // subsample mode
    {0xFE, 0x00},
    {0x59, 0x22},

    {0xFE, 0x00},
    {0x50, 0x01},
    {0x51, 0x00},
    {0x52, 0x00},
    {0x53, 0x00},
    {0x54, 0x00},
#endif

    {0x55, 0x00},
    {0x56, 0xf0},
    {0x57, 0x01},
    {0x58, 0x40},
    {0xfe, 0x00},
};

const uint8_t sensor_gc0328c_VGA_320_480_talbe[][2] =
{
#if (GC_QVGA_USE_SUBSAMPLE == 1)
    {0xFE, 0x00},
    {0x59, 0x11},
#endif

    {0xFE, 0x00},
    {0x50, 0x01},
    {0x51, 0x00},
    {0x52, 0x00},
    {0x53, 0x00},
    {0x54, 0x00},

    {0x55, 0x01},
    {0x56, 0xe0},
    {0x57, 0x01},
    {0x58, 0x40},
    {0xfe, 0x00},
};

const uint8_t sensor_gc0328c_VGA_480_320_talbe[][2] =
{
#if (GC_QVGA_USE_SUBSAMPLE == 1)
    {0xFE, 0x00},
    {0x59, 0x11},
#endif

    {0xFE, 0x00},
    {0x50, 0x01},
    {0x51, 0x00},
    {0x52, 0x00},
    {0x53, 0x00},
    {0x54, 0x00},

    {0x55, 0x01},
    {0x56, 0x40},
    {0x57, 0x01},
    {0x58, 0xe0},
    {0xfe, 0x00},
};


const uint8_t sensor_gc0328c_VGA_640_480_talbe[][2] =
{
#if (GC_QVGA_USE_SUBSAMPLE == 1)
    {0xFE, 0x00},
    {0x59, 0x11},
#endif

    {0xFE, 0x00},
    {0x50, 0x01},
    {0x51, 0x00},
    {0x52, 0x00},
    {0x53, 0x00},
    {0x54, 0x00},

    {0x55, 0x01},
    {0x56, 0xe0},
    {0x57, 0x02},
    {0x58, 0x80},
    {0xfe, 0x00},
};

avdk_err_t gc0328c_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t data = 0;

    SENSOR_I2C_READ(0xF0, &data);

    LOGI("%s, id: 0x%02X\n", __func__, data);

    if (data == GC0328C_CHIP_ID)
    {
        LOGI("%s success\n", __func__);
        return AVDK_ERR_OK;
    }

    return AVDK_ERR_GENERIC;
}

int gc0328c_init(bk_camera_sensor_ctlr_t *controller)

{
    uint32_t size = sizeof(sensor_gc0328c_init_talbe) / 2, i;

    LOGI("%s\n", __func__);

    for (i = 0; i < size; i++)
    {
        SENSOR_I2C_WRITE(sensor_gc0328c_init_talbe[i][0], sensor_gc0328c_init_talbe[i][1]);
    }

    return 0;
}

int gc0328c_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    int ret = -1;
    uint32_t size, i;

    LOGI("%s\n", __func__);

    if (width == 320 && height == 240)
    {
        size = sizeof(sensor_gc0328c_QVGA_320_240_talbe) / 2;

        for (i = 0; i < size; i++)
        {
            SENSOR_I2C_WRITE(sensor_gc0328c_QVGA_320_240_talbe[i][0],
                                sensor_gc0328c_QVGA_320_240_talbe[i][1]);
        }

        ret = 0;
    }
    else if (width == 320 && height == 480)
    {
        size = sizeof(sensor_gc0328c_VGA_320_480_talbe) / 2;

        for (i = 0; i < size; i++)
        {
            SENSOR_I2C_WRITE(sensor_gc0328c_VGA_320_480_talbe[i][0],
                                sensor_gc0328c_VGA_320_480_talbe[i][1]);
        }

        ret = 0;
    }
    else if (width == 480 && height == 272)
    {
        size = sizeof(sensor_gc0328c_WQVGA_480_272_talbe) / 2;

        for (i = 0; i < size; i++)
        {
            SENSOR_I2C_WRITE(sensor_gc0328c_WQVGA_480_272_talbe[i][0],
                                sensor_gc0328c_WQVGA_480_272_talbe[i][1]);
        }

        ret = 0;
    }
    else if (width == 480 && height == 320)
    {
        size = sizeof(sensor_gc0328c_VGA_480_320_talbe) / 2;

        for (i = 0; i < size; i++)
        {
            SENSOR_I2C_WRITE(sensor_gc0328c_VGA_480_320_talbe[i][0],
                                sensor_gc0328c_VGA_480_320_talbe[i][1]);
        }
        ret = 0;
    }
    else if (width == 640 && height == 480)
    {
        size = sizeof(sensor_gc0328c_VGA_640_480_talbe) / 2;

        for (i = 0; i < size; i++)
        {
            SENSOR_I2C_WRITE(sensor_gc0328c_VGA_640_480_talbe[i][0],
                                sensor_gc0328c_VGA_640_480_talbe[i][1]);
        }

        ret = 0;
    }
    else
    {
        LOGI("not supported width: %d, height: %d\r\n", width, height);
        ret = -1;
    }

    return ret;
}

int gc0328c_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    int ret = -1;

    uint32_t size, i;
    LOGI("%s\n", __func__);

    switch (fps)
    {
        case 5:
        {
            size = sizeof(sensor_gc0328c_5pfs_talbe) / 2;

            for (i = 0; i < size; i++)
            {
                SENSOR_I2C_WRITE(sensor_gc0328c_5pfs_talbe[i][0],
                                 sensor_gc0328c_5pfs_talbe[i][1]);
            }

            ret = 0;
        }
        break;
        case 10:
        {
            size = sizeof(sensor_gc0328c_10pfs_talbe) / 2;

            for (i = 0; i < size; i++)
            {
                SENSOR_I2C_WRITE(sensor_gc0328c_10pfs_talbe[i][0],
                                 sensor_gc0328c_10pfs_talbe[i][1]);
            }

            ret = 0;
        }
        break;
        case 20:
        {
            size = sizeof(sensor_gc0328c_20pfs_talbe) / 2;

            for (i = 0; i < size; i++)
            {
                SENSOR_I2C_WRITE(sensor_gc0328c_20pfs_talbe[i][0],
                                 sensor_gc0328c_20pfs_talbe[i][1]);
            }

            ret = 0;
        }
        break;
        case 25:
        {
            size = sizeof(sensor_gc0328c_25pfs_talbe) / 2;

            for (i = 0; i < size; i++)
            {
                SENSOR_I2C_WRITE(sensor_gc0328c_25pfs_talbe[i][0],
                                 sensor_gc0328c_25pfs_talbe[i][1]);
            }

            ret = 0;
        }
        break;
        case 30:
        {
            size = sizeof(sensor_gc0328c_30pfs_talbe) / 2;

            for (i = 0; i < size; i++)
            {
                SENSOR_I2C_WRITE(sensor_gc0328c_30pfs_talbe[i][0],
                                 sensor_gc0328c_30pfs_talbe[i][1]);
            }

            ret = 0;
        }
        break;

        default:
            LOGI("default 20fps");
    }

    return ret;
}

int gc0328c_reset(bk_camera_sensor_ctlr_t *controller)
{
    SENSOR_I2C_WRITE(0xFE, 0x80);
    return 0;
}

const dvp_sensor_config_t dvp_sensor_gc0328c =
{
    .name = "gc0328c",
    .clk = MCLK_24M,
    .vsync = SYNC_LOW_LEVEL,
    .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .default_width = 640,
    .default_height = 480,
    .default_fps = 30,
    .id = ID_GC0328C,
    .address = (GC0328C_WRITE_ADDRESS >> 1),
    .init = gc0328c_init,
    .detect = gc0328c_detect,
    .set_ppi = gc0328c_set_ppi,
    .set_fps = gc0328c_set_fps,
    .power_down = gc0328c_reset,
};

