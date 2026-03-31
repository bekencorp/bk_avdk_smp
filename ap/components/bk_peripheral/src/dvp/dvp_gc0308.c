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

#include <os/os.h>
#include <os/mem.h>
#include "dvp_sensor_devices.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include <driver/mipi_csi.h>
#include <driver/isp_types.h>

#define GC0308_WRITE_ADDRESS (0x42)
#define GC0308_READ_ADDRESS (0x43)
#define GC0308_CHIP_ID (0x9b)

#define TAG "gc0308"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

avdk_err_t gc0308_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

// gc0308_DEV
const uint8_t sensor_gc0308_init_talbe[][2] =
{
    {0xfe, 0x80},
    {0xfe, 0x00},  // set page0
    {0xd2, 0x10},  // close AEC
    {0x22, 0x55},  // close AWB
    {0x5a, 0x56},
    {0x5b, 0x40},
    {0x5c, 0x4a},
    {0x22, 0x57}, // Open AWB
    {0x01, 0xce},
    {0x02, 0x70},
    {0x0f, 0x00},
    {0xe2, 0x00},
    {0xe3, 0x96},
    {0xe4, 0x02},
    {0xe5, 0x58},
    {0xe6, 0x02},
    {0xe7, 0x58},
    {0xe8, 0x02},
    {0xe9, 0x58},
    {0xea, 0x0c},
    {0xeb, 0xbe},
    {0xec, 0x20},
    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x01},
    {0x0a, 0xe8},
    {0x0b, 0x02},
    {0x0c, 0x88},
    {0x0d, 0x02},
    {0x0e, 0x02},
    {0x10, 0x22},
    {0x11, 0xfd},
    {0x12, 0x2a},
    {0x13, 0x00},
    {0x14, 0x10},// change direction  10:normal , 11:H SWITCH,12: V SWITCH, 13:H&V SWITCH
    {0x15, 0x0a},
    {0x16, 0x05},
    {0x17, 0x01},
    {0x18, 0x44},
    {0x19, 0x44},
    {0x1a, 0x1e},
    {0x1b, 0x00},
    {0x1c, 0xc1},
    {0x1d, 0x08},
    {0x1e, 0x60},
    {0x1f, 0x16}, //pad drv ,00 03 13 1f 3f james remarked
    {0x20, 0xff},
    {0x21, 0xf8},
    {0x22, 0x57},
    {0x24, 0xa2},   // YUV
    {0x25, 0x0f},
    {0x26, 0x03},//vsync  maybe need changed, value is 0x02 //CHEN-TEST
    {0x2f, 0x01},
    {0x30, 0xf7},
    {0x31, 0x50},
    {0x32, 0x00},
    {0x39, 0x04},
    {0x3a, 0x18},
    {0x3b, 0x20},
    {0x3c, 0x00},
    {0x3d, 0x00},
    {0x3e, 0x00},
    {0x3f, 0x00},
    {0x50, 0x10},
    {0x53, 0x82},
    {0x54, 0x80},
    {0x55, 0x80},
    {0x56, 0x82},
    {0x57, 0x80},  // R
    {0x58, 0x80},  // G
    {0x59, 0x80},  // B
    {0x8b, 0x40},
    {0x8c, 0x40},
    {0x8d, 0x40},
    {0x8e, 0x2e},
    {0x8f, 0x2e},
    {0x90, 0x2e},
    {0x91, 0x3c},
    {0x92, 0x50},
    {0x5d, 0x12},
    {0x5e, 0x1a},
    {0x5f, 0x24},
    {0x60, 0x07},
    {0x61, 0x15},
    {0x62, 0x08},
    {0x64, 0x03},
    {0x66, 0xe8},
    {0x67, 0x86},
    {0x68, 0xa2},
    {0x69, 0x18},
    {0x6a, 0x0f},
    {0x6b, 0x00},
    {0x6c, 0x5f},
    {0x6d, 0x8f},
    {0x6e, 0x55},
    {0x6f, 0x38},
    {0x70, 0x15},
    {0x71, 0x33},
    {0x72, 0xdc},
    {0x73, 0x80},
    {0x74, 0x02},
    {0x75, 0x3f},
    {0x76, 0x02},
    {0x77, 0x36},
    {0x78, 0x88},
    {0x79, 0x81},
    {0x7a, 0x81},
    {0x7b, 0x22},
    {0x7c, 0xff},
    {0x93, 0x48},
    {0x94, 0x00},
    {0x95, 0x05},
    {0x96, 0xe8},
    {0x97, 0x40},
    {0x98, 0xf0},
    {0xb1, 0x38},
    {0xb2, 0x38},
    {0xbd, 0x38},
    {0xbe, 0x36},
    {0xd0, 0xc9},
    {0xd1, 0x10},
    {0xd3, 0x80},
    {0xd5, 0xf2},
    {0xd6, 0x16},
    {0xdb, 0x92},
    {0xdc, 0xa5},
    {0xdf, 0x23},
    {0xd9, 0x00},
    {0xda, 0x00},
    {0xe0, 0x09},
    {0xec, 0x20},
    {0xed, 0x04},
    {0xee, 0xa0},
    {0xef, 0x40},
    {0x80, 0x03},
    {0x80, 0x03},
    {0x9F, 0x0B},
    {0xA0, 0x16},
    {0xA1, 0x29},
    {0xA2, 0x3C},
    {0xA3, 0x4F},
    {0xA4, 0x5F},
    {0xA5, 0x6F},
    {0xA6, 0x8A},
    {0xA7, 0x9F},
    {0xA8, 0xB4},
    {0xA9, 0xC6},
    {0xAA, 0xD3},
    {0xAB, 0xDD},
    {0xAC, 0xE5},
    {0xAD, 0xF1},
    {0xAE, 0xFA},
    {0xAF, 0xFF},
    {0xc0, 0x00},
    {0xc1, 0x10},
    {0xc2, 0x1C},
    {0xc3, 0x30},
    {0xc4, 0x43},
    {0xc5, 0x54},
    {0xc6, 0x65},
    {0xc7, 0x75},
    {0xc8, 0x93},
    {0xc9, 0xB0},
    {0xca, 0xCB},
    {0xcb, 0xE6},
    {0xcc, 0xFF},
    {0xf0, 0x02},
    {0xf1, 0x01},
    {0xf2, 0x01},
    {0xf3, 0x30},
    {0xf9, 0x9f},
    {0xfa, 0x78},
    {0xfe, 0x01},  //set page 1
    {0x00, 0xf5},
    {0x02, 0x1a},
    {0x0a, 0xa0},
    {0x0b, 0x60},
    {0x0c, 0x08},
    {0x0e, 0x4c},
    {0x0f, 0x39},
    {0x11, 0x3f},
    {0x12, 0x72},
    {0x13, 0x13},
    {0x14, 0x42},
    {0x15, 0x43},
    {0x16, 0xc2},
    {0x17, 0xa8},
    {0x18, 0x18},
    {0x19, 0x40},
    {0x1a, 0xd0},
    {0x1b, 0xf5},
    {0x70, 0x40},
    {0x71, 0x58},
    {0x72, 0x30},
    {0x73, 0x48},
    {0x74, 0x20},
    {0x75, 0x60},
    {0x77, 0x20},
    {0x78, 0x32},
    {0x30, 0x03},
    {0x31, 0x40},
    {0x32, 0xe0},
    {0x33, 0xe0},
    {0x34, 0xe0},
    {0x35, 0xb0},
    {0x36, 0xc0},
    {0x37, 0xc0},
    {0x38, 0x04},
    {0x39, 0x09},
    {0x3a, 0x12},
    {0x3b, 0x1C},
    {0x3c, 0x28},
    {0x3d, 0x31},
    {0x3e, 0x44},
    {0x3f, 0x57},
    {0x40, 0x6C},
    {0x41, 0x81},
    {0x42, 0x94},
    {0x43, 0xA7},
    {0x44, 0xB8},
    {0x45, 0xD6},
    {0x46, 0xEE},
    {0x47, 0x0d},
    {0xfe, 0x00},
    {0xd2, 0x90}, // Open AEC at last.
    {0xfe, 0x00},//set Page0
    {0x10, 0x26},
    {0x11, 0x0d},// fd,modified by mormo 2010/07/06
    {0x1a, 0x2a},// 1e,modified by mormo 2010/07/06
    {0x1c, 0x49}, // c1,modified by mormo 2010/07/06
    {0x1d, 0x9a}, // 08,modified by mormo 2010/07/06
    {0x1e, 0x61}, // 60,modified by mormo 2010/07/06
    {0x3a, 0x20},
    {0x50, 0x14},// 10,modified by mormo 2010/07/06
    {0x53, 0x80},
    {0x56, 0x80},
    {0x8b, 0x20}, //LSC
    {0x8c, 0x20},
    {0x8d, 0x20},
    {0x8e, 0x14},
    {0x8f, 0x10},
    {0x90, 0x14},
    {0x94, 0x02},
    {0x95, 0x07},
    {0x96, 0xe0},
    {0xb1, 0x40}, // YCPT
    {0xb2, 0x40},
    {0xb3, 0x40},
    {0xb6, 0xe0},
    {0xd0, 0xcb}, // AECT  c9,modifed by mormo 2010/07/06
    {0xd3, 0x48}, // 80,modified by mormor 2010/07/06
    {0xf2, 0x02},
    {0xf7, 0x12},
    {0xf8, 0x0a},
    {0xfe, 0x01},//set  Page1
    {0x02, 0x20},
    {0x04, 0x10},
    {0x05, 0x08},
    {0x06, 0x20},
    {0x08, 0x0a},
    {0x0e, 0x44},
    {0x0f, 0x32},
    {0x10, 0x41},
    {0x11, 0x37},
    {0x12, 0x22},
    {0x13, 0x19},
    {0x14, 0x44},
    {0x15, 0x44},
    {0x19, 0x50},
    {0x1a, 0xd8},
    {0x32, 0x10},
    {0x35, 0x00},
    {0x36, 0x80},
    {0x37, 0x00},
    {0xfe, 0x00},// set back for page0

};

