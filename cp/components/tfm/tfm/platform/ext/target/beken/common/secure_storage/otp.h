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

#include <components/log.h>
#include "cmsis_compiler.h"
#include <stddef.h>

#pragma once

#define OTP_TAG "otp"

#define OTP_LOGI(...) BK_LOGI(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGW(...) BK_LOGW(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGE(...) BK_LOGE(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOGD(...) BK_LOGD(OTP_TAG, ##__VA_ARGS__)
#define OTP_LOG_RAW BK_LOG_RAW

#define PSA_ROT_PROV_DATA_MAGIC		   0xBEEFFEED

__PACKED_STRUCT otp_ps_data_t {
	uint32_t magic;
	uint8_t iak[32];
	uint32_t iak_len;
	uint32_t iak_type;
	uint8_t iak_id[32];

	uint8_t boot_seed[32]; //From TRNG
	uint8_t implementation_id[32]; //Beken chip ID
	uint8_t hw_version[32]; //Beken hardware version
	uint8_t verification_service_url[32];
	uint8_t profile_definition[32];

	uint8_t entropy_seed[64];
	uint8_t lcs[4];
};

#define OTP_S_BL2_ROTPK_SIZE          32

#define OTP_S_NV_COUNTER_BL2_OFFSET   0
#define OTP_S_NV_COUNTER_BL2_SIZE     32
#define OTP_S_HUK_OFFSET OTP_S_NV_COUNTER_BL2_SIZE
#define OTP_S_HUK_SIZE                32

#define OTP_DATA_OFFSET (OTP_S_HUK_SIZE + OTP_S_NV_COUNTER_BL2_SIZE)
#define OTP_PS_DATA_OFFSETOF(a, b)	(offsetof(a, b) + OTP_DATA_OFFSET - 4)

enum tfm_plat_err_t otp_ps_read(enum tfm_otp_element_id_t id, size_t out_len, uint8_t *out);
enum tfm_plat_err_t otp_ps_write(enum tfm_otp_element_id_t id, size_t in_len, const uint8_t *in);
int arm_ce_seed_read( unsigned char *buf, size_t buf_len );
enum tfm_plat_err_t otp_huk_provisioning(uint8_t *out, size_t out_len);
