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

#define GC20C3_WRITE_ADDRESS (0x62)
#define GC20C3_READ_ADDRESS (0x63)
#define CHIP_ID_ADDR_HB (0x03f0)
#define CHIP_ID_ADDR_LB (0x03f1)
#define CHIP_ID_VAL_HB (0x20)
#define CHIP_ID_VAL_LB (0xC3)

#define FPS_CTRL_BY_EXP 0 ////controled by exp time
#define FPS_CTRL_BY_LENGTH 1 //controled by framelen and linelen
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH

#define DEFAULT_FRAME_LEN 1125
#define DEFAULT_LINE_LEN 1000
#define GC20C3_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 30)

#define WIN_MAX_X 1928
#define WIN_MAX_Y 1088

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define TAG "gc20C3"

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
#include "gc20c3_1080p_calib.h"

#define GC20C3_REG_BYTE_NUM  2
#define GC20C3_DATA_BYTE_NUM 1

#define GC20C3_EXPTIME_H		0x0202
#define GC20C3_EXPTIME_L		0x0203

#define GC20C3_DGAIN_1			0x0474
#define GC20C3_DGAIN_2			0x0475

#define GC20C3_AGAIN_1			0x0d04
#define GC20C3_AGAIN_2			0x0d05
#define GC20C3_AGAIN_3			0x0e36
#define GC20C3_AGAIN_4			0x0e39
#define GC20C3_AGAIN_5			0x04a8
#define GC20C3_AGAIN_6			0x04a9
#define GC20C3_AGAIN_7			0x0052

#define GC20C3_REG_MIRROR_FLIP 0x022c
#define GC20C3_MIRROR_BIT      (1U << 0)
#define GC20C3_VFLIP_BIT       (1U << 1)

typedef struct
{
    uint8_t val1;
    uint8_t val2;
    uint8_t val3;
    uint8_t val4;
    uint8_t val5;
    uint8_t val6;
    uint8_t val7;
} gc20C3_again_val_t;

static UINT8 regValTable[13][7] = {   

   //  0d04  0d05  0e36  0e39   04a8   04a9   0052        |  实际倍数   | Again dB|
     { 0x00, 0x01, 0x15, 0x15,  0x01,  0x00,  0x64},    //|  X1        | 0.00    |
     { 0x00, 0x02, 0x15, 0x15,  0x01,  0x1b,  0x64},    //|  X1.43     | 3.09    |
     { 0x00, 0x03, 0x16, 0x16,  0x02,  0x00,  0x64},    //|  X2.01     | 6.05    |
     { 0x00, 0x04, 0x17, 0x17,  0x02,  0x37,  0x64},    //|  X2.87     | 9.17    |
     { 0x00, 0x05, 0x17, 0x17,  0x04,  0x02,  0x84},    //|  X4.03     | 12.11   |
     { 0x00, 0x06, 0x18, 0x18,  0x05,  0x32,  0x84},    //|  X5.79     | 15.25   |
     { 0x00, 0x07, 0x19, 0x19,  0x08,  0x05,  0x84},    //|  X8.09     | 18.16   |
     { 0x04, 0x97, 0x1a, 0x1a,  0x0b,  0x10,  0x84},    //|  X11.25    | 21.03   |
     { 0x08, 0x07, 0x1b, 0x1b,  0x10,  0x04,  0x84},    //|  X16.07    | 24.12   |
     { 0x0a, 0x4f, 0x1c, 0x1c,  0x16,  0x22,  0x88},    //|  X22.54    | 27.06   |
     { 0x0c, 0x07, 0x1d, 0x1d,  0x20,  0x06,  0x88},    //|  X32.10    | 30.13   |
     { 0x0d, 0x2f, 0x1e, 0x1e,  0x2d,  0x10,  0x88},    //|  X45.26    | 33.11   |
     { 0x0e, 0x07, 0x20, 0x20,  0x3f,  0x23,  0x72},    //|  X63.56    | 36.06   |
};

