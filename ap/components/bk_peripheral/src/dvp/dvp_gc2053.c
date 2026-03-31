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

#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <vsi_list.h>
#include <driver/isp_types.h>
#include "vsios_i2c.h"

#define GC2053_WRITE_ADDRESS (0x6e)
#define GC2053_READ_ADDRESS (0x6f)
#define CHIP_ID_ADDR_HB (0xF0)
#define CHIP_ID_ADDR_LB (0xF1)
#define CHIP_ID_VAL_HB (0x20)
#define CHIP_ID_VAL_LB (0x53)

#define TAG "gc2053"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

avdk_err_t dvp_gc2053_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

#define SENSOR_I2C_READ(reg, value)\
    do {\
        dvp_camera_i2c_read_uint8((GC2053_WRITE_ADDRESS >> 1), reg, value);\
    } while (0)

#define SENSOR_I2C_WRITE(reg, value)\
    do {\
        dvp_camera_i2c_write_uint8((GC2053_WRITE_ADDRESS >> 1), reg, value);\
    } while (0)

static const uint8_t sensor_gc2053_init_table[][2] = {
    {0xfe,0x80},
    {0xfe,0x80},
    {0xfe,0x80},
    {0xfe,0x00},
    {0xf2,0x00},
    {0xf3,0x0f},
    {0xf4,0x36},
    {0xf5,0xc0},
    {0xf6,0x44},
    {0xf7,0x01},
    {0xf8,0x63},
    {0xf9,0x40},
    {0xfc,0x8e},
    /****CISCTL & ANALOG****/
    {0xfe,0x00},
    {0x87,0x18},
    {0xee,0x30},
    {0xd0,0xb7},
    {0x03,0x01},
    {0x04,0x00},
    {0x05,0x04},
    {0x06,0x4c},
    {0x07,0x00},
    {0x08,0x11},
    {0x09,0x00},
    {0x0a,0x02},
    {0x0b,0x00},
    {0x0c,0x02},
    {0x0d,0x04},
    {0x0e,0x40},
    {0x12,0xe2},
    {0x13,0x16},
    {0x19,0x0a},
    {0x21,0x1c},
    {0x28,0x0a},
    {0x29,0x24},
    {0x2b,0x04},
    {0x32,0xf8},
    {0x37,0x03},
    {0x39,0x15},
    {0x43,0x07},
    {0x44,0x40},
    {0x46,0x0b},
    {0x4b,0x20},
    {0x4e,0x08},
    {0x55,0x20},
    {0x66,0x05},
    {0x67,0x05},
    {0x77,0x01},
    {0x78,0x00},
    {0x7c,0x93},
    {0x8c,0x12},
    {0x8d,0x92},
    {0x90,0x00},
    {0x41,0x04},
    {0x42,0x65},
    {0x9d,0x10},
    {0xce,0x7c},
    {0xd2,0x41},
    {0xd3,0xdc},
    {0xe6,0x50},
    /*gain*/ 
    {0xb6,0xc0},
    {0xb0,0x70},
    {0xb1,0x01},
    {0xb2,0x00},
    {0xb3,0x00},
    {0xb4,0x00},
    {0xb8,0x01},
    {0xb9,0x00},
    /*blk*/
    {0x26,0x30},
    {0xfe,0x01},
    {0x40,0x23},
    {0x55,0x07},
    {0x60,0x40},
    {0xfe,0x04},
    {0x14,0x78},
    {0x15,0x78},
    {0x16,0x78},
    {0x17,0x78},
    /*window*}*/
    {0xfe,0x01},
    {0x92,0x00},
    {0x94,0x03},
    {0x95,0x04},
    {0x96,0x38},
    {0x97,0x07},
    {0x98,0x80},
    /*ISP*/
    {0xfe,0x01},
    {0x01,0x05},
    {0x02,0x89},
    {0x04,0x01},
    {0x07,0xa6},
    {0x08,0xa9},
    {0x09,0xa8},
    {0x0a,0xa7},
    {0x0b,0xff},
    {0x0c,0xff},
    {0x0f,0x00},
    {0x50,0x1c},
    {0x89,0x03},
    {0xfe,0x04},
    {0x28,0x86},
    {0x29,0x86},
    {0x2a,0x86},
    {0x2b,0x68},
    {0x2c,0x68},
    {0x2d,0x68},
    {0x2e,0x68},
    {0x2f,0x68},
    {0x30,0x4f},
    {0x31,0x68},
    {0x32,0x67},
    {0x33,0x66},
    {0x34,0x66},
    {0x35,0x66},
    {0x36,0x66},
    {0x37,0x66},
    {0x38,0x62},
    {0x39,0x62},
    {0x3a,0x62},
    {0x3b,0x62},
    {0x3c,0x62},
    {0x3d,0x62},
    {0x3e,0x62},
    {0x3f,0x62},
    /****DVP & MIPI****/
    {0xfe,0x01},
    {0x9a,0x06},
    {0xfe,0x00},
    {0x7b,0x2a},
    {0x23,0x2d},
    {0xfe,0x03},
    {0x01,0x20},
    {0x02,0x56},
    {0x03,0xb2},
    {0x12,0x80},
    {0x13,0x07},
    {0xfe,0x00},
    {0x3e,0x40},
};

