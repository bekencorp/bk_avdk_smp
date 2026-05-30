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
#include <os/str.h>
#include <components/log.h>
#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <components/bk_camera_sensor.h>
#include <components/bk_isp_camera_types.h>

#include "csi_sensor_devices.h"
#include <driver/mipi_csi.h>
#include <vsi_list.h>
#include <driver/isp_types.h>

//###########################################################################################

#include "vsios_i2c.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_calib.h"
#include "gc4653_1080p_calib.h"
#include <driver/isp_base.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#define LOGTAG "GC4653"

// Use OS abstraction APIs directly to avoid pulling in VeriSilicon OSI headers.
#define TAG "gc4653"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct vsiGC4653_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} GC4653_DEVICE_S;

#define GC4653_REG_BYTE_NUM  2
#define GC4653_DATA_BYTE_NUM 1

#define GC4653_720P_30FPS_LINEAR_MODE (0)
#define GC4653_VMAX_720P30_LINEAR (1500)

#define GC4653_EXPTIME_H		0x202
#define GC4653_EXPTIME_L		0x203
#define GC4653_AGAIN_1			0x2b3
#define GC4653_AGAIN_2			0x2b4
#define GC4653_AGAIN_3			0x2b8
#define GC4653_AGAIN_4			0x2b9
#define GC4653_AGAIN_5			0x515
#define GC4653_AGAIN_6			0x519
#define GC4653_AGAIN_7			0x2d9

#define GC4653_DGAIN_1          0x20e
#define GC4653_DGAIN_2          0x20f

enum GC4653_REG_INDEX {
    REG_EXPTIME_H		= 0,
    REG_EXPTIME_L		= 1,
    REG_AGAIN_1			= 2,
    REG_AGAIN_2			= 3,
    REG_AGAIN_3			= 4,
    REG_AGAIN_4			= 5,
    REG_AGAIN_5			= 6,
    REG_AGAIN_6			= 7,
    REG_AGAIN_7			= 8,

    REG_DGAIN_1			= 9,
    REG_DGAIN_2			= 10,
};

static uint32_t regValTable[21][7] =
{
    //2b3    2b4   2b8     2b9   515     519    2d9
    {0x00,  0x00,  0x01,  0x00,  0x30,  0x28,  0x66},
    {0x20,  0x00,  0x01,  0x0B,  0x30,  0x2a,  0x68},
    {0x01,  0x00,  0x01,  0x19,  0x30,  0x27,  0x65},
    {0x21,  0x00,  0x01,  0x2A,  0x30,  0x29,  0x67},
    {0x02,  0x00,  0x02,  0x00,  0x30,  0x27,  0x65},
    {0x22,  0x00,  0x02,  0x17,  0x30,  0x29,  0x67},
    {0x03,  0x00,  0x02,  0x33,  0x30,  0x28,  0x66},
    {0x23,  0x00,  0x03,  0x14,  0x30,  0x2a,  0x68},
    {0x04,  0x00,  0x04,  0x00,  0x30,  0x2a,  0x68},
    {0x24,  0x00,  0x04,  0x2F,  0x30,  0x2b,  0x69},
    {0x05,  0x00,  0x05,  0x26,  0x30,  0x2c,  0x6A},
    {0x25,  0x00,  0x06,  0x28,  0x30,  0x2e,  0x6C},
    {0x06,  0x00,  0x08,  0x00,  0x30,  0x2f,  0x6D},
    {0x26,  0x00,  0x09,  0x1E,  0x30,  0x31,  0x6F},
    {0x46,  0x00,  0x0B,  0x0C,  0x30,  0x34,  0x72},
    {0x66,  0x00,  0x0D,  0x11,  0x30,  0x37,  0x75},
    {0x0e,  0x00,  0x10,  0x00,  0x30,  0x3a,  0x78},
    {0x2e,  0x00,  0x12,  0x3D,  0x30,  0x3e,  0x7C},
    {0x4e,  0x00,  0x16,  0x19,  0x30,  0x41,  0x7F},
    {0x6e,  0x00,  0x1A,  0x22,  0x30,  0x45,  0x83},
    {0x1e,  0x00,  0x20,  0x00,  0x30,  0x49,  0x87},

};

static uint32_t analog_gain_table[22] =
{
    64,
    75,
    89,
    106,
    128,
    151,
    179,
    212,
    256,
    303,
    358,
    424,
    512,
    606,
    716,
    849,
    1024,
    1213,
    1433,
    1698,
    2048,
    0xffff,
};

static GC4653_DEVICE_S *GC4653Dev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * GC4653_720P_CalibParam_dynamic = NULL;

