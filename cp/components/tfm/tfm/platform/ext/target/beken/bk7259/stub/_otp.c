// Copyright 2022-2024 Beken
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

// Committed fallback OTP map, kept in sync with the authoritative board
// otp1.csv/otp2.csv. The runtime build stages the freshly generated _otp.h/_otp.c
// (armino/partitions/_build) ahead of this copy; regenerate this file from the
// board csv if the layout changes.

#pragma once


 
#include "_otp.h"

#include <stddef.h>

const otp_item_t otp_map_1[25] = {
    {OTP_M52SUB_MEMCHECK,                     64,    0x0,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_M55SUB_MEMCHECK,                    220,    0x40,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_PRI_CONFIG,                           4,    0x11c,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_PRIVATE_KEY_1,                       32,    0x120,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_PRIVATE_KEY_2,                       32,    0x140,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_FLASH_AES_K2,                        32,    0x160,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_FLASH_AES_K1,                        32,    0x180,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_HW_RESERVED,                         96,    0x1a0,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MODEL_ID,                             4,    0x200,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MODEL_KEY,                           16,    0x204,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_TE200_DEVICE_ID,                      4,    0x214,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_DEVICE_ROOT_KEY,                     16,    0x218,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_BL1_BOOT_PUBLIC_KEY_HASH,            32,    0x228,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_BL2_BOOT_PUBLIC_KEY_HASH,            32,    0x248,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_LCS,                                  4,    0x268,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_2_8,                        16,    0x26c,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_LOCK_CONTROL,                         4,    0x27c,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_2_10,                        8,    0x280,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_BL2_SECURITY_COUNTER,                64,    0x288,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_SECURE_DEBUG_OFFSET,                  4,    0x2c8,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_SECURE_DEBUG_PK_HASH,                32,    0x2cc,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_2_14,                       20,    0x2ec,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_APP_SECURITY_COUNTER,                64,    0x300,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_HUK,                                 32,    0x340,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_3_3,                       160,    0x360,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
};

uint32_t otp_map_1_row(void)
{
    return sizeof(otp_map_1) / sizeof(otp_map_1[0]);
}

uint32_t otp_map_1_col(void)
{
    return sizeof(otp_map_1) / sizeof(otp_map_1[0]);
}

const otp_item_t otp_map_2[40] = {
    {OTP_PHY_PWR1,                           48,    0x0,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_PHY_PWR2,                           48,    0x30,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RFCALI1,                           256,    0x60,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RFCALI2,                           256,    0x160,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RFCALI3,                           256,    0x260,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RFCALI4,                           256,    0x360,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MAC_ADDRESS_1,                       8,    0x460,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MAC_ADDRESS_2,                       8,    0x468,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MAC_ADDRESS_3,                       8,    0x470,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MAC_ADDRESS_4,                       8,    0x478,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_4_1,                      112,    0x480,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_PACKAGE_TYPE,                        4,    0x4f0,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RSSI,                               60,    0x4f4,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_GADC_CALIBRATION_BACKUP,            72,    0x530,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_GADC_TEMPERATURE_BACKUP,             8,    0x578,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_VDDDIG_BANDGAP_BACKUP,               4,    0x580,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_DIA_BACKUP,                          4,    0x584,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_FACTORY_ID,                          4,    0x588,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_PRODUCT_ID,                         16,    0x58c,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_GADC_CALIBRATION,                   72,    0x59c,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_GADC_TEMPERATURE,                    8,    0x5e4,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_VDDDIG_BANDGAP,                      4,    0x5ec,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_DIA,                                 4,    0x5f0,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_MEMORY_CHECK_VDDDIG,                 4,    0x5f4,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_DEVICE_ID,                           8,    0x5f8,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RANDOM_KEY1,                        32,    0x600,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RANDOM_KEY2,                        32,    0x620,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_5_3,                      448,    0x640,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK2,                                16,    0x800,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK3,                                16,    0x810,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK1,                                32,    0x820,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK11,                               32,    0x840,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK12,                               32,    0x860,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK13,                               32,    0x880,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK14,                               32,    0x8a0,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK15,                               32,    0x8c0,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_EK1X,                               32,    0x8e0,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_CK1,                                 8,    0x900,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_CK2,                               256,    0x908,    OTP_READ_WRITE,    OTP_NON_SECURITY,    OTP_NO_NEED_CRC},
    {OTP_RESERVED_5_15,                     504,    0xa08,    OTP_READ_WRITE,    OTP_SECURITY,    OTP_NO_NEED_CRC},
};

uint32_t otp_map_2_row(void)
{
    return sizeof(otp_map_2) / sizeof(otp_map_2[0]);
}

uint32_t otp_map_2_col(void)
{
    return sizeof(otp_map_2) / sizeof(otp_map_2[0]);
}

const otp_item_t *otp_map = NULL;
