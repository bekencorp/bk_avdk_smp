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

#include "vsios_i2c.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_calib.h"
#include "gc2053_1080p_calib.h"

#define TAG "SC4336"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SC4336_WRITE_ADDRESS (0x60)
#define SC4336_READ_ADDRESS (0x64)
#define CHIP_ID_ADDR_HB (0x3107)
#define CHIP_ID_ADDR_LB (0x3108)
#define CHIP_ID_VAL_HB (0xdc)
#define CHIP_ID_VAL_LB (0x42)

#define SENSOR_OUTPUT_MIN_FPS 5
#define FPS_CTRL_BY_EXP 0 ////controled by exp time
#define FPS_CTRL_BY_LENGTH 1 //controled by framelen and linelen
#define FPS_CRTL_METHOD FPS_CTRL_BY_LENGTH

#define DEFAULT_FRAME_LEN 1500
#define DEFAULT_LINE_LEN 2800
#define SC4336_PCLK (DEFAULT_FRAME_LEN * DEFAULT_LINE_LEN * 30)

#define WIN_MAX_X 2568
#define WIN_MAX_Y 1448

#define SC4336_REG_BYTE_NUM  2
#define SC4336_DATA_BYTE_NUM 1
#define SC4336_REG_EXPOSURE_H 0x3e00
#define SC4336_REG_EXPOSURE_M 0x3e01
#define SC4336_REG_EXPOSURE_L 0x3e02
#define	SC4336_EXPOSURE_MIN 1
#define	SC4336_EXPOSURE_STEP 1
#define SC4336_VTS_MAX 0x7fff
#define SC4336_REG_VTS_H 0x320e
#define SC4336_REG_VTS_L 0x320f
#define SC4336_REG_MIRROR_FLIP 0x3221
#define SC4336_MIRROR_BITS     ((1U << 1) | (1U << 2))
#define SC4336_VFLIP_BITS      ((1U << 5) | (1U << 6))
#define SC4336_REG_DIG_GAIN 0x3e06
#define SC4336_REG_DIG_FINE_GAIN 0x3e07
#define SC4336_REG_ANA_GAIN	 0x3e09
#define SC4336_GAIN_MIN 0x0020
#define SC4336_GAIN_MAX (32 * 15 * 32)
#define SC4336_GAIN_STEP 1
#define SC4336_GAIN_DEFAULT 0x20

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

enum SC4336_REG_INDEX {
    REG_VTS_H       = 0,
    REG_VTS_L       = 1,
    REG_EXPOSURE_H  = 2,
    REG_EXPOSURE_M  = 3,
    REG_EXPOSURE_L  = 4,
    REG_DIG_GAIN     = 5,
    REG_DIG_FINE_GAIN = 6,
    REG_ANA_GAIN     = 7,
};

#define SC4336_720P_30FPS_LINEAR_MODE (0)

#define SC4336_VMAX_720P30_LINEAR (1125)

typedef struct vsiSC4336_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} SC4336_DEVICE_S;

typedef struct
{
    uint8_t val1;
    uint8_t val2;
    uint8_t val3;
    uint8_t val4;
} sc4336_again_val_t;

static SC4336_DEVICE_S *SC4336Dev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};

static ISP_CALIB_DATA_S * SC4336_720P_CalibParam_dynamic = NULL;