static GC4653_DEVICE_S *GC4653_GetSensorDev(ISP_PORT IspPort)
{
    if (GC4653Dev[IspPort.devId][IspPort.portId] == NULL)
    {
        GC4653Dev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(GC4653_DEVICE_S));
        if (GC4653Dev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d GC4653Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(GC4653Dev[IspPort.devId][IspPort.portId], 0 , sizeof(GC4653_DEVICE_S));
    }

    return GC4653Dev[IspPort.devId][IspPort.portId];
}

static int GC4653_SetStream(ISP_PORT IspPort, vsi_bool_t stream);


static int GC4653_InitRegInfo(ISP_PORT IspPort)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;
    pSnsRegsInfo->snsDev = pGC4653Dev->i2cBus;

    pSnsRegsInfo->addrByteNum = pGC4653Dev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = pGC4653Dev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = pGC4653Dev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 9;
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_EXPTIME_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_H].regAddr = GC4653_EXPTIME_H;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].regAddr = GC4653_EXPTIME_L;

    pSnsRegsInfo->snsData[REG_AGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_1].regAddr = GC4653_AGAIN_1;   
    pSnsRegsInfo->snsData[REG_AGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_2].regAddr = GC4653_AGAIN_2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].regAddr = GC4653_AGAIN_3;
    pSnsRegsInfo->snsData[REG_AGAIN_4].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_4].regAddr = GC4653_AGAIN_4;
    pSnsRegsInfo->snsData[REG_AGAIN_5].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_5].regAddr = GC4653_AGAIN_5;
    pSnsRegsInfo->snsData[REG_AGAIN_6].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_6].regAddr = GC4653_AGAIN_6;
    pSnsRegsInfo->snsData[REG_AGAIN_7].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_7].regAddr = GC4653_AGAIN_7;

    return BK_OK;
}

static int GC4653_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (GC4653_720P_CalibParam_dynamic == NULL)
    {
        GC4653_720P_CalibParam_dynamic = os_malloc(sizeof(GC4653_720P_CalibParam));
        if (GC4653_720P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc GC4653_720P_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(GC4653_720P_CalibParam_dynamic, &GC4653_720P_CalibParam, sizeof(GC4653_720P_CalibParam));
    }

    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(pGC4653Dev, 0, sizeof(*pGC4653Dev));
    pGC4653Dev->i2cBus              = snsDev;
    pGC4653Dev->i2cAttr.slave_addr  = 0x52;
    pGC4653Dev->i2cAttr.reg_bytes   = GC4653_REG_BYTE_NUM;
    pGC4653Dev->i2cAttr.data_bytes  = GC4653_DATA_BYTE_NUM;
    GC4653_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return ret;
    }

    GC4653_SetStream(IspPort, 0);

    return  BK_OK;
}

static int GC4653_Exit(ISP_PORT IspPort)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsios_i2c_sys_exit(pGC4653Dev->i2cBus);
    if (GC4653_720P_CalibParam_dynamic != NULL)
    {
        os_free(GC4653_720P_CalibParam_dynamic);
        GC4653_720P_CalibParam_dynamic = NULL;
    }
    return  BK_OK;
}

static int GC4653_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC4653Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC4653Dev->i2cAttr;

    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);

    return  BK_OK;
}

static int GC4653_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC4653Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC4653Dev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int GC4653_InitAeDefault(ISP_PORT IspPort)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC4653Dev->aeDefault;

    switch (pGC4653Dev->snsModeId) {
        case GC4653_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = GC4653_VMAX_720P30_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 18 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 2;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 358 * 1024;
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

static int GC4653_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if ((pSnsMode->width == pGC4653Dev->snsMode.width) &&
        (pSnsMode->height == pGC4653Dev->snsMode.height) &&
        (pSnsMode->hdrMode == pGC4653Dev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == pGC4653Dev->snsMode.stichMode)) {
        return BK_OK;
    }

    if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC4653_Linear1920x1080Init(pGC4653Dev->i2cBus, &pGC4653Dev->i2cAttr);
        LOGI("gc4653 1080p set mode end \r\n");
        os_memcpy(&pGC4653Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC4653Dev->snsModeId = GC4653_720P_30FPS_LINEAR_MODE;
        GC4653_InitAeDefault(IspPort);
    }
    else if ((pSnsMode->width  == 640) &&
        (pSnsMode->height == 480) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC4653_Linear1920x1080Init(pGC4653Dev->i2cBus, &pGC4653Dev->i2cAttr);
        os_printf("gc4653 480p set mode end \r\n");
        os_memcpy(&pGC4653Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC4653Dev->snsModeId = GC4653_720P_30FPS_LINEAR_MODE;
        GC4653_InitAeDefault(IspPort);
    } else {
        LOGI("gc4653 custom set mode end ~~~\r\n");
        os_memcpy(&pGC4653Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC4653Dev->snsModeId = GC4653_720P_30FPS_LINEAR_MODE;
        GC4653_InitAeDefault(IspPort);
    }

    return  BK_OK;
}

static int GC4653_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        // GC4653_WriteReg(IspPort, 0x3253, 0x00);
        // GC4653_WriteReg(IspPort, 0x3012, 1);
    } else {
        // GC4653_WriteReg(IspPort, 0x3012, 0);
    }

    pGC4653Dev->stream = stream;

    return  BK_OK;
}