#define AGAIN_TOTAL_LEVEL 14
static UINT32 gainLevelTable[14] = {	
		64  ,
		91  ,
		128 ,
		183 ,
		257 ,
		370 ,
		517 ,
		720 ,
		1028,
		1442,
		2054,
		2896,
		4067,
		0xffffffff,
   };

enum GC20C3_REG_INDEX {
    REG_EXPTIME_H       = 0,
    REG_EXPTIME_L       = 1,
    REG_DGAIN_1         = 2,
    REG_DGAIN_2         = 3,
    REG_AGAIN_1         = 4,
    REG_AGAIN_2         = 5,
    REG_AGAIN_3         = 6,
    REG_AGAIN_4         = 7,
    REG_AGAIN_5         = 8,
    REG_AGAIN_6         = 9,
    REG_AGAIN_7         = 10,
    
};

#define GC20C3_720P_30FPS_LINEAR_MODE (0)

#define GC20C3_VMAX_720P30_LINEAR (1125)

typedef struct vsiGC20C3_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} GC20C3_DEVICE_S;

static GC20C3_DEVICE_S *GC20C3Dev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * GC20C3_720P_CalibParam_dynamic = NULL;

static GC20C3_DEVICE_S *GC20C3_GetSensorDev(ISP_PORT IspPort)
{
    if (GC20C3Dev[IspPort.devId][IspPort.portId] == NULL)
    {
        GC20C3Dev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(GC20C3_DEVICE_S));
        if (GC20C3Dev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d GC20C3Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(GC20C3Dev[IspPort.devId][IspPort.portId], 0 , sizeof(GC20C3_DEVICE_S));
    }

    return GC20C3Dev[IspPort.devId][IspPort.portId];
}

static int GC20C3_SetStream(ISP_PORT IspPort, vsi_bool_t stream);


static int GC20C3_InitRegInfo(ISP_PORT IspPort)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC20C3Dev->snsRegsInfo;
    pSnsRegsInfo->snsDev = pGC20C3Dev->i2cBus;

    pSnsRegsInfo->addrByteNum = pGC20C3Dev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = pGC20C3Dev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = pGC20C3Dev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 11;   
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_EXPTIME_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_H].regAddr = GC20C3_EXPTIME_H;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].regAddr = GC20C3_EXPTIME_L;

    pSnsRegsInfo->snsData[REG_DGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DGAIN_1].regAddr = GC20C3_DGAIN_1;
    pSnsRegsInfo->snsData[REG_DGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DGAIN_2].regAddr = GC20C3_DGAIN_2;

    pSnsRegsInfo->snsData[REG_AGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_1].regAddr = GC20C3_AGAIN_1;
    pSnsRegsInfo->snsData[REG_AGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_2].regAddr = GC20C3_AGAIN_2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_3].regAddr = GC20C3_AGAIN_3;
    pSnsRegsInfo->snsData[REG_AGAIN_4].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_4].regAddr = GC20C3_AGAIN_4;
    pSnsRegsInfo->snsData[REG_AGAIN_5].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_5].regAddr = GC20C3_AGAIN_5;
    pSnsRegsInfo->snsData[REG_AGAIN_6].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_6].regAddr = GC20C3_AGAIN_6;
    pSnsRegsInfo->snsData[REG_AGAIN_7].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_7].regAddr = GC20C3_AGAIN_7;

    return BK_OK;
}

static int GC20C3_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (GC20C3_720P_CalibParam_dynamic == NULL)
    {
        GC20C3_720P_CalibParam_dynamic = os_malloc(sizeof(GC20C3_720P_CalibParam));
        if (GC20C3_720P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc GC20C3_720P_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(GC20C3_720P_CalibParam_dynamic, &GC20C3_720P_CalibParam, sizeof(GC20C3_720P_CalibParam));
    }

    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(pGC20C3Dev, 0, sizeof(*pGC20C3Dev));
    pGC20C3Dev->i2cBus              = snsDev;
    pGC20C3Dev->i2cAttr.slave_addr  = GC20C3_WRITE_ADDRESS;
    pGC20C3Dev->i2cAttr.reg_bytes   = GC20C3_REG_BYTE_NUM;
    pGC20C3Dev->i2cAttr.data_bytes  = GC20C3_DATA_BYTE_NUM;
    GC20C3_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return BK_FAIL;
    }

    GC20C3_SetStream(IspPort, 0);

    return  BK_OK;
}

