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

typedef struct {
    vsi_u8_t again_reg;
    vsi_u8_t dgain_low;
    vsi_u8_t dgain_high;
    vsi_u32_t gain;
} cv2005_gain_lut_t;

static const cv2005_gain_lut_t s_cv2005_gain_lut[] = {
    { 0x0, 0x40, 0x0, 1024 }, // 1.0x
    { 0x1, 0x40, 0x0, 1028 }, // 1.004x
    { 0x2, 0x40, 0x0, 1032 }, // 1.008x
    { 0x3, 0x40, 0x0, 1036 }, // 1.012x
    { 0x4, 0x40, 0x0, 1040 }, // 1.016x
    { 0x5, 0x40, 0x0, 1044 }, // 1.02x
    { 0x6, 0x40, 0x0, 1048 }, // 1.023x
    { 0x7, 0x40, 0x0, 1052 }, // 1.027x
    { 0x8, 0x40, 0x0, 1057 }, // 1.032x
    { 0x9, 0x40, 0x0, 1061 }, // 1.036x
    { 0xa, 0x40, 0x0, 1065 }, // 1.04x
    { 0xb, 0x40, 0x0, 1069 }, // 1.044x
    { 0xc, 0x40, 0x0, 1074 }, // 1.049x
    { 0xd, 0x40, 0x0, 1078 }, // 1.053x
    { 0xe, 0x40, 0x0, 1083 }, // 1.058x
    { 0xf, 0x40, 0x0, 1087 }, // 1.062x
    { 0x10, 0x40, 0x0, 1092 }, // 1.066x
    { 0x11, 0x40, 0x0, 1096 }, // 1.07x
    { 0x12, 0x40, 0x0, 1101 }, // 1.075x
    { 0x13, 0x40, 0x0, 1106 }, // 1.08x
    { 0x14, 0x40, 0x0, 1110 }, // 1.084x
    { 0x15, 0x40, 0x0, 1115 }, // 1.089x
    { 0x16, 0x40, 0x0, 1120 }, // 1.094x
    { 0x17, 0x40, 0x0, 1125 }, // 1.099x
    { 0x18, 0x40, 0x0, 1129 }, // 1.103x
    { 0x19, 0x40, 0x0, 1134 }, // 1.107x
    { 0x1a, 0x40, 0x0, 1139 }, // 1.112x
    { 0x1b, 0x40, 0x0, 1144 }, // 1.117x
    { 0x1c, 0x40, 0x0, 1149 }, // 1.122x
    { 0x1d, 0x40, 0x0, 1154 }, // 1.127x
    { 0x1e, 0x40, 0x0, 1159 }, // 1.132x
    { 0x1f, 0x40, 0x0, 1165 }, // 1.138x
    { 0x20, 0x40, 0x0, 1170 }, // 1.143x
    { 0x21, 0x40, 0x0, 1175 }, // 1.147x
    { 0x22, 0x40, 0x0, 1180 }, // 1.152x
    { 0x23, 0x40, 0x0, 1186 }, // 1.158x
    { 0x24, 0x40, 0x0, 1191 }, // 1.163x
    { 0x25, 0x40, 0x0, 1197 }, // 1.169x
    { 0x26, 0x40, 0x0, 1202 }, // 1.174x
    { 0x27, 0x40, 0x0, 1208 }, // 1.18x
    { 0x28, 0x40, 0x0, 1213 }, // 1.185x
    { 0x29, 0x40, 0x0, 1219 }, // 1.19x
    { 0x2a, 0x40, 0x0, 1224 }, // 1.195x
    { 0x2b, 0x40, 0x0, 1230 }, // 1.201x
    { 0x2c, 0x40, 0x0, 1236 }, // 1.207x
    { 0x2d, 0x40, 0x0, 1242 }, // 1.213x
    { 0x2e, 0x40, 0x0, 1248 }, // 1.219x
    { 0x2f, 0x40, 0x0, 1254 }, // 1.225x
    { 0x30, 0x40, 0x0, 1260 }, // 1.23x
    { 0x31, 0x40, 0x0, 1266 }, // 1.236x
    { 0x32, 0x40, 0x0, 1272 }, // 1.242x
    { 0x33, 0x40, 0x0, 1278 }, // 1.248x
    { 0x34, 0x40, 0x0, 1285 }, // 1.255x
    { 0x35, 0x40, 0x0, 1291 }, // 1.261x
    { 0x36, 0x40, 0x0, 1297 }, // 1.267x
    { 0x37, 0x40, 0x0, 1304 }, // 1.273x
    { 0x38, 0x40, 0x0, 1310 }, // 1.279x
    { 0x39, 0x40, 0x0, 1317 }, // 1.286x
    { 0x3a, 0x40, 0x0, 1323 }, // 1.292x
    { 0x3b, 0x40, 0x0, 1330 }, // 1.299x
    { 0x3c, 0x40, 0x0, 1337 }, // 1.306x
    { 0x3d, 0x40, 0x0, 1344 }, // 1.313x
    { 0x3e, 0x40, 0x0, 1351 }, // 1.319x
    { 0x3f, 0x40, 0x0, 1358 }, // 1.326x
    { 0x40, 0x40, 0x0, 1365 }, // 1.333x
    { 0x41, 0x40, 0x0, 1372 }, // 1.34x
    { 0x42, 0x40, 0x0, 1379 }, // 1.347x
    { 0x43, 0x40, 0x0, 1387 }, // 1.354x
    { 0x44, 0x40, 0x0, 1394 }, // 1.361x
    { 0x45, 0x40, 0x0, 1401 }, // 1.368x
    { 0x46, 0x40, 0x0, 1409 }, // 1.376x
    { 0x47, 0x40, 0x0, 1416 }, // 1.383x
    { 0x48, 0x40, 0x0, 1424 }, // 1.391x
    { 0x49, 0x40, 0x0, 1432 }, // 1.398x
    { 0x4a, 0x40, 0x0, 1440 }, // 1.406x
    { 0x4b, 0x40, 0x0, 1448 }, // 1.414x
    { 0x4c, 0x40, 0x0, 1456 }, // 1.422x
    { 0x4d, 0x40, 0x0, 1464 }, // 1.43x
    { 0x4e, 0x40, 0x0, 1472 }, // 1.438x
    { 0x4f, 0x40, 0x0, 1481 }, // 1.446x
    { 0x50, 0x40, 0x0, 1489 }, // 1.454x
    { 0x51, 0x40, 0x0, 1497 }, // 1.462x
    { 0x52, 0x40, 0x0, 1506 }, // 1.471x
    { 0x53, 0x40, 0x0, 1515 }, // 1.479x
    { 0x54, 0x40, 0x0, 1524 }, // 1.488x
    { 0x55, 0x40, 0x0, 1533 }, // 1.497x
    { 0x56, 0x40, 0x0, 1542 }, // 1.506x
    { 0x57, 0x40, 0x0, 1551 }, // 1.515x
    { 0x58, 0x40, 0x0, 1560 }, // 1.523x
    { 0x59, 0x40, 0x0, 1569 }, // 1.532x
    { 0x5a, 0x40, 0x0, 1579 }, // 1.542x
    { 0x5b, 0x40, 0x0, 1588 }, // 1.551x
    { 0x5c, 0x40, 0x0, 1598 }, // 1.561x
    { 0x5d, 0x40, 0x0, 1608 }, // 1.57x
    { 0x5e, 0x40, 0x0, 1618 }, // 1.58x
    { 0x5f, 0x40, 0x0, 1628 }, // 1.59x
    { 0x60, 0x40, 0x0, 1638 }, // 1.6x
    { 0x61, 0x40, 0x0, 1648 }, // 1.609x
    { 0x62, 0x40, 0x0, 1659 }, // 1.62x
    { 0x63, 0x40, 0x0, 1669 }, // 1.63x
    { 0x64, 0x40, 0x0, 1680 }, // 1.641x
    { 0x65, 0x40, 0x0, 1691 }, // 1.651x
    { 0x66, 0x40, 0x0, 1702 }, // 1.662x
    { 0x67, 0x40, 0x0, 1713 }, // 1.673x
    { 0x68, 0x40, 0x0, 1724 }, // 1.684x
    { 0x69, 0x40, 0x0, 1736 }, // 1.695x
    { 0x6a, 0x40, 0x0, 1747 }, // 1.706x
    { 0x6b, 0x40, 0x0, 1759 }, // 1.718x
    { 0x6c, 0x40, 0x0, 1771 }, // 1.729x
    { 0x6d, 0x40, 0x0, 1783 }, // 1.741x
    { 0x6e, 0x40, 0x0, 1795 }, // 1.753x
    { 0x6f, 0x40, 0x0, 1807 }, // 1.765x
    { 0x70, 0x40, 0x0, 1820 }, // 1.777x
    { 0x71, 0x40, 0x0, 1833 }, // 1.79x
    { 0x72, 0x40, 0x0, 1846 }, // 1.803x
    { 0x73, 0x40, 0x0, 1859 }, // 1.815x
    { 0x74, 0x40, 0x0, 1872 }, // 1.828x
    { 0x75, 0x40, 0x0, 1885 }, // 1.841x
    { 0x76, 0x40, 0x0, 1899 }, // 1.854x
    { 0x77, 0x40, 0x0, 1913 }, // 1.868x
    { 0x78, 0x40, 0x0, 1927 }, // 1.882x
    { 0x79, 0x40, 0x0, 1941 }, // 1.896x
    { 0x7a, 0x40, 0x0, 1956 }, // 1.91x
    { 0x7b, 0x40, 0x0, 1971 }, // 1.925x
    { 0x7c, 0x40, 0x0, 1985 }, // 1.938x
    { 0x7d, 0x40, 0x0, 2001 }, // 1.954x
    { 0x7e, 0x40, 0x0, 2016 }, // 1.969x
    { 0x7f, 0x40, 0x0, 2032 }, // 1.984x
    { 0x80, 0x40, 0x0, 2048 }, // 2.0x
    { 0x81, 0x40, 0x0, 2064 }, // 2.016x
    { 0x82, 0x40, 0x0, 2080 }, // 2.031x
    { 0x83, 0x40, 0x0, 2097 }, // 2.048x
    { 0x84, 0x40, 0x0, 2114 }, // 2.064x
    { 0x85, 0x40, 0x0, 2131 }, // 2.081x
    { 0x86, 0x40, 0x0, 2148 }, // 2.098x
    { 0x87, 0x40, 0x0, 2166 }, // 2.115x
    { 0x88, 0x40, 0x0, 2184 }, // 2.133x
    { 0x89, 0x40, 0x0, 2202 }, // 2.15x
    { 0x8a, 0x40, 0x0, 2221 }, // 2.169x
    { 0x8b, 0x40, 0x0, 2240 }, // 2.188x
    { 0x8c, 0x40, 0x0, 2259 }, // 2.206x
    { 0x8d, 0x40, 0x0, 2279 }, // 2.226x
    { 0x8e, 0x40, 0x0, 2299 }, // 2.245x
    { 0x8f, 0x40, 0x0, 2319 }, // 2.265x
    { 0x90, 0x40, 0x0, 2340 }, // 2.285x
    { 0x91, 0x40, 0x0, 2361 }, // 2.306x
    { 0x92, 0x40, 0x0, 2383 }, // 2.327x
    { 0x93, 0x40, 0x0, 2404 }, // 2.348x
    { 0x94, 0x40, 0x0, 2427 }, // 2.37x
    { 0x95, 0x40, 0x0, 2449 }, // 2.392x
    { 0x96, 0x40, 0x0, 2473 }, // 2.415x
    { 0x97, 0x40, 0x0, 2496 }, // 2.438x
    { 0x98, 0x40, 0x0, 2520 }, // 2.461x
    { 0x99, 0x40, 0x0, 2545 }, // 2.485x
    { 0x9a, 0x40, 0x0, 2570 }, // 2.51x
    { 0x9b, 0x40, 0x0, 2595 }, // 2.534x
    { 0x9c, 0x40, 0x0, 2621 }, // 2.56x
    { 0x9d, 0x40, 0x0, 2647 }, // 2.585x
    { 0x9e, 0x40, 0x0, 2674 }, // 2.611x
    { 0x9f, 0x40, 0x0, 2702 }, // 2.639x
    { 0xa0, 0x40, 0x0, 2730 }, // 2.666x
    { 0xa1, 0x40, 0x0, 2759 }, // 2.694x
    { 0xa2, 0x40, 0x0, 2788 }, // 2.723x
    { 0xa3, 0x40, 0x0, 2818 }, // 2.752x
    { 0xa4, 0x40, 0x0, 2849 }, // 2.782x
    { 0xa5, 0x40, 0x0, 2880 }, // 2.813x
    { 0xa6, 0x40, 0x0, 2912 }, // 2.844x
    { 0xa7, 0x40, 0x0, 2945 }, // 2.876x
    { 0xa8, 0x40, 0x0, 2978 }, // 2.908x
    { 0xa9, 0x40, 0x0, 3013 }, // 2.942x
    { 0xaa, 0x40, 0x0, 3048 }, // 2.977x
    { 0xab, 0x40, 0x0, 3084 }, // 3.012x
    { 0xac, 0x40, 0x0, 3120 }, // 3.047x
    { 0xad, 0x40, 0x0, 3158 }, // 3.084x
    { 0xae, 0x40, 0x0, 3196 }, // 3.121x
    { 0xaf, 0x40, 0x0, 3236 }, // 3.16x
    { 0xb0, 0x40, 0x0, 3276 }, // 3.199x
    { 0xb1, 0x40, 0x0, 3318 }, // 3.24x
    { 0xb2, 0x40, 0x0, 3360 }, // 3.281x
    { 0xb3, 0x40, 0x0, 3404 }, // 3.324x
    { 0xb4, 0x40, 0x0, 3449 }, // 3.368x
    { 0xb5, 0x40, 0x0, 3495 }, // 3.413x
    { 0xb6, 0x40, 0x0, 3542 }, // 3.459x
    { 0xb7, 0x40, 0x0, 3591 }, // 3.507x
    { 0xb8, 0x40, 0x0, 3640 }, // 3.555x
    { 0xb9, 0x40, 0x0, 3692 }, // 3.605x
    { 0xba, 0x40, 0x0, 3744 }, // 3.656x
    { 0xbb, 0x40, 0x0, 3799 }, // 3.71x
    { 0xbc, 0x40, 0x0, 3855 }, // 3.765x
    { 0xbd, 0x40, 0x0, 3912 }, // 3.82x
    { 0xbe, 0x40, 0x0, 3971 }, // 3.878x
    { 0xbf, 0x40, 0x0, 4032 }, // 3.938x
    { 0xc0, 0x40, 0x0, 4096 }, // 4.0x
    { 0xc1, 0x40, 0x0, 4161 }, // 4.063x
    { 0xc2, 0x40, 0x0, 4228 }, // 4.129x
    { 0xc3, 0x40, 0x0, 4297 }, // 4.196x
    { 0xc4, 0x40, 0x0, 4369 }, // 4.267x
    { 0xc5, 0x40, 0x0, 4443 }, // 4.339x
    { 0xc6, 0x40, 0x0, 4519 }, // 4.413x
    { 0xc7, 0x40, 0x0, 4599 }, // 4.491x
    { 0xc8, 0x40, 0x0, 4681 }, // 4.571x
    { 0xc9, 0x40, 0x0, 4766 }, // 4.654x
    { 0xca, 0x40, 0x0, 4854 }, // 4.74x
    { 0xcb, 0x40, 0x0, 4946 }, // 4.83x
    { 0xcc, 0x40, 0x0, 5041 }, // 4.923x
    { 0xcd, 0x40, 0x0, 5140 }, // 5.02x
    { 0xce, 0x40, 0x0, 5242 }, // 5.119x
    { 0xcf, 0x40, 0x0, 5349 }, // 5.224x
    { 0xd0, 0x40, 0x0, 5461 }, // 5.333x
    { 0xd1, 0x40, 0x0, 5577 }, // 5.446x
    { 0xd2, 0x40, 0x0, 5698 }, // 5.564x
    { 0xd3, 0x40, 0x0, 5825 }, // 5.688x
    { 0xd4, 0x40, 0x0, 5957 }, // 5.817x
    { 0xd5, 0x40, 0x0, 6096 }, // 5.953x
    { 0xd6, 0x40, 0x0, 6241 }, // 6.095x
    { 0xd7, 0x40, 0x0, 6393 }, // 6.243x
    { 0xd8, 0x40, 0x0, 6553 }, // 6.399x
    { 0xd9, 0x40, 0x0, 6721 }, // 6.563x
    { 0xda, 0x40, 0x0, 6898 }, // 6.736x
    { 0xdb, 0x40, 0x0, 7084 }, // 6.918x
    { 0xdc, 0x40, 0x0, 7281 }, // 7.11x
    { 0xdd, 0x40, 0x0, 7489 }, // 7.313x
    { 0xde, 0x40, 0x0, 7710 }, // 7.529x
    { 0xdf, 0x40, 0x0, 7943 }, // 7.757x
    { 0xe0, 0x40, 0x0, 8192 }, // 8.0x
    { 0xe1, 0x40, 0x0, 8456 }, // 8.258x
    { 0xe2, 0x40, 0x0, 8738 }, // 8.533x
    { 0xe3, 0x40, 0x0, 9039 }, // 8.827x
    { 0xe4, 0x40, 0x0, 9362 }, // 9.143x
    { 0xe5, 0x40, 0x0, 9709 }, // 9.481x
    { 0xe6, 0x40, 0x0, 10082 }, // 9.846x
    { 0xe7, 0x40, 0x0, 10485 }, // 10.239x
    { 0xe8, 0x40, 0x0, 10922 }, // 10.666x
    { 0xe9, 0x40, 0x0, 11397 }, // 11.13x
    { 0xea, 0x40, 0x0, 11915 }, // 11.636x
    { 0xeb, 0x40, 0x0, 12483 }, // 12.19x
    { 0xec, 0x40, 0x0, 13107 }, // 12.8x
    { 0xed, 0x40, 0x0, 13797 }, // 13.474x
    { 0xee, 0x40, 0x0, 14563 }, // 14.222x
    { 0xef, 0x40, 0x0, 15420 }, // 15.059x
    { 0xf0, 0x40, 0x0, 16384 }, // 16.0x
    { 0xf1, 0x40, 0x0, 17476 }, // 17.066x
    { 0xf2, 0x40, 0x0, 18724 }, // 18.285x
    { 0xf3, 0x40, 0x0, 20164 }, // 19.691x
    { 0xf4, 0x40, 0x0, 21845 }, // 21.333x
    { 0xf5, 0x40, 0x0, 23831 }, // 23.272x
    { 0xf6, 0x40, 0x0, 26214 }, // 25.6x
    { 0xf7, 0x40, 0x0, 29127 }, // 28.444x
    { 0xf8, 0x40, 0x0, 32768 }, // 32.0x
    { 0xf8, 0x42, 0x0, 33792 }, // 33.0x
    { 0xf8, 0x44, 0x0, 34816 }, // 34.0x
    { 0xf8, 0x46, 0x0, 35840 }, // 35.0x
    { 0xf8, 0x48, 0x0, 36864 }, // 36.0x
    { 0xf8, 0x4a, 0x0, 37888 }, // 37.0x
    { 0xf8, 0x4c, 0x0, 38912 }, // 38.0x
    { 0xf8, 0x4e, 0x0, 39936 }, // 39.0x
    { 0xf8, 0x50, 0x0, 40960 }, // 40.0x
    { 0xf8, 0x52, 0x0, 41984 }, // 41.0x
    { 0xf8, 0x54, 0x0, 43008 }, // 42.0x
    { 0xf8, 0x56, 0x0, 44032 }, // 43.0x
    { 0xf8, 0x58, 0x0, 45056 }, // 44.0x
    { 0xf8, 0x5a, 0x0, 46080 }, // 45.0x
    { 0xf8, 0x5c, 0x0, 47104 }, // 46.0x
    { 0xf8, 0x5e, 0x0, 48128 }, // 47.0x
    { 0xf8, 0x60, 0x0, 49152 }, // 48.0x
    { 0xf8, 0x62, 0x0, 50176 }, // 49.0x
    { 0xf8, 0x64, 0x0, 51200 }, // 50.0x
    { 0xf8, 0x66, 0x0, 52224 }, // 51.0x
    { 0xf8, 0x68, 0x0, 53248 }, // 52.0x
    { 0xf8, 0x6a, 0x0, 54272 }, // 53.0x
    { 0xf8, 0x6c, 0x0, 55296 }, // 54.0x
    { 0xf8, 0x6e, 0x0, 56320 }, // 55.0x
    { 0xf8, 0x70, 0x0, 57344 }, // 56.0x
    { 0xf8, 0x72, 0x0, 58368 }, // 57.0x
    { 0xf8, 0x74, 0x0, 59392 }, // 58.0x
    { 0xf8, 0x76, 0x0, 60416 }, // 59.0x
    { 0xf8, 0x78, 0x0, 61440 }, // 60.0x
    { 0xf8, 0x7a, 0x0, 62464 }, // 61.0x
    { 0xf8, 0x7c, 0x0, 63488 }, // 62.0x
    { 0xf8, 0x7e, 0x0, 64512 }, // 63.0x
    { 0xf8, 0x80, 0x0, 65536 }, // 64.0x
    { 0xf8, 0x82, 0x0, 66560 }, // 65.0x
    { 0xf8, 0x84, 0x0, 67584 }, // 66.0x
    { 0xf8, 0x86, 0x0, 68608 }, // 67.0x
    { 0xf8, 0x88, 0x0, 69632 }, // 68.0x
    { 0xf8, 0x8a, 0x0, 70656 }, // 69.0x
    { 0xf8, 0x8c, 0x0, 71680 }, // 70.0x
    { 0xf8, 0x8e, 0x0, 72704 }, // 71.0x
    { 0xf8, 0x90, 0x0, 73728 }, // 72.0x
    { 0xf8, 0x92, 0x0, 74752 }, // 73.0x
    { 0xf8, 0x94, 0x0, 75776 }, // 74.0x
    { 0xf8, 0x96, 0x0, 76800 }, // 75.0x
    { 0xf8, 0x98, 0x0, 77824 }, // 76.0x
    { 0xf8, 0x9a, 0x0, 78848 }, // 77.0x
    { 0xf8, 0x9c, 0x0, 79872 }, // 78.0x
    { 0xf8, 0x9e, 0x0, 80896 }, // 79.0x
    { 0xf8, 0xa0, 0x0, 81920 }, // 80.0x
    { 0xf8, 0xa2, 0x0, 82944 }, // 81.0x
    { 0xf8, 0xa4, 0x0, 83968 }, // 82.0x
    { 0xf8, 0xa6, 0x0, 84992 }, // 83.0x
    { 0xf8, 0xa8, 0x0, 86016 }, // 84.0x
    { 0xf8, 0xaa, 0x0, 87040 }, // 85.0x
    { 0xf8, 0xac, 0x0, 88064 }, // 86.0x
    { 0xf8, 0xae, 0x0, 89088 }, // 87.0x
    { 0xf8, 0xb0, 0x0, 90112 }, // 88.0x
    { 0xf8, 0xb2, 0x0, 91136 }, // 89.0x
    { 0xf8, 0xb4, 0x0, 92160 }, // 90.0x
    { 0xf8, 0xb6, 0x0, 93184 }, // 91.0x
    { 0xf8, 0xb8, 0x0, 94208 }, // 92.0x
    { 0xf8, 0xba, 0x0, 95232 }, // 93.0x
    { 0xf8, 0xbc, 0x0, 96256 }, // 94.0x
    { 0xf8, 0xbe, 0x0, 97280 }, // 95.0x
    { 0xf8, 0xc0, 0x0, 98304 }, // 96.0x
    { 0xf8, 0xc2, 0x0, 99328 }, // 97.0x
    { 0xf8, 0xc4, 0x0, 100352 }, // 98.0x
    { 0xf8, 0xc6, 0x0, 101376 }, // 99.0x
    { 0xf8, 0xc8, 0x0, 102400 }, // 100.0x
    { 0xf8, 0xca, 0x0, 103424 }, // 101.0x
    { 0xf8, 0xcc, 0x0, 104448 }, // 102.0x
    { 0xf8, 0xce, 0x0, 105472 }, // 103.0x
    { 0xf8, 0xd0, 0x0, 106496 }, // 104.0x
    { 0xf8, 0xd2, 0x0, 107520 }, // 105.0x
    { 0xf8, 0xd4, 0x0, 108544 }, // 106.0x
    { 0xf8, 0xd6, 0x0, 109568 }, // 107.0x
    { 0xf8, 0xd8, 0x0, 110592 }, // 108.0x
    { 0xf8, 0xda, 0x0, 111616 }, // 109.0x
    { 0xf8, 0xdc, 0x0, 112640 }, // 110.0x
    { 0xf8, 0xde, 0x0, 113664 }, // 111.0x
    { 0xf8, 0xe0, 0x0, 114688 }, // 112.0x
    { 0xf8, 0xe2, 0x0, 115712 }, // 113.0x
    { 0xf8, 0xe4, 0x0, 116736 }, // 114.0x
    { 0xf8, 0xe6, 0x0, 117760 }, // 115.0x
    { 0xf8, 0xe8, 0x0, 118784 }, // 116.0x
    { 0xf8, 0xea, 0x0, 119808 }, // 117.0x
    { 0xf8, 0xec, 0x0, 120832 }, // 118.0x
    { 0xf8, 0xee, 0x0, 121856 }, // 119.0x
    { 0xf8, 0xf0, 0x0, 122880 }, // 120.0x
    { 0xf8, 0xf2, 0x0, 123904 }, // 121.0x
    { 0xf8, 0xf4, 0x0, 124928 }, // 122.0x
    { 0xf8, 0xf6, 0x0, 125952 }, // 123.0x
    { 0xf8, 0xf8, 0x0, 126976 }, // 124.0x
    { 0xf8, 0xfa, 0x0, 128000 }, // 125.0x
    { 0xf8, 0xfc, 0x0, 129024 }, // 126.0x
    { 0xf8, 0xfe, 0x0, 130048 }, // 127.0x
    { 0xf8, 0x0, 0x1, 131072 }, // 128.0x
    { 0xf8, 0x2, 0x1, 132096 }, // 129.0x
    { 0xf8, 0x4, 0x1, 133120 }, // 130.0x
    { 0xf8, 0x6, 0x1, 134144 }, // 131.0x
    { 0xf8, 0x8, 0x1, 135168 }, // 132.0x
    { 0xf8, 0xa, 0x1, 136192 }, // 133.0x
    { 0xf8, 0xc, 0x1, 137216 }, // 134.0x
    { 0xf8, 0xe, 0x1, 138240 }, // 135.0x
    { 0xf8, 0x10, 0x1, 139264 }, // 136.0x
    { 0xf8, 0x12, 0x1, 140288 }, // 137.0x
    { 0xf8, 0x14, 0x1, 141312 }, // 138.0x
    { 0xf8, 0x16, 0x1, 142336 }, // 139.0x
    { 0xf8, 0x18, 0x1, 143360 }, // 140.0x
    { 0xf8, 0x1a, 0x1, 144384 }, // 141.0x
    { 0xf8, 0x1c, 0x1, 145408 }, // 142.0x
    { 0xf8, 0x1e, 0x1, 146432 }, // 143.0x
    { 0xf8, 0x20, 0x1, 147456 }, // 144.0x
    { 0xf8, 0x22, 0x1, 148480 }, // 145.0x
    { 0xf8, 0x24, 0x1, 149504 }, // 146.0x
    { 0xf8, 0x26, 0x1, 150528 }, // 147.0x
    { 0xf8, 0x28, 0x1, 151552 }, // 148.0x
    { 0xf8, 0x2a, 0x1, 152576 }, // 149.0x
    { 0xf8, 0x2c, 0x1, 153600 }, // 150.0x
    { 0xf8, 0x2e, 0x1, 154624 }, // 151.0x
    { 0xf8, 0x30, 0x1, 155648 }, // 152.0x
    { 0xf8, 0x32, 0x1, 156672 }, // 153.0x
    { 0xf8, 0x34, 0x1, 157696 }, // 154.0x
    { 0xf8, 0x36, 0x1, 158720 }, // 155.0x
    { 0xf8, 0x38, 0x1, 159744 }, // 156.0x
    { 0xf8, 0x3a, 0x1, 160768 }, // 157.0x
    { 0xf8, 0x3c, 0x1, 161792 }, // 158.0x
    { 0xf8, 0x3e, 0x1, 162816 }, // 159.0x
    { 0xf8, 0x40, 0x1, 163840 }, // 160.0x
    { 0xf8, 0x42, 0x1, 164864 }, // 161.0x
    { 0xf8, 0x44, 0x1, 165888 }, // 162.0x
    { 0xf8, 0x46, 0x1, 166912 }, // 163.0x
    { 0xf8, 0x48, 0x1, 167936 }, // 164.0x
    { 0xf8, 0x4a, 0x1, 168960 }, // 165.0x
    { 0xf8, 0x4c, 0x1, 169984 }, // 166.0x
    { 0xf8, 0x4e, 0x1, 171008 }, // 167.0x
    { 0xf8, 0x50, 0x1, 172032 }, // 168.0x
    { 0xf8, 0x52, 0x1, 173056 }, // 169.0x
    { 0xf8, 0x54, 0x1, 174080 }, // 170.0x
    { 0xf8, 0x56, 0x1, 175104 }, // 171.0x
    { 0xf8, 0x58, 0x1, 176128 }, // 172.0x
    { 0xf8, 0x5a, 0x1, 177152 }, // 173.0x
    { 0xf8, 0x5c, 0x1, 178176 }, // 174.0x
    { 0xf8, 0x5e, 0x1, 179200 }, // 175.0x
    { 0xf8, 0x60, 0x1, 180224 }, // 176.0x
    { 0xf8, 0x62, 0x1, 181248 }, // 177.0x
    { 0xf8, 0x64, 0x1, 182272 }, // 178.0x
    { 0xf8, 0x66, 0x1, 183296 }, // 179.0x
    { 0xf8, 0x68, 0x1, 184320 }, // 180.0x
    { 0xf8, 0x6a, 0x1, 185344 }, // 181.0x
    { 0xf8, 0x6c, 0x1, 186368 }, // 182.0x
    { 0xf8, 0x6e, 0x1, 187392 }, // 183.0x
    { 0xf8, 0x70, 0x1, 188416 }, // 184.0x
    { 0xf8, 0x72, 0x1, 189440 }, // 185.0x
    { 0xf8, 0x74, 0x1, 190464 }, // 186.0x
    { 0xf8, 0x76, 0x1, 191488 }, // 187.0x
    { 0xf8, 0x78, 0x1, 192512 }, // 188.0x
    { 0xf8, 0x7a, 0x1, 193536 }, // 189.0x
    { 0xf8, 0x7c, 0x1, 194560 }, // 190.0x
    { 0xf8, 0x7e, 0x1, 195584 }, // 191.0x
    { 0xf8, 0x80, 0x1, 196608 }, // 192.0x
    { 0xf8, 0x82, 0x1, 197632 }, // 193.0x
    { 0xf8, 0x84, 0x1, 198656 }, // 194.0x
    { 0xf8, 0x86, 0x1, 199680 }, // 195.0x
    { 0xf8, 0x88, 0x1, 200704 }, // 196.0x
    { 0xf8, 0x8a, 0x1, 201728 }, // 197.0x
    { 0xf8, 0x8c, 0x1, 202752 }, // 198.0x
    { 0xf8, 0x8e, 0x1, 203776 }, // 199.0x
    { 0xf8, 0x90, 0x1, 204800 }, // 200.0x
    { 0xf8, 0x92, 0x1, 205824 }, // 201.0x
    { 0xf8, 0x94, 0x1, 206848 }, // 202.0x
    { 0xf8, 0x96, 0x1, 207872 }, // 203.0x
    { 0xf8, 0x98, 0x1, 208896 }, // 204.0x
    { 0xf8, 0x9a, 0x1, 209920 }, // 205.0x
    { 0xf8, 0x9c, 0x1, 210944 }, // 206.0x
    { 0xf8, 0x9e, 0x1, 211968 }, // 207.0x
    { 0xf8, 0xa0, 0x1, 212992 }, // 208.0x
    { 0xf8, 0xa2, 0x1, 214016 }, // 209.0x
    { 0xf8, 0xa4, 0x1, 215040 }, // 210.0x
    { 0xf8, 0xa6, 0x1, 216064 }, // 211.0x
    { 0xf8, 0xa8, 0x1, 217088 }, // 212.0x
    { 0xf8, 0xaa, 0x1, 218112 }, // 213.0x
    { 0xf8, 0xac, 0x1, 219136 }, // 214.0x
    { 0xf8, 0xae, 0x1, 220160 }, // 215.0x
    { 0xf8, 0xb0, 0x1, 221184 }, // 216.0x
    { 0xf8, 0xb2, 0x1, 222208 }, // 217.0x
    { 0xf8, 0xb4, 0x1, 223232 }, // 218.0x
    { 0xf8, 0xb6, 0x1, 224256 }, // 219.0x
    { 0xf8, 0xb8, 0x1, 225280 }, // 220.0x
    { 0xf8, 0xba, 0x1, 226304 }, // 221.0x
    { 0xf8, 0xbc, 0x1, 227328 }, // 222.0x
    { 0xf8, 0xbe, 0x1, 228352 }, // 223.0x
    { 0xf8, 0xc0, 0x1, 229376 }, // 224.0x
    { 0xf8, 0xc2, 0x1, 230400 }, // 225.0x
    { 0xf8, 0xc4, 0x1, 231424 }, // 226.0x
    { 0xf8, 0xc6, 0x1, 232448 }, // 227.0x
    { 0xf8, 0xc8, 0x1, 233472 }, // 228.0x
    { 0xf8, 0xca, 0x1, 234496 }, // 229.0x
    { 0xf8, 0xcc, 0x1, 235520 }, // 230.0x
    { 0xf8, 0xce, 0x1, 236544 }, // 231.0x
    { 0xf8, 0xd0, 0x1, 237568 }, // 232.0x
    { 0xf8, 0xd2, 0x1, 238592 }, // 233.0x
    { 0xf8, 0xd4, 0x1, 239616 }, // 234.0x
    { 0xf8, 0xd6, 0x1, 240640 }, // 235.0x
    { 0xf8, 0xd8, 0x1, 241664 }, // 236.0x
    { 0xf8, 0xda, 0x1, 242688 }, // 237.0x
    { 0xf8, 0xdc, 0x1, 243712 }, // 238.0x
    { 0xf8, 0xde, 0x1, 244736 }, // 239.0x
    { 0xf8, 0xe0, 0x1, 245760 }, // 240.0x
    { 0xf8, 0xe2, 0x1, 246784 }, // 241.0x
    { 0xf8, 0xe4, 0x1, 247808 }, // 242.0x
    { 0xf8, 0xe6, 0x1, 248832 }, // 243.0x
    { 0xf8, 0xe8, 0x1, 249856 }, // 244.0x
    { 0xf8, 0xea, 0x1, 250880 }, // 245.0x
    { 0xf8, 0xec, 0x1, 251904 }, // 246.0x
    { 0xf8, 0xee, 0x1, 252928 }, // 247.0x
    { 0xf8, 0xf0, 0x1, 253952 }, // 248.0x
    { 0xf8, 0xf2, 0x1, 254976 }, // 249.0x
    { 0xf8, 0xf4, 0x1, 256000 }, // 250.0x
    { 0xf8, 0xf6, 0x1, 257024 }, // 251.0x
    { 0xf8, 0xf8, 0x1, 258048 }, // 252.0x
    { 0xf8, 0xfa, 0x1, 259072 }, // 253.0x
    { 0xf8, 0xfc, 0x1, 260096 }, // 254.0x
    { 0xf8, 0xfe, 0x1, 261120 }, // 255.0x
    { 0xf8, 0x0, 0x2, 262144 }, // 256.0x
};

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
        CV2005_1080P_CalibParam_dynamic = NULL;
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

            pAeSnsDft->maxAgain  = 256 * 1024;
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
    const cv2005_gain_lut_t *selected = &s_cv2005_gain_lut[0];
    const vsi_u32_t target_gain = *pGain;

    (void)pConvReg;

    for (vsi_u32_t i = 1; i < ARRAY_SIZE(s_cv2005_gain_lut); i++) {
        if (target_gain < s_cv2005_gain_lut[i].gain) {
            break;
        }
        selected = &s_cv2005_gain_lut[i];
    }

    *pAgainReg = selected->again_reg;
    *pDGainReg = ((vsi_u16_t)selected->dgain_high << 8) | selected->dgain_low;
    *pGain = selected->gain;

    return;
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
    const cv2005_gain_lut_t *selected = &s_cv2005_gain_lut[0];
    const vsi_u32_t target_gain = *pGain;
    vsi_u32_t selected_index = 0;
    static vsi_u32_t print_count = 0;

    (void)pConvReg;

    for (vsi_u32_t i = 1; i < ARRAY_SIZE(s_cv2005_gain_lut); i++) {
        if (target_gain < s_cv2005_gain_lut[i].gain) {
            break;
        }
        selected = &s_cv2005_gain_lut[i];
        selected_index = i;
    }

    if ((++print_count % 30) == 0) {
        LOGI("gain_lut[%u]: gain=%u, again=0x%02x, dgain=0x%02x%02x\n",
            selected_index, selected->gain, selected->again_reg,
            selected->dgain_high, selected->dgain_low);
    }

    *pAgainReg = selected->again_reg;
    *pDGainReg = ((vsi_u16_t)selected->dgain_high << 8) | selected->dgain_low;
    *pGain = selected->gain;

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
            *pAgain = gain;
            *pDgain = 1024;
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

void *cv2005_get_sensor_cfg(bk_camera_sensor_ctlr_t *controller)
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
    os_memset(csi_sensor, 0, sizeof(bk_camera_csi_sensor_t));
    config->bus->write_address = CV2005_WRITE_ADDRESS;
    os_memcpy(&csi_sensor->config, config, sizeof(bk_camera_sensor_config_t));

    csi_sensor->ops.init = cv2005_init;
    csi_sensor->ops.set_format = cv2005_set_format;
    csi_sensor->ops.reg_ctrl = cv2005_ctrl;
    csi_sensor->ops.get_sensor_object = cv2005_get_sensor_object;
    csi_sensor->ops.get_sensor_cfg = cv2005_get_sensor_cfg;
    csi_sensor->ops.query_support_formats = cv2005_query_support_formats;

    csi_sensor->isp_pub_attr = &cv2005_mipi_linear_attr;
    csi_sensor->sensor_config = &csi_sensor_cv2005;
    *handle = (bk_camera_sensor_handle_t)&csi_sensor->ops;

    return 0;
}


BK_CAMERA_SENSOR_DETECT_SECTION(cv2005_detect, CSI_CAMERA_PORT);