static int GC4653_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, GC4653_720P_CalibParam_dynamic);

    return  BK_OK;
}

static int GC4653_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = GC4653_Init;
    pIspSnsFunc->pfnSensorExit    = GC4653_Exit;
    pIspSnsFunc->pfnWriteReg      = GC4653_WriteReg;
    pIspSnsFunc->pfnReadReg       = GC4653_ReadReg;
    pIspSnsFunc->pfnSetMode       = GC4653_SetMode;
    pIspSnsFunc->pfnSetStream     = GC4653_SetStream;
    pIspSnsFunc->pfnSetIspDefault = GC4653_SetIspDefault;

    return BK_OK;
}

static int GC4653_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &pGC4653Dev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

#if 0
static int GC4653_SetFps(ISP_PORT IspPort, vsi_u32_t fps)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC4653Dev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;
    vsi_u32_t vts;

    switch(pGC4653Dev->snsModeId) {
        case GC4653_720P_30FPS_LINEAR_MODE:
            if ((fps <= 30 * ISP_SNS_FPS_ACCU) && (fps >= 0.5 * ISP_SNS_FPS_ACCU)) {
                vts = GC4653_VMAX_720P30_LINEAR * 30 * ISP_SNS_FPS_ACCU / fps;
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

static int GC4653_SlowFrameRate(ISP_PORT IspPort, vsi_u32_t fullLines)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC4653Dev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;

    fullLines = (fullLines > pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : fullLines;
    pAeSnsDft->fullLines = fullLines;

    pSnsRegsInfo->snsData[REG_VTS_H].data = ((fullLines & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_VTS_L].data = (fullLines & 0xFF);

    switch(pGC4653Dev->snsModeId) {
        case GC4653_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->maxIntLine = pAeSnsDft->fullLines - 2;
            break;
        default:
            break;
    }

    return BK_OK;
    
}

static void GC4653_CalcGain(vsi_u32_t *pGain, vsi_u8_t *pAgainReg, vsi_u16_t *pDGainReg, vsi_u8_t *pConvReg)
{
    vsi_u32_t again;
    vsi_u8_t  convGain;
    if (*pGain < 3072) {
        *pGain = 3072;
    }

    if (*pGain < 4480) {
        again = 2;
        convGain = 1;
    } else if (*pGain < 8960 ) {
        again = 4;
        convGain = 1;
    } else if (*pGain < 22528) {
        again = 8;
        convGain = 1;
    } else if (*pGain < 45056) {
        again = 1;
        convGain = 11;
    } else if (*pGain < 90112) {
        again = 2;
        convGain = 11;
    } else if (*pGain < 180224) {
        again = 4;
        convGain = 11;
    } else {
        again = 8;
        convGain = 11;
    }

    if (convGain == 1) {
        *pConvReg = 0;
    } else {
        *pConvReg = 1;
    }

    switch (again) {
        case 1:
            *pAgainReg = 0;
            break;
        case 2:
            *pAgainReg = 1;
            break;
        case 4:
            *pAgainReg = 2;
            break;
        case 8:
            *pAgainReg = 3;
            break;
        default:
            break;
    }
    /* 1024 / 256 = 4, isp gain accu 1024, sensor dgain reg accu 256*/
    *pDGainReg = (*pGain)  / (again * convGain * 4);
    *pGain = again * convGain * (*pDGainReg) * 4;

    return 0;
}

static int GC4653_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;
    vsi_u32_t gain;
    vsi_u8_t againReg;
    vsi_u16_t dGainReg;
    vsi_u8_t convReg;

    switch(pGC4653Dev->snsModeId) {
        case GC4653_720P_30FPS_LINEAR_MODE:
            gain = (*pAgain);
            GC4653_CalcGain(&gain, &againReg, &dGainReg, &convReg);
            *pAgain = gain;
            *pDgain = 1024;
            pSnsRegsInfo->snsData[REG_AGAIN].data = ((againReg & 0x03) | (convReg << 6));
            pSnsRegsInfo->snsData[REG_DGAIN_HCG_H].data = ((dGainReg >> 8) & 0xFF);
            pSnsRegsInfo->snsData[REG_DGAIN_HCG_L].data = (dGainReg & 0xFF) ;
            break;
        default:
            break;
    }

    return VSI_SUCCESS;
}

static int GC4653_SetExpRatio(ISP_PORT IspPort, ISP_EXP_RATIO_S *pExpRatio)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC4653Dev->aeDefault;

    vsios_memcpy(&pAeSnsDft->expRatio, pExpRatio, sizeof(*pExpRatio));

    return BK_OK;
}
#endif

static void GC4653_CalcGain(vsi_u32_t *pGain, vsi_u8_t *pAgainReg, vsi_u16_t *pDGainReg, vsi_u8_t *pConvReg)
{
    uint8_t i;
    uint8_t total;
    uint32_t gain = *pGain;
    total = sizeof(analog_gain_table) / sizeof(uint32_t);
    for(i = 0; i < total; i++)
    {
      if((analog_gain_table[i] <= gain)&&(gain < analog_gain_table[i+1]))
        break;
    }

    if (i == total) {
        LOGE("gc4653_SetSensor_gain: gain out of range\n");
        return;
    }

    ISP_PORT IspPort = {0};
    IspPort.devId = 0;
    IspPort.portId = 0;

    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("gc4653_SetSensor_gain: failed to get sensor dev\n");
        return;
    }

    // LOGI("analog_gain_table: %d\n",analog_gain_table[i]);

    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;

    pSnsRegsInfo->snsData[REG_AGAIN_1].data = regValTable[i][0];
    pSnsRegsInfo->snsData[REG_AGAIN_2].data = regValTable[i][1];
    pSnsRegsInfo->snsData[REG_AGAIN_3].data = regValTable[i][2];
    pSnsRegsInfo->snsData[REG_AGAIN_4].data = regValTable[i][3];
    pSnsRegsInfo->snsData[REG_AGAIN_5].data = regValTable[i][4];
    pSnsRegsInfo->snsData[REG_AGAIN_6].data = regValTable[i][5];
    pSnsRegsInfo->snsData[REG_AGAIN_7].data = regValTable[i][6];

    return;
}

