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

#define GC2053_WRITE_ADDRESS (0x6e)
#define GC2053_READ_ADDRESS (0x6f)
#define CHIP_ID_ADDR_HB (0xF0)
#define CHIP_ID_ADDR_LB (0xF1)
#define CHIP_ID_VAL_HB (0x20)
#define CHIP_ID_VAL_LB (0x53)

#define FPS_CTRL_BY_EXP 0 ////controled by exp time
#define FPS_CTRL_BY_LENGTH 1 //controled by framelen and linelen
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH

#define DEFAULT_FRAME_LEN 1125
#define DEFAULT_LINE_LEN 1333
#define GC2053_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 30)

#define WIN_MAX_X 1928
#define WIN_MAX_Y 1088

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define TAG "gc2053"

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
#include "gc2053_1080p_calib.h"

#define GC2053_REG_BYTE_NUM  1
#define GC2053_DATA_BYTE_NUM 1

#define GC2053_EXPTIME_H		0x03
#define GC2053_EXPTIME_L		0x04
#define GC2053_DGAIN_1			0xB1
#define GC2053_DGAIN_2			0xB2
#define GC2053_AGAIN_1			0xB3
#define GC2053_AGAIN_2			0xB4
#define GC2053_AGAIN_3			0xB8
#define GC2053_AGAIN_4			0xB9

typedef struct
{
    uint8_t val1;
    uint8_t val2;
    uint8_t val3;
    uint8_t val4;
} gc2053_again_val_t;

const uint8_t regValTable[29][4] = {
    //0xb4  0xb3 0xb8 0xb9
    {0x00, 0x00,0x01,0x00},
    {0x00, 0x10,0x01,0x0c},
    {0x00, 0x20,0x01,0x1b},
    {0x00, 0x30,0x01,0x2c},
    {0x00, 0x40,0x01,0x3f},

    {0x00, 0x50,0x02,0x16},
    {0x00, 0x60,0x02,0x35},
    {0x00, 0x70,0x03,0x16},
    {0x00, 0x80,0x04,0x02},
    {0x00, 0x90,0x04,0x31},

    {0x00, 0xa0,0x05,0x32},
    {0x00, 0xb0,0x06,0x35},
    {0x00, 0xc0,0x08,0x04},
    {0x00, 0x5a,0x09,0x19},
    {0x00, 0x83,0x0b,0x0f},

    {0x00, 0x93,0x0d,0x12},
    {0x00, 0x84,0x10,0x00},
    {0x00, 0x94,0x12,0x3a},
    {0x01, 0x2c,0x1a,0x02},
    {0x01, 0x3c,0x1b,0x20},

    {0x00, 0x8c,0x20,0x0f},
    {0x00, 0x9c,0x26,0x07},

    {0x02, 0x64,0x36,0x21},
    {0x02, 0x74,0x37,0x3a},
    {0x00, 0xc6,0x3d,0x02},
    {0x00, 0xdc,0x3f,0x3f},
    {0x02, 0x85,0x3f,0x3f},
    {0x02, 0x95,0x3f,0x3f},
    {0x00, 0xce,0x3f,0x3f},
};

#define AGAIN_TOTAL_LEVEL 30
UINT32 gainLevelTable[AGAIN_TOTAL_LEVEL] = {
    64,
    74,
    89,
    102,
    127,
    147,
    177,
    203,
    260,
    300,
    361,
    415,
    504,
    581,
    722,
    832,
    1027,
    1182,
    1408,
    1621,
    1990,
    2291,
    2850,
    3282,
    4048,
    5180,
    5500,
    6744,
    7073,
    0xffffffff,
};

enum GC2053_REG_INDEX {
    REG_EXPTIME_H       = 0,
    REG_EXPTIME_L       = 1,
    REG_DGAIN_1         = 2,
    REG_DGAIN_2         = 3,
    REG_AGAIN_1         = 4,
    REG_AGAIN_2         = 5,
    REG_AGAIN_3         = 6,
    REG_AGAIN_4         = 7,
};

#define GC2053_720P_30FPS_LINEAR_MODE (0)