const uint8_t sensor_gc0308_QVGA_320_240_talbe[][2] =
{
    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x01},
    {0x0a, 0xE8},
    {0x0b, 0x02},
    {0x0c, 0x88},           // 0x88
    {0x46, 0x80},           //crop window mode
    {0x47, 0x78},           //y0 pix
    {0x48, 0xa0},           //x0 pix
    {0x49, 0x00},           //[8] of height
    {0x4a, 0xf0},           //[0-7] of height
    {0x4b, 0x01},           //[9-8] of width
    {0x4c, 0x40},           //[0-7] of width
};


const uint8_t sensor_gc0308_VGA_640_480_talbe[][2] =
{
    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x01},
    {0x0a, 0xE8},
    {0x0b, 0x02},
    {0x0c, 0x88},           // 0x88
};

int gc0308_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_dvp_sensor_t *dvp_sensor = __containerof(controller, bk_camera_dvp_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_sensor, AVDK_ERR_INVAL, TAG, "dvp sensor is NULL");
    bk_camera_bus_t *bus = dvp_sensor->config.bus;
    uint32_t size = sizeof(sensor_gc0308_init_talbe) / 2, i;

    bk_mipi_csi_ext_set_enable(1);

    for (i = 0; i < size; i++)
    {
        bus->write8(bus, sensor_gc0308_init_talbe[i][0], sensor_gc0308_init_talbe[i][1]);
    }

    return 0;
}