static int GC20C3_Exit(ISP_PORT IspPort)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);

    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d already exit\n", __func__, __LINE__);
        return BK_OK;
    }

    vsios_i2c_sys_exit(pGC20C3Dev->i2cBus);

    if (GC20C3_720P_CalibParam_dynamic != NULL)
    {
        os_free(GC20C3_720P_CalibParam_dynamic);
        GC20C3_720P_CalibParam_dynamic = NULL;
    }

    os_free(pGC20C3Dev);
    GC20C3Dev[IspPort.devId][IspPort.portId] = NULL;
    return BK_OK;
}

static int GC20C3_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC20C3Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC20C3Dev->i2cAttr;

    // LOGD("i2c write (%x, %x) \r\n", addr, data);
    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);
    // vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int GC20C3_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pGC20C3Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pGC20C3Dev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int GC20C3_InitAeDefault(ISP_PORT IspPort)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pGC20C3Dev->aeDefault;

    switch (pGC20C3Dev->snsModeId) {
        case GC20C3_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = GC20C3_VMAX_720P30_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 30 * ISP_SNS_FPS_ACCU;   
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 2;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 832 * 1024;
            pAeSnsDft->minAgain  = 64 * 1024;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 0xFFFF;   
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

static int GC20C3_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if ((pSnsMode->width == pGC20C3Dev->snsMode.width) &&
        (pSnsMode->height == pGC20C3Dev->snsMode.height) &&
        (pSnsMode->hdrMode == pGC20C3Dev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == pGC20C3Dev->snsMode.stichMode)) {
        return BK_OK;
    }

    if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC20C3_Linear1920x1080Init(pGC20C3Dev->i2cBus, &pGC20C3Dev->i2cAttr);
        os_memcpy(&pGC20C3Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC20C3Dev->snsModeId = GC20C3_720P_30FPS_LINEAR_MODE;
        GC20C3_InitAeDefault(IspPort);
    }
    else if ((pSnsMode->width  == 640) &&
        (pSnsMode->height == 480) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // GC20C3_Linear1920x1080Init(pGC20C3Dev->i2cBus, &pGC20C3Dev->i2cAttr);
        os_memcpy(&pGC20C3Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC20C3Dev->snsModeId = GC20C3_720P_30FPS_LINEAR_MODE;
        GC20C3_InitAeDefault(IspPort);
    } else {
        LOGI("gc20C3 custom set mode end ~~~\n");
        os_memcpy(&pGC20C3Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pGC20C3Dev->snsModeId = GC20C3_720P_30FPS_LINEAR_MODE;
        GC20C3_InitAeDefault(IspPort);
    }

    return BK_OK;
}

static int GC20C3_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        // GC20C3_WriteReg(IspPort, 0x3283, 0x00);
        // GC20C3_WriteReg(IspPort, 0x3012, 1);
    } else {
        // GC20C3_WriteReg(IspPort, 0x3012, 0);
    }

    pGC20C3Dev->stream = stream;

    return BK_OK;
}

static int GC20C3_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, GC20C3_720P_CalibParam_dynamic);

    return BK_OK;
}

static int GC20C3_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = GC20C3_Init;
    pIspSnsFunc->pfnSensorExit    = GC20C3_Exit;
    pIspSnsFunc->pfnWriteReg      = GC20C3_WriteReg;
    pIspSnsFunc->pfnReadReg       = GC20C3_ReadReg;
    pIspSnsFunc->pfnSetMode       = GC20C3_SetMode;
    pIspSnsFunc->pfnSetStream     = GC20C3_SetStream;
    pIspSnsFunc->pfnSetIspDefault = GC20C3_SetIspDefault;

    return BK_OK;
}

