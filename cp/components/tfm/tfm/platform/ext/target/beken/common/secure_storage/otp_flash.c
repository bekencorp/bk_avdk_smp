/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* NOTE: For the security of the protected storage system, the bootloader
 * rollback protection, and the protection of cryptographic material  it is
 * CRITICAL to use a internal (in-die) persistent memory for the implementation
 * of the OTP_NV_COUNTERS flash area (see flash_otp_nv_layout.c).
 */
#include "tfm_plat_otp.h"

#include "flash_layout.h"
#include <string.h>
#include <stddef.h>
#include "otp.h"
// #include "../../../../../../../trustengine/dubhe_alt/inc/mbedtls/otp.h"

#define OTP_ROTPK_ID             5
#define OTP_SECURITY_COUNTER_ID  0

int mbedtls_get_otp_info_ex(int type, void *output, size_t size, size_t offset);
int mbedtls_set_otp_info_ex(int type, const void *input, size_t ilen, size_t offset);

enum tfm_plat_err_t tfm_plat_otp_init(void)
{
	OTP_LOGD("init\r\n");
	return TFM_PLAT_ERR_SUCCESS;
}

__attribute__((unused)) static enum tfm_plat_err_t write_to_output(enum tfm_otp_element_id_t id,
                                        uint32_t offset, size_t out_len,
                                        uint8_t *out)
{
	OTP_LOGD("write, id=%d, offset=%d out_len=%d\r\n", id, offset, out_len);
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_read(enum tfm_otp_element_id_t id,
                                      size_t out_len, uint8_t *out)
{
	int ret = 0;

	switch (id) {
	case PLAT_OTP_ID_BL2_ROTPK_0:
		ret = mbedtls_get_otp_info_ex(5, out, out_len, 0);
		break;
	case PLAT_OTP_ID_NV_COUNTER_BL2_0:
		ret =  mbedtls_get_otp_info_ex(9, out, out_len, OTP_S_NV_COUNTER_BL2_OFFSET);
		break;
	case PLAT_OTP_ID_HUK:
#if CONFIG_ENABLE_MCUBOOT_BL2
		OTP_LOGE("unsupported id=%d\r\n", id);
		return TFM_PLAT_ERR_UNSUPPORTED;
#else
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_S_HUK_OFFSET);
		if (ret != 0) {
			OTP_LOGE("failed to read huk from otp\r\n");
			return -1;	
		}
		ret = otp_huk_provisioning(out, out_len);
		if (ret != 0) {
			OTP_LOGE("otp huk provisioning failed\r\n");
			return -1;
		}
		break;
#endif
	case PLAT_OTP_ID_IAK:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak));
		break;
	case PLAT_OTP_ID_IAK_LEN:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak_len));
		break;
	case PLAT_OTP_ID_IAK_TYPE:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak_type));
		break;
	case PLAT_OTP_ID_IAK_ID:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak_id));
		break;
	case PLAT_OTP_ID_BOOT_SEED:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, boot_seed));
		break;
	case PLAT_OTP_ID_LCS:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, lcs));
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, implementation_id));
		break;
	case PLAT_OTP_ID_CERT_REF:
		ret = TFM_PLAT_ERR_SUCCESS;
		break;
	case PLAT_OTP_ID_HW_VERSION:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, hw_version));
		break;
	case PLAT_OTP_ID_VERIFICATION_SERVICE_URL:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, verification_service_url));
		break;
	case PLAT_OTP_ID_PROFILE_DEFINITION:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, profile_definition));
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		ret = mbedtls_get_otp_info_ex(9, out, out_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, entropy_seed));
		break;
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (ret != 0) {
		OTP_LOGE("mbedtls read otp id=%d failed, ret=%d\r\n", id, ret);
		return -1;
	}

	OTP_LOGD("read ok, id=%d, out_len=%d+\r\n", id, out_len);

	return TFM_PLAT_ERR_SUCCESS;

}

