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
#include "vsios_i2c.h"

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#define F58_WRITE_ADDRESS 0x80
#define F58_READ_ADDRESS 0x81
#define CHIP_ID_ADDR_HB (0x0A)
#define CHIP_ID_ADDR_LB (0x0B)
#define CHIP_ID_VAL_HB (0x08)
#define CHIP_ID_VAL_LB (0x73)

#define FPS_CTRL_BY_EXP 0 ////controled by exp time
#define FPS_CTRL_BY_LENGTH 1 //controled by framelen and linelen
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH

#define DEFAULT_FRAME_LEN 1125
#define DEFAULT_LINE_LEN 1111
#define F58_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 30)

#define WIN_MAX_X 1928
#define WIN_MAX_Y 1088

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define TAG "f58"

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
#include "f58_1080p_calib.h"

#define F58_REG_BYTE_NUM  1
#define F58_DATA_BYTE_NUM 1

#define F58_EXPTIME_H		0x03
#define F58_EXPTIME_L		0x04
#define F58_DGAIN_1			0xB1
#define F58_DGAIN_2			0xB2
#define F58_AGAIN_1			0xB3
#define F58_AGAIN_2			0xB4
#define F58_AGAIN_3			0xB8
#define F58_AGAIN_4			0xB9

enum F58_REG_INDEX {
	REG_EXPTIME_H		= 0,
	REG_EXPTIME_L		= 1,
	REG_DGAIN_1			= 2,
	REG_DGAIN_2			= 3,
	REG_AGAIN_1			= 4,
	REG_AGAIN_2			= 5,
	REG_AGAIN_3			= 6,
	REG_AGAIN_4			= 7,
};

#define F58_720P_30FPS_LINEAR_MODE (0)

#define F58_VMAX_720P30_LINEAR (1125)

typedef struct vsiF58_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} F58_DEVICE_S;

static F58_DEVICE_S *F58Dev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * F58_720P_CalibParam_dynamic = NULL;

static F58_DEVICE_S *F58_GetSensorDev(ISP_PORT IspPort)
{
    if (F58Dev[IspPort.devId][IspPort.portId] == NULL)
    {
        F58Dev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(F58_DEVICE_S));
        if (F58Dev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d F58Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(F58Dev[IspPort.devId][IspPort.portId], 0 , sizeof(F58_DEVICE_S));
    }

    return F58Dev[IspPort.devId][IspPort.portId];
}

static int F58_SetStream(ISP_PORT IspPort, vsi_bool_t stream);


static int F58_InitRegInfo(ISP_PORT IspPort)
{
	F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return VSI_FAILURE;
    }
	ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pF58Dev->snsRegsInfo;
	pSnsRegsInfo->snsDev = pF58Dev->i2cBus;

	pSnsRegsInfo->addrByteNum = pF58Dev->i2cAttr.reg_bytes;
	pSnsRegsInfo->dataByteNum = pF58Dev->i2cAttr.data_bytes;
	pSnsRegsInfo->slaveAddr   = pF58Dev->i2cAttr.slave_addr;
	pSnsRegsInfo->regCnt = 2;
	pSnsRegsInfo->delayMax = 2;

	pSnsRegsInfo->snsData[REG_EXPTIME_H].delayFrameNum = 2;
	pSnsRegsInfo->snsData[REG_EXPTIME_H].regAddr = F58_EXPTIME_H;
	pSnsRegsInfo->snsData[REG_EXPTIME_L].delayFrameNum = 2;
	pSnsRegsInfo->snsData[REG_EXPTIME_L].regAddr = F58_EXPTIME_L;

	// pSnsRegsInfo->snsData[REG_DGAIN_1].delayFrameNum = 2;
	// pSnsRegsInfo->snsData[REG_DGAIN_1].regAddr = F58_DGAIN_1;
	// pSnsRegsInfo->snsData[REG_DGAIN_2].delayFrameNum = 2;
	// pSnsRegsInfo->snsData[REG_DGAIN_2].regAddr = F58_DGAIN_2;

	// pSnsRegsInfo->snsData[REG_AGAIN_1].delayFrameNum = 2;
	// pSnsRegsInfo->snsData[REG_AGAIN_1].regAddr = F58_AGAIN_1;
	// pSnsRegsInfo->snsData[REG_AGAIN_2].delayFrameNum = 2;
	// pSnsRegsInfo->snsData[REG_AGAIN_2].regAddr = F58_AGAIN_2;
	// pSnsRegsInfo->snsData[REG_AGAIN_3].delayFrameNum = 2;
	// pSnsRegsInfo->snsData[REG_AGAIN_3].regAddr = F58_AGAIN_3;
	// pSnsRegsInfo->snsData[REG_AGAIN_4].delayFrameNum = 2;
	// pSnsRegsInfo->snsData[REG_AGAIN_4].regAddr = F58_AGAIN_4;

	return 0;
}

