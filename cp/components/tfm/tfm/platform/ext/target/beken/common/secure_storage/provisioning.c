/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_plat_otp.h"
#include <string.h>
#include <stddef.h>
#include "otp.h"

int mbedtls_set_otp_info_ex(int type, const void *input, size_t ilen, size_t offset);

enum tfm_plat_err_t otp_huk_provisioning(uint8_t *out, size_t out_len)
{
	uint8_t empty_huk[64] = {0};//huk length <= 64
	int ret;

	if (memcmp(out, empty_huk, out_len) != 0) {
		return TFM_PLAT_ERR_SUCCESS;
	}

	OTP_LOGI("HUK is empty, provisioning it\r\n");
	ret = arm_ce_seed_read(out, out_len);
	if (ret != 0) {
		OTP_LOGE("HUK TRNG read failed, ret=%d\r\n", ret);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	ret = mbedtls_set_otp_info_ex(OTP_REGION_USR_NON_SEC, out, out_len, OTP_HUK_OFF);
	if (ret != 0) {
		OTP_LOGE("set huk to otp failed, ret=%d\r\n", ret);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	OTP_LOGI("HUK provisioning is ok\r\n");

	return TFM_PLAT_ERR_SUCCESS;
}