#define GC2053_VMAX_720P30_LINEAR (1125)

typedef struct vsiGC2053_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} GC2053_DEVICE_S;

static GC2053_DEVICE_S *GC2053Dev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * GC2053_720P_CalibParam_dynamic = NULL;

static GC2053_DEVICE_S *GC2053_GetSensorDev(ISP_PORT IspPort)
{
    if (GC2053Dev[IspPort.devId][IspPort.portId] == NULL)
    {
        GC2053Dev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(GC2053_DEVICE_S));
        if (GC2053Dev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d GC2053Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(GC2053Dev[IspPort.devId][IspPort.portId], 0 , sizeof(GC2053_DEVICE_S));
    }

    return GC2053Dev[IspPort.devId][IspPort.portId];
}

static int GC2053_SetStream(ISP_PORT IspPort, vsi_bool_t stream);


static int GC2053_InitRegInfo(ISP_PORT IspPort)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC2053Dev->snsRegsInfo;
    pSnsRegsInfo->snsDev = pGC2053Dev->i2cBus;

    pSnsRegsInfo->addrByteNum = pGC2053Dev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = pGC2053Dev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = pGC2053Dev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 8;
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_EXPTIME_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_H].regAddr = GC2053_EXPTIME_H;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].regAddr = GC2053_EXPTIME_L;

    pSnsRegsInfo->snsData[REG_DGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DGAIN_1].regAddr = GC2053_DGAIN_1;
    pSnsRegsInfo->snsData[REG_DGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DGAIN_2].regAddr = GC2053_DGAIN_2;

    pSnsRegsInfo->snsData[REG_AGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_1].regAddr = GC2053_AGAIN_1;
    pSnsRegsInfo->snsData[REG_AGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_2].regAddr = GC2053_AGAIN_2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].regAddr = GC2053_AGAIN_3;
    pSnsRegsInfo->snsData[REG_AGAIN_4].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_4].regAddr = GC2053_AGAIN_4;

    return BK_OK;
}

static int GC2053_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (GC2053_720P_CalibParam_dynamic == NULL)
    {
        GC2053_720P_CalibParam_dynamic = os_malloc(sizeof(GC2053_720P_CalibParam));
        if (GC2053_720P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc GC2053_720P_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(GC2053_720P_CalibParam_dynamic, &GC2053_720P_CalibParam, sizeof(GC2053_720P_CalibParam));
    }

    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(pGC2053Dev, 0, sizeof(*pGC2053Dev));
    pGC2053Dev->i2cBus              = snsDev;
    pGC2053Dev->i2cAttr.slave_addr  = 0x6E;
    pGC2053Dev->i2cAttr.reg_bytes   = GC2053_REG_BYTE_NUM;
    pGC2053Dev->i2cAttr.data_bytes  = GC2053_DATA_BYTE_NUM;
    GC2053_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return BK_FAIL;
    }

    GC2053_SetStream(IspPort, 0);

    return  BK_OK;
}

static int GC2053_Exit(ISP_PORT IspPort)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);

    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d already exit\n", __func__, __LINE__);
        return BK_OK;
    }

    vsios_i2c_sys_exit(pGC2053Dev->i2cBus);

    if (GC2053_720P_CalibParam_dynamic != NULL)
    {
        os_free(GC2053_720P_CalibParam_dynamic);
        GC2053_720P_CalibParam_dynamic = NULL;
    }

    os_free(pGC2053Dev);
    GC2053Dev[IspPort.devId][IspPort.portId] = NULL;
    return BK_OK;
}

static int GC2053_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC2053Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC2053Dev->i2cAttr;

    // LOGD("i2c write (%x, %x) \r\n", addr, data);
    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);
    // vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int GC2053_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC2053Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC2053Dev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int GC2053_InitAeDefault(ISP_PORT IspPort)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC2053Dev->aeDefault;

    switch (pGC2053Dev->snsModeId) {
        case GC2053_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = GC2053_VMAX_720P30_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 20 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 2;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 832 * 1024;
            pAeSnsDft->minAgain  = 64 * 1024;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 48;
            pAeSnsDft->dampOver = 0x40;
            pAeSnsDft->dampUnder = 0x40;
            pAeSnsDft->tolerance = 1;
            pAeSnsDft->initExposure = 0x100 * pAeSnsDft->minAgain;
            break;
        default:
            break;
    }

    return BK_OK;
}