static int F58_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (F58_720P_CalibParam_dynamic == NULL)
    {
        F58_720P_CalibParam_dynamic = os_malloc(sizeof(F58_720P_CalibParam));
        if (F58_720P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc F58_720P_CalibParam_dynamic\n");
            return -1;
        }
        os_memcpy(F58_720P_CalibParam_dynamic, &F58_720P_CalibParam, sizeof(F58_720P_CalibParam));
    }

    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
    int ret;

    os_memset(pF58Dev, 0, sizeof(*pF58Dev));
    pF58Dev->i2cBus              = snsDev;
    pF58Dev->i2cAttr.slave_addr  = 0x6E;
    pF58Dev->i2cAttr.reg_bytes   = F58_REG_BYTE_NUM;
    pF58Dev->i2cAttr.data_bytes  = F58_DATA_BYTE_NUM;
    F58_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return -1;
    }

    F58_SetStream(IspPort, 0);

    return  0;
}

static int F58_Exit(ISP_PORT IspPort)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }

    vsios_i2c_sys_exit(pF58Dev->i2cBus);

    if (F58_720P_CalibParam_dynamic != NULL)
    {
        os_free(F58_720P_CalibParam_dynamic);
        F58_720P_CalibParam_dynamic = NULL;
    }
    return  0;
}

static int F58_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
	F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
	vsi_u8_t i2cBus = pF58Dev->i2cBus;
	vsios_i2c_attr_t *pI2cAttr = &pF58Dev->i2cAttr;

	// os_printf("i2c write (%x, %x) \r\n", addr, data);
	vsios_i2c_write(i2cBus, pI2cAttr, addr, data);
	// vsios_i2c_read(i2cBus, pI2cAttr, addr);

	return  0;
}

static int F58_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
    vsi_u8_t i2cBus = pF58Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pF58Dev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  0;
}

static int F58_InitAeDefault(ISP_PORT IspPort)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pF58Dev->aeDefault;

    switch (pF58Dev->snsModeId) {
        case F58_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = F58_VMAX_720P30_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 18 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 2;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 1056 * 1024;
            pAeSnsDft->minAgain  = 3 * 1024;
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

    return  0;
}

static int F58_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
    if ((pSnsMode->width == pF58Dev->snsMode.width) &&
        (pSnsMode->height == pF58Dev->snsMode.height) &&
        (pSnsMode->hdrMode == pF58Dev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == pF58Dev->snsMode.stichMode)) {
        return  0;
    }

    if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // F58_Linear1920x1080Init(pF58Dev->i2cBus, &pF58Dev->i2cAttr);
		os_printf("f58 1080p set mode end \r\n");
        os_memcpy(&pF58Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pF58Dev->snsModeId = F58_720P_30FPS_LINEAR_MODE;
        F58_InitAeDefault(IspPort);
    }
	else if ((pSnsMode->width  == 640) &&
        (pSnsMode->height == 480) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR)) {
        // F58_Linear1920x1080Init(pF58Dev->i2cBus, &pF58Dev->i2cAttr);
		os_printf("f58 480p set mode end \r\n");
        os_memcpy(&pF58Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pF58Dev->snsModeId = F58_720P_30FPS_LINEAR_MODE;
        F58_InitAeDefault(IspPort);
    } else {
        os_printf("f58 custom set mode end ~~~\r\n");
        os_memcpy(&pF58Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
        pF58Dev->snsModeId = F58_720P_30FPS_LINEAR_MODE;
        F58_InitAeDefault(IspPort);
    }

    return  0;
}

static int F58_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }

    if (stream) {
        // F58_WriteReg(IspPort, 0x3253, 0x00);
        // F58_WriteReg(IspPort, 0x3012, 1);
    } else {
        // F58_WriteReg(IspPort, 0x3012, 0);
    }

    pF58Dev->stream = stream;

    return  0;
}

