// Copyright 2024-2025 Beken
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
#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <components/bk_camera_sensor.h>
#include <avdk_check.h>

#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <vsi_list.h>
#include <driver/isp_types.h>
#include <components/bk_isp_camera_types.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#define GC8613YA_WRITE_ADDRESS (0x62)
#define GC8613YA_READ_ADDRESS (0x63)
#define CHIP_ID_ADDR_HB (0x03f0)
#define CHIP_ID_ADDR_MB (0x03f1)
#define CHIP_ID_ADDR_LB (0x03f2)
#define CHIP_ID_VAL_HB (0x86)
#define CHIP_ID_VAL_MB (0x13)
#define CHIP_ID_VAL_LB (0x23)

#define FPS_CTRL_BY_EXP 0 ////controled by exp time
#define FPS_CTRL_BY_LENGTH 1 //controled by framelen and linelen
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH

#define DEFAULT_FRAME_LEN 2250
#define DEFAULT_LINE_LEN 1600
#define GC8613YA_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 15)

#define WIN_MAX_X 3840
#define WIN_MAX_Y 2160

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define TAG "gc8613ya"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#####################################################################################################
#if 1

#include "vsios_i2c.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_calib.h"
#include "gc8613ya_1080p_calib.h"

#define GC8613YA_REG_BYTE_NUM  2
#define GC8613YA_DATA_BYTE_NUM 1

#define GC8613YA_EXPTIME_H		0x0202
#define GC8613YA_EXPTIME_L		0x0203
#define GC8613YA_AGAIN_031d_1		0x031d
#define GC8613YA_AGAIN_1			0x0614
#define GC8613YA_AGAIN_2			0x0615
#define GC8613YA_AGAIN_031d_2		0x031d
#define GC8613YA_AGAIN_3			0x0225
#define GC8613YA_AGAIN_4			0x1467
#define GC8613YA_AGAIN_5			0x1468
#define GC8613YA_AGAIN_6			0x00b8
#define GC8613YA_AGAIN_7			0x00b9
#define GC8613YA_AGAIN_8			0x1447



typedef struct
{
    uint8_t val1;
    uint8_t val2;
    uint8_t val3;
    uint8_t val4;
    uint8_t val5;
    uint8_t val6;
    uint8_t val7;
    uint8_t val8;
} gc8613ya_again_val_t;

static const uint8_t regValTable[30][8] = {
    //   0614  0615  0225  1467  1468  00b8  00b9  1447     | CG  |  Again  | Again dB|
    {0x00, 0x00, 0x00, 0x07, 0x07, 0x01, 0x00, 0x77}, //| LCG |  X1       | 0.00    |
    {0x90, 0x02, 0x00, 0x07, 0x07, 0x01, 0x09, 0x77}, //| LCG |  X1.144   | 1.17    |
    {0x01, 0x00, 0x00, 0x08, 0x08, 0x01, 0x19, 0x77}, //| LCG |  X1.398   | 2.91    |
    {0x91, 0x02, 0x00, 0x08, 0x08, 0x01, 0x2a, 0x77}, //| LCG |  X1.664   | 4.42    |
    {0x02, 0x00, 0x00, 0x09, 0x09, 0x01, 0x3d, 0x77}, //| LCG |  X1.967   | 5.88    |
    {0x00, 0x00, 0x00, 0x07, 0x07, 0x02, 0x10, 0x75}, //| HCG |  X2.251   | 7.05    |
    {0x90, 0x02, 0x00, 0x07, 0x07, 0x02, 0x29, 0x75}, //| HCG |  X2.654   | 8.48    |
    {0x01, 0x00, 0x00, 0x08, 0x08, 0x03, 0x0b, 0x75}, //| HCG |  X3.185   | 10.06   |
    {0x91, 0x02, 0x00, 0x08, 0x08, 0x03, 0x2f, 0x75}, //| HCG |  X3.736   | 11.45   |
    {0x02, 0x00, 0x00, 0x08, 0x08, 0x04, 0x1c, 0x75}, //| HCG |  X4.441   | 12.95   |
    {0x92, 0x02, 0x00, 0x09, 0x09, 0x05, 0x11, 0x75}, //| HCG |  X5.275   | 14.44   |
    {0x03, 0x00, 0x00, 0x0a, 0x0a, 0x06, 0x20, 0x75}, //| HCG |  X6.502   | 16.26   |
    {0x93, 0x02, 0x00, 0x0b, 0x0b, 0x07, 0x25, 0x75}, //| HCG |  X7.589   | 17.60   |
    {0x00, 0x00, 0x01, 0x0c, 0x0c, 0x08, 0x1e, 0x75}, //| HCG |  X8.483   | 18.57   |
    {0x90, 0x02, 0x01, 0x0d, 0x0d, 0x09, 0x3b, 0x75}, //| HCG |  X9.926   | 19.94   |
    {0x01, 0x00, 0x01, 0x0d, 0x0d, 0x0b, 0x3b, 0x75}, //| HCG |  X11.928  | 21.53   |
    {0x91, 0x02, 0x01, 0x0e, 0x0e, 0x0e, 0x03, 0x75}, //| HCG |  X14.053  | 22.96   |
    {0x02, 0x00, 0x01, 0x0f, 0x0f, 0x10, 0x25, 0x75}, //| HCG |  X16.581  | 24.39   |
    {0x92, 0x02, 0x01, 0x10, 0x10, 0x13, 0x35, 0x75}, //| HCG |  X19.839  | 25.95   |
    {0x03, 0x00, 0x01, 0x11, 0x11, 0x17, 0x30, 0x75}, //| HCG |  X23.759  | 27.52   |
    {0x93, 0x02, 0x01, 0x13, 0x13, 0x1c, 0x06, 0x75}, //| HCG |  X28.101  | 28.97   |
    {0x04, 0x00, 0x01, 0x14, 0x14, 0x21, 0x07, 0x75}, //| HCG |  X33.111  | 30.40   |
    {0x94, 0x02, 0x01, 0x15, 0x15, 0x27, 0x38, 0x75}, //| HCG |  X39.879  | 32.01   |
    {0x05, 0x00, 0x01, 0x17, 0x17, 0x2f, 0x18, 0x75}, //| HCG |  X47.388  | 33.51   |
    {0x95, 0x02, 0x01, 0x19, 0x19, 0x38, 0x09, 0x75}, //| HCG |  X56.14   | 34.99   |
    {0x06, 0x00, 0x01, 0x1a, 0x1a, 0x41, 0x37, 0x75}, //| HCG |  X65.87   | 36.37   |
    {0x96, 0x02, 0x01, 0x1c, 0x1c, 0x4e, 0x3b, 0x75}, //| HCG |  X78.92   | 37.94   |
    {0xb6, 0x04, 0x01, 0x1d, 0x1d, 0x5c, 0x10, 0x75}, //| HCG |  X92.25   | 39.30   |
    {0x86, 0x06, 0x01, 0x1e, 0x1e, 0x6f, 0x1b, 0x75}, //| HCG |  X111.43  | 40.94   |
    {0x06, 0x08, 0x01, 0x20, 0x20, 0x84, 0x3a, 0x75}, //| HCG |  X132.92  | 42.47   |
};