static const uint16_t sensor_sc4336_init_table[][2] = {
    {0x0103, 0x01},
    {0x36e9, 0x80},
    {0x37f9, 0x80},
    {0x301f, 0x03},
    {0x30b8, 0x44},
    {0x320e, 0x05},
    {0x320f, 0xdc},//30fps
    {0x3253, 0x10},
    {0x3301, 0x0a},
    {0x3302, 0xff},
    {0x3305, 0x00},
    {0x3306, 0x90},
    {0x3308, 0x20},
    {0x330a, 0x01},
    {0x330b, 0xb0},
    {0x330d, 0xf0},
    {0x3333, 0x10},
    {0x335e, 0x06},
    {0x335f, 0x0a},
    {0x3364, 0x5e},
    {0x337d, 0x0e},
    {0x3390, 0x08},
    {0x3391, 0x09},
    {0x3392, 0x0f},
    {0x3393, 0x18},
    {0x3394, 0x60},
    {0x3395, 0xff},
    {0x3396, 0x08},
    {0x3397, 0x09},
    {0x3398, 0x0f},
    {0x37f9, 0x80},
    {0x301f, 0x03},
    {0x30b8, 0x44},
    {0x320e, 0x05},
    {0x320f, 0xdc},//30fps
    {0x3253, 0x10},
    {0x3301, 0x0a},
    {0x3302, 0xff},
    {0x3305, 0x00},
    {0x3306, 0x90},
    {0x3308, 0x20},
    {0x330a, 0x01},
    {0x330b, 0xb0},
    {0x330d, 0xf0},
    {0x3333, 0x10},
    {0x335e, 0x06},
    {0x335f, 0x0a},
    {0x3364, 0x5e},
    {0x337d, 0x0e},
    {0x3390, 0x08},
    {0x3391, 0x09},
    {0x3392, 0x0f},
    {0x3393, 0x18},
    {0x3394, 0x60},
    {0x3395, 0xff},
    {0x3396, 0x08},
    {0x3397, 0x09},
    {0x3398, 0x0f},
    {0x3399, 0x0a},
    {0x339a, 0x18},
    {0x339b, 0x60},
    {0x339c, 0xff},
    {0x33a2, 0x04},
    {0x33ad, 0x3c},
    {0x33b2, 0x40},
    {0x33b3, 0x30},
    {0x33f8, 0x00},
    {0x33f9, 0xa0},
    {0x33fa, 0x00},
    {0x33fb, 0xe0},
    {0x33fc, 0x09},
    {0x33fd, 0x1f},
    {0x349f, 0x03},
    {0x34a6, 0x09},
    {0x34a7, 0x1f},
    {0x34a8, 0x28},
    {0x34a9, 0x28},
    {0x34aa, 0x01},
    {0x34ab, 0xd0},
    {0x34ac, 0x02},
    {0x34ad, 0x10},
    {0x34f8, 0x1f},
    {0x34f9, 0x20},
    {0x3630, 0xc0},
    {0x3631, 0x84},
    {0x3633, 0x44},
    {0x3637, 0x4c},
    {0x3641, 0x38},
    {0x3670, 0x56},
    {0x3674, 0xc0},
    {0x3675, 0xa0},
    {0x3676, 0xa0},
    {0x3677, 0x84},
    {0x3678, 0x88},
    {0x3679, 0x8a},
    {0x367c, 0x09},
    {0x367d, 0x0b},
    {0x367e, 0x08},
    {0x367f, 0x0f},
    {0x3696, 0x44},
    {0x3697, 0x54},
    {0x3698, 0x54},
    {0x36a0, 0x0f},
    {0x36a1, 0x1f},
    {0x36b0, 0x81},
    {0x36b1, 0x83},
    {0x36b2, 0x85},
    {0x36b3, 0x8b},
    {0x36b4, 0x09},
    {0x36b5, 0x0b},
    {0x36b6, 0x0f},
    {0x36ea, 0x07},
    {0x36eb, 0x04},
    {0x36ec, 0x0c},
    {0x36ed, 0xaa},
    {0x370f, 0x01},
    {0x3722, 0x09},
    {0x3724, 0x21},
    {0x3771, 0x09},
    {0x3772, 0x05},
    {0x3773, 0x05},
    {0x377a, 0x0f},
    {0x377b, 0x1f},
    {0x37fa, 0x07},
    {0x37fb, 0x31},
    {0x37fc, 0x11},
    {0x37fd, 0x16},
    {0x3905, 0x8c},
    {0x391d, 0x04},
    {0x3926, 0x21},
    {0x3933, 0x80},
    {0x3934, 0x03},
    {0x3935, 0x00},
    {0x3936, 0x1b},
    {0x3937, 0x76},
    {0x3938, 0x75},
    {0x3939, 0x00},
    {0x393a, 0x00},
    {0x39dc, 0x02},
    {0x3e00, 0x00},
    {0x3e01, 0x5d},
    {0x3e02, 0x40},
    {0x440e, 0x02},
    {0x4509, 0x28},
    {0x450d, 0x32},
    {0x5000, 0x06},
    {0x5799, 0x46},
    {0x579a, 0x77},
    {0x57d9, 0x46},
    {0x57da, 0x77},
    {0x5ae0, 0xfe},
    {0x5ae1, 0x40},
    {0x5ae2, 0x38},
    {0x5ae3, 0x30},
    {0x5ae4, 0x28},
    {0x5ae5, 0x38},
    {0x5ae6, 0x30},
    {0x5ae7, 0x28},
    {0x5ae8, 0x3f},
    {0x5ae9, 0x34},
    {0x5aea, 0x2c},
    {0x5aeb, 0x3f},
    {0x5aec, 0x34},
    {0x5aed, 0x2c},
    {0x36e9, 0x53},
    {0x37f9, 0x23},
    {0x0100, 0x01},
};