static int GC20C3_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &pGC20C3Dev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

static void GC20C3_CalcGain(uint32_t *pGain, gc20C3_again_val_t *again_val, uint16_t *pDGainReg, uint8_t *pConvReg)
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
    again_val->val5 = regValTable[i][4];    
    again_val->val6 = regValTable[i][5];
    again_val->val7 = regValTable[i][6];

    if (pDGainReg)
        *pDGainReg = 1024;  
    if (pConvReg)
        *pConvReg = 0;
}

static int GC20C3_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC20C3Dev->snsRegsInfo;
    uint32_t gain;
    gc20C3_again_val_t reg_val;
    uint16_t dGainReg;
    uint8_t convReg;

    switch(pGC20C3Dev->snsModeId) {
        case GC20C3_720P_30FPS_LINEAR_MODE:
        {
            uint32_t total_gain = (*pAgain) / 1024;
            if (total_gain == 0) total_gain = 1; 

            gain = total_gain;
            GC20C3_CalcGain(&gain, &reg_val, &dGainReg, &convReg);

            uint32_t dig_gain = (total_gain * 1024) / gain;
            if (dig_gain < 1024) dig_gain = 1024; // >= 1x
            
            *pAgain = gain * 1024;      // * 1024
            *pDgain = dig_gain;         //  * 1024

            
            pSnsRegsInfo->snsData[REG_AGAIN_2].data = reg_val.val1;
            pSnsRegsInfo->snsData[REG_AGAIN_1].data = reg_val.val2;
            pSnsRegsInfo->snsData[REG_AGAIN_3].data = reg_val.val3;
            pSnsRegsInfo->snsData[REG_AGAIN_4].data = reg_val.val4;
            pSnsRegsInfo->snsData[REG_AGAIN_5].data = reg_val.val5;
            pSnsRegsInfo->snsData[REG_AGAIN_6].data = reg_val.val6;
            pSnsRegsInfo->snsData[REG_AGAIN_7].data = reg_val.val7;


            pSnsRegsInfo->snsData[REG_DGAIN_1].data = (uint8_t)(dig_gain >> 8);
            pSnsRegsInfo->snsData[REG_DGAIN_2].data = (uint8_t)(dig_gain & 0xFF);
            break;
        }
        default:
            break;
    }

    return BK_OK;
}

static int GC20C3_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pGC20C3Dev->snsRegsInfo;

    switch(pGC20C3Dev->snsModeId) {
        case GC20C3_720P_30FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPTIME_H].data = ((*pIntLine & 0xFF00) >> 8);
            pSnsRegsInfo->snsData[REG_EXPTIME_L].data = (*pIntLine & 0xFF);
            break;
        default:
            break;
    }

    return BK_OK;
}

static int GC20C3_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    GC20C3_DEVICE_S *pGC20C3Dev = GC20C3_GetSensorDev(IspPort);
    if (pGC20C3Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &pGC20C3Dev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int GC20C3_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = GC20C3_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = GC20C3_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = GC20C3_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = GC20C3_GetSnsRegInfo;

    return BK_OK;
}

const ISP_SNS_OBJ_S snsGC20C3Obj = {
    .pfnInitIspSnsFunc = GC20C3_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = GC20C3_InitAeSnsFunc,
};

#endif
//#####################################################################################################

#define GC20C3_TABLE_SIZE(table) (sizeof(table) / sizeof(table[0]))

avdk_err_t gc20C3_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

struct i2c_config {
	uint16_t reg;
	uint8_t val;
};

