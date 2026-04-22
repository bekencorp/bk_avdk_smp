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

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

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
#include "cv2005_1080p_calib.h"
//#include <driver/isp_hardware.h>

#define LOGTAG "CV2005"

// Use OS abstraction APIs directly to avoid pulling in VeriSilicon OSI headers.
#define TAG "cv2005"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define CV2005_REG_BYTE_NUM  2
#define CV2005_DATA_BYTE_NUM 1

#define CV2005_EXPTIME_H        0x3049
#define CV2005_EXPTIME_L        0x3048

#define CV2005_GAIN_NE          0x3109

// #define CV2005_AGAIN         0x3118
// #define CV2005_DGAIN_L       0x311C
// #define CV2005_DGAINE_H      0x311D
#define CV2005_DGAIN_1          0x311C
#define CV2005_DGAIN_2          0x311D
#define CV2005_AGAIN_1          0x3118
//#define CV2005_AGAIN_2            0xB4
// #define CV2005_AGAIN_3           0xB8
// #define CV2005_AGAIN_4           0xB9

//to do 
// const uint8_t cv2005_regValTable[29][4] = {

// };

enum CV2005_REG_INDEX {
    REG_EXPTIME_H       = 0,
    REG_EXPTIME_L       = 1,
    REG_DGAIN_1         = 2,
    REG_DGAIN_2         = 3,
    REG_AGAIN_1         = 4,
    REG_AGAIN_2         = 5,
    REG_AGAIN_3         = 6,
    REG_AGAIN_4         = 7,
};

#define CV2005_1080P_30FPS_LINEAR_MODE (0)

#define CV2005_VMAX_1080P30_LINEAR (1125)

typedef struct vsiCV2005_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} CV2005_DEVICE_S;

static CV2005_DEVICE_S *CV2005Dev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * CV2005_1080P_CalibParam_dynamic = NULL;

static CV2005_DEVICE_S *CV2005_GetSensorDev(ISP_PORT IspPort)
{
    if (CV2005Dev[IspPort.devId][IspPort.portId] == NULL)
    {
        CV2005Dev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(CV2005_DEVICE_S));
        if (CV2005Dev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d CV2005Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(CV2005Dev[IspPort.devId][IspPort.portId], 0 , sizeof(CV2005_DEVICE_S));
    }

    return CV2005Dev[IspPort.devId][IspPort.portId];
}

static int CV2005_SetStream(ISP_PORT IspPort, vsi_bool_t stream);

static int CV2005_InitRegInfo(ISP_PORT IspPort)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pCV2005Dev->snsRegsInfo;
    pSnsRegsInfo->snsDev = pCV2005Dev->i2cBus;

    pSnsRegsInfo->addrByteNum = pCV2005Dev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = pCV2005Dev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = pCV2005Dev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 5;
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_EXPTIME_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_H].regAddr = CV2005_EXPTIME_H;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPTIME_L].regAddr = CV2005_EXPTIME_L;

    pSnsRegsInfo->snsData[REG_DGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DGAIN_1].regAddr = CV2005_DGAIN_1;
    pSnsRegsInfo->snsData[REG_DGAIN_2].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DGAIN_2].regAddr = CV2005_DGAIN_2;

    pSnsRegsInfo->snsData[REG_AGAIN_1].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_AGAIN_1].regAddr = CV2005_AGAIN_1;
    //pSnsRegsInfo->snsData[REG_AGAIN_2].delayFrameNum = 2;
    //pSnsRegsInfo->snsData[REG_AGAIN_2].regAddr = CV2005_AGAIN_2;
    // pSnsRegsInfo->snsData[REG_AGAIN_3].delayFrameNum = 2;
    // pSnsRegsInfo->snsData[REG_AGAIN_3].regAddr = CV2005_AGAIN_3;
    // pSnsRegsInfo->snsData[REG_AGAIN_4].delayFrameNum = 2;
    // pSnsRegsInfo->snsData[REG_AGAIN_4].regAddr = CV2005_AGAIN_4;

    return BK_OK;
}