#define SC4336_TABLE_SIZE(table) (sizeof(table) / sizeof(table[0]))

static int SC4336_SetStream(ISP_PORT IspPort, vsi_bool_t stream);

static SC4336_DEVICE_S *SC4336_GetSensorDev(ISP_PORT IspPort)
{
    if (SC4336Dev[IspPort.devId][IspPort.portId] == NULL)
    {
        SC4336Dev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(SC4336_DEVICE_S));
        if (SC4336Dev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d GC2053Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(SC4336Dev[IspPort.devId][IspPort.portId], 0 , sizeof(SC4336_DEVICE_S));
    }

    return SC4336Dev[IspPort.devId][IspPort.portId];
}


static int SC4336_InitRegInfo(ISP_PORT IspPort)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pSC4336Dev->snsRegsInfo;
    pSnsRegsInfo->snsDev = pSC4336Dev->i2cBus;

    pSnsRegsInfo->addrByteNum = pSC4336Dev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = pSC4336Dev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = pSC4336Dev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 8;
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_VTS_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_VTS_H].regAddr = SC4336_REG_VTS_H;
    pSnsRegsInfo->snsData[REG_VTS_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_VTS_L].regAddr = SC4336_REG_VTS_L;

    pSnsRegsInfo->snsData[REG_EXPOSURE_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPOSURE_H].regAddr = SC4336_REG_EXPOSURE_H;
    pSnsRegsInfo->snsData[REG_EXPOSURE_M].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPOSURE_M].regAddr = SC4336_REG_EXPOSURE_M;
    pSnsRegsInfo->snsData[REG_EXPOSURE_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPOSURE_L].regAddr = SC4336_REG_EXPOSURE_L;

    pSnsRegsInfo->snsData[REG_DIG_GAIN].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DIG_GAIN].regAddr = SC4336_REG_DIG_GAIN;
    pSnsRegsInfo->snsData[REG_DIG_FINE_GAIN].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_DIG_FINE_GAIN].regAddr = SC4336_REG_DIG_FINE_GAIN;
    pSnsRegsInfo->snsData[REG_ANA_GAIN].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_ANA_GAIN].regAddr = SC4336_REG_ANA_GAIN;

    return BK_OK;
}

static int SC4336_Init(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (SC4336_720P_CalibParam_dynamic == NULL)
    {
        SC4336_720P_CalibParam_dynamic = os_malloc(sizeof(GC2053_720P_CalibParam));
        if (SC4336_720P_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc SC4336_720P_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(SC4336_720P_CalibParam_dynamic, &GC2053_720P_CalibParam, sizeof(GC2053_720P_CalibParam));
    }

    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(pSC4336Dev, 0, sizeof(*pSC4336Dev));
    pSC4336Dev->i2cBus              = snsDev;
    pSC4336Dev->i2cAttr.slave_addr  = SC4336_WRITE_ADDRESS;
    pSC4336Dev->i2cAttr.reg_bytes   = SC4336_REG_BYTE_NUM;
    pSC4336Dev->i2cAttr.data_bytes  = SC4336_DATA_BYTE_NUM;
    SC4336_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return BK_FAIL;
    }

    SC4336_SetStream(IspPort, 0);

    return  BK_OK;
}

static int SC4336_Exit(ISP_PORT IspPort)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    vsios_i2c_sys_exit(pSC4336Dev->i2cBus);

    if (SC4336_720P_CalibParam_dynamic != NULL)
    {
        os_free(SC4336_720P_CalibParam_dynamic);
        SC4336_720P_CalibParam_dynamic = NULL;
    }
    return  BK_OK;
}