static int F58_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, F58_720P_CalibParam_dynamic);

    return  0;
}

static int F58_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = F58_Init;
    pIspSnsFunc->pfnSensorExit    = F58_Exit;
    pIspSnsFunc->pfnWriteReg      = F58_WriteReg;
    pIspSnsFunc->pfnReadReg       = F58_ReadReg;
    pIspSnsFunc->pfnSetMode       = F58_SetMode;
    pIspSnsFunc->pfnSetStream     = F58_SetStream;
    pIspSnsFunc->pfnSetIspDefault = F58_SetIspDefault;

    return  0;
}

static int F58_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }

    os_memcpy(pAeSnsDft, &pF58Dev->aeDefault, sizeof(*pAeSnsDft));

    return  0;
}


static void F58_CalcGain(vsi_u32_t *pGain, vsi_u8_t *pAgainReg, vsi_u16_t *pDGainReg, vsi_u8_t *pConvReg)
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
}

static int F58_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
    //ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pF58Dev->snsRegsInfo;
    vsi_u32_t gain;
    vsi_u8_t againReg;
    vsi_u16_t dGainReg;
    vsi_u8_t convReg;

    switch(pF58Dev->snsModeId) {
        case F58_720P_30FPS_LINEAR_MODE:
            gain = (*pAgain);
            F58_CalcGain(&gain, &againReg, &dGainReg, &convReg);
            *pAgain = gain;
            *pDgain = 1024;
            // pSnsRegsInfo->snsData[REG_AGAIN].data = ((againReg & 0x03) | (convReg << 6));
            // pSnsRegsInfo->snsData[REG_DGAIN_HCG_H].data = ((dGainReg >> 8) & 0xFF);
            // pSnsRegsInfo->snsData[REG_DGAIN_HCG_L].data = (dGainReg & 0xFF) ;
            break;
        default:
            break;
    }

    return 0;
}


static int F58_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pF58Dev->snsRegsInfo;

	// uint32_t new_lines = (((*pIntLine) - 1)/10 + 1) * 10;


    switch(pF58Dev->snsModeId) {
        case F58_720P_30FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPTIME_H].data = ((*pIntLine & 0xFF00) >> 8);
            pSnsRegsInfo->snsData[REG_EXPTIME_L].data = (*pIntLine & 0xFF);
            break;
        default:
            break;
    }

    return  0;
}

static int F58_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    F58_DEVICE_S *pF58Dev = F58_GetSensorDev(IspPort);
    if (pF58Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return -1;
    }

    os_memcpy(pSnsRegsInfo, &pF58Dev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return  0;
}

static int F58_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = F58_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = F58_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = F58_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = F58_GetSnsRegInfo;

    return  0;
}

const ISP_SNS_OBJ_S snsF58Obj = {
    .pfnInitIspSnsFunc = F58_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = F58_InitAeSnsFunc,
};

#endif
//#####################################################################################################

#define F58_TABLE_SIZE(table) (sizeof(table) / 2)

avdk_err_t f58_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config);

