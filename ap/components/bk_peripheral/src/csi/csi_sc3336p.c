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
#include <common/avdk_pixel_types.h>

#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#include "vsios_i2c.h"
#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include "mpi_isp_calib.h"
#include "sc3336p_2304x1296_calib.h"

#define TAG "sc3336p"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SC3336P_WRITE_ADDRESS (0x60)
#define SC3336P_READ_ADDRESS (0x61)
#define CHIP_ID_ADDR_HB (0x3107)
#define CHIP_ID_ADDR_LB (0x3108)

#define CHIP_ID_VAL_HB (0x9c)
#define CHIP_ID_VAL_LB (0x41)

#define WIN_MAX_X 2304
#define WIN_MAX_Y 1296

#define UINT16_HB(x) (((x) >> 8) & 0xFF)
#define UINT16_LB(x) ((x) & 0xFF)

#define SC3336P_REG_BYTE_NUM  2
#define SC3336P_DATA_BYTE_NUM 1
#define SC3336P_REG_EXPOSURE_H 0x3e00
#define SC3336P_REG_EXPOSURE_M 0x3e01
#define SC3336P_REG_EXPOSURE_L 0x3e02
#define SC3336P_REG_VTS_H 0x320e
#define SC3336P_REG_VTS_L 0x320f
#define SC3336P_REG_MIRROR_FLIP 0x3221
#define SC3336P_MIRROR_BITS     ((1U << 1) | (1U << 2))
#define SC3336P_VFLIP_BITS      ((1U << 5) | (1U << 6))

/* SC3336P_PCLK = VTS * HTS * FPS */
#define SC3336P_FPS_BASE         20

#define SC3336P_VTS_96M          1920
#define SC3336P_HTS_96M          2500
#define SC3336P_PCLK_96M         (SC3336P_VTS_96M * SC3336P_FPS_BASE * SC3336P_HTS_96M)

#define SC3336P_VTS_66M          1320
#define SC3336P_HTS_66M          2500
#define SC3336P_PCLK_66M         (SC3336P_VTS_66M * SC3336P_FPS_BASE * SC3336P_HTS_66M)

#define SC3336P_VTS_86M          1440
#define SC3336P_HTS_86M          3000
#define SC3336P_PCLK_86M         (SC3336P_VTS_86M * SC3336P_FPS_BASE * SC3336P_HTS_86M)

#define SC3336P_PCLK             (SC3336P_PCLK_86M)

#if (SC3336P_PCLK == SC3336P_PCLK_96M)
    #define SC3336P_VTS          SC3336P_VTS_96M
    #define SC3336P_HTS          SC3336P_HTS_96M
#elif (SC3336P_PCLK == SC3336P_PCLK_66M)
    #define SC3336P_VTS          SC3336P_VTS_66M
    #define SC3336P_HTS          SC3336P_HTS_66M
#elif (SC3336P_PCLK == SC3336P_PCLK_86M)
    #define SC3336P_VTS          SC3336P_VTS_86M
    #define SC3336P_HTS          SC3336P_HTS_86M
#endif

#define SC3336P_VMAX_1080P25FPS_LINEAR (1632)

#if (SC3336P_PCLK == SC3336P_PCLK_96M)
    #define SC3336P_VMAX_300W20FPS_LINEAR (1980)
    #define SC3336P_VMAX_300W10FPS_LINEAR (3960)
    #define SC3336P_VMAX_300W15FPS_LINEAR (2640)
#elif (SC3336P_PCLK == SC3336P_PCLK_66M)
    #define SC3336P_VMAX_300W20FPS_LINEAR (1620)
    #define SC3336P_VMAX_300W10FPS_LINEAR (3240)
    #define SC3336P_VMAX_300W15FPS_LINEAR (2160)
#elif (SC3336P_PCLK == SC3336P_PCLK_86M)
    #define SC3336P_VMAX_300W20FPS_LINEAR (1440)
    #define SC3336P_VMAX_300W10FPS_LINEAR (2880)
    #define SC3336P_VMAX_300W15FPS_LINEAR (1920)
#endif

enum SC3336P_REG_INDEX {
    REG_VTS_H       = 0,
    REG_VTS_L       = 1,
    REG_EXPOSURE_H  = 2,
    REG_EXPOSURE_M  = 3,
    REG_EXPOSURE_L  = 4,

    REG_GAIN_3E06    = 5,
    REG_GAIN_3E07    = 6,
    REG_GAIN_3E09    = 7,
};

#define SC3336P_300W20FPS_LINEAR_MODE (0)
#define SC3336P_1080P25FPS_LINEAR_MODE (1)
#define SC3336P_300W10FPS_LINEAR_MODE (2)
#define SC3336P_300W15FPS_LINEAR_MODE (3)

typedef struct vsisc3336p_DEVICE_S {
    vsi_u8_t i2cBus;
    vsios_i2c_attr_t i2cAttr;
    ISP_SNS_MODE_S snsMode;
    vsi_u8_t snsModeId;
    vsi_bool_t stream;
    AE_SNS_DEFAULT_S aeDefault;
    ISP_SNS_REGS_INFO_S snsRegsInfo;
} sc3336p_DEVICE_S;

typedef struct
{
    uint8_t val1;
    uint8_t val2;
    uint8_t val3;
    uint8_t val4;
} sc3336p_again_val_t;

static sc3336p_DEVICE_S *sc3336pDev[ISP_DEV_CNT][ISP_PORT_CNT] = {0};
static uint16_t s_sc3336p_width,s_sc3336p_height;

static ISP_CALIB_DATA_S * SC3336P_2304x1296_CalibParam_dynamic = NULL;