static int CV2005_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (CV2005_1080P_CalibParam_dynamic == NULL)
    {
        CV2005_1080P_CalibParam_dynamic = os_malloc(sizeof(CV2005_1080P_CalibParam));
        if (CV2005_1080P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc CV2005_1080P_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(CV2005_1080P_CalibParam_dynamic, &CV2005_1080P_CalibParam, sizeof(CV2005_1080P_CalibParam));
    }

    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(pCV2005Dev, 0, sizeof(*pCV2005Dev));
    pCV2005Dev->i2cBus              = snsDev;
    pCV2005Dev->i2cAttr.slave_addr  = 0x35;
    pCV2005Dev->i2cAttr.reg_bytes   = CV2005_REG_BYTE_NUM;
    pCV2005Dev->i2cAttr.data_bytes  = CV2005_DATA_BYTE_NUM;
    CV2005_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return ret;
    }

    CV2005_SetStream(IspPort, 0);

    return  BK_OK;
}

static int CV2005_Exit(ISP_PORT IspPort)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsios_i2c_sys_exit(pCV2005Dev->i2cBus);
    if (CV2005_1080P_CalibParam_dynamic != NULL)
    {
        os_free(CV2005_1080P_CalibParam_dynamic);
    }
    return  BK_OK;
}

static int CV2005_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pCV2005Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pCV2005Dev->i2cAttr;

    // os_printf("i2c write (%x, %x) \r\n", addr, data);
    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);
    // os_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int CV2005_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pCV2005Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pCV2005Dev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int CV2005_InitAeDefault(ISP_PORT IspPort)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pCV2005Dev->aeDefault;

    switch (pCV2005Dev->snsModeId) {
        case CV2005_1080P_30FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = CV2005_VMAX_1080P30_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 20 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 10;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 32 * 1024;
            pAeSnsDft->minAgain  = 1 * 1024;
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

static int CV2005_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if ((pSnsMode->width == pCV2005Dev->snsMode.width) &&
        (pSnsMode->height == pCV2005Dev->snsMode.height) &&
        (pSnsMode->hdrMode == pCV2005Dev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == pCV2005Dev->snsMode.stichMode)) {
        return BK_OK;
    }

    if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // CV2005_Linear1920x1080Init(pCV2005Dev->i2cBus, &pCV2005Dev->i2cAttr);
        LOGI("cv2005 1080p set mode end \r\n");
        os_memcpy(&pCV2005Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pCV2005Dev->snsModeId = CV2005_1080P_30FPS_LINEAR_MODE;
        CV2005_InitAeDefault(IspPort);
    }
    /*else if ((pSnsMode->width  == 640) &&
        (pSnsMode->height == 480) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // CV2005_Linear1920x1080Init(pCV2005Dev->i2cBus, &pCV2005Dev->i2cAttr);
        os_printf("cv2005 480p set mode end \r\n");
        os_memcpy(&pCV2005Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pCV2005Dev->snsModeId = CV2005_1080P_30FPS_LINEAR_MODE;
        CV2005_InitAeDefault(IspPort);
    } */else {
        LOGI("cv2005 custom set mode end ~~~\r\n");
        os_memcpy(&pCV2005Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pCV2005Dev->snsModeId = CV2005_1080P_30FPS_LINEAR_MODE;
        CV2005_InitAeDefault(IspPort);
    }

    return  BK_OK;
}

static ISP_PORT g_port = {0, 0};

void csi_read_cv2005(unsigned int addr)
{
    vsi_u32_t new_value = 0x55;
    CV2005_ReadReg(g_port, addr, &new_value);
    LOGI("0x%x is 0x%x\n", addr, new_value);
}

void csi_write_cv2005(unsigned int addr, unsigned int value)
{
    CV2005_WriteReg(g_port, addr, value);
    LOGI("0x%x to 0x%x\n", addr, value);
}


static int CV2005_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        CV2005_WriteReg(IspPort, 0x3000, 0x00);
        // CV2005_WriteReg(IspPort, 0x3012, 1);
    } else {
        CV2005_WriteReg(IspPort, 0x3000, 0x01);
    }

    pCV2005Dev->stream = stream;

    g_port = IspPort;
    LOGI("devid=%d, portid=%d\n", IspPort.devId, IspPort.portId);
    
    return  BK_OK;
}

static int CV2005_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, CV2005_1080P_CalibParam_dynamic);

    return  BK_OK;
}

static int CV2005_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = CV2005_Init;
    pIspSnsFunc->pfnSensorExit    = CV2005_Exit;
    pIspSnsFunc->pfnWriteReg      = CV2005_WriteReg;
    pIspSnsFunc->pfnReadReg       = CV2005_ReadReg;
    pIspSnsFunc->pfnSetMode       = CV2005_SetMode;
    pIspSnsFunc->pfnSetStream     = CV2005_SetStream;
    pIspSnsFunc->pfnSetIspDefault = CV2005_SetIspDefault;

    return BK_OK;
}