static int GC2053_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if ((pSnsMode->width == pGC2053Dev->snsMode.width) &&
        (pSnsMode->height == pGC2053Dev->snsMode.height) &&
        (pSnsMode->hdrMode == pGC2053Dev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == pGC2053Dev->snsMode.stichMode)) {
        return BK_OK;
    }

    if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC2053_Linear1920x1080Init(pGC2053Dev->i2cBus, &pGC2053Dev->i2cAttr);
        os_memcpy(&pGC2053Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC2053Dev->snsModeId = GC2053_720P_30FPS_LINEAR_MODE;
        GC2053_InitAeDefault(IspPort);
    }
    else if ((pSnsMode->width  == 640) &&
        (pSnsMode->height == 480) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC2053_Linear1920x1080Init(pGC2053Dev->i2cBus, &pGC2053Dev->i2cAttr);
        os_memcpy(&pGC2053Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC2053Dev->snsModeId = GC2053_720P_30FPS_LINEAR_MODE;
        GC2053_InitAeDefault(IspPort);
    } else {
        LOGI("gc2053 custom set mode end ~~~\n");
        os_memcpy(&pGC2053Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC2053Dev->snsModeId = GC2053_720P_30FPS_LINEAR_MODE;
        GC2053_InitAeDefault(IspPort);
    }

    return BK_OK;
}

static int GC2053_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        // GC2053_WriteReg(IspPort, 0x3253, 0x00);
        // GC2053_WriteReg(IspPort, 0x3012, 1);
    } else {
        // GC2053_WriteReg(IspPort, 0x3012, 0);
    }

    pGC2053Dev->stream = stream;

    return BK_OK;
}

static int GC2053_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, GC2053_720P_CalibParam_dynamic);

    return BK_OK;
}

static int GC2053_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = GC2053_Init;
    pIspSnsFunc->pfnSensorExit    = GC2053_Exit;
    pIspSnsFunc->pfnWriteReg      = GC2053_WriteReg;
    pIspSnsFunc->pfnReadReg       = GC2053_ReadReg;
    pIspSnsFunc->pfnSetMode       = GC2053_SetMode;
    pIspSnsFunc->pfnSetStream     = GC2053_SetStream;
    pIspSnsFunc->pfnSetIspDefault = GC2053_SetIspDefault;

    return BK_OK;
}

static int GC2053_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &pGC2053Dev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