#if (SC3336P_PCLK == SC3336P_PCLK_96M)
//2304x1296@20fps
static const uint16_t sensor_sc3336p_init_table[][2] = {
    //Preview Type:0:DVP Raw 10 bit// 1:Raw 8 bit// 2:YUV422// 3:RAW16
    //Preview Type:4:RGB565// 5:Pixart SPI// 6:MIPI 10bit// 7:MIPI 12bit// 8: MTK SPI
    //port  0:MIPI// 1:Parallel// 2:MTK// 3:SPI// 4:TEST// 5: HISPI// 6 : Z2P/Z4P
    //I2C Mode    :0:Normal 8Addr,8Data//  1:Samsung 8 Addr,8Data// 2:Micron 8 Addr,16Data
    //I2C Mode    :3:Stmicro 16Addr,8Data//4:Micron2 16 Addr,16Data
    //Out Format  :0:YCbYCr/RG_GB// 1:YCrYCb/GR_BG// 2:CbYCrY/GB_RG// 3:CrYCbY/BG_GR
    //MCLK Speed  :0:6M//1:8M//2:10M//3:11.4M//4:12M//5:12.5M//6:13.5M//7:15M//8:18M//9:24M
    //pin  :BIT0 pwdn// BIT1:reset
    //avdd  0:2.8V// 1:2.5V// 2:1.8V
    //dovdd  0:2.8V// 1:2.5V// 2:1.8V
    //dvdd  0:1.8V// 1:1.5V// 2:1.2V


    // [DataBase]
    // DBName=Demosens

    // [Vendor]
    // VendorName=SmartSens

    // [Sensor]
    // SensorName=SC3336PV20_RAW_30fps
    // width=2304
    // height=1296
    // port=0
    // type=6
    // pin=3
    // SlaveID=0x60
    // mode=3
    // FlagReg=0x36ff
    // FlagMask=0xff
    // FlagData=0x00
    // FlagReg1=0x36ff
    // FlagMask1=0xff
    // FlagData1=0x00
    // outformat=3
    // mclk=24
    // avdd=2.800000
    // dovdd=1.800000
    // dvdd=1.5
    // Ext0=0
    // Ext1=0
    // Ext2=0
    // AFVCC=0.0000
    // VPP=0.000000

    // [ParaList]
    {0x0103,0x01},
    {0x36e9,0x80},
    {0x37f9,0x80},
    {0x301f,0x02},
    {0x30b8,0x33},
    {0x320e,0x07},
    {0x320f,0xbc},
    {0x3253,0x10},
    {0x325f,0x20},
    {0x3301,0x04},
    {0x3306,0x50},
    {0x3309,0xf8},
    {0x330a,0x00},
    {0x330b,0xd8},
    {0x3314,0x13},
    {0x331f,0xe9},
    {0x3333,0x10},
    {0x3334,0x40},
    {0x335e,0x06},
    {0x335f,0x0a},
    {0x3364,0x5e},
    {0x337c,0x02},
    {0x337d,0x0e},
    {0x3390,0x01},
    {0x3391,0x03},
    {0x3392,0x07},
    {0x3393,0x04},
    {0x3394,0x04},
    {0x3395,0x04},
    {0x3396,0x08},
    {0x3397,0x0b},
    {0x3398,0x1f},
    {0x3399,0x04},
    {0x339a,0x0a},
    {0x339b,0x3a},
    {0x339c,0x60},
    {0x33a2,0x04},
    {0x33ac,0x08},
    {0x33ad,0x1c},
    {0x33ae,0x10},
    {0x33af,0x30},
    {0x33b1,0x80},
    {0x33b3,0x48},
    {0x33f9,0x60},
    {0x33fb,0x74},
    {0x33fc,0x4b},
    {0x33fd,0x5f},
    {0x349f,0x03},
    {0x34a6,0x4b},
    {0x34a7,0x5f},
    {0x34a8,0x20},
    {0x34a9,0x18},
    {0x34aa,0x00},
    {0x34ab,0xe8},
    {0x34ac,0x01},
    {0x34ad,0x00},
    {0x34f8,0x5f},
    {0x34f9,0x18},
    {0x3630,0xc0},
    {0x3631,0x84},
    {0x3632,0x64},
    {0x3633,0x32},
    {0x363b,0x03},
    {0x363c,0x08},
    {0x3641,0x38},
    {0x3670,0x4e},
    {0x3674,0xf0},
    {0x3675,0xc0},
    {0x3676,0xc0},
    {0x3677,0x86},
    {0x3678,0x86},
    {0x3679,0x86},
    {0x367c,0x48},
    {0x367d,0x49},
    {0x367e,0x4b},
    {0x367f,0x5f},
    {0x3690,0x22},
    {0x3691,0x22},
    {0x3692,0x33},
    {0x369c,0x4b},
    {0x369d,0x4f},
    {0x36b0,0x87},
    {0x36b1,0x90},
    {0x36b2,0xa1},
    {0x36b3,0xc8},
    {0x36b4,0x49},
    {0x36b5,0x4b},
    {0x36b6,0x4f},
    {0x36ea,0x0b},
    {0x36eb,0x0d},
    {0x36ec,0x1c},
    {0x36ed,0x26},
    {0x370f,0x01},
    {0x3722,0x09},
    {0x3724,0x41},
    {0x3725,0xc1},
    {0x3771,0x09},
    {0x3772,0x09},
    {0x3773,0x05},
    {0x377a,0x48},
    {0x377b,0x5f},
    {0x37fa,0x0b},
    {0x37fb,0x33},
    {0x37fc,0x11},
    {0x37fd,0x08},
    {0x3904,0x04},
    {0x3905,0x8c},
    {0x391d,0x04},
    {0x391f,0x49},
    {0x3921,0x20},
    {0x3926,0x21},
    {0x3933,0x80},
    {0x3934,0x08},
    {0x3935,0x00},
    {0x3936,0x90},
    {0x3937,0x78},
    {0x3938,0x77},
    {0x3939,0x00},
    {0x393a,0x00},
    {0x393b,0x00},
    {0x393c,0x1c},
    {0x39dc,0x02},
    {0x3e00,0x00},
    {0x3e01,0x00},
    {0x3e02,0x80},
    {0x3e09,0x00},
    {0x440d,0x10},
    {0x440e,0x01},
    {0x4509,0x20},
    {0x5780,0x76},
    {0x5784,0x10},
    {0x5785,0x04},
    {0x5787,0x0a},
    {0x5788,0x0a},
    {0x5789,0x04},
    {0x578a,0x0a},
    {0x578b,0x0a},
    {0x578c,0x04},
    {0x578d,0x40},
    {0x5790,0x08},
    {0x5791,0x04},
    {0x5792,0x04},
    {0x5793,0x08},
    {0x5794,0x04},
    {0x5795,0x04},
    {0x5799,0x46},
    {0x579a,0x77},
    {0x57a1,0x04},
    {0x57a8,0xd2},
    {0x57aa,0x2a},
    {0x57ab,0x7f},
    {0x57ac,0x00},
    {0x57ad,0x00},
    {0x59e2,0x08},
    {0x59e3,0x03},
    {0x59e4,0x00},
    {0x59e5,0x10},
    {0x59e6,0x06},
    {0x59e7,0x00},
    {0x59e8,0x08},
    {0x59e9,0x02},
    {0x59ea,0x00},
    {0x59eb,0x10},
    {0x59ec,0x04},
    {0x59ed,0x00},
    {0x5ae0,0xfe},
    {0x5ae1,0x40},
    {0x5ae2,0x38},
    {0x5ae3,0x30},
    {0x5ae4,0x28},
    {0x5ae5,0x38},
    {0x5ae6,0x30},
    {0x5ae7,0x28},
    {0x5ae8,0x3f},
    {0x5ae9,0x34},
    {0x5aea,0x2c},
    {0x5aeb,0x3f},
    {0x5aec,0x34},
    {0x5aed,0x2c},
    {0x36e9,0x53},
    {0x37f9,0x27},
    // {0x0100,0x01},
};
#elif (SC3336P_PCLK == SC3336P_PCLK_66M)
// 2304x1296@20fps
// VTS=1320.000000,
// HTS=2500.000000,
// SCLK=33.000000,
// PCLK=66.000000,
// MipiCLK=330.000000,
// Tline=37.878788us,
// TExp_step=1.0*Tline=37.878788us,
// TExp_offset=9.090909us
static const uint16_t sensor_sc3336p_init_table[][2] = {
    {0x0103,0x01},
    {0x36e9,0x80},
    {0x37f9,0x80},
    {0x3018,0x3a},
    {0x3019,0x0c},
    {0x301f,0x86},
    {0x30b8,0x33},
    {0x320e,0x05},
    {0x320f,0x28},
    {0x3253,0x10},
    {0x325f,0x20},
    {0x3301,0x04},
    {0x3306,0x50},
    {0x3309,0xa8},
    {0x330a,0x00},
    {0x330b,0xd8},
    {0x3314,0x13},
    {0x331f,0x99},
    {0x3333,0x10},
    {0x3334,0x40},
    {0x335e,0x06},
    {0x335f,0x0a},
    {0x3364,0x5e},
    {0x337c,0x02},
    {0x337d,0x0e},
    {0x3390,0x01},
    {0x3391,0x03},
    {0x3392,0x07},
    {0x3393,0x04},
    {0x3394,0x04},
    {0x3395,0x04},
    {0x3396,0x08},
    {0x3397,0x0b},
    {0x3398,0x1f},
    {0x3399,0x04},
    {0x339a,0x0a},
    {0x339b,0x3a},
    {0x339c,0xa0},
    {0x33a2,0x04},
    {0x33ac,0x08},
    {0x33ad,0x1c},
    {0x33ae,0x10},
    {0x33af,0x30},
    {0x33b1,0x80},
    {0x33b3,0x48},
    {0x33f9,0x50},
    {0x33fb,0x60},
    {0x33fc,0x4b},
    {0x33fd,0x5f},
    {0x349f,0x03},
    {0x34a6,0x4b},
    {0x34a7,0x5f},
    {0x34a8,0x20},
    {0x34a9,0x18},
    {0x34ab,0xe8},
    {0x34ac,0x01},
    {0x34ad,0x00},
    {0x34f8,0x5f},
    {0x34f9,0x18},
    {0x3630,0xc0},
    {0x3631,0x84},
    {0x3632,0x64},
    {0x3633,0x32},
    {0x363b,0x03},
    {0x363c,0x08},
    {0x3641,0x38},
    {0x3670,0x4e},
    {0x3674,0xc0},
    {0x3675,0xc0},
    {0x3676,0xc0},
    {0x3677,0x86},
    {0x3678,0x86},
    {0x3679,0x86},
    {0x367c,0x48},
    {0x367d,0x49},
    {0x367e,0x4b},
    {0x367f,0x5f},
    {0x3690,0x32},
    {0x3691,0x32},
    {0x3692,0x42},
    {0x369c,0x4b},
    {0x369d,0x5f},
    {0x36b0,0x87},
    {0x36b1,0x90},
    {0x36b2,0xa1},
    {0x36b3,0xd8},
    {0x36b4,0x49},
    {0x36b5,0x4b},
    {0x36b6,0x4f},
    {0x36ea,0x0b},
    {0x36eb,0x0d},
    {0x36ec,0x1c},
    {0x36ed,0x26},
    {0x370f,0x01},
    {0x3722,0x09},
    {0x3724,0x41},
    {0x3725,0xc1},
    {0x3771,0x09},
    {0x3772,0x09},
    {0x3773,0x05},
    {0x377a,0x48},
    {0x377b,0x5f},
    {0x37fa,0x0b},
    {0x37fb,0x33},
    {0x37fc,0x11},
    {0x37fd,0x18},
    {0x3904,0x04},
    {0x3905,0x8c},
    {0x391d,0x04},
    {0x3921,0x20},
    {0x3926,0x21},
    {0x3933,0x80},
    {0x3934,0x0a},
    {0x3935,0x00},
    {0x3936,0x2a},
    {0x3937,0x6a},
    {0x3938,0x6a},
    {0x39dc,0x02},
    {0x3e00,0x00},
    {0x3e01,0x00},
    {0x3e02,0x80},
    {0x3e09,0x00},
    {0x440d,0x10},
    {0x440e,0x01},
    {0x4509,0x20},
    {0x4819,0x05},
    {0x481b,0x03},
    {0x481d,0x09},
    {0x481f,0x02},
    {0x4821,0x08},
    {0x4823,0x02},
    {0x4825,0x02},
    {0x4827,0x03},
    {0x4829,0x04},
    {0x5ae0,0xfe},
    {0x5ae1,0x40},
    {0x5ae2,0x38},
    {0x5ae3,0x30},
    {0x5ae4,0x28},
    {0x5ae5,0x38},
    {0x5ae6,0x30},
    {0x5ae7,0x28},
    {0x5ae8,0x3f},
    {0x5ae9,0x34},
    {0x5aea,0x2c},
    {0x5aeb,0x3f},
    {0x5aec,0x34},
    {0x5aed,0x2c},
    {0x36e9,0x20},
    {0x37f9,0x20},
    // {0x0100,0x01},
};
#elif (SC3336P_PCLK == SC3336P_PCLK_86M)
// 2304x1296@20fps
// VTS=1440.000000,
// HTS=3000.000000,
// SCLK=43.200000,
// PCLK=86.400000,
// MipiCLK=432.000000,
// Tline=34.722222us,
// TExp_step=1.0*Tline=34.722222us,
// TExp_offset=6.944444us
static const uint16_t sensor_sc3336p_init_table[][2] = {
    {0x0103,0x01},
    {0x36e9,0x80},
    {0x37f9,0x80},
    {0x301f,0x88},
    {0x30b8,0x33},
    {0x320c,0x05},
    {0x320d,0xdc},
    {0x320e,0x05},
    {0x320f,0xa0},
    {0x3253,0x10},
    {0x325f,0x20},
    {0x3301,0x04},
    {0x3306,0x50},
    {0x3309,0xa8},
    {0x330a,0x00},
    {0x330b,0xd8},
    {0x3314,0x13},
    {0x331f,0x99},
    {0x3333,0x10},
    {0x3334,0x40},
    {0x335e,0x06},
    {0x335f,0x0a},
    {0x3364,0x5e},
    {0x337c,0x02},
    {0x337d,0x0e},
    {0x3390,0x01},
    {0x3391,0x03},
    {0x3392,0x07},
    {0x3393,0x04},
    {0x3394,0x04},
    {0x3395,0x04},
    {0x3396,0x08},
    {0x3397,0x0b},
    {0x3398,0x1f},
    {0x3399,0x04},
    {0x339a,0x0a},
    {0x339b,0x3a},
    {0x339c,0xa0},
    {0x33a2,0x04},
    {0x33ac,0x08},
    {0x33ad,0x1c},
    {0x33ae,0x10},
    {0x33af,0x30},
    {0x33b1,0x80},
    {0x33b3,0x48},
    {0x33f9,0x60},
    {0x33fb,0x74},
    {0x33fc,0x4b},
    {0x33fd,0x5f},
    {0x349f,0x03},
    {0x34a6,0x4b},
    {0x34a7,0x5f},
    {0x34a8,0x20},
    {0x34a9,0x18},
    {0x34ab,0xe8},
    {0x34ac,0x01},
    {0x34ad,0x00},
    {0x34f8,0x5f},
    {0x34f9,0x18},
    {0x3630,0xc0},
    {0x3631,0x84},
    {0x3632,0x64},
    {0x3633,0x32},
    {0x363b,0x03},
    {0x363c,0x08},
    {0x3641,0x38},
    {0x3670,0x4e},
    {0x3674,0xc0},
    {0x3675,0xc0},
    {0x3676,0xc0},
    {0x3677,0x86},
    {0x3678,0x86},
    {0x3679,0x86},
    {0x367c,0x48},
    {0x367d,0x49},
    {0x367e,0x4b},
    {0x367f,0x5f},
    {0x3690,0x32},
    {0x3691,0x32},
    {0x3692,0x42},
    {0x369c,0x4b},
    {0x369d,0x5f},
    {0x36b0,0x87},
    {0x36b1,0x90},
    {0x36b2,0xa1},
    {0x36b3,0xd8},
    {0x36b4,0x49},
    {0x36b5,0x4b},
    {0x36b6,0x4f},
    {0x36ea,0x09},
    {0x36eb,0x0d},
    {0x36ec,0x1c},
    {0x36ed,0x36},
    {0x370f,0x01},
    {0x3722,0x09},
    {0x3724,0x41},
    {0x3725,0xc1},
    {0x3771,0x09},
    {0x3772,0x09},
    {0x3773,0x05},
    {0x377a,0x48},
    {0x377b,0x5f},
    {0x37fa,0x06},
    {0x37fb,0x33},
    {0x37fc,0x11},
    {0x37fd,0x38},
    {0x3904,0x04},
    {0x3905,0x8c},
    {0x391d,0x04},
    {0x3921,0x20},
    {0x3926,0x21},
    {0x3933,0x80},
    {0x3934,0x0a},
    {0x3935,0x00},
    {0x3936,0x2a},
    {0x3937,0x6a},
    {0x3938,0x6a},
    {0x39dc,0x02},
    {0x3e00,0x00},
    {0x3e01,0x00},
    {0x3e02,0x80},
    {0x3e09,0x00},
    {0x440d,0x10},
    {0x440e,0x01},
    {0x4509,0x20},
    {0x4819,0x06},
    {0x481b,0x03},
    {0x481d,0x0c},
    {0x481f,0x03},
    {0x4821,0x08},
    {0x4823,0x03},
    {0x4825,0x03},
    {0x4827,0x03},
    {0x4829,0x05},
    {0x5ae0,0xfe},
    {0x5ae1,0x40},
    {0x5ae2,0x38},
    {0x5ae3,0x30},
    {0x5ae4,0x28},
    {0x5ae5,0x38},
    {0x5ae6,0x30},
    {0x5ae7,0x28},
    {0x5ae8,0x3f},
    {0x5ae9,0x34},
    {0x5aea,0x2c},
    {0x5aeb,0x3f},
    {0x5aec,0x34},
    {0x5aed,0x2c},
    {0x36e9,0x44},
    {0x37f9,0x34},
    // {0x0100,0x01},
};
#endif