static const uint8_t regValTable_1080p[26][8] = {
    //  0614  0615	 0225  1467	 1468  00b8  00b9  1447
    {0x00, 0x00, 0x00, 0x07, 0x07, 0x01, 0x00, 0x77},
    {0x90, 0x02, 0x00, 0x07, 0x07, 0x01, 0x09, 0x77},
    {0x01, 0x00, 0x00, 0x08, 0x08, 0x01, 0x19, 0x77},
    {0x91, 0x02, 0x00, 0x08, 0x08, 0x01, 0x2A, 0x77},
    {0x02, 0x00, 0x00, 0x09, 0x09, 0x01, 0x3D, 0x77},
    {0x00, 0x00, 0x00, 0x07, 0x07, 0x02, 0x10, 0x75},
    {0x90, 0x02, 0x00, 0x07, 0x07, 0x02, 0x29, 0x75},
    {0x01, 0x00, 0x00, 0x08, 0x08, 0x03, 0x0B, 0x75},
    {0x91, 0x02, 0x00, 0x08, 0x08, 0x03, 0x2F, 0x75},
    {0x02, 0x00, 0x00, 0x08, 0x08, 0x04, 0x1C, 0x75},
    {0x92, 0x02, 0x00, 0x09, 0x09, 0x05, 0x11, 0x75},
    {0x03, 0x00, 0x00, 0x0a, 0x0a, 0x06, 0x20, 0x75},
    {0x93, 0x02, 0x00, 0x0b, 0x0b, 0x07, 0x25, 0x75},
    {0x00, 0x00, 0x01, 0x0c, 0x0c, 0x08, 0x1E, 0x75},
    {0x90, 0x02, 0x01, 0x0d, 0x0d, 0x09, 0x3B, 0x75},
    {0x01, 0x00, 0x01, 0x0d, 0x0d, 0x0B, 0x3B, 0x75},
    {0x91, 0x02, 0x01, 0x0e, 0x0e, 0x0E, 0x03, 0x75},
    {0x02, 0x00, 0x01, 0x0f, 0x0f, 0x10, 0x25, 0x75},
    {0x92, 0x02, 0x01, 0x10, 0x10, 0x13, 0x35, 0x75},
    {0x03, 0x00, 0x01, 0x11, 0x11, 0x17, 0x30, 0x75},
    {0x93, 0x02, 0x01, 0x13, 0x13, 0x1C, 0x06, 0x75},
    {0x04, 0x00, 0x01, 0x14, 0x14, 0x21, 0x07, 0x75},
    {0x94, 0x02, 0x01, 0x15, 0x15, 0x27, 0x38, 0x75},
    {0x05, 0x00, 0x01, 0x17, 0x17, 0x2F, 0x18, 0x75},
    {0x95, 0x02, 0x01, 0x19, 0x19, 0x38, 0x09, 0x75},
    {0x06, 0x00, 0x01, 0x1a, 0x1a, 0x41, 0x37, 0x75},
};


#define AGAIN_TOTAL_LEVEL 31
#define AGAIN_TOTAL_LEVEL_1080p 27
static UINT32 gainLevelTable[AGAIN_TOTAL_LEVEL] = {
    64,
    73,
    89,
    106,
    125,
    144,
    169,
    203,
    239,
    284,
    337,
    416,
    485,
    542,
    635,
    763,
    899,
    1061,
    1269,
    1520,
    1798,
    2119,
    2552,
    3032,
    3592,
    4215,
    5050,
    5904,
    7131,
    8506,
    0xffffffff,
};

static UINT32 gainLevelTable_1080p[AGAIN_TOTAL_LEVEL_1080p] = {
    64,
    73,
    89,
    107,
    126,
    144,
    170,
    204,
    239,
    284,
    338,
    416,
    486,
    543,
    635,
    763,
    899,
    1061,
    1270,
    1521,
    1798,
    2119,
    2552,
    3033,
    3593,
    4216,
  0xffffffff,
};

enum GC8613YA_REG_INDEX {
    REG_EXPTIME_H       = 0,
    REG_EXPTIME_L       = 1,
    REG_AGAIN_031d_1    = 2,
    REG_AGAIN_1         = 3,
    REG_AGAIN_2         = 4,
    REG_AGAIN_031d_2    = 5,
    REG_AGAIN_3         = 6,
    REG_AGAIN_4         = 7,
    REG_AGAIN_5         = 8,
    REG_AGAIN_6         = 9,
    REG_AGAIN_7         = 10,
    REG_AGAIN_8         = 11,
};

#define GC8613YA_2160P_15FPS_LINEAR_MODE (0)
#define GC8613YA_1080P_20FPS_LINEAR_MODE (1)

typedef struct vsiGC8613YA_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} GC8613YA_DEVICE_S;

static GC8613YA_DEVICE_S *GC8613YADev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * GC8613YA_720P_CalibParam_dynamic = NULL;

static GC8613YA_DEVICE_S *GC8613YA_GetSensorDev(ISP_PORT IspPort)
{
    if (GC8613YADev[IspPort.devId][IspPort.portId] == NULL)
    {
        GC8613YADev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(GC8613YA_DEVICE_S));
        if (GC8613YADev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d GC8613YADev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(GC8613YADev[IspPort.devId][IspPort.portId], 0 , sizeof(GC8613YA_DEVICE_S));
    }

    return GC8613YADev[IspPort.devId][IspPort.portId];
}

static int GC8613YA_SetStream(ISP_PORT IspPort, vsi_bool_t stream);


static int GC8613YA_InitRegInfo(ISP_PORT IspPort)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC8613YADev->snsRegsInfo;
    pSnsRegsInfo->snsDev = pGC8613YADev->i2cBus;

    pSnsRegsInfo->addrByteNum = pGC8613YADev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = pGC8613YADev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = pGC8613YADev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 12;
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_EXPTIME_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_H].regAddr = GC8613YA_EXPTIME_H;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].regAddr = GC8613YA_EXPTIME_L;

    pSnsRegsInfo->snsData[REG_AGAIN_031d_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_031d_1].regAddr = GC8613YA_AGAIN_031d_1;

    pSnsRegsInfo->snsData[REG_AGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_1].regAddr = GC8613YA_AGAIN_1;
    pSnsRegsInfo->snsData[REG_AGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_2].regAddr = GC8613YA_AGAIN_2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_031d_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_031d_2].regAddr = GC8613YA_AGAIN_031d_2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].regAddr = GC8613YA_AGAIN_3;
    pSnsRegsInfo->snsData[REG_AGAIN_4].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_4].regAddr = GC8613YA_AGAIN_4;
    pSnsRegsInfo->snsData[REG_AGAIN_5].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_5].regAddr = GC8613YA_AGAIN_5;
    pSnsRegsInfo->snsData[REG_AGAIN_6].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_6].regAddr = GC8613YA_AGAIN_6;
    pSnsRegsInfo->snsData[REG_AGAIN_7].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_7].regAddr = GC8613YA_AGAIN_7;
    pSnsRegsInfo->snsData[REG_AGAIN_8].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_8].regAddr = GC8613YA_AGAIN_8;

    return BK_OK;
}