#if 0
static int GC2053_SetFps(ISP_PORT IspPort, vsi_u32_t fps)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC2053Dev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC2053Dev->snsRegsInfo;
    vsi_u32_t vts;

    switch(pGC2053Dev->snsModeId) {
        case GC2053_720P_30FPS_LINEAR_MODE:
            if ((fps <= 30 * ISP_SNS_FPS_ACCU) && (fps >= 0.5 * ISP_SNS_FPS_ACCU)) {
                vts = GC2053_VMAX_720P30_LINEAR * 30 * ISP_SNS_FPS_ACCU / fps;
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

static int GC2053_SlowFrameRate(ISP_PORT IspPort, vsi_u32_t fullLines)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC2053Dev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC2053Dev->snsRegsInfo;

    fullLines = (fullLines > pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : fullLines;
    pAeSnsDft->fullLines = fullLines;

    pSnsRegsInfo->snsData[REG_VTS_H].data = ((fullLines & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_VTS_L].data = (fullLines & 0xFF);

    switch(pGC2053Dev->snsModeId) {
        case GC2053_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->maxIntLine = pAeSnsDft->fullLines - 2;
            break;
        default:
            break;
    }

    return BK_OK;
}

static int GC2053_SetExpRatio(ISP_PORT IspPort, ISP_EXP_RATIO_S *pExpRatio)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC2053Dev->aeDefault;

    os_memcpy(&pAeSnsDft->expRatio, pExpRatio, sizeof(*pExpRatio));

    return BK_OK;
}
#endif

static void GC2053_CalcGain(uint32_t *pGain, gc2053_again_val_t *again_val, uint16_t *pDGainReg, uint8_t *pConvReg)
{
    uint32_t ae_gain = *pGain;
    int i;

    /* Find proper analog gain level according to requested gain */
    for (i = 0; i < AGAIN_TOTAL_LEVEL - 1; i++)
    {
        if ((gainLevelTable[i] <= ae_gain) && (ae_gain < gainLevelTable[i + 1]))
            break;
    }
    if (i >= AGAIN_TOTAL_LEVEL - 1)
    {
        i = AGAIN_TOTAL_LEVEL - 2;
    }

    /* Quantize gain to table level for reporting back to AE */
    *pGain = gainLevelTable[i];

    /* Map level index to sensor analog gain registers */
    again_val->val1 = regValTable[i][0];
    again_val->val2 = regValTable[i][1];
    again_val->val3 = regValTable[i][2];
    again_val->val4 = regValTable[i][3];

    /* For now keep digital gain at 1x and convReg unused */
    if (pDGainReg)
        *pDGainReg = 1024;
    if (pConvReg)
        *pConvReg = 0;
}

static uint8_t fix_level = 0;
void set_again_fix_level(uint8_t level)
{
	fix_level = level;
}

static int GC2053_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC2053Dev->snsRegsInfo;
    uint32_t gain;
    gc2053_again_val_t reg_val;
    uint16_t dGainReg;
    uint8_t convReg;

    switch(pGC2053Dev->snsModeId) {
        case GC2053_720P_30FPS_LINEAR_MODE:
            gain = (uint32_t)(*pAgain)/1024;
            GC2053_CalcGain(&gain, &reg_val, &dGainReg, &convReg);
            *pAgain = (vsi_u32_t)gain * 1024;
            *pDgain = 1024;

            /* Program analog gain registers according to calculated level */
            pSnsRegsInfo->snsData[REG_AGAIN_2].data = reg_val.val1;
            pSnsRegsInfo->snsData[REG_AGAIN_1].data = reg_val.val2;
            pSnsRegsInfo->snsData[REG_AGAIN_3].data = reg_val.val3;
            pSnsRegsInfo->snsData[REG_AGAIN_4].data = reg_val.val4;

            /* Program digital gain registers from *pDgain (1x by default) */
            pSnsRegsInfo->snsData[REG_DGAIN_1].data = (uint8_t)((*pDgain >> 8) & 0xFF);
            pSnsRegsInfo->snsData[REG_DGAIN_2].data = (uint8_t)(*pDgain & 0xFF);
            break;
        default:
            break;
    }

    //LOGI("%s: again = %u, dgain = %u\n", __func__, (unsigned int)(*pAgain), (unsigned int)(*pDgain));

    return BK_OK;
}

static int GC2053_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC2053Dev->snsRegsInfo;

    // uint32_t new_lines = (((*pIntLine) - 1)/10 + 1) * 10;

    switch(pGC2053Dev->snsModeId) {
        case GC2053_720P_30FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPTIME_H].data = ((*pIntLine & 0xFF00) >> 8);
            pSnsRegsInfo->snsData[REG_EXPTIME_L].data = (*pIntLine & 0xFF);
            break;
        default:
            break;
    }

    //LOGI("%s: intTime(line) = %u\n", __func__, (unsigned int)(*pIntLine));

    return BK_OK;
}

static int GC2053_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    GC2053_DEVICE_S *pGC2053Dev = GC2053_GetSensorDev(IspPort);
    if (pGC2053Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &pGC2053Dev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int GC2053_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = GC2053_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = GC2053_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = GC2053_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = GC2053_GetSnsRegInfo;

    return BK_OK;
}

const ISP_SNS_OBJ_S snsGC2053Obj = {
    .pfnInitIspSnsFunc = GC2053_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = GC2053_InitAeSnsFunc,
};