//1920x1080@25fps
static const uint16_t sensor_sc3336p_1080P25_init_table[][2] = {
    {0x0103,0x01},
    {0x36e9,0x80},
    {0x37f9,0x80},
    {0x301f,0x0a},
    {0x30b8,0x44},
    {0x3200,0x00},
    {0x3201,0x00},
    {0x3202,0x00},
    {0x3203,0x6c},
    {0x3204,0x09},
    {0x3205,0x07},
    {0x3206,0x04},
    {0x3207,0xab},
    {0x3208,0x07},
    {0x3209,0x80},
    {0x320a,0x04},
    {0x320b,0x38},
    {0x320e,0x06},
    {0x320f,0x60},
    {0x3210,0x00},
    {0x3211,0xc4},
    {0x3212,0x00},
    {0x3213,0x04},
    {0x3253,0x10},
    {0x325f,0x20},
    {0x3301,0x04},
    {0x3306,0x50},
    {0x330a,0x00},
    {0x330b,0xd8},
    {0x3314,0x13},
    {0x3333,0x10},
    {0x3334,0x40},
    {0x335e,0x06},
    {0x335f,0x0a},
    {0x3364,0x5e},
    {0x337c,0x02},
    {0x337d,0x0e},
    {0x3390,0x01},
    {0x3391,0x03},
    {0x3392,0x07},
    {0x3393,0x04},
    {0x3394,0x04},
    {0x3395,0x04},
    {0x3396,0x08},
    {0x3397,0x0b},
    {0x3398,0x1f},
    {0x3399,0x04},
    {0x339a,0x0a},
    {0x339b,0x3a},
    {0x339c,0xc4},
    {0x33a2,0x04},
    {0x33ac,0x08},
    {0x33ad,0x1c},
    {0x33ae,0x10},
    {0x33af,0x30},
    {0x33b1,0x80},
    {0x33b3,0x48},
    {0x33f9,0x60},
    {0x33fb,0x74},
    {0x33fc,0x4b},
    {0x33fd,0x5f},
    {0x349f,0x03},
    {0x34a6,0x4b},
    {0x34a7,0x5f},
    {0x34a8,0x20},
    {0x34a9,0x18},
    {0x34ab,0xe8},
    {0x34ac,0x01},
    {0x34ad,0x00},
    {0x34f8,0x5f},
    {0x34f9,0x18},
    {0x3630,0xc0},
    {0x3631,0x84},
    {0x3633,0x32},
    {0x363b,0x03},
    {0x363c,0x08},
    {0x3641,0x38},
    {0x3670,0x4e},
    {0x3674,0xc0},
    {0x3675,0xc0},
    {0x3676,0xc0},
    {0x3677,0x84},
    {0x3678,0x8a},
    {0x3679,0x8c},
    {0x367c,0x48},
    {0x367d,0x49},
    {0x367e,0x4b},
    {0x367f,0x5f},
    {0x3690,0x33},
    {0x3691,0x33},
    {0x3692,0x44},
    {0x369c,0x4b},
    {0x369d,0x5f},
    {0x36b0,0x87},
    {0x36b1,0x90},
    {0x36b2,0xa1},
    {0x36b3,0xd8},
    {0x36b4,0x49},
    {0x36b5,0x4b},
    {0x36b6,0x4f},
    {0x36ea,0x11},
    {0x36eb,0x0d},
    {0x36ec,0x1c},
    {0x36ed,0x26},
    {0x370f,0x01},
    {0x3722,0x09},
    {0x3724,0x41},
    {0x3725,0xc1},
    {0x3771,0x09},
    {0x3772,0x09},
    {0x3773,0x05},
    {0x377a,0x48},
    {0x377b,0x5f},
    {0x37fa,0x11},
    {0x37fb,0x33},
    {0x37fc,0x11},
    {0x37fd,0x08},
    {0x3904,0x04},
    {0x3905,0x8c},
    {0x391d,0x04},
    {0x3921,0x20},
    {0x3926,0x21},
    {0x3933,0x80},
    {0x3934,0x0a},
    {0x3935,0x00},
    {0x3936,0x2a},
    {0x3937,0x80},//0x6a
    {0x3938,0x80},//0x6a
    {0x39dc,0x02},
    {0x3e00,0x00},
    {0x3e01,0x00},
    {0x3e02,0x80},
    {0x3e09,0x00},
    {0x4509,0x20},
    {0x5ae0,0xfe},
    {0x5ae1,0x40},
    {0x5ae2,0x38},
    {0x5ae3,0x30},
    {0x5ae4,0x28},
    {0x5ae5,0x38},
    {0x5ae6,0x30},
    {0x5ae7,0x28},
    {0x5ae8,0x3f},
    {0x5ae9,0x34},
    {0x5aea,0x2c},
    {0x5aeb,0x3f},
    {0x5aec,0x34},
    {0x5aed,0x2c},
    {0x36e9,0x54},
    {0x37f9,0x47},
    // {0x0100,0x01},
};

