// Copyright 2023-2028 Beken
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

/* Secure-world OTP access for TF-M. These run in the SPE and use the generated
 * OTP map (otp_map_1/otp_map_2 from otp1.csv/otp2.csv). They let other secure
 * code (and the OTP NSC gateways) read/write OTP1/OTP2 - including regions the
 * NS world cannot reach. Activate the block with OTP_ACTIVE() before use and
 * otp_sleep() after.
 *
 * map_id : 1 = OTP1 (APB bank), 2 = OTP2 (AHB bank)
 * item   : otp1_id_t / otp2_id_t index (from _otp.h) */

#pragma once

#include <common/bk_include.h>
#include "_otp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTP_MAP_APB   1u   /**< OTP1 (APB) bank */
#define OTP_MAP_AHB   2u   /**< OTP2 (AHB) bank */

int  otp_active(void);
void otp_sleep(void);

#define OTP_ACTIVE() do { \
		if (otp_active() != 0) { \
			otp_sleep(); \
			return BK_ERR_OTP_INIT_FAIL; \
		} \
	} while (0)

bk_err_t        otp_read(uint8_t map_id, uint32_t item, uint8_t *buf, uint32_t size);
bk_err_t        otp_update(uint8_t map_id, uint32_t item, const uint8_t *buf, uint32_t size);
otp_privilege_t otp_read_permission(uint8_t map_id, uint32_t item);
bk_err_t        otp_write_permission(uint8_t map_id, uint32_t item, otp_privilege_t permission);
bk_err_t        otp_write_mask(uint8_t map_id, uint32_t item, otp_privilege_t permission);

/* Define the whole OTP1 range (0x0-0x400) as a secure range and enable the
 * secure protection. Secure-world only; the protection is permanent. */
bk_err_t        otp_secure_range_enable(void);

#ifdef __cplusplus
}
#endif