static const struct i2c_config sensor_gc20C3_init_table[] =
{//version 273c2e
//<MODE_2 type="00_GC20C3_MIPI2L_24M_1920x1080_30fps_raw10_linear">		
//<RESOLUTION width="1920" height="1080"/>		
//<MCLK type="24.0"/>
//<BAYERMODE type="GRBG"/>
//mipi_rate = 540 Mbps/lane
//wpclk = 135 Mhz
//rpclk = 108 Mhz
//FL = 1125
    { 0x03fe,0xff},
    { 0x03fe,0x00},
    { 0x03fe,0x10},
    { 0x0190,0x03},
    { 0x0b4d,0x02},
    { 0x0d40,0x01},
    { 0x0087,0x50},
    { 0x0209,0x00},
    { 0x03b5,0x11}, 
    { 0x031c,0x18}, 
    { 0x03b2,0x03}, 
    { 0x03bb,0xff}, 
    { 0x03be,0xff},
    { 0x0d11,0x0b},
    { 0x0d15,0x03},
    { 0x0d12,0x05},
    { 0x0d16,0x00},
    { 0x0d17,0x87},
    { 0x0d10,0x06},
    { 0x0d10,0x07},
    { 0x0d1a,0x02}, 
    { 0x0d18,0x01},
    { 0x0d19,0x48}, 
    { 0x0145,0x0d},
    { 0x0144,0x02},
    { 0x0142,0x04},
    { 0x0143,0x14},
    { 0x0146,0x05},
    { 0x0141,0x05}, 
    { 0x0149,0x05},
    { 0x014a,0x07}, 
    { 0x014b,0x06},
    { 0x0b4e,0x88},
    { 0x0e0c,0x00},
    { 0x0e0f,0x00},
    { 0x0e3a,0x98},
    { 0x0b45,0x06},
    { 0x0b47,0xf0},
    { 0x0d30,0x06},
    { 0x0d2f,0x05},
    { 0x0b40,0x57},
    { 0x0b43,0x00},
    { 0x0b41,0x0c},
    { 0x0d31,0x03},
    { 0x0b4f,0x00},
    { 0x0c23,0x40},
    { 0x0e23,0x1a},
    { 0x0e2a,0x09},
    { 0x0e2b,0xc9},
    { 0x0e41,0x87},
    { 0x0e3b,0xb5},
    { 0x0e3a,0x15},
    { 0x0e37,0x1f},
    { 0x0e0f,0x00},
    { 0x0e13,0x00},
    { 0x03bd,0x40},
    { 0x0d41,0x00},
    { 0x0217,0x02}, 
    { 0x0213,0x04}, 
    { 0x0219,0xc2},
    { 0x0259,0x04},
    { 0x025a,0x5e},
    { 0x0211,0x01},
    { 0x0340,0x04},
    { 0x0341,0x65},
    { 0x0342,0x03},
    { 0x0343,0xe8},
    { 0x0212,0x14},
    { 0x0350,0x06},
    { 0x0348,0x07},
    { 0x0349,0x88},
    { 0x034a,0x04},
    { 0x034b,0x40},
    { 0x0347,0x00},
    { 0x0b0c,0x00},
    { 0x0b0d,0x02},
    { 0x0b0e,0x07},
    { 0x0b0f,0x8a},
    { 0x034e,0x07},
    { 0x034f,0xa8},
    { 0x0004,0x0f},
    { 0x0444,0x00},
    { 0x0038,0x20},
    { 0x0039,0x20},
    { 0x003a,0x20},
    { 0x003b,0x20}, 
    { 0x0492,0x00},
    { 0x0493,0x00}, 
    { 0x0070,0x00},
    { 0x0094,0x07},
    { 0x0095,0x80},
    { 0x0096,0x04},
    { 0x0097,0x38},
    { 0x0099,0x04},
    { 0x009b,0x04},
    { 0x0438,0x0f},
    { 0x0439,0xf0},
    { 0x021a,0x10},
    { 0x0476,0x01},
    { 0x0430,0x23},
    { 0x0443,0x02},
    { 0x0038,0x00},
    { 0x0039,0x00},
    { 0x003a,0x00},
    { 0x003b,0x00},
    { 0x0070,0x80},
    { 0x0448,0x0d},
    { 0x0449,0x0d},
    { 0x044a,0x0d},
    { 0x044b,0x0d},
    { 0x044c,0x74},
    { 0x044d,0x74},
    { 0x044e,0x74},
    { 0x044f,0x74},
    { 0x0485,0x68},
    { 0x0d38,0x07},
    { 0x0d39,0x57},
    { 0x0e4e,0xa9},
    { 0x0072,0x09},
    { 0x0073,0x05},
    { 0x0c20,0x09},
    { 0x0c1d,0x02},
    { 0x0c1e,0x2c},
    { 0x0c1f,0xe6},
    { 0x0c19,0x00},
    { 0x0c1a,0x11},
    { 0x0c1b,0x00},
    { 0x0c1c,0x80},
    { 0x0261,0x13},
    { 0x0004,0x0f},
    { 0x0040,0x2c},
    { 0x004c,0x10},
    { 0x004d,0x40},
    { 0x004e,0xc0},
    { 0x0043,0x03},
	{ 0x0044,0x11},
	{ 0x0045,0x58},
	{ 0x0046,0x57},
	{ 0x0052,0x84},
	{ 0x0053,0xa0},
	{ 0x0055,0x20},
	{ 0x0152,0x14},
	//{ 0x0100,0x03},
	{ 0x031c,0x1f},
	{ 0x0336,0x01},
	{ 0x0336,0x00},
	{ 0x03fe,0x00}, 
};