#endif
//#####################################################################################################

#define GC2053_TABLE_SIZE(table) (sizeof(table) / 2)

avdk_err_t gc2053_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

static const uint8_t sensor_gc2053_init_table[][2] =
{
    { 0xfe, 0x80},
    { 0xfe, 0x80},
    { 0xfe, 0x80},
    { 0xfe, 0x00},
    { 0xf2, 0x00},
    { 0xf3, 0x00},
    { 0xf4, 0x36},
    { 0xf5, 0xc0},
    { 0xf6, 0x44}, //
    { 0xf7, 0x01}, //
    { 0xf8, 0x78}, //63
    { 0xf9, 0x40}, //
    { 0xfc, 0x8e},
     /****CISCTL & ANALOG****/
    { 0xfe, 0x00},
    { 0x87, 0x18},
    { 0xee, 0x30},
    { 0xd0, 0xb7},
    { 0x03, 0x02},
    { 0x04, 0x00},
    { 0x05, 0x05},
    { 0x06, 0x35}, //hb=1333
    { 0x07, 0x00},
    { 0x08, 0x11}, //vb=17
    { 0x09, 0x00},
    { 0x0a, 0x02},
    { 0x0b, 0x00},
    { 0x0c, 0x02},

    { 0x0d, UINT16_HB(WIN_MAX_Y)},
    { 0x0e, UINT16_LB(WIN_MAX_Y)}, //1088
    { 0x0f, UINT16_HB(WIN_MAX_X)},
    { 0x10, UINT16_LB(WIN_MAX_X)}, //1928

    { 0x12, 0xe2},
    { 0x13, 0x16},
    { 0x17, 0x01},
    { 0x19, 0x0a},
    { 0x21, 0x1c},
    { 0x28, 0x0a},
    { 0x29, 0x24},
    { 0x2b, 0x04},
    { 0x32, 0xf8},
    { 0x37, 0x03},
    { 0x39, 0x15},
    { 0x43, 0x07},
    { 0x44, 0x40},
    { 0x46, 0x0b},
    { 0x4b, 0x20},
    { 0x4e, 0x08},
    { 0x55, 0x20},
    { 0x66, 0x05},
    { 0x67, 0x05},
    { 0x77, 0x01},
    { 0x78, 0x00},
    { 0x7c, 0x93},
    { 0x8c, 0x12},

    { 0x8d, 0x92},
    { 0x90, 0x00},
    { 0x41, 0x04},
    { 0x42, 0x65}, //vts=1125
    { 0x9d, 0x10}, //min vb=16

    { 0xce, 0x7c},
    { 0xd2, 0x41},
    { 0xd3, 0xdc},
    { 0xe6, 0x50},
     /*gain*/
    { 0xb6, 0xc0},
    { 0xb0, 0x70},
    { 0xb1, 0x01},
    { 0xb2, 0x00},
    { 0xb3, 0x00},
    { 0xb4, 0x00},
    { 0xb8, 0x01},
    { 0xb9, 0x00},
     /*blk*/
    { 0x26, 0x30},
    { 0xfe, 0x01},
    { 0x40, 0x23},
    { 0x55, 0x07},
    { 0x60, 0x40},
    { 0xfe, 0x04},
    { 0x14, 0x78},
    { 0x15, 0x78},
    { 0x16, 0x78},
    { 0x17, 0x78},
     /*window*/
    { 0xfe, 0x01},
    { 0x92, 0x00},
    { 0x94, 0x03},
    { 0x95, 0x04},
    { 0x96, 0x40},  //1088
    { 0x97, 0x07},
    { 0x98, 0x80},  //1920
     /*ISP*/
    { 0xfe, 0x01},
    { 0x01, 0x05},
    { 0x02, 0x89},
    { 0x04, 0x01},
    { 0x07, 0xa6},
    { 0x08, 0xa9},
    { 0x09, 0xa8},
    { 0x0a, 0xa7},
    { 0x0b, 0xff},
    { 0x0c, 0xff},
    { 0x0f, 0x00},
    { 0x50, 0x1c},
    { 0x89, 0x03},
    { 0xfe, 0x04},
    { 0x28, 0x86},
    { 0x29, 0x86},
    { 0x2a, 0x86},
    { 0x2b, 0x68},
    { 0x2c, 0x68},
    { 0x2d, 0x68},
    { 0x2e, 0x68},
    { 0x2f, 0x68},
    { 0x30, 0x4f},
    { 0x31, 0x68},
    { 0x32, 0x67},
    { 0x33, 0x66},
    { 0x34, 0x66},
    { 0x35, 0x66},
    { 0x36, 0x66},
    { 0x37, 0x66},
    { 0x38, 0x62},
    { 0x39, 0x62},
    { 0x3a, 0x62},
    { 0x3b, 0x62},
    { 0x3c, 0x62},
    { 0x3d, 0x62},
    { 0x3e, 0x62},
    { 0x3f, 0x62},
     /****DVP & MIPI****/
    { 0xfe, 0x01},
    { 0x9a, 0x06},
    { 0xfe, 0x00},
    { 0x7b, 0x2a},
    { 0x23, 0x2d},
    { 0xfe, 0x03},
    { 0x01, 0x27},
    { 0x02, 0x56},
    { 0x03, 0xb6},
    { 0x12, 0x80},
    { 0x13, 0x07},
    { 0x15, 0x10},
    { 0xfe, 0x00},
    { 0x3e, 0x91},
};