static int GC8613YA_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (GC8613YA_720P_CalibParam_dynamic == NULL)
    {
        GC8613YA_720P_CalibParam_dynamic = os_malloc(sizeof(GC8613YA_1080P_CalibParam));
        if (GC8613YA_720P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc GC8613YA_720P_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(GC8613YA_720P_CalibParam_dynamic, &GC8613YA_1080P_CalibParam, sizeof(GC8613YA_1080P_CalibParam));
    }

    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(pGC8613YADev, 0, sizeof(*pGC8613YADev));
    pGC8613YADev->i2cBus              = snsDev;
    pGC8613YADev->i2cAttr.slave_addr  = GC8613YA_WRITE_ADDRESS;
    pGC8613YADev->i2cAttr.reg_bytes   = GC8613YA_REG_BYTE_NUM;
    pGC8613YADev->i2cAttr.data_bytes  = GC8613YA_DATA_BYTE_NUM;
    GC8613YA_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return BK_FAIL;
    }

    GC8613YA_SetStream(IspPort, 0);

    return  BK_OK;
}

static int GC8613YA_Exit(ISP_PORT IspPort)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);

    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d already exit\n", __func__, __LINE__);
        return BK_OK;
    }

    vsios_i2c_sys_exit(pGC8613YADev->i2cBus);

    if (GC8613YA_720P_CalibParam_dynamic != NULL)
    {
        os_free(GC8613YA_720P_CalibParam_dynamic);
        GC8613YA_720P_CalibParam_dynamic = NULL;
    }

    os_free(pGC8613YADev);
    GC8613YADev[IspPort.devId][IspPort.portId] = NULL;
    return BK_OK;
}

static int GC8613YA_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC8613YADev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC8613YADev->i2cAttr;

    // LOGD("i2c write (%x, %x) \r\n", addr, data);
    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);

    return  BK_OK;
}

static int GC8613YA_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC8613YADev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC8613YADev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int GC8613YA_InitAeDefault(ISP_PORT IspPort)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC8613YADev->aeDefault;

    switch (pGC8613YADev->snsModeId) {
        case GC8613YA_2160P_15FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = 2250;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 15 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms = pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);
            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;   
            pAeSnsDft->maxAgain  = 8506;
            pAeSnsDft->minAgain  = 64;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 100;
            pAeSnsDft->dampOver = 0x0;
            pAeSnsDft->dampUnder = 0x0;
            pAeSnsDft->tolerance = 1;
            pAeSnsDft->initExposure = 0x127 * pAeSnsDft->minAgain;
            break;
        case GC8613YA_1080P_20FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            // pAeSnsDft->fullLinesStd = 1800;
            // pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            // pAeSnsDft->fps = 20 * ISP_SNS_FPS_ACCU;
            // pAeSnsDft->linesPer500ms = pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);
            // pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            // pAeSnsDft->minIntLine  = 1;
            // pAeSnsDft->intLineStep = 1;   
            // pAeSnsDft->maxAgain  = 4216;
            // pAeSnsDft->minAgain  = 64;
            // pAeSnsDft->againStep = 1;

            // pAeSnsDft->maxDgain  = 1024;
            // pAeSnsDft->minDgain  = 1024;
            // pAeSnsDft->dgainStep = 1;

            // pAeSnsDft->aeTarget = 100;
            // pAeSnsDft->dampOver = 0x40;
            // pAeSnsDft->dampUnder = 0x40;
            // pAeSnsDft->tolerance = 1;
            // pAeSnsDft->initExposure = 0x18a * pAeSnsDft->minAgain;  //0x18a
            // break;

            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = 2250;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 15 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms = pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);
            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;   
            pAeSnsDft->maxAgain  = 8506;
            pAeSnsDft->minAgain  = 64;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 120;
            pAeSnsDft->dampOver = 0x0;
            pAeSnsDft->dampUnder = 0x0;
            pAeSnsDft->tolerance = 1;
            pAeSnsDft->initExposure = 0x127 * pAeSnsDft->minAgain;
            break;
        default:
            break;
    }

    return BK_OK;
}

static int GC8613YA_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if ((pSnsMode->width  == 3840) &&
        (pSnsMode->height == 2160) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC8613YA_Linear1920x1080Init(pGC8613YADev->i2cBus, &pGC8613YADev->i2cAttr);
        os_memcpy(&pGC8613YADev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC8613YADev->snsModeId = GC8613YA_2160P_15FPS_LINEAR_MODE;
        GC8613YA_InitAeDefault(IspPort);
    }
    else if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC8613YA_Linear1920x1080Init(pGC8613YADev->i2cBus, &pGC8613YADev->i2cAttr);
        os_memcpy(&pGC8613YADev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC8613YADev->snsModeId = GC8613YA_1080P_20FPS_LINEAR_MODE;
        GC8613YA_InitAeDefault(IspPort);
    } else {
        LOGI("gc8613ya custom set mode end ~~~\n");
        os_memcpy(&pGC8613YADev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC8613YADev->snsModeId = GC8613YA_2160P_15FPS_LINEAR_MODE;
        GC8613YA_InitAeDefault(IspPort);
    }

    return BK_OK;
}

static int GC8613YA_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        GC8613YA_WriteReg(IspPort, 0x03fe,0x10);
        GC8613YA_WriteReg(IspPort, 0x0115,0x12);
        GC8613YA_WriteReg(IspPort, 0x061b,0x17);
        GC8613YA_WriteReg(IspPort, 0x061c,0x50);
        GC8613YA_WriteReg(IspPort, 0x0a20,0x19);
        GC8613YA_WriteReg(IspPort, 0x0a34,0x40);
        GC8613YA_WriteReg(IspPort, 0x031c,0xce);
        GC8613YA_WriteReg(IspPort, 0x0a38,0x01);
        GC8613YA_WriteReg(IspPort, 0x0181,0xf0);
        GC8613YA_WriteReg(IspPort, 0x031d,0x2d);
        GC8613YA_WriteReg(IspPort, 0x0116,0x29);
        GC8613YA_WriteReg(IspPort, 0x0180,0x46);
        GC8613YA_WriteReg(IspPort, 0x0185,0x01);
        GC8613YA_WriteReg(IspPort, 0x0100,0x09);
        GC8613YA_WriteReg(IspPort, 0x031d,0x28);
        GC8613YA_WriteReg(IspPort, 0x03fe,0x00);
        GC8613YA_WriteReg(IspPort, 0x031d,0x2d);
        GC8613YA_WriteReg(IspPort, 0x0245,0xc7);
        GC8613YA_WriteReg(IspPort, 0x0618,0x01);
        GC8613YA_WriteReg(IspPort, 0x031d,0x28);
    } else {
        GC8613YA_WriteReg(IspPort, 0x031d, 0x2d);
        GC8613YA_WriteReg(IspPort, 0x0100, 0x00);
        GC8613YA_WriteReg(IspPort, 0x031d, 0x28);
    }

    pGC8613YADev->stream = 1;

    return BK_OK;
}