#if defined(OTP_WRITEABLE)
__attribute__((unused)) static enum tfm_plat_err_t read_from_input(enum tfm_otp_element_id_t id,
                                      uint32_t offset, size_t in_len,
                                      const uint8_t *in)
{
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id,
                                       size_t in_len, const uint8_t *in)
{
	int ret = 0;

	OTP_LOGD("write, id=%d, in_len=%d+\r\n", id, in_len);
	switch (id) {
	case PLAT_OTP_ID_NV_COUNTER_BL2_0:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_S_NV_COUNTER_BL2_OFFSET);
		break;
	case PLAT_OTP_ID_HUK:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_S_HUK_OFFSET);
		break;
	case PLAT_OTP_ID_IAK:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak));
		break;
	case PLAT_OTP_ID_IAK_LEN:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak_len));
		break;
	case PLAT_OTP_ID_IAK_TYPE:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak_type));
		break;
	case PLAT_OTP_ID_IAK_ID:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, iak_id));
		break;
	case PLAT_OTP_ID_BOOT_SEED:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, boot_seed));
		break;
	case PLAT_OTP_ID_LCS:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, lcs));
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, implementation_id));
		break;
	case PLAT_OTP_ID_CERT_REF:
		ret = TFM_PLAT_ERR_SUCCESS;
		break;
	case PLAT_OTP_ID_HW_VERSION:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, hw_version));
		break;
	case PLAT_OTP_ID_VERIFICATION_SERVICE_URL:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, verification_service_url));
		break;
	case PLAT_OTP_ID_PROFILE_DEFINITION:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, profile_definition));
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		ret = mbedtls_set_otp_info_ex(9, in, in_len, OTP_PS_DATA_OFFSETOF(struct otp_ps_data_t, entropy_seed));
		break;

	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (ret != 0) {
		OTP_LOGE("mbedtls write otp failed, ret=%d\r\n", ret);
		return -1;
	}

	OTP_LOGD("write, id=%d, in_len=%d-\r\n", id, in_len);
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
		case PLAT_OTP_ID_HUK: //PUF OTP
			*size = OTP_S_HUK_SIZE;
			break;
		case PLAT_OTP_ID_BL2_ROTPK_0: //PUF OTP
			*size = OTP_S_BL2_ROTPK_SIZE;
			break;
		case PLAT_OTP_ID_NV_COUNTER_BL2_0: //PUF OTP
			*size = OTP_S_NV_COUNTER_BL2_SIZE;
			break;

		case PLAT_OTP_ID_IAK:
			*size = sizeof(((struct otp_ps_data_t*)0)->iak);
			break;
		case PLAT_OTP_ID_IAK_LEN:
			*size = sizeof(((struct otp_ps_data_t*)0)->iak_len);
			break;
		case PLAT_OTP_ID_IAK_TYPE:
			*size = sizeof(((struct otp_ps_data_t*)0)->iak_type);
			break;
		case PLAT_OTP_ID_IAK_ID:
			*size = sizeof(((struct otp_ps_data_t*)0)->iak_id);
			break;

		case PLAT_OTP_ID_BOOT_SEED:
			*size = sizeof(((struct otp_ps_data_t*)0)->boot_seed);
			break;

		case PLAT_OTP_ID_IMPLEMENTATION_ID:
			*size = sizeof(((struct otp_ps_data_t*)0)->implementation_id);
			break;
		case PLAT_OTP_ID_CERT_REF:
			*size  = 19;
			break;
		case PLAT_OTP_ID_HW_VERSION:
			*size = sizeof(((struct otp_ps_data_t*)0)->hw_version);
			break;
		case PLAT_OTP_ID_VERIFICATION_SERVICE_URL:
			*size = sizeof(((struct otp_ps_data_t*)0)->verification_service_url);
			break;
		case PLAT_OTP_ID_PROFILE_DEFINITION:
			*size = sizeof(((struct otp_ps_data_t*)0)->profile_definition);
			break;
		case PLAT_OTP_ID_ENTROPY_SEED:
			*size = sizeof(((struct otp_ps_data_t*)0)->entropy_seed);
			break;
		case PLAT_OTP_ID_LCS:
			*size = sizeof(((struct otp_ps_data_t*)0)->lcs);
			break;
		default:
			OTP_LOGD("failed get size, unsupported id=%d\r\n", id);
			return TFM_PLAT_ERR_UNSUPPORTED;
	}

	OTP_LOGD("tfm_plat_otp_get_size id:%d len:%d\r\n", id , *size);

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