static int GC4653_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    //ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;
    vsi_u32_t gain;
    vsi_u8_t againReg;
    vsi_u16_t dGainReg;
    vsi_u8_t convReg;

    switch(pGC4653Dev->snsModeId) {
        case GC4653_720P_30FPS_LINEAR_MODE:
            gain = (*pAgain) / 1024;
            GC4653_CalcGain(&gain, &againReg, &dGainReg, &convReg);
            *pAgain = gain * 1024;
            *pDgain = 1024;
            break;
        default:
            break;
    }

    return BK_OK;
}


static int GC4653_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC4653Dev->snsRegsInfo;

    switch(pGC4653Dev->snsModeId) {
        case GC4653_720P_30FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPTIME_H].data = ((*pIntLine & 0xFF00) >> 8);
            pSnsRegsInfo->snsData[REG_EXPTIME_L].data = (*pIntLine & 0xFF);
            break;
        default:
            break;
    }

    return BK_OK;
}

static int GC4653_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    GC4653_DEVICE_S *pGC4653Dev = GC4653_GetSensorDev(IspPort);
    if (pGC4653Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &pGC4653Dev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int GC4653_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = GC4653_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = GC4653_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = GC4653_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = GC4653_GetSnsRegInfo;

    return BK_OK;
}

ISP_SNS_OBJ_S snsGC4653Obj = {
    .pfnInitIspSnsFunc = GC4653_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = GC4653_InitAeSnsFunc,
};

//###########################################################################################

#define GC4653_WRITE_ADDRESS (0x52)
#define GC4653_READ_ADDRESS (0x53)
#define CHIP_ID_ADDR_HB (0x3F0)
#define CHIP_ID_ADDR_LB (0x3F1)
#define CHIP_ID_VAL_HB (0x46)
#define CHIP_ID_VAL_LB (0x53)

#define FPS_CTRL_BY_EXP 0 ////controled by exp time
#define FPS_CTRL_BY_LENGTH 1 //controled by framelen and linelen
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH

#define DEFAULT_FRAME_LEN 1500
#define DEFAULT_LINE_LEN 1600
#define GC4653_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 25)