static int GC8613YA_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, GC8613YA_720P_CalibParam_dynamic);

    return BK_OK;
}

static int GC8613YA_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = GC8613YA_Init;
    pIspSnsFunc->pfnSensorExit    = GC8613YA_Exit;
    pIspSnsFunc->pfnWriteReg      = GC8613YA_WriteReg;
    pIspSnsFunc->pfnReadReg       = GC8613YA_ReadReg;
    pIspSnsFunc->pfnSetMode       = GC8613YA_SetMode;
    pIspSnsFunc->pfnSetStream     = GC8613YA_SetStream;
    pIspSnsFunc->pfnSetIspDefault = GC8613YA_SetIspDefault;

    return BK_OK;
}

static int GC8613YA_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &pGC8613YADev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

#if 0
static int GC8613YA_SetFps(ISP_PORT IspPort, vsi_u32_t fps)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC8613YADev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC8613YADev->snsRegsInfo;
    vsi_u32_t vts;

    switch(pGC8613YADev->snsModeId) {
        case GC8613YA_720P_30FPS_LINEAR_MODE:
            if ((fps <= 30 * ISP_SNS_FPS_ACCU) && (fps >= 0.5 * ISP_SNS_FPS_ACCU)) {
                vts = GC8613YA_VMAX_720P30_LINEAR * 30 * ISP_SNS_FPS_ACCU / fps;
            } else {
                return BK_FAIL;
            }
            vts = (vts >= pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : vts;
            pSnsRegsInfo->snsData[REG_VTS_H].data = ((vts & 0xFF00) >> 8);
            pSnsRegsInfo->snsData[REG_VTS_L].data = (vts & 0xFF);
            pAeSnsDft->fullLines = vts;
            pAeSnsDft->fps = fps;
            pAeSnsDft->maxIntLine = pAeSnsDft->fullLines - 2;
            break;
        default:
            return BK_FAIL;
            break;
    }

    return BK_OK;
}

static int GC8613YA_SlowFrameRate(ISP_PORT IspPort, vsi_u32_t fullLines)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC8613YADev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC8613YADev->snsRegsInfo;

    fullLines = (fullLines > pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : fullLines;
    pAeSnsDft->fullLines = fullLines;

    pSnsRegsInfo->snsData[REG_VTS_H].data = ((fullLines & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_VTS_L].data = (fullLines & 0xFF);

    switch(pGC8613YADev->snsModeId) {
        case GC8613YA_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->maxIntLine = pAeSnsDft->fullLines - 2;
            break;
        default:
            break;
    }

    return BK_OK;
}

static int GC8613YA_SetExpRatio(ISP_PORT IspPort, ISP_EXP_RATIO_S *pExpRatio)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC8613YADev->aeDefault;

    os_memcpy(&pAeSnsDft->expRatio, pExpRatio, sizeof(*pExpRatio));

    return BK_OK;
}
#endif

static void GC8613YA_CalcGain(uint32_t width, uint32_t height, uint32_t *pGain, gc8613ya_again_val_t *again_val, uint16_t *pDGainReg, uint8_t *pConvReg)
{
    uint32_t ae_gain = *pGain;
    int i;
    int again_total_level;
    UINT32 *gain_level_table;
    const uint8_t (*reg_val_table)[8];

    /* Select gain table by resolution */
    if (width == 1920 && height == 1080) {
        again_total_level = AGAIN_TOTAL_LEVEL_1080p;
        gain_level_table = gainLevelTable_1080p;
        reg_val_table = regValTable_1080p;
    } else {
        again_total_level = AGAIN_TOTAL_LEVEL;
        gain_level_table = gainLevelTable;
        reg_val_table = regValTable;
    }

    /* Find proper analog gain level according to requested gain */
    for (i = 0; i < again_total_level - 1; i++)
    {
        if ((gain_level_table[i] <= ae_gain) && (ae_gain < gain_level_table[i + 1]))
            break;
    }
    if (i >= again_total_level - 1)
    {
        i = again_total_level - 2;
    }

    /* Quantize gain to table level for reporting back to AE */
    *pGain = gain_level_table[i];

    /* Map level index to sensor analog gain registers */
    again_val->val1 = reg_val_table[i][0];
    again_val->val2 = reg_val_table[i][1];
    again_val->val3 = reg_val_table[i][2];
    again_val->val4 = reg_val_table[i][3];
    again_val->val5 = reg_val_table[i][4];
    again_val->val6 = reg_val_table[i][5];
    again_val->val7 = reg_val_table[i][6];
    again_val->val8 = reg_val_table[i][7];

    /* For now keep digital gain at 1x and convReg unused */
    if (pDGainReg)
        *pDGainReg = 1024;
    if (pConvReg)
        *pConvReg = 0;
}

static uint8_t fix_level = 0;
static void set_again_fix_level(uint8_t level)
{
	fix_level = level;
}

static int GC8613YA_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC8613YADev->snsRegsInfo;
    uint32_t gain;
    gc8613ya_again_val_t reg_val;
    uint16_t dGainReg;
    uint8_t convReg;


    gain = (uint32_t)(*pAgain);
    GC8613YA_CalcGain(pGC8613YADev->snsMode.width, pGC8613YADev->snsMode.height,
        &gain, &reg_val, &dGainReg, &convReg);
    *pAgain = (vsi_u32_t)gain;
    *pDgain = 1024;

    /* Program analog gain registers according to calculated level */
    pSnsRegsInfo->snsData[REG_AGAIN_031d_1].data = 0x2d;
    pSnsRegsInfo->snsData[REG_AGAIN_1].data = reg_val.val1;
    pSnsRegsInfo->snsData[REG_AGAIN_2].data = reg_val.val2;
    pSnsRegsInfo->snsData[REG_AGAIN_031d_2].data = 0x28;
    pSnsRegsInfo->snsData[REG_AGAIN_3].data = reg_val.val3;
    pSnsRegsInfo->snsData[REG_AGAIN_4].data = reg_val.val4;
    pSnsRegsInfo->snsData[REG_AGAIN_5].data = reg_val.val5;
    pSnsRegsInfo->snsData[REG_AGAIN_6].data = reg_val.val6;
    pSnsRegsInfo->snsData[REG_AGAIN_7].data = reg_val.val7;
    pSnsRegsInfo->snsData[REG_AGAIN_8].data = reg_val.val8;

    /* Program digital gain registers from *pDgain (1x by default) */
    // pSnsRegsInfo->snsData[REG_DGAIN_1].data = (uint8_t)((*pDgain >> 8) & 0xFF);
    // pSnsRegsInfo->snsData[REG_DGAIN_2].data = (uint8_t)(*pDgain & 0xFF);


    //LOGI("%s: again = %u, dgain = %u\n", __func__, (unsigned int)(*pAgain), (unsigned int)(*pDgain));

    return BK_OK;
}