static int CV2005_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &pCV2005Dev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

#if 0
static int CV2005_SetFps(ISP_PORT IspPort, vsi_u32_t fps)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pCV2005Dev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pCV2005Dev->snsRegsInfo;
    vsi_u32_t vts;

    switch(pCV2005Dev->snsModeId) {
        case CV2005_1080P_30FPS_LINEAR_MODE:
            if ((fps <= 30 * ISP_SNS_FPS_ACCU) && (fps >= 0.5 * ISP_SNS_FPS_ACCU)) {
                vts = CV2005_VMAX_1080P30_LINEAR * 30 * ISP_SNS_FPS_ACCU / fps;
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

static int CV2005_SlowFrameRate(ISP_PORT IspPort, vsi_u32_t fullLines)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pCV2005Dev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pCV2005Dev->snsRegsInfo;

    fullLines = (fullLines > pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : fullLines;
    pAeSnsDft->fullLines = fullLines;

    pSnsRegsInfo->snsData[REG_VTS_H].data = ((fullLines & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_VTS_L].data = (fullLines & 0xFF);

    switch(pCV2005Dev->snsModeId) {
        case CV2005_1080P_30FPS_LINEAR_MODE:
            pAeSnsDft->maxIntLine = pAeSnsDft->fullLines - 2;
            break;
        default:
            break;
    }

    return BK_OK;
    
}

static void CV2005_CalcGain(vsi_u32_t *pGain, vsi_u8_t *pAgainReg, vsi_u16_t *pDGainReg, vsi_u8_t *pConvReg)
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

static int CV2005_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pCV2005Dev->snsRegsInfo;
    vsi_u32_t gain;
    vsi_u8_t againReg;
    vsi_u16_t dGainReg;
    vsi_u8_t convReg;

    switch(pCV2005Dev->snsModeId) {
        case CV2005_1080P_30FPS_LINEAR_MODE:
            gain = (*pAgain);
            CV2005_CalcGain(&gain, &againReg, &dGainReg, &convReg);
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

static int CV2005_SetExpRatio(ISP_PORT IspPort, ISP_EXP_RATIO_S *pExpRatio)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    AE_SNS_DEFAULT_S *pAeSnsDft = &pCV2005Dev->aeDefault;

    vsios_memcpy(&pAeSnsDft->expRatio, pExpRatio, sizeof(*pExpRatio));

    return BK_OK;
}
#endif

static void CV2005_CalcGain(vsi_u32_t *pGain, vsi_u8_t *pAgainReg, vsi_u16_t *pDGainReg, vsi_u8_t *pConvReg)
{
    vsi_u32_t again_reg,again_real,dgain_want,dgain_reg;

    if(*pGain <= 1024)
        *pGain = 1024;
    else if(*pGain > 128 * 1024) {
        *pGain = 128 * 1024;
    }

    /////again_real = 256/(256-reg)
    /////dgain_real = reg / 64

    if(*pGain <= 32*1024) {
        again_reg = 256 - 256/(*pGain/1024);       //=240, 假如�?16�?.
        again_real = 256*1024/(256-again_reg);   //=16 * 1024
        dgain_want = *pGain * 1024 /again_real;    //=1024
        dgain_reg  = dgain_want * 64 /1024 ;     //=64
    } else {
        again_reg = 0xf8;                  //假如�?48�?.
        again_real = 32 * 1024;                   //32*1024
        dgain_want = *pGain * 1024 /again_real ;      //48 * 1024 / (32*1024) * 1024 = 1536.
        dgain_reg  = dgain_want * 64 /1024 ;      //1536*64/1024 = 96.
    }

    if (dgain_reg < 64){
        dgain_reg = 64;
        // dgain_real = 1;
    }

    *pAgainReg = again_reg;
    *pDGainReg = ((dgain_reg >> 8) & 0xff) | (dgain_reg & 0xff);

    return;
}

static int CV2005_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pCV2005Dev->snsRegsInfo;
    vsi_u32_t gain;
    vsi_u8_t againReg;
    vsi_u16_t dGainReg;
    vsi_u8_t convReg;

    switch(pCV2005Dev->snsModeId) {
        case CV2005_1080P_30FPS_LINEAR_MODE:
            gain = (*pAgain);
            CV2005_CalcGain(&gain, &againReg, &dGainReg, &convReg);
        //os_printf("(%x,%x) ==> (%x,%x) \r\n", *pAgain ,*pDgain, againReg, dGainReg);
            pSnsRegsInfo->snsData[REG_AGAIN_1].data = (againReg & 0xFF);
            pSnsRegsInfo->snsData[REG_DGAIN_2].data = ((dGainReg >> 8) & 0xFF);
            pSnsRegsInfo->snsData[REG_DGAIN_1].data = (dGainReg & 0xFF);
            break;
        default:
            break;
    }

    return BK_OK;
}


static int CV2005_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    vsi_u16_t shutter;

    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pCV2005Dev->snsRegsInfo;
    AE_SNS_DEFAULT_S *pAeSnsDft = &pCV2005Dev->aeDefault;

    // uint32_t new_lines = (((*pIntLine) - 1)/10 + 1) * 10;
    shutter = pAeSnsDft->fullLines - (*pIntLine);
    if(shutter < 10)
        shutter = 10;
    if(shutter > pAeSnsDft->fullLines - 1)
        shutter = pAeSnsDft->fullLines - 1;

    switch(pCV2005Dev->snsModeId) {
        case CV2005_1080P_30FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPTIME_H].data = ((shutter & 0xFF00) >> 8);
            pSnsRegsInfo->snsData[REG_EXPTIME_L].data = (shutter & 0xFF);
            break;
        default:
            break;
    }

    return BK_OK;
}