#define WIN_MAX_X 2560
#define WIN_MAX_Y 1440

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define MIPI_CLK_M			270
#define MIPI_CLK_MULTI		(0x5A + (MIPI_CLK_M - 270) / 3)

#define TAG "gc4653"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define GC4653_TABLE_SIZE(table) (sizeof(table) / 4)

avdk_err_t gc4653_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

bool gc4653_read_flag = false;
void mipi_phy_term_set(uint32_t v1, uint32_t v2);

const uint16_t sensor_gc4653_init_table[][2] = 
{
    /****************************************/
    //version 6.8
    //mclk 24Mhz
    //mipi_data_rate 648Mbps
    //framelength 1500
    //linelength 4800
    //pclk 216Mhz
    //rowtime 22.2222us
    //pattern grbg},
    {0x03fe,0xf0},
    {0x03fe,0x00},
    {0x0317,0x00},
    {0x0320,0x77},
    {0x0324,0xc8},
    {0x0325,0x06},
    {0x0326,MIPI_CLK_MULTI},
    {0x0327,0x03},
    {0x0334,0x40},
    {0x0336,MIPI_CLK_MULTI},
    {0x0337,0x82},
    {0x0315,0x25},
    {0x031c,0xc6},
    {0x0287,0x18},
    {0x0084,0x00},
    {0x0087,0x50},
    {0x029d,0x08},
    {0x0290,0x00},
    {0x0340,0x05},
    {0x0341,0xdc}, //vts 1500
    {0x0345,0x06},
    {0x034b,0xb0},
    {0x0352,0x08},
    {0x0354,0x08},
    {0x02d1,0xe0},
    {0x0223,0xf2},
    {0x0238,0xa4},
    {0x02ce,0x7f},
    {0x0232,0xc4},
    {0x02d3,0x05},
    {0x0243,0x06},
    {0x02ee,0x30},
    {0x026f,0x70},
    {0x0257,0x09},
    {0x0211,0x02},
    {0x0219,0x09},
    {0x023f,0x2d},
    {0x0518,0x00},
    {0x0519,0x01},
    {0x0515,0x08},
    {0x02d9,0x3f},
    {0x02da,0x02},
    {0x02db,0xe8},
    {0x02e6,0x20},
    {0x021b,0x10},
    {0x0252,0x22},
    {0x024e,0x22},
    {0x02c4,0x01},
    {0x021d,0x17},
    {0x024a,0x01},
    {0x02ca,0x02},
    {0x0262,0x10},
    {0x029a,0x20},
    {0x021c,0x0e},
    {0x0298,0x03},
    {0x029c,0x00},
    {0x027e,0x14},
    {0x02c2,0x10},
    {0x0540,0x20},
    {0x0546,0x01},
    {0x0548,0x01},
    {0x0544,0x01},
    {0x0242,0x1b},
    {0x02c0,0x1b},
    {0x02c3,0x20},
    {0x02e4,0x10},
    {0x022e,0x00},
    {0x027b,0x3f},
    {0x0269,0x0f},
    {0x02d2,0x40},
    {0x027c,0x08},
    {0x023a,0x2e},
    {0x0245,0xce},
    {0x0530,0x20},
    {0x0531,0x02},
    {0x0228,0x50},
    {0x02ab,0x00},
    {0x0250,0x00},
    {0x0221,0x50},
    {0x02ac,0x00},
    {0x02a5,0x02},
    {0x0260,0x0b},
    {0x0216,0x04},
    {0x0299,0x1c},
    {0x02bb,0x0d},
    {0x02a3,0x02},
    {0x02a4,0x02},
    {0x021e,0x02},
    {0x024f,0x08},
    {0x028c,0x08},
    {0x0532,0x3f},
    {0x0533,0x02},
    {0x0277,0xc0},
    {0x0276,0xc0},
    {0x0239,0xc0},
    {0x0202,0x05},
    {0x0203,0xd0},
    {0x0205,0xc0},
    {0x02b0,0x68},
    {0x0002,0xa9},
    {0x0004,0x01},
    {0x021a,0x98},
    {0x0266,0xa0},
    {0x0020,0x01},
    {0x0021,0x03},
    {0x0022,0x00},
    {0x0023,0x04},
    {0x0342,0x06},
    {0x0343,0x40}, //hts
    {0x03fe,0x10},
    {0x03fe,0x00},
    {0x0106,0x78},
    {0x0108,0x0c},
    {0x0114,0x01},
    {0x0115,0x12},
    {0x0180,0x46},
    {0x0181,0x30},
    {0x0182,0x05},
    {0x0185,0x01},
    {0x03fe,0x10},
    {0x03fe,0x00},
    {0x000f,0x00},
    {0x0100,0x09},
    {0x0080,0x02},
    {0x0097,0x0a},
    {0x0098,0x10},
    {0x0099,0x05},
    {0x009a,0xb0},
    {0x0317,0x08},
    {0x0a67,0x80},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0x10},
    {0x0a80,0x2b},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0313,0x80},
    {0x05be,0x01},
    {0x0317,0x00},
    {0x0a67,0x00},
};