#if 0
static const uint8_t sensor_f58_1920x1080_30fps_24Mhz_init_table[][2] =
{
    {0x12, 0x40},
    {0x48, 0xB3},
    {0x48, 0x33},
    {0x0E, 0x11},
    {0x0F, 0x0C},
    {0x10, 0x24},
    {0x11, 0x80},
    {0x57, 0x60},
    {0x58, 0x18},
    {0x5F, 0x01},
    {0x46, 0x18},
    {0x0D, 0xD0},
    {0x20, 0x00},
    {0x21, 0x05},
    {0x22, 0x65},
    {0x23, 0x04},
    {0x24, 0xC0},
    {0x25, 0x38},
    {0x26, 0x43},
    {0x27, 0x60},
    {0x28, 0x15},
    {0x29, 0x04},
    {0x2A, 0x51},
    {0x2B, 0x14},
    {0x2C, 0x02},
    {0x2D, 0x00},
    {0x2E, 0x14},
    {0x2F, 0x44},
    {0x41, 0xC4},
    {0x42, 0x03},
    {0x47, 0x42},
    {0x76, 0x60},
    {0x77, 0x09},
    {0x80, 0x01},
    {0xAF, 0x22},
    {0xAB, 0x00},
    {0x1D, 0x00},
    {0x1E, 0x04},
    {0x6C, 0x40},
    {0x70, 0x8D},
    {0x71, 0x4D},
    {0x72, 0x6C},
    {0x73, 0x36},
    {0x74, 0x02},
    {0x78, 0x9B},
    {0x89, 0x01},
    {0x6B, 0x00},
    {0x86, 0x00},
    {0x30, 0x8D},
    {0x31, 0x12},
    {0x32, 0x2F},
    {0x33, 0x20},
    {0x34, 0x3F},
    {0x35, 0x3F},
    {0x3A, 0xA0},
    {0x56, 0x80},
    {0x59, 0x50},
    {0x5A, 0x88},
    {0x61, 0x18},
    {0x64, 0xC2},
    {0x85, 0x48},
    {0x8A, 0x20},
    {0x90, 0x02},
    {0x91, 0x01},
    {0x94, 0xE0},
    {0x9B, 0x8F},
    {0xA6, 0x00},
    {0xA7, 0x80},
    {0xA9, 0x48},
    {0xB6, 0x00},
    {0xBF, 0x01},
    {0x5A, 0x19},
    {0x5D, 0x84},
    {0x5E, 0x90},
    {0x5F, 0x40},
    {0xBF, 0x00},
    {0x45, 0x09},
    {0x5B, 0xA6},
    {0x5C, 0x24},
    {0x5D, 0x42},
    {0x5E, 0xC3},
    {0x65, 0x32},
    {0x66, 0x10},
    {0x67, 0x32},
    {0x68, 0x50},
    {0x69, 0x70},
    {0x6A, 0x25},
    {0x7A, 0x88},
    {0x8D, 0x67},
    {0x8F, 0x90},
    {0x9E, 0x70},
    {0xA3, 0x11},
    {0xA4, 0x87},
    {0xA5, 0xA7},
    {0xB8, 0x41},
    {0xB9, 0x01},
    {0xBA, 0xF9},
    {0xBB, 0x02},
    {0xBF, 0x01},
    {0x4C, 0x0D},
    {0x5F, 0xC0},
    {0x64, 0x84},
    {0x65, 0x10},
    {0x66, 0x40},
    {0x67, 0x70},
    {0x6F, 0x40},
    {0xBF, 0x00},
    {0x13, 0x81},
    {0x4A, 0x01},
    {0xB1, 0x04},
    {0x50, 0x02},
    {0x49, 0x40},
    {0xBF, 0x01},
    {0x5E, 0x10},
    {0x6F, 0x52},
    {0xBF, 0x00},
    {0xBC, 0x11},
    {0x82, 0x00},
    {0x19, 0x20},
    {0x12, 0x00},
};
#endif