static avdk_err_t gc2053_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bk_mipi_csi_ext_set_enable(0);
    //bk_mipi_csi_enable_debug_pin();

    uint32_t size = GC2053_TABLE_SIZE(sensor_gc2053_init_table);

    for (int i = 0; i < size; i++)
    {
        bus->write8(bus, sensor_gc2053_init_table[i][0], sensor_gc2053_init_table[i][1]);
    }

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static avdk_err_t gc2053_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (width > 1928 || width <= 0 || height > 1088 || height <= 0)
    {
        LOGE("%s, %d width: %d, height: %d, fail...\n", __func__, __LINE__, width, height);
        return -1;
    }

    bk_mipi_csi_controller_init(width, height, 0x2b);

    bus->write8(bus, 0xFE, 0x1);
    uint16_t win_y_start = (WIN_MAX_Y - height) / 2;
    uint16_t win_x_start = (WIN_MAX_X - width) / 2;
    win_y_start = win_y_start - (win_y_start % 4);
    win_x_start = (win_x_start < 3) ? 3 : win_x_start;
    win_x_start = win_x_start - ((win_x_start - 3) % 4);


    bus->write8(bus, 0x91, UINT16_HB(win_y_start));
    bus->write8(bus, 0x92, UINT16_LB(win_y_start));
    bus->write8(bus, 0x93, UINT16_HB(win_x_start));
    bus->write8(bus, 0x94, UINT16_LB(win_x_start));
    bus->write8(bus, 0x95, UINT16_HB(height));
    bus->write8(bus, 0x96, UINT16_LB(height));
    bus->write8(bus, 0x97, UINT16_HB(width));
    bus->write8(bus, 0x98, UINT16_LB(width));

    bus->write8(bus, 0xFE, 0x3);
    uint16_t lwc_set = width * 5 / 4 / 2 * 2; //width*5/4, then align to 2
    bus->write8(bus, 0x13, UINT16_HB(lwc_set));
    bus->write8(bus, 0x12, UINT16_LB(lwc_set));
    return 0;
}

static avdk_err_t gc2053_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (fps > 30 || fps <= 0)
    {
        LOGE("not supported fps\r\n");
        return -1;
    }

    bus->write8(bus, 0xFE, 0x0);

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_LENGTH)
    {
        bus->write8(bus, 0x8d, 0x92);
        bus->write8(bus, 0x90, 0x00);
        uint16_t line_len = GC2053_PCLK / DEFAULT_FRAME_LEN / fps;
        bus->write8(bus, 0x05, UINT16_HB(line_len));
        bus->write8(bus, 0x06, UINT16_LB(line_len));
        //changing framelen changes inter frame time, to do
    }

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_EXP)
    {
        bus->write8(bus, 0x8d, 0x92);
        bus->write8(bus, 0x90, 0x01);
        //define by vb min , to do
    }

    return 0;
}

