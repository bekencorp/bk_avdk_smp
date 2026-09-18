/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* Platform OTP backend. Only the elements that have a real slot in the OTP map
 * are kept: BL2 ROTPK, HUK and the BL2/application security counter. All other
 * ids (attestation data, lifecycle, etc.) are not provisioned and return
 * TFM_PLAT_ERR_UNSUPPORTED. */

#include "tfm_plat_otp.h"

#include "flash_layout.h"
#include <string.h>
#include <stddef.h>
#include "otp.h"

int mbedtls_get_otp_info_ex(int type, void *output, size_t size, size_t offset);
int mbedtls_set_otp_info_ex(int type, const void *input, size_t ilen, size_t offset);

enum tfm_plat_err_t tfm_plat_otp_init(void)
{
	OTP_LOGD("init\r\n");
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_read(enum tfm_otp_element_id_t id,
                                      size_t out_len, uint8_t *out)
{
	int ret;

	switch (id) {
	case PLAT_OTP_ID_BL2_ROTPK_0:
		ret = mbedtls_get_otp_info_ex(OTP_REGION_BL2_PK_HASH, out, out_len, 0);
		break;
	case PLAT_OTP_ID_HUK:
#if CONFIG_ENABLE_MCUBOOT_BL2
		OTP_LOGE("unsupported id=%d\r\n", id);
		return TFM_PLAT_ERR_UNSUPPORTED;
#else
		ret = mbedtls_get_otp_info_ex(OTP_REGION_USR_NON_SEC, out, out_len, OTP_HUK_OFF);
		if (ret != 0) {
			OTP_LOGE("failed to read huk from otp\r\n");
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}
		ret = otp_huk_provisioning(out, out_len);
		if (ret != 0) {
			OTP_LOGE("otp huk provisioning failed\r\n");
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}
		break;
#endif
	case PLAT_OTP_ID_NV_COUNTER_BL2_0:
		ret = mbedtls_get_otp_info_ex(OTP_REGION_USR_NON_SEC, out, out_len, OTP_APP_SEC_CNT_OFF);
		break;
#if defined(TFM_PARTITION_INITIAL_ATTESTATION)
	/* TODO: when Initial Attestation is enabled, add IAK/BOOT_SEED/IMPLEMENTATION_ID/
	 * HW_VERSION/PROFILE_DEFINITION/... reads here with a real storage backend
	 * (OTP2 customer area or flash/PS). Do NOT reuse the old otp_ps_data_t offsets
	 * (they overflowed the OTP1 bank). */
#endif
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (ret != 0) {
		OTP_LOGE("mbedtls read otp id=%d failed, ret=%d\r\n", id, ret);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	OTP_LOGD("read ok, id=%d, out_len=%d\r\n", id, out_len);
	return TFM_PLAT_ERR_SUCCESS;
}

#if defined(OTP_WRITEABLE)
enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id,
                                       size_t in_len, const uint8_t *in)
{
	int ret;

	switch (id) {
	case PLAT_OTP_ID_NV_COUNTER_BL2_0:
		ret = mbedtls_set_otp_info_ex(OTP_REGION_USR_NON_SEC, in, in_len, OTP_APP_SEC_CNT_OFF);
		break;
#if defined(TFM_PARTITION_INITIAL_ATTESTATION)
	/* TODO: when Initial Attestation is enabled, add IAK/BOOT_SEED/IMPLEMENTATION_ID/
	 * HW_VERSION/PROFILE_DEFINITION/... writes here with a real storage backend
	 * (OTP2 customer area or flash/PS). Do NOT reuse the old otp_ps_data_t offsets
	 * (they overflowed the OTP1 bank). */
#endif
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (ret != 0) {
		OTP_LOGE("mbedtls write otp id=%d failed, ret=%d\r\n", id, ret);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	OTP_LOGD("write ok, id=%d, in_len=%d\r\n", id, in_len);
	return TFM_PLAT_ERR_SUCCESS;
}
#else
enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id,
                                       size_t in_len, const uint8_t *in)
{
	(void)id;
	(void)in_len;
	(void)in;
	return TFM_PLAT_ERR_UNSUPPORTED;
}
#endif

enum tfm_plat_err_t tfm_plat_otp_get_size(enum tfm_otp_element_id_t id,
                                          size_t *size)
{
	switch (id) {
	case PLAT_OTP_ID_HUK:
		*size = OTP_S_HUK_SIZE;
		break;
	case PLAT_OTP_ID_BL2_ROTPK_0:
		*size = OTP_S_BL2_ROTPK_SIZE;
		break;
	case PLAT_OTP_ID_NV_COUNTER_BL2_0:
		*size = OTP_S_NV_COUNTER_BL2_SIZE;
		break;
#if defined(TFM_PARTITION_INITIAL_ATTESTATION)
	/* TODO: when Initial Attestation is enabled, add IAK/BOOT_SEED/IMPLEMENTATION_ID/
	 * HW_VERSION/PROFILE_DEFINITION/... sizes here to match the storage backend
	 * chosen above (OTP2 customer area or flash/PS). */
#endif
	default:
		OTP_LOGD("failed get size, unsupported id=%d\r\n", id);
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_secure_provisioning_start(void)
{
    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_secure_provisioning_finish(void)
{
    return TFM_PLAT_ERR_SUCCESS;
}
