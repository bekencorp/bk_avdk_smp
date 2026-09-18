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

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_MPC_DRIVER_NOT_INIT             (BK_ERR_MPC_BASE - 1) /**< MPC driver not init */
#define BK_ERR_MPC_BLOCK_INDEX_OUT_OF_RANGE    (BK_ERR_MPC_BASE - 2) /**< MPC block index is out of range */
#define BK_ERR_MPC_INVALID_LUT_PARAM           (BK_ERR_MPC_BASE - 3) /**< MPC invalid lut parameter */
#define BK_ERR_MPC_INVALID_DEV                 (BK_ERR_MPC_BASE - 4) /**< MPC invalid device ID */

/* CP-domain MPC instances only (managed by the CP MPC driver). OTP1 has no MPC.
 * AP-domain MPCs (PSRAM/QSPI/SMEM3-6/NPU) belong to the AP subsystem and are not
 * managed by the CP driver. */
typedef enum {
	MPC_DEV_FLASH = 0, /**< MPC device FLASH */
	MPC_DEV_SMEM0,     /**< MPC device SMEM0 */
	MPC_DEV_SMEM1,     /**< MPC device SMEM1 */
	MPC_DEV_SMEM2,     /**< MPC device SMEM2 */
	MPC_DEV_OTP2,      /**< MPC device OTP2 (AHB bank) */
	MPC_DEV_MAX,       /**< MPC max device id */
} mpc_dev_t;

typedef enum {
	MPC_BLOCK_SECURE = 0, /**< MPC attribute secure */
	MPC_BLOCK_NON_SECURE, /**< MPC attribute non secure */
} mpc_block_secure_type_t;

#ifdef __cplusplus
}
#endif

