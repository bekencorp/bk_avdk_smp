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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_typedef.h>
#include <common/sys_config.h>

#define DD_HANDLE_MAGIC_WORD         (0xA5A50000)
#define DD_HANDLE_MAGIC_MASK         (0xFFFF0000)
#define DD_HANDLE_ID_MASK            (0x0000FFFF)

typedef UINT32 DD_HANDLE;
typedef enum _dd_device_type_
{
    DD_DEV_TYPE_NONE,
    DD_DEV_TYPE_START = DD_HANDLE_MAGIC_WORD-1,
    DD_DEV_TYPE_TIMER,
#if (CONFIG_BLUETOOTH)
    DD_DEV_TYPE_BLE,
#endif
#if CONFIG_FFT
    DD_DEV_TYPE_FFT,
#endif

#if CONFIG_GENERAL_DMA
    DD_DEV_TYPE_GDMA,
#endif
    DD_DEV_TYPE_GPIO,
#if CONFIG_I2S
    DD_DEV_TYPE_I2S,
#endif
    DD_DEV_TYPE_ICU,
    DD_DEV_TYPE_IRDA,
#if CONFIG_MAC_PHY_BYPASS
    DD_DEV_TYPE_MPB,
#endif

#if !CFG_CONFIG_FULLY_HOSTED
    DD_DEV_TYPE_SPI,
#endif
#if CONFIG_QSPI
    DD_DEV_TYPE_QSPI,
#endif
    DD_DEV_TYPE_SCTRL,
    DD_DEV_TYPE_WDT,
    DD_DEV_TYPE_TRNG,

    DD_DEV_TYPE_UART0,

#if CONFIG_UART1
    DD_DEV_TYPE_UART1,
#endif
#if CONFIG_UART2
    DD_DEV_TYPE_UART2,
#endif
#if CONFIG_HSLAVE_SPI
    DD_DEV_TYPE_SPIDMA,
#endif
#if CONFIG_DVP_CAMERA
    DD_DEV_TYPE_EJPEG,
#endif
    DD_DEV_TYPE_I2C1,
    DD_DEV_TYPE_I2C2,
    DD_DEV_TYPE_AUD_DAC,
    DD_DEV_TYPE_USB,
    DD_DEV_TYPE_USB_PLUG,
#if CONFIG_SDCARD
    DD_DEV_TYPE_SDCARD,
#endif
    DD_DEV_TYPE_SARADC,
    DD_DEV_TYPE_RF,
    DD_DEV_TYPE_END
} dd_device_type;

#ifdef __cplusplus
}
#endif