static avdk_err_t gc20C3_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bk_mipi_csi_ext_set_enable(0);
    //bk_mipi_csi_enable_debug_pin();

    uint32_t size = GC20C3_TABLE_SIZE(sensor_gc20C3_init_table);

    for (int i = 0; i < size; i++)
    {
        bus->write16(bus, sensor_gc20C3_init_table[i].reg, sensor_gc20C3_init_table[i].val);
    }

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static bool s_gc20C3_hmirror;
static bool s_gc20C3_vflip;

static void gc20C3_apply_mirror_reg(bk_camera_bus_t *bus)
{
    uint8_t val = 0;

    if (bus == NULL)
    {
        return;
    }

    bus->read16(bus, GC20C3_REG_MIRROR_FLIP, &val);
    val &= (uint8_t)~(GC20C3_MIRROR_BIT | GC20C3_VFLIP_BIT);
    if (s_gc20C3_hmirror)
    {
        val |= GC20C3_MIRROR_BIT;
    }
    if (s_gc20C3_vflip)
    {
        val |= GC20C3_VFLIP_BIT;
    }
    bus->write16(bus, GC20C3_REG_MIRROR_FLIP, val);
}

static avdk_err_t gc20C3_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (width > 1936|| width <= 0 || height > 1088 || height <= 0)
    {
        LOGE("%s, %d width: %d, height: %d, fail...\n", __func__, __LINE__, width, height);
        return -1;
    }

    bk_mipi_csi_controller_init(width, height, 0x2b);

//  bus->write16(bus, 0xFE, 0x1);
    uint16_t win_y_start = (WIN_MAX_Y - height) / 2;
    uint16_t win_x_start = (WIN_MAX_X - width) / 2;
    win_y_start = win_y_start - (win_y_start % 4);
    win_x_start = (win_x_start < 3) ? 3 : win_x_start;
    win_x_start = win_x_start - ((win_x_start - 3) % 4);

      bus->write16(bus, 0x0098, UINT16_HB(win_y_start));
      bus->write16(bus, 0x0099, UINT16_LB(win_y_start));
      bus->write16(bus, 0x009a, UINT16_HB(win_x_start));
      bus->write16(bus, 0x009b, UINT16_LB(win_x_start));
      bus->write16(bus, 0x0096, UINT16_HB(height));
      bus->write16(bus, 0x0097, UINT16_LB(height));
      bus->write16(bus, 0x0094, UINT16_HB(width));
      bus->write16(bus, 0x0095, UINT16_LB(width));

    uint16_t lwc_set = width * 5 / 4 / 2 * 2; //width*5/4, then align to 2
      bus->write16(bus, 0x010d,UINT16_HB(lwc_set));
      bus->write16(bus, 0x010e,UINT16_LB(lwc_set));
	  

		bus->write16(bus, 0x03fe,0x30);
		bus->write16(bus, 0x0100,0x03);    
		bus->write16(bus, 0x03fe,0x00);
    return 0;
}