static int SC4336_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pSC4336Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pSC4336Dev->i2cAttr;

    // LOGD("i2c write (%x, %x) \r\n", addr, data);
    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);
    // vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int SC4336_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = pSC4336Dev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &pSC4336Dev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int SC4336_InitAeDefault(ISP_PORT IspPort)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &pSC4336Dev->aeDefault;

    switch (pSC4336Dev->snsModeId) {
        case SC4336_720P_30FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesMax = 0xFFFF;
            pAeSnsDft->fullLinesStd = SC4336_VMAX_720P30_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 30 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 2;
            pAeSnsDft->minIntLine  = 1;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 1056 * 1024;
            pAeSnsDft->minAgain  = 0x40;
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

static int SC4336_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if ((pSnsMode->width == pSC4336Dev->snsMode.width) &&
        (pSnsMode->height == pSC4336Dev->snsMode.height) &&
        (pSnsMode->hdrMode == pSC4336Dev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == pSC4336Dev->snsMode.stichMode)) {
        return BK_OK;
    }

    LOGI("sc4336 custom set mode end ~~~\n");
    os_memcpy(&pSC4336Dev->snsMode, pSnsMode, sizeof(*pSnsMode));
    pSC4336Dev->snsModeId = SC4336_720P_30FPS_LINEAR_MODE;
    SC4336_InitAeDefault(IspPort);

    return BK_OK;
}

static int SC4336_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        // SC4336_WriteReg(IspPort, 0x3253, 0x00);
        // SC4336_WriteReg(IspPort, 0x3012, 1);
    } else {
        // SC4336_WriteReg(IspPort, 0x3012, 0);
    }

    pSC4336Dev->stream = stream;

    return BK_OK;
}

static int SC4336_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, SC4336_720P_CalibParam_dynamic);

    return BK_OK;
}

static int SC4336_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &pSC4336Dev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

static int SC4336_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &pSC4336Dev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int SC4336_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &pSC4336Dev->snsRegsInfo;

    // uint32_t new_lines = (((*pIntLine) - 1)/10 + 1) * 10;

    switch(pSC4336Dev->snsModeId) {
        case SC4336_720P_30FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPOSURE_H].data = ((*pIntLine & 0xF000) >> 12);
            pSnsRegsInfo->snsData[REG_EXPOSURE_M].data = ((*pIntLine & 0x0FF0) >> 4);
            pSnsRegsInfo->snsData[REG_EXPOSURE_L].data = (*pIntLine & 0x000F);
            break;
        default:
            break;
    }

    return BK_OK;
}

static void SC4336_CalcGain(vsi_u32_t *pAgain, vsi_u32_t *pDgain, vsi_u32_t *pFdgain)
{
    vsi_u32_t coarse_again = 0, coarse_dgian = 0, fine_dgian = 0;
    vsi_u32_t gain_factor;

    vsi_u32_t gain = *pAgain;

    if (gain < 32)
        gain = 32;
    else if (gain > SC4336_GAIN_MAX)
        gain = SC4336_GAIN_MAX;

    gain_factor = gain * 1000 / 32;
    if (gain_factor < 2000) {
        coarse_again = 0x00;
        coarse_dgian = 0x00;
        fine_dgian = gain_factor * 128 / 1000;
    } else if (gain_factor < 4000) {
        coarse_again = 0x08;
        coarse_dgian = 0x00;
        fine_dgian = gain_factor * 128 / 2000;
    } else if (gain_factor < 8000) {
        coarse_again = 0x09;
        coarse_dgian = 0x00;
        fine_dgian = gain_factor * 128 / 4000;
    } else if (gain_factor < 16000) {
        coarse_again = 0x0b;
        coarse_dgian = 0x00;
        fine_dgian = gain_factor * 128 / 8000;
    } else if (gain_factor < 32000) {
        coarse_again = 0x0f;
        coarse_dgian = 0x00;
        fine_dgian = gain_factor * 128 / 16000;
    } else if (gain_factor < 32000 * 2) {
        coarse_again = 0x1f;
        coarse_dgian = 0x00;
        fine_dgian = gain_factor * 128 / 32000;
    } else if (gain_factor < 32000 * 4) {
        //open dgain begin  max digital gain 4X
        coarse_again = 0x1f;
        coarse_dgian = 0x01;
        fine_dgian = gain_factor * 128 / 32000 / 2;
    } else if (gain_factor < 32000 * 8) {
        coarse_again = 0x1f;
        coarse_dgian = 0x03;
        fine_dgian = gain_factor * 128 / 32000 / 4;
    } else if (gain_factor < 32000 * 15) {
        coarse_again = 0x1f;
        coarse_dgian = 0x07;
        fine_dgian = gain_factor * 128 / 32000 / 8;
    } else {
        coarse_again = 0x1f;
        coarse_dgian = 0x07;
        fine_dgian = 0xf0;
    }

    *pDgain = coarse_dgian;
    *pAgain = coarse_again;
    *pFdgain = fine_dgian;
}