static const uint8_t sensor_f58_1920x1080_30fps_20Mhz_init_table[][2] =
{
    {0x12, 0x40},
    {0x48, 0xB3},
    {0x48, 0x33},
    {0x0E, 0x11},
    {0x0F, 0x0C},
    {0x10, 0x2A},
    {0x11, 0x80},
    {0x57, 0x60},
    {0x58, 0x18},
    {0x5F, 0x01},
    {0x46, 0x18},
    {0x0D, 0xD0},
    {0x20, 0x00},
    {0x21, 0x05},
    {0x22, 0x65},
    {0x23, 0x04},
    {0x24, 0xC0},
    {0x25, 0x38},
    {0x26, 0x43},
    {0x27, 0x72},
    {0x28, 0x15},
    {0x29, 0x04},
    {0x2A, 0x63},
    {0x2B, 0x14},
    {0x2C, 0x02},
    {0x2D, 0x00},
    {0x2E, 0x14},
    {0x2F, 0x44},
    {0x41, 0xC4},
    {0x42, 0x03},
    {0x47, 0x42},
    {0x76, 0x60},
    {0x77, 0x09},
    {0x80, 0x01},
    {0xAF, 0x22},
    {0xAB, 0x00},
    {0x1D, 0x00},
    {0x1E, 0x04},
    {0x6C, 0x40},
    {0x70, 0x8D},
    {0x71, 0x4D},
    {0x72, 0x6C},
    {0x73, 0x36},
    {0x74, 0x02},
    {0x78, 0x9B},
    {0x89, 0x01},
    {0x6B, 0x00},
    {0x86, 0x00},
    {0x90, 0x04},
    {0x91, 0x01},
    {0x30, 0x8D},
    {0x31, 0x12},
    {0x32, 0x2F},
    {0x33, 0x20},
    {0x34, 0x3F},
    {0x35, 0x3F},
    {0x3A, 0xA0},
    {0x3B, 0x52},
    {0x3C, 0x52},
    {0x56, 0x80},
    {0x59, 0x50},
    {0x5A, 0x88},
    {0x61, 0x18},
    {0x64, 0xC2},
    {0x85, 0x50},
    {0x8A, 0x20},
    {0x94, 0xE0},
    {0x9B, 0x8F},
    {0x9C, 0x21},
    {0xA6, 0x00},
    {0xA7, 0x80},
    {0xA9, 0x48},
    {0xB6, 0x00},
    {0xBF, 0x01},
    {0x5A, 0x19},
    {0x5D, 0x84},
    {0x5E, 0x90},
    {0x5F, 0x40},
    {0xBF, 0x00},
    {0x45, 0x09},
    {0x5B, 0xA0},
    {0x5C, 0x0C},
    {0x5D, 0x41},
    {0x5E, 0xC3},
    {0x65, 0x32},
    {0x66, 0x10},
    {0x67, 0x32},
    {0x68, 0x50},
    {0x69, 0x70},
    {0x6A, 0x23},
    {0x7A, 0x88},
    {0x8D, 0x67},
    {0x8F, 0x90},
    {0x9E, 0x70},
    {0xA3, 0x11},
    {0xA4, 0x87},
    {0xA5, 0xA7},
    {0xB8, 0x25},
    {0xB9, 0x01},
    {0xBA, 0xF9},
    {0xBB, 0x02},
    {0xBF, 0x01},
    {0x4C, 0x0D},
    {0x5F, 0xC0},
    {0x64, 0x84},
    {0x65, 0x10},
    {0x66, 0x40},
    {0x67, 0x00},
    {0x6F, 0x40},
    {0xBF, 0x00},
    {0x13, 0x81},
    {0x4A, 0x01},
    {0xB1, 0x04},
    {0x50, 0x02},
    {0x49, 0x40},
    {0xBF, 0x01},
    {0x5E, 0x90},
    {0x65, 0x90},
    {0x5F, 0xC1},
    {0x66, 0x41},
    {0x6F, 0x52},
    {0xBF, 0x00},
    {0xBC, 0x11},
    {0x82, 0x00},
    {0x19, 0x20},
    {0x12, 0x00},
};

static avdk_err_t f58_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bk_mipi_csi_ext_set_enable(0);

    uint32_t size = F58_TABLE_SIZE(sensor_f58_1920x1080_30fps_20Mhz_init_table);

    for (int i = 0; i < size; i++)
    {
        bus->write8(bus, sensor_f58_1920x1080_30fps_20Mhz_init_table[i][0], sensor_f58_1920x1080_30fps_20Mhz_init_table[i][1]);
    }

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static avdk_err_t f58_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    //bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (width > 1928 || width <= 0 || height > 1088 || height <= 0)
    {
        LOGE("%s, %d width: %d, height: %d, fail...\n", __func__, __LINE__, width, height);
        return -1;
    }

    bk_mipi_csi_controller_init(width, width, 0x2b);
    return 0;
}

static avdk_err_t f58_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    //bk_camera_bus_t *bus = csi_sensor->config.bus;

    return 0;
}