static avdk_err_t gc20C3_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (fps > 30 || fps <= 0)
    {
        LOGE("not supported fps\r\n");
        return -1;
    }

    //bus->write16(bus, 0xFE, 0x0);

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_LENGTH)
    {
        uint16_t line_len = GC20C3_PCLK / DEFAULT_LINE_LEN / fps;
         bus->write16(bus, 0x0340,UINT16_HB(line_len));
         bus->write16(bus, 0x0341,UINT16_LB(line_len));
        //changing framelen changes inter frame time, to do
    }

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_EXP)
    {
         uint16_t line_len = GC20C3_PCLK / DEFAULT_LINE_LEN / fps;
         bus->write16(bus, 0x0340,UINT16_HB(line_len));
         bus->write16(bus, 0x0341,UINT16_LB(line_len));  
    }

    return 0;
}

static avdk_err_t gc20C3_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    gc20C3_apply_mirror_reg(csi_sensor->config.bus);
    gc20C3_set_ppi(controller, format->width, format->height);
    gc20C3_set_fps(controller, format->fps);
    bk_mipi_csi_controller_reset();
    gc20C3_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t gc20C3_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg read
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("gc20C3 {%04x, %02x}\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("gc20C3 {%04x, %02x} -> {%04x, %02x}\n", addr, dump_val, addr, val);
        bus->write16(bus, addr, val);
    }

    if (cmd == 2) // standy, stream stop
    {
        bus->write16(bus, 0x0100,0x00);
        bus->write16(bus, 0x031c,0x10);
        bus->write16(bus, 0x03bb,0x07);
        bus->write16(bus, 0x03be,0x40);
        bus->write16(bus, 0x0d10,0x04);
		bus->write16(bus, 0x0b4d,0x00);
		bus->write16(bus, 0x0d40,0x00);
	 }

    if (cmd == 3) // resume from standy, stream start
    {
		 bus->write16(bus, 0x031c,0x18);
		 bus->write16(bus, 0x03fe,0x30);
		 bus->write16(bus, 0x0b4d,0x02);
		 bus->write16(bus, 0x0d40,0x01);
		 bus->write16(bus, 0x03bb,0xff);
		 bus->write16(bus, 0x03be,0x7f);
		 bus->write16(bus, 0x0d10,0x06);
		 bus->write16(bus, 0x0d11,0x0b);
		 bus->write16(bus, 0x0d10,0x07);
		 bus->write16(bus, 0x031c,0x1f);
		 bus->write16(bus, 0x0336,0x01);
		 bus->write16(bus, 0x0336,0x00); 
		 bus->write16(bus, 0x0100,0x03);    
		 bus->write16(bus, 0x03fe,0x00);
    }

    return 0;
}

