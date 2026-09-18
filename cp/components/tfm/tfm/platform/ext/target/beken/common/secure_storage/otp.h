// Copyright     2023-2028 Beken
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

#include "bk_tfm_log.h"
#include "cmsis_compiler.h"
#include <stddef.h>

#pragma once

#define OTP_TAG "otp"

#define OTP_LOGI(...) BK_LOGI(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGW(...) BK_LOGW(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGE(...) BK_LOGE(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGD(...) BK_LOGD(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOG_RAW BK_LOG_RAW

/* OTP region id passed to the crypto-engine OTP accessor. */
#define OTP_REGION_BL2_PK_HASH   5u   /* BL2 boot public key hash slot (0x248) */
#define OTP_REGION_USR_NON_SEC   8u   /* user data region, region base 0x280 */

/* Byte offsets inside OTP_REGION_USR_NON_SEC (offset = item offset - 0x280). */
#define OTP_APP_SEC_CNT_OFF      0x80u  /* application security counter (0x300) */
#define OTP_HUK_OFF              0xC0u  /* HUK (0x340) */

/* Element sizes in bytes. */
#define OTP_S_BL2_ROTPK_SIZE      32
#define OTP_S_HUK_SIZE            32
#define OTP_S_NV_COUNTER_BL2_SIZE 64

enum tfm_plat_err_t otp_ps_read(enum tfm_otp_element_id_t id, size_t out_len, uint8_t *out);
enum tfm_plat_err_t otp_ps_write(enum tfm_otp_element_id_t id, size_t in_len, const uint8_t *in);
int arm_ce_seed_read( unsigned char *buf, size_t buf_len );
enum tfm_plat_err_t otp_huk_provisioning(uint8_t *out, size_t out_len);