static int CV2005_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    CV2005_DEVICE_S *pCV2005Dev = CV2005_GetSensorDev(IspPort);
    if (pCV2005Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &pCV2005Dev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int CV2005_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = CV2005_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = CV2005_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = CV2005_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = CV2005_GetSnsRegInfo;

    return BK_OK;
}

ISP_SNS_OBJ_S snsCV2005Obj = {
    .pfnInitIspSnsFunc = CV2005_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = CV2005_InitAeSnsFunc,
};

//###########################################################################################

#define CV2005_WRITE_ADDRESS (0x6A) // I2C 写地址 例如sensor 的 7位 I2C 地址是 0x29 （0x52 >> 1 = 0x29）
#define CHIP_ID_ADDR_HB (0x3003)    // 芯片ID高字节寄存器地址
#define CHIP_ID_ADDR_LB (0x3002)    // 芯片ID低字节寄存器地址
#define CHIP_ID_VAL_HB (0x20)       // 芯片ID高字节值 ('F' 的 ASCII)
#define CHIP_ID_VAL_LB (0x05)       // 芯片ID低字节值 ('S' 的 ASCII)

#define FPS_CTRL_BY_EXP 0 // 通过曝光时间控制帧率
#define FPS_CTRL_BY_LENGTH 1 // 通过帧长和行长控制帧率
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH // 当前使用帧长/行长方式

#define DEFAULT_FRAME_LEN 1125
#define DEFAULT_LINE_LEN 1333
#define CV2005_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 30)

#define WIN_MAX_X 1928
#define WIN_MAX_Y 1088

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define MIPI_CLK_M          240

#define TAG "cv2005"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define CV2005_TABLE_SIZE(table) (sizeof(table) / 4)

avdk_err_t cv2005_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

bool cv2005_read_flag = false;
void mipi_phy_term_set(uint32_t v1, uint32_t v2);

const uint16_t sensor_cv2005_init_table[][2] = 
{
    {0x3031, 0x00},
    {0x3204, 0x40},
    {0x359d, 0x01},
    {0x35b0, 0x50},
    {0x35b1, 0x66},
    {0x3158, 0xFF},
    {0x389D, 0x0A},
    {0x389C, 0x6A},
    {0x38A0, 0x2B},
    {0x3878, 0x01},
    {0x3879, 0x15},
    {0x356f, 0x02},
    {0x36d8, 0x0c},
    {0x36d9, 0x0c},
    {0x3274, 0x00},
    {0x3275, 0x01},
    {0x3510, 0x24},
    {0x3512, 0x80},
    {0x3513, 0x01},
    {0x3109, 0x01},
    {0x3420, 0x3f},
    {0x3422, 0xC7},
    {0x3424, 0x5f},
    {0x3426, 0x87},
    {0x3428, 0x47},
    {0x3538, 0x01},
    {0x3628, 0x66},
    {0x3629, 0x7e},
    {0x3510, 0x7d},
    {0x3512, 0x80},
    {0x3513, 0x01},
    {0x3021, 0x05},
    {0x3020, 0x35},
    {0x3808, 0x4B},
    {0x380a, 0x02},
    {0x301d, 0x04},
    {0x301c, 0x65},
    {0x3834, 0x01},
    {0x3068, 0x22},
    {0x3072, 0xF0},
    {0x3073, 0x07},
    {0x3403, 0x10},
    {0x3842, 0x01},
    {0x3847, 0x01},
    {0x385a, 0x07},
    ////CV2005 Window setting.
    //full_width = 1928
    //full_height = 1088
    //active_width = 1920
    //active_height = 1080
    {0x3030, 0x01},  //DCROP_MODE
    {0x3038, 0x04},  //X_CROP_STA_L
    {0x3039, 0x00},  //X_CROP_STA_H
    {0x303A, 0x80},  //X_CROP_WIDTH_L
    {0x303B, 0x07},  //X_CROP_WIDTH_H
    {0x3034, 0x04},  //Y_DCROP_STA_L
    {0x3035, 0x00},  //Y_DCROP_STA_H
    {0x3036, 0x38},  //Y_DCROP_HEIGHT_L
    {0x3037, 0x04},  //Y_DCROP_HEIGHT_H
    {0x3000, 0x00},
    {0x3A0D, 0x01},
    {0x3A07, 0x01},
    {0x3A07, 0x00},
};