static avdk_err_t gc2053_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_dvp_io_config();
    bk_mipi_csi_ext_set_enable(1);
    rtos_delay_milliseconds(10);

    bk_camera_dvp_sensor_t *dvp_sensor = __containerof(controller, bk_camera_dvp_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_sensor, AVDK_ERR_INVAL, TAG, "dvp sensor is NULL");
    bk_camera_bus_t *bus = dvp_sensor->config.bus;

    uint32_t size = sizeof(sensor_gc2053_init_table) / 2, i;

    LOGI("%p start %x \n", bus->write8, bus->write_address);

    for (i = 0; i < size; i++)
    {
        // os_printf("i2c %x %x \r\n", sensor_gc2053_init_table[i][0], sensor_gc2053_init_table[i][1]);
        // SENSOR_I2C_WRITE(sensor_gc2053_init_table[i][0], sensor_gc2053_init_table[i][1]);
        bus->write8(bus, sensor_gc2053_init_table[i][0], sensor_gc2053_init_table[i][1]);
    }

    return 0;
}

static avdk_err_t gc2053_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    return 0;
}

static avdk_err_t gc2053_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    return 0;
}

avdk_err_t gc2053_reset(bk_camera_sensor_ctlr_t *controller)
{
    return 0;
}

const dvp_sensor_config_t dvp_sensor_gc2053 =
{
    .name = "gc2053",
    .clk = MCLK_24M,
    .fmt = BK_PIXEL_FORMAT_RGGB8,
    .vsync = SYNC_HIGH_LEVEL,
    .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .default_width = 1920,
    .default_height = 1080,
    .default_fps = 20,
    .id = ID_GC2053D,
    .address = (GC2053_WRITE_ADDRESS >> 1),
    .init = gc2053_init,
    .detect = dvp_gc2053_detect,
    .set_ppi = gc2053_set_ppi,
    .set_fps = gc2053_set_fps,
    .power_down = gc2053_reset,
};

extern ISP_SNS_OBJ_S snsGC2053Obj;
const ISP_PUB_ATTR_S gc2053_dvp_linear_attr = {
    .pSnsObj      = &snsGC2053Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB8,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t gc2053_format_array[] = {
    {
        .width = 1280,
        .height = 720,
        .fps = 30,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 25,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 20,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 20,
    },
};

static void *gc2053_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_dvp_sensor_t *dvp_sensor = __containerof(controller, bk_camera_dvp_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsGC2053Obj;
}

static void *gc2053_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_dvp_sensor_t *dvp_sensor = __containerof(controller, bk_camera_dvp_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(dvp_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)dvp_sensor->sensor_config;
}

static avdk_err_t gc2053_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &gc2053_format_array[0];
    format_array->size = ARRAY_SIZE(gc2053_format_array);
    return AVDK_ERR_OK;
}

static avdk_err_t gc2053_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    return AVDK_ERR_OK;
}

avdk_err_t dvp_gc2053_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;

    config->bus->write_address = GC2053_WRITE_ADDRESS;

    AVDK_RETURN_ON_FALSE(config->bus, AVDK_ERR_GENERIC, TAG, "bus is NULL");

    config->bus->read8(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read8(config->bus, CHIP_ID_ADDR_LB, &lb_id);

    if (hb_id != CHIP_ID_VAL_HB
        || lb_id != CHIP_ID_VAL_LB)
    {
        return AVDK_ERR_GENERIC;
    }
    LOGI("%s success id: 0x%02X%02X\n", __func__, hb_id, lb_id);

    bk_camera_dvp_sensor_t *dvp_sensor = os_malloc(sizeof(bk_camera_dvp_sensor_t));
    AVDK_RETURN_ON_FALSE(dvp_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(dvp_sensor, 0, sizeof(bk_camera_dvp_sensor_t));
    config->bus->write_address = GC2053_WRITE_ADDRESS;
    os_memcpy(&dvp_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    dvp_sensor->ops.init = gc2053_init;
    dvp_sensor->ops.set_format = gc2053_set_format;
    dvp_sensor->ops.reg_ctrl = NULL;
    dvp_sensor->ops.get_sensor_object = gc2053_get_sensor_object;
    dvp_sensor->ops.get_sensor_cfg = gc2053_get_sensor_cfg;
    dvp_sensor->ops.query_support_formats = gc2053_query_support_formats;
    dvp_sensor->isp_pub_attr = &gc2053_dvp_linear_attr;
    dvp_sensor->sensor_config = &dvp_sensor_gc2053;
    *handle = (bk_camera_sensor_handle_t)&dvp_sensor->ops;

    return AVDK_ERR_OK;
}

BK_CAMERA_SENSOR_DETECT_SECTION(dvp_gc2053_detect, DVP_CAMERA_PORT);