int gc0308_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_dvp_sensor_t *dvp_sensor = __containerof(controller, bk_camera_dvp_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_sensor, AVDK_ERR_INVAL, TAG, "dvp sensor is NULL");
    bk_camera_bus_t *bus = dvp_sensor->config.bus;
    uint32_t size, i;
    int ret = -1;

    LOGI("%s\n", __func__);

    if (width == 320 && height == 240)
    {
        size = sizeof(sensor_gc0308_QVGA_320_240_talbe) / 2;

        for (i = 0; i < size; i++)
        {
            bus->write8(bus, sensor_gc0308_QVGA_320_240_talbe[i][0],
                                sensor_gc0308_QVGA_320_240_talbe[i][1]);
        }

        ret = 0;
    }

    else if (width == 640 && height == 480)
    {

        size = sizeof(sensor_gc0308_VGA_640_480_talbe) / 2;
        for (i = 0; i < size; i++)
        {
            bus->write8(bus, sensor_gc0308_VGA_640_480_talbe[i][0],
                                sensor_gc0308_VGA_640_480_talbe[i][1]);
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

int gc0308_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    return 0;
}

int gc0308_reset(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_dvp_sensor_t *dvp_sensor = __containerof(controller, bk_camera_dvp_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_sensor, AVDK_ERR_INVAL, TAG, "dvp sensor is NULL");
    bk_camera_bus_t *bus = dvp_sensor->config.bus;
    bus->write8(bus, 0xFE, 0x80);
    return 0;
}

const dvp_sensor_config_t dvp_sensor_gc0308 =
{
    .name = "gc0308",
    .clk = MCLK_24M,
    .fmt = BK_PIXEL_FORMAT_YUYV,
    .vsync = SYNC_HIGH_LEVEL,
    .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .default_width = 640,
    .default_height = 480,
    .default_fps = 30,
    .id = ID_GC0308,
    .address = (GC0308_WRITE_ADDRESS >> 1),
    .init = gc0308_init,
    .detect = gc0308_detect,
    .set_ppi = gc0308_set_ppi,
    .set_fps = gc0308_set_fps,
    .power_down = gc0308_reset,
};

const ISP_PUB_ATTR_S gc0308_dvp_linear_attr = {
    .pSnsObj      = NULL,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t gc0308_format_array[] = {
    {
        .width = 640,
        .height = 480,
        .fps = 30,
    },
};

void *gc0308_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return NULL;
}

void *gc0308_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

static avdk_err_t gc0308_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &gc0308_format_array[0];
    format_array->size = ARRAY_SIZE(gc0308_format_array);
    return AVDK_ERR_OK;
}