static int GC8613YA_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC8613YADev->snsRegsInfo;

    // uint32_t new_lines = (((*pIntLine) - 1)/10 + 1) * 10;


    pSnsRegsInfo->snsData[REG_EXPTIME_H].data = ((*pIntLine & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_EXPTIME_L].data = (*pIntLine & 0xFF);
    //LOGI("%s: intTime(line) = %u\n", __func__, (unsigned int)(*pIntLine));

    return BK_OK;
}

static int GC8613YA_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    GC8613YA_DEVICE_S *pGC8613YADev = GC8613YA_GetSensorDev(IspPort);
    if (pGC8613YADev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &pGC8613YADev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int GC8613YA_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = GC8613YA_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = GC8613YA_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = GC8613YA_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = GC8613YA_GetSnsRegInfo;

    return BK_OK;
}

const ISP_SNS_OBJ_S snsGC8613YAObj = {
    .pfnInitIspSnsFunc = GC8613YA_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = GC8613YA_InitAeSnsFunc,
};

#endif
//#####################################################################################################

#define GC8613YA_TABLE_SIZE(table) (sizeof(table) / 2 /sizeof(uint16_t))

avdk_err_t gc8613ya_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

static const uint16_t sensor_gc8613ya_init_table[][2] =
{
    {0x03fe,0xf0},
    {0x03fe,0x00},
    {0x03fe,0x10},
    {0x0a38,0x01},
    {0x0a20,0x19},
    {0x061b,0x17},
    {0x061c,0x50},
    {0x061d,0x05},
    {0x061e,0x5a},
    {0x061f,0x05},
    {0x0a21,0x28},
    {0x0a30,0x00},
    {0x0a31,0x7e},
    {0x0a34,0x40},
    {0x0a35,0x08},
    {0x0a37,0x43},
    {0x0314,0x50},
    {0x0315,0x00},
    {0x031c,0xce},
    {0x0219,0x47},
    {0x0342,0x06},
    {0x0343,0x40},
    {0x0259,0x08},
    {0x025a,0x96},
    {0x0340,0x08},
    {0x0341,0xca},//vts = 2250
    {0x0351,0x00},
    {0x0345,0x02},
    {0x0347,0x02},
    {0x0348,0x0f},
    {0x0349,0x18},
    {0x034a,0x08},
    {0x034b,0x88},
    {0x034f,0xf0},
    {0x0094,0x0f},
    {0x0095,0x00},
    {0x0096,0x08},
    {0x0097,0x70},
    {0x0099,0x0c},
    {0x009b,0x0c},
    {0x060c,0x06},
    {0x060e,0x20},
    {0x060f,0x0f},
    {0x070c,0x06},
    {0x070e,0x20},
    {0x070f,0x0f},
    {0x0087,0x50},
    {0x141b,0x03},
    {0x0907,0xd5},
    {0x0909,0x06},
    {0x0901,0x0e},
    {0x0902,0x0b},
    {0x0904,0x08},
    {0x0908,0x09},
    {0x0903,0xc5},
    {0x090c,0x09},
    {0x0905,0x10},
    {0x0906,0x00},
    {0x072a,0x7c},
    {0x0724,0x2b},
    {0x0727,0x2b},
    {0x072b,0x1c},
    {0x073e,0x40},
    {0x0078,0x88},
    {0x0618,0x01},
    {0x1466,0x12},
    {0x1468,0x07},
    {0x1467,0x07},
    {0x0709,0x40},
    {0x0719,0x40},
    {0x1469,0x80},
    {0x146a,0xc0},
    {0x146b,0x03},
    {0x1480,0x02},
    {0x1481,0x80},
    {0x1484,0x08},
    {0x1485,0xc0},
    {0x1430,0x80},
    {0x1407,0x10},
    {0x1408,0x16},
    {0x1409,0x03},
    {0x1434,0x04},
    {0x1447,0x75},
    {0x1470,0x10},
    {0x1471,0x13},
    {0x1438,0x00},
    {0x143a,0x00},
    {0x0220,0x80},
    {0x024b,0x02},
    {0x0245,0xc7},
    {0x025b,0x07},
    {0x02bb,0x77},
    {0x0612,0x01},
    {0x0613,0x26},
    {0x0243,0x66},
    {0x0087,0x53},
    {0x0053,0x05},
    {0x0089,0x02},
    {0x0002,0xeb},
    {0x005a,0x0c},
    {0x0040,0x83},
    {0x0075,0x68},
    {0x0205,0x0c},
    {0x0202,0x01},
    {0x0203,0x27},
    {0x061a,0x02},
    {0x03fe,0x00},
    {0x0106,0x78},
    {0x0136,0x00},
    {0x0181,0xf0},
    {0x0185,0x01},
    {0x0180,0x46},
    {0x0106,0x38},
    {0x010d,0xc0},
    {0x010e,0x12},
    {0x0113,0x02},
    {0x0114,0x01},
    {0x0115,0x12},
    {0x0122,0x0b},
    {0x0123,0x30},
    {0x0124,0x04},
    {0x0125,0x20},
    {0x0126,0x09},
    {0x0127,0x15},
    {0x0128,0xa8},
    {0x0129,0x0b},
    {0x012a,0x10},
    {0x012b,0x0a},
    {0x0004,0x0f},
    {0x0219,0x47},
    {0x0054,0x98},
    {0x0076,0x01},
    {0x0052,0x02},
    {0x021a,0x10},
    {0x0430,0x10},
    {0x0431,0x10},
    {0x0432,0x10},
    {0x0433,0x10},
    {0x0434,0x6d},
    {0x0435,0x6d},
    {0x0436,0x6d},
    {0x0437,0x6d},
    {0x0704,0x03},
    {0x0706,0x02},
    {0x0716,0x02},
    {0x0708,0xc8},
    {0x0718,0xc8},
    {0x071d,0xdc},
    {0x071e,0x05},
    //otp autoload
    {0x031f,0x01},
    {0x031f,0x00},
    {0x0a67,0x80},
    {0x0a54,0x0e},
    {0x0a65,0x10},
    {0x0a98,0x04},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0089,0x02},
    {0x0aa0,0x00},
    {0x0023,0x00},
    {0x0022,0x00},
    {0x0025,0x00},
    {0x0024,0x00},
    {0x0028,0x0f},
    {0x0029,0x18},
    {0x002a,0x08},
    {0x002b,0x88},
    {0x0317,0x1c},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0xe0},
    {0x0a71,0x00},
    {0x0a72,0x02},
    {0x0a73,0x60},
    {0x0a75,0x41},
    {0x0a70,0x03},
    {0x0a5a,0x80},
    /* sleep 20 */
    {0x0089,0x02},
    {0x05be,0x01},
    {0x0a70,0x00},
    {0x0080,0x02},
    {0x0a67,0x00},
    {0x0100,0x09},
    {0x0058,0x00},
    {0x0059,0x04},
};