const uint16_t sensor_gc4653_init_table_1080p_40fps[][2] = 
{
    {0x03fe,0xf0},
    {0x03fe,0x00},
    {0x0317,0x00},
    {0x0320,0x77},
    {0x0324,0xc8},
    {0x0325,0x05},
    {0x0326,0x6c},
    {0x0327,0x03},
    {0x0334,0x40},
    {0x0335,0x51},
    {0x0336,0x5a},
    {0x0337,0x82},
    {0x0315,0x25},
    {0x031c,0xc6},
    {0x0287,0x18},
    {0x0084,0x00},
    {0x0087,0x50},
    {0x029d,0x08},
    {0x0290,0x00},
    {0x0340,0x04},  // vts 1250
    {0x0341,0xe2},
    {0x0344,0x00},
    {0x0345,0x06},
    {0x0346,0x00},
    {0x0347,0xb4},
    {0x0348,0x0a},
    {0x0349,0x10},
    {0x034a,0x04},
    {0x034b,0x48},
    {0x0351,0x00},
    {0x0352,0x08},
    {0x0353,0x01},
    {0x0354,0x40},
    {0x034c,0x07},
    {0x034d,0x80},
    {0x034e,0x04},
    {0x034f,0x38},
    {0x02d1,0xe0},
    {0x0223,0xf2},
    {0x0238,0xa4},
    {0x02ce,0x7f},
    {0x0232,0xc4},
    {0x02d3,0x05},
    {0x0243,0x06},
    {0x02ee,0x30},
    {0x026f,0x70},
    {0x0257,0x09},
    {0x0211,0x02},
    {0x0219,0x09},
    {0x023f,0x2d},
    {0x0518,0x00},
    {0x0519,0x01},
    {0x0515,0x08},
    {0x02d9,0x3f},
    {0x02da,0x02},
    {0x02db,0xe8},
    {0x02e6,0x20},
    {0x021b,0x10},
    {0x0252,0x22},
    {0x024e,0x22},
    {0x02c4,0x01},
    {0x021d,0x17},
    {0x024a,0x01},
    {0x02ca,0x02},
    {0x0262,0x10},
    {0x029a,0x20},
    {0x021c,0x0e},
    {0x0298,0x03},
    {0x029c,0x00},
    {0x027e,0x14},
    {0x02c2,0x10},
    {0x0540,0x20},
    {0x0546,0x01},
    {0x0548,0x01},
    {0x0544,0x01},
    {0x0242,0x1b},
    {0x02c0,0x1b},
    {0x02c3,0x20},
    {0x02e4,0x10},
    {0x022e,0x00},
    {0x027b,0x3f},
    {0x0269,0x0f},
    {0x000f,0x00},
    {0x02d2,0x40},
    {0x027c,0x08},
    {0x023a,0x2e},
    {0x0245,0xce},
    {0x0530,0x20},
    {0x0531,0x02},
    {0x0228,0x50},
    {0x02ab,0x00},
    {0x0250,0x00},
    {0x0221,0x50},
    {0x02ac,0x00},
    {0x02a5,0x02},
    {0x0260,0x0b},
    {0x0216,0x04},
    {0x0299,0x1C},
    {0x02bb,0x0d},
    {0x02a3,0x02},
    {0x02a4,0x02},
    {0x021e,0x02},
    {0x024f,0x08},
    {0x028c,0x08},
    {0x0532,0x3f},
    {0x0533,0x02},
    {0x0277,0x60},
    {0x0276,0x60},
    {0x0239,0xc0},
    {0x0202,0x05},
    {0x0203,0x46},
    {0x0205,0xc0},
    {0x02b0,0x68},
    {0x0002,0xa9},
    {0x0004,0x01},
    {0x0005,0x8a},
    {0x0006,0xe0},
    {0x0007,0x65},
    {0x0008,0x66},
    {0x0009,0x56},
    {0x000a,0x55},
    {0x0266,0xc0},
    {0x0021,0x04},
    {0x0022,0x00},
    {0x021a,0x98},
    {0x0020,0x01},
    {0x0023,0x08},
    {0x0023,0x00},
    {0x0342,0x08},
    {0x0343,0x70}, //2160
    {0x010d,0x60},
    {0x010e,0x09},
    {0x03fe,0x10},
    {0x03fe,0x00},
    {0x0104,0x20},
    {0x0105,0x20},
    {0x0106,0x78},
    {0x0108,0x0c},
    {0x0114,0x01},
    {0x0115,0x12},
    {0x0180,0x66},
    {0x0181,0x30},
    {0x0182,0x05},
    {0x0185,0x01},
    {0x03fe,0x10},
    {0x03fe,0x00},
    {0x0100,0x09},
    {0x0080,0x02},
    {0x0097,0x0a},
    {0x0098,0x10},
    {0x0099,0x05},
    {0x009a,0xb0},
    {0x0317,0x08},
    {0x0a67,0x80},
    {0x0a70,0x03},
    {0x0a82,0x00},
    {0x0a83,0x10},
    {0x0a80,0x2b},
    {0x05be,0x00},
    {0x05a9,0x01},
    {0x0313,0x80},
    {0x05be,0x01},
    {0x0317,0x00},
    {0x0a67,0x00},
};