#define sc3336p_TABLE_SIZE(table) (sizeof(table) / sizeof(table[0]))

static int sc3336p_SetStream(ISP_PORT IspPort, vsi_bool_t stream);

static sc3336p_DEVICE_S *sc3336p_GetSensorDev(ISP_PORT IspPort)
{
    if (sc3336pDev[IspPort.devId][IspPort.portId] == NULL)
    {
        sc3336pDev[IspPort.devId][IspPort.portId] = os_malloc(sizeof(sc3336p_DEVICE_S));
        if (sc3336pDev[IspPort.devId][IspPort.portId] == NULL)
        {
            LOGE("%s %d GC2053Dev[%d][%d] malloc failed \r\n", __func__, __LINE__, IspPort.devId, IspPort.portId);
            return NULL;
        }
        os_memset(sc3336pDev[IspPort.devId][IspPort.portId], 0 , sizeof(sc3336p_DEVICE_S));
    }

    return sc3336pDev[IspPort.devId][IspPort.portId];
}


static int sc3336p_InitRegInfo(ISP_PORT IspPort)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &psc3336pDev->snsRegsInfo;
    pSnsRegsInfo->snsDev = psc3336pDev->i2cBus;

    pSnsRegsInfo->addrByteNum = psc3336pDev->i2cAttr.reg_bytes;
    pSnsRegsInfo->dataByteNum = psc3336pDev->i2cAttr.data_bytes;
    pSnsRegsInfo->slaveAddr   = psc3336pDev->i2cAttr.slave_addr;
    pSnsRegsInfo->regCnt = 8;
    pSnsRegsInfo->delayMax = 2;

    pSnsRegsInfo->snsData[REG_VTS_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_VTS_H].regAddr = SC3336P_REG_VTS_H;
    pSnsRegsInfo->snsData[REG_VTS_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_VTS_L].regAddr = SC3336P_REG_VTS_L;

    pSnsRegsInfo->snsData[REG_EXPOSURE_H].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPOSURE_H].regAddr = SC3336P_REG_EXPOSURE_H;
    pSnsRegsInfo->snsData[REG_EXPOSURE_H].data = 0x00;
    pSnsRegsInfo->snsData[REG_EXPOSURE_M].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPOSURE_M].regAddr = SC3336P_REG_EXPOSURE_M;
    pSnsRegsInfo->snsData[REG_EXPOSURE_M].data = 0x00;
    pSnsRegsInfo->snsData[REG_EXPOSURE_L].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_EXPOSURE_L].regAddr = SC3336P_REG_EXPOSURE_L;
    pSnsRegsInfo->snsData[REG_EXPOSURE_L].data = 0x80;

    pSnsRegsInfo->snsData[REG_GAIN_3E06].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_GAIN_3E06].regAddr = 0x3e06;
    pSnsRegsInfo->snsData[REG_GAIN_3E07].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_GAIN_3E07].regAddr = 0x3e07;
    pSnsRegsInfo->snsData[REG_GAIN_3E09].delayFrameNum = 2;
    pSnsRegsInfo->snsData[REG_GAIN_3E09].regAddr = 0x3e09;

    return BK_OK;
}