static const uint16_t sensor_gc8613ya_init_table_1080p_20fps[][2] =
{
#if 0
    {0x03fe,0xf0},
    {0x03fe,0x00},
    {0x03fe,0x10},
    {0x0a38,0x01},
    {0x0a20,0x19},
    {0x061b,0x17},
    {0x061c,0x50},
    {0x061d,0x06},
    {0x061e,0x87},
    {0x061f,0x05},
    {0x0a21,0x10},
    {0x0a28,0x00},
    {0x0a30,0x00},
    {0x0a31,0xfb},
    {0x0a34,0x40},
    {0x0a35,0x08},
    {0x0a37,0x46},
    {0x0314,0x50},
    {0x031c,0xce},
    {0x0219,0x47},
    {0x0342,0x05},
    {0x0343,0xdc},
    {0x0259,0x04},
    {0x025a,0x00},
    {0x0340,0x07},
    {0x0341,0x08},//vts1800
    {0x0351,0x00},
    {0x0345,0x02},
    {0x0347,0x02},
    {0x0348,0x0f},
    {0x0349,0x10},
    {0x034a,0x08},
    {0x034b,0x88},
    {0x034f,0xf0},
    {0x0094,0x0f},
    {0x0095,0x00},
    {0x0096,0x08},
    {0x0097,0x70},
    {0x0099,0x09},
    {0x009b,0x09},
    {0x060c,0x0a},
    {0x060e,0x20},
    {0x060f,0x0f},
    {0x070c,0x0a},
    {0x070e,0x20},
    {0x070f,0x0f},
    {0x0087,0x50},
    {0x141b,0x03},
    {0x0907,0xd5},
    {0x0909,0x06},
    {0x0901,0x0e},
    {0x0902,0x1b},
    {0x0904,0x08},
    {0x0908,0x09},
    {0x0903,0xc5},
    {0x090c,0x09},
    {0x0905,0x10},
    {0x0906,0x00},
    {0x072a,0x7c},
    {0x0724,0x2b},
    {0x0727,0x2b},
    {0x072b,0x1c},
    {0x073e,0x40},
    {0x0078,0x88},
    {0x0618,0x01},
    {0x1466,0x12},
    {0x1468,0x07},
    {0x1467,0x07},
    {0x1478,0x07},
    {0x1477,0x07},
    {0x0709,0x40},
    {0x0719,0x40},
    {0x1469,0x80},
    {0x146a,0x20},
    {0x146b,0x03},
    {0x1479,0x80},
    {0x147a,0x20},
    {0x147b,0x03},
    {0x1480,0x02},
    {0x1481,0x80},
    {0x1482,0x02},
    {0x1483,0x80},
    {0x1484,0x08},
    {0x1485,0xc0},
    {0x1486,0x08},
    {0x1487,0xc0},
    {0x1407,0x10},
    {0x1408,0x16},
    {0x1409,0x03},
    {0x1434,0x04},
    {0x1447,0x75},
    {0x140d,0x04},
    {0x1461,0x10},
    {0x146c,0x10},
    {0x146d,0x10},
    {0x146e,0x2e},
    {0x146f,0x30},
    {0x1474,0x34},
    {0x1470,0x10},
    {0x1471,0x13},
    {0x143a,0x00},
    {0x0220,0x80},
    {0x024b,0x02},
    {0x0245,0xc7},
    {0x025b,0x07},
    {0x02bb,0x77},
    {0x0612,0x01},
    {0x0613,0x26},
    {0x0087,0x53},
    {0x0089,0x02},
    {0x0002,0xeb},
    {0x005a,0x0c},
    {0x0040,0x83},
    {0x0075,0x58},
    {0x0077,0x08},
    {0x0218,0x10},
    {0x0205,0x0c},
    {0x0202,0x01},
    {0x0203,0x8a},
    {0x061a,0x02},
    {0x0122,0x0b},
    {0x0123,0x30},
    {0x0124,0x0b},
    {0x0126,0x09},
    {0x0129,0x0b},
    {0x012a,0x16},
    {0x012b,0x0a},
    {0x03fe,0x00},
    {0x0106,0x78},
    {0x0136,0x00},
    {0x0181,0xf0},
    {0x0185,0x01},
    {0x0180,0x46},
    {0x0106,0x38},
    {0x010d,0x60},
    {0x010e,0x09},
    {0x0113,0x02},
    {0x0114,0x01},
    {0x0100,0x09},
    {0x0219,0x47},
    {0x0054,0x98},
    {0x0076,0x01},
    {0x0052,0x02},
    {0x021a,0x10},
    {0x0430,0x10},
    {0x0431,0x10},
    {0x0432,0x10},
    {0x0433,0x10},
    {0x0434,0x6d},
    {0x0435,0x6d},
    {0x0436,0x6d},
    {0x0437,0x6d},
    {0x0704,0x03},
    {0x0706,0x02},
    {0x0716,0x02},
    {0x0708,0xc8},
    {0x0718,0xc8},
    {0x071d,0xdc},
    {0x071e,0x05},
    //otp autoload
    {0x031f,0x01},
    {0x031f,0x00},
    {0x0a67,0x80},
    {0x0a54,0x0e},
    {0x0a65,0x10},
    {0x0a98,0x04},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0089,0x02},
    {0x0aa0,0x00},
    {0x0023,0x00},
    {0x0022,0x00},
    {0x0025,0x00},
    {0x0024,0x00},
    {0x0028,0x0f},
    {0x0029,0x18},
    {0x002a,0x08},
    {0x002b,0x88},
    {0x0317,0x1c},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0xe0},
    {0x0a71,0x00},
    {0x0a72,0x02},
    {0x0a73,0x60},
    {0x0a75,0x41},
    {0x0a70,0x03},
    {0x0a5a,0x80},
    /* sleep 20 */
    {0x0089,0x02},
    {0x05be,0x01},
    {0x0a70,0x00},
    {0x0080,0x02},
    {0x0a67,0x00},
    {0x0058,0x00},
    {0x0059,0x04},
#endif