int gc4653_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    uint32_t size = GC4653_TABLE_SIZE(sensor_gc4653_init_table);

    for (int i = 0; i < size; i++)
    {
        bus->write16(bus, sensor_gc4653_init_table[i][0], sensor_gc4653_init_table[i][1]);
    }
    bk_mipi_csi_phy_term_set(0x404, 0x606);
    return 0;
}

int gc4653_1080p_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    // default 1080p 40fps
    uint32_t size = GC4653_TABLE_SIZE(sensor_gc4653_init_table_1080p_40fps);

    for (int i = 0; i < size; i++)
    {
        bus->write16(bus, sensor_gc4653_init_table_1080p_40fps[i][0], sensor_gc4653_init_table_1080p_40fps[i][1]);
    }
    bk_mipi_csi_phy_term_set(0x404, 0x606);
    return 0;
}

int gc4653_set_fps_test(bk_camera_sensor_ctlr_t *controller, uint16_t hts, uint16_t vts)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bus->write16(bus, 0x340, UINT16_HB(vts));
    bus->write16(bus, 0x341, UINT16_LB(vts));

    bus->write16(bus, 0x342, UINT16_HB(hts));
    bus->write16(bus, 0x343, UINT16_LB(hts));

    return 0;
}

avdk_err_t gc4653_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (width > WIN_MAX_X || width <= 0 || height > WIN_MAX_Y || height <= 0)
    {
        return -1;
    }
    bk_mipi_csi_controller_init(width, height, 0x2b);

    uint16_t win_y_start = (WIN_MAX_Y - height) / 2;
    uint16_t win_x_start = (WIN_MAX_X - width) / 2;

    bus->write16(bus, 0x0351, UINT16_HB(win_y_start));
    bus->write16(bus, 0x0352, UINT16_LB(win_y_start));
    bus->write16(bus, 0x0353, UINT16_HB(win_x_start));
    bus->write16(bus, 0x0354, UINT16_LB(win_x_start));

    bus->write16(bus, 0x034c, UINT16_HB(width));
    bus->write16(bus, 0x034d, UINT16_LB(width));
    bus->write16(bus, 0x034e, UINT16_HB(height));
    bus->write16(bus, 0x034f, UINT16_LB(height));

    uint16_t lwc_set = width * 5 / 4 / 2 * 2; //width*5/4, then align to 2
    bus->write16(bus, 0x010e, UINT16_HB(lwc_set));
    bus->write16(bus, 0x010d, UINT16_LB(lwc_set));

    return 0;
}

avdk_err_t gc4653_1080p_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    return 0;
}

avdk_err_t gc4653_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (fps > 60 || fps <= 0)
    {
        LOGE("not supported fps\r\n");
        return -1;
    }

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_LENGTH)
    {
        uint16_t line_len = GC4653_PCLK / DEFAULT_FRAME_LEN / fps;
        bus->write16(bus, 0x342, UINT16_HB(line_len));
        bus->write16(bus, 0x343, UINT16_LB(line_len));
        //changing framelen changes inter frame time, to do
    }

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_EXP)
    {
        //define by vb min , to do
    }

    return 0;
}