static avdk_err_t f58_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    f58_set_ppi(controller, format->width, format->height);
    f58_set_fps(controller, format->fps);
    return AVDK_ERR_OK;
}

static avdk_err_t f58_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg read
    {
        uint8_t dump_val;
        bus->read8(bus, addr, &dump_val);
        LOGI("f58 {%04x, %02x}\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read8(bus, addr, &dump_val);
        LOGI("f58 {%04x, %02x} -> {%04x, %02x}\n", addr, dump_val, addr, val);
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

static const csi_sensor_config_t csi_sensor_f58 =
{
    .name = "f58",
    .clk = MCLK_24M,
    .mipi_data_type = 0x2b,
    // .vsync = SYNC_HIGH_LEVEL,
    // .hsync = SYNC_HIGH_LEVEL,
    /* default config */
    .default_width = 1920,
    .default_height = 1080,
    .default_fps = 30,
    .id = 0,
    .address = (F58_WRITE_ADDRESS >> 1),
    .init = f58_init,
    .detect = f58_detect,
    .set_ppi = f58_set_ppi,
    .set_fps = f58_set_fps,
    .reg_ctrl = f58_ctrl,
    // .power_down = gc2145_reset,
};

static const ISP_PUB_ATTR_S f58_mipi_linear_attr = {
    .pSnsObj      = (void*)&snsF58Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_RGGB10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t f58_format_array[] = {
    {
        .width = 1920,
        .height = 1080,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_RGGB10,
    }
};

static avdk_err_t f58_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &f58_format_array[0];
    format_array->size = ARRAY_SIZE(f58_format_array);
    return AVDK_ERR_OK;
}

static void *f58_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsF58Obj;
}

static void *f58_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

avdk_err_t f58_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = F58_WRITE_ADDRESS;

    AVDK_RETURN_ON_FALSE(config->bus, AVDK_ERR_GENERIC, TAG, "bus is NULL");

    /* disbale camera power */
    gpio_dev_unmap(config->pin_pwdn);
    BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_pwdn));
    bk_gpio_set_capacity(config->pin_pwdn, GPIO_DRIVER_CAPACITY_3);

    gpio_dev_unmap(config->pin_reset);
    BK_LOG_ON_ERR(bk_gpio_enable_output(config->pin_reset));
    bk_gpio_set_capacity(config->pin_reset, GPIO_DRIVER_CAPACITY_3);

    bk_gpio_set_output_high(config->pin_pwdn);
    bk_gpio_set_output_low(config->pin_reset);
    bk_gpio_set_output_high(config->pin_reset);
    rtos_delay_milliseconds(100);
    bk_gpio_set_output_low(config->pin_reset);
    rtos_delay_milliseconds(20);
    bk_gpio_set_output_high(config->pin_reset);

    bk_gpio_set_output_low(config->pin_pwdn);
    rtos_delay_milliseconds(100);

    config->bus->read8(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read8(config->bus, CHIP_ID_ADDR_LB, &lb_id);

    config->bus->read8(config->bus, CHIP_ID_ADDR_HB, &hb_id);
    config->bus->read8(config->bus, CHIP_ID_ADDR_LB, &lb_id);


    LOGI("%s, id: 0x%02X%02X\n", __func__, hb_id, lb_id);

    if (hb_id != CHIP_ID_VAL_HB
        || lb_id != CHIP_ID_VAL_LB)
    {
        return AVDK_ERR_GENERIC;
    }

    LOGI("%s success\n", __func__);

    bk_camera_csi_sensor_t *csi_sensor = os_malloc(sizeof(bk_camera_csi_sensor_t));
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(csi_sensor, 0, sizeof(bk_camera_csi_sensor_t));
    config->bus->write_address = F58_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = f58_init;
    csi_sensor->ops.set_format = f58_set_format;
    csi_sensor->ops.reg_ctrl = f58_ctrl;
    csi_sensor->ops.get_sensor_object = f58_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = f58_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = f58_query_support_formats;

    csi_sensor->isp_pub_attr = &f58_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_f58;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}


BK_CAMERA_SENSOR_DETECT_SECTION(f58_detect, CSI_CAMERA_PORT);
