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

#include <components/log.h>

#define CHIP_SUPPORT_TAG "chip_support"
#define CHIP_SUPPORT_LOGI(...) BK_LOGI(CHIP_SUPPORT_TAG, ##__VA_ARGS__)
#define CHIP_SUPPORT_LOGW(...) BK_LOGW(CHIP_SUPPORT_TAG, ##__VA_ARGS__)
#define CHIP_SUPPORT_LOGE(...) BK_LOGE(CHIP_SUPPORT_TAG, ##__VA_ARGS__)
#define CHIP_SUPPORT_LOGD(...) BK_LOGD(CHIP_SUPPORT_TAG, ##__VA_ARGS__)
#define CHIP_SUPPORT_LOGV(...) BK_LOGV(CHIP_SUPPORT_TAG, ##__VA_ARGS__)

/** @brief   This enumeration defines hardware_chip_version.  */
typedef enum
{
	CHIP_VERSION_A = 0,
	CHIP_VERSION_B,
	CHIP_VERSION_C,
	CHIP_VERSION_DEFAULT
}hardware_chip_version_e;

/** @brief   This enumeration defines hardware_chip_id_value.  */
typedef enum
{
	CHIP_ID_A_VALUE = 0x22041020,
	CHIP_ID_B_VALUE ,
	CHIP_ID_C_VALUE = 0x22091022,
	CHIP_ID_DEFAULT_VALUE
}hardware_chip_id_value_e;

/** @brief   This struct defines chip id and chip version id .  */
typedef struct {
       uint32_t version_id;
       uint32_t chip_id;
} soc_info_t;

typedef enum {
	BK_PACKAGE_TYPE_UNKNOWN,                    /* OTP-ID is unassigned or invalid. */
	BK_PACKAGE_TYPE_A_OLD_128A_S_MIC,           /* OTP-ID A0 5F 00 00: Die A, old 128A single-mic package. */
	BK_PACKAGE_TYPE_A_NEW_128A_S_MIC,           /* OTP-ID A1 5E 00 00: Die A, new 128A single-mic package. */
	BK_PACKAGE_TYPE_B_OLD_128A_S_MIC,           /* OTP-ID B0 4F 00 00: Die B(C), old 128A single-mic package. */
	BK_PACKAGE_TYPE_B_NEW_128A_S_OR_128B_D_MIC, /* OTP-ID B1 4E 00 00: Die B(C), shared by two new packages: 128A single-mic and 128B dual-mic. */
} bk_package_type_t;

/**
 * @brief get soc info(chip id and version id)
 *
 *get soc info(chip id and version id)
 *
 * @attention
 * - This API is used to get soc info(chip id and version id)
 *
 * @param
 * -soc_info[out]save the chip id and version id
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 *
 */
bk_err_t bk_soc_info_get(soc_info_t* soc_info);

/**
 * @brief Get the package type from OTP2.
 *
 * @param package_type[out] Package type decoded from the complete 4-byte OTP-ID.
 *
 * @return
 * - BK_OK: read succeeded; unmatched OTP-ID returns BK_PACKAGE_TYPE_UNKNOWN
 * - BK_ERR_NULL_PARAM: package_type is NULL
 * - BK_ERR_NOT_SUPPORT: OTP v1 is disabled
 * - others: OTP read error
 */
bk_err_t bk_get_package_type(bk_package_type_t *package_type);

bool bk_is_chip_supported(void);

/** 
* @brief  Get chip id version.
* 
* @return current chip_version 
* 	 0:  means chip_version_A.
* 	 1:  means chip_version_B.
* 	 2:  means chip_version_C.
* 	 3:  means default version.
*/
hardware_chip_version_e bk_get_hardware_chip_id_version();