#if 1
    {0x03fe,0xf0},
    {0x03fe,0x00},
    {0x03fe,0x10},
    {0x0a38,0x01},
    {0x0a20,0x19},
    {0x061b,0x17},
    {0x061c,0x50},
    {0x061d,0x05},
    {0x061e,0x5a},
    {0x061f,0x05},
    {0x0a21,0x28},
    {0x0a30,0x00},
    {0x0a31,0x7e},
    {0x0a34,0x40},
    {0x0a35,0x08},
    {0x0a37,0x43},
    {0x0314,0x50},
    {0x0315,0x00},
    {0x031c,0xce},
    {0x0219,0x47},
    {0x0342,0x06},
    {0x0343,0x40},
    {0x0259,0x08},
    {0x025a,0x96},
    {0x0340,0x08},
    {0x0341,0xca},
    {0x0351,0x00},
    {0x0345,0x02},
    {0x0347,0x02},
    {0x0348,0x0f},
    {0x0349,0x18},
    {0x034a,0x08},
    {0x034b,0x88},
    {0x034f,0xf0},
    {0x0094,0x07},
    {0x0095,0x80},
    {0x0096,0x04},
    {0x0097,0x38},
    {0x0098,0x02},
    {0x0099,0x1c},
    {0x009a,0x03},
    {0x009b,0xc0},
    {0x060c,0x06},
    {0x060e,0x20},
    {0x060f,0x0f},
    {0x070c,0x06},
    {0x070e,0x20},
    {0x070f,0x0f},
    {0x0087,0x50},
    {0x141b,0x03},
    {0x0907,0xd5},
    {0x0909,0x06},
    {0x0901,0x0e},
    {0x0902,0x0b},
    {0x0904,0x08},
    {0x0908,0x09},
    {0x0903,0xc5},
    {0x090c,0x09},
    {0x0905,0x10},
    {0x0906,0x00},
    {0x072a,0x7c},
    {0x0724,0x2b},
    {0x0727,0x2b},
    {0x072b,0x1c},
    {0x073e,0x40},
    {0x0078,0x88},
    {0x0618,0x01},
    {0x1466,0x12},
    {0x1468,0x07},
    {0x1467,0x07},
    {0x0709,0x40},
    {0x0719,0x40},
    {0x1469,0x80},
    {0x146a,0xc0},
    {0x146b,0x03},
    {0x1480,0x02},
    {0x1481,0x80},
    {0x1484,0x08},
    {0x1485,0xc0},
    {0x1430,0x80},
    {0x1407,0x10},
    {0x1408,0x16},
    {0x1409,0x03},
    {0x1434,0x04},
    {0x1447,0x75},
    {0x1470,0x10},
    {0x1471,0x13},
    {0x1438,0x00},
    {0x143a,0x00},
    {0x0220,0x80},
    {0x024b,0x02},
    {0x0245,0xc7},
    {0x025b,0x07},
    {0x02bb,0x77},
    {0x0612,0x01},
    {0x0613,0x26},
    {0x0243,0x66},
    {0x0087,0x53},
    {0x0053,0x05},
    {0x0089,0x02},
    {0x0002,0xeb},
    {0x005a,0x0c},
    {0x0040,0x83},
    {0x0075,0x68},
    {0x0205,0x0c},
    {0x0202,0x01},
    {0x0203,0x27},
    {0x061a,0x02},
    {0x03fe,0x00},
    {0x0106,0x78},
    {0x0136,0x00},
    {0x0181,0xf0},
    {0x0185,0x01},
    {0x0180,0x46},
    {0x0106,0x38},
    {0x010d,0x60},
    {0x010e,0x09},
    {0x0113,0x02},
    {0x0114,0x01},
    {0x0115,0x12},
    {0x0122,0x0b},
    {0x0123,0x30},
    {0x0124,0x04},
    {0x0125,0x20},
    {0x0126,0x09},
    {0x0127,0x15},
    {0x0128,0xa8},
    {0x0129,0x0b},
    {0x012a,0x10},
    {0x012b,0x0a},
    {0x0004,0x0f},
    {0x0219,0x47},
    {0x0054,0x98},
    {0x0076,0x01},
    {0x0052,0x02},
    {0x021a,0x10},
    {0x0430,0x10},
    {0x0431,0x10},
    {0x0432,0x10},
    {0x0433,0x10},
    {0x0434,0x6d},
    {0x0435,0x6d},
    {0x0436,0x6d},
    {0x0437,0x6d},
    {0x0704,0x03},
    {0x0706,0x02},
    {0x0716,0x02},
    {0x0708,0xc8},
    {0x0718,0xc8},
    {0x071d,0xdc},
    {0x071e,0x05},
    //otp autoload
    {0x031f,0x01},
    {0x031f,0x00},
    {0x0a67,0x80},
    {0x0a54,0x0e},
    {0x0a65,0x10},
    {0x0a98,0x04},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0089,0x02},
    {0x0aa0,0x00},
    {0x0023,0x00},
    {0x0022,0x00},
    {0x0025,0x00},
    {0x0024,0x00},
    {0x0028,0x0f},
    {0x0029,0x18},
    {0x002a,0x08},
    {0x002b,0x88},
    {0x0317,0x1c},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0xe0},
    {0x0a71,0x00},
    {0x0a72,0x02},
    {0x0a73,0x60},
    {0x0a75,0x41},
    {0x0a70,0x03},
    {0x0a5a,0x80},
    //sleep 20
    {0x0089,0x02},
    {0x05be,0x01},
    {0x0a70,0x00},
    {0x0080,0x02},
    {0x0a67,0x00},
    {0x0100,0x09},
    {0x0058,0x00},
    {0x0059,0x04},
#endif
};

static avdk_err_t gc8613ya_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    // bk_camera_bus_t *bus = csi_sensor->config.bus;
    // int size = GC8613YA_TABLE_SIZE(sensor_gc8613ya_init_table);

    bk_mipi_csi_ext_set_enable(0);

    bk_mipi_csi_enable_debug_pin();
    // for (int i = 0; i < size; i++)
    // {
    //     bus->write16(bus, sensor_gc8613ya_init_table[i][0], sensor_gc8613ya_init_table[i][1]);
    //     if (sensor_gc8613ya_init_table[i][0] == 0x0a5a) {
    //         rtos_delay_milliseconds(20);
    //     }
    // }

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static avdk_err_t gc8613ya_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    int size = 0;
    if (width == 1920 && height == 1080) {
        size = GC8613YA_TABLE_SIZE(sensor_gc8613ya_init_table_1080p_20fps);
        for (int i = 0; i < size; i++) {
            bus->write16(bus, sensor_gc8613ya_init_table_1080p_20fps[i][0], sensor_gc8613ya_init_table_1080p_20fps[i][1]);
            if (sensor_gc8613ya_init_table_1080p_20fps[i][0] == 0x0a5a) {
                rtos_delay_milliseconds(20);
            }
        }
    } else if(width == 3840 && height == 2160) {
        size = GC8613YA_TABLE_SIZE(sensor_gc8613ya_init_table);
        for (int i = 0; i < size; i++) {
            bus->write16(bus, sensor_gc8613ya_init_table[i][0], sensor_gc8613ya_init_table[i][1]);
            if (sensor_gc8613ya_init_table[i][0] == 0x0a5a) {
                rtos_delay_milliseconds(20);
            }
        }
    } else if(width == 1920 && height == 2160) {

    } else {
        LOGE("not supported width: %d, height: %d\r\n", width, height);
        return -1;
    }

    bk_mipi_csi_controller_init(width, height, 0x2b, 2);

    return 0;
}

