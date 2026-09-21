// Copyright 2024-2025 Beken
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

#include <string.h>
#include <os/mem.h>
#include "components/log.h"
#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#include "crypto_test.h"

#define TAG "mbedtls"
#define APP_SUCCESS (0)
#define APP_ERROR   (-1)

#define PSA_API_TEST_IKM_BITS        (256)
#define PSA_API_TEST_DERIVED_BITS    (128)
#define PSA_API_TEST_IV_LEN          (12)
#define PSA_API_TEST_TAG_LEN         (16)
#define PSA_API_TEST_PLAIN_LEN       (32)

static const uint8_t s_salt[] = "psa_api_test_salt";
static const uint8_t s_info[] = "psa_api_test_info";
static const uint8_t s_plain[PSA_API_TEST_PLAIN_LEN] = "psa_api_test_plaintext_block!!";

static psa_key_derivation_operation_t s_op;
static uint8_t s_iv[PSA_API_TEST_IV_LEN];
static uint8_t s_cipher[PSA_API_TEST_PLAIN_LEN + PSA_API_TEST_TAG_LEN];
static size_t s_cipher_len;

static uint32_t s_pass_cnt;
static uint32_t s_fail_cnt;
static uint32_t s_skip_cnt;

static int psa_api_test_record(const char *name, int ret)
{
	if (ret == APP_SUCCESS) {
		s_pass_cnt++;
		BK_LOGI(TAG, "PASS: %s\r\n", name);
	} else {
		s_fail_cnt++;
		BK_LOGE(TAG, "FAIL: %s\r\n", name);
	}

	return ret;
}

static void psa_api_test_skip(const char *name)
{
	s_skip_cnt++;
	BK_LOGW(TAG, "SKIP: %s\r\n", name);
}

static int psa_api_test_key_attr_setters(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_usage_t usage = PSA_KEY_USAGE_DERIVE;
	psa_algorithm_t alg = PSA_ALG_HKDF(PSA_ALG_SHA_256);
	psa_key_type_t type = PSA_KEY_TYPE_DERIVE;
	size_t bits = PSA_API_TEST_IKM_BITS;

	psa_set_key_usage_flags(&attr, usage);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_type(&attr, type);
	psa_set_key_bits(&attr, bits);

	if (psa_get_key_usage_flags(&attr) != usage ||
	    psa_get_key_algorithm(&attr) != alg ||
	    psa_get_key_type(&attr) != type ||
	    psa_get_key_bits(&attr) != bits) {
		BK_LOGE(TAG, "key attribute readback mismatch\r\n");
		psa_reset_key_attributes(&attr);
		return APP_ERROR;
	}

	psa_reset_key_attributes(&attr);
	return APP_SUCCESS;
}