avdk_err_t gc4653_1080p_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    switch (fps)
    {
        case 40:
            gc4653_set_fps_test(controller, 2160, 1250);
            break;

        case 30:
            gc4653_set_fps_test(controller, 3000, 1200);
            break;

        case 25:
            gc4653_set_fps_test(controller, 3600, 1200);
            break;

        case 20:
            gc4653_set_fps_test(controller, 3600, 1500);
            break;

        default:
            LOGE("not support fps\r\n");
            break;
    }

    return 0;
}

avdk_err_t gc4653_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        os_printf("gc4653 {%04x, %02x}, \r\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        os_printf("gc4653 {%04x, %02x} -> {%04x, %02x} \r\n", addr, dump_val, addr, val);
        bus->write16(bus, addr, val);
    }

    if (cmd == 2) // standy
    {
        bus->write16(bus, 0x100, 0x00);
        bus->write16(bus, 0x31c, 0xc7);
        bus->write16(bus, 0x317, 0x01);
    }

    if (cmd == 3) // resume from standy
    {
        bus->write16(bus, 0x317, 0x00);
        bus->write16(bus, 0x31c, 0xc6);
        bus->write16(bus, 0x100, 0x09);
    }

    return 0;
}

avdk_err_t gc4653_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    gc4653_set_ppi(controller, format->width, format->height);
    gc4653_set_fps(controller, format->fps);
    return AVDK_ERR_OK;
}

#define GC4653_1080P_TEST 0

#if GC4653_1080P_TEST
#define GC4653_INIT gc4653_1080p_init
#define GC4653_SET_PPI gc4653_1080p_set_ppi
#define GC4653_SET_FPS gc4653_1080p_set_fps
#else
#define GC4653_INIT gc4653_init
#define GC4653_SET_PPI gc4653_set_ppi
#define GC4653_SET_FPS gc4653_set_fps
#endif

const csi_sensor_config_t csi_sensor_gc4653 =
{
    .name = "gc4653",
    .clk = MCLK_24M,
    .mipi_data_type = 0x2b,
    // .vsync = SYNC_HIGH_LEVEL,
    // .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .default_width = 1280,
    .default_height = 720,
    .default_fps = 30,
    .id = ID_GC4653,
    .address = (GC4653_WRITE_ADDRESS >> 1),
    .init = GC4653_INIT,
    .detect = gc4653_detect,
    .set_ppi = GC4653_SET_PPI,
    .set_fps = GC4653_SET_FPS,
    .reg_ctrl = gc4653_ctrl,
    // .power_down = gc2145_reset,
    // .dump_register = gc2145_dump,
};


const ISP_PUB_ATTR_S gc4653_mipi_linear_attr = {
    .pSnsObj      = &snsGC4653Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_GRBG10,
    .snsFps      = 60 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t gc4653_format_array[] = {
    {
        .width = 1280,
        .height = 720,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 25,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 20,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 25,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 20,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },

    {
        .width = 1920,
        .height = 1080,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },

    {
        .width = 640,
        .height = 480,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },

    {
        .width = 1088,
        .height = 1088,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },

    {
        .width = 2560,
        .height = 1440,
        .fps = 20,
        .output_pixel_fmt = BK_PIXEL_FORMAT_GRBG10,
    },

};

void *gc4653_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsGC4653Obj;
}

void *gc4653_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

static avdk_err_t gc4653_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &gc4653_format_array[0];
    format_array->size = ARRAY_SIZE(gc4653_format_array);
    return AVDK_ERR_OK;
}

avdk_err_t gc4653_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
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

    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = GC4653_WRITE_ADDRESS;

    config->bus->read16(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read16(config->bus, CHIP_ID_ADDR_LB, &lb_id);

    os_printf("%s, id: 0x%02X%02X\n", __func__, hb_id, lb_id);

    if (hb_id != CHIP_ID_VAL_HB
        || lb_id != CHIP_ID_VAL_LB)
    {
        return AVDK_ERR_GENERIC;
    }

    LOGI("%s success\n", __func__);

    bk_camera_csi_sensor_t *csi_sensor = os_malloc(sizeof(bk_camera_csi_sensor_t));
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(csi_sensor, 0, sizeof(bk_camera_csi_sensor_t));

    config->bus->write_address = GC4653_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));
    
    csi_sensor->ops.init = gc4653_init;
    csi_sensor->ops.set_format = gc4653_set_format;
    csi_sensor->ops.reg_ctrl = gc4653_ctrl;
    csi_sensor->ops.get_sensor_object = gc4653_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = gc4653_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = gc4653_query_support_formats;
    csi_sensor->isp_pub_attr = &gc4653_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_gc4653;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return 0;
}


BK_CAMERA_SENSOR_DETECT_SECTION(gc4653_detect, CSI_CAMERA_PORT);