static avdk_err_t gc8613ya_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    // bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (fps > 30 || fps <= 0)
    {
        LOGE("not supported fps\r\n");
        return -1;
    }


    if (FPS_CRTL_METHOD == FPS_CTRL_BY_LENGTH)
    {

        //changing framelen changes inter frame time, to do
    }

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_EXP)
    {
        
        //define by vb min , to do
    }

    return 0;
}

static avdk_err_t gc8613ya_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    ISP_PORT IspPort = {0};
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    if (format->width == 3840 && format->height == 2160){
        gc8613ya_set_ppi(controller, format->width, format->height);
        gc8613ya_set_fps(controller, format->fps);
    }else if(format->width == 1920 && format->height == 1080){
        gc8613ya_set_ppi(controller, format->width, format->height);
        gc8613ya_set_fps(controller, format->fps);
    }else{
        GC8613YA_SetStream(IspPort, 0);
    }

    bk_mipi_csi_controller_reset();
    return AVDK_ERR_OK;
}

static avdk_err_t gc8613ya_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg read
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("gc8613ya {%04x, %02x}\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("gc8613ya {%04x, %02x} -> {%04x, %02x}\n", addr, dump_val, addr, val);
        bus->write16(bus, addr, val);
    }

    if (cmd == 2) // standy, stream stop
    {

        bus->write16(bus,0x0087,0x51);
        bus->write16(bus,0x0115,0x30);
        bus->write16(bus,0x0181,0x00);
        bus->write16(bus,0x0185,0x00);
        bus->write16(bus,0x0180,0x56);
        bus->write16(bus,0x0245,0x47);
        bus->write16(bus,0x0618,0x00);
        bus->write16(bus,0x0180,0x06);
        bus->write16(bus,0x0a34,0x00);
        bus->write16(bus,0x0a20,0x10);
        bus->write16(bus,0x061b,0x10);
        bus->write16(bus,0x061c,0x10);
        bus->write16(bus,0x031c,0x8d);
        bus->write16(bus,0x0a38,0x00);
    }

    if (cmd == 3) // resume from standy, stream start
    {
        //todo
    }

    return 0;
}

static const csi_sensor_config_t csi_sensor_gc8613ya =
{
    .name = "gc8613ya",
    .clk = MCLK_24M,
    .mipi_data_type = 0x2b,
    .default_width = 3840,
    .default_height = 2160,
    .default_fps = 15,
    .id = 0x8613,
    .address = (GC8613YA_WRITE_ADDRESS >> 1),
    .init = gc8613ya_init,
    .detect = gc8613ya_detect,
    .set_ppi = gc8613ya_set_ppi,
    .set_fps = gc8613ya_set_fps,
    .reg_ctrl = gc8613ya_ctrl,
    // .power_down = gc2145_reset,
};

static const ISP_PUB_ATTR_S gc8613ya_mipi_linear_attr = {
    .pSnsObj      = (void*)&snsGC8613YAObj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB10,
    .snsFps      = 15 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t gc8613ya_format_array[] = {
    {
        .width = 3840,
        .height = 2160,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1920,
        .height = 2160,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps =15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
};

static avdk_err_t gc8613ya_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &gc8613ya_format_array[0];
    format_array->size = ARRAY_SIZE(gc8613ya_format_array);
    return AVDK_ERR_OK;
}

static void *gc8613ya_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsGC8613YAObj;
}

static void *gc8613ya_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

avdk_err_t gc8613ya_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, mb_id = 0, lb_id = 0;
    config->bus->write_address = GC8613YA_WRITE_ADDRESS;

    AVDK_RETURN_ON_FALSE(config->bus, AVDK_ERR_GENERIC, TAG, "bus is NULL");

    LOGI("%s, rest_pin: %d, pwdn_pin: %d\n", __func__, config->pin_reset, config->pin_pwdn);
    /* enable camera power */
    if (config->pin_pwdn != 0xFF)
    {
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_pwdn));
        bk_gpio_set_capacity(config->pin_pwdn, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_high(config->pin_pwdn);
        rtos_delay_milliseconds(10);
    }

    if (config->pin_reset != 0xFF)
    {
        BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_reset));
        bk_gpio_set_capacity(config->pin_reset, GPIO_DRIVER_CAPACITY_3);
        bk_gpio_set_output_high(config->pin_reset);
        rtos_delay_milliseconds(10);
    }

    avdk_err_t ret_hb = config->bus->read16(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    avdk_err_t ret_mb = config->bus->read16(config->bus, CHIP_ID_ADDR_MB, &mb_id);
    avdk_err_t ret_lb = config->bus->read16(config->bus, CHIP_ID_ADDR_LB, &lb_id);
    LOGI("gc8613ya_detect id: 0x%02X%02X%02X (expect 0x%02X%02X%02X), ret %d/%d/%d, addr 0x%02X, scl %d sda %d i2c %d timeout %u\n",
         hb_id, mb_id, lb_id, CHIP_ID_VAL_HB, CHIP_ID_VAL_MB, CHIP_ID_VAL_LB,
         ret_hb, ret_mb, ret_lb, config->bus->write_address,
         config->bus->pin_scl, config->bus->pin_sda, config->bus->i2c_id,
         config->bus->timeout_ms);

    if (hb_id != CHIP_ID_VAL_HB
        || lb_id != CHIP_ID_VAL_LB || mb_id != CHIP_ID_VAL_MB)
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

    LOGI("%s success id: 0x%02X%02X\n", __func__, hb_id, lb_id);

    bk_camera_csi_sensor_t *csi_sensor = os_malloc(sizeof(bk_camera_csi_sensor_t));
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(csi_sensor, 0, sizeof(bk_camera_csi_sensor_t));
    config->bus->write_address = GC8613YA_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = gc8613ya_init;
    csi_sensor->ops.set_format = gc8613ya_set_format;
    csi_sensor->ops.reg_ctrl = gc8613ya_ctrl;
    csi_sensor->ops.get_sensor_object = gc8613ya_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = gc8613ya_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = gc8613ya_query_support_formats;

    csi_sensor->isp_pub_attr = &gc8613ya_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_gc8613ya;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}


BK_CAMERA_SENSOR_DETECT_SECTION(gc8613ya_detect, CSI_CAMERA_PORT);