static int psa_api_test_generate_random(void)
{
	uint8_t buf1[PSA_API_TEST_PLAIN_LEN];
	uint8_t buf2[PSA_API_TEST_PLAIN_LEN];
	psa_status_t status;

	status = psa_generate_random(buf1, sizeof(buf1));
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_generate_random status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	status = psa_generate_random(buf2, sizeof(buf2));
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_generate_random(2) status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	if (memcmp(buf1, buf2, sizeof(buf1)) == 0) {
		BK_LOGE(TAG, "psa_generate_random output not unique\r\n");
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

/* HKDF input secret only accepts PSA_KEY_TYPE_DERIVE keys. */
static int psa_api_test_create_ikm(psa_key_id_t *ikm_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attr, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&attr, PSA_API_TEST_IKM_BITS);

	status = psa_generate_key(&attr, ikm_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_generate_key(ikm) status=%d\r\n", (int)status);
		*ikm_id = 0;
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_derivation_setup(void)
{
	psa_status_t status;

	s_op = psa_key_derivation_operation_init();

	status = psa_key_derivation_setup(&s_op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_key_derivation_setup status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

/* HKDF requires the salt to be fed before the secret. */
static int psa_api_test_derivation_input_salt(void)
{
	psa_status_t status;

	status = psa_key_derivation_input_bytes(&s_op, PSA_KEY_DERIVATION_INPUT_SALT,
						s_salt, sizeof(s_salt) - 1);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_key_derivation_input_bytes(salt) status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_derivation_input_key(psa_key_id_t ikm_id)
{
	psa_status_t status;

	status = psa_key_derivation_input_key(&s_op, PSA_KEY_DERIVATION_INPUT_SECRET, ikm_id);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_key_derivation_input_key status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_derivation_input_info(void)
{
	psa_status_t status;

	status = psa_key_derivation_input_bytes(&s_op, PSA_KEY_DERIVATION_INPUT_INFO,
						s_info, sizeof(s_info) - 1);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_key_derivation_input_bytes(info) status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_derivation_output_key(psa_key_id_t *derived_id)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, PSA_API_TEST_DERIVED_BITS);

	status = psa_key_derivation_output_key(&attr, &s_op, derived_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_key_derivation_output_key status=%d\r\n", (int)status);
		*derived_id = 0;
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_derivation_abort(void)
{
	psa_status_t status;

	status = psa_key_derivation_abort(&s_op);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_key_derivation_abort status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_aead_encrypt(psa_key_id_t key_id)
{
	psa_status_t status;

	status = psa_generate_random(s_iv, sizeof(s_iv));
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_generate_random(iv) status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	status = psa_aead_encrypt(key_id, PSA_ALG_GCM, s_iv, sizeof(s_iv),
				  NULL, 0,
				  s_plain, sizeof(s_plain),
				  s_cipher, sizeof(s_cipher), &s_cipher_len);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_aead_encrypt status=%d\r\n", (int)status);
		s_cipher_len = 0;
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_aead_decrypt(psa_key_id_t key_id)
{
	uint8_t plain_out[PSA_API_TEST_PLAIN_LEN];
	size_t plain_len;
	psa_status_t status;

	status = psa_aead_decrypt(key_id, PSA_ALG_GCM, s_iv, sizeof(s_iv),
				  NULL, 0,
				  s_cipher, s_cipher_len,
				  plain_out, sizeof(plain_out), &plain_len);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_aead_decrypt status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	if (plain_len != sizeof(s_plain) ||
	    memcmp(plain_out, s_plain, sizeof(s_plain)) != 0) {
		BK_LOGE(TAG, "psa_aead_decrypt plaintext mismatch\r\n");
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

static int psa_api_test_destroy_key(psa_key_id_t key_id)
{
	psa_status_t status;

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_destroy_key(0x%x) status=%d\r\n", (unsigned)key_id, (int)status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int psa_api_test_main(void)
{
	psa_key_id_t ikm_id = 0;
	psa_key_id_t derived_id = 0;
	psa_status_t status;
	int derive_ready;

	s_pass_cnt = 0;
	s_fail_cnt = 0;
	s_skip_cnt = 0;
	s_cipher_len = 0;

	BK_LOGI(TAG, "psa api test start\r\n");

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "psa_crypto_init status=%d\r\n", (int)status);
		return APP_ERROR;
	}

	psa_api_test_record("psa_set_key_usage_flags/algorithm/type/bits",
			    psa_api_test_key_attr_setters());
	psa_api_test_record("psa_generate_random", psa_api_test_generate_random());
	psa_api_test_record("psa_generate_key(input key material)",
			    psa_api_test_create_ikm(&ikm_id));

	derive_ready = (psa_api_test_record("psa_key_derivation_setup",
					    psa_api_test_derivation_setup()) == APP_SUCCESS);

	if (derive_ready) {
		derive_ready = (psa_api_test_record("psa_key_derivation_input_bytes(salt)",
						    psa_api_test_derivation_input_salt()) == APP_SUCCESS);
	} else {
		psa_api_test_skip("psa_key_derivation_input_bytes(salt)");
	}

	if (derive_ready && ikm_id != 0) {
		derive_ready = (psa_api_test_record("psa_key_derivation_input_key",
						    psa_api_test_derivation_input_key(ikm_id)) == APP_SUCCESS);
	} else {
		derive_ready = 0;
		psa_api_test_skip("psa_key_derivation_input_key");
	}

	if (derive_ready) {
		derive_ready = (psa_api_test_record("psa_key_derivation_input_bytes(info)",
						    psa_api_test_derivation_input_info()) == APP_SUCCESS);
	} else {
		psa_api_test_skip("psa_key_derivation_input_bytes(info)");
	}

	if (derive_ready) {
		psa_api_test_record("psa_key_derivation_output_key",
				    psa_api_test_derivation_output_key(&derived_id));
	} else {
		psa_api_test_skip("psa_key_derivation_output_key");
	}

	psa_api_test_record("psa_key_derivation_abort", psa_api_test_derivation_abort());

	if (derived_id != 0) {
		psa_api_test_record("psa_aead_encrypt", psa_api_test_aead_encrypt(derived_id));
	} else {
		psa_api_test_skip("psa_aead_encrypt");
	}

	if (derived_id != 0 && s_cipher_len != 0) {
		psa_api_test_record("psa_aead_decrypt", psa_api_test_aead_decrypt(derived_id));
	} else {
		psa_api_test_skip("psa_aead_decrypt");
	}

	if (derived_id != 0) {
		psa_api_test_record("psa_destroy_key(derived key)",
				    psa_api_test_destroy_key(derived_id));
	} else {
		psa_api_test_skip("psa_destroy_key(derived key)");
	}

	if (ikm_id != 0) {
		psa_api_test_record("psa_destroy_key(input key material)",
				    psa_api_test_destroy_key(ikm_id));
	} else {
		psa_api_test_skip("psa_destroy_key(input key material)");
	}

	BK_LOGI(TAG, "psa api test summary: total=%u pass=%u fail=%u skip=%u\r\n",
		(unsigned)(s_pass_cnt + s_fail_cnt + s_skip_cnt),
		(unsigned)s_pass_cnt, (unsigned)s_fail_cnt, (unsigned)s_skip_cnt);

	return (s_fail_cnt == 0 && s_skip_cnt == 0) ? APP_SUCCESS : APP_ERROR;
}