int cv2005_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bk_mipi_csi_ext_set_enable(0);
    //bk_mipi_csi_enable_debug_pin();

    uint32_t size = CV2005_TABLE_SIZE(sensor_cv2005_init_table);

    for (int i = 0; i < size; i++)
    {
        bus->write16(bus, sensor_cv2005_init_table[i][0], sensor_cv2005_init_table[i][1]);
    }

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

int cv2005_set_fps_test(bk_camera_sensor_ctlr_t *controller, uint16_t hts, uint16_t vts)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bus->write16(bus, 0x301D, UINT16_HB(vts));
    bus->write16(bus, 0x301C, UINT16_LB(vts));

    bus->write16(bus, 0x3021, UINT16_HB(hts));
    bus->write16(bus, 0x3020, UINT16_LB(hts));

    return 0;
}

avdk_err_t cv2005_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    uint16_t full_width = 1928;
    uint16_t full_height = 1088;

    if (width > full_width || width <= 0 || height > full_height || height <= 0)
    {
        LOGE("Invalid width/height: %dx%d\n", width, height);
        return AVDK_ERR_INVAL;
    }

    bk_mipi_csi_controller_init(width, height, 0x2b);

    uint8_t WCROP_MODE = 4;
    uint8_t DCROP_MODE = 1;
    uint16_t X_CROP_STA = (full_width - width) >> 2 << 1;
    uint16_t X_CROP_WIDTH = width;

    uint16_t Y_DCROP_STA = (full_height - height) >> 2 << 1;
    uint16_t Y_DCROP_HEIGHT = height;

    if (full_height - height >= 32) {
        WCROP_MODE = 4;
        uint16_t Y_WCROP_STA = Y_DCROP_STA - 8;
        uint16_t Y_WCROP_HEIGHT = Y_DCROP_HEIGHT + 16;
        Y_DCROP_STA = 8;

        bus->write16(bus, 0x3014, WCROP_MODE);
        bus->write16(bus, 0x303C, UINT16_LB(Y_WCROP_STA));
        bus->write16(bus, 0x303D, UINT16_HB(Y_WCROP_STA));
        bus->write16(bus, 0x303E, UINT16_LB(Y_WCROP_HEIGHT));
        bus->write16(bus, 0x303F, UINT16_HB(Y_WCROP_HEIGHT));
    } else {
        WCROP_MODE = 0;
        bus->write16(bus, 0x3014, WCROP_MODE);
    }

    bus->write16(bus, 0x3030, DCROP_MODE);
    bus->write16(bus, 0x3038, UINT16_LB(X_CROP_STA));
    bus->write16(bus, 0x3039, UINT16_HB(X_CROP_STA));
    bus->write16(bus, 0x303A, UINT16_LB(X_CROP_WIDTH));
    bus->write16(bus, 0x303B, UINT16_HB(X_CROP_WIDTH));
    bus->write16(bus, 0x3034, UINT16_LB(Y_DCROP_STA));
    bus->write16(bus, 0x3035, UINT16_HB(Y_DCROP_STA));
    bus->write16(bus, 0x3036, UINT16_LB(Y_DCROP_HEIGHT));
    bus->write16(bus, 0x3037, UINT16_HB(Y_DCROP_HEIGHT));

    return AVDK_ERR_OK;
}