static avdk_err_t gc20C3_set_hmirror(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    s_gc20C3_hmirror = enable;
    gc20C3_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t gc20C3_set_vflip(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    s_gc20C3_vflip = enable;
    gc20C3_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static const csi_sensor_config_t csi_sensor_gc20C3 =
{
    .name = "gc20c3",
    .clk = MCLK_24M,
    .mipi_data_type = 0x2b,
    .default_width = 1920,
    .default_height = 1080,
    .default_fps = 30,
    .id = ID_GC20C3,
    .address = (GC20C3_WRITE_ADDRESS >> 1),
    .init = gc20C3_init,
    .detect = gc20C3_detect,
    .set_ppi = gc20C3_set_ppi,
    .set_fps = gc20C3_set_fps,
    .reg_ctrl = gc20C3_ctrl,
    // .power_down = gc2145_reset,
};

static const ISP_PUB_ATTR_S gc20C3_mipi_linear_attr = {
    .pSnsObj      = (void*)&snsGC20C3Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};
#define FIXEL_FORMAT  BK_PIXEL_FORMAT_RGGB10
static const bk_camera_sensor_format_t gc20C3_format_array[] = {
    {
        .width = 1280,
        .height = 720,
        .fps = 30,
        .output_pixel_fmt = FIXEL_FORMAT,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 25,
        .output_pixel_fmt = FIXEL_FORMAT,
    },
    {
        .width = 1280,
        .height = 720,
        .fps = 20,
        .output_pixel_fmt = FIXEL_FORMAT,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 30,
        .output_pixel_fmt = FIXEL_FORMAT,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 25,
        .output_pixel_fmt = FIXEL_FORMAT,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 20,
        .output_pixel_fmt = FIXEL_FORMAT,
    },

    {
        .width = 1920,
        .height = 1080,
        .fps = 15,
        .output_pixel_fmt = FIXEL_FORMAT,
    },

    {
        .width = 640,
        .height = 480,
        .fps = 30,
        .output_pixel_fmt = FIXEL_FORMAT,
    },

    {
        .width = 1088,
        .height = 1088,
        .fps = 15,
        .output_pixel_fmt = FIXEL_FORMAT,
    },

};

static avdk_err_t gc20C3_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &gc20C3_format_array[0];
    format_array->size = ARRAY_SIZE(gc20C3_format_array);
    return AVDK_ERR_OK;
}

static avdk_err_t gc20C3_ioctl(bk_camera_sensor_ctlr_t *controller, uint32_t cmd, void *arg)
{
    (void)controller;

    switch (cmd)
    {
        case BK_CAMERA_SENSOR_IOCTL_GET_DEFAULT_CPROC:
        {
            bk_isp_cproc_attr_t *out = (bk_isp_cproc_attr_t *)arg;
            const ISP_CPROC_ATTR_S *src;

            AVDK_RETURN_ON_FALSE(out, AVDK_ERR_INVAL, TAG, "default cproc arg is NULL");
            src = &GC20C3_720P_CalibParam.modules.cproc;
            out->enable = src->enable ? 1 : 0;
            out->op_type = src->opType;
            out->manual.brightness = src->manualAttr.brightness;
            out->manual.contrast = src->manualAttr.contrast;
            out->manual.saturation = src->manualAttr.saturation;
            out->manual.hue = src->manualAttr.hue;
            os_memcpy(out->auto_attr.brightness, src->autoAttr.brightness, sizeof(out->auto_attr.brightness));
            os_memcpy(out->auto_attr.contrast, src->autoAttr.contrast, sizeof(out->auto_attr.contrast));
            os_memcpy(out->auto_attr.saturation, src->autoAttr.saturation, sizeof(out->auto_attr.saturation));
            os_memcpy(out->auto_attr.hue, src->autoAttr.hue, sizeof(out->auto_attr.hue));
            return AVDK_ERR_OK;
        }

        default:
            return AVDK_ERR_UNSUPPORTED;
    }
}

static void *gc20C3_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsGC20C3Obj;
}

static void *gc20C3_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

avdk_err_t gc20C3_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = GC20C3_WRITE_ADDRESS;

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

    config->bus->read16(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read16(config->bus, CHIP_ID_ADDR_LB, &lb_id);
    os_printf("gc20C3 detect addr: 0x%02X id: 0x%02X%02X\n",
              GC20C3_WRITE_ADDRESS, hb_id, lb_id);

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
    config->bus->write_address = GC20C3_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = gc20C3_init;
    csi_sensor->ops.set_format = gc20C3_set_format;
    csi_sensor->ops.reg_ctrl = gc20C3_ctrl;
    csi_sensor->ops.set_hmirror = gc20C3_set_hmirror;
    csi_sensor->ops.set_vflip = gc20C3_set_vflip;
    csi_sensor->ops.get_sensor_object = gc20C3_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = gc20C3_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = gc20C3_query_support_formats;
    csi_sensor->ops.ioctl = gc20C3_ioctl;

    csi_sensor->isp_pub_attr = &gc20C3_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_gc20C3;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}


BK_CAMERA_SENSOR_DETECT_SECTION(gc20C3_detect, CSI_CAMERA_PORT);