avdk_err_t gc0308_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    gc0308_set_ppi(controller, format->width, format->height);
    gc0308_set_fps(controller, format->fps);
    bk_mipi_csi_controller_init(format->width, format->height, 0x2b);
    return AVDK_ERR_OK;
}

avdk_err_t gc0308_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t data = 0;

    config->bus->write_address = GC0308_WRITE_ADDRESS;

    AVDK_RETURN_ON_FALSE(config->bus, AVDK_ERR_GENERIC, TAG, "bus is NULL");

    LOGI("%s, rest_pin: %d, pwdn_pin: %d\n", __func__, config->pin_reset, config->pin_pwdn);
    /* enable camera power */
    if (config->pin_pwdn != 0xFF)
    {
        gpio_dev_unmap(config->pin_pwdn);
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_pwdn));
        bk_gpio_set_capacity(config->pin_pwdn, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_high(config->pin_pwdn);
        rtos_delay_milliseconds(10);
    }

    if (config->pin_reset != 0xFF)
    {
        gpio_dev_unmap(config->pin_reset);
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_reset));
        bk_gpio_set_capacity(config->pin_reset, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_high(config->pin_reset);
        rtos_delay_milliseconds(10);
    }

    config->bus->read8(config->bus, 0x00, &data);

    if (data != GC0308_CHIP_ID)
    {
        if (config->pin_reset != 0xFF)
        {
            bk_gpio_set_output_low(config->pin_reset);
        }

        if (config->pin_pwdn != 0xFF)
        {
            bk_gpio_set_output_low(config->pin_pwdn);
        }
        return AVDK_ERR_GENERIC;
    }

    LOGI("%s success id: 0x%02X\n", __func__, data);

    bk_camera_dvp_sensor_t *dvp_sensor = os_malloc(sizeof(bk_camera_dvp_sensor_t));
    AVDK_RETURN_ON_FALSE(dvp_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(dvp_sensor, 0, sizeof(bk_camera_dvp_sensor_t));
    config->bus->write_address = GC0308_WRITE_ADDRESS;
    os_memcpy(&dvp_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    dvp_sensor->ops.init = gc0308_init;
    dvp_sensor->ops.set_format = gc0308_set_format;
    dvp_sensor->ops.reg_ctrl = NULL;
    dvp_sensor->ops.get_sensor_object = gc0308_get_sensor_object;
    dvp_sensor->ops.get_sensor_cfg = gc0308_get_sensor_cfg;
    dvp_sensor->ops.query_support_formats = gc0308_query_support_formats;

    dvp_sensor->isp_pub_attr = &gc0308_dvp_linear_attr;
    dvp_sensor->sensor_config = &dvp_sensor_gc0308;
    *handle = (bk_camera_sensor_handle_t)&dvp_sensor->ops;

    return AVDK_ERR_OK;
}

BK_CAMERA_SENSOR_DETECT_SECTION(gc0308_detect, DVP_CAMERA_PORT);