avdk_err_t cv2005_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
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
        // uint16_t vts = CV2005_PCLK / DEFAULT_LINE_LEN / fps;
        // bus->write16(bus, 0x301D, UINT16_HB(vts));
        // bus->write16(bus, 0x301C, UINT16_LB(vts));
        uint16_t line_len = CV2005_PCLK / DEFAULT_FRAME_LEN / fps;
        bus->write16(bus, 0x3021, UINT16_HB(line_len));
        bus->write16(bus, 0x3020, UINT16_LB(line_len));
        //changing framelen changes inter frame time, to do
    }

    if (FPS_CRTL_METHOD == FPS_CTRL_BY_EXP)
    {
        //define by vb min , to do
    }

    return 0;
}

avdk_err_t cv2005_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        os_printf("cv2005 {%04x, %02x}, \r\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        os_printf("cv2005 {%04x, %02x} -> {%04x, %02x} \r\n", addr, dump_val, addr, val);
        bus->write16(bus, addr, val);
    }

    if (cmd == 2) // standy
    {
        bus->write16(bus, 0x3000, 0x01);
    }

    if (cmd == 3) // resume from standy
    {
        bus->write16(bus, 0x3000, 0x00);
    }

    return 0;
}

avdk_err_t cv2005_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");

    LOGI("setformat : width=%d, height=%d, fps=%d\n", format->width, format->height, format->fps);

    cv2005_set_ppi(controller, format->width, format->height);
    cv2005_set_fps(controller, format->fps);

    bk_mipi_csi_controller_reset();

    return AVDK_ERR_OK;
}

#define ID_CV2005 ID_GC2053

const csi_sensor_config_t csi_sensor_cv2005 = { //???
    .name = "cv2005",
    .clk = MCLK_24M,
    .mipi_data_type = 0x2b,
    // .vsync = SYNC_HIGH_LEVEL,
    // .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .default_width = 1920,
    .default_height = 1080,
    .default_fps = 30,
    .id = ID_CV2005,
    .address = (CV2005_WRITE_ADDRESS >> 1),
    .init = cv2005_init,
    .detect = cv2005_detect,
    .set_ppi = cv2005_set_ppi,
    .set_fps = cv2005_set_fps,
    .reg_ctrl = cv2005_ctrl,
};


const ISP_PUB_ATTR_S cv2005_mipi_linear_attr = {    //???
    .pSnsObj      = &snsCV2005Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

void *cv2005_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsCV2005Obj;
}

void *cv2005_get_sensor_cfg_xxx(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

static const bk_camera_sensor_format_t cv2005_format_array[] = {
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
        .fps = 25,
    },
    {
        .width = 1920,
        .height = 1080,
        .fps = 20,
    },

    {
        .width = 1920,
        .height = 1080,
        .fps = 15,
    },

    {
        .width = 640,
        .height = 480,
        .fps = 30,
    },

    {
        .width = 1088,
        .height = 1088,
        .fps = 15,
    },

};

static avdk_err_t cv2005_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &cv2005_format_array[0];
    format_array->size = ARRAY_SIZE(cv2005_format_array);
    return AVDK_ERR_OK;
}

avdk_err_t cv2005_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = CV2005_WRITE_ADDRESS;

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

    config->bus->read16(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read16(config->bus, CHIP_ID_ADDR_LB, &lb_id);

    LOGI("%s read id: 0x%02X%02X\n", __func__, hb_id, lb_id);

    //os_printf("%s, id: 0x%02X%02X\n", __func__, hb_id, lb_id);

    if (hb_id != CHIP_ID_VAL_HB
        || lb_id != CHIP_ID_VAL_LB)
    {
        return AVDK_ERR_GENERIC;
    }

    LOGI("%s success\n", __func__);

    bk_camera_csi_sensor_t *csi_sensor = os_malloc(sizeof(bk_camera_csi_sensor_t));
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    config->bus->write_address = CV2005_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));
    
    csi_sensor->ops.init = cv2005_init;
    csi_sensor->ops.set_format = cv2005_set_format;
    csi_sensor->ops.reg_ctrl = cv2005_ctrl;
    csi_sensor->ops.get_sensor_object = cv2005_get_sensor_object;
    //csi_sensor->ops.get_sensor_cfg = cv2005_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = cv2005_query_support_formats;

    //csi_sensor->isp_pub_attr = &cv2005_mipi_linear_attr;
    //csi_sensor->sensor_config = &csi_sensor_cv2005;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return 0;
}


BK_CAMERA_SENSOR_DETECT_SECTION(cv2005_detect, CSI_CAMERA_PORT);