static int sc3336p_SensorInit(ISP_PORT IspPort, vsi_u8_t snsDev)
{
    if (SC3336P_2304x1296_CalibParam_dynamic == NULL)
    {
        SC3336P_2304x1296_CalibParam_dynamic = os_malloc(sizeof(SC3336P_2304x1296_CalibParam));
        if (SC3336P_2304x1296_CalibParam_dynamic == NULL)
        {
            LOGE("Failed to malloc SC3336P_2304x1296_CalibParam_dynamic\n");
            return BK_FAIL;
        }
        os_memcpy(SC3336P_2304x1296_CalibParam_dynamic, &SC3336P_2304x1296_CalibParam, sizeof(SC3336P_2304x1296_CalibParam));
    }

    sc3336p_DEVICE_S *psc3336pDev = sc3336p_GetSensorDev(IspPort);
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    int ret;

    os_memset(psc3336pDev, 0, sizeof(*psc3336pDev));
    psc3336pDev->i2cBus              = snsDev;
    psc3336pDev->i2cAttr.slave_addr  = SC3336P_WRITE_ADDRESS;
    psc3336pDev->i2cAttr.reg_bytes   = SC3336P_REG_BYTE_NUM;
    psc3336pDev->i2cAttr.data_bytes  = SC3336P_DATA_BYTE_NUM;
    sc3336p_InitRegInfo(IspPort);

    ret = vsios_i2c_sys_init(snsDev);
    if (ret) {
        LOGE("Failed to i2c init %d\n", snsDev);
        return BK_FAIL;
    }

    sc3336p_SetStream(IspPort, 0);

    return  BK_OK;
}

static int sc3336p_SensorExit(ISP_PORT IspPort)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    vsios_i2c_sys_exit(psc3336pDev->i2cBus);

    if (SC3336P_2304x1296_CalibParam_dynamic != NULL)
    {
        os_free(SC3336P_2304x1296_CalibParam_dynamic);
        SC3336P_2304x1296_CalibParam_dynamic = NULL;
    }
    return  BK_OK;
}

static int sc3336p_WriteReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t data)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = psc3336pDev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &psc3336pDev->i2cAttr;

    vsios_i2c_write(i2cBus, pI2cAttr, addr, data);

    return  BK_OK;
}

static int sc3336p_ReadReg(ISP_PORT IspPort, vsi_u32_t addr, vsi_u32_t *pData)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    vsi_u8_t i2cBus = psc3336pDev->i2cBus;
    vsios_i2c_attr_t *pI2cAttr = &psc3336pDev->i2cAttr;

    *pData = vsios_i2c_read(i2cBus, pI2cAttr, addr);

    return  BK_OK;
}

static int sc3336p_InitAeDefault(ISP_PORT IspPort)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    AE_SNS_DEFAULT_S *pAeSnsDft = &psc3336pDev->aeDefault;

    switch (psc3336pDev->snsModeId) {
        case SC3336P_300W20FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesStd = SC3336P_VMAX_300W20FPS_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 20 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->fullLinesMax =
                pAeSnsDft->fps * SC3336P_VMAX_300W20FPS_LINEAR / ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            pAeSnsDft->minIntLine  = 8;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 3200;
            pAeSnsDft->minAgain  = 64;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 62;
            pAeSnsDft->dampOver = 0x40;
            pAeSnsDft->dampUnder = 0x40;
            pAeSnsDft->tolerance = 2;
            pAeSnsDft->initExposure = 0x08 * pAeSnsDft->minAgain;
            break;
        case SC3336P_1080P25FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesStd = SC3336P_VMAX_1080P25FPS_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 25 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->fullLinesMax =
                pAeSnsDft->fps * SC3336P_VMAX_1080P25FPS_LINEAR / ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            pAeSnsDft->minIntLine  = 8;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 3200;
            pAeSnsDft->minAgain  = 64;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 62;
            pAeSnsDft->dampOver = 0x40;
            pAeSnsDft->dampUnder = 0x40;
            pAeSnsDft->tolerance = 2;
            pAeSnsDft->initExposure = 0x08 * pAeSnsDft->minAgain;
            break;
            case SC3336P_300W10FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesStd = SC3336P_VMAX_300W10FPS_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 10 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->fullLinesMax =
                pAeSnsDft->fps * SC3336P_VMAX_300W10FPS_LINEAR / ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            pAeSnsDft->minIntLine  = 8;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 3200;
            pAeSnsDft->minAgain  = 64;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 62;
            pAeSnsDft->dampOver = 0x40;
            pAeSnsDft->dampUnder = 0x40;
            pAeSnsDft->tolerance = 2;
            pAeSnsDft->initExposure = 0x08 * pAeSnsDft->minAgain;
            break;
        case SC3336P_300W15FPS_LINEAR_MODE:
            pAeSnsDft->fullLinesStd = SC3336P_VMAX_300W15FPS_LINEAR;
            pAeSnsDft->fullLines = pAeSnsDft->fullLinesStd;
            pAeSnsDft->fps = 15 * ISP_SNS_FPS_ACCU;
            pAeSnsDft->fullLinesMax =
                pAeSnsDft->fps * SC3336P_VMAX_300W15FPS_LINEAR / ISP_SNS_FPS_ACCU;
            pAeSnsDft->linesPer500ms =
                pAeSnsDft->fullLines * pAeSnsDft->fps / (2 * ISP_SNS_FPS_ACCU);

            pAeSnsDft->maxIntLine  = pAeSnsDft->fullLines - 8;
            pAeSnsDft->minIntLine  = 8;
            pAeSnsDft->intLineStep = 1;

            pAeSnsDft->maxAgain  = 3200;
            pAeSnsDft->minAgain  = 64;
            pAeSnsDft->againStep = 1;

            pAeSnsDft->maxDgain  = 1024;
            pAeSnsDft->minDgain  = 1024;
            pAeSnsDft->dgainStep = 1;

            pAeSnsDft->aeTarget = 62;
            pAeSnsDft->dampOver = 0x40;
            pAeSnsDft->dampUnder = 0x40;
            pAeSnsDft->tolerance = 2;
            pAeSnsDft->initExposure = 0x08 * pAeSnsDft->minAgain;
            break;
        default:
            return BK_FAIL;
    }

    return BK_OK;
}

