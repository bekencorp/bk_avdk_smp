/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* NOTE: For the security of the protected storage system, the bootloader
 * rollback protection, and the protection of cryptographic material  it is
 * CRITICAL to use a internal (in-die) persistent memory for the implementation
 * of the PS_NV_COUNTERS flash area (see flash_otp_nv_layout.c).
 */
#include "tfm_plat_otp.h"

#include "flash_layout.h"
#include <string.h>
#include <stddef.h>
#include "otp.h"
#include "psa/protected_storage.h"

enum tfm_plat_err_t otp_ps_read(enum tfm_otp_element_id_t id, size_t out_len, uint8_t *out)
{
	const psa_storage_uid_t uid = BIT(31) | id;
	size_t read_data_len = 0;
	psa_status_t status;

	OTP_LOGD("read ps id:%x uid:%x len:%d\r\n", id, uid, out_len);
	status = psa_ps_get(uid, 0, out_len, out, &read_data_len);
	if (status != PSA_SUCCESS) {
		OTP_LOGE("read ps failed, id=%x len=%x status=%x\r\n", id, out_len, status);
		return status;
	}

	OTP_LOGD("read ps ok, id:%d actual_len:%d\r\n", id, read_data_len);
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t otp_ps_write(enum tfm_otp_element_id_t id, size_t in_len, const uint8_t *in)
{
	psa_storage_create_flags_t flags;
	const psa_storage_uid_t uid = BIT(31) | id;
	psa_status_t status;

	if (id == PLAT_OTP_ID_LCS) {
		flags = PSA_STORAGE_FLAG_NONE;
	} else {
		flags = PSA_STORAGE_FLAG_WRITE_ONCE;
	}

	status = psa_ps_set(uid, in_len, in, flags);
	if (status != PSA_SUCCESS) {
		OTP_LOGE("write ps write failed, status=%x\r\n", status);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	OTP_LOGI("write ps ok, id:%d len:%d\r\n", id, in_len);

	return TFM_PLAT_ERR_SUCCESS;
}