static int SC4336_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    SC4336_DEVICE_S *pSC4336Dev = SC4336_GetSensorDev(IspPort);
    if (pSC4336Dev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    vsi_u32_t fdgain = 0;

    switch(pSC4336Dev->snsModeId) {
        case SC4336_720P_30FPS_LINEAR_MODE:
            // LOGI("%s, before gain update (%x,%x) \r\n", __func__, *pAgain ,*pDgain);
            SC4336_CalcGain(pAgain, pDgain, &fdgain);
            // LOGI("%s, after gain update (%x,%x,%x) \r\n", __func__, *pAgain ,*pDgain, fdgain);
            pSC4336Dev->snsRegsInfo.snsData[REG_ANA_GAIN].data = *pAgain;
            pSC4336Dev->snsRegsInfo.snsData[REG_DIG_GAIN].data = (*pDgain);
            pSC4336Dev->snsRegsInfo.snsData[REG_DIG_FINE_GAIN].data = fdgain;
            break;
        default:
            break;
    }

    return BK_OK;
}

static int SC4336_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = SC4336_Init;
    pIspSnsFunc->pfnSensorExit    = SC4336_Exit;
    pIspSnsFunc->pfnWriteReg      = SC4336_WriteReg;
    pIspSnsFunc->pfnReadReg       = SC4336_ReadReg;
    pIspSnsFunc->pfnSetMode       = SC4336_SetMode;
    pIspSnsFunc->pfnSetStream     = SC4336_SetStream;
    pIspSnsFunc->pfnSetIspDefault = SC4336_SetIspDefault;

    return AVDK_ERR_OK;
}

static int SC4336_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = SC4336_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = NULL;
    pAeSnsFunc->pfnSlowFrameRate = NULL;
    pAeSnsFunc->pfnIntTimeUpdate = SC4336_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = SC4336_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = SC4336_GetSnsRegInfo;

    return AVDK_ERR_OK;
}

const ISP_SNS_OBJ_S snsSC4336Obj = {
    .pfnInitIspSnsFunc = SC4336_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = SC4336_InitAeSnsFunc,
};


static const ISP_PUB_ATTR_S sc4336_mipi_linear_attr = {
    .pSnsObj      = (void*)&snsSC4336Obj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_BGGR10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t sc4336_format_array[] = {
    {
        .width = 2560,
        .height = 1440,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },

    {
        .width = 2560,
        .height = 1440,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },

    {
        .width = 1280,
        .height = 720,
        .fps = 30,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },

    {
        .width = 1088,
        .height = 1088,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },
};

static avdk_err_t sc4336_init(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    bk_mipi_csi_ext_set_enable(0);

    uint32_t size = SC4336_TABLE_SIZE(sensor_sc4336_init_table);

    for (int i = 0; i < size; i++)
    {
        bus->write16(bus, sensor_sc4336_init_table[i][0], sensor_sc4336_init_table[i][1]);
    }

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static avdk_err_t sc4336_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (width > WIN_MAX_X || width <= 0 || height > WIN_MAX_Y || height <= 0)
    {
        LOGE("%s, %d width: %d, height: %d, fail...\n", __func__, __LINE__, width, height);
        return -1;
    }

    bk_mipi_csi_controller_init(width, height, 0x2b);

    s_sc4336_out_width = width;
    s_sc4336_out_height = height;
    sc4336_write_window_regs(bus, width, height);

    return 0;
}

static avdk_err_t sc4336_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (fps > 30 || fps < SENSOR_OUTPUT_MIN_FPS)
    {
        LOGE("not supported fps\r\n");
        return -1;
    }

    uint32_t hts = 0;
    unsigned int vts = 0;
    unsigned char val = 0;
    int ret = 0;

    ret += bus->read16(bus, 0x320c, &val);
    hts = val << 8;
    ret += bus->read16(bus, 0x320d, &val);
    hts = (hts | val);
    if (0 != ret)
    {
        LOGE("err: sc4336 read err\n");
        return -1;
    }

    vts = SC4336_PCLK / hts / fps;
    ret = bus->write16(bus, SC4336_REG_VTS_L, (vts & 0xff));
    ret += bus->write16(bus, SC4336_REG_VTS_H, (vts >> 8));
    if (0 != ret)
    {
        LOGE("err: sc4336_write err\n");
        return ret;
    }

    return BK_OK;
}