static avdk_err_t gc2053_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    gc2053_set_ppi(controller, format->width, format->height);
    gc2053_set_fps(controller, format->fps);
    bk_mipi_csi_controller_reset();
    return AVDK_ERR_OK;
}

static avdk_err_t gc2053_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg read
    {
        uint8_t dump_val;
        bus->read8(bus, addr, &dump_val);
        LOGI("gc2053 {%04x, %02x}\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read8(bus, addr, &dump_val);
        LOGI("gc2053 {%04x, %02x} -> {%04x, %02x}\n", addr, dump_val, addr, val);
        bus->write8(bus, addr, val);
    }

    if (cmd == 2) // standy, stream stop
    {
        bus->write8(bus, 0xfe, 0x0);
        bus->write8(bus, 0x3e, 0x1); //0x91 to 0x1
        bus->write8(bus, 0xf7, 0x0); //0x1 to 0x0
        bus->write8(bus, 0xfc, 0x1); //0x8e to 0x1
        bus->write8(bus, 0xf9, 0x41); //0x40 to 0x41
    }

    if (cmd == 3) // resume from standy, stream start
    {
        bus->write8(bus, 0xf9, 0x40); //0x41 to 0x40
        rtos_delay_milliseconds(1);
        bus->write8(bus, 0xf7, 0x1); //0x0 to 0x1
        bus->write8(bus, 0xfc, 0x8e); //0x1 to 0x8e
        bus->write8(bus, 0xfe, 0x0);
        bus->write8(bus, 0xfe, 0x0);
        bus->write8(bus, 0x3e, 0x91); //0x91 to 0x1
    }

    return 0;
}

static const csi_sensor_config_t csi_sensor_gc2053 =
{
    .name = "gc2053",
    .clk = MCLK_24M,
    .mipi_data_type = 0x2b,
    .default_width = 1920,
    .default_height = 1080,
    .default_fps = 30,
    .id = ID_GC2053,
    .address = (GC2053_WRITE_ADDRESS >> 1),
    .init = gc2053_init,
    .detect = gc2053_detect,
    .set_ppi = gc2053_set_ppi,
    .set_fps = gc2053_set_fps,
    .reg_ctrl = gc2053_ctrl,
    // .power_down = gc2145_reset,
};

static const ISP_PUB_ATTR_S gc2053_mipi_linear_attr = {
    .pSnsObj      = (void*)&snsGC2053Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t gc2053_format_array[] = {
    {
        .width = 1280,
        .height = 720,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 25,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 20,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 25,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 20,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },

    {
        .width = 1920,
        .height = 1080,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },

    {
        .width = 640,
        .height = 480,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },

    {
        .width = 1088,
        .height = 1088,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    },

};

static avdk_err_t gc2053_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &gc2053_format_array[0];
    format_array->size = ARRAY_SIZE(gc2053_format_array);
    return AVDK_ERR_OK;
}

static void *gc2053_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsGC2053Obj;
}

static void *gc2053_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

avdk_err_t gc2053_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = GC2053_WRITE_ADDRESS;

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

    config->bus->read8(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read8(config->bus, CHIP_ID_ADDR_LB, &lb_id);

    if (hb_id != CHIP_ID_VAL_HB
        || lb_id != CHIP_ID_VAL_LB)
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
    config->bus->write_address = GC2053_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = gc2053_init;
    csi_sensor->ops.set_format = gc2053_set_format;
    csi_sensor->ops.reg_ctrl = gc2053_ctrl;
    csi_sensor->ops.get_sensor_object = gc2053_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = gc2053_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = gc2053_query_support_formats;

    csi_sensor->isp_pub_attr = &gc2053_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_gc2053;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}


BK_CAMERA_SENSOR_DETECT_SECTION(gc2053_detect, CSI_CAMERA_PORT);