static int sc3336p_SetMode(ISP_PORT IspPort, ISP_SNS_MODE_S *pSnsMode)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if ((pSnsMode->width == psc3336pDev->snsMode.width) &&
        (pSnsMode->height == psc3336pDev->snsMode.height) &&
        (pSnsMode->hdrMode == psc3336pDev->snsMode.hdrMode) &&
        (pSnsMode->stichMode == psc3336pDev->snsMode.stichMode)) {
        return BK_OK;
    }
    LOGI("sc3336p_SetMode: %d, %d, %d, %d, %d\n", pSnsMode->width, pSnsMode->height, pSnsMode->hdrMode, pSnsMode->stichMode, pSnsMode->fps/ISP_SNS_FPS_ACCU);

    if ((pSnsMode->width  == 2304) &&
        (pSnsMode->height == 1296) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR) &&
        (pSnsMode->fps == 20 * ISP_SNS_FPS_ACCU)) {
        os_memcpy(&psc3336pDev->snsMode, pSnsMode, sizeof(*pSnsMode));
        psc3336pDev->snsModeId = SC3336P_300W20FPS_LINEAR_MODE;
        sc3336p_InitAeDefault(IspPort);
    } else if ((pSnsMode->width  == 1920) &&
        (pSnsMode->height == 1080) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR) &&
        (pSnsMode->fps == 25 * ISP_SNS_FPS_ACCU)) {
        os_memcpy(&psc3336pDev->snsMode, pSnsMode, sizeof(*pSnsMode));
        psc3336pDev->snsModeId = SC3336P_1080P25FPS_LINEAR_MODE;
        sc3336p_InitAeDefault(IspPort);
    } else if ((pSnsMode->width  == 2304) &&
        (pSnsMode->height == 1296) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR) &&
        (pSnsMode->fps == 10 * ISP_SNS_FPS_ACCU)) {
        os_memcpy(&psc3336pDev->snsMode, pSnsMode, sizeof(*pSnsMode));
        psc3336pDev->snsModeId = SC3336P_300W10FPS_LINEAR_MODE;
        sc3336p_InitAeDefault(IspPort);
    } else if ((pSnsMode->width  == 2304) &&
        (pSnsMode->height == 1296) &&
        (pSnsMode->hdrMode == HDR_MODE_LINEAR) &&
        (pSnsMode->fps == 15 * ISP_SNS_FPS_ACCU)) {
        os_memcpy(&psc3336pDev->snsMode, pSnsMode, sizeof(*pSnsMode));
        psc3336pDev->snsModeId = SC3336P_300W15FPS_LINEAR_MODE;
        sc3336p_InitAeDefault(IspPort);
    } else {
        LOGE("sc3336p_SetMode: not supported width: %d, height: %d, fps: %d\n", pSnsMode->width, pSnsMode->height, pSnsMode->fps);
        return BK_FAIL;
    }

    return BK_OK;
}

static int sc3336p_SetStream(ISP_PORT IspPort, vsi_bool_t stream)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (stream) {
        sc3336p_WriteReg(IspPort, 0x0100, 0x01);
    } else {
        sc3336p_WriteReg(IspPort, 0x0100, 0x00);
    }

    psc3336pDev->stream = stream;

    return BK_OK;
}

static int sc3336p_SetIspDefault(ISP_PORT IspPort)
{
    VSI_MPI_ISP_SetCalib(IspPort, SC3336P_2304x1296_CalibParam_dynamic);

    return BK_OK;
}

static int sc3336p_GetAeDefault(ISP_PORT IspPort, AE_SNS_DEFAULT_S *pAeSnsDft)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pAeSnsDft, &psc3336pDev->aeDefault, sizeof(*pAeSnsDft));

    return BK_OK;
}

static int sc3336p_SetFps(ISP_PORT IspPort, vsi_u32_t fps)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    AE_SNS_DEFAULT_S *pAeSnsDft = &psc3336pDev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &psc3336pDev->snsRegsInfo;
    vsi_u32_t vts;

    switch(psc3336pDev->snsModeId) {
        case SC3336P_300W20FPS_LINEAR_MODE:
            if ((fps <= 20 * ISP_SNS_FPS_ACCU) && (fps >= 10 * ISP_SNS_FPS_ACCU)) {
                vts = SC3336P_VMAX_300W20FPS_LINEAR * 20 * ISP_SNS_FPS_ACCU / fps;
            } else {
                return BK_FAIL;
            }
            break;  
        default:
            return BK_FAIL;
            break;
    }

    vts = (vts >= pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : vts;
    pSnsRegsInfo->snsData[REG_VTS_H].data = ((vts & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_VTS_L].data = (vts & 0xFF);
    pAeSnsDft->fullLines = vts;
    pAeSnsDft->fps = fps;
    pAeSnsDft->maxIntLine = pAeSnsDft->fullLines - 8;
    LOGI("sc3336p_SetFps: fullLines: %d, fps: %d, maxIntLine: %d\n", pAeSnsDft->fullLines, pAeSnsDft->fps/ISP_SNS_FPS_ACCU, pAeSnsDft->maxIntLine);

    return BK_OK;
}

static int sc3336p_SlowFrameRate(ISP_PORT IspPort, vsi_u32_t fullLines)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    const vsi_u32_t fps_min = 10 * ISP_SNS_FPS_ACCU;
    const vsi_u32_t fps_max = 20 * ISP_SNS_FPS_ACCU;
    vsi_u32_t tempfps;

    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    AE_SNS_DEFAULT_S *pAeSnsDft = &psc3336pDev->aeDefault;
    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &psc3336pDev->snsRegsInfo;

    fullLines = (fullLines > pAeSnsDft->fullLinesMax) ? pAeSnsDft->fullLinesMax : fullLines;
    if (fullLines == 0)
    {
        LOGE("sc3336p_SlowFrameRate: fullLines is 0\n");
        return BK_FAIL;
    }

    /* fps = VMAX * (20 * ACCU) / fullLines, keep fps in [10, 20] seconds^-1 @ ISP_SNS_FPS_ACCU */
    tempfps = (vsi_u32_t)(((vsi_u64_t)fps_max * SC3336P_VMAX_300W20FPS_LINEAR) / fullLines);
    tempfps = (tempfps/100) * 100;

    if (tempfps < fps_min)
    {
        tempfps = fps_min;
    }
    else if (tempfps > fps_max)
    {
        tempfps = fps_max;
    }

    fullLines = (vsi_u32_t)(((vsi_u64_t)SC3336P_VMAX_300W20FPS_LINEAR * 20 * ISP_SNS_FPS_ACCU) / tempfps);

    pAeSnsDft->fullLines = fullLines;

    pSnsRegsInfo->snsData[REG_VTS_H].data = ((fullLines & 0xFF00) >> 8);
    pSnsRegsInfo->snsData[REG_VTS_L].data = (fullLines & 0xFF);

    switch(psc3336pDev->snsModeId) {
        case SC3336P_300W20FPS_LINEAR_MODE:
        case SC3336P_300W10FPS_LINEAR_MODE:
        case SC3336P_300W15FPS_LINEAR_MODE:
        case SC3336P_1080P25FPS_LINEAR_MODE:
            pAeSnsDft->maxIntLine = fullLines - 8;
            pAeSnsDft->fps = tempfps;
            break;
        default:
            return BK_FAIL;
            break;
    }

    return VSI_SUCCESS;
}

static int sc3336p_GetSnsRegInfo(ISP_PORT IspPort, ISP_SNS_REGS_INFO_S *pSnsRegsInfo)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    os_memcpy(pSnsRegsInfo, &psc3336pDev->snsRegsInfo, sizeof(*pSnsRegsInfo));

    return BK_OK;
}

