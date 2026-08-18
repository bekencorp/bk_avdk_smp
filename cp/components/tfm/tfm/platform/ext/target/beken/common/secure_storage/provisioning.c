/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "tfm_plat_otp.h"
#include "cmsis_compiler.h"
#include "flash_layout.h"
#include <string.h>
#include <stddef.h>
#include "otp.h"
#include "psa/crypto.h"

#include "_otp.h"
#include "bk_tfm_log.h"

int mbedtls_set_otp_info_ex(int type, const uint8_t *data, size_t len, uint32_t offset);

/* Default provisioning data */
static const struct otp_ps_data_t psa_rot_prov_data = {
	PSA_ROT_PROV_DATA_MAGIC,
	/* IAK */
	{
		0xA9, 0xB4, 0x54, 0xB2, 0x6D, 0x6F, 0x90, 0xA4,
		0xEA, 0x31, 0x19, 0x35, 0x64, 0xCB, 0xA9, 0x1F,
		0xEC, 0x6F, 0x9A, 0x00, 0x2A, 0x7D, 0xC0, 0x50,
		0x4B, 0x92, 0xA1, 0x93, 0x71, 0x34, 0x58, 0x5F
	},
	/* IAK len */
	32,
#ifdef SYMMETRIC_INITIAL_ATTESTATION
	/* IAK type */
	PSA_ALG_HMAC(PSA_ALG_SHA_256),
#else
	/* IAK type */
	0x12,//PSA_ECC_FAMILY_SECP_R1,
#endif /* SYMMETRIC_INITIAL_ATTESTATION */
	/* IAK id */
	"bk7236n.IAK.id",

	/* boot seed: overwrite by seed from TRNG */
	{0},
	/* implementation id: bk7236n */
	{
		0x42, 0x4b, 0x37, 0x32, 0x33, 0x36, 0x4e, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	},
	/* hw version: */
	"bk7236n.v1.0",
	/* verification_service_url */
	"www.bekencorp.com",
	/* attestation_profile_definition */
	"bk7236n_PSA_L3_PROFILE",
	/* Entropy seed: overwrite by seed from TRNG */
	{0},
	/* LCS */
	{PLAT_OTP_LCS_SECURED},
};

static bool provisioning_is_required(void)
{
    enum tfm_plat_err_t err;
    enum plat_otp_lcs_t lcs;

    err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(lcs), (uint8_t*)&lcs);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    return lcs == PLAT_OTP_LCS_ASSEMBLY_AND_TEST
        || lcs == PLAT_OTP_LCS_PSA_ROT_PROVISIONING;
}

static void otp_ps_dump(void)
{
	enum tfm_otp_element_id_t id;
	uint8_t buf[128] = {0};
	size_t len = 0;
	int err;

	OTP_LOGD("dump the provisioning otp:\r\n");
	for (id = PLAT_OTP_ID_HUK; id <= PLAT_OTP_ID_SECURE_DEBUG_PK; id++) {
		err = tfm_plat_otp_get_size(id, &len);
		if (err == TFM_PLAT_ERR_UNSUPPORTED) {
			continue;
		}

		OTP_LOGI("otp/ps id=%d, provisioning data:\r\n", id);
		if (err != TFM_PLAT_ERR_SUCCESS) {
			OTP_LOGE("failed to get size, id=%d\r\n", id);
			continue;
		}
		err = tfm_plat_otp_read(id, len, buf);
		if (err != TFM_PLAT_ERR_SUCCESS) {
			OTP_LOGE("failed to read otp, id=%d\r\n", id);
			continue;
		}

		for (int i = 0; i < len; i++) {
			OTP_LOG_RAW("%02x ", buf[i]);
			if (((i % 16) == 0) && i) {
				OTP_LOG_RAW("\r\n");
			}
		}
		OTP_LOG_RAW("\r\n\r\n");
	}
}

static enum tfm_plat_err_t provision_seeds(void)
{
	enum tfm_plat_err_t err;
	uint8_t seeds[64] = {0};
	int ret = TFM_PLAT_ERR_SYSTEM_ERR;