static bool s_sc4336_hmirror;
static bool s_sc4336_vflip;
static uint16_t s_sc4336_out_width;
static uint16_t s_sc4336_out_height;

static void sc4336_write_window_regs(bk_camera_bus_t *bus, uint16_t width, uint16_t height)
{
    uint16_t win_y_start = (WIN_MAX_Y - height) / 2;
    uint16_t win_x_start = (WIN_MAX_X - width) / 2;

    win_y_start = (win_y_start < 4) ? 4 : win_y_start;
    win_y_start = win_y_start - ((win_y_start - 4) % 4);
    win_x_start = (win_x_start < 4) ? 4 : win_x_start;
    win_x_start = win_x_start - ((win_x_start - 4) % 4);

    /* Toggle window start by 1 pixel to preserve BGGR Bayer phase. */
    if (s_sc4336_hmirror)
    {
        win_x_start ^= 1;
    }
    if (s_sc4336_vflip)
    {
        win_y_start ^= 1;
    }

    bus->write16(bus, 0x3210, UINT16_HB(win_x_start));
    bus->write16(bus, 0x3211, UINT16_LB(win_x_start));
    bus->write16(bus, 0x3212, UINT16_HB(win_y_start));
    bus->write16(bus, 0x3213, UINT16_LB(win_y_start));
    bus->write16(bus, 0x3208, UINT16_HB(width));
    bus->write16(bus, 0x3209, UINT16_LB(width));
    bus->write16(bus, 0x320A, UINT16_HB(height));
    bus->write16(bus, 0x320B, UINT16_LB(height));
}

static void sc4336_apply_mirror_reg(bk_camera_bus_t *bus)
{
    uint8_t tmp;

    if (bus == NULL)
    {
        return;
    }

    tmp = 0;
    if (s_sc4336_hmirror)
    {
        tmp |= SC4336_MIRROR_BITS;
    }
    if (s_sc4336_vflip)
    {
        tmp |= SC4336_VFLIP_BITS;
    }
    bus->write16(bus, SC4336_REG_MIRROR_FLIP, tmp);

    if (s_sc4336_out_width > 0 && s_sc4336_out_height > 0)
    {
        sc4336_write_window_regs(bus, s_sc4336_out_width, s_sc4336_out_height);
    }
}

static avdk_err_t sc4336_set_hmirror(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    s_sc4336_hmirror = enable;
    sc4336_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t sc4336_set_vflip(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    s_sc4336_vflip = enable;
    sc4336_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t sc4336_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    sc4336_apply_mirror_reg(csi_sensor->config.bus);
    sc4336_set_ppi(controller, format->width, format->height);
    sc4336_set_fps(controller, format->fps);
    bk_mipi_csi_controller_reset();
    sc4336_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t sc4336_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg read
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("sc4336 {%04x, %02x}\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("sc4336 {%04x, %02x} -> {%04x, %02x}\n", addr, dump_val, addr, val);
        bus->write16(bus, addr, val);
    }

    if (cmd == 2) // standy, stream stop
    {
        LOGI("sc4336 stream stop\n");
        bus->write16(bus, 0x0100, 0x00);
    }

    if (cmd == 3) // resume from standy, stream start
    {
        LOGI("sc4336 stream start\n");
        bus->write16(bus, 0x0100, 0x01);
    }

    return 0;
}

static void *sc4336_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snsSC4336Obj;
}

static void *sc4336_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

static avdk_err_t sc4336_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &sc4336_format_array[0];
    format_array->size = ARRAY_SIZE(sc4336_format_array);
    return AVDK_ERR_OK;
}

avdk_err_t sc4336_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = SC4336_WRITE_ADDRESS;

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
    config->bus->write_address = SC4336_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = sc4336_init;
    csi_sensor->ops.set_format = sc4336_set_format;
    csi_sensor->ops.reg_ctrl = sc4336_ctrl;
    csi_sensor->ops.set_hmirror = sc4336_set_hmirror;
    csi_sensor->ops.set_vflip = sc4336_set_vflip;
    csi_sensor->ops.get_sensor_object = sc4336_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = sc4336_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = sc4336_query_support_formats;

    csi_sensor->isp_pub_attr = &sc4336_mipi_linear_attr;
    csi_sensor->sensor_config = NULL;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}

BK_CAMERA_SENSOR_DETECT_SECTION(sc4336_detect, CSI_CAMERA_PORT);