static int sc3336p_IntTimeUpdate(ISP_PORT IspPort, vsi_u32_t *pIntLine)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ISP_SNS_REGS_INFO_S *pSnsRegsInfo = &psc3336pDev->snsRegsInfo;

    switch(psc3336pDev->snsModeId) {
        case SC3336P_300W10FPS_LINEAR_MODE:
        case SC3336P_300W15FPS_LINEAR_MODE:
        case SC3336P_300W20FPS_LINEAR_MODE:
        case SC3336P_1080P25FPS_LINEAR_MODE:
            pSnsRegsInfo->snsData[REG_EXPOSURE_H].data = ((*pIntLine >> 12) & 0x0F);
            pSnsRegsInfo->snsData[REG_EXPOSURE_M].data = ((*pIntLine >> 4) & 0xFF);
            pSnsRegsInfo->snsData[REG_EXPOSURE_L].data = (*pIntLine & 0xF) << 4;
            break;
        default:
            break;
    }

    return BK_OK;
}

static void sc3336p_CalcGain(vsi_u32_t *pAgain, vsi_u32_t *gain_3e06, vsi_u32_t *gain_3e07, vsi_u32_t *gain_3e09)
{
    uint32_t total_gain = *pAgain;
    uint16_t a_gain = 0, d_gain = 0;
    total_gain = total_gain < 0x40 ? 0x40 : total_gain;
    *gain_3e09 = 0x00;

    if (total_gain < 97)
    {
        *gain_3e09 = 0x00;
        a_gain = 64;
    }
    else if (total_gain < 194)
    {
        *gain_3e09 = 0x40;
        a_gain = 97;
    }
    else if (total_gain < 389)
    {
        *gain_3e09 = 0x48;
        a_gain = 194;
    }
    else if (total_gain < 778)
    {
        *gain_3e09 = 0x49;
        a_gain = 389;
    }
    else if (total_gain < 1556)
    {
        *gain_3e09 = 0x4b;
        a_gain = 778;
    }
    else if (total_gain < 3112)
    {
        *gain_3e09 = 0x4f;
        a_gain = 1556;
    }
    else
    {
        *gain_3e09 = 0x5f;
        a_gain = 3112;
    }

    d_gain = (total_gain << 6) / a_gain;
    if (d_gain % 2)
        d_gain++;

    d_gain = d_gain < 64 ? 64 : d_gain;
    d_gain = d_gain > 1008 ? 1008 : d_gain;

    uint8_t i = 0;
    for (i = 0; (d_gain >> i) > 0; i++)
    {
    };

    *gain_3e06 = (1 << (i - 7)) - 1;
    *gain_3e07 = (d_gain << 1) >> (i - 7);
}

static int sc3336p_GainUpdate(ISP_PORT IspPort, vsi_u32_t *pAgain, vsi_u32_t *pDgain)
{
    sc3336p_DEVICE_S *psc3336pDev = sc3336pDev[IspPort.devId][IspPort.portId];
    if (psc3336pDev == NULL)
    {
        LOGE("%s %d failed\n", __func__, __LINE__);
        return BK_FAIL;
    }

    switch(psc3336pDev->snsModeId) {
        case SC3336P_300W10FPS_LINEAR_MODE:
        case SC3336P_300W15FPS_LINEAR_MODE:
        case SC3336P_300W20FPS_LINEAR_MODE:
        case SC3336P_1080P25FPS_LINEAR_MODE:
            *pDgain = 1024;
            vsi_u32_t gain_3e06 = 0, gain_3e07 = 0, gain_3e09 = 0;
            sc3336p_CalcGain(pAgain, &gain_3e06, &gain_3e07, &gain_3e09);
            psc3336pDev->snsRegsInfo.snsData[REG_GAIN_3E09].data = gain_3e09;
            psc3336pDev->snsRegsInfo.snsData[REG_GAIN_3E06].data = gain_3e06;
            psc3336pDev->snsRegsInfo.snsData[REG_GAIN_3E07].data = gain_3e07;
            break;
        default:
            break;
    }

    return BK_OK;
}

static int sc3336p_InitIspSnsFunc(ISP_SNS_FUNC_S *pIspSnsFunc)
{
    pIspSnsFunc->pfnSensorInit    = sc3336p_SensorInit;
    pIspSnsFunc->pfnSensorExit    = sc3336p_SensorExit;
    pIspSnsFunc->pfnWriteReg      = sc3336p_WriteReg;
    pIspSnsFunc->pfnReadReg       = sc3336p_ReadReg;
    pIspSnsFunc->pfnSetMode       = sc3336p_SetMode;
    pIspSnsFunc->pfnSetStream     = sc3336p_SetStream;
    pIspSnsFunc->pfnSetIspDefault = sc3336p_SetIspDefault;

    return AVDK_ERR_OK;
}

static int sc3336p_InitAeSnsFunc(AE_SNS_FUNC_S *pAeSnsFunc)
{
    pAeSnsFunc->pfnGetAeDefault  = sc3336p_GetAeDefault;
    pAeSnsFunc->pfnSetFps        = sc3336p_SetFps;
    pAeSnsFunc->pfnSlowFrameRate = sc3336p_SlowFrameRate;
    pAeSnsFunc->pfnIntTimeUpdate = sc3336p_IntTimeUpdate;
    pAeSnsFunc->pfnGainUpdate    = sc3336p_GainUpdate;
    pAeSnsFunc->pfnSetExpRatio   = NULL;
    pAeSnsFunc->pfnGetSnsRegInfo = sc3336p_GetSnsRegInfo;

    return AVDK_ERR_OK;
}

const ISP_SNS_OBJ_S snssc3336pObj = {
    .pfnInitIspSnsFunc = sc3336p_InitIspSnsFunc,
    .pfnInitAeSnsFunc  = sc3336p_InitAeSnsFunc,
};


static const ISP_PUB_ATTR_S sc3336p_mipi_linear_attr = {
    .pSnsObj      = (void*)&snssc3336pObj,
    .ispInputType = INPUT_TYPE_SENSOR,
    .ispMode      = ISP_MODE_RAW,
    .hdrMode      = HDR_MODE_LINEAR,
    .pixelFormat  = PIXEL_FORMAT_BGGR10,
    .snsFps      = 30 * ISP_SNS_FPS_ACCU,
};

static const bk_camera_sensor_format_t sc3336p_format_array[] = {
    {
        .width = 2304,
        .height = 1296,
        .fps = 20,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },

    {
        .width = 2304,
        .height = 1296,
        .fps = 10,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },

    {
        .width = 2304,
        .height = 1296,
        .fps = 15,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },

    {
        .width = 1920,
        .height = 1080,
        .fps = 25,
        .output_pixel_fmt = BK_PIXEL_FORMAT_BGGR10,
    },
};

static avdk_err_t sc3336p_init(bk_camera_sensor_ctlr_t *controller)
{

    bk_mipi_csi_ext_set_enable(0);

    bk_mipi_csi_phy_term_set(0x303, 0x808);

    return 0;
}

static bool s_sc3336p_hmirror;
static bool s_sc3336p_vflip;
static uint16_t s_sc3336p_base_win_x;
static uint16_t s_sc3336p_base_win_y;

static void sc3336p_capture_window_base(bk_camera_bus_t *bus)
{
    uint8_t hb = 0;
    uint8_t lb = 0;

    bus->read16(bus, 0x3210, &hb);
    bus->read16(bus, 0x3211, &lb);
    s_sc3336p_base_win_x = (uint16_t)((hb << 8) | lb);
    bus->read16(bus, 0x3212, &hb);
    bus->read16(bus, 0x3213, &lb);
    s_sc3336p_base_win_y = (uint16_t)((hb << 8) | lb);
}