	OTP_LOGI("provision boot seed\r\n");
	ret = arm_ce_seed_read(seeds, sizeof(psa_rot_prov_data.boot_seed));
	if (ret != 0) {
		OTP_LOGE("boot seed TRNG read failed, ret=%d\r\n", ret);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_BOOT_SEED, sizeof(psa_rot_prov_data.boot_seed), seeds);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	OTP_LOGI("provision entropy seed\r\n");
	memset(seeds, 0, sizeof(seeds));
	ret = TFM_PLAT_ERR_SYSTEM_ERR; 
	ret = arm_ce_seed_read(seeds, sizeof(psa_rot_prov_data.entropy_seed));
	if (ret != 0) {
		OTP_LOGE("entropy seed TRNG read failed, ret=%d\r\n", ret);
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_ENTROPY_SEED, sizeof(psa_rot_prov_data.entropy_seed), seeds);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

static enum tfm_plat_err_t provision_psa_rot(void)
{
	enum tfm_plat_err_t err;
	uint32_t new_lcs;

	err = tfm_plat_otp_write(PLAT_OTP_ID_IAK, sizeof(psa_rot_prov_data.iak), psa_rot_prov_data.iak);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_IAK_LEN, sizeof(psa_rot_prov_data.iak_len), (uint8_t*)&psa_rot_prov_data.iak_len);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_IAK_TYPE, sizeof(psa_rot_prov_data.iak_type), (uint8_t*)&psa_rot_prov_data.iak_type);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

#ifdef ATTEST_INCLUDE_COSE_KEY_ID
	err = tfm_plat_otp_write(PLAT_OTP_ID_IAK_ID, sizeof(psa_rot_prov_data.iak_id), psa_rot_prov_data.iak_id);
	if (err != TFM_PLAT_ERR_SUCCESS && err != TFM_PLAT_ERR_UNSUPPORTED) {
		return err;
	}
#endif /* ATTEST_INCLUDE_COSE_KEY_ID */

	err = tfm_plat_otp_write(PLAT_OTP_ID_IMPLEMENTATION_ID, sizeof(psa_rot_prov_data.implementation_id), psa_rot_prov_data.implementation_id);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_HW_VERSION, sizeof(psa_rot_prov_data.hw_version), psa_rot_prov_data.hw_version);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_VERIFICATION_SERVICE_URL, sizeof(psa_rot_prov_data.verification_service_url), psa_rot_prov_data.verification_service_url);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}
	err = tfm_plat_otp_write(PLAT_OTP_ID_PROFILE_DEFINITION, sizeof(psa_rot_prov_data.profile_definition), psa_rot_prov_data.profile_definition);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = provision_seeds();
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	new_lcs = PLAT_OTP_LCS_SECURED;
	err = tfm_plat_otp_write(PLAT_OTP_ID_LCS, sizeof(new_lcs), (uint8_t*)&new_lcs);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	otp_ps_dump();
	return err;
}

enum tfm_plat_err_t otp_ps_provisioning(void)
{
	enum tfm_plat_err_t err;

	if (provisioning_is_required()) {
		err = provision_psa_rot();
		if (err != TFM_PLAT_ERR_SUCCESS) {
			return err;
		}
	}
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t otp_huk_provisioning(uint8_t *out, size_t out_len)
{
	uint8_t empty_huk[64] = {0};//huk length <= 64
	int ret;

	if (memcmp(out, empty_huk, out_len) != 0) {
		return TFM_PLAT_ERR_SUCCESS;
	} else {
		OTP_LOGI("HUK is empty, provisioning it\r\n");
		ret = arm_ce_seed_read(out, out_len);
		if (ret != 0) {
			OTP_LOGE("HUK TRNG read failed, ret=%d\r\n", ret);
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}
		ret = mbedtls_set_otp_info_ex(9, out, out_len, OTP_S_HUK_OFFSET);
		if (ret != 0) {
			OTP_LOGE("set huk to otp failed, ret=%d\r\n", ret);
			return TFM_PLAT_ERR_SYSTEM_ERR;
		}
		OTP_LOGI("HUK provisioning is ok\r\n");
	}

	return TFM_PLAT_ERR_SUCCESS;
}