static void sc3336p_write_window_regs(bk_camera_bus_t *bus)
{
    uint16_t win_x = s_sc3336p_base_win_x;
    uint16_t win_y = s_sc3336p_base_win_y;

    /* Toggle window start by 1 pixel to preserve BGGR Bayer phase. */
    if (s_sc3336p_hmirror)
    {
        win_x ^= 1;
    }
    if (s_sc3336p_vflip)
    {
        win_y ^= 1;
    }

    bus->write16(bus, 0x3210, UINT16_HB(win_x));
    bus->write16(bus, 0x3211, UINT16_LB(win_x));
    bus->write16(bus, 0x3212, UINT16_HB(win_y));
    bus->write16(bus, 0x3213, UINT16_LB(win_y));
}

static void sc3336p_apply_mirror_reg(bk_camera_bus_t *bus)
{
    uint8_t tmp;

    if (bus == NULL)
    {
        return;
    }

    tmp = 0;
    if (s_sc3336p_hmirror)
    {
        tmp |= SC3336P_MIRROR_BITS;
    }
    if (s_sc3336p_vflip)
    {
        tmp |= SC3336P_VFLIP_BITS;
    }
    bus->write16(bus, SC3336P_REG_MIRROR_FLIP, tmp);

    if (s_sc3336p_width == 1920
        && s_sc3336p_height == 1080)
    {
        sc3336p_write_window_regs(bus);
    }
}

static avdk_err_t sc3336p_set_ppi(bk_camera_sensor_ctlr_t *controller, uint16_t width, uint16_t height)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (width > WIN_MAX_X || width <= 0 || height > WIN_MAX_Y || height <= 0)
    {
        LOGE("%s, %d width: %d, height: %d, fail...\n", __func__, __LINE__, width, height);
        return -1;
    }

    if (width == 2304 && height == 1296) {
        uint32_t size = sc3336p_TABLE_SIZE(sensor_sc3336p_init_table);
        for (int i = 0; i < size; i++) {
            bus->write16(bus, sensor_sc3336p_init_table[i][0], sensor_sc3336p_init_table[i][1]);
        }
    } else if (width == 1920 && height == 1080) {
        uint32_t size = sc3336p_TABLE_SIZE(sensor_sc3336p_1080P25_init_table);
        for (int i = 0; i < size; i++) {
            bus->write16(bus, sensor_sc3336p_1080P25_init_table[i][0], sensor_sc3336p_1080P25_init_table[i][1]);
        }
    } else {
        LOGE("not supported width: %d, height: %d\n", width, height);
        return -1;
    }

    bk_mipi_csi_controller_init(width, height, 0x2b);

    s_sc3336p_width = width;
    s_sc3336p_height = height;

    if (width == 1920 && height == 1080)
    {
        sc3336p_capture_window_base(bus);
    }

    return 0;
}

static avdk_err_t sc3336p_set_fps(bk_camera_sensor_ctlr_t *controller, uint16_t fps)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (s_sc3336p_width == 2304 && s_sc3336p_height == 1296) {
        uint32_t pclk = SC3336P_PCLK;
        uint16_t vts = pclk / SC3336P_HTS / fps;
        bus->write16(bus, SC3336P_REG_VTS_L, (vts & 0xff));
        bus->write16(bus, SC3336P_REG_VTS_H, (vts >> 8));
    } else if (s_sc3336p_width == 1920 && s_sc3336p_height == 1080) {
        uint32_t pclk = 2500 * 1360 * 30;
        uint16_t vts = pclk / 2500 / fps;
        bus->write16(bus, SC3336P_REG_VTS_L, (vts & 0xff));
        bus->write16(bus, SC3336P_REG_VTS_H, (vts >> 8));
    }

    return BK_OK;
}

static avdk_err_t sc3336p_set_hmirror(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    s_sc3336p_hmirror = enable;
    sc3336p_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t sc3336p_set_vflip(bk_camera_sensor_ctlr_t *controller, bool enable)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");

    s_sc3336p_vflip = enable;
    sc3336p_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t sc3336p_set_format(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_t *format)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    AVDK_RETURN_ON_FALSE(format, AVDK_ERR_INVAL, TAG, "format is NULL");
    sc3336p_apply_mirror_reg(csi_sensor->config.bus);
    sc3336p_set_ppi(controller, format->width, format->height);
    sc3336p_set_fps(controller, format->fps);
    bk_mipi_csi_controller_reset();
    sc3336p_apply_mirror_reg(csi_sensor->config.bus);
    return AVDK_ERR_OK;
}

static avdk_err_t sc3336p_ctrl(bk_camera_sensor_ctlr_t *controller, uint8_t cmd, uint16_t addr, uint8_t val)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, AVDK_ERR_INVAL, TAG, "csi sensor is NULL");
    bk_camera_bus_t *bus = csi_sensor->config.bus;

    if (cmd == 0) // sensor reg read
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("sc3336p {%04x, %02x}\n", addr, dump_val);
    }

    if (cmd == 1) // sensor reg write
    {
        uint8_t dump_val;
        bus->read16(bus, addr, &dump_val);
        LOGI("sc3336p {%04x, %02x} -> {%04x, %02x}\n", addr, dump_val, addr, val);
        bus->write16(bus, addr, val);
    }

    if (cmd == 2) // standy, stream stop
    {
        LOGI("sc3336p stream stop\n");
        bus->write16(bus, 0x0100, 0x00);
    }

    if (cmd == 3) // resume from standy, stream start
    {
        LOGI("sc3336p stream start\n");
        bus->write16(bus, 0x0100, 0x01);
    }

    return 0;
}

static void *sc3336p_get_sensor_object(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)&snssc3336pObj;
}

static void *sc3336p_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
{
    bk_camera_csi_sensor_t *csi_sensor = __containerof(controller, bk_camera_csi_sensor_t, ops);
    AVDK_RETURN_ON_FALSE(csi_sensor, NULL, TAG, "csi sensor is NULL");
    return (void*)csi_sensor->sensor_config;
}

static avdk_err_t sc3336p_query_support_formats(bk_camera_sensor_ctlr_t *controller, bk_camera_sensor_format_array_t *format_array)
{
    AVDK_RETURN_ON_FALSE(format_array, AVDK_ERR_INVAL, TAG, "format array is NULL");
    format_array->format_array = &sc3336p_format_array[0];
    format_array->size = ARRAY_SIZE(sc3336p_format_array);
    return AVDK_ERR_OK;
}


avdk_err_t sc3336p_detect(bk_camera_sensor_handle_t *handle, bk_camera_sensor_config_t *config)
{
    uint8_t hb_id = 0, lb_id;
    config->bus->write_address = SC3336P_WRITE_ADDRESS;

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
    config->bus->write_address = SC3336P_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = sc3336p_init;
    csi_sensor->ops.set_format = sc3336p_set_format;
    csi_sensor->ops.reg_ctrl = sc3336p_ctrl;
    csi_sensor->ops.set_hmirror = sc3336p_set_hmirror;
    csi_sensor->ops.set_vflip = sc3336p_set_vflip;
    csi_sensor->ops.get_sensor_object = sc3336p_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = sc3336p_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = sc3336p_query_support_formats;

    csi_sensor->isp_pub_attr = &sc3336p_mipi_linear_attr;
    csi_sensor->sensor_config = NULL;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return AVDK_ERR_OK;
}

BK_CAMERA_SENSOR_DETECT_SECTION(sc3336p_detect, CSI_CAMERA_PORT);
